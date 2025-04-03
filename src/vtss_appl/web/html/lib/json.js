// * -*- Mode: java; tab-width: 8; -*-
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

// *******************************  JSON.JS  *****************************
// *
// * Author: Peter Chen
// *
// * --------------------------------------------------------------------------
// *
// * Description:
// * Client-side JSON functions.
// * It is based on MooTools and current existing functions to transmit/receive
// * dynamic data with JSON syntax.
// *
// * To include in HTML file use:
// *
// * <script type="text/javascript" src="lib/mootools-core.js"></script>
// * <script type="text/javascript" src="lib/dynforms.js"></script>
// * <script type="text/javascript" src="lib/validate.js"></script>
// * <script type="text/javascript" src="lib/json.js"></script>
// *
// * --------------------------------------------------------------------------


/* Send HTML Request with JSON syntax.
 *
 * @method_name: [Mandatory], method name for JSON request.
 * @json_params: [Optional], JSON parameters according to the method name.
 * @cb_func:     [Optional], callback function when done the JSON request process.
 * @cb_params:   [Optional], callback parameters.
 * @cb_always:   [Optional], if true, always callback whether an error has occurred or not. In case this function doesn't redirect to another page, no error messages are printed from within this function. That's the responsibility of the called back function.
 */
function requestJsonDoc(method_name, json_params, cb_func, cb_params, cb_always)
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
            var redirecting = false;
            if (responseText.error) {
                if (responseText.error.message) {
                    if (responseText.error.message == "Access denied" && method_name.match(/.get$/)) {
                        redirecting = true;
                        document.location.href = 'insuf_priv_lvl.htm';
                    } else {
                        if (!cb_always) {
                            alert("JSON RPC Error. (" + responseText.error.message + ")");
                        }
                    }
                } else {
                    if (!cb_always) {
                        alert("JSON RPC Error. (" + responseText.error.code + ")");
                    }
                }
            } else if (typeof(cb_func) == "function" && !cb_always) {
                cb_func(responseText.result, cb_params);
            }

            if (!redirecting && cb_always && typeof(cb_func) == "function") {
                cb_func(responseText, cb_params);
            }

            // Hide the update image (updating.gif)
            elem = document.getElementById("update");
            if (elem) {
                elem.style.visibility = "hidden";
            }
            req = null; // MSIE leak avoidance
        },
        onFailure: function(xhr) {
            var redirecting = false;
            if (xhr.status == 404 && xhr.responseText.match(/"error":{"code":-32601,"message":"Access denied"}/)) {
                if (method_name.match(/.get$/)) {
                    redirecting = true;
                    document.location.href = 'insuf_priv_lvl.htm';
                } else {
                    if (typeof(cb_func) == "function" && !cb_always) {
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

            if (!redirecting && cb_always && typeof(cb_func) == "function") {
                cb_func(xhr.responseText, cb_params);
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

/* Compare element values between two JSON objects.
 *
 * Return true if each object element value is the same, false otherwise.
 * @obj1: [Mandatory], the JSON object 1.
 * @obj2: [Mandatory], the JSON object 2.
 */
function isSameJsonValues(obj1, obj2)
{
    if (Object.prototype.toString.call(obj1) == "[object Array]") {
        if (Object.prototype.toString.call(obj1) != "[object Array]") {
            return false; // different type.
        }
        for (var idx = 0; idx < obj1.length; idx++) {
            if (!isSameJsonValues(obj1[idx], obj2[idx])) {
                return false;
            }
        }
    } else if (Object.keys(obj1).length != Object.keys(obj2).length) {
        return false; // different child number.
    } else {
        var keys = Object.keys(obj1);
        for (var idx = 0; idx < keys.length; idx++) {
            if (keys[idx] && obj1[keys[idx]] != obj2[keys[idx]]) {
                return false;
            }
        }
    }

    return true;
}

/* Submit row data with JSON syntax.
 *
 * @orig_recv_json: [Optional], original received data with JSON syntax.
 *                  The submit operation will be ignored when the row data is the same as new one.
 *                  Notice that the format is different with "new_row"
 * @html_newrow:    [Mandatory], new row data with HTML syntax.
 * @key:            [Mandatory], method name for JSON request.
 * @trim_keytext:   [Mandatory], the key text which need to trim for JSON request.
 * @method_name:    [Mandatory], method name for JSON request.
 * @cb_func:        [Optional], callback function when done the JSON request process.
 * @cb_params:      [Optional], cb_func parameters.
 * @jsonReqSendCb:  [Optional], the callback function before sending the JSON request.
 * The engine will pass the JSON parameter to the callback function and the sending
 * process is terminated when the return value is 'false'.
 */
function submitJsonRow(orig_recv_json, html_newrow, key, trim_keytext, method_name, cb_func, cb_params, jsonReqSendCb)
{
    // Trim the unnessary string for element ID.
    var elems = html_newrow.getElementsByTagName("*");
    for (var i = 0, elem; i < elems.length; i++) {
        elem = html_newrow.getElementById(elems[i].id);
        if (!elem) {
            continue;
        }
        if (elem.id) {
            elem.id = elem.id.replace(RegExp(trim_keytext, "g"), "");
        }
        if (elem.name) {
            elem.name = elem.name.replace(RegExp(trim_keytext, "g"), "");
        }
    }
    var row_json = html2Json(html_newrow, trim_keytext);

    // Compare the original/new row data, ignore the Request if they are the same.
    if (orig_recv_json) {
        if (isSameJsonValues(orig_recv_json, row_json)) {
            return;
        }
    }

    // Fill JSON parameters then send JSON Request.
    var json_params;
    if (Object.prototype.toString.call(key) == "[object Array]") {
        json_params = key;
    } else {
        json_params = [key];
    }
    if (row_json) {
        /* Append to json parameter when the entry data is existed */
        json_params.push(row_json);
    }

    // Execute the callback function before sending the JSON request.
    if (typeof(jsonReqSendCb) == "function") {
        if (jsonReqSendCb(json_params, method_name) == false) {
            // The callback function terminated the sending process
            return;
        }
    }

    requestJsonDoc(method_name, json_params, cb_func, cb_params);
}

/* Set all configured elements as read-only */
function setReadOnly()
{
    var elems;

    // Disable <input> elements
    elems = document.getElementsByTagName("input");
    Object.each(elems, function(elem) {
        var val = elem.value;
        var id = elem.id;
        if (val == "Reset" || val == "Refresh" ||
            val == "autorefresh" || id == "autorefresh" ||
            val == " << " || val == " |<< " || val == " >> " || val == " >>| "
            ) {
            // Ignore page control buttons
        } else {
            elem.disabled = true;
            if (elem.onclick) {
                elem.onclick = null;
            }
            if (elem.type && elem.type == "button") {
                elem.title = "You are not allowed to change settings";
            }
        }
    });

    // Disable <img> onclick event
    elems = document.getElementsByTagName("img");
    Object.each(elems, function(elem) {
        if (elem.title) {
            var split_str = elem.title.split(" ");
            if (split_str[0] != "Navigate") {
                if (elem.onclick) {
                    elem.onclick = null;
                    elem.title = "You are not allowed to change settings";
                }
            }
        } else if (elem.onclick) {
            elem.onclick = null;
        }
    });
}

/* Convert interface description to row key value.
 *
 * @if_desc: [Mandatory], interface description.
 */
function ifDesc2key(if_desc, port_status_json_response)
{
    if (typeof(if_desc) == "number") {
        return if_desc;
    }

    // Get interface value
    var if_val = if_desc.split(" ")[1];

    if (if_desc.match(/^Gi/)) {
        if (!port_status_json_response) {
            alert("Missing input parameter in ifDesc2key().");
        }

        // Convert interface description to switch ID and port NO.
        var port_if = new Object();

        for (var idx = 0, new_switch_offset = 0; idx < port_status_json_response.length; idx++) {
            // Lookup if_desc in mapping table
            if(idx && (port_status_json_response[idx - 1].key.split(" ")[1].split("/")[0] != port_status_json_response[idx].key.split(" ")[1].split("/")[0])) {
                new_switch_offset = idx;
            }

            if (if_desc == port_status_json_response[idx].key) {
                // Found it
                port_if.swid = if_val.split("/")[0];
                port_if.port_no = idx - new_switch_offset + 1; /* uport start from 1 */
                return port_if;
            }
        }

        alert("Cannot find the interface mapping.");
        return 0;
    } else {
        return if_val;
    }
}

/* Convert row key value to interface description.
 *
 * @if_type: [Mandatory], interface type: vlan/port.
 * @key:     [Mandatory], row key.
 */
function key2IfDesc(if_type, key)
{
    if (if_type == "vlan") {
    } else if (if_type == "vlan") {
        return ("vlan " + key);
    } else if (if_type == "port") {
        // To-do, port interface
        alert("Not implemented yet in key2IfDesc(): " + if_type);
        return ("Gi 1/" + key);
    } else {
         alert("Not implemented yet in key2IfDesc(): " + if_type);
    }
}

/* Get interface type from interface description.
 *
 * @if_desc: [Mandatory], interface description.
 */
function getIfType(if_desc)
{
    if (/vlan/.test(if_desc)) {
        return "vlan";
    } else if (/Gi/.test(if_desc)) {
        return "port";
    } else {
         return "none";
    }
}

/* Eanble all read-only(disabled) elements.
 *
 * @html_obj: [Mandatory], JS object with HTML syntax.
 */
function enableReadOnlyElements(html_obj)
{
    // Eanble all read-only(disabled) <input> element
    var inputs = html_obj.getElementsByTagName("input");
    Object.each(inputs, function(elem) {
        if (elem.disabled) {
            elem.disabled = false;
        }
    });

    // Eanble all read-only(disabled) <select> element
    var selects = html_obj.getElementsByTagName("select");
    for (var idx = 0; idx < selects.length; idx++) {
        var elem = selects[idx];
        if (elem.disabled) {
            elem.disabled = false;
        }
    }

    return html_obj;
}

/* Copy HTML object.
 * The select element value doesn't clone to the new object by cloneNode()
 * So we made a new one for the feature.
 *
 * @html_obj: [Mandatory], the HTML object.
 */
function copyHtmlObject(html_obj) {
    var new_html_obj = html_obj.cloneNode(true);

    // Copy the value of select element to new object
    var selects = new_html_obj.getElementsByTagName("select");
    for (var idx = 0; idx < selects.length; idx++) {
        var elem_id = selects[idx].id;
        new_html_obj.getElementById(elem_id).value = html_obj.getElementById(elem_id).value;
    }

    // For MSIE, copy the value of checkbox element to new object
    if (navigator.appName && navigator.appName == 'Microsoft Internet Explorer') {
        var inputs = new_html_obj.getElementsByTagName("input");
        for (var idx = 0; idx < inputs.length; idx++) {
            if (inputs[idx].type != "checkbox") {
                continue;
            }
            var elem_id = inputs[idx].id;
            var new_field = new_html_obj.getElementById(elem_id);
            if (html_obj.getElementById(elem_id).checked) {
                new_field.setAttribute("checked", true);
                new_field.setAttribute("defaultChecked", true);
                new_field.value = html_obj.getElementById(elem_id).value;
            } else {
                new_field.removeAttribute("checked");
                new_field.removeAttribute("defaultChecked");
            }
        }
    }

    return new_html_obj;
}

/* Remove HTML object by ID.
 *
 * @html_obj:   [Mandatory], HTML object.
 * @remove_ids: [Mandatory], An array with field elements IDs.
 */
function removeHtmlObject(html_obj, remove_ids)
{
    Object.each(remove_ids, function(remove_id) {
        var elem = html_obj.getElementById(remove_id);
        if (elem) {
            elem.parentElement.removeChild(elem);
        }
    });

    return html_obj;
}

/* Convert HTML object to JSON object.
 *
 * @html_obj: [Mandatory], HTML object.
 * @trim_ids: [Optional], Trim unnecessary field IDs.
 *                        Actually, it is not essential procedure for JSON PRC.
 *                        It will ignore the redundant JSON fields.
 */
function html2Json(html_obj, trim_ids)
{
    var new_html_obj = copyHtmlObject(html_obj);

    // Trim unnecessary field
    new_html_obj = removeHtmlObject(new_html_obj, trim_ids);

    // Eanble all read-only(disabled) elements.
    new_html_obj = enableReadOnlyElements(new_html_obj);

    // Generate HTML query string
    var query_str = new_html_obj.toQueryString();

    // Append checkbox value (It is because there is no output in the query string while the checkbox is unchecked)
    var inputs = new_html_obj.getElementsByTagName("input");
    for (var idx = 0; idx < inputs.length; idx++) {
        if (inputs[idx].type != "checkbox") {
            continue;
        }
        if (inputs[idx].checked) {
            // Convert checkbox value to Boolean
            query_str = query_str.replace(RegExp(inputs[idx].id + "=on", "g"), inputs[idx].id + "=true");
        } else {
            query_str += "&" + inputs[idx].id + "=false";
        }
    }

    // Trim first "&" character
    if (query_str[0] == "&") {
        query_str = query_str.replace(RegExp("^&", "g"), "");
    }

    // Convert <checkbox> to Boolean value
    var json = query_str.length ? query_str.parseQueryString() : null /* Use 'null' when the entry data does not exist */;
    for (var idx = 0; idx < inputs.length; idx++) {
        if (inputs[idx].type != "checkbox") {
            continue;
        }
        if (json[inputs[idx].id]) {
            json[inputs[idx].id] = json[inputs[idx].id] == "true" ? true : false;
        }
    }

    // Convert <select> to Boolean value
    var selects = new_html_obj.getElementsByTagName("select");
    for (var idx = 0; idx < selects.length; idx++) {
        var elem = selects[idx];
        if ($(elem).options.length == 2 && json[elem.id]) {
            // Make sure this <select> is used for Boolean presentation
            var opts = $(elem).options;
            if ((opts[0].value == "true" && opts[1].value == "false") ||
                (opts[0].value == "false" && opts[1].value == "true")) {
                json[elem.id] = json[elem.id] == "true" ? true : false;
            }
        }
    }

    return json;
}

/* Convert row key to HTML element ID string. For example,
 * [1,{"Network":"10.9.52.0","IpSubnetMaskLength":24}] -> "1_10.9.52.0_24"
 */
function rowKey2Id(keys, init_str)
{
    if (Object.prototype.toString.call(keys) == "[object Array]" || Object.prototype.toString.call(keys) == "[object Object]") {
        var key_str = init_str ? init_str : "";
        Object.each(Object.keys(keys), function(k) {
            var val = keys[k];
            if (Object.prototype.toString.call(val) == "[object Object]") {
                key_str = rowKey2Id(val, key_str);
            } else {
                key_str += (key_str.length ? "," : "") + keys[k];
                first_elem = false;
            }
        });
        return key_str;
    } else {
        return keys;
    }
}

/* The "DynamicTable" class is based on current existing functions and use JSON
 * syntax to transmit/receive dynamic data.
 * It is used to create/update/validate the dynamic HTML tables.
 * If there is any unsuitable case for this class, it will alert message
 * "Not implemented yet" to remind that something new is needed to plusin.
 *
 * Quick reference:
 *  .initialize(ref, type, subtype, validate) - Initialize Class::DynamicTable.
 *  .initializeRows(row_cnt, field_cnt) - Initialize a set of rows.
 *  .initializeRow(field_cnt) - Initialize a single row.
 *  .restore() - Restore default setting.
 *
 *  .setTrimIds(trim_ids) - Set trim IDs.
 *
 *  .setRowPrefixs(type) - Set prefixs.
 *  .getRowPrefixs(type) - Get prefixs.
 *
 *  .getRecvJsonCnt() - Get received JSON data count.
 *  .cloneRecvJson(recv_json) - Clone received JSON data.
 *  .saveRecvJson(name, recv_json) - Save received JSON data.
 *  .getRecvJson(name) - Retrieve received JSON data.
 *  .removeRecvJson(name) - Remove received JSON data from local database.
 *
 *  .addRows(rows) - Add table rows.
 *  .addRow(row) - Add table row.
 *  .delRow(split_char, del_button_id) - Delete table row
 *  .addNewRow(row) - Add new row.
 *  .delNewRow(split_char, del_button_id) - Delete new row.
 *  .getNewrowsCnt() - Get new rows count.
 *  .update() - Update table.
 *
 *  .addResetButton(cb_func, cb_params) - Add reset button. ("Reset")
 *  .resetEvent(cb_func, cb_params) - Reset row data.
 *  .addSubmitButton(recv_json_name, method_name, cb_func, cb_params) - Add submit button. ("Save")
 *  .submitEvent(recv_json_name, method_name, cb_func, cb_params) - Submit row data.
 *  .addNewRowButton(add_newrow_cb, max_rows) - Add NewRow button. ("Add New Entry")
 *
 *  .validate() - Validate row data via callback functions.
 *
 * Example: See Examples below (last block).
 */
var DynamicTable = new Class({
    /* -----------------------------------------------------------------------
     * Class Property
     * ---------------------------------------------------------------------*/
    property: 'DynamicTable',

    /* -----------------------------------------------------------------------
     * Debug Flags.
     * Enable/Disable manually, for example: myDynamicTable.debug.validate = true;
     * ---------------------------------------------------------------------*/
    debug: {
        saveRecvJson:   false,  /* Only alert debug message when save received JSON data */
        getRecvJson:    false,  /* Only alert debug message when get received JSON data */
        submitEvent:    false,  /* Only alert debug message for sumbit event */
        validate:       false,  /* Only alert debug message for validation */
        all:            false   /* Alert all debug messages */
    },

    /* -----------------------------------------------------------------------
     * Public Functions
     * ---------------------------------------------------------------------*/
    /* Initialize class data.
     *
     * @reference: [Mandatory], the reference element that need to append the new table.
     * @type:      [Mandatory], table type.
     * @subtype:   [Optional], table sub-type.
     * @validate:  [Optional], validate callback function with JSON syntax.
     *
     * ===========================================================================
     * | Type       | Sub-Type       | Description
     * ===========================================================================
     * | "config"   |                | Common configured table
     * |            | columnOrder    | Configured table with column configured fields
     * |            | plusNewRow     | Configured table with new row
     * |            | plusRowCtrlBar | Configured table with new row contrl bar
     * | "display"  |                | Common display table
     * ---------------------------------------------------------------------------
     */
    initialize: function(reference, type, subtype, validate) {
        // Check table type/sub-type
        if (!this._isValidTableType(type, subtype)) {
            return;
        }

        // Initialize local variables
        this.localVars.reference = reference;
        this.localVars.headerFrag = document.createDocumentFragment();
        this.localVars.bodyFrag = document.createDocumentFragment();
        this.localVars.type = type;
        this.localVars.subtype = subtype ? subtype : "";
        this.localVars.recvJson = new Array();
        this.localVars.trim_ids = new Array();

        this.localVars.DoneSubmitFlag = true;
        this.localVars.rows = [];
        if (validate && (!validate.func || typeof(validate.func) != "function" || !validate.params)) {
            alert("JSON syntax error.\n{ func:<func_name>, params:[<param_0>, <params_1>, ...] }");
        }
        this.localVars.validate = validate ? validate : null;

        this.localVars.maxRows = 0;
        this.localVars.newrows = [];

        // Build table
        this._build();
    },

    /* Initialize the table rows that we can fill each row data
     * one by one after the initialization.
     *
     * @row_cnt:   [Mandatory], row count in the table.
     * @field_cnt: [Mandatory], column count in the table.
     */
    initializeRows: function(row_cnt, field_cnt) {
        if (!row_cnt && field_cnt) {
            alert("Row count cannot be zero when column count is greater than 1.");
            return;
        }

        var table_rows = new Array();
        Object.each(row_cnt, function() {
            var row = {"fields":[]};
            Object.each(field_cnt, function() {
                row.fields.push({});
            });
            table_rows.push(row);
        });
        return table_rows;
    },

    /* Initialize the single fields that we can fill each fields data
     * one by one after the initialization.
     *
     * @field_cnt: [Mandatory], column count in the table.
     */
    initializeRow: function(field_cnt) {
        var row = {"fields":[]};
        Object.each(field_cnt, function() {
            row.fields.push({});
        });
        return row;
    },

    /* Restore default setting. */
    restore: function() {
        this.initialize(this.localVars.reference, this.localVars.type, this.localVars.subtype, this.localVars.validate);
    },

    /* Set trim IDs.
     * It is used to trim the unnecessary field from the HTTP POST JSON data.
     *
     * @trim_ids: [Mandatory], the array object which contains the HTML element IDs.
     */
    setTrimIds: function(trim_ids) {
        this.localVars.trim_ids = trim_ids;
    },

    /* Set row prefixs.
     *
     * @type:       [Mandatory], type of prefix. ("rowKeyPrefix"/"rowDelPrefix"/"newrowKeyPrefix"/"newrowDelPrefix")
     * @new_prefix: [Mandatory], new prefix.
     */
    setRowPrefixs: function(type, new_prefix) {
        if (type == "rowKeyPrefix") {
            this.localVars.rowKeyPrefix = new_prefix;
        } else if (type == "rowDelPrefix") {
            this.localVars.rowDelPrefix = new_prefix;
        } else if (type == "newrowKeyPrefix") {
            this.localVars.newrowKeyPrefix = new_prefix;
        } else if (type == "newrowDelPrefix") {
            this.localVars.newrowDelPrefix = new_prefix;
        } else {
            alert("Not implemented yet in setRowPrefixs(): " + type);
        }
    },

    /* Get row prefixs.
     *
     * @type: [Mandatory], type of prefix. ("rowKeyPrefix"/"rowDelPrefix"/"newrowKeyPrefix"/"newrowDelPrefix")
     */
    getRowPrefixs: function(type) {
        if (type == "rowKeyPrefix") {
            return this.localVars.rowKeyPrefix;
        } else if (type == "rowDelPrefix") {
            return this.localVars.rowDelPrefix;
        } else if (type == "newrowKeyPrefix") {
            return this.localVars.newrowKeyPrefix;
        } else if (type == "newrowDelPrefix") {
            return this.localVars.newrowDelPrefix;
        } else {
            alert("Not implemented yet in getRowPrefixs(): " + type);
        }
    },

    /* Identify if using interface description as the row key.
     *
     * @key_is_if_desc: [Mandatory], Boolean flag, TRUE when using interface description as row key.
     * @if_type:        [Mandatory], interface type: vlan/port.
     */
    rowKeyIsIfDesc: function(key_is_if_desc, if_type) {
        this.localVars.rowKeyIsIfDesc = key_is_if_desc;
        this.localVars.rowKeyIfType = if_type;
    },

    /* Set if using interface zero-based for row key.
     * Notice that we should use one-based on WEB UI implementation,
     * the JSON library will automatic convert to zero-based when send out the
     * JSON parameter to JSON RPC.
     *
     * @key_is_zero_based: [Mandatory], Boolean flag, TRUE when using zero-based for row key.
     */
    rowKeyIsZeroBased: function(key_is_zero_based) {
        this.localVars.rowKeyIsZeroBased = key_is_zero_based;
    },

    /* Set if using Hash object for row key.
     *
     * @rowKeyIsHashObj: [Mandatory], Boolean flag, TRUE when using Hash object for row key, i.e. key:{"Name": "test"}
     */
    rowKeyIsHashObj: function(key_is_hash_obj) {
        this.localVars.rowKeyIsHashObj = key_is_hash_obj;
    },

    /* Register the callback function before send out the JSON request.
     *
     * @cb_func: [Mandatory], the callback function before sending the JSON request.
     * It can be used to update the JSON parameters before sending the JSON request.
     * The engine will pass the JSON parameter to the callback function and the sending
     * process is terminated when the return value is 'false'.
     */
    jsonReqSendCb: function(cb_func) {
        if (cb_func && typeof(cb_func) == "function") {
            this.localVars.jsonReqSendCb = cb_func;
        } else {
            alert("jsonReqSendCb() faild, it request a function object");
        }
    },

    /* get received JSON data count. */
    getRecvJsonCnt: function() {
        return this.localVars.recvJson.length;
    },

    /* Clone received JSON data.
     *
     * @recv_json: [Mandatory], received json data.
     */
    cloneRecvJson: function(recv_json) {
        if (!recv_json) {
            alert("Error parameter for cloneRecvJson()");
            return null;
        }

        return JSON.parse(JSON.stringify(recv_json));
    },

    /* Save received JSON data.
     *
     * @name:      [Mandatory], the name of JSON data.
     * @recv_json: [Mandatory], received json data.
     */
    saveRecvJson: function(name, recv_json) {
        if (!name || !name.length || !recv_json) {
            alert("Error parameters for saveRecvJson()");
            return;
        }

        // Convert interface description to row key value
        if (this.localVars.rowKeyIsIfDesc) {
            recv_json.each(function(data) {
                data.key = ifDesc2key(data.key);
            }, this);
        }

        var debug_msg = "Name: " + name + ", recv_json:\n";
        var record = {name: name, data: this.cloneRecvJson(recv_json)};

        // Lookup name in the original database
        for (var i = 0; i < this.localVars.recvJson.length; i++) {
            if (this.localVars.recvJson[i].name == name) {
                // Found the same name
                this.localVars.recvJson[i] = record;

                // Debug message
                debug_msg += "Same name already exists.\n";
                debug_msg += this._dumpRecvJson(record.data);
                this._debugMsg("saveRecvJson", debug_msg);
                return;
            }
        }

        // Push new record to "recvJson" array
        this.localVars.recvJson.push(record);

        // Debug message
        debug_msg += this._dumpRecvJson(this.localVars.recvJson[this.localVars.recvJson.length -1].data);
        this._debugMsg("saveRecvJson", debug_msg);
    },

    /* Retrieve received JSON data.
     *
     * @name: [Mandatory], the name of JSON data.
     */
    getRecvJson: function(name) {
        var recv_json = [], found = false;

        for (var i = 0; i < this.localVars.recvJson.length; i++) {
            if (this.localVars.recvJson[i].name == name) {
                // Clone a new one
                recv_json = this.cloneRecvJson(this.localVars.recvJson[i].data);
                found = true;
                break;
            }
        }
        if (!found) {
            alert("Cannot find name: " + name);
            return;
        }

        // Debug message
        var debug_msg = "Name: " + name + ", recv_json:\n";
        debug_msg += this._dumpRecvJson(recv_json);
        this._debugMsg("getRecvJson", debug_msg);

        return recv_json;
    },

    /* Remove received JSON data from local database.
     *
     * @name: [Mandatory], the name of JSON data.
     */
    removeRecvJson: function(name) {
        for (var i = 0; i < this.localVars.recvJson.length; i++) {
            if (this.localVars.recvJson[i].name == name) {
                this.localVars.newrows.splice(i, 1);
                return;
            }
        }
    },

    /* Return html id of table body.
     *
     */
    getTableBodyId: function() {
        var tbody = this.localVars.reference + "Tbody";
        return tbody;
    },

    /* Add table rows with JSON syntax.
     *
     * Syntax: rows:[<row_0>, <row_1>, ...]
     *         <row_n>   - {"fields":<fields>}
     *         <fields>  - [<field_0>, <field_1>, ...]
     *         <field_n> - {"type":<type>, "params":[<params_0>, <params_1>, ...]}
     *
     * When using the JSON syntax for the Web page layout, the "rows" attriable
     * is used to add new fields in a fragment element according to the original
     * dynamic-form function.
     *
     * @rows: [Mandatory], Rows parameters with JSON syntax.
     */
    addRows: function(rows) {
        rows.each(function(row) {
            this.addRow(row);
        }, this);
    },

    /* Add table row.
     * It will return an array with field elements for advanced attriable modification.
     *
     * @row: [Mandatory], row data with JSON syntax.
     */
    addRow: function(row) {
        this._debugMsg("addRow", "row: " + JSON.stringify(row));

        if (!this._isValidRowSyntax(row)) {
            return;
        }

        // Add row to table
        var tr = document.createElement("tr");
        var field_elems = this._addRowFields(tr, row.fields);
        if (this._isHeaderRow(row)) {
            this.localVars.headerFrag.appendChild(tr);
        } else {
            this.localVars.bodyFrag.appendChild(tr);

            // Update local variable: rows
            this.localVars.rows.push(row);
        }

        return field_elems;
    },

    /* Delete table row.
     *
     * @split_char: [Mandatory]. The character splitting the name from the ID
                    For example, if the ID of the button is "Button_47", the
                    split_char is "_" and the 47 is used to find the row in the
                    added rows (idx when addRow() or addRows() was used.
     * @del_button_id: [Mandatory]. The element ID of "Delete" button.
     */
    delRow: function(split_char, del_button_id) {
        // Remove parent <tr> element
        var fld = $(del_button_id);
        var tr = findParentElem(fld, "tr");
        var tbody = findParentElem(fld, "tbody");
        tbody.removeChild(tr);

        // Retrieve the new index
        var split_val = del_button_id.split(split_char);
        var idx = parseInt(split_val[1], 10);

        // Update localVars.rows()
        for (var i = 0; i < this.localVars.rows.length; i++) {
            if (this.localVars.rows[i].idx == idx) {
                this.localVars.rows.splice(i, 1);
                break;
            }
        }

        this.updateRowClass();
    },

    /* Add table new row.
     * It will return an array with field elements for advanced attriable modification.
     *
     * @row: [Mandatory]. row data with JSON syntax.
     * Beside the 'fields' key, an optional 'new_idx' key with value set to an
     * integer identifying the new row may be supplied. This can be used to
     * delete the new row again (with delNewRow()).
     */
    addNewRow: function(row) {
        if (!this._isValidRowSyntax(row)) {
            return;
        }

        // Remove empty row
        var tbody = $(this.localVars.reference + "Tbody");
        if (this.localVars.rows.length && this.localVars.rows[0].fields[0].type == "empty_row") {
            var trs = tbody.getElementsByTagName("tr");
            tbody.removeChild(trs[trs.length - 1]);
            this.localVars.rows.pop();
        }

        // Add row to table
        var tr = document.createElement("tr");
        var field_elems = this._addRowFields(tr, row.fields);
        tbody.appendChild(tr);

        // Update local variable: newrows
        this.localVars.newrows.push(row);

        // Update table row class
        this.updateRowClass();

        return field_elems;
    },

    /* Delete new table row.
     *
     * @split_char: [Mandatory]. The character splitting the name from the ID
                    For example, if the ID of the button is "Button_47", the
                    split_char is "_" and the 47 is used to find the row in the
                    added rows (new_idx when addNewRow() was used.
     * @del_button_id: [Mandatory]. The element ID of "Delete" button.
     */
    delNewRow: function(split_char, del_button_id) {
        // Remove parent <tr> element
        var fld = $(del_button_id);
        var tr = findParentElem(fld, "tr");
        var tbody = findParentElem(fld, "tbody");
        tbody.removeChild(tr);

        // Retrieve the new index
        var split_val = del_button_id.split(split_char);
        var new_idx = parseInt(split_val[1], 10);

        // Update local variable: newrows
        for (var i = 0; i < this.localVars.newrows.length; i++) {
            if (this.localVars.newrows[i].new_idx == new_idx) {
                this.localVars.newrows.splice(i, 1);
                break;
            }
        }

        this.updateRowClass();
    },

    /* Update table */
    update: function() {
        // Update table header
        var theader = $(this.localVars.reference + "THeader");
        if (theader) {
            clearChildNodes(theader);
            theader.appendChild(this.localVars.headerFrag);
        }

        // Update table rows
        var tbody = $(this.localVars.reference + "Tbody");
        clearChildNodes(tbody);
        tbody.appendChild(this.localVars.bodyFrag);

        // Update row class
        this.updateRowClass();

        /* The most common ways to select an element or elements via MoolTools Selectors.
         *
         * '='  : is equal to
         * '*=' : contain
         * '^=' : start with
         * '$=' : end with
         * '!=' : is not equal to
         * '~=' : contained in a space separated list
         */

        // Save the element attriable which is disabled in initial stage
        this.localVars.disabledElements = $(this.localVars.reference + "Form") ? $(this.localVars.reference + "Form").getElements('*:disabled') : [];

        // Save the <TD> tag element attriable which is hidden in initial stage
        var td_elems = $(this.localVars.reference + "Form") ? $(this.localVars.reference + "Form").getElements('td') : [];
        this.localVars.hiddenTdElements = [];
        td_elems.each(function(elem) {
            if (elem.hidden) {
                this.localVars.hiddenTdElements.push(elem);
            }
        }, this);
    },

    /* Add reset button.
     * The function will return button element for advanced attriable modification.
     * It isn't suit for multiple tables in one web page, In this case, we should
     * make another proprietary function to process all reset operations.
     *
     * @cb_func:   [Optional], callback function when done the reset process.
     * @cb_params: [Optional], callback parameters.
     */
    addResetButton: function(cb_func, cb_params) {
        // Ignore the operation when it alreay exists
        if ($(this.localVars.reference + "resetButton")) {
            return;
        }

        var  paragraph = $("saveResetButtonParagraph");
        if (!paragraph) {
            paragraph = document.createElement("p");
            paragraph.id = "saveResetButtonParagraph";
        }

        var td = document.createElement("td");
        paragraph.appendChild(td);

        var elem = document.createElement('input');
        elem.type = "button";
        elem.id = this.localVars.reference + "resetButton";
        elem.value = "Reset";
        var dynamic_table = this;
        elem.onclick = function() { dynamic_table.resetEvent(cb_func, cb_params); };

        // Append button
        td.appendChild(elem);
        tbody = $(this.localVars.reference).appendChild(paragraph);

        return elem;
    },

    /* Reset row data.
     *
     * @cb_func:   [Optional], callback function when done the reset event.
     * @cb_params: [Optional], callback parameters.
     */
    resetEvent: function(cb_func, cb_params) {
        $(this.localVars.reference + "Form").reset();

        // Remove new rows
        var tbody = $(this.localVars.reference + "Tbody");
        Object.each(this.localVars.newrows, function(row, idx) {
            var trs = tbody.getElementsByTagName("tr");
            tbody.removeChild(trs[trs.length - 1]);
        });

        // Empty local variable: newrows
        this.localVars.newrows = [];

        // Restore the element attriable which is disabled in initial stage
        var current_disabled_elems = $(this.localVars.reference + "Form") ? $(this.localVars.reference + "Form").getElements('*:disabled') : [];
        Object.each(current_disabled_elems, function(elem) {
            if (this.localVars.disabledElements.indexOf(elem) == -1) {
                elem.disabled = false;
            }
        }, this);
        Object.each(this.localVars.disabledElements, function(elem) {
            elem.disabled = true;
        });

        // Restore the <TD> tag element attriable which is hidden in initial stage
        if (this.localVars.hiddenTdElements) {
            var td_elems = $(this.localVars.reference + "Form") ? $(this.localVars.reference + "Form").getElements('td') : [];
            Object.each(td_elems, function(elem) {
                if (elem.hidden) {
                    elem.hidden = false;
                    elem.style.display = "";  // For MSIE
                }
            });
            Object.each(this.localVars.hiddenTdElements, function(elem) {
                elem.hidden = true;
                elem.style.display = "none";  // For MSIE
            });
        }

        // Execute callback function
        if (cb_func && typeof(cb_func) == "function") {
            cb_func.apply(cb_params);
        }
    },

    /* Add submit button.
     * The function will return button element for advanced attriable modification.
     * It isn't suit for multiple tables in one web page, In this case, we should
     * make another proprietary function to process all submit operations.
     *
     * @recv_json_name: [Mandatory], name of received JSON data.
     * @method_name:    [Mandatory], method name for JSON request.
     * @cb_func:        [Optional], callback function when done the JSON request process.
     * @cb_params:      [Optional], callback parameters.
     */
    addSubmitButton: function(recv_json_name, method_name, cb_func, cb_params) {
        // Ignore the operation when it alreay exists
        if ($(this.localVars.reference + "submitButton")) {
            // Re-enable 'Save button' function.
            $(this.localVars.reference + "submitButton").disabled = false;
            return;
        }

        var  paragraph = $("saveResetButtonParagraph");
        if (!paragraph) {
            var paragraph = document.createElement("p");
            paragraph.id = "saveResetButtonParagraph";
        }

        var td = document.createElement("td");
        paragraph.appendChild(td);

        var elem = document.createElement('input');
        elem.type = "button";
        elem.id = this.localVars.reference + "submitButton";
        elem.value = "Save";
        var dynamic_table = this;
        // Disable 'Save button' after clicked to avoid the user to speedy click it instant.
        elem.onclick = function() { this.disabled = true; dynamic_table.submitEvent(recv_json_name, method_name, cb_func, cb_params); };

        // Append button
        td.appendChild(elem);
        $(this.localVars.reference).appendChild(paragraph);

        return elem;
    },

    /* Submit row data.
     *
     * @recv_json_name:     [Mandatory], name of received JSON data.
     * @method_name_prefix: [Mandatory], method name for JSON request.
     * @cb_func:            [Mandatory], callback function when done the JSON request process.
     * @cb_params:          [Optional], callback parameters.
     */
    submitEvent: function(recv_json_name, method_name_prefix, cb_func, cb_params) {
        this._debugMsg("submitEvent", "recv_json_name: " + recv_json_name + ", method_name_prefix: " + method_name_prefix);

        // Validate table data
        if (!this.validate()) {
            this._debugMsg("submitEvent", "Validation failed");

            // The callback function terminated the sending process.
            // Restore the 'disabled' attriable of the submit button.
            var submit_elem = $(this.localVars.reference + "submitButton");
            if (submit_elem) {
                $(this.localVars.reference + "submitButton").disabled = false;
            }

            return;
        }

        var subtype = this.localVars.subtype;
        if (subtype == "columnOrder") {
            this._submitForm(method_name_prefix, cb_func, cb_params);
        } else {
            this._submitRows(recv_json_name, method_name_prefix);
            if (subtype == "plusNewRow") {
                this._submitNewRows(method_name_prefix, cb_func, cb_params);
            }

            // Execute callback function
            if (cb_func && typeof(cb_func) == "function") {
                this._waitDoneSubmitFlag();
                milliSleep(500);    // Made a little bit delay here to avoid the JSON RPC process the GET operation faster than SET operation.
                cb_func.apply(cb_params);
            }
        }
    },

    /* Add "New Row" button.
     * The function will return button element for advanced attriable modification.
     *
     * @add_newrow_cb: [Mandatory], calllback function for add new row.
     *                 Notice that the callback function must have three
     *                 parameters(key, val, key_prefix).
     *                 The button onclick evevt will call it with the specific
     *                 parameters (new_idx, null, newrow_key_prefix).
     *                 Example:
     *                 function addRow(key, val, key_prefix) {
     *                     var row;
     *                     // Do something to fill row data here.
     *                     return row;
     *                 }
     * @max_rows:      [Mandatory], maximum row count.
     */
    addNewRowButton: function(add_newrow_cb, max_rows) {
        // Update local variable: maxRows
        this.localVars.maxRows = max_rows;

        // Ignore the operation when it alreay exists
        if ($(this.localVars.reference + "newRowButton")) {
            return;
        }

        var  paragraph = $("newRowButtonParagraph");
        if (!paragraph) {
            paragraph = document.createElement("p");
            paragraph.id = "newRowButtonParagraph";
        }

        var td = document.createElement("td");
        paragraph.appendChild(td);

        var elem = document.createElement('input');
        elem.type = "button";
        elem.id = this.localVars.reference + "newRowButton";
        elem.value = "Add New Entry";
        var dynamic_table = this;
        elem.onclick = function() { dynamic_table._newRowButtonCb(add_newrow_cb); };

        // Append button
        td.appendChild(elem);
        $(this.localVars.reference).appendChild(paragraph);

        return elem;
    },

    /* Validate row data via callback functions.
     * Return true if it pass all validations, false otherwise.
     *
     * Syntax: validate:{func:<func_name>, params:[<param_0>, <params_1>, ...]}
     *
     * When using the JSON syntax for the Web page layout, the "validate" attribute
     * can be used to validate the row field value according to the callback function.
     * It is an optional attribute and can be appended in table/row/field attributes.
     */
    validate: function() {
        var validate;

        this._debugMsg("validate", "RowsCnt: " + this.localVars.rows.length + ", NewRowsCnt: " + this.localVars.newrows.length);

        // Lookup all "validate" elements
        for (var idx = 0; idx < 2; idx++) {
            var rows = (idx ? this.localVars.newrows : this.localVars.rows);
            for (var i = 0; i < rows.length; i++) {
                for (var j = 0; j < rows[i].fields.length; j++) {
                    // Process single field validate callback function
                    validate = rows[i].fields[j].validate;
                    if (validate && typeof(validate.func) == "function") {
                        this._debugMsg("validate", (idx ? "New" : "") + "Row[" + i + "][" + j + "] Validate: " + validate.func.name);
                        if (!validate.func.apply(null, validate.params)) {
                            return false;
                        }
                    }
                }

                // Process single row validate callback function
                validate = rows[i].validate;
                if (validate && typeof(validate.func) == "function") {
                    this._debugMsg("validate", (idx ? "New" : "") + "Row[" + i + "] Validate: " + validate.func.name);
                    if (!validate.func.apply(null, validate.params)) {
                        return false;
                    }
                }
            }
        }

        // Process table validate callback function
        validate = this.localVars.validate;
        if (validate && typeof(validate.func) == "function") {
            this._debugMsg("validate", "Table Validate: " + validate.func.name);
            if (!validate.func.apply(null, validate.params)) {
                return false;
            }
        }

        return true;
    },

    /* Update new row class. */
    updateRowClass: function() {
        if (this.localVars.subtype == "columnOrder") {
            return;
        }

        // Remove original class
        $$("#" + this.localVars.reference + "Tbody tr").removeClass("display_even");
        $$("#" + this.localVars.reference + "Tbody tr").removeClass("display_odd");
        $$("#" + this.localVars.reference + "Tbody tr:even").addClass("display_odd");
        $$("#" + this.localVars.reference + "Tbody tr:odd").addClass("display_even");
    },

    /* -----------------------------------------------------------------------
     * Local Variables / Private Functions
     * ---------------------------------------------------------------------*/
    localVars: {
        reference:         null,            /* Table reference */
        headerFrag:        null,            /* Table header fragment */
        bodyFrag:          null,            /* Table data row fragment */
        type:              "",              /* Table type */
        subtype:           "",              /* Table sub-type */
        recvJson:          null,            /* Received JSON data */
        doneSubmitFlag:    true,            /* Boolean flag, TRUE when done the submit operation */
        disabledElements:  [],              /* An array for storing the disabled elements in initial stage */
        hiddenTdElements:  [],              /* An array for storing the hidden <TD> tag elements in initial stage */

        rows:              [],              /* Table rows */
        validate:          null,            /* Table validation callback function.
                                             * It can be your local proprietary function or
                                             * the existing function in validate.js */

        rowKeyPrefix:      "rowKey_",       /* Prefix text for the row key */
        rowKeyIsIfDesc:    false,           /* Boolean flag, TRUE when using interface description as row key */
        rowKeyIsZeroBased: false,           /* Boolean flag, TRUE when using zero-based for row key */
        rowKeyIfType:      "",              /* Interface type, use when rowKeyIsIfDesc is TURE */
        rowKeyIsHashObj:   false,           /* Boolean flag, TRUE when using Hash object for row key, i.e. key:{"Name": "test"} */

        maxRows:           0,               /* (use for NewRow table) Table header count */
        newrows:           [],              /* (use for NewRow table) Table new rows */
        rowDelPrefix:      "rowDelete_",    /* (use for NewRow table) Prefix text for the row delete checkbox */
        newrowDelPrefix:   "newrowDelete_", /* (use for NewRow table) Prefix text for the new delete button */
        newrowKeyPrefix:   "newrowKey_"     /* (use for NewRow table) Prefix text for the new row key */
    },

    /* Build a dynamic table form for JSON data receive/transmit. */
    _build: function() {
        // Get reference element
        var my_ref = $(this.localVars.reference);
        if (!my_ref) {
            alert("Cannot find element ID: " + ref);
            return;
        }

        // Do nothing if Tbody already existing
        if ($(this.localVars.reference + "Tbody")) {
            return;
        }

        // Create element <form>
        if (this.localVars.type == "config") {
            var my_form = document.createElement("form");
            my_form.setAttribute('method',"post");
            my_form.setAttribute('action',"/json_rpc");
            my_form.id = this.localVars.reference + "Form";
        }

        // Create elements <table>, <thead> and <tbody>
        var my_tbl = document.createElement("table");
        if (this.localVars.subtype != "columnOrder") {
            var my_tbl_header = document.createElement("thead");
            my_tbl.appendChild(my_tbl_header);
            my_tbl_header.id = this.localVars.reference + "THeader";
        }

        var style = (this.localVars.type == "config" ? "config" : "display");
        if (navigator.appName && navigator.appName == 'Microsoft Internet Explorer') {
            my_tbl.setAttribute("className", style);
        }
        my_tbl.setAttribute("class", style);

        var my_tbl_body = document.createElement("tbody");
        my_tbl_body.id = this.localVars.reference + "Tbody";

        // Append to reference element
        my_tbl.appendChild(my_tbl_body);
        if (this.localVars.type == "config") {
            my_form.appendChild(my_tbl);
            my_ref.appendChild(my_form);
        } else {
            my_ref.appendChild(my_tbl);
        }
    },

    /* Validate supported table types/subtypes.
     *
     * @type:    [Mandatory], table type.
     * @subtype: [Optional], table sub-type.
     */
    _isValidTableType: function(type, subtype) {
        if (type != "config" && type != "display") {
            alert("Not implemented yet in _isValidTableType(): " + type);
            return false;
        }

        if (subtype &&
            subtype != "columnOrder" &&
            subtype != "plusNewRow" &&
            subtype != "plusRowCtrlBar") {
            alert("Not implemented yet in _isValidTableType(): " + subtype);
            return false;
        }

        return true;
    },

    /* Validate row syntax.
     *
     * @row: [Mandatory], row data with JSON syntax.
     */
    _isValidRowSyntax: function(row) {
        var syntax_err = false, field;

        if (!row.fields || (row.validate && (!row.validate.func || typeof(row.validate.func) != "function" || !row.validate.params))) {
            syntax_err = true;
        }

        for (var i = 0; i < row.fields; i++) {
            field = row.fields[i];
            if (!field.type || !field.params || (field.validate && (!field.validate.func || !field.validate.params))) {
                syntax_err = true;
                break;
            }
        }

        if (syntax_err) {
            alert("JSON syntax error.\n{fields:[\n    {type:<field_type>,\n    params:[<params_0>, <params_1>, <params_2>, ...]},\n    validate:{ func:<func_name>, params:[<param_0>, <params_1>, ...] }\n]}");
            return false;
        }
        return true;
    },

    /* Check if it is header row */
    _isHeaderRow: function(row) {
        return (row.fields[0].type == "disp_hdr");
    },

    /* Add new fields in <tr> element with JSON syntax.
     * It will return an array with field elements for advanced attriable modification.
     *
     * Syntax: fields:[<field_0>, <field_1>, ...]
     *         <field_n> - {"type":<type>, "params":[<params_0>, <params_1>, ...]}
     *
     * When using the JSON syntax for the Web page layout, the "fields" attriable
     * is used to add new fields in element <tr> according to the original
     * dynamic-form function.
     *
     * @tr:     [Mandatory], <tr> element.
     * @fields: [Mandatory], fields data with JSON syntax.
     *
     * ===========================================================================
     * | Type          | Description/Mapping   | Parameters
     * ===========================================================================
     * | "conf_hdr"    | Configured header     | @text:     [Mandatory], header text
     * |               | addTextCell()         |
     * ----------------|-----------------------|----------------------------------
     * | "disp_hdr"    | Display header        | @text:     [Mandatory], header text
     * |               | addTextHeaderCell()   | @colspan:  [Optional],  column span
     * |               |                       | @rowspan:  [Optional],  row span
     * |               |                       | @width:    [Optional],  width (e.g. "100px")
     * |               |                       | @name:     [Optional],  Name/ID
     * ----------------|-----------------------|----------------------------------
     * | "text"        | Text                  | @text:     [Mandatory], header text
     * |               | addTextCell()         | @style:    [Optional],  CSS style
     * |               |                       | @colspan:  [Optional],  column span
     * |               |                       | @name:     [Optional],  Name/ID
     * ----------------|-----------------------|----------------------------------
     * | "select"      | Select box            | @oT:       [Mandatory], an array containing select option texts
     * |               | addSelectCell()       | @oV:       [Mandatory], an array containing select option text mapping values
     * |               |                       | @value:    [Optional],  select value for this select box
     * |               |                       | @style:    [Optional],  CSS style
     * |               |                       | @name:     [Mandatory], Name/ID
     * |               |                       | @width:    [Optional],  width
     * |               |                       | @colspan:  [Optional],  column span
     * |               |                       | @disabled: [Optional],  disable this element
     * |               |                       | @onchange: [Optional],  onchange callback event
     * ----------------|-----------------------|----------------------------------
     * | "multi_select"| Multi-select box      | @oT:       [Mandatory], an array containing select option texts
     * |               | addMultiSelectCell()  | @oV:       [Mandatory], an array containing select option text mapping values
     * |               |                       | @values:   [Optional],  an array containing select values for this select box
     * |               |                       | @style:    [Optional],  CSS style
     * |               |                       | @name:     [Mandatory], Name/ID
     * |               |                       | @size:     [Optional],  display row size
     * |               |                       | @width:    [Optional],  width
     * ----------------|-----------------------|----------------------------------
     * | "input"       | Input text            | @text:     [Optional],  input text
     * |               | addInputCell()        | @style:    [Optional],  CSS style
     * |               |                       | @name:     [Mandatory], Name/ID
     * |               |                       | @size:     [Mandatory], number of chars to display
     * |               |                       | @maxsize:  [Optional],  maximum number of chars to enter (a.k.a. maxlength)
     * |               |                       | @width:    [Optional],  width
     * |               |                       | @colspan:  [Optional],  column span
     * |               |                       | @disabled: [Optional],  disable this element
     * |               |                       | @onchange: [Optional],  onchange callback event
     * ----------------|-----------------------|----------------------------------
     * | "input_area"  | Input Area            | @text:     [Optional],  input text
     * |               | addInputAreaCell()    | @style:    [Optional],  CSS style
     * |               |                       | @name:     [Mandatory], Name/ID
     * |               |                       | @cols:     [Mandatory], display column size
     * |               |                       | @rows:     [Mandatory], display row size
     * ----------------|-----------------------|----------------------------------
     * | "radio"       | Radio button          | @checked:  [Optional],  0: none checked, 1: checked
     * |               | addRadioCell()        | @style:    [Optional],  CSS style
     * |               |                       | @name:     [Mandatory], Name
     * |               |                       | @id:       [Mandatory], ID
     * |               |                       | @text:     [Mandatory], appended text for this radio button
     * ----------------|-----------------------|----------------------------------
     * | "chkbox"      | Checkbox              | @checked:  [Optional],  0: none checked, 1: checked
     * |               | addCheckBoxCell()     | @style:    [Optional],  CSS style
     * |               |                       | @name:     [Mandatory], Name/ID
     * |               |                       | @onchange: [Optional],  onchange callback event
     * ----------------|-----------------------|----------------------------------
     * | "button"      | Button                | @text:     [Mandatory], button display text
     * |               | addButtonCell()       | @style:    [Optional],  CSS style
     * |               |                       | @name:     [Optional],  Name/ID
     * |               |                       | @onclick:  [Optional],  onclick callback event
     * ----------------|-----------------------|----------------------------------
     * | "image"       | Image                 | @style:    [Optional],  CSS style
     * |               | addImageCell()        | @src:      [Mandatory], image source
     * |               |                       | @text:     [Optional],  alert text
     * |               |                       | @name:     [Optional],  Name/ID
     * |               |                       | @onclick:  [Optional],  onclick callback event
     * ----------------|-----------------------|----------------------------------
     * | "password"    | Password input text   | @text:     [Mandatory], input text
     * |               | addPasswordCell()     | @style:    [Optional],  CSS style
     * |               |                       | @name:     [Mandatory], Name/ID
     * |               |                       | @size:     [Mandatory], input text size
     * ----------------|-----------------------|----------------------------------
     * | "link"        | Hyperlink text        | @style:    [Optional],  CSS style
     * |               | addLink()             | @url:      [Mandatory], the URL for this hyperlink text
     * |               |                       | @text:     [Mandatory], hyperlink text
     * |               |                       | @target:   [Optional],  link target after click this hyperlink text
     * ----------------|-----------------------|----------------------------------
     * | "hidden_input"| Hidden input text     | @text:     [Mandatory], input text
     * |               | addHiddenInputCell()  | @name:     [Mandatory], Name/ID
     * |               |                       | @size:     [Mandatory], input text size
     * ----------------|-----------------------|----------------------------------
     * | "digit"       | Digit                 | @text:     [Mandatory], digit text
     * |               | addTextCell()         | @colspan:  [Optional],  column span
     * |               |                       | @name:     [Optional],  Name/ID
     * ----------------|-----------------------|----------------------------------
     * | "empty_row"   | "No entry exists"     | @colspan:  [Optional],  column span
     * |               |                       | @text:     [Optional],  display text
     * |               | addTextCell()         |
     * ===========================================================================
     */
    _addRowFields: function(tr, fields) {
        var elems = new Array();

        Object.each(fields, function(field, i) {
            var params = field.params;
            switch(field.type) {
            // Headers
            case "conf_hdr":
                elems.push(addTextCell(tr, params[0], "param_label"));
                break;
            case "disp_hdr":
                if (navigator.appName && navigator.appName == 'Microsoft Internet Explorer') {
                    tr.setAttribute("className", "display_header");
                }
                tr.setAttribute("class", "display_header");

                elems.push(addTextHeaderCell(tr, params[0], "hdrc", params[1], params[2], params[3], params[4]));
                break;

            // Basic elements
            case "text":
                elems.push(addTextCell(tr, params[0], params[1] ? params[1] : "cl", params[2], params[3]));
                break;
            case "select":
                elems.push(addSelectCell(tr, params[0], params[1], params[2], params[3], params[4], params[5], params[6], params[7], params[8]));
                break;
            case "multi_select":
                elems.push(addMultiSelectCell(tr, params[0], params[1], params[2], params[3], params[4], params[5], params[6]));
                break;
            case "input":
                elems.push(addInputCell(tr, params[0], params[1], params[2], params[3], params[4] ? params[4] : params[3], params[5], params[6], params[7], params[8]));
                break;
            case "input_area":
                elems.push(addInputAreaCell(tr, params[0], params[1], params[2], params[3], params[4]));
                break;
            case "radio":
                elems.push(addRadioCell(tr, params[0], params[1], params[2], params[3], params[4]));
                break;
            case "chkbox":
                elems.push(addCheckBoxCell(tr, params[0], params[1], params[2], params[3]));
                break;
            case "button":
                elems.push(addButtonCell(tr, params[0], params[1], params[2], params[3]));
                break;
            case "image":
                elems.push(addImageCell(tr, params[0] ? params[0] : "c", params[1], params[2] ? params[2] : " ", params[3], params[4]));
                break;
            case "password":
                elems.push(addPasswordCell(tr, params[0], params[1], params[2], params[3]));
                break;
            case "link":
                elems.push(addLink(tr, params[0], params[1], params[2]));
                break;

            // Hidden elements
            case "hidden_input":
                elems.push(addHiddenInputCell(tr, params[0], "hidden", params[1], params[2]));
                break;

            // Proprietary field type
            case "digit":
                elems.push(addTextCell(tr, params[0], "cr", params[1], params[2]));
                break;
            case "empty_row":
                elems.push(addTextCell(tr, params[1] ? params[1] : "No entry exists", "c", params[0]));
                break;

            // Miscellaneous

            default:
                elems.push(null);
                alert("Not implemented yet in _addRowFields(): " + field.type);
            }

            if (field.id && elems[i]) {
                elems[i].id = elems[i].name = field.id;
            }

            if (field.disabled && elems[i]) {
                elems[i].disabled = true;
            }

            if (field.hidden && elems[i]) {
                var td = findParentElem(elems[i], "td");
                if (td) {
                    td.hidden = true;
                    td.style.display = "none";  // For MSIE
                } else {
                    elems[i].hidden = true;
                    elems[i].style.display = "none";  // For MSIE
                }
            }

            if (field.colspan && elems[i]) {
                elems[i].setAttribute("colSpan", field.colspan);
            }

            if (field.rowspan && elems[i]) {
                elems[i].setAttribute("rowSpan", field.rowspan);
            }

            if (field.onchange && elems[i]) {
                elems[i].onchange = field.onchange;
            }
        });
        return elems;
    },

    /* The Callback function for "New Row" button
     *
     * @add_newrow_cb: [Mandatory], calllback function for add new row.
     */
    _newRowButtonCb: function(add_newrow_cb) {
        // Get a new entry ID
        var new_idx = this._getNewRowIndex(this.localVars.maxRows);
        if (!parseInt(new_idx, 10)) {
            return;
        }

        // Get new row data from callback function
        var new_row = add_newrow_cb(new_idx, null, this.localVars.newrowKeyPrefix);
        new_row.new_idx = new_idx;
        new_row.fields[0] = {type:"button", params:["Delete", "c", this.localVars.newrowDelPrefix + new_idx]};

        var elem = this.addNewRow(new_row);
        var dynamic_table = this;
        elem[0].onclick = function() { dynamic_table.delNewRow(dynamic_table.localVars.newrowDelPrefix, this.id); };
    },

    /* Get a new available row index.
     *
     * Return Values:
     *  0 - When reached the maximum entries number.
     *  None zero - the available entry index (start from 1).
     */
    _getNewRowIndex: function() {
        // Count current valid rows
        var fld, valid_row_cnt = 0;
        this.localVars.rows.each(function(row) {
            if (!this._isHeaderRow(row) && row.key) {
                fld = $(this.localVars.rowDelPrefix + rowKey2Id(row.key));
                if (fld && !fld.checked) {
                    valid_row_cnt++;
                }
            }
        }, this);
        for (idx = 1; idx <= this.localVars.maxRows - valid_row_cnt; idx++) {
            if (!$(this.localVars.newrowDelPrefix + idx)) {
                return idx;
            }
        }

        alert("Reached the maximum entries number (" + this.localVars.maxRows + ").");
        return 0;
    },

    /* Submit <form> data
     *
     * @method_name_prefix: [Mandatory], method name for JSON request.
     * @callback:           [Mandatory], callback function when done the JSON request process.
     * @cb_params:          [Optional], callback parameters.
     */
    _submitForm: function(method_name_prefix, cb_func, cb_params) {
        this._debugMsg("_submitForm", "method_name_prefix: " + method_name_prefix);

        // Each table form can follow the similar precedures below.
        // 1. Convert HTML query string to JSON
        var submit_json = html2Json($(this.localVars.reference + "Tbody"), this.localVars.trim_ids);
        var json_params;
        if (Object.prototype.toString.call(submit_json) == "[object Array]") {
            json_params = submit_json;
        } else {
            json_params = [submit_json];
        }
        if (typeof(this.localVars.jsonReqSendCb) == "function") {
            if (this.localVars.jsonReqSendCb(json_params, method_name_prefix + ".set") == false) {
                // The callback function terminated the sending process.
                // Restore the 'disabled' attriable of the submit button.
                var submit_elem = $(this.localVars.reference + "submitButton");
                if (submit_elem) {
                    $(this.localVars.reference + "submitButton").disabled = false;
                }
                return;
            }
        }

        // 2. Submit data with JSON syntax
        this._setDoneSubmitFlag(false);
        requestJsonDoc(method_name_prefix + ".set", json_params, this._doneSubmitCallback, this);
        if (cb_func && typeof(cb_func) == "function") {
            this._waitDoneSubmitFlag();
            milliSleep(500);    // Made a little bit delay here to avoid the JSON RPC process the GET operation faster than SET operation.
            cb_func.apply(cb_params);
        }
    },

    /* Submit rows data
     *
     * @recv_json_name:     [Mandatory], name of received JSON data.
     * @method_name_prefix: [Mandatory], method name for JSON request.
     * @cb_func:            [Mandatory], callback function when done the JSON request process.
     * @cb_params:          [Optional], callback parameters.
     */
    _submitRows: function(recv_json_name, method_name_prefix, cb_func, cb_params) {
        this._debugMsg("_submitRows", "recv_json_name: " + recv_json_name + ", method_name_prefix: " + method_name_prefix);

        // Ignore the submission if no available table data there.
        if ((this.localVars.rows.length == 0 && this.localVars.newrows.length == 0) ||
            (this.localVars.rows.length == 1 && this.localVars.rows[0].fields[0].type == "empty_row")) {
            alert("Warning!\n\nThe action is ignored since there is no available table data for the submission.");
            return;
        }

        // Lookup original entries twice
        // Process DELETE operation first then SET operation
        for (var idx = 0; idx < 2; idx++) {
            this.localVars.rows.each(function(row, i) {
                if (!this._isHeaderRow(row) && row.key != NaN) {
                    var key = row.key;
                    var fld = $(this.localVars.rowDelPrefix + rowKey2Id(key));

                    if (idx == 0) {  // Process DELETE operation
                        if (fld && fld.checked) {
                            var submit_json;
                            if (Object.prototype.toString.call(key) == "[object Array]" || Object.prototype.toString.call(key) == "[object Object]") {
                                submit_json = key;
                            } else {
                                submit_json = [this.localVars.rowKeyIsIfDesc ? key2IfDesc(this.localVars.rowKeyIfType, this.localVars.rowKeyIsZeroBased ? key + 1 : key) : key];
                            }

                            // Sumbit entry data only (without reload)
                            this._debugMsg("_submitRows", "calling requestJsonDoc(" + method_name_prefix + ".del)");
                            this._setDoneSubmitFlag(false);
                            requestJsonDoc(method_name_prefix + ".del", submit_json, this._doneSubmitCallback, this);
                            this._waitDoneSubmitFlag();
                        }
                    } else {    // Process SET operation
                        if (!fld || !fld.checked) { // No DELETE button exists or DELETE button exists but is not checked.
                            // Clone a new row and trim unnessary fields
                            var new_row;
                            if (fld) {
                                new_row = copyHtmlObject(findParentElem(fld, "tr"));
                                new_row.removeChild(new_row.firstChild); // Remove "Delete" field
                            } else {
                                new_row = findParentElem($(this.localVars.rowKeyPrefix + rowKey2Id(key)), "tr");
                            }
                            if (!fld && !new_row) {
                                alert("DynamicTable._submitRows() failed: Cannot find ID name '" +
                                      this.localVars.rowDelPrefix + rowKey2Id(key) + "' or '" +
                                      this.localVars.rowKeyPrefix + rowKey2Id(key) + "'\n\n" +
                                      "1. Add a delete checkbox for the entry if this table support dynamic entry creating.\n" +
                                      "2. Add a <TD> element and named with '" + this.localVars.rowKeyPrefix + rowKey2Id(key) + "'. (hidden if needed)");
                            }

                            // Trim unnecessary field
                            if (new_row) {
                                new_row = removeHtmlObject(new_row, this.localVars.trim_ids);
                            }

                            // Sumbit entry data only (without reload)
                            this._debugMsg("_submitRows", "calling submitJsonRow(" + method_name_prefix + ".set)");
                            var recv_json = this.getRecvJson(recv_json_name);
                            if (fld && recv_json[i] && recv_json[i].key.length && !recv_json[i].val) {
                                // Ignore the 'set' operation if all parameters are entry keys
                                this._debugMsg("_submitRows", "ignore submitJsonRow(" + method_name_prefix + ".set) since all parameters are entry keys");
                            } else {
                                this._setDoneSubmitFlag(false);
                                var submit_json = this.localVars.rowKeyIsIfDesc ? key2IfDesc(this.localVars.rowKeyIfType, this.localVars.rowKeyIsZeroBased ? key + 1 : key) : key;
                                submitJsonRow(fld ? recv_json[i].val : null, new_row, submit_json, "_" + this.localVars.rowKeyPrefix + rowKey2Id(key), method_name_prefix + ".set", this._doneSubmitCallback, this, this.localVars.jsonReqSendCb);
                            }
                        }
                    }
                }
            }, this);
        }
    },

    /* Submit new rows data
     *
     * @method_name_prefix: [Mandatory], method name for JSON request.
     * @cb_func:            [Mandatory], callback function when done the JSON request process.
     * @cb_params:          [Optional], callback parameters.
     */
    _submitNewRows: function(method_name_prefix, cb_func, cb_params) {
        this._debugMsg("_submitNewRows", "method_name_prefix: " + method_name_prefix);

        // Lookup new entries (the valild ID starts from 1)
        var new_idx = 1;
        this.localVars.newrows.each(function(row) {
            var fld = $(this.localVars.newrowDelPrefix + new_idx);
            if (fld) {
                // Need to consider the regular expression matched patten here.
                // For example, both newrowKey_1_xxx and newrowKey_11_xxx are
                // matched the searching patten 'newrowKey_1'.
                // That is the reason we added '_' at end of searching patten.
                var key_elems = $$('[id^=' + this.localVars.newrowKeyPrefix + new_idx + '_]');
                var key = this.localVars.newrows[new_idx - 1].key;

                if (Object.prototype.toString.call(key) == "[object Array]") {
                    // Update newrows key values
                    var key_elems_idx = 0;
                    key.each(function(elem, elem_idx) {
                        if (Object.prototype.toString.call(elem) == "[object Object]") {
                            Object.each(Object.keys(elem), function(k) {
                                elem[k] = $(this.localVars.newrowKeyPrefix + new_idx + '_' + k).value;
                            }, this);
                        } else {
                            key[elem_idx] = $(key_elems[key_elems_idx].id).value;
                        }
                        key_elems_idx = key_elems_idx + 1;
                    }, this);
                } else if (Object.prototype.toString.call(key) == "[object Object]" || this.localVars.rowKeyIsHashObj) {
                    key = new Hash();
                    key_elems.each(function(elem) {
                        var key_name = elem.id.substring((this.localVars.newrowKeyPrefix + new_idx + "_").length);
                        if (key_name.length) {
                            key.set(key_name, $(elem.id).value);
                        }
                    }, this);
                } else {
                    key = $(this.localVars.newrowKeyPrefix + new_idx).value;
                    key_elems.push(this.localVars.newrowKeyPrefix + new_idx);
                }

                // Clone a new row and trim unnessary fields
                var new_row = copyHtmlObject(findParentElem(fld, "tr"));
                new_row.removeChild(new_row.firstChild); // Remove "New Delete" button
                key_elems.each(function() {
                    new_row.removeChild(new_row.firstChild); // Remove "New Key" field
                });

                // Trim unnecessary fields
                if (new_row) {
                    new_row = removeHtmlObject(new_row, this.localVars.trim_ids);
                }

                // Sumbit entry data only (without reload)
                this._debugMsg("_submitRows", "calling submitJsonRow(" + method_name_prefix + ".add)");
                this._setDoneSubmitFlag(false);
                submitJsonRow(null, new_row, this.localVars.rowKeyIsIfDesc ? key2IfDesc(this.localVars.rowKeyIfType, key) : this.localVars.rowKeyIsZeroBased ? key - 1 : key, "_" + this.localVars.newrowKeyPrefix + new_idx, method_name_prefix + ".add", this._doneSubmitCallback, this, this.localVars.jsonReqSendCb);
                new_idx++;
            }
        }, this);
    },

    /* Set the submit operation done flag
     *
     * @value: [Mandatory], Boolean flag.
     */
    _setDoneSubmitFlag: function(value) {
        if (!value) {
            this._waitDoneSubmitFlag();
        }
        this._debugMsg("_setDoneSubmitFlag", "Set 'DoneSubmit' flag: " + value);
        this.localVars.doneSubmitFlag = value;
    },

    /* Waiting until done the submit operation */
    _waitDoneSubmitFlag: function() {
        this._debugMsg("_waitDoneSubmitFlag", "Waiting until done the submit operation ...");
        var wait_cnt = 0x1FFFF;
        while (--wait_cnt) {
            if (this.localVars.doneSubmitFlag) {
                return;
            }
        }
        this._debugMsg("_waitDoneSubmitFlag", "Timeout to wait the submit operation done.");
    },

    /* The callback fnuction when done the submit operation.
     * It is a internal function for submit row data one by one.
     *
     * @recv_json: [Mandatory], received json data.
     * @dyna_html_obj: [Mandatory], my dynamic table object.
     */
    _doneSubmitCallback: function(recv_json, dyna_html_obj) {
        dyna_html_obj._setDoneSubmitFlag(true);
    },

    /* Alert debug message.
     *
     * @func_name: [Mandatory], function name.
     * @msg:       [Mandatory], debug message.
     */
    _debugMsg: function(func_name, msg) {
        // Group similar functions
        var submit_group = (func_name == "submitEvent" ||
                            func_name == "_submitForm" ||
                            func_name == "_submitRows" ||
                            func_name == "_submitNewRows" ||
                            func_name == "_setDoneSubmitFlag" ||
                            func_name == "_waitDoneSubmitFlag" ||
                            func_name == "_doneSubmitCallback"
                            );

        if ((this.debug.submitEvent  && submit_group) ||
            (this.debug.saveRecvJson && func_name == "saveRecvJson") ||
            (this.debug.getRecvJson  && func_name == "getRecvJson") ||
            (this.debug.validate     && func_name == "validate") ||
            this.debug.all) {
            alert("Function Name: " + func_name + "\n\n" + msg);
        }
    },

    /* Dump Received JSON data.
     *
     * @json_obj: [Mandatory], JSON object.
     */
    _dumpRecvJson: function(json_obj) {
        var msg = "";

        // To-do, not good enough
        if (Object.prototype.toString.call(json_obj) == "[object Array]") {
            // JSON syntax: [ { "key":key, val:{"v1":v1, "v2":v2, ...} }, ... ]
            Object.each(json_obj, function(record) {
                msg += ("key: " + record.key + "\n");
                if (record.val) {
                    Object.each(Object.keys(record.val), function(key) {
                        msg += (key + ": " + record.val[key] + "\n");
                    });
                }
            });
        } else {
            Object.each(Object.keys(json_obj), function(key) {
                msg += (key + ": " + json_obj[key] + "\n");
            });
        }

        return msg;
    },

    /* -----------------------------------------------------------------------
     * Examples
     *
     * Each table can follow the similar precedures below.
     * // 1. Create a form with table body for receive/transmit JSON data
     * myDynamicTable = new DynamicTable(ref, "config", "columnOrder");
     *
     * // 2, Ignore the process if no data is received
     * if (!recv_json) {
     *     alert("Get dynamic data failed.");
     *     return;
     * }
     *
     * // 3. Save the received JSON data
     * myDynamicTable.saveRecvJson("config", recv_json);
     *
     * // 4. Add table rows
     * var table_rows = addTableRows(recv_json);
     * myDynamicTable.addRows(table_rows);
     *
     * // 5. Update this dynamic table
     * myDynamicTable.update();
     *
     * // 6. Add buttons
     * myDynamicTable.addSubmitButton("config", "xxx.config.global", requestUpdate);
     * myDynamicTable.addResetButton();
     * ---------------------------------------------------------------------*/
     /* Example: Add single row */
    addRowEx: function() {
        var row = {key:null, fields:[]};
        row.fields[0] = {type:"text", params:["row: 0 field: 0"]};
        row.fields[0] = {type:"text", params:["row: 0 field: 1"]};
        this.addRow(row);
    },

    /* Example: Add multiple rows (Ex.1) */
    initializeRowsEx: function() {
        var row, table_rows = this.initializeRows(2, 2);

        // 1st row
        row = table_rows[0];
        row.fields[0] = {type:"text", params:["row: 0 field: 0"]};
        row.fields[1] = {type:"text", params:["row: 0 field: 1"]};

        // 2nd row
        row = table_rows[1];
        row.fields[0] = {type:"text", params:["row: 1 field: 0"]};
        row.fields[1] = {type:"text", params:["row: 1 field: 1"]};

        this.addRows(table_rows);
    },

    /* Example: Add multiple rows (Ex.2) */
    addRowsEx: function() {
        var table_rows = [
            {fields:[
                {type:"text", params:["row: 0 field: 0"]},
                {type:"text", params:["row: 0 field: 1"]}
              ]
            },
            {fields:[
                {type:"text", params:["row: 1 field: 0"]},
                {type:"text", params:["row: 1 field: 1"]}
              ]
            },
        ];

        this.addRows(table_rows);
    },

    /* Get new rows count */
    getNewrowsCnt: function() {
        return this.localVars.newrows.length;
    },

    getRowsCnt: function() {
        return this.localVars.rows.length;
    }
}); /* Class::DynamicTable */

/**
 * Get element object from JSON specification.
 *
 * @json_spec    [Mandatory], the HTML object of JSON specification.
 * @catalog      [Mandatory], the catalog in the JSON specification.
 * @search_name  [Mandatory], the searching name in the JSON specification.
 * @element_name [Optional], the element name in the JSON specification.
 *
 * The syntax of JSON specification:
 * { "types":  [ {type_1},   {type_2}, ...,   {type_n} ],
 *   "groups": [ {group_1},  {type_2}, ...,   {group_n} ],
 *   "methods":[ {method_1}, {method_2}, ..., {method_n} ] }
 *
 * {type_n}: { "type-name": <type_name>,
 *             "class": <type_class>,
 *             "description": <type_desc>,
 *             "encoding-type": <type_encoding>,
 *             "elements": [ {type_elem_1}, {type_elem_2}, ..., {type_elem_n} ]
 *           }
 * {type_elem_n}: { "name": <type_elem_name>,
 *                  "type": <type_elem_type>,
 *                  "description": <type_elem_desc>
 *                }
 *
 * {group_n}: { "group-name": <group_name>,
 *              "description": <group_desc>
 *            }
 *
 * {method_n}: { "method-name": <method_name>,
 *               "description": <method_desc>,
 *               "web-privilege": <web_privilege>,
 *               "params":[ {method_param_1}, {method_param_2}, ..., {method_param_n} ],
 *               "result": [ { "name": "spec",
 *                             "description": <result_desc>,
 *                             "type": "vtss_appl_json_rpc_inventory_t" } ]
 * {method_param_n}: { "name": <method_param_name>,
 *                     "type": <method_param_type>,
 *                     "description": <method_param_desc>
 *                   }
 */
function getJsonSpecElement(json_spec, catalog, search_name, element_name) {
    // Check the input parameters
    if (catalog != "types" && catalog != "groups" && catalog != "methods") {
        alert("Only ['types', 'groups', 'methods'] catalogs are supported.");
    }
    if (catalog == "methods" && element_name != "brief" && element_name != "detail") {
         alert("Only ['brief', 'detail'] catalogs are supported.");
    }

    var json_catalog = catalog == "types" ? json_spec.types :
                       catalog == "groups" ? json_spec.groups : json_spec.methods;

    // Find the matched element by name
    for (var catalog_idx = 0; catalog_idx < json_catalog.length; ++catalog_idx) {
        var search_elem = json_catalog[catalog_idx];
        switch (catalog) {
            case "types":
                if (search_elem["type-name"] == search_name) {
                    if (element_name && search_elem.elements) {
                        // Find the specific element name
                        for (var elem_idx = 0; elem_idx < search_elem.elements.length; ++elem_idx) {
                            var type_elem = search_elem.elements[elem_idx];
                            if (type_elem.name == element_name) {
                                // Found it
                                return JSON.parse(JSON.stringify(type_elem));
                            }
                        }
                    } else {
                        // Return the current element which matched the 'search_elem'
                        return JSON.parse(JSON.stringify(search_elem));
                    }
                }
                break;
            case "groups":
                if (search_elem["group-name"] == search_name) {
                    // Found it
                    return JSON.parse(JSON.stringify(search_elem));
                }
                break;
            case "methods":
                if (search_elem["method-name"] == search_name) {
                    if ((element_name == "brief" && !search_elem.description) ||
                        (element_name == "detail" && search_elem.description)) {
                        // Find the specific element
                        return JSON.parse(JSON.stringify(search_elem));
                    }
                }
                break;
        }
    }

    alert("getJsonSpecElement() failed: Cannot find element description." +
          "\nCatalog: " + catalog +
          "\nSearch_name: " + search_name +
          (element_name ? ("\nElement_name: " + element_name) : ""));

    return null;
}

/**
 * Update HTML table parameters description.
 *
 * The function is used to fill all parameters description automatically.
 *
 * @update_elem_id [Mandatory], the HTML element ID which need to be updated.
 * @alias          [Mandatory], the alias of table parametern.
 * @desc           [Mandatory], the description of table parameter.
 * @dt_id          [Optional], given the ID for the HTML tag <dt>.
 * @dt_class       [Optional], given the class for the HTML tag <dt>.
 * @dd_id          [Optional], given the ID for the HTML tag <dd>.
 * @dd_class       [Optional], given the class for the HTML tag <dd>.
 */
function appendTableParamsDesc(update_elem_id, alias, desc, dt_id, dt_class, dd_id, dd_class) {
    var update_elem = $(update_elem_id);
    // Add parameter name
    var dt = document.createElement("dt");
    if (dt_id) {
        dt.id = dt.name = dt_id;
    }
    if (dt_class) {
        dt.setAttribute("class", dt_class); // For FF
        dt.setAttribute("className", dt_class); // For MSIE
    }
    dt.innerHTML = alias;
    update_elem.appendChild(dt);

    // Add parameter description
    var dd = document.createElement("dd");
    if (dd_id) {
        dd.id = dd.name = dd_id;
    }
    if (dd_class) {
        dd.setAttribute("class", dd_class); // For FF
        dd.setAttribute("className", dd_class); // For MSIE
    }
    dd.innerHTML = desc;
    update_elem.appendChild(dd);
}

/**
 * Update HTML table parameters description.
 *
 * The function is used to fill all parameters description automatically.
 *
 * @update_elem_id [Mandatory], the HTML element ID which need to be updated.
 * @json_spec      [Mandatory], the HTML object of JSON specification.
 * @method_name    [Mandatory], the JSON method name.
 * @param_names    [Mandatory], use Array object to fill each name of table parameter.
 * The array element syntax:
 * { "alias": <element_alias>,      [Mandatory]
 *   "type": <element_type>,        [Optional, it is used when this element is the key/value pair]
 *   "name": <element_name>,        [Mandatory, it is mandatory parameter when lack of <specific_desc> ]
 *   "description": <specific_desc> [Optional, output this specific description]
 *   "prefix": <prefix_text>        [Optional], the prefix text is added the front of parameter description.
 *   "suffix": <suffix_text>        [Optional], the suffix text is added the end of parameter description.
 *   "dt_id": <dt_id>               [Optional], given the ID for the HTML tag <dt>.
 *   "dt_class": <dt_class>         [Optional], given the class for the HTML tag <dt>.
 *   "dd_id": <dd_id>               [Optional], given the ID for the HTML tag <dd>.
 *   "dd_class": <dd_class>         [Optional], given the class for the HTML tag <dd>.
 * }
 *
 * For the new line, use <br> in the description.
 *
 * For example,
 * { "alias": "Instance ID", "name": "InstanceId" },
 * { "alias": "Network Address", "type": "mesa_ipv4_network_t", "name": "Network" },
 * { "alias": "Mask Length",     "type": "mesa_ipv4_network_t", "name": "IpSubnetMaskLength" },
 */
function updateTableParamsDesc(update_elem_id, json_spec, method_name, param_names) {
    var json_methods = json_spec.methods;

    /* Walk through all help parameters */
    for (var param_idx = 0; param_idx < param_names.length; ++param_idx) {
          var param = param_names[param_idx];
        if (param.description && !param.prefix && !param.suffix) {
            appendTableParamsDesc(update_elem_id, param.alias, param.description,
                                  param.dt_id, param.dt_class, param.dd_id, param.dd_class);
            continue; // Found the description of the parameter, continue next parameter processing
        } else if (param.type) {
            var type_elem = getJsonSpecElement(json_spec, "types", param.type, param.name);
            if (type_elem) {
                appendTableParamsDesc(update_elem_id, param.alias,
                                      (param.prefix ? param.prefix : "") + type_elem.description + (param.suffix ? param.suffix : ""),
                                       param.dt_id, param.dt_class, param.dd_id, param.dd_class);
                continue; // Found the description of the parameter, continue next parameter processing
            }
        }

        /* Walk through all methods */
        for (var method_idx = 0; method_idx < json_methods.length; ++method_idx) {
            var method = json_methods[method_idx];
            var found = false;

            /* Walk through all elements in the method parameter */
            for (var method_param_idx = 0; method_param_idx < method.params.length; ++method_param_idx) {
                var elem = method.params[method_param_idx];
                if (param.name == elem.name) {
                    var type_elem = getJsonSpecElement(json_spec, "types", elem.type, elem.name);
                    if (type_elem) {
                        appendTableParamsDesc(update_elem_id, param.alias,
                                              (param.prefix ? param.prefix : "") + type_elem.description + (param.suffix ? param.suffix : ""),
                                               param.dt_id, param.dt_class, param.dd_id, param.dd_class);
                        found = true;
                        break; // Found the description of the parameter, continue next parameter processing
                    }
                }
            }
            if (found) {
                break; // Found the description of the parameter, continue next parameter processing
            }

            /* Walk through all elements in the method result */
            for (var method_res_idx = 0; method_res_idx < method.result.length; method_res_idx++) {
                var elem = method.result[method_res_idx];
                if (param.name == elem.name) {
                    var type_elem = getJsonSpecElement(json_spec, "types", elem.type, elem.name);
                    if (type_elem) {
                        appendTableParamsDesc(update_elem_id, param.alias, type_elem.description);
                        found = true;
                        break; // Found the description of the parameter, continue next parameter processing
                    }
                }
            }
            if (found) {
                break; // Found the description of the parameter, continue next parameter processing
            }
        };
    }
}
