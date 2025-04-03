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

#ifndef __ALARM_ELEMENT_HXX__
#define __ALARM_ELEMENT_HXX__

#include "vtss/basics/notifications.hxx"
#include <vtss/basics/expose/json/specification/inventory.hxx>
#include "vtss/basics/expose/json/root-node.hxx"

namespace vtss {
namespace appl {
namespace alarm {
using namespace vtss::notifications;

struct Element : public Subject<bool> {
    enum class Type { NODE, LEAF };
    Element() : Subject(false), name_(""), type_(Type::NODE) {}
    Element(Type t, const std::string &n)
        : Subject(false), name_(n), type_(t) {}
    virtual bool ok() const { return true; }
    const std::string name() const;
    Type type() const { return type_; }

    // TODO, looks like you are missing functions to delete alarms

    // TODO: Does the make_alarm method really belong to the element class??? I
    // think it is prefectly okay to have the private overlaod in the element
    // class as this make's recursion easier.
    // It might be easier to understand how to use these classes if you created
    // a RootNote class that is derived from Node, and then implement the public
    // operations such make_alarm(..., std::string, ...), lookup(std::string),
    // suppress in the RootNode.
    // As far as I understand your design, then this is a fairly small change.
    mesa_rc make_alarm(SubjectRunner *const s, const std::string &prefix,
                       const std::string &nm, const std::string &expr,
                       const expose::json::specification::Inventory &i,
                       expose::json::RootNode &r);
    mesa_rc delete_alarm(const std::string &nm);
    Element *lookup(const std::string &name);
    Element *next(const str &nm);
    mesa_rc suppress(const std::string &n, bool s);
    void set(const bool &value, bool force = false);

  private:
    Element *lookup(const str &name);
    mesa_rc make_alarm(SubjectRunner *const s, const std::string &prefix,
                       const str &name, const std::string &expr,
                       const expose::json::specification::Inventory &i,
                       expose::json::RootNode &r);
    mesa_rc delete_alarm(const str &nm);

  public:
    const std::string name_;
    const Type type_;

    bool suppressed = false;
    Element *parent_;

    // TODO: Should this be moved to Leaf?? Not sure I understand why it is
    // needed in the base-class?
    // ANS: No cannot be moved to Leaf. Both leafs and nodes has an alarm state
    // that may change and they need an event to signal the state change to its
    // parent.
    std::unique_ptr<notifications::Event> event;
};

}  // namespace alarm
}  // namespace appl
}  // namespace vtss

#endif  // __ALARM_ELEMENT_HXX__
