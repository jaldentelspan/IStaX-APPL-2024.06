/*
 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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

/**
 * \file frr_ospf_access_test.cxx
 * \brief This file is used to test the APIs in FRR access layer.
 * Follow the procedures below to execute the file.
 * 1. cd webstax2/vtss_appl/vtss_basics
 * 2. mkdir build
 * 3. cd build
 * 4. cmake ..
 * 5. make -j8
 * 6. ./frr/frr_tests
*/

#include "../ospf/frr_ospf_access.hxx"
#include <string.h>
#include <vtss/basics/api_types.h>
#include <frr_daemon.hxx>
#include <vtss/basics/string.hxx>
#include "catch.hpp"

TEST_CASE("frr_ip_ospf_status", "[frr]") {
    std::string ip_ospf = R"({
"routerId":"192.168.2.1",
"tosRoutesOnly":true,
"rfc2328Conform":true,
"spfScheduleDelayMsecs":0,
"holdtimeMinMsecs":50,
"holdtimeMaxMsecs":5000,
"holdtimeMultplier":1,
"spfLastExecutedMsecs":766096,
"spfLastDurationMsecs":0,
"lsaMinIntervalMsecs":5000,
"lsaMinArrivalMsecs":1000,
"writeMultiplier":20,
"refreshTimerMsecs":10000,
"lsaExternalCounter":0,
"lsaExternalChecksum":0,
"lsaAsopaqueCounter":0,
"lsaAsOpaqueChecksum":0,
"attachedAreaCounter":1,
"areas":{
    "0.0.0.1":{
    "backbone":true,
    "areaIfTotalCounter":2,
    "areaIfActiveCounter":1,
    "nbrFullAdjacentCounter":0,
    "authentication":"authenticationNone",
    "spfExecutedCounter":1,
    "lsaNumber":1,
    "lsaRouterNumber":1,
    "lsaRouterChecksum":31025,
    "lsaNetworkNumber":1,
    "lsaNetworkChecksum":2,
    "lsaSummaryNumber":3,
    "lsaSummaryChecksum":4,
    "lsaAsbrNumber":5,
    "lsaAsbrChecksum":6,
    "lsaNssaNumber":7,
    "lsaNssaChecksum":8,
    "lsaOpaqueLinkNumber":9,
    "lsaOpaqueLinkChecksum":10,
    "lsaOpaqueAreaNumber":11,
    "lsaOpaqueAreaChecksum":12
    }
}
}
)";
    auto result = vtss::frr_ip_ospf_status_parse(ip_ospf);
    CHECK(result.router_id == 0xc0a80201);
    CHECK(result.tos_routes_only == true);
    CHECK(result.rfc2328 == true);
    CHECK(result.spf_schedule_delay.raw() == 0);
    CHECK(result.hold_time_min.raw() == 50);
    CHECK(result.hold_time_max.raw() == 5000);
    CHECK(result.hold_time_multiplier == 1);
    CHECK(result.spf_last_executed.raw() == 766096);
    CHECK(result.spf_last_duration.raw() == 0);
    CHECK(result.lsa_min_interval.raw() == 5000);
    CHECK(result.lsa_min_arrival.raw() == 1000);
    CHECK(result.write_multiplier == 20);
    CHECK(result.refresh_timer.raw() == 10000);
    CHECK(result.lsa_external_counter == 0);
    CHECK(result.lsa_external_checksum == 0);
    CHECK(result.lsa_asopaque_counter == 0);
    CHECK(result.lsa_asopaque_checksums == 0);
    CHECK(result.attached_area_counter == 1);
    CHECK(result.areas.size() == 1);
    vtss::FrrIpOspfArea area = result.areas[0x01];
    CHECK(area.backbone == true);
    CHECK(area.stub_no_summary == false);
    CHECK(area.stub_shortcut == false);
    CHECK(area.area_if_total_counter == 2);
    CHECK(area.area_if_activ_counter == 1);
    CHECK(area.full_adjancet_counter == 0);
    CHECK(area.authentication == VTSS_APPL_OSPF_AUTH_TYPE_NULL);
    CHECK(area.spf_executed_counter == 1);
    CHECK(area.lsa_nr == 1);
    CHECK(area.lsa_router_nr == 1);
    CHECK(area.lsa_router_checksum == 31025);
    CHECK(area.lsa_network_nr == 1);
    CHECK(area.lsa_network_checksum == 2);
    CHECK(area.lsa_summary_nr == 3);
    CHECK(area.lsa_summary_checksum == 4);
    CHECK(area.lsa_asbr_nr == 5);
    CHECK(area.lsa_asbr_checksum == 6);
    CHECK(area.lsa_nssa_nr == 7);
    CHECK(area.lsa_nssa_checksum == 8);
    CHECK(area.lsa_opaque_link_nr == 9);
    CHECK(area.lsa_opaque_link_checksum == 10);
    CHECK(area.lsa_opaque_area_nr == 11);
    CHECK(area.lsa_opaque_area_checksum == 12);
}

TEST_CASE("frr_ip_ospf_status_get2", "[frr]") {
    std::string ip_ospf = R"({
"routerId":"12.0.0.122",
"tosRoutesOnly":true,
"rfc2328Conform":true,
"spfScheduleDelayMsecs":0,
"holdtimeMinMsecs":50,
"holdtimeMaxMsecs":5000,
"holdtimeMultplier":1,
"spfLastExecutedMsecs":104238,
"spfLastDurationMsecs":1,
"lsaMinIntervalMsecs":5000,
"lsaMinArrivalMsecs":1000,
"writeMultiplier":20,
"refreshTimerMsecs":10000,
"lsaExternalCounter":0,
"lsaExternalChecksum":0,
"lsaAsopaqueCounter":0,
"lsaAsOpaqueChecksum":0,
"attachedAreaCounter":1,
"areas":{
    "0.0.0.2":{
        "backbone":false,
        "stubNoSummary":true,
        "stubShortcut":true,
        "areaIfTotalCounter":2,
        "areaIfActiveCounter":2,
        "nbrFullAdjacentCounter":2,
        "authentication":"authenticationNone",
        "spfExecutedCounter":6,
        "lsaNumber":6,
        "lsaRouterNumber":3,
        "lsaRouterChecksum":90108,
        "lsaNetworkNumber":3,
        "lsaNetworkChecksum":64139,
        "lsaSummaryNumber":0,
        "lsaSummaryChecksum":0,
        "lsaAsbrNumber":0,
        "lsaAsbrChecksum":0,
        "lsaNssaNumber":0,
        "lsaNssaChecksum":0,
        "lsaOpaqueLinkNumber":0,
        "lsaOpaqueLinkChecksum":0,
        "lsaOpaqueAreaNumber":0,
        "lsaOpaqueAreaChecksum":0
        }
    }
}
)";

    auto result = vtss::frr_ip_ospf_status_parse(ip_ospf);
    CHECK(result.router_id == 0x0c00007A);
    CHECK(result.tos_routes_only == true);
    CHECK(result.rfc2328 == true);
    CHECK(result.spf_schedule_delay.raw() == 0);
    CHECK(result.hold_time_min.raw() == 50);
    CHECK(result.hold_time_max.raw() == 5000);
    CHECK(result.hold_time_multiplier == 1);
    CHECK(result.spf_last_executed.raw() == 104238);
    CHECK(result.spf_last_duration.raw() == 1);
    CHECK(result.lsa_min_interval.raw() == 5000);
    CHECK(result.lsa_min_arrival.raw() == 1000);
    CHECK(result.write_multiplier == 20);
    CHECK(result.refresh_timer.raw() == 10000);
    CHECK(result.lsa_external_counter == 0);
    CHECK(result.lsa_external_checksum == 0);
    CHECK(result.lsa_asopaque_counter == 0);
    CHECK(result.lsa_asopaque_checksums == 0);
    CHECK(result.attached_area_counter == 1);
    CHECK(result.areas.size() == 1);
    vtss::FrrIpOspfArea area = result.areas[0x02];
    CHECK(area.backbone == false);
    CHECK(area.stub_no_summary == true);
    CHECK(area.stub_shortcut == true);
}

TEST_CASE("frr_ip_ospf_status_get3", "[frr]") {
    std::string ip_ospf = R"({
"routerId":"0.0.0.3",
"tosRoutesOnly":true,
"rfc2328Conform":true,
"spfScheduleDelayMsecs":0,
"holdtimeMinMsecs":50,
"holdtimeMaxMsecs":5000,
"holdtimeMultplier":1,
"spfLastExecutedMsecs":1068238,
"spfLastDurationMsecs":0,
"lsaMinIntervalMsecs":5000,
"lsaMinArrivalMsecs":1000,
"writeMultiplier":20,
"refreshTimerMsecs":10000,
"abrType":"Alternative Cisco",
"asbrRouter":"injectingExternalRoutingInformation",
"lsaExternalCounter":3,
"lsaExternalChecksum":79876,
"lsaAsopaqueCounter":0,
"lsaAsOpaqueChecksum":0,
"attachedAreaCounter":3,
"areas":{
    "0.0.0.6":{
        "shortcuttingMode":"Default",
        "areaIfTotalCounter":1,
        "areaIfActiveCounter":1,
        "nssa":true,
        "abr":true,
        "nssaTranslatorElected":true,
        "nbrFullAdjacentCounter":1,
        "authentication":"authenticationNone",
        "virtualAdjacenciesPassingCounter":0,
        "spfExecutedCounter":28,
        "lsaNumber":23,
        "lsaRouterNumber":2,
        "lsaRouterChecksum":45520,
        "lsaNetworkNumber":1,
        "lsaNetworkChecksum":44753,
        "lsaSummaryNumber":19,
        "lsaSummaryChecksum":562893,
        "lsaAsbrNumber":0,
        "lsaAsbrChecksum":0,
        "lsaNssaNumber":1,
        "lsaNssaChecksum":34867,
        "lsaOpaqueLinkNumber":0,
        "lsaOpaqueLinkChecksum":0,
        "lsaOpaqueAreaNumber":0,
        "lsaOpaqueAreaChecksum":0
        }
    }
}
)";

    auto result = vtss::frr_ip_ospf_status_parse(ip_ospf);
    CHECK(result.router_id == 0x00000003);
    CHECK(result.tos_routes_only == true);
    CHECK(result.rfc2328 == true);
    CHECK(result.spf_schedule_delay.raw() == 0);
    CHECK(result.hold_time_min.raw() == 50);
    CHECK(result.hold_time_max.raw() == 5000);
    CHECK(result.hold_time_multiplier == 1);
    CHECK(result.spf_last_executed.raw() == 1068238);
    CHECK(result.spf_last_duration.raw() == 0);
    CHECK(result.lsa_min_interval.raw() == 5000);
    CHECK(result.lsa_min_arrival.raw() == 1000);
    CHECK(result.write_multiplier == 20);
    CHECK(result.refresh_timer.raw() == 10000);
    CHECK(result.lsa_external_counter == 3);
    CHECK(result.lsa_external_checksum == 79876);
    CHECK(result.lsa_asopaque_counter == 0);
    CHECK(result.lsa_asopaque_checksums == 0);
    CHECK(result.attached_area_counter == 3);
    CHECK(result.areas.size() == 1);
    vtss::FrrIpOspfArea area = result.areas[0x06];
    CHECK(area.backbone == false);
    CHECK(area.stub_no_summary == false);
    CHECK(area.stub_shortcut == false);
    CHECK(area.nssa_translator_elected == true);
    CHECK(area.nssa_translator_always == false);
}

TEST_CASE("frr_ip_ospf_interface_parse", "[frr]") {
    SECTION("interface is up") {
        std::string ip_ospf_if = R"({
"interfaces":[
{
"yellow":{
    "ifUp":true,
    "mtuBytes":1500,
    "bandwidthMbit":1000,
    "ifFlags":"<UP,BROADCAST,RUNNING,MULTICAST>",
    "ospfEnabled":true,
    "ipAddress":"192.168.1.1",
    "ipAddressPrefixlen":24,
    "ospfIfType":"Broadcast",
    "localIfUsed":"192.168.1.255",
    "area":"0.0.0.0",
    "routerId":"0.0.0.1",
    "networkType":"BROADCAST",
    "cost":100,
    "transmitDelayMsecs":1000,
    "state":"Backup",
    "priority":1,
    "bdrId":"0.0.0.1",
    "bdrAddress":"192.168.1.1",
    "networkLsaSequence":2147483646,
    "mcastMemberOspfAllRouters":true,
    "mcastMemberOspfDesignatedRouters":true,
    "timerMsecs":100,
    "timerDeadMsecs":25,
    "timerWaitMsecs":25,
    "timerRetransmit":200,
    "timerHelloInMsecs":5726,
    "timerPassiveIface":true,
    "nbrCount":1,
    "nbrAdjacentCount":1
}
}
]
}
)";

        auto result = vtss::frr_ip_ospf_interface_status_parse(ip_ospf_if);
        vtss::FrrIpOspfIfStatus &status = result.begin()->second;
        CHECK(status.if_up == true);
        CHECK(status.mtu_bytes == 1500);
        CHECK(status.bandwidth_mbit == 1000);
        CHECK(status.if_flags == "<UP,BROADCAST,RUNNING,MULTICAST>");
        CHECK(status.ospf_enabled == true);
        CHECK(status.net.address == 0xc0a80101);
        CHECK(status.net.prefix_size == 24);
        CHECK(status.if_type == vtss::IfType_Broadcast);
        CHECK(status.local_if_used == 0xc0a801FF);
        CHECK(status.area.area == 0x0);
        CHECK(status.router_id == 0x1);
        CHECK(status.network_type == vtss::NetworkType_Broadcast);
        CHECK(status.cost == 100);
        CHECK(status.transmit_delay.raw() == 1000);
        CHECK(status.state == vtss::ISM_Backup);
        CHECK(status.priority == 1);
        CHECK(status.bdr_id == 0x01);
        CHECK(status.bdr_address == 0xc0a80101);
        CHECK(status.network_lsa_sequence == 2147483646);
        CHECK(status.mcast_member_ospf_all_routers == true);
        CHECK(status.mcast_member_ospf_designated_routers == true);
        CHECK(status.timer.raw() == 100);
        CHECK(status.timer_dead.raw() == 25);
        CHECK(status.timer_wait.raw() == 25);
        CHECK(status.timer_retransmit.raw() == 200);
        CHECK(status.timer_hello.raw() == 5726);
        CHECK(status.timer_passive_iface == true);
        CHECK(status.nbr_count == 1);
        CHECK(status.nbr_adjacent_count == 1);
    }

    SECTION("interface is down") {
        std::string ip_ospf_if = R"({
"interfaces":[
{
"yellow":{
    "ifDown":false,
    "ifIndex":6,
    "mtuBytes":1500,
    "bandwidthMbit":0,
    "ifFlags":"<BROADCAST,MULTICAST>",
    "ospfRunning":false
}
}
]
}
)";
        auto result = vtss::frr_ip_ospf_interface_status_parse(ip_ospf_if);
        vtss::FrrIpOspfIfStatus &status = result.begin()->second;
        CHECK(status.if_up == false);
        CHECK(status.mtu_bytes == 1500);
        CHECK(status.bandwidth_mbit == 0);
        CHECK(status.if_flags == "<BROADCAST,MULTICAST>");
        CHECK(status.ospf_enabled == false);
    }

    SECTION("interface frr vlink") {
        const std::string ip_ospf_if = R"({
"interfaces":[
{
"VLINK0":{
    "ifDown":false,
    "mtuBytes":3500,
    "bandwidthMbit":0,
    "ifFlags":"<BROADCAST,MULTICAST>",
    "ospfRunning":false
}
}
]
}
)";
        auto result = vtss::frr_ip_ospf_interface_status_parse(ip_ospf_if);
        vtss::FrrIpOspfIfStatus &status = result.begin()->second;
        CHECK(status.if_up == false);
        CHECK(status.mtu_bytes == 3500);
        CHECK(status.bandwidth_mbit == 0);
        CHECK(status.if_flags == "<BROADCAST,MULTICAST>");
        CHECK(status.ospf_enabled == false);
    }
}

