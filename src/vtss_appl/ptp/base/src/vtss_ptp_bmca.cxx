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

#include "vtss_ptp_bmca.h"
#include "vtss_ptp_os.h"
#include "vtss_ptp_api.h"
#include "vtss_ptp_internal_types.h"
#include "vtss_ptp_pack_unpack.h"
#include "vtss/appl/ptp.h"
#include "vtss_ptp_802_1as_bmca.h"

#if defined(VTSS_SW_OPTION_SYNCE)
#include "synce_ptp_if.h"
#endif

#if 1
static void dump_ClockDataSet(const ClockDataSet *ds, const vtss_appl_ptp_port_identity *rec)
{
    char str1 [40];
    char str2 [40];

    T_NG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"ClockDataSet: %s %d %d %d %s %d %d\n",
           ClockIdentityToString(ds->grandmasterIdentity, str1),
           ds->priority1, ds->priority2,
           ds->stepsRemoved,
           ClockIdentityToString(ds->sourcePortIdentity.clockIdentity, str2),
           ds->sourcePortIdentity.portNumber,
           ds->localPriority);
    T_NG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"ClockQuality : %s Receive port: %s %d\n",
           ClockQualityToString(&ds->clockQuality, str2),
           ClockIdentityToString(rec->clockIdentity, str1),
           rec->portNumber);
}
#endif

static ClockDataSet *addForeign(const u8*,MsgHeader*, ptp_clock_t *, PtpPort_t *);

/* see spec table 13 */
void vtss_ptp_bmca_m1(ptp_clock_t *ptpClock)
{
    /* Current data set */
    ptpClock->currentDS.stepsRemoved = 0;
    ptpClock->currentDS.offsetFromMaster = 0;
    ptpClock->currentDS.meanPathDelay = 0;
    ptpClock->currentDS.delayOk = false;
    ptpClock->currentDS.clock_mode = VTSS_PTP_CLOCK_FREERUN;
    ptpClock->currentDS.lock_period = 0;
//#if defined (VTSS_SW_OPTION_P802_1_AS)
#if 0
    ptpClock->currentDS.cur_802_1as.lastGMPhaseChange = 0LL;
    ptpClock->currentDS.cur_802_1as.lastGMFreqChange = 0.0;
    ptpClock->currentDS.cur_802_1as.gmTimeBaseIndicator = 0;
    ptpClock->currentDS.cur_802_1as.gmChangeCount = 0;
    ptpClock->currentDS.cur_802_1as.timeOfLastGMChangeEvent = 0;
    ptpClock->currentDS.cur_802_1as.timeOfLastGMPhaseChangeEvent = 0;
    ptpClock->currentDS.cur_802_1as.timeOfLastGMFreqChangeEvent = 0;
#endif //VTSS_SW_OPTION_P802_1_AS
    T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"ptpClock->currentDS.clock_mode = %d",ptpClock->currentDS.clock_mode);
    vtss_local_clock_mode_set(ptpClock->currentDS.clock_mode);

    /* Parent data set */
    memcpy(ptpClock->parentDS.grandmasterIdentity, ptpClock->defaultDS.status.clockIdentity, CLOCK_IDENTITY_LENGTH);
    ptpClock->parentDS.parentStats = false;
    ptpClock->parentDS.observedParentOffsetScaledLogVariance = 0;
    ptpClock->parentDS.observedParentClockPhaseChangeRate = 0;
    memcpy(ptpClock->parentDS.parentPortIdentity.clockIdentity, ptpClock->defaultDS.status.clockIdentity, CLOCK_IDENTITY_LENGTH);
    ptpClock->parentDS.parentPortIdentity.portNumber = 0;
    ptpClock->parentDS.grandmasterClockQuality = ptpClock->defaultDS.status.clockQuality;
    ptpClock->parentDS.grandmasterPriority1 = ptpClock->clock_init->cfg.priority1;
    ptpClock->parentDS.grandmasterPriority2 = ptpClock->clock_init->cfg.priority2;
    ptpClock->parentRecPort = NULL;

    /* TimeProperties data set */
    memcpy(&ptpClock->timepropertiesDS, ptpClock->time_prop, sizeof(vtss_appl_ptp_clock_timeproperties_ds_t));
    /* Clear path trace */
    memset(&ptpClock->path_trace, 0, sizeof(ptpClock->path_trace));

}

