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

#ifndef __VTSS_BASICS_EXPOSE_JSON_NAMESPACE_NODE_HXX__
#define __VTSS_BASICS_EXPOSE_JSON_NAMESPACE_NODE_HXX__

#include "vtss/basics/expose/json/node.hxx"

namespace vtss {
namespace expose {
namespace json {

struct NamespaceNode : public Node {
    friend struct Node;

    explicit NamespaceNode(const char *n) : Node(n) {}
    NamespaceNode(NamespaceNode *p, const char *n) : Node(p, n) {}

    virtual bool is_namespace_node() const { return true; }

    vtss::json::Result::Code handle(int priv, str method_name,
                                    const Request *req, ostreamBuf &out);

    virtual void handle_reflection(Reflection *r) {}
    virtual Node *lookup(str method_name);

    intrusive::List<Node> leafs;

  private:
    void attach__(Node *leaf);
};

}  // namespace json
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_JSON_NAMESPACE_NODE_HXX__