TEST_CASE("frr_ip_ospf_neighbor_parse", "[frr]") {
    std::string ip_ospf_neighbor = R"({
"192.168.1.3": [
    {
    "ifaceAddress":"192.168.1.3",
    "areaId":"0.0.0.0",
    "ifaceName":"enp9s0f0",
    "nbrPriority":1,
    "nbrState":"Full",
    "stateChangeCounter":5,
    "routerDesignatedId":"192.168.1.3",
    "routerDesignatedBackupId":"192.168.1.1",
    "optionsCounter":3,
    "optionsList":"*|-|-|-|-|-|E|-",
    "routerDeadIntervalTimerDueMsec":31663,
    "databaseSummaryListCounter":0,
    "linkStateRequestListCounter":0,
    "linkStateRetransmissionListCounter":0,
    "threadInactivityTimer":"on"
  },
  {
    "ifaceAddress":"1.0.1.2",
    "areaId":"0.0.0.0",
    "ifaceName":"VLINK0",
    "transitAreaId":"0.0.0.0",
    "nbrPriority":1,
    "nbrState":"2-Way",
    "stateChangeCounter":2,
    "lastPrgrsvChangeMsec":4478398,
    "routerDesignatedId":"1.0.1.10",
    "routerDesignatedBackupId":"1.0.1.9",
    "optionsCounter":2,
    "optionsList":"*|-|-|-|-|-|E|-",
    "routerDeadIntervalTimerDueMsec":31663,
    "databaseSummaryListCounter":0,
    "linkStateRequestListCounter":0,
    "linkStateRetransmissionListCounter":0,
    "threadInactivityTimer":"on"
  }
  ]
}
)";

    auto result = vtss::frr_ip_ospf_neighbor_status_parse(ip_ospf_neighbor);
    vtss::FrrIpOspfNeighborStatus &neighbor = result[0xc0a80103][0];
    CHECK(neighbor.if_address == 0xc0a80103);
    CHECK(neighbor.area.area == 0);
    CHECK(neighbor.nbr_priority == 1);
    CHECK(neighbor.nbr_state == vtss::NSM_Full);
    CHECK(neighbor.dr_ip_addr == 0xc0a80103);
    CHECK(neighbor.bdr_ip_addr == 0xc0a80101);
    CHECK(neighbor.options_counter == 3);
    CHECK(neighbor.options_list == "*|-|-|-|-|-|E|-");

    vtss::FrrIpOspfNeighborStatus &neighbor1 = result[0xc0a80103][1];
    CHECK(neighbor1.if_address == 0x01000102);
    CHECK(neighbor1.area.area == 0);
    CHECK(neighbor1.nbr_priority == 1);
    CHECK(neighbor1.nbr_state == vtss::NSM_TwoWay);
    CHECK(neighbor1.dr_ip_addr == 0x0100010a);
    CHECK(neighbor1.bdr_ip_addr == 0x01000109);
    CHECK(neighbor1.options_counter == 2);
    CHECK(neighbor1.options_list == "*|-|-|-|-|-|E|-");
}

TEST_CASE("frr_ip_ospf_neighbor_status_parse", "[frr]") {
    const std::string ip_ospf_neighbor = R"({
"192.168.1.3": [
    {
    "ifaceAddress":"192.168.1.3",
    "areaId":"0.0.0.1 [Stub]",
    "ifaceName":"vtss.vlan.1",
    "nbrPriority":1,
    "nbrState":"Full",
    "stateChangeCounter":5,
    "routerDesignatedId":"192.168.1.3",
    "routerDesignatedBackupId":"192.168.1.2",
    "optionsCounter":2,
    "optionsList":"*||||||E|",
    "routerDeadIntervalTimerDueMsec":34465,
    "databaseSummaryListCounter":0,
    "linkStateRequestListCounter":0,
    "linkStateRetransmissionListCounter":0,
    "threadInactivityTimer":"on",
    "threadLinkStateRequestRetransmission":"on",
    "threadLinkStateUpdateRetransmission":"on"
  }
  ]
}
)";

    auto result = vtss::frr_ip_ospf_neighbor_status_parse(ip_ospf_neighbor);
    vtss::FrrIpOspfNeighborStatus &neighbor = result[0xc0a80103][0];
    CHECK(neighbor.if_address == 0xc0a80103);
    CHECK(neighbor.area.area == 1);
    CHECK(neighbor.nbr_priority == 1);
    CHECK(neighbor.nbr_state == vtss::NSM_Full);
    CHECK(neighbor.dr_ip_addr == 0xc0a80103);
    CHECK(neighbor.bdr_ip_addr == 0xc0a80102);
    CHECK(neighbor.options_counter == 2);
    CHECK(neighbor.options_list == "*||||||E|");
    CHECK(neighbor.transit_id.area == 0x0);
}

TEST_CASE("frr_ip_ospf_neighbor_status_parse_transit_id", "[frr]") {
    const std::string ip_ospf_neighbor = R"({
"192.168.1.3": [
    {
    "nbrPriority":1,
    "state":"Full\/DROther",
    "nbrState":"Full",
    "deadTimeMsecs":31273,
    "areaId":"0.0.0.0",
    "transitAreaId":"0.0.0.4",
    "ifaceName":"VLINK0",
    "ifaceAddress":"192.168.1.3",
    "routerDesignatedId":"0.0.0.0",
    "routerDesignatedBackupId":"0.0.0.0",
    "optionsList":"*|-|-|-|-|-|E|-",
    "optionsCounter":2,
    "retransmitCounter":0,
    "requestCounter":0,
    "dbSummaryCounter":0,
    "routerDeadIntervalTimerDueMsec":39768,
    "databaseSummaryListCounter":0,
    "linkStateRequestListCounter":0,
    "linkStateRetransmissionListCounter":0,
    "threadInactivityTimer":"on"
    }
    ]
})";

    auto result = vtss::frr_ip_ospf_neighbor_status_parse(ip_ospf_neighbor);
    vtss::FrrIpOspfNeighborStatus &neighbor = result[0xc0a80103][0];
    CHECK(neighbor.if_address == 0xc0a80103);
    CHECK(neighbor.transit_id.area == 0x04);
}

TEST_CASE("frr_ip_ospf_neighbor_status_parse_multiple", "[frr]") {
    const std::string ip_ospf_neighbor = R"({
"192.168.1.3": [
    {
    "ifaceAddress":"192.168.1.3",
    "areaId":"0.0.0.1 [Stub]",
    "ifaceName":"vtss.vlan.1",
    "nbrPriority":1,
    "transitAreaId":"0.0.0.3",
    "nbrState":"Full",
    "stateChangeCounter":5,
    "routerDesignatedId":"192.168.1.3",
    "routerDesignatedBackupId":"192.168.1.2",
    "optionsCounter":2,
    "optionsList":"*||||||E|",
    "routerDeadIntervalTimerDueMsec":37820,
    "databaseSummaryListCounter":0,
    "linkStateRequestListCounter":0,
    "linkStateRetransmissionListCounter":0,
    "threadInactivityTimer":"on"
  },
  {
    "ifaceAddress":"192.168.1.4",
    "areaId":"0.0.0.2",
    "ifaceName":"vtss.vlan.2",
    "transitAreaId":"0.0.0.4",
    "nbrPriority":2,
    "nbrState":"Full",
    "stateChangeCounter":5,
    "routerDesignatedId":"192.168.1.4",
    "routerDesignatedBackupId":"192.168.1.5",
    "optionsCounter":2,
    "optionsList":"*||||||E|",
    "routerDeadIntervalTimerDueMsec":38259,
    "databaseSummaryListCounter":0,
    "linkStateRequestListCounter":0,
    "linkStateRetransmissionListCounter":0,
    "threadInactivityTimer":"on"
  }
  ]
}
)";
    auto result = vtss::frr_ip_ospf_neighbor_status_parse(ip_ospf_neighbor);
    CHECK(result[0xc0a80103].size() == 2);
    vtss::FrrIpOspfNeighborStatus &neighbor = result[0xc0a80103][0];
    CHECK(neighbor.if_address == 0xc0a80103);
    CHECK(neighbor.area.area == 1);
    CHECK(neighbor.transit_id.area == 3);
    CHECK(neighbor.nbr_priority == 1);
    CHECK(neighbor.nbr_state == vtss::NSM_Full);
    CHECK(neighbor.dr_ip_addr == 0xc0a80103);
    CHECK(neighbor.bdr_ip_addr == 0xc0a80102);
    CHECK(neighbor.options_counter == 2);
    CHECK(neighbor.options_list == "*||||||E|");

    vtss::FrrIpOspfNeighborStatus &neighbor1 = result[0xc0a80103][1];
    CHECK(neighbor1.if_address == 0xc0a80104);
    CHECK(neighbor1.area.area == 2);
    CHECK(neighbor1.transit_id.area == 4);
    CHECK(neighbor1.nbr_priority == 2);
    CHECK(neighbor1.nbr_state == vtss::NSM_Full);
    CHECK(neighbor1.dr_ip_addr == 0xc0a80104);
    CHECK(neighbor1.bdr_ip_addr == 0xc0a80105);
    CHECK(neighbor1.options_counter == 2);
    CHECK(neighbor1.options_list == "*||||||E|");
}

TEST_CASE("to_vty_ospf_router_conf_del", "[frr]") {
    auto result = vtss::to_vty_ospf_router_conf_del();
    CHECK(result.size() == 2);
    CHECK(result[0] == "configure terminal");
    CHECK(result[1] == "no router ospf");
}

TEST_CASE("to_vty_ospf_router_conf_set", "[frr]") {
    SECTION("default FrrOspfRouterConf") {
        auto result = vtss::to_vty_ospf_router_conf_set();
        CHECK(result.size() == 3);
        CHECK(result[0] == "configure terminal");
        CHECK(result[1] == "no router ospf");
        CHECK(result[2] == "router ospf");
    }
    SECTION("using FrrOspfRouterConf") {
        vtss::FrrOspfRouterConf conf;
        conf.ospf_router_id = 3;
        conf.ospf_router_rfc1583 = true;
        auto result = vtss::to_vty_ospf_router_conf_set(conf);
        CHECK(result.size() == 4);
        CHECK(result[0] == "configure terminal");
        CHECK(result[1] == "router ospf");
        CHECK(result[2] == "ospf router-id 0.0.0.3");
        CHECK(result[3] == "ospf rfc1583compatibility");
    }
    SECTION("set all fields in FrrOspRouterConf") {
        vtss::FrrOspfRouterConf conf;
        conf.ospf_router_id = 4;
        conf.ospf_router_rfc1583 = false;
        conf.ospf_router_abr_type = vtss::AbrType_Standard;
        auto result = vtss::to_vty_ospf_router_conf_set(conf);
        CHECK(result.size() == 5);
        CHECK(result[0] == "configure terminal");
        CHECK(result[1] == "router ospf");
        CHECK(result[2] == "ospf router-id 0.0.0.4");
        CHECK(result[3] == "no ospf rfc1583compatibility");
        CHECK(result[4] == "ospf abr-type standard");
    }
}

TEST_CASE("frr_ospf_router_conf_get", "[frr]") {
    std::string config = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
interface vtss.ifh
!
router ospf
    ospf router-id 0.1.2.3
    ospf abr-type shortcut
    refresh timer 40
    compatible rfc1583
    network 192.168.1.0/24 area 0.0.0.0
!
ip route 0.0.0.1/8 0.0.0.6 9
!
line vty
!
)";

    SECTION("invalid instance") {
        CHECK(bool(frr_ospf_router_conf_get(config, 0)) == false);
    }

    SECTION("parse ospf router") {
        auto result = frr_ospf_router_conf_get(config, 1);
        CHECK(result->ospf_router_id.valid() == true);
        CHECK(result->ospf_router_id.get() == 0x10203);
        CHECK(result->ospf_router_rfc1583.valid() == true);
        CHECK(result->ospf_router_rfc1583.get() == true);
        CHECK(result->ospf_router_abr_type.valid() == true);
        CHECK(result->ospf_router_abr_type.get() == vtss::AbrType_Shortcut);
    }

    std::string empty_config = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
interface vtss.ifh
!
router ospf
!
ip route 0.0.0.1/8 0.0.0.6 9
!
line vty
!
)";

    SECTION("check if no values") {
        auto result = frr_ospf_router_conf_get(empty_config, 1);
        CHECK(result->ospf_router_id.valid() == false);
        CHECK(result->ospf_router_rfc1583.valid() == false);
        CHECK(result->ospf_router_abr_type.valid() == false);
    }

    std::string no_config = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
interface vtss.ifh
!
ip route 0.0.0.1/8 0.0.0.6 9
!
line vty
!
)";

    SECTION("check if no router ospf") {
        auto result = frr_ospf_router_conf_get(no_config, 0);
        CHECK(result->ospf_router_id.valid() == false);
        CHECK(result->ospf_router_rfc1583.valid() == false);
        CHECK(result->ospf_router_abr_type.valid() == false);
    }
}

// frr_ospf_router_default_metric
TEST_CASE("to_vty_ospf_router_default_metric_conf_set", "[frr]") {
    auto cmds = vtss::to_vty_ospf_router_default_metric_conf_set(42);
    CHECK(cmds.size() == 3);
    CHECK(cmds[0] == "configure terminal");
    CHECK(cmds[1] == "router ospf");
    CHECK(cmds[2] == "default-metric 42");
}

TEST_CASE("to_vty_ospf_router_default_metric_conf_del", "[frr]") {
    auto cmds = vtss::to_vty_ospf_router_default_metric_conf_del();
    CHECK(cmds.size() == 3);
    CHECK(cmds[0] == "configure terminal");
    CHECK(cmds[1] == "router ospf");
    CHECK(cmds[2] == "no default-metric");
}

TEST_CASE("frr_ospf_router_default_metric_conf_get", "[frr]") {
    SECTION("invalid instance") {
        std::string empty_config = "";
        auto res = vtss::frr_ospf_router_default_metric_conf_get(empty_config, {0});
        CHECK(res.rc == VTSS_RC_ERROR);
    }
    SECTION("parse default-metric") {
        std::string config = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
router ospf
    default-metric 42
!
)";

        auto res = vtss::frr_ospf_router_default_metric_conf_get(config, {1});
        CHECK(res.rc == VTSS_RC_OK);
        CHECK(res.val.valid() == true);
        CHECK(res.val.get() == 42);
    }

    SECTION("parse no default-metric") {
        std::string config = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
router ospf
!
)";

        auto res = vtss::frr_ospf_router_default_metric_conf_get(config, {1});
        CHECK(res.rc == VTSS_RC_OK);
        CHECK(res.val.valid() == false);
    }
}

// frr_ospf_router_stub_router
TEST_CASE("to_vty_ospf_router_stub_router_conf_set", "[frr]") {
    vtss::FrrOspfRouterStubRouter val;
    val.is_administrative = true;
    val.on_startup_interval = 50;

    auto cmds = vtss::to_vty_ospf_router_stub_router_conf_set(val);
    CHECK(cmds.size() == 4);
    CHECK(cmds[0] == "configure terminal");
    CHECK(cmds[1] == "router ospf");
    CHECK(cmds[2] == "max-metric router-lsa administrative");
    CHECK(cmds[3] == "max-metric router-lsa on-startup 50");
}

TEST_CASE("to_vty_ospf_router_stub_router_conf_del", "[frr]") {
    vtss::FrrOspfRouterStubRouter val;
    val.is_administrative = true;
    val.on_shutdown_interval = 5;

    auto cmds = vtss::to_vty_ospf_router_stub_router_conf_del(val);
    CHECK(cmds.size() == 4);
    CHECK(cmds[0] == "configure terminal");
    CHECK(cmds[1] == "router ospf");
    CHECK(cmds[2] == "no max-metric router-lsa administrative");
    CHECK(cmds[3] == "no max-metric router-lsa on-shutdown");
}
TEST_CASE("frr_ospf_router_stub_router_conf_get", "[frr]") {
    SECTION("invalid instance") {
        std::string empty_config = "";
        auto res = vtss::frr_ospf_router_stub_router_conf_get(empty_config, {0});
        CHECK(res.rc == VTSS_RC_ERROR);
    }
    SECTION("parse max-metric") {
        std::string config = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
router ospf
 max-metric router-lsa on-startup 44
 max-metric router-lsa on-shutdown 50
!
)";

        auto res = vtss::frr_ospf_router_stub_router_conf_get(config, {1});
        CHECK(res.rc == VTSS_RC_OK);
        CHECK(res.val.is_administrative == false);
        CHECK(res.val.on_startup_interval.valid() == true);
        CHECK(res.val.on_startup_interval.get() == 44);
        CHECK(res.val.on_shutdown_interval.valid() == true);
        CHECK(res.val.on_shutdown_interval.get() == 50);
    }

    SECTION("parse no max-metric") {
        std::string config = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
router ospf
!
)";

        auto res = vtss::frr_ospf_router_stub_router_conf_get(config, {1});
        CHECK(res.rc == VTSS_RC_OK);
        CHECK(res.val.is_administrative == false);
        CHECK(res.val.on_startup_interval.valid() == false);
        CHECK(res.val.on_shutdown_interval.valid() == false);
    }
}

