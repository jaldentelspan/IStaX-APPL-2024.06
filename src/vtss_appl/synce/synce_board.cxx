/*

 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

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

// TODO:
// - Review synce_board.cxx (first Thomas review, and then Allan)
//   - Make sure to use io/pair/map/vector/etc from basics (instead of std)     -- DONE
//   - Do proper tracing, and make sure you have the traces needed to debug     -- DONE
//   - Look for error handling and check that it is done correctly              -- DONE
// - Review synce_board_graph.hxx, make sure the comments are correct, and      -- DONE
//   elaborate where you think it is needed.
// - API ifdef's in synce_board_graph_*.cxx needs to be converted to            -- only ifdef is in synce_board_graph_caracal1.cxx to discern between CARACAL_1 and CARACAL_LITE
//   capabilities
// - Move the macroes from synce_board_graph_*.cxx to synce_board_graph_*.hxx   -- DONE
// - Move graph impl to meba, and cleanup ifdef's

#include "vector.hxx"
#include "map.hxx"
#include "algorithm.hxx"

#include "synce_board.hxx"
#include "microchip/ethernet/board/api.h"
#include "pcb107_cpld.h"

#include "synce_trace.h"// For Trace
#include <vtss/basics/trace.hxx>

#ifdef VTSS_SW_OPTION_PHY
#include "phy_api.h"
#endif

struct Node;

enum Clock {
    CLOCK_UNKNOWN = 0,

    CLOCK_1,
    CLOCK_2,
    CLOCK_STATION,
};

struct Edge {
    uint32_t dst_idx;
    uint32_t src_idx;
    Node *dst;
    Node *src;
    vtss::Map<meba_attr_t, uint32_t> attr;
};

struct Node {
    uint32_t type;
    uint32_t dev_id;
    vtss::Map<uint32_t, Edge> outputs;
    vtss::Map<uint32_t, Edge *> inputs;
    vtss::Vector<vtss::Pair<uint32_t, uint32_t>> void_paths;
};

typedef vtss::Map<uint32_t, Node> G;

struct Indent {
    Indent() : cnt(1) {}
    Indent(const Indent &rhs) : cnt(rhs.cnt + 1) {}
    int cnt;
};

typedef vtss::Vector<Edge *> Path;
typedef vtss::Vector<Path> Paths;

void internal_graph_get();
Path path_for_clock(G &g, Clock c, uint32_t dev_id);
static G gg;

// TODO, move to meba
mesa_rc synce_board_init()
{
    if (meba_capability(board_instance, MEBA_CAP_BOARD_HAS_PCB107_CPLD)) {
        VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Call to pcb107_cpld_init.";
        pcb107_cpld_init();
    } else if (meba_capability(board_instance, MEBA_CAP_BOARD_HAS_PCB135_CPLD)) {
        pcb135_cpld_init();
    }

    return VTSS_RC_OK;
}

vtss::ostream &operator<<(vtss::ostream &o, const mesa_synce_divider_t &i) {
    switch (i) {
        case MESA_SYNCE_DIVIDER_1: o << "MESA_SYNCE_DIVIDER_1"; return o;
        case MESA_SYNCE_DIVIDER_2: o << "MESA_SYNCE_DIVIDER_2"; return o;
        case MESA_SYNCE_DIVIDER_4: o << "MESA_SYNCE_DIVIDER_4"; return o;
        case MESA_SYNCE_DIVIDER_5: o << "MESA_SYNCE_DIVIDER_5"; return o;
        case MESA_SYNCE_DIVIDER_8: o << "MESA_SYNCE_DIVIDER_8"; return o;
        case MESA_SYNCE_DIVIDER_16: o << "MESA_SYNCE_DIVIDER_16"; return o;
        case MESA_SYNCE_DIVIDER_25: o << "MESA_SYNCE_DIVIDER_25"; return o;
        default: o << "Unknown"; return o;
    }
}

vtss::ostream &operator<<(vtss::ostream &o, const Clock &i) {
    switch (i) {
        case CLOCK_UNKNOWN: o << "unknown"; return o;
        case CLOCK_1: o << "clk1"; return o;
        case CLOCK_2: o << "clk2"; return o;
        case CLOCK_STATION: o << "station"; return o;
        default: o << "Unexpected-clock-value(" << (int)i << ")"; return o;
    }
}

vtss::ostream &operator<<(vtss::ostream &o, const Indent &i) {
    for (int n = 0; n < i.cnt; n++) o << "    ";
    return o;
}

vtss::ostream &operator<<(vtss::ostream &o, const meba_attr_t attr) {
    switch (attr) {
        case MEBA_ATTR_CLOCK_ID:
            o << "ATTR_CLOCK_ID";
            return o;
        case MEBA_ATTR_FREQ:
            o << "ATTR_FREQ";
            return o;
        case MEBA_ATTR_FREQ_100M:
            o << "ATTR_FREQ_100M";
            return o;
        case MEBA_ATTR_FREQ_1G:
            o << "ATTR_FREQ_1G";
            return o;
        case MEBA_ATTR_FREQ_2_5G:
            o << "ATTR_FREQ_2_5G";
            return o;
        case MEBA_ATTR_FREQ_5G:
            o << "ATTR_FREQ_5G";
            return o;
        case MEBA_ATTR_FREQ_10G:
            o << "ATTR_FREQ_10G";
            return o;
        case MEBA_ATTR_FREQ_25G:
            o << "ATTR_FREQ_25G";
            return o;
        case MEBA_ATTR_INVALID:
        default:
            o << "ATTR_INVALID";
            return o;
    }
}

vtss::ostream &operator<<(vtss::ostream &o, const meba_synce_clock_frequency_t freq) {
    switch (freq) {
        default:
        case MEBA_SYNCE_CLOCK_FREQ_INVALID:
            o<<"FREQ_INVALID";
            return o;
        case MEBA_SYNCE_CLOCK_FREQ_UNKNOWN:
            o<<"FREQ_UNKNOWN";
            return o;
        case MEBA_SYNCE_CLOCK_FREQ_25MHZ:
            o<<"FREQ_25MHZ";
            return o;
        case MEBA_SYNCE_CLOCK_FREQ_31_25MHZ:
            o<<"FREQ_31_25MHZ";
            return o;
        case MEBA_SYNCE_CLOCK_FREQ_32_226MHZ:
            o<<"FREQ_32_226MHZ";
            return o;
        case MEBA_SYNCE_CLOCK_FREQ_39_062MHZ:
            o<<"FREQ_39_062MHZ";
            return o;
        case MEBA_SYNCE_CLOCK_FREQ_40_283MHZ:
            o<<"FREQ_40_283MHZ";
            return o;
        case MEBA_SYNCE_CLOCK_FREQ_60_606MHZ:
            o<<"FREQ_60_606MHZ";
            return o;
        case MEBA_SYNCE_CLOCK_FREQ_62_5MHZ:
            o<<"FREQ_62_5MHZ";
            return o;
        case MEBA_SYNCE_CLOCK_FREQ_78_125MHZ:
            o<<"FREQ_78_125MHZ";
            return o;
        case MEBA_SYNCE_CLOCK_FREQ_80_565MHZ:   // 80.5664062 Mhz
            o<<"FREQ_80_565MHZ";
            return o;
        case MEBA_SYNCE_CLOCK_FREQ_125MHZ:
            o<<"FREQ_125MHZ";
            return o;
        case MEBA_SYNCE_CLOCK_FREQ_156_25MHZ:
            o<<"FREQ_156_25MHZ";
            return o;
        case MEBA_SYNCE_CLOCK_FREQ_161_13MHZ:
            o<<"FREQ_161_13MHZ";
            return o;
        case MEBA_SYNCE_CLOCK_FREQ_312_5MHZ:
            o<<"FREQ_312_5MHZ";
            return o;
        case MEBA_SYNCE_CLOCK_FREQ_322_265MHZ:
            o<<"FREQ_322_265MHZ";
            return o;
        case MEBA_SYNCE_CLOCK_FREQ_644_53MHZ:
            o<<"FREQ_644_53MHZ";
            return o;
        case MEBA_SYNCE_CLOCK_FREQ_10MHZ:
            o<<"FREQ_10MHZ";
            return o;
        case MEBA_SYNCE_CLOCK_FREQ_1544_KHZ:    // 1544 KHz
            o<<"FREQ_1544_KHZ";
            return o;
        case MEBA_SYNCE_CLOCK_FREQ_2048_KHZ:     // 2048 KHz
            o<<"FREQ_2048_KHZ";
            return o;
    }
}

vtss::ostream &operator<<(vtss::ostream &o, const Edge &e) {
    bool insert_colon = false;
    o << "("
      << e.src->dev_id << ":" << e.src_idx << " -> "    
      << e.dst->dev_id << ":" << (e.dst_idx & ~(MESA_SYNCE_DEV_INPUT | MESA_SYNCE_TRI_STATE_FROM_SWITCH | MESA_SYNCE_TRI_STATE_FROM_PHY))
      << "/";
    if (e.dst_idx & MESA_SYNCE_DEV_INPUT) {
        o << "SYNCE_DEV_INPUT";
        insert_colon = true;
    }
    if (e.dst_idx & MESA_SYNCE_TRI_STATE_FROM_SWITCH) {
        o << (insert_colon ? ":" : "") << "TRI_STATE_FROM_SWITCH";
        insert_colon = true;
    }
    if (e.dst_idx & MESA_SYNCE_TRI_STATE_FROM_PHY) {
        o << (insert_colon ? ":" : "") << "TRI_STATE_FROM_PHY";
        insert_colon = true;
    }
    o << " @ ";
    for (auto attr:e.attr) {
        o << "(" << attr.first << ":" << attr.second << ")";
    }
    o << ")";
    if (!e.src->inputs.empty()) {
        o << " inputs: ";
        for (auto ee : e.src->inputs) {
            char buf[32];
            sprintf(buf, "0x%x ", ee.first);
            o << buf;
        }
    }
    o << "\n";
    return o;
}

vtss::ostream &operator<<(vtss::ostream &o, const Path &p) {
    o << "[";
    for (const auto &e : p)
        o << *e;
    o << "]\n";
    return o;
}

vtss::ostream &operator<<(vtss::ostream &o, const Paths &p) {
    
    o << "Object of type Paths i.e. vtss::Vector<Path> [";
    for (const auto &e : p)
        o << e;
    o << "]\n";
    return o;
}

vtss::ostream &operator<<(vtss::ostream &o, const meba_sync_dev_type_t t) {
    switch (t) {
        case MEBA_SYNCE_DEV_TYPE_PORT:
            o<< "DEV_TYPE_PORT";
            break;
        case MEBA_SYNCE_DEV_TYPE_CLOCK_IN:
            o<<"DEV_TYPE_CLOCK_IN";
            break;
        case MEBA_SYNCE_DEV_TYPE_DIVIDER:
            o <<"DEV_TYPE_DIVIDER";
            break;
        case MEBA_SYNCE_DEV_TYPE_DPLL:
            o <<"DEV_TYPE_DPLL";
            break;
        case MEBA_SYNCE_DEV_TYPE_MUX_PHY:
            o <<"DEV_TYPE_MUX_PHY";
            break;
        case MEBA_SYNCE_DEV_TYPE_MUX_BOARD:
            o <<"DEV_TYPE_MUX_BOARD";
            break;
        case MEBA_SYNCE_DEV_TYPE_MUX_SWITCH:
            o <<"DEV_TYPE_MUX_SWITCH";
            break;
        default:
            o <<"DEV_TYPE_UNKNOWN";
            break;
    }
    return o;
}

vtss::ostream &operator<<(vtss::ostream &o, const Node &n) {
    o << "Node type: " << (meba_sync_dev_type_t)n.type << " dev_id: " << n.dev_id << "\n";
    if (n.outputs.size() >0) {
        for (auto &x : n.outputs) {
            o << "  output " << x.first << ": " << x.second;
        }
        o << "\n";
    }
    if (n.inputs.size() >0) {
        for (auto &y : n.inputs) {
            char buf[32];
            sprintf(buf, "0x%x", y.first);
            o << "  input " << buf << ": " << *(y.second);
        }
        o << "\n";
    }
    if (n.void_paths.size()>0) {
        for (auto &z : n.void_paths) {
            o << "  void " << z.first << ": " << z.second;
        }
        o << "\n";
    }
    return o;
}

vtss::ostream &operator<<(vtss::ostream &o, const G &m) {
    o << "Object of type G i.e. vtss::Map<uint32_t, Node> [\n";
    for (const auto &e : m)
        o << e.first << " : " << e.second;
    o << "]\n";
    return o;
}

mesa_synce_divider_t synce_get_switch_clock_divider(int clk_id, int port, mesa_port_speed_t port_speed)
{
    // Current plan
    // - update graph parser to understand the attributes list                   - DONE
    // - Add freq attr for dpll-input
    // - meba_synce_graph_get must detect dpll type when called (and cache the
    //   value).
    // - update synce_get_switch_clock_divider to use the freq-attr to derive
    //   the expected_clock_frequency

    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Entered synce_get_switch_clock_divider with parameters clk_id = " << clk_id << ", port = " << port << " and port_speed = " << port_speed;

    int rcvrd_clock_frequency = 0;
    switch (synce_get_rcvrd_clock_frequency(clk_id, port, port_speed)) {
        case MEBA_SYNCE_CLOCK_FREQ_25MHZ:
            rcvrd_clock_frequency = 25000000;
            break;
        case MEBA_SYNCE_CLOCK_FREQ_39_062MHZ:
            rcvrd_clock_frequency = 39062500;
            break;
        case MEBA_SYNCE_CLOCK_FREQ_40_283MHZ:
            rcvrd_clock_frequency = 40283203;
            break;
        case MEBA_SYNCE_CLOCK_FREQ_60_606MHZ:
            rcvrd_clock_frequency = 60606060;
            break;
        case MEBA_SYNCE_CLOCK_FREQ_62_5MHZ:
            rcvrd_clock_frequency = 62500000;
            break;
        case MEBA_SYNCE_CLOCK_FREQ_78_125MHZ:
            rcvrd_clock_frequency = 78125000;
            break;
        case MEBA_SYNCE_CLOCK_FREQ_80_565MHZ:
            rcvrd_clock_frequency = 80566406;
            break;
        case MEBA_SYNCE_CLOCK_FREQ_125MHZ:
            rcvrd_clock_frequency = 125000000;
            break;
        case MEBA_SYNCE_CLOCK_FREQ_156_25MHZ:
            rcvrd_clock_frequency = 156250000;
            break;
        case MEBA_SYNCE_CLOCK_FREQ_161_13MHZ:
            rcvrd_clock_frequency = 161132812;
            break;
        case MEBA_SYNCE_CLOCK_FREQ_312_5MHZ:
            rcvrd_clock_frequency = 312500000;
            break;
        case MEBA_SYNCE_CLOCK_FREQ_322_265MHZ:
            rcvrd_clock_frequency = 322265625;
            break;
        case MEBA_SYNCE_CLOCK_FREQ_10MHZ:
            rcvrd_clock_frequency = 10000000;
            break;
        case MEBA_SYNCE_CLOCK_FREQ_1544_KHZ:
            rcvrd_clock_frequency = 1544000;
            break;
        case MEBA_SYNCE_CLOCK_FREQ_2048_KHZ:
            rcvrd_clock_frequency = 2048000;
            break;
        case MEBA_SYNCE_CLOCK_FREQ_32_226MHZ:
            rcvrd_clock_frequency = 32226562;
            break;
        case MEBA_SYNCE_CLOCK_FREQ_31_25MHZ:
            rcvrd_clock_frequency = 31250000;
            break;
        default:
            rcvrd_clock_frequency = 125000000;
    }

    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "rcvrd_clock_frequency = " << rcvrd_clock_frequency;

    int expected_clock_frequency=0;
    switch (synce_get_dpll_input_frequency(clk_id, port, port_speed)) {
        case MEBA_SYNCE_CLOCK_FREQ_25MHZ:
            expected_clock_frequency = 25000000;
            break;
        case MEBA_SYNCE_CLOCK_FREQ_39_062MHZ:
            expected_clock_frequency = 39062500;
            break;
        case MEBA_SYNCE_CLOCK_FREQ_40_283MHZ:
            expected_clock_frequency = 40283203;
            break;
        case MEBA_SYNCE_CLOCK_FREQ_60_606MHZ:
            expected_clock_frequency = 60606060;
            break;
        case MEBA_SYNCE_CLOCK_FREQ_62_5MHZ:
            expected_clock_frequency = 62500000;
            break;
        case MEBA_SYNCE_CLOCK_FREQ_78_125MHZ:
            expected_clock_frequency = 78125000;
            break;
        case MEBA_SYNCE_CLOCK_FREQ_80_565MHZ:
            expected_clock_frequency = 80566406;
            break;
        case MEBA_SYNCE_CLOCK_FREQ_125MHZ:
            expected_clock_frequency = 125000000;
            break;
        case MEBA_SYNCE_CLOCK_FREQ_156_25MHZ:
            expected_clock_frequency = 156250000;
            break;
        case MEBA_SYNCE_CLOCK_FREQ_161_13MHZ:
            expected_clock_frequency = 161132812;
            break;
        case MEBA_SYNCE_CLOCK_FREQ_312_5MHZ:
            expected_clock_frequency = 312500000;
            break;
        case MEBA_SYNCE_CLOCK_FREQ_322_265MHZ:
            expected_clock_frequency = 322265625;
            break;
        case MEBA_SYNCE_CLOCK_FREQ_10MHZ:
            expected_clock_frequency = 10000000;
            break;
        case MEBA_SYNCE_CLOCK_FREQ_1544_KHZ:
            expected_clock_frequency = 1544000;
            break;
        case MEBA_SYNCE_CLOCK_FREQ_2048_KHZ:
            expected_clock_frequency = 2048000;
            break;
        case MEBA_SYNCE_CLOCK_FREQ_32_226MHZ:
            expected_clock_frequency = 32226562;
            break;
        case MEBA_SYNCE_CLOCK_FREQ_31_25MHZ:
            expected_clock_frequency = 31250000;
            break;
        default:
            expected_clock_frequency = 125000000;
    }
    
    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "expected_clock_frequency = " << expected_clock_frequency;

    mesa_synce_divider_t clock_divider;
    switch (rcvrd_clock_frequency / expected_clock_frequency) {
        case 1:
            clock_divider = MESA_SYNCE_DIVIDER_1;
            break;
        case 2:
            clock_divider = MESA_SYNCE_DIVIDER_2;
            break;
        case 4:
            clock_divider = MESA_SYNCE_DIVIDER_4;
            break;
        case 5:
            clock_divider = MESA_SYNCE_DIVIDER_5;
            break;
        case 8:
            clock_divider = MESA_SYNCE_DIVIDER_8;
            break;
        case 16:
            clock_divider = MESA_SYNCE_DIVIDER_16;
            break;
        case 25:
            clock_divider = MESA_SYNCE_DIVIDER_25;
            break;
        default:
            clock_divider = MESA_SYNCE_DIVIDER_1;
    }

    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Returning from synce_get_switch_clock_divider with return value = " << clock_divider;
    return clock_divider;
}

// TODO move the MEBA (function is already there but it is a dummy)
void synce_mux_set(u32 mux, int port)
{
    VTSS_TRACE(TRACE_GRP_BOARD, INFO) << "Entered synce_mux_set with parameters mux = " << mux << " and port = " << port;
    
    if (vtss_board_type() == VTSS_BOARD_JAG_PCB107_REF || vtss_board_type() == VTSS_BOARD_JAGUAR2_REF || vtss_board_type() == VTSS_BOARD_JAGUAR2_CU48_REF) {
        u32 input = synce_get_mux_selector(mux, port);

        if (vtss_board_type() == VTSS_BOARD_JAGUAR2_REF && port == SYNCE_STATION_CLOCK_PORT) {
            /* In Zl30772 , input sma is not connected , so using jr2's clock_input_sma */
            u32 jr2_mux_id = 3 , jr2_sma_input = 17;
            VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Call to pcb107_cpld_mux_set with parameters mux = " << mux << " and input = " << input;
            pcb107_cpld_mux_set(jr2_mux_id, jr2_sma_input);
        } else {

            /* set up the multiplexer in the CPLD, pcb107 cpld is also used on PCB110 (alias VTSS_BOARD_JAGUAR2_REF) and PCB111 (alias VTSS_BOARD_JAGUAR2_CU48_REF)*/
            VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Call to pcb107_cpld_mux_set with parameters mux = " << mux << " and input = " << input;
            pcb107_cpld_mux_set(mux, input);
        }
    } else if(vtss_board_type() == VTSS_BOARD_FIREANT_PCB135_REF) {
        VTSS_TRACE(TRACE_GRP_BOARD, INFO) << "A PCB135";
        if (meba_capability(board_instance, MEBA_CAP_BOARD_HAS_PCB135_CPLD)) {
            u32 input = synce_get_mux_selector(mux, port);
            VTSS_TRACE(TRACE_GRP_BOARD, INFO) << "Call to pcb135_cpld_mux_set with parameters mux = " << mux << " and input = " << input;

            pcb135_cpld_mux_set(mux, input);
        } else {
            // PCB135 rev C
            uint32_t src_idx=0;
            internal_graph_get();
            auto path = path_for_clock(gg, (Clock)(mux+1), port);
            VTSS_TRACE(TRACE_GRP_BOARD, INFO) << path;
            for (const auto &e : path) {
                meba_synce_mux_set(board_instance, e->src->dev_id, src_idx, e->src_idx);
                VTSS_TRACE(TRACE_GRP_BOARD, INFO) << *e;
                src_idx = e->dst_idx;
            }
        }
    } else if(vtss_board_type() == VTSS_BOARD_LAN9668_8PORT_REF) {
        if (port<4) {
            // Route recovered clock from port to clk1-out/clk2-out on phy device 100
            // Route clk1-in/clk2-in to clk1-out/clk2-out on phy device 200
            meba_synce_mux_set(board_instance, 100, port|MESA_SYNCE_DEV_INPUT, mux);
            meba_synce_mux_set(board_instance, 200, mux, mux);
        } else {
            // Route recovered clock from port to clk1-out/clk2-out on phy device 200
            meba_synce_mux_set(board_instance, 200, port|MESA_SYNCE_DEV_INPUT, mux);
        }
    } else if (vtss_board_type() == VTSS_BOARD_LAN9694_PCB8398) {
        uint32_t src_idx=0;
        internal_graph_get();
        auto path = path_for_clock(gg, (Clock)(mux+1), port);
        VTSS_TRACE(TRACE_GRP_BOARD, INFO) << path;
        for (const auto &e : path) {
            meba_synce_mux_set(board_instance, e->src->dev_id, src_idx, e->src_idx);
            VTSS_TRACE(TRACE_GRP_BOARD, INFO) << *e;
            src_idx = e->dst_idx;
        }
    } else if (fast_cap(MESA_CAP_SYNCE)) {
        u32 input = synce_get_mux_selector_w_attr(mux, port);

        if (input & MESA_SYNCE_TRI_STATE_FROM_SWITCH) {  // With Caracal-1 and other boards with internal PHY's a "virtual" board mux selects between PHY and SERDES (switch) ports.
            // SERDES is nominated
            mepa_synce_clock_conf_t phy_clock_config;
            phy_clock_config.src = MEPA_SYNCE_CLOCK_SRC_DISABLED;          // Disable any internal PHY input port selection - any port number on internal PHY will do for identification
            phy_clock_config.freq = MEPA_FREQ_125M;            // Frequency is not important since source is going to be disabled anyway
            int i;
            for (i = 0; i < SYNCE_PORT_COUNT; ++i) {
                u32 mux_input = synce_get_mux_selector_w_attr(mux, i);
                if (mux_input & MESA_SYNCE_TRI_STATE_FROM_PHY) break;
            }
            if (i < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) {
                phy_clock_config.dst = (mepa_synce_clock_dst_t) synce_get_phy_recovered_clock(mux, i);
                VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Call to meba_phy_synce_clock_conf_set with parameters port = " << i << ", clk_port = " << phy_clock_config.dst << " and src = " << phy_clock_config.src;
                if (meba_phy_synce_clock_conf_set(board_instance, i, &phy_clock_config) != VTSS_RC_OK) {
                    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Error returned for port = " << i;
                }
                     
            }
        } else if (input & MESA_SYNCE_TRI_STATE_FROM_PHY) {  // With Caracal-1 and other boards with internal PHY's a "virtual" board mux selects between PHY and SERDES (switch) ports.
            // PHY is nominated - disable any SERDES input port selection
            mesa_synce_clk_port_t switch_clk_port = synce_get_switch_recovered_clock(mux, port);
            mesa_synce_clock_in_t clk_in;
            clk_in.enable = false;         // Disable clock source
            clk_in.squelsh = true;         // Squelsh is not important since source is going to be disabled anyway
            int i;
            for (i = 0; i < SYNCE_PORT_COUNT; ++i) {
                u32 mux_input = synce_get_mux_selector_w_attr(mux, i);
                if (mux_input & MESA_SYNCE_TRI_STATE_FROM_SWITCH) break;
            }
            if (i < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) {
                clk_in.port_no = i;
                VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Call to mesa_synce_clock_in_set with parameters clk_port = " << switch_clk_port << ", enable = " << clk_in.enable << " and port_no = " << clk_in.port_no;
                if (mesa_synce_clock_in_set(NULL, switch_clk_port, &clk_in) != VTSS_RC_OK) {
                    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Error returned for port = " << i;
                }
            }
        }
    }

    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Returning from synce_mux_set";
}

