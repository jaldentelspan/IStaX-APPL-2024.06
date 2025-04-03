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

#include "web_api.h"
#if defined(VTSS_SW_OPTION_FAN)
#include "fan_api.h"
#else
#define fan_module_enabled() (false)
#endif /* VTSS_SW_OPTION_FAN */
#if defined(VTSS_SW_OPTION_LED_POW_REDUC)
#include "led_pow_reduc_api.h"
#else
#define led_pow_reduc_module_enabled() (false)
#endif /* VTSS_SW_OPTION_LED_POW_REDUC */
#if defined(VTSS_SW_OPTION_THERMAL_PROTECT)
#include "thermal_protect_api.h"
#else
#define thermal_protect_module_enabled() (false)
#endif /* VTSS_SW_OPTION_THERMAL_PROTECT */
#if defined(VTSS_SW_OPTION_FRR)
#include "frr_daemon.hxx" /* For frr_has_XXX() */
#endif /* VTSS_SW_OPTION_FRR */

#if defined(VTSS_SW_OPTION_REDBOX)
#include <vtss/appl/redbox.h>
#endif

int WEB_navbar(CYG_HTTPD_STATE* p, int &size) {
#define ITEM(X) WEB_navbar_menu_item(p, size, X, level)
    int level = -1;

    ITEM("Configuration");
    ITEM(" System");
#if defined(VTSS_SW_OPTION_SYSLOG)
    ITEM("  Information,sysinfo_config.htm");
#endif /* VTSS_SW_OPTION_SYSLOG */
    ITEM("  IP,ip_config.htm");
#if defined(VTSS_SW_OPTION_SNMP)
    ITEM("  SNMP,,snmp_menu");
    ITEM("   System,snmp.htm");
#if defined(VTSS_SW_OPTION_JSON_RPC)
    ITEM("   Trap");
    ITEM("    Destinations,trap.htm");
    ITEM("    Sources,trap_source.htm");
#endif /* VTSS_SW_OPTION_JSON_RPC */
    ITEM("   Communities,snmpv3_communities.htm");
    ITEM("   Users,snmpv3_users.htm");
    ITEM("   Groups,snmpv3_groups.htm");
    ITEM("   Views,snmpv3_views.htm");
    ITEM("   Access,snmpv3_accesses.htm");
#endif /* VTSS_SW_OPTION_SNMP */
#if defined(VTSS_SW_OPTION_NTP)
    ITEM("  NTP,ntp.htm");
#endif
#if defined(VTSS_SW_OPTION_DAYLIGHT_SAVING)
    ITEM("  Time,daylight_saving_config.htm");
#endif
#if defined(VTSS_SW_OPTION_SYSLOG)
    ITEM("  Log,syslog_config.htm");
#endif /* VTSS_SW_OPTION_SYSLOG */
#if defined(VTSS_SW_OPTION_LED_POW_REDUC) || defined(VTSS_SW_OPTION_FAN) || defined(VTSS_SW_OPTION_EEE) || defined(VTSS_SW_OPTION_PHY_POWER_CONTROL)
    ITEM(" Green Ethernet");
    if (fan_module_enabled()) {
        ITEM("  Fan,fan_config.htm");
    }
    if (led_pow_reduc_module_enabled()) {
        ITEM("  LED,led_pow_reduc_config.htm");
    }
#if defined(VTSS_SW_OPTION_PORT_POWER_SAVINGS)
    ITEM("  Port Power Savings,port_power_savings_config.htm");
#endif /* VTSS_SW_OPTION_PORT_POWER_SAVINGS */
#endif /* Green Ethernet */
    if (thermal_protect_module_enabled()) {
        ITEM(" Thermal Protection,thermal_protect_config.htm");
    }
    ITEM(" Ports,ports.htm");
#if defined (VTSS_SW_OPTION_CFM)
    ITEM(" CFM");
    ITEM("  Global,cfm_config_global.htm");
    ITEM("  Domain,cfm_config_md.htm");
    ITEM("  Service,cfm_config_ma.htm");
    ITEM("  MEP,cfm_config_mep.htm");
#endif
#if defined(VTSS_SW_OPTION_APS)
    ITEM(" APS,aps_config.htm");
#endif
#if defined(VTSS_SW_OPTION_ERPS)
    ITEM(" ERPS,erps_ctrl.htm");
#endif
#if defined(VTSS_SW_OPTION_IEC_MRP)
    ITEM(" Media Redundancy,iec_mrp_ctrl.htm");
#endif
#if defined(VTSS_SW_OPTION_DHCP_HELPER)
    ITEM(" DHCPv4");
#if defined(VTSS_SW_OPTION_DHCP_SERVER)
    ITEM("  Server");
    ITEM("   Mode,dhcp_server_mode.htm");
    ITEM("   Excluded IP,dhcp_server_excluded.htm");
    ITEM("   Pool,dhcp_server_pool.htm");
#endif /* VTSS_SW_OPTION_DHCP_SERVER */
#if defined(VTSS_SW_OPTION_DHCP_SNOOPING)
    ITEM("  Snooping,dhcp_snooping.htm");
#endif
#if defined(VTSS_SW_OPTION_DHCP_RELAY)
    ITEM("  Relay,dhcp_relay.htm");
#endif
#endif /* VTSS_SW_OPTION_DHCP_HELPER */
#if defined(VTSS_SW_OPTION_DHCP6_SNOOPING)
    ITEM(" DHCPv6");
    ITEM("  Snooping,dhcp6_snooping.htm");
#endif /* VTSS_SW_OPTION_DHCP6_SNOOPING */
#if defined(VTSS_SW_OPTION_DHCP6_RELAY)
    ITEM("  Relay,dhcp6_relay.htm");
#endif //VTSS_SW_OPTION_DHCP6_RELAY
    ITEM(" Security");
    ITEM("  Switch");
#if defined(VTSS_SW_OPTION_USERS)
    ITEM("   Users,users.htm");
#else
    ITEM("   Password,passwd_config.htm");
#endif
#if defined(VTSS_SW_OPTION_PRIV_LVL)
    ITEM("   Privilege Levels,priv_lvl.htm");
#endif
#if defined(VTSS_SW_OPTION_AUTH)
    ITEM("   Auth Method,auth_method_config.htm");
#endif
#if defined(VTSS_SW_OPTION_SSH)
    ITEM("   SSH,ssh_config.htm");
#endif
#if defined(VTSS_SW_OPTION_HTTPS) || defined(VTSS_SW_OPTION_FAST_CGI)
    ITEM("   HTTPS,https_config.htm");
#endif
#if defined(VTSS_SW_OPTION_ACCESS_MGMT)
    ITEM("   Access Management,access_mgmt.htm");
#endif
#if defined(VTSS_SW_OPTION_SNMP)
#if defined(VTSS_SW_OPTION_RMON)
    ITEM("   RMON");
    ITEM("    Statistics,rmon_statistics_config.htm");
    ITEM("    History,rmon_history_config.htm");
    ITEM("    Alarm,rmon_alarm_config.htm");
    ITEM("    Event,rmon_event_config.htm");
#endif /* VTSS_SW_OPTION_RMON */
#endif /* VTSS_SW_OPTION_SNMP */
    ITEM("  Network");
#if defined(VTSS_SW_OPTION_PSEC_LIMIT)
    ITEM("   Port Security");
    ITEM("    Configuration,psec_limit.htm");
    ITEM("    MAC Addresses,psec_limit_mac.htm");
#endif
#if defined(VTSS_SW_OPTION_DOT1X)
    ITEM("   NAS,nas.htm");
#endif
#if defined(VTSS_SW_OPTION_ACL)
    ITEM("   ACL");
    ITEM("    Ports,acl_ports.htm");
    ITEM("    Rate Limiters,acl_ratelimiter.htm");
    ITEM("    Access Control List,acl.htm");
#endif
#if defined(VTSS_SW_OPTION_IP_SOURCE_GUARD)
    ITEM("   IP Source Guard");
    ITEM("    Configuration,ip_source_guard.htm");
    ITEM("    Static Table,ip_source_guard_static_table.htm");
#endif
#if defined(VTSS_SW_OPTION_IPV6_SOURCE_GUARD)
    ITEM("   IPv6 Source Guard");
    ITEM("    Configuration,ipv6_source_guard_conf.htm");
    ITEM("    Static Table,ipv6_source_guard_static_table.htm");
#endif
#if defined(VTSS_SW_OPTION_ARP_INSPECTION)
    ITEM("   ARP Inspection");
    ITEM("    Port Configuration,arp_inspection.htm");
    ITEM("    VLAN Configuration,arp_inspection_vlan.htm");
    ITEM("    Static Table,arp_inspection_static_table.htm");
    ITEM("    Dynamic Table,dynamic_arp_inspection.htm");
#endif



#if defined(VTSS_SW_OPTION_AUTH)
#if defined(VTSS_SW_OPTION_RADIUS) || defined(VTSS_SW_OPTION_TACPLUS)
    ITEM("  AAA");
#endif /* defined(VTSS_SW_OPTION_RADIUS) || defined(VTSS_SW_OPTION_TACPLUS) */
#if defined(VTSS_SW_OPTION_RADIUS)
    ITEM("   RADIUS,auth_radius_config.htm");
#endif /* defined(VTSS_SW_OPTION_RADIUS) */
#if defined(VTSS_SW_OPTION_TACPLUS)
    ITEM("   TACACS+,auth_tacacs_config.htm");
#endif /* defined(VTSS_SW_OPTION_TACPLUS) */
#endif /* defined(VTSS_SW_OPTION_AUTH) */
#if defined(VTSS_SW_OPTION_AGGR) || defined(VTSS_SW_OPTION_LACP)
    ITEM(" Aggregation");
#if defined(VTSS_SW_OPTION_AGGR)
    ITEM("  Common,aggr_common.htm");
    ITEM("  Groups,aggr_groups.htm");
#endif
#if defined(VTSS_SW_OPTION_LACP)
    ITEM("  LACP,lacp_port_config.htm");
#endif
#endif /* defined(VTSS_SW_OPTION_AGGR) || defined(VTSS_SW_OPTION_LACP) */
#if defined(VTSS_SW_OPTION_ETH_LINK_OAM)
    ITEM(" Link OAM");
    ITEM("  Port Settings,eth_link_oam_port_config.htm");
    ITEM("  Event Settings,eth_link_oam_link_event_config.htm");
#endif
#if defined(VTSS_SW_OPTION_LOOP_PROTECTION)
    ITEM(" Loop Protection,loop_config.htm");
#endif /* defined(VTSS_SW_OPTION_LOOP_PROTECTION) */
#if defined(VTSS_SW_OPTION_MSTP)
#if defined(VTSS_SW_OPTION_BUILD_SMB)
#define VTSS_MSTP_FULL 1
#endif
    ITEM(" Spanning Tree");
    ITEM("  Bridge Settings,mstp_sys_config.htm");
#if defined(VTSS_MSTP_FULL)
    ITEM("  MSTI Mapping,mstp_msti_map_config.htm");
    ITEM("  MSTI Priorities,mstp_msti_config.htm");
    ITEM("  CIST Ports,mstp_port_config.htm");
    ITEM("  MSTI Ports,mstp_msti_port_config.htm");
#else
    ITEM("  Bridge Ports,mstp_port_config.htm");
#endif // VTSS_MSTP_FULL
#endif // MSTP
#if ((defined(VTSS_SW_OPTION_SMB_IPMC) || defined(VTSS_SW_OPTION_MVR)) && defined(VTSS_SW_OPTION_IPMC_LIB))
    ITEM(" IPMC Profile");
    ITEM("  Profile Table,ipmc_lib_profile_table.htm");
    ITEM("  Address Entry,ipmc_lib_range_table.htm");
#endif /* (VTSS_SW_OPTION_SMB_IPMC || VTSS_SW_OPTION_MVR) && VTSS_SW_OPTION_IPMC_LIB */
#if defined(VTSS_SW_OPTION_MVR)
    ITEM(" MVR,mvr.htm");
#endif /* VTSS_SW_OPTION_MVR */
#if defined(VTSS_SW_OPTION_IPMC)
    ITEM(" IPMC");
    ITEM("  IGMP Snooping");
    ITEM("   Basic Configuration,ipmc_igmps.htm");
    ITEM("   VLAN Configuration,ipmc_igmps_vlan.htm");
#if defined(VTSS_SW_OPTION_SMB_IPMC)
    ITEM("   Port Filtering Profile,ipmc_igmps_filtering.htm");
    ITEM("  MLD Snooping");
    ITEM("   Basic Configuration,ipmc_mldsnp.htm");
    ITEM("   VLAN Configuration,ipmc_mldsnp_vlan.htm");
    ITEM("   Port Filtering Profile,ipmc_mldsnp_filtering.htm");
#endif /* VTSS_SW_OPTION_SMB_IPMC */
#endif /* VTSS_SW_OPTION_IPMC */
#if defined(VTSS_SW_OPTION_LLDP)
    ITEM(" LLDP");
    ITEM("  LLDP,lldp_config.htm");
#if defined(VTSS_SW_OPTION_LLDP_MED)
    ITEM("  LLDP-MED,lldp_med_config.htm");
#endif
#endif
#if defined(VTSS_SW_OPTION_POE)
    ITEM(" PoE,poe_config.htm");
#endif
#if defined(VTSS_SW_OPTION_SYNCE)
    ITEM(" SyncE,synce_config.htm");
#endif
#if defined(VTSS_SW_OPTION_MAC)
    ITEM(" MAC Table,mac.htm");
#endif /* VTSS_SW_OPTION_MAC */
#if defined(VTSS_SW_OPTION_VLAN)
    if (fast_cap(MESA_CAP_L2_SVL_FID_CNT)) {
        ITEM(" VLANs");
        ITEM("  Configuration,vlan.htm");
        ITEM("  SVL,vlan_svl.htm");
    } else {
        ITEM(" VLANs,vlan.htm");
    }
#endif /* VTSS_SW_OPTION_VLAN  */
#if defined(VTSS_SW_OPTION_VLAN_TRANSLATION)
    ITEM(" VLAN Translation");
    ITEM("  Port to Group Configuration,vlan_trans_port_config.htm");
    ITEM("  VLAN Translation Mappings,map.htm");
#endif /* VTSS_SW_OPTION_VLAN_TRANSLATION */
#if defined(VTSS_SW_OPTION_PVLAN)
    ITEM(" Private VLANs");
    ITEM("  Membership,pvlan.htm");
    ITEM("  Port Isolation,port_isolation.htm");
#endif /* VTSS_SW_OPTION_PVLAN */
#if defined(VTSS_SW_OPTION_VCL)
    ITEM(" VCL");
    ITEM("  MAC-based VLAN,mac_based_vlan.htm");
    ITEM("  Protocol-based VLAN");
    ITEM("   Protocol to Group,vcl_protocol_grp_map.htm");
    ITEM("   Group to VLAN,vcl_grp_2_vlan_mapping.htm");
    ITEM("  IP Subnet-based VLAN,subnet_based_vlan.htm");
#endif /* VTSS_SW_OPTION_VCL */
#if defined(VTSS_SW_OPTION_VOICE_VLAN)
    ITEM(" Voice VLAN");
    ITEM("  Configuration,voice_vlan_config.htm");
    ITEM("  OUI,voice_vlan_oui.htm");
#endif /* VTSS_SW_OPTION_VOICE_VLAN */
#if defined(VTSS_SW_OPTION_QOS)
    ITEM(" QoS");
    ITEM("  Port Classification,qos_port_classification.htm");
    ITEM("  Port Policing,qos_port_policers.htm");
#if defined(VTSS_SW_OPTION_BUILD_SMB)
    ITEM("  Queue Policing,qos_queue_policers.htm");
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) */
    ITEM("  Port Scheduler,qos_port_schedulers.htm");
    ITEM("  Port Shaping,qos_port_shapers.htm");