// frr_ospf_router_spf_throttling
TEST_CASE("frr_ospf_router_spf_throttling_conf_set", "[frr]") {
    vtss::FrrOspfRouterSPFThrotlling val;
    val.delay = 200;
    val.init_holdtime = 400;
    val.max_holdtime = 10000;

    auto cmds = vtss::to_vty_ospf_router_spf_throttling_conf_set(val);
    CHECK(cmds.size() == 3);
    CHECK(cmds[0] == "configure terminal");
    CHECK(cmds[1] == "router ospf");
    CHECK(cmds[2] == "timers throttle spf 200 400 10000");
}

TEST_CASE("frr_ospf_router_spf_throttling_conf_del", "[frr]") {
    auto cmds = vtss::to_vty_ospf_router_spf_throttling_conf_del();
    CHECK(cmds.size() == 3);
    CHECK(cmds[0] == "configure terminal");
    CHECK(cmds[1] == "router ospf");
    CHECK(cmds[2] == "no timers throttle spf");
}
TEST_CASE("frr_ospf_router_spf_throttling_conf_get", "[frr]") {
    SECTION("invalid instance") {
        std::string empty_config = "";
        auto res = vtss::frr_ospf_router_spf_throttling_conf_get(empty_config, {0});
        CHECK(res.rc == VTSS_RC_ERROR);
    }

    SECTION("parse spf throttling values") {
        std::string config = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
router ospf
 timers throttle spf 500 10000 10000
!
)";

        auto res = vtss::frr_ospf_router_spf_throttling_conf_get(config, {1});
        CHECK(res.rc == VTSS_RC_OK);
        CHECK(res.val.delay == 500);
        CHECK(res.val.init_holdtime == 10000);
        CHECK(res.val.max_holdtime == 10000);
    }

    SECTION("parse default spf throttling values") {
        std::string config = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
router ospf
!
)";

        auto res = vtss::frr_ospf_router_spf_throttling_conf_get(config, {1});
        CHECK(res.rc == VTSS_RC_OK);
        CHECK(res.val.delay == 0);
        CHECK(res.val.init_holdtime == 50);
        CHECK(res.val.max_holdtime == 5000);
    }
}

// frr_ospf_router_distribute_conf
TEST_CASE("frr_ospf_router_redistribute_conf_set", "[frr]") {
    vtss::FrrOspfRouterRedistribute val;
    val.protocol = vtss::Protocol_Connected;
    val.metric_type = vtss::MetricType_One;
    val.metric = 42;
    val.route_map = "route_map";

    auto cmds = vtss::to_vty_ospf_router_redistribute_conf_set(val);
    CHECK(cmds.size() == 3);
    CHECK(cmds[0] == "configure terminal");
    CHECK(cmds[1] == "router ospf");
    CHECK(cmds[2] == "redistribute connected metric 42 metric-type 1");
}

TEST_CASE("frr_ospf_router_redistribute_conf_del", "[frr]") {
    vtss_appl_ip_route_protocol_t protocol = VTSS_APPL_IP_ROUTE_PROTOCOL_KERNEL;
    auto cmds = vtss::to_vty_ospf_router_redistribute_conf_del(protocol);
    CHECK(cmds.size() == 3);
    CHECK(cmds[0] == "configure terminal");
    CHECK(cmds[1] == "router ospf");
    CHECK(cmds[2] == "no redistribute kernel");
}

TEST_CASE("frr_ospf_router_redistribute_conf_get", "[frr]") {
    SECTION("invalid instance") {
        std::string empty_config = "";
        auto res = vtss::frr_ospf_router_redistribute_conf_get(empty_config, {0});
        CHECK(res.size() == 0);
    }

    SECTION("parse multiple protocols") {
        const std::string config = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
router ospf
    redistribute kernel metric 1000 metric-type 1 route-map ASD
    redistribute connected metric 1000 metric-type 1
    redistribute rip
    redistribute static route-map ADD
    redistribute bgp metric 999
!
)";

        auto res = vtss::frr_ospf_router_redistribute_conf_get(config, {1});
        CHECK(res.size() == 5);
        CHECK(res[0].protocol == vtss::Protocol_Kernel);
        CHECK(res[0].metric_type == vtss::MetricType_One);
        CHECK(res[0].metric.valid() == true);
        CHECK(res[0].metric.get() == 1000);
        CHECK(res[0].route_map.valid() == true);
        CHECK(res[0].route_map.get() == "ASD");
        CHECK(res[1].protocol == vtss::Protocol_Connected);
        CHECK(res[1].metric_type == vtss::MetricType_One);
        CHECK(res[1].metric.valid() == true);
        CHECK(res[1].metric.get() == 1000);
        CHECK(res[1].route_map.valid() == false);
        CHECK(res[2].protocol == vtss::Protocol_Rip);
        CHECK(res[2].metric_type == vtss::MetricType_Two);
        CHECK(res[2].metric.valid() == false);
        CHECK(res[2].route_map.valid() == false);
        CHECK(res[3].protocol == vtss::Protocol_Static);
        CHECK(res[3].metric_type == vtss::MetricType_Two);
        CHECK(res[3].metric.valid() == false);
        CHECK(res[3].route_map.valid() == true);
        CHECK(res[3].route_map.get() == "ADD");
        CHECK(res[4].protocol == vtss::Protocol_Bgp);
        CHECK(res[4].metric_type == vtss::MetricType_Two);
        CHECK(res[4].metric.valid() == true);
        CHECK(res[4].metric.get() == 999);
        CHECK(res[4].route_map.valid() == false);
    }
}

// frr_ospf_router_default_route
TEST_CASE("to_vty_ospf_router_default_route_conf_set", "[frr]") {
    vtss::FrrOspfRouterDefaultRoute val;
    val.always = true;
    val.metric_type = vtss::MetricType_One;
    val.metric = 1000;
    val.route_map = "test_route_map";

    auto cmds = vtss::to_vty_ospf_router_default_route_conf_set(val);
    CHECK(cmds.size() == 3);
    CHECK(cmds[0] == "configure terminal");
    CHECK(cmds[1] == "router ospf");
    CHECK(cmds[2] ==
          "default-information originate always metric 1000 metric-type 1 "
          "route-map test_route_map");
}

TEST_CASE("to_vty_ospf_router_default_route_conf_del", "[frr]") {
    auto cmds = vtss::to_vty_ospf_router_default_route_conf_del();
    CHECK(cmds.size() == 3);
    CHECK(cmds[0] == "configure terminal");
    CHECK(cmds[1] == "router ospf");
    CHECK(cmds[2] == "no default-information originate");
}

TEST_CASE("frr_ospf_router_default_route_conf_get", "[frr]") {
    SECTION("invalid instance") {
        std::string empty_config = "";
        auto res = frr_ospf_router_default_route_conf_get(empty_config, {0});
        CHECK(res.rc == VTSS_RC_ERROR);
    }

    SECTION("parse with always") {
        const std::string config = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
!
router ospf
    default-information originate always route-map NNNN
!
!
)";

        auto res = frr_ospf_router_default_route_conf_get(config, {1});
        CHECK(res.rc == VTSS_RC_OK);
        CHECK(res.val.always.valid() == true);
        CHECK(res.val.always.get() == true);
        CHECK(res.val.metric.valid() == false);
        CHECK(res.val.metric_type.valid() == true);
        CHECK(res.val.metric_type.get() == vtss::MetricType_Two);
        CHECK(res.val.route_map.valid() == true);
        CHECK(res.val.route_map.get() == "NNNN");
    }

    SECTION("parse without always") {
        const std::string config = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
!
router ospf
    default-information originate metric 1230 metric-type 1
!
!
)";

        auto res = frr_ospf_router_default_route_conf_get(config, {1});
        CHECK(res.rc == VTSS_RC_OK);
        CHECK(res.val.always.valid() == true);
        CHECK(res.val.always.get() == false);
        CHECK(res.val.metric.valid() == true);
        CHECK(res.val.metric.get() == 1230);
        CHECK(res.val.metric_type.valid() == true);
        CHECK(res.val.metric_type.get() == vtss::MetricType_One);
        CHECK(res.val.route_map.valid() == false);
    }

    SECTION("parse without anything") {
        const std::string config = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
!
router ospf
    default-information originate
!
!
)";

        auto res = frr_ospf_router_default_route_conf_get(config, {1});
        CHECK(res.rc == VTSS_RC_OK);
        CHECK(res.val.always.valid() == true);
        CHECK(res.val.always.get() == false);
        CHECK(res.val.metric.valid() == false);
        CHECK(res.val.metric_type.valid() == true);
        CHECK(res.val.metric_type.get() == vtss::MetricType_Two);
        CHECK(res.val.route_map.valid() == false);
    }

    SECTION("parse only route-map") {
        const std::string config = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
!
router ospf
    default-information originate route-map ASBCDREE21
!
!
)";

        auto res = frr_ospf_router_default_route_conf_get(config, {1});
        CHECK(res.rc == VTSS_RC_OK);
        CHECK(res.val.always.valid() == true);
        CHECK(res.val.always.get() == false);
        CHECK(res.val.metric.valid() == false);
        CHECK(res.val.metric_type.valid() == true);
        CHECK(res.val.metric_type.get() == vtss::MetricType_Two);
        CHECK(res.val.route_map.valid() == true);
        CHECK(res.val.route_map.get() == "ASBCDREE21");
    }

    SECTION("parse no default-information originate") {
        std::string config = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
router ospf
!
)";

        auto res = vtss::frr_ospf_router_default_route_conf_get(config, {1});
        CHECK(res.rc == VTSS_RC_OK);
        CHECK(res.val.always.valid() == false);
        CHECK(res.val.metric_type.valid() == false);
        CHECK(res.val.metric.valid() == false);
        CHECK(res.val.route_map.valid() == false);
    }
}

TEST_CASE("to_vty_ospf_area_network_conf_set", "[frr]") {
    vtss::FrrOspfAreaNetwork val;
    val.net.address = 0x01;
    val.net.prefix_size = 8;
    val.area = 0x02;

    auto result = to_vty_ospf_area_network_conf_set(val);
    CHECK(result.size() == 3);
    CHECK(result[0] == "configure terminal");
    CHECK(result[1] == "router ospf");
    CHECK(result[2] == "network 0.0.0.1/8 area 0.0.0.2");
}

TEST_CASE("to_vty_ospf_area_network_conf_del", "[frr]") {
    vtss::FrrOspfAreaNetwork val;
    val.net.address = 0x03;
    val.net.prefix_size = 16;
    val.area = 0x05;

    auto result = to_vty_ospf_area_network_conf_del(val);
    CHECK(result.size() == 3);
    CHECK(result[0] == "configure terminal");
    CHECK(result[1] == "router ospf");
    CHECK(result[2] == "no network 0.0.0.3/16 area 0.0.0.5");
}

TEST_CASE("frr_ospf_area_network_conf_get", "[frr]") {
    std::string config = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
interface vtss.ifh
!
router ospf
    ospf router-id 0.1.2.3
    ospf abr-type shortcut
    refresh timer 40
    compatible rfc1583
    network 192.168.1.0/24 area 0.0.0.0
!
ip route 0.0.0.1/8 0.0.0.6 9
!
line vty
!
)";

    SECTION("invalid instance") {
        CHECK(frr_ospf_area_network_conf_get(config, 0).size() == 0);
    }

    SECTION("parse network area") {
        auto result = frr_ospf_area_network_conf_get(config, 1);
        CHECK(result.size() == 1);
        CHECK(result[0].net.address == 0xc0a80100);
        CHECK(result[0].net.prefix_size == 24);
        CHECK(result[0].area == 0);
    }

    std::string no_network = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
interface vtss.ifh
!
router ospf
    ospf router-id 0.1.2.3
    ospf abr-type shortcut
    refresh timer 40
    compatible rfc1583
!
ip route 0.0.0.1/8 0.0.0.6 9
!
line vty
!
)";

    SECTION("parse no network area") {
        auto result = frr_ospf_area_network_conf_get(no_network, 1);
        CHECK(result.size() == 0);
    }
}

// frr_ospf_area_range
TEST_CASE("to_vty_ospf_area_range_conf_set", "[frr]") {
    vtss::FrrOspfAreaNetwork val;
    val.net.address = 10;
    val.net.prefix_size = 24;
    val.area = 2;

    auto res = vtss::to_vty_ospf_area_range_conf_set(val);
    CHECK(res.size() == 3);
    CHECK(res[0] == "configure terminal");
    CHECK(res[1] == "router ospf");
    CHECK(res[2] == "area 0.0.0.2 range 0.0.0.10/24");
}

TEST_CASE("to_vty_ospf_area_range_conf_del", "[frr]") {
    vtss::FrrOspfAreaNetwork val;
    val.net.address = 20;
    val.net.prefix_size = 16;
    val.area = 3;

    auto res = vtss::to_vty_ospf_area_range_conf_del(val);
    CHECK(res.size() == 3);
    CHECK(res[0] == "configure terminal");
    CHECK(res[1] == "router ospf");
    CHECK(res[2] == "no area 0.0.0.3 range 0.0.0.20/16");
}

TEST_CASE("frr_ospf_area_range_conf_get", "[frr]") {
    SECTION("parse invalid instance") {
        std::string empty_config = "";
        auto res = frr_ospf_area_range_conf_get(empty_config, {0});
        CHECK(res.size() == 0);
    }

    SECTION("parse no area") {
        const std::string config = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
router ospf
!
line vty
!
)";

        auto res = frr_ospf_area_range_conf_get(config, {1});
        CHECK(res.size() == 0);
    }

    SECTION("parse multiple areas") {
        const std::string config = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
router ospf
    area 0.0.0.0 range 10.0.0.0/8
    area 0.0.0.1 range 20.0.0.0/16
!
line vty
!
)";

        auto res = frr_ospf_area_range_conf_get(config, {1});
        CHECK(res.size() == 2);
        CHECK(res[0].area == 0);
        CHECK(res[0].net.address == 0xa000000);
        CHECK(res[0].net.prefix_size == 8);
        CHECK(res[1].area == 1);
        CHECK(res[1].net.address == 0x14000000);
        CHECK(res[1].net.prefix_size == 16);
    }

    SECTION("parse different areas") {
        const std::string config = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
router ospf
    area 0.0.0.0 range 10.0.0.0/8
    area 0.0.0.1 range 20.0.0.0/16 not-advertise
!
line vty
!
)";

        auto res = vtss::frr_ospf_area_range_conf_get(config, {1});
        CHECK(res.size() == 2);
        CHECK(res[0].area == 0);
        CHECK(res[0].net.address == 0xa000000);
        CHECK(res[0].net.prefix_size == 8);
        CHECK(res[1].area == 1);
        CHECK(res[1].net.address == 0x14000000);
        CHECK(res[1].net.prefix_size == 16);
    }
}

// frr_ospf_area_range_not_advertise
TEST_CASE("to_vty_ospf_area_range_not_advertise_set", "[frr]") {
    vtss::FrrOspfAreaNetwork net;
    net.area = 1;
    net.net.address = 2;
    net.net.prefix_size = 16;

    auto res = vtss::to_vty_ospf_area_range_not_advertise_conf_set(net);
    CHECK(res.size() == 3);
    CHECK(res[0] == "configure terminal");
    CHECK(res[1] == "router ospf");
    CHECK(res[2] == "area 0.0.0.1 range 0.0.0.2/16 not-advertise");
}

TEST_CASE("to_vty_ospf_area_range_not_advertise_del", "[frr]") {
    vtss::FrrOspfAreaNetwork net;
    net.area = 2;
    net.net.address = 3;
    net.net.prefix_size = 24;

    auto res = vtss::to_vty_ospf_area_range_not_advertise_conf_del(net);
    CHECK(res.size() == 3);
    CHECK(res[0] == "configure terminal");
    CHECK(res[1] == "router ospf");
    CHECK(res[2] == "no area 0.0.0.2 range 0.0.0.3/24 not-advertise");
}

TEST_CASE("frr_ospf_area_range_not_advertise_conf_get", "[frr]") {
    SECTION("invalid instance") {
        std::string empty_config = "";
        auto res = vtss::frr_ospf_area_range_not_advertise_conf_get(empty_config, {0});
        CHECK(res.size() == 0);
    }

    SECTION("parse no area") {
        const std::string config = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
router ospf
!
line vty
!
)";

        auto res = vtss::frr_ospf_area_range_not_advertise_conf_get(config, {1});
        CHECK(res.size() == 0);
    }

    SECTION("parse not advertise area") {
        const std::string config = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
router ospf
    area 0.0.0.6 range 0.0.0.7/24 not-advertise
!
line vty
!
)";
        auto res = vtss::frr_ospf_area_range_not_advertise_conf_get(config, {1});
        CHECK(res.size() == 1);
        CHECK(res[0].area == 6);
        CHECK(res[0].net.address == 0x07);
        CHECK(res[0].net.prefix_size == 24);
    }

    SECTION("parse different areas") {
        const std::string config = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
router ospf
    area 0.0.0.6 range 0.0.0.7/24 not-advertise
    area 0.0.0.8 range 0.0.0.9/24
!
line vty
!
)";

        auto res = vtss::frr_ospf_area_range_not_advertise_conf_get(config, {1});
        CHECK(res.size() == 1);
        CHECK(res[0].area == 6);
        CHECK(res[0].net.address == 0x07);
        CHECK(res[0].net.prefix_size == 24);
    }
}

