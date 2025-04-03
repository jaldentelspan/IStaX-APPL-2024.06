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

#include "vtss_ptp_internal_types.h"
#include "vtss_tod_api.h"
#include "vtss/appl/ptp.h"

static const char *bool_ToString(bool b)
{
        return (b ? " True" : "False");
}

/**
 * compare two PortIdentity's
 * \return value as memcmp i.e.
 * a < b => <0, a == b => 0, a > b => >0
 */
int PortIdentitycmp(const vtss_appl_ptp_port_identity* a, const vtss_appl_ptp_port_identity* b)
{
    int ret;

    ret = memcmp(a->clockIdentity, b->clockIdentity, CLOCK_IDENTITY_LENGTH);
    if (!ret) {
        if (a->portNumber > b->portNumber) {
            return 1;
        } else if (a->portNumber < b->portNumber) {
            return -1;
        }
    }
    return ret;
}

char *ClockIdentityToString(const vtss_appl_clock_identity clockIdentity, char *str)
{
    sprintf(str, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
            clockIdentity[0], clockIdentity[1], clockIdentity[2], clockIdentity[3],
            clockIdentity[4], clockIdentity[5], clockIdentity[6], clockIdentity[7]);
    return str;
}

const char *ClockProfileToString(const int profile)
{
    switch(profile) {
        case VTSS_APPL_PTP_PROFILE_NO_PROFILE: return "no profile";
        case VTSS_APPL_PTP_PROFILE_1588:       return "ieee1588";
        case VTSS_APPL_PTP_PROFILE_G_8265_1:   return "g8265.1";
        case VTSS_APPL_PTP_PROFILE_G_8275_1:   return "g8275.1";
        case VTSS_APPL_PTP_PROFILE_G_8275_2:   return "g8275.2";
        case VTSS_APPL_PTP_PROFILE_IEEE_802_1AS: return "802.1as";
        case VTSS_APPL_PTP_PROFILE_AED_802_1AS: return "802.1as-aed";
    }
    return "undefined";
}

const char * ClockAccuracyToString (u8 clockAccuracy)
{
    const char *str;
    switch (clockAccuracy) {
        case 0x20: str = "25 ns";  break;
        case 0x21: str = "100 ns"; break;
        case 0x22: str = "250 ns"; break;
        case 0x23: str = "1 us";   break;
        case 0x24: str = "2.5 us"; break;
        case 0x25: str = "10 us";  break;
        case 0x26: str = "25 us";  break;
        case 0x27: str = "100 us"; break;
        case 0x28: str = "250 us"; break;
        case 0x29: str = "1 ms";   break;
        case 0x2A: str = "2.5 ms"; break;
        case 0x2B: str = "10 ms";  break;
        case 0x2C: str = "25 ms";  break;
        case 0x2D: str = "100 ms"; break;
        case 0x2E: str = "250 ms"; break;
        case 0x2F: str = "1 s";    break;
        case 0x30: str = "10 s";   break;
        case 0x31: str = ">10 s";  break;
        case 0xFE: str = "Unknwn"; break;
        default:
            if (clockAccuracy >= 0x80 && clockAccuracy <= 0xFD) str = "Altrna";
            else  str = "Resrvd";
            break;
    }

    return str;
}

char *ClockQualityToString(const vtss_appl_clock_quality *clockQuality, char *str)
{
    sprintf(str, "Cl:%03d Ac:%-6s Va:%05d",
            clockQuality->clockClass, ClockAccuracyToString (clockQuality->clockAccuracy),clockQuality->offsetScaledLogVariance);
    return str;
}

char *TimeIntervalToString_ps(const mesa_timeinterval_t *t, char* str, char delim)
{
    mesa_timeinterval_t t1;
    char str1[14];
    if (*t < 0) {
        t1 = -*t;
        str[0] = '-';
    } else {
        t1 = *t;
        str[0] = ' ';
    }

    sprintf(str + 1, "%d.%s%c%04d", VTSS_INTERVAL_SEC(t1), vtss_tod_ns2str(VTSS_INTERVAL_NS(t1), str1, delim), delim, VTSS_INTERVAL_PS(t1));

    return str;
}

// String array 'str' passed as argument should be minimum 50 bytes size. Otherwise, it may result in memory corruption.
char *TimeStampToString(const mesa_timestamp_t *t, char* str)
{
    sprintf(str, "%5u s_msb %10u s %11u ns %5u ps", t->sec_msb, t->seconds, t->nanoseconds, t->nanosecondsfrac);
    return str;
}

