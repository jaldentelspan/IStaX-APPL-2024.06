/* socket.c

   BSD socket interface code... */

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

/* SO_BINDTODEVICE support added by Elliot Poger (poger@leland.stanford.edu).
 * This sockopt allows a socket to be bound to a particular interface,
 * thus enabling the use of DHCPD on a multihomed host.
 * If SO_BINDTODEVICE is defined in your system header files, the use of
 * this sockopt will be automatically enabled. 
 * I have implemented it under Linux; other systems should be doable also.
 */

#include "main.h"
#include "dhcpd.h"
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/uio.h>

#ifdef USE_SOCKET_FALLBACK
# if !defined (USE_SOCKET_SEND)
#  define if_register_send if_register_fallback
#  define send_packet send_fallback
#  define if_reinitialize_send if_reinitialize_fallback
# endif
#endif

#if defined(DHCPv6)
/*
 * XXX: this is gross.  we need to go back and overhaul the API for socket
 * handling.
 */
static unsigned int global_v6_socket_references = 0;
static int global_v6_socket = -1;

static void if_register_multicast(struct interface_info *info);
#endif

/*
 * If we can't bind() to a specific interface, then we can only have
 * a single socket. This variable insures that we don't try to listen
 * on two sockets.
 */
#if !defined(SO_BINDTODEVICE) && !defined(USE_FALLBACK)
static int once = 0;
#endif /* !defined(SO_BINDTODEVICE) && !defined(USE_FALLBACK) */

/* Reinitializes the specified interface after an address change.   This
   is not required for packet-filter APIs. */

#if defined (USE_SOCKET_SEND) || defined (USE_SOCKET_FALLBACK)
void if_reinitialize_send (struct interface_info *info)
{
#if 0
#ifndef USE_SOCKET_RECEIVE
	once = 0;
	close (info -> wfdesc);
#endif
	if_register_send (info);
#endif
}
#endif

#ifdef USE_SOCKET_RECEIVE
void if_reinitialize_receive (struct interface_info *info)
{
#if 0
	once = 0;
	close (info -> rfdesc);
	if_register_receive (info);
#endif
}
#endif

#if defined (USE_SOCKET_SEND) || \
	defined (USE_SOCKET_RECEIVE) || \
		defined (USE_SOCKET_FALLBACK)
/* Generic interface registration routine... */
int
if_register_socket(struct interface_info *info, int family,
		   int *do_multicast)
{
	struct sockaddr_storage name;
	int name_len;
	int sock;
	int flag;
	int domain;

	/* INSIST((family == AF_INET) || (family == AF_INET6)); */

#if !defined(SO_BINDTODEVICE) && !defined(USE_FALLBACK)
	/* Make sure only one interface is registered. */
	if (once) {
		log_fatal ("The standard socket API can only support %s",
		       "hosts with a single network interface.");
	}
	once = 1;
#endif

	/* 
	 * Set up the address we're going to bind to, depending on the
	 * address family. 
	 */ 
	memset(&name, 0, sizeof(name));
#ifdef DHCPv6
	if (family == AF_INET6) {
		struct sockaddr_in6 *addr = (struct sockaddr_in6 *)&name; 
		addr->sin6_family = AF_INET6;
		addr->sin6_port = local_port;
		/* XXX: What will happen to multicasts if this is nonzero? */
		memcpy(&addr->sin6_addr,
		       &local_address6, 
		       sizeof(addr->sin6_addr));
#ifdef HAVE_SA_LEN
		addr->sin6_len = sizeof(*addr);
#endif
		name_len = sizeof(*addr);
		domain = PF_INET6;
		if ((info->flags & INTERFACE_STREAMS) == INTERFACE_UPSTREAM) {
			*do_multicast = 0;
		}
	} else { 
#else 
	{
#endif /* DHCPv6 */
		struct sockaddr_in *addr = (struct sockaddr_in *)&name; 
		addr->sin_family = AF_INET;
		addr->sin_port = local_port;
		memcpy(&addr->sin_addr,
		       &local_address,
		       sizeof(addr->sin_addr));
#ifdef HAVE_SA_LEN
		addr->sin_len = sizeof(*addr);
#endif
		name_len = sizeof(*addr);
		domain = PF_INET;
	}

	/* Make a socket... */
	sock = vtss_socket(domain, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		log_fatal("Can't create dhcp socket: %m");
	}

	/* Set the REUSEADDR option so that we don't fail to start if
	   we're being restarted. */
	flag = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
			(char *)&flag, sizeof(flag)) < 0) {
		log_fatal("Can't set SO_REUSEADDR option on dhcp socket: %m");
	}

	/* Set the BROADCAST option so that we can broadcast DHCP responses.
	   We shouldn't do this for fallback devices, and we can detect that
	   a device is a fallback because it has no ifp structure. */
	if (info->ifp &&
	    (setsockopt(sock, SOL_SOCKET, SO_BROADCAST,
			 (char *)&flag, sizeof(flag)) < 0)) {
		log_fatal("Can't set SO_BROADCAST option on dhcp socket: %m");
	}