// frr_ospf_area_range_cost
TEST_CASE("to_vty_ospf_area_range_cost_conf_set", "[frr]") {
    vtss::FrrOspfAreaNetworkCost val;
    val.net.address = 2;
    val.net.prefix_size = 16;
    val.area = 1;
    val.cost = 20;

    auto res = to_vty_ospf_area_range_cost_conf_set(val);
    CHECK(res.size() == 3);
    CHECK(res[0] == "configure terminal");
    CHECK(res[1] == "router ospf");
    CHECK(res[2] == "area 0.0.0.1 range 0.0.0.2/16 cost 20");
}

TEST_CASE("to_vty_ospf_area_range_cost_conf_del", "[frr]") {
    vtss::FrrOspfAreaNetworkCost val;
    val.net.address = 3;
    val.net.prefix_size = 8;
    val.area = 2;
    val.cost = 30;

    auto res = to_vty_ospf_area_range_cost_conf_del(val);
    CHECK(res.size() == 3);
    CHECK(res[0] == "configure terminal");
    CHECK(res[1] == "router ospf");
    CHECK(res[2] == "no area 0.0.0.2 range 0.0.0.3/8 cost 0");
}

TEST_CASE("frr_ospf_area_range_cost_conf_get", "[frr]") {
    SECTION("invalid instance") {
        std::string empty_config = "";
        auto res = vtss::frr_ospf_area_range_cost_conf_get(empty_config, {0});
        CHECK(res.size() == 0);
    }

    SECTION("parse no area") {
        const std::string config = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
router ospf
!
line vty
!
)";

        auto res = vtss::frr_ospf_area_range_cost_conf_get(config, {1});
        CHECK(res.size() == 0);
    }

    SECTION("parse cost area") {
        const std::string config = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
router ospf
    area 0.0.0.6 range 0.0.0.7/24 cost 80
!
line vty
!
)";

        auto res = vtss::frr_ospf_area_range_cost_conf_get(config, {1});
        CHECK(res.size() == 1);
        CHECK(res[0].area == 6);
        CHECK(res[0].net.address == 0x07);
        CHECK(res[0].net.prefix_size == 24);
        CHECK(res[0].cost == 80);
    }
}

// frr_ospf_virtual_link
TEST_CASE("to_vty_ospf_area_virtual_link_conf_set", "[frr]") {
    SECTION("all default") {
        vtss::FrrOspfAreaVirtualLink val;
        val.area = 1;
        val.dst = 2;

        auto res = vtss::to_vty_ospf_area_virtual_link_conf_set(val);
        CHECK(res.size() == 3);
        CHECK(res[0] == "configure terminal");
        CHECK(res[1] == "router ospf");
        CHECK(res[2] == "area 0.0.0.1 virtual-link 0.0.0.2");
    }

    SECTION("set interval") {
        vtss::FrrOspfAreaVirtualLink val;
        val.area = 1;
        val.dst = 2;
        val.hello_interval = 20;
        val.dead_interval = 30;

        auto res = vtss::to_vty_ospf_area_virtual_link_conf_set(val);
        CHECK(res.size() == 3);
        CHECK(res[0] == "configure terminal");
        CHECK(res[1] == "router ospf");
        CHECK(res[2] ==
              "area 0.0.0.1 virtual-link 0.0.0.2 hello-interval 20 "
              "dead-interval 30");
    }
}

TEST_CASE("to_vty_ospf_area_virtual_link_conf_del", "[frr]") {
    vtss::FrrOspfAreaVirtualLink val;
    val.area = 3;
    val.dst = 4;

    auto res = vtss::to_vty_ospf_area_virtual_link_conf_del(val);
    CHECK(res.size() == 3);
    CHECK(res[0] == "configure terminal");
    CHECK(res[1] == "router ospf");
    CHECK(res[2] == "no area 0.0.0.3 virtual-link 0.0.0.4");
}

TEST_CASE("frr_ospf_area_virtual_link_conf_get", "[frr]") {
    SECTION("parse invalid instace") {
        std::string empty_config = "";
        auto res = vtss::frr_ospf_area_virtual_link_conf_get(empty_config, {0});
        CHECK(res.size() == 0);
    }

    SECTION("parse no virtual link") {
        const std::string config = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
router ospf
!
line vty
!
)";

        auto res = vtss::frr_ospf_area_virtual_link_conf_get(config, {1});
        CHECK(res.size() == 0);
    }

    SECTION("parse multiple virtual links") {
        const std::string config = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
router ospf
    area 0.0.0.3 virtual-link 0.0.0.4
    area 0.0.0.2 virtual-link 0.0.0.5
!
line vty
!
)";

        auto res = vtss::frr_ospf_area_virtual_link_conf_get(config, {1});
        CHECK(res.size() == 2);
        CHECK(res[0].area == 3);
        CHECK(res[0].dst == 4);
        CHECK(res[1].area == 2);
        CHECK(res[1].dst == 5);
    }

    SECTION("parser virtual links with intervals") {
        const std::string config = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
router ospf
    area 0.0.0.3 virtual-link 0.0.0.4
    area 0.0.0.4 virtual-link 0.0.0.6 hello-interval 20 retransmit-interval 50 transmit-delay 10 dead-interval 100
!
line vty
!
)";

        auto res = vtss::frr_ospf_area_virtual_link_conf_get(config, {1});
        CHECK(res.size() == 2);
        CHECK(res[0].area == 3);
        CHECK(res[0].dst == 4);
        CHECK(res[1].area == 4);
        CHECK(res[1].dst == 6);
        CHECK(res[1].hello_interval.get() == 20);
        CHECK(res[1].retransmit_interval.get() == 50);
        CHECK(res[1].transmit_delay.get() == 10);
        CHECK(res[1].dead_interval.get() == 100);
    }
}

// frr_ospf_area_stub
TEST_CASE("to_vty_ospf_stub_area_conf_set", "[frr]") {
    SECTION("add stub area") {
        auto res = vtss::to_vty_ospf_stub_area_conf_set(
                {1, false, false, vtss::NssaTranslatorRoleCandidate});
        CHECK(res.size() == 4);
        CHECK(res[0] == "configure terminal");
        CHECK(res[1] == "router ospf");
        CHECK(res[2] == "area 0.0.0.1 stub");
        CHECK(res[3] == "no area 0.0.0.1 stub no-summary");
    }

    SECTION("add NSSA") {
        auto res = vtss::to_vty_ospf_stub_area_conf_set(
                {1, true, false, vtss::NssaTranslatorRoleCandidate});
        CHECK(res.size() == 4);
        CHECK(res[0] == "configure terminal");
        CHECK(res[1] == "router ospf");
        CHECK(res[2] == "area 0.0.0.1 nssa");
        CHECK(res[3] == "no area 0.0.0.1 nssa no-summary");
    }
}

TEST_CASE("to_vty_ospf_stub_area_conf_del", "[frr]") {
    auto res = vtss::to_vty_ospf_stub_area_conf_del(2);
    CHECK(res.size() == 4);
    CHECK(res[0] == "configure terminal");
    CHECK(res[1] == "router ospf");
    CHECK(res[2] == "no area 0.0.0.2 nssa");
    CHECK(res[3] == "no area 0.0.0.2 stub");
}

TEST_CASE("frr_ospf_stub_area_conf_get", "[frr]") {
    SECTION("parse invalid instance") {
        std::string empty_config = "";
        auto res = vtss::frr_ospf_stub_area_conf_get(empty_config, {0});
        CHECK(res.size() == 0);
    }

    SECTION("parse no stub area") {
        const std::string config = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
router ospf
!
line vty
!
)";

        auto res = vtss::frr_ospf_stub_area_conf_get(config, {1});
        CHECK(res.size() == 0);
    }

    SECTION("parse multiple stub areas") {
        const std::string config = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
router ospf
    area 0.0.0.4 stub
    area 0.0.0.2 stub
!
line vty
!
)";

        auto res = vtss::frr_ospf_stub_area_conf_get(config, {1});
        CHECK(res.size() == 2);
        CHECK(res[0].area == 4);
        CHECK(res[1].area == 2);
    }

    SECTION("parse stub and stub no-summary") {
        const std::string config = R"(frr version 3.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
router ospf
    area 0.0.0.4 stub
    area 0.0.0.3 stub no-summary
    area 0.0.0.2 stub
!
line vty
!
)";

        auto res = vtss::frr_ospf_stub_area_conf_get(config, 1);
        CHECK(res.size() == 3);
    }

    SECTION("parse no nssa area") {
        const std::string config = R"(frr version 4.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
router ospf
!
line vty
!
)";

        auto res = vtss::frr_ospf_stub_area_conf_get(config, {1});
        CHECK(res.size() == 0);
    }

    SECTION("parse multiple NSSAs") {
        const std::string config = R"(frr version 4.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
router ospf
    area 0.0.0.4 nssa
    area 0.0.0.2 nssa
!
line vty
!
)";

        auto res = vtss::frr_ospf_stub_area_conf_get(config, {1});
        CHECK(res.size() == 2);
        CHECK(res[0].area == 4);
        CHECK(res[1].area == 2);
    }

    SECTION("parse nssa and nssa no-summary") {
        const std::string config = R"(frr version 4.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
router ospf
    area 0.0.0.4 nssa
    area 0.0.0.3 nssa
    area 0.0.0.3 nssa no-summary
    area 0.0.0.2 nssa
!
line vty
!
)";

        auto res = vtss::frr_ospf_stub_area_conf_get(config, 1);
        CHECK(res.size() == 3);
    }
}

// frr_ospf_area_authentication
TEST_CASE("to_vty_ospf_area_authentication_set", "[frr]") {
    SECTION("set mode pwd") {
        const mesa_ipv4_t area = 3;
        vtss::FrrOspfAuthMode mode = vtss::FRR_OSPF_AUTH_MODE_PWD;

        auto res = to_vty_ospf_area_authentication_conf_set(area, mode);
        CHECK(res.size() == 3);
        CHECK(res[0] == "configure terminal");
        CHECK(res[1] == "router ospf");
        CHECK(res[2] == "area 0.0.0.3 authentication");
    }

    SECTION("set mod msg digest") {
        const mesa_ipv4_t area = 4;
        vtss::FrrOspfAuthMode mode = vtss::FRR_OSPF_AUTH_MODE_MSG_DIGEST;

        auto res = to_vty_ospf_area_authentication_conf_set(area, mode);
        CHECK(res.size() == 3);
        CHECK(res[0] == "configure terminal");
        CHECK(res[1] == "router ospf");
        CHECK(res[2] == "area 0.0.0.4 authentication message-digest");
    }

    SECTION("set mode null") {
        const mesa_ipv4_t area = 5;
        vtss::FrrOspfAuthMode mode = vtss::FRR_OSPF_AUTH_MODE_NULL;

        auto res = to_vty_ospf_area_authentication_conf_set(area, mode);
        CHECK(res.size() == 3);
        CHECK(res[0] == "configure terminal");
        CHECK(res[1] == "router ospf");
        CHECK(res[2] == "no area 0.0.0.5 authentication");
    }
}

TEST_CASE("frr_ospf_area_authentication_conf_get", "[frr]") {
    const std::string config = R"(frr version 3.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
interface vtss.ifh
!
router ospf
    area 0.0.0.0 authentication
!
line vty
!
)";

    SECTION("invalid instance") {
        auto res = vtss::frr_ospf_area_authentication_conf_get(config, 0);
        CHECK(res.size() == 0);
    }

    SECTION("parse required area") {
        auto res = vtss::frr_ospf_area_authentication_conf_get(config, 1);
        CHECK(res.size() == 1);
        CHECK(res[0].area == 0);
        CHECK(res[0].auth_mode == vtss::FRR_OSPF_AUTH_MODE_PWD);
    }

    const std::string config_digest = R"(frr version 3.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
interface vtss.ifh
!
router ospf
    area 0.0.0.0 authentication message-digest
!
line vty
!
)";

    SECTION("parse message-digest") {
        auto res = vtss::frr_ospf_area_authentication_conf_get(config_digest, 1);
        CHECK(res.size() == 1);
        CHECK(res[0].area == 0);
        CHECK(res[0].auth_mode == vtss::FRR_OSPF_AUTH_MODE_MSG_DIGEST);
    }
}

// frr_ospf_stub_area_stub_conf(stub no_summary)
TEST_CASE("to_vty_ospf_stub_area_conf_set(no_summary)", "[frr]") {
    auto res = vtss::to_vty_ospf_stub_area_conf_set({1, false, true, vtss::NssaTranslatorRoleCandidate});
    CHECK(res.size() == 4);
    CHECK(res[0] == "configure terminal");
    CHECK(res[1] == "router ospf");
    CHECK(res[2] == "area 0.0.0.1 stub");
    CHECK(res[3] == "area 0.0.0.1 stub no-summary");
}

TEST_CASE("to_vty_ospf_stub_area_conf_set(no stub no_summary)", "[frr]") {
    auto res = vtss::to_vty_ospf_stub_area_conf_set(
            {2, false, false, vtss::NssaTranslatorRoleCandidate});
    CHECK(res.size() == 4);
    CHECK(res[0] == "configure terminal");
    CHECK(res[1] == "router ospf");
    CHECK(res[2] == "area 0.0.0.2 stub");
    CHECK(res[3] == "no area 0.0.0.2 stub no-summary");
}

TEST_CASE("frr_ospf_stub_area_conf_get(stub no-summary)", "[frr]") {
    SECTION("invalid instance") {
        std::string empty_config = "";
        auto res = vtss::frr_ospf_stub_area_conf_get(empty_config, 0);
        CHECK(res.size() == 0);
    }

    SECTION("nothing to parse") {
        const std::string config = R"(frr version 3.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
!
line vty
!
)";

        auto res = vtss::frr_ospf_stub_area_conf_get(config, 1);
        CHECK(res.size() == 0);
    }

    SECTION("parse multiple stub areas") {
        const std::string config = R"(frr version 3.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
router ospf
    area 0.0.0.2 stub no-summary
    area 0.0.0.4 stub no-summary
    area 0.0.0.5 stub
    area 0.0.0.6 nssa
    area 0.0.0.6 nssa no-summary
    area 0.0.0.7 nssa
    area 0.0.0.7 nssa no-summary
    area 0.0.0.8 nssa
!
line vty
!
)";

        auto res = vtss::frr_ospf_stub_area_conf_get(config, 1);
        CHECK(res.size() == 6);
        CHECK(res[0].area == 2);
        CHECK(res[1].area == 4);
        CHECK(res[2].area == 5);
        CHECK(res[3].area == 6);
        CHECK(res[4].area == 7);
        CHECK(res[5].area == 8);
    }
}

// frr_ospf_stub_area_stub_conf(nssa no_summary)
TEST_CASE("to_vty_ospf_stub_area_conf_set(nssa no_summary)", "[frr]") {
    auto res = vtss::to_vty_ospf_stub_area_conf_set(
            {1, true, true, vtss::NssaTranslatorRoleCandidate});
    CHECK(res.size() == 4);
    CHECK(res[0] == "configure terminal");
    CHECK(res[1] == "router ospf");
    CHECK(res[2] == "area 0.0.0.1 nssa");
    CHECK(res[3] == "area 0.0.0.1 nssa no-summary");
}

TEST_CASE("to_vty_ospf_stub_area_conf_set(no nssa no_summary)", "[frr]") {
    auto res = vtss::to_vty_ospf_stub_area_conf_set(
            {2, true, false, vtss::NssaTranslatorRoleCandidate});
    CHECK(res.size() == 4);
    CHECK(res[0] == "configure terminal");
    CHECK(res[1] == "router ospf");
    CHECK(res[2] == "area 0.0.0.2 nssa");
    CHECK(res[3] == "no area 0.0.0.2 nssa no-summary");
}

