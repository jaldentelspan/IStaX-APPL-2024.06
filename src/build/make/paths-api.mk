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

# Microchip is aware that some terminology used in this technical document is
# antiquated and inappropriate. As a result of the complex nature of software
# where seemingly simple changes have unpredictable, and often far-reaching
# negative results on the software's functionality (requiring extensive retesting
# and revalidation) we are unable to make the desired changes in all legacy
# systems without compromising our product or our clients' products.

MESA_API_VERSION       ?= v2024.06-1-0-g20182dd90
MESA_API_BRANCH        ?= 2024.06-soak
MESA_API_NAME          ?= mesa-$(MESA_API_VERSION)@$(MESA_API_BRANCH)
MESA_API_BASE          ?= /opt/mscc/$(MESA_API_NAME)
MESA_API_INSTALL       ?= /usr/local/bin/mscc-install-pkg

ifneq ("$(wildcard $(TOP)/vtss_api)","")
API_PATH := $(TOP)/vtss_api
API_ABS_PATH := $(TOPABS)/vtss_api
ifneq ("$(wildcard $(API_PATH)/bin)","")
API_BUILD_PATH := $(API_PATH)/bin/${MSCC_SDK_ARCH}
else
API_BUILD_PATH := ./build_api
endif
else
API_PATH := $(MESA_API_BASE)
API_ABS_PATH := $(API_PATH)
API_BUILD_PATH := $(API_PATH)/bin/${MSCC_SDK_ARCH}
endif

export MESA_API_BRANCH
export MESA_API_VERSION
export MESA_API_NAME
export MESA_API_BASE
export MESA_API_INSTALL
export API_PATH
export API_BUILD_PATH
