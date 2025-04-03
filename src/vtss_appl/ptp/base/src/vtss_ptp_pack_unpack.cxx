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

#include "vtss_ptp_api.h"
#include "vtss_ptp_os.h"
#include "vtss_tod_api.h"
#include "vtss_ptp_internal_types.h"
/*
 * Forward declarations
 */

/*
 * helper functions
 */

void vtss_ptp_pack_header(u8 *buf, const MsgHeader *header)
{
    char str[40];
    buf[PTP_MESSAGE_MESSAGE_TYPE_OFFSET] = (header->messageType & 0x0f) | ((header->transportSpecific & 0x0f)<<4);
    T_IG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"hdr: messageType %d, transportSpecific %d", header->messageType, header->transportSpecific);
    buf[PTP_MESSAGE_VERSION_PTP_OFFSET] = (header->versionPTP & 0x0f) | ((header->minorVersionPTP & 0x0f)<<4);
    T_IG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"hdr: versionPTP %d, minorVersionPTP %d", header->versionPTP, header->minorVersionPTP);
    vtss_tod_pack16(header->messageLength, buf + PTP_MESSAGE_MESSAGE_LENGTH_OFFSET);
    buf[PTP_MESSAGE_DOMAIN_OFFSET] = header->domainNumber;
    buf[PTP_MESSAGE_RESERVED_BYTE_OFFSET] = header->reserved1; /* reserved */
    memset((buf + 16), 0, 4);                                  /* reserved */

    memcpy((buf + PTP_MESSAGE_FLAG_FIELD_OFFSET), header->flagField, 2);
    T_IG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"hdr: flagField %02x %02x", header->flagField[0], header->flagField[1]);
    vtss_tod_pack64(header->correctionField, buf + PTP_MESSAGE_CORRECTION_FIELD_OFFSET);
    T_IG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"hdr: correctionField %s",vtss_tod_TimeInterval_To_String(&header->correctionField,str,0));
    memcpy(buf + PTP_MESSAGE_SOURCE_PORT_ID_OFFSET, header->sourcePortIdentity.clockIdentity,sizeof(vtss_appl_clock_identity));
    vtss_tod_pack16(header->sourcePortIdentity.portNumber,buf + PTP_MESSAGE_SOURCE_PORT_ID_OFFSET + sizeof(vtss_appl_clock_identity));
    T_IG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"hdr: sourcePortIdentity %s Portno = %d",
        ClockIdentityToString (header->sourcePortIdentity.clockIdentity, str), header->sourcePortIdentity.portNumber);
    vtss_tod_pack16(header->sequenceId, buf + PTP_MESSAGE_SEQUENCE_ID_OFFSET);
    T_IG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"hdr: sequenceId %d", header->sequenceId);
    buf[PTP_MESSAGE_CONTROL_FIELD_OFFSET] = header->controlField;
    T_DG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"hdr: controlField %d", header->controlField);
    buf[PTP_MESSAGE_LOGMSG_INTERVAL_OFFSET] = header->logMessageInterval;
    T_DG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"hdr: logMessageInterval %d", header->logMessageInterval);

}

