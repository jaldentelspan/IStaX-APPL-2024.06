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
#include "ucd_snmp_os_wrapper_udp.h"

#define UDP_STATS_LINE  "Udp: %lu %lu %lu %lu"
#define UDP_STATS_PREFIX_LEN    5

u_long linux_udp_stat_get(udp_stat_scalar_t *udp_stat)
{
    FILE *in = fopen ("/proc/net/snmp", "r");
    char line [1024];

    if (!in) {
        return -1;
    }

    while (line == fgets (line, sizeof(line), in)) {
        if (!strncmp( line, UDP_STATS_LINE, UDP_STATS_PREFIX_LEN )) {
            sscanf ( line, UDP_STATS_LINE,
                                &udp_stat->udpInDatagrams,
                                &udp_stat->udpNoPorts,
                                &udp_stat->udpInErrors,
                                &udp_stat->udpOutDatagrams);
        }

    }

    fclose (in);
    return 0;
}


static int cmp_udp_entry(udp_connTable_entry_t *key, udp_connTable_entry_t *data) {
    int rc = -1;
    if ((rc = memcmp(key->udpConnectionLocalAddress, data->udpConnectionLocalAddress, sizeof(data->udpConnectionLocalAddress))) != 0) {
        return rc;
    }

    if (key->udpConnectionLocalPort > data->udpConnectionLocalPort) {
        return 1;
    } else if (key->udpConnectionLocalPort < data->udpConnectionLocalPort){
        return -1;
    }

    return 0;
}

static int  udp_conn_entry_get_next(udp_connTable_entry_t *key)
{
    FILE *in;
    char line [256];
    udp_connTable_entry_t buf, entry;
    int found = 0;

    memset (&buf, 0xff, sizeof(buf));
    if (! (in = fopen ("/proc/net/udp", "r"))) {
        snmp_log(LOG_ERR, "snmpd: cannot open /proc/net/udp ...\n");
        return -1;
    }

    T_D("key localaddr = %d.%d.%d.%d", key->udpConnectionLocalAddress[0], key->udpConnectionLocalAddress[1], key->udpConnectionLocalAddress[2],
            key->udpConnectionLocalAddress[3]);
    T_D("key lport = %lu", key->udpConnectionLocalPort);

    while (line == fgets (line, sizeof(line), in)) {
        if (2 != sscanf (line, "%*d: %x:%x %*x:%*x %*x",
                         (unsigned int*)entry.udpConnectionLocalAddress, (unsigned int*)&entry.udpConnectionLocalPort))
          continue;

        T_D("entry localaddr = %d.%d.%d.%d", entry.udpConnectionLocalAddress[0], entry.udpConnectionLocalAddress[1], entry.udpConnectionLocalAddress[2],
                entry.udpConnectionLocalAddress[3]);
        T_D("entry lport = %lu", entry.udpConnectionLocalPort);
        T_D("buf localaddr = %d.%d.%d.%d", buf.udpConnectionLocalAddress[0], buf.udpConnectionLocalAddress[1], buf.udpConnectionLocalAddress[2],
                buf.udpConnectionLocalAddress[3]);
        T_D("buf lport = %lu", buf.udpConnectionLocalPort);
        T_D("==============================================================================");
        if (cmp_udp_entry(&entry, key) > 0 && cmp_udp_entry(&entry, &buf) < 0) {
            T_D("replace buf to entry");
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

u_long linux_udp_conn_entry_iterator(void **entry_addr, udp_connTable_entry_t *entry)
{
    static udp_connTable_entry_t *ptr;
    udp_connTable_entry_t buf;
    if (*entry_addr == NULL) {
        memset(&buf, 0, sizeof(buf));
        ptr = &buf;
        T_D("GET FIRST");
        if (!udp_conn_entry_get_next(ptr)) {
            goto got_it;
        } else {
            return -2;
        }
    } else {
        ptr = entry;
        T_D("GET NEXT by");
        if (!udp_conn_entry_get_next(ptr)) {
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


