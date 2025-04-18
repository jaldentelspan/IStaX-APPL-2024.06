/* dhcrelay.c

   DHCP/BOOTP Relay Agent. */

/*
 * Copyright(c) 2004-2008 by Internet Systems Consortium, Inc.("ISC")
 * Copyright(c) 1997-2003 by Internet Software Consortium
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *   Internet Systems Consortium, Inc.
 *   950 Charter Street
 *   Redwood City, CA 94063
 *   <info@isc.org>
 *   http://www.isc.org/
 *
 * This software has been written for Internet Systems Consortium
 * by Ted Lemon in cooperation with Vixie Enterprises and Nominum, Inc.
 * To learn more about Internet Systems Consortium, see
 * ``http://www.isc.org/''.  To learn more about Vixie Enterprises,
 * see ``http://www.vix.com''.   To learn more about Nominum, Inc., see
 * ``http://www.nominum.com''.
 */

// Disable GCC 6.3 warnings due to  VTSS_ERPS_BLOCK_RING_PORT macro etc.
#if defined(__GNUC__) && __GNUC__ >= 6
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#endif

#include "dhcpd.h"
#include "ip_api.h"
#include "misc_api.h"
#include "vtss/appl/interface.h"
#include "vtss/basics/expose/table-status.hxx"
#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#endif
#include <sys/time.h>
#include <vtss/basics/trace.hxx>

#ifdef ISCDHCP_ISTAX_PLATFORM
#include "iscdhcp_istax.h"
#endif /* ISCDHCP_ISTAX_PLATFORM */

TIME default_lease_time = 43200; /* 12 hours... */
TIME max_lease_time = 86400; /* 24 hours... */
struct tree_cache *global_options[256];

struct option *requested_opts[2];

/* Information about each active vlan interface*/
typedef vtss::expose::TableStatus<
    vtss::expose::ParamKey<mesa_vid_t>,
    vtss::expose::ParamVal<mesa_ipv4_t *>> dhcp_relay_vlan_info;

dhcp_relay_vlan_info dhcp_relay_vlan_if_info("dhcp_relay_vlan_if_info", VTSS_MODULE_ID_DHCP_RELAY);

/* Needed to prevent linking against conflex.c. */
int lexline;
int lexchar;
char *token_line;
char *tlname;

const char *path_dhcrelay_pid = _PATH_DHCRELAY_PID;

unsigned long bogus_agent_drops = 0;	/* Packets dropped because agent option
				   field was specified and we're not relaying
				   packets that already have an agent option
				   specified. */
unsigned long bogus_giaddr_drops = 0;	/* Packets sent to us to relay back to a
				   client, but with a bogus giaddr. */
unsigned long client_packets_relayed = 0;	/* Packets relayed from client to server. */
unsigned long server_packet_errors = 0;	/* Errors sending packets to servers. */
unsigned long server_packets_relayed = 0;	/* Packets relayed from server to client. */
unsigned long client_packet_errors = 0;	/* Errors sending packets to clients. */

int add_agent_options = 0;	/* If nonzero, add relay agent options. */

unsigned long agent_option_errors = 0;    /* Number of packets forwarded without
				   agent options because there was no room. */
unsigned long drop_agent_mismatches = 0;	/* If nonzero, drop server replies that
				   don't have matching circuit-id's. */
unsigned long corrupt_agent_options = 0;	/* Number of packets dropped because
				   relay agent information option was bad. */
unsigned long missing_agent_option = 0;	/* Number of packets dropped because no
				   RAI option matching our ID was found. */
unsigned long bad_circuit_id = 0;		/* Circuit ID option in matching RAI option
				   did not match any known circuit ID. */
unsigned long missing_circuit_id = 0;	/* Circuit ID option in matching RAI option
				   was missing. */
unsigned long bad_remote_id = 0;		/* Remote ID option in matching RAI option
				   did not match any known remote ID. */
unsigned long missing_remote_id = 0;	/* Remote ID option in matching RAI option
				   was missing. */
unsigned long receive_server_packets = 0;       /* Receive DHCP message from server */
unsigned long receive_client_packets = 0;       /* Receive DHCP message from client */
unsigned long receive_client_agent_option = 0;  /* Receive relay agent information option from client */
unsigned long replace_agent_option = 0;         /* Replace relay agent information option */
unsigned long keep_agent_option = 0;            /* Keep relay agent information option */
unsigned long drop_agent_option = 0;            /* Drop relay agent information option */
unsigned long max_hop_count = 254; /*10;*/		/* Maximum hop count */

#ifdef DHCPv6
	/* Force use of DHCPv6 interface-id option. */
isc_boolean_t use_if_id = ISC_FALSE;
#endif

	/* Maximum size of a packet with agent options added. */
int dhcp_max_agent_option_packet_length = DHCP_MTU_MIN;

	/* What to do about packets we're asked to relay that
	   already have a relay option: */
enum { forward_and_append,	/* Forward and append our own relay option. */
       forward_and_replace,	/* Forward, but replace theirs with ours. */
       forward_untouched,	/* Forward without changes. */
       discard } agent_relay_mode = forward_and_replace;

extern u_int16_t local_port;
extern u_int16_t remote_port;

/* Relay agent server list. */
struct server_list {
	struct server_list *next;
	struct sockaddr_in to;
} *servers;

#ifdef DHCPv6
struct stream_list {
	struct stream_list *next;
	struct interface_info *ifp;
	struct sockaddr_in6 link;
	int id;
} *downstreams, *upstreams;

static struct stream_list *parse_downstream(char *);
static struct stream_list *parse_upstream(char *);
static void setup_streams(void);
#endif

static void do_relay4(struct interface_info *, struct dhcp_packet *,
	              unsigned int, unsigned int, struct iaddr,
		      struct hardware *);
static int add_relay_agent_options(struct interface_info *,
				   struct dhcp_packet *, unsigned,
				   struct in_addr);
static int find_interface_by_agent_option(struct dhcp_packet *,
			       struct interface_info **, u_int8_t *, int);
static int strip_relay_agent_options(struct interface_info *,
				     struct interface_info **,
				     struct dhcp_packet *, unsigned);

#ifndef ISCDHCP_ISTAX_PLATFORM
static char copyright[] = "Copyright 2004-2008 Internet Systems Consortium.";
static char arr[] = "All rights reserved.";
static char message[] = "Internet Systems Consortium DHCP Relay Agent";
static char url[] = "For info, please visit http://www.isc.org/sw/dhcp/";
#endif /* ISCDHCP_ISTAX_PLATFORM */

#ifdef DHCPv6
#define DHCRELAY_USAGE \
"Usage: dhcrelay [-4] [-d] [-q] [-a] [-D]\n"\
"                     [-A <length>] [-c <hops>] [-p <port>]\n" \
"                     [-m append|replace|forward|discard]\n" \
"                     [-i interface0 [ ... -i interfaceN]\n" \
"                     server0 [ ... serverN]\n\n" \
"       dhcrelay -6   [-d] [-q] [-I] [-c <hops>] [-p <port>]\n" \
"                     -l lower0 [ ... -l lowerN]\n" \
"                     -u upper0 [ ... -u upperN]\n" \
"       lower (client link): [address%%]interface[#index]\n" \
"       upper (server link): [address%%]interface"
#else
#define DHCRELAY_USAGE \
"Usage: dhcrelay [-d] [-q] [-a] [-D] [-A <length>] [-c <hops>] [-p <port>]\n" \
"                [-m append|replace|forward|discard]\n" \
"                [-i interface0 [ ... -i interfaceN]\n" \
"                server0 [ ... serverN]\n\n"
#endif

#ifdef DHCPv6
static void usage() {
	log_fatal(DHCRELAY_USAGE);
}
#endif /* DHCPv6 */

#ifdef ISCDHCP_ISTAX_PLATFORM
static iscdhcp_reply_update_circuit_id_callback_t iscdhcpupdate_reply_circuit_id_callback = NULL;
static iscdhcp_reply_check_circuit_id_callback_t iscdhcp_reply_circuit_id_callback = NULL;
static iscdhcp_reply_send_client_callback_t iscdhcp_reply_send_client_callback = NULL;
static iscdhcp_reply_send_server_callback_t iscdhcp_reply_send_server_callback = NULL;
static iscdhcp_reply_fill_giaddr_callback_t iscdhcp_reply_fill_giaddr_callback = NULL;
static unsigned char iscdhcp_platform_mac[6];
static unsigned long iscdhcp_relay_running = 0;


/* Callback function for update circut ID
   Return -1: Update circut ID fail
   Return  0: Update circut ID success */
void iscdhcp_reply_update_circuit_id_register(iscdhcp_reply_update_circuit_id_callback_t cb) {
    iscdhcpupdate_reply_circuit_id_callback = cb;
}

/* Callback function for check circut ID
   Return -1: circut ID is invalid
   Return  0: circut ID is valid */
void iscdhcp_reply_check_circuit_id_register(iscdhcp_reply_check_circuit_id_callback_t cb) {
    iscdhcp_reply_circuit_id_callback = cb;
}

/* Callback function for send DHCP message to client
   Return -1: send packet fail
   Return  0: send packet success */
void iscdhcp_reply_send_client_register(iscdhcp_reply_send_client_callback_t cb) {
    iscdhcp_reply_send_client_callback = cb;
}

/* Callback function for send DHCP message to server
   Return -1: send packet fail
   Return  0: send packet success */
void iscdhcp_reply_send_server_register(iscdhcp_reply_send_server_callback_t cb) {
    iscdhcp_reply_send_server_callback = cb;
}