#if defined(DHCPv6) && defined(SO_REUSEPORT)
	/*
	 * We only set SO_REUSEPORT on AF_INET6 sockets, so that multiple
	 * daemons can bind to their own sockets and get data for their
	 * respective interfaces.  This does not (and should not) affect
	 * DHCPv4 sockets; we can't yet support BSD sockets well, much
	 * less multiple sockets.
	 */
	if (local_family == AF_INET6) {
		flag = 1;
		if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT,
			       (char *)&flag, sizeof(flag)) < 0) {
			log_fatal("Can't set SO_REUSEPORT option on dhcp "
				  "socket: %m");
		}
	}
#endif

	/* Bind the socket to this interface's IP address. */
#ifdef ISCDHCP_ISTAX_PLATFORM
        while (bind(sock, (struct sockaddr *)&name, name_len) < 0) {
            VTSS_OS_MSLEEP(1000);
            printf("E dhcp_relay %s#%d: error in bind()\n", __FUNCTION__, __LINE__);
        }
#else
	if (bind(sock, (struct sockaddr *)&name, name_len) < 0) {
		log_error("Can't bind to dhcp address: %m");
		log_error("Please make sure there is no other dhcp server");
		log_error("running and that there's no entry for dhcp or");
		log_error("bootp in /etc/inetd.conf.   Also make sure you");
		log_error("are not running HP JetAdmin software, which");
		log_fatal("includes a bootp server.");
	}
#endif /* ISCDHCP_ISTAX_PLATFORM */

#if defined(SO_BINDTODEVICE)
#endif

	/* IP_BROADCAST_IF instructs the kernel which interface to send
	 * IP packets whose destination address is 255.255.255.255.  These
	 * will be treated as subnet broadcasts on the interface identified
	 * by ip address (info -> primary_address).  This is only known to
	 * be defined in SCO system headers, and may not be defined in all
	 * releases.
	 */
#if defined(SCO) && defined(IP_BROADCAST_IF)
        if (info->address_count &&
	    setsockopt(sock, IPPROTO_IP, IP_BROADCAST_IF, &info->addresses[0],
		       sizeof(info->addresses[0])) < 0)
		log_fatal("Can't set IP_BROADCAST_IF on dhcp socket: %m");
#endif

#ifdef DHCPv6
	/*
	 * If we turn on IPV6_PKTINFO, we will be able to receive 
	 * additional information, such as the destination IP address.
	 * We need this to spot unicast packets.
	 */
	if (family == AF_INET6) {
		int on = 1;
#ifdef IPV6_RECVPKTINFO
		/* RFC3542 */
		if (setsockopt(sock, IPPROTO_IPV6, IPV6_RECVPKTINFO, 
		               &on, sizeof(on)) != 0) {
			log_fatal("setsockopt: IPV6_RECVPKTINFO: %m");
		}
#else
		/* RFC2292 */
		if (setsockopt(sock, IPPROTO_IPV6, IPV6_PKTINFO, 
		               &on, sizeof(on)) != 0) {
			log_fatal("setsockopt: IPV6_PKTINFO: %m");
		}
#endif
	}

	if ((family == AF_INET6) &&
	    ((info->flags & INTERFACE_UPSTREAM) != 0)) {
		int hop_limit = 32;
		if (setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS,
			       &hop_limit, sizeof(int)) < 0) {
			log_fatal("setsockopt: IPV6_MULTICAST_HOPS: %m");
		}
	}
