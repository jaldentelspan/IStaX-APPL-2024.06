# 
# Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.
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

COMPILER_PREFIX := 

# Mimic rules.mak in Linux
ACTUAL_CFLAGS := $(filter-out -fno-rtti -Woverloaded-virtual -Wno-unused-but-set-variable,$(LINUX_GLOBAL_CFLAGS) -Wno-pointer-sign)
ACTUAL_CXXFLAGS := $(filter-out -Wstrict-prototypes -Wno-sign-compare -Wno-unused-but-set-variable -Wno-strict-aliasing -Wno-enum-compare,$(LINUX_GLOBAL_CFLAGS))

DEFINES += -DVTSS_NO_TURNKEY=1

WARNING_FLAGS := -Wall                        \
                 -Wno-sign-compare            \
                 -Wno-unused-but-set-variable \
                 -Wno-unused-function         \
                 -Wno-unused-variable         \
                 -fsanitize=address

CFLAGS = $(INCLUDES)      \
         $(DEFINES)       \
         $(ACTUAL_CFLAGS) \
         $(CFLAGS_CUSTOM) \
         $(WARNING_FLAGS)

CXXFLAGS = -Wall                    \
          $(INCLUDES)               \
          $(DEFINES)                \
          $(ACTUAL_CXXFLAGS)        \
          $(CXXFLAGS_CUSTOM)        \
          $(WARNING_FLAGS)          \
          -fno-exceptions -fno-rtti \
          -std=c++17                \
          -U__STRICT_ANSI__

LDSOFLAGS = -shared -Wl,--no-allow-shlib-undefined
LDFLAGS  = -L$(LINUX_INSTALL)/lib $(LINUX_GLOBAL_LDFLAGS) -fsanitize=address -fuse-ld=gold
XAR      = $(COMPILER_PREFIX)ar
XCC      = $(if $(CCACHE),ccache) $(COMPILER_PREFIX)gcc
XCXX     = $(if $(CCACHE),ccache) $(COMPILER_PREFIX)g++
XSTRIP   = $(COMPILER_PREFIX)strip
XLD      = $(XCC)
XLDXX    = $(XCXX)
XOBJCOPY = $(COMPILER_PREFIX)objcopy

HOST_CC         = $(if $(CCACHE),ccache) gcc
HOST_CXX        = $(if $(CCACHE),ccache) g++

TARGET_CPU := 2 # Used for firmware image target CPU type

# compile_c <module-id> <o-file> <c-file> <x-flags>
define compile_c
$(call what,[CC ] $3)
$(Q)$(XCC) -c -g -o $2 -DVTSS_MODULE_ID=$1 -MD $(CFLAGS) $4 $3
endef

define compile_cxx
$(call what,[CXX] $3)
$(Q)$(XCXX) -c -g -o $2 -DVTSS_MODULE_ID=$1 -MD $(CXXFLAGS) $4 $3
endef

define compile_lib_cxx
$(call what,[CXX-LIB] $3)
$(Q)$(XCXX) -c -o $2 -DVTSS_MODULE_ID=$1 -MD $(CXXFLAGS) -fPIC $4 $3
endef

# shared_library <module-id> <so-file> <o-files> <x-flags>
define shared_library
$(call what,[CXX] $2 $3)
$(Q)$(XCC) $3 -shared -o $2 -DVTSS_MODULE_ID=$1
endef

%.bin: %.elf
	$(call what,Converting ELF to binary $@)
	$(Q)$(XOBJCOPY) -O binary $< $*.bin