TEST_CASE("to_vty_ospf_stub_area_conf_set(nssa candidate)", "[frr]") {
    auto res = vtss::to_vty_ospf_stub_area_conf_set(
            {2, true, false, vtss::NssaTranslatorRoleCandidate});
    CHECK(res.size() == 4);
    CHECK(res[0] == "configure terminal");
    CHECK(res[1] == "router ospf");
    CHECK(res[2] == "area 0.0.0.2 nssa");
    CHECK(res[3] == "no area 0.0.0.2 nssa no-summary");
}

TEST_CASE("to_vty_ospf_stub_area_conf_set(nssa always)", "[frr]") {
    auto res = vtss::to_vty_ospf_stub_area_conf_set(
            {2, true, false, vtss::NssaTranslatorRoleAlways});
    CHECK(res.size() == 4);
    CHECK(res[0] == "configure terminal");
    CHECK(res[1] == "router ospf");
    CHECK(res[2] == "area 0.0.0.2 nssa translate-always");
    CHECK(res[3] == "no area 0.0.0.2 nssa no-summary");
}

TEST_CASE("to_vty_ospf_stub_area_conf_set(nssa never)", "[frr]") {
    auto res = vtss::to_vty_ospf_stub_area_conf_set(
            {2, true, false, vtss::NssaTranslatorRoleNever});
    CHECK(res.size() == 4);
    CHECK(res[0] == "configure terminal");
    CHECK(res[1] == "router ospf");
    CHECK(res[2] == "area 0.0.0.2 nssa translate-never");
    CHECK(res[3] == "no area 0.0.0.2 nssa no-summary");
}

TEST_CASE("frr_ospf_area_virtual_link_authentication_conf_set", "[frr]") {
    vtss::FrrOspfAreaVirtualLink link;
    link.area = 1;
    link.dst = 2;
    SECTION("no authentication") {
        auto res = vtss::to_vty_ospf_area_virtual_link_authentication_conf_set(
                link, vtss::FRR_OSPF_AUTH_MODE_NULL);
        CHECK(res.size() == 3);
        CHECK(res[0] == "configure terminal");
        CHECK(res[1] == "router ospf");
        CHECK(res[2] ==
              "area 0.0.0.1 virtual-link 0.0.0.2 authentication null");
    }

    SECTION("pass authentication") {
        auto res = vtss::to_vty_ospf_area_virtual_link_authentication_conf_set(
                link, vtss::FRR_OSPF_AUTH_MODE_PWD);
        CHECK(res.size() == 3);
        CHECK(res[0] == "configure terminal");
        CHECK(res[1] == "router ospf");
        CHECK(res[2] == "area 0.0.0.1 virtual-link 0.0.0.2 authentication");
    }

    SECTION("message-digest authentication") {
        auto res = vtss::to_vty_ospf_area_virtual_link_authentication_conf_set(
                link, vtss::FRR_OSPF_AUTH_MODE_MSG_DIGEST);
        CHECK(res.size() == 3);
        CHECK(res[0] == "configure terminal");
        CHECK(res[1] == "router ospf");
        CHECK(res[2] ==
              "area 0.0.0.1 virtual-link 0.0.0.2 authentication "
              "message-digest");
    }
}

TEST_CASE("frr_ospf_area_virtual_link_authentication_conf_get", "[frr]") {
    SECTION("invalid instance") {
        std::string empty_config = "";
        auto res = vtss::frr_ospf_area_virtual_link_authentication_conf_get(empty_config, {0});
        CHECK(res.size() == 0);
    }

    SECTION("no authentication") {
        const std::string config = R"(frr version 3.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
router ospf
    area 0.0.0.1 virtual-link 0.0.0.2 authentication null
!
line vty
!
)";

        auto res = vtss::frr_ospf_area_virtual_link_authentication_conf_get(config, {1});
        CHECK(res.size() == 1);
        CHECK(res[0].virtual_link.area == 1);
        CHECK(res[0].virtual_link.dst == 2);
        CHECK(res[0].auth_mode == vtss::FRR_OSPF_AUTH_MODE_NULL);
    }

    SECTION("password authentication") {
        const std::string config = R"(frr version 3.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
router ospf
    area 0.0.0.1 virtual-link 0.0.0.2 authentication
!
line vty
!
)";

        auto res = vtss::frr_ospf_area_virtual_link_authentication_conf_get(config, {1});
        CHECK(res.size() == 1);
        CHECK(res[0].virtual_link.area == 1);
        CHECK(res[0].virtual_link.dst == 2);
        CHECK(res[0].auth_mode == vtss::FRR_OSPF_AUTH_MODE_PWD);
    }

    SECTION("password authentication-key (authentication type should not be "
            "changed)") {
        const std::string config = R"(frr version 3.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
router ospf
    area 0.0.0.1 virtual-link 0.0.0.2
    area 0.0.0.1 virtual-link 0.0.0.2 authentication-key 123
!
line vty
!
)";

        auto res = vtss::frr_ospf_area_virtual_link_authentication_conf_get(config, {1});
        CHECK(res.size() == 1);
        CHECK(res[0].virtual_link.area == 1);
        CHECK(res[0].virtual_link.dst == 2);
        CHECK(res[0].auth_mode == vtss::FRR_OSPF_AUTH_MODE_AREA_CFG);
    }

    SECTION("message-digest authentication") {
        const std::string config = R"(frr version 3.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
router ospf
    area 0.0.0.1 virtual-link 0.0.0.2 authentication message-digest
!
line vty
!
)";

        auto res = vtss::frr_ospf_area_virtual_link_authentication_conf_get(config, {1});
        CHECK(res.size() == 1);
        CHECK(res[0].virtual_link.area == 1);
        CHECK(res[0].virtual_link.dst == 2);
        CHECK(res[0].auth_mode == vtss::FRR_OSPF_AUTH_MODE_MSG_DIGEST);
    }
}

TEST_CASE("to_vty_ospf_area_virtual_link_message_digest_conf_set", "[frr]") {
    vtss::FrrOspfAreaVirtualLink val;
    val.area = 1;
    val.dst = 2;
    vtss::FrrOspfDigestData data(3, "vtss");

    auto res = vtss::to_vty_ospf_area_virtual_link_message_digest_conf_set(
            val, data);
    CHECK(res.size() == 3);
    CHECK(res[0] == "configure terminal");
    CHECK(res[1] == "router ospf");
    CHECK(res[2] ==
          "area 0.0.0.1 virtual-link 0.0.0.2 message-digest-key 3 md5 vtss");
}

TEST_CASE("frr_ospf_area_virtual_link_authentication_key_conf_del", "[frr]") {
    vtss::FrrOspfAreaVirtualLink val;
    val.area = 1;
    val.dst = 2;
    vtss::FrrOspfDigestData data(3, "test123");

    auto res = vtss::to_vty_ospf_area_virtual_link_message_digest_conf_del(
            val, data);
    CHECK(res.size() == 3);
    CHECK(res[0] == "configure terminal");
    CHECK(res[1] == "router ospf");
    CHECK(res[2] ==
          "no area 0.0.0.1 virtual-link 0.0.0.2 message-digest-key 3 md5 "
          "test123");
}

TEST_CASE("frr_ospf_area_virtual_link_message_digest_conf_get", "[frr]") {
    SECTION("invalid instance") {
        std::string empty_config = "";
        auto res = vtss::frr_ospf_area_virtual_link_message_digest_conf_get(empty_config, {0});
        CHECK(res.size() == 0);
    }

    SECTION("parse one") {
        const std::string config = R"(frr version 3.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
router ospf
    area 0.0.0.1 virtual-link 0.0.0.2 message-digest-key 1 md5 vtss
!
line vty
!
)";

        auto res = vtss::frr_ospf_area_virtual_link_message_digest_conf_get(config, {1});
        CHECK(res.size() == 1);
        CHECK(res[0].virtual_link.area == 1);
        CHECK(res[0].virtual_link.dst == 2);
        CHECK(res[0].digest_data.keyid == 1);
        CHECK(res[0].digest_data.key == "vtss");
    }
}

TEST_CASE("to_vty_ospf_area_virtual_link_authentication_key_conf_set",
          "[frr]") {
    vtss::FrrOspfAreaVirtualLink val;
    val.area = 1;
    val.dst = 2;
    std::string key = "vtss";

    auto res = vtss::to_vty_ospf_area_virtual_link_authentication_key_conf_set(
            val, key);
    CHECK(res.size() == 3);
    CHECK(res[0] == "configure terminal");
    CHECK(res[1] == "router ospf");
    CHECK(res[2] ==
          "area 0.0.0.1 virtual-link 0.0.0.2 authentication-key vtss");
}

TEST_CASE("to_vty_ospf_area_virtual_link_authentication_key_conf_del",
          "[frr]") {
    vtss::FrrOspfAreaVirtualLinkKey val;
    val.virtual_link.area = 1;
    val.virtual_link.dst = 2;
    val.key_data = "test123";

    auto res =
            vtss::to_vty_ospf_area_virtual_link_authentication_key_conf_del(val);
    CHECK(res.size() == 3);
    CHECK(res[0] == "configure terminal");
    CHECK(res[1] == "router ospf");
    CHECK(res[2] ==
          "no area 0.0.0.1 virtual-link 0.0.0.2 authentication-key test123");
}

TEST_CASE("frr_ospf_area_virtual_link_authentication_key_conf_get", "[frr]") {
    SECTION("invalid instance") {
        std::string empty_config = "";
        auto res = vtss::frr_ospf_area_virtual_link_authentication_key_conf_get(empty_config, {0});
        CHECK(res.size() == 0);
    }

    SECTION("parse one") {
        const std::string config = R"(frr version 3.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
router ospf
    area 0.0.0.1 virtual-link 0.0.0.2 authentication-key ASD
!
line vty
!
)";

        auto res = vtss::frr_ospf_area_virtual_link_authentication_key_conf_get(config, {1});
        CHECK(res.size() == 1);
        CHECK(res[0].virtual_link.area == 1);
        CHECK(res[0].virtual_link.dst == 2);
        CHECK(res[0].key_data == "ASD");
    }

    SECTION("parse none") {
        const std::string config = R"(frr version 3.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
router ospf
!
line vty
!
)";

        auto res = vtss::frr_ospf_area_virtual_link_authentication_key_conf_get(config, {1});
        CHECK(res.size() == 0);
    }
}

TEST_CASE("to_vty_ospf_if_mtu_ignore_conf_set", "[frr]") {
    SECTION("ignore") {
        auto result = vtss::to_vty_ospf_if_mtu_ignore_conf_set("test1", true);
        CHECK(result.size() == 3);
        CHECK(result[0] == "configure terminal");
        CHECK(result[1] == "interface test1");
        CHECK(result[2] == "ip ospf mtu-ignore");
    }

    SECTION("enforce") {
        auto result = vtss::to_vty_ospf_if_mtu_ignore_conf_set("test1", false);
        CHECK(result.size() == 3);
        CHECK(result[0] == "configure terminal");
        CHECK(result[1] == "interface test1");
        CHECK(result[2] == "no ip ospf mtu-ignore");
    }
}

TEST_CASE("to_vty_ospf_router_passive_if_default", "[frr]") {
    SECTION("enable") {
        auto result = vtss::to_vty_ospf_router_passive_if_default(true);
        CHECK(result.size() == 3);
        CHECK(result[0] == "configure terminal");
        CHECK(result[1] == "router ospf");
        CHECK(result[2] == "passive-interface default");
    }

    SECTION("disable") {
        auto result = vtss::to_vty_ospf_router_passive_if_default(false);
        CHECK(result.size() == 3);
        CHECK(result[0] == "configure terminal");
        CHECK(result[1] == "router ospf");
        CHECK(result[2] == "no passive-interface default");
    }
}

TEST_CASE("to_vty_ospf_router_passive_if_conf_set", "[frr]") {
    SECTION("not passive") {
        auto result =
                vtss::to_vty_ospf_router_passive_if_conf_set("test1", false);
        CHECK(result.size() == 3);
        CHECK(result[0] == "configure terminal");
        CHECK(result[1] == "router ospf");
        CHECK(result[2] == "no passive-interface test1");
    }
    SECTION("passive") {
        auto result = vtss::to_vty_ospf_router_passive_if_conf_set("test1", true);
        CHECK(result.size() == 3);
        CHECK(result[0] == "configure terminal");
        CHECK(result[1] == "router ospf");
        CHECK(result[2] == "passive-interface test1");
    }
}

TEST_CASE("frr_ospf_router_passive_if_default_get", "[frr]") {
    SECTION("invalid istance") {
        std::string empty_config = "";
        auto result = vtss::frr_ospf_router_passive_if_default_get(empty_config, 0);
        CHECK(result.rc == VTSS_RC_ERROR);
    }

    std::string config_default_if = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
interface vtss.vlan.1234
!
router ospf
    passive-interface default
    ospf router-id 0.1.2.3
    passive-interface vtss.vlan.1234
!
ip route 0.0.0.1/8 0.0.0.6 9
!
line vty
!
)";
    SECTION("parse passive-interface default") {
        auto result = vtss::frr_ospf_router_passive_if_default_get(config_default_if, 1);
        CHECK(result.val == true);
    }
}

TEST_CASE("frr_ospf_router_passive_if_conf_get", "[frr]") {
    vtss_ifindex_t index{1234};
    SECTION("invalid instance") {
        std::string empty_config = "";
        auto result = vtss::frr_ospf_router_passive_if_conf_get(empty_config, 0, index);
        CHECK(result.rc == VTSS_RC_ERROR);
    }

    std::string config_valid_if = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
interface vtss.vlan.1234
!
router ospf
    ospf router-id 0.1.2.3
    passive-interface vtss.vlan.1234
!
ip route 0.0.0.1/8 0.0.0.6 9
!
line vty
!
)";

    SECTION("parse passive-interface") {
        auto result = vtss::frr_ospf_router_passive_if_conf_get(config_valid_if, 1, index);
        CHECK(result.val == true);
    }

    std::string config_no_valid_if = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
interface vtss.vlan.1234
!
router ospf
    ospf router-id 0.1.2.3
    passive-interface wlan1
!
ip route 0.0.0.1/8 0.0.0.6 9
!
line vty
!
)";

    SECTION("parse no valid passive-interface") {
        auto result = vtss::frr_ospf_router_passive_if_conf_get(config_no_valid_if, 1, index);
        CHECK(result.val == false);
    }

    std::string config_passive_default = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
interface vtss.vlan.1234
!
router ospf
    passive-interface default
!
ip route 0.0.0.1/8 0.0.0.6 9
!
line vty
!
)";

    SECTION("parse default passive-interface") {
        auto result = vtss::frr_ospf_router_passive_if_conf_get(config_passive_default, 1, index);
        CHECK(result.val == true);
    }

    std::string config_passive_default_no_passive = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
interface vtss.vlan.1234
!
router ospf
    passive-interface default
    no passive-interface vtss.vlan.1234
!
ip route 0.0.0.1/8 0.0.0.6 9
!
line vty
!
)";

    SECTION("parse default no passive-interface") {
        auto result = vtss::frr_ospf_router_passive_if_conf_get(config_passive_default_no_passive, 1, index);
        CHECK(result.val == false);
    }
}

TEST_CASE("frr_ospf_router_passive_if_disable_all", "[frr]") {
    std::string running_conf = R"(
frr version 4.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
!
!
!
router ospf
 timers throttle spf 200 400 10000
 passive-interface vtss.vlan.21
!
line vty
!
)";

    SECTION("disable all passive interfaces") {
        auto res = vtss::to_vty_ospf_router_passive_if_disable_all(running_conf);
        CHECK(res.size() == 3);
        CHECK(res[0] == "configure terminal");
        CHECK(res[1] == "router ospf");
        CHECK(res[2] == "no passive-interface vtss.vlan.21");
    }
}

//----------------------------------------------------------------------------
//** OSPF administrative distance
//----------------------------------------------------------------------------
TEST_CASE("to_vty_ospf_router_admin_distance_set", "[frr]") {
    auto res = vtss::to_vty_ospf_router_admin_distance_set(110);
    CHECK(res.size() == 3);
    CHECK(res[0] == "configure terminal");
    CHECK(res[1] == "router ospf");
    CHECK(res[2] == "distance 110");
}

TEST_CASE("frr_ospf_router_admin_distance_get", "[frr]") {
    SECTION("invalid istance") {
        std::string empty_config = "";
        auto result = vtss::frr_ospf_router_admin_distance_get(empty_config, 0);
        CHECK(result.rc == VTSS_RC_ERROR);
    }

    std::string running_conf_str = R"(frr version 4.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
interface vtss.ifh
!
router ospf
    distance 123
!
line vty
!
)";

    SECTION("parse administrative distance") {
        auto result = vtss::frr_ospf_router_admin_distance_get(running_conf_str, 1);
        CHECK(result.val == 123);
    }
}