#endif /* DHCPv6 */

#ifdef HAVE_GET_HW_ADDR
	/* If this is a normal IPv4 address, get the hardware address. */
	if ((local_family == AF_INET) && (strcmp(info->name, "fallback") != 0))
		get_hw_addr(info->name, &info->hw_address);
#endif 

	return sock;
}
#endif /* USE_SOCKET_SEND || USE_SOCKET_RECEIVE || USE_SOCKET_FALLBACK */

#if defined (USE_SOCKET_SEND) || defined (USE_SOCKET_FALLBACK)
void if_register_send (struct interface_info *info)
{
#ifndef USE_SOCKET_RECEIVE
	info -> wfdesc = if_register_socket (info, AF_INET, 0);
#if defined (USE_SOCKET_FALLBACK)
	/* Fallback only registers for send, but may need to receive as
	   well. */
	info -> rfdesc = info -> wfdesc;
#endif
#else
	info -> wfdesc = info -> rfdesc;
#endif
	if (!quiet_interface_discovery)
		log_info ("Sending on   Socket/%s%s%s",
		      info -> name,
		      (info -> shared_network ? "/" : ""),
		      (info -> shared_network ?
		       info -> shared_network -> name : ""));
}

#if defined (USE_SOCKET_SEND)
void if_deregister_send (struct interface_info *info)
{
#ifndef USE_SOCKET_RECEIVE
	close (info -> wfdesc);
#endif
	info -> wfdesc = -1;

	if (!quiet_interface_discovery)
		log_info ("Disabling output on Socket/%s%s%s",
		      info -> name,
		      (info -> shared_network ? "/" : ""),
		      (info -> shared_network ?
		       info -> shared_network -> name : ""));
}
#endif /* USE_SOCKET_SEND */
#endif /* USE_SOCKET_SEND || USE_SOCKET_FALLBACK */

#ifdef USE_SOCKET_RECEIVE
void if_register_receive (struct interface_info *info)
{
	/* If we're using the socket API for sending and receiving,
	   we don't need to register this interface twice. */
	info -> rfdesc = if_register_socket (info, AF_INET, 0);
	if (!quiet_interface_discovery)
		log_info ("Listening on Socket/%s%s%s",
		      info -> name,
		      (info -> shared_network ? "/" : ""),
		      (info -> shared_network ?
		       info -> shared_network -> name : ""));
}

void if_deregister_receive (struct interface_info *info)
{
	close (info -> rfdesc);
	info -> rfdesc = -1;

	if (!quiet_interface_discovery)
		log_info ("Disabling input on Socket/%s%s%s",
		      info -> name,
		      (info -> shared_network ? "/" : ""),
		      (info -> shared_network ?
		       info -> shared_network -> name : ""));
}
#endif /* USE_SOCKET_RECEIVE */


#ifdef DHCPv6 
/*
 * This function joins the interface to DHCPv6 multicast groups so we will
 * receive multicast messages.
 */
