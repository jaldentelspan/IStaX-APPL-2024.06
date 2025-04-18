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

/*
******************************************************************************

    Include File

******************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "icli_api.h"
#include "icli_porting_util.h"
#include "icli_porting_trace.h"
#include "msg_api.h"    // msg_switch_exists()
#include "mgmt_api.h"   // mgmt_iport_list2txt()
#include "misc_api.h"   // uport2iport(), iport2uport()
#include "port_api.h"
#include "topo_api.h"   // topo_usid2isid(), topo_isid2usid()
#include <vtss/basics/set.hxx>

/*
******************************************************************************

    Constant and Macro

******************************************************************************
*/
/*
******************************************************************************

    Type Definition

******************************************************************************
*/

/*
******************************************************************************

    Static Variable

******************************************************************************
*/

/*
******************************************************************************

    Static Function

******************************************************************************
*/
/* Display header text with optional new line before and after */
static void icli_header_nl_char(i32 session_id, const char *txt, BOOL pre, BOOL post, char c)
{
    int i, len;

    if (pre) {
        ICLI_PRINTF("\n");
    }
    ICLI_PRINTF("%s:\n", txt);
    len = (strlen(txt) + 1);
    for (i = 0; i < len; i++) {
        ICLI_PRINTF("%c", c);
    }
    ICLI_PRINTF("\n");
    if (post) {
        ICLI_PRINTF("\n");
    }
}

/*
******************************************************************************

    Public Function

******************************************************************************/

struct IcliPort {
    u32 port_type;
    vtss_isid_t isid;
    mesa_port_no_t iport;
    mesa_port_no_t uport;
};

bool operator<(const IcliPort &x, const IcliPort &y)
{
    if (x.isid < y.isid) {
        return true;
    }
    if (y.isid < x.isid) {
        return false;
    }
    if (x.port_type < y.port_type) {
        return true;
    }
    if (y.port_type < x.port_type) {
        return false;
    }
    if (x.iport < y.iport) {
        return true;
    }
    return false;
}

static vtss::Set<IcliPort> icli_port_set;


BOOL vtss_icli_get_first_port(port_iter_t  *pit)
{
    if (!pit) {
        return false;
    }
    auto i = icli_port_set.begin();
    if (i == icli_port_set.end()) {
        return FALSE;
    }
    pit->first = TRUE;
    pit->port_type = i->port_type;
    pit->m_isid = i->isid;
    pit->iport = i->iport;
    pit->uport = i->uport;
    pit->m_state = PORT_ITER_STATE_NEXT;
    return TRUE;
}

BOOL vtss_icli_get_next_port(port_iter_t  *pit)
{
    if (!pit) {
        return FALSE;
    }
    switch (pit->m_state) {
    case PORT_ITER_STATE_FIRST:
        return vtss_icli_get_first_port(pit);
    default:
        IcliPort index = {pit->port_type, pit->m_isid, pit->iport};
        auto i = icli_port_set.find(index);
        if (i == icli_port_set.end() || ++i == icli_port_set.end()) {
            return FALSE;
        }
        pit->port_type = i->port_type;
        pit->m_isid = i->isid;
        pit->iport = i->iport;
        pit->uport = i->uport;
        return TRUE;
    }
}

void icli_port_iter_add_port(u32 type, vtss_isid_t isid, mesa_port_no_t iport,
                             mesa_port_no_t uport)
{
    IcliPort index = {type, isid, iport, uport};
    icli_port_set.set(index);
}

/* Print two counters (RX and TX) in columns  */
// IN - Session_Id - For iCLI printing
//      rx_name    - RX counter name - Set to NULL if there is no RX counter
//      tx_name    - TX counter name - Set to NULL if there is no TX counter
//      rx_val     - RX counter value
//      tx_val     - TX counter value
void icli_cmd_stati(i32 session_id, const char *rx_name, const char *tx_name, u64 rx_val, u64 tx_val)
{
    char buf[80];

    if (rx_name != NULL) {
        sprintf(buf, "Rx %s:", rx_name);
        ICLI_PRINTF("%-23s %17" PRIu64 "   ", buf, rx_val);
    } else {
        ICLI_PRINTF("%-23s %17s   ", "", "");
    }


    if (tx_name != NULL) {
        sprintf(buf, "Tx %s:", strlen(tx_name) ? tx_name : rx_name);
        ICLI_PRINTF("%-20s %20" PRIu64, buf, tx_val);
    }

    ICLI_PRINTF("\n");
}

