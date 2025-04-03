# Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.
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

include $(BUILD)/make/templates/linuxSwitch.in

WEB_PROFILE_LIST = \
    lan966x

# To enable debug and address sanitizer include the following line
#Custom/Defines     := -DVTSS_SW_OPTION_DEBUG -DVTSS_SW_OPTION_ASAN

Custom/OmitModules := eee poe

# Build all listed profiles

# DefineTargetByPackage($1,$2)
# $1: package name
# $2: profile names
$(eval $(foreach p,$(WEB_PROFILE_LIST),$(call DefineTargetByPackage,web,$(p))))

# Normal boilerplate
$(eval $(call linuxSwitch/Multi,WEBSTAX,brsdk,arm))
$(eval $(call linuxSwitch/Build))
