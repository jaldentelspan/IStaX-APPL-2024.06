/*

 Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "main.h"
#include "xxrp_trace.h"
#include "vtss_xxrp_types.hxx"
#include "vtss_xxrp_api.h"
#include "vtss_xxrp_callout.h"

namespace vtss {
namespace mrp {

static const Array<std::string, 11> registrar_events2text = {"Begin!",
                                                             "rNew!",
                                                             "rJoinIn!",
                                                             "rJoinMt!",
                                                             "rLv!",
                                                             "rLA!",
                                                             "txLA!",
                                                             "Re-declare!",
                                                             "Flush!",
                                                             "leavetimer!",
                                                             "Invalid!"
                                                            };

ostream &operator<<(ostream &o, xxrp_registrar_events &event)
{
    o << registrar_events2text[event];
    return o;
}

static const Array<std::string, 17> applicant_events2text = {"Begin!",
                                                             "New!",
                                                             "Join!",
                                                             "Lv!",
                                                             "rNew!",
                                                             "rJoinIn!",
                                                             "rIn!",
                                                             "rJoinMt!",
                                                             "rMt!",
                                                             "rLv!",
                                                             "rLA!",
                                                             "Re-declare!",
                                                             "periodic!",
                                                             "tx!",
                                                             "txLA!",
                                                             "txLAF!",
                                                             "Invalid!"
                                                            };

ostream &operator<<(ostream &o, xxrp_applicant_events &event)
{
    o << applicant_events2text[event];
    return o;
}

static const MrpRegStm mrp_reg_stm[XXRP_REG_ST_CNT][XXRP_EV_REG_CNT]
= {
    {   /* State: IN */
        {XXRP_MT,         XXRP_CB_REG_DO_NOTHING},
        {XXRP_IN,         XXRP_CB_REG_NEW},
        {XXRP_IN,         XXRP_CB_REG_DO_NOTHING},
        {XXRP_IN,         XXRP_CB_REG_DO_NOTHING},
        {XXRP_LV,         XXRP_CB_REG_START_TIMER},
        {XXRP_LV,         XXRP_CB_REG_START_TIMER},
        {XXRP_LV,         XXRP_CB_REG_START_TIMER},
        {XXRP_LV,         XXRP_CB_REG_START_TIMER},
        {XXRP_MT,         XXRP_CB_REG_DO_NOTHING},
        {XXRP_REG_ST_CNT, XXRP_CB_REG_DO_NOTHING}
    },
    {   /* State: LV */
        {XXRP_MT,         XXRP_CB_REG_DO_NOTHING},
        {XXRP_IN,         XXRP_CB_REG_STOP_TIMER_NEW},
        {XXRP_IN,         XXRP_CB_REG_STOP_TIMER},
        {XXRP_IN,         XXRP_CB_REG_STOP_TIMER},
        {XXRP_REG_ST_CNT, XXRP_CB_REG_DO_NOTHING},
        {XXRP_REG_ST_CNT, XXRP_CB_REG_DO_NOTHING},
        {XXRP_REG_ST_CNT, XXRP_CB_REG_DO_NOTHING},
        {XXRP_REG_ST_CNT, XXRP_CB_REG_DO_NOTHING},
        {XXRP_MT,         XXRP_CB_REG_LEAVE},
        {XXRP_MT,         XXRP_CB_REG_LEAVE}
    },
    {   /* State: MT */
        {XXRP_MT,         XXRP_CB_REG_DO_NOTHING},
        {XXRP_IN,         XXRP_CB_REG_NEW},
        {XXRP_IN,         XXRP_CB_REG_JOIN},
        {XXRP_IN,         XXRP_CB_REG_JOIN},
        {XXRP_REG_ST_CNT, XXRP_CB_REG_DO_NOTHING},
        {XXRP_REG_ST_CNT, XXRP_CB_REG_DO_NOTHING},
        {XXRP_REG_ST_CNT, XXRP_CB_REG_DO_NOTHING},
        {XXRP_REG_ST_CNT, XXRP_CB_REG_DO_NOTHING},
        {XXRP_MT,         XXRP_CB_REG_DO_NOTHING},
        {XXRP_MT,         XXRP_CB_REG_DO_NOTHING}
    }
};

static const MrpAppStm mrp_app_stm[XXRP_APP_ST_CNT][XXRP_EV_APP_CNT]
= {
    {   /* State: VO */
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_VN,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_VP,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_AO,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_LO,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_LO,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_LO,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_SEND_INVALID},
        {XXRP_LO,         XXRP_CB_APP_SEND_INVALID},
        {XXRP_LO,         XXRP_CB_APP_DO_NOTHING}
    },
    {   /* State: VP */
        {XXRP_VO,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_VN,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_VO,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_AP,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_AA,         XXRP_CB_APP_SEND_JOIN},
        {XXRP_AA,         XXRP_CB_APP_SEND},
        {XXRP_VP,         XXRP_CB_APP_DO_NOTHING}
    },
    {   /* State: VN */
        {XXRP_VO,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_LA,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_AN,         XXRP_CB_APP_SEND_NEW},
        {XXRP_AN,         XXRP_CB_APP_SEND_NEW},
        {XXRP_VN,         XXRP_CB_APP_DO_NOTHING}
    },
    {   /* State: AN */
        {XXRP_VO,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_LA,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_VN,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_VN,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_VN,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_QA,         XXRP_CB_APP_SEND_NEW},
        {XXRP_QA,         XXRP_CB_APP_SEND_NEW},
        {XXRP_VN,         XXRP_CB_APP_DO_NOTHING}
    },
    {   /* State: AA */
        {XXRP_VO,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_VN,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_LA,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_QA,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_QA,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_VP,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_VP,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_VP,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_QA,         XXRP_CB_APP_SEND_JOIN},
        {XXRP_QA,         XXRP_CB_APP_SEND_JOIN},
        {XXRP_VP,         XXRP_CB_APP_DO_NOTHING}
    },
    {   /* State: QA */
        {XXRP_VO,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_VN,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_LA,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_AA,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_AA,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_VP,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_VP,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_VP,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_AA,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_SEND_INVALID},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_SEND_JOIN},
        {XXRP_VP,         XXRP_CB_APP_DO_NOTHING}
    },
    {   /* State: LA */
        {XXRP_VO,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_VN,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_AA,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_VO,         XXRP_CB_APP_SEND_LEAVE},
        {XXRP_LO,         XXRP_CB_APP_SEND_INVALID},
        {XXRP_LO,         XXRP_CB_APP_DO_NOTHING}
    },
    {   /* State: AO */
        {XXRP_VO,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_VN,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_AP,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_QO,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_LO,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_LO,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_LO,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_SEND_INVALID},
        {XXRP_LO,         XXRP_CB_APP_SEND_INVALID},
        {XXRP_LO,         XXRP_CB_APP_DO_NOTHING}
    },
    {   /* State: QO */
        {XXRP_VO,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_VN,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_QP,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_AO,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_AO,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_LO,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_LO,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_LO,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_SEND_INVALID},
        {XXRP_LO,         XXRP_CB_APP_SEND_INVALID},
        {XXRP_LO,         XXRP_CB_APP_DO_NOTHING}
    },
    {   /* State: AP */
        {XXRP_VO,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_VN,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_AO,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_QP,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_VP,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_VP,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_VP,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_QA,         XXRP_CB_APP_SEND_JOIN},
        {XXRP_QA,         XXRP_CB_APP_SEND_JOIN},
        {XXRP_VP,         XXRP_CB_APP_DO_NOTHING}
    },
    {   /* State: QP */
        {XXRP_VO,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_VN,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_QO,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_AP,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_AP,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_VP,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_VP,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_VP,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_AP,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_SEND_INVALID},
        {XXRP_QA,         XXRP_CB_APP_SEND_JOIN},
        {XXRP_VP,         XXRP_CB_APP_DO_NOTHING}
    },
    {   /* State: LO */
        {XXRP_VO,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_VN,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_VP,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_VO,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_VO,         XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING},
        {XXRP_VO,         XXRP_CB_APP_SEND},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_SEND_INVALID},
        {XXRP_APP_ST_CNT, XXRP_CB_APP_DO_NOTHING}
    }
};

