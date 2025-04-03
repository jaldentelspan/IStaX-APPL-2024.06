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

/*
******************************************************************************

    Revision history
    > CP.Wang, 2012/09/12 13:25
        - create

******************************************************************************
*/
#ifndef __ICLI_PORTING_HELP_H__
#define __ICLI_PORTING_HELP_H__
//****************************************************************************

/*
******************************************************************************

    HELP String
    Please sort the insertion by ICLI_HELP_'X'

******************************************************************************
*/

/* A */
#define ICLI_HELP_AGGREGATION       "Aggregation mode"

/* B */
#define ICLI_HELP_BANNER            "Define a banner"

/* C */
#define ICLI_HELP_CLEAR             "Reset functions"
#define ICLI_HELP_CLEAR_IP          "IP protocol"
#define ICLI_HELP_CLEAR_IP_DHCP     "Delete items from the DHCP database"

/* D */
#define ICLI_HELP_DEBUG             "Debugging functions"
#define ICLI_HELP_DEFAULT           "Set a command to its defaults"
#define ICLI_HELP_DEFAULT_ROUTE     "Establish default route"
#define ICLI_HELP_DHCP              "Dynamic Host Configuration Protocol"
#define ICLI_HELP_DHCPV6            "Dynamic Host Configuration Protocol V6"
#define ICLI_HELP_DHCP_BINDING      "DHCP address bindings"
#define ICLI_HELP_DHCP_POOL_NAME    "Pool name in 32 characters"
#define ICLI_HELP_DHCP_RELAY        "DHCP relay agent configuration"
#define ICLI_HELP_DNS               "Domain Name System"
#define ICLI_HELP_DO                "To run exec commands in the configuration mode"
#define ICLI_HELP_DO_LINE           "Exec Command"
#define ICLI_HELP_DOMAIN_NAME       "A valid name consist of a sequence of domain labels separated by '.', each domain label starting and ending with an alphanumeric character and possibly also containing '-' characters. The length of a domain label must be 63 characters or less."

/* E */
#define ICLI_HELP_EDITING           "Enable command line editing"
#define ICLI_HELP_END               "Go back to EXEC mode"
#define ICLI_HELP_EXEC_TIMEOUT      "Set the EXEC timeout"
#define ICLI_HELP_EXEC_MIN          "Timeout in minutes"
#define ICLI_HELP_EXEC_SEC          "Timeout in seconds"

/* F */

/* G */

/* H */
#define ICLI_HELP_HISTORY           "Control the command history function"
#define ICLI_HELP_HISTORY_SIZE      "Set history buffer size"
#define ICLI_HELP_HISTORY_NUM       "Number of history commands, 0 means disable"
#define ICLI_HELP_HTTP              "Hypertext Transfer Protocol "

/* I */
#define ICLI_HELP_IGMP              "Internet Group Management Protocol"
#define ICLI_HELP_INTERFACE         "Select an interface to configure"
#define ICLI_HELP_INTF_COMPAT       "Interface compatibility"
#define ICLI_HELP_INTF_PRI          "Interface CoS priority"
#define ICLI_HELP_INTF_RV           "Robustness Variable"
#define ICLI_HELP_INTF_QI           "Query Interval in seconds"
#define ICLI_HELP_INTF_QRI          "Query Response Interval in tenths of seconds"
#define ICLI_HELP_INTF_LMQI         "Last Member Query Interval in tenths of seconds"
#define ICLI_HELP_INTF_URI          "Unsolicited Report Interval in seconds"
#define ICLI_HELP_IMD_LEAVE         "Immediate leave configuration"
#define ICLI_HELP_IP                "Interface Internet Protocol configuration commands"
#define ICLI_HELP_IP_DHCP           "Configure DHCP server parameters"
#define ICLI_HELP_IP_DHCP6_GENERIC  "Configure DHCPv6 related parameters"
#define ICLI_HELP_IP4_ADRS          "Configure the IPv4 address of an interface"
#define ICLI_HELP_IP6_ADRS          "Configure the IPv6 address of an interface"
#define ICLI_HELP_IPMC              "IPv4/IPv6 multicast configuration"
#define ICLI_HELP_IPMC_PROFILE      "IPMC profile configuration"
#define ICLI_HELP_IPMC_RANGE        "A range of IPv4/IPv6 multicast addresses for the profile"
#define ICLI_HELP_IPMC_VID          "VLAN identifier (VID)"
#define ICLI_HELP_IPV6              "IPv6 configuration commands"

/* J */

/* K */

/* L */
#define ICLI_HELP_LENGTH            "Set number of lines on a screen"
#define ICLI_HELP_LENGTH_NUM        "Number of lines on screen (0 for no pausing)"
#define ICLI_HELP_LINE              "Configure a terminal line"
#define ICLI_HELP_LOCATION          "Enter terminal location description"

/* M */
#define ICLI_HELP_MLD               "Multicast Listener Discovery"
#define ICLI_HELP_MOTD              "Set Message of the Day banner"
#define ICLI_HELP_MROUTER           "Multicast router port configuration"
#define ICLI_HELP_MVR               "Multicast VLAN Registration configuration"
#define ICLI_HELP_MVR_NAME          "MVR multicast name"
#define ICLI_HELP_MVR_VLAN          "MVR multicast VLAN"

/* N */
#define ICLI_HELP_NO                "Negate a command or set its defaults"