void vtss_ptp_unpack_header(const u8 *buf, MsgHeader *header)
{
    char str [40];

    header->transportSpecific = (buf[PTP_MESSAGE_MESSAGE_TYPE_OFFSET]>>4) & 0x0f;
    T_DG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"transportSpecific %d", header->transportSpecific);
    header->messageType = buf[PTP_MESSAGE_MESSAGE_TYPE_OFFSET] & 0x0f;
    T_DG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"messageType %d", header->messageType);
    header->versionPTP = buf[PTP_MESSAGE_VERSION_PTP_OFFSET] & 0x0f;
    T_DG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"versionPTP %d", header->versionPTP);
    header->messageLength = vtss_tod_unpack16(buf + PTP_MESSAGE_MESSAGE_LENGTH_OFFSET);
    header->domainNumber = buf[PTP_MESSAGE_DOMAIN_OFFSET];
    T_DG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"domainNumber %d", header->domainNumber);
    header->reserved1 = buf[PTP_MESSAGE_RESERVED_BYTE_OFFSET]; /* reserved */
    T_DG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"reserved1 %d", header->reserved1);
    memcpy(header->flagField,buf + PTP_MESSAGE_FLAG_FIELD_OFFSET, 2);
    T_IG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"msgUnpackHeader: flagField %02x %02x", header->flagField[0],header->flagField[1]);
    header->correctionField = vtss_tod_unpack64(buf + PTP_MESSAGE_CORRECTION_FIELD_OFFSET);
    T_IG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"msgUnpackHeader: correctionField %s 0x" VPRI64x " ",vtss_tod_TimeInterval_To_String(&header->correctionField,str,0), header->correctionField);
    memcpy(header->sourcePortIdentity.clockIdentity, (buf + PTP_MESSAGE_SOURCE_PORT_ID_OFFSET), sizeof(vtss_appl_clock_identity));
    header->sourcePortIdentity.portNumber = vtss_tod_unpack16(buf +20+sizeof(vtss_appl_clock_identity));
    T_IG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"msgUnpackHeader: sourcePortIdentity %s Portno = %d",
        ClockIdentityToString (header->sourcePortIdentity.clockIdentity, str), header->sourcePortIdentity.portNumber);
    header->sequenceId = vtss_tod_unpack16(buf + PTP_MESSAGE_SEQUENCE_ID_OFFSET);
    T_IG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"msgUnpackHeader: sequenceId %d", header->sequenceId);
    header->controlField = *(u8*)(buf + PTP_MESSAGE_CONTROL_FIELD_OFFSET);
    T_DG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"controlField %d", header->controlField);
    header->logMessageInterval = buf[PTP_MESSAGE_LOGMSG_INTERVAL_OFFSET];
    T_DG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"logMessageInterval %d", header->logMessageInterval);
}

void vtss_ptp_pack_timestamp(u8 *buf, const mesa_timestamp_t *ptp_time)
{
    vtss_tod_pack16(ptp_time->sec_msb, buf + PTP_MESSAGE_ORIGIN_TIMESTAMP_OFFSET);
    vtss_tod_pack32(ptp_time->seconds, buf + PTP_MESSAGE_ORIGIN_TIMESTAMP_OFFSET + sizeof(ptp_time->sec_msb));
    vtss_tod_pack32(ptp_time->nanoseconds, buf + PTP_MESSAGE_ORIGIN_TIMESTAMP_OFFSET + sizeof(ptp_time->sec_msb) + sizeof(ptp_time->seconds));
}

void vtss_ptp_unpack_timestamp(const u8 *buf, mesa_timestamp_t *ptp_time)
{
    ptp_time->sec_msb = vtss_tod_unpack16(buf + PTP_MESSAGE_ORIGIN_TIMESTAMP_OFFSET);
    ptp_time->seconds = vtss_tod_unpack32(buf + PTP_MESSAGE_ORIGIN_TIMESTAMP_OFFSET  + sizeof(ptp_time->sec_msb));
    ptp_time->nanoseconds = vtss_tod_unpack32(buf + PTP_MESSAGE_ORIGIN_TIMESTAMP_OFFSET + sizeof(ptp_time->sec_msb) + sizeof(ptp_time->seconds));
    ptp_time->nanosecondsfrac = 0; //TO DO : sub nano second support in future
}

void vtss_ptp_pack_correctionfield(u8 *buf, const mesa_timeinterval_t *corr)
{
    vtss_tod_pack64(*corr, buf + PTP_MESSAGE_CORRECTION_FIELD_OFFSET);
}

void vtss_ptp_update_correctionfield(u8 *buf, mesa_timeinterval_t *corr)
{
    char str[40];
    char str1[40];
    mesa_timeinterval_t correctionField = vtss_tod_unpack64(buf + PTP_MESSAGE_CORRECTION_FIELD_OFFSET);
    T_IG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"correctionField %s, corr %s", vtss_tod_TimeInterval_To_String(&correctionField,str,','),
        vtss_tod_TimeInterval_To_String(corr,str1,','));
    correctionField += *corr;
    vtss_tod_pack64(correctionField, buf + PTP_MESSAGE_CORRECTION_FIELD_OFFSET);
    T_IG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"new correctionField %s", vtss_tod_TimeInterval_To_String(&correctionField,str,','));
}

