/*
 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "stream_serializer.hxx"

using namespace vtss;
using namespace expose::snmp;

VTSS_MIB_MODULE("streamMib", "STREAM", stream_mib_init, VTSS_MODULE_ID_STREAM, root, h)
{
    h.add_history_element("202311230000Z", "Initial version");
    h.add_history_element("202401290000Z", "Added stream collection support");
    h.description("Private MIB for Stream matching.");
}
#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))

namespace vtss
{
namespace appl
{
namespace stream
{
namespace interfaces
{
// Parent: vtss
NS(ns_stream, root,      1, "streamMibObjects");

// Parent: vtss/stream
NS(ns_capabilities, ns_stream, 1, "streamCapabilities");
NS(ns_conf,         ns_stream, 2, "streamConfig");
NS(ns_status,       ns_stream, 3, "streamStatus");

static StructRO2<StreamCapabilities>                  stream_capabilities(           &ns_capabilities, OidElement(1, "streamCapabilitiesList"));
static StructRO2<StreamCollectionCapabilities>        stream_collection_capabilities(&ns_capabilities, OidElement(2, "streamCapabilitiesCollection"));
static TableReadWriteAddDelete2<StreamConf>           stream_conf(                   &ns_conf,         OidElement(1, "streamConfigTable"),           OidElement(2, "streamConfigRowEditor"));
static TableReadWriteAddDelete2<StreamCollectionConf> stream_collection_conf(        &ns_conf,         OidElement(3, "streamConfigCollectionTable"), OidElement(4, "streamConfigCollectionRowEditor"));
static TableReadOnly2<StreamStatus>                   stream_status(                 &ns_status,       OidElement(1, "streamStatusTable"));
static TableReadOnly2<StreamCollectionStatus>         stream_collection_status(      &ns_status,       OidElement(2, "streamStatusCollectionTable"));

}  // namespace interfaces
}  // namespace stream
}  // namespace appl
}  // namespace vtss