/* Callback function for add agent option */
void iscdhcp_reply_fill_giaddr_register(iscdhcp_reply_fill_giaddr_callback_t cb) {
    iscdhcp_reply_fill_giaddr_callback = cb;
}

/* Set platform MAC address */
void iscdhcp_set_platform_mac(unsigned char *platform_mac) {
    memcpy(iscdhcp_platform_mac, platform_mac, 6);
}

/* Set relay mode operation */
void iscdhcp_relay_mode_set(uint32_t relay_mode) {
    if (iscdhcp_relay_running != relay_mode)
        iscdhcp_relay_running = relay_mode;
}

/* Add DHCP relay server */
void iscdhcp_add_dhcp_server(uint32_t server) {
    struct server_list *sp;

    if (!server)
        return;

    sp = ((struct server_list *) dmalloc(sizeof *sp, MDL));
    if (!sp)
    	log_fatal("no memory for DHCP server.\n");

    server = htonl(server);
    memcpy(&sp->to.sin_addr, &server, sizeof sp->to.sin_addr);
    sp->to.sin_port = local_port;
    sp->to.sin_family = AF_INET;
#ifdef HAVE_SA_LEN
    sp->to.sin_len = sizeof sp->to;
#endif
    sp->next = servers;
    servers = sp;
}

/* Delete DHCP relay server */
void iscdhcp_del_dhcp_server(uint32_t server) {
    struct server_list *sp, *prev_sp = NULL;

    server = htonl(server);
    for (sp = servers; sp; sp = sp->next) {
        if (memcmp(&sp->to.sin_addr, &server, sizeof sp->to.sin_addr) == 0) {
            if (prev_sp == NULL)
                servers = sp->next;
            else
                prev_sp->next = sp->next;
            dfree(sp, MDL);
            break;
        }
        prev_sp = sp;
    }
}

/* Clear DHCP relay server */
void iscdhcp_clear_dhcp_server(void) {
    struct server_list *sp = servers;

    while (sp) {
        servers = sp->next;
        dfree(sp, MDL);
        sp = servers;
    }
}

/* Set relay agent information mode operation */
void iscdhcp_set_agent_info_mode(uint32_t relay_info_mode) {
    if (add_agent_options != relay_info_mode)
        add_agent_options = relay_info_mode;
}

/* Set relay agent information policy */
void iscdhcp_set_relay_info_policy(uint32_t relay_info_policy) {
    switch (relay_info_policy) {
        case 0: //replace
            agent_relay_mode = forward_and_replace;
            break;
        case 1: //keep
            agent_relay_mode = forward_untouched;
            break;
        case 2: //drop
            agent_relay_mode = discard;
            break;            
    }
}

/* Set relay Maximum hop count */
void iscdhcp_set_max_hops(uint32_t relay_max_hops) {
    max_hop_count = relay_max_hops;
}

/* change interface IP address */
void iscdhcp_change_interface_addr(const char *interface_name, struct in_addr *interface_addr) {
    struct interface_info *iface;
	T_D("Enter");

	for (iface = interfaces; iface; iface = iface->next) {
		if (!strcmp(iface->name, interface_name)) {
		    memcpy(iface->addresses, interface_addr, sizeof(struct in_addr));
            break;
        }
	}
}

void
remove_ipv4_addr_from_interface(struct interface_info *iface, int index_to_remove) {

	T_D("Enter, array size = %d, array max size = %d", iface->address_count, iface->address_max);

	if ((index_to_remove >= iface->address_count) || (index_to_remove < 0)) {
		T_D("Invalid remove index, exit");
		return;
	}

	/* Array is empty. No action needed. */
	if (iface->address_count == 0) {
		T_D("Empty array, exit");
		return;
	}

	/* Only one entry in array. */
	if (iface->address_count == 1) {
		iface->address_count--;
		T_D("Last entry removed, exit");
		return;
	}

	struct in_addr *tmp = NULL;

	/* As the counterpart add_ipv4_addr_to_interface works in size increments of 4, we assume same algorithm here.*/
	
	/* Resizing array to save memory. */
	if (iface->address_count <= (iface->address_max-4)) {
		T_D("Resizing array before deleting entry.");
		int new_max = iface->address_max - 4;
		tmp = (struct in_addr*)dmalloc(new_max * sizeof(struct in_addr), MDL);
		
		if (tmp != NULL) {
			if (index_to_remove != 0) {
				memcpy(tmp, iface->addresses, index_to_remove * sizeof(struct in_addr));
			}

			if(index_to_remove != (iface->address_count - 1)) {
				memcpy(tmp+index_to_remove, iface->addresses+index_to_remove+1, (iface->address_count-index_to_remove-1) * sizeof(struct in_addr));
			}

			dfree(iface->addresses, MDL);
			iface->addresses = tmp;
			iface->address_max = new_max;
			iface->address_count--;
			T_D("Array resized and entry removed, exit");
			return;
		}
	}

	/* No array resizing needed. */
	if ((iface->address_count > (iface->address_max-4)) || tmp == NULL) {
		T_D("Array will not be resized.");

		int num_to_move = iface->address_count - index_to_remove - 1;
		memmove(&iface->addresses[index_to_remove], &iface->addresses[index_to_remove+1], num_to_move * sizeof(struct in_addr));
		iface->address_count--;
		T_D("Entry removed, exit");
		return;
	}

}

static void dhcp_relay_if_down(mesa_vid_t vlan) {
	mesa_ipv4_t ipv4_addr;
	char buf[20];

	T_D("Enter, interface is down");

	/* Deleting vlan from info table if it exists there.*/
	if (dhcp_relay_vlan_if_info.get(vlan, &ipv4_addr) != VTSS_RC_OK) {
		T_D("Entry did not exist in relay, exit");
		return;
	}

	dhcp_relay_vlan_if_info.del(vlan);
	
	if (!interfaces) {
		T_D("Interfaces object has not been created, exit");
		return;
	}

	struct interface_info *tmp;
	for (tmp = interfaces; tmp; tmp = tmp->next) {
		for (int i = 0; i < tmp->address_count; i++) {
			T_D("vlan ip = %s, %u, tmp->address ip = %s, %u", misc_ipv4_txt(ipv4_addr, buf), ipv4_addr, inet_ntoa(tmp->addresses[i]), ntohl(tmp->addresses[i].s_addr));
			if (ntohl(tmp->addresses[i].s_addr) == ipv4_addr) {
				T_D("Ip found, will be removed from array.");
				remove_ipv4_addr_from_interface(tmp, i);
				return;
			}
		}
	}
}

static void dhcp_relay_if_up(mesa_vid_t vlan)
{
	mesa_ipv4_t                curr_ip_addr;
	mesa_ipv4_t                new_ip_addr;
	char                       buf[20];
	bool                       updating_ip = false;
	vtss_ifindex_t             ifindex;
        vtss_appl_ip_if_key_ipv4_t key = {};

	T_D("Enter, interface is up");

	(void)vtss_ifindex_from_vlan(vlan, &ifindex);
        key.ifindex = ifindex;
	if (vtss_appl_ip_if_status_ipv4_itr(&key, &key) == VTSS_RC_OK && key.ifindex == ifindex) {
		T_D("Vlan ip = %s", misc_ipv4_txt(key.addr.address, buf));
		new_ip_addr = key.addr.address;
		if (dhcp_relay_vlan_if_info.get(vlan, &curr_ip_addr) == VTSS_RC_OK) {
			updating_ip = true;
		} else {
			curr_ip_addr = new_ip_addr;
		}
		dhcp_relay_vlan_if_info.set(vlan, &new_ip_addr);

		if (!interfaces) {
			T_D("Interfaces object has not been set");
			return;
		}

		/* Runs when relay has been started. */
		struct interface_info *tmp;
		for (tmp = interfaces; tmp; tmp = tmp->next) {
			for (int i = 0; i < tmp->address_count; i++) {
				//ntohl
				T_D("vlan ip = %s, %u, tmp->address ip = %s, %u", misc_ipv4_txt(curr_ip_addr, buf), curr_ip_addr, inet_ntoa(tmp->addresses[i]), ntohl(tmp->addresses[i].s_addr));
				if (ntohl(tmp->addresses[i].s_addr) == curr_ip_addr) {
					T_D("curr ip found");
					if (updating_ip) {
						T_D("Updating ip address");
						tmp->addresses[i].s_addr = htonl(new_ip_addr);
					}
					return;
				}	
			}
    		} 
		/* Adding ip address to list. */
		T_D("Adding new ip address");
		tmp = interfaces;
		struct in_addr in_addr;
		in_addr.s_addr = htonl(new_ip_addr);
		add_ipv4_addr_to_interface(tmp, &in_addr);

	} else {
		/* Interface is up but has no ip address.*/
		T_D("Vlan has no ip address, removing old ip from relay data");
		dhcp_relay_if_down(vlan);
	}
}

void DHCP_RELAY_if_updated_callback(vtss_ifindex_t  ifidx) {
    VTSS_TRACE(DEBUG) << "Enter, updates to " << ifidx;
    if (vtss_ifindex_is_vlan(ifidx)) {
        vtss_ifindex_elm_t ife;
        (void)vtss_ifindex_decompose(ifidx, &ife);
        mesa_vid_t vlan = ife.ordinal;

        vtss_appl_ip_if_status_link_t if_status_link;

        if (vtss_appl_ip_if_status_link_get(ifidx, &if_status_link) != VTSS_RC_OK) {
            return;
        }

        if (if_status_link.flags & VTSS_APPL_IP_IF_LINK_FLAG_UP) {
            dhcp_relay_if_up(vlan);
        } else {
            dhcp_relay_if_down(vlan);
        }

        VTSS_TRACE(DEBUG) << "Exit, vlan_if_info.size = " << dhcp_relay_vlan_if_info.size();
    }
}

