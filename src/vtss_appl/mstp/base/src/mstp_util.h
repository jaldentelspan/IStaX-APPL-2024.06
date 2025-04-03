/*
 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _MSTP_UTIL_H_
#define _MSTP_UTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \file mstp_util.h
 * \brief State Machine Procedures
 *
 * This file contain the definition of the <em> 802.1D-2004 17.21:
 * State machine procedures. </em>
 *
 * \author Lars Povlsen <lpovlsen@vitesse.com>
 *
 * \date 09-01-2009
 */

#define struct_cmp(p1, op, p2) (memcmp(&p1, &p2, sizeof(p1)) op 0)

static inline u32 unal_ntohl(const u8 *bytes)
{
    return (u32)((bytes[0] << 24) + (bytes[1] << 16) + (bytes[2] << 8) + (bytes[3]));
}

static inline u16 unal_ntohs(const u8 *bytes)
{
    return (u16)((bytes[0] << 8) + bytes[1]);
}

static inline int portid2hostport(const PortId_t *pi)
{
    return (int)(0xFFF & unal_ntohs(pi->bytes));
}

static inline timer bpdu_time(const u8 *b)
{
    /* Convert fractional timer in BPDU to host timer (roundup) */
    return (timer)((int)b[0]) + (b[1] >= (u8) 0x80 ? 1 : 0);
}

static inline BOOL bpduGuard(const mstp_cistport_t *cist)
{
    return cist->bpduGuard ||
           (cist->operEdge &&
            !cist->adminAutoEdgePort &&
            cist->bridge->conf.bridge.bpduGuard);
}

static inline BOOL bpduFiltering(const mstp_cistport_t *cist)
{
    return cist->operEdge &&
           !cist->adminAutoEdgePort &&
           cist->bridge->conf.bridge.bpduFiltering;
}

static inline BOOL portEnabled(const mstp_cistport_t *cist)
{
    return cist->linkEnabled && !cist->stpInconsistent;
}

static inline const char *rstp_portrole2str(PortRole_t pr)
{
    switch (pr) {
    case AltBackupPort:
        return "AltBackupPort";

    case RootPort:
        return "RootPort";

    case DesignatedPort:
        return "DesignatedPort";

    case AlternatePort:
        return "AlternatePort";

    case BackupPort:
        return "BackupPort";

    case MasterPort:
        return "MasterPort";

    case DisabledPort:
        return "DisabledPort";

    case UnknownPort:
        return "UnknownPort";
    }

    return "<Undefined-port-role>";
}

static inline const char *rstp_rcvinfo2str(RcvdInfo_t info)
{
    switch (info) {
    case SuperiorDesignatedInfo:
        return "SuperiorDesignatedInfo";

    case RepeatedDesignatedInfo:
        return "RepeatedDesignatedInfo";

    case InferiorDesignatedInfo:
        return "InferiorDesignatedInfo";

    case InferiorRootAlternateInfo:
        return "InferiorRootAlternateInfo";

    case OtherInfo:
        return "OtherInfo";
    }

    return "<Undefined-info>";
}

static inline const char *rstp_info2str(InfoIs_t info)
{
    switch (info) {
    case Received:
        return "Received";

    case Mine:
        return "Mine";

    case Aged:
        return "Aged";

    case Disabled:
        return "DisabledPort";
    }

    return "<Undefined-infoIs>";
}

static inline const char *rstp_portstate2str(const mstp_port_t *port)
{
    if (port->cistport->stpInconsistent) {
        return "Error-Disabled";
    }

    if (!port->cistport->linkEnabled) {
        return "Disabled";
    }

    if (port->forwarding) {
        return "Forwarding";
    }

    if (port->learning) {
        return "Learning";
    }

    return "Discarding";
}

BOOL reRooted(const mstp_port_t *iport);

BOOL betterorsameInfo(const mstp_port_t *port, InfoIs_t newInfoIs);

void clearReselectTree(const struct mstp_tree *tree);

/** mstp_port_setstate(port, state)
 * Wrapper function for enableForwarding() and disableForwarding()
 *
 * \param port Port to control
 *
 * \param state to set
 */
static inline void mstp_port_setstate(const mstp_port_t *port, mstp_fwdstate_t state)
{
    const mstp_tree_t *tree = port->tree;

    vtss_mstp_trap(tree->msti, MSTP_TRAP_TOPOLOGY_CHANGE);
    vtss_mstp_port_setstate(port->port_no, tree->msti, state);
}

/** 17.21.3 \def disableForwarding(p)
 * An implementation dependent procedure that causes the Forwarding
 * Process (7.7) to stop forwarding frames through the Port. The
 * procedure does not complete until forwarding has stopped.
 */
#define disableForwarding(t) mstp_port_setstate(t, MSTP_FWDSTATE_BLOCKING)

/** 17.21.4 \def disableLearning(p)
 * An implementation dependent procedure that causes the Learning
 * Process (7.8) to stop learning from the source address of frames
 * received on the Port. The procedure does not complete until
 * learning has stopped.
 */
#define disableLearning(t) /* mstp_port_setstate(t, MSTP_FWDSTATE_BLOCKING) */

/** 17.21.5 \def enableForwarding(p)
 * An implementation dependent procedure that causes the Forwarding
 * Process (7.7) to start forwarding frames through the Port. The
 * procedure does not complete until forwarding has been enabled.
 */
#define enableForwarding(t) mstp_port_setstate(t, MSTP_FWDSTATE_FORWARDING)

