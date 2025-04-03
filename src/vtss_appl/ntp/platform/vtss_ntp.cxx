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

#include "vtss_ntp.h"
#include "misc_api.h"
#include "mgmt_api.h" //for mgmt_txt2ipv6

#ifdef VTSS_SW_OPTION_ICFG
#include "vtss_ntp_icfg.h"
#endif

// for public API
#include "vtss/basics/expose/snmp/iterator-compose-static-range.hxx"

// for process daemon, ntpd
#include "subject.hxx"
#include <vtss/basics/notifications/process-daemon.hxx>

// Path to NTP configuration file.
#define NTPD_CONF_FILE "/tmp/ntp.conf"
#define NTPD_LOG_FILE "/tmp/ntp.log"

/****************************************************************************/
/*  Global variables                                                        */
/****************************************************************************/

/* Global structure */
static ntp_global_t         ntp_global;
static ntp_conf_t           old_ntp_conf;

static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "ntp", "ntp Module"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

#define NTP_CRIT_ENTER() critd_enter(&ntp_global.crit, __FILE__, __LINE__)
#define NTP_CRIT_EXIT()  critd_exit( &ntp_global.crit, __FILE__, __LINE__)

//static ntp_server_status_t server_info[VTSS_APPL_NTP_SERVER_MAX_COUNT];
//static uchar      ip_addr_string[VTSS_APPL_NTP_SERVER_MAX_COUNT][VTSS_APPL_SYSUTIL_INPUT_DOMAIN_NAME_LEN + 1];

// create external process for ntpd
static vtss::notifications::ProcessDaemon vtss_process_ntpd(&vtss::notifications::subject_main_thread, "ntpd");

/****************************************************************************/
/*  Various local functions                                                 */
/****************************************************************************/

/* Determine if ntp configuration has changed */
static int ntp_conf_changed(ntp_conf_t *old, ntp_conf_t *new_)
{
    return (memcmp(new_, old, sizeof(ntp_conf_t)));
}

/* Set ntp defaults */
static void ntp_default_set(ntp_conf_t *conf)
{
    conf->mode_enabled = NTP_MGMT_DISABLED;
    conf->interval_min = 6;
    conf->interval_max = 8;
    conf->drift_valid  = 1;
    conf->drift_data = 0; /* The value was 22,experiment result, before BugZilla# 1022 fixed */
    conf->drift_trained  = 1;
    memset(&conf->server[0], 0, sizeof(ntp_server_config_t)*VTSS_APPL_NTP_SERVER_MAX_COUNT);
}

/****************************************************************************/
/*  Management functions                                                    */
/****************************************************************************/

static mesa_rc ntp_create_conf_file(ntp_conf_t *conf)
{
    mesa_rc     rc = VTSS_RC_OK;
    FILE        *confdfile;
    int         i;

    confdfile = fopen(NTPD_CONF_FILE, "w+");

    if (confdfile == NULL) {
        T_E("Could not open ntp.conf file");
        rc = NTP_ERROR_PARM;
        return rc;
    } else {

        fprintf(confdfile, "# ntp.conf: Managed by VTSS process daemon\n");

        for (i = 0; i < VTSS_APPL_NTP_SERVER_MAX_COUNT; i++) {
            if (strlen(conf->server[i].ip_host_string) > 0) {
                fprintf(confdfile, "server %s iburst minpoll 5 maxpoll 8\n", conf->server[i].ip_host_string);
            }
        }
        fprintf(confdfile, "\n");

        fprintf(confdfile, "# Allow only time queries, at a limited rate, sending KoD when in excess.\n");
        fprintf(confdfile, "# Allow all local queries (IPv4, IPv6)\n");
        fprintf(confdfile, "restrict default nomodify nopeer noquery limited kod\n");
        fprintf(confdfile, "restrict -6 default nomodify nopeer noquery limited kod\n");
        fprintf(confdfile, "restrict 127.0.0.1\n");
        fprintf(confdfile, "restrict -6 [::1]\n");
        fprintf(confdfile, "\n");
    }

    // Close the file
    if (confdfile) {
        fclose(confdfile);
    }

    return rc;
}

