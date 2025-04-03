/*
 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#include "msg_api.h"
#include "critd_api.h"
#include "time.h"
#include "misc_api.h"

#include "daylight_saving_api.h"
#include "daylight_saving.h"

#if defined(VTSS_SW_OPTION_SYSUTIL)
#include "sysutil_api.h"
#endif /* VTSS_SW_OPTION_SYSUTIL */

#ifdef VTSS_SW_OPTION_ICFG
#include "daylight_saving_icfg.h"
#endif
// test

/****************************************************************************/
/*  Global variables                                                        */
/****************************************************************************/

/* Global structure */
static time_global_t dst_global;
static time_t dst_start_utc_time;
static time_t dst_end_utc_time;

/* Trace registration. Initialized by time_dst_init() */
static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "time_dst", "time (configuration)"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_ERROR
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

#define TIME_DST_CRIT_ENTER() critd_enter(&dst_global.crit, __FILE__, __LINE__)
#define TIME_DST_CRIT_EXIT()  critd_exit( &dst_global.crit, __FILE__, __LINE__)

#define SECSPERMIN          60
#define MINSPERHOUR         60
#define HOURSPERDAY         24
#define DAYSPERWEEK         7
#define DAYSPERNYEAR        365
#define DAYSPERLYEAR        366
#define SECSPERHOUR         (SECSPERMIN * MINSPERHOUR)
#define SECSPERDAY          ((u32) SECSPERHOUR * HOURSPERDAY)
#define MONSPERYEAR         12
#define EPOCH_YEAR          1970

#define IS_LEAP(y) (((y) % 4) == 0 && (((y) % 100) != 0 || ((y) % 400) == 0))

#define VTSS_APPL_VALIDATE_INPUT_RANGE(val, min, max) if(val < min || val > max) {\
                                                            return VTSS_RC_ERROR; \
                                                      }
#define VTSS_APPL_VALIDATE_INPUT_RANGE_MAX(val, max)  if(val > max) {\
                                                            return VTSS_RC_ERROR; \
                                                      }

