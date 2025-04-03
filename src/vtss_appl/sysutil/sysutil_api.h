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

#ifndef _VTSS_SYSTEM_API_H_
#define _VTSS_SYSTEM_API_H_

#include "main.h"
#include "vtss/appl/sysutil.h"

#ifdef __cplusplus
extern "C" {
#endif

/* system error codes (mesa_rc) */
typedef enum {
    SYSTEM_ERROR_GEN = MODULE_ERROR_START(VTSS_MODULE_ID_SYSTEM),  /* Generic error code */
    SYSTEM_ERROR_PARM,                      /* Illegal parameter */
    SYSTEM_ERROR_STACK_STATE,               /* Illegal primary/secondary switch state */
    SYSTEM_ERROR_NOT_ADMINISTRATIVELY_NAME, /* Illegal administratively name */
    SYSTEM_ERROR_PARM_NULL,                 /* Illegal parameter data NULL */
    SYSTEM_ERROR_KEY,                       /* Illegal parameter key */
    SYSTEM_ERROR_TM_NOT_SUPPORT,            /* Temperature monitor feature is not supported */
    SYSTEM_ERROR_LICENSE_OPEN,              /* Unable to open license file */
    SYSTEM_ERROR_LICENSE_GETS,              /* Unable to read from license */
    SYSTEM_ERROR_LICENSE_CLOSE,             /* Unable to close license */

} system_error_t;

/* system services */
typedef enum {
    SYSTEM_SERVICES_PHYSICAL     = 1,     /* layer functionality 1 physical (e.g., repeaters) */
    SYSTEM_SERVICES_DATALINK     = 2,     /* layer functionality 2 datalink/subnetwork (e.g., bridges) */
    SYSTEM_SERVICES_INTERNET     = 4,     /* layer functionality 3 internet (e.g., supports the IP) */
    SYSTEM_SERVICES_END_TO_END   = 8,     /* layer functionality 4 end-to-end (e.g., supports the TCP) */
    SYSTEM_SERVICES_SESSION      = 16,     /* layer functionality 5 session */
    SYSTEM_SERVICES_PRESENTATION = 32,    /* layer functionality 6 presentation */
    SYSTEM_SERVICES_APPLICATION  = 64     /* layer functionality 7 application (e.g., supports sthe SMTP) */
} system_services_t;

/* system configuration */
/* Notice that "CUSTOMIZED_SYS_ADMIN_NAME" and "CUSTOMIZED_SYS_ADMIN_NAME_LEN"
 * must be defined in config.mk
 */
#if defined(CUSTOMIZED_SYS_ADMIN_NAME)
    #if CUSTOMIZED_SYS_ADMIN_NAME==1
        #define VTSS_SYS_ADMIN_NAME         ""
        #define VTSS_SYS_ADMIN_NAME_DEFAULT_NULL_STR
    #else
        #define VTSS_SYS_ADMIN_NAME         TO_STR(CUSTOMIZED_SYS_ADMIN_NAME)
    #endif
#else
#define VTSS_SYS_ADMIN_NAME         "admin"
#endif /* CUSTOMIZED_SYS_ADMIN_NAME */

#if defined(CUSTOMIZED_SYS_ADMIN_PASSWORD)
#define VTSS_SYS_ADMIN_PASSWORD     TO_STR(CUSTOMIZED_SYS_ADMIN_PASSWORD)
#else
#define VTSS_SYS_ADMIN_PASSWORD     ""
#endif /* CUSTOMIZED_SYS_ADMIN_PASSWORD */

#if defined(CUSTOMIZED_SYS_PASSWORD_MAGIC_STR)
#define VTSS_SYS_PASSWORD_MAGIC_STR CUSTOMIZED_SYS_PASSWORD_MAGIC_STR
#else
#define VTSS_SYS_PASSWORD_MAGIC_STR "MSCC"
#endif /* CUSTOMIZED_SYS_PASSWORD_MAGIC_STR */

#define VTSS_SYS_STRING_LEN                 256  /* The valid length is 0-255 */
#define VTSS_SYS_INPUT_STRING_LEN           255
#define VTSS_SYS_USERNAME_LEN               32  /* The valid length is 0-31 */
#define VTSS_SYS_INPUT_USERNAME_LEN         31
#define VTSS_SYS_PASSWD_LEN                 32  /* The valid length is 0-31 */
#define VTSS_SYS_INPUT_PASSWD_LEN           31

#define VTSS_SYS_HASH_SHA1_LEN              20  /* The output length of SHA1 */
#define VTSS_SYS_HASH_SHA256_LEN            32  /* The output length of SHA1 */
#define VTSS_SYS_HASH_DIGEST_LEN            VTSS_SYS_HASH_SHA256_LEN  /* Use SHA256 for password encryption */
#define VTSS_SYS_PASSWD_SALT_PADDING_LEN    12  /* The length of password salt padding */
#define VTSS_SYS_PASSWD_ENCRYPTED_LEN       ((VTSS_SYS_HASH_DIGEST_LEN + VTSS_SYS_PASSWD_SALT_PADDING_LEN + VTSS_SYS_HASH_SHA1_LEN) * 2 + 1)
#define VTSS_SYS_PASSWD_ARRAY_SIZE          (VTSS_SYS_USERNAME_LEN > VTSS_SYS_PASSWD_ENCRYPTED_LEN ? VTSS_SYS_PASSWD_LEN : VTSS_SYS_PASSWD_ENCRYPTED_LEN)
#define VTSS_SYS_BASE64_ARRAY_SIZE(x)       ((((x) / 3 + (((x) % 3) ? 1 : 0)) * 4) + 1)

typedef struct {
    char sys_contact[VTSS_SYS_STRING_LEN];
    char sys_name[VTSS_SYS_STRING_LEN];
    char sys_location[VTSS_SYS_STRING_LEN];
#ifndef VTSS_SW_OPTION_USERS
    char sys_passwd[VTSS_SYS_PASSWD_ARRAY_SIZE];
#endif /* VTSS_SW_OPTION_USERS */
    int  sys_services;
    int  tz_off;                /* +- 1439 minutest */
} system_conf_t;

/* system error text */
const char *system_error_txt(mesa_rc rc);

/* check string is administratively name */
BOOL system_name_is_administratively(char string[VTSS_SYS_STRING_LEN]);

int system_get_tz_off(void);                /* TZ offset in minutes */
const char *system_get_tz_display(void);    /* TZ offset for display per ISO8601: +-hhmm */

/* Get system description */
char *system_get_descr(void);

/* Get system configuration */
mesa_rc system_get_config(system_conf_t *conf);

/* Set system configuration */
mesa_rc system_set_config(system_conf_t *conf);

/* Encode the password.
 * Parameter:
 * username      [IN]  - username.
 * clear_pwd     [IN]  - Clear password.
 * salt_padding  [IN]  - The salt for the hash function.
 * encrypted_pwd [OUT] - The encrypted password.
 *
 * Notes that the hash process only affect when the input parameter 'conf->encrypted == FALSE',
 * and it will generate a random salt padding when the input parameter 'salt_padding == NULL'.
 */
void sysutil_password_encode(const char username[VTSS_SYS_USERNAME_LEN], const char clear_pwd[VTSS_SYS_PASSWD_LEN], const char salt_padding[VTSS_SYS_PASSWD_SALT_PADDING_LEN * 2 + 1], char encrypted_pwd[VTSS_SYS_PASSWD_ENCRYPTED_LEN]);

/* A compare function with slower process time.
 * It is to make the compare function slower, so that the brute-force attacks
 * are too slow to be worthwhile. The process speed is slow enough for the
 * attacker, but still fast enough for the normal comparing.
 *
 * Return TRUE when equal, FALSE otherwise.
*/
BOOL sysutil_slow_equals(u8 *a, u32 len_a, u8 *b, u32 len_b);

/* Verify the encrypted password format is valid.
 *
 * Return TRUE when equal, FALSE otherwise.
 */
BOOL sysutil_encrypted_password_fomat_is_valid(const char username[VTSS_SYS_USERNAME_LEN], const char encrypted_password[VTSS_SYS_PASSWD_ENCRYPTED_LEN]);

#ifndef VTSS_SW_OPTION_USERS
/* Encode the system password */
void system_password_encode(const char clear_pwd[VTSS_SYS_PASSWD_LEN], char encrypted_pwd[VTSS_SYS_PASSWD_ENCRYPTED_LEN]);

/* Verify system clear password  */
BOOL system_clear_password_verify(const char clear_pwd[VTSS_SYS_PASSWD_LEN]);

/* Get system encrypted passwd */
const char *system_get_encrypted_passwd(void);

/* Set system passwd */
mesa_rc system_set_passwd(BOOL is_encrypted, const char *pass);
#endif /* VTSS_SW_OPTION_USERS */

mesa_rc system_license_open( void **handle);

// Get one line of the license.
// If include_details is true, the full license file is retrieved line by line,
// otherwise it stops when the raw licenses are beginning.
// Returns MESA_RC_OK on success. Stop when line[0] == '\0'.
// Returns != MESA_RC_OK on error.
// Remember to always invoke system_license_close() once system_license_open()
// has returned VTSS_RC_OK or file handles and memory will be leaked.
mesa_rc system_license_gets( void  *handle, char *line, size_t line_length, bool include_details);

// Once system_license_open() succeeds, remember to always call this one.
mesa_rc system_license_close(void  *handle);

/* Initialize module */
mesa_rc system_init(vtss_init_data_t *data);

/* Get cpu util*/
BOOL vtss_cpuload_get(unsigned int *average_point1s,
                      unsigned int *average_1s,
                      unsigned int *average_10s);

#if defined(VTSS_APPL_SYSUTIL_TM_SUPPORTED)
#define TM_ALARM_CLEAR                FALSE
#define TM_ALARM_RAISE                TRUE
/* Set temperature monitor all config defaults */
mesa_rc system_set_temperature_config_default_all();
/* Get system temperature debug */
mesa_rc system_get_temperature_debug(
    vtss_appl_sysutil_tm_sensor_type_t sensor, int *temp);
/* Get system temperature debug */
mesa_rc system_set_temperature_debug(
    vtss_appl_sysutil_tm_sensor_type_t sensor, int temp);
mesa_rc system_get_temperature_config(
    vtss_appl_sysutil_tm_sensor_type_t sensor,
    vtss_appl_sysutil_tm_config_t      *config);
mesa_rc system_init_temperature_config_default(
    vtss_appl_sysutil_tm_sensor_type_t sensor,
    vtss_appl_sysutil_tm_config_t      *config);
const char *get_sensor_type_name(vtss_appl_sysutil_tm_sensor_type_t sensor);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _VTSS_SYSTEM_API_H_ */

