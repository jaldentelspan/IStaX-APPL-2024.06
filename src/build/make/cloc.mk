########################################################-*- mode: Makefile -*-
#
# Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.
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
# ------------------------------------------------------------------------

# -----------------oOo-----------------
# cloc is the tool that provides the best output.
# sloccount is just for reference.

# -----------------oOo-----------------
ifneq ($(V),)
  what =
  Q    =
else
  what = @echo $1
  Q    = @
endif

# $1 is the base-dir, e.g. $(TOPABS)/vtss_appl
define dirs
$(sort $(filter-out $(notdir $1),$(shell find $1 -maxdepth 1 -type d -printf "%f\n")))
endef

# $1 is list of dirs to exclude $2 from. $3 is a list of dirs to add instead (optional)
define rm_add
$(sort $(filter-out $2,$1) $3)
endef

# "cloc" is the preferred method since there might be problems with "sloccount" due to
# its way of organizing folders of the same name.
CLOC_OUT      := $(TOPABS)/build/obj/cloc.txt
SLOCCOUNT_OUT := $(TOPABS)/build/obj/sloccount.txt

# $1: heading (e.g. vtss_appl), $2: Files
define sloccount_doit
	$(Q)echo >> $(SLOCCOUNT_OUT)
	$(Q)echo $1: >> $(SLOCCOUNT_OUT)
	$(Q)echo Processing $1
	$(Q)sloccount $2 >> $(SLOCCOUNT_OUT)
	$(Q)echo
endef

CLOC_OPTS := --force-lang="C/C++ Header,hxx" --force-lang="C,icli" --force-lang="C,inc" --force-lang="YAML,module_info" --exclude-ext="adoc,buildpath,conf,ctx,diff,gemspec,gitignore,htaccess,json,json_history,log,md,mib,mib_history,project,rdoc,stm,svg,vtss"
# $1: heading (e.g. vtss_appl), $2: Files
define cloc_doit
	$(Q)echo >> $(CLOC_OUT)
	$(Q)echo $1: >> $(CLOC_OUT)
	$(Q)echo Processing $1
	$(Q)cloc $2 $(CLOC_OPTS) --ignored $(TOPABS)/build/obj/cloc.$1.ignored --out $(TOPABS)/build/obj/cloc.$1.txt
	$(Q)cat $(TOPABS)/build/obj/cloc.$1.txt >> $(CLOC_OUT)
	$(Q)echo
endef

CODE_VERSION := $(shell $(BUILD)/tools/code_version)

# -----------------oOo-----------------
# vtss_appl
DIR_appl   := $(TOPABS)/vtss_appl
FILES_appl := $(call dirs,$(DIR_appl),vtss_appl)
FILES_appl := $(call rm_add,$(FILES_appl),md5)
FILES_appl := $(call rm_add,$(FILES_appl),tacplus)
FILES_appl := $(call rm_add,$(FILES_appl),upnp,upnp/platform)
FILES_appl := $(call rm_add,$(FILES_appl),zls,zls/platform)
FILES_appl := $(addprefix $(DIR_appl)/,$(FILES_appl))

# -----------------oOo-----------------
# vtss_api
DIR_api   := $(TOPABS)/vtss_api
FILES_api := $(call dirs,$(DIR_api))
FILES_api := $(call rm_add,$(FILES_api),.cmake)
FILES_api := $(call rm_add,$(FILES_api),appl)
FILES_api := $(call rm_add,$(FILES_api),doc)
FILES_api := $(call rm_add,$(FILES_api),third_party)
FILES_api := $(addprefix $(DIR_api)/,$(FILES_api))

# -----------------oOo-----------------
# vtss_basics
DIR_basics   := $(TOPABS)/vtss_basics
FILES_basics := $(call dirs,$(DIR_basics))
FILES_basics := $(call rm_add,$(FILES_basics),test)
FILES_basics := $(addprefix $(DIR_basics)/,$(FILES_basics))

# -----------------oOo-----------------
# test
DIR_test   := $(TOPABS)/vtss_test
FILES_test := $(call dirs,$(DIR_test))
FILES_test := $(addprefix $(DIR_test)/,$(FILES_test))

# -----------------oOo-----------------

sloccount:
	$(Q)echo SLOCCOUNT report based on the following code revision ID: $(CODE_VERSION) > $(SLOCCOUNT_OUT)
	$(call sloccount_doit,vtss_appl,$(FILES_appl))
	$(call sloccount_doit,vtss_api,$(FILES_api))
	$(call sloccount_doit,vtss_basics,$(FILES_basics))
	$(call sloccount_doit,vtss_test,$(FILES_test))
	$(call sloccount_doit,vtss_appl+vtss_api+vtss_basics+vtss_test,$(FILES_appl) $(FILES_api) $(FILES_basics) $(FILES_test))
	$(Q)echo "Generated $(SLOCCOUNT_OUT)."

cloc:
	$(Q)echo CLOC report based on the following code revision ID: $(CODE_VERSION) > $(CLOC_OUT)
	$(call cloc_doit,vtss_appl,$(FILES_appl))
	$(call cloc_doit,vtss_api,$(FILES_api))
	$(call cloc_doit,vtss_basics,$(FILES_basics))
	$(call cloc_doit,vtss_test,$(FILES_test))
	$(call cloc_doit,vtss_appl+vtss_api+vtss_basics+vtss_test,$(FILES_appl) $(FILES_api) $(FILES_basics) $(FILES_test))
	$(Q)echo "Generated $(CLOC_OUT). Ignored files can be found in $(TOPABS)/build/obj/cloc.*.ignored"