/* see spec table 16 */
static bool s1(const ForeignMasterDS *foreign, ptp_clock_t *ptpClock, PtpPort_t *port)
{
    bool new_master = false;
    /* Current data set */
    ptpClock->currentDS.stepsRemoved = foreign->ds.stepsRemoved + 1;

    /* Parent data set */
    memcpy(&ptpClock->parentDS.parentPortIdentity, &foreign->ds.sourcePortIdentity, sizeof(vtss_appl_ptp_port_identity));

    if (memcmp(ptpClock->parentDS.grandmasterIdentity, foreign->ds.grandmasterIdentity, sizeof(vtss_appl_clock_identity))) {
        new_master = true;
    }
    memcpy(ptpClock->parentDS.grandmasterIdentity, foreign->ds.grandmasterIdentity, sizeof(vtss_appl_clock_identity));

    ptpClock->parentDS.grandmasterClockQuality = foreign->ds.clockQuality;
    ptpClock->parentDS.grandmasterPriority1 = foreign->ds.priority1;
    ptpClock->parentDS.grandmasterPriority2 = foreign->ds.priority2;
    ptpClock->parentStepsRemoved = foreign->ds.stepsRemoved;  /* to be used in the bmca_data_set_comparison_algorithm */
    ptpClock->parentRecPort = port;

    /* TimeProperties data set */
    ptpClock->timepropertiesDS.currentUtcOffset = foreign->currentUtcOffset;
    ptpClock->timepropertiesDS.currentUtcOffsetValid = getFlag(foreign->flagField[1], PTP_CURRENT_UTC_OFFSET_VALID);
    ptpClock->timepropertiesDS.leap59 = getFlag(foreign->flagField[1], PTP_LI_59);
    ptpClock->timepropertiesDS.leap61 = getFlag(foreign->flagField[1], PTP_LI_61);
    ptpClock->timepropertiesDS.timeTraceable = getFlag(foreign->flagField[1], PTP_TIME_TRACEABLE);
    ptpClock->timepropertiesDS.frequencyTraceable = getFlag(foreign->flagField[1], PTP_FREQUENCY_TRACEABLE);
    ptpClock->timepropertiesDS.ptpTimescale = getFlag(foreign->flagField[1], PTP_PTP_TIMESCALE);
    ptpClock->timepropertiesDS.timeSource = foreign->timeSource;

#if defined(VTSS_SW_OPTION_SYNCE)
    //(void) vtss_synce_ptp_clock_ssm_ql_set(ptpClock->localClockId, ptpClock->parentDS.grandmasterClockQuality.clockClass);
    ptpClock->ssm.ptsf_loss_of_announce = false;
    vtss_ptp_ptsf_state_set(ptpClock->ssm.localClockId);
#endif
    /* the configuration learns the actual utcOffset */
    ptpClock->time_prop->currentUtcOffset = foreign->currentUtcOffset;

    return new_master;
}


/**
 * \brief Compare two data sets. P1588 9.3.4
 *
 *
 * \param a clockDataSet a
 * \param b
 * \param aPort receiver port for ClockDataSet a
 * \param bPort receiver port for ClockDataSet b
 *
 * \return similar to memcmp()s
 *  note: communicationTechnology can be ignored because
 *  if they differed they would not have made it here
 */


#define B_GT_A -1
#define B_GT_A_TOP -2
#define A_GT_B 1
#define A_GT_B_TOP 2
#define ERROR_1 0
#define ERROR_2 0

