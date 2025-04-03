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

#ifndef __VTSS_BASICS_EXPOSE_JSON_SPECIFICATION_REFLECTOR_ECHO_HXX__
#define __VTSS_BASICS_EXPOSE_JSON_SPECIFICATION_REFLECTOR_ECHO_HXX__

#include <string>
#include <vtss/basics/vector.hxx>
#include <vtss/basics/expose/json/node.hxx>
#include <vtss/basics/expose/json/reflection.hxx>
#include <vtss/basics/expose/json/specification/indent.hxx>
#include <vtss/basics/expose/json/specification/inventory.hxx>
#include <vtss/basics/expose/json/specification/type-descriptor-struct.hxx>

namespace vtss {
namespace expose {
namespace json {
namespace specification {

struct ReflectorEcho : public Reflection {
    ReflectorEcho(Inventory &i, const std::string &m, const std::string &p,
                  Node *n, bool target_specific);

    bool target_specific() { return target_specific_; }

    void input_begin();
    void input_end();

    void output_begin();
    void output_end();

    void argument_name(const char *name, const char *desc);
    void argument_begin(unsigned idx);
    void argument_end();

    void variable_begin(const char *name, const char *desc, const char *depends,
                        const char *depends_group);
    void variable_end();

    void type_begin(const char *name, const char *desc);
    void type_end();

    void alias_begin();
    void alias_end();

    void has_get_all_variant();
    void has_notification();

    void type_terminal(JsonCoreType::E type, const char *c_type_name,
                       const char *c_type_alias, const char *description);

    void type_terminal_enum(const char *c_type_name, const char *desc,
                            const ::vtss_enum_descriptor_t *d);

    Indent indent;
    int alias_ = 0;
    int type_level = 0;
    bool input_section = false;
    bool output_section = false;
    Inventory &inventory;
    ParamDescriptor *p = nullptr;
    MethodDescriptor *desc = nullptr;
    Node *node;
    bool target_specific_;

    std::string pending_name;
    std::string pending_desc;
    std::string pending_dep;
    Vector<std::shared_ptr<TypeDescriptorStruct>> typestack;
};

}  // namespace specification
}  // namespace json
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_JSON_SPECIFICATION_REFLECTOR_ECHO_HXX__
