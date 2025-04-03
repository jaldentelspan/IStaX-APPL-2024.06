// * -*- Mode: java; tab-width: 8; -*-
/*

 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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

/*
 * This file contains the requestJsonDoc function from the json.js file.
 * It is intended as a (much) faster loading alternative to the json.js file
 * for webpages that only needs this function.
 */

/* Send HTML Request with JSON syntax.
 *
 * @method_name: [Mandatory], method name for JSON request.
 * @json_params: [Optional], JSON parameters according to the method name.
 * @cb_func:     [Optional], callback function when done the JSON request process.
 * @cb_params:   [Optional], callback parameters.
 */
function requestJsonDoc(method_name, json_params, cb_func, cb_params)
{
    // Visible the update image (updating.gif)
    var elem = document.getElementById("update");
    if (elem) {
        elem.style.visibility = "visible";
    }

    // Request JSON Format: { "method":"<method_name>", "params":"<json_params>", "id":"jsonrpc" }

    var req_json = {"method":method_name, "params":[], "id":"jsonrpc"};
    if (json_params){
        if (Object.prototype.toString.call(json_params) == "[object Array]") {
            req_json.params = json_params;
        } else {
            req_json.params.push(json_params);
        }
    }
    var req_data = JSON.encode(req_json);

    var req = new Request.JSON({
        url: "/json_rpc",
        data: req_data,
        onSuccess: function(responseText) {
            // Response JSON Format: { "id":"jsonrpc", "error":null, "result":null }
            if (responseText.error) {
                if (responseText.error.message) {
                    if (responseText.error.message == "Access denied" && method_name.match(/.get$/)) {
                        document.location.href = 'insuf_priv_lvl.htm';
                    } else {
                        alert("Error: " + responseText.error.message);
                    }
                } else {
                    alert("Error: " + responseText.error.code);
                }
            } else if (typeof(cb_func) == "function") {
                cb_func(responseText.result, cb_params);
            }

            // Hide the update image (updating.gif)
            elem = document.getElementById("update");
            if (elem) {
                elem.style.visibility = "hidden";
            }
            req = null; // MSIE leak avoidance
        },
        onFailure: function(xhr) {
            if (xhr.status == 404 && xhr.responseText.match(/"error":{"code":-32601,"message":"Access denied"}/)) {
                if (method_name.match(/.get$/)) {
                    document.location.href = 'insuf_priv_lvl.htm';
                } else {
                    if (typeof(cb_func) == "function") {
                        cb_func([], cb_params);
                    }
                    setReadOnly();
                    if (typeof(cb_func) == "function") {
                        alert("You are not allowed to change settings.");
                    }
                }
            } else {
                var json_response = JSON.decode(xhr.responseText);
                var err_msg = (json_response && json_response.error.message) ? "\n" + json_response.error.message : "";
                alert("HTTP Status: " + xhr.status + " " + xhr.statusText + "." + err_msg);
            }

            // Hide the update image (updating.gif)
            elem = document.getElementById("update");
            if (elem) {
                elem.style.visibility = "hidden";
            }
            req = null; // MSIE leak avoidance
        }
    }).send();

    return req.xhr ? req.xhr : null;
}
