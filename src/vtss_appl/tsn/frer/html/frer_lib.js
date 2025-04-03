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
// frer_lib_port_map
// Dynamically filled
/******************************************************************************/
var frer_lib_port_map;

/******************************************************************************/
// frer_lib_mode_map
/******************************************************************************/
var frer_lib_mode_map = {
    generation: "Generation",
    recovery:   "Recovery"
};

/******************************************************************************/
// frer_lib_algorithm_map
/******************************************************************************/
frer_lib_algorithm_map = {
    vector: "Vector",
    match:  "Match"
};

/******************************************************************************/
// frer_lib_oper_warnings_map
/******************************************************************************/
var frer_lib_oper_warnings_map = {
    WarningNone:                                       "None",
    WarningStreamNotFound:                             "At least one of the ingress streams doesn't exist",
    WarningStreamAttachFail:                           "Failed to attach to at least one of the ingress streams, possibly because it is part of a stream collection",
    WarningStreamHasConfigurationalWarnings:           "At least one of the ingress streams has configurational warnings",
    WarningIngressEgressOverlap:                       "There is an overlap between ingress and egress ports",
    WarningEgressPortCnt:                              "In generation mode, only one egress port is specified",
    WarningIngressNoLink:                              "At least one of the ingress ports doesn't have link",
    WarningEgressNoLink:                               "At least one of the egress ports doesn't have link",
    WarningVlanMembership:                             "At least one of the egress ports is not member of the FRER VLAN",
    WarningStpBlocked:                                 "At least one of the egress ports is blocked by STP",
    WarningMstpBlocked:                                "At least one of the egress ports is blocked by MSTP",
    WarningStreamCollectionNotFound:                   "Stream collection doesn't exist",
    WarningStreamCollectionAttachFail:                 "Failed to attach to stream collection",
    WarningStreamCollectionHasConfigurationalWarnings: "The specified stream collection has configurational warnings"
};

/******************************************************************************/
// frer_lib_oper_warnings_to_str()
/******************************************************************************/
function frer_lib_oper_warnings_to_str(statu)
{
    var str = "";
    Object.keys(frer_lib_oper_warnings_map).forEach(function(key) {
        if (statu[key]) {
            str += (str.length ? "\n" : "") + frer_lib_oper_warnings_map[key];
        }
    });

    return str;
}

/******************************************************************************/
// frer_lib_oper_state_to_str()
/******************************************************************************/
function frer_lib_oper_state_to_str(statu)
{
    if (!statu) {
        return "Not created";
    } else if (statu.OperState == "disabled") {
        return "Disabled";
    } else if (statu.OperState == "active") {
        return frer_lib_oper_warnings_to_str(statu);
    } else {
        // Internal error
        return "Internal Error. See console or crashlog for details";
    }
}

/******************************************************************************/
// frer_lib_oper_state_to_image()
/******************************************************************************/
function frer_lib_oper_state_to_image(statu)
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
// frer_lib_led_error_to_image()
/******************************************************************************/
function frer_lib_led_error_to_image(statu, conf)
{
    if (!conf || !statu || statu.OperState != "active" || conf.Mode != "recovery" || !conf.LaErrDetection) {
        return "images/led-off.gif"; // Gray
    } else {
        if (statu.LatentError) {
            return "images/led-down.gif"; // Red
        } else {
            return "images/led-up.gif"; // Green
        }
    }
}

/******************************************************************************/
// frer_lib_egress_ifnames_to_port_list()
/******************************************************************************/
function frer_lib_egress_ifnames_to_port_list(ifnames, print_none)
{
    var arr, arr2, port_no_list_str, comma, first_val, second_val, i, j;

    arr = [];
    Object.each(ifnames, function(ifname) {
        arr.push(frer_lib_port_map[ifname]);
    });

    arr2 = stream_lib_array_sort_and_uniquify(arr);

    port_no_list_str = "";
    comma            = "";
    i                = 0;
    while (i < arr2.length) {
        first_val  = arr2[i];
        second_val = first_val;

        for (j = i + 1; j < arr2.length; j++) {
            if (arr2[j] == second_val + 1) {
                second_val = arr2[j];
            } else {
                // Not a consecutive number. Get out of here
                break;
            }
        }

        port_no_list_str += comma;
        comma = ",";

        if (first_val == second_val) {
            // Not a range
            port_no_list_str += first_val;
            i++;
        } else {
            // A range
            port_no_list_str += first_val + "-" + second_val;
            i = j;
        }
    }

    if (print_none && port_no_list_str.length === 0) {
        return "None";
    }

    return port_no_list_str;
}