static mesa_rc ntp_conf_set(void)
{
    ntp_conf_t *conf;

    NTP_CRIT_ENTER();
    conf = &ntp_global.ntp_conf;

    if (conf->mode_enabled == NTP_MGMT_ENABLED) {
        // to disable first
        T_D("To disable process daemon on NTP module first.");
        vtss_process_ntpd.adminMode(vtss::notifications::ProcessDaemon::DISABLE);
        VTSS_OS_MSLEEP(1000);
        // create configuration
        ntp_create_conf_file(conf);
        T_D("To enable process daemon on NTP module.");
        vtss_process_ntpd.adminMode(vtss::notifications::ProcessDaemon::ENABLE);
    } else if (conf->mode_enabled == NTP_MGMT_DISABLED) {
        vtss_process_ntpd.adminMode(vtss::notifications::ProcessDaemon::DISABLE);
    }
    NTP_CRIT_EXIT();

    return VTSS_RC_OK;
}

static mesa_rc ntp_validate_server(ntp_server_config_t *server)
{
    int                 ip_str_len;
    mesa_rc             rc;
    mesa_ipv4_t         ipv4;
#ifdef VTSS_SW_OPTION_IPV6
    mesa_ipv6_t         ipv6;
#endif/* VTSS_SW_OPTION_IPV6 */

    switch ( server->ip_type ) {
    case NTP_IP_TYPE_IPV4:
        ip_str_len = strlen(server->ip_host_string);
        if (ip_str_len == 0) {
            return VTSS_RC_OK;
        }

        rc = mgmt_txt2ipv4(server->ip_host_string, &ipv4, NULL, 0);
        if ( rc == VTSS_RC_OK ) {
            return VTSS_RC_OK;
        }

        rc = misc_str_is_domainname(server->ip_host_string);
        if (rc != VTSS_RC_OK) {
            T_D("domain name [%s] error %s\n", server->ip_host_string, error_txt(rc));
        }
        return rc;
    case NTP_IP_TYPE_IPV6:
#ifdef VTSS_SW_OPTION_IPV6
        memset(&ipv6, 0, sizeof(mesa_ipv6_t));
        if (memcmp(&server->ipv6_addr, &ipv6, sizeof(mesa_ipv6_t)) == 0) {
            return ((mesa_rc)NTP_ERROR_PARM);
        } else {
            return VTSS_RC_OK;
        }
#else
        return ((mesa_rc)NTP_ERROR_PARM);
#endif/* VTSS_SW_OPTION_IPV6 */
    default:
        return ((mesa_rc)NTP_ERROR_PARM);
    }
}

/* ntp error text */
const char *ntp_error_txt(ntp_error_t rc)
{
    const char *txt;

    switch (rc) {
    case NTP_ERROR_GEN:
        txt = "ntp generic error";
        break;
    case NTP_ERROR_PARM:
        txt = "ntp parameter error";
        break;
    default:
        txt = "ntp unknown error";
        break;
    }

    return txt;
}

/* Set ntp defaults */
void vtss_ntp_default_set(ntp_conf_t *conf)
{
    ntp_default_set(conf);
    return;
}

/* Get ntp configuration */
mesa_rc ntp_mgmt_conf_get(ntp_conf_t *conf)
{
    T_D("enter");

    NTP_CRIT_ENTER();
    *conf = ntp_global.ntp_conf;
    NTP_CRIT_EXIT();

    T_D("exit");
    return VTSS_RC_OK;
}

/* Set ntp configuration */
mesa_rc ntp_mgmt_conf_set(ntp_conf_t *conf)
{
    mesa_rc         rc      = VTSS_RC_OK;
    int             changed = 0;
    int             i;

    T_D("enter, mode: %ld", (long)conf->mode_enabled);

    /* check illegal parameter */
    if (conf->mode_enabled != NTP_MGMT_ENABLED && conf->mode_enabled != NTP_MGMT_DISABLED) {
        return ((mesa_rc)NTP_ERROR_PARM);
    }
    for (i = 0; i < VTSS_APPL_NTP_SERVER_MAX_COUNT; i++) {
        if (ntp_validate_server(&conf->server[i]) != VTSS_RC_OK) {
            return ((mesa_rc)NTP_ERROR_PARM);
        }
    }

    NTP_CRIT_ENTER();
    changed = ntp_conf_changed(&ntp_global.ntp_conf, conf);
    old_ntp_conf = ntp_global.ntp_conf;
    ntp_global.ntp_conf = *conf;
    NTP_CRIT_EXIT();

    if (changed) {
        /* Apply all configuration to switch */
        ntp_conf_set();
    }

    T_D("exit");
    return rc;
}

