// * -*- Mode: java; c-basic-offset: 4; tab-width: 8; c-comment-only-line-offset: 0; -*-
/*

 Copyright (c) 2006-2018 Microsemi Corporation "Microsemi". All Rights Reserved.

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
/*
Description : Functions that formats strings.
*/


//
// Formats a string to XX-XX-XX-XX-XX-XX.
//
function toMacAddress(Value, AlertOn) {

    // Default value
    var  AlertOn = (AlertOn == null) ? 1 : AlertOn;

    if (!IsMacAddress(Value, AlertOn)) {
        return "00-00-00-00-00-00";
    }

    var MACAddr = Array();

    // Split the max address up in 6 part ( Allowed format is 00-11-22-33-44-55 or 001122334455 )
    if (Value.indexOf("-") != -1) {
        MACAddr = Value.split("-");
    } else {
        for (var j = 0; j <= 5; j++) {
            MACAddr[j] = Value.substring(j*2, j*2+2);
        }
    }

    // Generate return var with format XX-XX-XX-XX-XX-XX
    var ReturnMacAddr = new String;
    for (var i = 0; i <= 5; i++) {
        // Pad for 0 in case of MAC address format ( 1-2-23-44-45-46 )
        if (MACAddr[i].length == 1) {
            MACAddr[i] = "0" + MACAddr[i];
        }

        if (i == 5) {
            ReturnMacAddr += MACAddr[i];
        } else {
            ReturnMacAddr += MACAddr[i] + "-";
        }
    }

    return ReturnMacAddr.toUpperCase();
}

//
// Formats a string to XX-XX-XX.
//
function toOuiAddress(Value) {

    if (!IsOuiAddress(Value, 1)) {
        return "00-00-00";
    }

    var OuiAddr = Array();

    // Split the max address up in 3 part ( Allowed format is 00-11-22 or 001122 )
    if (Value.indexOf("-") != -1) {
        OuiAddr = Value.split("-");
    } else {
        for (var j = 0; j <= 2; j++) {
            OuiAddr[j] = Value.substring(j*2, j*2+2);
        }
    }

    // Generate return var with format XX-XX-XX
    var ReturnOuiAddr = new String;
    for (var i = 0; i <= 2; i++) {
        // Pad for 0 in case of MAC address format ( 1-2-23 )
        if (OuiAddr[i].length == 1) {
            OuiAddr[i] = "0" + OuiAddr[i];
        }

        if (i == 2) {
            ReturnOuiAddr += OuiAddr[i];
        } else {
            ReturnOuiAddr += OuiAddr[i] + "-";
        }
    }

    return ReturnOuiAddr.toUpperCase();
}

//
// Convert a string to another string with its ASCII codes (leading with '0x').
//
function textStrToAsciiStr(sText)
{
    var idx, nmx, ret = "";

    if (sText === null) {
        return ret;
    }

    for (idx = 0; idx < sText.length; idx++) {
        nmx = sText.charCodeAt(idx);
        ret = ret + "0x" + nmx.toString(16);
    }

    return ret;
}

//
// Convert a string in ASCII codes (with leading '0x') to another textual string.
//
function asciiStrToTextStr(sAscii)
{
    var idx, nmx, ret = "";

    if (sAscii === null) {
        return ret;
    }

    nmx = sAscii.split("0x");
    if (nmx.length > 0) {
        for (idx = 1; idx < nmx.length; idx++) {
            ret = ret + String.fromCharCode(parseInt(nmx[idx], 16))
        }
    }

    return ret;
}

/* Convert number to string with specific digits(e.g., 2 to "02", or 5 to "0005").
 *
 * @number:     [Mandatory], the number which wants to be converted.
 * @length:     [Mandatory], specify the digits for the number.
 */
function pad(number, length)
{
    var str = '' + number;
    while (str.length < length) {
        str = '0' + str;
    }

    return str;

}

/* format seconds to stirng with "d hh:mm:ss" format.
 *
 * @seconds:     [Mandatory], the seconds which want to be converted.
 */
function secondsToDays(seconds)
{
    var days = Math.floor(seconds / 86400); // 86400 = 60 * 60 * 24
    var hrs   = Math.floor((seconds % 86400) / 3600); // 3600 = 60 * 60
    var mins = Math.floor(((seconds % 86400) % 3600) / 60);
    var secs = Math.floor((((seconds % 86400) % 3600) % 60));
  return (days ? (days + "d "): "") + pad(hrs, 2) + ":" + pad(mins, 2) + ":" + pad(secs, 2);
}
