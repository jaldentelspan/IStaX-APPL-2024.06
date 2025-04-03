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

MSCC_SDK_VERSION       ?= 2024.06
MSCC_SDK_STEM          ?= mscc-brsdk
MSCC_SDK_BRANCH        ?=

ifeq ($(MSCC_SDK_BRANCH),brsdk)
    MSCC_SDK_NAME          ?= $(MSCC_SDK_STEM)-$(MSCC_SDK_ARCH)-$(MSCC_SDK_VERSION)
else ifeq ($(MSCC_SDK_BRANCH),)
    MSCC_SDK_NAME          ?= $(MSCC_SDK_STEM)-$(MSCC_SDK_ARCH)-$(MSCC_SDK_VERSION)
else
    MSCC_SDK_NAME          ?= $(MSCC_SDK_STEM)-$(MSCC_SDK_ARCH)-$(MSCC_SDK_VERSION)-$(MSCC_SDK_BRANCH)
endif

MSCC_SDK_BASE          ?= /opt/mscc/$(MSCC_SDK_NAME)
ifeq ($(MSCC_SDK_ARCH),arm64)
    MSCC_SDK_GNU           ?= $(MSCC_SDK_BASE)/$(MSCC_SDK_TARGET)
    MSCC_SDK_UCLIBC        ?= $(MSCC_SDK_BASE)/$(MSCC_SDK_TARGET)
    MSCC_SDK_ROOTFS	   ?= $(MSCC_SDK_GNU)/xstax/release/rootfs.tar
else ifeq ($(MSCC_SDK_ARCH),arm)
    MSCC_SDK_GNU           ?= $(MSCC_SDK_BASE)/$(MSCC_SDK_TARGET)
    MSCC_SDK_UCLIBC        ?= $(MSCC_SDK_BASE)/$(MSCC_SDK_TARGET)
    MSCC_SDK_ROOTFS	   ?= $(MSCC_SDK_GNU)/xstax/release/rootfs.tar
else
    MSCC_SDK_GNU           ?= $(MSCC_SDK_BASE)/$(MSCC_SDK_TARGET)
    MSCC_SDK_UCLIBC        ?= $(MSCC_SDK_BASE)/mipsel-mips32r2-linux-uclibc
    MSCC_SDK_ROOTFS	   ?= $(MSCC_SDK_GNU)/xstax/release/rootfs.tar
endif

MSCC_SDK_INSTALL       ?= /usr/local/bin/mscc-install-pkg

MSCC_SDK_CMAKE_TC_PATH ?= $(MSCC_SDK_PATH)/usr/share/buildroot/toolchainfile.cmake
MSCC_SDK_CMAKE_TC ?= -DCMAKE_TOOLCHAIN_FILE=$(MSCC_SDK_CMAKE_TC_PATH)

include $(TOPABS)/build/make/paths-api.mk

init::
	$(call what, "Using brsdk:       $(MSCC_SDK_BASE) - $(MSCC_SDK_NAME) - $(MSCC_SDK_FLAVOR)")
	$(call what, "Using toolchain:   $(MSCC_TOOLCHAIN_BASE)")


# Unconditionally check (and possibly install) both toolchain and sdk. We need
# to do this unconditionally as the sdk may be installed and the toolchain may
# not be installed, and we cannot do the "same" trick as we do below, as the
# toolchain version depends on the sdk...
-include dummy
.PHONY: dummy
dummy:
	@MSCC_SDK_INSTALL=$(MSCC_SDK_INSTALL) \
	MSCC_SDK_NAME=$(MSCC_SDK_NAME) \
	MSCC_SDK_BRANCH=$(MSCC_SDK_BRANCH) \
	MSCC_SDK_VERSION=$(MSCC_SDK_VERSION) \
	MSCC_SDK_BASE=$(MSCC_SDK_BASE) \
	MSCC_SDK_SETUP=$(MSCC_SDK_BASE)/sdk-setup.mk \
	$(TOPABS)/build/make/install-sdk.rb

# Install the brsdk and toolchain if the $(MSCC_SDK_BASE)/sdk-setup.mk file is
# missing. This turns out to be needed to force the installation to take place
# before the '$(MSCC_SDK_BASE)/sdk-setup.mk' is loaded (if we only relay on the
# mechanism above then the sdk-setup.mk is not installed before including).
$(MSCC_SDK_BASE)/sdk-setup.mk:
	@MSCC_SDK_INSTALL=$(MSCC_SDK_INSTALL) \
	MSCC_SDK_NAME=$(MSCC_SDK_NAME) \
	MSCC_SDK_BRANCH=$(MSCC_SDK_BRANCH) \
	MSCC_SDK_VERSION=$(MSCC_SDK_VERSION) \
	MSCC_SDK_BASE=$(MSCC_SDK_BASE) \
	MSCC_SDK_SETUP=$(MSCC_SDK_BASE)/sdk-setup.mk \
	$(TOPABS)/build/make/install-sdk.rb
-include $(MSCC_SDK_BASE)/sdk-setup.mk

MSCC_TOOLCHAIN_BASE    ?= /opt/mscc/mscc-toolchain-bin-${MSCC_TOOLCHAIN_FILE}

VTSS_EXTERNAL_BUILD_ENV_TOP := $(MSCC_SDK_BASE)
export VTSS_EXTERNAL_BUILD_ENV_TOP
export MSCC_SDK_SYSROOT
export MSCC_SDK_PATH

PATH := $(MSCC_SDK_PATH)/bin:$(MSCC_SDK_PATH)/sbin:$(PATH)