static i8 bmca_data_set_comparison_algorithm(const ClockDataSet *a, const ClockDataSet *b, const vtss_appl_ptp_port_identity *aPort, const vtss_appl_ptp_port_identity *bPort, u8 profile)
{
    int sGTr, aGTb;

    T_NG(VTSS_TRACE_GRP_PTP_BASE_BMCA, " BMCA set A : ");
    dump_ClockDataSet(a, aPort);
    T_NG(VTSS_TRACE_GRP_PTP_BASE_BMCA, " set B : ");
    dump_ClockDataSet(b, bPort);
    if (profile != VTSS_APPL_PTP_PROFILE_G_8275_1 && profile != VTSS_APPL_PTP_PROFILE_G_8275_2) {
        T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"Non 8275 BMCA");
        if (0 == memcmp(a->grandmasterIdentity, b->grandmasterIdentity, CLOCK_IDENTITY_LENGTH)) {
            goto part2;
        }
        if (a->priority1 > b->priority1)
            return B_GT_A;
        else if (a->priority1 < b->priority1)
            return A_GT_B;

        if (a->clockQuality.clockClass > b->clockQuality.clockClass)
            return B_GT_A;
        else if (a->clockQuality.clockClass < b->clockQuality.clockClass)
            return A_GT_B;

        if (a->clockQuality.clockAccuracy > b->clockQuality.clockAccuracy)
            return B_GT_A;
        else if (a->clockQuality.clockAccuracy < b->clockQuality.clockAccuracy)
            return A_GT_B;

        if (a->clockQuality.offsetScaledLogVariance > b->clockQuality.offsetScaledLogVariance)
            return B_GT_A;
        else if (a->clockQuality.offsetScaledLogVariance < b->clockQuality.offsetScaledLogVariance)
            return A_GT_B;

        if (a->priority2 > b->priority2)
            return B_GT_A;
        else if (a->priority2 < b->priority2)
            return A_GT_B;

        if (0 < memcmp(a->grandmasterIdentity, b->grandmasterIdentity, CLOCK_IDENTITY_LENGTH))
            return B_GT_A;
        else
            return A_GT_B;

    } else {
        T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"8275 BMCA");
        /* G8275 Comparison algorithm */
        if (a->clockQuality.clockClass > b->clockQuality.clockClass)
            return B_GT_A;
        else if (a->clockQuality.clockClass < b->clockQuality.clockClass)
            return A_GT_B;

        if (a->clockQuality.clockAccuracy > b->clockQuality.clockAccuracy)
            return B_GT_A;
        else if (a->clockQuality.clockAccuracy < b->clockQuality.clockAccuracy)
            return A_GT_B;

        if (a->clockQuality.offsetScaledLogVariance > b->clockQuality.offsetScaledLogVariance)
            return B_GT_A;
        else if (a->clockQuality.offsetScaledLogVariance < b->clockQuality.offsetScaledLogVariance)
            return A_GT_B;

        if (a->priority2 > b->priority2)
            return B_GT_A;
        else if (a->priority2 < b->priority2)
            return A_GT_B;

        if (a->localPriority > b->localPriority)
            return B_GT_A;
        else if (a->localPriority < b->localPriority)
            return A_GT_B;

        if (a->clockQuality.clockClass <= 127)
            goto part2;

        aGTb = memcmp(a->grandmasterIdentity, b->grandmasterIdentity, CLOCK_IDENTITY_LENGTH);
        if (aGTb < 0) return A_GT_B;
        else if (aGTb > 0) return B_GT_A;
        goto part2;

    }
    part2:
    T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"BMCA part 2");
    if (a->stepsRemoved > b->stepsRemoved+1)
        return B_GT_A;
    else if (a->stepsRemoved+1 < b->stepsRemoved)
        return A_GT_B;

    if (a->stepsRemoved > b->stepsRemoved) {
        sGTr = PortIdentitycmp(&a->sourcePortIdentity, aPort);
        T_NG(VTSS_TRACE_GRP_PTP_BASE_BMCA, "A sGTr %d", sGTr);
        if (sGTr > 0)
            return B_GT_A;
        else if (sGTr < 0)
            return B_GT_A_TOP;
        else
            return ERROR_1;
    } else if (a->stepsRemoved < b->stepsRemoved) {
        sGTr = PortIdentitycmp(&b->sourcePortIdentity, bPort);
        T_NG(VTSS_TRACE_GRP_PTP_BASE_BMCA, "B sGTr %d", sGTr);
        if (sGTr > 0)
            return A_GT_B;
        else if (sGTr < 0)
            return A_GT_B_TOP;
        else
            return ERROR_1;
    } else { /* A==B (identity and steps removed) */
        T_NG(VTSS_TRACE_GRP_PTP_BASE_BMCA, "Src port identity compare");
        aGTb = PortIdentitycmp(&a->sourcePortIdentity, &b->sourcePortIdentity);
        if ( aGTb > 0)
            return B_GT_A_TOP;
        else if ( aGTb < 0)
            return A_GT_B_TOP;
        else {
            if (aPort->portNumber > bPort->portNumber)
                return B_GT_A_TOP;
            else if (aPort->portNumber < bPort->portNumber)
                return A_GT_B_TOP;
            else
                return ERROR_2;
        }
    }
}

/**
 * State decision algorithm (P1588 9.3.3)
 * And 8275.1 alternate state decission algotihm (G.8275.1 6.3.7)
 *
 * Purpose: To obtain the state of a port
 *
 *
 * \param foreign [IN] pointer to the best master clock for the port.
 * \param ptpClock [IN/OUT] pointer to clock data set
 * \param ptpPort [IN/OUT] pointer to port data set
 *
 * \return recommended state
 */
