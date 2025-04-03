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

function isValidDomainOrServiceName(field, maxlen, what) {

    var badchars = " :";
    var i;
    if (typeof (field) == "string") {
        field = document.getElementById(field);
    }

    if (field.value.length === 0) {
        return GiveAlert(what + " cannot be empty", field);
    }

    if (field.value.length > maxlen) {
        return GiveAlert(what + " must not exceed " + maxlen + " characters", field);
    }

    if (field.value.charAt(0) == ' ' || field.value.charAt(0) == '\t' ) {
        return GiveAlert(what + " must not contain leading whitespace", field);
    }

    if (field.value.charAt(field.value.length - 1) == ' ' || field.value.charAt(field.value.length - 1) == '\t') {
        return GiveAlert(what + " must not contain trailing whitespace", field);
    }

    if ("1234567890".indexOf(field.value.charAt(0)) != -1) {
        return GiveAlert(what + " must not contain leading digit", field);
    }

    for (i = 0; i < badchars.length; i++) {
        if (field.value.indexOf(badchars.charAt(i)) != -1) {
            return GiveAlert(what + " contains one or more illegal characters  " + badchars.charAt(i), field);
        }
    }

    if (field.value.toUpperCase().includes("ALL")) {
        return GiveAlert(what + " must not include the word ALL (case insensitive)" , field);
    }

    // This check corresponds to isgraph()
    for (i = 0; i < field.value.length; i++) {
        if (field.value.charCodeAt(i) < 0x21 || field.value.charCodeAt(i) > 0x7E ) {
            return GiveAlert(what + " contains non graphical characters" , field);
        }
    }

    return true;
}
