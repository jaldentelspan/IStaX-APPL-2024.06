/* dispatch.c

   Network input dispatcher... */

/*
 * Copyright (c) 2004-2008 by Internet Systems Consortium, Inc. ("ISC")
 * Copyright (c) 1995-2003 by Internet Software Consortium
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *   Internet Systems Consortium, Inc.
 *   950 Charter Street
 *   Redwood City, CA 94063
 *   <info@isc.org>
 *   http://www.isc.org/
 *
 * This software has been written for Internet Systems Consortium
 * by Ted Lemon in cooperation with Vixie Enterprises and Nominum, Inc.
 * To learn more about Internet Systems Consortium, see
 * ``http://www.isc.org/''.  To learn more about Vixie Enterprises,
 * see ``http://www.vix.com''.   To learn more about Nominum, Inc., see
 * ``http://www.nominum.com''.
 */

#include "dhcpd.h"

struct timeout *timeouts;
static struct timeout *free_timeouts;

void set_time(TIME t)
{
	/* Do any outstanding timeouts. */
	if (cur_tv . tv_sec != t) {
		cur_tv . tv_sec = t;
		cur_tv . tv_usec = 0;
		process_outstanding_timeouts ((struct timeval *)0);
	}
}

struct timeval *process_outstanding_timeouts (struct timeval *tvp)
{
	/* Call any expired timeouts, and then if there's
	   still a timeout registered, time out the select
	   call then. */
      another:
	if (timeouts) {
		struct timeout *t;
		if ((timeouts -> when . tv_sec < cur_tv . tv_sec) ||
		    ((timeouts -> when . tv_sec == cur_tv . tv_sec) &&
		     (timeouts -> when . tv_usec <= cur_tv . tv_usec))) {
			t = timeouts;
			timeouts = timeouts -> next;
			(*(t -> func)) (t -> what);
			if (t -> unref)
				(*t -> unref) (&t -> what, MDL);
			t -> next = free_timeouts;
			free_timeouts = t;
			goto another;
		}
		if (tvp) {
			tvp -> tv_sec = timeouts -> when . tv_sec;
			tvp -> tv_usec = timeouts -> when . tv_usec;
		}
		return tvp;
	} else
		return (struct timeval *)0;
}

/* Wait for packets to come in using select().   When one does, call
   receive_packet to receive the packet and possibly strip hardware
   addressing information from it, and then call through the
   bootp_packet_handler hook to try to do something with it. */

void dispatch ()
{
	struct timeval tv, *tvp;
	isc_result_t status;

	/* Wait for a packet or a timeout... XXX */
	do {
		tvp = process_outstanding_timeouts (&tv);
		status = omapi_one_dispatch (0, tvp);
	} while (status == ISC_R_TIMEDOUT || status == ISC_R_SUCCESS);
	log_fatal ("omapi_one_dispatch failed: %s -- exiting.",
		   isc_result_totext (status));
}

void add_timeout (struct timeval *when, void (*where) PROTO ((void *)), void *what, tvref_t ref, tvunref_t unref)
{
	struct timeout *t, *q;

	/* See if this timeout supersedes an existing timeout. */
	t = (struct timeout *)0;
	for (q = timeouts; q; q = q -> next) {
		if ((where == NULL || q -> func == where) &&
		    q -> what == what) {
			if (t)
				t -> next = q -> next;
			else
				timeouts = q -> next;
			break;
		}
		t = q;
	}

	/* If we didn't supersede a timeout, allocate a timeout
	   structure now. */
	if (!q) {
		if (free_timeouts) {
			q = free_timeouts;
			free_timeouts = q -> next;
		} else {
			q = ((struct timeout *)
			     dmalloc (sizeof (struct timeout), MDL));
			if (!q)
				log_fatal ("add_timeout: no memory!");
		}
		memset (q, 0, sizeof *q);
		q -> func = where;
		q -> ref = ref;
		q -> unref = unref;
		if (q -> ref)
			(*q -> ref)(&q -> what, what, MDL);
		else
			q -> what = what;
	}

	q -> when . tv_sec = when -> tv_sec;
	q -> when . tv_usec = when -> tv_usec;

	/* Now sort this timeout into the timeout list. */

	/* Beginning of list? */
	if (!timeouts || (timeouts -> when . tv_sec > q -> when . tv_sec) ||
	    ((timeouts -> when . tv_sec == q -> when . tv_sec) &&
	     (timeouts -> when . tv_usec > q -> when . tv_usec))) {
		q -> next = timeouts;
		timeouts = q;
		return;
	}

	/* Middle of list? */
	for (t = timeouts; t -> next; t = t -> next) {
		if ((t -> next -> when . tv_sec > q -> when . tv_sec) ||
		    ((t -> next -> when . tv_sec == q -> when . tv_sec) &&
		     (t -> next -> when . tv_usec > q -> when . tv_usec))) {
			q -> next = t -> next;
			t -> next = q;
			return;
		}
	}

	/* End of list. */
	t -> next = q;
	q -> next = (struct timeout *)0;
}

void cancel_timeout (void (*where) PROTO ((void *)), void *what)
{
	struct timeout *t, *q;

	/* Look for this timeout on the list, and unlink it if we find it. */
	t = (struct timeout *)0;
	for (q = timeouts; q; q = q -> next) {
		if (q -> func == where && q -> what == what) {
			if (t)
				t -> next = q -> next;
			else
				timeouts = q -> next;
			break;
		}
		t = q;
	}

	/* If we found the timeout, put it on the free list. */
	if (q) {
		if (q -> unref)
			(*q -> unref) (&q -> what, MDL);
		q -> next = free_timeouts;
		free_timeouts = q;
	}
}

#if defined (DEBUG_MEMORY_LEAKAGE_ON_EXIT)
void cancel_all_timeouts ()
{
	struct timeout *t, *n;
	for (t = timeouts; t; t = n) {
		n = t -> next;
		if (t -> unref && t -> what)
			(*t -> unref) (&t -> what, MDL);
		t -> next = free_timeouts;
		free_timeouts = t;
	}
}

void relinquish_timeouts ()
{
	struct timeout *t, *n;
	for (t = free_timeouts; t; t = n) {
		n = t -> next;
		dfree (t, MDL);
	}
}
#endif
