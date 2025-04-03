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
// psfp_lib_color_mode_map
/******************************************************************************/
var psfp_lib_color_mode_map = {
    colorBlind: "Color Blind",
    colorAware: "Color Aware"
};

/******************************************************************************/
// psfp_lib_coupling_flag_map
/******************************************************************************/
var psfp_lib_coupling_flag_map = {
    0: "0",
    1: "1"
};

/******************************************************************************/
// psfp_lib_gate_state_map
/******************************************************************************/
var psfp_lib_gate_state_map = {
    closed: "Closed",
    open:   "Open"
};

/******************************************************************************/
// psfp_lib_time_unit_map
/******************************************************************************/
var psfp_lib_time_unit_map = {
    ns: "ns",
    us: "us",
    ms: "ms"
};

/******************************************************************************/
// psfp_lib_ipv_map
/******************************************************************************/
var psfp_lib_ipv_map = {
    "-1": "Disabled",
    "0":  "0",
    "1":  "1",
    "2":  "2",
    "3":  "3",
    "4":  "4",
    "5":  "5",
    "6":  "6",
    "7":  "7"
};

/******************************************************************************/
// psfp_lib_gcl_len_map
// Dynamically filled.
/******************************************************************************/
var psfp_lib_gcl_len_map;

/******************************************************************************/
// psfp_lib_filter_warnings_map
/******************************************************************************/
var psfp_lib_filter_warnings_map = {

    WarningNone:                             "None",
    WarningNoStreamOrStreamCollection:       "Neither a stream or a stream collection is specified",
    WarningStreamNotFound:                   "The specified stream ID does not exist",
    WarningStreamCollectionNotFound:         "The specified stream collection ID does not exist",
    WarningStreamAttachFail:                 "Unable to attach to the specified stream, possibly because it is part of a stream collection",
    WarningStreamCollectionAttachFail:       "Unable to attach to the specified stream collection",
    WarningStreamHasConfigurationalWarnings: "The specified stream has configurational warnings.",
    WarningStreamCollectionHasConfWarnings:  "The specified stream collection has configurational warnings.",
    WarningFlowMeterNotFound:                "The specified flow meter ID does not exist",
    WarningGateNotFound:                     "The specified stream gate ID does not exist",
    WarningGateNotEnabled:                   "The specified stream gate is not enabled"
};

/******************************************************************************/
// psfp_lib_filter_warnings_to_str()
/******************************************************************************/
function psfp_lib_filter_warnings_to_str(statu)
{
    var str = "";

    if (!statu) {
        return str;
    }

    Object.keys(psfp_lib_filter_warnings_map).forEach(function(key) {
        if (statu[key]) {
            str += (str.length ? "\n" : "") + psfp_lib_filter_warnings_map[key];
        }
    });

    return str;
}

/******************************************************************************/
// psfp_lib_filter_warnings_to_image()
/******************************************************************************/
function psfp_lib_filter_warnings_to_image(statu)
{
    if (!statu) {
        return "images/led-off.gif"; // Gray
    } else if (statu.WarningNone) {
        return "images/led-up.gif"; // Green
    } else {
        return "images/led-yellow.gif"; // Yellow
    }
}

// When FlowMeterId and GateId are -1, they are not used.
const psfp_lib_flow_meter_id_none = 4294967295;
const psfp_lib_gate_id_none       = 4294967295;

// Buttons that get disabled while a JSON request is in progress. We use a map
// in order to be able to add the same button over and over again, without
// growing an array.
var psfp_lib_disable_buttons_during_update = {};

/******************************************************************************/
// psfp_lib_disable_button_add()
/******************************************************************************/
function psfp_lib_disable_button_add(button_name)
{
    psfp_lib_disable_buttons_during_update[button_name] = true;
}

/******************************************************************************/
// psfp_lib_disable_button_del()
/******************************************************************************/
function psfp_lib_disable_button_del(button_name)
{
    delete psfp_lib_disable_buttons_during_update[button_name];
}

