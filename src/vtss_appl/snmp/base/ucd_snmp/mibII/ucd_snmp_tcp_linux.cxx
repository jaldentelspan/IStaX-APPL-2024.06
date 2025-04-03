/* -----------------------------------------------------------------------------

 Portions of this software may have been derived from the UCD-SNMP
 project,  <http://www.net-snmp.org/>  from the University of
 California at Davis, which was originally based on the Carnegie Mellon
 University SNMP implementation.  Portions of this software are therefore
 covered by the appropriate copyright disclaimers included herein.

 The release used was version 5.0.11.2 of June 2008.  "net-snmp-5.0.11.2"

 -------------------------------------------------------------------------------
*/

#include <main.h>

#include <vtss_module_id.h>

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_SNMP
#include "vtss_os_wrapper_snmp.h"
#include "ucd_snmp_os_wrapper_tcp.h"

#define TCP_STATS_LINE  "Tcp: %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu"
#define TCP_STATS_PREFIX_LEN    5

u_long linux_tcp_stat_get(tcp_stat_scalar_t *tcp_stat)
{
    FILE *in = fopen ("/proc/net/snmp", "r");
    char line [1024];

    if (!in) {
        return -1;
    }

    while (line == fgets (line, sizeof(line), in)) {
        if (!strncmp( line, TCP_STATS_LINE, TCP_STATS_PREFIX_LEN )) {
            sscanf ( line, TCP_STATS_LINE,
                                &tcp_stat->tcpRtoAlgorithm,
                                &tcp_stat->tcpRtoMin,
                                &tcp_stat->tcpRtoMax,
                                &tcp_stat->tcpMaxConn,
                                &tcp_stat->tcpActiveOpens,
                                &tcp_stat->tcpPassiveOpens,
                                &tcp_stat->tcpAttemptFails,
                                &tcp_stat->tcpEstabResets,
                                &tcp_stat->tcpCurrEstab,
                                &tcp_stat->tcpInSegs,
                                &tcp_stat->tcpOutSegs,
                                &tcp_stat->tcpRetransSegs,
                                &tcp_stat->tcpInErrs,
                                &tcp_stat->tcpOutRsts);
        }

    }

    fclose (in);
    return 0;
}

static int cmp_tcp_entry(tcp_connTable_entry_t *key, tcp_connTable_entry_t *data) {
    int rc = -1;
    if ((rc = memcmp(key->tcpConnectionLocalAddress, data->tcpConnectionLocalAddress, sizeof(data->tcpConnectionLocalAddress))) != 0) {
        return rc;
    }

    if (key->tcpConnectionLocalPort > data->tcpConnectionLocalPort) {
        return 1;
    } else if (key->tcpConnectionLocalPort < data->tcpConnectionLocalPort){
        return -1;
    }

    if ((rc = memcmp(key->tcpConnectionRemAddress, data->tcpConnectionRemAddress, sizeof(data->tcpConnectionRemAddress))) != 0) {
        return rc;
    }

    if (key->tcpConnectionRemPort > data->tcpConnectionRemPort) {
        return 1;
    } else if (key->tcpConnectionRemPort < data->tcpConnectionRemPort){
        return -1;
    }

    return 0;
}