static vtss_ptp_bmc_recommended_state_t bmca_bmc_state_decision_algorithm(const ForeignMasterDS *foreign, ptp_clock_t *ptpClock, PtpPort_t *ptpPort)
{
    i8 tmp = 0;
    ClockDataSet ptpClock_d0;
    ClockDataSet parent_ds;
    PtpPort_t * parent_port;

    memcpy(ptpClock_d0.grandmasterIdentity, ptpPort->portDS.status.portIdentity.clockIdentity, sizeof(vtss_appl_clock_identity));
    ptpClock_d0.priority1 = ptpClock->clock_init->cfg.priority1;
    ptpClock_d0.clockQuality = ptpClock->defaultDS.status.clockQuality;
    ptpClock_d0.priority2 = ptpClock->clock_init->cfg.priority2;
    ptpClock_d0.stepsRemoved = ptpClock->currentDS.stepsRemoved;
    ptpClock_d0.sourcePortIdentity = ptpClock->parentDS.parentPortIdentity;
    ptpClock_d0.localPriority = ptpClock->clock_init->cfg.localPriority;

    memcpy(parent_ds.grandmasterIdentity, ptpClock->parentDS.grandmasterIdentity, sizeof(vtss_appl_clock_identity));
    parent_ds.priority1 = ptpClock->parentDS.grandmasterPriority1;
    parent_ds.clockQuality = ptpClock->parentDS.grandmasterClockQuality;
    parent_ds.priority2 = ptpClock->parentDS.grandmasterPriority2;
    parent_ds.stepsRemoved = ptpClock->parentStepsRemoved;
    parent_ds.sourcePortIdentity = ptpClock->parentDS.parentPortIdentity;
    parent_port = (ptpClock->parentRecPort) ? ptpClock->parentRecPort : ptpPort;
    parent_ds.localPriority = parent_port->port_config->localPriority;
    T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"parentlocalPriority %d, parentport %d, port %d",parent_ds.localPriority, parent_port->portDS.status.portIdentity.portNumber, ptpPort->portDS.status.portIdentity.portNumber);

    /* D0 is class 1 - 127 ? */
    if ((ptpClock_d0.clockQuality.clockClass <=127) && (ptpClock_d0.clockQuality.clockClass >= 1)) {
        if (bmca_data_set_comparison_algorithm(&ptpClock_d0, &foreign->ds, &ptpClock_d0.sourcePortIdentity, &ptpPort->portDS.status.portIdentity, ptpClock->clock_init->cfg.profile) > 0) {
            T_IG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"Master M1 %d",ptpPort->portDS.status.portIdentity.portNumber);
            return VTSS_PTP_BMC_MASTER_M1;
        }
        T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"Passive %d, clock class %d",ptpPort->portDS.status.portIdentity.portNumber, ptpClock_d0.clockQuality.clockClass);

        return VTSS_PTP_BMC_PASSIVE;
    }
    else if (((tmp = bmca_data_set_comparison_algorithm(&ptpClock_d0, &foreign->ds, &ptpClock_d0.sourcePortIdentity, &ptpPort->portDS.status.portIdentity, ptpClock->clock_init->cfg.profile)) > 0) &&
             (ptpClock_d0.clockQuality.clockClass != 255))
    {
        T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"Master port %d compare result %d",ptpPort->portDS.status.portIdentity.portNumber, tmp);
        T_IG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"Master M2 %d",ptpPort->portDS.status.portIdentity.portNumber);
        return VTSS_PTP_BMC_MASTER_M2;
    }
    else {
        if ((ptpClock->parentRecPort == NULL) || ((tmp = bmca_data_set_comparison_algorithm(&foreign->ds, &parent_ds, &ptpPort->portDS.status.portIdentity,
                                                                                            &ptpClock->parentRecPort->portDS.status.portIdentity, ptpClock->clock_init->cfg.profile)) > 0))
        {
            T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"Slave port %d compare result %d",ptpPort->portDS.status.portIdentity.portNumber, tmp);
            T_IG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"Slave %d",ptpPort->portDS.status.portIdentity.portNumber);
            T_IG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"foreign:");
            dump_ClockDataSet(&foreign->ds, &ptpPort->portDS.status.portIdentity);
                if (ptpClock->parentRecPort != NULL) {
                    T_IG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"parent:");
                    dump_ClockDataSet(&parent_ds, &ptpClock->parentRecPort->portDS.status.portIdentity);
                }
            return VTSS_PTP_BMC_SLAVE;
        }
        else if (((tmp = bmca_data_set_comparison_algorithm(&parent_ds, &foreign->ds, &ptpClock->parentRecPort->portDS.status.portIdentity,
                                                            &ptpPort->portDS.status.portIdentity, ptpClock->clock_init->cfg.profile)) == A_GT_B_TOP) &&
                 (ptpClock_d0.clockQuality.clockClass != 255))
        {
            T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"Passive port %d compare result %d", ptpPort->portDS.status.portIdentity.portNumber, tmp);
            return VTSS_PTP_BMC_PASSIVE;
        } else if (tmp == A_GT_B) {
            T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"Master port %d compare result %d", ptpPort->portDS.status.portIdentity.portNumber, tmp);
            T_IG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"Master M3 %d", ptpPort->portDS.status.portIdentity.portNumber);
            return VTSS_PTP_BMC_MASTER_M3;
        } else {
            return VTSS_PTP_BMC_UNCHANGED;
        }
    }
}