/* Get enabled/disabled text */
const char *icli_bool_txt(BOOL enabled)
{
    return (enabled ? "enabled" : "disabled");
}

/* Get port information text */
static char *icli_port_info_txt_generic(vtss_usid_t usid, mesa_port_no_t uport, char *str_buf_p, BOOL is_short)
{
    icli_switch_port_range_t switch_range;
    const char *type;

    switch_range.usid = usid;
    switch_range.begin_uport = uport;
    switch_range.port_cnt = 1;
    str_buf_p[0] = '\0';
    if (icli_port_from_usid_uport(&switch_range)) {
        type = is_short ? icli_port_type_get_short_name((icli_port_type_t)(switch_range.port_type)) : icli_port_type_get_name((icli_port_type_t)(switch_range.port_type));
        sprintf(str_buf_p, "%s %u/%u",
                type,
                switch_range.switch_id,
                switch_range.begin_port);
    }
    return str_buf_p;
}

char *icli_port_info_txt(vtss_usid_t usid, mesa_port_no_t uport, char *str_buf_p)
{
    if (usid == VTSS_USID_LOCAL) {
        usid = topo_isid2usid(VTSS_ISID_START);
    }
    T_I("sid:%d, port:%d", usid, uport);
    return icli_port_info_txt_generic(usid, uport, str_buf_p, FALSE);
}

char *icli_port_info_txt_short(vtss_usid_t usid, mesa_port_no_t uport, char *str_buf_p)
{
    return icli_port_info_txt_generic(usid, uport, str_buf_p, TRUE);
}

/* Display port header text with optional new line before and after */
// IN: pre - TRUE to add new line before printing the header
//     post - TRUE to add new line after printing the header

void icli_port_header(i32 session_id, vtss_isid_t usid, mesa_port_no_t uport, const char *txt, BOOL pre, BOOL post)
{
    char str_buf[ICLI_PORTING_STR_BUF_SIZE];

    (void) icli_port_info_txt(usid, uport, str_buf);
    if (txt) {
        sprintf(str_buf + strlen(str_buf), " %s", txt);
    }
    icli_header_nl_char(session_id, str_buf, pre, post, '-');
}

static void icli_table_header_parm(i32 session_id, const char *txt, BOOL parm)
{
    int i, j, len, count = 0;

    ICLI_PRINTF("%s\n", txt);
    while (*txt == ' ') {
        ICLI_PRINTF(" ");
        txt++;
    }
    len = strlen(txt);
    for (i = 0; i < len; i++) {
        //ignore CR/LF
        if (txt[i] == 0xa /* LF */ || txt[i] == 0xd /* CR */) {
            continue;
        } else if (txt[i] == ' ') {
            count++;
        } else {
            for (j = 0; j < count; j++) {
                ICLI_PRINTF("%c", count > 1 && (parm || j >= (count - 2)) ? ' ' : '-');
            }
            ICLI_PRINTF("-");
            count = 0;
        }
    }
    for (j = 0; j < count; j++) {
        ICLI_PRINTF("%c", count > 1 && (parm || j >= (count - 2)) ? ' ' : '-');
    }
    ICLI_PRINTF("\n");
}

/* Display underlined header with new line before and after */
void icli_header(i32 session_id, const char *txt, BOOL post)
{
    icli_header_nl_char(session_id, txt, TRUE, post, '=');
}

/* Display header text with under line (including underlining of spaces) */
void icli_table_header(i32 session_id, const char *txt)
{
    icli_table_header_parm(session_id, txt, FALSE);
}

