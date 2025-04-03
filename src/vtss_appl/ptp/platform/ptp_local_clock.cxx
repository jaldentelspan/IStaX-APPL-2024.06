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

/* ptp_local_clock.c */

#include "vtss_ptp_local_clock.h"   /* define callouts from base part */
#include "ptp_local_clock.h"        /* define platform specific part */
#include "vtss_tod_api.h"
#include "ptp.h"
#include "ptp_api.h"
#include <sys/time.h>
#include "critd_api.h"
#include "misc_api.h"
#include "vtss_tod_mod_man.h"
#include "tod_api.h"

#define CLOCK_ADJ_SLEW_RATE (10000LL*ADJ_SCALE)
#if defined(VTSS_SW_OPTION_SYNCE_DPLL)
#include "synce_custom_clock_api.h"
#if defined(VTSS_SW_OPTION_ZLS30387)
#include "zl_3038x_api_pdv_api.h"
#endif //
#endif

// Since it is difficult to keep 1pps and soft clock in sync, actual LTC
// time would be adjusted once in a while. Suppose SOFT_CLOCK_PPS_SYNC_CNT
// is 60, then chip LTC would adjusted when 'vtss_local_clock_ratio_set' is
// is called 60 times.
#define SOFT_CLOCK_PPS_SYNC_CNT 500

// instance specific clock data.
static struct {
    int                             clock_option; /* clock adjustment option */
    vtss_appl_ptp_preferred_adj_t   adj_method;   /* preferred clock adjust method (only relevant for clock domain 0)*/
    uint32_t                        clkDomain;
}   instClkData[PTP_CLOCK_INSTANCES];

static critd_t clockmutex;          /* clock internal data protection */

#define CLOCK_LOCK()   critd_enter(&clockmutex, __FILE__, __LINE__)
#define CLOCK_UNLOCK() critd_exit (&clockmutex, __FILE__, __LINE__)

static uint32_t totalClkDomains = 1 + VTSS_PTP_SW_CLK_DOMAIN_CNT; // total clock domains possible on current device.
static uint32_t hwClkDomains = 1;                                 // Number of hardware or chip clock domains.
static localClockData_t localClock[VTSS_PTP_MAX_CLOCK_DOMAINS];
static sync_clock_feature_t features = SYNCE_CLOCK_FEATURE_NONE; /* pr default no Synce features are available */


static int vtss_ptp_adjustment_option(int instance)
{
    return instClkData[instance].clock_option;
}

