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

MSCC_SDK_VERSION       ?= v02.11
MSCC_SDK_ARCH          := arm
MSCC_SDK_NAME          ?= mscc-brsdk-$(MSCC_SDK_ARCH)-$(MSCC_SDK_VERSION)
MSCC_SDK_FLAVOR        ?= minimal
MSCC_SDK_BASE          ?= /opt/mscc/$(MSCC_SDK_NAME)
MSCC_SDK_INSTALL       ?= /usr/local/bin/mscc-install-bsp
MSCC_SDK_CMAKE_TC_PATH ?= $(MSCC_SDK_BASE)/stage2/$(MSCC_SDK_FLAVOR)/x86_64-linux/usr/share/buildroot/toolchainfile.cmake
MSCC_SDK_CMAKE_TC      ?= -DCMAKE_TOOLCHAIN_FILE=$(MSCC_SDK_CMAKE_TC_PATH)

include $(TOPABS)/build/make/paths-api.mk

init::
	$(call what,Using toolchain: $(MSCC_SDK_BASE) - $(MSCC_SDK_ARCH) - $(MSCC_SDK_FLAVOR))

$(MSCC_SDK_BASE)/sdk-setup.mk:
	@if [ -x $(MSCC_SDK_INSTALL) ] ; then \
		echo "Trying to install BSP $(MSCC_SDK_VERSION) for $(MSCC_SDK_ARCH)"; \
		sudo $(MSCC_SDK_INSTALL) -a $(MSCC_SDK_ARCH) $(MSCC_SDK_VERSION); \
	else \
		echo "Please install $(MSCC_SDK_NAME) in $(MSCC_SDK_BASE)"; \
		exit 1; \
	fi

-include $(MSCC_SDK_BASE)/sdk-setup.mk

VTSS_EXTERNAL_BUILD_ENV_TOP := $(MSCC_SDK_BASE)
export VTSS_EXTERNAL_BUILD_ENV_TOP
export MSCC_SDK_SYSROOT
export MSCC_SDK_PATH