/******************************************************************************/
// psfp_lib_update_visibility_set()
/******************************************************************************/
function psfp_lib_update_visibility_set(show)
{
    var fld;

    // We control the visibility of the "update" image (the rotating thing that
    // shows that something is happening) ourselves, because we may perform more
    // than one JSON request per call to psfp_lib_json_request().
    // If we didn't do this, the requestJsonDoc() function would show it on
    // every request and hide it on every response, so if all requests could be
    // sent in one go, the image would go away after the first response, and
    // that looks silly if later responses take longer time.
    // If the image ID is "update", json.js controls it, so we call it
    // "psfp_update" in the HTML file.

    fld = $("psfp_update");
    if (fld) {
        fld.style.visibility = show ? "visible" : "hidden";

        for (var button_name in psfp_lib_disable_buttons_during_update) {
            fld = $(button_name);
            if (fld) {
                // Disable the button when we show the updating image.
                fld.disabled = show;
            } else {
                // Button doesn't exist. Purge from map. This is safe according
                // to ECMAScript 5.1 standard section 12.6.4.
                delete psfp_lib_disable_buttons_during_update[button_name];
            }
        }
    }
}

var psfp_lib_json_request_get_cnt;

/******************************************************************************/
// psfp_lib_on_json_get_received()
/******************************************************************************/
function psfp_lib_on_json_get_received(recv_json, params)
{
    if (!recv_json) {
        alert(params.name + ": Get dynamic data failed.");
        return;
    }

    if (psfp_lib_json_request_get_cnt < 1) {
       alert("Huh? psfp_lib_json_request_get_cnt = " + psfp_lib_json_request_get_cnt);
       psfp_lib_json_request_get_cnt = 1;
    }

    if (params.sel) {
        // A particular instance has been requested. In that case, recv_json
        // contains an error code or the actual result.
        params.req[params.name] = recv_json.result ? JSON.parse(JSON.stringify(recv_json.result)) : undefined;
    } else {
        // We only get the result, and not error codes, etc.
        params.req[params.name] = JSON.parse(JSON.stringify(recv_json));
    }

    psfp_lib_json_request_get_cnt--;

    if (psfp_lib_json_request_get_cnt === 0) {
        // Hide the "psfp_update" image.
        psfp_lib_update_visibility_set(false);

        // All requests are now fulfilled. Callback.
        params.func();
    }
}

/******************************************************************************/
// psfp_lib_json_request()
/******************************************************************************/
function psfp_lib_json_request(request, func_to_execute_when_done, selected_inst)
{
    psfp_lib_json_request_get_cnt = Object.keys(request).length;

    if (!psfp_lib_json_request_get_cnt) {
        alert("psfp_lib_json_request(): Internal error: Nothing requested");
        return;
    }

    psfp_lib_update_visibility_set(true);

    if (request.capabilities) {
        requestJsonDoc("psfp.capabilities.get", null, psfp_lib_on_json_get_received, {name: "capabilities", req: request, func: func_to_execute_when_done});
    }

    if (request.flow_meter_default_conf) {
        requestJsonDoc("psfp.config.flow_meter_default.get", null, psfp_lib_on_json_get_received, {name: "flow_meter_default_conf", req: request, func: func_to_execute_when_done});
    }

    if (request.flow_meter_conf) {
        requestJsonDoc("psfp.config.flow_meter.get", selected_inst, psfp_lib_on_json_get_received, {name: "flow_meter_conf", req: request, sel: selected_inst, func: func_to_execute_when_done}, selected_inst ? true /* always call us back - unless redirecting */ : false);
    }

    if (request.flow_meter_statu) {
        requestJsonDoc("psfp.status.flow_meter.get", selected_inst, psfp_lib_on_json_get_received, {name: "flow_meter_statu", req: request, sel: selected_inst, func: func_to_execute_when_done}, selected_inst ? true /* always call us back - unless redirecting */ : false);
    }

    if (request.gate_default_conf) {
        requestJsonDoc("psfp.config.gate_default.get", null, psfp_lib_on_json_get_received, {name: "gate_default_conf", req: request, func: func_to_execute_when_done});
    }

    if (request.gate_conf) {
        requestJsonDoc("psfp.config.gate.get", selected_inst, psfp_lib_on_json_get_received, {name: "gate_conf", req: request, sel: selected_inst, func: func_to_execute_when_done}, selected_inst ? true /* always call us back - unless redirecting */ : false);
    }

    if (request.gate_statu) {
        requestJsonDoc("psfp.status.gate.get", selected_inst, psfp_lib_on_json_get_received, {name: "gate_statu", req: request, sel: selected_inst, func: func_to_execute_when_done}, selected_inst ? true /* always call us back - unless redirecting */ : false);
    }

    if (request.filter_default_conf) {
        requestJsonDoc("psfp.config.filter_default.get", null, psfp_lib_on_json_get_received, {name: "filter_default_conf", req: request, func: func_to_execute_when_done});
    }

    if (request.filter_conf) {
        requestJsonDoc("psfp.config.filter.get", selected_inst, psfp_lib_on_json_get_received, {name: "filter_conf", req: request, sel: selected_inst, func: func_to_execute_when_done}, selected_inst ? true /* always call us back - unless redirecting */ : false);
    }

    if (request.filter_statu) {
        requestJsonDoc("psfp.status.filter.get", selected_inst, psfp_lib_on_json_get_received, {name: "filter_statu", req: request, sel: selected_inst, func: func_to_execute_when_done}, selected_inst ? true /* always call us back - unless redirecting */ : false);
    }

    if (request.filter_stati) {
        requestJsonDoc("psfp.statistics.filter.get", selected_inst, psfp_lib_on_json_get_received, {name: "filter_stati", req: request, sel: selected_inst, func: func_to_execute_when_done}, selected_inst ? true /* always call us back - unless redirecting */ : false);
    }

    // Cannot prevent this function from returning once requested. Caller must
    // use func_to_execute_when_done() to go on.
}