//----------------------------------------------------------------------------
//** OSPF authentication key
//----------------------------------------------------------------------------
TEST_CASE("to_vty_ospf_if_authentication_key_del", "[frr]") {
    auto res = vtss::to_vty_ospf_if_authentication_key_del("test1");

    CHECK(res.size() == 3);
    CHECK(res[0] == "configure terminal");
    CHECK(res[1] == "interface test1");
    CHECK(res[2] == "no ip ospf authentication-key");
}

TEST_CASE("to_vty_ospf_if_authentication_key_set", "[frr]") {
    auto res = vtss::to_vty_ospf_if_authentication_key_set("test1", "key123");

    CHECK(res.size() == 3);
    CHECK(res[0] == "configure terminal");
    CHECK(res[1] == "interface test1");
    CHECK(res[2] == "ip ospf authentication-key key123");
}

TEST_CASE("frr_ospf_if_authentication_key_conf_get", "[frr]") {
    SECTION("expected interface") {
        const std::string config = R"(frr version 3.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
interface vtss.vlan.1234
    ip ospf authentication-key 1234
!
line vty
!
)";

        auto res = vtss::frr_ospf_if_authentication_key_conf_get(config, {1234});
        CHECK(res.val == std::string("1234"));
    }

    SECTION("different interface") {
        const std::string config_wrong_if = R"(frr version 3.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
interface test_intf0_wrong
    ip ospf authentication-key 1234
!
line vty
!
)";

        auto res = vtss::frr_ospf_if_authentication_key_conf_get(config_wrong_if, {0});
        CHECK(res.rc == VTSS_RC_OK);
        CHECK(res.val == "");
    }

    SECTION("no key authentication") {
        const std::string config_no_key = R"(frr version 3.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
interface vtss.vlan.1234
!
line vty
!
)";

        auto res = vtss::frr_ospf_if_authentication_key_conf_get(config_no_key, {1234});
        CHECK(res.rc == VTSS_RC_OK);
        CHECK(res.val == "");
    }
}

TEST_CASE("to_vty_ospf_if_authentication_set", "[frr]") {
    SECTION("set mode null") {
        auto res = vtss::to_vty_ospf_if_authentication_set("test1", vtss::FRR_OSPF_AUTH_MODE_NULL);
        CHECK(res.size() == 3);
        CHECK(res[0] == "configure terminal");
        CHECK(res[1] == "interface test1");
        CHECK(res[2] == "ip ospf authentication null");
    }
    SECTION("set mode pwd") {
        auto res = vtss::to_vty_ospf_if_authentication_set("test2", vtss::FRR_OSPF_AUTH_MODE_PWD);
        CHECK(res.size() == 3);
        CHECK(res[0] == "configure terminal");
        CHECK(res[1] == "interface test2");
        CHECK(res[2] == "ip ospf authentication");
    }
    SECTION("set mode msg digest") {
        auto res = vtss::to_vty_ospf_if_authentication_set("test3", vtss::FRR_OSPF_AUTH_MODE_MSG_DIGEST);
        CHECK(res.size() == 3);
        CHECK(res[0] == "configure terminal");
        CHECK(res[1] == "interface test3");
        CHECK(res[2] == "ip ospf authentication message-digest");
    }
    SECTION("set mode area cfg") {
        auto res = vtss::to_vty_ospf_if_authentication_set("test4", vtss::FRR_OSPF_AUTH_MODE_AREA_CFG);
        CHECK(res.size() == 3);
        CHECK(res[0] == "configure terminal");
        CHECK(res[1] == "interface test4");
        CHECK(res[2] == "no ip ospf authentication");
    }
}

TEST_CASE("frr_ospf_if_authentication_conf_get", "[frr]") {
    SECTION("parse no authentication") {
        const std::string config = R"(frr version 3.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
interface vtss.vlan.1234
!
line vty
!
)";

        auto res = frr_ospf_if_authentication_conf_get(config, {1234});
        CHECK(res.rc == VTSS_RC_OK);
        CHECK(res.val == vtss::FRR_OSPF_AUTH_MODE_AREA_CFG);
    }

    SECTION("parse no interface") {
        const std::string config = R"(frr version 3.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
interface test_intf0_wrong
!
line vty
!
)";

        auto res = frr_ospf_if_authentication_conf_get(config, {0});
        CHECK(res.rc == VTSS_RC_OK);
        CHECK(res.val == vtss::FRR_OSPF_AUTH_MODE_AREA_CFG);
    }

    SECTION("parse multiple authentications") {
        const std::string config = R"(frr version 3.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
interface vtss.vlan.1234
   ip ospf authentication message-digest
   ip ospf authentication-key 123mara1
!
line vty
!
)";

        auto res = frr_ospf_if_authentication_conf_get(config, {1234});
        CHECK(res.rc == VTSS_RC_OK);
        CHECK(res.val == vtss::FRR_OSPF_AUTH_MODE_MSG_DIGEST);
    }
}

TEST_CASE("to_vty_ospf_if_field_conf_set", "[frr]") {
    auto res = vtss::to_vty_ospf_if_field_conf_set("test1", "field2", 2);
    CHECK(res.size() == 3);
    CHECK(res[0] == "configure terminal");
    CHECK(res[1] == "interface test1");
    CHECK(res[2] == "ip ospf field2 2");
}

TEST_CASE("to_vty_ospf_if_field_conf_del", "[frr]") {
    auto res = vtss::to_vty_ospf_if_field_conf_del("test2", "field3");
    CHECK(res.size() == 3);
    CHECK(res[0] == "configure terminal");
    CHECK(res[1] == "interface test2");
    CHECK(res[2] == "no ip ospf field3");
}

TEST_CASE("frr_ospf_if_XXX_conf_get", "[frr]") {
    SECTION("frr_ospf_if_XXX_conf_get") {
        const std::string config = R"(frr version 3.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
interface vtss.vlan.1234
    ip ospf hello-interval 2
    ip ospf dead-interval 3
    ip ospf retransmit-interval 4000
!
line vty
!
)";

        auto hello_res = vtss::frr_ospf_if_hello_interval_conf_get(config, {1234});
        CHECK(hello_res.rc == VTSS_RC_OK);
        CHECK(hello_res.val == 2);
        auto dead_res = vtss::frr_ospf_if_dead_interval_conf_get(config, {1234});
        CHECK(dead_res.rc == VTSS_RC_OK);
        CHECK(dead_res.val.val == 3);
        auto retransmit_res = vtss::frr_ospf_if_retransmit_interval_conf_get(config, {1234});
        CHECK(retransmit_res.rc == VTSS_RC_OK);
        CHECK(retransmit_res.val == 4000);
    }

    SECTION("dead-interval minimal set") {
        const std::string config = R"(frr version 3.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
interface vtss.vlan.1234
    ip ospf dead-interval minimal hello-multiplier 10
!
line vty
!
)";
        auto dead_res = vtss::frr_ospf_if_dead_interval_conf_get(config, {1234});
        CHECK(dead_res.val.val == 10);
        CHECK(dead_res.val.multiplier == true);
        auto hello_res = vtss::frr_ospf_if_hello_interval_conf_get(config, {1234});
        CHECK(hello_res.val == 10);
    }

    SECTION("dead-interval minimal and hello-interval") {
        const std::string config = R"(frr version 3.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
interface vtss.vlan.1234
    ip ospf hello-interval 30
    ip ospf dead-interval minimal hello-multiplier 20
!
line vty
!
)";

        auto hello_res = vtss::frr_ospf_if_hello_interval_conf_get(config, {1234});
        CHECK(hello_res.val == 30);
    }
}

TEST_CASE("to_vty_ospf_if_message_digest_set", "[frr]") {
    vtss::FrrOspfDigestData data(1, "asd");
    auto res = vtss::to_vty_ospf_if_message_digest_set("test1", data);
    CHECK(res.size() == 3);
    CHECK(res[0] == "configure terminal");
    CHECK(res[1] == "interface test1");
    CHECK(res[2] == "ip ospf message-digest-key 1 md5 asd");
}

TEST_CASE("to_vty_ospf_if_message_digest_del", "[frr]") {
    auto res = vtss::to_vty_ospf_if_message_digest_del("test2", 2);
    CHECK(res.size() == 3);
    CHECK(res[0] == "configure terminal");
    CHECK(res[1] == "interface test2");
    CHECK(res[2] == "no ip ospf message-digest-key 2");
}

TEST_CASE("frr_ospf_if_message_digest_conf_set", "[frr]") {
    SECTION("wrong interface") {
        const std::string config = R"(frr version 3.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
interface test_intf0_wrong
    ip ospf authentication message-digest
    ip ospf message-digest-key 100 md5 123141
    ip ospf message-digest-key 255 md5 123141
!
line vty
!
)";

        auto res = vtss::frr_ospf_if_message_digest_conf_get(config, {0});
        CHECK(res.size() == 0);
    }

    SECTION("interface") {
        const std::string config = R"(frr version 3.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
interface vtss.vlan.1234
    ip ospf authentication message-digest
    ip ospf message-digest-key 100 md5 13
    ip ospf message-digest-key 255 md5 12
!
line vty
!
)";

        auto res = vtss::frr_ospf_if_message_digest_conf_get(config, {1234});
        CHECK(res.size() == 2);
        CHECK(res[0].keyid == 100);
        CHECK(res[0].key == "13");
        CHECK(res[1].keyid == 255);
        CHECK(res[1].key == "12");
    }
}

TEST_CASE("frr_ip_ospf_route_status", "[frr]") {
    std::string ip_ospf_route = R"({
    "1.0.1.0\/24":
    [{ "routeType": "N", "cost": 10, "area": "0.0.0.0", "nexthops": [ { "ip": " ", "directly attached to": "vtss.vlan.100" } ]
    }],
    "1.0.2.0\/24":
    [{ "routeType": "N", "cost": 20, "area": "0.0.0.0", "nexthops": [ { "ip": "1.0.1.2", "via": "vtss.vlan.100" } ]
    }],
    "1.0.3.0\/24":
    [{ "routeType": "N", "cost": 30, "area": "0.0.0.0", "nexthops": [ { "ip": " ", "directly attached to": "vtss.vlan.200" } ]
    }],
    "1.0.4.0\/24":
    [{ "routeType": "N", "cost": 40, "area": "0.0.0.0", "nexthops": [ { "ip": "1.0.3.4", "via": "vtss.vlan.200" } ]
    }],
    "1.0.17.0\/24":
    [{ "routeType": "N", "cost": 50, "area": "0.0.0.0", "nexthops": [ { "ip": "1.0.3.4", "via": "vtss.vlan.200" } ]
    }],
    "1.1.7.0\/24":
    [{
        "routeType": "N IA",
        "cost": 60,
        "area": "0.0.0.0",
        "nexthops": [
        { "ip": "1.0.1.2", "via": "vtss.vlan.100" },
        { "ip": "1.0.3.4", "via": "vtss.vlan.200" }
        ]
    }],
    "1.1.8.0\/24":
    [{ "routeType": "N IA", "cost": 70, "area": "0.0.0.0",
        "nexthops": [
        { "ip": "1.0.1.2", "via": "vtss.vlan.100" },
        { "ip": "1.0.3.4", "via": "vtss.vlan.200" }
        ]
    }],
    "1.1.9.0\/24":
    [{ "routeType": "N IA", "cost": 80, "area": "0.0.0.0",
        "nexthops": [
        { "ip": "1.0.1.2", "via": "vtss.vlan.100" },
        { "ip": "1.0.3.4", "via": "vtss.vlan.200" }
        ]
    }],
    "0.0.0.42\/32": [{ "routeType": "R ", "cost": 90, "area": "0.0.0.0", "routerType": "abr", "nexthops": [ { "ip": "1.0.3.4", "via": "vtss.vlan.200" } ]
    }],

)";

    auto result = vtss::frr_ip_ospf_route_status_parse(ip_ospf_route);
    vtss::APPL_FrrOspfRouteKey k;
    vtss::APPL_FrrOspfRouteStatus v;
    k.inst_id = 1;
    k.network.address = 0x01000100;
    k.network.prefix_size = 24;
    k.route_type = vtss::RT_Network;
    k.area = 0;
    k.nexthop_ip = 0;
    auto itr = result.find(k);
    CHECK(itr != result.end());
    CHECK(result.size() == 12);
    CHECK(itr->second.cost == 10);
    CHECK(itr->second.router_type == vtss::RouterType_None);
    CHECK(itr->second.is_connected == true);
    CHECK(itr->second.is_ia == false);
    CHECK(itr->second.ifname == "vtss.vlan.100");
    k.network.address = 0x01010800;
    k.route_type = vtss::RT_NetworkIA;
    k.area = 0;
    k.nexthop_ip = 0x01000304;
    itr = result.find(k);
    CHECK(itr != result.end());
    CHECK(itr->second.cost == 70);
    CHECK(itr->second.router_type == vtss::RouterType_None);
    CHECK(itr->second.is_connected == false);
    CHECK(itr->second.is_ia == false);
    CHECK(itr->second.ifname == "vtss.vlan.200");
    itr = result.greater_than(k);
    CHECK(itr->second.cost == 80);
    CHECK(itr->second.ifname == "vtss.vlan.100");
    k.network.address = 0x0000002a;
    k.network.prefix_size = 32;
    k.route_type = vtss::RT_Router;
    k.area = 0;
    k.nexthop_ip = 0x01000304;
    itr = result.find(k);
    CHECK(itr->second.cost == 90);
    CHECK(itr->second.router_type == vtss::RouterType_ABR);
    k.network.address = 0x01000200;
    itr = result.find(k);
    CHECK(itr == result.end());
}

TEST_CASE("frr_show_ip_ospf_db_parse_1", "[frr]") {
    std::string ip_ospf_db = R"({
  "routerId":"0.0.0.3"
}
)";

    auto result = vtss::frr_ip_ospf_db_parse_raw(ip_ospf_db);
    vtss::FrrOspfDbStatus db = result;

    CHECK(db.router_id == 0x3);
}


TEST_CASE("frr_show_ip_ospf_db_parse_2", "[frr]") {
    std::string ip_ospf_db = R"({
  "routerId":"0.0.0.4",
  "areas":[
    {
      "type":1,
      "desc":"Router Link States",
      "area":"0.0.0.5"
    }
  ],
  "AS External Link States":{
    "type":5,
    "desc":"AS External Link States"
  },
  "NSSA-external Link States":{
    "type":7,
    "desc":"NSSA-external Link States"
  }
}
)";

    auto result = vtss::frr_ip_ospf_db_parse_raw(ip_ospf_db);
    vtss::FrrOspfDbStatus db = result;

    CHECK(db.router_id == 0x4);

    CHECK(db.type5.type == 0x5);
    CHECK(db.type5.desc == "AS External Link States");

    CHECK(db.type7.type == 0x7);
    CHECK(db.type7.desc == "NSSA-external Link States");
}

TEST_CASE("frr_show_ip_ospf_db_parse_3", "[frr]") {
    std::string ip_ospf_db = R"({
  "routerId":"0.0.0.3",
  "areas":[
    {
      "type":1,
      "desc":"Router Link States",
      "area":"0.0.0.5"
    }
  ],
  "AS External Link States":{
    "type":5,
    "desc":"AS External Link States"
  },
  "NSSA-external Link States":{
    "type":7,
    "desc":"NSSA-external Link States",
    "links":[
      {
        "id":"1.99.1.0",
        "router":"0.0.0.15",
        "age":1139,
        "seq":2147483697,
        "checksum":12521,
        "external":"E2",
        "prefix":"1.99.1.0",
        "prefix_len":24,
        "tag":2
      }
    ]
  }
}
)";

    auto result = vtss::frr_ip_ospf_db_parse_raw(ip_ospf_db);
    vtss::FrrOspfDbStatus db = result;

    CHECK(db.type7.links[0].link_id == 0x1630100);
    CHECK(db.type7.links[0].adv_router == 15);

    CHECK(db.type7.links[0].age == 1139);
    CHECK(db.type7.links[0].sequence == 2147483697);
    CHECK(db.type7.links[0].checksum == 12521);

    CHECK(db.type7.links[0].external == 2);

    CHECK(db.type7.links[0].summary_route.address == 0x1630100);
    CHECK(db.type7.links[0].summary_route.prefix_size == 24);

    CHECK(db.type7.links[0].tag == 2);
}