static vtss_ptp_bmc_recommended_state_t bmc_algorithm(PtpPort_t *ptpPort, ptp_clock_t *ptpClock)
{
    i16 i, best;
    vtss_ptp_bmc_recommended_state_t rec_state = VTSS_PTP_BMC_UNCHANGED;
    char str [40];

    for (i = 1, best = 0; i < ptpPort->number_foreign_records; ++i) {
        if (ptpPort->foreignMasterDS[i].qualified && ptpPort->foreignMasterDS[best].qualified) {
            if (bmca_data_set_comparison_algorithm(&ptpPort->foreignMasterDS[i].ds,
                                                   &ptpPort->foreignMasterDS[best].ds, &ptpPort->portDS.status.portIdentity, &ptpPort->portDS.status.portIdentity, ptpClock->clock_init->cfg.profile) > 0)
                best = i;
        } else if (ptpPort->foreignMasterDS[i].qualified && !ptpPort->foreignMasterDS[best].qualified) {
            best = i;
        }
    }

    T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"bmc: best record %d num_of_foreign %d qualified %d", best, ptpPort->number_foreign_records, ptpPort->foreignMasterDS[best].qualified);
    ptpPort->foreign_record_best = best;

    if (!ptpPort->number_foreign_records || !ptpPort->foreignMasterDS[best].qualified) {
        T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"no qualified records on port %d", ptpPort->portDS.status.portIdentity.portNumber);
        if (ptpClock->slavePort == ptpPort->portDS.status.portIdentity.portNumber) {
            vtss_ptp_bmca_m1(ptpClock);
        }
        rec_state = VTSS_PTP_BMC_MASTER_M1;
    } else {
        rec_state = bmca_bmc_state_decision_algorithm(&ptpPort->foreignMasterDS[best], ptpClock, ptpPort);
        T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"rec_state %d for port %d", rec_state, ptpPort->portDS.status.portIdentity.portNumber);
        switch (rec_state) {
            case VTSS_PTP_BMC_MASTER_M1:
            case VTSS_PTP_BMC_MASTER_M2:
                if (ptpClock->slavePort == ptpPort->portDS.status.portIdentity.portNumber) {
                    vtss_ptp_bmca_m1(ptpClock);
                }
                break;
            case VTSS_PTP_BMC_SLAVE:
                T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"Slave %d, current parent Id %s", ptpPort->portDS.status.portIdentity.portNumber,
                     ClockIdentityToString(ptpClock->parentDS.parentPortIdentity.clockIdentity, str));
                T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"Slave %d, new parent Id %s", ptpPort->portDS.status.portIdentity.portNumber,
                     ClockIdentityToString(ptpPort->foreignMasterDS[best].ds.grandmasterIdentity, str));
                if (s1(&ptpPort->foreignMasterDS[best], ptpClock, ptpPort)) {
                    T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"Uncalibrated %d", ptpPort->portDS.status.portIdentity.portNumber);
                    rec_state = VTSS_PTP_BMC_UNCALIBRATED;
                }
                T_D("*** BMC ALGORITHM  (slave) ***");
                break;
            default:
                break;
        }
    }
    return rec_state;
}

/*
 * Announce receipt timeout timer
 */
/*lint -esym(459, vtss_ptp_bmca_announce_to_timer) */
static void vtss_ptp_bmca_announce_to_timer(vtss_ptp_sys_timer_t *timer, void *t)
{
    PtpPort_t *ptpPort = (PtpPort_t *)t;
    T_IG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"announce timeout, port %d", ptpPort->portDS.status.portIdentity.portNumber);
    ptpPort->number_foreign_records = 0;
    ptpPort->foreign_record_i = 0;
    vtss_ptp_timer_start(&ptpPort->announce_bmca_timer, 1, false);

//    vtss_ptp_announce_receipt_timeout(ptpClock, ptpPort);
}

static void qualifyAnnounceMessages(PtpPort_t *ptpPort)
{
    int i;
    for (i = 0; i < ptpPort->max_foreign_records; i++) {
        if (ptpPort->foreignMasterDS[i].foreignMasterAnnounceMessages < PTP_FOREIGN_MASTER_THRESHOLD) {
            if (ptpPort->parent->selected_master == i) {
                ptpPort->parent->ssm.ptsf_loss_of_announce = true;
                T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA, "*** QUALIFY ANNOUNCE MESSAGES  loss***");
#if defined(VTSS_SW_OPTION_SYNCE)
                vtss_ptp_ptsf_state_set(ptpPort->parent->ssm.localClockId);
#endif
            }
            if (ptpPort->foreignMasterDS[i].foreignMasterAnnounceMessages ==0) {
                ptpPort->foreignMasterDS[i].qualified = false;
            }
        }
        ptpPort->foreignMasterDS[i].foreignMasterAnnounceMessages = 0;
    }
}

/*
 * Announce QUALIFICATION timer
 */
/*lint -esym(459, vtss_ptp_bmca_announce_qual_timer) */
static void vtss_ptp_bmca_announce_qual_timer(vtss_ptp_sys_timer_t *timer, void *t)
{
    PtpPort_t *ptpPort = (PtpPort_t *)t;
    T_NG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"announce qualification timer, port %d", ptpPort->portDS.status.portIdentity.portNumber);
    qualifyAnnounceMessages(ptpPort);
    vtss_ptp_timer_start(timer, PTP_FOREIGN_MASTER_TIME_WINDOW(ptpPort->portDS.status.s_802_1as.currentLogAnnounceInterval), true);
}

