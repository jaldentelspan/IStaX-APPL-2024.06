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
// redbox_lib_mode_map
/******************************************************************************/
var redbox_lib_mode_map = {
    prpSan: "PRP-SAN",
    hsrSan: "HSR-SAN",
    hsrPrp: "HSR-PRP",
    hsrHsr: "HSR-HSR"
};

/******************************************************************************/
// redbox_lib_port_map
// Dynamically filled
/******************************************************************************/
var redbox_lib_port_map;

/******************************************************************************/
// redbox_lib_net_id_map
/******************************************************************************/
var redbox_lib_net_id_map = {
    1: "1",
    2: "2",
    3: "3",
    4: "4",
    5: "5",
    6: "6",
    7: "7"
};

/******************************************************************************/
// redbox_lib_lan_id_map
/******************************************************************************/
var redbox_lib_lan_id_map = {
    lanA: "A",
    lanB: "B"
};

/******************************************************************************/
// redbox_lib_pcp_map
/******************************************************************************/
var redbox_lib_pcp_map = {
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
// redbox_lib_node_type_map
/******************************************************************************/
var redbox_lib_node_type_map = {
    danP:       "DANP",
    danPRedBox: "DANP-RedBox",
    vDanP:      "VDANP",
    danH:       "DANH",
    danHRedBox: "DANH-RedBox",
    vDanH:      "VDANH",
    san:        "SAN"
};

/******************************************************************************/
// redbox_lib_sv_type_map
/******************************************************************************/
var redbox_lib_sv_type_map = {
    prpDd: "PRP-DD",
    prpDa: "PRP-DA",
    hsr:   "HSR"
};

/******************************************************************************/
// redbox_lib_oper_warnings_map
/******************************************************************************/
var redbox_lib_oper_warnings_map = {
    WarningNone:                                "None",
    WarningMtuTooHighLrePorts:                  "The MTU is too high on at least one of the LRE ports (max is 2000)",
    WarningMtuTooHighNonLrePorts:               "The MTU is too high on at least one non-LRE port. Frames larger than 1994 cannot traverse the HSR/PRP network",
    WarningInterlinkNotCTagged:                 "Interlink port must use C-tags",
    WarningInterlinkNotMemberOfVlan:            "Interlink port is not member of the supervision frame VLAN ID",
    WarningNeighborRedBoxNotConfigured:         "The neighbor RedBox is not configured",
    WarningNeighborRedBoxNotActive:             "The neighbor RedBox is not active",
    WarningNeighborRedBoxPortANotSetToNeighbor: "The neighbor's port A is not configured as a RedBox neighbor",
    WarningNeighborRedBoxPortBNotSetToNeighbor: "The neighbor's port B is not configured as a RedBox neighbor",
    WarningNeighborRedBoxOverlappingVlans:      "The neighbor's interlink port has coinciding VLAN memberships with this RedBox's interlink port",
    WarningStpEnabledInterlink:                 "Interlink port has spanning tree enabled"
};

/******************************************************************************/
// redbox_lib_notif_status_map
/******************************************************************************/
var redbox_lib_notif_status_map = {
    NtPntFull:          "NodesTable/ProxyNodeTable is full",
    PortAWrongLan:      "Port A has received PRP traffic with wrong Lan ID",
    PortBWrongLan:      "Port B has received PRP traffic with wrong Lan ID",
    PortCWrongLan:      "Port C has received PRP traffic with wrong Lan ID",
    PortAHsrUntaggedRx: "Port A has received traffic without an HSR tag",
    PortBHsrUntaggedRx: "Port B has received traffic without an HSR tag",
    PortCHsrUntaggedRx: "Port C has received traffic without an HSR tag",
    PortADown:          "Port A's link is down",
    PortBDown:          "Port B's link is down"
};

/******************************************************************************/
// redbox_lib_port_map_create()
/******************************************************************************/
function redbox_lib_port_map_create(interfaces)
{
    var inst;

    // The allowed ports might differ between PortA and PortB (w.r.t. Neighbor
    // only).
    redbox_lib_port_map = {"PortA": [], "PortB": []};

    // Each RedBox instance maps to different ports. We create a portmap array
    // with a hash per instance containing the allowed values.
    Object.keys(redbox_lib_port_map).forEach(function(port) {
        // port = "PortA" or "PortB"

        // For each possible RedBox instance:
        for (inst = 0; inst < interfaces.length; inst++) {
            // "NONE" is always present (since this is the default).
            var content = new Map();
            content["NONE"] = "";

            // Neighbor is present on PortA unless this is the first instance
            // Neighbor is present on PortB unless this is the last  instance
            if ((inst === 0 && port == "PortA") || (inst === interfaces.length - 1 && port == "PortB")) {
                // Don't add neighbor
            } else {
                content["RedBox-Neighbor"] = "Neighbor";
            }

            // Now add all the ports that are available for this instance.
            interfaces[inst].val.Interfaces.forEach(function(interfac) {
                // E.g. "Gi 1/1": "Gi 1/1"
                content[interfac] = interfac;
            });

            redbox_lib_port_map[port].push(content);
        }
    });
}

/******************************************************************************/
// redbox_lib_oper_state_to_image()
/******************************************************************************/
function redbox_lib_oper_state_to_image(statu)
{
    if (!statu || statu.OperState == "disabled") {
        return "images/led-off.gif"; // Gray
    } else if (statu.OperState == "active") {
        if (statu.WarningNone) {
            return "images/led-up.gif"; // Green
        } else {
            return "images/led-yellow.gif"; // Yellow
        }
    } else {
        // Internal error
        return "images/led-down.gif"; // Red
    }
}

/******************************************************************************/
// redbox_lib_oper_warnings_to_str()
/******************************************************************************/
function redbox_lib_oper_warnings_to_str(statu)
{
    var str = "";
    Object.keys(redbox_lib_oper_warnings_map).forEach(function(key) {
        if (statu[key]) {
            str += (str.length ? "\n" : "") + redbox_lib_oper_warnings_map[key];
        }
    });

    return str;
}

/******************************************************************************/
// redbox_lib_oper_state_to_str()
/******************************************************************************/
function redbox_lib_oper_state_to_str(statu)
{
    if (!statu) {
        return "Not created";
    } else if (statu.OperState == "disabled") {
        return "Disabled";
    } else if (statu.OperState == "active") {
        return redbox_lib_oper_warnings_to_str(statu);
    } else {
        // Internal error
        return "Internal Error. See console or crashlog for details";
    }
}

/******************************************************************************/
// redbox_lib_notif_status_to_str()
/******************************************************************************/
function redbox_lib_notif_status_to_str(statu)
{
    var str = "";
    Object.keys(redbox_lib_notif_status_map).forEach(function(key) {
        if (statu[key]) {
            str += (str.length ? "\n" : "") + redbox_lib_notif_status_map[key];
        }
    });

    if (!str.length) {
        str = "None";
    }

    return str;
}

/******************************************************************************/
// redbox_lib_notif_to_image()
/******************************************************************************/
function redbox_lib_notif_to_image(statu)
{
    if (!statu || statu.OperState == "disabled") {
        return "images/led-off.gif"; // Gray
    } else if (statu.OperState == "active") {
        if (redbox_lib_notif_status_to_str(statu) != "None") {
            return "images/led-down.gif"; // Red
        } else {
            return "images/led-up.gif"; // Green
        }
    } else {
        // Internal error
        return "iumages/led-down.gif"; // Red
    }
}

/******************************************************************************/
// redbox_lib_notif_to_str()
/******************************************************************************/
function redbox_lib_notif_to_str(statu)
{
    if (!statu) {
        return "Not created";
    } else if (statu.OperState == "disabled") {
        return "Disabled";
    } else if (statu.OperState == "active") {
        return redbox_lib_notif_status_to_str(statu);
    } else {
        // Internal error
        return "Internal Error. See console or crashlog for details";
    }
}

/******************************************************************************/
// redbox_lib_d2h()
// Converts a decimal number to a hexadecimal with leading zeros up to 'width'.
/******************************************************************************/
function redbox_lib_d2h(d, width)
{
    var h = d.toString(16);
    var l = h.length;
    var i;

    for (i = l; i < width; i++) {
        h = "0" + h;
    }

    return "0x" + h;
}

/******************************************************************************/
// redbox_lib_max_value_to_digit_cnt()
// Finds the number of characters required to represent a value in decimal.
/******************************************************************************/
function redbox_lib_max_value_to_digit_cnt(max)
{
    var digits = 1;

    while (max >= 10) {
        digits++;
        max /= 10;
    }

    return digits;
}

/******************************************************************************/
// redbox_lib_port_to_str()
/******************************************************************************/
function redbox_lib_port_to_str(port)
{
    if (port == "RedBox-Neighbor") {
        return "Neighbor";
    }

    return port;
}

// Buttons that get disabled while a JSON request is in progress. We use a map
// in order to be able to add the same button over and over again, without
// growing an array.
var redbox_lib_disable_buttons_during_update = {};

/******************************************************************************/
// redbox_lib_disable_button_add()
/******************************************************************************/
function redbox_lib_disable_button_add(button_name)
{
    redbox_lib_disable_buttons_during_update[button_name] = true;
}

/******************************************************************************/
// redbox_lib_disable_button_del()
/******************************************************************************/
function redbox_lib_disable_button_del(button_name)
{
    delete redbox_lib_disable_buttons_during_update[button_name];
}

/******************************************************************************/
// redbox_lib_update_visibility_set()
/******************************************************************************/
function redbox_lib_update_visibility_set(show)
{
    var fld;

    // We control the visibility of the "update" image (the rotating thing that
    // shows that something is happening) ourselves, because we may perform more
    // than one JSON request per call to redbox_lib_json_request().
    // If we didn't do this, the requestJsonDoc() function would show it on
    // every request and hide it on every response, so if all requests could be
    // sent in one go, the image would go away after the first response, and
    // that looks silly if later responses take longer time.
    // If the image ID is "update", json.js controls it, so we call it
    // "redbox_update" in the HTML file.

    fld = $("redbox_update");
    if (fld) {
        fld.style.visibility = show ? "visible" : "hidden";

        for (var button_name in redbox_lib_disable_buttons_during_update) {
            fld = $(button_name);
            if (fld) {
                // Disable the button when we show the updating image.
                fld.disabled = show;
            } else {
                // Button doesn't exist. Purge from map. This is safe according
                // to ECMAScript 5.1 standard section 12.6.4.
                delete redbox_lib_disable_buttons_during_update[button_name];
            }
        }
    }
}

var redbox_lib_json_request_get_cnt;

/******************************************************************************/
// redbox_lib_on_json_get_received()
/******************************************************************************/
function redbox_lib_on_json_get_received(recv_json, params)
{
    if (!recv_json) {
        alert(params.name + ": Get dynamic data failed.");
        return;
    }

    if (redbox_lib_json_request_get_cnt < 1) {
       alert("Huh? redbox_lib_json_request_get_cnt = " + redbox_lib_json_request_get_cnt);
       redbox_lib_json_request_get_cnt = 1;
    }

    if (params.sel) {
        // A particular instance has been requested. In that case, recv_json
        // contains an error code or the actual result.
        params.req[params.name] = recv_json.result ? JSON.parse(JSON.stringify(recv_json.result)) : undefined;
    } else {
        // We only get the result, and not error codes, etc.
        params.req[params.name] = JSON.parse(JSON.stringify(recv_json));
    }

    redbox_lib_json_request_get_cnt--;

    if (redbox_lib_json_request_get_cnt === 0) {
        // Hide the "redbox_update" image.
        redbox_lib_update_visibility_set(false);

        // All requests are now fulfilled. Callback caller of
        // redbox_lib_json_request()
        params.func();
    }
}

/******************************************************************************/
// redbox_lib_json_request()
/******************************************************************************/
function redbox_lib_json_request(request, func_to_execute_when_done, selected_inst)
{
    redbox_lib_json_request_get_cnt = Object.keys(request).length;

    if (!redbox_lib_json_request_get_cnt) {
        alert("redbox_lib_json_request(): Internal error: Nothing requested");
        return;
    }

    redbox_lib_update_visibility_set(true);

    if (request.capabilities) {
        requestJsonDoc("redbox.capabilities.capabilities.get", null, redbox_lib_on_json_get_received, {name: "capabilities", req: request, func: func_to_execute_when_done});
    }

    if (request.interfaces) {
        requestJsonDoc("redbox.capabilities.interfaces.get", null, redbox_lib_on_json_get_received, {name: "interfaces", req: request, func: func_to_execute_when_done});
    }

    if (request.default_conf) {
        requestJsonDoc("redbox.config.defaultConf.get", null, redbox_lib_on_json_get_received, {name: "default_conf", req: request, func: func_to_execute_when_done});
    }

    if (request.conf) {
        requestJsonDoc("redbox.config.get", selected_inst, redbox_lib_on_json_get_received, {name: "conf", req: request, sel: selected_inst, func: func_to_execute_when_done}, selected_inst ? true /* always call us back - unless redirecting */ : false);
    }

    if (request.statu) {
        requestJsonDoc("redbox.status.get", selected_inst, redbox_lib_on_json_get_received, {name: "statu", req: request, sel: selected_inst, func: func_to_execute_when_done}, selected_inst ? true /* always call us back - unless redirecting */ : false);
    }

    if (request.stati) {
        requestJsonDoc("redbox.statistics.get", selected_inst, redbox_lib_on_json_get_received, {name: "stati", req: request, sel: selected_inst, func: func_to_execute_when_done}, selected_inst ? true /* always call us back - unless redirecting */ : false);
    }

    if (request.nt) {
        requestJsonDoc("redbox.status.nt.get", selected_inst, redbox_lib_on_json_get_received, {name: "nt", req: request, sel: selected_inst, func: func_to_execute_when_done}, selected_inst ? true /* always call us back - unless redirecting */ : false);
    }

    if (request.nt_mac) {
        // Unfortunately, there's no way to obtain the MAC addresses only for
        // the selected instance.
        requestJsonDoc("redbox.status.nt_mac.get", undefined, redbox_lib_on_json_get_received, {name: "nt_mac", req: request, sel: selected_inst, func: func_to_execute_when_done}, selected_inst ? true /* always call us back - unless redirecting */ : false);
    }

    if (request.pnt) {
        requestJsonDoc("redbox.status.pnt.get", selected_inst, redbox_lib_on_json_get_received, {name: "pnt", req: request, sel: selected_inst, func: func_to_execute_when_done}, selected_inst ? true /* always call us back - unless redirecting */ : false);
    }

    if (request.pnt_mac) {
        // Unfortunately, there's no way to obtain the MAC addresses only for
        // the selected instance.
        requestJsonDoc("redbox.status.pnt_mac.get", undefined, redbox_lib_on_json_get_received, {name: "pnt_mac", req: request, sel: selected_inst, func: func_to_execute_when_done}, selected_inst ? true /* always call us back - unless redirecting */ : false);
    }

    // Cannot prevent this function from returning once requested. Caller must
    // use func_to_execute_when_done() to go on.
}

var redbox_lib_json_request_set_cnt;

/******************************************************************************/
// redbox_lib_on_json_set_received()
/******************************************************************************/
function redbox_lib_on_json_set_received(recv_json, params)
{
    var error_msg;

    if (!recv_json) {
        alert(params.name + ": Set data failed for instance " + params.sel);
        return;
    }

    if (redbox_lib_json_request_set_cnt < 1) {
       alert("Huh? redbox_lib_json_request_set_cnt = " + redbox_lib_json_request_set_cnt);
       redbox_lib_json_request_set_cnt = 1;
    }

    redbox_lib_json_request_set_cnt--;

    if (redbox_lib_json_request_set_cnt === 0) {
        // Hide the "redbox_update" image.
        redbox_lib_update_visibility_set(false);
    }

    // Convert recv_json to an error message - if it wasn't successful.
    if (recv_json && recv_json.error) {
        if (recv_json.error.message) {
            error_msg = recv_json.error.message;
        } else if (recv_json.error.code) {
            error_msg = recv_json.error.code;
        } else {
            // Leave msg undefined
        }
    } else {
        // Leave error_msg undefined
    }

    // We call back for every requestJsonDoc() we have done, and give an
    // indicator in last argument whether this is the last callback or not.
    params.func(params.sel, error_msg, redbox_lib_json_request_set_cnt === 0);
}

/******************************************************************************/
// redbox_lib_json_submit()
/******************************************************************************/
function redbox_lib_json_submit(json_func, requests, func_to_execute_per_submit)
{
    var inst, i;

    redbox_lib_json_request_set_cnt = Object.keys(requests).length;

    if (!redbox_lib_json_request_set_cnt) {
        alert("redbox_lib_json_submit(): Internal error: Nothing submitted");
        return;
    }

    redbox_lib_update_visibility_set(true);

    for (i = 0; i < requests.length; i++) {
        requestJsonDoc(json_func, requests[i], redbox_lib_on_json_set_received, {name: json_func, sel: requests[i][0], func: func_to_execute_per_submit}, true /* always call us back - unless redirecting */);
    }
}

/******************************************************************************/
// redbox_lib_inst_get()
/******************************************************************************/
function redbox_lib_inst_get(data, inst)
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

var redbox_lib_autorefresh_timer;

/******************************************************************************/
// redbox_lib_autorefresh_update()
/******************************************************************************/
function redbox_lib_autorefresh_update()
{
    if ($("autorefresh").checked) {
        if (redbox_lib_autorefresh_timer) {
            clearTimeout(redbox_lib_autorefresh_timer);
        }

        redbox_lib_autorefresh_timer = setTimeout('requestUpdate()', settingsRefreshInterval());
    }
}

/******************************************************************************/
// redbox_lib_on_autorefresh_click()
/******************************************************************************/
function redbox_lib_on_autorefresh_click()
{
    if ($("autorefresh").checked) {
        requestUpdate();
    } else if (redbox_lib_autorefresh_timer) {
        clearTimeout(redbox_lib_autorefresh_timer);
        redbox_lib_autorefresh_timer = null;
    }
}

