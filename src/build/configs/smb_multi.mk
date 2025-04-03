#
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
#
include $(BUILD)/make/templates/linuxSwitch.in

SMB_PROFILE_LIST =   \
    caracal1         \
    caracal2         \
    caracal_lite     \
    jr2_24           \
    jr2_48           \
    lynx2            \
    ocelot_10        \
    ocelot_8         \
    serval2          \
    serval2_lite     \
    serval_t         \
    serval_te        \
    serval_te10      \
    serval_tep       \
    serval_tp        \
    sparxIII_10      \
    sparxIII_18      \
    sparxIII_24      \
    sparxIII_26      \
    sparxIII_pds408g \
    sparxIV_34       \
    sparxIV_44       \
    sparxIV_52_48    \
    sparxIV_80_24    \
    sparxIV_80_48    \
    sparxIV_90_48    \

# To enable Private MIBs, comment in the following line:
# Custom/AddModules := private_mib private_mib_gen

# Custom/Stage2 := debug

# DefineTargetByPackage($1,$2)
# $1: package name
# $2: profile name

# Build all listed profiles
$(eval $(foreach p,$(SMB_PROFILE_LIST),$(call DefineTargetByPackage,smb,$(p))))

# Normal boilerplate
$(eval $(call linuxSwitch/Multi,SMBSTAX,brsdk,mipsel))
$(eval $(call linuxSwitch/Build))