void vtss_ptp_pack_sourceportidentity(u8 *buf, const vtss_appl_ptp_port_identity *sourcePortIdentity)
{
    char str[40];
    memcpy(buf + PTP_MESSAGE_SOURCE_PORT_ID_OFFSET, sourcePortIdentity->clockIdentity,sizeof(vtss_appl_clock_identity));
    vtss_tod_pack16(sourcePortIdentity->portNumber,buf + PTP_MESSAGE_SOURCE_PORT_ID_OFFSET + sizeof(vtss_appl_clock_identity));
    T_IG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"hdr: sourcePortIdentity %s Portno = %d",
         ClockIdentityToString (sourcePortIdentity->clockIdentity, str), sourcePortIdentity->portNumber);
}


/*
 * Common pack function for Sync, Delay_Req and Follow_Up messages
 */
void vtss_ptp_pack_msg44(u8 *buf, const MsgHeader *header, const mesa_timestamp_t *originTimestamp)
{
    char str[50];
    /* header part */
    vtss_ptp_pack_header(buf, header);
    /* according to 802.1as.d7 messageTypeSpecific = 0 */
    vtss_tod_pack32(0 ,buf + PTP_MESSAGE_RESERVED_FOR_TS_OFFSET);
    /* Sync message specific part */
    vtss_ptp_pack_timestamp(buf, originTimestamp);
    T_IG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"originTimeStamp %s", TimeStampToString(originTimestamp,str));
}

/*
 * Common pack function for Pdelay_Req, Pdelay_Resp and Pdelay_Resp_Follow_Up messages
 */
void vtss_ptp_pack_msg_pdelay_xx(u8 *buf, MsgHeader *header, mesa_timestamp_t *timestamp, vtss_appl_ptp_port_identity *port_id)
{
    char str[50];
    /* header part */
    vtss_ptp_pack_header(buf, header);
    /* according to 802.1as.d7 messageTypeSpecific = 0 */
    vtss_tod_pack32(0 ,buf + PTP_MESSAGE_RESERVED_FOR_TS_OFFSET);
    /* Sync message specific part */
    vtss_ptp_pack_timestamp(buf, timestamp);
    T_IG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"timestamp %s", TimeStampToString(timestamp,str));
    memcpy((buf + PTP_MESSAGE_PDELAY_PORT_ID_OFFSET), port_id->clockIdentity, sizeof(vtss_appl_clock_identity)); /* requestingPortIdentity */
    vtss_tod_pack16(port_id->portNumber, buf + PTP_MESSAGE_PDELAY_PORT_ID_OFFSET + sizeof(vtss_appl_clock_identity));
    T_IG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"port_id->clockIdentity %s", ClockIdentityToString(port_id->clockIdentity, str));
    T_IG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"port_id->portNumber %d", port_id->portNumber);
}

/*
 * Common unpack function for Pdelay_Req, Pdelay_Resp and Pdelay_Resp_Follow_Up messages
 */
void vtss_ptp_unpack_msg_pdelay_xx(u8 *buf, mesa_timestamp_t *timestamp, vtss_appl_ptp_port_identity *port_id)
{
    timestamp->sec_msb = vtss_tod_unpack16(buf+ + PTP_MESSAGE_PDELAY_TIMESTAMP_OFFSET);
    timestamp->seconds = vtss_tod_unpack32(buf + PTP_MESSAGE_PDELAY_TIMESTAMP_OFFSET + sizeof(timestamp->sec_msb));
    timestamp->nanoseconds = vtss_tod_unpack32(buf + PTP_MESSAGE_PDELAY_TIMESTAMP_OFFSET + sizeof(timestamp->sec_msb) + sizeof(timestamp->seconds));
    timestamp->nanosecondsfrac = 0;
    memcpy(port_id->clockIdentity, (buf + PTP_MESSAGE_PDELAY_PORT_ID_OFFSET), sizeof(vtss_appl_clock_identity));
    port_id->portNumber = vtss_tod_unpack16(buf + PTP_MESSAGE_PDELAY_PORT_ID_OFFSET+sizeof(vtss_appl_clock_identity));
    T_DG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"requestingPortIdentity.portNumber %d", port_id->portNumber);
}