static const MrpLeaveAllStm mrp_leaveall_stm[XXRP_BOOL_ST_CNT][XXRP_EV_LA_CNT]
= {
    {   /* passive */
        {XXRP_PASSIVE,     XXRP_CB_LA_START_TIMER},
        {XXRP_BOOL_ST_CNT, XXRP_CB_LA_DO_NOTHING},
        {XXRP_PASSIVE,     XXRP_CB_LA_START_TIMER},
        {XXRP_ACTIVE,      XXRP_CB_LA_START_TIMER}
    },
    {   /* active */
        {XXRP_PASSIVE, XXRP_CB_LA_START_TIMER},
        {XXRP_PASSIVE, XXRP_CB_LA_SLA},
        {XXRP_PASSIVE, XXRP_CB_LA_START_TIMER},
        {XXRP_ACTIVE,  XXRP_CB_LA_START_TIMER}
    }
};

static const MrpPeriodicStm mrp_periodic_stm[XXRP_BOOL_ST_CNT][XXRP_EV_PER_CNT]
= {
    {   /* State: Passive */
        {XXRP_ACTIVE,      XXRP_CB_PER_START},
        {XXRP_ACTIVE,      XXRP_CB_PER_START},
        {XXRP_BOOL_ST_CNT, XXRP_CB_PER_DO_NOTHING},
        {XXRP_BOOL_ST_CNT, XXRP_CB_PER_DO_NOTHING}
    },
    {   /* State: Active */
        {XXRP_ACTIVE,      XXRP_CB_PER_START},
        {XXRP_BOOL_ST_CNT, XXRP_CB_PER_DO_NOTHING},
        {XXRP_PASSIVE,     XXRP_CB_PER_DO_NOTHING},
        {XXRP_ACTIVE,      XXRP_CB_PER_START_TRIGGER}
    }
};

xxrp_applicant_states MrpMadState::applicant() const
{
    return (xxrp_applicant_states)(0xf & data);
}

void MrpMadState::applicant(xxrp_applicant_states s)
{
    data &= 0xf0;
    data |= (0xf) & s;
}

xxrp_registrar_states MrpMadState::registrar() const
{
    return (xxrp_registrar_states)(0x03 & (data >> 4));
}

void MrpMadState::registrar(xxrp_registrar_states s)
{
    data &= 0xcf;
    data |= ((0xf) & s) << 4;
}

xxrp_registrar_admin MrpMadState::registrar_admin() const
{
    return (xxrp_registrar_admin)(0x0c & (data >> 4));
}

void MrpMadState::registrar_admin(xxrp_registrar_admin s)
{
    data &= 0x3f;
    data |= ((0xf) & s) << 6;
}

Pair <xxrp_registrar_states, xxrp_registrar_cbs>
MrpMadMachines::registrar_next_state(xxrp_registrar_states state, xxrp_registrar_events event)
{
    return Pair <xxrp_registrar_states, xxrp_registrar_cbs>
           (mrp_reg_stm[state][event].state, mrp_reg_stm[state][event].cb_enum);
}

Pair <xxrp_applicant_states, xxrp_applicant_cbs>
MrpMadMachines::applicant_next_state(xxrp_applicant_states state, xxrp_applicant_events event)
{
    /* Any STM transition optimizations should be placed here */
    return Pair <xxrp_applicant_states, xxrp_applicant_cbs>
           (mrp_app_stm[state][event].state, mrp_app_stm[state][event].cb_enum);
}

MrpMadMachines::MrpMadMachines(MrpMad &mad)
    : parent_(mad)
{
    static vlan_registration_type_t reg_admin_status[VTSS_APPL_VLAN_ID_MAX + 1];

    /* Initialize data array */
    /* 1: Set Applicant and Registrar STMs in their Begin! states */
    const vtss::VlanList &v = parent_.parent_.vlan_list();
    for (auto it = v.begin(); it != v.end(); ++it)
    {
        data.emplace_back( MrpMadState(XXRP_MT, XXRP_VO, XXRP_NORMAL) );
    }

    (void)xxrp_mgmt_vlan_state(parent_.port_, reg_admin_status);
    for_all_machines([&] (mesa_vid_t v, MrpMadState & s) {
        if (reg_admin_status[v] == VLAN_REGISTRATION_TYPE_FIXED) {
            /* 2: Set the registrar state */
            s.registrar(XXRP_IN);
            /* 3: Trigger Join! event on this port's applicant */
            handle_event_app(XXRP_EV_APP_JOIN, v);
            /* 4: The Join! propagation will take place next */
            /*    This is done in the MAP propagate_join()   */
        }
        s.registrar_admin((xxrp_registrar_admin)reg_admin_status[v]);
    });
}

mesa_rc MrpMadMachines::registrar_do_callback(xxrp_registrar_cbs cb_enum,
        u32 port_no, mesa_vid_t v)
{
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "Do callback: Registrar FSM"
            << " on port " << parent_.port_;
    switch (cb_enum) {
    case XXRP_CB_REG_DO_NOTHING:
        return VTSS_RC_OK;
    case XXRP_CB_REG_NEW:
        // Not yet supported
        return VTSS_RC_OK;
    case XXRP_CB_REG_JOIN:
        return join_indication(port_no, v);
    case XXRP_CB_REG_LEAVE:
        return leave_indication(port_no, v);
    case XXRP_CB_REG_START_TIMER:
        parent_.start_leave_timer();
        return VTSS_RC_OK;
    case XXRP_CB_REG_STOP_TIMER:
        parent_.stop_leave_timer();
        return VTSS_RC_OK;
    case XXRP_CB_REG_STOP_TIMER_NEW:
        // Not yet supported
        return VTSS_RC_OK;
    default:
        return XXRP_ERROR_INVALID_PARAMETER;
    }
}

// This can be entirely replaced by the overloaded version
mesa_rc MrpMadMachines::applicant_do_callback(xxrp_applicant_cbs cb_enum)
{
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "Do callback: Applicant FSM"
            << " on port " << parent_.port_;
    switch (cb_enum) {
    case XXRP_CB_APP_DO_NOTHING:
        return VTSS_RC_OK;
    case XXRP_CB_APP_SEND_NEW:
    case XXRP_CB_APP_SEND_JOIN:
    case XXRP_CB_APP_SEND:
    case XXRP_CB_APP_SEND_LEAVE:
    case XXRP_CB_APP_SEND_INVALID:
    default:
        return XXRP_ERROR_INVALID_PARAMETER;
    }
}