static mesa_rc calc_clock_option(vtss_appl_ptp_preferred_adj_t adj_method, vtss_appl_ptp_profile_t profile, bool basic_servo, int *clock_option)
{
    mesa_rc rc = VTSS_RC_OK;
    vtss_appl_ptp_preferred_adj_t my_method;
    bool type_2b = false;
#if defined(VTSS_SW_OPTION_SYNCE_DPLL)
    PTP_RC(clock_features_get(&features));
#endif
    *clock_option = CLOCK_OPTION_INTERNAL_TIMER;
    my_method = adj_method;
    if (my_method == VTSS_APPL_PTP_PREFERRED_ADJ_AUTO) {
        T_IG(_C,"Auto adjust selection");
        if (basic_servo) { // Basic servo which is in-house developed cannot use zarlink dpll.
            my_method = VTSS_APPL_PTP_PREFERRED_ADJ_LTC;
        } else if (profile == VTSS_APPL_PTP_PROFILE_G_8275_1 || profile == VTSS_APPL_PTP_PROFILE_G_8275_2) {
            my_method = VTSS_APPL_PTP_PREFERRED_ADJ_COMMON;
        } else if (profile == VTSS_APPL_PTP_PROFILE_G_8265_1) {
            my_method = VTSS_APPL_PTP_PREFERRED_ADJ_SINGLE;
        } else if (profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
            my_method = VTSS_APPL_PTP_PREFERRED_ADJ_LTC;
        } else {
            my_method = VTSS_APPL_PTP_PREFERRED_ADJ_INDEPENDENT;
        }
    }
    switch (my_method) {
        case VTSS_APPL_PTP_PREFERRED_ADJ_SINGLE:
#if defined(VTSS_SW_OPTION_SYNCE_DPLL)
#if defined(VTSS_SW_OPTION_ZLS30387)
            type_2b = zl_3038x_get_type2b_enabled();
#endif
            if (type_2b) {
                *clock_option = CLOCK_OPTION_PTP_DPLL;
            } else if (fast_cap(MEBA_CAP_SYNCE_DPLL_MODE_SINGLE) && features != SYNCE_CLOCK_FEATURE_NONE) {
                *clock_option = CLOCK_OPTION_SYNCE_DPLL;
            } else {
                T_IG(_C,"Fallback to LTC adjustment");
            }
#else
            rc = VTSS_RC_ERROR;
            T_IG(_C,"Synce DPLL not supported");
#endif
            break;
        case VTSS_APPL_PTP_PREFERRED_ADJ_INDEPENDENT:
            if (fast_cap(MEBA_CAP_SYNCE_DPLL_MODE_DUAL) && fast_cap(MESA_CAP_SYNCE_SEPARATE_TIMING_DOMAINS)) {
                if (features == SYNCE_CLOCK_FEATURE_DUAL || features == SYNCE_CLOCK_FEATURE_DUAL_INDEPENDENT) {
                    *clock_option = CLOCK_OPTION_PTP_DPLL;
                } else {
                    T_IG(_C,"DPLL does not support independant. Fallback to LTC adjustment");
                }
            } else {
                T_IG(_C,"Board does not support independent. Fallback to LTC adjustment");
            }
            break;
        case  VTSS_APPL_PTP_PREFERRED_ADJ_COMMON:
            if (fast_cap(MEBA_CAP_SYNCE_DPLL_MODE_DUAL) && fast_cap(MESA_CAP_SYNCE_SEPARATE_TIMING_DOMAINS)) {
                *clock_option = CLOCK_OPTION_SYNCE_DPLL; // synce dpll option needed for switching servo from packet to hybrid mode.
            } else if (fast_cap(MEBA_CAP_SYNCE_DPLL_MODE_SINGLE)) { // Common mode is the same as single mode but with DPLL in generic mode.
                *clock_option = CLOCK_OPTION_SYNCE_DPLL;
                T_IG(_C,"Synce freq or phase phase adjustment");
            } else {
                T_IG(_C,"Fallback to LTC adjustment");
            }
            break;
        default:
            break;
    }
#if defined(VTSS_SW_OPTION_SYNCE_DPLL) && defined(VTSS_SW_OPTION_ZLS30387)
    ptp_clock_source_t source;
    source = (*clock_option == CLOCK_OPTION_SYNCE_DPLL) ? PTP_CLOCK_SOURCE_SYNCE : PTP_CLOCK_SOURCE_INDEP;
    if (!zl_3038x_get_type2b_enabled()) {
        if (VTSS_RC_OK != clock_ptp_timer_source_set(source)) {
            T_IG(_C,"clock_ptp_timer_source_set %d, only supported if a DPLL is present", source);
        }
    }
    rc = zl_3038x_servo_dpll_config(*clock_option, ((my_method == VTSS_APPL_PTP_PREFERRED_ADJ_INDEPENDENT) || (my_method == VTSS_APPL_PTP_PREFERRED_ADJ_LTC)));

#endif // defined(VTSS_SW_OPTION_SYNCE_DPLL) && defined(VTSS_SW_OPTION_ZLS30387)

    T_IG(_C,"adjust selection: preferred method %d, my_method %d, clock_option %d, profile %d, features %d", adj_method, my_method, *clock_option, profile, features);
    return rc;
}

mesa_rc vtss_local_clock_adj_method(uint32_t instance, vtss_appl_ptp_preferred_adj_t adj_method, vtss_appl_ptp_profile_t profile, uint32_t clk_domain, bool basic_servo)
{
    mesa_rc rc = VTSS_RC_OK;

    if (instClkData[instance].adj_method != adj_method) {
        vtssLocalClockReset(instance);
        T_IG(_C,"adj_method changed, i.e. clock data reset.");
    }
    instClkData[instance].clkDomain = clk_domain;
    instClkData[instance].adj_method = adj_method;
    T_IG(_C,"adj_method %d", adj_method);

    if (clk_domain == 0) {
        // Only clock domain 0 supports DPLL or LTC adjustment.
        rc = calc_clock_option(adj_method, profile, basic_servo, &instClkData[instance].clock_option);
        if (rc != VTSS_RC_OK) {
            T_IG(_C,"Preferred adj_method not supported");
        }
    } else if (clk_domain < hwClkDomains) {
        // Clock domains 1,2 support only internal LTC adjustment.
        instClkData[instance].clock_option = CLOCK_OPTION_INTERNAL_TIMER;
    } else {
        instClkData[instance].clock_option = CLOCK_OPTION_SOFTWARE;
    }
    return rc;
}

void ptp_local_clock_critd_init()
{
    critd_init(&clockmutex, "ptp.clockmutex", VTSS_MODULE_ID_PTP, CRITD_TYPE_MUTEX);
}

void vtss_local_clock_initialize()
{
    hwClkDomains = fast_cap(MESA_CAP_TS_DOMAIN_CNT);
    totalClkDomains = hwClkDomains + VTSS_PTP_SW_CLK_DOMAIN_CNT;

    CLOCK_LOCK();
    for (int inst= 0; inst < PTP_CLOCK_INSTANCES; inst++) {
        instClkData[inst].clock_option = CLOCK_OPTION_INTERNAL_TIMER;
        instClkData[inst].adj_method   = VTSS_APPL_PTP_PREFERRED_ADJ_LTC;
        instClkData[inst].clkDomain = 0;
    }

    for (uint32_t clk = 0; clk < VTSS_PTP_MAX_CLOCK_DOMAINS; clk++) {
        localClock[clk].t0 = {};
        localClock[clk].drift = 0;
        localClock[clk].ratio = 0;
        localClock[clk].ptp_offset = 0;
        localClock[clk].adj = 0;
        localClock[clk].set_time_count = 0;
        localClock[clk].ppsDomain = 0;
        localClock[clk].ppsProcDelay = 0;
        localClock[clk].ppsSyncCnt = 0;
    }
    CLOCK_UNLOCK();

}