/* ISC DHCP initial function */
int iscdhcp_init(void) {
	isc_result_t status;
	struct server_list *sp = NULL;
	struct interface_info *tmp = NULL;
	u_int16_t port_local = htons(67), port_remote= htons(68);
#ifdef DHCPv6
	struct stream_list *sl = NULL;
	int local_family_set = 0;
#endif

	T_D("Enter");
	/* Set up the OMAPI. */
	status = omapi_init();
	if (status != ISC_R_SUCCESS)
		log_fatal("Can't initialize OMAPI: %s",
			   isc_result_totext(status));

	/* Set up the OMAPI wrappers for the interface object. */
	interface_setup();

    local_family = AF_INET;
    quiet_interface_discovery = 1;

    status = interface_allocate(&tmp, MDL);
    if (status != ISC_R_SUCCESS)
    	log_fatal("%s: interface_allocate: %s",
    		  "eth",
    		  isc_result_totext(status));

    strcpy(tmp->name, ISTAX_IFNAME);
    interface_snorf(tmp, INTERFACE_REQUESTED);
    interface_dereference(&tmp, MDL);
    
    drop_agent_mismatches = 1;

	/* Set default port */
	if (local_family == AF_INET) {
		port_local = htons(67);
 		port_remote = htons(68);
	}
#ifdef DHCPv6
	else {
		port_local = htons(547);
		port_remote = htons(546);
	}
#endif
    local_port = port_local;
    remote_port = port_remote;

	if (local_family == AF_INET) {
#ifndef ISCDHCP_ISTAX_PLATFORM
		/* We need at least one server */
		if (servers == NULL) {
			log_fatal("No servers specified.");
		}
#endif /* ISCDHCP_ISTAX_PLATFORM */

		/* Set up the server sockaddrs. */
		for (sp = servers; sp; sp = sp->next) {
			sp->to.sin_port = local_port;
			sp->to.sin_family = AF_INET;
#ifdef HAVE_SA_LEN
			sp->to.sin_len = sizeof sp->to;
#endif
		}
	}

	/* Get the current time... */
	gettimeofday(&cur_tv, NULL);

	/* Discover all the network interfaces. */
	T_D("Discovering interfaces");
	discover_interfaces(DISCOVER_RELAY);

#ifdef DHCPv6
	if (local_family == AF_INET6)
		setup_streams();
#endif

	/* Set up the packet handler... */
	if (local_family == AF_INET)
		bootp_packet_handler = do_relay4;
#ifdef DHCPv6
	else
		dhcpv6_packet_handler = do_packet6;
#endif

	/* Start dispatching packets and timeouts... */
	dispatch();

	/* Not reached */
	return (0);
}
#else
int 
main(int argc, char **argv) {
	isc_result_t status;
	struct servent *ent;
	struct server_list *sp = NULL;
	struct interface_info *tmp = NULL;
	char *service_local, *service_remote;
	u_int16_t port_local, port_remote;
	int no_daemon = 0, quiet = 0;
	int fd;
	int i;
#ifdef DHCPv6
	struct stream_list *sl = NULL;
	int local_family_set = 0;
#endif

	/* Make sure that file descriptors 0(stdin), 1,(stdout), and
	   2(stderr) are open. To do this, we assume that when we
	   open a file the lowest available file descriptor is used. */
	fd = open("/dev/null", O_RDWR);
	if (fd == 0)
		fd = open("/dev/null", O_RDWR);
	if (fd == 1)
		fd = open("/dev/null", O_RDWR);
	if (fd == 2)
		log_perror = 0; /* No sense logging to /dev/null. */
	else if (fd != -1)
		close(fd);

	openlog("dhcrelay", LOG_NDELAY, LOG_DAEMON);

#if !defined(DEBUG)
	setlogmask(LOG_UPTO(LOG_INFO));
#endif	

	/* Set up the OMAPI. */
	status = omapi_init();
	if (status != ISC_R_SUCCESS)
		log_fatal("Can't initialize OMAPI: %s",
			   isc_result_totext(status));

	/* Set up the OMAPI wrappers for the interface object. */
	interface_setup();

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-4")) {
#ifdef DHCPv6
			if (local_family_set && (local_family == AF_INET6)) {
				usage();
			}
			local_family_set = 1;
			local_family = AF_INET;
		} else if (!strcmp(argv[i], "-6")) {
			if (local_family_set && (local_family == AF_INET)) {
				usage();
			}
			local_family_set = 1;
			local_family = AF_INET6;
#endif
		} else if (!strcmp(argv[i], "-d")) {
			no_daemon = 1;
		} else if (!strcmp(argv[i], "-q")) {
			quiet = 1;
			quiet_interface_discovery = 1;
		} else if (!strcmp(argv[i], "-p")) {
			if (++i == argc)
				usage();
			local_port = htons(atoi (argv[i]));
#ifdef DEBUG
			log_debug("binding to user-specified port %d",
				  ntohs(local_port));
#endif
		} else if (!strcmp(argv[i], "-c")) {
			int hcount;
			if (++i == argc)
				usage();
			hcount = atoi(argv[i]);
			if (hcount <= 255)
				max_hop_count= hcount;
			else
				usage();
 		} else if (!strcmp(argv[i], "-i")) {
#ifdef DHCPv6
			if (local_family_set && (local_family == AF_INET6)) {
				usage();
			}
			local_family_set = 1;
			local_family = AF_INET;
#endif
			status = interface_allocate(&tmp, MDL);
			if (status != ISC_R_SUCCESS)
				log_fatal("%s: interface_allocate: %s",
					  argv[i],
					  isc_result_totext(status));
			if (++i == argc) {
				usage();
			}
			strcpy(tmp->name, argv[i]);
			interface_snorf(tmp, INTERFACE_REQUESTED);
			interface_dereference(&tmp, MDL);
		} else if (!strcmp(argv[i], "-a")) {
#ifdef DHCPv6
			if (local_family_set && (local_family == AF_INET6)) {
				usage();
			}
			local_family_set = 1;
			local_family = AF_INET;
#endif
			add_agent_options = 1;
		} else if (!strcmp(argv[i], "-A")) {
#ifdef DHCPv6
			if (local_family_set && (local_family == AF_INET6)) {
				usage();
			}
			local_family_set = 1;
			local_family = AF_INET;
#endif
			if (++i == argc)
				usage();

			dhcp_max_agent_option_packet_length = atoi(argv[i]);

			if (dhcp_max_agent_option_packet_length > DHCP_MTU_MAX)
				log_fatal("%s: packet length exceeds "
					  "longest possible MTU\n",
					  argv[i]);
		} else if (!strcmp(argv[i], "-m")) {
#ifdef DHCPv6
			if (local_family_set && (local_family == AF_INET6)) {
				usage();
			}
			local_family_set = 1;
			local_family = AF_INET;
#endif
			if (++i == argc)
				usage();
			if (!strcasecmp(argv[i], "append")) {
				agent_relay_mode = forward_and_append;
			} else if (!strcasecmp(argv[i], "replace")) {
				agent_relay_mode = forward_and_replace;
			} else if (!strcasecmp(argv[i], "forward")) {
				agent_relay_mode = forward_untouched;
			} else if (!strcasecmp(argv[i], "discard")) {
				agent_relay_mode = discard;
			} else
				usage();
		} else if (!strcmp(argv[i], "-D")) {
#ifdef DHCPv6
			if (local_family_set && (local_family == AF_INET6)) {
				usage();
			}
			local_family_set = 1;
			local_family = AF_INET;
#endif
			drop_agent_mismatches = 1;
#ifdef DHCPv6
		} else if (!strcmp(argv[i], "-I")) {
			if (local_family_set && (local_family == AF_INET)) {
				usage();
			}
			local_family_set = 1;
			local_family = AF_INET6;
			use_if_id = ISC_TRUE;
		} else if (!strcmp(argv[i], "-l")) {
			if (local_family_set && (local_family == AF_INET)) {
				usage();
			}
			local_family_set = 1;
			local_family = AF_INET6;
			if (downstreams != NULL)
				use_if_id = ISC_TRUE;
			if (++i == argc)
				usage();
			sl = parse_downstream(argv[i]);
			sl->next = downstreams;
			downstreams = sl;
		} else if (!strcmp(argv[i], "-u")) {
			if (local_family_set && (local_family == AF_INET)) {
				usage();
			}
			local_family_set = 1;
			local_family = AF_INET6;
			if (++i == argc)
				usage();
			sl = parse_upstream(argv[i]);
			sl->next = upstreams;
			upstreams = sl;
#endif
		} else if (!strcmp(argv[i], "--version")) {
			log_info("isc-dhcrelay-%s", PACKAGE_VERSION);
			exit(0);
		} else if (!strcmp(argv[i], "--help") ||
			   !strcmp(argv[i], "-h")) {
			log_info(DHCRELAY_USAGE);
			exit(0);
 		} else if (argv[i][0] == '-') {
			usage();
 		} else {
			struct hostent *he;
			struct in_addr ia, *iap = NULL;

#ifdef DHCPv6
			if (local_family_set && (local_family == AF_INET6)) {
				usage();
			}
			local_family_set = 1;
			local_family = AF_INET;
#endif
			if (inet_aton(argv[i], &ia)) {
				iap = &ia;
			} else {
				he = gethostbyname(argv[i]);
				if (!he) {
					log_error("%s: host unknown", argv[i]);
				} else {
					iap = ((struct in_addr *)
					       he->h_addr_list[0]);
				}
			}

			if (iap) {
				sp = ((struct server_list *)
				      dmalloc(sizeof *sp, MDL));
				if (!sp)
					log_fatal("no memory for server.\n");
				sp->next = servers;
				servers = sp;
				memcpy(&sp->to.sin_addr, iap, sizeof *iap);
			}
 		}
	}

	if (local_family == AF_INET) {
		path_dhcrelay_pid = getenv("PATH_DHCRELAY_PID");
		if (path_dhcrelay_pid == NULL)
			path_dhcrelay_pid = _PATH_DHCRELAY_PID;
	}
