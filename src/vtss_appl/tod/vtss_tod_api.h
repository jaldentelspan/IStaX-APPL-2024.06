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

#ifndef _VTSS_TOD_API_H_
#define _VTSS_TOD_API_H_
#include "main_types.h"

#include "microchip/ethernet/switch/api.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VTSS_INTERVAL_PICO(t)           ((((i32)(t & 0xffff)) * 1000) / 0x10000)
// -------------------------------------------------------------------------
// vtss_hw_get_ns_cnt()
// Get the current time in a internal hw format (time counter)
u64 vtss_tod_get_ts_cnt(void);


/**
 * \brief Timer Callback, called from interupt.
 * In estax called from internal timer interrupt every 10 ms.
 * In Jaguar applications called every 1 sec. (When Jaguar NS timer wraps)
 * \return 0 normal, 1 => set GPIO out in Estax, 2 => clear GPIO out in Estax
 *
 */
int vtss_tod_api_callout(void);

/**
 * \brief Get the current time in a Timestamp
 * \param ts pointer to a TimeStamp structure
 * \param tc pointer to a time counter
 *
 */
void vtss_tod_gettimeofday(u32 domain, mesa_timestamp_t *ts, u64 *tc);

/**
 * \brief Set the current time in a Timestamp
 * \param t pointer to a TimeStamp structure
 *
 */
void vtss_tod_settimeofday(const u32 domain, const mesa_timestamp_t *t);

/**
 * \brief Set delta from the current time  
 * \param ts       [IN] pointer to a TimeStamp structure
 * \param begative [IN] True if ts is subtracted from cuttent time, else ts is added.
 *
 */
void vtss_tod_settimeofday_delta(const u32 domain, const mesa_timestamp_t *ts, BOOL negative);

/**
 * \brief Convert a hw time counter to current time (in sec and ns)
 *
 * \param hw_time hw time counter
 * \param t pointer to a TimeStamp structure
 *
 */
void vtss_tod_ts_to_time(u32 domain, u64 hw_time, mesa_timestamp_t *t);

/**
 * \brief Adjust the clock timer ratio
 *
 * \param adj Clock ratio frequency offset in 0,1ppb (parts pr billion).
 *      ratio > 0 => clock runs faster
 *
 */
void vtss_tod_set_adjtimer(u32 domain, i64 adj);

/* hw implementation dependent */

void vtss_tod_ts_cnt_sub(u64 *r, u64 x, u64 y);

void vtss_tod_ts_cnt_add(u64 *r, u64 x, u64 y);

u32 vtss_tod_phy_cnt_to_ts_cnt(u32 ns);

u64 vtss_tod_ts_cnt_to_ns(u64 ts);

void vtss_tod_timeinterval_to_ts_cnt(u64 *r, mesa_timeinterval_t x);

void vtss_tod_ts_cnt_to_timeinterval(mesa_timeinterval_t *r, u64 x);

char *vtss_tod_TimeInterval_To_String(const mesa_timeinterval_t *t, char* str, char delim);

char *vtss_tod_ScaledNs_To_String (const mesa_scaled_ns_t *t, char* str, char delim);

void vtss_tod_sub_TimeInterval(mesa_timeinterval_t *r, const mesa_timestamp_t *x, const mesa_timestamp_t *y);

void vtss_tod_add_TimeInterval(mesa_timestamp_t *r, const mesa_timestamp_t *x, const mesa_timeinterval_t *y);

void vtss_tod_sub_TimeStamps(i64 *r, const mesa_timestamp_t *x, const mesa_timestamp_t *y);

void vtss_tod_sub_TimeStamp(mesa_timestamp_t *r, const mesa_timestamp_t *x, const mesa_timeinterval_t *y);

void vtss_tod_scaledNS_to_timestamp(const mesa_scaled_ns_t *t, mesa_timestamp_t *ts, BOOL *negative);

void vtss_tod_sub_scaled_time(mesa_scaled_ns_t *ret, const mesa_timestamp_t *x, const mesa_timestamp_t *y);

/*!
 * \brief Print time stamp in format %Y/%m/%d %H:%M:%S
 * *
 * \param t   [IN]  Pointer to the time stam to print
 * \param sz  [IN]  Size of the char buffer for printing result
 * \param s   [OUT] Pointer to char buffer where result can be printed
 *
 * \return Number of characters printed, or 0 if buffer is too small
 */
    size_t vtss_tod_timestamp_to_string(mesa_timestamp_t *t, size_t sz, char *s);
    
