/*
 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

 Unpublished rights reserved under the copyright laws of the United States of
 America, other countries and international treaties. Permission to use, copy,
 store and modify, the software and its source code is granted but only in
 connection with products utilizing the Microsemi switch and PHY products.
 Permission is also granted for you to integrate into other products, disclose,
 transmit and distribute the software only in an absolute machine readable
 format (e.g. HEX file) and only in or with products utilizing the Microsemi
 switch and PHY products.  The source code of the software may not be
 disclosed, transmitted or distributed without the prior written permission of
 Microsemi.

 This copyright notice must appear in any copy, modification, disclosure,
 transmission or distribution of the software.  Microsemi retains all
 ownership, copyright, trade secret and proprietary rights in the software and
 its source code, including all modifications thereto.

 THIS SOFTWARE HAS BEEN PROVIDED "AS IS". MICROSEMI HEREBY DISCLAIMS ALL
 WARRANTIES OF ANY KIND WITH RESPECT TO THE SOFTWARE, WHETHER SUCH WARRANTIES
 ARE EXPRESS, IMPLIED, STATUTORY OR OTHERWISE INCLUDING, WITHOUT LIMITATION,
 WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR USE OR PURPOSE AND
 NON-INFRINGEMENT.
*/

#include <vtss/basics/enum-descriptor.h>
#include <vtss/appl/ipmc_lib.h>

vtss_enum_descriptor_t ipmc_lib_expose_querier_state_txt[] {
    {VTSS_APPL_IPMC_LIB_QUERIER_STATE_DISABLED, "disabled"},
    {VTSS_APPL_IPMC_LIB_QUERIER_STATE_INIT,     "initial"},
    {VTSS_APPL_IPMC_LIB_QUERIER_STATE_IDLE,     "idle"},
    {VTSS_APPL_IPMC_LIB_QUERIER_STATE_ACTIVE,   "active"},
    {0, 0}
};

#ifdef VTSS_SW_OPTION_SMB_IPMC
vtss_enum_descriptor_t ipmc_lib_expose_filter_mode_txt[] {
    {VTSS_APPL_IPMC_LIB_FILTER_MODE_EXCLUDE, "exclude"},
    {VTSS_APPL_IPMC_LIB_FILTER_MODE_INCLUDE, "include"},
    {0, 0}
};
#endif /* VTSS_SW_OPTION_SMB_IPMC */