/*
 * BMCA State Decission timeout
 */
/*lint -esym(459, vtss_ptp_bmca_state_decission_timer) */
static void vtss_ptp_bmca_state_decission_timer(vtss_ptp_sys_timer_t *timer, void *t)
{
    PtpPort_t *ptpPort = (PtpPort_t *)t;
    ptp_clock_t *ptpClock = ptpPort->parent;
    vtss_ptp_bmc_recommended_state_t rec_state = VTSS_PTP_BMC_UNCHANGED;
    T_NG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"event BMCA state decission timer, port %d", ptpPort->portDS.status.portIdentity.portNumber);
    if (ptpClock->clock_init->cfg.protocol != VTSS_APPL_PTP_PROTOCOL_OAM && ptpClock->clock_init->cfg.protocol != VTSS_APPL_PTP_PROTOCOL_ONE_PPS) {
        if (ptpClock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_ORD_BOUND ||
            ptpClock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_SLAVE_ONLY || ptpPort->virtual_port) {
            rec_state = bmc_algorithm(ptpPort, ptpClock);
        } else if (ptpClock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_MASTER_ONLY) {
            rec_state = VTSS_PTP_BMC_MASTER_M1;
        }
        if ((rec_state == VTSS_PTP_BMC_MASTER_M1 || rec_state == VTSS_PTP_BMC_MASTER_M2 || rec_state == VTSS_PTP_BMC_MASTER_M3) &&
            (ptpPort->port_config->notMaster)) {
            rec_state = VTSS_PTP_BMC_PASSIVE;
        }
        vtss_ptp_recommended_state(rec_state, ptpClock, ptpPort);
    }
}

void vtss_ptp_bmca_init(ptp_clock_t *ptpClock)
{
    u32 portidx;
    T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"no of ports %d", ptpClock->clock_init->numberPorts);
    for (portidx = 0; portidx < ptpClock->clock_init->numberPorts; portidx++) {
        PtpPort_t *ptpPort = &ptpClock->ptpPort[portidx];
        vtss_ptp_timer_init(&ptpPort->announce_to_timer,   "announce_to",   portidx, vtss_ptp_bmca_announce_to_timer,     ptpPort);
        vtss_ptp_timer_init(&ptpPort->announce_qual_timer, "announce_qual", portidx, vtss_ptp_bmca_announce_qual_timer,   ptpPort);
        vtss_ptp_timer_init(&ptpPort->announce_bmca_timer, "announce_bmca", portidx, vtss_ptp_bmca_state_decission_timer, ptpPort);
        T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"timer");
    }
}

/**
 * add or update an entry in the foreign master data set
 * if record already exists, it is updated.
 * if it does not exist, the entry is added.
 * if the list of foreign masters is full, then the oldest entry is overwritten
 */

ClockDataSet * addForeign(const u8 *buf, MsgHeader *header, ptp_clock_t *ptpClock, PtpPort_t *ptpPort)
{
    int i, j;
    bool found = false;

    T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"updateForeign");

    j = ptpPort->foreign_record_best;
    for (i = 0; i < ptpPort->number_foreign_records; ++i) {
        if (!PortIdentitycmp(&header->sourcePortIdentity, &ptpPort->foreignMasterDS[j].ds.sourcePortIdentity)) {
            if (++ptpPort->foreignMasterDS[j].foreignMasterAnnounceMessages >= PTP_FOREIGN_MASTER_THRESHOLD) {
                ptpPort->foreignMasterDS[j].qualified = true;
                if (ptpPort->parent->selected_master == j) {
                    T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"*** ADD FOREIGN (UPDATE) ***");
                }
            }
            found = true;
            T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"updateForeign: update record %d", i);
            break;
        }
        T_NG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"updateForeign: check: i %d, j %d found %d", i, j, found);
        j = (j + 1)%ptpPort->number_foreign_records;
    }

    if (!found) {
        if (ptpPort->number_foreign_records < ptpPort->max_foreign_records) {
            ++ptpPort->number_foreign_records;
        }

        j = ptpPort->foreign_record_i;
        ptpPort->foreignMasterDS[j].foreignMasterAnnounceMessages = 1;
        if (ptpPort->portDS.status.portState == VTSS_APPL_PTP_LISTENING) {
            ptpPort->foreignMasterDS[j].qualified = true;
            if (ptpPort->parent->selected_master == j) {
                T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"*** ADD FOREIGN (NEW) ***");
            }
        } else {
            T_IG(VTSS_TRACE_GRP_PTP_BASE_BMCA, "qualified false");
            ptpPort->foreignMasterDS[j].qualified = false;
        }
        ptpPort->foreign_record_i = (ptpPort->foreign_record_i + 1)%ptpPort->max_foreign_records;
        T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"Port %d, number_foreign_records %d, j %d, foreign_record_i %d", ptpPort->portDS.status.portIdentity.portNumber, ptpPort->number_foreign_records, j, ptpPort->foreign_record_i);
    }

    if (ptpPort->port_config->masterOnly && (ptpClock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_G_8275_1 || \
            ptpClock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_G_8275_2 || \
            ptpClock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_1588     || \
            ptpClock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_NO_PROFILE)) {
        ptpPort->number_foreign_records = 0;
    }

    vtss_ptp_unpack_announce(buf, &ptpPort->foreignMasterDS[j]);
    ptpPort->foreignMasterDS[j].ds.localPriority = ptpPort->port_config->localPriority;;
    T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"localPriority %d, port %d",ptpPort->foreignMasterDS[j].ds.localPriority, ptpPort->portDS.status.portIdentity.portNumber);

    return &ptpPort->foreignMasterDS[j].ds;
}