#ifdef DHCPv6
	else {
		path_dhcrelay_pid = getenv("PATH_DHCRELAY6_PID");
		if (path_dhcrelay_pid == NULL)
			path_dhcrelay_pid = _PATH_DHCRELAY6_PID;
	}
#endif

	if (!quiet) {
		log_info("%s %s", message, PACKAGE_VERSION);
		log_info(copyright);
		log_info(arr);
		log_info(url);
	} else {
		quiet = 0;
		log_perror = 0;
	}

	/* Set default port */
	if (local_family == AF_INET) {
 		service_local = "bootps";
 		service_remote = "bootpc";
		port_local = htons(67);
 		port_remote = htons(68);
	}
#ifdef DHCPv6
	else {
		service_local = "dhcpv6-server";
		service_remote = "dhcpv6-client";
		port_local = htons(547);
		port_remote = htons(546);
	}
#endif

	if (!local_port) {
		ent = getservbyname(service_local, "udp");
		if (ent)
			local_port = ent->s_port;
		else
			local_port = port_local;

		ent = getservbyname(service_remote, "udp");
		if (ent)
			remote_port = ent->s_port;
		else
			remote_port = port_remote;

		endservent();
	}

	if (local_family == AF_INET) {
		/* We need at least one server */
		if (servers == NULL) {
			log_fatal("No servers specified.");
		}


		/* Set up the server sockaddrs. */
		for (sp = servers; sp; sp = sp->next) {
			sp->to.sin_port = local_port;
			sp->to.sin_family = AF_INET;
#ifdef HAVE_SA_LEN
			sp->to.sin_len = sizeof sp->to;
#endif
		}
	}
#ifdef DHCPv6
	else {
		unsigned code;

		/* We need at least one upstream and one downstream interface */
		if (upstreams == NULL || downstreams == NULL) {
			log_info("Must specify at least one lower "
				 "and one upper interface.\n");
			usage();
		}

		/* Set up the initial dhcp option universe. */
		initialize_common_option_spaces();

		/* Check requested options. */
		code = D6O_RELAY_MSG;
		if (!option_code_hash_lookup(&requested_opts[0],
					     dhcpv6_universe.code_hash,
					     &code, 0, MDL))
			log_fatal("Unable to find the RELAY_MSG "
				  "option definition.");
		code = D6O_INTERFACE_ID;
		if (!option_code_hash_lookup(&requested_opts[1],
					     dhcpv6_universe.code_hash,
					     &code, 0, MDL))
			log_fatal("Unable to find the INTERFACE_ID "
				  "option definition.");
	}
#endif

	/* Get the current time... */
	gettimeofday(&cur_tv, NULL);

	/* Discover all the network interfaces. */
	discover_interfaces(DISCOVER_RELAY);

#ifdef DHCPv6
	if (local_family == AF_INET6)
		setup_streams();
#endif

	/* Become a daemon... */
	if (!no_daemon) {
		int pid;
		FILE *pf;
		int pfdesc;

		log_perror = 0;

		if ((pid = fork()) < 0)
			log_fatal("Can't fork daemon: %m");
		else if (pid)
			exit(0);

		pfdesc = open(path_dhcrelay_pid,
			       O_CREAT | O_TRUNC | O_WRONLY, 0644);

		if (pfdesc < 0) {
			log_error("Can't create %s: %m", path_dhcrelay_pid);
		} else {
			pf = fdopen(pfdesc, "w");
			if (!pf)
				log_error("Can't fdopen %s: %m",
				      path_dhcrelay_pid);
			else {
				fprintf(pf, "%ld\n",(long)getpid());
				fclose(pf);
			}	
		}

		close(0);
		close(1);
		close(2);
		pid = setsid();

		chdir("/");
	}

	/* Set up the packet handler... */
	if (local_family == AF_INET)
		bootp_packet_handler = do_relay4;
#ifdef DHCPv6
	else
		dhcpv6_packet_handler = do_packet6;
#endif

	/* Start dispatching packets and timeouts... */
	dispatch();

	/* Not reached */
	return (0);
}
#endif /* ISCDHCP_ISTAX_PLATFORM */

/* Note: ip is pointing to the same memory as the global variable interfaces, so it should be used with caution. */
static void
do_relay4(struct interface_info *ip, struct dhcp_packet *packet,
	  unsigned int length, unsigned int from_port, struct iaddr from,
	  struct hardware *hfrom) {
	struct server_list *sp;
	struct sockaddr_in to;
	struct interface_info *out;
	struct hardware hto;
#ifndef ISCDHCP_ISTAX_PLATFORM
        struct hardware *htop;
#endif
	struct in_addr server_vlan_ip;


#ifdef ISCDHCP_ISTAX_PLATFORM
	T_D("Enter, packet type %s, xid 0x%x, from_port %u", (packet->op == BOOTREQUEST) ? "BOOTREQUEST" : "BOOTREPLY", packet->xid, from_port);
    T_D("iscdhcp_relay_running=%s", iscdhcp_relay_running ? "T" : "F");
   // Discarding packets that arrived directly trough socket from IP stack and not through helper.
    if (packet->giaddr.s_addr == 0) {
        T_I("Discarding packet");
        return;
    }
    // Packet counter needs to be incremented here, after extra packets that result from socket + helper issue, have been discarded.
     if (packet->op == BOOTREQUEST)
        ++receive_client_packets;
     else if (packet->op == BOOTREPLY) {
        ++receive_server_packets;
     }

    T_D("!relay_running %s, !servers %s, !ip->addresses %s", !iscdhcp_relay_running ? "T" : "F", !servers ? "T" : "F", !ip->addresses ? "T" : "F");
    
    if (!iscdhcp_relay_running || !servers || !ip->addresses) {
	if (!servers) {
	   ++client_packet_errors;
        }
	T_D("Exit on error");
        return;
    }
    /* Ingore the process if the DHCP message from platform. 
       Because the relay agent forward the DHCP message to DHCP server
       via unicast package, the DHCP relay *MUST* running after platform
       IP address active. */
    if (memcmp(iscdhcp_platform_mac, packet->chaddr, 6) == 0)
        return;
#endif /* ISCDHCP_ISTAX_PLATFORM */

	if (packet->hlen > sizeof packet->chaddr) {
		log_info("Discarding packet with invalid hlen.");
		return;
	}

	/* Find the interface that corresponds to the giaddr
	   in the packet. */
	T_D("Giaddr is %s", inet_ntoa(packet->giaddr));
	
	if (packet->giaddr.s_addr) {
		for (out = interfaces; out; out = out->next) {
				int i;
				for (i = 0 ; i < out->address_count ; i++ ) {
					T_D("out->addresses[%d] is %s", i, inet_ntoa(out->addresses[i]));
					if (out->addresses[i].s_addr == packet->giaddr.s_addr) {
						i = -1;
						break;
					}
				}
				if (i == -1)
					break;
		}
		if (!out) {
			T_D("Out variable is null!");
		}
	} else {
            out = NULL;
	}

	/* If it's a bootreply, forward it to the client. */
	if (packet->op == BOOTREPLY) {
		if (!(packet->flags & htons(BOOTP_BROADCAST)) &&
			can_unicast_without_arp(out)) {
			to.sin_addr = packet->yiaddr;
			to.sin_port = remote_port;

#ifndef ISCDHCP_ISTAX_PLATFORM
			/* and hardware address is not broadcast */
			htop = &hto;
#endif
		} else {
			to.sin_addr.s_addr = htonl(INADDR_BROADCAST);
			to.sin_port = remote_port;

#ifndef ISCDHCP_ISTAX_PLATFORM
			/* hardware address is broadcast */
			htop = NULL;
#endif
		}
		to.sin_family = AF_INET;
#ifdef HAVE_SA_LEN
		to.sin_len = sizeof to;
#endif

		memcpy(&hto.hbuf[1], packet->chaddr, packet->hlen);
		hto.hbuf[0] = packet->htype;
		hto.hlen = packet->hlen + 1;

		/* Wipe out the agent relay options and, if possible, figure
		   out which interface to use based on the contents of the
		   option that we put on the request to which the server is
		   replying. */
		if (!(length =
		      strip_relay_agent_options(ip, &out, packet, length))) {
			return;
			}

		if (!out) {
			T_D("Error exit. Out is null.");
			log_error("Packet to bogus giaddr %s.\n",
			      inet_ntoa(packet->giaddr));
			++bogus_giaddr_drops;
			return;
		}

#ifdef ISCDHCP_ISTAX_PLATFORM
		if (iscdhcp_reply_send_client_callback) {
		    if (iscdhcp_reply_send_client_callback((char *)packet, length, &to, packet->chaddr, htonl(packet->xid))){
				T_D("Sending packet to client failed.");
				++server_packet_errors;
			} else {		
				++server_packets_relayed;
			}
		}
#else
		if (send_packet(out, NULL, packet, length, out->addresses[0],
				&to, htop) < 0) {
			T_D("Sending packet to client failed.");
			++server_packet_errors;
		} else {
#ifdef DEBUG
			log_debug("Forwarded BOOTREPLY for %s to %s",
			       print_hw_addr(packet->htype, packet->hlen,
					      packet->chaddr),
			       inet_ntoa(to.sin_addr));
#endif
			++server_packets_relayed;
		}
#endif /* ISCDHCP_ISTAX_PLATFORM */
		T_D("Exit after sending BOOTREPLY\n");
		return;
	}

	/* Add relay agent options if indicated.   If something goes wrong,
	   drop the packet. */
#ifdef ISCDHCP_ISTAX_PLATFORM
		/* Note: this function does not find the giaddr but actually the address of the vlan that communicates to the server. */
        if (iscdhcp_reply_fill_giaddr_callback) {
            iscdhcp_reply_fill_giaddr_callback(packet->chaddr, htonl(packet->xid), (uint32_t *) &server_vlan_ip);
        }
#endif

	if (!(length = add_relay_agent_options(ip, packet, length,
					       ip->addresses[0])))
		return;

	/* If giaddr is not already set, Set it so the server can
	   figure out what net it's from and so that we can later
	   forward the response to the correct net.    If it's already
	   set, the response will be sent directly to the relay agent
	   that set giaddr, so we won't see it. */
	if (!packet->giaddr.s_addr)
		packet->giaddr = ip->addresses[0];
	if (packet->hops < max_hop_count)
		packet->hops = packet->hops + 1;
	else
		return;


#ifdef ISCDHCP_ISTAX_PLATFORM
    T_D("%s fallback_interface", fallback_interface ? "Found" : "Not Found");
	for (sp = servers; sp; sp = sp->next) {
        struct interface_info *out_if;
        for (out_if = interfaces; out_if; out_if = out_if->next) {
	        int i;
	        for (i = 0; i < out_if->address_count; i++) {
                    T_D("out_if->addresses[%d].s_addr=%s, fd=%d", i, inet_ntoa(out_if->addresses[i]), out_if->wfdesc);
                }
        }
#endif /* ISCDHCP_ISTAX_PLATFORM */

	/* Otherwise, it's a BOOTREQUEST, so forward it to all the
	   servers. */
#ifdef ISCDHCP_ISTAX_PLATFORM
		T_D("Relaying BOOTREQUEST to server %s", inet_ntoa(sp->to.sin_addr));
#endif /* ISCDHCP_ISTAX_PLATFORM */

		if (send_packet((fallback_interface
				 ? fallback_interface : interfaces),
				 NULL, packet, length, server_vlan_ip,
				 &sp->to, NULL) < 0) {
			T_D("Sending packet to client failed");
			++client_packet_errors;
		} else {
#ifdef DEBUG
			log_debug("Forwarded BOOTREQUEST for %s to %s",
			       print_hw_addr(packet->htype, packet->hlen,
					      packet->chaddr),
			       inet_ntoa(sp->to.sin_addr));
#endif
			++client_packets_relayed;

			/* Trigger this callback function after send out the DHCP packet successfully.
			   It is used to count the per-port statistic.
			*/
			if (iscdhcp_reply_send_server_callback) {
			       iscdhcp_reply_send_server_callback((char *)packet, length, ntohl(sp->to.sin_addr.s_addr));
			}
		}
	}

	T_D("Exit after sending BOOTREQUEST\n");
				 
}

