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
/* vtss_tod_api.c */

#include "vtss_tod_api.h"
#include "tod.h"
#include "main.h"
#include "sysutil_api.h"

#include "microchip/ethernet/switch/api.h"
#include "vtss/basics/i128.hxx"

#include "vtss_tod_mod_man.h"
#include "tod_api.h"

static bool tod_cap_sub_nano_sec = false;


void tod_capability_sub_nano_set(bool set)
{
    tod_cap_sub_nano_sec = set;
}
// -------------------------------------------------------------------------
// vtss_tod_gettimeofday()
// Get the current time in a Timestamp
// Jaguar : get time from API layer (SW) with 4 ns resolution (only domain 0).
// Luton26: get time from API layer (HW) with 4 ns resolution (only domain 0).
// Serval1: get time from API layer (HW) with 4 ns resolution (only domain 0).
// Jaguar2: get time from API layer (HW) with 4 ns resolution (domain 0..2).
void vtss_tod_gettimeofday(u32 domain, mesa_timestamp_t *ts, u64 *tc)
{
    TOD_RC(mesa_ts_domain_timeofday_get(NULL, domain, ts, tc));
}

// -------------------------------------------------------------------------
// vtss_tod_settimeofday()
// Set the current time from a Timestamp
// Jaguar : set time in API layer (SW).
// Luton26: set time in API layer (HW).
void vtss_tod_settimeofday(const u32 domain, const mesa_timestamp_t *ts)
{
    // After every LTC adjustment in switch, phy's LTC also must be adjusted.
    if (mepa_phy_ts_cap() && domain == 0) {
        tod_mod_man_time_set(ts);
    } else {
        TOD_RC(mesa_ts_domain_timeofday_set(NULL, domain, ts));
    }
}

// -------------------------------------------------------------------------
// vtss_tod_settimeofday_delta()
// Set delta from the current time 
// Jaguar         : set time in API layer (SW).
// Luton26, Serval: set time in API layer (HW).
void vtss_tod_settimeofday_delta(const u32 domain, const mesa_timestamp_t *ts, BOOL negative)
{
    // After every LTC adjustment in switch, phy's LTC also must be adjusted. Otherwise tod_mod_man will detect "outofsync" and PTP is temporarily stopped.
    if (mepa_phy_ts_cap() && domain == 0) {
        tod_mod_man_time_step(ts, negative);
    } else {
        TOD_RC(mesa_ts_domain_timeofday_set_delta(NULL, domain, ts, negative));
    }
}

// -------------------------------------------------------------------------
// vtss_tod_get_ts_cnt()
// Get the current time as a time counter
// Jaguar : get time from API layer (SW) with 4 ns resolution.
// Luton26: get time from API layer (HW) with 4 ns resolution.
u64 vtss_tod_get_ts_cnt(void)
{
    mesa_timestamp_t ts;
    uint64_t tc;

    TOD_RC(mesa_ts_timeofday_get(NULL, &ts, &tc));
    return tc;
}