#if defined(VTSS_SW_OPTION_BUILD_SMB)
    ITEM("  Port Tag Remarking,qos_port_tag_remarking.htm");
    ITEM("  Port DSCP,qos_port_dscp_config.htm");
    ITEM("  DSCP-Based QoS,dscp_based_qos_ingr_classifi.htm");
    ITEM("  DSCP Translation,dscp_translation.htm");
    ITEM("  DSCP&nbsp;Classification,dscp_classification.htm");
    if (fast_cap(MESA_CAP_QOS_INGRESS_MAP_CNT)) {
        ITEM("  Ingress Map,qos_ingress_map.htm");
    }
    if (fast_cap(MESA_CAP_QOS_EGRESS_MAP_CNT)) {
        ITEM("  Egress Map,qos_egress_map.htm");
    }
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) */
    ITEM("  QoS Control List,qcl_v2.htm");
    ITEM("  Storm Policing,stormctrl.htm");
    if (fast_cap(MESA_CAP_QOS_WRED_GROUP_CNT)) {
        ITEM("  WRED,qos_wred.htm");
    }
#endif /* VTSS_SW_OPTION_QOS */

#if defined(VTSS_SW_OPTION_TSN) && defined(VTSS_SW_OPTION_BUILD_ISTAX)
    ITEM(" TSN");
    ITEM("  Streams,stream_ctrl.htm");
    ITEM("  Stream Collections,stream_collection_config.htm");
    if (fast_cap(MESA_CAP_QOS_TAS) || fast_cap(MESA_CAP_L2_PSFP)) {
        ITEM("  PTP check,tsn_config.htm");
    }
    if (fast_cap(MESA_CAP_QOS_FRAME_PREEMPTION)) {
        ITEM("  Frame Preemption,tsn_port_fp_config.htm");
    }
    if (fast_cap(MESA_CAP_QOS_TAS)) {
        ITEM("  TAS");
        ITEM("   Ports,tsn_port_tas_config.htm");
        ITEM("   Max SDU,tsn_port_tas_max_sdu.htm");
    }
    if (fast_cap(MESA_CAP_L2_PSFP)) {
        ITEM("  PSFP");
        ITEM("   Flow Meters,psfp_flow_meter_config.htm");
        ITEM("   Stream Gates,psfp_gate_ctrl.htm");
        ITEM("   Stream Filters,psfp_filter_config.htm");
    }
