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

#include "alarm_icli_functions.h"
#include "vtss/appl/alarm.h"
// #include "misc_api.h"
// #include "icli_porting_util.h"
// #include "topo_api.h"

/***************************************************************************/
/*  Internal types                                                         */
/****************************************************************************/

/***************************************************************************/
/*  Internal functions                                                     */
/****************************************************************************/

/***************************************************************************/
/*  Functions called by iCLI                                                */
/****************************************************************************/

static mesa_rc lookup_alarm_stat(const char *alarm_name,
                                 vtss_appl_alarm_name_t *const out) {
    mesa_rc res;
    vtss_appl_alarm_name_t tmp_name_in = {};

    if (alarm_name && strcmp(alarm_name, "") != 0) {
        res = vtss_appl_alarm_status_itr((vtss_appl_alarm_name_t *)0, out);
        while (res == VTSS_RC_OK) {
            if (strcmp(alarm_name, out->alarm_name) == 0) {
                return VTSS_RC_OK;
            }
            tmp_name_in = *out;
            res = vtss_appl_alarm_status_itr(&tmp_name_in, out);
        }
    }
    icli_session_self_printf("   Alarm=\"%s\" Not Found\n", alarm_name);
    return VTSS_RC_ERROR;
}

static mesa_rc lookup_alarm_conf(const char *alarm_name,
                                 vtss_appl_alarm_name_t *const out) {
    vtss_appl_alarm_name_t tmp_name_in;
    mesa_rc res;
    if ((alarm_name != (char *)0) && strcmp(alarm_name, "") != 0) {
        res = vtss_appl_alarm_conf_itr((vtss_appl_alarm_name_t *)0, out);
        while (res == VTSS_RC_OK) {
            if (strcmp(alarm_name, out->alarm_name) == 0) {
                return VTSS_RC_OK;
            }
            tmp_name_in = *out;
            res = vtss_appl_alarm_conf_itr(&tmp_name_in, out);
        }
    }
    icli_session_self_printf("   (Created) Alarm=\"%s\" Not Found\n", alarm_name);
    return VTSS_RC_ERROR;
}

icli_rc_t alarm_create_alarm(u32 session_id, const char *alarm_name,
                             const char *alarm_expression) {
    vtss_appl_alarm_name_t tmp_name = {};
    vtss_appl_alarm_expression_t tmp_expression = {};
    mesa_rc res;
    if (alarm_name) {
        if (strlen(alarm_name) >= ALARM_NAME_SIZE) {
            icli_session_self_printf("   Error:strlen of Alarm_name=\"%s\" too big %zd >= %d\n",
                                     alarm_name, strlen(alarm_name), ALARM_NAME_SIZE);
            return ICLI_RC_ERROR;
        }
        strcpy(tmp_name.alarm_name, alarm_name);
    }

    if (alarm_expression) {
        if (strlen(alarm_expression) >= ALARM_EXPRESSION_SIZE) {
            icli_session_self_printf("   Error:strlen of Alarm_expression=\"%s\" too big %zd >= %d\n",
               alarm_expression, strlen(alarm_expression), ALARM_EXPRESSION_SIZE);
            return ICLI_RC_ERROR;
        }
        strcpy(tmp_expression.alarm_expression, alarm_expression);
    }

    res = vtss_appl_alarm_conf_add(&tmp_name, &tmp_expression);
    if (res != VTSS_RC_OK) {
        icli_session_self_printf(" Error: Failed creating alarm=\"%s\"\n", alarm_name);
        return ICLI_RC_ERROR;
    } else {
        return ICLI_RC_OK;
    }
}


