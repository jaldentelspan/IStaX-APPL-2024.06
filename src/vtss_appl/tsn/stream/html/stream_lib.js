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

/******************************************************************************/
// stream_lib_dmac_match_type_map
/******************************************************************************/
var stream_lib_dmac_match_type_map = {
    any:          "Any",
    multicast:    "Multicast",
    broadcast:    "Broadcast",
    unicast:      "Unicast",
    notBroadcast: "Not Broadcast",
    notUnicast:   "Not Unicast",
    valueMask:    "MAC/Mask"
};

/******************************************************************************/
// stream_lib_smac_match_type_map
/******************************************************************************/
var stream_lib_smac_match_type_map = {
    any:       "Any",
    valueMask: "MAC/Mask"
};

/******************************************************************************/
// stream_lib_port_map
// Dynamically filled
/******************************************************************************/
var stream_lib_port_map;

/******************************************************************************/
// stream_lib_mac_is_zero()
/******************************************************************************/
function stream_lib_mac_is_zero(mac)
{
    if (mac == "00:00:00:00:00:00") {
        return true;
    }

    return false;
}

/******************************************************************************/
// stream_lib_mac_is_broadcast()
/******************************************************************************/
function stream_lib_mac_is_broadcast(mac)
{
    if (mac == "FF:FF:FF:FF:FF:FF") {
        return true;
    }

    return false;
}

/******************************************************************************/
// stream_lib_vlan_tag_match_map
/******************************************************************************/
var stream_lib_vlan_tag_match_map = {
    both:     "Optional",
    untagged: "Not Allowed",
    tagged:   "Required"
};

/******************************************************************************/
// stream_lib_vlan_tag_type_map
/******************************************************************************/
var stream_lib_vlan_tag_type_map = {
    any:     "Any",
    cTagged: "C-tag",
    sTagged: "S-tag"
};

/******************************************************************************/
// stream_lib_vlan_pcp_map
/******************************************************************************/
var stream_lib_vlan_pcp_map = {
    0: "0",
    1: "1",
    2: "2",
    3: "3",
    4: "4",
    5: "5",
    6: "6",
    7: "7"
};

/******************************************************************************/
// stream_lib_vlan_pcp_mask_map
/******************************************************************************/
var stream_lib_vlan_pcp_mask_map = {
    0: "0x0",
    1: "0x1",
    2: "0x2",
    3: "0x3",
    4: "0x4",
    5: "0x5",
    6: "0x6",
    7: "0x7"
};

/******************************************************************************/
// stream_lib_vcap_bit_map
/******************************************************************************/
var stream_lib_vcap_bit_map = {
    "any":  "Any",
    "zero": "0",
    "one":  "1"
};

/******************************************************************************/
// stream_lib_vcap_bit_yes_no_map
/******************************************************************************/
var stream_lib_vcap_bit_yes_no_map = {
    "any":  "Any",
    "zero": "No",
    "one":  "Yes"
};

/******************************************************************************/
// stream_lib_protocol_type_map
/******************************************************************************/
var stream_lib_protocol_type_map = {
    "any":   "Any",
    "etype": "EtherType",
    "llc":   "LLC",
    "snap":  "SNAP",
    "ipv4":  "IPv4",
    "ipv6":  "IPv6"
};

/******************************************************************************/
// stream_lib_snap_oui_type_map
/******************************************************************************/
var stream_lib_snap_oui_type_map = {
    "rfc1042": "RFC1042",
    "dot1h":   "802.1H",
    "custom":  "Custom"
};

/******************************************************************************/
// stream_lib_match_type_map
/******************************************************************************/
var stream_lib_match_type_map = {
    "any":   "Any",
    "value": "Value",
    "range": "Range"
};

/******************************************************************************/
// stream_lib_ip_proto_type_map
/******************************************************************************/
var stream_lib_ip_proto_type_map = {
    "any":    "Any",
    "tcp":    "TCP",
    "udp":    "UDP",
    "custom": "Custom"
};

/******************************************************************************/
// stream_lib_oper_warnings_map
/******************************************************************************/
var stream_lib_oper_warnings_map = {
    WarningNone:                  "None",
    WarningNotInstalledOnAnyPort: "The stream does not have any member ports"
};