#if defined(VTSS_SW_OPTION_TSN_FRER)
    if (fast_cap(MESA_CAP_L2_FRER)) {
        ITEM("  FRER,frer_ctrl.htm");
    }
#endif /* defined(VTSS_SW_OPTION_TSN_FRER) */
#endif /* defined(VTSS_SW_OPTION_TSN) && defined(VTSS_SW_OPTION_BUILD_ISTAX) */

#if defined(VTSS_SW_OPTION_MIRROR) && defined(VTSS_SW_OPTION_JSON_RPC)
#if defined(VTSS_SW_OPTION_MIRROR_LOOP_PORT)
    ITEM(" Mirroring,mirror_no_reflector_port.htm");
#else
    ITEM(" Mirroring,mirror_ctrl.htm");
#endif
#endif /* VTSS_SW_OPTION_RMIRROR  && VTSS_SW_OPTION_JSON_RPC */
#if defined(VTSS_SW_OPTION_UPNP)
    ITEM(" UPnP,upnp.htm");
#endif /* VTSS_SW_OPTION_UPNP */
#if defined(VTSS_SW_OPTION_PTP)
    ITEM(" PTP,ptp_config.htm");
#endif /* VTSS_SW_OPTION_PTP */
#if defined(VTSS_SW_OPTION_MRP)
    ITEM(" MRP");
    ITEM("  Ports,mrp_port.htm");