static void
if_register_multicast(struct interface_info *info) {
	int sock = info->rfdesc;
	struct ipv6_mreq mreq;

	if (inet_pton(AF_INET6, All_DHCP_Relay_Agents_and_Servers,
		      &mreq.ipv6mr_multiaddr) <= 0) {
		log_fatal("inet_pton: unable to convert '%s'", 
			  All_DHCP_Relay_Agents_and_Servers);
	}
	mreq.ipv6mr_interface = if_nametoindex(info->name);
	if (setsockopt(sock, IPPROTO_IPV6, IPV6_JOIN_GROUP, 
		       &mreq, sizeof(mreq)) < 0) {
		log_fatal("setsockopt: IPV6_JOIN_GROUP: %m");
	}

	/*
	 * The relay agent code sets the streams so you know which way
	 * is up and down.  But a relay agent shouldn't join to the
	 * Server address, or else you get fun loops.  So up or down
	 * doesn't matter, we're just using that config to sense this is
	 * a relay agent.
	 */
	if ((info->flags & INTERFACE_STREAMS) == 0) {
		if (inet_pton(AF_INET6, All_DHCP_Servers,
			      &mreq.ipv6mr_multiaddr) <= 0) {
			log_fatal("inet_pton: unable to convert '%s'", 
				  All_DHCP_Servers);
		}
		mreq.ipv6mr_interface = if_nametoindex(info->name);
		if (setsockopt(sock, IPPROTO_IPV6, IPV6_JOIN_GROUP, 
			       &mreq, sizeof(mreq)) < 0) {
			log_fatal("setsockopt: IPV6_JOIN_GROUP: %m");
		}
	}
}

void
if_register6(struct interface_info *info, int do_multicast) {
	/* Bounce do_multicast to a stack variable because we may change it. */
	int req_multi = do_multicast;

	if (global_v6_socket_references == 0) {
		global_v6_socket = if_register_socket(info, AF_INET6,
						      &req_multi);
		if (global_v6_socket < 0) {
			/*
			 * if_register_socket() fatally logs if it fails to
			 * create a socket, this is just a sanity check.
			 */
			log_fatal("Impossible condition at %s:%d", MDL);
		} else {
			log_info("Bound to *:%d", ntohs(local_port));
		}
	}
		
	info->rfdesc = global_v6_socket;
	info->wfdesc = global_v6_socket;
	global_v6_socket_references++;

	if (req_multi)
		if_register_multicast(info);

	get_hw_addr(info->name, &info->hw_address);

	if (!quiet_interface_discovery) {
		if (info->shared_network != NULL) {
			log_info("Listening on Socket/%d/%s/%s",
				 global_v6_socket, info->name, 
				 info->shared_network->name);
			log_info("Sending on   Socket/%d/%s/%s",
				 global_v6_socket, info->name,
				 info->shared_network->name);
		} else {
			log_info("Listening on Socket/%s", info->name);
			log_info("Sending on   Socket/%s", info->name);
		}
	}
}

void 
if_deregister6(struct interface_info *info) {
	/* Dereference the global v6 socket. */
	if ((info->rfdesc == global_v6_socket) &&
	    (info->wfdesc == global_v6_socket) &&
	    (global_v6_socket_references > 0)) {
		global_v6_socket_references--;
		info->rfdesc = -1;
		info->wfdesc = -1;
	} else {
		log_fatal("Impossible condition at %s:%d", MDL);
	}

	if (!quiet_interface_discovery) {
		if (info->shared_network != NULL) {
			log_info("Disabling input on  Socket/%s/%s", info->name,
		       		 info->shared_network->name);
			log_info("Disabling output on Socket/%s/%s", info->name,
		       		 info->shared_network->name);
		} else {
			log_info("Disabling input on  Socket/%s", info->name);
			log_info("Disabling output on Socket/%s", info->name);
		}
	}

	if (global_v6_socket_references == 0) {
		close(global_v6_socket);
		global_v6_socket = -1;

		log_info("Unbound from *:%d", ntohs(local_port));
	}
}
#endif /* DHCPv6 */

