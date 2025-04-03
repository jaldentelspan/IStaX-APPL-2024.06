#
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
#
# Standard setup
.SUFFIXES: # We go by ourselves!

#Include config file, first via absolute path, the relative to BUILD
ifneq ($(wildcard $(CONFIG_FILE)),)
include $(CONFIG_FILE)
else
include $(BUILD)/$(CONFIG_FILE)
endif

ifneq ($(filter $(VTSS_PRODUCT_NAME),10G_phy b2 estax_api 1g_phy),$(VTSS_PRODUCT_NAME))
  # E-StaX stuff
  include $(BUILD)/make/setup.mk
endif

include $(TOPABS)/build/make/paths-$(TARGET).mk

.PHONY: init all clean clobber ccversion automatic build_api coverity_cfg

ifneq ($(V),)
  what =
  Q    =
else
  what = @echo $1
  Q    = @
endif

# Internal/external CPU
ifeq (,$(findstring -DVTSS_OPT_VCORE_III=0,$(DEFINES)))
VTSS_INTERNAL_CPU=1
else
VTSS_EXTERNAL_CPU=1
endif

# Default install paths
DESTDIR ?= install
RBINDIR ?= $(DESTDIR)/bin
LIBDIR  ?= $(DESTDIR)/lib
BINDIR  ?= $(DESTDIR)/usr/bin
VARDIR  ?= $(DESTDIR)/var
ETCDIR  ?= $(DESTDIR)/etc
LIBDIR  ?= $(DESTDIR)/lib

# Objects
OBJECTS = $(foreach m,$(MODULES),$(OBJECTS_$m))

# Targets
TARGETS = $(foreach m,$(MODULES),$(TARGETS_$m))

# Establish VTSS_SW_OPTION_xxx for defined modules 'xxx'
UCMODULES := $(shell echo $(MODULES) | tr '[:lower:]' '[:upper:]')
DEFINES   += $(foreach mod,$(UCMODULES),-DVTSS_SW_OPTION_$(mod)=1 )
DEFINES   += -DDISABLED_MODULES="$(foreach m,$(MODULES_DISABLED),$m)"
DEFINES   += -DMIBPREFIX=$(if $(Custom/MibPrefix),$(Custom/MibPrefix),VTSS)
DEFINES   += -DMIB_ENTERPRISE_NAME=$(if $(Custom/MibEnterpriseName),$(Custom/MibEnterpriseName),vtss)
DEFINES   += -DMIB_ENTERPRISE_OID=$(if $(Custom/MibEnterpriseOid),$(Custom/MibEnterpriseOid),6603)
DEFINES   += -DMIB_ENTERPRISE_PRODUCT_NAME=$(if $(Custom/MibEnterpriseProductName),$(Custom/MibEnterpriseProductName),vtssSwitchMgmt)
DEFINES   += -DMIB_ENTERPRISE_PRODUCT_ID=$(if $(Custom/MibEnterpriseProductId),$(Custom/MibEnterpriseProductId),1)
define set_module
  MODULE_$1 := 1

endef
$(eval $(foreach mod,$(UCMODULES),$(call set_module,$(mod))))

DEFINES += $(if $(filter ZLS30341,$(UCMODULES)),-DZLS30341_INCLUDED,)
DEFINES += $(if $(filter ZLS30361,$(UCMODULES)),-DZLS30361_INCLUDED,)

# @arg $1 is list of modules to check for (at least one present => true)
# @arg $2 is what to return if true
# @arg $3 is what to return if false
define if-module
  $(if $(filter $1,$(MODULES)),$2,$3)
endef

# @arg $1 is list of modules to check for (all present => true)
# @arg $2 is what to return if true
# @arg $3 is what to return if false
define if-all-modules
  $(if $(filter-out $(MODULES),$1),$3,$2)
endef


# @arg $1 is the stem of a variable to expand for this platform
define BSP_Component
  $(if $(CustomBSP),$(call CustomBSP,$1),$($1_$(VTSS_PRODUCT_CHIP)) $($1_$(VTSS_PRODUCT_HW)))
endef

# Slurp module defns - main/vtss_appl *last*
ifeq ($(filter icli,$(MODULES)),icli)
include $(BUILD)/make/setup_icli.in # iCLI boilterplate rules
endif
include $(foreach m,$(filter-out main vtss_appl,$(MODULES)),$(BUILD)/make/module_$m.in)
include $(foreach m,$(filter     main vtss_appl,$(MODULES)),$(BUILD)/make/module_$m.in)