// Update ltc time of pps domain to be in sync with software clock.
void vtss_soft_clock_sync_pps(uint32_t clkDomain)
{
    mesa_timestamp_t t1 = {}, t2;
    uint64_t tmp;
    int64_t diff;
    mesa_timeinterval_t nano = (2 * localClock[clkDomain].ppsProcDelay) << 16;

    T_IG(_C, "Synchronising software clock with 1pps domain.");
    vtss_tod_add_TimeInterval(&t1, &localClock[clkDomain].t0, &localClock[clkDomain].drift);
    t1.seconds += localClock[clkDomain].ptp_offset;

    vtss_tod_gettimeofday(localClock[clkDomain].ppsDomain, &t2, &tmp);
    vtss_tod_sub_TimeStamps(&diff, &t1, &t2);
    diff += (nano >> 16);

    if (labs(diff) > VTSS_ONE_MIA) {
        vtss_tod_add_TimeInterval(&t1, &t1, &nano);
        vtss_tod_settimeofday(localClock[clkDomain].ppsDomain, &t1);
    } else {
        mesa_timestamp_t t3 = {.nanoseconds = (uint32_t)labs(diff)};
        vtss_tod_settimeofday_delta(localClock[clkDomain].ppsDomain, &t3, diff < 0 ? TRUE : FALSE);
    }
}

bool vtss_domain_clock_time_get(mesa_timestamp_t *t, u32 time_domain, u64 *hw_time)
{
    bool ret = true;

    if (time_domain < hwClkDomains) {
        vtss_tod_gettimeofday(time_domain, t, hw_time);
    } else if (time_domain < totalClkDomains) {
        mesa_timeinterval_t deltat;
        char buf[50];
        vtss_tod_gettimeofday(SOFTWARE_CLK_DOMAIN, t, hw_time);
        T_DG(_C,"chip time %s", TimeStampToString(t,buf));
        CLOCK_LOCK();
        vtss_tod_sub_TimeInterval(&deltat, t, &localClock[time_domain].t0);
        deltat = deltat/(1<<16); /* normalize to ns */
        deltat = (deltat*localClock[time_domain].ratio);
        deltat = (deltat/VTSS_ONE_MIA)<<16;
        vtss_tod_add_TimeInterval(t, t, &deltat);
        vtss_tod_add_TimeInterval(t, t, &localClock[time_domain].drift);
        t->seconds += localClock[time_domain].ptp_offset;
        CLOCK_UNLOCK();
        T_NG(_C,"time_domain = %d,  deltat = %s",time_domain, vtss_tod_TimeInterval_To_String(&deltat,buf,0));
        T_NG(_C, "drift = %s ptp_offset = %d", vtss_tod_TimeInterval_To_String(&localClock[time_domain].drift,buf,0), localClock[time_domain].ptp_offset);
        T_NG(_C, "timestamp %s", TimeStampToString(t,buf));
    } else {
        ret = false;
    }
    return ret;
}

bool vtss_local_clock_time_get(mesa_timestamp_t *t, int instance, u64 *hw_time)
{
    uint32_t clkDomain = instClkData[instance].clkDomain;
    T_DG(_C,"instance %d, time_domain %d",instance, clkDomain);
    return vtss_domain_clock_time_get(t, clkDomain, hw_time);
}


bool vtss_local_clock_time_set(const mesa_timestamp_t *t, u32 time_domain)
{
    char buf [50];
    mesa_timestamp_t thw = {0,0,0};
    mesa_timestamp_t my_time;
    i32 ptp_offset;
    u64 tc;
    bool ret = true;

    T_IG(_C,"Set timeofday domain %d, time %s", time_domain, TimeStampToString(t,buf));
    if (time_domain < hwClkDomains) {
        vtss_tod_settimeofday(time_domain, t);
        ptp_time_setting_start();
        CLOCK_LOCK();
        localClock[time_domain].set_time_count++;
        CLOCK_UNLOCK();
    } else if (time_domain < totalClkDomains) {
        vtss_tod_gettimeofday(SOFTWARE_CLK_DOMAIN, &thw, &tc);
        ptp_offset = t->seconds - thw.seconds;
        CLOCK_LOCK();
        localClock[time_domain].t0 = thw;
        localClock[time_domain].drift = ((mesa_timeinterval_t)(t->nanoseconds << 16) - (mesa_timeinterval_t)(thw.nanoseconds << 16));
        vtss_tod_add_TimeInterval(&my_time, &thw, &localClock[time_domain].drift);
        my_time.seconds += ptp_offset;

        T_IG(_C,"drift: %s",vtss_tod_TimeInterval_To_String(&localClock[time_domain].drift,buf,0));
        T_IG(_C,"his time:t_sec = %d,  t_nsec = %d, ptp_offset = %d",t->seconds, t->nanoseconds, ptp_offset);
        T_IG(_C,"my time :t_sec = %d,  t_nsec = %d",my_time.seconds, my_time.nanoseconds);
        localClock[time_domain].ptp_offset = ptp_offset;
        CLOCK_UNLOCK();
        if (localClock[time_domain].ppsDomain) {
            mesa_timestamp_t t2;
            mesa_timeinterval_t nano = (2 * localClock[time_domain].ppsProcDelay) << 16;
            vtss_tod_add_TimeInterval(&t2, &my_time, &nano);
            vtss_tod_settimeofday(localClock[time_domain].ppsDomain, &t2);
            localClock[time_domain].ppsSyncCnt = 0;
        }
    } else {
        ret = false;
    }
    return ret;
}