void vtss_ptp_bmca_state(PtpPort_t *ptpPort, bool enable)
{
    T_NG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"bmca enable %s", enable ? "TRUE" : "FALSE");
    if(ptpPort->parent->clock_init->cfg.profile != VTSS_APPL_PTP_PROFILE_IEEE_802_1AS && ptpPort->parent->clock_init->cfg.profile != VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
        if(enable && ! ptpPort->virtual_port) {
            vtss_ptp_timer_start(&ptpPort->announce_qual_timer, PTP_FOREIGN_MASTER_TIME_WINDOW(ptpPort->port_config->logAnnounceInterval), true);
            vtss_ptp_timer_start(&ptpPort->announce_to_timer, PTP_LOG_TIMEOUT(ptpPort->port_config->logAnnounceInterval)*ptpPort->port_config->announceReceiptTimeout, false);
            T_NG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"bmca qual timer value %d, announce to %d", PTP_FOREIGN_MASTER_TIME_WINDOW(ptpPort->port_config->logAnnounceInterval), PTP_LOG_TIMEOUT(ptpPort->port_config->logAnnounceInterval)*ptpPort->port_config->announceReceiptTimeout);
        } else {
            vtss_ptp_timer_stop(&ptpPort->announce_qual_timer);
        }
    }
}

bool vtss_ptp_bmca_announce_slave(ptp_clock_t *ptpClock, PtpPort_t *ptpPort, ptp_tx_buffer_handle_t *buf_handle, MsgHeader *header, ptp_path_trace_t *path_sequence)
{
    ForeignMasterDS foreign;
    if ( SEQUENCE_ID_CHECK(header->sequenceId, ptpPort->parent_last_announce_sequence_number)
            && !PortIdentitycmp(&header->sourcePortIdentity, &ptpClock->parentDS.parentPortIdentity)) { /* Announce from current master: Update parentDS */
        ptpPort->parent_last_announce_sequence_number = header->sequenceId;
        /* addForeign() takes care of vtss_ptp_unpack_announce(), bmcStateSecision updates parent data, if it changes */
        //received announce from current master: update the path trace
        ptpClock->path_trace = *path_sequence;
        vtss_ptp_timer_start(&ptpPort->announce_bmca_timer, 1, false);
        (void)addForeign(buf_handle->frame + buf_handle->header_length, header, ptpClock, ptpPort);
        vtss_ptp_unpack_announce(buf_handle->frame + buf_handle->header_length, &foreign);
        foreign.ds.localPriority = ptpPort->port_config->localPriority;;
        (void)s1(&foreign, ptpClock, ptpPort);
        T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"ANNOUNCE_RECEIPT from current master");
        vtss_ptp_timer_start(&ptpPort->announce_to_timer, PTP_LOG_TIMEOUT(ptpPort->port_config->logAnnounceInterval)*ptpPort->port_config->announceReceiptTimeout, false);
    } else if (PortIdentitycmp(&header->sourcePortIdentity, &ptpClock->parentDS.parentPortIdentity)) {
        /* addForeign() takes care of vtss_ptp_unpack_announce() */
        vtss_ptp_timer_start(&ptpPort->announce_to_timer, PTP_LOG_TIMEOUT(ptpPort->port_config->logAnnounceInterval)*ptpPort->port_config->announceReceiptTimeout, false);
        vtss_ptp_timer_start(&ptpPort->announce_bmca_timer, 1, false);
        /*announce = */
        (void)addForeign(buf_handle->frame + buf_handle->header_length, header, ptpClock, ptpPort);
        T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"ANNOUNCE_RECEIPT from non master");

    } else { /* Announce from current master, but out of sequence */
        ptpPort->parent_last_announce_sequence_number = header->sequenceId;
    }
    return false;
}