// -------------------------------------------------------------------------
// vtss_tod_ts_to_time()
// Convert a hw time counter to a timestamp, assuming that the hw time
// counter has not wrapped more than once.
// Time counters are converted to nano second counters by ignoring fractional nano second part.
void vtss_tod_ts_to_time(u32 domain, u64 hw_time, mesa_timestamp_t *ts)
{
    u64 tc;
    u32 tc_ns, diff, hw_time_ns = hw_time >> 16;
    if (tod_cap_sub_nano_sec) {
        if (domain == 0 && mepa_phy_ts_cap() && tod_mod_man_time_settling()) {
            tod_mod_man_settling_time_ts_proc(hw_time, ts);
            return;
        } else {
            /* fireAnt API returns actual Tod. So, Tod from API is always newer than timestamp.*/
            TOD_RC(mesa_ts_domain_timeofday_get(NULL, domain, ts, &tc));
            T_DG(_C,"then: ts_sec = %u,  ts_nsec = 0x%x, hw_time_ns = 0x%x tc=0x%x",ts->seconds, ts->nanoseconds, hw_time_ns, tc);
            if ((ts->nanoseconds) < hw_time_ns) {
                ts->seconds--;
            }
            ts->nanoseconds = hw_time_ns;
            ts->nanosecondsfrac = hw_time & 0xFFFF;

            return;
        }
    } else {
        if ((fast_cap(MESA_CAP_TS_DOMAIN_CNT)) > 1) {
            TOD_RC(mesa_ts_domain_timeofday_get(NULL, domain, ts, &tc));
        } else {
            TOD_RC(mesa_ts_timeofday_get(NULL, ts, &tc));
        }
        tc_ns = (u32)(tc >> 16);
        hw_time_ns = (u32)(hw_time >> 16);
        /* add time counter difference */
        T_DG(_C,"now: ts_sec = %d,  ts_nsec = %d, tc_ns = %u",ts->seconds, ts->nanoseconds, tc_ns);
        diff = tc_ns-hw_time_ns;
        if (tc_ns < hw_time_ns) { /* time counter has wrapped */
            diff += fast_cap(MESA_CAP_TS_HW_TIME_WRAP_LIMIT);
            T_IG(_C,"counter wrapped: tc_ns = %u,  hw_time_ns = %u, diff = %u",tc_ns, hw_time_ns, diff);
        }
        /* clock rate offset adjustment */
        //if (adjust_divisor) {
        //    T_IG(_C,"diff before ns: %u", diff);
        //    diff += (i32)diff/adjust_divisor;
        //    T_IG(_C,"diff after ns: %u", diff);
        //}
        
        while (diff > fast_cap(MESA_CAP_TS_HW_TIME_CNT_PR_SEC)) {
            --ts->seconds;
            diff -= fast_cap(MESA_CAP_TS_HW_TIME_CNT_PR_SEC);
            T_IG(_C,"big diff: tc_ns = %u, hw_time_ns = %u",tc_ns, hw_time_ns);
        }
        
        diff = diff * fast_cap(MESA_CAP_TS_HW_TIME_NSEC_PR_CNT);
        if (diff > ts->nanoseconds) {
            --ts->seconds;
            ts->nanoseconds += 1000000000L;
        }
        ts->nanoseconds -= diff;
        ts->nanosecondsfrac = 0; // sub nano second support not available on other platforms.
            
        T_DG(_C,"then: ts_sec = %d,  ts_nsec = %d, hw_time_ns = %u",ts->seconds, ts->nanoseconds, hw_time_ns);
        
        return;
    }
}

static CapArray<i32, MESA_CAP_TS_IO_CNT> old_switch_adj;

#define MIN_ADJ_RATE_DELTA 5
// -------------------------------------------------------------------------
// vtss_tod_set_adjtimer()
// Adjust the time rate in the HW
// adj = clockrate adjustment in scaled PPB.
void vtss_tod_set_adjtimer(u32 domain, i64 adj)
{
    if (labs(adj) < fast_cap(MESA_CAP_TS_HW_TIME_MIN_ADJ_RATE)) adj = 0;
    i32 switch_adj = (adj*10LL + (1<<15))>>16; /* add (1<<15) to make the correct rounding */
    if (domain < fast_cap(MESA_CAP_TS_IO_CNT)) {
        if (abs(switch_adj - old_switch_adj[domain]) >= MIN_ADJ_RATE_DELTA) {
            T_IG(_C,"frequency adjustment:domain %d, adj = " VPRI64d " switch_adj = %d", domain, adj, switch_adj);
            TOD_RC(mesa_ts_domain_adjtimer_set(NULL, domain, switch_adj));
            if (mepa_phy_ts_cap()) {
                if (domain == 0) {
                    TOD_RC(tod_mod_man_slave_freq_adjust(&adj));
                }
            }
            old_switch_adj[domain] = switch_adj;
        } else {
            T_IG(_C,"change < 0,5 ppb not applied old_switch_adj %d, switch_adj = %d", old_switch_adj[domain], switch_adj);
        }
    } else {
        T_WG(_C,"invalid domain = %d", domain);
    }
}

// -------------------------------------------------------------------------
// vtss_tod_ts_cnt_sub()
// Calculate time difference (in time counter units) r = x-y
void vtss_tod_ts_cnt_sub(u64 *r, u64 x, u64 y)
{
    *r = x-y;
    if (x < y) {
        // Convert to ns and add compensation
        x = (x >> 16) + fast_cap(MESA_CAP_TS_HW_TIME_CNT_PR_SEC);
        y = y >> 16;
        // Subtract and convert back
        *r = (x-y) << 16;
        } /* time counter has wrapped */
}