#if 0
static int ntp_dppm = NTP_DEFAULT_DRIFT;
static ulong ntp_updatime = 0;
static double ntp_offset = 0;
static int ntp_drift = 0;
static int ntp_first_dppm = 0;
static int ntp_max_dppm = 0;
static int ntp_min_dppm = 0;
static ulong ntp_step_count = 0;
static ulong ntp_step_time[STEP_ENTRY_NO];
static char ntp_per_server[16];
static char ntp_last_offset_in_range;
static double ntp_last_offest;
static int ntp_current_status;
static ntp_freq_data_t ntp_freq;
static unsigned char timer_reset_count = 0;

void vtss_ntp_timer_reset_counter(void)
{
    timer_reset_count++;
}


void vtss_ntp_timer_reset(void)
{
    ntp_timer_reset();
}

void vtss_ntp_timer_init(void)
{
    ntp_timer_init();
}


void vtss_ntp_last_sys_info(char in_range, double offset, int status)
{
    ntp_last_offset_in_range = in_range;
    ntp_last_offest = offset;
    ntp_current_status = status;
}

void vtss_ntp_freq_info(ulong mu, double curr_offset, double last_offset, double result_frequency)
{
    ntp_freq.mu = mu;
    ntp_freq.curr_offset = curr_offset;
    ntp_freq.last_offset = last_offset;
    ntp_freq.result_frequency = result_frequency;
}


void vtss_ntp_freq_get(ntp_freq_data_t *freq)
{
    freq->mu = ntp_freq.mu;
    freq->curr_offset = ntp_freq.curr_offset;
    freq->last_offset = ntp_freq.last_offset;
    freq->result_frequency = ntp_freq.result_frequency;
}

/* Set ntp defaults */
void vtss_ntp_default_set(ntp_conf_t *conf)
{
    ntp_default_set(conf);
    return;
}

/* Set ntp configuration */
void vtss_ntp_adj_time(double dppm, ulong updatime, double offset, char *ipstr)
{
    int     deltappm;
#if 0
    ulong   reload_value = hal_get_reload_value();
#endif

    deltappm = -((int)(dppm * 1e6) + ntp_drift);
    if (deltappm != ntp_dppm) {
    }

#if 0
    printf("hal_clock_set_adjtimer set IPaddr %s(%lu) %d PPM (%.6f) -- %x\n", ipstr, updatime, deltappm, offset, reload_value);
    printf("hal_clock_set_adjtimer set IPaddr %s(%lu) %d PPM (%.6f)\n", ipstr, updatime, deltappm, offset);
#endif


    ntp_dppm = deltappm;
    ntp_updatime = updatime;
    ntp_offset = offset;
    memset(ntp_per_server, 0, sizeof(ntp_per_server));
    strcpy(ntp_per_server, ipstr);

    if (ntp_first_dppm  == 0) {
        ntp_max_dppm = ntp_min_dppm = ntp_first_dppm = deltappm;
    }

    if (ntp_min_dppm > deltappm) {
        ntp_min_dppm = deltappm;
    }

    if (ntp_max_dppm < deltappm) {
        ntp_max_dppm = deltappm;
    }
}

extern u_long   current_time;       /* current time (s) */

void ntp_mgmt_sys_status_get(ntp_sys_status_t *status)
{
    NTP_CRIT_ENTER();
    status->currentime = current_time;
    status->currentppm = ntp_dppm;
    status->updatime = ntp_updatime;
    status->offset = ntp_offset;
    status->drift = ntp_drift;
    status->first_dppm = ntp_first_dppm;
    status->max_dppm = ntp_max_dppm;
    status->min_dppm = ntp_min_dppm;
    status->step_count = ntp_step_count;
    //status->step_time = ntp_step_time;
    memcpy(status->step_time, ntp_step_time, sizeof(status->step_time));
    memcpy(status->ip_string, ntp_per_server, sizeof(status->ip_string));
    status->timer_rest_count = timer_reset_count;
    status->last_offset_in_range = ntp_last_offset_in_range;
    status->last_offest = ntp_last_offest;
    status->current_status = ntp_current_status;
    NTP_CRIT_EXIT();
}