#if defined (USE_SOCKET_SEND) || defined (USE_SOCKET_FALLBACK)
ssize_t send_packet (struct interface_info *interface, struct packet *packet,
                     struct dhcp_packet *raw, size_t len, struct in_addr from,
                     struct sockaddr_in *to, struct hardware *hto)
{
	int result;
#ifdef IGNORE_HOSTUNREACH
	int retry = 0;
	do {
#endif
		result = sendto (interface -> wfdesc, (char *)raw, len, 0,
				 (struct sockaddr *)to, sizeof *to);
#ifdef IGNORE_HOSTUNREACH
	} while (to -> sin_addr.s_addr == htonl (INADDR_BROADCAST) &&
		 result < 0 &&
		 (errno == EHOSTUNREACH ||
		  errno == ECONNREFUSED) &&
		 retry++ < 10);
#endif
	if (result < 0) {
		log_error ("send_packet: %m");
		if (errno == ENETUNREACH)
			log_error ("send_packet: please consult README file%s",
				   " regarding broadcast address.");
	}
	return result;
}

#endif /* USE_SOCKET_SEND || USE_SOCKET_FALLBACK */

#ifdef DHCPv6
/*
 * Solaris 9 is missing the CMSG_LEN and CMSG_SPACE macros, so we will 
 * synthesize them (based on the BIND 9 technique).
 */

#ifndef CMSG_LEN
static size_t CMSG_LEN(size_t len) {
	size_t hdrlen;
	/*
	 * Cast NULL so that any pointer arithmetic performed by CMSG_DATA
	 * is correct.
	 */
	hdrlen = (size_t)CMSG_DATA(((struct cmsghdr *)NULL));
	return hdrlen + len;
}
#endif /* !CMSG_LEN */

#ifndef CMSG_SPACE
static size_t CMSG_SPACE(size_t len) {
	struct msghdr msg;
	struct cmsghdr *cmsgp;

	/*
	 * XXX: The buffer length is an ad-hoc value, but should be enough
	 * in a practical sense.
	 */
	union {
		struct cmsghdr cmsg_sizer;
		u_int8_t pktinfo_sizer[sizeof(struct cmsghdr) + 1024];
	} dummybuf;

	memset(&msg, 0, sizeof(msg));
	msg.msg_control = &dummybuf;
	msg.msg_controllen = sizeof(dummybuf);

	cmsgp = (struct cmsghdr *)&dummybuf;
	cmsgp->cmsg_len = CMSG_LEN(len);

	cmsgp = CMSG_NXTHDR(&msg, cmsgp);
	if (cmsgp != NULL) {
		return (char *)cmsgp - (char *)msg.msg_control;
	} else {
		return 0;
	}
}
#endif /* !CMSG_SPACE */

#endif /* DHCPv6 */

#if defined(DHCPv6) || defined(VTSS_SW_OPTION_DHCP6_RELAY)
/* 
 * For both send_packet6() and receive_packet6() we need to use the 
 * sendmsg()/recvmsg() functions rather than the simpler send()/recv()
 * functions.
 *
 * In the case of send_packet6(), we need to do this in order to insure
 * that the reply packet leaves on the same interface that it arrived 
 * on. 
 *
 * In the case of receive_packet6(), we need to do this in order to 
 * get the IP address the packet was sent to. This is used to identify
 * whether a packet is multicast or unicast.
 *
 * Helpful man pages: recvmsg, readv (talks about the iovec stuff), cmsg.
 *
 * Also see the sections in RFC 3542 about IPV6_PKTINFO.
 */

