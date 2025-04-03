# Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.
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

BINMERGE := ../../third_party/eCos/host/webstax_binmerge/webstax_binmerge.c

compile:
	gcc -g -m32 -Wall $(BINMERGE) -o webstax_binmerge

compile_256:
	gcc -g -m32 -Wall $(BINMERGE) -DALLOWED_MAXIMUM_SECTOR_SIZE=256 -o webstax_binmerge_256

dbg:
	ddd webstax_binmerge

# If not given on the command line, use this one:
REDBOOT_PATH ?= ../obj/redboot.bin
FIRMWARE_PATH ?= ../obj/*.gz

# Example #1:
# Create main file from redboot and runtime code
# with the following features:
#  - Dual Image Support
#  - Flash sector is 256 KBytes, size is 16MB, for example: M26P128 (the one on Vitesse EVB)
#  - Verbose output
#  - MAC Address = 00:01:c1:01:02:03
#  - Board ID = 1234
#  - Default bootscript
flash16_256: compile_256
	./webstax_binmerge_256 -v -sb 256 -r $(REDBOOT_PATH) -w $(FIRMWARE_PATH) -o main_flash16_256.bin -a "MAC=00:01:c1:01:02:03" -a "BOARDID=1234"

# Example #2:
# Create main file from redboot and runtime code
# with the following features:
#  - Dual Image Support
#  - Flash sector is 64 KBytes, size is 16MB, for example: MX25L128
#  - Verbose output
#  - MAC Address = 00:01:c1:01:02:03
#  - Board ID = 1234
#  - Default bootscript
flash16_64: compile
	./webstax_binmerge -v -sb 64 -sc 256 -r $(REDBOOT_PATH) -w $(FIRMWARE_PATH) -o main_flash16_64.bin -a "MAC=00:01:c1:01:02:03" -a "BOARDID=1234"

# Example #3:
# Create main file from redboot and runtime code 
# with the following features:
#  - Dual Image Support
#  - Flash sector is 128 KBytes, size is 16MB.
#  - Verbose output
#  - MAC Address = 00:01:c1:01:02:03
#  - Board ID = 1234
#  - Default bootscript
flash16_128: compile
	./webstax_binmerge -v -r $(REDBOOT_PATH) -w $(FIRMWARE_PATH) -o main_flash16_128.bin -a "MAC=00:01:c1:01:02:03" -a "BOARDID=1234"

# Example #4:
# Create main file from redboot and runtime code
# with the following features:
#  - Dual Image Support
#  - Flash sector is 64 KBytes, size is 32MB, for example: MX25L256
#  - Verbose output
#  - MAC Address = 00:01:c1:01:02:03
#  - Board ID = 1234
#  - Default bootscript
flash32_64: compile
	./webstax_binmerge -v -sb 64 -sf 32 -r $(REDBOOT_PATH) -w $(FIRMWARE_PATH) -o main_flash32_64.bin -a "MAC=00:01:c1:01:02:03" -a "BOARDID=1234"

