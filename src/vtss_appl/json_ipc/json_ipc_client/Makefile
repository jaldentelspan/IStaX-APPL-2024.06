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

BUILD = $(CURDIR)/../../../build

MSCC_SDK_ARCH = mips
MSCC_SDK_FLAVOR = smb
include $(BUILD)/make/templates/base.in
include $(BUILD)/make/paths-brsdk.mk
include $(BUILD)/make/target-brsdk.mk

OBJS = json_ipc_client.o

all: json_ipc_client mfi clean

json_ipc_client: $(OBJS)
	$(CC) -o $@ $^

mfi:
	mkdir usr
	mkdir usr/bin
	mv json_ipc_client usr/bin
	tar -cvf json_ipc_client.tar usr
	xz --check=none json_ipc_client.tar
	$(MSCC_SDK_BASE)/stage1/x86_64-linux/usr/bin/mfi.rb -i $(BUILD)/obj/smb_serval_t.mfi -o /tftpboot/$(USER)/json_ipc_client.mfi rootfs -a append -f json_ipc_client.tar.xz

clean:
	rm -fr usr
	rm -f $(OBJS) json_ipc_client.d json_ipc_client.tar.xz