static const int dst_mon_lengths[2][MONSPERYEAR] = {
    { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
    { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

static const int dst_year_lengths[2] = {
    DAYSPERNYEAR, DAYSPERLYEAR
};

typedef struct {
    uchar   month;
    uchar   date;
    ushort  year;
    uchar   hour;
    uchar   minute;
    u32     dst_offset; /* 1 - 1439 minutes */
    int     tz_offset;  /* +- 1439 minutes   */
} time_dst_non_recurring_utc_t;

typedef struct {
    ushort  year;
    uchar   month;
    uchar   week;
    ushort  day;
    uchar   hour;
    uchar   minute;
    u32     dst_offset; /* 1 - 1439 minutes */
    int     tz_offset;  /* +- 1439 minutes   */
} time_dst_recurring_utc_t;

/****************************************************************************/
/*  Various local functions                                                 */
/****************************************************************************/
/* set timezone offset on system timezone offset to update the value on old parameter */
static mesa_rc _time_dst_set_tz_offset(int tz_off)
{
    mesa_rc       rc = VTSS_RC_OK;
    system_conf_t conf;

    if ((rc = system_get_config(&conf)) != VTSS_RC_OK) {
        return rc;
    }

    conf.tz_off = tz_off;
    rc = system_set_config(&conf);

    return rc;
}

/* Determine if daylight saving configuration has changed */
static int _time_dst_conf_changed(time_conf_t *old, time_conf_t *new_conf)
{
    return (new_conf->dst_mode != old->dst_mode ||
            memcmp(&new_conf->dst_start_time, &old->dst_start_time, sizeof(vtss_appl_clock_summertime_struct_t)) ||
            memcmp(&new_conf->dst_end_time, &old->dst_end_time, sizeof(vtss_appl_clock_summertime_struct_t)) ||
            new_conf->dst_offset != old->dst_offset ||
            strcmp(new_conf->tz_acronym, old->tz_acronym) ||
            new_conf->tz_offset != old->tz_offset ||
            new_conf->tz != old->tz);
}

/* daylight saving time, Set system defaults */
void time_dst_default_set(time_conf_t *conf)
{
    conf->dst_mode = VTSS_APPL_CLOCK_SUMMER_TIME_DISABLED;

    conf->dst_start_time.week   = 1;
    conf->dst_start_time.day    = 1;
    conf->dst_start_time.date   = 1;
    conf->dst_start_time.month  = 1;
    conf->dst_start_time.year   = 2014;
    conf->dst_start_time.hour   = 0;
    conf->dst_start_time.minute = 0;

    conf->dst_end_time.week   = 1;
    conf->dst_end_time.day    = 1;
    conf->dst_end_time.date   = 1;
    conf->dst_end_time.month  = 1;
    conf->dst_end_time.year   = 2097;
    conf->dst_end_time.hour   = 0;
    conf->dst_end_time.minute = 0;

    conf->dst_offset = 1;

    memset(conf->tz_acronym, 0x0, VTSS_APPL_CLOCK_TIMEZONE_NAME_LENGTH + 1);
    //conf->tz_acronym[0] = '\0';
    conf->tz_offset = 0x0;
    conf->tz = 0;
}

mesa_rc vtss_appl_clock_conf_set(const vtss_appl_clock_conf_t *new_conf)
{
    T_D("Entering");
    time_conf_t conf;

    /* check parameter */
    if (new_conf == NULL) {
        T_E("conf == NULL\n");
        return VTSS_RC_ERROR;
    }
    VTSS_RC(time_dst_get_config(&conf));

    //set timezone offset
    conf.tz = (new_conf->timezone_conf.timezone_offset) * 10;
    conf.tz_offset = new_conf->timezone_conf.timezone_offset;

    //set timezone name
    if (strlen(new_conf->timezone_conf.timezone_acronym) > VTSS_APPL_CLOCK_TIMEZONE_NAME_LENGTH) {
        return VTSS_RC_ERROR;
    }
    strncpy(&conf.tz_acronym[0], &(new_conf->timezone_conf.timezone_acronym[0]), sizeof(conf.tz_acronym));

    if (new_conf->summertime_conf.stime_mode == VTSS_APPL_CLOCK_SUMMER_TIME_RECURRING) {
        if (new_conf->summertime_conf.stime_start.date | new_conf->summertime_conf.stime_start.year | new_conf->summertime_conf.stime_end.date
            | new_conf->summertime_conf.stime_end.year) {
            T_D("Year and date must be set to 0, signaling that they are not used.");
            return VTSS_RC_ERROR;
        }
        VTSS_APPL_VALIDATE_INPUT_RANGE(new_conf->summertime_conf.stime_start.week, 1, 5);
        VTSS_APPL_VALIDATE_INPUT_RANGE(new_conf->summertime_conf.stime_end.week, 1, 5);
        VTSS_APPL_VALIDATE_INPUT_RANGE(new_conf->summertime_conf.stime_start.day, 1, 7);
        VTSS_APPL_VALIDATE_INPUT_RANGE(new_conf->summertime_conf.stime_end.day, 1, 7);
    } else if (new_conf->summertime_conf.stime_mode == VTSS_APPL_CLOCK_SUMMER_TIME_NON_RECURRING) {
        if (new_conf->summertime_conf.stime_start.week | new_conf->summertime_conf.stime_start.day | new_conf->summertime_conf.stime_end.week
            | new_conf->summertime_conf.stime_end.day) {
            T_D("Week and day must be set to 0, signaling that they are not used.");
            return VTSS_RC_ERROR;
        }
        VTSS_APPL_VALIDATE_INPUT_RANGE(new_conf->summertime_conf.stime_start.year, 1, 2097);
        VTSS_APPL_VALIDATE_INPUT_RANGE(new_conf->summertime_conf.stime_end.year, 1, 2097);
        VTSS_APPL_VALIDATE_INPUT_RANGE(new_conf->summertime_conf.stime_start.date, 1, 31);
        VTSS_APPL_VALIDATE_INPUT_RANGE(new_conf->summertime_conf.stime_end.date, 1, 31);
    }

    VTSS_APPL_VALIDATE_INPUT_RANGE_MAX(new_conf->summertime_conf.stime_start.week, 5);
    VTSS_APPL_VALIDATE_INPUT_RANGE_MAX(new_conf->summertime_conf.stime_end.week, 5);
    VTSS_APPL_VALIDATE_INPUT_RANGE_MAX(new_conf->summertime_conf.stime_start.day, 7);
    VTSS_APPL_VALIDATE_INPUT_RANGE_MAX(new_conf->summertime_conf.stime_end.day, 7);
    VTSS_APPL_VALIDATE_INPUT_RANGE_MAX(new_conf->summertime_conf.stime_start.year, 2097);
    VTSS_APPL_VALIDATE_INPUT_RANGE_MAX(new_conf->summertime_conf.stime_end.year, 2097);
    VTSS_APPL_VALIDATE_INPUT_RANGE_MAX(new_conf->summertime_conf.stime_start.date, 31);
    VTSS_APPL_VALIDATE_INPUT_RANGE_MAX(new_conf->summertime_conf.stime_end.date, 31);

    VTSS_APPL_VALIDATE_INPUT_RANGE(new_conf->summertime_conf.stime_start.month, 1, 12);
    VTSS_APPL_VALIDATE_INPUT_RANGE(new_conf->summertime_conf.stime_end.month, 1, 12);
    //set summer time  start
    memcpy(&conf.dst_start_time, &(new_conf->summertime_conf.stime_start), sizeof(vtss_appl_clock_summertime_struct_t));
    //set summer time  end
    memcpy(&conf.dst_end_time, &(new_conf->summertime_conf.stime_end), sizeof(vtss_appl_clock_summertime_struct_t));


    //set summer time mode
    conf.dst_mode = new_conf->summertime_conf.stime_mode;
    conf.dst_offset = new_conf->summertime_conf.stime_offset;
    VTSS_RC(time_dst_set_config(&conf));

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_clock_conf_get(vtss_appl_clock_conf_t *conf)
{
    T_D("Entering");
    time_conf_t current_conf;

    if (conf == NULL) {
        T_E("conf == NULL\n");
        return VTSS_RC_ERROR;
    }
    memset(&current_conf, 0, sizeof(time_conf_t));
    VTSS_RC(time_dst_get_config(&current_conf));

    //get timezone offset
    conf->timezone_conf.timezone_offset = current_conf.tz_offset;

    //get timezone name
    memcpy(&(conf->timezone_conf.timezone_acronym[0]), &current_conf.tz_acronym[0], sizeof(current_conf.tz_acronym));
    conf->timezone_conf.timezone_acronym[VTSS_APPL_CLOCK_TIMEZONE_NAME_LENGTH] = '\0';

    //get summer time mode
    conf->summertime_conf.stime_mode = (vtss_appl_clock_summertime_mode_t)current_conf.dst_mode;

    //get summer time starting time
    memcpy(&(conf->summertime_conf.stime_start), &current_conf.dst_start_time, sizeof(vtss_appl_clock_summertime_struct_t));
    memcpy(&(conf->summertime_conf.stime_end), &current_conf.dst_end_time, sizeof(vtss_appl_clock_summertime_struct_t));

    //get summertime offset
    conf->summertime_conf.stime_offset = current_conf.dst_offset;

    return VTSS_RC_OK;
}
/* daylight saving time, none recurring convert */
static u32 _time_dst_non_recurring_utc_convert(time_dst_non_recurring_utc_t *cfg)
{
    /*
     * The function receives the year, month, date, hour, minute, time zone offset and
     * DST offset as input and calculate the UTC time as output.When calculating
     * the start time of DST, the Standard Time is used so dst_offset must be 0.
     * When calculating the end time of DST, the DST time is used so the dst_offset
     * must be the configured value, non-zero.
     */

    time_t     value = 0;
    u16     year, month, leapyear;

    /* Don't use trace api T_D/E/W here becasue it thos api might
     * call to this function to get time and it will cause infinit loop
     */

    //printf("cfg->year: %u\n", cfg->year);
    //printf("cfg->month: %u\n", cfg->month);
    //printf("cfg->date: %u\n", cfg->date);
    //printf("cfg->hour: %u\n", cfg->hour);
    //printf("cfg->minute: %u\n", cfg->minute);
    //printf("cfg->tz_offset: %u\n", cfg->tz_offset);
    //printf("cfg->dst_offset: %u\n", cfg->dst_offset);

    /* year */
    for (year = EPOCH_YEAR; year < cfg->year; ++year) {
        leapyear = IS_LEAP(year);
        value += dst_year_lengths[leapyear] * SECSPERDAY;
    }

    //printf("year: %d\n",value);

    /* month */
    for (month = 0; month < (cfg->month - 1); ++month) {
        leapyear = IS_LEAP(cfg->year);
        value += dst_mon_lengths[leapyear][month] * SECSPERDAY;
    }
    //printf("month: %d\n",value);

    /* date */
    value += (cfg->date - 1) * SECSPERDAY;
    //printf("date: %d\n",value);

    /* hour */
    value += (cfg->hour) * SECSPERHOUR;
    //printf("hour: %d\n",value);

    /* minute */
    value += (cfg->minute) * SECSPERMIN;
    //printf("minute: %d\n",value);

    /* time zone. For example, the time zone is +8:00 in TW so when
     * it is 08:00 am in TW, it is 00:00 am UTC time. We need to
     * subtract the tz_offset to get the real UTC time.
     */
    value -= (cfg->tz_offset) * SECSPERMIN;
    //printf("timezone: %d\n",value);

    /* dst offset. For example, if the dst offset is 60 minutes in TW,
     * it means the time is adjusted to 60 minutes forward. When it is
     * 09:00 am in TW, it is 00:00 am UTC time. We need to subrtact
     * the dst offset to get the real UTC time.
     */
    value -= (cfg->dst_offset) * SECSPERMIN;
    //printf("dst_offset: %d\n",value);

    return value;
}

/* daylight saving time, recurring convert */
static u32 _time_dst_recurring_utc_convert(time_dst_recurring_utc_t *cfg)
{
    int     leapyear;
    time_t  value = 0;
    int     i;
    u16     year;
    int     d, m1, yy0, yy1, yy2, dow;

    /* Don't use trace api T_D/E/W here becasue it thos api might
     * call to this function to get time and it will cause infinit loop
     */

    //printf("cfg->year: %u\n", cfg->year);
    //printf("cfg->month: %u\n", cfg->month);
    //printf("cfg->week: %u\n", cfg->week);
    //printf("cfg->day: %u\n", cfg->day);
    //printf("cfg->hour: %u\n", cfg->hour);
    //printf("cfg->minute: %u\n", cfg->minute);
    //printf("cfg->tz_offset: %u\n", cfg->tz_offset);
    //printf("cfg->dst_offset: %u\n", cfg->dst_offset);

    /*
    ** Mm.n.d - nth "dth day" of month m.
    */
    /* year */
    for (year = EPOCH_YEAR; year < cfg->year; ++year) {
        leapyear = IS_LEAP(year);
        value += dst_year_lengths[leapyear] * SECSPERDAY;

    }
    //printf("year: %d\n",value);

    leapyear = IS_LEAP(year);

    for (i = 0; i < cfg->month - 1; ++i) {
        value += dst_mon_lengths[leapyear][i] * SECSPERDAY;
    }
    //printf("month: %d\n",value);

    /*
    ** Use Zeller's Congruence to get day-of-week of first day of
    ** month.
    */
    m1 = (cfg->month + 9) % 12 + 1;
    yy0 = (cfg->month <= 2) ? (cfg->year - 1) : cfg->year;
    yy1 = yy0 / 100;
    yy2 = yy0 % 100;
    dow = ((26 * m1 - 2) / 10 +
           1 + yy2 + yy2 / 4 + yy1 / 4 - 2 * yy1) % 7;
    if (dow < 0) {
        dow += DAYSPERWEEK;
    }

    /*
    ** "dow" is the day-of-week of the first day of the month. Get
    ** the day-of-month (zero-origin) of the first "dow" day of the
    ** month.
    */
    d = cfg->day - dow;
    if (d < 0) {
        d += DAYSPERWEEK;
    }
    for (i = 1; i < cfg->week; ++i) {
        if (d + DAYSPERWEEK >=
            dst_mon_lengths[leapyear][cfg->month - 1]) {
            break;
        }
        d += DAYSPERWEEK;
    }

    /*
    ** "d" is the day-of-month (zero-origin) of the day we want.
    */
    value += d * SECSPERDAY;
    //printf("day: %d\n",value);

    /* hour */
    value += (cfg->hour) * SECSPERHOUR;
    //printf("hour: %d\n",value);

    /* minute */
    value += (cfg->minute) * SECSPERMIN;
    //printf("minute: %d\n",value);

    /* time zone. For example, the time zone is +8:00 in TW so when
     * it is 08:00 am in TW, it is 00:00 am UTC time. We need to
     * subtract the tz_offset to get the real UTC time.
     */
    value -= (cfg->tz_offset) * SECSPERMIN;
    //printf("timezone: %d\n",value);

    /* dst offset. For example, if the dst offset is 60 minutes in TW,
     * it means the time is adjusted to 60 minutes forward. When it is
     * 09:00 am in TW, it is 00:00 am UTC time. We need to subrtact
     * the dst offset to get the real UTC time.
     */
    value -= (cfg->dst_offset) * SECSPERMIN;
    //printf("dst_offset: %d\n",value);

    return value;
}

static BOOL system_dst_end_in_next_year(time_conf_t *conf)
{
    time_dst_recurring_utc_t      utc_start, utc_end;

    utc_start.year          = EPOCH_YEAR;
    utc_start.month         = conf->dst_start_time.month;
    utc_start.week          = conf->dst_start_time.week;
    utc_start.day           = conf->dst_start_time.day;
    utc_start.hour          = conf->dst_start_time.hour;
    utc_start.minute        = conf->dst_start_time.minute;
    utc_start.tz_offset     = 0;
    utc_start.dst_offset    = 0; /* calculate DST time */

    utc_end.year        = EPOCH_YEAR;
    utc_end.month       = conf->dst_end_time.month;
    utc_end.week        = conf->dst_end_time.week;
    utc_end.day         = conf->dst_end_time.day;
    utc_end.hour        = conf->dst_end_time.hour;
    utc_end.minute      = conf->dst_end_time.minute;
    utc_end.tz_offset   = 0;
    utc_end.dst_offset  = 0; /* calculate DST time */

    return _time_dst_recurring_utc_convert(&utc_end) < _time_dst_recurring_utc_convert(&utc_start) ? TRUE : FALSE;
}

/* daylight saving time, recurring time set */
static void _time_dst_recurring_time_set(time_conf_t *conf)
{
    /*
     * The function receives the year, month, week, day, hour, minute, time zone offset and
     * DST offset as input and calculate the UTC time as output.When calculating
     * the start time of DST, the Standard Time is used so dst_offset must be 0.
     * When calculating the end time of DST, the DST time is used so the dst_offset
     * must be the configured value, non-zero.
     */

    /* Don't use trace api T_D/E/W here becasue it thos api might
     * call to this function to get time and it will cause infinit loop
     */

    time_dst_recurring_utc_t      utc1_cfg;
    time_t     current_time = time(NULL);
    struct tm  *timeinfo_p;
    struct tm  timeinfo;
    u16        year_to_cal;
    BOOL       dst_end_in_next_year = system_dst_end_in_next_year(conf);

    timeinfo_p = localtime_r(&current_time, &timeinfo);
    year_to_cal = timeinfo_p->tm_year + 1900;

//    printf("DST is%s end in next year\n", dst_end_in_next_year ? "" : " NOT");
    /* calculate the end time of DST */
    utc1_cfg.year = year_to_cal;
    utc1_cfg.month = conf->dst_end_time.month;
    utc1_cfg.week = conf->dst_end_time.week;
    utc1_cfg.day = conf->dst_end_time.day;
    utc1_cfg.hour = conf->dst_end_time.hour;
    utc1_cfg.minute = conf->dst_end_time.minute;
    utc1_cfg.tz_offset = conf->tz_offset;
    utc1_cfg.dst_offset = conf->dst_offset; /* calculate DST time */

    dst_end_utc_time = _time_dst_recurring_utc_convert(&utc1_cfg);
    if (current_time > dst_end_utc_time) {
        /* The daylight saving of this year is over and to calculate
         * the duration of next year directly.
         */
        year_to_cal++;
        utc1_cfg.year = year_to_cal;
        dst_end_utc_time = _time_dst_recurring_utc_convert(&utc1_cfg);
    }

    /* calculate the start time of DST */
    /* In Southern Hemisphere countries, the daylight saving time might be end in next year, in this case
     * the start dst year must be minus 1 from end dst year.
     */
    utc1_cfg.year = dst_end_in_next_year ? --year_to_cal : year_to_cal;
    utc1_cfg.month = conf->dst_start_time.month;
    utc1_cfg.week = conf->dst_start_time.week;
    utc1_cfg.day = conf->dst_start_time.day;
    utc1_cfg.hour = conf->dst_start_time.hour;
    utc1_cfg.minute = conf->dst_start_time.minute;
    utc1_cfg.tz_offset = conf->tz_offset;
    utc1_cfg.dst_offset = 0; /* calculate Standard time */
    dst_start_utc_time = _time_dst_recurring_utc_convert(&utc1_cfg);
//    printf("start time:%d, end time:%d, current time:%d%s",
//        dst_start_utc_time, dst_end_utc_time, current_time,
//        current_time > dst_start_utc_time && current_time < dst_end_utc_time ? "(In DST duration, " : "\n");
//    if (current_time > dst_start_utc_time && current_time < dst_end_utc_time) {
//        printf("end after %d second)\n", dst_end_utc_time - current_time);
//    }
}

/* Check time zone and subtype is supported
 * Return TRUE when the timezone is supported. Otherwise, return FALSE.
 * Notice this list have to fully match with web UI drop down menu.
 */
static BOOL _time_dst_zones_is_supported(time_conf_t *conf)
{
    switch (conf->tz) {
    case -7200: // (UTC-12:00) International Date Line West
    case -6600: // (UTC-11:00) Coordinated Universal Time-11
    case -6000: // (UTC-10:00) Hawaii
    case -6001: // (UTC-10:00) Aleutian Island
    case -5700: // (UTC-09:30) Marquesas Islands
    case -5400: // (UTC-09:00) Alaska
    case -5401: // (UTC-09:00) Coordinated Universal Time-09
    case -4800: // (UTC-08:00) Pacific Time (US and Canada)
    case -4801: // (UTC-08:00) Baja California
    case -4802: // (UTC-08:00) Coordinated Universal Time-08
    case -4200: // (UTC-07:00) Arizona
    case -4201: // (UTC-07:00) Chihuahua, La Paz, Mazatlan
    case -4202: // (UTC-07:00) Mountain Time (US and Canada)
    case -3600: // (UTC-06:00) Central America
    case -3601: // (UTC-06:00) Central Time (US and Canada)
    case -3602: // (UTC-06:00) Easter Island
    case -3603: // (UTC-06:00) Guadalajara, Mexico City, Monterrey
    case -3604: // (UTC-06:00) Saskatchewan
    case -3000: // (UTC-05:00) Bogota, Lima, Quito, Rio Branco
    case -3001: // (UTC-05:00) Eastern Time (US and Canada)
    case -3002: // (UTC-05:00) Indiana (East)
    case -3003: // (UTC-05:00) Chetumal
    case -3004: // (UTC-05:00) Haiti
    case -3005: // (UTC-05:00) Havana
    case -2400: // (UTC-04:00) Atlantic Time (Canada)
    case -2401: // (UTC-04:00) Georgetown, La Paz, Manaus, San Juan
    case -2402: // (UTC-04:00) Asuncion
    case -2403: // (UTC-04:00) Santiago
    case -2404: // (UTC-04:00) Caracas
    case -2405: // (UTC-04:00) Cuiaba
    case -2406: // (UTC-04:00) Turks and Caicos
    case -2100: // (UTC-03:30) Newfoundland
    case -1800: // (UTC-03:00) Brasilia
    case -1801: // (UTC-03:00) Buenos Aires
    case -1802: // (UTC-03:00) Cayenne, Fortaleza
    case -1803: // (UTC-03:00) Greenland
    case -1804: // (UTC-03:00) Montevideo
    case -1805: // (UTC-03:00) Araguaina
    case -1806: // (UTC-03:00) Saint Pierre and Miquelon
    case -1807: // (UTC-03:00) Salvador
    case -1200: // (UTC-02:00) Coordinated Universal Time-02
    case  -600: // (UTC-01:00) Azores
    case  -601: // (UTC-01:00) Cape Verde Is.
    case     0: // (UTC)       Coordinated Universal Time
    case     1: // (UTC+00:00) Casablanca
    case     2: // (UTC+00:00) Dublin, Edinburgh, Lisbon, London
    case     3: // (UTC+00:00) Monrovia, Reykjavik
    case   600: // (UTC+01:00) Amsterdam, Berlin, Bern, Rome, Stockholm, Vienna
    case   601: // (UTC+01:00) Belgrade, Bratislava, Budapest, Ljubljana, Prague
    case   602: // (UTC+01:00) Brussels, Copenhagen, Madrid, Paris
    case   603: // (UTC+01:00) Sarajevo, Skopje, Warsaw, Zagreb
    case   604: // (UTC+01:00) West Central Africa
    case   605: // (UTC+01:00) Windhoek
    case  1200: // (UTC+02:00) Amman
    case  1201: // (UTC+02:00) Athens, Bucharest
    case  1202: // (UTC+02:00) Beirut
    case  1203: // (UTC+02:00) Cairo
    case  1204: // (UTC+02:00) Harare, Pretoria
    case  1205: // (UTC+02:00) Helsinki, Kyiv, Riga, Sofia, Tallinn, Vilnius
    case  1206: // (UTC+02:00) Jerusalem
    case  1207: // (UTC+02:00) Kaliningrad
    case  1208: // (UTC+02:00) Tripoli
    case  1209: // (UTC+02:00) Coordinated Universal Time+09
    case  1800: // (UTC+03:00) Baghdad
    case  1801: // (UTC+03:00) Kuwait, Riyadh
    case  1802: // (UTC+03:00) Moscow, St. Petersburg, Volgograd
    case  1803: // (UTC+03:00) Nairobi
    case  1804: // (UTC+03:00) Istanbul
    case  1805: // (UTC+03:00) Minsk
    case  2100: // (UTC+03:30) Tehran
    case  2400: // (UTC+04:00) Abu Dhabi, Muscat
    case  2401: // (UTC+04:00) Baku
    case  2402: // (UTC+04:00) Astrakhan, Ulyanovsk
    case  2403: // (UTC+04:00) Port Louis
    case  2404: // (UTC+04:00) Yerevan
    case  2405: // (UTC+04:00) Tbilisi
    case  2406: // (UTC+04:00) Izhevsk, Samara
    case  2700: // (UTC+04:30) Kabul
    case  3000: // (UTC+05:00) Ekaterinburg
    case  3001: // (UTC+05:00) Islamabad, Karachi
    case  3002: // (UTC+05:00) Tashkent
    case  3300: // (UTC+05:30) Chennai, Kolkata, Mumbai, New Delhi
    case  3301: // (UTC+05:30) Sri Jayawardenapura
    case  3450: // (UTC+05:45) Kathmandu
    case  3600: // (UTC+06:00) Astana
    case  3601: // (UTC+06:00) Dhaka
    case  3602: // (UTC+06:00) Omsk
    case  3900: // (UTC+06:30) Yangon (Rangoon)
    case  4200: // (UTC+07:00) Bangkok, Hanoi, Jakarta
    case  4201: // (UTC+07:00) Krasnoyarsk
    case  4202: // (UTC+07:00) Hovd
    case  4203: // (UTC+07:00) Novosibirsk
    case  4204: // (UTC+07:00) Tomsk
    case  4205: // (UTC+07:00) Barnaul, Gorno-Altaysk
    case  4800: // (UTC+08:00) Beijing, Chongqing, Hong Kong, Urumqi
    case  4801: // (UTC+08:00) Irkutsk, Ulaan Bataar
    case  4802: // (UTC+08:00) Kuala Lumpur, Singapore
    case  4803: // (UTC+08:00) Perth
    case  4804: // (UTC+08:00) Taipei
    case  4805: // (UTC+08:00) Ulaanbaatar
    case  5100: // (UTC+08:30) Pyongyang
    case  5250: // (UTC+08:45) Eucla
    case  5400: // (UTC+09:00) Osaka, Sapporo, Tokyo
    case  5401: // (UTC+09:00) Seoul
    case  5402: // (UTC+09:00) Yakutsk
    case  5403: // (UTC+09:00) Chita
    case  5700: // (UTC+09:30) Adelaide
    case  5701: // (UTC+09:30) Darwin
    case  6000: // (UTC+10:00) Brisbane
    case  6001: // (UTC+10:00) Canberra, Melbourne, Sydney
    case  6002: // (UTC+10:00) Guam, Port Moresby
    case  6003: // (UTC+10:00) Hobart
    case  6004: // (UTC+10:00) Vladivostok
    case  6300: // (UTC+10:30) Lord Howe Island
    case  6600: // (UTC+11:00) Magadan
    case  6601: // (UTC+11:00) Bougainville Island
    case  6602: // (UTC+11:00) Chokurdakh
    case  6603: // (UTC+11:00) Norfolk Island
    case  6604: // (UTC+11:00) Sakhalin
    case  6605: // (UTC+11:00) Solomon Is., New Caledonia
    case  7200: // (UTC+12:00) Auckland, Wellington
    case  7201: // (UTC+12:00) Fiji
    case  7202: // (UTC+12:00) Coordinated Universal Time+12
    case  7203: // (UTC+12:00) Anadyr, Petropavlovsk-Kamchatsky
    case  7650: // (UTC+12:45) Chatham Islands
    case  7800: // (UTC+13:00) Nuku'alofa
    case  7801: // (UTC+13:00) Samoa
    case  8400: // (UTC+14:00) Kiritimati Island
        break;
    default:
        if (conf->tz % 10) { // sub_type must equal 0 when the specific time zone(manual setting) is excluded from the options list
            return FALSE;
        }
    }

    return TRUE;
}

/****************************************************************************/
/*  Various global functions                                                */
/****************************************************************************/

/* daylight saving error text */
const char *daylight_saving_error_txt(time_dst_error_t rc)
{
    const char *txt;

    switch (rc) {
    case TIME_DST_ERROR_GEN:
        txt = "Daylight saving generic error";
        break;
    case TIME_DST_ERROR_PARM:
        txt = "Daylight saving parameter error";
        break;
    case TIME_DST_ERROR_STACK_STATE:
        txt = "Daylight saving stack state error";
        break;
    case TIME_DST_ERROR_ZONE_SUBTYPE:
        txt = "Daylight saving time zone subtype error";
        break;
    default:
        txt = "Daylight saving unknown error";
        break;
    }

    return txt;
}

/* daylight saving time, time set */
static void time_dst_time_set(time_conf_t *conf)
{
    time_dst_non_recurring_utc_t  utc_cfg;

    TIME_DST_CRIT_ENTER();
    if (conf->dst_mode == VTSS_APPL_CLOCK_SUMMER_TIME_DISABLED) {
        dst_start_utc_time = 0;
        dst_end_utc_time = 0;
    } else if (conf->dst_mode == VTSS_APPL_CLOCK_SUMMER_TIME_NON_RECURRING) {
        // calculate strat time (UTC)
        utc_cfg.year = conf->dst_start_time.year;
        utc_cfg.month = conf->dst_start_time.month;
        utc_cfg.date = conf->dst_start_time.date;
        utc_cfg.hour = conf->dst_start_time.hour;
        utc_cfg.minute = conf->dst_start_time.minute;
        utc_cfg.dst_offset = 0; /* calculate standard time */
        utc_cfg.tz_offset = conf->tz_offset;
        dst_start_utc_time = _time_dst_non_recurring_utc_convert(&utc_cfg);
        // calculate end time (UTC)
        utc_cfg.year = conf->dst_end_time.year;
        utc_cfg.month = conf->dst_end_time.month;
        utc_cfg.date = conf->dst_end_time.date;
        utc_cfg.hour = conf->dst_end_time.hour;
        utc_cfg.minute = conf->dst_end_time.minute;
        utc_cfg.dst_offset = conf->dst_offset; /* calculate DST time */
        utc_cfg.tz_offset = conf->tz_offset;
        dst_end_utc_time = _time_dst_non_recurring_utc_convert(&utc_cfg);
        //printf("start time:%d, end time:%d\n",dst_start_utc_time, dst_end_utc_time);
    } else if (conf->dst_mode == VTSS_APPL_CLOCK_SUMMER_TIME_RECURRING) {
        _time_dst_recurring_time_set(conf);
    }
    TIME_DST_CRIT_EXIT();

    return;
}

/* daylight saving time, get daylight saving time offset */
u32 time_dst_get_offset(void)
{
    u32     dst_off = 0;
    time_t  current_time = time(NULL);

    /* This is is not protected by TIME_DST_CRIT_ENTER/EXIT, since this would create
       a deadlock for trace statements inside critical regions of this module */

    if (dst_global.system_conf.dst_mode == VTSS_APPL_CLOCK_SUMMER_TIME_DISABLED) {
        dst_off = 0;
    } else if (dst_global.system_conf.dst_mode == VTSS_APPL_CLOCK_SUMMER_TIME_NON_RECURRING) {
        if (current_time >=  dst_start_utc_time && current_time <= dst_end_utc_time) {
            dst_off = dst_global.system_conf.dst_offset;
        } else {
            dst_off = 0;
        }
    } else if (dst_global.system_conf.dst_mode == VTSS_APPL_CLOCK_SUMMER_TIME_RECURRING) {
        if (current_time >  dst_end_utc_time) {
            _time_dst_recurring_time_set(&dst_global.system_conf);
        }
        if (current_time >=  dst_start_utc_time && current_time <= dst_end_utc_time) {
            dst_off = dst_global.system_conf.dst_offset;
        } else {
            dst_off = 0;
        }
    }

    return dst_off;
}

/* Get daylight saving configuration */
mesa_rc time_dst_get_config(time_conf_t *conf)
{
    T_D("enter");

    TIME_DST_CRIT_ENTER();
    *conf = dst_global.system_conf;
    TIME_DST_CRIT_EXIT();

    T_D("exit");

    return VTSS_RC_OK;
}

/* Set daylight saving configuration */
mesa_rc time_dst_set_config(time_conf_t *conf)
{
    mesa_rc rc          = VTSS_RC_OK;
    int     dst_changed = 0;
    time_conf_t temp_conf;

    T_D("enter");

    // Parameter checking
    if (conf->tz_offset <= -1440 || conf->tz_offset >= 1440 ||
        conf->tz <= -14400 || conf->tz >= 14400) {
        T_D("exit: TIME_DST_ERROR_PARM");
        return TIME_DST_ERROR_PARM;
    }
    if (_time_dst_zones_is_supported(conf) == FALSE) {
        T_D("exit: TIME_DST_ERROR_ZONE_SUBTYPE");
        return TIME_DST_ERROR_ZONE_SUBTYPE;
    }

    temp_conf = *conf;
    if (!strcmp(temp_conf.tz_acronym, TIME_DST_NULL_ACRONYM)) {
        temp_conf.tz_acronym[0] = '\0';
    }

    TIME_DST_CRIT_ENTER();
    if (msg_switch_is_primary()) {
        dst_changed = _time_dst_conf_changed(&dst_global.system_conf, conf);
        dst_global.system_conf = temp_conf;
    } else {
        T_W("not primary switch");
        rc = TIME_DST_ERROR_STACK_STATE;
    }
    TIME_DST_CRIT_EXIT();

    if (dst_changed) {
        /* Apply daylight saving time to system */
        time_dst_time_set(&temp_conf);

        /* Apply tz offset to old parameter (sysutil) */
        rc = _time_dst_set_tz_offset(conf->tz_offset);
    }

    T_D("exit");

    return rc;
}

/* update timezone offset from xml module, for backward compatible issue */
mesa_rc time_dst_update_tz_offset(int tz_off)
{
    mesa_rc       rc = VTSS_RC_OK;
    time_conf_t   conf;

    if ((rc = time_dst_get_config(&conf)) != VTSS_RC_OK) {
        return rc;
    }

    conf.tz_offset = tz_off;
    conf.tz = (tz_off * 10);
    rc = time_dst_set_config(&conf);

    return rc;
}

/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/

/* Read/create system switch configuration */
static mesa_rc time_dst_conf_read_switch(vtss_isid_t isid_add)
{
    vtss_isid_t             isid;
    mesa_rc                 rc = VTSS_RC_OK;
    system_conf_t           sys_conf;

    T_D("enter, isid_add: %d", isid_add);

    for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
        if (isid_add != VTSS_ISID_GLOBAL && isid_add != isid) {
            continue;
        }

        TIME_DST_CRIT_ENTER();

        /* Use default values */
        time_dst_default_set(&dst_global.system_conf);
        if ((rc = system_get_config(&sys_conf)) != VTSS_RC_OK) {
            T_D("get timezone fail!");
        } else {
            dst_global.system_conf.tz_offset = sys_conf.tz_off;
            dst_global.system_conf.tz = (sys_conf.tz_off * 10);
        }

        TIME_DST_CRIT_EXIT();
    }

    T_D("exit");
    return rc;
}

/* Read/create system stack configuration */
static mesa_rc time_dst_conf_read_stack(BOOL create)
{
    int                     changed;
    time_conf_t             *old_conf_p, new_conf;
    mesa_rc                 rc = VTSS_RC_OK;
    system_conf_t           sys_conf;

    T_D("enter, create: %d", create);

    changed = 0;
    TIME_DST_CRIT_ENTER();

    /* Use default values */
    time_dst_default_set(&new_conf);
    if ((rc = system_get_config(&sys_conf)) != VTSS_RC_OK) {
        T_D("get timezone fail!");
    } else {
        new_conf.tz_offset = sys_conf.tz_off;
        new_conf.tz = (sys_conf.tz_off * 10);
        T_D("tz_offset %d", new_conf.tz_offset);
        changed = 1;
    }

    old_conf_p = &dst_global.system_conf;
    if (_time_dst_conf_changed(old_conf_p, &new_conf)) {
        changed = 1;
        T_D("run into changed = 1");
    }

    dst_global.system_conf = new_conf;
    TIME_DST_CRIT_EXIT();

    if (changed) {
        /* Apply daylight saving time to system */
        time_dst_time_set(&new_conf);

        /* Apply tz offset to old parameter (sysutil) */
        rc = _time_dst_set_tz_offset(new_conf.tz_offset);
    }

    T_D("exit");

    return rc;
}

/* Module start */
static void time_dst_start(BOOL init)
{
    time_conf_t       *conf_p;

    T_D("enter, init: %d", init);

    if (init) {
        /* Initialize configuration */
        conf_p = &dst_global.system_conf;
        time_dst_default_set(conf_p);

        /* initial DST variables */
        dst_start_utc_time = 0;
        dst_end_utc_time = 0;

        /* Create semaphore for critical regions */
        critd_init(&dst_global.crit, "daylight_saving", VTSS_MODULE_ID_DAYLIGHT_SAVING, CRITD_TYPE_MUTEX);

    } else {
    }

    T_D("exit");
}
#ifdef VTSS_SW_OPTION_PRIVATE_MIB
VTSS_PRE_DECLS void Daylight_saving_mib_init(void);
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_daylight_saving_json_init(void);
#endif

extern "C" int daylight_saving_icli_cmd_register();

/* Initialize module */
mesa_rc time_dst_init(vtss_init_data_t *data)
{
    mesa_rc     rc = VTSS_RC_OK;
    vtss_isid_t isid = data->isid;

    T_D("enter, cmd: %d, isid: %u, flags: 0x%x", data->cmd, data->isid, data->flags);

    switch (data->cmd) {
    case INIT_CMD_INIT:
        T_D("INIT");
        time_dst_start(1);
#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        Daylight_saving_mib_init();
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
        vtss_appl_daylight_saving_json_init();
#endif
        daylight_saving_icli_cmd_register();
#ifdef VTSS_SW_OPTION_ICFG
        if ((rc = vtss_daylight_saving_icfg_init()) != VTSS_RC_OK) {
            T_D("Calling vtss_daylight_saving_icfg_init() failed rc = %s", error_txt(rc));
        }
#endif
        break;

    case INIT_CMD_START:
        T_D("START");
        time_dst_start(0);
        break;

    case INIT_CMD_CONF_DEF:
        T_D("CONF_DEF, isid: %d", isid);
        if (isid == VTSS_ISID_LOCAL) {
            /* Reset local configuration */
        } else if (isid == VTSS_ISID_GLOBAL) {
            /* Reset stack configuration */
            rc = time_dst_conf_read_stack(1);
        } else if (VTSS_ISID_LEGAL(isid)) {
            /* Reset switch configuration */
            rc = time_dst_conf_read_switch(isid);
        }

        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        T_D("ICFG_LOADING_PRE");
        /* Read stack and switch configuration */
        if ((rc = time_dst_conf_read_stack(0)) != VTSS_RC_OK) {
            return rc;
        }

        rc = time_dst_conf_read_switch(VTSS_ISID_GLOBAL);
        break;

    default:
        break;
    }

    return rc;
}