/* Display header text with under line */
void icli_parm_header(i32 session_id, const char *txt)
{
    icli_table_header_parm(session_id, txt, TRUE);
}

// Function printing the port interface and number - e.g. GiabitEthernet 1/1
// IN : Session_Id : The session_id
//      usid       : User switch id.
//      uport      : User port number
void icli_print_port_info_txt(i32 session_id, vtss_isid_t usid, mesa_port_no_t uport)
{
    char str_buf[ICLI_PORTING_STR_BUF_SIZE];
    (void) icli_port_info_txt(usid, uport, str_buf);
    ICLI_PRINTF("%s ", str_buf);
}

/* Display two counters in one row */
void icli_stats(i32 session_id, const char *counter_str_1_p, const char *counter_str_2_p, u32 counter_1, u32 counter_2)
{
    char str_buf[ICLI_PORTING_STR_BUF_SIZE];

    sprintf(str_buf, "%s:", counter_str_1_p);
    ICLI_PRINTF("%-28s%10u   ", str_buf, counter_1);
    if (counter_str_2_p != NULL) {
        sprintf(str_buf, "%s:", counter_str_2_p);
        ICLI_PRINTF("%-28s%10u", str_buf, counter_2);
    }
    ICLI_PRINTF("\n");
}

/* Port list string */
char *icli_iport_list_txt(mesa_port_list_t &port_list, char *buf)
{
    if (strlen(mgmt_iport_list2txt(port_list, buf)) == 0) {
        strcpy(buf, "None");
    }

    return buf;
}

const char *icli_time_txt(time_t time_val)
{
    const char *s;

    s = misc_time2interval(time_val);
    while (*s == ' ') {
        s++;
    }
    if (!strncmp(s, "0d ", 3)) {
        s += 3;
    }

    return s;
}

char *icli_mac_txt(const uchar *mac, char *buf)
{
    sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return buf;
}

/* Convert to iport_list from icli_range_list.
   Return value -
    0 : success
    -1: fail - invalid parameter
    -2: fail - iport_list array size overflow */
int icli_rangelist2iportlist(icli_unsigned_range_t *icli_range_list_p, mesa_port_list_t &port_list)
{
    u32             range_idx, range_value;
    mesa_port_no_t  iport;
    u32 iport_list_max_num = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);

    if (!icli_range_list_p) {
        return -1; //invalid parameter
    }

    port_list.clear_all();

    for (range_idx = 0; range_idx < icli_range_list_p->cnt; range_idx++) {
        for (range_value = icli_range_list_p->range[range_idx].min; range_value <= icli_range_list_p->range[range_idx].max; range_value++) {
            iport = uport2iport(range_value);
            if (iport >= iport_list_max_num) {
                return -2; //iport_lis array size overflow
            }
            port_list[iport] = TRUE;
        }
    }

    return 0;
}

// void print_icli_stack_port_range_t(icli_stack_port_range_t *plist)
// {
//     if (!plist) {
//         printf("No range\n");
//         return;
//     }
//     for (uint32_t rdx = 0; rdx < plist->cnt; rdx++) {
//         printf("%d/%d-%d\n",
//                plist->switch_range[rdx].usid,
//                plist->switch_range[rdx].begin_uport,
//                plist->switch_range[rdx].begin_uport + plist->switch_range[rdx].port_cnt-1);
//     }
// }

/* Convert to iport_list from icli_port_type_list.
   Return value -
    0 : success
    -1: fail - invalid parameter
    -2: fail - iport_list array size overflow
    1 : fail - different switch ID existing in port type list */