void vtss_ptp_unpack_delay_resp(const u8 *buf, MsgDelayResp *resp)
{
    char str [50];
    resp->receiveTimestamp.sec_msb = vtss_tod_unpack16(buf + PTP_MESSAGE_RECEIVE_TIMESTAMP_OFFSET);
    resp->receiveTimestamp.seconds = vtss_tod_unpack32(buf + PTP_MESSAGE_RECEIVE_TIMESTAMP_OFFSET + sizeof(resp->receiveTimestamp.sec_msb));
    resp->receiveTimestamp.nanoseconds = vtss_tod_unpack32(buf + PTP_MESSAGE_RECEIVE_TIMESTAMP_OFFSET + sizeof(resp->receiveTimestamp.sec_msb) + sizeof(resp->receiveTimestamp.seconds));
    resp->receiveTimestamp.nanosecondsfrac = 0;
    T_IG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"receiveTimestamp %s", TimeStampToString(&resp->receiveTimestamp,str));
    memcpy(resp->requestingPortIdentity.clockIdentity, (buf + PTP_MESSAGE_REQ_PORT_ID_OFFSET), sizeof(vtss_appl_clock_identity));
    T_DG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"requestingPortIdentity.clockIdentity %s",
        ClockIdentityToString (resp->requestingPortIdentity.clockIdentity, str));
    resp->requestingPortIdentity.portNumber = vtss_tod_unpack16(buf + PTP_MESSAGE_REQ_PORT_ID_OFFSET+sizeof(vtss_appl_clock_identity));
    T_DG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"requestingPortIdentity.portNumber %d", resp->requestingPortIdentity.portNumber);
}

