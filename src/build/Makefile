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
.PHONY: top all clean mrproper TAGS tags ctags ID release-note ccversion copy_vtss_libs style_inplace code_style_chk jslint tidy doxygen doxygen-no-pdf show_modules show_all_modules chks $(MODULES) update-all rs1014 automatic install cloc sloccount bt install-hiawatha smoke smoke_log smoke_debug show_defines copyrights copyrights-update cdb compile_commands.json show_configs vscode_workspace coverity-first coverity-next html

# Absolute TOP (for including)
TOPABS := $(CURDIR)/..

# Relative TOP (for compiling) - absolute if WEBSTAX_OBJDIR specified
TOP := $(if $(WEBSTAX_OBJDIR),$(TOPABS),../..)

# Absolute build directory
BUILD := $(TOPABS)/build

# Object file directory
OBJ := $(if $(WEBSTAX_OBJDIR),$(WEBSTAX_OBJDIR),$(BUILD)/obj)

# Look for a local checkout of the linux sources
LINUX_SRC_PARENT_DIR ?= $(TOPABS)

# Default config file, possibly a symlink
CONFIG_FILE := $(if $(WEBSTAX_CONFIG),$(WEBSTAX_CONFIG),$(BUILD)/config.mk)

# Subtarget config file - for easy build of the same set of subtargets
SUBTARGET_CONFIG_FILE := $(if $(WEBSTAX_CONFIG),$(WEBSTAX_CONFIG),$(BUILD)/.subtargets.mk)

# Determine real name of config, possibly a link
REAL_CONFIG_FILE := $(shell readlink $(CONFIG_FILE) || echo $(CONFIG_FILE))
# And the basename
CONFIG_BASENAME  := $(basename $(notdir $(REAL_CONFIG_FILE)))

# Export these settings to child processes
export TOP TOPABS BUILD OBJ CONFIG_FILE REAL_CONFIG_FILE CONFIG_BASENAME LINUX_SRC_PARENT_DIR

-include $(CONFIG_FILE)
-include $(SUBTARGET_CONFIG_FILE)

ifeq ($(wildcard $(CONFIG_FILE)),)
$(CONFIG_FILE):
	@echo "You must configure your build target:"
	@tools/set_build_config.rb | egrep '($(MAKECMDGOALS))'
	@echo "Alternatively, add CONFIG_FILE=<path-to-config> to the make command line."
	@exit 1
endif

TARGET_LIST := $(if $(BUILD_SUBTARGETS),$(BUILD_SUBTARGETS),$(ALL_TARGETS_LIST))

# Exporting information that is need for the release procedure.
export MODULES
export VTSS_PRODUCT_MISC
export TARGET_LIST

top: all

all code_style_chk style_inplace jslint tidy doxygen doxygen-no-pdf clean compile copy_vtss_libs ccversion install linux_clone frr_clone smoke smoke_log smoke_debug show_defines list_subtargets coverity_cfg html: $(OBJ)
	$(MAKE) -C $(OBJ) -f $(BUILD)/make/compile.mk $@

# Rule to "focus" on a single module
$(MODULES): $(OBJ)
	@mkdir -p $(OBJ)
	$(MAKE) -C $(OBJ) -f $(BUILD)/make/compile.mk MODULES=$@

clang_complete:
	$(MAKE) -C $(OBJ) -f $(BUILD)/make/compile.mk clang_complete

vscode_workspace: cdb
	$(MAKE) -C $(OBJ) -f $(BUILD)/make/compile.mk $(MAKECMDGOALS)

