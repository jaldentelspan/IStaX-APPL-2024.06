// Many comments in this file is from RFC2131 and RFC2132
// This is the copyright informations from those documents:
//         "Distribution of this memo is unlimited."

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

#ifndef __DHCP_CLIENT_HXX__
#define __DHCP_CLIENT_HXX__

extern "C" {
//#include "main.h"
#include "main_types.h"
}
#include <string.h>
#include <stdlib.h>
#include "dhcp_client_api.h"
#include "ip_utils.hxx"
#include "dhcp_frame.hxx"
#include "frame_utils.hxx"
#include "vtss/basics/optional.hxx"
#include "vtss/basics/stream.hxx"
#include "vtss/basics/notifications.hxx"

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_DHCP_CLIENT
#include "vtss/basics/trace.hxx"

/* Declare a buffer with size 256 for DHCP client */
#define DHCPC_BUF256_DECLARE()        \
    char buf[256] = {};               \
    size_t buf_size = 0;              \
    size_t buf_free_size = 256;

/* The point reference of buffer   */
#define DHCPC_BUF256_PTR        (&buf[buf_size])

/* The maximum free size of buffer */
#define DHCPC_BUF256_MAX        (buf_free_size)

/* The used size of buffer         */
#define DHCPC_BUF256_USED_SIZE  (buf_size)

/* Write a single byte into buffer */
#define DHCPC_BUF256_PUSH_BYTE(val) \
    if (buf_free_size) {            \
        buf[buf_size] = val;        \
        buf_size ++;                \
        buf_free_size --;           \
    }

/* Write data into the buffer first then call this difinition for commitment */
#define DHCPC_BUF256_PUSH_BYTES_COMMIT(s)       \
    {                                           \
        if (buf_free_size) {                    \
            size_t cnt = s;                     \
            VTSS_ASSERT(buf_free_size >= cnt);  \
            buf_size += cnt;                    \
            buf_free_size -= cnt;               \
        }                                       \
    }

/*
 * From rfc2131 page 35
 * Removed reboot logic
 *                                        -------
 *                                       |       |<-------------------+
 *                 +-------------------->| INIT  |                    |
 *                 |         +---------->|       |<---+               |
 *                 |         |            -------     |               |
 *              DHCPNAK/     |               |                        |
 *           Discard offer   |      -/Send DHCPDISCOVER               |
 *                 |         |               |                        |
 *                 |      DHCPACK            v        |               |
 *                 |   (not accept.)/   -----------   |               |
 *                 |  Send DHCPDECLINE |           |                  |
 *                 |         |         | SELECTING |<----+            |
 *                 |        /          |           |     |DHCPOFFER/  |
 *                 |       /            -----------   |  |Collect     |
 *                 |      /                  |   |       |  replies   |
 *                 |     /  +----------------+   +-------+            |
 *                 |    |   v   Select offer/                         |
 *                ------------  send DHCPREQUEST      |               |
 *        +----->|            |             DHCPNAK, Lease expired/   |
 *        |      | REQUESTING |                  Halt network         |
 *    DHCPOFFER/ |            |                       |               |
 *    Discard     ------------                        |               |
 *        |        |        |                   -----------           |
 *        +--------+     DHCPACK/              |           |          |
 *                   Record lease, set    -----| REBINDING |          |
 *                     timers T1, T2     /     |           |          |
 *                          |        DHCPACK/   -----------           |
 *                          v     Record lease, set   ^               |
 *                       -------      /timers T1,T2   |               |
 *               +----->|       |<---+                |               |
 *               |      | BOUND |<---+                |               |
 *  DHCPOFFER, DHCPACK, |       |    |            T2 expires/   DHCPNAK/
 *   DHCPNAK/Discard     -------     |             Broadcast  Halt network
 *               |       | |         |            DHCPREQUEST         |
 *               +-------+ |        DHCPACK/          |               |
 *                    T1 expires/   Record lease, set |               |
 *                 Send DHCPREQUEST timers T1, T2     |               |
 *                 to leasing server |                |               |
 *                         |   ----------             |               |
 *                         |  |          |------------+               |
 *                         +->| RENEWING |                            |
 *                            |          |----------------------------+
 *                             ----------
 */

namespace vtss {
namespace dhcp {

size_t client_id_if_mac_get(char *buf, size_t max, vtss_ifindex_t ifidx);
size_t client_id_hex_str_convert(char *buf, size_t max, const char *hex_str);
size_t client_def_hostname_get(char *buf, int max);
size_t client_vendor_class_identifier(char *buf, int max);


struct OfferList {
    size_t valid_offers = 0;
    ConfPacket list[VTSS_DHCP_MAX_OFFERS];
};

/* DHCP option hardware type
 *
 * RFC 2131 Dynamic Host Configuration Protocol [Page 10]
 * -----------------------------------------------------
 *      FIELD      OCTETS       DESCRIPTION
 *      -----      ------       -----------
 *      op            1         Message op code / message type.
 *                              1 = BOOTREQUEST, 2 = BOOTREPLY
 *      htype         1         Hardware address type, see ARP section in
 *                              "Assigned Numbers" RFC; e.g., '1' = 10mb
 *                              ethernet.
 *      hlen          1         Hardware address length (e.g.  '6' for 10mb
 *                              ethernet).
 *
 * RFC 1700 Assigned Numbers [Page 163-164]
 * ----------------------------------------
 *      Number Hardware Type (hrd)                           References
 *      ------ -----------------------------------           ----------
 *           1 Ethernet (10Mb)                                    [JBP]
 *           2 Experimental Ethernet (3Mb)                        [JBP]
 *           3 Amateur Radio AX.25                                [PXK]
 *           4 Proteon ProNET Token Ring                          [JBP]
 *           5 Chaos                                              [GXP]
 *           6 IEEE 802 Networks                                  [JBP]
 *           7 ARCNET                                             [JBP]
 *           8 Hyperchannel                                       [JBP]
 *           9 Lanstar                                             [TU]
 *          10 Autonet Short Address                             [MXB1]
 *          11 LocalTalk                                         [JKR1]
 *          12 LocalNet (IBM PCNet or SYTEK LocalNET)             [JXM]
 *          13 Ultra link                                        [RXD2]
 *          14 SMDS                                              [GXC1]
 *          15 Frame Relay                                        [AGM]
 *          16 Asynchronous Transmission Mode (ATM)              [JXB2]
 *          17 HDLC                                               [JBP]
 *          18 Fibre Channel                            [Yakov Rekhter]
 *          19 Asynchronous Transmission Mode (ATM)      [Mark Laubach]
 *          20 Serial Line                                        [JBP]
 *          21 Asynchronous Transmission Mode (ATM)              [MXB1]
 *
 * RFC 2132 DHCP Options and BOOTP Vendor Extensions section 9.14.
 * ---------------------------------------------------------------
 *      Client-identifier [Page 30]
 *      The client identifier MAY consist of type-value pairs similar to the
 *      'htype'/'chaddr' fields defined in [3]. For instance, it MAY consist
 *      of a hardware type and hardware address. In this case the type field
 *      SHOULD be one of the ARP hardware types defined in STD2 [22].  A
 *      hardware type of 0 (zero) should be used when the value field
 *      contains an identifier other than a hardware address (e.g. a fully
 *      qualified domain name).
 */
enum OptionHtype {
    OPTIONHTYPE_NOT_HW_ADDR = 0x0,
    OPTIONHTYPE_ETHERNET = 0x1
};

template <typename L>
struct ScopeLock {
    ScopeLock(L &l) : l_(l) { l_.lock(); }
    ~ScopeLock() { l_.unlock(); }