/******************************************************************************/
// stream_lib_collection_oper_warnings_map
/******************************************************************************/
var stream_lib_collection_oper_warnings_map = {
    WarningNone:                        "None",
    WarningNoStreamsAttached:           "No streams attached",
    WarningNoClientsAttached:           "No clients attached",
    WarningAtLeastOneStreamHasOperWarn: "At least one of the attached streams has configurational warnings"
};

/******************************************************************************/
// stream_lib_oper_warnings_to_str()
/******************************************************************************/
function stream_lib_oper_warnings_to_str(statu)
{
    var str;

    if (!statu) {
        return "Not created";
    } else {
        str = "";
        Object.keys(stream_lib_oper_warnings_map).forEach(function(key) {
            if (statu[key]) {
                str += (str.length ? "\n" : "") + stream_lib_oper_warnings_map[key];
            }
        });

        return str;
    }
}

/******************************************************************************/
// stream_lib_collection_oper_warnings_to_str()
/******************************************************************************/
function stream_lib_collection_oper_warnings_to_str(statu)
{
    var str;

    if (!statu) {
        return "Not created";
    } else {
        str = "";
        Object.keys(stream_lib_collection_oper_warnings_map).forEach(function(key) {
            if (statu[key]) {
                str += (str.length ? "\n" : "") + stream_lib_collection_oper_warnings_map[key];
            }
        });

        return str;
    }
}

/******************************************************************************/
// stream_lib_oper_warnings_to_image()
/******************************************************************************/
function stream_lib_oper_warnings_to_image(statu)
{
    if (!statu) {
        return "images/led-off.gif"; // Gray
    } else if (statu.WarningNone) {
        return "images/led-up.gif"; // Green
    } else {
        return "images/led-yellow.gif"; // Yellow
    }
}

/******************************************************************************/
// stream_lib_collection_attached_clients_to_str()
/******************************************************************************/
function stream_lib_collection_attached_clients_to_str(statu)
{
    var clients = ["psfp", "frer"], str = "", client, fld;

    clients.forEach(function(client) {
        fld = client + "ClientAttached";
        if (statu && statu[fld]) {
            str += (str.length ? ", " : "") + client.toUpperCase() + " (" + statu[client + "ClientId"] + ")";
        }
    });

    if (!str.length) {
        return "None";
    }

    return str;
}

/******************************************************************************/
// stream_lib_attached_clients_to_str()
/******************************************************************************/
function stream_lib_attached_clients_to_str(statu)
{
    if (statu && statu.StreamCollectionId != "0") {
        return "-";
    }

    // The remaining is identical to that of stream collections
    return stream_lib_collection_attached_clients_to_str(statu);
}

/******************************************************************************/
// stream_lib_stream_collection_cell_get()
/******************************************************************************/
function stream_lib_stream_collection_cell_get(statu)
{
    if (!statu || statu.StreamCollectionId == "0") {
        return {type: "text", params: ["None"]};
    } else {
        return {type: "link", params: ["cl", "stream_collection_config.htm", statu.StreamCollectionId]};
    }
}

/******************************************************************************/
// stream_lib_d2h()
// Converts a decimal number to a hexadecimal with leading zeros up to 'width'.
/******************************************************************************/
function stream_lib_d2h(d, width)
{
    var h = d.toString(16);
    var l = h.length;
    var i;

    for (i = l; i < width; i++) {
        h = "0" + h;
    }

    return "0x" + h.toUpperCase();
}

/******************************************************************************/
// stream_lib_ids_to_list()
/******************************************************************************/
function stream_lib_ids_to_list(coll_conf, print_dash)
{
    var i, j, first_val, second_val, temp_val, comma = "", stream_id_list = "", max;

    // Find maximum stream ID. We do this in order to share this function
    // between the stream module and FRER. Otherwise, we could have used
    // globals.coll_capabilities.StreamsPerCollectionMax.
    for (i = 0; i < 1000; i++) {
        if (coll_conf["StreamId" + i] == undefined) {
            break;
        }
    }

    max = i;

    // We know the stream ID list is already sorted numerically.
    i = 0;
    while (i < max) {
        first_val = coll_conf["StreamId" + i];
        if (first_val === 0) {
            // Invariant: The first occurrence of 0 marks the end of the list.
            break;
        }

        second_val = first_val;
        j = i + 1;
        while (j < max) {
            temp_val = coll_conf["StreamId" + j];

            if (temp_val == second_val + 1) {
                second_val = temp_val;
                j++;
            } else {
                // Not a consecutive number. Get out of here.
                break;
            }
        }

        stream_id_list += comma;
        comma = ",";

        if (first_val == second_val) {
            // Not a range
            stream_id_list += first_val;
            i++;
        } else {
            // A range
            stream_id_list += first_val + "-" + second_val;
            i = j;
        }
    }

    if (print_dash && stream_id_list.length === 0) {
        return "-";
    }

    return stream_id_list;
}

