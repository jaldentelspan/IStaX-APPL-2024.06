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

#ifndef __VTSS_BASICS_EXAMPLES_VECTOR_EXERCISE_HXX__
#define __VTSS_BASICS_EXAMPLES_VECTOR_EXERCISE_HXX__

#include <iostream>
#include "vtss/basics/vector.hxx"

namespace vtss {
namespace VectorExercise {

struct Edge {
    Edge(int d, unsigned c) : dst(d), cost(c) {}
    const int dst;
    const unsigned cost;
};

struct Node {
    Node(int i) : id(i) {}
    const int id;
    int acc_prev;
    unsigned acc_cost;
    Vector<Edge> edges;
};

struct ShortestPathCalculatorImpl {
    void node_add(int node_id) {
        nodes.emplace_back(node_id);
    }

    void edge_add(int from_node, int to_node, unsigned cost) {
        nodes[from_node].edges.emplace_back(to_node, cost);
    }

    vtss::Vector<int> shortest_path(int from_node, int to_node) {
        clear();

        nodes[from_node].acc_cost = 0;

        Vector<int> stack;
        stack.push_back(from_node);

        while (!stack.empty()) {
            int id = stack.back();
            stack.pop_back();

            int cost = nodes[id].acc_cost;

            for (const auto &e: nodes[id].edges) {
                auto &d = nodes[e.dst];

                if (cost + e.cost < d.acc_cost) {
                    d.acc_prev = id;
                    d.acc_cost = cost + e.cost;
                    stack.push_back(e.dst);
                }
            }
        }

        if (nodes[to_node].acc_cost == 0xffffffff)
            return Vector<int>();

        Vector<int> res;
        res.push_back(to_node);

        int last = to_node;
        while (true) {
            last = nodes[last].acc_prev;
            res.push_back(last);
            if (last == from_node) break;
        }

        reverse(res.begin(), res.end());

        return res;

    }

    void clear() {
        for (auto &e: nodes) {
            e.acc_prev = -1;
            e.acc_cost = 0xffffffff;
        }
    }

    Vector<Node> nodes;
};

}  // namespace VectorExercise
}  // namespace vtss

#endif  // __VTSS_BASICS_EXAMPLES_VECTOR_EXERCISE_HXX__