# Gotta have the path to the system include files first in the INCLUDES list, to overcome
# problems arising if a module contains header files with the same name as one of the
# standard header files.
INCLUDES := $(SYS_INCLUDES) $(INCLUDES)

cxx_path:= /cxx/
c_path:= /c/
slash:= /

# Settings for the "automatic" SW test harness + framework
AUTOMATIC_TEST_BLDINFO	= $(OBJ)/vtss_test/buildinfo.txt
AUTOMATIC_TEST_DEST	= $(OBJ)/automatic.tar.gz
AUTOMATIC_TEST_DEST_TMP	= $(OBJ)/automatic.tar
AUTOMATIC_TEST_SRC_DIRS	= vtss_api vtss_appl

# Targets - The user can user the M variable to select which modules he wants to have as target.
ifneq ($(M),)
  TIDY_TARGETS                       := $(foreach i,$(M),$(TIDY_FILES_$i))
  VTSS_CODE_STYLE_CHK_TARGETS        := $(foreach i,$(M),$(subst $(c_path),$(slash),$(subst $(cxx_path),$(slash),$(VTSS_CODE_STYLE_CHK_FILES_$i))))
  VTSS_CODE_STYLE_INDENT_CHK_TARGETS := $(foreach i,$(M),$(subst $(c_path),$(slash),$(subst $(cxx_path),$(slash),$(VTSS_CODE_STYLE_CHK_FILES_TWO_INDENT_$i))))
  JSLINT_TARGETS                     := $(foreach i,$(M),$(JSLINT_FILES_$i))
else
  TIDY_TARGETS                       := $(TIDY_FILES)                           $(foreach i,$(MODULES),$(TIDY_FILES_$i))
  VTSS_CODE_STYLE_CHK_TARGETS        := $(VTSS_CODE_STYLE_CHK_FILES)            $(foreach i,$(MODULES),$(subst $(c_path),$(slash),$(subst $(cxx_path),$(slash),$(VTSS_CODE_STYLE_CHK_FILES_$i))))
  VTSS_CODE_STYLE_INDENT_CHK_TARGETS := $(VTSS_CODE_STYLE_CHK_FILES_TWO_INDENT) $(foreach i,$(MODULES),$(subst $(c_path),$(slash),$(subst $(cxx_path),$(slash),$(VTSS_CODE_STYLE_CHK_FILES_TWO_INDENT_$i))))
  JSLINT_TARGETS                     := $(JSLINT_FILES)                         $(foreach i,$(MODULES),$(JSLINT_FILES_$i))
endif

# Dummy
init:: ;

all: init compile $(TARGETS)
	$(call what,Images produced:)
	$(Q)-ls -l $(addprefix *.,$(IMAGETYPES))

compile: $(OBJECTS) $(LIB_FILES)

$(OBJECTS) $(LIB_FILES) $(TARGETS): | build_api

clean::
	-rm -rf *.[oadc] *.elf *.so *.xz *.dat *.txt *.bin *.ireg *.htm *.css *.squashfs *.cxx *.itb *.ext4 *.ubifs html_out *-rootfs *-itb *-etx4 *.dtbo *.dtb *icli* automatic* vtss_test *.tmp *.dtb

clobber::

automatic:
	@echo "Product:             $(VTSS_PRODUCT_NAME)" > $(AUTOMATIC_TEST_BLDINFO)
	@echo "GUI:                 $(VTSS_PRODUCT_NAME_GUI)" >> $(AUTOMATIC_TEST_BLDINFO)
	@echo "Chip:                $(VTSS_PRODUCT_CHIP)" >> $(AUTOMATIC_TEST_BLDINFO)
	@echo "Hardware type:       $(VTSS_PRODUCT_HW)" >> $(AUTOMATIC_TEST_BLDINFO)
	@echo "Stacking/standalone: $(VTSS_PRODUCT_STACKABLE)" >> $(AUTOMATIC_TEST_BLDINFO)
	$(Q)tar -c -f $(AUTOMATIC_TEST_DEST_TMP) vtss_test
	$(Q)tar -C$(BUILD)/.. -r -f $(AUTOMATIC_TEST_DEST_TMP) --exclude vtss_test/automatic/examples vtss_test
	$(Q)( cd $(BUILD)/.. && find $(AUTOMATIC_TEST_SRC_DIRS) -name test -a -type d ) | xargs tar -C$(BUILD)/.. -r -f $(AUTOMATIC_TEST_DEST_TMP)
	$(Q)tar -C$(BUILD)/.. -r -f $(AUTOMATIC_TEST_DEST_TMP) vtss_appl/snmp/mibs build/lib
	$(Q)gzip -c < $(AUTOMATIC_TEST_DEST_TMP) > $(AUTOMATIC_TEST_DEST)
	$(Q)rm $(AUTOMATIC_TEST_DEST_TMP)
	@echo "Automatic package: $(AUTOMATIC_TEST_DEST)"