#if defined(VTSS_SW_OPTION_MVRP)
    ITEM("  MVRP,mvrp_config.htm");
#endif /* VTSS_SW_OPTION_MVRP */
#endif /* VTSS_SW_OPTION_MRP */
#if defined(VTSS_SW_OPTION_GVRP)
    ITEM(" GVRP");
    ITEM("  Global config,gvrp_config.htm");
    ITEM("  Port config,gvrp_port.htm");
#endif /* VTSS_SW_OPTION_GVRP */
#if defined(VTSS_SW_OPTION_SFLOW)
    ITEM(" sFlow,sflow.htm");
#endif /* VTSS_SW_OPTION_SFLOW */
#if defined(VTSS_SW_OPTION_DDMI)
    ITEM(" DDMI,ddmi_config.htm");
#endif /* VTSS_SW_OPTION_DDMI */
#if defined(VTSS_SW_OPTION_UDLD)
    ITEM(" UDLD,udld_config.htm");
#endif

#if defined(VTSS_SW_OPTION_FRR_ROUTER)
    if (frr_has_router()) {
        ITEM(" Router");
        ITEM("  Key-Chain,frr_router_key_chain_config.htm");
        ITEM("  Key-Chain Key ID,frr_router_key_chain_key_id_config.htm");
        ITEM("  Access-list,frr_router_access_list_config.htm");
    }
