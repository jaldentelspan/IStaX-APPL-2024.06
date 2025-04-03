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

#include "vtss/basics/rbtree-base.hxx"

namespace vtss {

const RbtreeBase::NodeBase *RbtreeBase::NodeBase::next_() const {
    if (right) return right->left_most();

    // move up in the tree until "n" is found at the left pointer
    const NodeBase *n = this;
    while (n) {
        if (n->parent && n->parent->left == n) {
            n = n->parent;
            break;
        }
        n = n->parent;
    }

    return n;
}

const RbtreeBase::NodeBase *RbtreeBase::NodeBase::prev_() const {
    if (left) return left->right_most();

    // move up in the tree until "n" is found at the right pointer
    const NodeBase *n = this;
    while (n) {
        if (n->parent && n->parent->right == n) {
            n = n->parent;
            break;
        }
        n = n->parent;
    }

    return n;
}

void RbtreeBase::NodeBase::swap(NodeBase *rhs) {
    using vtss::swap;
    NodeBase *rhs_new_parent = parent;
    NodeBase *rhs_new_right = right;
    NodeBase *rhs_new_left = left;
    NodeBase *new_parent = rhs->parent;
    NodeBase *new_right = rhs->right;
    NodeBase *new_left = rhs->left;

    // Prefer to handle when rhs is a child of this, instead of
    // handling if this is a child of rhs!
    if (parent == rhs) {
        rhs->swap(this);
        return;
    }

    // If we have a parent-child relation ship, then we know that 'this'
    // is always the parent and child is always 'rhs'
    new_left = rhs->left;
    new_right = rhs->right;
    rhs_new_parent = parent;

    // Update the parent to this pointer, and rhs->parent to rhs
    // pointers
    if (rhs->parent == parent) {
        // parent can not be null, as this would imply two roots!
        swap(parent->right, parent->left);
    } else {
        if (parent) {
            if (parent->left == this)
                parent->left = rhs;
            else
                parent->right = rhs;
        }

        if (rhs->parent) {
            if (rhs->parent->left == rhs)
                rhs->parent->left = this;
            else
                rhs->parent->right = this;
        }
    }

    // Update rhs children parent pointers
    if (rhs->left) rhs->left->parent = this;
    if (rhs->right) rhs->right->parent = this;

    // Calculate new 'left' pointers for rhs, and update the 'left' child
    // parent pointer if it is not rhs.
    if (left == rhs) {
        rhs_new_left = this;

    } else {
        rhs_new_left = left;
        if (left) left->parent = rhs;
    }

    // Calculate new 'right ' pointers for rhs, and update the 'right'
    // child parent pointer if it is not rhs.
    if (right == rhs) {
        rhs_new_right = this;
    } else {
        rhs_new_right = right;
        if (right) right->parent = rhs;
    }

    // Calculate the new parent pointer
    if (this == rhs->parent)
        new_parent = rhs;
    else
        new_parent = rhs->parent;

    // Apply the calculated pointers
    left = new_left;
    right = new_right;
    parent = new_parent;
    rhs->left = rhs_new_left;
    rhs->right = rhs_new_right;
    rhs->parent = rhs_new_parent;

    // Swap colors
    swap(color, rhs->color);
}

RbtreeBase::NodeBase *RbtreeBase::find(const KeyBase &k, Where &where) const {
    where = EMPTY_TREE;
    NodeBase *n = root;
    while (n) {
        int comp_result = k.compare_key(n);
        if (comp_result == 0) {
            where = MATCH;
            return n;

        } else if (comp_result < 0) {
            if (n->left == nullptr) {
                where = LEFT;
                return n;

            } else {
                n = n->left;
            }

        } else {
            if (n->right == nullptr) {
                where = RIGHT;
                return n;

            } else {
                n = n->right;
            }
        }
    }

    return nullptr;
}

RbtreeBase::NodeBase *RbtreeBase::greater_than(const KeyBase &k) const {
    Where w;
    auto n = find(k, w);

    switch (w) {
    case EMPTY_TREE:
        return nullptr;

    case MATCH:
    case RIGHT:
        return n->next();

    case LEFT:
        return n;
    }

    return nullptr;
}

RbtreeBase::NodeBase *RbtreeBase::greater_than_or_equal(
        const KeyBase &k) const {
    Where w;
    auto n = find(k, w);

    switch (w) {
    case EMPTY_TREE:
        return nullptr;

    case RIGHT:
        return n->next();

    case LEFT:
    case MATCH:
        return n;
    }

    return nullptr;
}

RbtreeBase::NodeBase *RbtreeBase::lesser_than_or_equal(const KeyBase &k) const {
    Where w;
    auto n = find(k, w);

    switch (w) {
    case EMPTY_TREE:
        return nullptr;

    case MATCH:
    case RIGHT:
        return n;

    case LEFT:
        return n->prev();
    }

    return nullptr;
}

RbtreeBase::NodeBase *RbtreeBase::lesser_than(const KeyBase &k) const {
    Where w;
    auto n = find(k, w);

    switch (w) {
    case EMPTY_TREE:
        return nullptr;

    case RIGHT:
        return n;

    case LEFT:
    case MATCH:
        return n->prev();
    }

    return nullptr;
}

bool RbtreeBase::insert_new_element(NodeBase *parent, NodeBase *new_node,
                                    const Where where) {
    if (!new_node) return false;

    size_++;
    new_node->color = RED;
    new_node->left = nullptr;
    new_node->right = nullptr;
    new_node->parent = parent;

    switch (where) {
    case EMPTY_TREE:
        root = new_node;
        break;

    case LEFT:
        parent->left = new_node;
        break;

    case RIGHT:
        parent->right = new_node;
        break;

    case MATCH:
        VTSS_ASSERT(0);
        break;
    }

    insert_balance(new_node);
    verify_properties();

    return true;
}

void RbtreeBase::insert_balance(NodeBase *n) {
    // case 1
    // If this is the new root - then it must be black, and we are done.
    if (n->parent == nullptr) {
        n->color = BLACK;
        return;
    }

    // case 2
    // If the color of parent node is black, then we are done. (We replace a
    // black node with a red node).
    if (node_color(n->parent) == BLACK) {
        return;
    }

    // case 3
    // We know that parent is black, and if uncle is also black we can do
    // re-painting.
    if (node_color(n->uncle()) == RED) {
        n->parent->color = BLACK;
        n->uncle()->color = BLACK;
        n->grandparent()->color = RED;
        insert_balance(n->grandparent());  // recursive call
        return;
    }

    // case 4
    // We know that parent is red and the uncle U is black (otherwise caught by
    // earlier rules).
    if (n == n->parent->right && n->parent == n->grandparent()->left) {
        rotate_left(n->parent);
        n = n->left;
    } else if (n == n->parent->left && n->parent == n->grandparent()->right) {
        rotate_right(n->parent);
        n = n->right;
    }

    // case 5
    // Always applies after case 4
    n->parent->color = BLACK;
    n->grandparent()->color = RED;
    if (n == n->parent->left && n->parent == n->grandparent()->left) {
        rotate_right(n->grandparent());
    } else {
        VTSS_ASSERT(n == n->parent->right &&
                    n->parent == n->grandparent()->right);
        rotate_left(n->grandparent());
    }
}

void RbtreeBase::rotate_left(NodeBase *n) {
    NodeBase *r = n->right;
    replace_node(n, r);
    n->right = r->left;
    if (r->left != nullptr) {
        r->left->parent = n;
    }
    r->left = n;
    n->parent = r;
}

void RbtreeBase::rotate_right(NodeBase *n) {
    NodeBase *L = n->left;
    replace_node(n, L);
    n->left = L->right;
    if (L->right != nullptr) {
        L->right->parent = n;
    }
    L->right = n;
    n->parent = L;
}

void RbtreeBase::replace_node(NodeBase *oldn, NodeBase *newn) {
    if (oldn->parent == nullptr) {
        root = newn;
    } else {
        if (oldn == oldn->parent->left)
            oldn->parent->left = newn;
        else
            oldn->parent->right = newn;
    }
    if (newn != nullptr) {
        newn->parent = oldn->parent;
    }
}

void RbtreeBase::erase(NodeBase *n) {
    NodeBase *child;

    size_--;

    if (n->left != nullptr && n->right != nullptr) {
        // Copy key/value from predecessor and then delete it instead
        NodeBase *pred = n->left->right_most();

        // Needs to make sure that 'root' keep pointing at the root node (after
        // the swap)
        if (n == root) root = pred;

        // Swap the elements
        n->swap(pred);
    }

    // VTSS_ASSERT(n->left == nullptr || n->right == nullptr);

    // At this point we have at most one child, "child" is pointing to that
    child = n->right == nullptr ? n->left : n->right;
    if (node_color(n) == BLACK) {
        n->color = node_color(child);
        delete_balance(n);
    }

    replace_node(n, child);
    if (n->parent == nullptr && child != nullptr)  // root must be black
        child->color = BLACK;

    verify_properties();
}

void RbtreeBase::delete_balance(NodeBase *n) {
    // case 1
    if (n->parent == nullptr) return;

    // case 2
    if (node_color(n->sibling()) == RED) {
        n->parent->color = RED;
        n->sibling()->color = BLACK;
        if (n == n->parent->left)
            rotate_left(n->parent);
        else
            rotate_right(n->parent);
    }

    // case 3
    if (node_color(n->parent) == BLACK && node_color(n->sibling()) == BLACK &&
        node_color(n->sibling()->left) == BLACK &&
        node_color(n->sibling()->right) == BLACK) {
        n->sibling()->color = RED;
        delete_balance(n->parent);
        return;
    }

    // case 4
    if (node_color(n->parent) == RED && node_color(n->sibling()) == BLACK &&
        node_color(n->sibling()->left) == BLACK &&
        node_color(n->sibling()->right) == BLACK) {
        n->sibling()->color = RED;
        n->parent->color = BLACK;
        return;
    }

    // case 5
    if (n == n->parent->left && node_color(n->sibling()) == BLACK &&
        node_color(n->sibling()->left) == RED &&
        node_color(n->sibling()->right) == BLACK) {
        n->sibling()->color = RED;
        n->sibling()->left->color = BLACK;
        rotate_right(n->sibling());

    } else if (n == n->parent->right && node_color(n->sibling()) == BLACK &&
               node_color(n->sibling()->right) == RED &&
               node_color(n->sibling()->left) == BLACK) {
        n->sibling()->color = RED;
        n->sibling()->right->color = BLACK;
        rotate_left(n->sibling());
    }

    // case 6
    n->sibling()->color = node_color(n->parent);
    n->parent->color = BLACK;
    if (n == n->parent->left) {
        VTSS_ASSERT(node_color(n->sibling()->right) == RED);
        n->sibling()->right->color = BLACK;
        rotate_left(n->parent);
    } else {
        VTSS_ASSERT(node_color(n->sibling()->left) == RED);
        n->sibling()->left->color = BLACK;
        rotate_right(n->parent);
    }
}

#if defined(RBTREE_VERIFY)
void RbtreeBase::verify_properties() const {
    verify_property_1(root);
    verify_property_2(root);
    /* Property 3 is implicit */
    verify_property_4(root);
    verify_property_5(root);
}

void RbtreeBase::verify_property_1(NodeBase *n) const {
    VTSS_ASSERT(node_color(n) == RED || node_color(n) == BLACK);
    if (n == nullptr) return;
    verify_property_1(n->left);
    verify_property_1(n->right);
}

void RbtreeBase::verify_property_2(NodeBase *root) const {
    VTSS_ASSERT(node_color(root) == BLACK);
}

void RbtreeBase::verify_property_4(NodeBase *n) const {
    if (node_color(n) == RED) {
        VTSS_ASSERT(node_color(n->left) == BLACK);
        VTSS_ASSERT(node_color(n->right) == BLACK);
        VTSS_ASSERT(node_color(n->parent) == BLACK);
    }
    if (n == nullptr) return;
    verify_property_4(n->left);
    verify_property_4(n->right);
}

void RbtreeBase::verify_property_5(NodeBase *root) const {
    int black_count_path = -1;
    verify_property_5_helper(root, 0, &black_count_path);
}

void RbtreeBase::verify_property_5_helper(NodeBase *n, int black_count,
                                          int *path_black_count) const {
    if (node_color(n) == BLACK) {
        black_count++;
    }
    if (n == nullptr) {
        if (*path_black_count == -1) {
            *path_black_count = black_count;
        } else {
            VTSS_ASSERT(black_count == *path_black_count);
        }
        return;
    }
    verify_property_5_helper(n->left, black_count, path_black_count);
    verify_property_5_helper(n->right, black_count, path_black_count);
}
#endif  // defined(RBTREE_VERIFY)
}  // namespace vtss