void vtss_ptp_pack_announce(u8 *buf, const mesa_timestamp_t *originTimestamp, ptp_clock_t *ptp_clock, u16 sequenceNo, AnnounceDS * ann_ds)
{
    char str [40];


    // The following special processing is only done if the clock is itself the grand master. Otherwise the UTC offset value, leap_59, leap_51 etc. in ann_ds is assumed to be correct already.
    //
    // If a leap date is configured, check originTimestamp against configured leapDate and compensate ann_ds->currentUtcOffset if needed.
    //
    // Timestamp is in PTP epoch so it must be converted to UTC. This is done using the configured UTC offset.
    // If the resulting UTC time is before the leap event then the result can be assumed to be OK as is.
    // If the resulting UTC time is exactly at the time of the leap event or after it, the calculated UTC time must be compensated with the leap event
    // i.e. a leap second must either be added or subtracted. This implies that the UTC offset value should be compensated in the same way.
    if (!memcmp(ann_ds->grandmaster_identity, ptp_clock->defaultDS.status.clockIdentity, sizeof(vtss_appl_clock_identity))) {
        T_DG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK, "clock is itself the grandmaster ");
        
        ptp_clock->timepropertiesDS.currentUtcOffset = ptp_clock->time_prop->currentUtcOffset;
        ptp_clock->timepropertiesDS.leap59 = ptp_clock->time_prop->leap59;
        ptp_clock->timepropertiesDS.leap61 = ptp_clock->time_prop->leap61;

        if (ptp_clock->time_prop->pendingLeap) {
            
            // Convert leap date to seconds elapsed since 1970-01-01 
            u64 leap_date_in_secs = (u64)ptp_clock->time_prop->leapDate * 86400;

            // While taking into account the configured UTC offset, compare the leap date against the value of originTimestamp
            if ((((u64)originTimestamp->sec_msb << 32) + originTimestamp->seconds) >= leap_date_in_secs + ptp_clock->time_prop->currentUtcOffset) {
                
                if (ptp_clock->time_prop->leapType == VTSS_APPL_PTP_LEAP_SECOND_59) {
                    ptp_clock->timepropertiesDS.currentUtcOffset = ptp_clock->time_prop->currentUtcOffset - 1;
                }
                else {
                    ptp_clock->timepropertiesDS.currentUtcOffset = ptp_clock->time_prop->currentUtcOffset + 1;
                }
            }
            else if ((((u64)originTimestamp->sec_msb << 32) + originTimestamp->seconds + 86400) >= leap_date_in_secs + ptp_clock->time_prop->currentUtcOffset) {  // Less than one day until leap-event
                if (ptp_clock->timepropertiesDS.leapType == VTSS_APPL_PTP_LEAP_SECOND_59) {
                    ptp_clock->timepropertiesDS.leap59 = true;
                    ptp_clock->timepropertiesDS.leap61 = false;
                }
                else {
                    ptp_clock->timepropertiesDS.leap59 = false;
                    ptp_clock->timepropertiesDS.leap61 = true;
                }
            }
        }

        ann_ds->currentUtcOffset = ptp_clock->timepropertiesDS.currentUtcOffset;

        /* header part only variable part is packed */
        /* flag field */
        u8 flags =
            (ptp_clock->timepropertiesDS.leap61 ? PTP_LI_61 : 0) |
            (ptp_clock->timepropertiesDS.leap59 ? PTP_LI_59 : 0) |
            (ptp_clock->timepropertiesDS.currentUtcOffsetValid ? PTP_CURRENT_UTC_OFFSET_VALID :0) |
            (ptp_clock->timepropertiesDS.ptpTimescale ? PTP_PTP_TIMESCALE : 0) |
            (ptp_clock->timepropertiesDS.timeTraceable ? PTP_TIME_TRACEABLE : 0) |
            (ptp_clock->timepropertiesDS.frequencyTraceable ? PTP_FREQUENCY_TRACEABLE : 0);

        ann_ds->flags[1] = flags;
    }

    /* header part only variable part is packed */
    /* flag field */
    buf[PTP_MESSAGE_FLAG_FIELD_OFFSET+1] = ann_ds->flags[1];
    T_DG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"flagField 0x%02x 0x%02x", buf[PTP_MESSAGE_FLAG_FIELD_OFFSET], buf[PTP_MESSAGE_FLAG_FIELD_OFFSET+1]);
    
    // domain number configuration may change 
    buf[PTP_MESSAGE_DOMAIN_OFFSET] = ptp_clock->clock_init->cfg.domainNumber;
    T_IG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"domainNumber %d", ptp_clock->clock_init->cfg.domainNumber);
    
    vtss_tod_pack16(sequenceNo, buf + PTP_MESSAGE_SEQUENCE_ID_OFFSET); /* sequence ID */
    T_IG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"sequenceNo %d", sequenceNo);

    /* Announce message specific part */
    if (ptp_clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS) {
        /* Ensure the reserved fields are zero in announce messages to comply with 802.1as-2020 and AVNU case 10.1a */
        mesa_timestamp_t timestamp = {0,0,0};
        vtss_ptp_pack_timestamp(buf, &timestamp);
        buf[PTP_MESSAGE_RESERVED_OFFSET] = 0; /* reservedField */
        buf[PTP_MESSAGE_RESERVED_OFFSET+1] = 0; /* reservedField */
    } else {
        vtss_ptp_pack_timestamp(buf, originTimestamp);
    }

    vtss_tod_pack16(ann_ds->currentUtcOffset, buf + PTP_MESSAGE_CURRENT_UTC_OFFSET); /* currentUtcOffset */
    T_IG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"currentUtcOffset %d", ann_ds->currentUtcOffset);
    
    buf[PTP_MESSAGE_GM_PRI1_OFFSET] = ann_ds->grandmaster_priority1;  /* grandmasterPriority1 */
    T_IG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"grandmasterPriority1 %d", ann_ds->grandmaster_priority1);
    buf[PTP_MESSAGE_GM_CLOCK_Q_OFFSET] = ann_ds->grandmaster_clockQuality.clockClass;  /* grandmasterClockQuality */
    buf[PTP_MESSAGE_GM_CLOCK_Q_OFFSET+1] = ann_ds->grandmaster_clockQuality.clockAccuracy;  /* grandmasterClockQuality */
    vtss_tod_pack16(ann_ds->grandmaster_clockQuality.offsetScaledLogVariance, buf + PTP_MESSAGE_GM_CLOCK_Q_OFFSET + 2); /* grandmasterClockQuality */
    T_IG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"grandmasterClockQuality %d,%d,%d", ann_ds->grandmaster_clockQuality.clockClass,
         ann_ds->grandmaster_clockQuality.clockAccuracy, ann_ds->grandmaster_clockQuality.offsetScaledLogVariance);

    buf[PTP_MESSAGE_GM_PRI2_OFFSET] = ann_ds->grandmaster_priority2;  /* grandmasterPriority2 */
    T_IG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"grandmasterPriority2 %d", ann_ds->grandmaster_priority2);
    memcpy((buf + PTP_MESSAGE_GM_IDENTITY_OFFSET), ann_ds->grandmaster_identity, sizeof(vtss_appl_clock_identity)); /* grandmasterIdentity */
    T_IG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"grandmasterIdentity %s",
         ClockIdentityToString (ann_ds->grandmaster_identity, str));
    vtss_tod_pack16(ann_ds->steps_removed, buf + PTP_MESSAGE_STEPS_REMOVED_OFFSET); /* stepsRemoved */
    T_IG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"stepsRemoved %d", ann_ds->steps_removed);
    buf[PTP_MESSAGE_TIME_SOURCE_OFFSET] = ann_ds->time_source;  /* timeSource */
    T_IG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"timeSource %d", ann_ds->time_source);

}