int icli_porttypelist2iportlist(vtss_usid_t usid, icli_stack_port_range_t *port_type_list_p, mesa_port_list_t &port_list)
{
    u32             range_idx, cnt_idx;
    mesa_port_no_t  iport;
    u32 iport_list_max_num = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);

    if (!port_type_list_p) {
        return -1; //invalid parameter
    }

    port_list.clear_all();

    for (range_idx = 0; range_idx < port_type_list_p->cnt; range_idx++) {
        if (usid != VTSS_ISID_GLOBAL && usid != port_type_list_p->switch_range[range_idx].usid) {
            return 1; //different switch ID existing in port type list
        }
        for (cnt_idx = 0; cnt_idx < port_type_list_p->switch_range[range_idx].port_cnt; cnt_idx++) {
            iport = uport2iport(port_type_list_p->switch_range[range_idx].begin_uport + cnt_idx);
            if (iport >= iport_list_max_num) {
                return -2; //iport_lis array size overflow
            }
            port_list[iport] = TRUE;
        }
    }

    return 0;
}

/* Get port type text from switch_range.
   Note: The value of first_range_p will change to FALSE after we called this API.
 */
static char *icli_switch_range_txt(BOOL *first_range_p, icli_switch_port_range_t *switch_range_p, char *str_buf_p, BOOL short_form)
{
    char temp_str_buf[8];

    sprintf(temp_str_buf, "-%u", switch_range_p->begin_port + switch_range_p->port_cnt - 1);
    if (*first_range_p) {
        sprintf(str_buf_p, "%s %u/%u%s",
                short_form ? icli_port_type_get_short_name((icli_port_type_t)(switch_range_p->port_type)) : icli_port_type_get_name((icli_port_type_t)(switch_range_p->port_type)),
                switch_range_p->switch_id,
                switch_range_p->begin_port,
                switch_range_p->port_cnt == 1 ? "" : temp_str_buf);
        *first_range_p = FALSE;
    } else {
        sprintf(str_buf_p, ",%u%s",
                switch_range_p->begin_port,
                switch_range_p->port_cnt == 1 ? "" : temp_str_buf);
    }
    return str_buf_p;
}


/* Get port list information text */
char *icli_port_list_info_txt(vtss_isid_t isid, mesa_port_list_t &port_list, char *str_buf_p, BOOL short_form)
{
    icli_switch_port_range_t    icli_port, pre_switch_range;
    BOOL                        new_type_range = TRUE, pre_switch_range_exist = FALSE;

    /* Initialize local parameters */
    pre_switch_range.port_type = ICLI_PORT_TYPE_NONE;
    str_buf_p[0] = '\0';

    /* Go through port table */
    memset(&icli_port, 0x0, sizeof(icli_port));
    icli_port.switch_id = icli_isid2switchid(isid);
    while (icli_switch_port_get_next(&icli_port)) {
        if (!port_list[icli_port.begin_iport]) {
            if (pre_switch_range_exist) {   // Output the previous information
                pre_switch_range_exist = FALSE;
                if (new_type_range && strlen(str_buf_p)) {   //Add a space character
                    strcpy(str_buf_p + strlen(str_buf_p), " ");
                }
                (void)icli_switch_range_txt(&new_type_range, &pre_switch_range, str_buf_p + strlen(str_buf_p), short_form);
            }
            continue;
        }

        if (pre_switch_range_exist) {
            if (icli_port.port_type == pre_switch_range.port_type) {
                // Pre-testing range is invalid?
                icli_switch_port_range_t test_switch_range;
                test_switch_range = pre_switch_range;
                test_switch_range.port_cnt++;
                if (icli_port_switch_range_valid(&test_switch_range, NULL, NULL)) {
                    pre_switch_range = test_switch_range;
                    continue;
                }
            }

            // Output the previous information
            if (new_type_range && strlen(str_buf_p)) {   //Add a space character
                strcpy(str_buf_p + strlen(str_buf_p), " ");
            }
            (void)icli_switch_range_txt(&new_type_range, &pre_switch_range, str_buf_p + strlen(str_buf_p), short_form);
        }

        // Reset the "new_type_range" flag to display the interface type
        if (pre_switch_range.port_type != ICLI_PORT_TYPE_NONE && icli_port.port_type != pre_switch_range.port_type) {
            new_type_range = TRUE;
        }

        pre_switch_range = icli_port;
        pre_switch_range_exist = TRUE;
    }

    if (pre_switch_range_exist) {   // Output the last information
        if (new_type_range && strlen(str_buf_p)) {   //Add a space character
            strcpy(str_buf_p + strlen(str_buf_p), " ");
        }
        (void)icli_switch_range_txt(&new_type_range, &pre_switch_range, str_buf_p + strlen(str_buf_p), short_form);
    }

    return str_buf_p;
}