mesa_rc MrpMadMachines::applicant_do_callback(xxrp_applicant_cbs cb_enum, mesa_vid_t v,
        u8 * all_events, u32 & total_events)
{
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "Do callback: Applicant FSM"
            << " on port " << parent_.port_;
    switch (cb_enum) {
    case XXRP_CB_APP_DO_NOTHING:
        return VTSS_RC_OK;
    case XXRP_CB_APP_SEND_NEW:
        MRP_SET_EVENT(all_events, v, MRP_EVENT_NEW);
        total_events++;
        return VTSS_RC_OK;
    case XXRP_CB_APP_SEND_JOIN:
        if (registrar(v) == XXRP_IN) {
            MRP_SET_EVENT(all_events, v, MRP_EVENT_JOININ);
        } else {
            MRP_SET_EVENT(all_events, v, MRP_EVENT_JOINMT);
        }
        total_events++;
        return VTSS_RC_OK;
    case XXRP_CB_APP_SEND:
        if (registrar(v) == XXRP_IN) {
            MRP_SET_EVENT(all_events, v, MRP_EVENT_IN);
        } else {
            MRP_SET_EVENT(all_events, v, MRP_EVENT_MT);
        }
        total_events++;
        return VTSS_RC_OK;
    case XXRP_CB_APP_SEND_LEAVE:
        MRP_SET_EVENT(all_events, v, MRP_EVENT_LV);
        total_events++;
        return VTSS_RC_OK;
    case XXRP_CB_APP_SEND_INVALID:
        MRP_SET_EVENT(all_events, v, MRP_EVENT_INVALID);
        return VTSS_RC_OK;
    default:
        return XXRP_ERROR_INVALID_PARAMETER;
    }
}

template <typename F>
void MrpMadMachines::for_all_machines_in_msti(F f, vtss_appl_mstp_msti_t msti) {
    const VlanList &v = parent_.parent_.vlan_list();
    mstp_msti_config_t msti_config;

    if (vtss_appl_mstp_msti_config_get(&msti_config, NULL) == VTSS_RC_OK) {
        for (auto it = v.begin(); it != v.end(); ++it) {
            if (msti_config.map.map[*it] == msti) {
                f(*it, data[vlan2index(*it)]);
            }
        }
    }
}

mesa_rc MrpMadMachines::handle_event_reg(xxrp_registrar_events event, u32 port_no,
        mesa_vid_t v)
{
    Pair<xxrp_registrar_states, xxrp_registrar_cbs> reg_pair;

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "Handle Registrar FSM event: "
            << event << " for VLAN " << v << " on port " << parent_.port_;
    reg_pair = registrar_next_state(registrar(v), event);
    if (reg_pair.first != XXRP_REG_ST_CNT) {
        registrar(v, reg_pair.first);
    }
    return registrar_do_callback(reg_pair.second, port_no, v);
}

mesa_rc MrpMadMachines::handle_event_app(xxrp_applicant_events event, mesa_vid_t v)
{
    Pair<xxrp_applicant_states, xxrp_applicant_cbs> app_pair;

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "Handle Applicant FSM event: "
            << event << " for VLAN " << v << " on port " << parent_.port_;
    /* 0: Get next state / action pair */
    app_pair = applicant_next_state(applicant(v), event);
    /* 1: Change the applicant state if needed */
    if (app_pair.first != XXRP_APP_ST_CNT) {
        applicant(v, app_pair.first);
    }
    /* 2: Request a transmit opportunity if entering an appropriate state */
    if (requires_tx(app_pair.first)) {
        parent_.request_tx();
    }
    /* 3: Execute the corresponding callback */
    return applicant_do_callback(app_pair.second);
}

mesa_rc MrpMadMachines::handle_event_app(xxrp_applicant_events event, mesa_vid_t v,
        u8 * all_events, u32 & total_events)
{
    Pair<xxrp_applicant_states, xxrp_applicant_cbs> app_pair;

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "Handle Applicant FSM event: "
            << event << " for VLAN " << v << " on port " << parent_.port_;
    /* 0: Get next state / action pair */
    app_pair = applicant_next_state(applicant(v), event);
    /* 1: Change the applicant state if needed */
    if (app_pair.first != XXRP_APP_ST_CNT) {
        applicant(v, app_pair.first);
    }
    /* 2: Request a transmit opportunity if entering an appropriate state */
    if (requires_tx(app_pair.first)) {
        parent_.request_tx();
    }
    /* 3: Execute the corresponding callback */
    return applicant_do_callback(app_pair.second, v, all_events, total_events);
}

bool MrpMadMachines::requires_tx(xxrp_applicant_states state)
{
    xxrp_applicant_states states[] = {XXRP_VP, XXRP_VN, XXRP_AN, XXRP_AA,
                                      XXRP_LA, XXRP_AP, XXRP_LO
                                     };
    xxrp_applicant_states *p;

    p =  vtss::find(states, states + 7, state);
    return (p != states + 7) ? true : false;
}

size_t MrpMadMachines::vlan2index(mesa_vid_t v) const
{
    return parent_.parent_.vlan2index(v);
}

bool MrpMadMachines::not_declaring(mesa_vid_t v)
{
    xxrp_applicant_states states[] = {XXRP_VO, XXRP_AO, XXRP_QO, XXRP_LO};
    xxrp_applicant_states *p;
    xxrp_applicant_states state = applicant(v);

    p = vtss::find(states, states + 4, state);
    return (p != states + 4) ? true : false;
}

mesa_rc MrpMadMachines::join_indication(u32 port_no, mesa_vid_t v)
{
    mesa_rc rc {VTSS_RC_OK};

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "MAD_Join.indication on port "
            << port_no;
    /* 1: Request the VLAN module to register this VID */
    rc = XXRP_mvrp_vlan_port_membership_add(port_no, v);
    /* IEEE 802.1Q - 2014 defines the FailedRegistrations statistic for
       each MRP application, on a per-port basis. For MVRP, this is the
       number of failed registration attempts due to lack of space in the
       filtering database.
       However, in our implementation, there is always space in the FDB and
       the above call can never fail that way. (It may fail due to coding errors though)
       So, this statistic is always 0 */
    // In case of an error in the registration, increment relevant counter
    if (rc != VTSS_RC_OK) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, INFO) << "That is unexpected!";
    }
    /* 2: Propagate MAD_Join.request to other participants */
    rc = parent_.parent_.map->propagate_join(port_no, v);

    return rc;
}

mesa_rc MrpMadMachines::leave_indication(u32 port_no, mesa_vid_t v)
{
    mesa_rc rc {VTSS_RC_OK};

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "MAD_Leave.indication on port "
            << port_no;
    /* 1: Request the VLAN module to deregister this VID */
    rc = XXRP_mvrp_vlan_port_membership_del(port_no, v);
    /* 2: Propagate MAD_Leave.request to other participants */
    /*    only if this is last port to have the VID registered */
    if (parent_.parent_.map->last_of_set(port_no, v)) {
        rc = parent_.parent_.map->propagate_leave(port_no, v);
    }

    return rc;
}