void vtss_ptp_unpack_announce(const u8 *buf, ForeignMasterDS *announce)
{
    char str [40];
    /* from header */
    memcpy(announce->ds.sourcePortIdentity.clockIdentity, (buf + PTP_MESSAGE_SOURCE_PORT_ID_OFFSET), sizeof(vtss_appl_clock_identity));
    T_DG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"sourcePortIdentity.ds.clockIdentity %s",
        ClockIdentityToString (announce->ds.sourcePortIdentity.clockIdentity, str));
    announce->ds.sourcePortIdentity.portNumber = vtss_tod_unpack16(buf + PTP_MESSAGE_SOURCE_PORT_ID_OFFSET + sizeof(vtss_appl_clock_identity));
    T_DG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"sourcePortIdentity.portNumber %d", announce->ds.sourcePortIdentity.portNumber);
    memcpy(announce->flagField, buf + PTP_MESSAGE_FLAG_FIELD_OFFSET, sizeof(announce->flagField));
    T_DG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"flagField %02x %02x", announce->flagField[0], announce->flagField[1]);
    /* originTimeStamp not used*/
    announce->currentUtcOffset = (i16)vtss_tod_unpack16(buf + PTP_MESSAGE_CURRENT_UTC_OFFSET);
    T_DG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"currentUTCOffset %d", announce->currentUtcOffset);

    announce->ds.priority1 = buf[PTP_MESSAGE_GM_PRI1_OFFSET];
    T_DG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"grandmasterPriority1 %d", announce->ds.priority1);
    announce->ds.clockQuality.clockClass = buf[PTP_MESSAGE_GM_CLOCK_Q_OFFSET];
    announce->ds.clockQuality.clockAccuracy = buf[PTP_MESSAGE_GM_CLOCK_Q_OFFSET+1];
    announce->ds.clockQuality.offsetScaledLogVariance = vtss_tod_unpack16(buf + PTP_MESSAGE_GM_CLOCK_Q_OFFSET+2);
    T_DG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"grandmasterClockQuality %d,%d,%d", announce->ds.clockQuality.clockClass,
        announce->ds.clockQuality.clockAccuracy, announce->ds.clockQuality.offsetScaledLogVariance);
    announce->ds.priority2 = buf[PTP_MESSAGE_GM_PRI2_OFFSET];
    T_DG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"grandmasterPriority2 %d", announce->ds.priority2);
    memcpy(announce->ds.grandmasterIdentity, (buf + PTP_MESSAGE_GM_IDENTITY_OFFSET), sizeof(vtss_appl_clock_identity));
    T_DG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"grandmasterIdentity %s",
        ClockIdentityToString (announce->ds.grandmasterIdentity, str));
    announce->ds.stepsRemoved = vtss_tod_unpack16(buf + PTP_MESSAGE_STEPS_REMOVED_OFFSET);
    T_DG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"stepsRemoved %d", announce->ds.stepsRemoved);
    announce->timeSource = buf[PTP_MESSAGE_TIME_SOURCE_OFFSET];
    T_DG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"timeSource %d", announce->timeSource);
}