// note alarm_name is optional in alarm_delete_alarm
icli_rc_t alarm_delete_alarm(u32 session_id, const char *alarm_name) {
    vtss_appl_alarm_name_t tmp_name_in;
    vtss_appl_alarm_name_t tmp_name_out;
    mesa_rc res;
    mesa_rc res1;
    if (alarm_name != (char *)0 && strlen(alarm_name) >= ALARM_NAME_SIZE) {
        icli_session_self_printf("   Error:strlen of Alarm=\"%s\" too big %zd >= %d\n", alarm_name,
               strlen(alarm_name), ALARM_NAME_SIZE);
        return ICLI_RC_ERROR;
    }
    if ((alarm_name == (char *)0) || strcmp(alarm_name, "") == 0) {

        res = vtss_appl_alarm_conf_itr((vtss_appl_alarm_name_t *)0,
                                       &tmp_name_out);
        while (res == VTSS_RC_OK) {
            res1 = vtss_appl_alarm_conf_del(&tmp_name_out);
            if (res1 != VTSS_RC_OK) {
                icli_session_self_printf(" Error: Failed deleting alarm=\"%s\"\n", alarm_name);
            }
            tmp_name_in = tmp_name_out;
            res = vtss_appl_alarm_conf_itr(&tmp_name_in, &tmp_name_out);
        }
    } else {
        res = lookup_alarm_conf(alarm_name, &tmp_name_out);
        if (res == VTSS_RC_OK) {
            res1 = vtss_appl_alarm_conf_del(&tmp_name_out);
            if (res1 != VTSS_RC_OK) {
                icli_session_self_printf(" Error: Failed deleting alarm=\"%s\"\n", alarm_name);
                return ICLI_RC_ERROR;
            } else {
                return ICLI_RC_OK;
            }
        }
    }
    return ICLI_RC_OK;
}


icli_rc_t alarm_suppress_alarm(u32 session_id, const char *alarm_name) {
    vtss_appl_alarm_name_t tmp_name = {};
    mesa_rc res;
    mesa_rc res1;
    vtss_appl_alarm_suppression_t supp;

    if (alarm_name) {
        if (strlen(alarm_name) >= ALARM_NAME_SIZE) {
            icli_session_self_printf("   Error:strlen of Alarm=\"%s\" too big %zd >= %d\n", alarm_name,
                                     strlen(alarm_name), ALARM_NAME_SIZE);
            return ICLI_RC_ERROR;
        }
        res = lookup_alarm_stat(alarm_name, &tmp_name);
        if (res == VTSS_RC_OK) {
            supp.suppress = true;
            res1 = vtss_appl_alarm_suppress_set(&tmp_name, &supp);
            if (res1 != VTSS_RC_OK) {
                icli_session_self_printf(" Error: Failed suppressing alarm=\"%s\"\n", alarm_name);
                return ICLI_RC_ERROR;
            } else {
                return ICLI_RC_OK;
            }
        }
        return ICLI_RC_OK;
    }
    return ICLI_RC_ERROR;

}

icli_rc_t alarm_no_suppress_alarm(u32 session_id, const char *alarm_name) {
    vtss_appl_alarm_name_t tmp_name = {};
    mesa_rc res;
    mesa_rc res1;
    vtss_appl_alarm_suppression_t supp;

    if (alarm_name) {
        if (strlen(alarm_name) >= ALARM_NAME_SIZE) {
            icli_session_self_printf("   Error:strlen of Alarm=\"%s\" too big %zd >= %d\n", alarm_name,
                                     strlen(alarm_name), ALARM_NAME_SIZE);
            return ICLI_RC_ERROR;
        }
    }

    res = lookup_alarm_stat(alarm_name, &tmp_name);
    if (res == VTSS_RC_OK) {
        supp.suppress = false;
        res1 = vtss_appl_alarm_suppress_set(&tmp_name, &supp);
        if (res1 != VTSS_RC_OK) {
            icli_session_self_printf(" Error: Failed unsuppressing alarm=\"%s\"\n", alarm_name);
            return ICLI_RC_ERROR;
        } else {
            return ICLI_RC_OK;
        }
    }
    return ICLI_RC_OK;
}


// note filter is optional in alarm_show_alarm_sources
icli_rc_t alarm_show_alarm_sources(u32 session_id,char *filter) {
    //    return ICLI_RC_ERROR;

   vtss_appl_alarm_source_t as;
   int tmp_index_in;
   int tmp_index_out;
   int n,i,j,k;

   if (vtss_appl_alarm_sources_itr((int *)0, &tmp_index_out) != VTSS_RC_OK)
   {
      return ICLI_RC_ERROR;
   }

   icli_session_self_printf("alarmName                                                                       type                          [enum values]       \n");
   icli_session_self_printf("------------------------------------------------------------------------------  ----------------------------  --------------------\n");

   do {
     tmp_index_in = tmp_index_out;
     vtss_appl_alarm_sources_get(&tmp_index_in,&as);
     if ((filter == (char *)0) || strstr(as.alarm_name,filter) != (char *)0) {
        icli_session_self_printf("%s",as.alarm_name);
        n=strlen(as.alarm_name);
        if (n <= 78) 
          for (i=n;i<80;i++) icli_session_self_printf(" "); 
        else {
          icli_session_self_printf("\n"); 
          for (i=0;i<80;i++) icli_session_self_printf(" "); 
        }
        icli_session_self_printf("%s",as.type);
        n=strlen(as.type);
        if (n <= 28) 
          for (i=n;i<30;i++) icli_session_self_printf(" "); 
        else {
          icli_session_self_printf("\n"); 
          for (i=0;i<80+30;i++) icli_session_self_printf(" "); 
        }
        j=0;
        k=0;
        if (as.enum_values[j] == 0) icli_session_self_printf("\n"); 
        while (as.enum_values[j] != 0) {
          icli_session_self_printf("%c",as.enum_values[j]);
          if (as.enum_values[j]==',') {
            icli_session_self_printf("\n"); 
            j++;
            for (i=0;i<80+30;i++) icli_session_self_printf(" "); 
            k=0;
          } else {
             k++; 
             j++;
             if (as.enum_values[j] == 0) icli_session_self_printf("\n"); 
          }
        }
     }
   } while  (vtss_appl_alarm_sources_itr(&tmp_index_in, &tmp_index_out) == VTSS_RC_OK);

   return ICLI_RC_OK;
}

