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

#ifndef __VTSS_BASICS_EXPOSE_JSON_SERIALIZE_ENUM_HXX__
#define __VTSS_BASICS_EXPOSE_JSON_SERIALIZE_ENUM_HXX__

#include <vtss/basics/enum-descriptor.h>
#include <vtss/basics/expose/json/pre-loader.hxx>
#include <vtss/basics/expose/json/pre-exporter.hxx>
#include <vtss/basics/expose/json/pre-handler-reflector.hxx>

namespace vtss {
namespace expose {
namespace json {

void serialize_enum(Exporter &e, int32_t &i, const vtss_enum_descriptor_t *d);
void serialize_enum(Loader &e, int32_t &i, const vtss_enum_descriptor_t *d);
void serialize_enum(HandlerReflector &e, const char *name, const char *desc,
                    const vtss_enum_descriptor_t *d);

}  // namespace json
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_JSON_SERIALIZE_ENUM_HXX__
