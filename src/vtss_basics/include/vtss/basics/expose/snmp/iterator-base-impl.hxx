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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_BASE_IMPL_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_BASE_IMPL_HXX__

#include "vtss/basics/expose/snmp/types_params.hxx"
#include "vtss/basics/expose/snmp/iterator-common.hxx"
#include "vtss/basics/expose/snmp/iterator-row-editor2.hxx"
#include "vtss/basics/expose/snmp/handlers/get.hxx"
#include "vtss/basics/expose/snmp/handlers/set.hxx"

namespace vtss {
namespace expose {
namespace snmp {

template <typename... T>
void OidIteratorRO<T...>::call_serialize(GetHandler &h) {
    value.do_serialize(h);
}

template <typename... T>
void OidIteratorRO<T...>::call_serialize_values(GetHandler &h) {
    value.do_serialize_values(h);
}

template <typename... T>
void OidIteratorRO<T...>::call_serialize(SetHandler &h) {
    value.do_serialize(h);
}

template <typename... T>
void OidIteratorRO<T...>::call_serialize_values(SetHandler &h) {
    value.do_serialize_values(h);
}


template <typename INTERFACE>
void OidIteratorRO2<INTERFACE>::call_serialize(GetHandler &h) {
    INTERFACE if_;
    value.do_serialize2(h, if_);
}

template <typename INTERFACE>
void OidIteratorRO2<INTERFACE>::call_serialize_values(GetHandler &h) {
    INTERFACE if_;
    value.do_serialize2_values(h, if_);
}

template <typename INTERFACE>
void OidIteratorRO2<INTERFACE>::call_serialize(SetHandler &h) {
    INTERFACE if_;
    value.do_serialize2(h, if_);
}

template <typename INTERFACE>
void OidIteratorRO2<INTERFACE>::call_serialize_values(SetHandler &h) {
    INTERFACE if_;
    value.do_serialize2_values(h, if_);
}


template <typename... T>
void IteratorRowEditor<T...>::call_serialize(GetHandler &h) {
    value.do_serialize(h);
}

template <typename... T>
void IteratorRowEditor<T...>::call_serialize_values(GetHandler &h) {
    value.do_serialize_values(h);
}

template <typename... T>
void IteratorRowEditor<T...>::call_serialize(SetHandler &h) {
    value.do_serialize(h);
}

template <typename... T>
void IteratorRowEditor<T...>::call_serialize_values(SetHandler &h) {
    value.do_serialize_values(h);
}


template <typename INTERFACE>
void IteratorRowEditor2<INTERFACE>::call_serialize(GetHandler &h) {
    INTERFACE if_;
    value.do_serialize2(h, if_);
}

template <typename INTERFACE>
void IteratorRowEditor2<INTERFACE>::call_serialize_values(GetHandler &h) {
    INTERFACE if_;
    value.do_serialize2_values(h, if_);
}

template <typename INTERFACE>
void IteratorRowEditor2<INTERFACE>::call_serialize(SetHandler &h) {
    INTERFACE if_;
    ParamTuple_ undo_value;
    undo_value = value;
    value.do_serialize2(h, if_);
    if (h.error_code() != ErrorCode::noError) value = undo_value;
}

template <typename INTERFACE>
void IteratorRowEditor2<INTERFACE>::call_serialize_values(SetHandler &h) {
    INTERFACE if_;
    ParamTuple_ undo_value;
    undo_value = value;
    value.do_serialize2_values(h, if_);
    if (h.error_code() != ErrorCode::noError) value = undo_value;
}


template <typename... T>
void IteratorSingleRow<T...>::call_serialize(GetHandler &h) {
    value.do_serialize(h);
}

template <typename... T>
void IteratorSingleRow<T...>::call_serialize_values(GetHandler &h) {
    value.do_serialize_values(h);
}

template <typename... T>
void IteratorSingleRow<T...>::call_serialize(SetHandler &h) {
    value.do_serialize(h);
}

template <typename... T>
void IteratorSingleRow<T...>::call_serialize_values(SetHandler &h) {
    value.do_serialize_values(h);
}


template <typename INTERFACE>
void IteratorSingleRow2<INTERFACE>::call_serialize(GetHandler &h) {
    INTERFACE if_;
    value.do_serialize2(h, if_);
}

template <typename INTERFACE>
void IteratorSingleRow2<INTERFACE>::call_serialize_values(GetHandler &h) {
    INTERFACE if_;
    value.do_serialize2_values(h, if_);
}

template <typename INTERFACE>
void IteratorSingleRow2<INTERFACE>::call_serialize(SetHandler &h) {
    INTERFACE if_;
    value.do_serialize2(h, if_);
}

template <typename INTERFACE>
void IteratorSingleRow2<INTERFACE>::call_serialize_values(SetHandler &h) {
    INTERFACE if_;
    value.do_serialize2_values(h, if_);
}


template <typename INTERFACE>
mesa_rc OidIteratorRO2<INTERFACE>::value_itr() {
    return value.itr(itr_ptr);
}

template <typename INTERFACE>
mesa_rc OidIteratorRO2<INTERFACE>::value_get() {
    return value.get(get_ptr);
}

template <typename INTERFACE>
void OidIteratorRO2<INTERFACE>::value_clear_input() {
    value.clear_input();
}

template <typename INTERFACE>
void OidIteratorRO2<INTERFACE>::value_copy_next_to_data() {
    value.copy_next_to_data();
}

template <typename INTERFACE>
bool OidIteratorRO2<INTERFACE>::value_export_output(OidExporter &exporter) {
    INTERFACE interface;
    return value.export_output2(exporter, interface);
}

template <typename INTERFACE>
bool OidIteratorRO2<INTERFACE>::value_parse_input(OidImporter &importer) {
    INTERFACE interface;
    return value.parse_input2(importer, interface);
}

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_BASE_IMPL_HXX__
