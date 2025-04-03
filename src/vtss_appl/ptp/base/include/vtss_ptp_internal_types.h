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

/*
 Microchip is aware that some terminology used in this technical document is
 antiquated and inappropriate. As a result of the complex nature of software
 where seemingly simple changes have unpredictable, and often far-reaching
 negative results on the software's functionality (requiring extensive retesting
 and revalidation) we are unable to make the desired changes in all legacy
 systems without compromising our product or our clients' products.
*/

#ifndef VTSS_PTP_INTERNAL_TYPES_H
#define VTSS_PTP_INTERNAL_TYPES_H

#include "vtss_ptp_sys_timer.h"
#include "vtss_ptp_port.h"

#define TICK_SIZE 15
extern const u16 Ticktable [TICK_SIZE];
extern const u32 TickTimeNs;  // timer tick period in nanosec

/* no support for intervals less than -PTP_LOG_TICKS_PR_SEC */
#define PTP_LOG_TIMEOUT(x)                                                    \
  (((x) + PTP_LOG_TICKS_PR_SEC) < 0 ? 0 :                                     \
  ((((x) + PTP_LOG_TICKS_PR_SEC) >= TICK_SIZE) ? (Ticktable[TICK_SIZE - 1]) : \
  Ticktable[(x) + PTP_LOG_TICKS_PR_SEC]))

#define PTP_SYNC_RECEIPT_TIMEOUT(x)         (10*PTP_LOG_TIMEOUT(x))
#define PTP_FOREIGN_MASTER_THRESHOLD        2
#define PTP_FOREIGN_MASTER_TIME_WINDOW(x)   (4*(PTP_LOG_TIMEOUT(x)))

/**
 * \brief message header control field values
 */
enum {
    PTP_SYNC_MESSAGE=0,  PTP_DELAY_REQ_MESSAGE,  PTP_FOLLOWUP_MESSAGE,
    PTP_DELAY_RESP_MESSAGE,  PTP_MANAGEMENT_MESSAGE,
    PTP_ALL_OTHERS
};

/**
 * \brief TLV Type field values used in this implementation
 */
#define  TLV_ORGANIZATION_EXTENSION                   0x3
#define  REQUEST_UNICAST_TRANSMISSION                 0x4
#define  GRANT_UNICAST_TRANSMISSION                   0x5
#define  CANCEL_UNICAST_TRANSMISSION                  0x6
#define  ACKNOWLEDGE_CANCEL_UNICAST_TRANSMISSION      0x7
#define  TLV_PATH_TRACE                               0x8
#define  TLV_ORGANIZXATION_EXTENSION_DO_NOT_PROPAGATE 0x8000

/**
 * \brief version 2 flags octet 0 defines
 */
#define  PTP_ALTERNATE_MASTER_FLAG      0x01
#define  PTP_TWO_STEP_FLAG              0x02
#define  PTP_UNICAST_FLAG               0x04
#define  PTP_RESERVED_0_3_FLAG          0x08
#define  PTP_RESERVED_0_4_FLAG          0x10
#define  PTP_PROFILE_SPEC_1_FLAG        0x20
#define  PTP_PROFILE_SPEC_2_FLAG        0x40
#define  PTP_RESERVED_0_7_FLAG          0x80

/**
 * \brief version 2 flags octet 1 defines
 */
#define  PTP_LI_61                      0x01
#define  PTP_LI_59                      0x02
#define  PTP_CURRENT_UTC_OFFSET_VALID   0x04
#define  PTP_PTP_TIMESCALE              0x08
#define  PTP_TIME_TRACEABLE             0x10
#define  PTP_FREQUENCY_TRACEABLE        0x20
#define  PTP_RESERVED_1_6_FLAE          0x40
#define  PTP_RESERVED_1_7_FLAG          0x80

/* enum used by this implementation */



#define PACKET_SIZE  80

#define HEADER_LENGTH             34
#define ANNOUNCE_PACKET_LENGTH    64
#define SYNC_PACKET_LENGTH        44
#define DELAY_REQ_PACKET_LENGTH   44
#define FOLLOW_UP_PACKET_LENGTH   44
#define DELAY_RESP_PACKET_LENGTH  54
#define P_DELAY_REQ_PACKET_LENGTH  54
#define P_DELAY_RESP_PACKET_LENGTH  54
#define P_DELAY_RESP_FOLLOW_UP_PACKET_LENGTH  54
#define SIGNALLING_MIN_PACKET_LENGTH  44
#define REQUEST_UNICAST_TRANSMITTION_TLV_LENGTH 10
#define GRANT_UNICAST_TRANSMITTION_TLV_LENGTH 12
#define CANCEL_UNICAST_TRANSMITTION_TLV_LENGTH 6
#define ACKNOWLEDGE_CANCEL_UNICAST_TRANSMITTION_TLV_LENGTH 6
#define IEEE802_VALID_FRAME_LENGTH 1500