void follow_path_to_dpll_(Edge *start, Paths &paths, Path p, int *xx, Indent indent) {
    p.push_back(start);

    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << indent << "Starting from node with dev_id: " << start->dst->dev_id << " of type: " << start->dst->type;

    switch (start->dst->type) {
        case MEBA_SYNCE_DEV_TYPE_PORT:
        case MEBA_SYNCE_DEV_TYPE_DIVIDER:
        case MEBA_SYNCE_DEV_TYPE_MUX_PHY:
        case MEBA_SYNCE_DEV_TYPE_MUX_SWITCH:
        case MEBA_SYNCE_DEV_TYPE_MUX_BOARD:
            break;

        case MEBA_SYNCE_DEV_TYPE_DPLL:
            VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << indent << "Reached input port on DPLL"; 
            paths.push_back(std::move(p));
            return;

        default:
            VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Unsupported/invalid node type in SyncE board graph.";
            return;
    }

    for (auto &e: start->dst->outputs) {
        VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << indent << " port: " << e.first;
        follow_path_to_dpll_(&e.second, paths, p, xx, indent);
    }
}

bool check_valid_path(const G &g, const Path &p) {
    bool ret_val = false;

    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Entered check_valid_path with parameters g = " << g << " and p = " << p;
    if (p.size() != 0) {
        ret_val = true;
        if (p.size() > 1) {
            auto a = p.begin();
            auto b = p.begin() + 1;
        
            for (; b != p.end(); ++a, ++b) {
                auto x = vtss::find((*a)->dst->void_paths.begin(),
                                   (*a)->dst->void_paths.end(),
                                   vtss::make_pair((*a)->dst_idx, (*b)->src_idx));
                if (x != (*a)->dst->void_paths.end()) {
                    ret_val = false;
                    break;
                }
            }
        }
    }
    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Returning from check_valid_path with return value = " << ret_val;
    return ret_val;
}

