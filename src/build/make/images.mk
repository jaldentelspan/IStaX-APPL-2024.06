# Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.
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

ifneq ("$(MSCC_SDK_BASE)","")	# Only if we have MSCC_SDK_BASE

# Linux target (if applicable)
MACHINE = $(call linuxSwitch/TargetName,$(VTSS_PRODUCT_HW),$(VTSS_PRODUCT_CHIP))

ifeq ($(origin MFI), undefined)
    MFI := $(TOPABS)/build/tools/mfi.rb
endif

metadata.txt: always
	$(Q)rm -f $@
	$(Q)echo "Date: $(CTIME)" > $@
	$(Q)echo "Version: $(RELEASE_VERSION)" >> $@
	$(Q)echo "Revision: $(CODE_REVISION)" >> $@
	$(Q)echo "Buildno: $(BUILD_NUMBER)" >> $@

# $1: Image file (*.mfi)
# $2: Build flavor type
# This tests maximum image size.
# Constraints due to NOR/NAND-configurations:
#   In NOR/NAND-configurations, we always use the bringup-image in the flash
#   images, and we can go as low as to 8 MB flashes. Only the size of the
#   bringup image is of interest.
#
# Constraints due to NOR-only configurations:
#   It is mostly the 32MB dual-image version that constrain us.
#   This one has a limit of 14.5 MB of which we leave 0.5 MB for customer
#   improvements.
#   The 32MB single-image and 64MB dual-image versions have a limit of 28 MB,
#   of which we leave 4 MB for customer improvements.
#   However, we have written in the release note that customers must start using
#   the 32MB single-image version or the 64MB dual-image version - at least for
#   JR2 and ServalT.
#   For Caracal and Ocelot, we must still have a limit size for SMBStaX (and
#   WebStaX) of 14 MB, so that we can get to write in the release note that we
#   no longer support those platforms before it actually happens.
#
# This boils down to:
# IStaX:
#   All targets:         Max 24.00 MB = 25,165,824 bytes (max NOR storage size - 4.0 MB for customer improvements)
# SMBStaX:
#   Caracal and Ocelot:  Max 14.00 MB = 14,680,064 bytes (max 16MB dual-image NOR-only image size - 0.5 MB for customer improvements)
#   Others:              Max 24.00 MB = 25,165,824 bytes (max NOR storage size - 4.0 MB for customer improvements)
# WebStaX:
#   All targets:         Max 14.00 MB = 14,680,064 bytes (max 16MB dual-image NOR-only image size - 0.5 MB for customer improvements)
# Bringup:
#   Caracal and Ocelot:  Max  7.25 MB =  7,602,176 bytes
#   Others:              Max  7.75 MB =  8,126,464 bytes
#
# The function is disabled under normal compilations, because it might be that a
# customer wants to add his own code that makes the MFI file bigger than what is
# room for in a pure NOR-flash-based design, which is fine.
# The WS2_IMAGE_SIZE_CHK environment variable is therefore only set when
# compiling through Jenkins/SimpleGrid.
ifneq ("$(WS2_IMAGE_SIZE_CHK)","")
define check-image-size
	$(Q)if [ $2 = "ISTAX" ] || [ $2 = "ISTAX380" ] || [ $2 = "ISTAXMACSEC" ]; then                                 \
		S=25165824;                                                                                            \
	elif [ $2 = "SMBSTAX" ] ; then                                                                                 \
		if echo $1 | grep -Eq "pcb090|pcb091|pcb120|pcb123" ; then                                             \
			S=14680064;                                                                                    \
		else                                                                                                   \
			S=25165824;                                                                                    \
		fi;                                                                                                    \
	elif [ $2 = "WEBSTAX" ] ; then                                                                                 \
		S=14680064;                                                                                            \
	elif [ $2 = "BRINGUP" ] ; then                                                                                 \
		if echo $1 | grep -Eq "pcb090|pcb091|pcb120|pcb123" ; then                                             \
			S=7602176;                                                                                     \
		else                                                                                                   \
			S=8126464;                                                                                     \
		fi;                                                                                                    \
	else                                                                                                           \
		sh -c "echo \"Error: Package $2: Max size not defined\" >> /proc/self/fd/2";                           \
	fi;                                                                                                            \
	actualsize=$$(wc -c < "$1") ;                                                                                  \
	if [ $$actualsize -gt $$S ] ; then                                                                             \
		sh -c "echo \"Error: Image $1 size ($$actualsize) greater than size limit ($$S)\" >> /proc/self/fd/2"; \
	else                                                                                                           \
		echo "Info: Image $1 size ($$actualsize) lower than size limit ($$S)";                                 \
	fi
endef
endif # WS2_IMAGE_SIZE_CHK

# On WSL, symlinks work in Linux, but not Windows, so if your Web-server is a
# Windows app, you cannot get new.mfi or new.itb when these are symlinks, so
# copying them instead.
define copy-to-dir-and-link
	$(Q)if [ ! -z $(USER) ] ; then					\
	    if [ -d $1 ] ; then						\
	        cp -v $2 $1;						\
		if grep -i microsoft /proc/version > /dev/null ; then	\
	            cp -v $2 $1/$3;					\
		else							\
	            ln -sf $2 $1/$3;					\
	            echo "Linked $1/$3 => $1/$2";			\
		fi;							\
	        chmod 664 $1/$2 $1/$3;					\
	    fi;								\
	fi
endef
endif				# MSCC_SDK_BASE