#define MESSAGE_INTERVAL_REQUEST_TLV_LENGTH 16

#define GPTP_CAPABLE_TLV_LENGTH 16

#define GPTP_CAPABLE_MESSAGE_INTERVAL_REQUEST_TLV_LENGTH 14

#define TLV_HEADER_SIZE             4

#define ENCAPSULATION_SIZE          42

#define FOLLOW_UP_TLV_LENGTH 32

// Default Ethernet header length and tag length.
#define ETH_HDR_LEN                 14
#define ETH_TAG_LEN                 4

/**
 * \brief Transparent clock port states
 */
#define FOLLOW_UP_NO_ACTION     0
#define FOLLOW_UP_CREATE        1 // create a followup when sync is transmitted.
#define FOLLOW_UP_WAIT_TX       2 // waiting for TX done or followup
#define FOLLOW_UP_WAIT_READY    3 // TX done, waiting for followup
#define FOLLOW_UP_WAIT_TX_READY 4 // followup received, waiting for TX done

#define VTSS_MAX_PORT_CNT 64    /* used as array dimension where u64 is used as a portmask. */

#define MINOR_SDO_ID            0    /* minor sdoId defined in G802.1AS clause 8.1, and encoded in the one byte reserved field in the message header */
#define MAJOR_SDOID_802_1AS     1    /* major sdoId defined in G802.1AS clause 8.1, and encoded in the transportSpecific field in the message header */
#define MAJOR_SDOID_OTHER       0    /* in other profiles than 802.1AS: transportSpecific field in the message header */
#define MAJOR_SDOID_CMLDS_802_1AS 2  /* major sdoId defined in G802.1AS clause 11.2.15 */
/**
 * \brief Check if sequence id x > y, or wrapped.
 */
static inline bool SEQUENCE_ID_CHECK(u16 x,u16 y)
{
    u16 z= x-y;
    return (z > 0 && z < 0x8000);
}

/* Message header */
typedef struct MsgHeader_s {
    u16  versionPTP;
    u16 minorVersionPTP;
    u8  transportSpecific;
    u8  messageType;
    u16 messageLength;
    u8  domainNumber;
    u8  reserved1;
    u8 flagField[2];
    mesa_timeinterval_t correctionField;
    vtss_appl_ptp_port_identity sourcePortIdentity;
    u16  sequenceId;
    u8  controlField;
    i8  logMessageInterval;
} MsgHeader;

/* Announce Message */
typedef struct MsgAnnounce_s {
    mesa_timestamp_t            originTimestamp;
    i16                         currentUtcOffset;
    u8                          grandmasterPriority1;
    vtss_appl_clock_quality     grandmasterClockQuality;
    u8                          grandmasterPriority2;
    vtss_appl_clock_identity    grandmasterIdentity;
    u16                         stepsRemoved;
    u8                          timeSource;
} MsgAnnounce;

/* Sync or Delay_Req message */
/* Sync or Delay_Req message */
typedef struct {
    mesa_timestamp_t  originTimestamp;
} MsgSync;

/* Delay_Resp message */
typedef struct {
    mesa_timestamp_t  receiveTimestamp;
    vtss_appl_ptp_port_identity requestingPortIdentity;
} MsgDelayResp;

/* Signalling message */
typedef struct {
    vtss_appl_ptp_port_identity targetPortIdentity;
} MsgSignalling;

/* TLV type */
typedef struct {
    u16 tlvType;
    u16 lengthField;
    const u8*     valueField;
} TLV;

/* main program data structure */

/**
 * PTP version 2 data types
 *
 */

typedef struct {
    /* dynamic */
    u16 stepsRemoved;
    mesa_timeinterval_t offsetFromMaster;
    mesa_timeinterval_t meanPathDelay;
    bool delayOk;           /* true indicates that meanPathDelay is ok (E2E slave ports)*/
    vtss_ptp_clock_mode_t clock_mode;
    u32 lock_period;
} CurrentDS;