void vtss_ptp_unpack_announce_msg(const u8 *buf, MsgAnnounce *announce)
{
    char str [40];
    /* originTimeStamp not used*/
    //vtss_ptp_unpack_timestamp(buf, &announce->originTimestamp);
    announce->currentUtcOffset = (i16)vtss_tod_unpack16(buf + PTP_MESSAGE_CURRENT_UTC_OFFSET);
    T_DG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"currentUTCOffset %d", announce->currentUtcOffset);
    announce->grandmasterPriority1 = buf[PTP_MESSAGE_GM_PRI1_OFFSET];
    T_DG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"grandmasterPriority1 %d", announce->grandmasterPriority1);
    announce->grandmasterClockQuality.clockClass = buf[PTP_MESSAGE_GM_CLOCK_Q_OFFSET];
    announce->grandmasterClockQuality.clockAccuracy = buf[PTP_MESSAGE_GM_CLOCK_Q_OFFSET+1];
    announce->grandmasterClockQuality.offsetScaledLogVariance = vtss_tod_unpack16(buf + PTP_MESSAGE_GM_CLOCK_Q_OFFSET+2);
    T_DG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"grandmasterClockQuality %d,%d,%d", announce->grandmasterClockQuality.clockClass,
         announce->grandmasterClockQuality.clockAccuracy, announce->grandmasterClockQuality.offsetScaledLogVariance);
    announce->grandmasterPriority2 = buf[PTP_MESSAGE_GM_PRI2_OFFSET];
    T_DG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"grandmasterPriority2 %d", announce->grandmasterPriority2);
    memcpy(announce->grandmasterIdentity, (buf + PTP_MESSAGE_GM_IDENTITY_OFFSET), sizeof(vtss_appl_clock_identity));
    T_DG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"grandmasterIdentity %s",
         ClockIdentityToString (announce->grandmasterIdentity, str));
    announce->stepsRemoved = vtss_tod_unpack16(buf + PTP_MESSAGE_STEPS_REMOVED_OFFSET);
    T_DG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"stepsRemoved %d", announce->stepsRemoved);
    announce->timeSource = buf[PTP_MESSAGE_TIME_SOURCE_OFFSET];
    T_DG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"timeSource %d", announce->timeSource);
}

void vtss_ptp_unpack_steps_removed(const u8 *buf, u16 *steps_removed)
{
    *steps_removed = vtss_tod_unpack16(buf + PTP_MESSAGE_STEPS_REMOVED_OFFSET);
}

void vtss_ptp_pack_transparent_follow_up(u8 *buf, MsgHeader *header,
                                mesa_timestamp_t *preciseOriginTimestamp, mesa_timeinterval_t *correctionValue)
{
    char str[50];
    /* update header part (all other header values are already defined*/
    header->messageType = PTP_MESSAGE_TYPE_FOLLOWUP;  /* messageType */
    header->messageLength = FOLLOW_UP_PACKET_LENGTH; /* messageLength */
    header->correctionField = *correctionValue; /* correctionField */
    header->controlField = PTP_FOLLOWUP_MESSAGE;  /* control */
    vtss_ptp_pack_header(buf, header);
    /* FollowUp message specific part */
    vtss_ptp_pack_timestamp(buf, preciseOriginTimestamp);
    T_IG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"originTimeStamp %s", TimeStampToString(preciseOriginTimestamp,str));
    T_IG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"correctionValue %s", vtss_tod_TimeInterval_To_String(correctionValue,str,','));

}