BOOL icli_uport_is_included(vtss_usid_t usid, mesa_port_no_t uport, icli_stack_port_range_t *plist)
{
    u32             rdx;
    u16             bgn, cnt;

    if (!plist) {
        return FALSE;
    }
    for (rdx = 0; rdx < plist->cnt; rdx++) {
        if (usid != plist->switch_range[rdx].usid) {
            continue;
        }
        cnt = plist->switch_range[rdx].port_cnt;
        bgn = plist->switch_range[rdx].begin_uport;
        if ((uport >= bgn) && (uport < bgn + cnt)) {
            return TRUE;
        }
    }
    return FALSE;
}

/* Check if the usid is a member of the portlist */
BOOL icli_usid_is_included(vtss_usid_t usid, icli_stack_port_range_t *plist)
{
    u32             rdx;

    if (!plist) {
        return FALSE;
    }
    for (rdx = 0; rdx < plist->cnt; rdx++) {
        if (usid == plist->switch_range[rdx].usid) {
            return TRUE;
        }
    }
    return FALSE;
}

/*
 * Initialize iCLI switch iterator to iterate over all switches in USID order.
 * Use icli_switch_iter_getnext() to filter out non-selected, non-configurable switches.
 */
mesa_rc icli_switch_iter_init(switch_iter_t *sit)
{
    return switch_iter_init(sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_USID_CFG);
}

/*
 * iCLI switch iterator. Returns selected switches.
 * If list == NULL, all existing switches are returned.
 * Updates sit->first to match first selected switch.
 * NOTE: sit->last is not updated and therefore unreliable.
 */
BOOL icli_switch_iter_getnext(switch_iter_t *sit, icli_stack_port_range_t *list)
{
    BOOL first = FALSE;
    u32  i;
    while (switch_iter_getnext(sit)) {
        if (sit->first) {
            first = TRUE;
        }
        if (list) {
            // Loop though the list and check that current switch is specified.
            for (i = 0; i < list->cnt; i++) {
                if (sit->usid == list->switch_range[i].usid) {
                    sit->first = first;
                    return TRUE;
                }
            }
        } else {
            return TRUE; // No list - return every switch.
        }
    }
    return FALSE; // Done - No more switches.
}

/*
 * Initialize iCLI port iterator to iterate over all ports in uport order.
 * Use icli_port_iter_getnext() to filter out non-selected ports.
 */
mesa_rc icli_port_iter_init(port_iter_t *pit, vtss_isid_t isid, u32 flags)
{
    return port_iter_init(pit, NULL, isid, PORT_ITER_SORT_ORDER_UPORT, flags);
}

/*
 * iCLI port iterator. Returns selected ports.
 * If list == NULL, all existing ports are returned.
 * Updates pit->first to match first selected port.
 * NOTE: pit->last is not updated and therefore unreliable.
 */
BOOL icli_port_iter_getnext(port_iter_t *pit, icli_stack_port_range_t *list)
{
    if (pit->m_state != PORT_ITER_STATE_FIRST) {
        pit->first = FALSE;
    }

    while (vtss_icli_get_next_port(pit)) {
        if (!list || icli_uport_is_included(topo_isid2usid(pit->m_isid), pit->uport, list)) {
            return TRUE;
        }
    }
    return FALSE;
}


