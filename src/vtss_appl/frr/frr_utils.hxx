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

#ifndef _FRR_UTILS_HXX_
#define _FRR_UTILS_HXX_

#include <vtss/appl/ip.h>
#include <vtss/appl/interface.h>
#include <vtss/appl/types.h>
#include <vtss/basics/parse_group.hxx>
#include <vtss/basics/parser_impl.hxx>
#include <vtss/basics/string.hxx>
#include <vtss/basics/time.hxx>
#include <string>
#include "frr.hxx"

#define VTSS_IFINDEX_CONVERT_OS_IF_NAME_FAILED_ (VTSS_APPL_VLAN_ID_MAX + 1 + 9000000)
#define VTSS_IFINDEX_CONVERT_OS_IF_ID_FAILED_   (VTSS_APPL_VLAN_ID_MAX + 1 + 3000000)
#define VTSS_IFINDEX_CONVERT_VLINK_FAILED_      (1 + 2000000)
#define VTSS_IFINDEX_CONVERT_VLAN_FAILED_       (1000000)

/******************************************************************************/
// frr_util_secret_key_cryptography()
//
// Encrypt the plain text of secret key with AES256-like cryptography, or
// decrypt the encrypted hex string of the secret key to plain text.
//
// \param is_encrypt [IN]  Set 'true' to encrypted the data, set 'false' to decrypted it.
// \param input      [IN]  Input data, it is plain text or encrypted data based on 'is_encrypt'.
// \param output_len [IN]  The output data length.
// \param output     [OUT] Output data, it is either plain text or the encrypted data depending on 'is_encrypt' (includes terminating null).
//
// Return VTSS_RC_OK when encryption successful, error code otherwise.
/******************************************************************************/
mesa_rc frr_util_secret_key_cryptography(const bool is_encrypt, const char *const input, const uint32_t output_len, char *const output);

vtss::FrrRes<std::string> frr_util_os_ifname_get(const vtss_ifindex_t &ifindex);

// Get variable values and assign to parameters.
// E.g.:
// "area     0.0.0.1    range     1.2.3.0/24 cost     3"
//    |         |         |           |        |      |
//    V         V         V           V        V      V
//  area_lit <area_val> range_lit <net_val>  cost_lit <cost_val>
//
// frr_util_group_spaces(line, {&area_lit, &area_val, &range_lit, &net_val, &cost_lit, &cost_val})
//
bool frr_util_group_spaces(const vtss::str &line, std::initializer_list<vtss::parser::ParserBase *> args, std::initializer_list<vtss::parser::ParserBase *> optional = {});

/******************************************************************************/
// FrrUtilsConfStreamParserCB
/******************************************************************************/
struct FrrUtilsConfStreamParserCB {
    virtual void root(const vtss::str &line)
    {
    }

    virtual void key_chain(const std::string &keychainname, const vtss::str &line)
    {
    }

    virtual void key_chain_key_id(const std::string &keychainname, const std::string &keyid, const vtss::str &line)
    {
    }

    virtual void interface(const std::string &ifname, const vtss::str &line)
    {
    }

    virtual void router(const std::string &name, const vtss::str &line)
    {
    }

    virtual ~FrrUtilsConfStreamParserCB() = default;
};

/******************************************************************************/
// frr_util_conf_parser()
/******************************************************************************/
void frr_util_conf_parser(std::string conf, FrrUtilsConfStreamParserCB &cb);

/******************************************************************************/
// FrrUtilsEatAll
/******************************************************************************/
struct FrrUtilsEatAll {
    bool operator()(char c)
    {
        return true;
    }
};

/******************************************************************************/
// FrrUtilsGetWord
/******************************************************************************/
struct FrrUtilsGetWord {
    bool operator()(char c)
    {
        if (c != ' ') {
            return true;
        } else {
            return false;
        }
    }
};

/******************************************************************************/
// frr_util_time_to_seconds()
/******************************************************************************/
vtss::seconds frr_util_time_to_seconds(const std::string &up_time);

/******************************************************************************/
// frr_util_ifindex_valid()
// Check if the ifindex is valid for FRR or not.
/******************************************************************************/
bool frr_util_ifindex_valid(vtss_ifindex_t vtss_ifindex);

vtss_appl_ip_route_protocol_t frr_util_route_protocol_from_str(const std::string &val);

#endif // _FRR_UTILS_HXX_

