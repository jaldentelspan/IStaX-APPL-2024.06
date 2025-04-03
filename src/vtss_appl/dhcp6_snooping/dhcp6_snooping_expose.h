/*

 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _DHCP6_EXPOSE_H_
#define _DHCP6_EXPOSE_H_

#include <vtss/basics/algorithm.hxx>
#include <vtss/basics/map.hxx>
#include "vtss/appl/dhcp6_snooping.h"

#include "vtss/basics/types.hxx"
#include "vtss/basics/expose.hxx"

#include "dhcp6_snooping_frame.h"

typedef enum {
    DHCP6_SNOOPING_ADDRESS_STATE_NOSTATE,    // Client is not in any particular state
    DHCP6_SNOOPING_ADDRESS_STATE_SOLICIT,    // Client is soliciting for a DHCP server
    DHCP6_SNOOPING_ADDRESS_STATE_REQUEST,    // Client is requesting an address
    DHCP6_SNOOPING_ADDRESS_STATE_CONFIRM,    // Client is trying to confirm current address
    DHCP6_SNOOPING_ADDRESS_STATE_REBIND,     // Client is trying to rebind current address
    DHCP6_SNOOPING_ADDRESS_STATE_ASSIGNED,   // Client has been assigned an address
    DHCP6_SNOOPING_ADDRESS_STATE_RELEASING,  // Client is releasing an address
} dhcp6_address_state_t;

/**
 * DHCP Unique Identifier (DUID) class. An instance of this class uniquely
 * identifies either a DHCP client or a DHCP server.
 *
 * This struct is only for internal usage. It may encapsulate the public DUID
 * type vtss_appl_dhcp6_snooping_duid_t for various operations.
 */
struct dhcp_duid_t : vtss_appl_dhcp6_snooping_duid_t {
public:
    dhcp_duid_t()
    {
        clear();
    }
    virtual ~dhcp_duid_t() {}

    dhcp_duid_t(const uint8_t *const _data, uint32_t _length)
    {
        assign(data, length);
    }

    dhcp_duid_t(const vtss_appl_dhcp6_snooping_duid_t &base)
    {
        assign(base.data, base.length);
    }

    dhcp_duid_t(const vtss_appl_dhcp6_snooping_duid_t *base)
    {
        assign(base->data, base->length);
    }

    void clear()
    {
        memset(data, 0, sizeof(data));
        length = 0;
    }

    void assign(const uint8_t *const _data, uint32_t _length)
    {
        clear();

        length = vtss::min(_length, DHCP6_DUID_MAX_SIZE);
        memcpy(data, _data, length);
    }

    const std::string to_string() const
    {
        char buffer[2 * length + 1];
        for (int i = 0; i < length; i++) {
            sprintf(buffer + 2 * i, "%02X", data[i]);
        }
        buffer[2 * length] = 0;
        return std::string(buffer);
    }

    bool operator!= (const dhcp_duid_t &other_duid) const;

    bool operator< (const dhcp_duid_t &other_duid) const;
};


struct client_interface_info_t : vtss_appl_dhcp6_snooping_assigned_ip_t {
    uint32_t                    transaction_id;         // DHCP transaction ID for this entry
    dhcp6_address_state_t       address_state;          // Address assignment state of this entry
    bool                        rapid_commit;           // Client asked for rapid commit (2-step exchange)

};

typedef vtss::Map<vtss_appl_dhcp6_snooping_iaid_t, client_interface_info_t> interface_address_map_t;

/**
 * \brief DHCPv6 extended client IP information.
 */
struct registered_clients_info_t : vtss_appl_dhcp6_snooping_client_info_t {
    mesa_port_no_t              port_no;                // Internal port number for sake of convenience
    mesa_ipv6_t                 link_local_ip_address;  // Client link-local IPv6 address
    time_t                      last_access_time;       // The last time this client was accessed by a DHCP message
    interface_address_map_t     if_map;                 // Map of assigned addresses, indexed by interface IAID

    registered_clients_info_t() {}
    virtual ~registered_clients_info_t()
    {
        if_map.clear();
    }

    /*
     * Return entry idle time in seconds
     */
    time_t get_idle_time(time_t curr_time);

    /*
     * Return expiry time in seconds
     */
    uint32_t get_curr_expiry_time(const client_interface_info_t &item) const;

    /*
     * Return true if at least one address entry has an assigned address
     */
    bool has_assigned_addresses();

    /*
     * Return true if at least one address entry has an pending address request
     */
    bool has_pending_requests();
};

bool compare_registered_clients_info(const dhcp_duid_t &a,
                                     vtss::Pair<const dhcp_duid_t, registered_clients_info_t> &b);

bool compare_assigned_ip(const vtss_appl_dhcp6_snooping_iaid_t &a,
                         vtss::Pair<const vtss_appl_dhcp6_snooping_iaid_t, client_interface_info_t> &b);

/******************************************************************************/
// dhcp6_snooping_expose_client_info_get()
/******************************************************************************/
static inline mesa_rc dhcp6_snooping_expose_client_info_get(const vtss_appl_dhcp6_snooping_duid_t *duid, vtss_appl_dhcp6_snooping_client_info_t *client_info)
{
    if (!duid) {
        return VTSS_RC_ERROR;
    }

    return vtss_appl_dhcp6_snooping_client_info_get(*duid, client_info);
}

/******************************************************************************/
// dhcp6_snooping_expose_assigned ip_get()
/******************************************************************************/
static inline mesa_rc dhcp6_snooping_expose_assigned_ip_get(const vtss_appl_dhcp6_snooping_duid_t *duid, const vtss_appl_dhcp6_snooping_iaid_t *iaid, vtss_appl_dhcp6_snooping_assigned_ip_t *address_info)
{
    if (!duid || !iaid) {
        return VTSS_RC_ERROR;
    }

    return vtss_appl_dhcp6_snooping_assigned_ip_get(*duid, *iaid, address_info);
}

#endif /* _DHCP6_EXPOSE_H_ */