var psfp_lib_json_request_set_cnt;

/******************************************************************************/
// psfp_lib_on_json_set_received()
/******************************************************************************/
function psfp_lib_on_json_set_received(recv_json, params)
{
    var error_msg;

    if (!recv_json) {
        alert(params.name + ": Set data failed for instance " + params.sel);
        return;
    }

    if (psfp_lib_json_request_set_cnt < 1) {
       alert("Huh? psfp_lib_json_request_set_cnt = " + psfp_lib_json_request_set_cnt);
       psfp_lib_json_request_set_cnt = 1;
    }

    psfp_lib_json_request_set_cnt--;

    if (psfp_lib_json_request_set_cnt === 0) {
        // Hide the "psfp_update" image.
        psfp_lib_update_visibility_set(false);
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
    params.func(params.sel, error_msg, psfp_lib_json_request_set_cnt === 0);
}

/******************************************************************************/
// psfp_lib_json_submit()
/******************************************************************************/
function psfp_lib_json_submit(json_func, requests, func_to_execute_per_submit)
{
    var inst, i;

    psfp_lib_json_request_set_cnt = Object.keys(requests).length;

    if (!psfp_lib_json_request_set_cnt) {
        alert("psfp_lib_json_submit(): Internal error: Nothing submitted");
        return;
    }

    psfp_lib_update_visibility_set(true);

    for (i = 0; i < requests.length; i++) {
        requestJsonDoc(json_func, requests[i], psfp_lib_on_json_set_received, {name: json_func, sel: requests[i][0], func: func_to_execute_per_submit}, true /* always call us back - unless redirecting */);
    }
}

/******************************************************************************/
// psfp_lib_inst_get()
/******************************************************************************/
function psfp_lib_inst_get(data, inst)
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

var psfp_lib_autorefresh_timer;

/******************************************************************************/
// psfp_lib_autorefresh_update()
/******************************************************************************/
function psfp_lib_autorefresh_update()
{
    if ($("autorefresh").checked) {
        if (psfp_lib_autorefresh_timer) {
            clearTimeout(psfp_lib_autorefresh_timer);
        }

        psfp_lib_autorefresh_timer = setTimeout('requestUpdate()', settingsRefreshInterval());
    }
}

/******************************************************************************/
// psfp_lib_on_autorefresh_click()
/******************************************************************************/
function psfp_lib_on_autorefresh_click()
{
    if ($("autorefresh").checked) {
        requestUpdate();
    } else if (psfp_lib_autorefresh_timer) {
        clearTimeout(psfp_lib_autorefresh_timer);
        psfp_lib_autorefresh_timer = null;
    }
}