/* Strip any Relay Agent Information options from the DHCP packet
   option buffer.   If there is a circuit ID suboption, look up the
   outgoing interface based upon it. */

static int
strip_relay_agent_options(struct interface_info *in,
			  struct interface_info **out,
			  struct dhcp_packet *packet,
			  unsigned length) {
	int is_dhcp = 0;
	u_int8_t *op, *nextop, *sp, *max;
	int good_agent_option = 0;
	int status;
	/* If we're not adding agent options to packets, we're not taking
	   them out either. */
	if (!add_agent_options) {
		return (length);
	}

	/* If there's no cookie, it's a bootp packet, so we should just
	   forward it unchanged. */
	if (memcmp(packet->options, DHCP_OPTIONS_COOKIE, 4))
		return (length);

	max = ((u_int8_t *)packet) + length;
	sp = op = &packet->options[4];

	while (op < max) {
		switch(*op) {
			/* Skip padding... */
		      case DHO_PAD:
			if (sp != op)
				*sp = *op;
			++op;
			++sp;
			continue;

			/* If we see a message type, it's a DHCP packet. */
		      case DHO_DHCP_MESSAGE_TYPE:
			is_dhcp = 1;
			goto skip;
			break;

			/* Quit immediately if we hit an End option. */
		      case DHO_END:
			if (sp != op)
				*sp++ = *op++;
			goto out;

		      case DHO_DHCP_AGENT_OPTIONS:
			/* We shouldn't see a relay agent option in a
			   packet before we've seen the DHCP packet type,
			   but if we do, we have to leave it alone. */
			if (!is_dhcp)
				goto skip;

			/* Do not process an agent option if it exceeds the
			 * buffer.  Fail this packet.
			 */
			nextop = op + op[1] + 2;
			if (nextop > max)
				return (0);

			status = find_interface_by_agent_option(packet,
								out, op + 2,
								op[1]);
			if (status == -1 && drop_agent_mismatches)
				return (0);
			if (status)
				good_agent_option = 1;
			op = nextop;
			break;

		      skip:
			/* Skip over other options. */
		      default:
			/* Fail if processing this option will exceed the
			 * buffer(op[1] is malformed).
			 */
			nextop = op + op[1] + 2;
			if (nextop > max)
				return (0);

			if (sp != op) {
				memmove(sp, op, op[1] + 2);
				sp += op[1] + 2;
				op = nextop;
			} else
				op = sp = nextop;

			break;
		}
	}
      out:

	/* If it's not a DHCP packet, we're not supposed to touch it. */
	if (!is_dhcp)
		return (length);

	/* If none of the agent options we found matched, or if we didn't
	   find any agent options, count this packet as not having any
	   matching agent options, and if we're relying on agent options
	   to determine the outgoing interface, drop the packet. */

	if (!good_agent_option) {
		++missing_agent_option;
		if (drop_agent_mismatches)
			return (0);
	}

	/* Adjust the length... */
	if (sp != op) {
		length = sp -((u_int8_t *)packet);

		/* Make sure the packet isn't short(this is unlikely,
		   but WTH) */
		if (length < BOOTP_MIN_LEN) {
			memset(sp, DHO_PAD, BOOTP_MIN_LEN - length);
			length = BOOTP_MIN_LEN;
		}
	}
	return (length);
}


/* Find an interface that matches the circuit ID specified in the
   Relay Agent Information option.   If one is found, store it through
   the pointer given; otherwise, leave the existing pointer alone.

   We actually deviate somewhat from the current specification here:
   if the option buffer is corrupt, we suggest that the caller not
   respond to this packet.  If the circuit ID doesn't match any known
   interface, we suggest that the caller to drop the packet.  Only if
   we find a circuit ID that matches an existing interface do we tell
   the caller to go ahead and process the packet. */

static int
find_interface_by_agent_option(struct dhcp_packet *packet,
			       struct interface_info **out,
			       u_int8_t *buf, int len) {
	int i = 0;
	u_int8_t *circuit_id = 0;
	unsigned circuit_id_len = 0;
	u_int8_t *remote_id = 0;
	unsigned remote_id_len = 0;
	struct interface_info *ip;

	while (i < len) {
		/* If the next agent option overflows the end of the
		   packet, the agent option buffer is corrupt. */
		if (i + 1 == len ||
		    i + buf[i + 1] + 2 > len) {
			++corrupt_agent_options;
			return (-1);
		}
		switch(buf[i]) {
			/* Remember where the circuit ID is... */
		      case RAI_CIRCUIT_ID:
			circuit_id = &buf[i + 2];
			circuit_id_len = buf[i + 1];
			i += circuit_id_len + 2;
			continue;

			/* Remember where the circuit ID is... */
		      case RAI_REMOTE_ID:
			remote_id = &buf[i + 2];
			remote_id_len = buf[i + 1];
			i += remote_id_len + 2;
			continue;

		      default:
			i += buf[i + 1] + 2;
			break;
		}
	}

	/* If there's no circuit ID, it's not really ours, tell the caller
	   it's no good. */
	if (!circuit_id) {
		++missing_circuit_id;
		return (-1);
	}

	/* If there's no remote ID, it's not really ours, tell the caller
	   it's no good. */
	if (!remote_id) {
		++missing_remote_id;
		return (-1);
	}

#ifdef ISCDHCP_ISTAX_PLATFORM
    if (iscdhcp_reply_circuit_id_callback) {
        if (iscdhcp_reply_circuit_id_callback(packet->chaddr, htonl(packet->xid), circuit_id)) {
            /* If we didn't get a match, the circuit ID was bogus. */
    	    ++bad_circuit_id;
        }
	}

	/* Scan the interface list looking for an interface whose
	   name matches the one specified in remote_id. */
    /* peter, 2009/3, it won't match our remote ID, if we keep the oringal option 82 information */
    if (agent_relay_mode == forward_untouched) {
		ip = interfaces;
    } else {
        int remote_id_match = 0;
    	for (ip = interfaces; ip; ip = ip->next) {
    		if (ip->remote_id &&
    		    ip->remote_id_len == remote_id_len &&
    		    !memcmp(ip->remote_id, remote_id, remote_id_len)) {
                remote_id_match = 1;
    			break;
            }
        }
        if (!remote_id_match) {
            /* If we didn't get a match, the remote ID was bogus. */
	        ++bad_remote_id;
        }
	}
#else
	/* Scan the interface list looking for an interface whose
	   name matches the one specified in circuit_id. */

	for (ip = interfaces; ip; ip = ip->next) {
		if (ip->circuit_id &&
		    ip->circuit_id_len == circuit_id_len &&
		    !memcmp(ip->circuit_id, circuit_id, circuit_id_len))
			break;
	}
#endif /* ISCDHCP_ISTAX_PLATFORM */

	/* If we got a match, use it. */
	if (ip) {
		*out = ip;
		return (1);
	}

#ifndef ISCDHCP_ISTAX_PLATFORM
	/* If we didn't get a match, the circuit ID was bogus. */
	++bad_circuit_id;
#endif /* ISCDHCP_ISTAX_PLATFORM */
	return (-1);
}