void vtss_ntp_step_count_set(unsigned long  count)
{
    static ulong  i = 0;

    ntp_step_count = count;
    ntp_step_time[i] = current_time;

    i = (i + 1) % STEP_ENTRY_NO;
}


void
vtss_ntp_server_info(unsigned long curr_time, char *ip_str, int flag,
                     int poll, int burst, unsigned long lastupdate,
                     unsigned long nextupdate, double offset)
{
    int i, avi_index = -1;

    for (i = 0; i < 5; i++) {
        if (memcmp(server_info[i].ip_string, ip_str,
                   sizeof(server_info[i].ip_string)) == 0) {

            server_info[i].curr_time = curr_time;
            server_info[i].flag = flag;
            server_info[i].poll_int = poll;
            server_info[i].burst = burst;
            server_info[i].lastupdate = lastupdate;
            server_info[i].nextupdate = nextupdate;
            server_info[i].offset = offset;
            break;
        } else if (avi_index == -1 && server_info[i].poll_int == 0) {
            avi_index = i;
        }
    }

    if (i == 5) {
        /* not found */
        memcpy(server_info[avi_index].ip_string, ip_str,
               sizeof(server_info[avi_index].ip_string));
        server_info[avi_index].curr_time = curr_time;
        server_info[avi_index].flag = flag;
        server_info[avi_index].poll_int = poll;
        server_info[avi_index].burst = burst;
        server_info[avi_index].lastupdate = lastupdate;
        server_info[avi_index].nextupdate = nextupdate;
    }
}

void ntp_mgmt_sys_server_get(ntp_server_status_t *status, ulong *curr_time, int num)
{
    NTP_CRIT_ENTER();
    *curr_time = current_time;
    memcpy(status, server_info, num * sizeof(ntp_server_status_t));
    NTP_CRIT_EXIT();
}
#endif

/****************************************************************************/
/*  Configuration                                                           */
/****************************************************************************/

/* Read/create ntp stack configuration */
static void ntp_conf_read_stack(BOOL create)
{
    int             changed;
    ntp_conf_t      *old_ntp_conf_p, new_ntp_conf;

    T_D("enter, create: %d", create);

    changed = 0;
    NTP_CRIT_ENTER();
    /* Use default values */
    ntp_default_set(&new_ntp_conf);

    old_ntp_conf_p = &ntp_global.ntp_conf;
    if (ntp_conf_changed(old_ntp_conf_p, &new_ntp_conf)) {
        changed = 1;
    }
    ntp_global.ntp_conf = new_ntp_conf;
    NTP_CRIT_EXIT();

    if (changed && create) {
        /* Apply all configuration to switch */
        ntp_conf_set();
    }

    T_D("exit");
}

/* Module start */
static void ntp_start(BOOL init)
{
    ntp_conf_t *conf_p;

    T_D("enter, init: %d", init);

    if (init) {
        /* Initialize ntp configuration */
        conf_p = &ntp_global.ntp_conf;
        ntp_default_set(conf_p);

        /* Create semaphore for critical regions */
        critd_init(&ntp_global.crit, "ntp", VTSS_MODULE_ID_NTP, CRITD_TYPE_MUTEX);

    }
    T_D("exit");
}

/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/
#ifdef VTSS_SW_OPTION_PRIVATE_MIB
/* Initialize private mib */
VTSS_PRE_DECLS void ntp_mib_init(void);
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_ntp_json_init(void);
#endif
extern "C" int ntp_icli_cmd_register();