// Runtime function for ICLI that determines stacking information
BOOL icli_runtime_stacking(u32                session_id,
                           icli_runtime_ask_t ask,
                           icli_runtime_t     *runtime)
{
    switch (ask) {
    case ICLI_ASK_PRESENT:
        runtime->present = FALSE;
        return TRUE;
    case ICLI_ASK_BYWORD:
        // Subtract 1, because the last switch is the VTSS_ISID_GLOBAL which shall not  be used here.
        icli_sprintf(runtime->byword, "<Switch ID: %u-%u>", VTSS_USID_START, VTSS_USID_END - 1);
        return TRUE;
    case ICLI_ASK_RANGE:
        runtime->range.type = ICLI_RANGE_TYPE_UNSIGNED;
        runtime->range.u.sr.cnt = 1;
        runtime->range.u.sr.range[0].min = VTSS_USID_START;
        runtime->range.u.sr.range[0].max = VTSS_USID_END - 1; // Subtract 1, because the last switch is the VTSS_ISID_GLOBAL which shall not  be used here.
        return TRUE;
    case ICLI_ASK_HELP:
        icli_sprintf(runtime->help, "Switch ID");
        return TRUE;
    default:
        break;
    }
    return FALSE;
}

BOOL icli_present_cap(icli_runtime_ask_t ask,
                      icli_runtime_t     *runtime,
                      int                cap)
{
    if (ask == ICLI_ASK_PRESENT) {
        runtime->present = (fast_cap(cap) ? TRUE : FALSE);
        return TRUE;
    }
    return FALSE;
}

BOOL icli_present_phy_cap(icli_runtime_ask_t ask, icli_runtime_t *runtime, mepa_phy_cap_t cap)
{
    if (ask == ICLI_ASK_PRESENT) {
        runtime->present = (port_phy_cap_check(cap) ? TRUE : FALSE);
        return TRUE;
    }
    return FALSE;
}

/**
 * \brief Check configurable switches if existing.
 *
 * \param session_id                          [IN]     - The ICLI session ID.
 * \param port_list_p                         [IN&OUT] - Pointer to structure that contains the port range information.
 * \param omit_non_exist                      [IN]     - If it is needed to ommit non-existing switches in the port range information.
 * \parm                                                 The parameter of 'port_list_p' will be updaetd if this value is TRUE.
 * \param alert_configurable_but_non_existing [IN]     - If it is needed to display the alert message alert message(configurable but non-existing) on ICLI session.
 *
 * \return TRUE  - All switches are configurable and existing in the port range information.
 * \return FALSE - Found at least one non-existing switch or it isn't configurable switch in the port range information.
 **/
/*lint --e{644} */
//(644) Variable 'new_port_list' may not have been initialized.
// We initialize it when we needed.
BOOL icli_cmd_port_range_exist(u32 session_id, icli_stack_port_range_t *port_list_p, BOOL omit_non_exist, BOOL alert_configurable_but_non_existing)
{
    BOOL                    rc = TRUE;
    u32                     range_idx, new_cnt = 0;
    icli_stack_port_range_t new_port_list;
    vtss_isid_t             isid;

    /* Check input parameter */
    if (!port_list_p) {
        ICLI_PRINTF("%% Error - Input parameter is pointed to NULL address.\n");
        return FALSE;
    }

    /* Initialize local variable */
    if (omit_non_exist) {
        memset(&new_port_list, 0, sizeof(new_port_list));
    }

    /* Process each valid entry */
    for (range_idx = 0; range_idx < port_list_p->cnt; range_idx++) {
        isid = port_list_p->switch_range[range_idx].isid;

        if (!msg_switch_configurable(isid)) {
            rc = FALSE;
        } else if (!msg_switch_exists(isid)) {
            rc = FALSE;
            if (alert_configurable_but_non_existing) {
                ICLI_PRINTF("%% The configure switch %u is non-existing.\n", port_list_p->switch_range[range_idx].usid);
            }
        }

        //break for loop if we don't need to omit or alert message
        if (!omit_non_exist && !alert_configurable_but_non_existing && !rc) {
            break;
        }

        if (omit_non_exist && msg_switch_exists(isid)) {
            new_port_list.switch_range[new_cnt++] = port_list_p->switch_range[range_idx];
        }
    }

    /* Omit non-existing switches from the port range database */
    if (omit_non_exist) {
        *port_list_p = new_port_list;
        port_list_p->cnt = new_cnt;
    }

    if (alert_configurable_but_non_existing && !rc) {
        ICLI_PRINTF("\n");
    }

    return rc;
}