/*
 * Examine a packet to see if it's a candidate to have a Relay
 * Agent Information option tacked onto its tail.   If it is, tack
 * the option on.
 */
static int
add_relay_agent_options(struct interface_info *ip, struct dhcp_packet *packet,
			unsigned length, struct in_addr giaddr) {
	int is_dhcp = 0, mms;
	unsigned optlen;
	u_int8_t *op, *nextop, *sp, *max, *end_pad = NULL;

	T_D("Enter");

	/* If there's no cookie, it's a bootp packet, so we should just
	   forward it unchanged. */
	if (memcmp(packet->options, DHCP_OPTIONS_COOKIE, 4))
		return (length);

	max = ((u_int8_t *)packet) + dhcp_max_agent_option_packet_length;

	/* Commence processing after the cookie. */
	sp = op = &packet->options[4];

	while (op < max) {
		switch(*op) {
			/* Skip padding... */
		      case DHO_PAD:
			/* Remember the first pad byte so we can commandeer
			 * padded space.
			 *
			 * XXX: Is this really a good idea?  Sure, we can
			 * seemingly reduce the packet while we're looking,
			 * but if the packet was signed by the client then
			 * this padding is part of the checksum(RFC3118),
			 * and its nonpresence would break authentication.
			 */
			if (end_pad == NULL)
				end_pad = sp;

			if (sp != op)
				*sp++ = *op++;
			else
				sp = ++op;

			continue;

			/* If we see a message type, it's a DHCP packet. */
		      case DHO_DHCP_MESSAGE_TYPE:
			is_dhcp = 1;
			goto skip;

			/*
			 * If there's a maximum message size option, we
			 * should pay attention to it
			 */
		      case DHO_DHCP_MAX_MESSAGE_SIZE:
			mms = ntohs(*(op + 2));
			if (mms < dhcp_max_agent_option_packet_length &&
			    mms >= DHCP_MTU_MIN)
				max = ((u_int8_t *)packet) + mms;
			goto skip;

			/* Quit immediately if we hit an End option. */
		      case DHO_END:
			goto out;

		      case DHO_DHCP_AGENT_OPTIONS:
			/* We shouldn't see a relay agent option in a
			   packet before we've seen the DHCP packet type,
			   but if we do, we have to leave it alone. */
			if (!is_dhcp)
				goto skip;

			end_pad = NULL;

			/* There's already a Relay Agent Information option
			   in this packet.   How embarrassing.   Decide what
			   to do based on the mode the user specified. */
			++receive_client_agent_option;
			switch(agent_relay_mode) {
			      case forward_and_append:
    	        if (!add_agent_options) {
    	            /* Drop incoming packet with option 82 when relay information mode
    	               is disabled but relay Information policy is 'Replace' */
				    ++drop_agent_option;
				    return (0);
		        }
				++replace_agent_option;
				goto skip;
			      case forward_untouched:
				++keep_agent_option;
				return (length);
			      case discard:
				++drop_agent_option;
				return (0);
			      case forward_and_replace:
    	        if (!add_agent_options) {
    	            /* Drop incoming packet with option 82 when relay information mode
    	               is disabled but relay Information policy is 'Replace' */
				    ++drop_agent_option;
				    return (0);
		        }
				++replace_agent_option;
			      default:
				break;
			}

			/* Skip over the agent option and start copying
			   if we aren't copying already. */
			op += op[1] + 2;
			break;

		      skip:
			/* Skip over other options. */
		      default:
			/* Fail if processing this option will exceed the
			 * buffer(op[1] is malformed).
			 */
			nextop = op + op[1] + 2;
			if (nextop > max)
				return (0);

			end_pad = NULL;

			if (sp != op) {
				memmove(sp, op, op[1] + 2);
				sp += op[1] + 2;
				op = nextop;
			} else
				op = sp = nextop;

			break;
		}
	}
      out:

	/* If it's not a DHCP packet or won't add agent options,
       we're not supposed to touch it. */
	if (!is_dhcp || !add_agent_options) {
		T_D("Exit, option 82 not added");
		return (length);
	}

	/* If the packet was padded out, we can store the agent option
	   at the beginning of the padding. */

	if (end_pad != NULL)
		sp = end_pad;

	/* Remember where the end of the packet was after parsing
	   it. */
	op = sp;

	/* Sanity check.  Had better not ever happen. */
	if ((ip->circuit_id_len > 255) ||(ip->circuit_id_len < 1))
		log_fatal("Circuit ID length %d out of range [1-255] on "
			  "%s\n", ip->circuit_id_len, ip->name);
	optlen = ip->circuit_id_len + 2;            /* RAI_CIRCUIT_ID + len */

	if (ip->remote_id) {
		if (ip->remote_id_len > 255 || ip->remote_id_len < 1)
			log_fatal("Remote ID length %d out of range [1-255] "
				  "on %s\n", ip->circuit_id_len, ip->name);
		optlen += ip->remote_id_len + 2;    /* RAI_REMOTE_ID + len */
	}

	/* We do not support relay option fragmenting(multiple options to
	 * support an option data exceeding 255 bytes).
	 */
	if ((optlen < 3) ||(optlen > 255))
		log_fatal("Total agent option length(%u) out of range "
			   "[3 - 255] on %s\n", optlen, ip->name);

	/*
	 * Is there room for the option, its code+len, and DHO_END?
	 * If not, forward without adding the option.
	 */
	if (max - sp >= optlen + 3) {
#ifdef DEBUG
		log_debug("Adding %d-byte relay agent option", optlen + 3);
#endif
		/* Okay, cons up *our* Relay Agent Information option. */
		*sp++ = DHO_DHCP_AGENT_OPTIONS;
		*sp++ = optlen;

		/* Copy in the circuit id... */
#ifdef ISCDHCP_ISTAX_PLATFORM
        if (iscdhcpupdate_reply_circuit_id_callback) {
            (void) iscdhcpupdate_reply_circuit_id_callback(packet->chaddr, htonl(packet->xid), ip->circuit_id);
	    }
#endif
		*sp++ = RAI_CIRCUIT_ID;
		*sp++ = ip->circuit_id_len;
		memcpy(sp, ip->circuit_id, ip->circuit_id_len);
		sp += ip->circuit_id_len;

		/* Copy in remote ID... */
		if (ip->remote_id) {
			*sp++ = RAI_REMOTE_ID;
			*sp++ = ip->remote_id_len;
			memcpy(sp, ip->remote_id, ip->remote_id_len);
			sp += ip->remote_id_len;
		}
	} else {
		++agent_option_errors;
		log_error("No room in packet (used %d of %d) "
			  "for %d-byte relay agent option: omitted",
			   (int) (sp - ((u_int8_t *) packet)),
			   (int) (max - ((u_int8_t *) packet)),
			   optlen + 3);
	}

	/*
	 * Deposit an END option unless the packet is full (shouldn't
	 * be possible).
	 */
	if (sp < max)
		*sp++ = DHO_END;

	/* Recalculate total packet length. */
	length = sp -((u_int8_t *)packet);

	/* Make sure the packet isn't short(this is unlikely, but WTH) */
	if (length < BOOTP_MIN_LEN) {
		memset(sp, DHO_PAD, BOOTP_MIN_LEN - length);
		return (BOOTP_MIN_LEN);
	}

	return (length);
}

#ifdef DHCPv6
/*
 * Parse a downstream argument: [address%]interface[#index].
 */