MrpMad::MrpMad(TimerService &ts, u32 p, MrpAppl &parent)
    : notifications::EventHandler(&ts),
      timer_service(ts),
      joinTimer(this),
      leaveTimer(this),
      periodicTimer(this),
      leaveAllTimer(this),
      parent_(parent),
      port_(p),
      regApp(*this)
{
    if (parent_.periodic_state_[port_]) {
        periodic_state = XXRP_ACTIVE;
        start_periodic_timer();
    } else {
        periodic_state = XXRP_PASSIVE;
    }

    leaveall_state = XXRP_PASSIVE;
    start_leaveall_timer();
}

MrpMad::~MrpMad()
{
    VTSS_ASSERT(executing_ == false);
    /* Just so there are no leftovers, delete any timers */
    if (joinTimerFlag) { timer_service.timer_del(joinTimer); }
    if (leaveTimerFlag) { timer_service.timer_del(leaveTimer); }
    if (leaveAllTimerFlag) { timer_service.timer_del(leaveAllTimer); }
    if (periodicTimerFlag) { timer_service.timer_del(periodicTimer); }
    /* And deregister all registered VLANS for the MVRP user */
    regApp.for_all_machines([&] (mesa_vid_t v, MrpMadState & s) {
        if (s.registrar() != XXRP_MT) {
            (void)XXRP_mvrp_vlan_port_membership_del(port_, v);
        }
    });
}

void MrpMad::execute(notifications::Timer *t)
{
    executing_ = true;
    if (t == &joinTimer) {
        joinTimerFlag = false;
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, DEBUG) << "MRP joinTimer expired";
        bool txLA {false};
        u8 all_events[XXRP_MAX_ATTR_EVENTS_ARR];
        u32 total_events {0};

        memset(all_events, MRP_EVENT_INVALID_BYTE, sizeof(all_events));

        /* Check if we have tx! or txLA! */
        if (leaveall_state == XXRP_ACTIVE) {
            txLA = true;
            (void)handle_event_leaveall(XXRP_EV_LA_TX);
        }
        if (txLA) {
            regApp.for_all_machines_forwarding([&] (mesa_vid_t v, MrpMadState & s) {
                regApp.handle_event_app(XXRP_EV_APP_TXLA, v, all_events, total_events);
            });
        } else {
            regApp.for_all_machines_forwarding([&] (mesa_vid_t v, MrpMadState & s) {
                regApp.handle_event_app(XXRP_EV_APP_TX, v, all_events, total_events);
            });
        }
        parent_.vtss_mvrp_tx(port_, all_events, total_events, txLA);
        if (txLA) {
            regApp.for_all_machines([&] (mesa_vid_t v, MrpMadState & s) {
                regApp.handle_event_app(XXRP_EV_APP_RLEAVEALL, v);
                if (regApp.registrar_admin(v) == XXRP_NORMAL) {
                    regApp.handle_event_reg(XXRP_EV_REG_TX_LEAVEALL, port_, v);
                    regApp.handle_event_reg(XXRP_EV_REG_RLEAVEALL, port_, v);
                }
            });
        }
    } else if (t == &leaveTimer) {
        leaveTimerFlag = false;
        leaveTimerCount = 0;
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, DEBUG) << "MRP leaveTimer expired";
        regApp.for_all_machines([&] (mesa_vid_t v, MrpMadState & s) {
            if (regApp.registrar_admin(v) == XXRP_NORMAL) {
                regApp.handle_event_reg(XXRP_EV_REG_TIMER, port_, v);
            }
        });
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, DEBUG) << "count: " << leaveTimerCount;
    } else if (t == &periodicTimer) {
        periodicTimerFlag = false;
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, DEBUG) << "MRP periodicTimer expired";
        VTSS_ASSERT(this!=nullptr);
        (void)this->handle_event_periodic(XXRP_EV_PER_TIMER);
    } else if (t == &leaveAllTimer) {
        leaveAllTimerFlag = false;
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, DEBUG) << "MRP leaveAllTimer expired";
        VTSS_ASSERT(this!=nullptr);
        (void)this->handle_event_leaveall(XXRP_EV_LA_TIMER);
    }
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, DEBUG) << "MRP joinTimer expired done";
    executing_ = false;
}

void MrpMad::update_peer_mac(u8 * mac_addr)
{
    memcpy(peer_mac_address, mac_addr, sizeof(peer_mac_address));
    return;
}

void MrpMad::request_tx()
{
    /* NOTE: joinTimer can be either restarted if it is already
       running or kept as is. The following implementation is
       keeping the timer running as is. This will speed up the
       response time of the protocol. The standard is quite
       abstract on whether a restart should occur or not.
       Maybe in the future we do it this way and test the
       performance of the protocol.
       Reference: 10.7.1 Notational conventions and abbreviations
       IEEE 802.1Q 2014 */
    if (!joinTimerFlag) {
        // Start timer and make a record of this in the flag
        /* Join Timer is random in the range 1..join_time */
        milliseconds time = (milliseconds) (rand() % parent_.timeouts[port_].join.raw32() + 1);
        timer_service.timer_add(joinTimer, time);
        joinTimerFlag = true;
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, DEBUG) << "MRP joinTimer started on port "
                << port_ << " - "
                << time.raw32() << "ms";
    }
}

void MrpMad::start_periodic_timer()
{
    /* Periodic Timer is fixed to 1sec */
    milliseconds time = (milliseconds) 1000;
    if (!periodicTimerFlag) {
        timer_service.timer_add(periodicTimer, time);
        periodicTimerFlag = true;
    } else {
        timer_service.timer_del(periodicTimer);
        timer_service.timer_add(periodicTimer, time);
    }
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, DEBUG) << "MRP periodicTimer started on port "
            << port_ << " - "
            << time.raw32() << "ms";
}

void MrpMad::start_leaveall_timer()
{
    /* LeaveAll Timer is random in the range leaveall_time..1.5*leaveall_time */
    milliseconds time = (milliseconds) (rand() % (parent_.timeouts[port_].leaveAll.raw32() / 2) + parent_.timeouts[port_].leaveAll.raw32());
    if (!leaveAllTimerFlag) {
        timer_service.timer_add(leaveAllTimer, time);
        leaveAllTimerFlag = true;
    } else {
        timer_service.timer_del(leaveAllTimer);
        timer_service.timer_add(leaveAllTimer, time);
    }
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, DEBUG) << "MRP leaveAllTimer started on port "
            << port_ << " - "
            << time.raw32() << "ms";
}

void MrpMad::start_leave_timer()
{
    milliseconds time = parent_.timeouts[port_].leave;
    if (!leaveTimerFlag) {
        // Start timer and make a record of this in the flag
        timer_service.timer_add(leaveTimer, time);
        leaveTimerFlag = true;
    } else {
        /* Small *hack* - If the timer is running, restart it
           That ensures that the 'individual' STM timer will run
           for at least leave_time before it expires */
        timer_service.timer_del(leaveTimer);
        timer_service.timer_add(leaveTimer, time);
    }
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, DEBUG) << "MRP leaveTimer started on port "
            << port_ << " - "
            << time.raw32() << "ms";
    leaveTimerCount++;
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, DEBUG) << "count: " << leaveTimerCount;
}

