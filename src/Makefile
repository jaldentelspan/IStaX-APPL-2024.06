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

.PHONY: all compile co_make_hw

all:
	@echo "To compile, change directory to the 'build' directory"

compile:
	$(MAKE) -C build

.PHONY: co_make_hw
co_make_hw:
	mkdir -p src/ts
	mkdir -p src/testbench
	touch .top
	cd src/testbench; \
	cvs co -P exb_jaguar_tcllib ;\
	cvs co -P exb_heathrow_tcllib ;\
	cvs co -P exb_fg_tcllib ;\
	cvs co -P exb_basic_tcllib ;\
	cvs co -P exb_sw_tcllib ;\
	cvs co -P exb_basic_post_tcllib ;\
	cvs co -P exb_jaguar_tcllib ;\
	cvs co -P sw_postsil_tcl_lib ;
	cd src/ts; \
	cvs co -P exb_webstax_test 
	$(call do_co,tools)
	echo "export EXB_DESIGN_NAME := jaguar" > src/exb_design_name.mk