Paths follow_path_to_dpll(Edge *start, int *xx) {
    Path p;
    Paths paths;

    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Entered follow_path_to_dpll with parameters start = " << start << " and xx = " << xx;
    
    follow_path_to_dpll_(start, paths, p, xx, Indent());

    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Returning from follow_path_to_dpll with return value = " << paths;
    return paths;
}

void map_clock(G m, Clock c) {
    Indent indent;

    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Entered map_clock with parameters m = " << m << " and c = " << c;

    for (auto &n : m) {
        if (n.second.type != MEBA_SYNCE_DEV_TYPE_PORT && n.second.type != MEBA_SYNCE_DEV_TYPE_CLOCK_IN) continue;

        VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Starting from node with dev_id: " << n.second.dev_id << " of type: " << n.second.type;

        for (auto &e: n.second.outputs) {
            VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "  port: " << e.first;

            int xx = 0;
            auto paths = follow_path_to_dpll(&e.second, &xx);

            if (xx) {
                continue;
            }

            Edge *clock_unassigned_edge = nullptr;
            bool clock_assigned = false;

            // check it clock is assigned for one path
            for (auto &ee: paths) {
                if (ee.back()->attr[MEBA_ATTR_CLOCK_ID] == CLOCK_UNKNOWN && !clock_unassigned_edge) {
                    clock_unassigned_edge = ee.back();
                } else if (ee.back()->attr[MEBA_ATTR_CLOCK_ID] == c) {
                    clock_assigned = true;
                }
            }

            if (!clock_assigned && clock_unassigned_edge) {
                clock_unassigned_edge->attr[MEBA_ATTR_CLOCK_ID] = c;
            }

            for (auto &e: paths) {
                VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "    " << e;
            }
        }
    }

    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Returning from map_clock";
}

