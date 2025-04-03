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

#ifndef _STREAM_API_H_
#define _STREAM_API_H_

#include <vtss/appl/stream.h>
#include <vtss/basics/expose/table-status.hxx>
#include <vtss/basics/enum_macros.hxx>

// To be able to do client++
VTSS_ENUM_INC(vtss_appl_stream_client_t);

mesa_rc stream_init(vtss_init_data_t *data);
const char *stream_error_txt(mesa_rc rc);

// Utility functions for obtaining and clearing counters for a particular stream
mesa_rc     stream_util_counters_get(  vtss_appl_stream_id_t stream_id, mesa_ingress_counters_t &counters);
mesa_rc     stream_util_counters_clear(vtss_appl_stream_id_t stream_id);
const char *stream_util_client_to_str(vtss_appl_stream_client_t client, bool capital = false);
const char *stream_util_dmac_match_type_to_str(vtss_appl_stream_dmac_match_type_t dmac_match_type);
const char *stream_util_protocol_type_to_str(mesa_vce_type_t t);
char       *stream_util_oper_warnings_to_str(char *buf, size_t size, vtss_appl_stream_oper_warnings_t oper_warnings);
char       *stream_collection_util_oper_warnings_to_str(char *buf, size_t size, vtss_appl_stream_collection_oper_warnings_t oper_warnings);
char       *stream_collection_util_stream_id_list_to_str(const vtss_appl_stream_id_t *ids, size_t sz, char *buf);
mesa_rc     stream_collection_util_counters_get(  vtss_appl_stream_collection_id_t stream_collection_id, mesa_ingress_counters_t &counters);
mesa_rc     stream_collection_util_counters_clear(vtss_appl_stream_collection_id_t stream_collection_id);

typedef int32_t (*stream_icli_pr_t)(const char *fmt, ...) __attribute__((__format__(__printf__, 1, 2)));
void stream_debug_statistics_show(vtss_appl_stream_id_t stream_id, stream_icli_pr_t pr, bool first);
void stream_debug_vces_show(      vtss_appl_stream_id_t stream_id, stream_icli_pr_t pr, bool first);
void stream_debug_iflows_show(    vtss_appl_stream_id_t stream_id, stream_icli_pr_t pr, bool first);

// Make clients able to connect as observers whenever a stream is added or
// removed or the stream's port list changes. The uint32_t is an
// ever-incrementing dummy value used to make the underlying expose
// functionality issue a change.
typedef vtss::expose::TableStatus<vtss::expose::ParamKey<vtss_appl_stream_id_t>, vtss::expose::ParamVal<uint32_t>> stream_notif_table_t;
extern stream_notif_table_t stream_notif_table;

// Make clients able to connect as observers whenever a stream collection is
// added, modified, or removed. The uint32_t is an ever-incrementing dummy value
// used to make the underlying expose functionality issue a change.
typedef vtss::expose::TableStatus<vtss::expose::ParamKey<vtss_appl_stream_collection_id_t>, vtss::expose::ParamVal<uint32_t>> stream_collection_notif_table_t;
extern stream_collection_notif_table_t stream_collection_notif_table;

#endif /* _STREAM_API_H_ */

