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

#ifndef __VTSS_BASICS_EXPOSE_JSON_NODE_HXX__
#define __VTSS_BASICS_EXPOSE_JSON_NODE_HXX__

#include <vtss/basics/json-rpc.hxx>
#include <vtss/basics/intrusive_list.hxx>
#include <vtss/basics/expose/json/request.hxx>
#include <vtss/basics/expose/json/reflection.hxx>

namespace vtss {
namespace expose {
namespace json {

struct NamespaceNode;

struct Node : public intrusive::ListNode {
    typedef bool(*get_ptr)();

    explicit Node(const char *n, const char *d = nullptr)
        : name_(n), description_(d) {}
    Node(NamespaceNode *p, const char *n, const char *d = nullptr);

    virtual bool is_namespace_node() const { return false; }
    virtual bool is_function_exporter_abstract() const { return false; }
    virtual bool is_function_exporter() const { return false; }

    virtual bool is_notification() const { return false; }
    virtual vtss::json::Result::Code handle(int priv, str method_name,
                                            const Request *req,
                                            ostreamBuf &out) = 0;

    virtual void handle_reflection(Reflection *r) = 0;

    void attach_to(NamespaceNode *p);
    virtual Node *lookup(str method_name);

    virtual ~Node() {
        // TODO, raise!
        unlink();
    }

    str name() const { return str(name_); }
    str description() const { return str(description_); }
    void description(const char *d) { description_ = d; }
    str depends_on_capability() const { return str(depends_on_capability_); }
    void depends_on_capability(const char *d) { depends_on_capability_ = d; }
    str depends_on_capability_group() const {
        return str(depends_on_capability_group_);
    }
    void depends_on_capability_group(const char *d) {
        depends_on_capability_group_ = d;
    }

    void implemented(get_ptr i) { implemented_ = i; }

    bool implemented() {
        if (!implemented_) return true;
        return (*implemented_)();
    }

    const Node *parent() const { return parent_; }

    bool has_notification() const { return has_get_all_notification_; }
    void has_notification(bool b) { has_get_all_notification_ = b; }

    std::string abs_name() const;
    std::string abs_name(const std::string &s) const;

  protected:
    Node *parent_ = nullptr;
    const char *name_ = nullptr;
    const char *description_ = nullptr;
    const char *depends_on_capability_ = nullptr;
    const char *depends_on_capability_group_ = nullptr;
    get_ptr implemented_ = nullptr;
    bool has_get_all_notification_ = false;
};

}  // namespace json
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_JSON_NODE_HXX__