/* Initialize module */
mesa_rc ntp_init(vtss_init_data_t *data)
{
    vtss_isid_t isid = data->isid;
#ifdef VTSS_SW_OPTION_ICFG
    mesa_rc     rc = VTSS_RC_OK;
#endif

    switch (data->cmd) {
    case INIT_CMD_INIT:
        T_D("INIT");
        ntp_start(1);
#ifdef VTSS_SW_OPTION_ICFG
        if ((rc = vtss_ntp_icfg_init()) != VTSS_RC_OK) {
            T_D("Calling vtss_ntp_icfg_init() failed rc = %s", error_txt(rc));
        }
#endif

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        /* Register private mib */
        ntp_mib_init();
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
        vtss_appl_ntp_json_init();
#endif

        ntp_icli_cmd_register();
        //////////////////////////////////////////////
        // init process
        //////////////////////////////////////////////
        vtss_process_ntpd.executable = "/usr/sbin/ntpd";

        // Use SIGKILL instead of SIGTERM to kill process
        vtss_process_ntpd.kill_policy(true);

        // Allow the first adjustment to be Big
        vtss_process_ntpd.arguments.push_back("-g");

        // Do not fork - we want to keep contact such that we can monitor it
        vtss_process_ntpd.arguments.push_back("-n");

        // Do not listen to virtual IPs.
        vtss_process_ntpd.arguments.push_back("-L");

        // Specify  the  name  and  path of the configuration file
        vtss_process_ntpd.arguments.push_back("-c");
        vtss_process_ntpd.arguments.push_back(NTPD_CONF_FILE);

        // Specify  the  name  and  path of the log file
        vtss_process_ntpd.arguments.push_back("-l");
        vtss_process_ntpd.arguments.push_back(NTPD_LOG_FILE);

        // Redirect stdout/stderr to trace system
        vtss_process_ntpd.trace_stdout_conf(VTSS_TRACE_MODULE_ID,
                                            VTSS_TRACE_GRP_DEFAULT,
                                            VTSS_TRACE_LVL_INFO);

        vtss_process_ntpd.trace_stderr_conf(VTSS_TRACE_MODULE_ID,
                                            VTSS_TRACE_GRP_DEFAULT,
                                            VTSS_TRACE_LVL_ERROR);
        break;

    case INIT_CMD_START:
        T_D("START");
        ntp_start(0);
        break;

    case INIT_CMD_CONF_DEF:
        T_D("CONF_DEF, isid: %d", isid);
        if (isid == VTSS_ISID_GLOBAL) {
            /* Reset stack configuration */
            ntp_conf_read_stack(1);
        }

        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        T_D("ICFG_LOADING_PRE");

        memset(&old_ntp_conf, 0, sizeof(old_ntp_conf));
        old_ntp_conf.mode_enabled = NTP_MGMT_INITIAL;

        /* Read stack and switch configuration */
        ntp_conf_read_stack(0);
        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}

/****************************************************************************/
/*  Public API                                                              */
/****************************************************************************/
#if 1
/**
 * \brief Get NTP Parameters
 *
 * To read current global parameters in NTP.
 *
 * \param conf [OUT] The NTP global configuration data
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_ntp_global_config_get(
    vtss_appl_ntp_global_config_t                       *const conf
)
{
    mesa_rc     rc;
    ntp_conf_t  ntpConf;

    /* check parameter */
    if (conf == NULL) {
        T_E("conf == NULL\n");
        return VTSS_RC_ERROR;
    }

    if ((rc = ntp_mgmt_conf_get(&ntpConf)) != VTSS_RC_OK) {
        T_E("ntp_mgmt_conf_get()rc=%d\n", rc);
        return VTSS_RC_ERROR;
    }

    /* pack output */
    conf->mode = (ntpConf.mode_enabled == NTP_MGMT_ENABLED) ? TRUE : FALSE;
    return VTSS_RC_OK;
}

/**
 * \brief Set NTP Parameters
 *
 * To modify current global parameters in NTP.
 *
 * \param conf [IN] The NTP global configuration data
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_ntp_global_config_set(
    const vtss_appl_ntp_global_config_t                 *const conf
)
{
    mesa_rc     rc;
    BOOL        original_mode;
    ntp_conf_t  ntpConf;


    /* check parameter */
    if (conf == NULL) {
        T_E("conf == NULL\n");
        return VTSS_RC_ERROR;
    }

    if ( (conf->mode != TRUE) &&
         (conf->mode != FALSE) ) {
        T_E("conf->mode = %d\n", conf->mode);
        return VTSS_RC_ERROR;
    }

    if ((rc = ntp_mgmt_conf_get(&ntpConf)) != VTSS_RC_OK) {
        T_E("ntp_mgmt_conf_get()rc=%d\n", rc);
        return VTSS_RC_ERROR;
    }

    original_mode = ntpConf.mode_enabled;

    if (conf->mode == TRUE) {
        ntpConf.mode_enabled = NTP_MGMT_ENABLED;
    } else {
        ntpConf.mode_enabled = NTP_MGMT_DISABLED;
    }

    if (ntpConf.mode_enabled != original_mode && (rc = ntp_mgmt_conf_set(&ntpConf)) != VTSS_RC_OK) {
        T_D("%s\n", error_txt(rc));
        return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}