#endif
#if defined(VTSS_SW_OPTION_FRR_OSPF)
    if (frr_has_ospfd()) {
        ITEM(" OSPF");
        ITEM("  Configuration,frr_ospf_global_config.htm");
        ITEM("  Network Area,frr_ospf_network_area_config.htm");
        ITEM("  Passive Interface,frr_ospf_passive_intf_config.htm");
        ITEM("  Stub Area,frr_ospf_stub_area_config.htm");
        ITEM("  Area Authentication,frr_ospf_area_auth_config.htm");
        ITEM("  Area Range,frr_ospf_area_range_config.htm");
        ITEM("  Interfaces,frr_ospf_intf_config.htm");
        ITEM("  Virtual Link,frr_ospf_vlink_config.htm");
    }
#endif
#if defined(VTSS_SW_OPTION_FRR_OSPF6)
    if (frr_has_ospf6d()) {
        ITEM(" OSPFv3");
        ITEM("  Configuration,frr_ospf6_global_config.htm");
        ITEM("  Interface Area,frr_ospf6_passive_intf_config.htm");
        ITEM("  Stub Area,frr_ospf6_stub_area_config.htm");
        ITEM("  Area Range,frr_ospf6_area_range_config.htm");
        ITEM("  Interfaces,frr_ospf6_intf_config.htm");
    }
#endif
#if defined(VTSS_SW_OPTION_FRR_RIP)
    if (frr_has_ripd()) {
        ITEM(" RIP");
        ITEM("  Configuration,frr_rip_global_config.htm");
        ITEM("  Network,frr_rip_network_config.htm");
        ITEM("  Neighbors,frr_rip_neighbor_config.htm");
        ITEM("  Passive Interface,frr_rip_passive_intf_config.htm");
        ITEM("  Interfaces,frr_rip_intf_config.htm");
        ITEM("  Offset-List,frr_rip_offset_list_config.htm");
    }
#endif

#if defined(VTSS_SW_OPTION_REDBOX)
    if (vtss_appl_redbox_supported()) {
        ITEM(" RedBox,redbox_config.htm");
    }
#endif /* defined(VTSS_SW_OPTION_REDBOX) */

    ITEM("Monitor");
    ITEM(" System");
    ITEM("  Information,sys.htm");
    ITEM("  LED status,sys_led_status.htm");
    ITEM("  CPU Load,perf_cpuload.htm");
#if defined(VTSS_SW_OPTION_IP)
    ITEM("  IP Status,ip_status.htm");
#endif
#if defined(VTSS_SW_OPTION_FRR)
    ITEM("  IPv4 Routing Info. Base,ip_routing_info_base_status.htm");
    ITEM("  IPv6 Routing Info. Base,ipv6_routing_info_base_status.htm");
#endif /* VTSS_SW_OPTION_FRR */
#if defined(VTSS_SW_OPTION_SYSLOG)
    ITEM("  Log,syslog.htm");
    ITEM("  Detailed Log,syslog_detailed.htm");
#endif /* VTSS_SW_OPTION_SYSLOG */
#if defined(VTSS_SW_OPTION_PSU)
    ITEM("  Power Supply,sys_psu_status.htm");
#endif /* VTSS_SW_OPTION_PSU */
#if defined(VTSS_SW_OPTION_PORT_POWER_SAVINGS) || defined(VTSS_SW_OPTION_FAN) || defined(VTSS_SW_OPTION_EEE)
    ITEM(" Green Ethernet");
#if defined(VTSS_SW_OPTION_PORT_POWER_SAVINGS)
    ITEM("  Port Power Savings,port_power_savings_status.htm");
#endif /* VTSS_SW_OPTION_PORT_POWER_SAVINGS */
    if (fan_module_enabled()) {
        ITEM("  Fan,fan_status.htm");
    }
#endif
    if (thermal_protect_module_enabled()) {
        ITEM(" Thermal Protection,thermal_protect_status.htm");
    }
    ITEM(" Ports");
    ITEM("  State,main.htm");
    ITEM("  Traffic Overview,stat_ports.htm");
#if defined(VTSS_SW_OPTION_QOS)
    ITEM("  QoS Statistics,qos_counter.htm");
#if defined(VTSS_SW_OPTION_QOS_QCL_INCLUDE)
#if VTSS_SW_OPTION_QOS_QCL_INCLUDE == 1
    ITEM("  QCL Status,qcl_v2_stat.htm");
#endif /* VTSS_SW_OPTION_QOS_QCL_INCLUDE == 1 */
#else
    ITEM("  QCL Status,qcl_v2_stat.htm");
#endif /* defined(VTSS_SW_OPTION_QOS_QCL_INCLUDE) */
#endif /* defined(VTSS_SW_OPTION_QOS) */
    ITEM("  Detailed Statistics,stat_detailed.htm");
    ITEM("  Name Map,port_namemap.htm");
#if defined (VTSS_SW_OPTION_CFM)
    ITEM(" CFM");
    ITEM("  Status, cfm_mep_status.htm");
#endif
#if defined(VTSS_SW_OPTION_APS)
    ITEM(" APS,aps_status.htm");