void MrpMad::stop_leave_timer()
{
    if (leaveTimerCount == 0) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, WARNING) << "MRP LeaveTimer is not running";
        return;
    }
    if ((leaveTimerFlag) && (leaveTimerCount == 1)) {
        // Stop timer and make a record of this in the flag
        timer_service.timer_del(leaveTimer);
        leaveTimerFlag = false;
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, DEBUG) << "MRP leaveTimer stopped";
    }
    leaveTimerCount--;
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, DEBUG) << "count: " << leaveTimerCount;
}

mesa_rc MrpMad::redeclare(vtss_appl_mstp_msti_t msti)
{
    regApp.for_all_machines_in_msti([&] (mesa_vid_t v, MrpMadState & s) {
        regApp.handle_event_app(XXRP_EV_APP_REDECLARE, v);
        if (regApp.registrar_admin(v) == XXRP_NORMAL) {
            regApp.handle_event_reg(XXRP_EV_REG_REDECLARE, port_, v);
        }
    }, msti);
    return VTSS_RC_OK;
}

mesa_rc MrpMad::flush(vtss_appl_mstp_msti_t msti)
{
    regApp.for_all_machines_in_msti([&] (mesa_vid_t v, MrpMadState & s) {
        if (regApp.registrar_admin(v) == XXRP_NORMAL) {
            regApp.handle_event_reg(XXRP_EV_REG_FLUSH, port_, v);
        }
    }, msti);
    return handle_event_leaveall(XXRP_EV_LA_TIMER);
}

mesa_rc MrpMad::transmit_leave(vtss_appl_mstp_msti_t msti)
{
    u8 all_events[XXRP_MAX_ATTR_EVENTS_ARR];
    u32 total_events {0};

    memset(all_events, MRP_EVENT_INVALID_BYTE, sizeof(all_events));

    regApp.for_all_machines_in_msti([&] (mesa_vid_t v, MrpMadState & s) {
        if (regApp.not_declaring(v)) {
            /* Indirect access of encode event INVALID */
            regApp.applicant_do_callback(XXRP_CB_APP_SEND_INVALID, v, all_events, total_events);
        } else {
            /* Indirect access of encode event LEAVE */
            regApp.applicant_do_callback(XXRP_CB_APP_SEND_LEAVE, v, all_events, total_events);
        }
    }, msti);
    parent_.vtss_mvrp_tx(port_, all_events, total_events, false);
    return VTSS_RC_OK;
}

Pair <xxrp_bool_states, xxrp_leaveall_cbs>
MrpMad::leaveall_next_state(xxrp_bool_states state, xxrp_leaveall_events event)
{
    return Pair <xxrp_bool_states, xxrp_leaveall_cbs>
           (mrp_leaveall_stm[state][event].state,
            mrp_leaveall_stm[state][event].cb_enum);
}

mesa_rc MrpMad::leaveall_do_callback(xxrp_leaveall_cbs cb_enum)
{
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, DEBUG) << "Do callback: LeaveAll FSM"
            << " on port " << port_;
    switch (cb_enum) {
    case XXRP_CB_LA_DO_NOTHING:
        return VTSS_RC_OK;
    case XXRP_CB_LA_START_TIMER:
        start_leaveall_timer();
        return VTSS_RC_OK;
    case XXRP_CB_LA_SLA:
        // processed in execute()
        return VTSS_RC_OK;
    default:
        return XXRP_ERROR_INVALID_PARAMETER;
    }
}

mesa_rc MrpMad::handle_event_leaveall(xxrp_leaveall_events event)
{
    Pair<xxrp_bool_states, xxrp_leaveall_cbs> leaveall_pair;

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, DEBUG) << "Handle event: LeaveAll FSM"
            << " on port " << port_;
    /* 0: Get next state / action pair */
    leaveall_pair = leaveall_next_state(leaveall_state, event);
    /* 1: Change the leaveall state if needed */
    if (leaveall_pair.first != XXRP_BOOL_ST_CNT) {
        leaveall_state = leaveall_pair.first;
    }
    /* 2: Request a transmit opportunity if entering an appropriate state */
    if (leaveall_pair.first == XXRP_ACTIVE) {
        request_tx();
    }
    /* 3: Execute the corresponding callback */
    return leaveall_do_callback(leaveall_pair.second);
}

Pair <xxrp_bool_states, xxrp_periodic_cbs>
MrpMad::periodic_next_state(xxrp_bool_states state, xxrp_periodic_events event)
{
    return Pair <xxrp_bool_states, xxrp_periodic_cbs>
           (mrp_periodic_stm[state][event].state, mrp_periodic_stm[state][event].cb_enum);
}

mesa_rc MrpMad::periodic_do_callback(xxrp_periodic_cbs cb_enum)
{
    switch (cb_enum) {
    case XXRP_CB_PER_DO_NOTHING:
        return VTSS_RC_OK;
    case XXRP_CB_PER_START:
        start_periodic_timer();
        return VTSS_RC_OK;
    case XXRP_CB_PER_START_TRIGGER:
        /* 1: Trigger periodic! on all applicant FSMs on this port */
        regApp.for_all_machines([&] (mesa_vid_t v, MrpMadState & s) {
            regApp.handle_event_app(XXRP_EV_APP_PERIODIC, v);
        });
        /* 2: Then start the timer once again */
        start_periodic_timer();
        return VTSS_RC_OK;
    default:
        return XXRP_ERROR_INVALID_PARAMETER;
    }
}

mesa_rc MrpMad::handle_event_periodic(xxrp_periodic_events event)
{
    Pair<xxrp_bool_states, xxrp_periodic_cbs> periodic_pair;

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, DEBUG) << "Handle event: Periodic FSM"
            << " on port " << port_;
    /* 0: Get next state / action pair */
    periodic_pair = periodic_next_state(periodic_state, event);
    /* 1: Change the periodic state if needed */
    if (periodic_pair.first != XXRP_BOOL_ST_CNT) {
        periodic_state = periodic_pair.first;
    }
    /* 2: Execute the corresponding callback */
    return periodic_do_callback(periodic_pair.second);
}

bool MrpMad::operPointToPointMAC()
{
    mstp_port_mgmt_status_t ps;

    memset(&ps, 0, sizeof(mstp_port_mgmt_status_t));
    if ((mstp_get_port_status(0, port_, &ps)) && (ps.core.operPointToPointMAC)) {
        return true;
    } else {
        return false;
    }
}

MrpMap::MrpMap(MrpAppl & parent)
    : parent_(parent)
{
    vtss_appl_mstp_msti_t msti {};

    /* When the MAP is constructed, no MADs are there yet */
    /* So, make the map empty, and it will be filled in later */
    /* when each MAD gets constructed */
    for (msti = 0; msti < MRP_MSTI_MAX; ++msti) {
        ring[msti].clear();
    }
}

