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

#include "vtss_module_id.h"

/* These module name will shown as privilege group name.
   Please don't use space in module name, use under line instead of it.
   The module name can be used as a command keyword. */
const char * const vtss_module_names[VTSS_MODULE_ID_NONE + 1] =
{
    [VTSS_MODULE_ID_API_IO]                /*   0 */ = "obsolete_api_io",
    [VTSS_MODULE_ID_API_CI]                /*   1 */ = "api_cil",
    [VTSS_MODULE_ID_API_AI]                /*   2 */ = "api_ail",
    [VTSS_MODULE_ID_SPROUT]                /*   3 */ = "sprout",
    [VTSS_MODULE_ID_MAIN]                  /*   4 */ = "main",
    [VTSS_MODULE_ID_TOPO]                  /*   5 */ = "Stack",
    [VTSS_MODULE_ID_CONF]                  /*   6 */ = "conf",
    [VTSS_MODULE_ID_MSG]                   /*   7 */ = "msg",
    [VTSS_MODULE_ID_PACKET]                /*   8 */ = "packet",
    [VTSS_MODULE_ID_TRACE]                 /*   9 */ = "trace",
    [VTSS_MODULE_ID_IP_STACK_GLUE]         /*  10 */ = "ip_stack_glue",
    [VTSS_MODULE_ID_PORT]                  /*  11 */ = "Ports",
    [VTSS_MODULE_ID_MAC]                   /*  12 */ = "MAC_Table",
    [VTSS_MODULE_ID_VLAN]                  /*  13 */ = "VLANs",
    [VTSS_MODULE_ID_QOS]                   /*  14 */ = "QoS",
    [VTSS_MODULE_ID_MIRROR]                /*  15 */ = "Mirroring",
    [VTSS_MODULE_ID_MISC]                  /*  16 */ = "Miscellaneous",
    [VTSS_MODULE_ID_ACL]                   /*  17 */ = "ACL",
    [VTSS_MODULE_ID_SYMREG]                /*  18 */ = "SymReg",
    [VTSS_MODULE_ID_AGGR]                  /*  19 */ = "Aggregation",
    [VTSS_MODULE_ID_RSTP]                  /*  20 */ = "Spanning_Tree",
    [VTSS_MODULE_ID_DOT1X]                 /*  21 */ = "802.1X",
    [VTSS_MODULE_ID_AFI]                   /*  22 */ = "AFI",
    [VTSS_MODULE_ID_PVLAN]                 /*  23 */ = "Private_VLANs",
    [VTSS_MODULE_ID_SYSTEM]                /*  24 */ = "System",
    [VTSS_MODULE_ID_CLI]                   /*  25 */ = "cli",
    [VTSS_MODULE_ID_WEB]                   /*  26 */ = "web",
    [VTSS_MODULE_ID_PING]                  /*  27 */ = "Diagnostics",
    [VTSS_MODULE_ID_FIRMWARE]              /*  28 */ = "Firmware",
    [VTSS_MODULE_ID_UNMGD]                 /*  29 */ = "unmgd",
    [VTSS_MODULE_ID_MSG_TEST]              /*  30 */ = "msg_test",
    [VTSS_MODULE_ID_LED]                   /*  31 */ = "led",
    [VTSS_MODULE_ID_CRITD]                 /*  32 */ = "critd",
    [VTSS_MODULE_ID_L2PROTO]               /*  33 */ = "l2proto",
    [VTSS_MODULE_ID_LLDP]                  /*  34 */ = "LLDP",
    [VTSS_MODULE_ID_LACP]                  /*  35 */ = "LACP",
    [VTSS_MODULE_ID_SNMP]                  /*  36 */ = "SNMP",
    [VTSS_MODULE_ID_SYSLOG]                /*  37 */ = "System_Log",
    [VTSS_MODULE_ID_IPMC_LIB]              /*  38 */ = "IPMC_LIB",
    [VTSS_MODULE_ID_PERF_MON_CONF_UNUSED]  /*  39 */ = "Perf_Mon_Conf_Unused",
    [VTSS_MODULE_ID_VTSS_LB]               /*  40 */ = "vtss_lb",
    [VTSS_MODULE_ID_INTERRUPT]             /*  41 */ = "interrupt",
    [VTSS_MODULE_ID_SYNCE]                 /*  42 */ = "SyncE",
    [VTSS_MODULE_ID_POE]                   /*  43 */ = "POE",
    [VTSS_MODULE_ID_MODULE_CONFIG]         /*  44 */ = "module_config",
    [VTSS_MODULE_ID_EPS_UNUSED]            /*  45 */ = "EPS_unused",
    [VTSS_MODULE_ID_MEP_UNUSED]            /*  46 */ = "MEP_Unused",
    [VTSS_MODULE_ID_HTTPS]                 /*  47 */ = "HTTPS",
    [VTSS_MODULE_ID_AUTH]                  /*  48 */ = "Authentication",
    [VTSS_MODULE_ID_SSH]                   /*  49 */ = "SSH",
    [VTSS_MODULE_ID_RADIUS]                /*  50 */ = "Radius",
    [VTSS_MODULE_ID_ACCESS_MGMT]           /*  51 */ = "Access_Management",
    [VTSS_MODULE_ID_UPNP]                  /*  52 */ = "UPnP",
    [VTSS_MODULE_ID_IP_DNS]                /*  53 */ = "DNS",
    [VTSS_MODULE_ID_DHCP_HELPER]           /*  54 */ = "DHCP_Helper",
    [VTSS_MODULE_ID_DHCP_RELAY]            /*  55 */ = "DHCP_Relay",
    [VTSS_MODULE_ID_DHCP_SNOOPING]         /*  56 */ = "DHCP_Snooping",
    [VTSS_MODULE_ID_NTP]                   /*  57 */ = "NTP",
    [VTSS_MODULE_ID_USERS]                 /*  58 */ = "Users",
    [VTSS_MODULE_ID_PRIV_LVL]              /*  59 */ = "Privilege_Levels",
    [VTSS_MODULE_ID_SECURITY]              /*  60 */ = "Security(access)",
    [VTSS_MODULE_ID_DEBUG]                 /*  61 */ = "Debug",
    [VTSS_MODULE_ID_EVC_UNUSED]            /*  62 */ = "EVC_Unused",
    [VTSS_MODULE_ID_ARP_INSPECTION]        /*  63 */ = "ARP_Inspection",
    [VTSS_MODULE_ID_IP_SOURCE_GUARD]       /*  64 */ = "IP_Source_Guard",
    [VTSS_MODULE_ID_PTP]                   /*  65 */ = "PTP",
    [VTSS_MODULE_ID_PSEC]                  /*  66 */ = "Port_Security",
    [VTSS_MODULE_ID_PSEC_LIMIT]            /*  67 */ = "PSec_Limit_Ctrl",
    [VTSS_MODULE_ID_MVR]                   /*  68 */ = "MVR",
    [VTSS_MODULE_ID_IPMC]                  /*  69 */ = "IPMC_Snooping",
    [VTSS_MODULE_ID_VOICE_VLAN]            /*  70 */ = "Voice_VLAN",
    [VTSS_MODULE_ID_LLDPMED]               /*  71 */ = "LLDP_MED",
    [VTSS_MODULE_ID_ERPS]                  /*  72 */ = "ERPS",
    [VTSS_MODULE_ID_ETH_LINK_OAM]          /*  73 */ = "ETH_LINK_OAM",
    [VTSS_MODULE_ID_EEE]                   /*  74 */ = "EEE",
    [VTSS_MODULE_ID_FAN]                   /*  75 */ = "Fan_Control",
    [VTSS_MODULE_ID_TOD]                   /*  76 */ = "Time_Of_Day_Control",
    [VTSS_MODULE_ID_LED_POW_REDUC]         /*  77 */ = "LED_Power_Reduced",
    [VTSS_MODULE_ID_THERMAL_PROTECT]       /*  78 */ = "Thermal_Protection",
    [VTSS_MODULE_ID_VCL]                   /*  79 */ = "VCL",
    [VTSS_MODULE_ID_UFDMA_AIL]             /*  80 */ = "uFDMA_AIL",
    [VTSS_MODULE_ID_UFDMA_CIL]             /*  81 */ = "uFDMA_CIL",
    [VTSS_MODULE_ID_THREAD_LOAD_MONITOR]   /*  82 */ = "Thread Load Monitor",
    [VTSS_MODULE_ID_SFLOW]                 /*  83 */ = "sFlow",
    [VTSS_MODULE_ID_Y1564_UNUSED]          /*  84 */ = "Y.1564_Unused",
    [VTSS_MODULE_ID_VLAN_TRANSLATION]      /*  85 */ = "VLAN_Translation",
    [VTSS_MODULE_ID_MRP]                   /*  86 */ = "MRP",
    [VTSS_MODULE_ID_MVRP]                  /*  87 */ = "MVRP",
    [VTSS_MODULE_ID_SYNCE_DPLL]            /*  88 */ = "SyncE DPLL",
    [VTSS_MODULE_ID_XXRP]                  /*  89 */ = "XXRP",
    [VTSS_MODULE_ID_IP_ROUTING]            /*  90 */ = "Ip_Routing",
    [VTSS_MODULE_ID_LOOP_PROTECT]          /*  91 */ = "Loop_Protect",
    [VTSS_MODULE_ID_RMON]                  /*  92 */ = "RMON",
    [VTSS_MODULE_ID_TIMER]                 /*  93 */ = "Timer",
    [VTSS_MODULE_ID_SECURITY_NETWORK]      /*  94 */ = "Security(network)",
    [VTSS_MODULE_ID_ICLI]                  /*  95 */ = "Industrial_CLI",
    [VTSS_MODULE_ID_REMOTE_TS_PHY_UNUSED]  /*  96 */ = "RemoteTs_PHY_Unused",
    [VTSS_MODULE_ID_DAYLIGHT_SAVING]       /*  97 */ = "Daylight_Saving",
    [VTSS_MODULE_ID_PHY]                   /*  98 */ = "PHY",
    [VTSS_MODULE_ID_CONSOLE]               /*  99 */ = "Console",
    [VTSS_MODULE_ID_GREEN_ETHERNET]        /* 100 */ = "Green_Ethernet",
    [VTSS_MODULE_ID_ICFG]                  /* 101 */ = "Industrial_Config",
    [VTSS_MODULE_ID_IP]                    /* 102 */ = "IP",
    [VTSS_MODULE_ID_IP_CHIP_UNUSED]        /* 103 */ = "IP_chip_Unused",
    [VTSS_MODULE_ID_DHCP_CLIENT]           /* 104 */ = "DHCP_Client",
    [VTSS_MODULE_ID_SNTP]                  /* 105 */ = "SNTP",
    [VTSS_MODULE_ID_MEBA]                  /* 106 */ = "MEBA",
    [VTSS_MODULE_ID_ZL_3034X_API]          /* 107 */ = "ZL_3034X_API",
    [VTSS_MODULE_ID_ZL_3034X_PDV]          /* 108 */ = "ZL_3034X_PDV",
    [VTSS_MODULE_ID_DHCP_SERVER]           /* 109 */ = "DHCP_SERVER",
    [VTSS_MODULE_ID_RFC2544_UNUSED]        /* 110 */ = "RFC2544",
    [VTSS_MODULE_ID_JSON_RPC]              /* 111 */ = "JSON_RPC",
    [VTSS_MODULE_ID_MACSEC]                /* 112 */ = "MACsec",
    [VTSS_MODULE_ID_BASICS]                /* 113 */ = "VTSS_BASICS",
    [VTSS_MODULE_ID_DHCP]                  /* 114 */ = "DHCP",
    [VTSS_MODULE_ID_DBGFS]                 /* 115 */ = "dbgfs",
    [VTSS_MODULE_ID_SNMP_DEMO]             /* 116 */ = "snmp_demo",
    [VTSS_MODULE_ID_PERF_MON_UNUSED]       /* 117 */ = "Performance_Monitor_Unused",
    [VTSS_MODULE_ID_POST]                  /* 118 */ = "POST",
    [VTSS_MODULE_ID_ZTP]                   /* 119 */ = "ZTP",
    [VTSS_MODULE_ID_RMIRROR]               /* 120 */ = "RMirror",
    [VTSS_MODULE_ID_DDMI]                  /* 121 */ = "DDMI",
    [VTSS_MODULE_ID_SUBJECT]               /* 122 */ = "Subject",
    [VTSS_MODULE_ID_UDLD]                  /* 123 */ = "UDLD",
    [VTSS_MODULE_ID_DIAG_UNUSED]           /* 124 */ = "Extended_Diagnostic_Unused",
    [VTSS_MODULE_ID_HQOS_UNUSED]           /* 125 */ = "HQoS_Unused",
    [VTSS_MODULE_ID_DHCP6C]                /* 126 */ = "DHCPv6_Client",
    [VTSS_MODULE_ID_MPLS_TP_UNUSED]        /* 127 */ = "MPLS_TP_Unused",
    [VTSS_MODULE_ID_TT_LOOP_UNUSED]        /* 128 */ = "TT_LOOP_Unused",
    [VTSS_MODULE_ID_JSON_RPC_NOTIFICATION] /* 127 */ = "JSON_RPC_Notification",
    [VTSS_MODULE_ID_FLEX_LINKS]            /* 130 */ = "FLEX_LINKS",
    [VTSS_MODULE_ID_MRP_UNUSED]            /* 131 */ = "MRP_Unused",
    [VTSS_MODULE_ID_JSON_API_EXPOSE]       /* 132 */ = "JSON_API_EXPOSE",
    [VTSS_MODULE_ID_FAST_CGI]              /* 133 */ = "FAST_CGI",
    [VTSS_MODULE_ID_NTP_LINUX]             /* 134 */ = "NTP_LINUX",
    [VTSS_MODULE_ID_EXPRESS_SETUP]         /* 135 */ = "ExpressSetup",
    [VTSS_MODULE_ID_ALARM]                 /* 136 */ = "Alarm",
    [VTSS_MODULE_ID_SR_UNUSED]             /* 137 */ = "Seamless_Redundancy_Unused",
    [VTSS_MODULE_ID_JSON_IPC]              /* 138 */ = "JSON_IPC",
    [VTSS_MODULE_ID_CIP]                   /* 139 */ = "CIP",
    [VTSS_MODULE_ID_ERRDISABLE]            /* 140 */ = "ErrDisable",
    [VTSS_MODULE_ID_SW_PUSH_BUTTON]        /* 141 */ = "Push_Button",
    [VTSS_MODULE_ID_OPTIONAL_MODULES]      /* 142 */ = "OptionalModules",
    [VTSS_MODULE_ID_FRR]                   /* 143 */ = "FRR",
    [VTSS_MODULE_ID_TRACEROUTE]            /* 144 */ = "Traceroute",
    [VTSS_MODULE_ID_DHCP6_RELAY]           /* 145 */ = "DHCPv6Relay",
    [VTSS_MODULE_ID_DHCP6_SNOOPING]        /* 146 */ = "DHCPv6_Snooping",
    [VTSS_MODULE_ID_IPV6_SOURCE_GUARD]     /* 147 */ = "IPv6_Source_Guard",
    [VTSS_MODULE_ID_CFM]                   /* 148 */ = "CFM",
    [VTSS_MODULE_ID_CPUPORT]               /* 149 */ = "CPU_Port",
    [VTSS_MODULE_ID_APS]                   /* 150 */ = "APS",
    [VTSS_MODULE_ID_TSN]                   /* 151 */ = "TSN",
    [VTSS_MODULE_ID_FRR_RIP]               /* 152 */ = "FRR_RIP",
    [VTSS_MODULE_ID_FRR_ROUTER]            /* 153 */ = "FRR_ROUTER",
    [VTSS_MODULE_ID_FRR_OSPF]              /* 154 */ = "FRR_OSPF",
    [VTSS_MODULE_ID_PSFP]                  /* 155 */ = "PSFP",
    [VTSS_MODULE_ID_FRR_OSPF6]             /* 156 */ = "FRR_OSPF6",
    [VTSS_MODULE_ID_FRER]                  /* 157 */ = "FRER",
    [VTSS_MODULE_ID_KR]                    /* 158 */ = "KR",
    [VTSS_MODULE_ID_IEC_MRP]               /* 159 */ = "IEC_MRP",
    [VTSS_MODULE_ID_CUST_0]                /* 160 */ = "CUST_0",
    [VTSS_MODULE_ID_CUST_1]                /* 161 */ = "CUST_1",
    [VTSS_MODULE_ID_CUST_2]                /* 162 */ = "CUST_2",
    [VTSS_MODULE_ID_CUST_3]                /* 163 */ = "CUST_3",
    [VTSS_MODULE_ID_CUST_4]                /* 164 */ = "CUST_4",
    [VTSS_MODULE_ID_REDBOX]                /* 165 */ = "RedBox",
    [VTSS_MODULE_ID_STREAM]                /* 166 */ = "Stream",

    /* Add new module name above it. And please don't use space
       in module name, use underscore instead. */
    [VTSS_MODULE_ID_NONE]                            = "none"
};