bool vtss_ptp_bmca_announce_master(ptp_clock_t *ptpClock, PtpPort_t *ptpPort, ptp_tx_buffer_handle_t *buf_handle, MsgHeader *header)
{
    T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"ANNOUNCE_RECEIPT");
    
    (void)addForeign(buf_handle->frame + buf_handle->header_length, header, ptpClock, ptpPort);
    vtss_ptp_timer_start(&ptpPort->announce_bmca_timer, 1, false);
    vtss_ptp_timer_start(&ptpPort->announce_to_timer, PTP_LOG_TIMEOUT(ptpPort->port_config->logAnnounceInterval)*ptpPort->port_config->announceReceiptTimeout, false);
    return false;
}

void vtss_ptp_bmca_announce_virtual(ptp_clock_t *ptpClock, PtpPort_t *ptpPort, ClockDataSet *clock_ds, vtss_appl_ptp_clock_timeproperties_ds_t *time_prop)
{
    T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"Virtual announce message");

    int j;
    bool found = false;

    T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"updateForeign");
    // Only one master can exist pr virtual port.
    j = 0;
    if (ptpPort->number_foreign_records > 0) {
        found = true;
    }
    if (++ptpPort->foreignMasterDS[j].foreignMasterAnnounceMessages >= PTP_FOREIGN_MASTER_THRESHOLD) {
        ptpPort->foreignMasterDS[j].qualified = true;
        if (ptpPort->parent->selected_master == j) {
            T_D("*** ADD FOREIGN (UPDATE) ***");
        }
    }
    T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"found %d, AnnMessages %d, qualified %d, port_state %s", found, ptpPort->foreignMasterDS[j].foreignMasterAnnounceMessages, ptpPort->foreignMasterDS[j].qualified, PortStateToString(ptpPort->portDS.status.portState));
    if (!found) {
        ptpPort->number_foreign_records = 1;

        j = ptpPort->foreign_record_i;
        ptpPort->foreignMasterDS[j].foreignMasterAnnounceMessages = 1;
        if (ptpPort->portDS.status.portState == VTSS_APPL_PTP_LISTENING) {
            ptpPort->foreignMasterDS[j].qualified = true;
            if (ptpPort->parent->selected_master == j) {
                T_D("*** ADD FOREIGN (NEW) ***");
            }
        } else {
            ptpPort->foreignMasterDS[j].qualified = false;
        }
        T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"found %d, AnnMessages %d, qualified %d", found, ptpPort->foreignMasterDS[j].foreignMasterAnnounceMessages, ptpPort->foreignMasterDS[j].qualified);
        ptpPort->foreign_record_i = 0;
        T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"Port %d, number_foreign_records %d, j %d, foreign_record_i %d", ptpPort->portDS.status.portIdentity.portNumber, ptpPort->number_foreign_records, j, ptpPort->foreign_record_i);
    }

    ptpPort->foreignMasterDS[j].ds = *clock_ds;
    ptpPort->foreignMasterDS[j].currentUtcOffset = time_prop->currentUtcOffset;
    ptpPort->foreignMasterDS[j].timeSource = time_prop->timeSource;
    ptpPort->foreignMasterDS[j].flagField[0] = 0;
    ptpPort->foreignMasterDS[j].flagField[1] = 
        (time_prop->leap61 ? PTP_LI_61 : 0) |
        (time_prop->leap59 ? PTP_LI_59 : 0) |
        (time_prop->currentUtcOffsetValid ? PTP_CURRENT_UTC_OFFSET_VALID :0) |
        (time_prop->ptpTimescale ? PTP_PTP_TIMESCALE : 0) |
        (time_prop->timeTraceable ? PTP_TIME_TRACEABLE : 0) |
        (time_prop->frequencyTraceable ? PTP_FREQUENCY_TRACEABLE : 0);

    ptpPort->foreignMasterDS[j].ds.localPriority = ptpPort->port_config->localPriority;;
    T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"localPriority %d, port %d",ptpPort->foreignMasterDS[j].ds.localPriority, ptpPort->portDS.status.portIdentity.portNumber);
    // Update Parent record only if there is no change in the current master status.
    if (!PortIdentitycmp(&ptpPort->portDS.status.portIdentity, &ptpClock->parentDS.parentPortIdentity)) {
        (void)s1(&ptpPort->foreignMasterDS[j], ptpClock, ptpPort);
        T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"ANNOUNCE_RECEIPT from current master");
    }

    vtss_ptp_timer_start(&ptpPort->announce_bmca_timer, 1, false);
    T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"Announce interval %d, timeout %d, timeout time %d", ptpPort->portDS.status.s_802_1as.currentLogAnnounceInterval, ptpPort->port_config->announceReceiptTimeout, PTP_LOG_TIMEOUT(ptpPort->portDS.status.s_802_1as.currentLogAnnounceInterval)*ptpPort->port_config->announceReceiptTimeout);
    vtss_ptp_timer_start(&ptpPort->announce_to_timer, PTP_LOG_TIMEOUT(0)*ptpPort->port_config->announceReceiptTimeout, false);
    return;
}
