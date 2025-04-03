#
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
#

###############################################################################
# TODO, the -g options should be controlled by the debug target

ifeq ($(MSCC_SDK_ARCH),mipsel)
    TARGET = mipsel-buildroot-linux-gnu
else ifeq ($(MSCC_SDK_ARCH),arm)
    TARGET = arm-buildroot-linux-gnueabihf
else ifeq ($(MSCC_SDK_ARCH),arm64)
    TARGET = arm64-buildroot-linux-gnu
else
    TARGET = UNKNOWN
endif

ifeq (,$(findstring -DVTSS_SW_OPTION_DEBUG,$(DEFINES)))
    TARGET_C_AND_CXX_FLAGS_COMMON = -pthread -Os -pipe -feliminate-unused-debug-types -DMSCC_BRSDK="\"$(MSCC_SDK_NAME)\""
else
    ifeq (,$(findstring -DVTSS_SW_OPTION_ASAN,$(DEFINES)))
        TARGET_C_AND_CXX_FLAGS_COMMON = -pthread -O0 -pipe -feliminate-unused-debug-types -DMSCC_BRSDK="\"$(MSCC_SDK_NAME)\""
    else
        TARGET_C_AND_CXX_FLAGS_COMMON = -pthread -O0 -pipe -feliminate-unused-debug-types -fsanitize=address -Wno-psabi -DMSCC_BRSDK="\"$(MSCC_SDK_NAME)\""
    endif
endif

TARGET_C_AND_CXX_FLAGS =           \
  $(TARGET_C_AND_CXX_FLAGS_COMMON) \
  -Wall                            \
  -Wno-sign-compare                \
  -Wno-write-strings               \
  --sysroot $(MSCC_SDK_SYSROOT)    \
  -fsigned-char                    \
  -z noexecstack

###############################################################################
TARGET_CFLAGS        = $(TARGET_C_AND_CXX_FLAGS)

# The -fno-exceptions is required by parts of vtss_basics.
# In order to backtrace work correctly when -fno-exceptions is used, we also
# need to use -fasynchronous-unwind-tables.
# The drawback of using the latter flag is that it increases the final image
# size. In bringup images cannot toleate this as they must fit in a NOR flash
# (mipsel), so below, we make sure only to add it when compiling non-bringup
# images.
ifneq (BRINGUP,$(findstring BRINGUP, $(VTSS_PRODUCT_NAME)))
    # Not a bringup image
    ASYNC_UNWIND_TABLES_CXX_FLAG = -fasynchronous-unwind-tables
endif

###############################################################################
TARGET_CXXFLAGS_COMMON = $(TARGET_C_AND_CXX_FLAGS) \
  -fno-rtti                                        \
  -fno-exceptions                                  \
  $(ASYNC_UNWIND_TABLES_CXX_FLAG)                  \
  -std=c++17

###############################################################################
TARGET_CXXFLAGS = $(TARGET_CXXFLAGS_COMMON)

SDK_PREFIX = $(MSCC_SDK_PREFIX)

override CFLAGS   += $(INCLUDES) $(DEFINES) $(CPPFLAGS) $(TARGET_CFLAGS)
override CXXFLAGS += $(INCLUDES) $(DEFINES) $(CPPFLAGS) $(TARGET_CXXFLAGS) $(CFLAGS_CUSTOM)

COMPILER_WRAPPER_PREFIX = $(if $(COMPILE_COMMANDS),$(TOP)/build/make/compile-db-element.rb ,$(if $(CCACHE),ccache ))
COMPILE_COMMANDS_DRY = $(if $(COMPILE_COMMANDS),true )

LDFLAGS     = -L$(CFLAGS) --sysroot $(MSCC_SDK_SYSROOT)
LDXXFLAGS   = -L$(CXXFLAGS)
STRIP_FLAGS = --remove-section=.comment --remove-section=.note
ifeq ($(MSCC_SDK_ARCH),arm64)
ifeq (,$(findstring -DVTSS_SW_OPTION_DEBUG,$(DEFINES)))
LDSOFLAGS   = -shared --sysroot $(MSCC_SDK_SYSROOT)
else
LDSOFLAGS   = -shared --sysroot $(MSCC_SDK_SYSROOT) -fsanitize=address
endif
else
LDSOFLAGS   = -shared --sysroot $(MSCC_SDK_GNU)/xstax/release/x86_64-linux/usr/$(TARGET)/sysroot
endif
XAR         = $(COMPILE_COMMANDS_DRY)$(SDK_PREFIX)ar
CC          = $(COMPILER_WRAPPER_PREFIX)$(SDK_PREFIX)gcc
XCC         = $(CC)
UCC         = $(MSCC_SDK_UCLIBC)/xstax/release/x86_64-linux/bin/$(SDK_PREFIX)gcc
XCXX        = $(COMPILER_WRAPPER_PREFIX)$(SDK_PREFIX)g++
XSTRIP      = $(MSCC_SDK_PREFIX)strip
XLD         = $(SDK_PREFIX)gcc
XLDXX       = $(SDK_PREFIX)g++
XOBJCOPY    = echo XOBJCOPY should not be used

TARGET_CPU := vcoreiii

BUILD_CC  = gcc
BUILD_CXX = g++

HOST_CC  = $(BUILD_CC) $(BUILD_CFLAGS)
HOST_CXX = $(BUILD_CXX) $(BUILD_CXXFLAGS)

# compile_c <module-id> <o-file> <c-file> <x-flags>
define compile_c
$(call what,[CC ] $3 $4)
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

DTC_FLAGS += -Wno-unit_address_vs_reg \
	-Wno-unit_address_format \
	-Wno-avoid_unnecessary_addr_size \
	-Wno-alias_paths \
	-Wno-graph_child_address \
	-Wno-simple_bus_reg \
	-Wno-unique_unit_address \
	-Wno-interrupt_provider \
	-Wno-pci_device_reg \
	-i $(KERNEL_DTS_PATH)/$(ARCH)

#DTC_FLAGS += -Wnode_name_chars_strict \
#	-Wproperty_name_chars_strict

dtc_cpp_flags  = -nostdinc $(addprefix -I,$(KERNEL_DTS_PATH)) -undef -D__DTS__

define compile_dtc
$(call what,[DTC] $2)
$(Q)$(HOST_CC) -E -Wp,-MD,$3.pre.tmp $(dtc_cpp_flags) -x assembler-with-cpp -o $2.tmp $1 ;\
dtc -o $2 $(DTC_FLAGS) -d $3.dtc.tmp $2.tmp ;\
$(TOPABS)/build/tools/fixdeps.rb $3.o $3.dtb $3.pre.tmp $3.dtc.tmp > $3.dtb.d
endef

%.dtb: %.dts
	$(call compile_dtc,$<,$@,$*)

ifeq ($(COMPILE_COMMANDS),1)
else
# shared_library <module-id> <so-file> <o-files> <x-flags>
define shared_library
$(call what,[CXX] $2 $3)
$(Q)$(XCC) $3 -shared -o $2 -DVTSS_MODULE_ID=$1
endef
endif

%.bin: %.elf
	$(call what,Converting ELF to binary $@)
	$(Q)$(XOBJCOPY) -O binary $< $*.bin
