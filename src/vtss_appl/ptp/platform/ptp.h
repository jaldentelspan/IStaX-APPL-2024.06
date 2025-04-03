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
 Microchip is aware that some terminology used in this technical document is
 antiquated and inappropriate. As a result of the complex nature of software
 where seemingly simple changes have unpredictable, and often far-reaching
 negative results on the software's functionality (requiring extensive retesting
 and revalidation) we are unable to make the desired changes in all legacy
 systems without compromising our product or our clients' products.
*/

#ifndef _PTP_H_
#define _PTP_H_

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_PTP
#define VTSS_TRACE_GRP_SERVO        (1 + VTSS_TRACE_GRP_PTP_CNT)
#define VTSS_TRACE_GRP_INTERFACE    (2 + VTSS_TRACE_GRP_PTP_CNT)
#define VTSS_TRACE_GRP_CLOCK        (3 + VTSS_TRACE_GRP_PTP_CNT)
#define VTSS_TRACE_GRP_1_PPS        (4 + VTSS_TRACE_GRP_PTP_CNT)
#define VTSS_TRACE_GRP_EGR_LAT      (5 + VTSS_TRACE_GRP_PTP_CNT)
#define VTSS_TRACE_GRP_PHY_TS       (6 + VTSS_TRACE_GRP_PTP_CNT)
#define VTSS_TRACE_GRP_PTP_SER      (7 + VTSS_TRACE_GRP_PTP_CNT)
#define VTSS_TRACE_GRP_PTP_PIM      (8 + VTSS_TRACE_GRP_PTP_CNT)
#define VTSS_TRACE_GRP_PTP_ICLI     (9 + VTSS_TRACE_GRP_PTP_CNT)
#define VTSS_TRACE_GRP_PHY_1PPS     (10+ VTSS_TRACE_GRP_PTP_CNT)
#define VTSS_TRACE_GRP_SYS_TIME     (11+ VTSS_TRACE_GRP_PTP_CNT)
#define VTSS_TRACE_GRP_ACE          (12+ VTSS_TRACE_GRP_PTP_CNT)
#define VTSS_TRACE_GRP_MS_SERVO     (13+ VTSS_TRACE_GRP_PTP_CNT)

#define _S VTSS_TRACE_GRP_SERVO
#define _I VTSS_TRACE_GRP_INTERFACE
#define _C VTSS_TRACE_GRP_CLOCK


/****************************************************************************/
// API Error Return Codes (mesa_rc)
/****************************************************************************/
enum {
    PTP_ERROR_INV_PARAM = MODULE_ERROR_START(VTSS_MODULE_ID_PTP),
    PTP_RC_INVALID_PORT_NUMBER,
    PTP_RC_INTERNAL_PORT_NOT_ALLOWED,
    PTP_RC_MISSING_PHY_TIMESTAMP_RESOURCE,
    PTP_RC_MISSING_IP_ADDRESS,
    PTP_RC_UNSUPPORTED_ACL_FRAME_TYPE,
    PTP_RC_UNSUPPORTED_PTP_ENCAPSULATION_TYPE,
    PTP_RC_UNSUPPORTED_1PPS_OPERATION_MODE,
    PTP_RC_CONFLICT_NTP_ENABLED,
    PTP_RC_CONFLICT_PTP_ENABLED,
    PTP_RC_MULTIPLE_SLAVES,
    PTP_RC_MULTIPLE_TC,
    PTP_RC_CLOCK_DOMAIN_CONFLICT,
    PTP_RC_MISSING_ACL_RESOURCES,
    PTP_RC_ADJ_METHOD_CHANGE_NOT_ALLOWED,
};

#define PTP_CONF_VERSION    10

#define PTP_READY()    (ptp_global.ready)

#define TEMP_LOCK()    vtss_global_lock(__FILE__, __LINE__)
#define TEMP_UNLOCK()  vtss_global_unlock(__FILE__, __LINE__)

#define PTP_CORE_LOCK()   critd_enter(&ptp_global.coremutex, __FILE__, __LINE__)
#define PTP_CORE_UNLOCK() critd_exit (&ptp_global.coremutex, __FILE__, __LINE__)

/* ARP Inspection ACE IDs */
#define PTP_ACE_ID_START        1

/*
 * used to initialize the run time options default
 */
#define DEFAULT_MAX_FOREIGN_RECORDS  5
#define DEFAULT_MAX_OUTSTANDING_RECORDS 25
#define DEFAULT_INGRESS_LATENCY      0       /* in nsec */
#define DEFAULT_EGRESS_LATENCY       0       /* in nsec */
#define DEFAULT_DELAY_ASYMMETRY      0       /* in nsec */
#define DEFAULT_PTP_DOMAIN_NUMBER     0
#define DEFAULT_UTC_OFFSET           0
#define DEFAULT_SYNC_INTERVAL        0         /* sync interval = 2**n sec */
#define DEFAULT_ANNOUNCE_INTERVAL    1         /* announce interval = 2**n sec */
#define DEFAULT_DELAY_REQ_INTERVAL   0         /* logarithmic value - Note: This has been changed to 0 to accomodate the limitations of ACI_BASIC_PHASE_LOW */
#define DEFAULT_ANNOUNCE_RECEIPT_TIMEOUT 3     /* timeout in announce interval periods */
#define DEFAULT_DELAY_S              6
#define DEFAULT_AP                   3
#define DEFAULT_AI                   30
#define DEFAULT_AD                   40



#define PTP_RC(expr) { mesa_rc my_ptp_rc = (expr); if (my_ptp_rc < VTSS_RC_OK) { \
        T_W("Error code: %s", error_txt(my_ptp_rc)); }}

#define PTP_RETURN(expr) { mesa_rc my_ptp_rc = (expr); if (my_ptp_rc < VTSS_RC_OK) return my_ptp_rc; }

void ptp_time_setting_start(void);
#endif /* _PTP_H_ */