// -------------------------------------------------------------------------
// vtss_tod_ts_cnt_add()
// Calculate time sum (in time counter units) r = x+y
void vtss_tod_ts_cnt_add(u64 *r, u64 x, u64 y)
{
    *r = x+y;
    if (*r > fast_cap(MESA_CAP_TS_HW_TIME_WRAP_LIMIT)) {*r -= fast_cap(MESA_CAP_TS_HW_TIME_WRAP_LIMIT);} /* time counter has wrapped */
}


// -------------------------------------------------------------------------
// vtss_tod_timeinterval_to_ts_cnt()
// Convert a timeInterval to time difference (in time counter units)
// It is assumed that the timeinterval is > 0 and < counter wrap around time
void vtss_tod_timeinterval_to_ts_cnt(u64 *r, mesa_timeinterval_t x)
{
    if (x < -VTSS_SEC_NS_INTERVAL(1,0) || x >= VTSS_SEC_NS_INTERVAL(1,0)) {
        T_WG(_C,"Time interval overflow ");
        *r = 0;
        return;
    }
    if (x >= 0) {
        *r = VTSS_INTERVAL_NS(x) / fast_cap(MESA_CAP_TS_HW_TIME_NSEC_PR_CNT);
        //if (adjust_divisor) {
        //    *r -= *r/adjust_divisor;
        //}
    } else {
        *r = (u64)(fast_cap(MESA_CAP_TS_HW_TIME_WRAP_LIMIT) - VTSS_INTERVAL_NS(-x) / fast_cap(MESA_CAP_TS_HW_TIME_NSEC_PR_CNT)) << 16;
    }
    /* Fractional nano second. */
    if (x & 0xFFFF) {
        *r = *r | (x & 0xFFFF);
    }

    T_NG(_C,"x " VPRI64d ", r " VPRI64u, x>>16, *r);
}

// -------------------------------------------------------------------------
// vtss_tod_ts_cnt_to_timeinterval()
// Convert a time difference (in time counter units) to timeInterval
// it is assumed that time difference is < 1 sec.
void vtss_tod_ts_cnt_to_timeinterval(mesa_timeinterval_t *r, u64 x)
{
    x = (x >> 16) * fast_cap(MESA_CAP_TS_HW_TIME_NSEC_PR_CNT);
    //if (adjust_divisor) {
    //    x += x/adjust_divisor;
    //}
    *r = VTSS_SEC_NS_INTERVAL(0,x);
    /* Fractional nano second in Sparx5*/
    if (x & 0xFFFF) {
        *r = *r | (x & 0xFFFF);
    }
}

// -------------------------------------------------------------------------
// vtss_tod_ts_cnt_to_ns(ts)
// Convert time counter units to a nanosec value
u64 vtss_tod_ts_cnt_to_ns(u64 ts)
{
    return ts * fast_cap(MESA_CAP_TS_HW_TIME_NSEC_PR_CNT);
}


char *vtss_tod_ns2str (u32 nanoseconds, char *str, char delim)
{
    /* nsec time stamp. Format: mmm,uuu,nnn */
    int m, u, n;
    char d[2];
    d[0] = delim;
    d[1] = 0;
    m = nanoseconds / 1000000;
    u = (nanoseconds / 1000) % 1000;
    n = nanoseconds % 1000;
    sprintf(str, "%03d%s%03d%s%03d", m, d, u, d, n);
    return str;
}


void vtss_tod_scaledNS_to_timestamp(const mesa_scaled_ns_t *t, mesa_timestamp_t *ts, BOOL *negative)
{
    vtss::I128 t2;

    t2 = vtss::I128(t->scaled_ns_high, t->scaled_ns_low);
    if (t2 < 0) {
        t2 = -t2;
        *negative = TRUE;
    } else {
        *negative = FALSE;
    }
    ts->nanosecondsfrac = (uint16_t)(t->scaled_ns_low & 0xFFFF);
    t2 = t2 / 0x10000; // convert to nanoseconds.

    vtss::I128 sec = t2 / VTSS_ONE_MIA;
    vtss::I128 nsec = t2 % VTSS_ONE_MIA;
    ts->nanoseconds = nsec.to_int32();
    ts->seconds = sec.to_int32();
    ts->sec_msb = (uint16_t)((sec.to_int64() >> 32) & 0xFFFF);
}

