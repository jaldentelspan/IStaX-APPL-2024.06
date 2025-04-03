#!/bin/sh
##########################################################################
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
# ------------------------------------------------------------------------
LOGOUTPUT='/dev/null'

# 10G:
# tx2rx_lp_en = 1
# txlb_en = 1
symreg_sparx5 SD10G_LANE_LANE_06 0x8    > $LOGOUTPUT
symreg_sparx5 SD10G_LANE_LANE_0E 0x68   > $LOGOUTPUT
symreg_sparx5 SD10G_LANE_LANE_83 0x70   > $LOGOUTPUT

# r_bist_en = 1
# r_free_run_mode = 1
symreg_sparx5 SD10G_LANE_LANE_76 0x5    > $LOGOUTPUT
# r_bist_chk_zero = 1

# r_bist_chk = 1
symreg_sparx5 SD10G_LANE_LANE_77 0x3    > $LOGOUTPUT

# Check BIST status
echo CHECKING RESULT FOR 10G. ALL VALUES MUST BE 0x1
for i in 0 1 2 3 4 5 6 7 8 9 10 11
do
symreg_sparx5 SD10G_LANE_LANE_E0[$i].BIST_RUN
symreg_sparx5 SD10G_LANE_LANE_E0[$i].BIST_OK
done
echo CHECKING DONE FOR 10G.

# r_bist_en = 0
# r_free_run_mode = 0
symreg_sparx5 SD10G_LANE_LANE_76 0x0    > $LOGOUTPUT
# r_bist_chk = 0
symreg_sparx5 SD10G_LANE_LANE_77 0x0    > $LOGOUTPUT

# tx2rx_lp_en = 0
# txlb_en = 0
symreg_sparx5 SD10G_LANE_LANE_06 0x0    > $LOGOUTPUT
symreg_sparx5 SD10G_LANE_LANE_0E 0x48   > $LOGOUTPUT

# 25G:
# tx2rx_lp_en = 1
# txlb_en = 1
symreg_sparx5 SD25G_LANE_CMU_FF                    0x0 > $LOGOUTPUT
symreg_sparx5 SD25G_LANE_LANE_04.LN_CFG_TX2RX_LP_EN  1 > $LOGOUTPUT
symreg_sparx5 SD25G_LANE_LANE_40.LN_R_RX_POL_INV     0 > $LOGOUTPUT

symreg_sparx5 SD25G_LANE_CMU_FF                    0x0 > $LOGOUTPUT

# r_free_run_mode = 1
symreg_sparx5 SD25G_LANE_LANE_34.LN_R_FREE_RUN_MODE  1 > $LOGOUTPUT
# r_bist_en = 1
symreg_sparx5 SD25G_LANE_LANE_33                   0x1 > $LOGOUTPUT
# r_bist_chk_zero = 1
symreg_sparx5 SD25G_LANE_LANE_34.LN_R_BIST_CHK_ZERO  1 > $LOGOUTPUT

symreg_sparx5 SD25G_LANE_CMU_FF                    0x0 > $LOGOUTPUT
# r_bist_chk = 1
symreg_sparx5 SD25G_LANE_LANE_34.LN_R_BIST_CHK       1 > $LOGOUTPUT
symreg_sparx5 SD25G_LANE_CMU_FF                    0x0 > $LOGOUTPUT

sleep 0.01

# Check BIST status
echo CHECKING STATUS FOR 25G. ALL VALUES MUST BE 0x1 
for i in 0 1 2 3 4 5 6 7 
do
symreg_sparx5 SD25G_LANE_LANE_E0[$i].LN_BIST_RUN
symreg_sparx5 SD25G_LANE_LANE_E0[$i].LN_BIST_OK
done
echo CHECKING DONE FOR 25G.

# r_bist_en = 0
symreg_sparx5 SD25G_LANE_LANE_33                  0x0  > $LOGOUTPUT
# r_free_run_mode = 0
# r_bist_chk_zero = 0
# r_bist_chk = 0
symreg_sparx5 SD25G_LANE_LANE_34                  0x0  > $LOGOUTPUT

# tx2rx_lp_en = 0
# txlb_en = 0
symreg_sparx5 SD25G_LANE_LANE_04.LN_CFG_TX2RX_LP_EN 0  > $LOGOUTPUT
symreg_sparx5 SD25G_LANE_LANE_40.LN_R_RX_POL_INV    1  > $LOGOUTPUT