/**
 * \brief Check configurable switche if existing.
 *
 * \param session_id                          [IN] - The ICLI session ID.
 * \param usid                                [IN] - User switch ID.
 * \param alert_not_configurable              [IN] - If it is needed to display the alert message(not configurable switch) on ICLI session.
 * \param alert_configurable_but_non_existing [IN] - If it is needed to display the alert message(configurable but non-existing switch) on ICLI session.
 *
 * \return TRUE  - The switch is configurable and existing.
 * \return FALSE - The switch isn't configurable or it is non-existing.
 **/
BOOL icli_cmd_switch_exist(u32 session_id, vtss_usid_t usid, BOOL alert_not_configurable, BOOL alert_configurable_but_non_existing)
{
    vtss_usid_t isid = topo_usid2isid(usid);

    if (msg_switch_configurable(isid)) {
        if (!msg_switch_exists(isid)) {
            if (alert_configurable_but_non_existing) {
                ICLI_PRINTF("%% The configure switch ID %u is non-existing.\n", usid);
            }
            return FALSE;
        }
        return TRUE;
    } else if (alert_not_configurable) {
        ICLI_PRINTF("%% Switch ID %u isn't configure switch.\n", usid);
    }
    return FALSE;
}

/**
 * \brief Check configurable switches if existing.
 *
 * \param session_id                          [IN]     - The ICLI session ID.
 * \param switch_list_p                       [IN&OUT] - Pointer to structure that contains the switch range information.
 * \param omit_non_exist                      [IN]     - If it is needed to ommit non-existing switches in the switch range information.
 * \parm                                                 The parameter of 'switch_list_p' will be updaetd if this value is TRUE.
 * \param alert_configurable_but_non_existing [IN]     - If it is needed to display the alert message alert message(configurable but non-existing) on ICLI session.
 *
 * \return TRUE  - All switches are configurable and existing in the port range information.
 * \return FALSE - Found at least one non-existing switch or it isn't configurable switch in the port range information.
 **/
/*lint --e{644} */
//(644) Variable 'new_switch_list' may not have been initialized.
// We initialize it when we needed.
BOOL icli_cmd_switch_range_exist(u32 session_id, icli_unsigned_range_t *switch_list_p, BOOL omit_non_exist, BOOL alert_configurable_but_non_existing)
{
    BOOL                    rc = TRUE;
    u32                     range_idx, new_cnt = 0;
    icli_unsigned_range_t   new_switch_list;
    vtss_usid_t             usid_idx;
    vtss_isid_t             isid;
    BOOL                    is_new_range, is_non_existing, found_non_existing_in_range;
    unsigned int            new_min, new_max;

    /* Check input parameter */
    if (!switch_list_p) {
        ICLI_PRINTF("%% Error - Input parameter is pointed to NULL address.\n");
        return FALSE;
    }

    /* Initialize local variable */
    if (omit_non_exist) {
        memset(&new_switch_list, 0, sizeof(new_switch_list));
    }

    /* Process each valid entry */
    for (range_idx = 0; range_idx < switch_list_p->cnt; range_idx++) {
        is_new_range = TRUE;
        found_non_existing_in_range = FALSE;
        new_min = switch_list_p->range[range_idx].min;
        new_max = switch_list_p->range[range_idx].max;

        for (usid_idx = (vtss_usid_t)switch_list_p->range[range_idx].min;
             usid_idx <= (vtss_usid_t)switch_list_p->range[range_idx].max; usid_idx++) {
            isid = topo_usid2isid(usid_idx);
            is_non_existing = TRUE;
            if (!msg_switch_configurable(isid)) {
                rc = FALSE;
            } else if (!msg_switch_exists(isid)) {
                rc = FALSE;
                if (alert_configurable_but_non_existing) {
                    ICLI_PRINTF("%% The configure switch %u is non-existing.\n", usid_idx);
                }
                found_non_existing_in_range = TRUE;
            } else {
                if (is_new_range) {
                    new_min = (unsigned int)usid_idx;
                    is_new_range = FALSE;
                }
                new_max = (unsigned int)usid_idx;
                is_non_existing = FALSE;
            }

            //break for loop if we don't need to omit or alert message
            if (!omit_non_exist && !alert_configurable_but_non_existing && !rc) {
                break;
            }

            if (omit_non_exist &&
                !is_new_range &&
                (is_non_existing || (usid_idx == (vtss_usid_t)switch_list_p->range[range_idx].max /* last new range */))) {
                new_switch_list.range[new_cnt].min = new_min;
                new_switch_list.range[new_cnt++].max = new_max;
                is_new_range = TRUE;
            }
        }

        if (omit_non_exist && !found_non_existing_in_range) {
            new_switch_list.range[new_cnt++] = switch_list_p->range[range_idx];
        }
    }

    /* Omit non-existing switches from the port range database */
    if (omit_non_exist) {
        *switch_list_p = new_switch_list;
        switch_list_p->cnt = new_cnt;
    }

    if (alert_configurable_but_non_existing && !rc) {
        ICLI_PRINTF("\n");
    }

    return rc;
}