char *vtss_tod_ScaledNs_To_String (const mesa_scaled_ns_t *t, char* str, char delim)
{
    vtss::I128 t2;
    char str1 [14];
    char str2 [33];
    t2 = vtss::I128(t->scaled_ns_high, t->scaled_ns_low);
    T_DG(_C,"high %d, low %" PRIi64, t->scaled_ns_high, t->scaled_ns_low);
    t2 = t2 / 0x10000; // convert to whole nanosec
    if (t2 < 0) {
        t2 = -t2;
        str[0] = '-';
    } else {
        str[0] = ' ';
    }
    vtss::I128 sec = t2 / VTSS_ONE_MIA;
    vtss::I128 nsec = t2 % VTSS_ONE_MIA;
    int i = sizeof(str2)-1;
    str2[i--] = 0; // 0'terminated string
    int dig = 0;
    do {
        vtss::I128 digit = sec % 10;
        str2[i--] = '0' + digit.to_char();
        if ((++dig % 3) == 0 && sec != 0) str2[i--] = delim;
        sec = sec/10;
    } while (sec > 0 && i > 0);
    if (sec != 0) {
        T_WG(_C,"buffer overflow in scaled_ns printout");
    }
    sprintf(str+1, "%s.%s", str2+i+1, vtss_tod_ns2str (nsec.to_int32(), str1, delim));
    return str;
}

char *vtss_tod_TimeInterval_To_String (const mesa_timeinterval_t *t, char* str, char delim)
{
    mesa_timeinterval_t t1;
    char str1 [14];
    int ret = 0;
    if (*t < 0) {
        t1 = -*t;
        str[0] = '-';
    } else {
        t1 = *t;
        str[0] = ' ';
    }
    if (t1 == VTSS_MAX_TIMEINTERVAL) {
        strcpy(str+1, "NAN");
    } else {
        ret = sprintf(str+1, "%d.%s", VTSS_INTERVAL_SEC(t1), vtss_tod_ns2str (VTSS_INTERVAL_NS(t1), str1, delim));
        if (tod_cap_sub_nano_sec) {
            char str2[2] = {delim, '\0'};
            sprintf(str+ret+1, "%s%03d", str2, VTSS_INTERVAL_PICO(t1));
        }
    }
    return str;
}

//48-bits used entirely for unsigned nano seconds part can hold up to 76 hours of time range.
//If the timestamps differ by more than 3.5 days, then timestamp difference must be found by other means.
void vtss_tod_sub_TimeInterval(mesa_timeinterval_t *r, const mesa_timestamp_t *x, const mesa_timestamp_t *y)
{
    *r = (mesa_timeinterval_t)x->seconds + (((mesa_timeinterval_t)x->sec_msb)<<32) - 
                                 (mesa_timeinterval_t)y->seconds - (((mesa_timeinterval_t)y->sec_msb)<<32);
    *r = *r*1000000000LL;
    *r += ((mesa_timeinterval_t)x->nanoseconds - (mesa_timeinterval_t)y->nanoseconds);
    *r *= (1<<16);
    *r += x->nanosecondsfrac - y->nanosecondsfrac;
}

/* This function does a "simple" subtraction of time stamps without PTP adjustment - without rotating left 16 */
// Maximum timestamp difference can be 292 years.
void vtss_tod_sub_TimeStamps(i64 *r, const mesa_timestamp_t *x, const mesa_timestamp_t *y)
{
    *r = (i64)x->seconds + (((i64)x->sec_msb)<<32) - (i64)y->seconds - (((i64)y->sec_msb)<<32);
    *r = *r*1000000000LL;
    *r += ((i64)x->nanoseconds - (i64)y->nanoseconds);
}