  private:
    L &l_;
};

// Copy selected options from a DHCP frame (including native fields)
template <typename LL>
void copy_dhcp_options(const DhcpFrame<LL> &f, const Mac_t &src_mac,
                       ConfPacket *copy) {
    {  // derive and copy IP address
        u32 ip_ = f.yiaddr().as_int();
        u32 prefix;

        Option::SubnetMask o;
        if (f.option_get(o)) {
            VTSS_TRACE(INFO) << "  add mask";
            (void)vtss_conv_ipv4mask_to_prefix(o.ip().as_int(), &prefix);
        } else {
            VTSS_TRACE(INFO) << "  derive mask from IP class";
            prefix = vtss_ipv4_addr_to_prefix(ip_);
        }

        VTSS_TRACE(INFO) << "  add IP";
        copy->ip.as_api_type().address = ip_;
        copy->ip.as_api_type().prefix_size = prefix;
    }

    // copy mac address
    src_mac.copy_to(copy->server_mac.as_api_type().addr);

    // copy xid
    copy->xid = f.xid();

    {  // server identifier
        Option::ServerIdentifier op_server_identifier;
        if (f.option_get(op_server_identifier)) {
            copy->server_ip = op_server_identifier.ip().as_int();
            VTSS_TRACE(INFO) << "  add server identifier";
        } else {
            VTSS_TRACE(INFO) << "  no server identifier";
        }
    }

    {  // extract default route
        Option::RouterOption o;
        if (f.option_get(o)) {
            copy->default_gateway = o.ip().as_int();
            VTSS_TRACE(INFO) << "  add gateway";
        } else {
            VTSS_TRACE(INFO) << "  no gateway";
        }
    }

    {  // extract dns server
        Option::DnsServer o;
        if (f.option_get(o)) {
            copy->domain_name_server = o.ip().as_int();
            VTSS_TRACE(INFO) << "  add domain name server";
        } else {
            VTSS_TRACE(INFO) << "  no domain name server";
        }
    }

    {  //  vendor specific information
        Option::VendorSpecificInformation o;
        if (f.option_get(o)) {
            VTSS_TRACE(INFO) << "  add vendor specific information";
            copy->vendor_specific_information = str((char *)o.data(), o.size());
        } else {
            VTSS_TRACE(INFO) << "  no vendor specific information";
        }
    }

    {  //  Domain Name
        Option::DomainName o;
        if (f.option_get(o)) {
            VTSS_TRACE(INFO) << "  add domain name information";
            copy->domain_name = str((char *)o.data(), o.size());
        }
        else {
            VTSS_TRACE(INFO) << "  no domain name information";
        }
    }

    {  //  boot file name
        Option::BootfileName o;
        if (f.option_get(o)) {
            VTSS_TRACE(INFO) << "  add boot file name";
            copy->boot_file_name = str((char *)o.data(), o.size());
        } else {
            VTSS_TRACE(INFO) << "  no boot file name";
        }
    }
}

template <typename FrameTxService, typename TimerService, typename Lock>
struct Client : public notifications::EventHandler {
    // LOCKING: This class expected that the lock reference provided in the
    // constructor is allready locked, before a menber function is called. With
    // exception of the execute(Timer *t) function.

    typedef typename TimerService::clock_t Clock;
    typedef typename FrameTxService::template UdpStackType<DhcpFrame>::T
            FrameStack;

    typedef Optional<ConfPacket> AckConfPacket;

    enum { MAX_DHCP_MESSAGE_SIZE = 1024 };

    // Constructor and destructor
    Client(FrameTxService &tx, TimerService &ts, Lock &lock, vtss_ifindex_t ifidx,
           const vtss_appl_ip_dhcp4c_param_t &params);
    ~Client();

    // This is the public interface which may be used to control the dhcp client
    mesa_rc start();
    mesa_rc stop();
    mesa_rc if_down();
    mesa_rc if_up();
    mesa_rc release();
    mesa_rc decline();
    mesa_rc bind();
    mesa_rc fallback();
    vtss_ifindex_t ifindex() const { return ifidx_; }
    void params_set(const vtss_appl_ip_dhcp4c_param_t *const params) {
        params_ = *params;
    }

    // Accept one the the received offers
    bool accept(unsigned idx);

    // Access to internal state
    mesa_rc offers(size_t max_offers, size_t *valid_ovvers,
                   ConfPacket *list) const;
    AckConfPacket ack_conf() const {
        VTSS_TRACE(INFO) << "get";
        return ack_conf_.get();
    }
    AckConfPacket ack_conf(notifications::Event &t) {
        VTSS_TRACE(INFO) << "Attach " << (void *)&t;
        return ack_conf_.get(t);
    }
    vtss_appl_ip_dhcp4c_state_t state() const { return state_; }
    vtss_appl_ip_if_status_dhcp4c_t status() const;

    // The dhcp state machine is implemented in frame_event and execute(Timer
    // *).
    // The dhcp clients assumes that the integrator injects DHCP frames in here.
    template <typename LL>
    void frame_event(const DhcpFrame<LL> &f, const Mac_t src_mac);