#define TIMEINTERVAL_1SEC (1000000000LL<<16)

bool vtss_local_clock_time_set_delta(const mesa_timestamp_t *t, u32 time_domain, BOOL negative)
{
    char buf [50];
    bool ret = true;

    T_IG(_C,"Set delta timeofday domain %d, time %s negative %d",time_domain, TimeStampToString(t,buf), negative);
    if (time_domain < hwClkDomains) {
        vtss_tod_settimeofday_delta(time_domain, t, negative);
        ptp_time_setting_start();
        CLOCK_LOCK();
        localClock[time_domain].set_time_count++;
        CLOCK_UNLOCK();
    } else if (time_domain < totalClkDomains) {
        CLOCK_LOCK();
        if (negative) {
            localClock[time_domain].ptp_offset -= t->seconds;
            localClock[time_domain].drift -= ((mesa_timeinterval_t)t->nanoseconds)<<16;
            while (localClock[time_domain].drift < -TIMEINTERVAL_1SEC) {
                localClock[time_domain].drift += TIMEINTERVAL_1SEC;
                localClock[time_domain].ptp_offset--;
            }
        } else {
            localClock[time_domain].ptp_offset += t->seconds;
            localClock[time_domain].drift += ((mesa_timeinterval_t)t->nanoseconds)<<16;
            while (localClock[time_domain].drift > TIMEINTERVAL_1SEC) {
                localClock[time_domain].drift -= TIMEINTERVAL_1SEC;
                localClock[time_domain].ptp_offset++;
            }
        }
        if (localClock[time_domain].ppsDomain) {
            mesa_timestamp_t t2;
            mesa_timeinterval_t nano = (t->nanoseconds + localClock[time_domain].ppsProcDelay) << 16;
            vtss_tod_add_TimeInterval(&t2, t, &nano);
            vtss_tod_settimeofday_delta(localClock[time_domain].ppsDomain, &t2, negative);
            localClock[time_domain].ppsSyncCnt = 0;
        }

        T_IG(_C,"drift: %s",vtss_tod_TimeInterval_To_String(&localClock[time_domain].drift,buf,0));
        T_IG(_C,"delta time:t_sec = %d,  t_nsec = %d, negative %d",t->seconds, t->nanoseconds, negative);
        CLOCK_UNLOCK();
    } else {
        ret = false;
    }

    return ret;
}


bool vtss_domain_clock_convert_to_time(u64 cur_time, mesa_timestamp_t *t, u32 time_domain)
{
    char buf1 [30];
    char buf2 [30];
    bool ret = true;

    CLOCK_LOCK();
    if (time_domain < hwClkDomains) {
        vtss_tod_ts_to_time(time_domain,  cur_time, t);
    } else if (time_domain < totalClkDomains) {
        mesa_timeinterval_t deltat;
        *t = {};
        vtss_tod_ts_to_time(SOFTWARE_CLK_DOMAIN, cur_time, t);
        vtss_tod_sub_TimeInterval(&deltat, t, &localClock[time_domain].t0);
        deltat = deltat/(1<<16); /* normalize to ns */
        deltat = (deltat*localClock[time_domain].ratio);
        deltat = (deltat/VTSS_ONE_MIA)<<16;
        vtss_tod_add_TimeInterval(t, t, &deltat);
        vtss_tod_add_TimeInterval(t, t, &localClock[time_domain].drift);
        t->seconds += localClock[time_domain].ptp_offset;
        T_IG(_C,"time_domain = %d,  deltat = %s,drift = %s",time_domain, vtss_tod_TimeInterval_To_String(&deltat,buf1,0),
        vtss_tod_TimeInterval_To_String(&localClock[time_domain].drift,buf2,0));
    } else {
        ret = false;
    }
    T_DG(_C,"t_sec = %d,  t_nsec = %d, ptp_offset = %d",t->seconds, t->nanoseconds, localClock[time_domain].ptp_offset);
    CLOCK_UNLOCK();

    return ret;
}

