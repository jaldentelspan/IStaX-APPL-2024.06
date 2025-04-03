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

// **********************************  MENU.JS  ********************************
// *
// * Author: Lars Povlsen
// *
// * --------------------------------------------------------------------------
// *
// * Description:  Client-side JavaScript functions.
// *
// * To include in HTML file use:
// *
// * <script language="javascript" type="text/javascript" src="lib/menu.js"></script>
// *
// * --------------------------------------------------------------------------

var curReq;
var curSid;

function StopUpdate()
{
    curReq && curReq.abort();
}

function emptyStackList()
{
    // Remove all in list to provide visual feedback of update
    var sel = $('stackselect');
    if(sel) {
        while(sel.options.length) {
            sel.removeChild(sel.firstChild);
        }
    }
}

function stackSelect(sel)
{
    var opt = sel.options[sel.selectedIndex];
    curSid = opt.value;
    if(opt.get('stack_a') == 0) {
        $('selrow').setStyle("display", "none");
    }
    if(curSid) {
        var main = parent.main;
        // SID-aware page in focus?
        if(typeof(main.SpomSidSelectorUpdate) == "function" &&
           curSid != main.current_sid) {
            main.SpomSidSelectorUpdate(curSid, opt.get('ports'));
        }
    }
}

function stackSelectedSid()
{
    var sel = $('stackselect');
    var opt = sel.options[sel.selectedIndex];
    return opt;
}

function processUpdate(req, ref)
{
    var sel = $('stackselect');
    emptyStackList();
    if(req.responseText) {
        var selected = 0;
        var members = req.responseText.split(",");
        for(var i = 0; i < members.length; i++) {
            if (members[i]) {
                var val = members[i].split(":");
                var sid = val[0], flags = val[1];
                var opt = new Element("option", {'value': sid, 'text': "Switch " + sid});
                opt.set('ports', parseInt(val[2]));
                opt.set('stack_a', parseInt(val[3]));
                opt.set('stack_b', parseInt(val[4]));
                if(flags && flags == "M") {
                    opt.setAttribute("selected", true);
                    opt.setAttribute("defaultSelected", true);
                    opt.set("class", "primary");
                    selected = i;
                }
                sel.appendChild(opt);
            }
        }
        sel.selectedIndex = selected;
        $('selrow').setStyle("display", ""); // Stack selector enabled & visible
    }
    stackSelect(sel);
}

function requestUpdate() {
    curReq = loadXMLDoc("/stat/sid", processUpdate, null);
}

var class_map = {'false': 'open',
                 'true': 'closed'};

function MooComplete(li)
{
    var paren = li.getParent('ul').getParent('li');
    paren && paren.slider.slideIn();

    // Check if we need to scroll up
    if(!li.slider.open) {
        var winSize = $(window).getSize().y;
        var bPos = winSize - li.getPosition().y;
        if($(window).getScrollSize().y > winSize &&
           bPos < 50) {
            var scroller = new Fx.Scroll(window).toElement(li);
        }
    }
}

function MenuLoad()
{
    // Curtains down initially
    $('menu').setStyle("display", "none");

    var actuators = $('menu').getElements('a.actuator');
    // Initial state
    actuators.getParent().setProperty("class", "closed");
    actuators.each(function(el){
            var li = el.getParent(); // A -> LI
            // Make Fx on contained UL (LI -> UL)
            li.slider = new Fx.Slide(li.getLast(), {
                    duration: 200,
                    onComplete: function() { MooComplete(li);
                    }});
            el.addEvent('click', function() {
                    li.setProperty("class", li.slider.open ? "closed" : "open");
                    li.slider.toggle();
                });
            li.slider.hide();
        });

    if(!configStackable) {
        curSid = 0;             // Just pretend sid = 0
    } else {
        requestUpdate();        // Stacking, get current mode & state
    }

    if(typeof(NavBarUpdate) == "function") {
        NavBarUpdate();
    }

    $('menu').setStyle("display", ""); // All menu visible now

    var startElem = $('main.htm').getParent('ul').getParent('li');
    while(startElem) {
        startElem.slider.show();
        startElem.setProperty("class", "open");
        startElem = startElem.getParent('ul').getParent('li');
    }
}

function DoStackRefresh()
{
    emptyStackList();
    requestUpdate();
}

function DelayedStackRefresh()
{
    setTimeout('DoStackRefresh()', 1000);
}

window.addEvent('domready', function() { MenuLoad(); });