char *vtss_tod_ns2str(u32 nanoseconds, char *str, char delim);

static inline u16 vtss_tod_unpack16(const u8 *buf)
{
    return (buf[0]<<8) + buf[1];
}

static inline void vtss_tod_pack16(u16 v, u8 *buf)
{
    buf[0] = (v>>8) & 0xff;
    buf[1] = v & 0xff;
}

static inline u32 vtss_tod_unpack24(const u8 *buf)
{
    return (buf[0]<<16) + (buf[1]<<8) + buf[2];
}

static inline void vtss_tod_pack24(u32 v, u8 *buf)
{
    buf[0] = (v>>16) & 0xff;
    buf[1] = (v>>8) & 0xff;
    buf[2] = v & 0xff;
}

static inline u32 vtss_tod_unpack32(const u8 *buf)
{
    return (buf[0]<<24) + (buf[1]<<16) + (buf[2]<<8) + buf[3];
}

static inline void vtss_tod_pack32(u32 v, u8 *buf)
{
    buf[0] = (v>>24) & 0xff;
    buf[1] = (v>>16) & 0xff;
    buf[2] = (v>>8) & 0xff;
    buf[3] = v & 0xff;
}

static inline i64 vtss_tod_unpack64(const u8 *buf)
{
    int i;
    u64 v = 0;
    for (i = 0; i < 8; i++)
    {
        v = v<<8;
        v += buf[i];
    }
    return v;
}

static inline void vtss_tod_pack64(i64 v, u8 *buf)
{
    int i;
    for (i = 7; i >= 0; i--)
    {
        buf[i] = v & 0xff;
        v = v>>8;
    }
}

#ifdef __cplusplus
}

inline mesa_timestamp_t operator+(const mesa_timestamp_t &x, const uint64_t &y)
{
    mesa_timestamp_t r;
    
    uint64_t y_ns = y % MESA_ONE_MIA;
    uint64_t y_s = y / MESA_ONE_MIA;
    uint64_t ovr = 0;
    r.nanosecondsfrac = x.nanosecondsfrac;
    r.nanoseconds = x.nanoseconds + y_ns;
    if (r.nanoseconds > 1E9) {
        ovr = 1;
        r.nanoseconds = r.nanoseconds - MESA_ONE_MIA;
    }
    r.seconds = x.seconds + y_s + ovr;
    ovr = (r.seconds < x.seconds) ? 1 : 0;
    r.sec_msb = x.sec_msb + ovr;
    return r;
}

inline i64 operator-(const mesa_timestamp_t &x, const mesa_timestamp_t &y)
{
    i64 r;
    vtss_tod_sub_TimeStamps(&r, &x, &y);
    return r;
}

inline mesa_timestamp_t operator-(const mesa_timestamp_t &x, const mesa_timeinterval_t &y)
{
    mesa_timestamp_t r;
    vtss_tod_sub_TimeStamp(&r, &x, &y);
    return r;
}

inline bool operator<(const mesa_timestamp_t &x, const mesa_timestamp_t &y)
{
    if (x.sec_msb < y.sec_msb) {
        return true;
    }
    if (x.sec_msb > y.sec_msb) {
        return false;
    }
    if (x.seconds < y.seconds) {
        return true;
    }
    if (x.seconds > y.seconds) {
        return false;
    }
    if (x.nanoseconds < y.nanoseconds) {
        return true;
    }
    if (x.nanoseconds > y.nanoseconds) {
        return false;
    }
    if (x.nanosecondsfrac < y.nanosecondsfrac) {
        return true;
    }
    return false;
}

inline bool operator>(const mesa_timestamp_t &x, const mesa_timestamp_t &y)
{
    return y<x;
}
    
inline bool operator==(const mesa_timestamp_t &x, const mesa_timestamp_t &y)
{
    return (x.sec_msb == y.sec_msb) && (x.seconds == y.seconds) &&
           (x.nanoseconds == y.nanoseconds) && (x.nanosecondsfrac == y.nanosecondsfrac);
}
    
inline bool operator!=(const mesa_timestamp_t &x, const mesa_timestamp_t &y)
{
    return !(x == y);
}

void tod_add_timestamps(const mesa_timestamp_t *in1, const mesa_timestamp_t *in2, mesa_timestamp_t *const out);

void tod_sub_timestamps(const mesa_timestamp_t *in1, const mesa_timestamp_t *in2, mesa_timestamp_t *const out);
#endif

#endif // _VTSS_TOD_API_H_

