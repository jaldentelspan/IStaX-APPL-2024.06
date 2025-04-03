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

#ifdef VTSS_SW_OPTION_FAN

#include "icli_api.h"
#include "icli_porting_util.h"

#include "fan.h"
#include "fan_api.h"
#include "fan_icli_functions.h"

#include "msg_api.h"
#ifdef VTSS_SW_OPTION_ICFG
#include "icfg_api.h"
#endif


/***************************************************************************/
/*  Code start :)                                                           */
/****************************************************************************/
// See fan_icli_functions.h
void fan_status(i32 session_id)
{
  vtss_appl_fan_status_t   status;
  char header_txt[255];
  char str_buf[255];
  switch_iter_t   sit;
  mesa_rc rc;
  u8 sensor_id;
  u8 sensor_cnt;
  // Loop through all switches in stack

  (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_USID);
  while (switch_iter_getnext(&sit)) {
    if ((rc = vtss_appl_fan_status_get(&status, sit.isid))) {
      ICLI_PRINTF("%s\n", error_txt(rc));
    }
    strcpy(header_txt, ""); //Clear string
    (void)vtss_appl_fan_temperature_sensors_count_get(sit.isid, &sensor_cnt);
    for (sensor_id = 0; sensor_id < sensor_cnt; sensor_id++) {
      strcat(header_txt, "Chip Temp.  ");
    }
    strcat(header_txt, "Fan Speed\n");

    ICLI_PRINTF("%s", header_txt);
    T_N("vtss_board_type():%d, sensor_cnt:%d", vtss_board_type(), sensor_cnt);
    for (sensor_id = 0; sensor_id < sensor_cnt; sensor_id++) {
      sprintf(str_buf, "%d %s", status.chip_temp[sensor_id], "C");
      ICLI_PRINTF("%-12s", str_buf);
    }
    sprintf(str_buf, "%d %s", status.fan_speed, "RPM");
    ICLI_PRINTF("%-10s", str_buf);
  }
  ICLI_PRINTF("\n");
}

// See fan_icli_functions.h
mesa_rc fan_temp(u32 session_id, BOOL has_t_on, i8 new_temp_on, BOOL has_t_max, i8 new_temp_max, BOOL has_pwm, mesa_fan_pwd_freq_t pwm, BOOL no)
{
  vtss_appl_fan_conf_t     fan_conf;
  vtss_appl_fan_conf_t     default_fan_conf;
  T_I("has_t_max:%d, new_temp_max:%d, has_t_on:%d, new_temp_on:%d\n", has_t_max, new_temp_max, has_t_on, new_temp_on);

  // Get configuration for the current switch
  (void)vtss_appl_fan_conf_get(&fan_conf);
  (void)vtss_appl_fan_default_conf_get(&default_fan_conf);
  // update with new configuration
  if (has_t_on) {
    if (no) {
      fan_conf.glbl_conf.t_on = default_fan_conf.glbl_conf.t_on;
    } else {
      fan_conf.glbl_conf.t_on = new_temp_on;
    }
  }
  if (has_t_max) {
    if (no) {
      fan_conf.glbl_conf.t_max = default_fan_conf.glbl_conf.t_max;
    } else {
      fan_conf.glbl_conf.t_max  = new_temp_max;
    }
  }

  if (has_pwm) {
    if (no) {
      fan_conf.glbl_conf.pwm = default_fan_conf.glbl_conf.pwm;
    } else {
      fan_conf.glbl_conf.pwm = pwm;
    }
    (void)vtss_appl_fan_default_conf_get(&default_fan_conf);
  }
  if (no && (!has_t_on) && (!has_t_max) && (!has_pwm)) {
    fan_conf.glbl_conf.t_on = default_fan_conf.glbl_conf.t_on;
    fan_conf.glbl_conf.t_max = default_fan_conf.glbl_conf.t_max;
    fan_conf.glbl_conf.pwm = default_fan_conf.glbl_conf.pwm;
  }

  if (fan_conf.glbl_conf.t_on >= fan_conf.glbl_conf.t_max) {
    ICLI_PRINTF("temp-max (%d) MUST be higher than temp-on (%d)\n", fan_conf.glbl_conf.t_max, fan_conf.glbl_conf.t_on);
    return VTSS_APPL_FAN_ERROR_T_CONF;
  }

  // Write back new configuration
  ICLI_RC_CHECK_PRINT_RC(vtss_appl_fan_conf_set(&fan_conf));
  return VTSS_RC_OK;
}

#ifdef VTSS_SW_OPTION_ICFG

/* ICFG callback functions */
static mesa_rc fan_global_conf(const vtss_icfg_query_request_t *req,
                               vtss_icfg_query_result_t *result)
{
  vtss_appl_fan_conf_t     fan_conf;
  vtss_appl_fan_conf_t     default_fan_conf;
  char                     buf[128] = "";
  char                    *pbuf = buf;
  // Get configuration for the current switch
  (void)vtss_appl_fan_conf_get(&fan_conf);
  (void)vtss_appl_fan_default_conf_get(&default_fan_conf);

  vtss_icfg_conf_print_t conf_print;
  vtss_icfg_conf_print_init(&conf_print, req, result);
  conf_print.show_default_values = TRUE;

  if (req->all_defaults || fan_conf.glbl_conf.t_on != default_fan_conf.glbl_conf.t_on) {
    pbuf += sprintf(pbuf, "temp-on %d ", fan_conf.glbl_conf.t_on);
  }
  if (req->all_defaults || fan_conf.glbl_conf.t_max != default_fan_conf.glbl_conf.t_max) {
    pbuf += sprintf(pbuf, "temp-max %d ", fan_conf.glbl_conf.t_max);
  }
  if (req->all_defaults || fan_conf.glbl_conf.pwm != default_fan_conf.glbl_conf.pwm) {
    pbuf += sprintf(pbuf, "pwm %s", mesa_fan_pwd_freq_t_to_str(fan_conf.glbl_conf.pwm));
  }
  if (pbuf != buf)  {
    VTSS_RC(vtss_icfg_conf_print(&conf_print, "green-ethernet fan", "%s", buf));
  }
  return VTSS_RC_OK;
}


/* ICFG Initialization function */
mesa_rc fan_icfg_init(void)
{
  VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_FAN_GLOBAL_CONF, "green-ethernet", fan_global_conf));
  return VTSS_RC_OK;
}
#endif // VTSS_SW_OPTION_ICFG
#endif // #ifdef VTSS_SW_OPTION_FAN