/**
 * \brief Iterate function of NTP Server Configuration
 *
 * To get first and get next indexes.
 *
 * \param prev_serverIdx [IN]  previous server index number.
 * \param next_serverIdx [OUT] next server index number.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_ntp_server_config_itr(
    const u32                                   *const prev_serverIdx,
    u32                                         *const next_serverIdx
)
{
    return vtss::expose::snmp::IteratorComposeStaticRange<u32, 1, VTSS_APPL_NTP_SERVER_MAX_COUNT> (prev_serverIdx, next_serverIdx);
}


/**
 * \brief Get NTP Server Configuration
 *
 * To read configuration of the server in NTP.
 *
 * \param serverIdx  [IN]  Index number of the server.
 *
 * \param serverConf [OUT] The current configuration of the server
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_ntp_server_config_get(
    u32                                         serverIdx,
    vtss_appl_ntp_server_config_t               *const serverConf
)
{
    mesa_rc                 rc;
    ntp_conf_t              ntpConf;
    ntp_server_config_t     *pSrvConf;


    /* check parameter */
    if (serverConf == NULL) {
        T_E("serverConf == NULL\n");
        return VTSS_RC_ERROR;
    }

    /* server index range check */
    if ( (serverIdx == 0) ||
         (serverIdx > VTSS_APPL_NTP_SERVER_MAX_COUNT) ) {
        return VTSS_RC_ERROR;
    }

    if ((rc = ntp_mgmt_conf_get(&ntpConf)) != VTSS_RC_OK) {
        T_E("ntp_mgmt_conf_get()rc=%d\n", rc);
        return VTSS_RC_ERROR;
    }

    memset(serverConf, 0, sizeof(vtss_appl_ntp_server_config_t));

    /* server index in database is 0 base. */
    pSrvConf = &ntpConf.server[serverIdx - 1];
    if (pSrvConf->ip_type == NTP_IP_TYPE_IPV4) {
        if (strlen(pSrvConf->ip_host_string) == 0) {
            serverConf->address.type = VTSS_INET_ADDRESS_TYPE_NONE;
        } else if (misc_str_is_ipv4(pSrvConf->ip_host_string) == VTSS_RC_OK) {
            serverConf->address.type = VTSS_INET_ADDRESS_TYPE_IPV4;
            rc = mgmt_txt2ipv4(pSrvConf->ip_host_string, &serverConf->address.address.ipv4, NULL, 0);
            if ( rc != VTSS_RC_OK ) {
                T_W("[%d]IPv4 string error:rc=%d[%s]\n", serverIdx, rc, pSrvConf->ip_host_string);
                return VTSS_RC_ERROR;
            }
        } else {
            serverConf->address.type = VTSS_INET_ADDRESS_TYPE_DNS;
            strncpy(serverConf->address.address.domain_name.name, pSrvConf->ip_host_string,
                    sizeof(serverConf->address.address.domain_name.name) - 1);
        }
    } else {
#ifdef VTSS_SW_OPTION_IPV6
        serverConf->address.type = VTSS_INET_ADDRESS_TYPE_IPV6;
        serverConf->address.address.ipv6 = pSrvConf->ipv6_addr;
#else
        T_W("[%d]incompatible ip_type %d\n", serverIdx, pSrvConf->ip_type);
        return VTSS_RC_ERROR;
#endif
    }

    return VTSS_RC_OK;
}