bool MrpMap::last_of_set(u32 port_no, mesa_vid_t v)
{
    vtss_appl_mstp_msti_t msti;

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT , DEBUG) << "Checking if port " << port_no
            << " is the last to have Vlan "
            << v << " registered";
    /* 1: Fetch the MSTI corresponding to that VID */
    if (vtss_appl_mstp_msti_lookup(v, &msti) != VTSS_RC_OK) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, WARNING) << "Error in MSTI lookup of VID "
                << v;
        return false;
    }

    /* Check if the port is a member of the set first and then check the rest */
    auto it = vtss::find(ring[msti].cbegin(), ring[msti].cend(), port_no);
    if (it != ring[msti].cend()) {
        /* 2: Iterate through the MSTI ring, and check the registrar of each port
              for the registration status of that VID */
        int port_count = 0;
        for (it = ring[msti].cbegin(); it != ring[msti].cend(); ++it) {
            if (*it == (u32)port_no) {
                continue;
            }
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, NOISE) << "Inspecting port " << *it
                    << ", member of MSTI " << msti << ", for VLAN " << v << " membership";
            /* If at least two ports have the VID registered, end of story */
            if (parent_.mad[*it]->regApp.registrar(v) != XXRP_MT) {
                port_count++;
            }
            if (port_count > 1) {
                return false;
            }
        }
    }
    /* Only reach this if no other port has the VID registered */
    return true;
}

mesa_rc MrpMap::propagate_join(u32 port_no, vtss_appl_mstp_msti_t msti)
{
    parent_.mad[port_no]->regApp.for_all_machines_in_msti([&] (mesa_vid_t v, MrpMadState & s) {
        if (s.registrar() != XXRP_MT) {
            propagate_join(port_no, v);
        }
    }, msti);
    return VTSS_RC_OK;
}

mesa_rc MrpMap::propagate_join(u32 port_no, mesa_vid_t v)
{
    vtss_appl_mstp_msti_t msti;

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "Propagate Join! for Vlan "
            << v << " initiated by port "
            << port_no;
    /* 1: Fetch the MSTI corresponding to that VID */
    if (vtss_appl_mstp_msti_lookup(v, &msti) != VTSS_RC_OK) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, WARNING) << "Error in MSTI lookup of VID "
                << v;
        return VTSS_RC_ERROR;
    }

    /* Check if the port is a member of the MSTI first and then propagate */
    auto it = vtss::find(ring[msti].cbegin(), ring[msti].cend(), port_no);
    if (it != ring[msti].cend()) {
        /* 2: Iterate through the MSTI ring, and trigger Join! event on the applicant
          of that VID. But not on the port that initiated the propagation. */
        for (it = ring[msti].cbegin(); it != ring[msti].cend(); ++it) {
            if (*it == port_no) {
                continue;
            }
            (void)parent_.mad[*it]->regApp.handle_event_app(XXRP_EV_APP_JOIN, v);
        }
    }

    return VTSS_RC_OK;
}

mesa_rc MrpMap::propagate_leave(u32 port_no, vtss_appl_mstp_msti_t msti)
{
    parent_.mad[port_no]->regApp.for_all_machines_in_msti([&] (mesa_vid_t v, MrpMadState & s) {
        if (s.registrar() != XXRP_MT) {
            if (last_of_set(port_no, v)) {
                propagate_leave(port_no, v);
            }
        }
    }, msti);
    return VTSS_RC_OK;
}

mesa_rc MrpMap::propagate_leave(u32 port_no, mesa_vid_t v)
{
    vtss_appl_mstp_msti_t msti;

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "Propagate Lv! for Vlan "
            << v << " initiated by port "
            << port_no;
    /* 1: Fetch the MSTI corresponding to that VID */
    if (vtss_appl_mstp_msti_lookup(v, &msti) != VTSS_RC_OK) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, WARNING) << "Error in MSTI lookup of VID "
                << v;
        return VTSS_RC_ERROR;
    }

    /* Check if the port is a member of the MSTI first and then propagate */
    auto it = vtss::find(ring[msti].cbegin(), ring[msti].cend(), port_no);
    if (it != ring[msti].cend()) {
        /* 2: Iterate through the MSTI ring, and trigger Lv! event on the applicant
              of that VID. But not on the port that initiated the propagation. */
        for (it = ring[msti].cbegin(); it != ring[msti].cend(); ++it) {
            if (*it == (u32)port_no) {
                continue;
            }
            (void)parent_.mad[*it]->regApp.handle_event_app(XXRP_EV_APP_LEAVE, v);
        }
    }
    return VTSS_RC_OK;
}

mesa_rc MrpMap::trigger_join(u32 port_no, vtss_appl_mstp_msti_t msti)
{
    /* Check if the port is a member of the MSTI first and then propagate */
    auto it = vtss::find(ring[msti].cbegin(), ring[msti].cend(), port_no);
    if (it != ring[msti].cend()) {
        for (it = ring[msti].cbegin(); it != ring[msti].cend(); ++it) {
            if (*it == port_no) {
                continue;
            }
            parent_.mad[*it]->regApp.for_all_machines_in_msti([&] (mesa_vid_t v, MrpMadState & s) {
                if ((s.registrar() != XXRP_MT) && (parent_.mad[port_no]->regApp.not_declaring(v))) {
                    parent_.mad[port_no]->regApp.handle_event_app(XXRP_EV_APP_JOIN, v);
                    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "Triggered Join! "
                            << "for VLAN " << v << " on port " << port_no;
                }
            }, msti);
        }
    }
    return VTSS_RC_OK;
}

mesa_rc MrpMap::trigger_leave(u32 port_no, vtss_appl_mstp_msti_t msti)
{
    parent_.mad[port_no]->regApp.for_all_machines_in_msti([&] (mesa_vid_t v, MrpMadState & s) {
        if (s.registrar() != XXRP_MT) {
            auto it = vtss::find(ring[msti].cbegin(), ring[msti].cend(), port_no);
            if (it != ring[msti].cend()) {
                for (it = ring[msti].cbegin(); it != ring[msti].cend(); ++it) {
                    if (*it == port_no) {
                        continue;
                    }
                    if (parent_.mad[*it]->regApp.registrar(v) != XXRP_MT) {
                        (void)parent_.mad[*it]->regApp.handle_event_app(XXRP_EV_APP_LEAVE, v);
                    }
                }
            }
        }
    }, msti);
    return VTSS_RC_OK;
}

mesa_rc MrpMap::add_port_to_map(u32 port_no, vtss_appl_mstp_msti_t msti)
{
    /* Check if the port is forwarding - won't add a port that is discarding */
    if (mstp_port_state(msti, port_no)) {
        /* It is possible that we receive a "port forwarding" event from STP
           for a port that is already forwarding and in our ring.
           That will happen for ports that are already managed by STP and
           in forwarding state, when STP is disabled simply because
           STP sets all ports to forwarding when it is disabled.
           That is why we need to check if the port is in the ring already
           and avoid any double placements.
           P.S. The find function is not the fastest method, so if speed becomes
           an issue, we might change the ring from 'vector' to 'set' */
        if (vtss::find(ring[msti].cbegin(), ring[msti].cend(), port_no) == ring[msti].cend()) {
            /* 1: First add the port to the corresponding MAP context */
            ring[msti].emplace(ring[msti].end(), port_no);
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "Added port "
                    << port_no << " in the MAP for MSTI " << msti;
            /* 2: Then propagate Join! for each VLAN already registered */
            (void)propagate_join(port_no, msti);
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "Propagated (if any) Join! "
                    << "events in the MAP for MSTI " << msti;
            /* 3: And trigger Join! to this port for each non-declared VLAN */
            (void)trigger_join(port_no, msti);
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "Triggered (if any) Join! "
                    << "event on port " << port_no;
        }
    }
    return VTSS_RC_OK;
}