const char *PortStateToString(u8 state)
{

    switch (state) {
    case VTSS_APPL_PTP_INITIALIZING:
        return "init";
    case VTSS_APPL_PTP_FAULTY:
        return "flty";
    case VTSS_APPL_PTP_LISTENING:
        return "lstn";
    case VTSS_APPL_PTP_PASSIVE:
        return "pass";
    case VTSS_APPL_PTP_UNCALIBRATED:
        return "uncl";
    case VTSS_APPL_PTP_SLAVE:
        return "slve";
    case VTSS_APPL_PTP_PRE_MASTER:
        return "pmst";
    case VTSS_APPL_PTP_MASTER:
        return "mstr";
    case VTSS_APPL_PTP_DISABLED:
        return "dsbl";
    case VTSS_APPL_PTP_P2P_TRANSPARENT:
        return "p2pt";
    case VTSS_APPL_PTP_E2E_TRANSPARENT:
        return "e2et";
    case VTSS_APPL_PTP_FRONTEND:
        return "frnd";
    default:
        return "?   ";
    }
}

const char *DeviceTypeToString(vtss_appl_ptp_device_type_t type)
{

    switch (type) {
    case VTSS_APPL_PTP_DEVICE_NONE:
        return "Inactive ";
    case VTSS_APPL_PTP_DEVICE_ORD_BOUND:
        return "Ord-Bound";
    case VTSS_APPL_PTP_DEVICE_P2P_TRANSPARENT:
        return "P2pTransp";
    case VTSS_APPL_PTP_DEVICE_E2E_TRANSPARENT:
        return "E2eTransp";
    case VTSS_APPL_PTP_DEVICE_SLAVE_ONLY:
        return "Slaveonly";
    case VTSS_APPL_PTP_DEVICE_MASTER_ONLY:
        return "Mastronly";
    case VTSS_APPL_PTP_DEVICE_BC_FRONTEND:
        return "BC-frontend";
    case VTSS_APPL_PTP_DEVICE_AED_GM:
        return "AED-GM";
    case VTSS_APPL_PTP_DEVICE_INTERNAL:
        return "Internal";
    default:
        return "?";
    }
}

char * vtss_ptp_TimePropertiesToString(const vtss_appl_ptp_clock_timeproperties_ds_t * properties, char *str, size_t size)
{
    snprintf(str, size, "UtcOffset: %d, Valid: %s, leap59: %s, leap61: %s, TimeTrac: %s, FreqTrac: %s Scale: %s, source: %d", 
             properties->currentUtcOffset, 
             bool_ToString(properties->currentUtcOffsetValid),
             bool_ToString(properties->leap59),
             bool_ToString(properties->leap61),
             bool_ToString(properties->timeTraceable),
             bool_ToString(properties->frequencyTraceable),
             bool_ToString(properties->ptpTimescale),
             properties->timeSource);
    return str;
}

#if defined (VTSS_SW_OPTION_P802_1_AS)
char * vtss_ptp_SystemIdentityToString(const vtss_ptp_system_identity_t *systemIdentity, char *str, size_t size)
{
    char str1[40];
    char str2[40];
    snprintf(str, size, "priority1 %u, clockQuality %s, priority2 %u, clockIdentity %s", 
             systemIdentity->priority1,
             ClockQualityToString(&systemIdentity->clockQuality, str1),
             systemIdentity->priority2,
             ClockIdentityToString(systemIdentity->clockIdentity, str2));
    return str;
}

char * vtss_ptp_PriorityVectorToString(const vtss_ptp_priority_vector_t * priority, char *str, size_t size)
{
    char str1[120];
    char str2[40];
    snprintf(str, size, "    systemIdentity:\n      %s,\n    stepsRemoved: %d, sourcePortIdentity: %s.%d, portNumber: %d", 
             vtss_ptp_SystemIdentityToString(&priority->systemIdentity, str1, sizeof(str1)),
             priority->stepsRemoved,
             ClockIdentityToString(priority->sourcePortIdentity.clockIdentity, str2),
             priority->sourcePortIdentity.portNumber,
             priority->portNumber);
    return str;
}

const char *vtss_ptp_InfoIsToString(vtss_ptp_infoIs_t i)
{
    switch (i) {
        case VTSS_PTP_INFOIS_RECEIVED:
            return "Received";
        case VTSS_PTP_INFOIS_MINE:
            return "Mine    ";
        case VTSS_PTP_INFOIS_AGED:
            return "Aged    ";
        case VTSS_PTP_INFOIS_DISABLED:
            return "Disabled";
        default:
            return "?";
    }
}

const char *vtss_ptp_PortRoleToString(vtss_appl_ptp_802_1as_port_role_t i)
{
    switch (i) {
        case VTSS_APPL_PTP_PORT_ROLE_DISABLED_PORT:
            return "Disabled";
        case VTSS_APPL_PTP_PORT_ROLE_MASTER_PORT:
            return "Master  ";
        case VTSS_APPL_PTP_PORT_ROLE_PASSIVE_PORT:
            return "Passive ";
        case VTSS_APPL_PTP_PORT_ROLE_SLAVE_PORT:
            return "Slave   ";
        default:
            return "Unknown ";
    }
}
#endif //(VTSS_SW_OPTION_P802_1_AS)