static int  tcp_conn_entry_get_next(tcp_connTable_entry_t *key)
{
    FILE *in;
    char line [256];
    unsigned int state;
    tcp_connTable_entry_t buf, entry;
    int found = 0;

    memset (&buf, 0xff, sizeof(buf));
    if (! (in = fopen ("/proc/net/tcp", "r"))) {
        snmp_log(LOG_ERR, "snmpd: cannot open /proc/net/tcp ...\n");
        return -1;
    }

    T_D("key localaddr = %d.%d.%d.%d", key->tcpConnectionLocalAddress[0], key->tcpConnectionLocalAddress[1], key->tcpConnectionLocalAddress[2],
            key->tcpConnectionLocalAddress[3]);
    T_D("key lport = %lu", key->tcpConnectionLocalPort);
    T_D("key remaddr = %d.%d.%d.%d", key->tcpConnectionRemAddress[0], key->tcpConnectionRemAddress[1], key->tcpConnectionRemAddress[2],
            key->tcpConnectionRemAddress[3]);
    T_D("key rport = %lu", key->tcpConnectionRemPort);

    while (line == fgets (line, sizeof(line), in)) {
        uint32_t localAddress, remAddress;
        if (5 != sscanf (line, "%*d: %x:%x %x:%x %x",
                         &localAddress, &entry.tcpConnectionLocalPort,
                         &remAddress, &entry.tcpConnectionRemPort, &state)) {
          continue;
        }
        
        entry.tcpConnectionLocalAddress[0] = (localAddress & 0xFF000000)>>24;
        entry.tcpConnectionLocalAddress[1] = (localAddress & 0xFF0000)>>16;
        entry.tcpConnectionLocalAddress[2] = (localAddress & 0xFF00)>>8;
        entry.tcpConnectionLocalAddress[3] = (localAddress & 0xFF);
        entry.tcpConnectionRemAddress[0] = (remAddress & 0xFF000000)>>24;
        entry.tcpConnectionRemAddress[1] = (remAddress & 0xFF0000)>>16;
        entry.tcpConnectionRemAddress[2] = (remAddress & 0xFF00)>>8;
        entry.tcpConnectionRemAddress[3] = (remAddress & 0xFF);
        T_D("entry localaddr = %d.%d.%d.%d", entry.tcpConnectionLocalAddress[0], entry.tcpConnectionLocalAddress[1], entry.tcpConnectionLocalAddress[2],
                entry.tcpConnectionLocalAddress[3]);
        T_D("entry lport = %lu", entry.tcpConnectionLocalPort);
        T_D("entry remaddr = %d.%d.%d.%d", entry.tcpConnectionRemAddress[0], entry.tcpConnectionRemAddress[1], entry.tcpConnectionRemAddress[2],
                entry.tcpConnectionRemAddress[3]);
        T_D("entry rport = %lu", entry.tcpConnectionRemPort);
        T_D("buf localaddr = %d.%d.%d.%d", buf.tcpConnectionLocalAddress[0], buf.tcpConnectionLocalAddress[1], buf.tcpConnectionLocalAddress[2],
                buf.tcpConnectionLocalAddress[3]);
        T_D("buf lport = %lu", buf.tcpConnectionLocalPort);
        T_D("buf remaddr = %d.%d.%d.%d", buf.tcpConnectionRemAddress[0], buf.tcpConnectionRemAddress[1], buf.tcpConnectionRemAddress[2],
                buf.tcpConnectionRemAddress[3]);
        T_D("buf rport = %lu", buf.tcpConnectionRemPort);

        T_D("==============================================================================");
        if (cmp_tcp_entry(&entry, key) > 0 && cmp_tcp_entry(&entry, &buf) < 0) {
            T_D("replace buf to entry");
            switch (state) {
            case TCP_LISTEN:
                entry.tcpConnectionState = 2;
                break;
            case TCP_SYN_SENT:
                entry.tcpConnectionState = 3;
                break;
            case TCP_SYN_RECV:
                entry.tcpConnectionState = 4;
                break;
            case TCP_ESTABLISHED:
                entry.tcpConnectionState = 5;
                break;
            case TCP_CLOSE_WAIT:
                entry.tcpConnectionState = 8;
                break;
            case TCP_FIN_WAIT1:
                entry.tcpConnectionState = 6;
                break;
            case TCP_CLOSING:
                entry.tcpConnectionState = 10;
                break;
            case TCP_LAST_ACK:
                entry.tcpConnectionState = 9;
                break;
            case TCP_FIN_WAIT2:
                entry.tcpConnectionState = 7;
                break;
            case TCP_TIME_WAIT:
                entry.tcpConnectionState = 11;
                break;
            default:
                entry.tcpConnectionState = 1;
            }
            buf = entry;
            found = 1;
        }
    }

    fclose (in);

    if (found) {
        *key = buf;
    }
    return found ? 0 : -2;

}

u_long linux_tcp_conn_entry_iterator(void **entry_addr, tcp_connTable_entry_t *entry)
{
    static tcp_connTable_entry_t *ptr;
    tcp_connTable_entry_t buf;
    if (*entry_addr == NULL) {
        memset(&buf, 0, sizeof(buf));
        ptr = &buf;
        T_D("GET FIRST");
        if (!tcp_conn_entry_get_next(ptr)) {
            goto got_it;
        } else {
            return -2;
        }
    } else {
        ptr = entry;
        T_D("GET NEXT by");
        if (!tcp_conn_entry_get_next(ptr)) {
            goto got_it;
        } else {
            return -2;
        }

    }

got_it:
    if (*entry_addr == NULL) {
       *entry = *ptr;
    }

    *entry_addr = ptr;

    return 0;
}
