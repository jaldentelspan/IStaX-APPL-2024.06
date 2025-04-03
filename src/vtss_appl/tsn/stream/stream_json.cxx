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

#include "stream_serializer.hxx"

using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;
using namespace vtss::appl::stream::interfaces;

namespace vtss
{
void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_stream("stream");
extern "C" void stream_json_init(void)
{
    json_node_add(&ns_stream);
}

// Parent: vtss/stream --------------------------------------------------------
NS(ns_conf,   ns_stream, "config");
NS(ns_status, ns_stream, "status");

static StructReadOnly<StreamCapabilities>            stream_capabilities(           &ns_stream, "capabilities");
static StructReadOnly<StreamCollectionCapabilities>  stream_collection_capabilities(&ns_stream, "collectionCapabilities");
static StructReadOnly<StreamDefaultConf>             stream_default_conf(           &ns_conf,   "defaultConf");
static StructReadOnly<StreamCollectionDefaultConf>   stream_collection_default_conf(&ns_conf,   "defaultCollectionConf");
static TableReadWriteAddDeleteNoNS<StreamConf>       stream_conf(                   &ns_conf);
static TableReadWriteAddDelete<StreamCollectionConf> stream_collection_conf(        &ns_conf,   "collectionConf");
static TableReadOnlyNoNS<StreamStatus>               stream_status(                 &ns_status);
static TableReadOnly<StreamCollectionStatus>         stream_collection_status(      &ns_status, "collectionStatus");