/**
 * \brief Set NTP Server Configuration
 *
 * To modify configuration of the server in NTP.
 *
 * \param serverIdx  [IN] Index number of the server.
 *
 * \param serverConf [IN] The configuration set to the server
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_ntp_server_config_set(
    u32                                         serverIdx,
    const vtss_appl_ntp_server_config_t         *const serverConf
)
{
    mesa_rc                 rc;
    ntp_conf_t              ntpConf;
    ntp_server_config_t     *pSrvConf, *pSrvEnt;
    int                     i;
#ifdef VTSS_SW_OPTION_IPV6
    u16                     *tmp;
#endif

    /* check parameter */
    if (serverConf == NULL) {
        T_E("serverConf == NULL\n");
        return VTSS_RC_ERROR;
    }

    /* server index range check */
    if ( (serverIdx == 0) ||
         (serverIdx > VTSS_APPL_NTP_SERVER_MAX_COUNT) ) {
        return VTSS_RC_ERROR;
    }

    if ((rc = ntp_mgmt_conf_get(&ntpConf)) != VTSS_RC_OK) {
        T_E("ntp_mgmt_conf_get()rc=%d\n", rc);
        return VTSS_RC_ERROR;
    }

    /* server index in database is 0 base. */
    pSrvConf = &ntpConf.server[serverIdx - 1];
    memset(pSrvConf, 0, sizeof(ntp_server_config_t));

    switch (serverConf->address.type) {
    case VTSS_INET_ADDRESS_TYPE_NONE:
        T_D("Set Clear (%d)\n", serverIdx);
        break;
    case VTSS_INET_ADDRESS_TYPE_IPV4:
        /* configure IPv4 address*/
        pSrvConf->ip_type = NTP_IP_TYPE_IPV4;
        (void) misc_ipv4_txt(serverConf->address.address.ipv4, pSrvConf->ip_host_string);
        T_D("Set IPv4 (%d, %s)\n", serverIdx, pSrvConf->ip_host_string);
        break;
    case VTSS_INET_ADDRESS_TYPE_DNS:
        /* configure domain name */
        pSrvConf->ip_type = NTP_IP_TYPE_IPV4;
        if (strlen(serverConf->address.address.domain_name.name) > VTSS_APPL_SYSUTIL_INPUT_DOMAIN_NAME_LEN) {
            T_D("DNS name length %d exceeds maximum %d\n",
                (int)strlen(serverConf->address.address.domain_name.name), VTSS_APPL_SYSUTIL_INPUT_DOMAIN_NAME_LEN);
            return VTSS_RC_ERROR;
        }
        strncpy(pSrvConf->ip_host_string, serverConf->address.address.domain_name.name,
                sizeof(pSrvConf->ip_host_string) - 1);
        T_D("Set DNS (%d, %s)\n", serverIdx, pSrvConf->ip_host_string);
        break;
    case VTSS_INET_ADDRESS_TYPE_IPV6:
#ifdef VTSS_SW_OPTION_IPV6
        /* configure as IPv6 address */
        pSrvConf->ip_type = NTP_IP_TYPE_IPV6;
        pSrvConf->ipv6_addr = serverConf->address.address.ipv6;

        tmp = (u16 *)&pSrvConf->ipv6_addr;
        T_D("Set IPv6 (%d, %x:%x:%x:%x:%x:%x:%x:%x)\n",
            serverIdx, tmp[0], tmp[1], tmp[2], tmp[3], tmp[4], tmp[5], tmp[6], tmp[7]);
        break;
#else
        T_D("IPv6 not supported.\n");
        return VTSS_RC_ERROR;
#endif
    default:
        return VTSS_RC_ERROR;
    }

    /* check duplicates */
    if (serverConf->address.type != VTSS_INET_ADDRESS_TYPE_NONE) {
        pSrvEnt = &ntpConf.server[0];
        for (i = 0; i < VTSS_APPL_NTP_SERVER_MAX_COUNT; i++, pSrvEnt++) {
            if (pSrvConf == pSrvEnt) {
                continue;
            }

            if (pSrvConf->ip_type == pSrvEnt->ip_type) {
                switch (pSrvConf->ip_type) {
                case NTP_IP_TYPE_IPV4:
                    if (!strcmp(pSrvConf->ip_host_string, pSrvEnt->ip_host_string)) {
                        T_D("duplicate with index %d, return.\n", (i + 1));
                        return VTSS_RC_ERROR;
                    }
                    break;
#ifdef VTSS_SW_OPTION_IPV6
                case NTP_IP_TYPE_IPV6:
                    if (!memcmp(&pSrvConf->ipv6_addr, &pSrvEnt->ipv6_addr, sizeof(mesa_ipv6_t))) {
                        T_D("duplicate with index %d, return.\n", (i + 1));
                        return VTSS_RC_ERROR;
                    }
                    break;
#endif /* VTSS_SW_OPTION_IPV6 */
                default:
                    break;
                }
            }
        }/* end for */
    }

    if ((rc = ntp_mgmt_conf_set(&ntpConf)) != VTSS_RC_OK) {
        T_D("%s, return.\n", error_txt(rc));
        return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}
#endif

