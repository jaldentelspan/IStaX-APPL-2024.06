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
# This defines paths and target rules preamble/postamble
TARGET      := linux-ppc

# General-purpose OS name
OPSYS       := LINUX

# Basic modules
MODULES := vtss_api vtss_appl
export MODULES

INCLUDES := 
DEFINES :=

# Packages together the above lines for compiler defines
DEFINES += -DVTSS_OPSYS_$(OPSYS)=1
DEFINES += -D$(VTSS_PRODUCT_CHIP)

ifneq (,$(VTSS_PRODUCT_HW))
DEFINES += -D$(VTSS_PRODUCT_HW)
endif

# This allows two boards (e.g. E-StaX and B2) to be enabled
ifneq (,$(VTSS_PRODUCT_HW2))
DEFINES += -D$(VTSS_PRODUCT_HW2)
endif

DEFINES += -DVTSS_OPT_VCORE_III=0

# Optional number of ports
ifneq (,$(VTSS_PRODUCT_PORTS))
DEFINES += -DVTSS_OPT_PORT_COUNT=$(VTSS_PRODUCT_PORTS)
endif