void vtss_ptp_update_flags(u8 *buf, u8 *flags)
{
    memcpy((buf + PTP_MESSAGE_FLAG_FIELD_OFFSET), flags, 2);
}

void vtss_ptp_pack_signalling(u8 *buf, const vtss_appl_ptp_port_identity *targetPortIdentity, const vtss_appl_ptp_port_identity *sourcePortIdentity,
                              u8 domain, u8 sdoId, u16 version, u16 minorVersion, u16 sequenceId, u16 packetLength, u8 flag, u8 controlField)
{
    MsgHeader header;
    header.messageType = PTP_MESSAGE_TYPE_SIGNALLING;
    header.transportSpecific = sdoId;
    header.versionPTP = version;
    header.minorVersionPTP = minorVersion;
    header.messageLength = packetLength;
    header.domainNumber = domain;
    header.reserved1 = MINOR_SDO_ID;
    header.flagField[0] = flag;
    header.flagField[1] = 0;
    header.correctionField = 0;
    header.sourcePortIdentity = *sourcePortIdentity;
    header.sequenceId = sequenceId;
    header.controlField = controlField;
    header.logMessageInterval = 0x7F;
    vtss_ptp_pack_header(buf, &header);
    /* Signalling message specific part */
    memcpy((buf + PTP_MESSAGE_SIGNAL_TARGETPORTIDENTITY), targetPortIdentity->clockIdentity, sizeof(vtss_appl_clock_identity)); /* targetPortIdentity */
    vtss_tod_pack16(targetPortIdentity->portNumber, buf + PTP_MESSAGE_SIGNAL_TARGETPORTIDENTITY + sizeof(vtss_appl_clock_identity));        /* Target portno */
}

void vtss_ptp_unpack_signalling(const u8 *buf, MsgSignalling *resp)
{
    char str [40];
    memcpy(resp->targetPortIdentity.clockIdentity, (buf + PTP_MESSAGE_SIGNAL_TARGETPORTIDENTITY), sizeof(vtss_appl_clock_identity));
    T_DG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"targetPortIdentity.clockIdentity %s",
        ClockIdentityToString (resp->targetPortIdentity.clockIdentity, str));
    resp->targetPortIdentity.portNumber = vtss_tod_unpack16(buf + PTP_MESSAGE_SIGNAL_TARGETPORTIDENTITY + sizeof(vtss_appl_clock_identity));
    T_DG(VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK,"targetPortIdentity.portNumber %d", resp->targetPortIdentity.portNumber);
}


int vtss_ptp_pack_tlv(u8 *buf, u16 length, const TLV *tlv)
{
    if (length >=TLV_HEADER_SIZE+tlv->lengthField) {
        vtss_tod_pack16(tlv->tlvType,buf);
        vtss_tod_pack16(tlv->lengthField,buf+sizeof(tlv->tlvType));
        memcpy(buf+TLV_HEADER_SIZE, tlv->valueField, tlv->lengthField);
        return VTSS_RC_OK;
    }
    return VTSS_RC_ERROR;
}

int vtss_ptp_unpack_tlv(const u8 *buf, u16 length, TLV *tlv)
{
    tlv->tlvType = vtss_tod_unpack16(buf);
    tlv->lengthField = vtss_tod_unpack16(buf+sizeof(tlv->tlvType));
    if ((tlv->tlvType > 0) && (tlv->lengthField > 0) && (length >= TLV_HEADER_SIZE+tlv->lengthField)) {
        tlv->valueField = buf + TLV_HEADER_SIZE;
        return VTSS_RC_OK;
    }
    return VTSS_RC_ERROR;
}