/* In most cases, a privilege level group consists of a single module
   (e.g. LACP, RSTP or QoS), but a few of them contains more than one.
   For example, the "security" privilege group consists of authentication,
   system access management, port security, TTPS, SSH, ARP inspection and
   IP source guard modules.
   The privilege level groups shares the same array of "vtss_module_names[]".
   And use "vtss_priv_lvl_groups_filter[]" to filter the privilege level group which
   we don't need them.
   For a new module, if the module needs an independent privilege level group
   then the filter value should be equal 0. If this module is included by other
   privilege level group then the filter value should be equal 1.
   Set filter value '0' means a privilege level group mapping to a single module
   Set filter value '1' means this module will be filetered in privilege groups */
const int vtss_priv_lvl_groups_filter[VTSS_MODULE_ID_NONE+1] =
{
    /*[VTSS_MODULE_ID_API_IO]                  0 */ 1,
    /*[VTSS_MODULE_ID_API_CI]                  1 */ 1,
    /*[VTSS_MODULE_ID_API_AI]                  2 */ 1,
    /*[VTSS_MODULE_ID_SPROUT]                  3 */ 1,
    /*[VTSS_MODULE_ID_MAIN]                    4 */ 1,
    /*[VTSS_MODULE_ID_TOPO]                    5 */ 1,
    /*[VTSS_MODULE_ID_CONF]                    6 */ 1,
    /*[VTSS_MODULE_ID_MSG]                     7 */ 1,
    /*[VTSS_MODULE_ID_PACKET]                  8 */ 1,
    /*[VTSS_MODULE_ID_TRACE]                   9 */ 1,
    /*[VTSS_MODULE_ID_IP_STACK_GLUE]          10 */ 1,
#ifdef VTSS_SW_OPTION_PORT
    /*[VTSS_MODULE_ID_PORT]                   11 */ 0,
#else
    /*[VTSS_MODULE_ID_PORT]                   11 */ 1,
#endif
#ifdef VTSS_SW_OPTION_MAC
    /*[VTSS_MODULE_ID_MAC]                    12 */ 0,
#else
    /*[VTSS_MODULE_ID_MAC]                    12 */ 1,
#endif
#ifdef VTSS_SW_OPTION_VLAN
    /*[VTSS_MODULE_ID_VLAN]                   13 */ 0,
#else
    /*[VTSS_MODULE_ID_VLAN]                   13 */ 1,
#endif
#ifdef VTSS_SW_OPTION_QOS
    /*[VTSS_MODULE_ID_QOS]                    14 */ 0,
#else
    /*[VTSS_MODULE_ID_QOS]                    14 */ 1,
#endif
#ifdef VTSS_SW_OPTION_MIRROR
#if defined(VTSS_SW_OPTION_RMIRROR)
    /*[VTSS_MODULE_ID_MIRROR]                 15 */ 1,
#else
    /*[VTSS_MODULE_ID_MIRROR]                 15 */ 0,
#endif
#else
    /*[VTSS_MODULE_ID_MIRROR]                 15 */ 1,
#endif
    /*[VTSS_MODULE_ID_MISC]                   16 */ 0,
    /*[VTSS_MODULE_ID_ACL]                    17 */ 1,
    /*[VTSS_MODULE_ID_SYMREG]                 18 */ 1,
#ifdef VTSS_SW_OPTION_AGGR
    /*[VTSS_MODULE_ID_AGGR]                   19 */ 0,
#else
    /*[VTSS_MODULE_ID_AGGR]                   19 */ 1,
#endif
#ifdef VTSS_SW_OPTION_MSTP
    /*[VTSS_MODULE_ID_RSTP]                   20 */ 0,
#else
    /*[VTSS_MODULE_ID_RSTP]                   20 */ 1,
#endif
    /*[VTSS_MODULE_ID_DOT1X]                  21 */ 1,
    /*[VTSS_MODULE_ID_AFI]                    22 */ 1,
#ifdef VTSS_SW_OPTION_PVLAN
    /*[VTSS_MODULE_ID_PVLAN]                  23 */ 0,
#else
    /*[VTSS_MODULE_ID_PVLAN]                  23 */ 1,
#endif
    /*[VTSS_MODULE_ID_SYSTEM]                 24 */ 0,
    /*[VTSS_MODULE_ID_CLI]                    25 */ 1,
    /*[VTSS_MODULE_ID_WEB]                    26 */ 1,
    /*[VTSS_MODULE_ID_PING]                   27 */ 0,
#ifdef VTSS_SW_OPTION_FIRMWARE
    /*[VTSS_MODULE_ID_FIRMWARE]               28 */ 0,
#else
    /*[VTSS_MODULE_ID_FIRMWARE]               28 */ 1,
#endif
    /*[VTSS_MODULE_ID_UNMGD]                  29 */ 1,
    /*[VTSS_MODULE_ID_MSG_TEST]               30 */ 1,
    /*[VTSS_MODULE_ID_LED]                    31 */ 1,
    /*[VTSS_MODULE_ID_CRITD]                  32 */ 1,
    /*[VTSS_MODULE_ID_L2PROTO]                33 */ 1,
#ifdef VTSS_SW_OPTION_LLDP
    /*[VTSS_MODULE_ID_LLDP]                   34 */ 0,
#else
    /*[VTSS_MODULE_ID_LLDP]                   34 */ 1,
#endif
#ifdef VTSS_SW_OPTION_LACP
    /*[VTSS_MODULE_ID_LACP]                   35 */ 0,
#else
    /*[VTSS_MODULE_ID_LACP]                   35 */ 1,
#endif
    /*[VTSS_MODULE_ID_SNMP]                   36 */ 1,
    /*[VTSS_MODULE_ID_SYSLOG]                 37 */ 1,
    /*[VTSS_MODULE_ID_IPMC_LIB]               38 */ 1,
    /*[VTSS_MODULE_ID_PERF_MON_CONF_UNUSED]   39 */ 1,
    /*[VTSS_MODULE_ID_VTSS_LB]                40 */ 1,
    /*[VTSS_MODULE_ID_INTERRUPT]              41 */ 1,
#ifdef VTSS_MODULE_ID_SYNCE
    /*[VTSS_MODULE_ID_SYNCE]                  42 */ 0,
#else
    /*[VTSS_MODULE_ID_SYNCE]                  42 */ 1,
#endif
#ifdef VTSS_SW_OPTION_POE
    /*[VTSS_MODULE_ID_POE]                    43 */ 0,
#else
    /*[VTSS_MODULE_ID_POE]                    43 */ 1,
#endif
    /*[VTSS_MODULE_ID_MODULE_CONFIG]          44 */ 1,
    /*[VTSS_MODULE_ID_EPS_UNUSED]             45 */ 1,
    /*[VTSS_MODULE_ID_MEP_UNUSED]             46 */ 1,
    /*[VTSS_MODULE_ID_HTTPS]                  47 */ 1,
    /*[VTSS_MODULE_ID_AUTH]                   48 */ 1,
    /*[VTSS_MODULE_ID_SSH]                    49 */ 1,
    /*[VTSS_MODULE_ID_RADIUS]                 50 */ 1,
    /*[VTSS_MODULE_ID_ACCESS_MGMT]            51 */ 1,
#ifdef VTSS_SW_OPTION_UPNP
    /*[VTSS_MODULE_ID_UPNP]                   52 */ 0,
#else
    /*[VTSS_MODULE_ID_UPNP]                   52 */ 1,
#endif
    /*[VTSS_MODULE_ID_IP_DNS]                 53 */ 1,
    /*[VTSS_MODULE_ID_DHCP_HELPER]            54 */ 1,
    /*[VTSS_MODULE_ID_DHCP_RELAY]             55 */ 1,
    /*[VTSS_MODULE_ID_DHCP_SNOOPING]          56 */ 1,
#ifdef VTSS_SW_OPTION_NTP
    /*[VTSS_MODULE_ID_NTP]                    57 */ 0,
#else
    /*[VTSS_MODULE_ID_NTP]                    57 */ 1,
#endif
    /*[VTSS_MODULE_ID_USERS]                  58 */ 1,
    /*[VTSS_MODULE_ID_PRIV_LVL]               59 */ 1,
    /*[VTSS_MODULE_ID_SECURITY]               60 */ 0,
    /*[VTSS_MODULE_ID_DEBUG]                  61 */ 0,
    /*[VTSS_MODULE_ID_EVC_UNUSED]             62 */ 1,
    /*[VTSS_MODULE_ID_ARP_INSPECTION]         63 */ 1,
    /*[VTSS_MODULE_ID_IP_SOURCE_GUARD]        64 */ 1,
#ifdef VTSS_SW_OPTION_PTP
    /*[VTSS_MODULE_ID_PTP]                    65 */ 0,
#else
    /*[VTSS_MODULE_ID_PTP]                    65 */ 1,
#endif
    /*[VTSS_MODULE_ID_PSEC]                   66 */ 1,
    /*[VTSS_MODULE_ID_PSEC_LIMIT]             67 */ 1,
#ifdef VTSS_SW_OPTION_MVR
    /*[VTSS_MODULE_ID_MVR]                    68 */ 0,
#else
    /*[VTSS_MODULE_ID_MVR]                    68 */ 1,
#endif
#ifdef VTSS_SW_OPTION_IPMC
    /*[VTSS_MODULE_ID_IPMC]                   69 */ 0,
#else
    /*[VTSS_MODULE_ID_IPMC]                   69 */ 1,
#endif
#ifdef VTSS_SW_OPTION_VOICE_VLAN
    /*[VTSS_MODULE_ID_VOICE_VLAN]             70 */ 0,
#else
    /*[VTSS_MODULE_ID_VOICE_VLAN]             70 */ 1,
#endif
    /*[VTSS_MODULE_ID_LLDPMED]                71 */ 1,
#ifdef VTSS_SW_OPTION_ERPS
    /*[VTSS_MODULE_ID_ERPS]                   72 */ 0,
#else
    /*[VTSS_MODULE_ID_ERPS]                   72 */ 1,
#endif
#ifdef VTSS_SW_OPTION_ETH_LINK_OAM
    /*[VTSS_MODULE_ID_ETH_LINK_OAM]           73 */ 0,
#else
    /*[VTSS_MODULE_ID_ETH_LINK_OAM]           73 */ 1,
#endif
    /*[VTSS_MODULE_ID_EEE]                    74 */ 1,
    /*[VTSS_MODULE_ID_FAN]                    75 */ 1,
    /*[VTSS_MODULE_ID_TOD]                    76 */ 1,
    /*[VTSS_MODULE_ID_LED_POW_REDUC]          77 */ 1,
    /*[VTSS_MODULE_ID_THERMAL_PROTECT]        78 */ 1,
#ifdef VTSS_SW_OPTION_VCL
    /*[VTSS_MODULE_ID_VCL]                    79 */ 0,
#else
    /*[VTSS_MODULE_ID_VCL]                    79 */ 1,
#endif
#if defined(MSCC_BRSDK)
    /*[VTSS_MODULE_ID_UFDMA_AIL]              80 */ 0,
    /*[VTSS_MODULE_ID_UFDMA_CIL]              81 */ 0,
#else
    /*[VTSS_MODULE_ID_UFDMA_AIL]              80 */ 1,
    /*[VTSS_MODULE_ID_UFDMA_CIL]              81 */ 1,
#endif
    /*[VTSS_MODULE_ID_THREAD_LOAD_MONITOR]    82 */ 1,
#ifdef VTSS_SW_OPTION_SFLOW
    /*[VTSS_MODULE_ID_SFLOW]                  83 */ 0,
#else
    /*[VTSS_MODULE_ID_SFLOW]                  83 */ 1,
#endif
    /*[VTSS_MODULE_ID_Y1564_UNUSED]           84 */ 1,
#ifdef VTSS_SW_OPTION_VLAN_TRANSLATION
    /*[VTSS_MODULE_ID_VLAN_TRANSLATION]       85 */ 0,
#else
    /*[VTSS_MODULE_ID_VLAN_TRANSLATION]       85 */ 1,
#endif
#ifdef VTSS_SW_OPTION_MRP
    /*[VTSS_MODULE_ID_MRP]                    86 */ 0,
#else
    /*[VTSS_MODULE_ID_MRP]                    86 */ 1,
#endif
    /*[VTSS_MODULE_ID_MVRP]                   87 */ 1,
    /*[VTSS_MODULE_ID_SYNCE_DPLL]             88 */ 1,
#ifdef VTSS_SW_OPTION_XXRP
    /*[VTSS_MODULE_ID_XXRP]                   89 */ 0,
#else
    /*[VTSS_MODULE_ID_XXRP]                   89 */ 1,
#endif
#ifdef VTSS_SW_OPTION_IP_ROUTING
    /*[VTSS_MODULE_ID_IP_ROUTING]             90 */ 0,
#else
    /*[VTSS_MODULE_ID_IP_ROUTING]             90 */ 1,
#endif
#ifdef VTSS_SW_OPTION_LOOP_PROTECTION
    /*[VTSS_MODULE_ID_LOOP_PROTECT]           91 */ 0,
#else
    /*[VTSS_MODULE_ID_LOOP_PROTECT]           91 */ 1,
#endif
    /*[VTSS_MODULE_ID_RMON]                   92 */ 1,
    /*[VTSS_MODULE_ID_TIMER]                  93 */ 1,
    /*[VTSS_MODULE_ID_SECURITY_NETWORK]       94 */ 0,
    /*[VTSS_MODULE_ID_ICLI]                   95 */ 1,
    /*[VTSS_MODULE_ID_REMOTE_TS_PHY_UNUSED]   96 */ 1,
    /*[VTSS_MODULE_ID_DAYLIGHT_SAVING]        97 */ 1,
    /*[VTSS_MODULE_ID_PHY]                    98 */ 1,
#ifdef VTSS_SW_OPTION_CONSOLE
    /*[VTSS_MODULE_ID_CONSOLE]                99 */ 0,
#else
    /*[VTSS_MODULE_ID_CONSOLE]                99 */ 1,
#endif
#if defined(VTSS_SW_OPTION_EEE) || defined(VTSS_SW_OPTION_FAN) || defined(VTSS_SW_OPTION_LED_POW_REDUC)
    /*[VTSS_MODULE_ID_GREEN_ETHERNET]        100 */ 0,
#else
    /*[VTSS_MODULE_ID_GREEN_ETHERNET]        100 */ 1,
#endif
    /*[VTSS_MODULE_ID_ICFG]                  101 */ 1,
#if defined(VTSS_SW_OPTION_IP)
    /*[VTSS_MODULE_ID_IP]                    102 */ 0,
#else
    /*[VTSS_MODULE_ID_IP]                    102 */ 1,
#endif
    /*[VTSS_MODULE_ID_IP_CHIP_UNUSED]        103 */ 1,
    /*[VTSS_MODULE_ID_DHCP_CLIENT]           104 */ 1,
#if defined(VTSS_SW_OPTION_SNTP)
    /*[VTSS_MODULE_ID_SNTP]                  105 */ 0,
#else
    /*[VTSS_MODULE_ID_SNTP]                  105 */ 1,
#endif
    /*[VTSS_MODULE_ID_MEBA]                  106 */ 1,
    /*[VTSS_MODULE_ID_ZL_3034X_API]          107 */ 1,
    /*[VTSS_MODULE_ID_ZL_3034X_PDV]          108 */ 1,
    /*[VTSS_MODULE_ID_DHCP_SERVER]           109 */ 1,
    /*[VTSS_MODULE_ID_RFC2544_UNUSED]        110 */ 1,
    /*[VTSS_MODULE_ID_JSON_RPC]              111 */ 1,
    /*[VTSS_MODULE_ID_MACSEC]                112 */ 1,
    /*[VTSS_MODULE_ID_BASICS]                113 */ 1,
#if defined(VTSS_SW_OPTION_DHCP_HELPER)
    /*[VTSS_MODULE_ID_DHCP]                  114 */ 0,
#else
    /*[VTSS_MODULE_ID_DHCP]                  114 */ 1,
#endif
    /*[VTSS_MODULE_ID_DBGFS]                 115 */ 1,
    /*[VTSS_MODULE_ID_SNMP_DEMO]             116 */ 1,
    /*[VTSS_MODULE_ID_PERF_MON_UNUSED]       117 */ 1,
    /*[VTSS_MODULE_ID_POST]                  118 */ 1,
    /*[VTSS_MODULE_ID_ZTP]                   119 */ 1,
#if defined(VTSS_SW_OPTION_RMIRROR)
    /*[VTSS_MODULE_ID_RMIRROR]               120 */ 0,
#else
    /*[VTSS_MODULE_ID_RMIRROR]               120 */ 1,
#endif
#if defined(VTSS_SW_OPTION_DDMI)
    /*[VTSS_MODULE_ID_DDMI]                  121 */ 0,
#else
    /*[VTSS_MODULE_ID_DDMI]                  121 */ 1,
#endif
    /*[VTSS_MODULE_ID_SUBJECT]               122 */ 1,
#if defined(VTSS_SW_OPTION_UDLD)
    /*[VTSS_MODULE_ID_UDLD]                  123 */ 0,
#else
    /*[VTSS_MODULE_ID_UDLD]                  123 */ 1,
#endif
    /*[VTSS_MODULE_ID_DIAG_UNUSED]           124 */ 1,
    /*[VTSS_MODULE_ID_HQOS_UNUSED]           125 */ 1,
#ifdef VTSS_SW_OPTION_DHCP6_CLIENT
    /*[VTSS_MODULE_ID_DHCP6C]                126 */ 0,
#else
    /*[VTSS_MODULE_ID_DHCP6C]                126 */ 1,
#endif
    /*[VTSS_MODULE_ID_MPLS_TP_UNUSED]        127 */ 1,
    /*[VTSS_MODULE_ID_TT_LOOP_UNUSED]        128 */ 1,
    /*[VTSS_MODULE_ID_JSON_RPC_NOTIFICATION] 129 */ 1,
    /*[VTSS_MODULE_ID_FLEX_LINKS]            130 */ 1,
    /*[VTSS_MODULE_ID_MRP_UNUSED]            131 */ 1,
    /*[VTSS_MODULE_ID_JSON_API_EXPOSE]       132 */ 1,
    /*[VTSS_MODULE_ID_FCGI_SERVER]           133 */ 1,
    /*[VTSS_MODULE_ID_NTP_LINUX]             134 */ 1,
    /*[VTSS_MODULE_ID_EXPRESS_SETUP]         135 */ 1,
#ifdef VTSS_SW_OPTION_ALARM
    /*[VTSS_MODULE_ID_ALARM]                 136 */ 0,
#else
    /*[VTSS_MODULE_ID_ALARM]                 136 */ 1,
#endif
    /*[VTSS_MODULE_ID_SR_UNUSED]             137 */ 1,
    /*[VTSS_MODULE_ID_JSON_IPC]              138 */ 1,
    /*[VTSS_MODULE_ID_CIP]                   139 */ 1,
    /*[VTSS_MODULE_ID_ERRDISABLE]            140 */ 1,
    /*[VTSS_MODULE_ID_SW_PUSH_BUTTON]        141 */ 1,
    /*[VTSS_MODULE_ID_OPTIONAL_MODULES]      142 */ 1,
    /*[VTSS_MODULE_ID_FRR]                   143 */ 1,
    /*[VTSS_MODULE_ID_TRACEROUTE]            144 */ 1,
    /*[VTSS_MODULE_ID_DHCP6_RELAY]           145 */ 1,
    /*[VTSS_MODULE_ID_DHCP6_SNOOPING]        146 */ 1,
    /*[VTSS_MODULE_ID_IPV6_SOURCE_GUARD]     147 */ 1,
#ifdef VTSS_SW_OPTION_CFM
    /*[VTSS_MODULE_ID_CFM]                   148 */ 0,
#else
    /*[VTSS_MODULE_ID_CFM]                   148 */ 1,
#endif
    /*[VTSS_MODULE_ID_CPUPORT]               149 */ 1,
#ifdef VTSS_SW_OPTION_APS
    /*[VTSS_MODULE_ID_APS]                   150 */ 0,
#else
    /*[VTSS_MODULE_ID_APS]                   150 */ 1,
#endif
    /*[VTSS_MODULE_ID_TSN]                   151 */ 1,
    /*[VTSS_MODULE_ID_FRR_RIP]               152 */ 1,
    /*[VTSS_MODULE_ID_FRR_ROUTER]            153 */ 1,
    /*[VTSS_MODULE_ID_FRR_OSPF]              154 */ 1,
    /*[VTSS_MODULE_ID_PSFP]                  155 */ 1,
    /*[VTSS_MODULE_ID_FRR_OSPF6]             156 */ 1,
    /*[VTSS_MODULE_ID_FRER]                  157 */ 1,
    /*[VTSS_MODULE_ID_KR]                    158 */ 1,
    /*[VTSS_MODULE_ID_IEC_MRP]               159 */ 1,
    /*[VTSS_MODULE_ID_CUST_0]                160 */ 1,
    /*[VTSS_MODULE_ID_CUST_1]                161 */ 1,
    /*[VTSS_MODULE_ID_CUST_2]                162 */ 1,
    /*[VTSS_MODULE_ID_CUST_3]                163 */ 1,
    /*[VTSS_MODULE_ID_CUST_4]                164 */ 1,
    /*[VTSS_MODULE_ID_REDBOX]                165 */ 1,
    /*[VTSS_MODULE_ID_STREAM]                166 */ 1,

    /* Hint:
     * For a new module, if the module needs an independent privilege level group
     * then the filter value should be equal 0. If this module is included by other
     * privilege level group then the filter value should be equal 1.
     * Set filter value '0' means a privilege level group mapping to a single module
     * Set filter value '1' means this module will be filtered in privilege groups
     **/

    // LAST ELEMENT ////////////////////////////////////////////////////////////
    /*[VTSS_MODULE_ID_NONE] */                 0
};

