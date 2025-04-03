# 
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
# 
LINUX_ROOT	= $(TOP)/sw_genie_kernel_2_4_20
LINUX_INCLUDE	= $(LINUX_ROOT)/include

init::
	-$(MAKE) -C $(LINUX_ROOT) symlinks

MFLAGS		= -fno-strict-aliasing -fverbose-asm -Wall -Wstrict-prototypes \
			-Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings \
			-Waggregate-return -Wmissing-prototypes -Wmissing-declarations \
			-Wno-long-long -O6 -ggdb -mcpu=403 
CFLAGS		= $(MFLAGS) $(INCLUDES) -I$(LINUX_INCLUDE) $(DEFINES)
LDFLAGS		= 

COMPILER_PREFIX = ppc_405-
XCC		= $(COMPILER_PREFIX)gcc
XLD		= $(XCC)

# compile_c <o-file> <c-file> <x-flags>
compile_c = $(XCC) -c -o $*.o -MD $(CFLAGS) $<

LINT_PROJ_CONFIG = $(BUILD)/make/proj_api.lnt

%.o: %.c
	$(call compile_c, $@, $<)

%.bin: %.elf
	-@echo I will not build $*.bin for this platform
