// * -*- Mode: java; tab-width: 8; -*-
/*

 Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.

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

/**
 * \file frr_ospf_lib.js
 * \brief This file contains the Javascript common utilities for FRR OSPF module.
 */

/**
 * restoreFiledValue(): Restore HTML filed value with the original setting
 *
 * @param  [Mandatory] html_field_id : The HTML filed ID.
 * @param  [Mandatory] ref_db_name   : The reference database name.
 * @param  [Mandatory] keys          : The entry keys in the database.
 * @param  [Mandatory] param_name    : The parameter name in the entry data.
 */
function restoreFiledValue(html_field_id, ref_db_name, keys, param_name) {

    var orignalConfig = myDynamicTable.getRecvJson(ref_db_name);
    orignalConfig.each(function(row) {
        if (rowKey2Id(row.key) == keys) {
            // Found the orignal row
            $(html_field_id).value = row.val[param_name];
        }
    });
}





