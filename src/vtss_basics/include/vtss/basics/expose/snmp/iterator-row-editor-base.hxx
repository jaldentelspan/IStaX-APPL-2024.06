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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_ROW_EDITOR_BASE_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_ROW_EDITOR_BASE_HXX__

#include "vtss/basics/api_types.h"
#include "vtss/basics/expose/snmp/oid_sequence.hxx"

namespace vtss {
namespace expose {
namespace snmp {

class IteratorRowEditorBase {
  public:
    virtual uint32_t get_external_state() = 0;
    virtual void set_external_state(uint32_t s) = 0;
    virtual mesa_rc invoke_add() = 0;
    virtual void invoke_reset_values() = 0;

    IteratorRowEditorBase(uint32_t &i) : internal_state(i) { }
    virtual ~IteratorRowEditorBase() { }

    mesa_rc set();
    mesa_rc get_next(const OidSequence &idx, OidSequence &next);
    mesa_rc get(const OidSequence &oid);

  protected:
    mesa_rc go_idle();
    mesa_rc go_active(uint32_t s);
    mesa_rc set_is_idle(uint32_t s);
    mesa_rc set_is_clear(uint32_t s);
    mesa_rc set_is_write(uint32_t);
    mesa_rc set_is_active(uint32_t s);

    uint32_t &internal_state;
};

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_ROW_EDITOR_BASE_HXX__
