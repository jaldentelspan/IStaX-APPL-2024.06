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
.PHONY: doxygen doxygen_app doxy-file-target doxygen-no-pdf

DOXYGEN               := doxygen
DOXYGEN_OUTPUT_FOLDER := $(OBJ)/doxygen
# Remove -include statements from $(INCLUDES), since they'll confuse Doxygen.
INCLUDES_WITHOUT_INCLUDE = $(filter-out -include %.h,$(INCLUDES))
DOXYGEN_INCLUDES      = $(patsubst -I%,%,$(INCLUDES_WITHOUT_INCLUDE))
DOXYGEN_DEFINES       = $(patsubst -D%,%,$(DEFINES))

doxygen: doxy-file-target doxygen_app
doxygen-no-pdf: doxy-file-target doxygen_app-no-pdf

# There is a bug in pdflatex that shows itself whenever a
# hyperlink is split across two pages. An example is very long
# enums (e.g. vtss_target_type_t), that are likely to get split.
# To overcome this, a page break is inserted just before the
# troublesome construct directly in the generated tex file.
# When the bug appears, "make" returns Error 2 from "make doxygen".
# Have a look at refman.log to find the page in the PDF (that doesn't get
# created) on which the error occurs. Then create the PDF by using
# the following construct in refman.tex: \usepackage[...,draft,...]{hyperref}
# and look at the generated PDF. Once found, add a rule in the
# insert_pagebreak() "function" below, that inserts \newpage
# at the beginning of the line.
# Useful links:
#   How to debug the problem:                http://tug.org/pipermail/pdftex/2002-February/002216.html
#   How to insert a page break in Tex files: http://stackoverflow.com/questions/22601053/pagebreak-in-markdown-while-creating-pdf
define insert_pagebreak
	$(Q)if [ -f $1/latex/vtss__init__api_8h.tex ]; then sed -i -e "s|^\(enum.*vtss\\\\-\\\\_\\\\-target\\\\-\\\\_\\\\-type\\\\-\\\\_\\\\-t\}.*\)|\\\\newpage \1|" $1/latex/vtss__init__api_8h.tex; fi;
endef

define doxy-doit
	$(Q)@mkdir -p $5
	$(Q)$(DOXYGEN) -s -g $6 >/dev/null
	$(Q)sed -i -e "s|\(\bPROJECT_NAME\b.*=\).*|\1 \"MicroSemi $(7) API\"|"    $6
	$(Q)sed -i -e "s|\(\bINPUT\b.*=\).*|\1 $2|"                        $6
	$(Q)sed -i -e 's|\(\bPREDEFINED\b.*=\).*|\1 $3|'                   $6
	$(Q)sed -i -e 's|\(\bINCLUDE_PATH\b.*=\).*|\1 $4|'                 $6
	$(Q)sed -i -e "s|\(\bOUTPUT_DIRECTORY\b.*=\).*|\1 $5|"             $6
	$(Q)sed -i -e "s|\(\bFULL_PATH_NAMES\b.*=\).*|\1 YES|"             $6
	$(Q)sed -i -e "s|\(\bSTRIP_FROM_PATH\b.*=\).*|\1 $(OBJ)/../..|"    $6
	$(Q)sed -i -e "s|\(\bWARN_NO_PARAMDOC\b.*=\).*|\1 YES|"            $6
	$(Q)sed -i -e "s|\(\bOPTIMIZE_OUTPUT_FOR_C\b.*=\).*|\1 YES|"       $6
	$(Q)sed -i -e "s|\(\bSOURCE_BROWSER\b.*=\).*|\1 YES|"              $6
	$(Q)sed -i -e "s|\(\bDISTRIBUTE_GROUP_DOC\b.*=\).*|\1 YES|"        $6
	$(Q)sed -i -e "s|\(\bPDF_HYPERLINKS\b.*=\).*|\1 YES|"              $6
	$(Q)sed -i -e "s|\(\bUSE_PDFLATEX\b.*=\).*|\1 YES|"                $6
	$(Q)sed -i -e "s|\(\bLATEX_BATCHMODE\b.*=\).*|\1 YES|"             $6
	$(Q)sed -i -e "s|\(\bEXTRACT_ALL\b.*=\).*|\1 NO|"                  $6
	$(Q)sed -i -e "s|\(\bSORT_MEMBER_DOCS\b.*=\).*|\1 NO|"             $6
	$(Q)sed -i -e "s|\(\bDOT_GRAPH_MAX_NODES\b.*=\).*|\1 200|"         $6
	$(call what,Doxygen $1 - Processing source files)
	$(Q)$(DOXYGEN) $6 > /dev/null


endef

define doxy-pdf
	$(call what,Doxygen $1 - PDF processing of doxygen output)
	$(call insert_pagebreak,$5)
	$(Q)sed -i -- "s/latex_count=[0-9]\{1,\}/latex_count=40/g" $5/latex/Makefile # Seen issue with the the Makefile for Latex just exits with error code 2 (no other indication of what is wrong). Changing the latex count is the fix.
	$(Q)$(MAKE) -C $5/latex 1>/dev/null 2>/dev/null
        $(Q)cp $5/latex/refman.pdf $5/latex/$7_api.pdf # Rename pdf file in order to distinligsh between Chip and Application APIs
endef

define doxy-target-no-pdf
$1:
	$$(call doxy-doit,$1,$2,$3,$4,$(DOXYGEN_OUTPUT_FOLDER)/$1,$(DOXYGEN_OUTPUT_FOLDER)/$1.cfg,$5)
endef

define doxy-target
$1:
	$$(call doxy-doit,$1,$2,$3,$4,$(DOXYGEN_OUTPUT_FOLDER)/$1,$(DOXYGEN_OUTPUT_FOLDER)/$1.cfg,$5)
	$$(call doxy-pdf,$1,$2,$3,$4,$(DOXYGEN_OUTPUT_FOLDER)/$1,$(DOXYGEN_OUTPUT_FOLDER)/$1.cfg,$5)
endef

# In order for doxygen to give warnings about un-documented functions, it is required that the file is doxygen documented,
# but hey, I can't figure out how to get doxygen to give a warning if a file in not doxygen documented, so therefore I
# have made the work-around below for finding files that are not documented.
define file_desc_chk
ifneq ($(strip $1),)
$$(warning warning: The following files do not contain file doxygen documentation: $1)
endif

endef

doxy-file-target:
	$(eval $(call file_desc_chk,$(shell grep -L '\\file' $(TOP)/vtss_appl/include/vtss/appl/*.h)))

$(eval $(call doxy-target,doxygen_app,$(PUBLIC_HEADERS),$(DOXYGEN_DEFINES),$(TOP)/vtss_appl/include/vtss/appl,"Application"))

$(eval $(call doxy-target-no-pdf,doxygen_app-no-pdf,$(PUBLIC_HEADERS),$(DOXYGEN_DEFINES),$(TOP)/vtss_appl/include/vtss/appl,"Application"))
