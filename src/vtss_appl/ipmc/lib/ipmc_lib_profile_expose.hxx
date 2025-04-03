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

#include <vtss/appl/ipmc_lib.h>

#ifndef _IPMC_LIB_PROFILE_EXPOSE_HXX_
#define _IPMC_LIB_PROFILE_EXPOSE_HXX_

// Wrapper functions needed because the serializer exposes IPv4 range tables
// separately from IPv6 range tables.

typedef struct {
    vtss_appl_ipmc_lib_profile_range_conf_t conf;
} ipmc_lib_expose_profile_range_conf_ipv4_t;

typedef struct {
    vtss_appl_ipmc_lib_profile_range_conf_t conf;
} ipmc_lib_expose_profile_range_conf_ipv6_t;

inline mesa_rc ipmc_lib_expose_ipmc_profile_range_ipv4_itr(const vtss_appl_ipmc_lib_profile_range_key_t *in, vtss_appl_ipmc_lib_profile_range_key_t *out)
{
    return vtss_appl_ipmc_lib_profile_range_itr(in, out, VTSS_APPL_IPMC_LIB_PROFILE_RANGE_TYPE_IPV4);
}

inline mesa_rc ipmc_lib_expose_ipmc_profile_range_ipv6_itr(const vtss_appl_ipmc_lib_profile_range_key_t *in, vtss_appl_ipmc_lib_profile_range_key_t *out)
{
    return vtss_appl_ipmc_lib_profile_range_itr(in, out, VTSS_APPL_IPMC_LIB_PROFILE_RANGE_TYPE_IPV6);
}

inline mesa_rc ipmc_lib_expose_ipmc_profile_range_ipv4_conf_get(const vtss_appl_ipmc_lib_profile_range_key_t *key, ipmc_lib_expose_profile_range_conf_ipv4_t *conf)
{
    return vtss_appl_ipmc_lib_profile_range_conf_get(key, &conf->conf);
}

inline mesa_rc ipmc_lib_expose_ipmc_profile_range_ipv6_conf_get(const vtss_appl_ipmc_lib_profile_range_key_t *key, ipmc_lib_expose_profile_range_conf_ipv6_t *conf)
{
    return vtss_appl_ipmc_lib_profile_range_conf_get(key, &conf->conf);
}

inline mesa_rc ipmc_lib_expose_ipmc_profile_range_ipv4_conf_set(const vtss_appl_ipmc_lib_profile_range_key_t *key, const ipmc_lib_expose_profile_range_conf_ipv4_t *conf)
{
    vtss_appl_ipmc_lib_profile_range_conf_t real_conf = conf->conf;
    real_conf.start.is_ipv4 = true;
    real_conf.end.is_ipv4   = true;
    return vtss_appl_ipmc_lib_profile_range_conf_set(key, &real_conf);
}

inline mesa_rc ipmc_lib_expose_ipmc_profile_range_ipv6_conf_set(const vtss_appl_ipmc_lib_profile_range_key_t *key, const ipmc_lib_expose_profile_range_conf_ipv6_t *conf)
{
    vtss_appl_ipmc_lib_profile_range_conf_t real_conf = conf->conf;
    real_conf.start.is_ipv4 = false;
    real_conf.end.is_ipv4   = false;
    return vtss_appl_ipmc_lib_profile_range_conf_set(key, &real_conf);
}

typedef struct {
    vtss_appl_ipmc_lib_profile_rule_conf_t conf;
    vtss_appl_ipmc_lib_profile_range_key_t next_range; // Controls insertion point.
} ipmc_lib_expose_profile_rule_conf_t;

inline mesa_rc ipmc_lib_expose_profile_rule_conf_get(const vtss_appl_ipmc_lib_profile_key_t *profile_key, const vtss_appl_ipmc_lib_profile_range_key_t *range_key, ipmc_lib_expose_profile_rule_conf_t *rule_conf)
{
    // Gotta search for the rule coming after this one to get the next_range.
    vtss_appl_ipmc_lib_profile_key_t       prf_key = *profile_key;
    vtss_appl_ipmc_lib_profile_range_key_t rng_key = *range_key;

    // Get current configuration.
    VTSS_RC(vtss_appl_ipmc_lib_profile_rule_conf_get(profile_key, range_key, &rule_conf->conf));

    // Fill out the next_range member
    if (vtss_appl_ipmc_lib_profile_rule_itr(&prf_key, &prf_key, &rng_key, &rng_key, true /* only iterate over this profile */) == VTSS_RC_OK) {
        rule_conf->next_range = rng_key;
    } else {
        // No next.
        rule_conf->next_range.name[0] = '\0';
    }

    return VTSS_RC_OK;
}

inline mesa_rc ipmc_lib_expose_profile_rule_conf_set(const vtss_appl_ipmc_lib_profile_key_t *profile_key, const vtss_appl_ipmc_lib_profile_range_key_t *range_key, const ipmc_lib_expose_profile_rule_conf_t *rule_conf)
{
    // We insert the rule last if we get an empty next range name. We insert it
    // last by specifying a nullptr, because in the following call an empty next
    // range name means: leave the rule where it is in the list.
    return vtss_appl_ipmc_lib_profile_rule_conf_set(profile_key, range_key, &rule_conf->conf, strlen(rule_conf->next_range.name) ? &rule_conf->next_range : nullptr);
}

inline mesa_rc ipmc_lib_expose_profile_rule_itr(const vtss_appl_ipmc_lib_profile_key_t *profile_prev, vtss_appl_ipmc_lib_profile_key_t *profile_next, const vtss_appl_ipmc_lib_profile_range_key_t *range_prev, vtss_appl_ipmc_lib_profile_range_key_t *range_next)
{
    // Gotta wrap this call, because it has an optional parameter, which is
    // false by default, indicating to iterate across all profiles.
    return vtss_appl_ipmc_lib_profile_rule_itr(profile_prev, profile_next, range_prev, range_next);
}

#endif // _IPMC_LIB_PROFILE_EXPOSE_HXX_