static struct stream_list *
parse_downstream(char *arg) {
	struct stream_list *dp, *up;
	struct interface_info *ifp = NULL;
	char *ifname, *addr, *iid;
	isc_result_t status;

	if (!supports_multiple_interfaces(ifp) &&
	    (downstreams != NULL))
		log_fatal("No support for multiple interfaces.");

	/* Decode the argument. */
	ifname = strchr(arg, '%');
	if (ifname == NULL) {
		ifname = arg;
		addr = NULL;
	} else {
		*ifname++ = '\0';
		addr = arg;
	}
	iid = strchr(ifname, '#');
	if (iid != NULL) {
		*iid++ = '\0';
	}
	if (strlen(ifname) >= sizeof(ifp->name)) {
		log_error("Interface name '%s' too long", ifname);
		usage();
	}

	/* Don't declare twice. */
	for (dp = downstreams; dp; dp = dp->next) {
		if (strcmp(ifname, dp->ifp->name) == 0)
			log_fatal("Down interface '%s' declared twice.",
				  ifname);
	}

	/* Share with up side? */
	for (up = upstreams; up; up = up->next) {
		if (strcmp(ifname, up->ifp->name) == 0) {
			log_info("Interface '%s' is both down and up.",
				 ifname);
			ifp = up->ifp;
			break;
		}
	}

	/* New interface. */
	if (ifp == NULL) {
		status = interface_allocate(&ifp, MDL);
		if (status != ISC_R_SUCCESS)
			log_fatal("%s: interface_allocate: %s",
				  arg, isc_result_totext(status));
		strcpy(ifp->name, ifname);
		if (interfaces) {
			interface_reference(&ifp->next, interfaces, MDL);
			interface_dereference(&interfaces, MDL);
		}
		interface_reference(&interfaces, ifp, MDL);
		ifp->flags |= INTERFACE_REQUESTED | INTERFACE_DOWNSTREAM;
	}

	/* New downstream. */
	dp = (struct stream_list *) dmalloc(sizeof(*dp), MDL);
	if (!dp)
		log_fatal("No memory for downstream.");
	dp->ifp = ifp;
	if (iid != NULL) {
		dp->id = atoi(iid);
	} else {
		dp->id = -1;
	}
	/* !addr case handled by setup. */
	if (addr && (inet_pton(AF_INET6, addr, &dp->link.sin6_addr) <= 0))
		log_fatal("Bad link address '%s'", addr);

	return dp;
}

/*
 * Parse an upstream argument: [address]%interface.
 */
static struct stream_list *
parse_upstream(char *arg) {
	struct stream_list *up, *dp;
	struct interface_info *ifp = NULL;
	char *ifname, *addr;
	isc_result_t status;

	/* Decode the argument. */
	ifname = strchr(arg, '%');
	if (ifname == NULL) {
		ifname = arg;
		addr = All_DHCP_Servers;
	} else {
		*ifname++ = '\0';
		addr = arg;
	}
	if (strlen(ifname) >= sizeof(ifp->name)) {
		log_fatal("Interface name '%s' too long", ifname);
	}

	/* Shared up interface? */
	for (up = upstreams; up; up = up->next) {
		if (strcmp(ifname, up->ifp->name) == 0) {
			ifp = up->ifp;
			break;
		}
	}
	for (dp = downstreams; dp; dp = dp->next) {
		if (strcmp(ifname, dp->ifp->name) == 0) {
			ifp = dp->ifp;
			break;
		}
	}

	/* New interface. */
	if (ifp == NULL) {
		status = interface_allocate(&ifp, MDL);
		if (status != ISC_R_SUCCESS)
			log_fatal("%s: interface_allocate: %s",
				  arg, isc_result_totext(status));
		strcpy(ifp->name, ifname);
		if (interfaces) {
			interface_reference(&ifp->next, interfaces, MDL);
			interface_dereference(&interfaces, MDL);
		}
		interface_reference(&interfaces, ifp, MDL);
		ifp->flags |= INTERFACE_REQUESTED | INTERFACE_UPSTREAM;
	}

	/* New upstream. */
	up = (struct stream_list *) dmalloc(sizeof(*up), MDL);
	if (up == NULL)
		log_fatal("No memory for upstream.");

	up->ifp = ifp;

	if (inet_pton(AF_INET6, addr, &up->link.sin6_addr) <= 0)
		log_fatal("Bad address %s", addr);

	return up;
}

/*
 * Setup downstream interfaces.
 */
static void
setup_streams(void) {
	struct stream_list *dp, *up;
	int i;
	isc_boolean_t link_is_set;

	for (dp = downstreams; dp; dp = dp->next) {
		/* Check interface */
		if (dp->ifp->v6address_count == 0)
			log_fatal("Interface '%s' has no IPv6 addresses.",
				  dp->ifp->name);

		/* Check/set link. */
		if (IN6_IS_ADDR_UNSPECIFIED(&dp->link.sin6_addr))
			link_is_set = ISC_FALSE;
		else
			link_is_set = ISC_TRUE;
		for (i = 0; i < dp->ifp->v6address_count; i++) {
			if (IN6_IS_ADDR_LINKLOCAL(&dp->ifp->v6addresses[i]))
				continue;
			if (!link_is_set)
				break;
			if (!memcmp(&dp->ifp->v6addresses[i],
				    &dp->link.sin6_addr,
				    sizeof(dp->link.sin6_addr)))
				break;
		}
		if (i == dp->ifp->v6address_count)
			log_fatal("Can't find link address for interface '%s'.",
				  dp->ifp->name);
		if (!link_is_set)
			memcpy(&dp->link.sin6_addr,
			       &dp->ifp->v6addresses[i],
			       sizeof(dp->link.sin6_addr));

		/* Set interface-id. */
		if (dp->id == -1)
			dp->id = dp->ifp->index;
	}

	for (up = upstreams; up; up = up->next) {
		up->link.sin6_port = local_port;
		up->link.sin6_family = AF_INET6;
#ifdef HAVE_SA_LEN
		up->link.sin6_len = sizeof(up->link);
#endif

		if (up->ifp->v6address_count == 0)
			log_fatal("Interface '%s' has no IPv6 addresses.",
				  up->ifp->name);
	}
}

/*
 * Add DHCPv6 agent options here.
 */
static const int required_forw_opts[] = {
	D6O_INTERFACE_ID,
	D6O_RELAY_MSG,
	0
};

/*
 * Process a packet upwards, i.e., from client to server.
 */
static void
process_up6(struct packet *packet, struct stream_list *dp) {
	char forw_data[65535];
	unsigned cursor;
	struct dhcpv6_relay_packet *relay;
	struct option_state *opts;
	struct stream_list *up;

	/* Check if the message should be relayed to the server. */
	switch (packet->dhcpv6_msg_type) {
	      case DHCPV6_SOLICIT:
	      case DHCPV6_REQUEST:
	      case DHCPV6_CONFIRM:
	      case DHCPV6_RENEW:
	      case DHCPV6_REBIND:
	      case DHCPV6_RELEASE:
	      case DHCPV6_DECLINE:
	      case DHCPV6_INFORMATION_REQUEST:
	      case DHCPV6_RELAY_FORW:
	      case DHCPV6_LEASEQUERY:
		log_info("Relaying %s from %s port %d going up.",
			 dhcpv6_type_names[packet->dhcpv6_msg_type],
			 piaddr(packet->client_addr),
			 ntohs(packet->client_port));
		break;

	      case DHCPV6_ADVERTISE:
	      case DHCPV6_REPLY:
	      case DHCPV6_RECONFIGURE:
	      case DHCPV6_RELAY_REPL:
	      case DHCPV6_LEASEQUERY_REPLY:
		log_info("Discarding %s from %s port %d going up.",
			 dhcpv6_type_names[packet->dhcpv6_msg_type],
			 piaddr(packet->client_addr),
			 ntohs(packet->client_port));
		return;

	      default:
		log_info("Unknown %d type from %s port %d going up.",
			 packet->dhcpv6_msg_type,
			 piaddr(packet->client_addr),
			 ntohs(packet->client_port));
		return;
	}

	/* Build the relay-forward header. */
	relay = (struct dhcpv6_relay_packet *) forw_data;
	cursor = sizeof(*relay);
	relay->msg_type = DHCPV6_RELAY_FORW;
	if (packet->dhcpv6_msg_type == DHCPV6_RELAY_FORW) {
		if (packet->dhcpv6_hop_count >= max_hop_count) {
			log_info("Hop count exceeded,");
			return;
		}
		relay->hop_count = packet->dhcpv6_hop_count + 1;
		if (dp) {
			memcpy(&relay->link_address, &dp->link.sin6_addr, 16);
		} else {
			/* On smart relay add: && !global. */
			if (!use_if_id && downstreams->next) {
				log_info("Shan't get back the interface.");
				return;
			}
			memset(&relay->link_address, 0, 16);
		}
	} else {
		relay->hop_count = 0;
		if (!dp)
			return;
		memcpy(&relay->link_address, &dp->link.sin6_addr, 16);
	}
	memcpy(&relay->peer_address, packet->client_addr.iabuf, 16);

	/* Get an option state. */
	opts = NULL;
	if (!option_state_allocate(&opts, MDL)) {
		log_fatal("No memory for upwards options.");
	}
	
	/* Add an interface-id (if used). */
	if (use_if_id) {
		int if_id;

		if (dp) {
			if_id = dp->id;
		} else if (!downstreams->next) {
			if_id = downstreams->id;
		} else {
			log_info("Don't know the interface.");
			option_state_dereference(&opts, MDL);
			return;
		}

		if (!save_option_buffer(&dhcpv6_universe, opts,
					NULL, (unsigned char *) &if_id,
					sizeof(int),
					D6O_INTERFACE_ID, 0)) {
			log_error("Can't save interface-id.");
			option_state_dereference(&opts, MDL);
			return;
		}
	}

	/* Add the relay-msg carrying the packet. */
	if (!save_option_buffer(&dhcpv6_universe, opts,
				NULL, (unsigned char *) packet->raw,
				packet->packet_length,
				D6O_RELAY_MSG, 0)) {
		log_error("Can't save relay-msg.");
		option_state_dereference(&opts, MDL);
		return;
	}

	/* Finish the relay-forward message. */
	cursor += store_options6(forw_data + cursor,
				 sizeof(forw_data) - cursor,
				 opts, packet, 
				 required_forw_opts, NULL);
	option_state_dereference(&opts, MDL);

	/* Send it to all upstreams. */
	for (up = upstreams; up; up = up->next) {
		send_packet6(up->ifp, (unsigned char *) forw_data,
			     (size_t) cursor, &up->link);
	}
}
			     