mesa_rc MrpMap::remove_port_from_map(u32 port_no, vtss_appl_mstp_msti_t msti)
{
    auto it = vtss::find(ring[msti].cbegin(), ring[msti].cend(), port_no);
    if (it != ring[msti].cend()) {
        /* 1: Propagate Lv! for each VLAN registered, if last of set */
        (void)propagate_leave(port_no, msti);
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "Propagated (if any) Lv! "
                << "events in the MAP for MSTI " << msti;
        /* 2: Trigger Lv! for each VLAN registered, to the other ports */
        /*    that also have it registered */
        (void)trigger_leave(port_no, msti);
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "Triggered (if any) Lv! "
                << "event on port " << port_no;
        /* 3: Transmit a Leave msg for each VLAN declared */
        (void)parent_.mad[port_no]->transmit_leave(msti);
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "Transmitted (if any) leave "
                << "message out of port " << port_no;
        /* 4: Now, remove the port from the MAP context */
        ring[msti].erase(it);
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "Removed port "
                << port_no << " from the MAP for MSTI " << msti;
    }
    return VTSS_RC_OK;
}

bool MrpMap::mstp_port_state(vtss_appl_mstp_msti_t msti, u32 port_no)
{
    mstp_port_mgmt_status_t ps;

    memset(&ps, 0, sizeof(mstp_port_mgmt_status_t));
    if (mstp_get_port_status(msti, port_no, &ps)
            && (strncmp(ps.fwdstate, "Forwarding", sizeof("Forwarding")) == 0)) {
        return true;
    } else {
        return false;
    }
}

mesa_rc MrpAppl::global_state(bool state)
{
    if (global_state_ == state) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "MRP is already "
                << ((state == true) ? "enabled" : "disabled");
        return VTSS_RC_OK;
    }

    if (state) {
        /* Construct MAP */
        assert(map == nullptr);
        map = VTSS_CREATE(MrpMap, *this);
        if (!map) {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, WARNING) << "Could not allocate "
                    << "memory for the MRP MAP";
            return VTSS_RC_ERROR;
        }
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "MAP was constructed successfully";
    } else {
        /* Destruct MAP */
        vtss_destroy(map);
        map = nullptr;
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "MAP was destructed successfully";
    }
    global_state_ = state;
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "MRP is now globally "
            << ((state == true) ? "enabled" : "disabled");
    return VTSS_RC_OK;
}

mesa_rc MrpAppl::port_state(u32 port_no, bool state)
{
    if (!global_state()) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "MRP is not enabled!";
        return VTSS_RC_ERROR;
    }

    if (port_state_[port_no] == state) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "MRP is already "
                << ((state == true) ? "enabled" : "disabled")
                << " on port " << port_no;
        return VTSS_RC_OK;
    }

    if (state) {
        /* 1: Construct the MAD for that port */
        assert(mad[port_no] == nullptr);
        mad[port_no] = VTSS_CREATE(MrpMad, ts_, port_no, *this);
        if (!mad[port_no]) {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, WARNING) << "Could not allocate "
                    << "memory for the MRP MAD of port " << port_no;
            return VTSS_RC_ERROR;
        }
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "MAD was constructed "
                << "successfully on port " << port_no;
        /* 2: Now that the MAD is ready, add the port to the MAP contexts  */
        for (int msti = 0; msti < MRP_MSTI_MAX; ++msti) {
            /* The function won't add a discarding port */
            map->add_port_to_map(port_no, msti);
        }
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "MAP was updated "
                << "with the addition of port " << port_no;
    } else {
        /* 1: Before destructing the MAD, remove the port from the MAP contexts  */
        for (int msti = 0; msti < MRP_MSTI_MAX; ++msti) {
            map->remove_port_from_map(port_no, msti);
        }
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "MAP was updated "
                << "with the removal of port " << port_no;
        /* 2: And then destruct the MAD */
        vtss_destroy(mad[port_no]);
        mad[port_no] = nullptr;
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "MAD was destructed "
                << "successfully on port " << port_no;
    }
    port_state_[port_no] = state;
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "MRP is now "
            << ((state == true) ? "enabled" : "disabled")
            << " on port " << port_no;
    return VTSS_RC_OK;
}

mesa_rc MrpAppl::port_timers(u32 port_no,
                             MrpTimeouts t)
{
    timeouts[port_no] = t;
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "MRP Timers are now updated "
            << " on port " << port_no;
    return VTSS_RC_OK;
}

mesa_rc MrpAppl::periodic_state(u32 port_no, const bool state)
{
    /* 1: Update the state record */
    periodic_state_[port_no] = state;
    /* 2: If MRP Participant is present, update the STM as well */
    if (port_state(port_no)) {
        if (state) {
            mad[port_no]->handle_event_periodic(XXRP_EV_PER_ENABLED);
        } else {
            mad[port_no]->handle_event_periodic(XXRP_EV_PER_DISABLED);
        }
    }
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "MRP PeriodicTransmission "
            << "is now " << ((state == true) ? "enabled" : "disabled")
            << " on port " << port_no;
    return VTSS_RC_OK;
}

mesa_rc MrpAppl::vlan_list(const VlanList & vls)
{
    int vid = 0;

    if (!global_state()) {
        // Easy case where MRP is not running - Simply copy vls
        for (vid = XXRP_VLAN_ID_MIN; vid < XXRP_VLAN_ID_MAX; ++vid) {
            (vls.get(vid)) ? vlan_list_.set(vid) : vlan_list_.clear(vid);
        }
        update_vlan2index();
    } else {
        // Need to prepare for any VLAN changes before committing them
        for (vid = XXRP_VLAN_ID_MIN; vid < XXRP_VLAN_ID_MAX; ++vid) {
            if (vlan_list_.get(vid) != vls.get(vid)) {
                if (vls.get(vid)) { // Add managed VLAN
                    add_managed_vlan(vid);
                } else { // Remove managed VLAN
                    remove_managed_vlan(vid);
                }
            }
        }
    }
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "List of managed VLANs for MVRP "
            << " was updated successfully - " << vlan_list_;
    return VTSS_RC_OK;
}

mesa_rc MrpAppl::handle_vlan_change(u32 port_no,
                                    mesa_vid_t v,
                                    vlan_registration_type_t t)
{
    mesa_rc rc {VTSS_RC_OK};
    const VlanList &vls = vlan_list();

    /* Do not process anything for a non-managed VLAN */
    if (!vls.get(v)) {
        return VTSS_RC_OK;
    }
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "Handle Vlan change on port "
            << port_no << " - "
            << "Registrar administrative status "
            << "changed to " << t;
    /* 1: First set the registrar administrative status */
    mad[port_no]->regApp.registrar_admin(v, (xxrp_registrar_admin)t);
    /* 2: then... */
    if (t == VLAN_REGISTRATION_TYPE_FIXED) { // MAD_Join.request
        /* 3: Set the registrar state */
        mad[port_no]->regApp.registrar(v, XXRP_IN);
        /* 4: Trigger Join! event on this port's applicant */
        (void)mad[port_no]->regApp.handle_event_app(XXRP_EV_APP_JOIN, v);
        /* 5: Propagate Join! event on the applicants of all other ports of the MSTI set */
        rc = map->propagate_join(port_no, v);
    } else { // MAD_Leave.request
        /* 3: Set the registrar state */
        mad[port_no]->regApp.registrar(v, XXRP_MT);
        /* 4: Trigger Lv! event on this port's applicant */
        (void)mad[port_no]->regApp.handle_event_app(XXRP_EV_APP_LEAVE, v);
        /* 5: Propagate Lv! event on the applicants of all other ports of the MSTI set */
        /*    only if this is last port to have the VID registered */
        if (map->last_of_set(port_no, v)) {
            rc = map->propagate_leave(port_no, v);
        }
    }
    return rc;
}