void internal_graph_get() {
    static const meba_synce_graph_t *g = NULL;

    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Entered internal_graph_get";

    if (g == NULL) {
        // Get the SyncE board graph
        meba_synce_graph_get(board_instance, &g);

        if (g == NULL) {
            VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "meba_synce_graph_get returned NULL pointer";
            return;
        }

        // Create all nodes
        for (size_t i = 0; i < g->graph_length; ++i) {
            gg[g->graph[i].src.dev_id].type = g->graph[i].src.type;
            gg[g->graph[i].src.dev_id].dev_id = g->graph[i].src.dev_id;
            gg[g->graph[i].dst.dev_id].type = g->graph[i].dst.type;
            gg[g->graph[i].dst.dev_id].dev_id = g->graph[i].dst.dev_id;
        }
    
        // Create all connections
        for (size_t i = 0; i < g->graph_length; ++i) {
            if (g->graph[i].type == MEBA_SYNCE_GRAPH_ELEMENT_TYPE_CONNECTION) {
                gg[g->graph[i].src.dev_id].outputs[g->graph[i].src.idx] = {g->graph[i].dst.idx, g->graph[i].src.idx, &gg[g->graph[i].dst.dev_id], &gg[g->graph[i].src.dev_id], {} };
                gg[g->graph[i].dst.dev_id].inputs[g->graph[i].dst.idx] = &gg[g->graph[i].src.dev_id].outputs[g->graph[i].src.idx];
            } else if (g->graph[i].type == MEBA_SYNCE_GRAPH_ELEMENT_TYPE_INVALID_CONF) {
                gg[g->graph[i].src.dev_id].void_paths.push_back(vtss::make_pair(g->graph[i].src.idx, g->graph[i].dst.idx));
            } else {
                VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Unknown edge type";
            }
        }

        for (size_t i = 0; i < g->attr_length; ++i) {
            if (g->attr[i].idx & MESA_SYNCE_DEV_INPUT) {
                if (gg.find(g->attr[i].dev_id) != gg.end() && gg[g->attr[i].dev_id].inputs.find(g->attr[i].idx) != gg[g->attr[i].dev_id].inputs.end()) {
                    if (gg[g->attr[i].dev_id].inputs[g->attr[i].idx]->attr.find(g->attr[i].type) == gg[g->attr[i].dev_id].inputs[g->attr[i].idx]->attr.end()) {
                        gg[g->attr[i].dev_id].inputs[g->attr[i].idx]->attr[g->attr[i].type] = static_cast<meba_synce_clock_frequency_t>(g->attr[i].value);
                    } else {
                        VTSS_TRACE(TRACE_GRP_BOARD, ERROR) << "Attribute with type = " << g->attr[i].type
                                                           << " has already been assigned to edge connected to terminal with dev_idx = " << g->attr[i].idx
                                                           << " on node with dev_id = " << g->attr[i].dev_id;
                    }
                } else {
                    char buf[32];
                    sprintf(buf, "0x%x", g->attr[i].idx);
                    VTSS_TRACE(TRACE_GRP_BOARD, ERROR) << "Attribute with type = " << g->attr[i].type
                                                       << " is referring to (dev_id, dev_idx) = ("
                                                       << g->attr[i].dev_id << ", " << buf
                                                       << ") that could not be found in the board graph.";
                }
            } else {

                if (gg.find(g->attr[i].dev_id) != gg.end() && gg[g->attr[i].dev_id].outputs.find(g->attr[i].idx) != gg[g->attr[i].dev_id].outputs.end()) {
                    if (gg[g->attr[i].dev_id].outputs[g->attr[i].idx].attr.find(g->attr[i].type) == gg[g->attr[i].dev_id].outputs[g->attr[i].idx].attr.end()) {
                        gg[g->attr[i].dev_id].outputs[g->attr[i].idx].attr[g->attr[i].type] = static_cast<meba_synce_clock_frequency_t>(g->attr[i].value);
                    } else {
                        VTSS_TRACE(TRACE_GRP_BOARD, ERROR) << "Attribute with type = " << g->attr[i].type
                                                           << " has already been assigned to edge connected to terminal with dev_idx = " << g->attr[i].idx
                                                           << " on node with dev_id = " << g->attr[i].dev_id;
                    }
                } else {
                    VTSS_TRACE(TRACE_GRP_BOARD, ERROR) << "Attribute with type = " << g->attr[i].type
                                                       << " is referring to (dev_id, dev_idx) = ("
                                                       << g->attr[i].dev_id << ", " << g->attr[i].idx
                                                       << ") that could not be found in the board graph.";
                }
            }
        }

        // Map clocks
        map_clock(gg, CLOCK_1);
        map_clock(gg, CLOCK_2);
        map_clock(gg, CLOCK_STATION);
    }

    VTSS_TRACE(TRACE_GRP_BOARD, NOISE) << "Returning from internal_graph_get with return value = " << gg;
    return;
}