typedef struct {
    MsgHeader   syncForwardingHeader;    /* Master identity */
    MsgSync syncForwardingcontent;
    u8 sync_followup_action;
    mesa_timestamp_t sync_ingress_time;
    mesa_timeinterval_t syncResidenceTime[VTSS_MAX_PORT_CNT];
    mesa_timeinterval_t rx_delay_asy;   /* delay asymmetry for the port where the sync packet is received, used when sending the followup */
    i8 age;
    vtss_appl_ptp_protocol_adr_t sender;
    u8 msgFbuf[PACKET_SIZE];  /* used when forwarding packets in a transparent clock */
    u32 header_length;
    u64  port_mask;                 /* indicates which ports the sync is forwarded to */
    struct ptp_clock_s *parent;         /* pointer to clock instance data */
    /* 2-step tx timestamp callback contexts */
    ptp_tx_timestamp_context_t sync_ts_context;
} SyncOutstandingListEntry;

#define SYNC_2STEP_MAX_OUTSTANDING_TIME 20
#define IN_USE_SYNC_FREE 0
#define IN_USE_SYNC_WAIT_FOLLOW 1
#define IN_USE_SYNC_WAIT_TX 2
/*
 * List of outstanding Sync requests (2 step), there is a list pr port.
 */
typedef struct {
    u16 listSize;
    SyncOutstandingListEntry *list;
} SyncOutstandingList;

typedef struct {
    vtss_appl_ptp_port_identity sourcePortIdentity;
    u8 inUse;
    u16 sequenceId;
    PtpPort_t *originPtpPort;                     /* port that DelReq is received from */
    mesa_timestamp_t delayReqRxTime;
    mesa_timestamp_t delayReqTxTime[VTSS_MAX_PORT_CNT];
    u8 msgFbuf[PACKET_SIZE];  /* used when forwarding packets in a transparent clock */
    u16 masterPort;
    vtss_appl_ptp_protocol_adr_t sender;
    vtss_appl_ptp_protocol_adr_t rsp_sender;
    i8 age;
    struct ptp_clock_s *parent;         /* pointer to clock instance data */
    /* 2-step tx timestamp callback contexts */
    ptp_tx_timestamp_context_t del_req_ts_context;
} DelayReqListEntry;

#define DELAY_REQ_E2E_MAX_OUTSTANDING_TIME 20
#define IN_USE_FREE 0
#define IN_USE_WAIT_RESP 1
#define IN_USE_WAIT_TX 2

typedef struct {
    u16 listSize;
    DelayReqListEntry *list;
} DelayReqList;

typedef struct {
    struct ptp_clock_s *clock;     /* pointer to clock data */
    DelayReqList outstanding_delay_req_list;
    SyncOutstandingList sync_outstanding_list;
    /* timer for ageing outstanding messages*/
    vtss_ptp_sys_timer_t age_timer;
} ptp_tc_t;

/*
 * internal functions
 */

/**
 * \brief BMC Recommended State
 */
typedef enum {
    VTSS_PTP_BMC_UNCHANGED,     /* no state change required */
    VTSS_PTP_BMC_MASTER_M1,
    VTSS_PTP_BMC_MASTER_M2,
    VTSS_PTP_BMC_MASTER_M3,
    VTSS_PTP_BMC_SLAVE,
    VTSS_PTP_BMC_UNCALIBRATED,
    VTSS_PTP_BMC_PASSIVE,
} vtss_ptp_bmc_recommended_state_t;

typedef struct {
    u32 cumulativeScaledRateOffset;         /*  */
    u16 gmTimeBaseIndicator;                /*  */
    mesa_scaled_ns_t lastGmPhaseChange;     /*  */
    i32 scaledLastGmFreqChange;             /*  */
} ptp_follow_up_tlv_info_t;

typedef struct {
    MsgHeader   syncForwardingHeader;    /* Master identity */
    u8 sync_followup_action;
    u64 sync_ingress_time;
    vtss_timeinterval_t syncResidenceTime[VTSS_MAX_PORT_CNT];
    vtss_timeinterval_t rx_delay_asy;   /* delay asymmetry for the port where the sync packet is received, used when sending the followup */
    double rateRatio;
    u8 msgFbuf[PACKET_SIZE];  /* used when forwarding packets in a transparent clock */
    size_t msgFbuf_size;      /* actual size of the Followup packet */
    u64  port_mask;                 /* indicates which ports the sync is forwarded to */
    /* 2-step tx timestamp callback contexts */
    ptp_tx_timestamp_context_t sync_ts_context;
    size_t               last_ptp_sync_msg_size; /* save last sync msg size for use during sync-timeout period*/
    mesa_timestamp_t     last_sync_tx_time;
    mesa_timestamp_t     last_sync_ingress_time; /* save last sync ingress time for use during sync-timeout period */
} vtss_ptp_port_sync_sync_t;





#endif