bool vtss_local_clock_convert_to_time(u64 cur_time, mesa_timestamp_t *t, int instance)
{
    uint32_t clkDomain = instClkData[instance].clkDomain;

    T_DG(_C,"instance %d, time_domain %d",instance, clkDomain);
    return vtss_domain_clock_convert_to_time(cur_time, t, clkDomain);
}

// Clock domain 0, conversion
void vtss_local_clock_convert_to_hw_tc(u32 ns, u64 *cur_time)
{
    uint64_t ts_cnt;
    mesa_rc  rc;

    if ((rc = mesa_packet_ns_to_ts_cnt(NULL, ns, &ts_cnt)) != VTSS_RC_OK) {
        T_EG(_C, "Unable to convert ns (%u) to ts. Error = %s", ns, error_txt(rc));
    }

    *cur_time = ts_cnt;

    T_NG(_C, "ns = %u,  cur_time = " VPRI64u " ",ns, *cur_time);
}

i64 vtss_local_clock_ratio_get(u32 instance)
{
    uint32_t clkDomain = instClkData[instance].clkDomain;

    T_DG(_C,"domain %d, adj " VPRI64d ".", clkDomain, localClock[clkDomain].adj);
    return localClock[clkDomain].adj;
}

void vtss_local_clock_ratio_set(i64 adj, u32 instance)
{
    char buf1 [30];
    char buf2 [30];
    int current_option;
    u32 time_domain = instClkData[instance].clkDomain;
    T_IG(_C,"frequency adjustment. instance %d domain %d, adj " VPRI64d ".", instance, time_domain, adj);

    if (time_domain < hwClkDomains) {
        if (adj > ADJ_FREQ_MAX_LL*ADJ_SCALE) {
            adj = ADJ_FREQ_MAX_LL*ADJ_SCALE;
        } else if (adj < -ADJ_FREQ_MAX_LL*ADJ_SCALE) {
            adj = -ADJ_FREQ_MAX_LL*ADJ_SCALE;
        }
        //TODO : verify slew_rate purpose.
        T_DG(_C,"before: adj " VPRI64d ", actual_adj " VPRI64d, adj, localClock[time_domain].adj);
        if (adj  > localClock[time_domain].adj + CLOCK_ADJ_SLEW_RATE) adj = localClock[time_domain].adj + CLOCK_ADJ_SLEW_RATE;
        if (adj  < localClock[time_domain].adj - CLOCK_ADJ_SLEW_RATE) adj = localClock[time_domain].adj - CLOCK_ADJ_SLEW_RATE;
        T_DG(_C,"after: adj " VPRI64d, adj);
    }
    CLOCK_LOCK();
    current_option = vtss_ptp_adjustment_option(instance);
    // if (adj != actual_adj[time_domain]) {  // FIXME: Have disabled this check as it is not reliable. Always do adjustment when asked to do so. Do not rely on actual value being known.
    if (1) {                                  // NOTE: This line has been inserted as a replacement for the line above.
        if (time_domain == 0) {
            if (current_option == CLOCK_OPTION_INTERNAL_TIMER) {
                vtss_tod_set_adjtimer(time_domain, adj);
            } else if (current_option == CLOCK_OPTION_PTP_DPLL) {
                // Do frequency adjustment in PTP reference clock.
#if defined(VTSS_SW_OPTION_SYNCE_DPLL)
                clock_output_adjtimer_set(adj);
                if (!ptp_mesa_cap_ts_separate_domain()) {
                    vtss_tod_set_adjtimer(time_domain, adj);
                }
#else
                T_WG(_C,"No support for Sync_XO in current HW");
#endif
            } else if (current_option == CLOCK_OPTION_SYNCE_DPLL) {
                bool type_2b = false;
#if defined(VTSS_SW_OPTION_ZLS30387)
                type_2b = zl_3038x_get_type2b_enabled();
#endif
                if (type_2b) {
                /* Synce dpll should not be used for adjusting PTP clock. So,fall back to internal timer.
                   But, Clock option is set to Synce dpll to switch to hybrid mode. */
                    vtss_tod_set_adjtimer(time_domain, adj);
                } else {
#if defined(VTSS_SW_OPTION_SYNCE_DPLL)
                    // Do frequency adjustment in Controller (in DCO mode).
                    if (clock_adjtimer_set(adj)) {
                        T_DG(_C,"Synce Clock adjustment");
                    }
#endif
                }
            } else {
                T_WG(_C,"undefined clock adj method %d", current_option);
            }

        } else if (time_domain < hwClkDomains) {
            vtss_tod_set_adjtimer(time_domain, adj);
        } else {
            mesa_timeinterval_t deltat;
            mesa_timestamp_t t = {0,0,0};
            u64 tc;
            vtss_tod_gettimeofday(SOFTWARE_CLK_DOMAIN, &t, &tc);
            vtss_tod_sub_TimeInterval(&deltat, &t, &localClock[time_domain].t0);
            T_NG(_C,"deltat = %s, ratio " VPRI64d,vtss_tod_TimeInterval_To_String(&deltat,buf1,0), localClock[time_domain].ratio);
            deltat = deltat/(1<<16); /* normalize to ns */
            deltat = (deltat*localClock[time_domain].ratio);
            T_NG(_C,"deltat = " VPRI64d,deltat);
            deltat = (deltat/VTSS_ONE_MIA)<<16;
            localClock[time_domain].drift += deltat;

            if (localClock[time_domain].drift > ((mesa_timeinterval_t)VTSS_ONE_MIA)<<16) {
                ++localClock[time_domain].ptp_offset;
                localClock[time_domain].drift -= ((mesa_timeinterval_t)VTSS_ONE_MIA)<<16;
                T_IG(_C,"drift adjusted = %s ptp_offset = %d",vtss_tod_TimeInterval_To_String(&deltat,buf1,0), localClock[time_domain].ptp_offset);
            } else if (deltat < -(((mesa_timeinterval_t)VTSS_ONE_MIA)<<16)) {
                --localClock[time_domain].ptp_offset;
                localClock[time_domain].drift += ((mesa_timeinterval_t)VTSS_ONE_MIA)<<16;
                T_IG(_C,"drift adjusted = %s, ptp_offset = %d",vtss_tod_TimeInterval_To_String(&deltat,buf1,0), localClock[time_domain].ptp_offset);
            }
            localClock[time_domain].t0 = t;
            localClock[time_domain].ratio = adj/ADJ_SCALE;
            T_DG(_C,"adj = " VPRI64d ",  deltat = %s, drift = %s",adj,
                 vtss_tod_TimeInterval_To_String(&deltat,buf1,0), vtss_tod_TimeInterval_To_String(&localClock[time_domain].drift,buf2,0));
            if (localClock[time_domain].ppsDomain) {
                vtss_tod_set_adjtimer(localClock[time_domain].ppsDomain, adj);
                localClock[time_domain].ppsSyncCnt++;
                if (localClock[time_domain].ppsSyncCnt >= SOFT_CLOCK_PPS_SYNC_CNT) {
                    vtss_soft_clock_sync_pps(time_domain);
                    localClock[time_domain].ppsSyncCnt = 0;
                }
            }
        }
    }
    T_IG(_C,"frequency adjustment: adj = " VPRI64d, adj);
    localClock[time_domain].adj = adj;
    CLOCK_UNLOCK();
}