Path path_for_clock(G &g, Clock c, uint32_t dev_id) {
    Path path;

    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Entered path_for_clock with parameters g = " << g << ", c = " << c << " and dev_id = " << dev_id;

    for (auto &e : g) {
        if (e.second.type != MEBA_SYNCE_DEV_TYPE_PORT && e.second.type != MEBA_SYNCE_DEV_TYPE_CLOCK_IN) continue;
        if (e.second.dev_id != dev_id) continue;

        auto edge = e.second.outputs.find(0);
        if (edge == e.second.outputs.end()) continue;

        int xx = 0;
        auto paths = follow_path_to_dpll(&(edge->second), &xx);

        for (auto &ee: paths) {
            if (!check_valid_path(g, ee)) continue;

            if (ee.back()->attr[MEBA_ATTR_CLOCK_ID] == c) {
                path = ee;
            }
        }
    }

    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Returning from path_for_clock with return value = " << path;
    return path;
}

bool nominate(G &g, Clock c, uint32_t dev_id) {
    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Entered nominate with parameters g = " << g << ", c = " << c << " and dev_id = " << dev_id;
    auto p = path_for_clock(g, c, dev_id);
    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Returning from nominate with return value = " << p.size();
    return p.size();
}

int dpll_input(G &g, Clock c, uint32_t dev_id) {
    int ret_val = -1;
    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Entered dpll_input with parameters g = " << g << ", c = " << c << " and dev_id = " << dev_id;
    auto p = path_for_clock(g, c, dev_id);
    if (p.size()) ret_val = p.back()->dst_idx & ~MESA_SYNCE_DEV_INPUT;
    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Returning from dpll_input with return value = " << ret_val;
    return ret_val;
}