// This function uses vtss_tod_sub_TimeStamps which uses 64-bit integer for holding nano seconds.
// 64-bit signed nanoseconds number can hold time range up to 292 years i.e. timestamp difference can
// be up to 292 years.
void vtss_tod_sub_scaled_time(mesa_scaled_ns_t *ret, const mesa_timestamp_t *x, const mesa_timestamp_t *y)
{
    uint32_t nanoCarry = 0;
    int64_t nanoDiff = 0;

    if (x->nanosecondsfrac < y->nanosecondsfrac) {
        ret->scaled_ns_low = TOD_FRAC_PER_NANO_SECOND;
        nanoCarry = 1;
    } else {
        ret->scaled_ns_low = 0;
    }
    ret->scaled_ns_low += x->nanosecondsfrac - y->nanosecondsfrac;

    vtss_tod_sub_TimeStamps(&nanoDiff, x, y);
    if (nanoCarry) {
        --nanoDiff;
    }
    ret->scaled_ns_low += (nanoDiff << 16);
    if (nanoDiff >= 0) {
        ret->scaled_ns_high = nanoDiff >> 48;
    } else {
        ret->scaled_ns_high = (0xFFFF0000) | (nanoDiff >> 48);
    }
    T_DG(_C, "nanosec offset " VPRI64d " \n", nanoDiff);
}

static void timestamp_fix(mesa_timestamp_t *r, const mesa_timestamp_t *x)
{
    if ((int16_t)r->nanosecondsfrac < 0) {
        r->nanoseconds -= 1;
        r->nanosecondsfrac += TOD_FRAC_PER_NANO_SECOND;
    } else if (r->nanosecondsfrac >= TOD_PICO_PER_NANO) {
        r->nanoseconds += 1;
        r->nanosecondsfrac -= TOD_FRAC_PER_NANO_SECOND;
    }
    if ((i32)r->nanoseconds < 0) {
        r->seconds -= 1;
        r->nanoseconds += VTSS_ONE_MIA;
    }
    if (r->nanoseconds > VTSS_ONE_MIA) {
        r->seconds += 1;
        r->nanoseconds -= VTSS_ONE_MIA;
    }
    if (r->seconds < 70000 && x->seconds > (0xffffffffL -70000L)) ++r->sec_msb; /* sec counter has wrapped */
    if (x->seconds < 70000 && r->seconds > (0xffffffffL -70000L)) --r->sec_msb; /* sec counter has wrapped (negative) */
}

void vtss_tod_add_TimeInterval(mesa_timestamp_t *r, const mesa_timestamp_t *x, const mesa_timeinterval_t *y)
{
    // A temporary variable is needed. r and x may point to the same structure, so when updating the
    // fields of the result (r), the value of the original argument (x) is getting lost and timestamp_fix
    // will then not work correctly
    mesa_timestamp_t tmp;
    tmp.seconds = x->seconds + VTSS_INTERVAL_SEC(*y);
    tmp.sec_msb = x->sec_msb;
    tmp.nanoseconds = x->nanoseconds + VTSS_INTERVAL_NS(*y);
    tmp.nanosecondsfrac = x->nanosecondsfrac + (*y & 0xFFFF);

    timestamp_fix(&tmp, x);
    *r = tmp;
}

void vtss_tod_sub_TimeStamp(mesa_timestamp_t *r, const mesa_timestamp_t *x, const mesa_timeinterval_t *y)
{
    mesa_timeinterval_t y_temp = -*y;
    vtss_tod_add_TimeInterval(r, x, &y_temp);
}

void tod_add_timestamps(const mesa_timestamp_t *in1, const mesa_timestamp_t *in2, mesa_timestamp_t *const out)
{
    uint32_t nanosec_frac = in1->nanosecondsfrac + in2->nanosecondsfrac;
    uint64_t nanosec = in1->nanoseconds + in2->nanoseconds;
    uint64_t seconds = in1->seconds + in2->seconds;

    nanosec += (nanosec_frac/TOD_FRAC_NANO_LIMIT) ? 1 : 0;
    out->nanosecondsfrac = nanosec_frac % TOD_FRAC_NANO_LIMIT;

    seconds += (nanosec/VTSS_ONE_MIA);
    out->nanoseconds = nanosec % VTSS_ONE_MIA;

    out->sec_msb = (seconds >> 32) & 0xFFFF;
    out->seconds = seconds & 0xFFFFFFFF;

    out->sec_msb += in1->sec_msb + in2->sec_msb;
}