ccversion:
	$(Q)$(XCC) --version
	$(Q)$(XCXX) --version
	$(Q)which $(XCC) $(XCXX) $(if $(CCACHE),ccache)

CLEANUP="0" # Set to 1 to remove temp. files for code style chk. 0 = leave temp files for the user to see what the issues were
# Create a .vtss_style file in case of style errors
code_style_chk: $(VTSS_CODE_STYLE_CHK_TARGETS)
	$(Q)perl $(BUILD)/tools/chk_scripts/vtss_c_code_style_chk.pl --cleanup=$(CLEANUP) $(VTSS_CODE_STYLE_CHK_TARGETS)
	$(Q)perl $(BUILD)/tools/chk_scripts/vtss_c_code_style_chk.pl --cleanup=$(CLEANUP) --indent 2 $(VTSS_CODE_STYLE_INDENT_CHK_TARGETS)

# Overwrite files themselves in case of style errors
style_inplace: $(VTSS_CODE_STYLE_CHK_TARGETS)
	$(Q)perl $(BUILD)/tools/chk_scripts/vtss_c_code_style_chk.pl --inplace $(VTSS_CODE_STYLE_CHK_TARGETS)
	$(Q)perl $(BUILD)/tools/chk_scripts/vtss_c_code_style_chk.pl --inplace --indent 2 $(VTSS_CODE_STYLE_INDENT_CHK_TARGETS)

jslint: $(JSLINT_TARGETS)
	$(call what,'Javascript linting $(words $^) file(s)')
	$(Q)perl ../make/jslint.pl $^

tidy: $(TIDY_TARGETS)
	$(call what,'Tidy $(words $^) file(s)')
	$(Q)perl $(BUILD)/tools/chk_scripts/html_tidy_chk.pl $^

html : Custom/NoHtmlCompress = 1
html: vtss-www-rootfs.squashfs

smoke_log:
	$(Q)ruby $(BUILD)/../vtss_test/automatic/smoke_test.rb -s

smoke:
	$(Q)ruby $(BUILD)/../vtss_test/automatic/smoke_test.rb -t "$(M)"

smoke_debug:
	$(Q)ruby $(BUILD)/../vtss_test/automatic/smoke_test.rb --debug -t "$(M)"

clang_complete:
	$(Q)$(BUILD)/make/clang_complete_conf_gen.pl $(ECOS_TOOLCHAIN_TOP) $(CXXFLAGS) > .clang_complete

vscode_workspace:
	$(Q)$(BUILD)/make/vscode_workspace.py $(MAKECMDGOALS) $(ECOS_TOOLCHAIN_TOP) $(CXXFLAGS)

check_public_headers_appl:
	$(Q)../make/check_public_headers.rb

show_defines:
	$(info Defines: $(DEFINES))

coverity_cfg:
	rm -fr coverity.xml template-*-config-* gcc-config-*
	cov-configure --config coverity.xml --gcc
	cov-configure --config coverity.xml --comptype prefix --compiler ccache --template
	cov-configure --config coverity.xml --compiler `basename $(SDK_PREFIX)gcc` --comptype gcc --template
	cov-configure --config coverity.xml --list-configured-compilers text

ifneq ($(MAKECMDGOALS),clean)
  # Generated dependencies
  -include $(OBJECTS:.o=.d)
  ifdef VTSS_EXTERNAL_CPU
    -include $(OBJECTS_ICLI:.o=.d)
  endif
  # Standard rules
  include $(BUILD)/make/target-$(TARGET).mk
endif

# odd targets
include $(BUILD)/make/doxygen.mk
#
include $(TOPABS)/build/make/images.mk