/*
 * Process a packet downwards, i.e., from server to client.
 */
static void
process_down6(struct packet *packet) {
	struct stream_list *dp;
	struct option_cache *oc;
	struct data_string relay_msg;
	const struct dhcpv6_packet *msg;
	struct data_string if_id;
	struct sockaddr_in6 to;
	struct iaddr peer;

	/* The packet must be a relay-reply message. */
	if (packet->dhcpv6_msg_type != DHCPV6_RELAY_REPL) {
		if (packet->dhcpv6_msg_type < dhcpv6_type_name_max)
			log_info("Discarding %s from %s port %d going down.",
				 dhcpv6_type_names[packet->dhcpv6_msg_type],
				 piaddr(packet->client_addr),
				 ntohs(packet->client_port));
		else
			log_info("Unknown %d type from %s port %d going down.",
				 packet->dhcpv6_msg_type,
				 piaddr(packet->client_addr),
				 ntohs(packet->client_port));
		return;
	}

	/* Inits. */
	memset(&relay_msg, 0, sizeof(relay_msg));
	memset(&if_id, 0, sizeof(if_id));
	memset(&to, 0, sizeof(to));
	to.sin6_family = AF_INET6;
#ifdef HAVE_SA_LEN
	to.sin6_len = sizeof(to);
#endif
	to.sin6_port = remote_port;
	peer.len = 16;

	/* Get the relay-msg option (carrying the message to relay). */
	oc = lookup_option(&dhcpv6_universe, packet->options, D6O_RELAY_MSG);
	if (oc == NULL) {
		log_info("No relay-msg.");
		return;
	}
	if (!evaluate_option_cache(&relay_msg, packet, NULL, NULL,
				   packet->options, NULL,
				   &global_scope, oc, MDL) ||
	    (relay_msg.len < sizeof(struct dhcpv6_packet))) {
		log_error("Can't evaluate relay-msg.");
		return;
	}
	msg = (const struct dhcpv6_packet *) relay_msg.data;

	/* Get the interface-id (if exists) and the downstream. */
	oc = lookup_option(&dhcpv6_universe, packet->options,
			   D6O_INTERFACE_ID);
	if (oc != NULL) {
		int if_index;

		if (!evaluate_option_cache(&if_id, packet, NULL, NULL,
					   packet->options, NULL,
					   &global_scope, oc, MDL) ||
		    (if_id.len != sizeof(int))) {
			log_info("Can't evaluate interface-id.");
			goto cleanup;
		}
		memcpy(&if_index, if_id.data, sizeof(int));
		for (dp = downstreams; dp; dp = dp->next) {
			if (dp->id == if_index)
				break;
		}
	} else {
		if (use_if_id) {
			/* Require an interface-id. */
			log_info("No interface-id.");
			goto cleanup;
		}
		for (dp = downstreams; dp; dp = dp->next) {
			/* Get the first matching one. */
			if (!memcmp(&dp->link.sin6_addr,
				    &packet->dhcpv6_link_address,
				    sizeof(struct in6_addr)))
				break;
		}
	}
	/* Why bother when there is no choice. */
	if (!dp && !downstreams->next)
		dp = downstreams;
	if (!dp) {
		log_info("Can't find the down interface.");
		goto cleanup;
	}
	memcpy(peer.iabuf, &packet->dhcpv6_peer_address, peer.len);
	to.sin6_addr = packet->dhcpv6_peer_address;

	/* Check if we should relay the carried message. */
	switch (msg->msg_type) {
		/* Relay-Reply of for another relay, not a client. */
	      case DHCPV6_RELAY_REPL:
		to.sin6_port = local_port;
		/* Fall into: */

	      case DHCPV6_ADVERTISE:
	      case DHCPV6_REPLY:
	      case DHCPV6_RECONFIGURE:
	      case DHCPV6_RELAY_FORW:
	      case DHCPV6_LEASEQUERY_REPLY:
		log_info("Relaying %s to %s port %d down.",
			 dhcpv6_type_names[msg->msg_type],
			 piaddr(peer),
			 ntohs(to.sin6_port));
		break;

	      case DHCPV6_SOLICIT:
	      case DHCPV6_REQUEST:
	      case DHCPV6_CONFIRM:
	      case DHCPV6_RENEW:
	      case DHCPV6_REBIND:
	      case DHCPV6_RELEASE:
	      case DHCPV6_DECLINE:
	      case DHCPV6_INFORMATION_REQUEST:
	      case DHCPV6_LEASEQUERY:
		log_info("Discarding %s to %s port %d down.",
			 dhcpv6_type_names[msg->msg_type],
			 piaddr(peer),
			 ntohs(to.sin6_port));
		goto cleanup;

	      default:
		log_info("Unknown %d type to %s port %d down.",
			 msg->msg_type,
			 piaddr(peer),
			 ntohs(to.sin6_port));
		goto cleanup;
	}

	/* Send the message to the downstream. */
	send_packet6(dp->ifp, (unsigned char *) relay_msg.data,
		     (size_t) relay_msg.len, &to);

      cleanup:
	if (relay_msg.data != NULL)
		data_string_forget(&relay_msg, MDL);
	if (if_id.data != NULL)
		data_string_forget(&if_id, MDL);
}

/*
 * Called by the dispatch packet handler with a decoded packet.
 */
void
dhcpv6(struct packet *packet) {
	struct stream_list *dp;

	/* Try all relay-replies downwards. */
	if (packet->dhcpv6_msg_type == DHCPV6_RELAY_REPL) {
		process_down6(packet);
		return;
	}
	/* Others are candidates to go up if they come from down. */
	for (dp = downstreams; dp; dp = dp->next) {
		if (packet->interface != dp->ifp)
			continue;
		process_up6(packet, dp);
		return;
	}
	/* Relay-forward could work from an unknown interface. */
	if (packet->dhcpv6_msg_type == DHCPV6_RELAY_FORW) {
		process_up6(packet, NULL);
		return;
	}

	log_info("Can't process packet from interface '%s'.",
		 packet->interface->name);
}
#endif

/* Stub routines needed for linking with DHCP libraries. */
void
bootp(struct packet *packet) {
	return;
}

void
dhcp(struct packet *packet) {
	return;
}

void
classify(struct packet *p, class_t *c) {
	return;
}

int
check_collection(struct packet *p, struct lease *l, struct collection *c) {
	return 0;
}

isc_result_t
find_class(class_t **cls, const char *c1, const char *c2, int i) {
	return ISC_R_NOTFOUND;
}

int
parse_allow_deny(struct option_cache **oc, struct parse *p, int i) {
	return 0;
}

isc_result_t
dhcp_set_control_state(control_object_state_t oldstate,
		       control_object_state_t newstate) {
	return ISC_R_SUCCESS;
}

/* Get counters */
void iscdhcp_get_couters(iscdhcp_relay_counter_t *counters) {
    counters->server_packets_relayed	    = server_packets_relayed;
    counters->server_packet_errors  	    = server_packet_errors;
    counters->client_packets_relayed        = client_packets_relayed;
    counters->client_packet_errors  	    = client_packet_errors;
    counters->agent_option_errors           = agent_option_errors;
    counters->missing_agent_option  	    = missing_agent_option;
    counters->bad_circuit_id		        = bad_circuit_id;
    counters->missing_circuit_id	        = missing_circuit_id;
    counters->bad_remote_id 		        = bad_remote_id;
    counters->missing_remote_id 	        = missing_remote_id;
    counters->receive_server_packets        = receive_server_packets;
    counters->receive_client_packets        = receive_client_packets;
    counters->receive_client_agent_option   = receive_client_agent_option;
    counters->replace_agent_option          = replace_agent_option;
    counters->keep_agent_option             = keep_agent_option;
    counters->drop_agent_option             = drop_agent_option;
}

/* Set counters */
void iscdhcp_set_couters(iscdhcp_relay_counter_t *counters) {
    server_packets_relayed      = counters->server_packets_relayed;
    server_packet_errors        = counters->server_packet_errors;
    client_packets_relayed      = counters->client_packets_relayed;
    client_packet_errors        = counters->client_packet_errors;
    agent_option_errors         = counters->agent_option_errors;
    missing_agent_option        = counters->missing_agent_option;
    bad_circuit_id              = counters->bad_circuit_id;
    missing_circuit_id          = counters->missing_circuit_id;
    bad_remote_id               = counters->bad_remote_id;
    missing_remote_id           = counters->missing_remote_id;
    receive_server_packets      = counters->receive_server_packets;
    receive_client_packets      = counters->receive_client_packets;
    receive_client_agent_option = counters->receive_client_agent_option;
    replace_agent_option        = counters->replace_agent_option;
    keep_agent_option           = counters->keep_agent_option;
    drop_agent_option           = counters->drop_agent_option;
}

/* Clear counters */
void iscdhcp_clear_couters(void) {
    server_packets_relayed      = 0;
    server_packet_errors        = 0;
    client_packets_relayed      = 0;
    client_packet_errors        = 0;
    agent_option_errors         = 0;
    missing_agent_option        = 0;
    bad_circuit_id              = 0;
    missing_circuit_id          = 0;
    bad_remote_id               = 0;
    missing_remote_id           = 0;
    receive_server_packets      = 0;
    receive_client_packets      = 0;
    receive_client_agent_option = 0;
    replace_agent_option        = 0;
    keep_agent_option           = 0;
    drop_agent_option           = 0;
}

#if defined(__GNUC__) && __GNUC__ >= 6
#pragma GCC diagnostic pop
#endif