// note alarm_name is optional in alarm_show_alarm_status
icli_rc_t alarm_show_alarm_status(u32 session_id, const char *alarm_name) {
    vtss_appl_alarm_name_t tmp_name_in = {};
    vtss_appl_alarm_name_t tmp_name_out = {};
    vtss_appl_alarm_status_t status;
    mesa_rc res;
    mesa_rc res1;
    int n,i;

    if ((alarm_name != (char *)0) && strlen(alarm_name) >= ALARM_NAME_SIZE) {
        icli_session_self_printf("   Error: Strlen of Alarm=\"%s\" too big %zd >= %d\n",
               alarm_name, strlen(alarm_name), ALARM_NAME_SIZE);
        return ICLI_RC_ERROR;
    }

   icli_session_self_printf("alarmName                                                                       Active  Suppressed  ExposedActive\n");
   icli_session_self_printf("------------------------------------------------------------------------------  ------  ----------  -------------\n");
    if ((alarm_name == (char *)0) || strcmp(alarm_name, "") == 0) {
        res = vtss_appl_alarm_status_itr((vtss_appl_alarm_name_t *)0,
                                         &tmp_name_out);
        while (res == VTSS_RC_OK) {
            res1 = vtss_appl_alarm_status_get(&tmp_name_out, &status);
            if (res1 != VTSS_RC_OK) {
                icli_session_self_printf("   Error: Failed reading status of Alarm=\"%s\"\n",
                       tmp_name_out.alarm_name);
            } else {
                icli_session_self_printf("%s",tmp_name_out.alarm_name);
                n=strlen(tmp_name_out.alarm_name);
                if (n <= 78) 
                  for (i=n;i<80;i++) icli_session_self_printf(" "); 
                else {
                  icli_session_self_printf("\n"); 
                  for (i=0;i<80;i++) icli_session_self_printf(" "); 
                }
                icli_session_self_printf("%s   %s       %s\n",
                                     (status.active == true) ? "true " : "false",
                                     (status.suppressed == true) ? "true " : "false",
                                     (status.exposed_active == true)
                                             ? "true "
                                             : "false");
            }
            tmp_name_in = tmp_name_out;
            res = vtss_appl_alarm_status_itr(&tmp_name_in, &tmp_name_out);
        }
    } else {
        res = lookup_alarm_stat(alarm_name, &tmp_name_out);
        if (res == VTSS_RC_OK) {
            res1 = vtss_appl_alarm_status_get(&tmp_name_out, &status);
            if (res1 != VTSS_RC_OK) {
                icli_session_self_printf("   Error: Failed reading status of Alarm=\"%s\"\n",
                       tmp_name_out.alarm_name);
                return ICLI_RC_ERROR;
            } else {
                icli_session_self_printf("%s",tmp_name_out.alarm_name);
                n=strlen(tmp_name_out.alarm_name);
                if (n <= 78) 
                  for (i=n;i<80;i++) icli_session_self_printf(" "); 
                else {
                  icli_session_self_printf("\n"); 
                  for (i=0;i<80;i++) icli_session_self_printf(" "); 
                }
                icli_session_self_printf("%s   %s       %s\n",
                                     (status.active == true) ? "true " : "false",
                                     (status.suppressed == true) ? "true " : "false",
                                     (status.exposed_active == true)
                                             ? "true "
                                             : "false");
            }
        }
    }
    return ICLI_RC_OK;
}