void vtss_local_clock_ratio_clear(u32 instance)
{
    char buf1 [30];
    char buf2 [30];
    i64 adj = 0;
    int current_option;
    uint32_t time_domain = instClkData[instance].clkDomain;
    T_IG(_C,"frequency adjustment clear. domain %d", time_domain);

    CLOCK_LOCK();
    current_option = vtss_ptp_adjustment_option(instance);
    if (time_domain == 0) {
        if (adj != localClock[time_domain].adj) {
            if (current_option == CLOCK_OPTION_INTERNAL_TIMER) {
                vtss_tod_set_adjtimer(time_domain, adj);
            } else if (current_option == CLOCK_OPTION_PTP_DPLL) {
                // Do frequency adjustment in PTP reference clock.
#if defined(VTSS_SW_OPTION_SYNCE_DPLL)
                clock_output_adjtimer_set(adj);

            } else if (current_option == CLOCK_OPTION_SYNCE_DPLL) {
                if (clock_adjtimer_set(adj)) {
                    T_DG(_C,"SyncE Clock adjustment");
                }
            } else if (current_option == CLOCK_OPTION_PTP_DPLL) {
                // Do frequency adjustment in Synthesizer (in DCO mode).
                PTP_RC(clock_output_adjtimer_set(adj));
#endif
            } else {
                T_WG(_C,"undefined clock adj method %d", current_option);
            }

            T_IG(_C,"frequency adjustment: adj = " VPRI64d, adj);
        }

    } else if (time_domain < hwClkDomains) {
        vtss_tod_set_adjtimer(time_domain, adj);
    } else {
        mesa_timeinterval_t deltat;
        mesa_timestamp_t t = {0,0,0};
        u64 tc;
        vtss_tod_gettimeofday(SOFTWARE_CLK_DOMAIN, &t, &tc);
        vtss_tod_sub_TimeInterval(&deltat, &t, &localClock[time_domain].t0);
        deltat = deltat/(1<<16); /* normalize to ns */
        deltat = (deltat*localClock[time_domain].ratio);
        deltat = (deltat/VTSS_ONE_MIA)<<16;
        localClock[time_domain].drift += deltat;

        if (localClock[time_domain].drift > ((mesa_timeinterval_t)VTSS_ONE_MIA)<<16) {
            ++localClock[time_domain].ptp_offset;
            localClock[time_domain].drift -= ((mesa_timeinterval_t)VTSS_ONE_MIA)<<16;
            T_IG(_C,"drift adjusted = %s ptp_offset = %d",vtss_tod_TimeInterval_To_String(&deltat,buf1,0), localClock[time_domain].ptp_offset);
        } else if (deltat < -(((mesa_timeinterval_t)VTSS_ONE_MIA)<<16)) {
            --localClock[time_domain].ptp_offset;
            localClock[time_domain].drift += ((mesa_timeinterval_t)VTSS_ONE_MIA)<<16;
            T_IG(_C,"drift adjusted = %s, ptp_offset = %d",vtss_tod_TimeInterval_To_String(&deltat,buf1,0), localClock[time_domain].ptp_offset);
        }
        localClock[time_domain].t0 = t;
        localClock[time_domain].ratio = 0;
        T_DG(_C,"adj = " VPRI64d ",  deltat = %s, drift = %s",adj,
             vtss_tod_TimeInterval_To_String(&deltat,buf1,0), vtss_tod_TimeInterval_To_String(&localClock[time_domain].drift,buf2,0));
        if (localClock[time_domain].ppsDomain) {
            vtss_tod_set_adjtimer(localClock[time_domain].ppsDomain, adj);
        }
    }
    localClock[time_domain].adj = adj;
    CLOCK_UNLOCK();
}