TEST_CASE("frr_show_ip_ospf_db_parse_4", "[frr]") {
    std::string ip_ospf_db = R"({
  "routerId":"0.0.0.3",
  "areas":[
    {
      "type":1,
      "desc":"Router Link States",
      "area":"0.0.0.5",
      "links":[
        {
          "id":"0.0.0.1",
          "router":"0.0.0.1",
          "age":1229,
          "seq":2147483701,
          "checksum":12975,
          "link":2
        },
        {
          "id":"0.0.0.2",
          "router":"0.0.0.2",
          "age":65,
          "seq":2147483731,
          "checksum":22384,
          "link":2
        }
      ]
    }
  ],
  "AS External Link States":{
    "type":5,
    "desc":"AS External Link States",
    "links":[
      {
        "id":"1.88.1.0",
        "router":"0.0.0.14",
        "age":1139,
        "seq":2147483697,
        "checksum":12521,
        "external":"E2",
        "prefix":"1.99.1.0",
        "prefix_len":24,
        "tag":2
      }
    ]
  },
  "NSSA-external Link States":{
    "type":7,
    "desc":"NSSA-external Link States",
    "links":[
      {
        "id":"1.99.1.0",
        "router":"0.0.0.15",
        "age":1139,
        "seq":2147483697,
        "checksum":12521,
        "external":"E2",
        "prefix":"1.99.1.0",
        "prefix_len":24,
        "tag":2
      }
    ]
  }
}
)";

    auto result = vtss::frr_ip_ospf_db_parse_raw(ip_ospf_db);
    vtss::FrrOspfDbStatus db = result;

    CHECK(db.area[0].type == 1);
    CHECK(db.area[0].desc == "Router Link States");
    CHECK(db.area[0].area_id.area == 5);

    CHECK(db.area[0].links[0].link_id == 1);
    CHECK(db.area[0].links[0].adv_router == 1);
    CHECK(db.area[0].links[0].age == 1229);
    CHECK(db.area[0].links[0].sequence == 2147483701);
    CHECK(db.area[0].links[0].checksum == 12975);
    CHECK(db.area[0].links[0].router_link_count == 2);


    CHECK(db.area[0].links[1].link_id == 2);
    CHECK(db.area[0].links[1].adv_router == 2);
    CHECK(db.area[0].links[1].age == 65);
    CHECK(db.area[0].links[1].sequence == 2147483731);
    CHECK(db.area[0].links[1].checksum == 22384);
    CHECK(db.area[0].links[1].router_link_count == 2);
}

TEST_CASE("frr_show_ip_ospf_db_parse_5", "[frr]") {
    std::string ip_ospf_db = R"({
  "routerId":"0.0.0.125",
  "areas":[
    {
      "type":1,
      "desc":"Router Link States",
      "area":"0.0.0.5",
      "links":[
        {
          "id":"0.0.0.1",
          "router":"0.0.0.1",
          "age":1229,
          "seq":2147483701,
          "checksum":12975,
          "link":2
        },
        {
          "id":"0.0.0.2",
          "router":"0.0.0.2",
          "age":65,
          "seq":2147483731,
          "checksum":22384,
          "link":2
        }
      ]
    },
    {
      "type":1,
      "desc":"Router Link States",
      "area":"0.0.0.7",
      "links":[
        {
          "id":"0.0.0.3",
          "router":"0.0.0.3",
          "age":1229,
          "seq":2147483701,
          "checksum":12975,
          "link":2
        },
        {
          "id":"0.0.0.4",
          "router":"0.0.0.4",
          "age":65,
          "seq":2147483731,
          "checksum":22384,
          "link":2
        }
      ]
    }
  ],
  "AS External Link States":{
    "type":5,
    "desc":"AS External Link States",
    "links":[
      {
        "id":"1.88.1.0",
        "router":"0.0.0.14",
        "age":1139,
        "seq":2147483697,
        "checksum":12521,
        "external":"E2",
        "prefix":"1.99.1.0",
        "prefix_len":24,
        "tag":2
      }
    ]
  },
  "NSSA-external Link States":{
    "type":7,
    "desc":"NSSA-external Link States",
    "links":[
      {
        "id":"1.99.1.0",
        "router":"0.0.0.15",
        "age":1139,
        "seq":2147483697,
        "checksum":12521,
        "external":"E2",
        "prefix":"1.99.1.0",
        "prefix_len":24,
        "tag":2
      }
    ]
  }
})";

    auto result = vtss::frr_ip_ospf_db_parse(ip_ospf_db);
    vtss::APPL_FrrOspfDbKey k;
    vtss::APPL_FrrOspfDbLinkStateVal v;

    k.area_id = 0x00000005;
    k.type = 1;
    k.link_id = 0x00000002;
    k.adv_router = 0x00000002;

    auto itr = result.find(k);
    CHECK(itr != result.end());
    CHECK(result.size() == 6);
    CHECK(itr->second.age == 65);
    CHECK(itr->second.checksum == 22384);
    CHECK(itr->second.age == 65);
    CHECK(itr->second.sequence == 0x80000053);
}

TEST_CASE("frr_show_ip_ospf_db_parse_6", "[frr]") {
    std::string ip_ospf_db = R"({
  "routerId":"0.0.0.125",
  "areas":[
    {
      "type":1,
      "desc":"Router Link States",
      "area":"0.0.0.5",
      "links":[
        {
          "id":"0.0.0.1",
          "router":"0.0.0.1",
          "age":1229,
          "seq":2147483701,
          "checksum":12975,
          "link":2
        },
        {
          "id":"0.0.0.2",
          "router":"0.0.0.2",
          "age":65,
          "seq":2147483731,
          "checksum":22384,
          "link":2
        }
      ]
    },
    {
      "type":1,
      "desc":"Router Link States",
      "area":"0.0.0.7",
      "links":[
        {
          "id":"0.0.0.3",
          "router":"0.0.0.3",
          "age":1229,
          "seq":2147483701,
          "checksum":12975,
          "link":2
        },
        {
          "id":"0.0.0.4",
          "router":"0.0.0.4",
          "age":60,
          "seq":2147483731,
          "checksum":22184,
          "link":2
        }
      ]
    }
  ],
  "AS External Link States":{
    "type":5,
    "desc":"AS External Link States",
    "links":[
      {
        "id":"1.88.1.0",
        "router":"0.0.0.14",
        "age":1139,
        "seq":2147483697,
        "checksum":12521,
        "external":"E2",
        "prefix":"1.99.1.0",
        "prefix_len":24,
        "tag":2
      }
    ]
  },
  "NSSA-external Link States":{
    "type":7,
    "desc":"NSSA-external Link States",
    "links":[
      {
        "id":"1.99.1.0",
        "router":"0.0.0.15",
        "age":1139,
        "seq":2147483697,
        "checksum":12521,
        "external":"E2",
        "prefix":"1.99.1.0",
        "prefix_len":24,
        "tag":2
      }
    ]
  }
}
)";

    auto result = vtss::frr_ip_ospf_db_parse(ip_ospf_db);
    vtss::APPL_FrrOspfDbKey k;
    vtss::APPL_FrrOspfDbLinkStateVal v;

    k.area_id = 0x00000007;
    k.type = 1;
    k.link_id = 0x00000004;
    k.adv_router = 0x00000004;

    auto itr = result.find(k);
    CHECK(itr != result.end());
    CHECK(result.size() == 6);
    CHECK(itr->second.age == 60);
    CHECK(itr->second.checksum == 22184);
    CHECK(itr->second.sequence == 0x80000053);
}

TEST_CASE("frr_show_ip_ospf_db_parse_7", "[frr]") {
    std::string ip_ospf_db = R"({
    "routerId":"0.0.0.3",
    "areas":[
      {
        "type":1,
        "desc":"Router Link States",
        "area":"0.0.0.0",
        "links":[
          {
            "id":"0.0.0.3",
            "router":"0.0.0.3",
            "age":171,
            "seq":2147483659,
            "checksum":15834,
            "link":2
          }
        ]
      },
      {
        "type":3,
        "desc":"Summary Link States",
        "area":"0.0.0.0",
        "links":[
          {
            "id":"1.3.12.0",
            "router":"0.0.0.3",
            "age":211,
            "seq":2147483649,
            "checksum":2104,
            "prefix":"1.3.12.0",
            "prefix_len":24
          }
        ]
      },
      {
        "type":1,
        "desc":"Router Link States",
        "area":"0.0.0.3 [Stub]",
        "links":[
          {
            "id":"0.0.0.3",
            "router":"0.0.0.3",
            "age":171,
            "seq":2147483654,
            "checksum":61757,
            "link":1
          }
        ]
      },
      {
        "type":3,
        "desc":"Summary Link States",
        "area":"0.0.0.3 [Stub]",
        "links":[
          {
            "id":"0.0.0.0",
            "router":"0.0.0.3",
            "age":211,
            "seq":2147483650,
            "checksum":32730,
            "prefix":"0.0.0.0",
            "prefix_len":0
          },
          {
            "id":"1.0.1.0",
            "router":"0.0.0.3",
            "age":211,
            "seq":2147483650,
            "checksum":49549,
            "prefix":"1.0.1.0",
            "prefix_len":24
          },
          {
            "id":"1.0.3.0",
            "router":"0.0.0.3",
            "age":211,
            "seq":2147483650,
            "checksum":43937,
            "prefix":"1.0.3.0",
            "prefix_len":24
          }
        ]
      }
    ],
    "AS External Link States":{
      "type":5,
      "desc":"AS External Link States",
      "links":[
        {
          "id":"1.88.1.0",
          "router":"0.0.0.14",
          "age":1139,
          "seq":2147483697,
          "checksum":12521,
          "external":"E2",
          "prefix":"1.99.1.0",
          "prefix_len":24,
          "tag":2
        }
      ]
    },
    "NSSA-external Link States":{
      "type":7,
      "desc":"NSSA-external Link States",
      "links":[
        {
          "id":"1.99.1.0",
          "router":"0.0.0.15",
          "age":1139,
          "seq":2147483697,
          "checksum":12521,
          "external":"E2",
          "prefix":"1.99.1.0",
          "prefix_len":24,
          "tag":2
        }
      ]
    }
  }
)";

    auto result = vtss::frr_ip_ospf_db_parse(ip_ospf_db);
    vtss::APPL_FrrOspfDbKey k;
    vtss::APPL_FrrOspfDbLinkStateVal v;

    k.area_id = 0x00000003;
    k.type = 1;
    k.link_id = 0x00000003;
    k.adv_router = 0x00000003;

    auto itr = result.find(k);
    CHECK(itr != result.end());
    CHECK(result.size() == 8);
    CHECK(itr->second.age == 171);
    CHECK(itr->second.checksum == 61757);
    CHECK(itr->second.sequence == 0x80000006);

#if 0
    std::cout << result.size() << std::endl;
#endif
}

TEST_CASE("frr_show_ip_ospf_db_router_1", "[frr]") {
    std::string ip_ospf_db = R"({
  "routerId":"0.0.0.3",
  "areas":[
    {
      "area":"0.0.0.0",
      "routes":[
        {
          "lsAge":155,
          "optionsList":"*|-|-|-|-|-|E|-",
          "lsType":"router-LSA",
          "linkStateId":"0.0.0.3",
          "advRouter":"0.0.0.3",
          "sequence":2147483707,
          "checksum":56331,
          "length":48,
          "links":[
            {
              "linkConnectedTo":"Stub Network",
              "linkID":"1.0.1.0",
              "linkData":"255.255.255.0",
              "metric":10
            },
            {
              "linkConnectedTo":"a Virtual Link",
              "linkID":"1.0.3.0",
              "linkData":"255.255.255.0",
              "metric":20
            }
          ]
        }
      ]
    }
  ]
}
)";

    auto result = vtss::frr_ip_ospf_db_router_parse_raw(ip_ospf_db);
    vtss::FrrOspfDbRouterStatus db = result;

    CHECK(db.router_id == 0x3);
    CHECK(db.areas[0].area_id.area == 0);

    CHECK(db.areas[0].routes[0].age == 155);
    CHECK(db.areas[0].routes[0].options == "*|-|-|-|-|-|E|-");
    CHECK(db.areas[0].routes[0].type == 1);
    CHECK(db.areas[0].routes[0].link_state_id == 3);
    CHECK(db.areas[0].routes[0].adv_router == 3);
    CHECK(db.areas[0].routes[0].sequence == 2147483707);
    CHECK(db.areas[0].routes[0].checksum == 56331);
    CHECK(db.areas[0].routes[0].length == 48);

    CHECK(db.areas[0].routes[0].links[0].link_connected_to == 3);
    CHECK(db.areas[0].routes[0].links[0].link_id == 0x1000100);
    CHECK(db.areas[0].routes[0].links[0].link_data == 0xffffff00);
    CHECK(db.areas[0].routes[0].links[0].metric == 10);

    CHECK(db.areas[0].routes[0].links[1].link_connected_to == 4);
    CHECK(db.areas[0].routes[0].links[1].link_id == 0x1000300);
    CHECK(db.areas[0].routes[0].links[1].link_data == 0xffffff00);
    CHECK(db.areas[0].routes[0].links[1].metric == 20);
}

TEST_CASE("frr_show_ip_ospf_db_router_2", "[frr]") {
    std::string ip_ospf_db = R"({
  "routerId":"0.0.0.3",
  "areas":[
    {
      "area":"0.0.0.0",
      "routes":[
        {
          "lsAge":660,
          "optionsList":"*|-|-|-|-|-|E|-",
          "lsType":"router-LSA",
          "linkStateId":"0.0.0.3",
          "advRouter":"0.0.0.3",
          "sequence":2147483653,
          "checksum":12780,
          "length":48,
          "links":[
            {
              "linkConnectedTo":"Stub Network",
              "linkID":"1.0.3.0",
              "linkData":"255.255.255.0",
              "metric":10
            },
            {
              "linkConnectedTo":"Stub Network",
              "linkID":"1.0.1.0",
              "linkData":"255.255.255.0",
              "metric":10
            }
          ]
        }
      ]
    },
    {
      "area":"0.0.0.3 [Stub]",
      "routes":[
        {
          "lsAge":700,
          "optionsList":"*|-|-|-|-|-|-|-",
          "lsType":"router-LSA",
          "linkStateId":"0.0.0.3",
          "advRouter":"0.0.0.3",
          "sequence":2147483650,
          "checksum":28398,
          "length":24,
          "links":[
            {
              "linkConnectedTo":"Stub Network",
              "linkID":"1.0.3.0",
              "linkData":"255.255.255.0",
              "metric":10
            }
          ]
        }
      ]
    },
    {
      "area":"0.0.0.5 [Stub]",
      "routes":[
        {
          "lsAge":700,
          "optionsList":"*|-|-|-|-|-|-|-",
          "lsType":"router-LSA",
          "linkStateId":"0.0.0.5",
          "advRouter":"0.0.0.5",
          "sequence":2147483650,
          "checksum":28398,
          "length":24
        }
      ]
    }
  ]
}
)";

    auto result = vtss::frr_ip_ospf_db_router_parse(ip_ospf_db);
    vtss::APPL_FrrOspfDbCommonKey k;
    vtss::APPL_FrrOspfDbRouterStateVal v;

    k.area_id = 0x00000003;
    k.type = vtss::FrrOspfLsdbType_Router;
    k.link_state_id = 0x00000003;
    k.adv_router = 0x00000003;

    auto itr = result.find(k);
    CHECK(itr != result.end());
    CHECK(result.size() == 3);
    CHECK(itr->second.age == 700);
    CHECK(itr->second.checksum == 28398);
    CHECK(itr->second.sequence == 0x80000002);
}

TEST_CASE("frr_show_ip_ospf_db_net_1", "[frr]") {
    std::string ip_ospf_db = R"({
  "routerId":"0.0.0.3",
  "areas":[
    {
      "area":"0.0.0.0",
      "routes":[
        {
          "lsAge":155,
          "optionsList":"*|-|-|-|-|-|E|-",
          "lsType":"network-LSA",
          "linkStateId":"0.0.0.3",
          "advRouter":"0.0.0.3",
          "sequence":2147483707,
          "checksum":56331,
          "length":48,
          "networkMask":24,
          "attachedRouter1":"1.0.3.1",
          "attachedRouter2":"1.0.3.2",
          "attachedRouter3":"1.0.3.3",
          "attachedRouter4":"1.0.3.4",
          "attachedRouter5":"1.0.3.5"
        }
      ]
    }
  ]
}
)";

    auto result = vtss::frr_ip_ospf_db_net_parse_raw(ip_ospf_db);
    vtss::FrrOspfDbNetStatus db = result;

    CHECK(db.router_id == 0x3);
    CHECK(db.areas[0].area_id.area == 0);

    CHECK(db.areas[0].routes[0].age == 155);
    CHECK(db.areas[0].routes[0].options == "*|-|-|-|-|-|E|-");
    CHECK(db.areas[0].routes[0].type == 2);
    CHECK(db.areas[0].routes[0].link_state_id == 3);
    CHECK(db.areas[0].routes[0].adv_router == 3);
    CHECK(db.areas[0].routes[0].sequence == 2147483707);
    CHECK(db.areas[0].routes[0].checksum == 56331);
    CHECK(db.areas[0].routes[0].length == 48);

    CHECK(db.areas[0].routes[0].network_mask == 24);
    CHECK(db.areas[0].routes[0].attached_router[0] == 0x1000301);
    CHECK(db.areas[0].routes[0].attached_router[1] == 0x1000302);
    CHECK(db.areas[0].routes[0].attached_router[2] == 0x1000303);
}