    // The second half of the dhcp state machine. Events requested by the dhcp
    // client will be delivered through this interface.
    // WARNING, this is a async call, and must be locked inside this class
    void execute(notifications::Timer *t);

  private:
    template <typename LL>
    void latch_settings(const DhcpFrame<LL> &f, const Mac_t &src_mac);

    // Reset all state, arm the network halt timer, and start over when it fires
    void timed_retry();

    // clear all state, and cancel all timers
    void clear_state();

    void rearm_network_halt_timer();

    // Reset all state, goto to selection and send a new discovery
    void start_over();

    // validate and set the timers  timer_renew(T1), timer_rebind(T2) and
    // timer_ip_lease
    template <typename LL>
    bool latch_timers(const DhcpFrame<LL> &f);

    // Add options to the dhcp frames
    void add_client_identifier_and_hostname(FrameStack &f);
    void add_client_identifier(FrameStack &f);
    void add_hostname(FrameStack &f);
    void add_parameter_request_list(FrameStack &f);
    void add_vendor_class_identifier(FrameStack &f);

    // Build and send a discovery frame
    bool send_discover();

    // Build and send a request frame
    bool send_request();

    // Build and send a broadcast request frame
    bool send_request_broadcast();

    // Build and send a (renew) request frame
    bool send_renew();

    // Build and send a release frame
    bool send_release();

    // Build and send a decline frame
    bool send_decline();

    // Helper functions to emit the frames on the wire
    bool send_unicast(FrameStack &f);
    bool send_broadcast(FrameStack &f);

    bool update_xid();
    void sample_time();

    // The actually DHCP client state
    vtss_appl_ip_dhcp4c_state_t state_;

    // Policy services
    FrameTxService &tx_service;
    TimerService &timer_service;
    Lock lock_;

    // The four times used to drive the DHCP state machine
    notifications::Timer timer_network_halt;
    notifications::Timer timer_ip_lease;
    notifications::Timer timer_renew;   // T1
    notifications::Timer timer_rebind;  // T2

    // The time configurations is set in the ACK frame. When this frame is
    // received we validate and latch the information here.
    seconds ip_address_lease_time;
    seconds renewal_time_value;
    seconds rebinding_time_value;

    // Latched when offer is requested
    OfferList offer_list;
    IPv4_t yiaddr, server_identifier;
    Mac_t server_mac;
    Buffer boot_file_name;
    Buffer vendor_specific_information;

    // Latch the entire ack frame to provide options
    notifications::Subject<AckConfPacket> ack_conf_;

    // The vlan we operate on. Can not be changed!
    const vtss_ifindex_t ifidx_;

    // Parameters used to adjust the dhcp options
    vtss_appl_ip_dhcp4c_param_t params_;

    // session id
    u32 xid_;
    u32 xid_seed_;

    u32 timer_network_halt_retries;