/******************************************************************************/
// frer_lib_egress_port_list_to_ifnames()
/******************************************************************************/
function frer_lib_egress_port_list_to_ifnames(key, result)
{
    var fld = $(key), x = {str: fld.value}, range, comma, start, n, local_result, local_result2, i, ifname, max;

    max = Object.keys(frer_lib_port_map).length;

    x.idx = 0;
    x.len = x.str.length;
    local_result = [];

    if (!stream_lib_remove_ws(x)) {
        GiveAlert("Ports must be separated by commas, not spaces", fld);
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
            GiveAlert("Invalid character ('" + x.str.charAt(x.idx) + "') found in Egress Port List", fld);
            return false;
        }

        // Check for valid port number.
        if (n < 1 || n > max) {
            GiveAlert("Port numbers are integers between 1 and " + max, fld);
            return false;
        }

        if (range) {
            // End of range has been reached
            range = false;
            if (n < start) {
                GiveAlert("Invalid range detected in Egress Port List", fld);
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
        GiveAlert("An Egress Port List cannot end with a comma or a dash", fld);
        return false;
    }

    // local_result now contains an array of the stream IDs.
    // Get rid of duplicates while sorting the array.
    local_result2 = stream_lib_array_sort_and_uniquify(local_result);

    if (local_result2.length > globals.capabilities.EgressPortCntMax) {
        GiveAlert("At most " + globals.capabilities.EgressPortCntMax + " ports can be specified", fld);
        return false;
    }

    // Convert to interface names.
    for (i = 0; i < local_result2.length; i++) {
        for (ifname in frer_lib_port_map) {
            if (frer_lib_port_map[ifname] == local_result2[i]) {
                result.push(ifname);
                break;
            }
        }
    }

    return true;
}

// Buttons that get disabled while a JSON request is in progress. We use a map
// in order to be able to add the same button over and over again, without
// growing an array.
var frer_lib_disable_buttons_during_update = {};

/******************************************************************************/
// frer_lib_disable_button_add()
/******************************************************************************/
function frer_lib_disable_button_add(button_name)
{
    frer_lib_disable_buttons_during_update[button_name] = true;
}

/******************************************************************************/
// frer_lib_disable_button_del()
/******************************************************************************/
function frer_lib_disable_button_del(button_name)
{
    delete frer_lib_disable_buttons_during_update[button_name];
}

/******************************************************************************/
// frer_lib_update_visibility_set()
/******************************************************************************/
function frer_lib_update_visibility_set(show)
{
    var fld;

    // We control the visibility of the "update" image (the rotating thing that
    // shows that something is happening) ourselves, because we may perform more
    // than one JSON request per call to frer_lib_json_request().
    // If we didn't do this, the requestJsonDoc() function would show it on
    // every request and hide it on every response, so if all requests could be
    // sent in one go, the image would go away after the first response, and
    // that looks silly if later responses take longer time.
    // If the image ID is "update", json.js controls it, so we call it
    // "frer_update" in the HTML file.

    fld = $("frer_update");
    if (fld) {
        fld.style.visibility = show ? "visible" : "hidden";

        for (var button_name in frer_lib_disable_buttons_during_update) {
            fld = $(button_name);
            if (fld) {
                // Disable the button when we show the updating image.
                fld.disabled = show;
            } else {
                // Button doesn't exist. Purge from map. This is safe according
                // to ECMAScript 5.1 standard section 12.6.4.
                delete frer_lib_disable_buttons_during_update[button_name];
            }
        }
    }
}

var frer_lib_json_request_get_cnt;

/******************************************************************************/
// frer_lib_on_json_get_received()
/******************************************************************************/
function frer_lib_on_json_get_received(recv_json, params)
{
    if (!recv_json) {
        alert(params.name + ": Get dynamic data failed.");
        return;
    }

    if (frer_lib_json_request_get_cnt < 1) {
       alert("Huh? frer_lib_json_request_get_cnt = " + frer_lib_json_request_get_cnt);
       frer_lib_json_request_get_cnt = 1;
    }

    if (params.sel) {
        // A particular instance has been requested. In that case, recv_json
        // contains an error code or the actual result.
        params.req[params.name] = recv_json.result ? JSON.parse(JSON.stringify(recv_json.result)) : undefined;
    } else {
        // We only get the result, and not error codes, etc.
        params.req[params.name] = JSON.parse(JSON.stringify(recv_json));
    }

    frer_lib_json_request_get_cnt--;

    if (frer_lib_json_request_get_cnt === 0) {
        // Hide the "frer_update" image.
        frer_lib_update_visibility_set(false);

        // All requests are now fulfilled. Callback.
        params.func();
    }
}

/******************************************************************************/
// frer_lib_json_request()
/******************************************************************************/
function frer_lib_json_request(request, func_to_execute_when_done, selected_inst)
{
    frer_lib_json_request_get_cnt = Object.keys(request).length;

    if (!frer_lib_json_request_get_cnt) {
        alert("frer_lib_json_request(): Internal error: Nothing requested");
        return;
    }

    frer_lib_update_visibility_set(true);

    if (request.capabilities) {
        requestJsonDoc("frer.capabilities.get", null, frer_lib_on_json_get_received, {name: "capabilities", req: request, func: func_to_execute_when_done});
    }

    if (request.port_name_map) {
        requestJsonDoc("port.namemap.get", null, frer_lib_on_json_get_received, {name: "port_name_map", req: request, func: func_to_execute_when_done});
    }

    if (request.default_conf) {
        requestJsonDoc("frer.config.defaultConf.get", null, frer_lib_on_json_get_received, {name: "default_conf", req: request, func: func_to_execute_when_done});
    }

    if (request.conf) {
        requestJsonDoc("frer.config.get", selected_inst, frer_lib_on_json_get_received, {name: "conf", req: request, sel: selected_inst, func: func_to_execute_when_done}, selected_inst ? true /* always call us back - unless redirecting */ : false);
    }

    if (request.statu) {
        requestJsonDoc("frer.status.get", selected_inst, frer_lib_on_json_get_received, {name: "statu", req: request, sel: selected_inst, func: func_to_execute_when_done}, selected_inst ? true /* always call us back - unless redirecting */ : false);
    }

    if (request.stati) {
        requestJsonDoc("frer.statistics.get", selected_inst, frer_lib_on_json_get_received, {name: "stati", req: request, sel: selected_inst, func: func_to_execute_when_done}, selected_inst ? true /* always call us back - unless redirecting */ : false);
    }

    // Cannot prevent this function from returning once requested. Caller must
    // use func_to_execute_when_done() to go on.
}

var frer_lib_json_request_set_cnt;

/******************************************************************************/
// frer_lib_on_json_set_received()
/******************************************************************************/
function frer_lib_on_json_set_received(recv_json, params)
{
    var error_msg;

    if (!recv_json) {
        alert(params.name + ": Set data failed for instance " + params.sel);
        return;
    }

    if (frer_lib_json_request_set_cnt < 1) {
       alert("Huh? frer_lib_json_request_set_cnt = " + frer_lib_json_request_set_cnt);
       frer_lib_json_request_set_cnt = 1;
    }

    frer_lib_json_request_set_cnt--;

    if (frer_lib_json_request_set_cnt === 0) {
        // Hide the "frer_update" image.
        frer_lib_update_visibility_set(false);
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
    params.func(params.sel, error_msg, frer_lib_json_request_set_cnt === 0);
}

/******************************************************************************/
// frer_lib_json_submit()
/******************************************************************************/
function frer_lib_json_submit(json_func, requests, func_to_execute_per_submit)
{
    var inst, i;

    frer_lib_json_request_set_cnt = Object.keys(requests).length;

    if (!frer_lib_json_request_set_cnt) {
        alert("frer_lib_json_submit(): Internal error: Nothing submitted");
        return;
    }

    frer_lib_update_visibility_set(true);

    for (i = 0; i < requests.length; i++) {
        requestJsonDoc(json_func, requests[i], frer_lib_on_json_set_received, {name: json_func, sel: requests[i][0], func: func_to_execute_per_submit}, true /* always call us back - unless redirecting */);
    }
}

/******************************************************************************/
// frer_lib_inst_get()
/******************************************************************************/
function frer_lib_inst_get(data, inst)
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

var frer_lib_autorefresh_timer;

/******************************************************************************/
// frer_lib_autorefresh_update()
/******************************************************************************/
function frer_lib_autorefresh_update()
{
    if ($("autorefresh").checked) {
        if (frer_lib_autorefresh_timer) {
            clearTimeout(frer_lib_autorefresh_timer);
        }

        frer_lib_autorefresh_timer = setTimeout('requestUpdate()', settingsRefreshInterval());
    }
}

/******************************************************************************/
// frer_lib_on_autorefresh_click()
/******************************************************************************/
function frer_lib_on_autorefresh_click()
{
    if ($("autorefresh").checked) {
        requestUpdate();
    } else if (frer_lib_autorefresh_timer) {
        clearTimeout(frer_lib_autorefresh_timer);
        frer_lib_autorefresh_timer = null;
    }
}