BOOL icli_is_switchport_runtime(u32                session_id,
                                icli_runtime_ask_t ask,
                                icli_runtime_t     *runtime)
{
    icli_variable_value_t       value;
    icli_stack_port_range_t     *plist;
    port_iter_t pit;
    switch (ask) {
    case ICLI_ASK_PRESENT:

        if (ICLI_MODE_PARA_GET(NULL, &value) != ICLI_RC_OK) {
            T_E("Failed to get mode para");
            ICLI_PRINTF("%% Fail to get mode para\n");
            runtime->present = FALSE;
            return TRUE;
        }

        plist = &(value.u.u_port_type_list); // port list

        if (plist->cnt >= ICLI_RANGE_LIST_CNT) {
            T_E("Something wrong with plist, defaulting to all ports. plist->cnt:%d", plist->cnt);
            plist = NULL;
        }

        VTSS_RC(icli_port_iter_init(&pit, VTSS_ISID_START, PORT_ITER_FLAGS_ALL));
        while (icli_port_iter_getnext(&pit, plist)) {
            if (ICLI_PORT_TYPE_CPU != pit.port_type) {
                runtime->present = TRUE;
                return TRUE;
            }
        }

        runtime->present = FALSE;
        return TRUE;
    default:
        return FALSE;
    }
}

BOOL icli_is_cpuport_runtime(u32                session_id,
                             icli_runtime_ask_t ask,
                             icli_runtime_t     *runtime)
{
    icli_variable_value_t       value;
    icli_stack_port_range_t     *plist;
    port_iter_t pit;
    switch (ask) {
    case ICLI_ASK_PRESENT:

        if (ICLI_MODE_PARA_GET(NULL, &value) != ICLI_RC_OK) {
            T_E("Failed to get mode para");
            ICLI_PRINTF("%% Fail to get mode para\n");
            runtime->present = FALSE;
            return TRUE;
        }

        plist = &(value.u.u_port_type_list); // port list

        if (plist->cnt >= ICLI_RANGE_LIST_CNT) {
            T_E("Something wrong with plist, defaulting to all ports. plist->cnt:%d", plist->cnt);
            plist = NULL;
        }

        VTSS_RC(icli_port_iter_init(&pit, VTSS_ISID_START, PORT_ITER_FLAGS_CPU));
        while (icli_port_iter_getnext(&pit, plist)) {
            if (ICLI_PORT_TYPE_CPU == pit.port_type) {
                runtime->present = TRUE;
                return TRUE;
            }
        }

        runtime->present = FALSE;
        return TRUE;
    default:
        return FALSE;
    }
}
