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

#include "../test/catch.hpp"
#include "vector-exercise.hxx"


namespace vtss {

std::ostream &operator<<(std::ostream &o, const Vector<int> &v) {
    o << "[";

    bool first = true;
    for (auto i : v) {
        if (first)
            first = false;
        else
            o << ", ";
        o << i;
    }

    o << "]";
    return o;
}

namespace VectorExercise {

TEST_CASE("Simple graph") {
    ShortestPathCalculatorImpl g;

    g.node_add(0);
    g.node_add(1);
    g.edge_add(0, 1, 1);

    SECTION("Path from 0 to 1") {
        CHECK(g.shortest_path(0, 1) == Vector<int>({0, 1}));
    }

    SECTION("Path from 1 to 0") {
        CHECK(g.shortest_path(1, 0) == Vector<int>({}));
    }
}

TEST_CASE("9-node-graph") {
    ShortestPathCalculatorImpl g;

    for (int i = 0; i < 9; ++i) g.node_add(i);

    g.edge_add(0, 1, 1);
    g.edge_add(0, 4, 100);

    g.edge_add(1, 2, 1);
    g.edge_add(1, 0, 1);

    g.edge_add(2, 1, 1);
    g.edge_add(2, 4, 2);
    g.edge_add(2, 5, 1);

    g.edge_add(3, 4, 1);
    g.edge_add(3, 6, 2);

    g.edge_add(4, 0, 1);
    g.edge_add(4, 2, 2);
    g.edge_add(4, 3, 1);
    g.edge_add(4, 5, 5);
    g.edge_add(4, 6, 4);
    g.edge_add(4, 8, 100);

    g.edge_add(5, 2, 1);
    g.edge_add(5, 4, 5);

    g.edge_add(6, 3, 2);
    g.edge_add(6, 4, 4);
    g.edge_add(6, 7, 3);

    g.edge_add(7, 6, 3);
    g.edge_add(7, 8, 8);

    g.edge_add(8, 4, 1);
    g.edge_add(8, 7, 8);

    SECTION("Path from 0 to 8") {
        CHECK(g.shortest_path(0, 8) == Vector<int>({0, 1, 2, 4, 3, 6, 7, 8}));
    }

    SECTION("Path from 8 to 0") {
        CHECK(g.shortest_path(8, 0) == Vector<int>({8, 4, 0}));
    }
}

}  // namespace VectorExercise
}  // namespace vtss