bool vtss_local_clock_fine_adj_offset(i64 offset, u32 domain)
{
    bool ret = true;

#if defined(VTSS_SW_OPTION_SYNCE_DPLL)
    if (domain == 0 && features == SYNCE_CLOCK_FEATURE_DUAL) {
        if (offset <= (32767<<16) && offset >= (-32768<<16)) {
            PTP_RC(clock_adj_phase_set(offset));
        } else {
            T_WG(_C,"clock offset adj %d ns: too big", (u32)(offset>>16));
        }
    } else if (domain < totalClkDomains) {
#else
    if (domain < totalClkDomains) {
#endif
        i32 ns_phase = (offset>>16);
        /* limit the max phase adjustment pr adjustment */
        if (domain < hwClkDomains) {
            if (ns_phase > MESA_HW_TIME_MAX_FINE_ADJ) ns_phase = MESA_HW_TIME_MAX_FINE_ADJ;
            if (ns_phase < -MESA_HW_TIME_MAX_FINE_ADJ) ns_phase = -MESA_HW_TIME_MAX_FINE_ADJ;
        }
        ret = vtss_local_clock_adj_offset(ns_phase, domain);
    } else {
        ret = false;
    }

    return ret;
}

// Adjusts TOD by offset in negative direction.
bool vtss_local_clock_adj_offset(i32 offset, u32 time_domain)
{
    bool ret = true;
    mesa_timeinterval_t oneSecInterval = ((mesa_timeinterval_t)VTSS_ONE_MIA)<<16;

    CLOCK_LOCK();
    T_IG(_C,"offset adjustment. domain %d, offset %d.", time_domain, offset);
    if (time_domain < hwClkDomains) {
        if (offset > MESA_HW_TIME_MAX_FINE_ADJ || offset < -MESA_HW_TIME_MAX_FINE_ADJ) {
            mesa_timestamp_t ts = {};
            BOOL             neg;

            if (offset < 0) {
                ts.nanoseconds = -offset;
                neg = FALSE;
            } else {
                ts.nanoseconds = offset;
                neg = TRUE;
            }
            vtss_tod_settimeofday_delta(time_domain, &ts, neg);
            ptp_time_setting_start();
        } else {
            PTP_RC(mesa_ts_domain_timeofday_offset_set(NULL, time_domain, offset));
            T_IG(_C,"clock offset adj %d", offset);
            // After every clock adjustment in switch, phy's LTC also must be adjusted.
            if (mepa_phy_ts_cap() && (time_domain == 0)) {
                int nano = offset;
                BOOL neg = TRUE;

                if (offset < 0) {
                    nano = -offset;
                    neg = FALSE;
                }
                tod_mod_man_time_fine_adj(nano, neg);
            }
        }
        localClock[time_domain].set_time_count++;
    } else if (time_domain < totalClkDomains) {
        T_IG(_C,"offset adjustment: %d ns for time_domain %d", offset, time_domain);
        localClock[time_domain].drift -= (i64)offset<<16;

        if (localClock[time_domain].drift > oneSecInterval) {
            ++localClock[time_domain].ptp_offset;
            localClock[time_domain].drift -= oneSecInterval;
            T_IG(_C,"drift adjusted = %d ptp_offset = %d",offset, localClock[time_domain].ptp_offset);
        } else if (localClock[time_domain].drift < 0) {
            --localClock[time_domain].ptp_offset;
            localClock[time_domain].drift += oneSecInterval;
            T_IG(_C,"drift adjusted = %d, ptp_offset = %d",offset, localClock[time_domain].ptp_offset);
        }
        if (localClock[time_domain].ppsDomain) {
            PTP_RC(mesa_ts_domain_timeofday_offset_set(NULL, localClock[time_domain].ppsDomain, offset));
        }
    } else {
        ret = false;
    }
    CLOCK_UNLOCK();

    return ret;
}

int vtss_ptp_adjustment_method(int instance)
{
    int option;
    CLOCK_LOCK();
    option = vtss_ptp_adjustment_option(instance);
    CLOCK_UNLOCK();
    return option;
}

u32 vtss_local_clock_set_time_count_get(int instance)
{
    u32 count;
    CLOCK_LOCK();
    count =  localClock[instClkData[instance].clkDomain].set_time_count;
    CLOCK_UNLOCK();
    return count;
}

bool vtss_local_clock_set_time_count_incr(u32 domain)
{
    if (domain >= totalClkDomains) {
        return false;
    }

    CLOCK_LOCK();
    localClock[domain].set_time_count++;
    CLOCK_UNLOCK();

    return true;
}

u32 vtss_domain_clock_set_time_count_get(u32 domain)
{
    u32 count = 0xFFFFFFFF;

    if (domain >= totalClkDomains) {
        return count;
    }
    CLOCK_LOCK();
    count =  localClock[domain].set_time_count;
    CLOCK_UNLOCK();
    return count;
}

//Get software clock data
bool vtss_local_clock_soft_data_get(uint32_t inst, localClockData_t *data)
{
    uint32_t clkDomain = 0;

    if (inst >= PTP_CLOCK_INSTANCES) {
        return false;
    }
    clkDomain = instClkData[inst].clkDomain;

    CLOCK_LOCK();
    data->t0             = localClock[clkDomain].t0;
    data->drift          = localClock[clkDomain].drift;
    data->ratio          = localClock[clkDomain].ratio;
    data->ptp_offset     = localClock[clkDomain].ptp_offset;
    data->adj            = localClock[clkDomain].adj;
    data->set_time_count = localClock[clkDomain].set_time_count;
    data->ppsDomain      = localClock[clkDomain].ppsDomain;
    data->ppsProcDelay   = localClock[clkDomain].ppsProcDelay;
    CLOCK_UNLOCK();

    return true;
}

bool vtssLocalClockPpsConfSet(uint32_t instance, uint32_t ppsDomain, bool set)
{
    uint32_t clkDomain;
    mesa_timestamp_t t1, t2;
    mesa_timeinterval_t nano;
    uint64_t hwTime;

    if (instance >= PTP_CLOCK_INSTANCES || ppsDomain >= hwClkDomains) {
        return false;
    }
    clkDomain = instClkData[instance].clkDomain;
    if (!set) {
        CLOCK_LOCK();
        localClock[clkDomain].ppsDomain = 0;
        CLOCK_UNLOCK();
    } else {

        vtss_tod_gettimeofday(ppsDomain, &t1, &hwTime);
        vtss_tod_gettimeofday(ppsDomain, &t2, &hwTime);
        CLOCK_LOCK();
        localClock[clkDomain].ppsDomain = ppsDomain;
        //ppsProcDelay is the approximate one way time needed from application to chip.
        vtss_tod_sub_TimeStamps(&localClock[clkDomain].ppsProcDelay, &t2, &t1);
        localClock[clkDomain].ppsProcDelay = localClock[clkDomain].ppsProcDelay/2;
        CLOCK_UNLOCK();
        //Get current time from software and set it in chip 1pps Domain.
        vtss_local_clock_time_get(&t1, instance, &hwTime);
        nano = (localClock[clkDomain].ppsProcDelay * 2) << 16;
        vtss_tod_add_TimeInterval(&t2, &t1, &nano);
        vtss_tod_settimeofday(ppsDomain, &t2);
    }

    return true;
}

void vtssLocalClockReset(uint32_t inst)
{
    uint32_t clk;

    if (inst >= PTP_CLOCK_INSTANCES) {
        return;
    }

    clk = instClkData[inst].clkDomain;

    vtss_local_clock_ratio_clear(inst);

    T_IG(_C, "Resetting Clock data structures");
    CLOCK_LOCK();
    instClkData[inst].clock_option = CLOCK_OPTION_INTERNAL_TIMER;
    instClkData[inst].adj_method   = VTSS_APPL_PTP_PREFERRED_ADJ_LTC;
    instClkData[inst].clkDomain = 0;

    localClock[clk].t0 = {};
    localClock[clk].drift = 0;
    localClock[clk].ratio = 0;
    localClock[clk].ptp_offset = 0;
    localClock[clk].adj = 0;
    localClock[clk].set_time_count = 0;
    localClock[clk].ppsDomain = 0;
    localClock[clk].ppsProcDelay = 0;
    localClock[clk].ppsSyncCnt = 0;
    CLOCK_UNLOCK();
}

void ptp_debug_hybrid_mode_set(bool enable)
{
    (void)clock_ptp_timer_source_set(enable ? PTP_CLOCK_SOURCE_SYNCE : PTP_CLOCK_SOURCE_INDEP);
    instClkData[0].clock_option = enable ? CLOCK_OPTION_INTERNAL_TIMER : PTP_CLOCK_SOURCE_SYNCE;
}