void tod_sub_timestamps(const mesa_timestamp_t *in1, const mesa_timestamp_t *in2, mesa_timestamp_t *const out)
{
    bool nsec_frac_carry = false;
    bool nsec_carry = false;
    bool sec_carry = false;
    uint64_t tmp = 0;

    nsec_frac_carry = in1->nanosecondsfrac < in2->nanosecondsfrac ? true : false;
    out->nanosecondsfrac = in1->nanosecondsfrac - in2->nanosecondsfrac;

    if (in1->nanoseconds < in2->nanoseconds ||
       (in1->nanoseconds == in2->nanoseconds && nsec_frac_carry)) {
        nsec_carry = true;
        tmp = VTSS_ONE_MIA;
    }
    out->nanoseconds = tmp + in1->nanoseconds - in2->nanoseconds;
    if (nsec_frac_carry) {
        out->nanoseconds--;
    }

    if (in1->seconds < in2->seconds ||
       (in1->seconds == in2->seconds && nsec_carry)) {
        sec_carry = true;
    }
    out->seconds = in1->seconds - in2->seconds;
    if (nsec_carry) {
        out->seconds--;
    }

    out->sec_msb = in1->sec_msb - in2->sec_msb;
    if (sec_carry) {
        out->sec_msb--;
    }
}

#if 0
void arithTest(void)
{
    mesa_timestamp_t r;
    mesa_timestamp_t x [] = {{0,123,4567},{0,123,4567},{0,123,900000123}, {1,4294967295LU,900000123}};
    mesa_timeinterval_t y [] = {4568*(1<<16),-4568*(1<<16),(mesa_timeinterval_t)100000000*(1<<16), (mesa_timeinterval_t)200000012*(1<<16) };
    mesa_timeinterval_t r1;
    mesa_timestamp_t x1 [] = {{0,123,4567},{0,123,4567},{0,123,900000123},{0,123,900000123},{0,123,900000119},{1,1,900000119},{0,4294967295LU,900000123}};
    mesa_timestamp_t y1 [] = {{0,122,4545},{0,124,4545},{0,122,900000125},{0,124,900000125},{0,123,900000123},{0,4294967295LU,900000123},{1,1,900000119}};
    mesa_timestamp_t r2;
    mesa_timestamp_t x2 [] = {{0,123,4567},{0,123,4567},{0,123,900000123}, {1,4294967295LU,900000123}};
    mesa_timeinterval_t y2 [] = {4568*(1<<16),4565*(1<<16),(mesa_timeinterval_t)100000000*(1<<16), (mesa_timeinterval_t)200000012*(1<<16) };
    uint i;
    char str1 [40];
    char str2 [40];
    char str3 [40];
    for (i=0; i < sizeof(y)/sizeof(mesa_timeinterval_t); i++) {
        vtss_tod_add_TimeInterval(&r,&x[i],&y[i]);
        T_I ("vtss_tod_add_TimeInterval: %s = %s + %s\n", TimeStampToString(&r,str1), TimeStampToString(&x[i],str2), vtss_tod_TimeInterval_To_String (&y[i],str3,','));
    }
    for (i=0; i < sizeof(y1)/sizeof(mesa_timestamp_t); i++) {
        vtss_tod_sub_TimeInterval(&r1,&x1[i],&y1[i]);
        T_I ("vtss_tod_sub_TimeInterval: %s = %s - %s\n", vtss_tod_TimeInterval_To_String(&r1,str1,','), TimeStampToString(&x1[i],str2), TimeStampToString (&y1[i],str3));
    }
    for (i=0; i < sizeof(y2)/sizeof(mesa_timeinterval_t); i++) {
        vtss_tod_sub_TimeStamp(&r2,&x2[i],&y2[i]);
        T_I ("subTimeStamp: %s = %s - %s\n", TimeStampToString(&r2,str1), TimeStampToString(&x2[i],str2), vtss_tod_TimeInterval_To_String (&y2[i],str3, ','));
    }
}
#endif

size_t vtss_tod_timestamp_to_string(mesa_timestamp_t *t, size_t sz, char *s)
{
    time_t time;
#if __INTPTR_MAX__ == __INT32_MAX__
    time = t->seconds;
#else
    time = t->sec_msb;
    time = (time << 32) + t->seconds;
#endif

    time += (system_get_tz_off() * 60); // compensate for timezone
    struct tm tm;
    size_t c1 = strftime(s, sz, "%Y/%m/%d %H:%M:%S", gmtime_r(&time,&tm));
    if (c1>0 && t->nanoseconds) {
        size_t c2 = snprintf(s+c1, sz-c1, ".%09d",t->nanoseconds);
        return c2 ? c1+c2 : 0;
    }
    return c1;
}
