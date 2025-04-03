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

#ifndef __VTSS_BASICS_RBTREE_BASE_HXX__
#define __VTSS_BASICS_RBTREE_BASE_HXX__

#include "vtss/basics/common.hxx"
#include "vtss/basics/utility.hxx"

namespace vtss {

struct RbtreeBase {
    // See http://en.literateprograms.org/Red-black_tree_(C) and
    // http://en.wikipedia.org/wiki/Red%E2%80%93black_tree

    enum Color { RED, BLACK };
    enum Where { MATCH = 0, EMPTY_TREE, LEFT, RIGHT };

    struct NodeBase {
        NodeBase *left;
        NodeBase *right;
        NodeBase *parent;
        Color color;

        const NodeBase *left_most() const {
            const NodeBase *n = this;
            while (n->left) n = n->left;
            return n;
        }

        NodeBase *left_most() {
            NodeBase *n = this;
            while (n->left) n = n->left;
            return n;
        }

        NodeBase *right_most() {
            NodeBase *n = this;
            while (n->right) n = n->right;
            return n;
        }

        NodeBase *sibling() {
            return this == parent->left ? parent->right : parent->left;
        }

        NodeBase *uncle() { return parent->sibling(); }
        NodeBase *grandparent() { return parent->parent; }

        NodeBase *next() { return const_cast<NodeBase *>(next_()); }
        const NodeBase *next() const { return next_(); }

        NodeBase *prev() { return const_cast<NodeBase *>(prev_()); }
        const NodeBase *prev() const { return prev_(); }

        void swap(NodeBase *rhs);

      private:
        const NodeBase *next_() const;
        const NodeBase *prev_() const;
    };

    struct KeyBase {
        virtual int compare_key(const NodeBase *rhs) const = 0;
    };

    RbtreeBase(){};
    RbtreeBase(RbtreeBase &&rhs) {
        size_ = rhs.size_;
        root = rhs.root;

        rhs.size_ = 0;
        rhs.root = nullptr;
    }

    RbtreeBase(const RbtreeBase &rhs) = delete;
    RbtreeBase &operator=(const RbtreeBase &m) = delete;
    RbtreeBase &operator=(RbtreeBase &&rhs) {
        using vtss::swap;
        swap(size_, rhs.size_);
        swap(root, rhs.root);
        return *this;
    }

    size_t size() const { return size_; }

    void erase(NodeBase *n);

    const NodeBase *begin() const {
        if (!root) return nullptr;
        return root->left_most();
    }

    NodeBase *begin() {
        if (!root) return nullptr;
        return root->left_most();
    }

    const NodeBase *last() const {
        if (!root) return nullptr;
        return root->right_most();
    }

    NodeBase *last() {
        if (!root) return nullptr;
        return root->right_most();
    }

    NodeBase *find(const KeyBase &k, Where &where) const;
    NodeBase *find(const KeyBase &k) const {
        Where w;
        NodeBase *n = find(k, w);
        if (w != MATCH) return nullptr;
        return n;
    }

    NodeBase *lesser_than(const KeyBase &k) const;
    NodeBase *lesser_than_or_equal(const KeyBase &k) const;
    NodeBase *greater_than(const KeyBase &k) const;
    NodeBase *greater_than_or_equal(const KeyBase &k) const;

    bool insert_new_element(NodeBase *parent, NodeBase *new_node,
                            const Where where);


  private:
    void delete_balance(NodeBase *n);
    void insert_balance(NodeBase *n);
    void rotate_left(NodeBase *n);
    void rotate_right(NodeBase *n);
    void replace_node(NodeBase *oldn, NodeBase *newn);

    #if defined(RBTREE_VERIFY)
    void verify_properties() const;
    void verify_property_1(NodeBase *n) const;
    void verify_property_2(NodeBase *root) const;
    void verify_property_4(NodeBase *n) const;
    void verify_property_5(NodeBase *root) const;
    void verify_property_5_helper(NodeBase *n, int black_count,
                                  int *path_black_count) const;
    #else
    void verify_properties() const {}
    #endif

    Color node_color(NodeBase *n) const {
        return n == nullptr ? BLACK : n->color;
    }

    size_t size_ = 0;
    NodeBase *root = nullptr;
};

}  // namespace vtss

#endif  // __VTSS_BASICS_RBTREE_BASE_HXX__