int phy_recovered_clock(G &g, Clock c, uint32_t dev_id) {
    int ret_val = -1;
    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Entered phy_recovered_clock with parameters g = " << g << ", c = " << c << " and dev_id = " << dev_id;
    auto p = path_for_clock(g, c, dev_id);
    if (p.size()) {
        for (auto &e: p) {
            if (e->src->type == MEBA_SYNCE_DEV_TYPE_MUX_PHY) {
                ret_val = e->src_idx;
                break;
            }
        }
    }
    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Returning from phy_recovered_clock with return value = " << ret_val;
    return ret_val;
}

int switch_recovered_clock(G &g, Clock c, uint32_t dev_id) {
    int ret_val = -1;
    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Entered switch_recovered_clock with parameters g = " << g << ", c = " << c << " and dev_id = " << dev_id;
    auto p = path_for_clock(g, c, dev_id);
    if (p.size()) {
        for (auto &e: p) {
            if (e->src->type == MEBA_SYNCE_DEV_TYPE_MUX_SWITCH) {
                ret_val = e->src_idx;
                break;
            }
        }
    }
    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Returning from switch_recovered_clock with return value = " << ret_val;
    return ret_val;
}

meba_attr_t port_speed_2_meba_attr(mesa_port_speed_t port_speed)
{
    switch (port_speed) {
        case MESA_SPEED_100M:  return MEBA_ATTR_FREQ_100M;
        case MESA_SPEED_1G:    return MEBA_ATTR_FREQ_1G;
        case MESA_SPEED_2500M: return MEBA_ATTR_FREQ_2_5G;
        case MESA_SPEED_5G:    return MEBA_ATTR_FREQ_5G;
        case MESA_SPEED_10G:   return MEBA_ATTR_FREQ_10G;
        case MESA_SPEED_25G:   return MEBA_ATTR_FREQ_25G;
        default:               return MEBA_ATTR_INVALID;
    }
}