/** 17.21.6 \def enableLearning(p)
 * An implementation dependent procedure that causes the Learning
 * Process (7.8) to start learning from frames received on the
 * Port. The procedure does not complete until learning has been
 * enabled.
 */
#define enableLearning(t) mstp_port_setstate(t, MSTP_FWDSTATE_LEARNING)

/** call_fdbFlush(p)
 * Implementation support for fdbFlush:
 *
 * Set by the topology change state machine to instruct the filtering
 * database to remove all entries for this Port, immediately if
 * rstpVersion (17.20.11) is TRUE, or by rapid ageing (17.19.1) if
 * stpVersion (17.20.12) is TRUE. Reset by the filtering database once
 * the entries are removed if rstpVersion is TRUE, and immediately if
 * stpVersion is TRUE.
 *
 * \note This implementation \e always flush immediately. (Since we
 * have both hardware flush and learning.)
 *
 * \note 802.1Q addition:
 *
 * The following variable is specified in 17.19 of IEEE Std 802.1D;
 * however, its use in this standard is modified as indicated. There
 * is one instance per-Port of this variable for the CIST and one
 * per-Port for each MSTI:
 *
 * s) fdbFlush. In addition to the definition of fdbFlush contained in
 * IEEE Std 802.1D, setting the fdbFlush variable does not result in
 * flushing of filtering database entries in the case that the Port is
 * an Edge Port (i.e., operEdge is TRUE).
 *
 */
static inline void fdbFlush(const mstp_port_t *port)
{
    if (!port->cistport->operEdge) {
        vtss_mstp_port_flush(port->port_no, port->tree->msti);
    }
}

timer FwdDelay(const mstp_port_t *port);
timer HelloTime(const mstp_port_t *port);
timer MaxAge(const mstp_port_t *port);
void newTcWhile(mstp_port_t *port);
RcvdInfo_t rcvInfo(mstp_port_t *port);
void recordAgreement(mstp_port_t *port);
void recordDispute(mstp_port_t *port);
void recordProposal(mstp_port_t *port);
void recordPriority(mstp_port_t *port);
void recordTimes(mstp_port_t *port);
void setSyncTree(const mstp_tree_t *tree);
void setReRootTree(const mstp_tree_t *tree);
void setSelectedTree(const mstp_tree_t *tree);
void setTcFlags(mstp_port_t *port);
void setTcPropTree(const mstp_port_t *port);
void txConfig(mstp_port_t *port);
void txMstp(mstp_port_t *port);
void txTcn(mstp_port_t *port);
void updtBPDUVersion(mstp_port_t *port);
void updtRcvdInfoWhile(mstp_port_t *port);
void updtRoleDisabledTree(mstp_tree_t *tree);
BOOL fromSameRegion(mstp_port_t *port);

/** \def anyReSelect(t)
 *
 * Helper function for determining whether a tree has any ports with
 * \a reselect set
 */
static inline BOOL anyReSelect(const struct mstp_tree *tree)
{
    mstp_port_t *port;

    ForAllPorts(tree, port) {
        if (port->reselect) {
            T_N("%s: **** RESELECT TRUE (%d) ***", __FUNCTION__, port->port_no);
            return TRUE;
        }
    }

    return FALSE;
}

/** \def mstp_setrole(t, p, r)
 *
 * Helper function for setting port role (and emitting callouts)
 */
static inline void mstp_setrole(mstp_port_t *port, PortRole_t role)
{
    PortRole_t old_role;

    // Only real transitions
    if (port->role != role) {
        old_role = port->role;
        port->role = role;
        vtss_mstp_port_setrole(port->port_no, port->tree->msti, (vtss_mstp_portrole_t) old_role, (vtss_mstp_portrole_t) role);
    }
}

BOOL allSynced(const struct mstp_tree *tree, const mstp_port_t *port);
void updtRolesTree(struct mstp_tree *tree);

void setAllRcvdMsgs(mstp_port_t *port, BOOL v);
BOOL rcvdAnyMsg(mstp_port_t *port);
BOOL mstiDesignatedOrTCpropagatingRootPort(mstp_port_t *port);
BOOL allTransmitReady(mstp_port_t *port);
BOOL mstiMasterPort(mstp_port_t *port);

/** newInfoXst
 *
 * Somewhat ill-defined, this variable is used by MSTI Ports to
 * communicate to the common, shared CIST/MSTI variable.
 */
#define newInfoXst(t,v) do { if(isCistPort(t)) t->cistport->newInfo = v; else t->cistport->newInfoMsti = v; } while(0)

/** updtXstInfo
 *
 * Somewhat ill-defined, this variable is used by CIST/MSTI Ports to
 * refer to the separate conditions for CIST/MSTI ports
 */
#define updtXstInfo(t)  (isCistPort(t) ? updtCistMsg(t) : updtMstiMsg(t))

BOOL updtCistMsg(const mstp_port_t *port);
BOOL updtMstiMsg(const mstp_port_t *port);

/** rcvdXstMsg
 *
 * Somewhat ill-defined, this variable is used by CIST/MSTI Ports to
 * refer to the separate conditions for CIST/MSTI ports
 */
#define rcvdXstMsg(t)   (isCistPort(t) ? rcvdCistMsg(t) : rcvdMstiMsg(t))

BOOL rcvdCistMsg(const mstp_port_t *port);
BOOL rcvdMstiMsg(const mstp_port_t *port);

void recordMastered(mstp_port_t *port);
#ifdef __cplusplus
}
#endif

#endif /* _MSTP_UTIL_H_ */