clean_vscode_workspace:
	rm -rf $(TOPABS)/.vscode
	rm -f $(TOPABS)/*.code-workspace
	rm -f $(TOPABS)/compile_commands.json

compile_commands.json:
	@mkdir -p $(OBJ)/cdb
	@echo "skip: *" > $(OBJ)/cdb/.nsr
	COMPILE_COMMANDS=1 TOP=../../.. OBJ=$(OBJ)/cdb $(MAKE) -C $(OBJ)/cdb -f $(BUILD)/make/compile.mk compile_commands.json

cdb: compile_commands.json

ctidy: compile_commands.json
	./tools/ctidy -a -v -p obj/cdb

ctidy_git: compile_commands.json
	(./tools/ctidy -g -v -p obj/cdb)

# Make a clean coverity build and create local HTML/JSON results.
coverity-first:
	./tools/coverity/analyze.rb -D -p
	@echo "You may now copy the generated HTML/JSON files to a safe place to have something to compare with in the next iteration"

# Same as coverity-first, but doesn't do a clean build, so it's (much) faster.
# Can be used after a code update to see if it helped (compare the previous
# JSON file with the new one).
coverity-next:
	./tools/coverity/analyze.rb -D -C -p

# Create object directory, and generate a file that causes the backup process
# not to take backup of this folder and sub-folders.
$(OBJ):
	@mkdir -p $(OBJ)
	@rm -rf $(OBJ)/install
	@echo "skip: *" > $(OBJ)/.nsr
	$(MAKE) -C $(OBJ) -f $(BUILD)/make/compile.mk clang_complete

clobber: clean_vscode_workspace
#	[ -d $(OBJ) ] || mkdir -p $(OBJ)
#	$(MAKE) -C $(OBJ) -f $(BUILD)/make/compile.mk $@
	-rm -fr $(OBJ)
	-rm -f $(TOPABS)/vtss_appl/web/html/navbar.htm

mrproper: clobber
	-rm -fr config.mk $(SUBTARGET_CONFIG_FILE) TAGS $(TOPABS)/ID 

copyrights:
	@./release/copyright.rb

copyrights-update:
	@./release/copyright.rb --update

automatic:
	@mkdir -p $(OBJ)/vtss_test
	$(MAKE) -C $(OBJ) -f $(BUILD)/make/compile.mk automatic

TAGS:
	find .. -follow -regex .*\\.[chCHxX]+[x]* -print 2>/dev/null | grep -v -E "(build\/obj|.*#.*)" | etags --members -o $@ -

# If you have the old emacs ctags program, use this one
tags:
	find .. -follow -regex .*\\.[chCHxX]+[x]* -print 2>/dev/null | grep -v -E "(build\/obj|.*#.*)" | ctags --members -o $@ -

# If you have the sourceforge exuberant-ctags program, use this one
# We do it in a number of tempi to get them ordered correctly.
# The reason for redirecting errors to /dev/null is that not everybody have the
# folders for which this is done.
# The keyword "append" actually means "prepend", so do it in opposite order.
ctags:
	rm -f tags
#	ctags --recurse --append=yes ../linux-src      2>/dev/null
#	ctags --recurse --append=yes ../linux-src.orig 2>/dev/null
	ctags --recurse --append=yes ../vtss_api       2>/dev/null
	ctags --recurse --append=yes ../vtss_api.orig  2>/dev/null
	ctags --recurse --append=yes ../vtss_basics
	ctags --recurse --append=yes ../vtss_appl

ID:
	(cd $(TOPABS); mkid -s)

release-note:
	@./release/release_note.rb

rs1014:
	@./release/rs1014.rb

-include $(TOPABS)/build/make/paths-$(TARGET).mk
show_modules:
	@echo OS:                 $(VTSS_OS)
	@echo Platform:           "$(VTSS_PRODUCT_NAME)	$(VTSS_PRODUCT_NAME_GUI)	$(VTSS_PRODUCT_CHIP)	$(VTSS_PRODUCT_HW)	$(VTSS_PRODUCT_STACKABLE)"
	@echo External-toolchain: $(VTSS_EXTERNAL_BUILD_ENV_TOP)
	@echo External-mesa:      $(MESA_API_NAME)
	@echo Enabled:            $(MODULES)
	@echo Disabled:           $(MODULES_DISABLED)

show_all_modules:
	@echo AllModules: $(call AllModules)

# $1: Profile name
# $2: MFI/ITB filename
# $3: MEBA .dtso
# $4: API name
# $5: Chip name
# $6: MEBA name
# $7: DTB
define ShowOneTarget
@printf "%-30s %-20s %-20s %-20s %-20s %-20s %-20s %-20s\n" $1 $2 $3 $4 $5 $6 $7 $8

endef

show_targets:
	@echo Platform: $(VTSS_PRODUCT_NAME)
	@printf "%-30s %-20s %-20s %-20s %-20s %-20s %-20s\n" 'Profile Name' 'MFI/ITB File' 'DTSO' 'API' 'Chip' 'MEBA' 'DTB'
	@$(foreach c,$(TARGET_LIST),$(call ShowOneTarget,$c,$(cfg_tgtname_$c),$(cfg_dtso_$c),$(cfg_api_$c),$(cfg_chip_$c),$(cfg_meba_$c),$(cfg_dtb_$c)))

# $1: Target name
define ShowConfig
$(info $$(eval $$(call DefineTarget,$1,$(cfg_tgtname_$1),$(cfg_dtso_$1),$(cfg_api_$1),$(cfg_chip_$1),$(cfg_meba_$1),$(cfg_dtb_$1))))
endef

ifeq ($(VTSS_PRODUCT_CHIP),"UNDEF")
define ShowConfigTarget
$(info $$(eval $$(call linuxSwitch/Multi,$(VTSS_PRODUCT_NAME),$(TARGET),$(MSCC_SDK_ARCH))))

endef
else
define ShowConfigTarget
$(info $$(eval $$(call Setup/Switch,$(VTSS_PRODUCT_CHIP),$(VTSS_PRODUCT_HW))))
$(info $$(eval $$(call Setup/LinuxTarget,$(TARGET),$(MSCC_SDK_ARCH))))
$(info $$(eval $$(call linuxSwitch/Product,$(VTSS_ARCHITECTURE_TYPE),$(VTSS_PRODUCT_NAME),$(VTSS_PRODUCT_STACKABLE),$(TARGET))))

endef
endif

show_configs:
	@echo
	$(info # )
	$(info # Generated Configuration: $(shell date))
	$(info # )
	$(info include $$(BUILD)/make/templates/linuxSwitch.in)
	$(info )
	$(info Custom/Defines          := $(Custom/Defines))
	$(info Custom/OmitModules      := $(Custom/OmitModules))
	$(info Custom/AddModules       := $(Custom/AddModules))
	$(info )
	$(info $(foreach t,$(TARGET_LIST),$(call ShowConfig,$t)))
	$(info )
	$(call ShowConfigTarget)
	$(info )
	$(info $$(eval $$(call linuxSwitch/Build)))
	$(info )
	$(info # Configuration Summary)
	$(info # VTSS_PRODUCT_CHIP           = $(VTSS_PRODUCT_CHIP))
	$(info # VTSS_PRODUCT_HW             = $(VTSS_PRODUCT_HW))
	$(info # TARGET                      = $(TARGET))
	$(info # MSCC_SDK_ARCH               = $(MSCC_SDK_ARCH))
	$(info # VTSS_ARCHITECTURE_TYPE      = $(VTSS_ARCHITECTURE_TYPE))
	$(info # VTSS_PRODUCT_STACKABLE      = $(VTSS_PRODUCT_STACKABLE))
	$(info # VTSS_PRODUCT_NAME           = $(VTSS_PRODUCT_NAME))
	$(info # VTSS_PRODUCT_NAME_GUI       = $(VTSS_PRODUCT_NAME_GUI))
	$(info # VTSS_PRODUCT_DESC_GUI       = $(VTSS_PRODUCT_DESC_GUI))
	$(info # VTSS_SOFTWARE_ID            = $(VTSS_SOFTWARE_ID))
	$(info # MSCC_SDK_FLAVOR             = $(MSCC_SDK_FLAVOR))
	$(info # VTSS_EXTERNAL_BUILD_ENV_TOP = $(VTSS_EXTERNAL_BUILD_ENV_TOP))
	$(info # MESA_API_NAME               = $(MESA_API_NAME))
	$(info # API_BUILD_PATH              = $(API_BUILD_PATH))
	$(info # TOPABS                      = $(TOPABS))
	$(info # REAL_CONFIG_FILE            = $(REAL_CONFIG_FILE))
	$(info # TARGET_LIST                 = $(TARGET_LIST))
	$(info # BUILD_SUBTARGETS            = $(BUILD_SUBTARGETS))
	$(info )
	$(info # DEFINES                     = $(DEFINES))
	$(info )
	$(info # MODULES                     = $(MODULES))
	$(info )
	$(info # MODULES_DISABLED            = $(MODULES_DISABLED))
	$(info )

chks:
	@$(MAKE) --quiet tidy code_style_chk jslint hdr_check smilint copyrights

style:
	@$(MAKE) --quiet code_style_chk

hdr_check:
	$(MAKE) -C $(OBJ) -f $(BUILD)/make/compile.mk check_public_headers_appl

licenses:
	@./release/licenses.rb

licenses_check:
	@./release/licenses.rb --check

cloc:
	@$(MAKE) -C $(OBJ) -f $(BUILD)/make/cloc.mk cloc

sloccount:
	$(MAKE) -C $(OBJ) -f $(BUILD)/make/cloc.mk sloccount

smilint:
	(cd $(BUILD)/../vtss_appl/snmp/mibs; ruby vtss-mib-lint.rb VTSS-*-MIB.mib)

#
# Installing Hiawatha following the instructions from : https://www.hiawatha-webserver.org/howto/compilation_and_installation
#
# We use our configuration to overwrite the default hiawatha configuration file (The "cp $(BUILD)/../vtss_appl/web/misc/hiawatha.conf /usr/local/etc/hiawatha;")
#
install-hiawatha:
	@if hiawatha -v >/dev/null 2>&1;\
          then echo "Hiawatha already in installed"; \
        else \
          if [ `whoami` != "root" ];\
            then >&2 echo "ERROR: You must be root - you are `whoami`"; \
          else\
            apt-get install libc6-dev libssl-dev dpkg-dev debhelper fakeroot libxml2-dev libxslt1-dev;\
	    wget https://www.hiawatha-webserver.org/files/hiawatha-9.13.tar.gz --directory-prefix=$(OBJ);\
	    tar -xzf $(OBJ)/hiawatha-9.13.tar.gz -C $(OBJ); \
            cd $(OBJ)/hiawatha-9.13;\
	    mkdir -p build;\
            cd build;\
            cmake ..;\
	    make;\
            make install/strip;\
            cp $(BUILD)/../vtss_appl/web/misc/hiawatha.conf /usr/local/etc/hiawatha;\
	  fi;\
	fi

test_vtss_basics:
	./make/run_vtss_basics_tests.sh

bt:
	if [ "$(VTSS_OS)" = "Linux" ]; then \
	   export LD_LIBRARY_PATH=/opt/vtss-cross-ecos-mips32-24kec-v2/bin/mipsel-vtss-elf-gdb;\
	   cd $(BUILD)/obj; ruby $(BUILD)/tools/chk_scripts/vtss-backtrace-resolve --gdb /opt/vtss-cross-ecos-mips32-24kec-v2/bin/mipsel-vtss-elf-gdb --elf *.elf --debug ;\
	else \
	   cd $(BUILD)/obj; ruby $(BUILD)/tools/chk_scripts/vtss-backtrace-resolve --elf *.elf --debug ;\
	fi

help:
	@echo "----- Targets -----"
	@echo "ID                     : Generate id file with mkid"
	@echo "TAGS                   : Generate etags"
	@echo "archive                : Make library archives for modules that support this"
	@echo "automatic              : Build deployment package for automatic software test"
	@echo "chks                   : Runs all the four code checks above"
	@echo "clang_complete         : Create a .clang_complete files which cab be used to add clang based autocompletion in your favourite editor."
	@echo "clean                  : Remove all obj files"
	@echo "clobber                : Remove all obj files"
	@echo "clean_vscode_workspace : Remove all Visual Studio Code workspace files"
	@echo "code_style_chk         : Check files for Vitesse Code Style. CLEANUP=0 - Leave temp. files for user to see what the issues were"
	@echo "compile                : Compile the project"
	@echo "copyrights             : Checks if copyrights headers are correct for all files changed since last release."
	@echo "copyrights-update      : Checks if copyrights headers are correct and attempts to update those that are not."
	@echo "doxygen-no-pdf         : Checks that the API and Application is documented with doxygen comments, but does not generate pdf files"
	@echo "doxygen                : Checks that the API and Application is documented with doxygen comments"
	@echo "licenses               : Generate licenses_<arch>.txt."
	@echo "licenses_check         : Check that licenses_<arch>.txt are up-to-date."
	@echo "hdr_check              : Check that all public headers are self-contained."
	@echo "jslint                 : Do java script lint"
	@echo "linux_clone            : Clone the linux kernel into this workspace use it instead of the kernel provided by the SDK (only for ADT builds)"
	@echo "frr_clone              : Clone the FRR (Free Range Routing) into this workspace use it instead of the package normally appended to the firmware image"
	@echo "<target>.mfi           : For a multi-target configuration, build only that target. See also target above"
	@echo "<target>.elf           : For a multi-target configuration, build only that target. See also target above"
	@echo "mrproper               : Removing all obj files + the config.mk and .subtargets.mk files"
	@echo "release-note           : Generate release-note"
	@echo "rs1014                 : Checks if the features defined in RS1014 match the build system and generates documentation"
	@echo "show_modules           : Shows the modules for the selected configuration"
	@echo "show_targets           : Shows the detailed target information for the selected configuration"
	@echo "smilint                : Run smilint (and check for VTSS conventions) on the MIBs checked-in"
	@echo "style                  : Alias for 'code_style_chk'"
	@echo "style_inplace          : Check files for style. Modify files themselves"
	@echo "test_vtss_basics       : Run the unit tests in vtss_basics"
	@echo "tidy                   : Check HTML syntax"
	@echo "vscode_workspace       : Create a Visual Studio Code workspace for the WebStaX source. This also creates a clang_complete database"
	@echo "bt                     : Call back-trace program for debugging exceptions etc."
	@echo "smoke                  : Run smoke-test locally - ram-loads image found in build/obj, and run testcase(s). Use M to select either module or specific testcase."
	@echo "smoke_log              : Show the log result of the last smoke-testcase that has run."
	@echo "smoke_debug            : Run a smoketest 'outside' the framework and stops a first error found. Use for debuging using e.g. pry. Use M=xyz to select which testcase to run."
	@echo ""
	@echo ""
	@echo "install_hiawatha       : If using external CPU and Ubuntu you need to have Hiawatha webserver intalled. install_hiawatha does it for you.\n\
	                  The default Hiawatha configuration file is overwritten with the configuration file used in WebStax"
	@echo ""
	@echo "install                : Installs application and webpages for the system. For Ubuntu systems you can change the default web page path (/var/www/webstax/), by setting WEBSITEROOT=your-path"
	@echo ""

# On WSL, symlinks work in Linux, but not Windows, so if your Web-server is a
# Windows app, you cannot get new.mfi or new.itb when these are symlinks, so
# copying them instead.
define copy-to-dir-and-link
	$(Q)if [ ! -z $(USER) ] ; then					\
	    if [ -d $1 ] ; then						\
	        cp -v obj/$2 $1;					\
		if grep -i microsoft /proc/version > /dev/null ; then	\
	            cp -v obj/$2 $1/$3;					\
		else							\
	            ln -sf $2 $1/$3;					\
	            echo "Linked $1/$3 => $1/$2";			\
		fi;							\
	        chmod 664 $1/$2 $1/$3;					\
	    fi;								\
	fi
endef