#endif
#if defined(VTSS_SW_OPTION_ERPS)
    ITEM(" ERPS,erps_status.htm");
#endif
#if defined(VTSS_SW_OPTION_IEC_MRP)
    ITEM(" Media Redundancy,iec_mrp_status.htm");
#endif
#if defined(VTSS_SW_OPTION_ETH_LINK_OAM)
    ITEM(" Link OAM");
    ITEM("  Statistics,eth_link_oam_stat_detailed.htm");
    ITEM("  Port Status,eth_link_oam_port_status.htm");
    ITEM("  Event Status,eth_link_oam_link_status.htm");
#endif
#if defined(VTSS_SW_OPTION_DHCP_HELPER)
    ITEM(" DHCPv4");
#if defined(VTSS_SW_OPTION_DHCP_SERVER)
    ITEM("  Server");
    ITEM("   Statistics,dhcp_server_stat.htm");
    ITEM("   Binding,dhcp_server_stat_binding.htm");
    ITEM("   Declined IP,dhcp_server_stat_declined.htm");
#endif /* VTSS_SW_OPTION_DHCP_SERVER */
#if defined(VTSS_SW_OPTION_DHCP_SNOOPING)
    ITEM("  Snooping Table,dyna_dhcp_snooping.htm");
#endif
#if defined(VTSS_SW_OPTION_DHCP_RELAY)
    ITEM("  Relay Statistics,dhcp_relay_statistics.htm");
#endif /* VTSS_SW_OPTION_DHCP_RELAY */
    ITEM("  Detailed Statistics,dhcp_helper_statistics.htm");
#endif /* VTSS_SW_OPTION_DHCP_HELPER */
#if defined(VTSS_SW_OPTION_DHCP6_SNOOPING)
    ITEM(" DHCPv6");
    ITEM("  Snooping Table,dhcp6_snooping_table.htm");
    ITEM("  Snooping Statistics,dhcp6_snooping_stats.htm");
#endif /* VTSS_SW_OPTION_DHCP6_SNOOPING */
#if defined(VTSS_SW_OPTION_DHCP6_RELAY)
    ITEM("  Relay,dhcp6_relay_status.htm");
#endif //VTSS_SW_OPTION_DHCP6_RELAY
    ITEM(" Security");

    ITEM("  Switch");
#if defined(VTSS_SW_OPTION_ACCESS_MGMT)
    ITEM("   Access Management Statistics,access_mgmt_statistics.htm");
#endif
#if defined(VTSS_SW_OPTION_RMON)
    ITEM("   RMON");
    ITEM("    Statistics,rmon_statistics_status.htm");
    ITEM("    History,rmon_history_status.htm");
    ITEM("    Alarm,rmon_alarm_status.htm");
    ITEM("    Event,rmon_event_status.htm");
#endif /* VTSS_SW_OPTION_RMON */

    ITEM("  Network");
#if defined(VTSS_SW_OPTION_PSEC)
    ITEM("   Port Security");
    ITEM("    Overview,psec_status_switch.htm");
    ITEM("    Details,psec_status_port.htm");
#endif
#if defined(VTSS_SW_OPTION_DOT1X)
    ITEM("   NAS");
    ITEM("    Switch,nas_status_switch.htm");
    ITEM("    Port,nas_status_port.htm");
#endif
#if defined(VTSS_SW_OPTION_ACL)
    ITEM("   ACL Status,acl_status.htm");
#endif /* VTSS_SW_OPTION_ACL */
#if defined(VTSS_SW_OPTION_IP_SOURCE_GUARD)
    ITEM("   IP Source Guard,dyna_ip_source_guard.htm");
#endif /* VTSS_SW_OPTION_IP_SOURCE_GUARD */
#if defined(VTSS_SW_OPTION_IPV6_SOURCE_GUARD)
    ITEM("   IPv6 Source Guard,ipv6_source_guard_dynamic_table.htm");
#endif /* VTSS_SW_OPTION_IPV6_SOURCE_GUARD */
#if defined(VTSS_SW_OPTION_ARP_INSPECTION)
    ITEM("   ARP Inspection,dyna_arp_inspection.htm");
#endif /* VTSS_SW_OPTION_ARP_INSPECTION */







#if defined(VTSS_SW_OPTION_RADIUS)
    ITEM("  AAA");
    ITEM("   RADIUS Overview,auth_status_radius_overview.htm");
    ITEM("   RADIUS Details,auth_status_radius_details.htm");
#endif /* VTSS_SW_OPTION_RADIUS */

#if defined(VTSS_SW_OPTION_AGGR) || defined(VTSS_SW_OPTION_LACP)
    ITEM(" Aggregation");
#if defined(VTSS_SW_OPTION_AGGR)
    ITEM("  Status,aggregation_status.htm");
#endif
#if defined(VTSS_SW_OPTION_LACP)
    ITEM("  LACP");
    ITEM("   System Status,lacp_sys_status.htm");
    ITEM("   Internal Status,lacp_internal_status.htm");
    ITEM("   Neighbor Status,lacp_neighbor_status.htm");
    ITEM("   Port Statistics,lacp_statistics.htm");
#endif
#endif /* defined(VTSS_SW_OPTION_AGGR) || defined(VTSS_SW_OPTION_LACP) */
#if defined(VTSS_SW_OPTION_LOOP_PROTECTION)
    ITEM(" Loop Protection,loop_status.htm");