/* Send an IPv6 packet */
#ifdef ISCDHCP_ISTAX_PLATFORM
ssize_t send_packet6(unsigned int out_if_idx, int socket_no,
		     const unsigned char *raw, size_t len,
		     struct sockaddr_in6 *to) {
#else
ssize_t send_packet6(struct interface_info *interface,
		     const unsigned char *raw, size_t len,
		     struct sockaddr_in6 *to) {
#endif
	struct msghdr m;
	struct iovec v;
	int result;
	struct in6_pktinfo *pktinfo;
	struct cmsghdr *cmsg;
	union {
		struct cmsghdr cmsg_sizer;
		u_int8_t pktinfo_sizer[CMSG_SPACE(sizeof(struct in6_pktinfo))];
	} control_buf;

	/*
	 * Initialize our message header structure.
	 */
	memset(&m, 0, sizeof(m));

	/*
	 * Set the target address we're sending to.
	 */
	m.msg_name = to;
	m.msg_namelen = sizeof(*to);

	/*
	 * Set the data buffer we're sending. (Using this wacky 
	 * "scatter-gather" stuff... we only have a single chunk 
	 * of data to send, so we declare a single vector entry.)
	 */
	v.iov_base = (char *)raw;
	v.iov_len = len;
	m.msg_iov = &v;
	m.msg_iovlen = 1;

	/*
	 * Setting the interface is a bit more involved.
	 * 
	 * We have to create a "control message", and set that to 
	 * define the IPv6 packet information. We could set the
	 * source address if we wanted, but we can safely let the
	 * kernel decide what that should be. 
	 */
	m.msg_control = &control_buf;
	m.msg_controllen = sizeof(control_buf);
	cmsg = CMSG_FIRSTHDR(&m);
	cmsg->cmsg_level = IPPROTO_IPV6;
	cmsg->cmsg_type = IPV6_PKTINFO;
	cmsg->cmsg_len = CMSG_LEN(sizeof(*pktinfo));
	pktinfo = (struct in6_pktinfo *)CMSG_DATA(cmsg);
	memset(pktinfo, 0, sizeof(*pktinfo));
#ifdef ISCDHCP_ISTAX_PLATFORM
	pktinfo->ipi6_ifindex = out_if_idx;
#else
	pktinfo->ipi6_ifindex = if_nametoindex(interface->name);
#endif
	m.msg_controllen = cmsg->cmsg_len;

#ifdef ISCDHCP_ISTAX_PLATFORM
	result = sendmsg(socket_no, &m, 0);
#else
	result = sendmsg(interface->wfdesc, &m, 0);
	if (result < 0) {
		log_error("send_packet6: %m");
	}
#endif
	return result;
}
#endif /* DHCPv6 || VTSS_SW_OPTION_DHCP6_RELAY */

#ifdef USE_SOCKET_RECEIVE
ssize_t receive_packet (struct interface_info *interface, unsigned char *buf, size_t len,
                        struct sockaddr_in *from, struct hardware *hfrom)
{
	SOCKLEN_T flen = sizeof *from;
	int result;

	/*
	 * The normal Berkeley socket interface doesn't give us any way
	 * to know what hardware interface we received the message on,
	 * but we should at least make sure the structure is emptied.
	 */
	memset(hfrom, 0, sizeof(*hfrom));
	memset(from, 0, sizeof(*from));

#ifdef IGNORE_HOSTUNREACH
	int retry = 0;
	do {
#endif
		result = recvfrom (interface -> rfdesc, (char *)buf, len, 0,
				   (struct sockaddr *)from, &flen);
#ifdef IGNORE_HOSTUNREACH
	} while (result < 0 &&
		 (errno == EHOSTUNREACH ||
		  errno == ECONNREFUSED) &&
		 retry++ < 10);
#endif
	return result;
}

#endif /* USE_SOCKET_RECEIVE */

#ifdef DHCPv6
ssize_t 
receive_packet6(struct interface_info *interface, 
		unsigned char *buf, size_t len, 
		struct sockaddr_in6 *from, struct in6_addr *to_addr,
		unsigned int *if_idx)
{
	struct msghdr m;
	struct iovec v;
	int result;
	struct cmsghdr *cmsg;
	struct in6_pktinfo *pktinfo;
	int found_pktinfo;
	union {
	        struct cmsghdr cmsg_sizer;
	        u_int8_t pktinfo_sizer[CMSG_SPACE(sizeof(struct in6_pktinfo))];
	} control_buf;

	/*
	 * Initialize our message header structure.
	 */
	memset(&m, 0, sizeof(m));

	/*
	 * Point so we can get the from address.
	 */
	m.msg_name = from;
	m.msg_namelen = sizeof(*from);

	/*
	 * Set the data buffer we're receiving. (Using this wacky 
	 * "scatter-gather" stuff... but we that doesn't really make
	 * sense for us, so we use a single vector entry.)
	 */
	v.iov_base = buf;
	v.iov_len = len;
	m.msg_iov = &v;
	m.msg_iovlen = 1;

	/*
	 * Getting the interface is a bit more involved.
	 *
	 * We set up some space for a "control message". We have 
	 * previously asked the kernel to give us packet 
	 * information (when we initialized the interface), so we
	 * should get the destination address from that.
	 */
	m.msg_control = &control_buf;
	m.msg_controllen = sizeof(control_buf);

	result = recvmsg(interface->rfdesc, &m, 0);

	if (result >= 0) {
		/*
		 * If we did read successfully, then we need to loop
		 * through the control messages we received and 
		 * find the one with our destination address.
		 *
		 * We also keep a flag to see if we found it. If we 
		 * didn't, then we consider this to be an error.
		 */
		found_pktinfo = 0;
		cmsg = CMSG_FIRSTHDR(&m);
		while (cmsg != NULL) {
			if ((cmsg->cmsg_level == IPPROTO_IPV6) && 
			    (cmsg->cmsg_type == IPV6_PKTINFO)) {
				pktinfo = (struct in6_pktinfo *)CMSG_DATA(cmsg);
				*to_addr = pktinfo->ipi6_addr;
				*if_idx = pktinfo->ipi6_ifindex;
				found_pktinfo = 1;
			}
			cmsg = CMSG_NXTHDR(&m, cmsg);
		}
		if (!found_pktinfo) {
			result = -1;
			errno = EIO;
		}
	}

	return result;
}
#endif /* DHCPv6 */

#if defined (USE_SOCKET_FALLBACK)
/* This just reads in a packet and silently discards it. */

isc_result_t fallback_discard (omapi_object_t *object)
{
	char buf [1540];
	struct sockaddr_in from;
	SOCKLEN_T flen = sizeof from;
	int status;
	struct interface_info *interface;

	if (object -> type != dhcp_type_interface)
		return ISC_R_INVALIDARG;
	interface = (struct interface_info *)object;

	status = recvfrom (interface -> wfdesc, buf, sizeof buf, 0,
			   (struct sockaddr *)&from, &flen);
#if defined (DEBUG)
	/* Only report fallback discard errors if we're debugging. */
	if (status < 0) {
		log_error ("fallback_discard: %m");
		return ISC_R_UNEXPECTED;
	}
#endif
	return ISC_R_SUCCESS;
}
#endif /* USE_SOCKET_FALLBACK */

#if defined (USE_SOCKET_SEND)
int can_unicast_without_arp (struct interface_info *ip)
{
	return 0;
}

int can_receive_unicast_unconfigured (struct interface_info *ip)
{
#if defined (SOCKET_CAN_RECEIVE_UNICAST_UNCONFIGURED)
	return 1;
#else
	return 0;
#endif
}

int supports_multiple_interfaces (struct interface_info *ip)
{
#if defined (SO_BINDTODEVICE)
	return 1;
#else
	return 0;
#endif
}

/* If we have SO_BINDTODEVICE, set up a fallback interface; otherwise,
   do not. */

void maybe_setup_fallback ()
{
#if defined (USE_SOCKET_FALLBACK)
	isc_result_t status;
	struct interface_info *fbi = (struct interface_info *)0;
	if (setup_fallback (&fbi, MDL)) {
		fbi -> wfdesc = if_register_socket (fbi, AF_INET, 0);
		fbi -> rfdesc = fbi -> wfdesc;
		log_info ("Sending on   Socket/%s%s%s",
		      fbi -> name,
		      (fbi -> shared_network ? "/" : ""),
		      (fbi -> shared_network ?
		       fbi -> shared_network -> name : ""));
	
		status = omapi_register_io_object ((omapi_object_t *)fbi,
						   if_readsocket, 0,
						   fallback_discard, 0, 0);
		if (status != ISC_R_SUCCESS)
			log_fatal ("Can't register I/O handle for %s: %s",
				   fbi -> name, isc_result_totext (status));
		interface_dereference (&fbi, MDL);
	}
#endif
}
#endif /* USE_SOCKET_SEND */