/******************************************************************************/
// stream_lib_remove_ws()
// Returns (in x.str) a string where all white space is removed.
// If two consecutive numbers are only having white space between them,
// the function returns false. Otherwise true.
/******************************************************************************/
function stream_lib_remove_ws(x)
{
    var result = "", whitespace_seen = false;

    while (x.idx < x.len) {
        var c = x.str.charAt(x.idx);

        if (c != ' ') {
            if (whitespace_seen && result.length > 0 && result.charAt(result.length - 1) >= '0' && result.charAt(result.length - 1) <= '9' && c >= '0' && c <= '9') {
                // Two numbers with a space between them is not allowed.
                return false;
            }

            result += c;
            whitespace_seen = false;
        } else {
            whitespace_seen = true;
        }

        x.idx++;
    }

    x.str = result;
    x.idx = 0;
    x.len = x.str.length;
    return true;
}

/******************************************************************************/
// stream_lib_strtoul()
/******************************************************************************/
function stream_lib_strtoul(x)
{
    var len = x.str.length, sub = "", c;

    if (x.idx == x.len) {
        // No more characters in string
        return -1;
    }

    while (x.idx < x.len) {
        c = x.str.charAt(x.idx);

        if (c < '0' || c > '9') {
            break;
        }

        x.idx++;
        sub += c;
    }

    if (sub.length === 0) {
        return -1;
    }

    return parseInt(sub, 10);
}

/******************************************************************************/
// stream_lib_array_sort_and_uniquify()
/******************************************************************************/
function stream_lib_array_sort_and_uniquify(a)
{
    // If we didn't provide an inline  function here, it would assume string
    // comparison rather than integer comparison (for some odd reason).
    a.sort(function(x, y) {
        return x - y;
    });

    return a.filter(function(item, pos, ary) {
        return !pos || item != ary[pos - 1];
    });
}

/******************************************************************************/
// stream_lib_list_to_ids()
/******************************************************************************/
function stream_lib_list_to_ids(key, result, max_id, max_cnt)
{
    var fld = $(key), x = {str: fld.value}, range, comma, start, n, local_result, local_result2, i;

    x.idx = 0;
    x.len = x.str.length;
    local_result = [];

    if (!stream_lib_remove_ws(x)) {
        GiveAlert("Stream IDs must be separated by commas, not spaces", fld);
        return false;
    }

    if (x.idx == x.len) {
        // An empty list is allowed.
        return true;
    }

    range = false;
    comma = false;
    while (x.idx < x.len) {
        n = stream_lib_strtoul(x);

        if (n < 0) {
            GiveAlert("Invalid character ('" + x.str.charAt(x.idx) + "') found in stream ID list", fld);
            return false;
        }

        // Check for valid stream ID.
        if (n < 1 || n > max_id) {
            GiveAlert("Stream IDs must be integers between 1 and " + max_id, fld);
            return false;
        }

        if (range) {
            // End of range has been reached
            range = false;
            if (n < start) {
                GiveAlert("Invalid range detected in stream ID list", fld);
                return false;
            } else {
                for (i = start; i <= n; i++) {
                    local_result.push(i);
                }
            }
        } else if (x.str.charAt(x.idx) == '-') {
            // Start of range
            start = n;
            range = true;
            x.idx++;
        } else {
            local_result.push(n);
        }

        comma = false;
        if (!range && x.str.charAt(x.idx) == ',') {
            comma = true;
            x.idx++;
        }
    }

    // Check for trailing comma/dash
    if (comma || range) {
        GiveAlert("A stream ID list cannot end with a comma or a dash", fld);
        return false;
    }

    // local_result now contains an array of the stream IDs.
    // Get rid of duplicates while sorting the array.
    local_result2 = stream_lib_array_sort_and_uniquify(local_result);

    if (local_result2.length > max_cnt) {
        GiveAlert("At most " + max_cnt + " stream IDs can be specified", fld);
        return false;
    }

    // Convert to what is expected by JSON on the DUT.
    for (i = 0; i < local_result2.length; i++) {
        result["StreamId" + i] = local_result2[i];
    }

    // Add empty at the end.
    for (i = local_result2.length; i < max_cnt; i++) {
        result["StreamId" + i] = 0;
    }

    return true;
}