#endif /* defined(VTSS_SW_OPTION_LOOP_PROTECTION) */
#if defined(VTSS_SW_OPTION_MSTP)
    ITEM(" Spanning Tree");
#if defined(VTSS_MSTP_FULL)
    ITEM("  Bridge Status,mstp_status_bridges.htm");
#else
    ITEM("  Bridge Status,mstp_status_bridge.htm");
#endif
    ITEM("  Port Status,mstp_status_ports.htm");
    ITEM("  Port Statistics,mstp_statistics.htm");
#endif  /* VTSS_SW_OPTION_MSTP */
#if defined(VTSS_SW_OPTION_MVR)
    ITEM(" MVR");
    ITEM("  Statistics,mvr_status.htm");
    ITEM("  MVR Channel Groups,mvr_groups_info.htm");
    ITEM("  MVR SFM Information,mvr_groups_sfm.htm");
#endif /* VTSS_SW_OPTION_MVR */
#if defined(VTSS_SW_OPTION_IPMC)
    ITEM(" IPMC");
    ITEM("  IGMP Snooping");
    ITEM("   Status,ipmc_igmps_status.htm");
    ITEM("   Groups Information,ipmc_igmps_groups_info.htm");
#if defined(VTSS_SW_OPTION_SMB_IPMC)
    ITEM("   IPv4 SFM Information,ipmc_igmps_v3info.htm");
    ITEM("  MLD Snooping");
    ITEM("   Status,ipmc_mldsnp_status.htm");
    ITEM("   Groups Information,ipmc_mldsnp_groups_info.htm");
    ITEM("   IPv6 SFM Information,ipmc_mldsnp_v2info.htm");
#endif /* VTSS_SW_OPTION_SMB_IPMC */
#endif /* VTSS_SW_OPTION_IPMC */

#if defined(VTSS_SW_OPTION_LLDP)
    ITEM(" LLDP");
    ITEM("  Neighbors,lldp_neighbors.htm");
#if defined(VTSS_SW_OPTION_LLDP_MED)
    ITEM("  LLDP-MED Neighbors,lldp_neighbors_med.htm");
#endif /* VTSS_SW_OPTION_LLDP_MED */
#if defined(VTSS_SW_OPTION_POE)
    ITEM("  PoE,lldp_poe_neighbors.htm");
#endif /* VTSS_SW_OPTION_POE */
#if defined(VTSS_SW_OPTION_EEE)
    ITEM("  EEE,lldp_eee_neighbors.htm");
#endif
    ITEM("  Port Statistics,lldp_statistic.htm");
#endif /* VTSS_SW_OPTION_LLDP */
#if defined(VTSS_SW_OPTION_PTP)
    ITEM(" PTP");
    ITEM("  PTP,ptp.htm");
    ITEM("  PTP Statistics,ptp_as_statistics.htm");
#endif /* VTSS_SW_OPTION_PTP */
#if defined(VTSS_SW_OPTION_POE)
    ITEM(" PoE,poe_status.htm");
#endif /* VTSS_SW_OPTION_POE */
#if defined(VTSS_SW_OPTION_MAC)
    ITEM(" MAC Table,dyna_mac.htm");
#endif /* VTSS_SW_OPTION_MAC */
#if defined(VTSS_SW_OPTION_VLAN)
    ITEM(" VLANs");
    ITEM("  Membership,vlan_membership_stat.htm");
    ITEM("  Ports,vlan_port_stat.htm");
#endif /* VTSS_SW_OPTION_VLAN */
#if defined(VTSS_SW_OPTION_MVRP)
    ITEM(" MVRP,mvrp_stat.htm");
#endif /* VTSS_SW_OPTION_MVRP */
#if defined(VTSS_SW_OPTION_SFLOW)
    ITEM(" sFlow,sflow_status.htm");
#endif /* VTSS_SW_OPTION_SFLOW */
#if defined(VTSS_SW_OPTION_DDMI)
    ITEM(" DDMI");
    ITEM("  Overview,ddmi_overview.htm");
    ITEM("  Detailed,ddmi_detailed.htm");
#endif /* VTSS_SW_OPTION_DDMI */
#if defined(VTSS_SW_OPTION_UDLD)
    ITEM(" UDLD,udld_status.htm");
#endif

#if defined(VTSS_SW_OPTION_TSN) && defined(VTSS_SW_OPTION_BUILD_ISTAX)
    ITEM(" TSN");

    if (fast_cap(MESA_CAP_QOS_FRAME_PREEMPTION)) {
        ITEM("  Frame Preemption,tsn_port_fp_status.htm");
    }
    if (fast_cap(MESA_CAP_QOS_TAS)) {
        ITEM("  TAS,tsn_port_tas_status.htm");
    }
   if (fast_cap(MESA_CAP_L2_PSFP)) {
        ITEM("  PSFP");
          ITEM("   Capabilities,psfp_capabilities.htm");
          ITEM("   Stream Gate Status,psfp_gate_status_overview.htm");
          ITEM("   Stream Filter Status,psfp_filter_status.htm");
          ITEM("   Stream Filter Statistics,psfp_filter_statistics.htm");
   }
