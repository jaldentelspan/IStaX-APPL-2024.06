/*

 Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_ERROR_CODE_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_ERROR_CODE_HXX__

#include "vtss/basics/stream.hxx"

namespace vtss {
namespace expose {
namespace snmp {

namespace ErrorCode {
    enum E {
        // No error occurred. This code is also used in all request PDUs, since
        // they have no error status to report.
        noError = 0,

        // The size of the Response-PDU would be too large to transport.
        tooBig = 1,

        // The name of a requested object was not found.
        noSuchName = 2,

        // A value in the request didn't match the structure that the recipient
        // of the request had for the object. For example, an object in the
        // request was specified with an incorrect length or type.
        badValue = 3,

        // An attempt was made to set a variable that has an Access value
        // indicating that it is read-only.
        readOnly = 4,

        // An error occurred other than one indicated by a more specific error
        // code in this table.
        genErr = 5,

        // Access was denied to the object for security reasons.
        noAccess = 6,

        // The object type in a variable binding is incorrect for the object.
        wrongType = 7,

        // A variable binding specifies a length incorrect for the object.
        wrongLength = 8,

        // A variable binding specifies an encoding incorrect for the object.
        wrongEncoding = 9,

        // The value given in a variable binding is not possible for the object.
        wrongValue = 10,

        // A specified variable does not exist and cannot be created.
        noCreation = 11,

        // A variable binding specifies a value that could be held by the
        // variable but cannot be assigned to it at this time.
        inconsistentValue = 12,

        // An attempt to set a variable required a resource that is not
        // available.
        resourceUnavailable = 13,

        // An attempt to set a particular variable failed.
        commitFailed = 14,

        // An attempt to set a particular variable as part of a group of
        // variables failed, and the attempt to then undo the setting of other
        // variables was not successful.
        undoFailed = 15,

        // A problem occurred in authorization.
        authorizationError = 16,

        // The variable cannot be written or created.
        notWritable = 17,

        // The name in a variable binding specifies a variable that does not
        // exist.
        inconsistentName = 18
    };

    ostream& operator<<(ostream &o, E e);
};  // namespace ErrorCode

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_ERROR_CODE_HXX__
