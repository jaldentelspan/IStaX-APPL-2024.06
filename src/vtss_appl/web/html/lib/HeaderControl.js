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

// *****************************  HeaderControl.js  ***************************
// *
// * Author: Lars Povlsen
// *
// * --------------------------------------------------------------------------
// *
// * Description:  Control-Bar class (MooTools 1.2 or greater)
// *
// * --------------------------------------------------------------------------

function HeaderControlSetup(mName, sName, onchange)
{
    var ctl = $(mName);
    if(typeOf(ctl) != "element") {
        console.error("%s is of type %s, expecting element", mName, typeOf(ctl));
        return;
    }
    if(onchange) {
        ctl.addEvent('change', onchange.bind(ctl, [sName]));
    } else if(ctl.type == "select-one")
        ctl.addEvent('change', function() {
                var val = this.get('value');
                $$('select[id^="' + sName + '"]:enabled').each(function(item) {
                        item.getElements('option').each(function(opt, i) {
                                if (val == opt.value && opt.disabled != true) {
                                    opt.selected = true;
                                    item.selectedIndex = i;
                                    $try(function() {item.onchange()});
                                }
                            });
                    });
            });
    else if(ctl.type == "select-multiple")
        ctl.addEvent('change', function() {
                var val = Array();
                for (var i=0, j=0; i<this.options.length; i++) {
                    if(this.options[i].selected) {
                        val[j++] = this.options[i].value;
                    }
                }
                $$('select[id^="' + sName + '"]:enabled').each(function(item) {
                        item.getElements('option').each(function(opt, i) {
                                var found=0;
                                for (var i=0; i<val.length; i++) {
                                    if (val[i] == opt.value && opt.disabled != true) {
                                        opt.selected = true;
                                        $try(function() {item.onchange()});
                                        found=1;
                                    }
                                }
                                if (!found) {
                                    opt.selected = false;
                                }
                            });
                    });
            });
    else if(ctl.type == "text")
        ctl.addEvent('change', function() {
                var val = this.value;
                $$('input[id^="' + sName + '"]:enabled').each(function(item) {
                        item.set('value', val);
                        $try(function() {item.onchange()});
                    });
            });
    else if(ctl.type == "checkbox")
        ctl.addEvent('click', function() {
                var val = this.checked;
                $$('input[id^="' + sName + '"]:enabled').each(function(item) {
                        item.set('checked', val);
                        $try(function() {item.onclick.run()});
                    });
            });
    else
        alert("unknown type: " + ctl.type);
}

var HeaderControlBar = new Class({
	initialize: function(elms, opts){
            this.elms = elms;
            this.id = typeOf(opts) == "object" && opts.id ? opts.id : "HeaderControlBar";
            this.where = typeOf(opts) == "object" && opts.where ? opts.where : "top";
            this.element = new Element("tr", {"class": "config_odd", "id": this.id});
        },
        toElement: function() {
            return this.element;
        },
	extend: function(elms) {
            this.elms.extend(elms);
        },
	construct: function(location, rowClass) {
            var tr = this.element;
            if(rowClass)
                tr.set("class", rowClass);
            this.elms.each(function(elm, ix) {
                    var td = new Element("td");
                    if(typeOf(elm) == "string") {
                        td.appendText(elm);
                    } else if(typeOf(elm) == "null") {
                        td.appendText(" ");
                    } else if(typeOf(elm) == "object") {
                        if(elm.className)
                            td.addClass(elm.className);
                        if($defined(elm.text)) {
                            td.appendText(elm.text);
                        } else if($defined(elm.name)) {
                            var sources = $$('*[id^="' + elm.name + '"]');
                            if(sources && sources.length) {
                                var src = sources[0];
                                var sel = src.clone();
                                sel.set("disabled", 0);
                                if(sel.type == "select-one") {
                                    if(elm.options) {
                                        var oT = elm.options[0];
                                        var oV = elm.options[1];
                                        sel.empty();
                                        oT.each(function(elm, ix) {
                                                var o = new Element("option", {'value': oV[ix], 'text': oT[ix]});
                                                o.inject(sel);
                                            });
                                    }
                                    var dopt = new Element("option", {'value': "_none_", 'text': "<>"});
                                    dopt.inject(sel, 'top');
                                    Array.from(sel.options).each(function(opt, ix) {
                                            var val = (ix == 0 ? true : false);
                                            opt.setAttribute("defaultSelected", val);
                                            opt.defaultSelected = opt.selected = val;
                                        });
                                } if(sel.type == "text") {
                                    sel.defaultValue = sel.value; // Needed for MSIE 8
                                }
                                sel.set("name", "__ctl__" + ix);
                                sel.set("id", "__ctl__" + ix);
                                HeaderControlSetup(sel, elm.name, elm.onchange);
                                sel.inject(td);
                                td.className = src.getParent().className;
                            } else {
                                td.appendText(" "); // Empty
                            }
                        } else {
                            td.appendText("illegal object: " + JSON.encode(elm));
                        }
                    } else {
                        td.appendText("illegal type: " + typeOf(elm));
                    }
                    td.inject(tr);
                });
            var oldBar = $(this.id);
            if(oldBar)
                oldBar.destroy();
            tr.inject(location, this.where);
        }
    });