mesa_rc MrpAppl::handle_port_state_change(u32 port_no, vtss_appl_mstp_msti_t msti,
        port_states state)
{
    if ((state != MRP_FORWARDING) && (state != MRP_DISCARDING)) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "Invalid MSTP port state";
        return VTSS_RC_OK;
    }
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "Port " << port_no
            << " changed state to "
            << ((state == MRP_FORWARDING) ? "Forwarding" : "Discarding")
            << " in MAP context " << msti;

    if (global_state()) {
        if (port_state(port_no)) {
            if (state == MRP_FORWARDING) {
                map->add_port_to_map(port_no, msti);
            } else { /* VTSS_MRP_MSTP_PORT_DELETE */
                map->remove_port_from_map(port_no, msti);
            }
        }
    }
    return VTSS_RC_OK;
}

void MrpAppl::handle_port_role_change(u32 port_no, vtss_appl_mstp_msti_t msti,
                                      port_roles role)
{
    if ((role != MRP_DESIGNATED) && (role != MRP_ROOT_ALTERNATE)) {
        // Should never occur since the fn is called when needed only
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, WARNING) << "Huh?"
                << " Invalid MSTP port role : " << role;
        return;
    }
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "Port " << port_no
            << " changed role to "
            << ((role == MRP_DESIGNATED) ? "Designated" : "Root/Alternate")
            << " in MAP context " << msti;

    if (global_state()) {
        if (port_state(port_no)) {
            if (role == MRP_DESIGNATED) {
                mad[port_no]->flush(msti);
            } else {
                mad[port_no]->redeclare(msti);
            }
        }
    }
}

void MrpAppl::update_vlan2index()
{
    const VlanList &v = vlan_list();
    size_t c = 0;

    for (auto it = v.begin(); it != v.end(); ++it) {
        vlan2index_[*it] = c;
        ++c;
    }
}

mesa_rc MrpAppl::add_managed_vlan(const mesa_vid_t vid)
{
    const VlanList &v = vlan_list();
    uint16_t index = 0;
    mesa_vid_t next_vid = 0;
    bool found_pos = false;
    vlan_registration_type_t reg_admin_status[VTSS_APPL_VLAN_ID_MAX + 1] = {};

    print_vlan2index();
    for (auto it = v.begin(); it != v.end(); ++it) {
        if (*it > vid) {
            found_pos = true;
            next_vid = *it;
            break; // Found position
        }
        ++index;
    }
    for (int port = 0; port < L2_MAX_PORTS_; ++port) {
        if (!port_state(port)) {
            continue;
        }
        Vector<MrpMadState> &d = mad[port]->regApp.fetch_data();
        if (found_pos) {
            d.emplace(d.begin() + index, MrpMadState(XXRP_MT, XXRP_VO, XXRP_NORMAL));
        } else {
            d.emplace_back(MrpMadState(XXRP_MT, XXRP_VO, XXRP_NORMAL));
        }
    }
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "Constructed MadStates for the new "
            << "VLAN " << vid << " on all active ports";

    vlan_list_.set(vid);
    if (found_pos) {
        vlan2index_[vid] = index;
        for (auto it = v.begin(); it != v.end(); ++it) {
            if (*it < next_vid) {
                continue;
            }
            vlan2index_[*it] = ++index;
        }
    } else {
        vlan2index_[vid] = index;
    }
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "Updated indexes for the new VLAN";
    print_vlan2index();

    for (int port = 0; port < L2_MAX_PORTS_; ++port) {
        if (!port_state(port)) {
            continue;
        }
        (void)xxrp_mgmt_vlan_state(port, reg_admin_status);
        if (reg_admin_status[vid] == VLAN_REGISTRATION_TYPE_FIXED) {
            /* 1: Set the registrar state */
            mad[port]->regApp.registrar(XXRP_IN);
            /* 2: Trigger Join! event on this port's applicant */
            mad[port]->regApp.handle_event_app(XXRP_EV_APP_JOIN, vid);
            /* 3: The Join! propagation will take place next */
            map->propagate_join(port, vid);
        }
        mad[port]->regApp.registrar_admin(vid, (xxrp_registrar_admin)reg_admin_status[vid]);
    }
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "Propagated Join! (if any) "
            << "for the new VLAN";
    return VTSS_RC_OK;
}

mesa_rc MrpAppl::remove_managed_vlan(const mesa_vid_t vid)
{
    const VlanList &v = vlan_list();
    uint16_t index = 0;
    mesa_vid_t prev_vid = 0;

    print_vlan2index();
    for (auto it = v.begin(); it != v.end(); ++it) {
        if (*it == vid) {
            break; // Found position
        }
        prev_vid = *it;
        ++index;
    }

    for (int port = 0; port < L2_MAX_PORTS_; ++port) {
        if (!port_state(port)) {
            continue;
        }
        if (mad[port]->regApp.registrar(vid) != XXRP_MT) {
            /* 1: Set the registrar state */
            mad[port]->regApp.registrar(vid, XXRP_MT);
            /* 2: Request the VLAN module to deregister this VID */
            (void)XXRP_mvrp_vlan_port_membership_del(port, vid);
            /* 3: Trigger Lv! event on this port's applicant */
            (void)mad[port]->regApp.handle_event_app(XXRP_EV_APP_LEAVE, vid);
            /* 4: Propagate Lv! event on the applicants of all other ports of the MSTI set
                  only if this is last port to have the VID registered */
            if (map->last_of_set(port, vid)) {
                map->propagate_leave(port, vid);
            }
        }
    }
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "Propagated Lv! (if any) "
            << "for the old VLAN";

    for (int port = 0; port < L2_MAX_PORTS_; ++port) {
        if (!port_state(port)) {
            continue;
        }
        Vector<MrpMadState> &d = mad[port]->regApp.fetch_data();
        d.erase(d.begin() + vlan2index_[vid]);
    }
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "Destructed MadStates for the old "
            << "VLAN " << vid << " on all active ports";

    vlan_list_.clear(vid);
    for (auto it = v.begin(); it != v.end(); ++it) {
        if (*it <= prev_vid) {
            continue;
        }
        vlan2index_[*it] = index++;
    }
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "Updated indexes for the old VLAN";
    print_vlan2index();
    return VTSS_RC_OK;
}

void MrpAppl::print_vlan2index()
{
    const VlanList &v = vlan_list();
    for (auto it = v.begin(); it != v.end(); ++it) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, RACKET) << vlan2index_[*it];
    }
}

} // namespace mrp
} // namespace vtss
