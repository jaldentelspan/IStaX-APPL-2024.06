// * -*- Mode: java; tab-width: 8; -*-
/*

 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

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

requestUpdateWhenDone.timeleft = 0;
requestUpdateWhenDone.pendingRequests = 0;

function requestUpdateWhenDone(maxMillisecs)
{
    granularity = 50; // Milliseconds
    if (typeof requestUpdateWhenDone.timeleft == undefined) {
        // initialisation only
        requestUpdateWhenDone.timeleft = 0;
    }
    if (maxMillisecs === undefined) {
        // We get here if requestUpdateWhenDone is called without maxMillisecs parameter, i.e. by its own call to setTimeout
        requestUpdateWhenDone.timeleft -= granularity;
    } else {
        requestUpdateWhenDone.timeleft = maxMillisecs;
    }

    if (requestUpdateWhenDone.pendingRequests <= 0 || requestUpdateWhenDone.timeleft <= 0) {
        // we are done
        console.log("requestUpdateWhenDone. pending: %d timeleft %d", requestUpdateWhenDone.pendingRequests, requestUpdateWhenDone.timeleft );
        requestUpdate();
    } else {
        // go again
        setTimeout(requestUpdateWhenDone, granularity);
    }
}

function resetPending()
{
    requestUpdateWhenDone.pendingRequests = 0;
}

function incrementPending()
{
    requestUpdateWhenDone.pendingRequests++;
}

function decrementPending()
{
    requestUpdateWhenDone.pendingRequests--;
}

function stringToBits(val, bits) {
    var i;
    for (i = 0; i < 8; i ++) {
        bits[i] = val & 0x1;
        val = val >> 1;
    }
    return bits;
}