#if defined(VTSS_SW_OPTION_TSN_FRER)
   if (fast_cap(MESA_CAP_L2_FRER)) {
        ITEM("  FRER");
          ITEM("   Status, frer_status.htm");
          ITEM("   Statistics, frer_statistics.htm");
   }
#endif /* defined(VTSS_SW_OPTION_TSN_FRER) */
#endif /* defined(VTSS_SW_OPTION_TSN) && defined(VTSS_SW_OPTION_BUILD_ISTAX) */

#if defined(VTSS_SW_OPTION_FRR_OSPF)
    if (frr_has_ospfd()) {
        ITEM(" OSPF");
        ITEM("  Status,frr_ospf_global_status.htm");
        ITEM("  Area,frr_ospf_area_status.htm");
        ITEM("  Neighbor,frr_ospf_nbr_status.htm");
        ITEM("  Interface,frr_ospf_intf_status.htm");
        ITEM("  Routing,frr_ospf_routing_status.htm");
        ITEM("  Database");
        ITEM("   General Database,frr_ospf_db.htm");
        ITEM("   Detail Database");
        ITEM("    Router,frr_ospf_db_router.htm");
        ITEM("    Network,frr_ospf_db_network.htm");
        ITEM("    Summary,frr_ospf_db_summary.htm");
        ITEM("    ASBR Summary,frr_ospf_db_asbr_summary.htm");
        ITEM("    External,frr_ospf_db_external.htm");
        ITEM("    NSSA External,frr_ospf_db_nssa_external.htm");
    }
#endif /* VTSS_SW_OPTION_FRR_OSPF */
#if defined(VTSS_SW_OPTION_FRR_OSPF6)
    if (frr_has_ospfd()) {
        ITEM(" OSPFv3");
        ITEM("  Status,frr_ospf6_global_status.htm");
        ITEM("  Area,frr_ospf6_area_status.htm");
        ITEM("  Neighbor,frr_ospf6_nbr_status.htm");
        ITEM("  Interface,frr_ospf6_intf_status.htm");
        ITEM("  Routing,frr_ospf6_routing_status.htm");
        ITEM("  Database");
        ITEM("   General Database,frr_ospf6_db.htm");
        ITEM("   Detail Database");
        ITEM("    Router,frr_ospf6_db_router.htm");
        ITEM("    Network,frr_ospf6_db_network.htm");
        ITEM("    Link,frr_ospf6_db_link.htm");
        ITEM("    IntraAreaPrefix,frr_ospf6_db_intra_area_prefix.htm");
        ITEM("    InterArea Prefix,frr_ospf6_db_summary.htm");
        ITEM("    InterArea Router,frr_ospf6_db_asbr_summary.htm");
        ITEM("    External,frr_ospf6_db_external.htm");
    }
#endif /* VTSS_SW_OPTION_FRR_OSPF6 */
#if defined(VTSS_SW_OPTION_FRR_RIP)
    if (frr_has_ripd()) {
        ITEM(" RIP");
        ITEM("  Status,frr_rip_global_status.htm");
        ITEM("  Interface,frr_rip_intf_status.htm");
        ITEM("  Peer,frr_rip_peer_status.htm");
        ITEM("  Database,frr_rip_db_status.htm");
    }
#endif /* VTSS_SW_OPTION_FRR_RIP */

#if defined(VTSS_SW_OPTION_REDBOX)
    if (vtss_appl_redbox_supported()) {
        ITEM(" RedBox");
        ITEM("  Status,redbox_status.htm");
        ITEM("  Statistics,redbox_statistics_overview.htm");
        ITEM("  NodesTable,redbox_nt_overview.htm");
        ITEM("  ProxyNodeTable,redbox_pnt_overview.htm");
    }
#endif /* defined(VTSS_SW_OPTION_REDBOX) */

    ITEM("Diagnostics");
    ITEM(" Ping (IPv4),ping4.htm");
#if defined(VTSS_SW_OPTION_IPV6)
    ITEM(" Ping (IPv6),ping6.htm");
#endif /* VTSS_SW_OPTION_IPV6 */
    ITEM(" Traceroute (IPv4),traceroute4.htm");
#if defined(VTSS_SW_OPTION_IPV6)
    ITEM(" Traceroute (IPv6),traceroute6.htm");
#endif /* VTSS_SW_OPTION_IPV6 */
#if defined(VTSS_SW_OPTION_ETH_LINK_OAM)
    ITEM(" Link OAM");
    ITEM("  MIB Retrieval,eth_link_oam_mib_support.htm");
#endif
    ITEM(" VeriPHY,veriphy.htm");
    ITEM("Maintenance");
    ITEM(" Restart Device,wreset.htm");
    ITEM(" Factory Defaults,factory.htm");
    ITEM(" Software");
    ITEM("  Upload,upload.htm");
    ITEM("  Image Select,sw_select.htm");
#if defined(VTSS_SW_OPTION_ICFG)
    ITEM(" Configuration");
    ITEM("  Save startup-config,icfg_conf_save.htm");
    ITEM("  Download,icfg_conf_download.htm");
    ITEM("  Upload,icfg_conf_upload.htm");
    ITEM("  Activate,icfg_conf_activate.htm");
    ITEM("  Delete,icfg_conf_delete.htm");
#endif /* VTSS_SW_OPTION_ICFG*/

    return level;
}
