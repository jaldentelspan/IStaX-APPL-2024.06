// * -*- Mode: java; tab-width: 8; -*-
/*

 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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


var oTOperState     = Array("Disabled","Active","InternalError");
var oVOperState     = Array("disabled","active","internalError");

var oTPortState     = Array("Blocked","Forwarding");
var oVPortState     = Array("blocked","forwarding");

var oTRecoveryProfile   = Array("10 ms", "30 ms", "200 ms", "500 ms");
var oVRecoveryProfile   = Array("ms10", "ms30", "ms200", "ms500");

var oTInRecoveryProfile = Array("200 ms", "500 ms" );
var oVInRecoveryProfile = Array("ms200", "ms500");

var oTRole          = Array("Client", "Manager", "Auto Manager" );
var oVRole          = Array("mrc", "mrm", "mra");

var oTInRole        = Array("None", "Client", "Manager" );
var oVInRole        = Array("none", "mic", "mim" );

var oTInMode        = Array("Link Check", "Ring Check");
var oVInMode        = Array("linkCheck", "ringCheck");

var oTPortType      = Array("Ring port 1", "Ring port 1", "Interconnection" );
var oVPortType      = Array("port1", "port2", "interconnection" );

var oTSfTrigger     = Array("Link","MEP");
var oVSfTrigger     = Array("link","mep");

var oTRingPort      = Array("RingPort1","RingPort2");
var oVRingPort      = Array("ringPort2","ringPort2");

var oTState         = Array("Disabled","Open","Closed","Undefined");
var oVState         = Array("disabled","open","closed","undefined");

var oTOUIType       = Array("Default","Siemens","Custom");
var oVOUIType       = Array("default","siemens","custom");

var warningMessages =   Array("Port1 is not member of the ring's control VLAN, which is configured for tagged operation",
                              "Port2 is not member of the ring's control VLAN, which is configured for tagged operation",
                              "Port1 is not member of the interconnection control VLAN, which is configured for tagged operation",
                              "Port2 is not member of the interconnection control VLAN, which is configured for tagged operation",
                              "Interconnection port is not member of the interconnection control VLAN, which is configured for tagged operation",
                              "Port1 is not member of its own PVID (ring's control VLAN is configured for untagged operation)",
                              "Port2 is not member of its own PVID (ring's control VLAN is configured for untagged operation)",
                              "Port1 and Port2's PVID differ (ring VLAN is configured for untagged operation)",
                              "Port1 is not member of the interconnection port's PVID (interconnection's control VLAN is configured for untagged operation)",
                              "Port2 is not member of the interconnection port's PVID (interconnection's control VLAN is configured for untagged operation)",
                              "Interconnection port is not member of its own PVID (interconnection's control VLAN is configured for untagged operation)",
                              "Port1 untags ring's control VLAN, which is configured for tagged operation",
                              "Port2 untags ring's control VLAN, which is configured for tagged operation",
                              "Port1 untags interconnection's control VLAN, which is configured for tagged operation",
                              "Port2 untags interconnection's control VLAN, which is configured for tagged operation",
                              "Interconnection port untags interconnection's control VLAN, which is configured for tagged operation",
                              "Port1 tags its own PVID (ring's control VLAN is configured for untagged operation)",
                              "Port2 tags its own PVID (ring's control VLAN is configured for untagged operation)",
                              "Port1 tags the interconnection port's PVID (interconnection's control VLAN is configured for untagged operation)",
                              "Port2 tags the interconnection port's PVID (interconnection's control VLAN is configured for untagged operation)",
                              "Interconnection port tags itw own PVID (interconnection's control VLAN is configured for untagged operation)",
                              "Port1 MEP is not found. Using link-state for signal-fail instead",
                              "Port2 MEP is not found. Using link-state for signal-fail instead",
                              "Interconnection MEP is not found. Using link-state for signal-fail instead",
                              "Port1 MEP is administratively disabled. Using link-state for signal-fail instead",
                              "Port2 MEP is administratively disabled. Using link-state for signal-fail instead",
                              "Interconnection MEP is administratively disabled. Using link-state for signal-fail instead",
                              "Port1 MEP is not a Down-MEP. Using link-state for signal-fail instead",
                              "Port2 MEP is not a Down-MEP. Using link-state for signal-fail instead",
                              "Interconnection MEP is not a Down-MEP. Using link-state for signal-fail instead",
                              "Port1 MEP's residence port is not that of Port1. Using link-state for signal-fail instead",
                              "Port2 MEP's residence port is not that of Port2. Using link-state for signal-fail instead",
                              "Interconnection MEP's residence port is not that of the interconnection port. Using link-state for signal-fail instead",
                              "Port1 has spanning tree enabled",
                              "Port2 has spanning tree enabled",
                              "Interconnection port has spanning tree enabled",
                              "Multiple MRMs detected on the ring. This is normal if MRAs are negotiating. Cleared after 10 seconds w/o detection",
                              "Multiple MIMs with same ID detected on the interconnection ring. Cleared after 10 seconds w/o detection",
                              "An internal error has occurred. A code update is required. Please check console for trace output.");


function printOperWarnings(warnings)
{
    var warning_str = ""; 
    var newline = "";
    var matchPattern = 0;

    // Javascript only supports bitwise operations on 32 bit, so we break the 64 bit input into a low and high part
    var warningHigh = Math.trunc(warnings/0x100000000);
    var warningLow  = warnings - warningHigh * 0x100000000;

    for (i=0; i<32; i++ ) {
        matchPattern = 1 << i;
        if (warningLow & matchPattern) {
            warning_str = warning_str + newline + warningMessages[i];
            newline = "\n";
        }
    }

    for (i=0; i<warningMessages.length - 32; i++ ) {
        matchPattern = 1 << i;
        if (warningHigh & matchPattern) {
            warning_str = warning_str + newline + warningMessages[i+32];
            newline = "\n";
        }
    }

    return warning_str;
}