meba_synce_clock_frequency_t clock_frequency(G &g, Clock c, uint32_t dev_id, mesa_port_speed_t port_speed) {
    meba_synce_clock_frequency_t freq = MEBA_SYNCE_CLOCK_FREQ_INVALID;
    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Entered clock_frequency with parameters g = " << g << ", c = " << c << " and dev_id = " << dev_id;
    auto p = path_for_clock(g, c, dev_id);
    if (p.size()) {
        freq = MEBA_SYNCE_CLOCK_FREQ_UNKNOWN;
        for (auto &e: p) {
            if (e->attr.find(port_speed_2_meba_attr(port_speed)) != e->attr.end()) {
                freq = static_cast<meba_synce_clock_frequency_t>(e->attr[port_speed_2_meba_attr(port_speed)]);
                break;
            } else if (e->attr[MEBA_ATTR_FREQ] != MEBA_SYNCE_CLOCK_FREQ_UNKNOWN) {
                freq = static_cast<meba_synce_clock_frequency_t>(e->attr[MEBA_ATTR_FREQ]);
                break;
            }
        }
    }
    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Returning from clock_frequency with return value = " << freq;
    return freq;
}

meba_synce_clock_frequency_t dpll_frequency(G &g, Clock c, uint32_t dev_id, mesa_port_speed_t port_speed) {
    meba_synce_clock_frequency_t freq = MEBA_SYNCE_CLOCK_FREQ_INVALID;
    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Entered dpll_frequency with parameters g = " << g << ", c = " << c << " and dev_id = " << dev_id;
    auto p = path_for_clock(g, c, dev_id);
    if (p.size()) {
        freq = MEBA_SYNCE_CLOCK_FREQ_UNKNOWN;
        for (auto e = p.rbegin(); e != p.rend(); ++e) {
            if ((*e)->attr.find(port_speed_2_meba_attr(port_speed)) != (*e)->attr.end()) {
                freq = static_cast<meba_synce_clock_frequency_t>((*e)->attr[port_speed_2_meba_attr(port_speed)]);
                break;
            } else if ((*e)->attr[MEBA_ATTR_FREQ] != MEBA_SYNCE_CLOCK_FREQ_UNKNOWN) {
                freq = static_cast<meba_synce_clock_frequency_t>((*e)->attr[MEBA_ATTR_FREQ]);
                break;
            }
        }
    }
    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Returning from dpll_frequency with return value = " << freq;
    return freq;
}

meba_synce_clock_frequency_t phy_output_frequency(G &g, Clock c, uint32_t dev_id, mesa_port_speed_t port_speed) {
    meba_synce_clock_frequency_t freq = MEBA_SYNCE_CLOCK_FREQ_INVALID;
    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Entered phy_output_frequency with parameters g = " << g << ", c = " << c << " and dev_id = " << dev_id;
    auto p = path_for_clock(g, c, dev_id);
    if (p.size()) {
        freq = MEBA_SYNCE_CLOCK_FREQ_UNKNOWN;
        for (auto e = p.rbegin(); e != p.rend(); ++e) {
            if ((*e)->attr.find(port_speed_2_meba_attr(port_speed)) != (*e)->attr.end()) {
                freq = static_cast<meba_synce_clock_frequency_t>((*e)->attr[port_speed_2_meba_attr(port_speed)]);
            } else if ((*e)->attr[MEBA_ATTR_FREQ] != MEBA_SYNCE_CLOCK_FREQ_UNKNOWN) {
                freq = static_cast<meba_synce_clock_frequency_t>((*e)->attr[MEBA_ATTR_FREQ]);
            }
            if ((*e)->src->type == MEBA_SYNCE_DEV_TYPE_MUX_PHY) {
                break;
            }
        }
    }
    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Returning from phy_output_frequency with return value = " << freq;
    return freq;
}

bool synce_get_source_port(int source, int port)
{
    bool ret_val;
    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Entered synce_get_source_port with parameters  source = " << source << " and port = " << port;
    internal_graph_get();
#if defined(VTSS_SW_OPTION_PTP)
    if (port >= SYNCE_PORT_COUNT && source != STATION_CLOCK_SOURCE_NO) {
        ret_val = true;
    }
#else
    if (0) {}
#endif
    else {
        ret_val = nominate(gg, (Clock)(source + 1), port);
    }
    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Returning from synce_get_source_port with return value = " << ret_val;
    return ret_val;
}

