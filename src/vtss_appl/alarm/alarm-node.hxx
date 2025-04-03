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

#ifndef __ALARM_NODE_HXX__
#define __ALARM_NODE_HXX__

#include <vtss/basics/list.hxx>
#include "alarm-element.hxx"

namespace vtss {
namespace appl {
namespace alarm {

bool operator<(const std::unique_ptr<Element> &x,
               const std::unique_ptr<Element> &y);

// The alarm node's is used to build the tree in the alarm hierarchy.
struct Node : public Element, public EventHandler {
    Node(SubjectRunner *const s, const std::string &n)
        : Element(Element::Type::NODE, n), EventHandler(s) {}
    ~Node() {}
    virtual bool ok() const;
    //    virtual mesa_rc suppress(const str &n, bool s);
    virtual void execute(Event *e);

    // TODO, Please use a std::unique_ptr<Element> directly in the interface.
    // This makes it clear to everybody that we are transfering ownership!
    // TODO, should return an error code
    Pair<Set<std::unique_ptr<Element>>::iterator, bool> add_child(Element *element);

    Set<std::unique_ptr<Element>> children;
};

}  // namespace alarm
}  // namespace appl
}  // namespace vtss

#endif  // __ALARM_NODE_HXX__