/* O */
#define ICLI_HELP_OSPF              "Open Shortest Path First (OSPF)"
#define ICLI_HELP_OSPF6             "Open Shortest Path First for IPv6 (OSPFv3)"

/* P */
#define ICLI_HELP_PING              "Send ICMP echo messages"
#define ICLI_HELP_PORT_ID           "Port ID in the format of switch-id/port-id, e.g. 1/5"
#define ICLI_HELP_PORT_LIST         "List of port ID, e.g. 1/1,3-5;2/2-4,6"
#define ICLI_HELP_PORT_TYPE         "Port type in Fast, Gigabit or 10 Gigabit Ethernet"
#define ICLI_HELP_PORT_TYPE_LIST    "List of port type and port ID, ex, Fast 1/1 Gigabit 2/3-5 Gigabit 3/2-4 10 Gigabit 4/6"
#define ICLI_HELP_PRIVILEGE         "Change privilege level for line"
#define ICLI_HELP_PRIVILEGE_LEVEL   "Assign default privilege level for line"
#define ICLI_HELP_PROFILE_NAME      "Profile name in 16 characters"
#define ICLI_HELP_OSPF_PROCESS_ID   "OSPF process ID"
#define ICLI_HELP_OSPF6_PROCESS_ID  "OSPFv3 process ID"

/* Q */

/* R */
#define ICLI_HELP_RIP               "Routing Information Protocol (RIP)"
#define ICLI_HELP_ROUTE             "Configure static routes"
#define ICLI_HELP_ROUTE4_NH         "Next hop router's IPv4 address"
#define ICLI_HELP_ROUTE6_NH         "Next hop router's IPv6 address"
#define ICLI_HELP_ROUTER            "Routing process"

/* S */
#define ICLI_HELP_SERVICE           "Modify use of network based services"
#define ICLI_HELP_SHOW              "Show running system information"
#define ICLI_HELP_SHOW_INTERFACE    "Interface status and configuration"
#define ICLI_HELP_SHOW_IP           "IP information"
#define ICLI_HELP_SHOW_IP_DHCP      "Show items in the DHCP database"
#define ICLI_HELP_SNMP              "Set SNMP server's configurations"
#define ICLI_HELP_SNMP_HOST         "Set SNMP host's configurations"
#define ICLI_HELP_SNMP_HOST_NAME    "Name of the host configuration"
#define ICLI_HELP_STATELESS         "Obtain IPv6 address using auto-configuration"
#define ICLI_HELP_STATISTICS        "Traffic statistics"
#define ICLI_HELP_STATUS            "Status"
#define ICLI_HELP_STP               "Spanning Tree protocol"
#define ICLI_HELP_SSH               "Secure Shell"
#define ICLI_HELP_SWITCHPORT        "Set switching mode characteristics"
#define ICLI_HELP_SWITCH            "Switch"
#define ICLI_HELP_SWITCH_ID         "Switch ID"
#define ICLI_HELP_SWITCH_LIST       "List of switch ID, ex, 1,3-5,6"
#define ICLI_HELP_STP_BRIDGE_PRIORITY "Represents the STP bridge priority. Supported values are 0/4096/8192/12288/16384/20480/24576/28672/32768/36864/40960/45056/49152/53248/57344/61440 i.e divisible by 4096. Default value is 32768."
#define ICLI_HELP_STP_PORT_PRIORITY  "Represents the priority field for the port identifier. Port priority must be divisible by 16,supported values are 0/16/32/48/64/80/96/112/128/144/160/176/192/208/224/240. Default value is 128."

/* T */
#define ICLI_HELP_TRACEROUTE        "Send IP Traceroute messages"

/* U */
#define ICLI_HELP_URL               "Uniform Resource Locator. It is a specific character string that constitutes a reference to a resource. Syntax: <protocol>://[<username>[:<password>]@]<host>[:<port>][/<path>] If the following special characters: space !\"#$%&'()*+,/:;<=>?@[\\]^`{|}~ need to be contained in the input URL string, they should be percent-encoded. A valid file name is a text string drawn from alphabet (A-Za-z), digits (0-9), dot (.), hyphen (-), under score (_). The maximum length is 63 and hyphen must not be first character. The file name content that only contains '.' is not allowed."
#define ICLI_HELP_URL_FILE          "Uniform Resource Locator. It is a specific character string that constitutes a reference to a resource. Syntax: <protocol>://[<username>[:<password>]@]<host>[:<port>][/<path>]/<file_name> If the following special characters: space !\"#$%&'()*+,/:;<=>?@[\\]^`{|}~ need to be contained in the input URL string, they should be percent-encoded. A valid file name is a text string drawn from alphabet (A-Za-z), digits (0-9), dot (.), hyphen (-), under score (_). The maximum length is 63 and hyphen must not be first character. The file name content that only contains '.' is not allowed."

/* V */
#define ICLI_HELP_VLAN_LIST         "List of VLAN ID, e.g. 1,3-5,7"
#define ICLI_HELP_VLINK_LIST        "List of VLINK ID, e.g. 1,3-5,7"

/* W */
#define ICLI_HELP_WIDTH             "Set width of the display terminal"
#define ICLI_HELP_WIDTH_NUM         "Number of characters on a screen line (0 for unlimited width)"

/* X */

/* Y */

/* Z */


//****************************************************************************
#endif //__ICLI_PORTING_HELP_H__