int synce_get_selector_ref_no(int source, int port)
{
    VTSS_TRACE(TRACE_GRP_BOARD, INFO) << "Entered synce_get_selector_ref_no with parameters source = " << source << " and port = " << port;
    internal_graph_get();
    int ret_val = dpll_input(gg, (Clock)(source + 1), port);
    VTSS_TRACE(TRACE_GRP_BOARD, INFO) << "Returning from synce_get_selector_ref_no with return value = " << ret_val;
    return ret_val;
}

int synce_get_switch_recovered_clock(int source, int port)
{
    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Entered synce_get_switch_recovered_clock with parameters source = " << source << " and port = " << port;
    internal_graph_get();
    int ret_val = switch_recovered_clock(gg, (Clock)(source + 1), port);
    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Returning from synce_get_switch_recovered_clock with return value = " << ret_val;
    return ret_val;
}

int synce_get_phy_recovered_clock(int source, int port)
{
    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Entered synce_get_phy_recovered_clock with parameters source = " << source << " and port = " << port;
    internal_graph_get();
    int ret_val = phy_recovered_clock(gg, (Clock)(source + 1), port);
    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Returning from synce_get_phy_recovered_clock with return value = " << ret_val;
    return ret_val;
}

meba_synce_clock_frequency_t synce_get_rcvrd_clock_frequency(int source, int port, mesa_port_speed_t port_speed)
{
    meba_synce_clock_frequency_t ret_val;
    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Entered synce_get_rcvrd_clock_frequency with parameters source = " << source << ", port = " << port << " and port_speed = " << port_speed;
    internal_graph_get();
#if defined(VTSS_SW_OPTION_PTP)
    if (port >= SYNCE_PORT_COUNT) {
        ret_val = MEBA_SYNCE_CLOCK_FREQ_UNKNOWN;
    }
#else
    if (0) {}
#endif
    else {
        ret_val = clock_frequency(gg, (Clock)(source + 1), port, port_speed);
    }
    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Returning from synce_get_rcvrd_clock_frequency with return value = " << ret_val;
    return ret_val;
}

meba_synce_clock_frequency_t synce_get_dpll_input_frequency(int source, int port, mesa_port_speed_t port_speed)
{
    meba_synce_clock_frequency_t ret_val;
    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Entered synce_get_dpll_input_frequency with parameters source = " << source << ", port = " << port << " and port_speed = " << port_speed;
    internal_graph_get();
#if defined(VTSS_SW_OPTION_PTP)
    if (port >= SYNCE_PORT_COUNT) {
        ret_val = MEBA_SYNCE_CLOCK_FREQ_UNKNOWN;
    }
#else
    if (0) {}
#endif
    else {
        ret_val = dpll_frequency(gg, (Clock)(source + 1), port, port_speed);
    }
    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Returning from synce_get_dpll_input_frequency with return value = " << ret_val;
    return ret_val;
}

meba_synce_clock_frequency_t synce_get_phy_output_frequency(int source, int port, mesa_port_speed_t port_speed)
{
    meba_synce_clock_frequency_t ret_val;
    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Entered synce_get_phy_output_frequency with parameters source = " << source << ", port = " << port << " and port_speed = " << port_speed;
    internal_graph_get();
#if defined(VTSS_SW_OPTION_PTP)
    if (port >= SYNCE_PORT_COUNT) {
        ret_val = MEBA_SYNCE_CLOCK_FREQ_UNKNOWN;
    }
#else
    if (0) {}
#endif
    else {
        ret_val = phy_output_frequency(gg, (Clock)(source + 1), port, port_speed);
    }
    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Returning from synce_get_phy_output_frequency with return value = " << ret_val;
    return ret_val;
}

int board_mux_input(G &g, Clock c, uint32_t dev_id) {
    int ret_val = 20;     // FIXME: This is a constant meaning "no clock" i.e. no path to DPLL input (is for instance the case for PTP clocks) - should be replaced by a defined constant
    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Entered board_mux_input with parameters g = " << g << ", c = " << c << " and dev_id = " << dev_id;
    auto p = path_for_clock(g, c, dev_id);
    if (p.size()) {
        for (auto &e: p) {
            if (e->dst->type == MEBA_SYNCE_DEV_TYPE_MUX_BOARD) {
                ret_val = e->dst_idx;
                break;
            }
        }
    }
    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Returning from board_mux_input with return value = " << ret_val;
    return ret_val;
}

u32 synce_get_mux_selector_w_attr(int source, int port)
{
    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Entered synce_get_mux_selector_w_attr with parameters source = " << source << " and port = " << port;
    internal_graph_get();
    u32 ret_val = board_mux_input(gg, (Clock)(source + 1), port);
    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Returning from synce_get_mux_selector_w_attr with return value = " << ret_val;
    return ret_val;
}

u32 synce_get_mux_selector(int source, int port)
{
    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Entered synce_get_mux_selector with parameters source = " << source << " and port = " << port;
    internal_graph_get();
    u32 ret_val = board_mux_input(gg, (Clock)(source + 1), port) & ~(MESA_SYNCE_DEV_INPUT | MESA_SYNCE_TRI_STATE_FROM_SWITCH | MESA_SYNCE_TRI_STATE_FROM_PHY);
    VTSS_TRACE(TRACE_GRP_BOARD, DEBUG) << "Returning from synce_get_mux_selector with return value = " << ret_val;
    return ret_val;
}

#ifdef __cplusplus
extern "C" {
#endif
void synce_icli_debug_graph_dump(i32 session_id)
{
    vtss::fdstream o(session_id);
    internal_graph_get();
    o << gg;
}

void synce_icli_path_for_clock(i32 session_id, uint32_t clk, uint32_t devid)
{
    vtss::fdstream o(session_id);
    internal_graph_get();
    o << path_for_clock(gg, (Clock)(clk+1), devid) << "\n";
}
#ifdef __cplusplus
}
#endif
