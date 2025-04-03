/*
 Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _VTSS_DAYLIGHT_SAVING_API_H_
#define _VTSS_DAYLIGHT_SAVING_API_H_
#include "vtss/appl/daylight_saving.h"
#ifdef __cplusplus
extern "C" {
#endif

/* time daylight saving error codes (mesa_rc) */
typedef enum {
    TIME_DST_ERROR_GEN = MODULE_ERROR_START(VTSS_MODULE_ID_DAYLIGHT_SAVING),    /* Generic error code */
    TIME_DST_ERROR_PARM,                                                        /* Illegal parameter */
    TIME_DST_ERROR_STACK_STATE,                                                 /* Illegal primary/secondary switch state */
    TIME_DST_ERROR_ZONE_SUBTYPE                                                 /* Illegal time zone subtype */
} time_dst_error_t;

/* daylight saving time configuration */
typedef struct {
    vtss_appl_clock_summertime_mode_t    dst_mode;
    vtss_appl_clock_summertime_struct_t  dst_start_time;
    vtss_appl_clock_summertime_struct_t  dst_end_time;
    u32                                  dst_offset;             /* 1 - 1439 minutes */
    char                                 tz_acronym[VTSS_APPL_CLOCK_TIMEZONE_NAME_LENGTH + 1];
    int                                  tz_offset;              /* +- 1439 minutes   */
    int                                  tz;
    /* system timezone: (total minutes)*10 + id
     -7200: "(UTC-12:00) International Date Line West",
     -6600: "(UTC-11:00) Coordinated Universal Time-11",
     -6000: "(UTC-10:00) Hawaii",
     -6001: "(UTC-10:00) Aleutian Island",
     -5700: "(UTC-09:30) Marquesas Islands",
     -5400: "(UTC-09:00) Alaska",
     -5401: "(UTC-09:00) Coordinated Universal Time-09",
     -4800: "(UTC-08:00) Pacific Time (US and Canada)",
     -4801: "(UTC-08:00) Baja California",
     -4802, "(UTC-08:00) Coordinated Universal Time-08",
     -4200: "(UTC-07:00) Arizona",
     -4201: "(UTC-07:00) Chihuahua, La Paz, Mazatlan",
     -4202: "(UTC-07:00) Mountain Time (US and Canada)",
     -3600: "(UTC-06:00) Central America",
     -3601: "(UTC-06:00) Central Time (US and Canada)",
     -3602: "(UTC-06:00) Easter Island",
     -3603: "(UTC-06:00) Guadalajara, Mexico City, Monterrey",
     -3604: "(UTC-06:00) Saskatchewan",
     -3000: "(UTC-05:00) Bogota, Lima, Quito, Rio Branco",
     -3001: "(UTC-05:00) Eastern Time (US and Canada)",
     -3002: "(UTC-05:00) Indiana (East)",
     -3003: "(UTC-05:00) Chetumal",
     -3004: "(UTC-05:00) Haiti",
     -3005: "(UTC-05:00) Havana",
     -2400: "(UTC-04:00) Atlantic Time (Canada)",
     -2401: "(UTC-04:00) Georgetown, La Paz, Manaus, San Juan"
     -2402: "(UTC-04:00) Asuncion",
     -2403: "(UTC-04:00) Santiago",
     -2404: "(UTC-04:00) Caracas",
     -2405: "(UTC-04:00) Cuiaba",
     -2406: "(UTC-04:00) Turks and Caicos",
     -2100: "(UTC-03:30) Newfoundland",
     -1800: "(UTC-03:00) Brasilia",
     -1801: "(UTC-03:00) Buenos Aires",
     -1802: "(UTC-03:00) Cayenne, Fortaleza",
     -1803: "(UTC-03:00) Greenland",
     -1804: "(UTC-03:00) Montevideo",
     -1805, "(UTC-03:00) Araguaina",
     -1806, "(UTC-03:00) Saint Pierre and Miquelon",
     -1807, "(UTC-03:00) Salvador",
     -1200: "(UTC-02:00) Coordinated Universal Time-02",
      -600: "(UTC-01:00) Azores",
      -601: "(UTC-01:00) Cape Verde Is.",
         0: "(UTC)       Coordinated Universal Time",
         1: "(UTC+00:00) Casablanca",
         2: "(UTC+00:00) Dublin, Edinburgh, Lisbon, London",
         3: "(UTC+00:00) Monrovia, Reykjavik",
       600: "(UTC+01:00) Amsterdam, Berlin, Bern, Rome, Stockholm, Vienna",
       601: "(UTC+01:00) Belgrade, Bratislava, Budapest, Ljubljana, Prague",
       602: "(UTC+01:00) Brussels, Copenhagen, Madrid, Paris",
       603: "(UTC+01:00) Sarajevo, Skopje, Warsaw, Zagreb",
       604: "(UTC+01:00) West Central Africa",
       605: "(UTC+01:00) Windhoek",
      1200: "(UTC+02:00) Amman",
      1201: "(UTC+02:00) Athens, Bucharest",
      1202: "(UTC+02:00) Beirut",
      1203: "(UTC+02:00) Cairo",
      1204: "(UTC+02:00) Harare, Pretoria",
      1205: "(UTC+02:00) Helsinki, Kyiv, Riga, Sofia, Tallinn, Vilnius",
      1206: "(UTC+02:00) Jerusalem",
      1207: "(UTC+02:00) Kaliningrad",
      1208: "(UTC+02:00) Tripoli",
      1209" "(UTC+02:00) Coordinated Universal Time+09",
      1800: "(UTC+03:00) Baghdad",
      1801: "(UTC+03:00) Kuwait, Riyadh",
      1802: "(UTC+03:00) Moscow, St. Petersburg, Volgograd",
      1803: "(UTC+03:00) Nairobi",
      1804: "(UTC+03:00) Istanbul",
      1805: "(UTC+03:00) Minsk",
      2100: "(UTC+03:30) Tehran",
      2400: "(UTC+04:00) Abu Dhabi, Muscat",
      2401: "(UTC+04:00) Baku",
      2402: "(UTC+04:00) Astrakhan, Ulyanovsk",
      2403: "(UTC+04:00) Port Louis",
      2404: "(UTC+04:00) Yerevan",
      2405: "(UTC+04:00) Tbilisi",
      2406: "(UTC+04:00) Izhevsk, Samara",
      2700: "(UTC+04:30) Kabul",
      3000: "(UTC+05:00) Ekaterinburg",
      3001: "(UTC+05:00) Islamabad, Karachi",
      3002: "(UTC+05:00) Tashkent",
      3300: "(UTC+05:30) Chennai, Kolkata, Mumbai, New Delhi",
      3301: "(UTC+05:30) Sri Jayawardenapura",
      3450: "(UTC+05:45) Kathmandu",
      3600: "(UTC+06:00) Astana",
      3601: "(UTC+06:00) Dhaka",
      3602: "(UTC+06:00) Omsk",
      3900: "(UTC+06:30) Yangon (Rangoon)",
      4200: "(UTC+07:00) Bangkok, Hanoi, Jakarta",
      4201: "(UTC+07:00) Krasnoyarsk",
      4202: "(UTC+07:00) Hovd",
      4203: "(UTC+07:00) Novosibirsk",
      4204: "(UTC+07:00) Tomsk",
      4205: "(UTC+07:00) Barnaul, Gorno-Altaysk",
      4800: "(UTC+08:00) Beijing, Chongqing, Hong Kong, Urumqi",
      4801: "(UTC+08:00) Irkutsk, Ulaan Bataar",
      4802: "(UTC+08:00) Kuala Lumpur, Singapore",
      4803: "(UTC+08:00) Perth",
      4804: "(UTC+08:00) Taipei",
      4805, "(UTC+08:00) Ulaanbaatar",
      5100: "(UTC+08:30) Pyongyang",
      5250: "(UTC+08:45) Eucla",
      5400: "(UTC+09:00) Osaka, Sapporo, Tokyo",
      5401: "(UTC+09:00) Seoul",
      5402: "(UTC+09:00) Yakutsk",
      5403: "(UTC+09:00) Chita",
      5700: "(UTC+09:30) Adelaide",
      5701: "(UTC+09:30) Darwin",
      6000: "(UTC+10:00) Brisbane",
      6001: "(UTC+10:00) Canberra, Melbourne, Sydney",
      6002: "(UTC+10:00) Guam, Port Moresby",
      6003: "(UTC+10:00) Hobart",
      6004: "(UTC+10:00) Vladivostok",
      6300: "(UTC+10:30) Lord Howe Island",
      6600: "(UTC+11:00) Magadan",
      6601: "(UTC+11:00) Bougainville Island",
      6602: "(UTC+11:00) Chokurdakh",
      6603: "(UTC+11:00) Norfolk Island",
      6604: "(UTC+11:00) Sakhalin",
      6605, "(UTC+11:00) Solomon Is., New Caledonia",
      7200: "(UTC+12:00) Auckland, Wellington",
      7201: "(UTC+12:00) Fiji",
      7202: "(UTC+12:00) Coordinated Universal Time+12",
      7203: "(UTC+12:00) Anadyr, Petropavlovsk-Kamchatsky",
      7650: "(UTC+12:45) Chatham Islands",
      7800: "(UTC+13:00) Nuku'alofa",
      7801: "(UTC+13:00) Samoa",
      8400: "(UTC+14:00) Kiritimati Island"
    */
} time_conf_t;

/* Using the special string '' to present the null acronym timezone */
#define TIME_DST_NULL_ACRONYM   "''"

/* daylight saving error text */
const char *daylight_saving_error_txt(time_dst_error_t rc);

/* daylight saving time, Set system defaults */
void time_dst_default_set(time_conf_t *conf);

/* Get daylight saving time offset */
u32 time_dst_get_offset(void);

/* Get daylight saving configuration */
mesa_rc time_dst_get_config(time_conf_t *conf);

/* Set daylight saving configuration */
mesa_rc time_dst_set_config(time_conf_t *conf);

/* Update time zone offset */
mesa_rc time_dst_update_tz_offset(int tz_off);

/* Initialize module */
mesa_rc time_dst_init(vtss_init_data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* _VTSS_DAYLIGHT_SAVING_API_H_ */