var stream_lib_json_request_cnt;

/******************************************************************************/
// stream_lib_on_json_received()
/******************************************************************************/
function stream_lib_on_json_received(recv_json, params)
{
    if (!recv_json) {
        alert(params.name + ": Get dynamic data failed.");
        return;
    }

    if (stream_lib_json_request_cnt < 1) {
       alert("Huh? stream_lib_json_request_cnt = " + stream_lib_json_request_cnt);
       stream_lib_json_request_cnt = 1;
    }

    if (params.sel) {
        // A particular instance has been requested. In that case, recv_json
        // contains an error code or the actual result.
        params.req[params.name] = recv_json.result ? JSON.parse(JSON.stringify(recv_json.result)) : undefined;
    } else {
        // We only get the result, and not error codes, etc.
        params.req[params.name] = JSON.parse(JSON.stringify(recv_json));
    }

    stream_lib_json_request_cnt--;

    if (stream_lib_json_request_cnt === 0) {
        // All requests are not fulfilled. Callback caller of
        // stream_lib_json_request()
        params.func();
    }
}

/******************************************************************************/
// stream_lib_json_request()
/******************************************************************************/
function stream_lib_json_request(request, func_to_execute_when_done, selected_inst)
{
    stream_lib_json_request_cnt = Object.keys(request).length;

    if (!stream_lib_json_request_cnt) {
        alert("stream_lib_json_request(): Internal error: Nothing requested");
        return;
    }

    if (request.capabilities) {
        requestJsonDoc("stream.capabilities.get", null, stream_lib_on_json_received, {name: "capabilities", req: request, func: func_to_execute_when_done});
    }

    if (request.coll_capabilities) {
        requestJsonDoc("stream.collectionCapabilities.get", null, stream_lib_on_json_received, {name: "coll_capabilities", req: request, func: func_to_execute_when_done});
    }

    if (request.port_name_map) {
        requestJsonDoc("port.namemap.get", null, stream_lib_on_json_received, {name: "port_name_map", req: request, func: func_to_execute_when_done});
    }

    if (request.default_conf) {
        requestJsonDoc("stream.config.defaultConf.get", null, stream_lib_on_json_received, {name: "default_conf", req: request, func: func_to_execute_when_done});
    }

    if (request.coll_default_conf) {
        requestJsonDoc("stream.config.defaultCollectionConf.get", null, stream_lib_on_json_received, {name: "coll_default_conf", req: request, func: func_to_execute_when_done});
    }

    if (request.conf) {
        requestJsonDoc("stream.config.get", selected_inst, stream_lib_on_json_received, {name: "conf", req: request, sel: selected_inst, func: func_to_execute_when_done}, selected_inst ? true /* always call us back - unless redirecting */ : false);
    }

    if (request.coll_conf) {
        requestJsonDoc("stream.config.collectionConf.get", selected_inst, stream_lib_on_json_received, {name: "coll_conf", req: request, sel: selected_inst, func: func_to_execute_when_done}, selected_inst ? true /* always call us back - unless redirecting */ : false);
    }

    if (request.statu) {
        requestJsonDoc("stream.status.get", selected_inst, stream_lib_on_json_received, {name: "statu", req: request, sel: selected_inst, func: func_to_execute_when_done}, selected_inst ? true /* always call us back - unless redirecting */ : false);
    }

    if (request.coll_statu) {
        requestJsonDoc("stream.status.collectionStatus.get", selected_inst, stream_lib_on_json_received, {name: "coll_statu", req: request, sel: selected_inst, func: func_to_execute_when_done}, selected_inst ? true /* always call us back - unless redirecting */ : false);
    }

    // Cannot prevent this function from returning once requested. Caller must
    // use func_to_execute_when_done() to go on.
}

/******************************************************************************/
// stream_lib_inst_get()
/******************************************************************************/
function stream_lib_inst_get(data, inst)
{
    var i;

    if (!data) {
        return undefined;
    }

    for (i = 0; i < data.length; i++) {
        if (data[i].key == inst) {
            return data[i];
        }
    }

    return undefined;
}

