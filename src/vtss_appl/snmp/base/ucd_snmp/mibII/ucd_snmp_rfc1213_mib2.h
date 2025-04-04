/********************************************************************
       Copyright 1989, 1991, 1992 by Carnegie Mellon University

                          Derivative Work -
Copyright 1996, 1998, 1999, 2000 The Regents of the University of California

                         All Rights Reserved

Permission to use, copy, modify and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appears in all copies and
that both that copyright notice and this permission notice appear in
supporting documentation, and that the name of CMU and The Regents of
the University of California not be used in advertising or publicity
pertaining to distribution of the software without specific written
permission.

CMU AND THE REGENTS OF THE UNIVERSITY OF CALIFORNIA DISCLAIM ALL
WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL CMU OR
THE REGENTS OF THE UNIVERSITY OF CALIFORNIA BE LIABLE FOR ANY SPECIAL,
INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
FROM THE LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*********************************************************************/
/* This file was generated by mib2c and is intended for use as a mib module
   for the ucd-snmp snmpd agent. */

#ifndef UCD_SNMP_RFC1213_MIB2_H
#define UCD_SNMP_RFC1213_MIB2_H
#ifdef __cplusplus
extern "C" {
#endif

/*
 * function declarations
 */
#if RFC1213_SUPPORTED_SYSTEM
/* system ----------------------------------------------------------*/
void            ucd_snmp_init_mib2_system(void);
#endif /* RFC1213_SUPPORTED_SYSTEM */

#if RFC1213_SUPPORTED_ORTABLE
/* sysORTable ----------------------------------------------------------*/
#define SYS_ORTABLE_REGISTERED_OK              0
#define SYS_ORTABLE_REGISTRATION_FAILED       -1
#define SYS_ORTABLE_UNREGISTERED_OK            0
#define SYS_ORTABLE_NO_SUCH_REGISTRATION      -1

#define REGISTER_SYSOR_ENTRY(theoid, descr)                      \
  (void)register_sysORTable(theoid, sizeof(theoid)/sizeof(oid), descr);
#define REGISTER_SYSOR_TABLE(theoid, len, descr)                      \
  (void)register_sysORTable(theoid, len, descr);
#define UNREGISTER_SYSOR_TABLE(theoid, len)                      \
  (void)unregister_sysORTable(theoid, len);

int register_sysORTable_sess(oid *oidin, size_t oidlen, const char *descr, struct snmp_session *ss);
int register_sysORTable(oid *oidin, size_t oidlen, const char *descr);
int unregister_sysORTable_sess(oid *oidin, size_t oidlen, struct snmp_session *ss);
int unregister_sysORTable(oid *oidin, size_t oidlen);
//static void unregister_sysORTable_by_session(struct snmp_session *ss);
void ucd_snmp_get_sysOR_LastChange(struct timeval *sysOR_LastChangeTimeval);

struct ifnet *get_IpInterface(int if_num);
FindVarMethod   var_sysORTable;
#endif /* RFC1213_SUPPORTED_ORTABLE */

#if RFC1213_SUPPORTED_INTERFACES
void            ucd_snmp_init_mib2_interfaces(void);
FindVarMethod   var_interfaces;
FindVarMethod   var_ifTable;
#endif /* RFC1213_SUPPORTED_INTERFACES */

#if RFC1213_SUPPORTED_IP
/* ip ----------------------------------------------------------*/
void            ucd_snmp_init_mib2_ip(void);
FindVarMethod   var_ip;
/* FindVarMethod   var_ipRouteTable; */
FindVarMethod   var_ipNetToMediaTable;
FindVarMethod   var_ipAddrTable;
WriteMethod     write_ipForwarding;
WriteMethod     write_ipDefaultTTL;
WriteMethod     write_ipRouteDest;
WriteMethod     write_ipRouteIfIndex;
WriteMethod     write_ipRouteMetric1;
WriteMethod     write_ipRouteMetric2;
WriteMethod     write_ipRouteMetric3;
WriteMethod     write_ipRouteMetric4;
WriteMethod     write_ipRouteNextHop;
WriteMethod     write_ipRouteType;
WriteMethod     write_ipRouteAge;
WriteMethod     write_ipRouteMask;
WriteMethod     write_ipRouteMetric5;
WriteMethod     write_ipNetToMediaIfIndex;
WriteMethod     write_ipNetToMediaPhysAddress;
WriteMethod     write_ipNetToMediaNetAddress;
WriteMethod     write_ipNetToMediaType;
#endif /* RFC1213_SUPPORTED_IP */

#if RFC1213_SUPPORTED_ICMP
/* icmp ----------------------------------------------------------*/
void            ucd_snmp_init_mib2_icmp(void);
FindVarMethod   var_icmp;
#endif /* RFC1213_SUPPORTED_ICMP */

#if RFC1213_SUPPORTED_TCP
/* tcp ----------------------------------------------------------*/
void            ucd_snmp_init_mib2_tcp(void);
FindVarMethod   var_tcp;
FindVarMethod   var_tcpConnTable;
/* FindVarMethod   var_ipv6TcpConnTable; */
WriteMethod     write_tcpConnState;
/* WriteMethod     write_ipv6TcpConnState; */
#endif /* RFC1213_SUPPORTED_TCP */

#if RFC1213_SUPPORTED_UDP
/* udp ----------------------------------------------------------*/
void            ucd_snmp_init_mib2_udp(void);
FindVarMethod   var_udp;
/* FindVarMethod   var_ipv6UdpTable; */
FindVarMethod   var_udpTable;
#endif /* RFC1213_SUPPORTED_UDP */

#if RFC1213_SUPPORTED_SNMP
/* snmp ----------------------------------------------------------*/
void            ucd_snmp_init_mib2_snmp(void);
FindVarMethod   var_snmp;
WriteMethod     write_snmpEnableAuthenTraps;
#endif /* RFC1213_SUPPORTED_SNMP */

#ifdef __cplusplus
}
#endif
#endif /* UCD_SNMP_RFC1213_MIB2_H */
