# Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.
#
# Unpublished rights reserved under the copyright laws of the United States of
# America, other countries and international treaties. Permission to use, copy,
# store and modify, the software and its source code is granted but only in
# connection with products utilizing the Microsemi switch and PHY products.
# Permission is also granted for you to integrate into other products, disclose,
# transmit and distribute the software only in an absolute machine readable
# format (e.g. HEX file) and only in or with products utilizing the Microsemi
# switch and PHY products.  The source code of the software may not be
# disclosed, transmitted or distributed without the prior written permission of
# Microsemi.
#
# This copyright notice must appear in any copy, modification, disclosure,
# transmission or distribution of the software.  Microsemi retains all
# ownership, copyright, trade secret and proprietary rights in the software and
# its source code, including all modifications thereto.
#
# THIS SOFTWARE HAS BEEN PROVIDED "AS IS". MICROSEMI HEREBY DISCLAIMS ALL
# WARRANTIES OF ANY KIND WITH RESPECT TO THE SOFTWARE, WHETHER SUCH WARRANTIES
# ARE EXPRESS, IMPLIED, STATUTORY OR OTHERWISE INCLUDING, WITHOUT LIMITATION,
# WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR USE OR PURPOSE AND
# NON-INFRINGEMENT.

# Makefile for LACP protocol system
# Only used when building the standalone simulator

OBJECTS = vtss_lacp.o vtss_os_linux.o
PROGRAMS = lacp
RM = rm -f
CC = gcc
ifdef NODEBUG
CFLAGS = -O2 -Wall -DNDEBUG=1
else
CFLAGS = -ggdb -Wall
endif
CFLAGS += -I.. -I.
LIBS = -lreadline -ltermcap

all : $(PROGRAMS)

lacp : $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LIBS)

vtss_lacp.o vtss_lacp.d : ../vtss_lacp.c
	$(CC) -MMD $(CFLAGS) -c -o vtss_lacp.o $<

vtss_os_linux.o vtss_os_linux.d : vtss_os_linux.c
	$(CC) -MMD $(CFLAGS) -c -o vtss_os_linux.o $<

clean :
	$(RM) $(OBJECTS) $(PROGRAMS) $(OBJECTS:.o=.d)

config :
	$(CFG_PROPAGATE) $(TOPDIR)/config.mk config.mk

.PHONY : all clean config

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),config)
-include $(OBJECTS:.o=.d)
endif
endif