TEST_CASE("frr_show_ip_ospf_db_net_2", "[frr]") {
    std::string ip_ospf_db = R"({
  "routerId":"192.168.7.2",
  "areas":[
    {
      "area":"0.0.0.0",
      "routes":[
        {
          "lsAge":1592,
          "optionsList":"*|-|-|-|-|-|E|-",
          "lsType":"network-LSA",
          "linkStateId":"192.168.3.1",
          "advRouter":"0.0.0.2",
          "sequence":2147484833,
          "checksum":28117,
          "length":32,
          "networkMask":24,
          "attachedRouter":"0.0.0.1"
        },
        {
          "lsAge":663,
          "optionsList":"*|-|-|-|-|-|E|-",
          "lsType":"network-LSA",
          "linkStateId":"192.168.5.1",
          "advRouter":"0.0.0.1",
          "sequence":2147484008,
          "checksum":26789,
          "length":32,
          "networkMask":24,
          "attachedRouter":"192.168.7.2"
        },
        {
          "lsAge":893,
          "optionsList":"*|-|-|-|-|-|E|-",
          "lsType":"network-LSA",
          "linkStateId":"192.168.7.2",
          "advRouter":"192.168.7.2",
          "sequence":2147483756,
          "checksum":2699,
          "length":32,
          "networkMask":24,
          "attachedRouter":"192.168.7.2"
        }
      ]
    },
    {
      "area":"0.0.0.2 [NSSA]",
      "routes":[
        {
          "lsAge":723,
          "optionsList":"*|-|-|-|N\/P|-|-|-",
          "lsType":"network-LSA",
          "linkStateId":"192.168.6.1",
          "advRouter":"192.168.7.2",
          "sequence":2147483756,
          "checksum":36753,
          "length":32,
          "networkMask":24,
          "attachedRouter":"192.168.6.3"
        }
      ]
    }
  ]
}
)";

    auto result = vtss::frr_ip_ospf_db_net_parse(ip_ospf_db);
    vtss::APPL_FrrOspfDbCommonKey k;
    vtss::APPL_FrrOspfDbNetStateVal v;

    k.area_id = 0x00000000;
    k.type = vtss::FrrOspfLsdbType_Network;
    k.link_state_id = 0xC0A80501;
    k.adv_router = 0x00000001;

    auto itr = result.find(k);
    CHECK(itr != result.end());
    CHECK(result.size() == 4);
    CHECK(itr->second.age == 663);
    CHECK(itr->second.checksum == 26789);
    CHECK(itr->second.sequence == 0x80000168);
}

TEST_CASE("frr_show_ip_ospf_db_summary_1", "[frr]") {
    std::string ip_ospf_db = R"({
  "routerId":"0.0.0.3",
  "areas":[
    {
      "area":"0.0.0.0",
      "routes":[
        {
          "lsAge":155,
          "optionsList":"*|-|-|-|-|-|E|-",
          "lsType":"summary-LSA",
          "linkStateId":"0.0.0.3",
          "advRouter":"0.0.0.3",
          "sequence":2147483707,
          "checksum":56331,
          "length":48,
          "networkMask":24,
          "metric":20
        }
      ]
    }
  ]
}
)";

    auto result = vtss::frr_ip_ospf_db_summary_parse_raw(ip_ospf_db);
    vtss::FrrOspfDbSummaryStatus db = result;

    CHECK(db.router_id == 0x3);
    CHECK(db.areas[0].area_id.area == 0);

    CHECK(db.areas[0].routes[0].age == 155);
    CHECK(db.areas[0].routes[0].options == "*|-|-|-|-|-|E|-");
    CHECK(db.areas[0].routes[0].type == 3);
    CHECK(db.areas[0].routes[0].link_state_id == 3);
    CHECK(db.areas[0].routes[0].adv_router == 3);
    CHECK(db.areas[0].routes[0].sequence == 2147483707);
    CHECK(db.areas[0].routes[0].checksum == 56331);
    CHECK(db.areas[0].routes[0].length == 48);

    CHECK(db.areas[0].routes[0].network_mask == 24);
    CHECK(db.areas[0].routes[0].metric == 20);
}

TEST_CASE("frr_show_ip_ospf_db_summary_2", "[frr]") {
    std::string ip_ospf_db = R"({
  "routerId":"192.168.7.2",
  "areas":[
    {
      "area":"0.0.0.0",
      "routes":[
        {
          "lsAge":715,
          "optionsList":"*|-|-|-|-|-|E|-",
          "lsType":"summary-LSA",
          "linkStateId":"10.0.2.0",
          "advRouter":"0.0.0.1",
          "sequence":2147484123,
          "checksum":30961,
          "length":28,
          "networkMask":24,
          "metric":10
        },
        {
          "lsAge":655,
          "optionsList":"*|-|-|-|-|-|E|-",
          "lsType":"summary-LSA",
          "linkStateId":"192.168.2.0",
          "advRouter":"0.0.0.1",
          "sequence":2147484006,
          "checksum":13388,
          "length":28,
          "networkMask":24,
          "metric":10
        },
        {
          "lsAge":1284,
          "optionsList":"*|-|-|-|-|-|E|-",
          "lsType":"summary-LSA",
          "linkStateId":"192.168.6.0",
          "advRouter":"192.168.7.2",
          "sequence":2147483756,
          "checksum":47946,
          "length":28,
          "networkMask":24,
          "metric":10
        }
      ]
    },
    {
      "area":"0.0.0.2 [NSSA]",
      "routes":[
        {
          "lsAge":1404,
          "optionsList":"*|-|-|-|-|-|-|-",
          "lsType":"summary-LSA",
          "linkStateId":"0.0.0.0",
          "advRouter":"192.168.7.2",
          "sequence":2147483756,
          "checksum":29453,
          "length":28,
          "networkMask":0,
          "metric":1
        },
        {
          "lsAge":1164,
          "optionsList":"*|-|-|-|-|-|-|-",
          "lsType":"summary-LSA",
          "linkStateId":"10.0.2.0",
          "advRouter":"192.168.7.2",
          "sequence":2147483757,
          "checksum":38856,
          "length":28,
          "networkMask":24,
          "metric":20
        },
        {
          "lsAge":1344,
          "optionsList":"*|-|-|-|-|-|-|-",
          "lsType":"summary-LSA",
          "linkStateId":"10.0.4.0",
          "advRouter":"192.168.7.2",
          "sequence":2147483756,
          "checksum":33755,
          "length":28,
          "networkMask":24,
          "metric":20
        },
        {
          "lsAge":1174,
          "optionsList":"*|-|-|-|-|-|-|-",
          "lsType":"summary-LSA",
          "linkStateId":"192.168.2.0",
          "advRouter":"192.168.7.2",
          "sequence":2147483757,
          "checksum":26776,
          "length":28,
          "networkMask":24,
          "metric":20
        },
        {
          "lsAge":1114,
          "optionsList":"*|-|-|-|-|-|-|-",
          "lsType":"summary-LSA",
          "linkStateId":"192.168.3.0",
          "advRouter":"192.168.7.2",
          "sequence":2147483756,
          "checksum":24481,
          "length":28,
          "networkMask":24,
          "metric":20
        },
        {
          "lsAge":1674,
          "optionsList":"*|-|-|-|-|-|-|-",
          "lsType":"summary-LSA",
          "linkStateId":"192.168.5.0",
          "advRouter":"192.168.7.2",
          "sequence":2147483756,
          "checksum":58404,
          "length":28,
          "networkMask":24,
          "metric":10
        },
        {
          "lsAge":1464,
          "optionsList":"*|-|-|-|-|-|-|-",
          "lsType":"summary-LSA",
          "linkStateId":"192.168.7.0",
          "advRouter":"192.168.7.2",
          "sequence":2147483756,
          "checksum":52792,
          "length":28,
          "networkMask":24,
          "metric":10
        }
      ]
    }
  ]

}
)";

    auto result = vtss::frr_ip_ospf_db_summary_parse(ip_ospf_db);

    vtss::APPL_FrrOspfDbCommonKey k;
    vtss::APPL_FrrOspfDbSummaryStateVal v;

    k.area_id = 0x00000002;
    k.type = vtss::FrrOspfLsdbType_Summary;
    k.link_state_id = 0xC0A80500;
    k.adv_router = 0xC0A80702;

    auto itr = result.find(k);
    CHECK(itr != result.end());
    CHECK(result.size() == 10);
    CHECK(itr->second.age == 1674);
    CHECK(itr->second.checksum == 58404);
    CHECK(itr->second.sequence == 0x8000006c);
}

TEST_CASE("frr_show_ip_ospf_db_asbr_summary_1", "[frr]") {
    std::string ip_ospf_db = R"({
  "routerId":"0.0.0.3",
  "areas":[
    {
      "area":"0.0.0.0",
      "routes":[
        {
          "lsAge":155,
          "optionsList":"*|-|-|-|-|-|E|-",
          "lsType":"summary-LSA",
          "linkStateId":"0.0.0.3",
          "advRouter":"0.0.0.3",
          "sequence":2147483707,
          "checksum":56331,
          "length":48,
          "networkMask":0,
          "metric":30
        }
      ]
    }
  ]
}
)";

    auto result = vtss::frr_ip_ospf_db_asbr_summary_parse_raw(ip_ospf_db);
    vtss::FrrOspfDbASBRSummaryStatus db = result;

    CHECK(db.router_id == 0x3);
    CHECK(db.areas[0].area_id.area == 0);

    CHECK(db.areas[0].routes[0].age == 155);
    CHECK(db.areas[0].routes[0].options == "*|-|-|-|-|-|E|-");
    CHECK(db.areas[0].routes[0].type == 3);
    CHECK(db.areas[0].routes[0].link_state_id == 3);
    CHECK(db.areas[0].routes[0].adv_router == 3);
    CHECK(db.areas[0].routes[0].sequence == 2147483707);
    CHECK(db.areas[0].routes[0].checksum == 56331);
    CHECK(db.areas[0].routes[0].length == 48);

    CHECK(db.areas[0].routes[0].network_mask == 0);
    CHECK(db.areas[0].routes[0].metric == 30);
}

TEST_CASE("frr_show_ip_ospf_db_external_1", "[frr]") {
    std::string ip_ospf_db = R"({
  "routerId":"192.168.7.2",
  "AS External Link States":[
    {
      "routes":[
        {
          "lsAge":297,
          "optionsList":"*|-|-|-|-|-|E|-",
          "lsType":"AS-external-LSA",
          "linkStateId":"2.2.2.0",
          "advRouter":"0.0.0.4",
          "sequence":2147484003,
          "checksum":33741,
          "length":36,
          "networkMask":24,
          "type":2,
          "metric":20,
          "forwardAddress":"0.0.0.0"
        },
        {
          "lsAge":253,
          "optionsList":"*|-|-|-|-|-|E|-",
          "lsType":"AS-external-LSA",
          "linkStateId":"139.1.5.0",
          "advRouter":"192.168.7.2",
          "sequence":2147483984,
          "checksum":36075,
          "length":36,
          "networkMask":24,
          "type":1,
          "metric":20,
          "forwardAddress":"192.168.6.3"
        }
      ]
    }
  ]
}
)";

    auto result = vtss::frr_ip_ospf_db_external_parse_raw(ip_ospf_db);
    vtss::FrrOspfDbExternalStatus db = result;

    CHECK(db.router_id == 0xc0a80702);

    CHECK(db.type5[0].routes[0].age == 297);
    CHECK(db.type5[0].routes[0].options == "*|-|-|-|-|-|E|-");
    CHECK(db.type5[0].routes[0].type == 5);
    CHECK(db.type5[0].routes[0].link_state_id == 0x2020200);
    CHECK(db.type5[0].routes[0].adv_router == 4);
    CHECK(db.type5[0].routes[0].sequence == 0x80000163);
    CHECK(db.type5[0].routes[0].checksum == 33741);
    CHECK(db.type5[0].routes[0].length == 36);

    CHECK(db.type5[0].routes[0].network_mask == 24);
    CHECK(db.type5[0].routes[0].metric_type == 2);
    CHECK(db.type5[0].routes[0].metric == 20);
    CHECK(db.type5[0].routes[0].forward_address == 0);
}

TEST_CASE("frr_show_ip_ospf_db_external_2", "[frr]") {
    std::string ip_ospf_db = R"({
  "routerId":"192.168.7.2",
  "AS External Link States":[
    {
      "routes":[
        {
          "lsAge":297,
          "optionsList":"*|-|-|-|-|-|E|-",
          "lsType":"AS-external-LSA",
          "linkStateId":"2.2.2.0",
          "advRouter":"0.0.0.4",
          "sequence":2147484003,
          "checksum":33741,
          "length":36,
          "networkMask":24,
          "type":2,
          "metric":20,
          "forwardAddress":"0.0.0.0"
        },
        {
          "lsAge":253,
          "optionsList":"*|-|-|-|-|-|E|-",
          "lsType":"AS-external-LSA",
          "linkStateId":"139.1.5.0",
          "advRouter":"192.168.7.2",
          "sequence":2147483984,
          "checksum":36075,
          "length":36,
          "networkMask":24,
          "type":1,
          "metric":20,
          "forwardAddress":"192.168.6.3"
        }
      ]
    }
  ]
}
)";

    auto result = vtss::frr_ip_ospf_db_external_parse(ip_ospf_db);

    vtss::APPL_FrrOspfDbCommonKey k;
    vtss::APPL_FrrOspfDbExternalStateVal v;

    k.area_id = 0xffffffff;
    k.type = vtss::FrrOspfLsdbType_AsExternal;
    k.link_state_id = 0x8B010500;
    k.adv_router = 0xC0A80702;

    auto itr = result.find(k);
    CHECK(itr != result.end());
    CHECK(result.size() == 2);
    CHECK(itr->second.age == 253);
    CHECK(itr->second.checksum == 36075);
    CHECK(itr->second.sequence == 0x80000150);
}

TEST_CASE("frr_show_ip_ospf_db_external_3", "[frr]") {
    std::string ip_ospf_db = R"({
  "routerId":"0.0.0.3",
  "AS External Link States":[
    {
      "routes":[
      ]
    }
  ]
}
)";

    auto result = vtss::frr_ip_ospf_db_external_parse(ip_ospf_db);
    CHECK(result.size() == 0);
}

TEST_CASE("frr_show_ip_ospf_db_nssa_external_1", "[frr]") {
    std::string ip_ospf_db = R"({
  "routerId":"0.0.0.3",
  "areas":[
    {
      "area":"0.0.0.0",
      "routes":[
        {
          "lsAge":165,
          "optionsList":"*|-|-|-|-|-|E|-",
          "lsType":"NSSA-LSA",
          "linkStateId":"0.0.0.3",
          "advRouter":"0.0.0.3",
          "sequence":2147483707,
          "checksum":56331,
          "length":48,
          "networkMask":24,
          "type":2,
          "metric":20,
          "forwardAddress":"0.0.0.0"
        }
      ]
    }
  ]
}
)";

    auto result = vtss::frr_ip_ospf_db_nssa_external_parse_raw(ip_ospf_db);
    vtss::FrrOspfDbNSSAExternalStatus db = result;

    CHECK(db.router_id == 0x3);
    CHECK(db.areas[0].area_id.area == 0);

    CHECK(db.areas[0].routes[0].age == 165);
    CHECK(db.areas[0].routes[0].options == "*|-|-|-|-|-|E|-");
    CHECK(db.areas[0].routes[0].type == 7);
    CHECK(db.areas[0].routes[0].link_state_id == 3);
    CHECK(db.areas[0].routes[0].adv_router == 3);
    CHECK(db.areas[0].routes[0].sequence == 2147483707);
    CHECK(db.areas[0].routes[0].checksum == 56331);
    CHECK(db.areas[0].routes[0].length == 48);

    CHECK(db.areas[0].routes[0].network_mask == 24);
    CHECK(db.areas[0].routes[0].metric_type == 2);
    CHECK(db.areas[0].routes[0].metric == 20);
    CHECK(db.areas[0].routes[0].forward_address == 0);
}