    typename TimerService::clock_t::time_point secs_;
};

// IMPLEMENTATION ------------------------------------------------------------
template <typename FS, typename TS, typename Lock>
Client<FS, TS, Lock>::Client(FS &tx, TS &ts, Lock &lock, vtss_ifindex_t ifidx,
                             const vtss_appl_ip_dhcp4c_param_t &params)
    : notifications::EventHandler(&ts),
      state_(VTSS_APPL_IP_DHCP4C_STATE_STOPPED),
      tx_service(tx),
      timer_service(ts),
      lock_(lock),
      timer_network_halt(this),
      timer_ip_lease(this),
      timer_renew(this),
      timer_rebind(this),
      ack_conf_(),
      ifidx_(ifidx),
      params_(params),
      xid_(0),
      xid_seed_(0),
      timer_network_halt_retries(0) {}

template <typename FS, typename TS, typename Lock>
Client<FS, TS, Lock>::~Client() {
    clear_state();
}

template <typename FS, typename TS, typename Lock>
mesa_rc Client<FS, TS, Lock>::start() {
    start_over();
    return VTSS_RC_OK;
}

template <typename FS, typename TS, typename Lock>
mesa_rc Client<FS, TS, Lock>::stop() {
    clear_state();
    state_ = VTSS_APPL_IP_DHCP4C_STATE_STOPPED;
    return VTSS_RC_OK;
}

template <typename FS, typename TS, typename Lock>
mesa_rc Client<FS, TS, Lock>::fallback() {
    clear_state();
    state_ = VTSS_APPL_IP_DHCP4C_STATE_FALLBACK;
    return VTSS_RC_OK;
}

template <typename FS, typename TS, typename Lock>
mesa_rc Client<FS, TS, Lock>::if_down() {
    return stop();
}

template <typename FS, typename TS, typename Lock>
mesa_rc Client<FS, TS, Lock>::if_up() {
    if (state_ == VTSS_APPL_IP_DHCP4C_STATE_STOPPED ||
        state_ == VTSS_APPL_IP_DHCP4C_STATE_SELECTING) {
        start_over();
    }
    return VTSS_RC_OK;
}

template <typename FS, typename TS, typename Lock>
mesa_rc Client<FS, TS, Lock>::release() {
    mesa_rc rc;

    switch (state_) {
    case VTSS_APPL_IP_DHCP4C_STATE_REQUESTING:
    case VTSS_APPL_IP_DHCP4C_STATE_REBINDING:
    case VTSS_APPL_IP_DHCP4C_STATE_BOUND:
    case VTSS_APPL_IP_DHCP4C_STATE_BOUND_ARP_CHECK:
    case VTSS_APPL_IP_DHCP4C_STATE_RENEWING:
        rc = send_release() ? VTSS_RC_OK : VTSS_RC_ERROR;
        start_over();
        return rc;

    default:
        return VTSS_RC_ERROR;
    }
}

template <typename FS, typename TS, typename Lock>
mesa_rc Client<FS, TS, Lock>::decline() {
    mesa_rc rc;

    VTSS_TRACE(DEBUG) << "enter client decline()";

    switch (state_) {
    case VTSS_APPL_IP_DHCP4C_STATE_REQUESTING:
    case VTSS_APPL_IP_DHCP4C_STATE_REBINDING:
    case VTSS_APPL_IP_DHCP4C_STATE_BOUND:
    case VTSS_APPL_IP_DHCP4C_STATE_BOUND_ARP_CHECK:
    case VTSS_APPL_IP_DHCP4C_STATE_RENEWING:
        rc = send_decline() ? VTSS_RC_OK : VTSS_RC_ERROR;
        // From RFC2131 p.17:
        // "If the client detects that the address is already in use (e.g., through the use of ARP), the
        // client MUST send a DHCPDECLINE message to the server and restarts
        // the configuration process.  The client SHOULD wait a minimum of ten
        // seconds before restarting the configuration process to avoid excessive network traffic in case of looping."
        // Therefore we use a timed_retry here.
        timed_retry();
        return rc;

    default:
        return VTSS_RC_ERROR;
    }
}

template <typename FS, typename TS, typename Lock>
mesa_rc Client<FS, TS, Lock>::bind() {
    if (state_ == VTSS_APPL_IP_DHCP4C_STATE_BOUND_ARP_CHECK) {
        state_ = VTSS_APPL_IP_DHCP4C_STATE_BOUND;
        ack_conf_.signal();
    }
    return VTSS_RC_OK;
}

template <typename FS, typename TS, typename Lock>
vtss_appl_ip_if_status_dhcp4c_t Client<FS, TS, Lock>::status() const {
    vtss_appl_ip_if_status_dhcp4c_t status;
    status.state = state_;

    switch (state_) {
    case VTSS_APPL_IP_DHCP4C_STATE_REQUESTING:
    case VTSS_APPL_IP_DHCP4C_STATE_REBINDING:
    case VTSS_APPL_IP_DHCP4C_STATE_BOUND:
    case VTSS_APPL_IP_DHCP4C_STATE_BOUND_ARP_CHECK:
    case VTSS_APPL_IP_DHCP4C_STATE_RENEWING:
        status.server_ip = server_identifier.as_int();
        break;

    default:
        status.server_ip = 0;
    }

    return status;
}

template <typename FS, typename TS, typename Lock>
bool Client<FS, TS, Lock>::accept(unsigned idx) {
    if (state_ != VTSS_APPL_IP_DHCP4C_STATE_SELECTING) {
        VTSS_TRACE(INFO) << "Can only accept when state is SELECTING, current "
                            "state: "
                         <<  dhcp4c_state_to_txt(state_);

        return false;
    }

    if (idx >= offer_list.valid_offers) {
        VTSS_TRACE(INFO) << "Can not accept offer: " << idx
                         << " as the number of valid offers is "
                         << offer_list.valid_offers;
        return false;
    }

    // latch selected parameters offered
    yiaddr.as_int(offer_list.list[idx].ip.as_api_type().address);
    if (offer_list.list[idx].server_ip.valid())
        server_identifier.as_int(offer_list.list[idx].server_ip.get());
    else
        server_identifier.as_int(0);
    server_mac.copy_from(offer_list.list[idx].server_mac.as_api_type().addr);
    vendor_specific_information =
            offer_list.list[idx].vendor_specific_information;
    boot_file_name = offer_list.list[idx].boot_file_name;

    // According to RFC 2131 "The DHCPREQUEST message contains the same 'xid' as
    // the DHCPOFFER message."
    xid_ = offer_list.list[idx].xid;

    // Offer accepted, send request
    state_ = VTSS_APPL_IP_DHCP4C_STATE_REQUESTING;
    VTSS_TRACE(INFO) << ifidx_ << " Offer accepted";

    // Latch the option set we have accepted
    ack_conf_.set(AckConfPacket(offer_list.list[idx]));

    if (!send_request_broadcast()) {
        timed_retry();  // network error, will go to init
        return false;
    }

    rearm_network_halt_timer();
    return true;
}

template <typename FS, typename TS, typename Lock>
mesa_rc Client<FS, TS, Lock>::offers(size_t max_offers, size_t *valid_offers,
                                     ConfPacket *list) const {
    int i = 0;

    for (i = 0; i < min(max_offers, offer_list.valid_offers); ++i, ++list)
        *list = offer_list.list[i];
    *valid_offers = i;

    return VTSS_RC_OK;
}

template <typename FS, typename TS, typename Lock>
template <typename LL>
void Client<FS, TS, Lock>::frame_event(const DhcpFrame<LL> &f,
                                       const Mac_t src_mac) {
    Option::MessageType message_type_;

    if (!f.option_get(message_type_)) {
        VTSS_TRACE(INFO) << ifidx_ << " Not message type";
        return;
    }

    MessageType message_type = message_type_.message_type();
    VTSS_TRACE(INFO) << ifidx_ << " Got message: " << message_type
                     << " state: " << dhcp4c_state_to_txt(state_);

    if (f.xid() != xid_) {
        VTSS_TRACE(DEBUG) << ifidx_ << " Xid not matched: "
                          << (unsigned int)xid_ << " != "
                          << (unsigned int)f.xid() << ", skipping";
        return;
    }

    switch (state_) {
    case VTSS_APPL_IP_DHCP4C_STATE_SELECTING: {
        // Only react on offers
        if (message_type != DHCPOFFER) {
            return;
        }

        // Verify that offer is useful
        Option::ServerIdentifier op_server_identifier;
        if (!f.option_get(op_server_identifier)) {
            VTSS_TRACE(DEBUG) << ifidx_
                              << " SELECTING, no server identifier... retrying";
            timed_retry();
            return;
        }

        u32 prefix = 0;
        Option::SubnetMask mask;
        if (f.option_get(mask)) {
            (void)vtss_conv_ipv4mask_to_prefix(mask.ip().as_int(), &prefix);
        }
        if (prefix == 0) {
            prefix = vtss_ipv4_addr_to_prefix(f.yiaddr().as_int());
        }

        // Cache the received offer, and notify callbacks such that
        // they know there is something to accept
        if (offer_list.valid_offers < VTSS_DHCP_MAX_OFFERS) {
            ConfPacket *offer = &offer_list.list[offer_list.valid_offers];
            offer->clear();

            VTSS_TRACE(INFO) << ifidx_ << " Gather options from offer";
            copy_dhcp_options(f, src_mac, offer);

            offer_list.valid_offers++;
            VTSS_TRACE(INFO) << "signal";
            ack_conf_.signal();

        } else {
            VTSS_TRACE(INFO) << ifidx_ << " Offer overflow. Can not cache more "
                             << "than " << VTSS_DHCP_MAX_OFFERS << " offers. ";
        }

        return;
    }

    case VTSS_APPL_IP_DHCP4C_STATE_REQUESTING: {
        // We need to start over
        if (message_type == DHCPNAK) {
            VTSS_TRACE(DEBUG) << ifidx_ << " REQUESTING, got nak";
            timed_retry();
            return;
        }

        // Ignore everyting beside ACK and NAK
        if (message_type != DHCPACK) {
            VTSS_TRACE(DEBUG) << ifidx_ << " REQUESTING, not an ack";
            return;
        }

        // DHCPACK/ Record lease, set timers T1, T2 -> BOUND
        VTSS_TRACE(DEBUG) << ifidx_ << " Latching timers";
        if (!latch_timers(f)) {
            VTSS_TRACE(INFO) << ifidx_ << " Failed to patch timers";
            timed_retry();  // Invalid ACK message
            return;
        }

        if (!update_xid()) start_over();
        VTSS_TRACE(DEBUG) << ifidx_ << " REQUESTING, goto bound";
        timer_service.timer_del(timer_network_halt);
        timer_service.timer_add(timer_renew, renewal_time_value);     // T1
        timer_service.timer_add(timer_rebind, rebinding_time_value);  // T2
        timer_service.timer_add(timer_ip_lease, ip_address_lease_time);
        state_ = VTSS_APPL_IP_DHCP4C_STATE_BOUND_ARP_CHECK;
        latch_settings(f, src_mac);  // will notify listners
        return;
    }

    case VTSS_APPL_IP_DHCP4C_STATE_REBINDING:
    case VTSS_APPL_IP_DHCP4C_STATE_RENEWING:
        // DHCPNAK -> INIT
        if (message_type == DHCPNAK) {
            VTSS_TRACE(DEBUG) << ifidx_ << " got nak";
            timed_retry();
            return;
        }

        // Ignore everyting beside ACK and NAK
        if (message_type != DHCPACK) {
            VTSS_TRACE(DEBUG) << ifidx_ << " not an ack";
            return;
        }

        // DHCPACK/ Record lease, set/timers T1,T2 -> BOUND
        if (!latch_timers(f)) {
            VTSS_TRACE(DEBUG) << ifidx_ << " invlaid timers";
            timed_retry();  // Invalid ACK message
            return;
        }

        if (!update_xid()) start_over();
        timer_service.timer_del(timer_network_halt);
        timer_service.timer_add(timer_renew, renewal_time_value);     // T1
        timer_service.timer_add(timer_rebind, rebinding_time_value);  // T2
        timer_service.timer_add(timer_ip_lease, ip_address_lease_time);
        timer_network_halt_retries = 0;
        state_ = VTSS_APPL_IP_DHCP4C_STATE_BOUND;
        latch_settings(f, src_mac);  // will notify listners
        return;

    default:
        break;
    }
}

template <typename FS, typename TS, typename Lock>
void Client<FS, TS, Lock>::execute(notifications::Timer *t) {
    // Explicit locked here!!!
    ScopeLock<Lock> locked(lock_);

    VTSS_TRACE(DEBUG) << ifidx_ << " execute time event. state: "
                      << dhcp4c_state_to_txt(state_);

    switch (state_) {
    case VTSS_APPL_IP_DHCP4C_STATE_STOPPED:
        VTSS_TRACE(DEBUG) << ifidx_ << " STOPPED";
        break;

    case VTSS_APPL_IP_DHCP4C_STATE_FALLBACK:
        VTSS_TRACE(DEBUG) << ifidx_ << " FALLBACK";
        break;

    case VTSS_APPL_IP_DHCP4C_STATE_INIT:
        if (t == &timer_network_halt) {
            VTSS_TRACE(DEBUG) << ifidx_ << " INIT: goto to selecting";
            state_ = VTSS_APPL_IP_DHCP4C_STATE_SELECTING;
            if (!send_discover()) {
                timed_retry();
                return;
            }
            rearm_network_halt_timer();
        }
        break;

    case VTSS_APPL_IP_DHCP4C_STATE_SELECTING:
        if (t == &timer_network_halt) {
            VTSS_TRACE(DEBUG) << ifidx_ << " SELECTING: time out, will retry";
            start_over();
        }
        break;

    case VTSS_APPL_IP_DHCP4C_STATE_REQUESTING:
        if (t == &timer_network_halt) {
            VTSS_TRACE(DEBUG) << ifidx_ << " REQUESTING: time out, will retry";
            start_over();
        }
        break;

    case VTSS_APPL_IP_DHCP4C_STATE_REBINDING:
        // remain in same state, but retransmit request
        if (t == &timer_network_halt) {
            VTSS_TRACE(DEBUG) << ifidx_ << " REBINDING: time out, will retry";
            if (!send_request_broadcast()) {
                timed_retry();  // will go to init state
                return;
            }
            rearm_network_halt_timer();
            return;
        }

        // Lease expired/ Halt network -> INIT
        if (t == &timer_ip_lease) {
            VTSS_TRACE(DEBUG) << ifidx_
                              << " REBINDING: lease time out, goto to init";
            // out of luck
            timed_retry();  // will go to init state
            return;
        }
        break;

    case VTSS_APPL_IP_DHCP4C_STATE_BOUND_ARP_CHECK:
        break;

    case VTSS_APPL_IP_DHCP4C_STATE_BOUND:
        // T1 expires/Send DHCPREQUEST to leasing server -> RENEWING
        if (t == &timer_renew) {
            VTSS_TRACE(DEBUG) << ifidx_ << " BOUND: start renew";

            // in rfc2131  the secs fields is defined as:
            //
            //     Filled in by client, seconds elapsed since client
            //     began address acquisition or renewal process.
            //
            // We therefore need to sample the current time when we send a new
            // discovery and when we leave bind
            sample_time();

            state_ = VTSS_APPL_IP_DHCP4C_STATE_RENEWING;
            if (!send_renew()) {
                timed_retry();  // network error, will go to init state
                return;
            }

            rearm_network_halt_timer();
            return;
        }
        break;

    case VTSS_APPL_IP_DHCP4C_STATE_RENEWING:
        // remain in same state, but retransmit request
        if (t == &timer_network_halt) {
            VTSS_TRACE(DEBUG) << ifidx_ << " RENEWING: time out, will retry";
            if (!send_renew()) {
                timed_retry();  // network error, will go to init state
                return;
            }
            rearm_network_halt_timer();
            return;
        }

        // T2 expires/ Broadcast DHCPREQUEST -> REBINDING
        if (t == &timer_rebind) {
            VTSS_TRACE(DEBUG) << ifidx_ << " RENEWING: time out, goto rebind";
            state_ = VTSS_APPL_IP_DHCP4C_STATE_REBINDING;
            if (!send_request_broadcast()) {
                timed_retry();  // network error, will go to init state
                return;
            }

            rearm_network_halt_timer();
            return;
        }
        break;
    }
}

template <typename FS, typename TS, typename Lock>
template <typename LL>
void Client<FS, TS, Lock>::latch_settings(const DhcpFrame<LL> &f,
                                          const Mac_t &src_mac) {
    VTSS_TRACE(INFO) << ifidx_ << " setting ack conf";
    ConfPacket copy;
    if (ack_conf_.get().valid()) {
        copy = ack_conf_.get().get();
    } else {
        copy.clear();
    }

    copy_dhcp_options(f, src_mac, &copy);
    VTSS_TRACE(INFO) << "set " << copy;
    ack_conf_.set(AckConfPacket(copy), true);
}

template <typename FS, typename TS, typename Lock>
void Client<FS, TS, Lock>::timed_retry() {
    clear_state();
    rearm_network_halt_timer();
    state_ = VTSS_APPL_IP_DHCP4C_STATE_INIT;
}

template <typename FS, typename TS, typename Lock>
void Client<FS, TS, Lock>::clear_state() {
    state_ = VTSS_APPL_IP_DHCP4C_STATE_STOPPED;
    timer_service.timer_del(timer_network_halt);
    timer_service.timer_del(timer_ip_lease);
    timer_service.timer_del(timer_renew);
    timer_service.timer_del(timer_rebind);
    renewal_time_value = seconds(0);
    rebinding_time_value = seconds(0);
    ip_address_lease_time = seconds(0);

    for (int i = 0; i < offer_list.valid_offers; ++i)
        offer_list.list[i].clear();
    offer_list.valid_offers = 0;

    VTSS_TRACE(INFO) << ifidx_ << " clearing ack frame";

    VTSS_TRACE(INFO) << "set <none>";
    ack_conf_.set(AckConfPacket(), true);
}

template <typename FS, typename TS, typename Lock>
void Client<FS, TS, Lock>::rearm_network_halt_timer() {
    timer_service.timer_del(timer_network_halt);

    timer_network_halt_retries++;
    if (timer_network_halt_retries < 5) {
        timer_service.timer_add(timer_network_halt, seconds(3));
    } else {
        timer_service.timer_add(timer_network_halt, seconds(15));
    }
}

template <typename FS, typename TS, typename Lock>
void Client<FS, TS, Lock>::start_over() {
    VTSS_TRACE(DEBUG) << "enter client start_over()";
    clear_state();
    rearm_network_halt_timer();
    state_ = VTSS_APPL_IP_DHCP4C_STATE_SELECTING;
    send_discover();  // error is handled by network halt timer
}

template <typename FS, typename TS, typename Lock>
template <typename LL>
bool Client<FS, TS, Lock>::latch_timers(const DhcpFrame<LL> &f) {
    // Nice for debugging
    // ip_address_lease_time = seconds(240);
    // rebinding_time_value = seconds(120);
    // renewal_time_value = seconds(15);
    // return true;

    // get ip address lease time. This option MUST be
    // present in ACK messages. rfc2131 page 29
    Option::IpAddressLeaseTime op_ip_address_lease_time;
    if (f.option_get(op_ip_address_lease_time)) {
        ip_address_lease_time = seconds(op_ip_address_lease_time.val());

    } else {
        // Invalid ACK message
        VTSS_TRACE(WARNING) << ifidx_ << " no IpAddressLeaseTime";
        return false;
    }

    // get renewval time. defaults to 0.5 * ip_address_lease_time.
    // rfc2131 4.4.5 page 41
    Option::RenewalTimeValue op_renewal_time_value;
    if (f.option_get(op_renewal_time_value)) {
        renewal_time_value = seconds(op_renewal_time_value.val());
    } else {
        renewal_time_value = seconds(0.5 * ip_address_lease_time.raw());
    }

    // get rebinding time. defaults to 0.875 *
    // ip_address_lease_time. rfc2131 4.4.5 page 41
    Option::RebindingTimeValue op_rebinding_time_value;
    if (f.option_get(op_rebinding_time_value)) {
        rebinding_time_value = seconds(op_rebinding_time_value.val());
    } else {
        rebinding_time_value = seconds(0.875 * ip_address_lease_time.raw());
    }

    // sanity checks
    if (ip_address_lease_time < renewal_time_value ||
        ip_address_lease_time < rebinding_time_value) {
        VTSS_TRACE(WARNING) << ifidx_ << " check 1 failed";
        return false;
    }

    // sanity checks
    if (rebinding_time_value < renewal_time_value) {
        VTSS_TRACE(WARNING) << ifidx_ << " check 2 failed";
        return false;
    }

    VTSS_TRACE(DEBUG) << "ip_address_lease_time = "
                      << ip_address_lease_time.raw() << " seconds"
                      << " rebinding_time_value = "
                      << rebinding_time_value.raw()<< " seconds"
                      << " renewal_time_value = " << renewal_time_value.raw()
                      << " seconds";
    return true;
}

template <typename FS, typename TS, typename Lock>
void Client<FS, TS, Lock>::add_client_identifier_and_hostname(FrameStack &f) {
    add_hostname(f);
    add_client_identifier(f);
}

template <typename FS, typename TS, typename Lock>
void Client<FS, TS, Lock>::add_client_identifier(FrameStack &f) {
    DHCPC_BUF256_DECLARE();

    /* Add option 61: client identifier
     *
     * Code   Len   Type  Client-Identifier
     * +-----+-----+-----+-----+-----+---
     * |  61 |  n  |  t1 |  i1 |  i2 | ...
     * +-----+-----+-----+-----+-----+---
     *
     * The hardware type field is requested in option 61, so the length field
     * need to plus one more byte. When the hardware type is 'Ethernet'(0x01),
     * the value is hardware address and its length is 6.
     */

    switch (params_.client_id.type) {
        case VTSS_APPL_IP_DHCP4C_ID_TYPE_IF_MAC:
            DHCPC_BUF256_PUSH_BYTE(OPTIONHTYPE_ETHERNET);
            DHCPC_BUF256_PUSH_BYTES_COMMIT(
                    client_id_if_mac_get(DHCPC_BUF256_PTR,
                                         DHCPC_BUF256_MAX,
                                         params_.client_id.if_mac));
            break;
        case VTSS_APPL_IP_DHCP4C_ID_TYPE_ASCII: {
            DHCPC_BUF256_PUSH_BYTE(OPTIONHTYPE_NOT_HW_ADDR);

            size_t ascii_str_len = strlen(params_.client_id.ascii);
            for (size_t i = 0; i < ascii_str_len; i++) {
                // Convert to lower case
                DHCPC_BUF256_PUSH_BYTE(tolower(params_.client_id.ascii[i]));
            }
            break;
        }
        case VTSS_APPL_IP_DHCP4C_ID_TYPE_HEX: {
            // Use the completed hex value when the DHCP client-id type is hex
            // The hardware type is refer to the first byte of user configured
            // hex input
             DHCPC_BUF256_PUSH_BYTES_COMMIT(
                client_id_hex_str_convert(DHCPC_BUF256_PTR,
                                          DHCPC_BUF256_MAX,
                                          params_.client_id.hex));
            break;
        }
        case VTSS_APPL_IP_DHCP4C_ID_TYPE_AUTO:
        default:
            // Default option (for backward compatible purpose), the value is
            // hostname-sysmac[3]-sysmac[4]-sysmac[5]
            DHCPC_BUF256_PUSH_BYTE(OPTIONHTYPE_NOT_HW_ADDR);
            // The return value of client_def_hostname_get(), which the string
            // length is including the terminating zero character
            DHCPC_BUF256_PUSH_BYTES_COMMIT(
                client_def_hostname_get(DHCPC_BUF256_PTR,
                                        DHCPC_BUF256_MAX) - 1);
            break;
    }

    // Refer to RFC 2132 9.14. Client-identifier
    // The code for this option is 61, and its minimum length is 2.
    if (DHCPC_BUF256_USED_SIZE > 2) {
        Option::ClientIdentifier client_identifier;
        if (client_identifier.set((const u8 *)buf, DHCPC_BUF256_USED_SIZE) == false) {
            VTSS_TRACE(DEBUG) << "Set client iden len " << DHCPC_BUF256_USED_SIZE << "fail";
        }
        if (f->add_option(client_identifier) ==  false) {
            VTSS_TRACE(DEBUG) << "Add client iden option fail";
        }
        VTSS_TRACE(INFO) << "Adding client identifier:" << buf;
    }
}

template <typename FS, typename TS, typename Lock>
void Client<FS, TS, Lock>::add_hostname(FrameStack &f) {
    // Trying to be (bug-)complient with the old dhcp client
    DHCPC_BUF256_DECLARE();

    /* Add option 12: hostname
     *
     * Code Len Host Name
     * +-----+-----+-----+-----+-----+-----+-----+-----+--
     * | 12  | n   | h1  | h2  | h3  | h4  | h5  | h6  | ...
     * +-----+-----+-----+-----+-----+-----+-----+-----+--
     */

    size_t hostname_str_len = strlen(params_.hostname);
    if (hostname_str_len) {
        for (size_t i = 0; i < hostname_str_len; i++) {
            // Convert to lower case
            DHCPC_BUF256_PUSH_BYTE(tolower(params_.hostname[i]));
        }
    } else {
        // New configured hostname is not set. Call the original API to get the
        // hostname
        // Default option (for backward compatible purpose), the value is
        // hostname-sysmac[3]-sysmac[4]-sysmac[5]
        DHCPC_BUF256_PUSH_BYTES_COMMIT(
            client_def_hostname_get(DHCPC_BUF256_PTR,
                                    DHCPC_BUF256_MAX)-1); //excluding terminating zero
    }

    if (DHCPC_BUF256_USED_SIZE) {
        Option::HostName host_name;
        if (host_name.set((const u8 *)buf, DHCPC_BUF256_USED_SIZE) == false){
            VTSS_TRACE(DEBUG) << "Set host name len "<< DHCPC_BUF256_USED_SIZE << " return fail";
        }
        if (f->add_option(host_name) == false){
            VTSS_TRACE(DEBUG) << "add host name return fail";
        }
        VTSS_TRACE(INFO) << "Adding host name:" << buf;
    }
}

template <typename FS, typename TS, typename Lock>
void Client<FS, TS, Lock>::add_parameter_request_list(FrameStack &f) {
    Option::ParameterRequestList p;

    p.add(Option::CODE_SERVER_IDENTIFIER);
    p.add(Option::CODE_IP_ADDRESS_LEASE_TIME);
    p.add(Option::CODE_RENEWAL_TIME_VALUE);
    p.add(Option::CODE_REBINDING_TIME_VALUE);
    p.add(Option::CODE_SUBNET_MASK);
    p.add(Option::CODE_ROUTER_OPTION);
    p.add(Option::CODE_DOMAIN_NAME_SERVER_OPTION);
    p.add(Option::CODE_DOMAIN_NAME);
    p.add(Option::CODE_BROADCAST_ADDRESS_OPTION);
    p.add(Option::CODE_NETWORK_TIME_PROTOCOL_SERVERS_OPTION);

    // Options which depends on flags
    if (params_.dhcpc_flags & VTSS_APPL_IP_DHCP4C_FLAG_OPTION_43)
        p.add(Option::CODE_VENDOR_SPECIFIC_INFORMATION);

    if (params_.dhcpc_flags & VTSS_APPL_IP_DHCP4C_FLAG_OPTION_67)
        p.add(Option::CODE_BOOTFILE_NAME);

    f->add_option(p);
}

template <typename FS, typename TS, typename Lock>
void Client<FS, TS, Lock>::add_vendor_class_identifier(FrameStack &f) {
    if (params_.dhcpc_flags & VTSS_APPL_IP_DHCP4C_FLAG_OPTION_60) {
        char buf[254];
        size_t s = client_vendor_class_identifier(buf, 254);

        Option::VendorClassIdentifier o;
        o.set(buf, s);

        f->add_option(o);
    }
}

template <typename FS, typename TS, typename Lock>
bool Client<FS, TS, Lock>::send_discover() {
    // choose a id to use for this dhcp session
    if (!update_xid()) start_over();

    // in rfc2131  the secs fields is defined as:
    //
    //     Filled in by client, seconds elapsed since client
    //     began address acquisition or renewal process.
    //
    // we therefor need to sample the current time when we send a new
    // discovery and when we leave bind
    sample_time();
    seconds sec(Clock::to_seconds(Clock::now() - secs_));

    FrameStack f;
    f->build_req_frame(tx_service.mac_address(ifidx_), xid_, sec.raw32());
    f->add_option(Option::MessageType(DHCPDISCOVER));
    f->add_option(Option::MaximumDHCPMessageSize(MAX_DHCP_MESSAGE_SIZE));
    add_parameter_request_list(f);
    add_vendor_class_identifier(f);
    add_client_identifier_and_hostname(f);

    f->add_option(Option::End());
    return send_broadcast(f);
}

template <typename FS, typename TS, typename Lock>
bool Client<FS, TS, Lock>::send_request() {
    seconds sec(Clock::to_seconds(Clock::now() - secs_));

    FrameStack f;
    f->build_req_frame(tx_service.mac_address(ifidx_), xid_, sec.raw32());
    f->add_option(Option::MessageType(DHCPREQUEST));
    f->add_option(Option::MaximumDHCPMessageSize(MAX_DHCP_MESSAGE_SIZE));
    add_parameter_request_list(f);
    add_vendor_class_identifier(f);
    add_client_identifier_and_hostname(f);

    f->add_option(Option::ServerIdentifier(server_identifier));
    f->add_option(Option::RequestedIPAddress(yiaddr));
    f->add_option(Option::End());
    return send_unicast(f);
}

template <typename FS, typename TS, typename Lock>
bool Client<FS, TS, Lock>::send_renew() {
    seconds sec(Clock::to_seconds(Clock::now() - secs_));

    VTSS_TRACE(DEBUG) << "Building renew packet.";

    FrameStack f;
    f->build_req_frame(tx_service.mac_address(ifidx_), xid_, sec.raw32());
    f->ciaddr(yiaddr);
    f->add_option(Option::MessageType(DHCPREQUEST));
    f->add_option(Option::MaximumDHCPMessageSize(MAX_DHCP_MESSAGE_SIZE));
    add_parameter_request_list(f);
    add_vendor_class_identifier(f);
    add_client_identifier_and_hostname(f);

    f->add_option(Option::End());
    return send_unicast(f);
}

template <typename FS, typename TS, typename Lock>
bool Client<FS, TS, Lock>::send_request_broadcast() {
    seconds sec(Clock::to_seconds(Clock::now() - secs_));

    VTSS_TRACE(DEBUG) << "Building broadcast request packet.";

    FrameStack f;
    f->build_req_frame(tx_service.mac_address(ifidx_), xid_, sec.raw32());
    /*According to  RFC 2131 p.41, the client should set the ciaddr field to its current address when in rebinding state.*/
    if (state_ == VTSS_APPL_IP_DHCP4C_STATE_REBINDING) {
        f->ciaddr(yiaddr);
    }
    f->add_option(Option::MessageType(DHCPREQUEST));
    f->add_option(Option::MaximumDHCPMessageSize(MAX_DHCP_MESSAGE_SIZE));
    add_parameter_request_list(f);
    add_vendor_class_identifier(f);
    add_client_identifier_and_hostname(f);

    /* According to RFC 2131, when in rebinding state the request packet must not include a server identifier (p.40).
       Same applies for the requested ip option (p.32).*/
    if (state_ != VTSS_APPL_IP_DHCP4C_STATE_REBINDING) {
        f->add_option(Option::ServerIdentifier(server_identifier));
        f->add_option(Option::RequestedIPAddress(yiaddr));
    }
    f->add_option(Option::End());
    return send_broadcast(f);
}

template <typename FS, typename TS, typename Lock>
bool Client<FS, TS, Lock>::send_release() {
    seconds sec(Clock::to_seconds(Clock::now() - secs_));

    FrameStack f;
    f->build_req_frame(tx_service.mac_address(ifidx_), xid_, sec.raw32());
    f->ciaddr(yiaddr);
    f->add_option(Option::MessageType(DHCPRELEASE));
    add_client_identifier_and_hostname(f);
    add_vendor_class_identifier(f);
    f->add_option(Option::End());
    return send_unicast(f);
}

template <typename FS, typename TS, typename Lock>
bool Client<FS, TS, Lock>::send_decline() {
    seconds sec(Clock::to_seconds(Clock::now() - secs_));

    FrameStack f;
    f->build_req_frame(tx_service.mac_address(ifidx_), xid_, sec.raw32());
    f->add_option(Option::MessageType(DHCPDECLINE));
    add_client_identifier_and_hostname(f);
    f->add_option(Option::End());
    return send_unicast(f);
}

template <typename FS, typename TS, typename Lock>
bool Client<FS, TS, Lock>::send_unicast(FrameStack &f) {
    return tx_service.send_udp(f, yiaddr, 68,          // src-udp
                               server_identifier, 67,  // dst-udp
                               server_mac, ifidx_);     // dst-mac
}

template <typename FS, typename TS, typename Lock>
bool Client<FS, TS, Lock>::send_broadcast(FrameStack &f) {
    return tx_service.send_udp(f, IPv4_t(0u), 68,              // src-udp
                               IPv4_t(0xffffffffu), 67,        // dst-udp
                               build_mac_broadcast(), ifidx_);  // dst-mac
}

template <typename FS, typename TS, typename Lock>
bool Client<FS, TS, Lock>::update_xid() {

    if (vtss_random(&xid_) != VTSS_RC_OK) {
        VTSS_TRACE(ERROR) << " Generate dhcp xid failed!";
        return false;
    }
    VTSS_TRACE(DEBUG) << ifidx_ << " XID = " << xid_;
    return true;
}

template <typename FS, typename TS, typename Lock>
void Client<FS, TS, Lock>::sample_time() {
    secs_ = Clock::now();
}

} /* dhcp */
} /* vtss */
#undef VTSS_TRACE_MODULE_ID
#endif

