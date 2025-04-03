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

var oTRingType      = Array("Major","Sub","InterSub");
var oVRingType      = Array("major","sub","interconnectedSub");

var oTCommand       = Array("No request","Force switch to Port0","Force switch to Port1","Manual switch to Port0","Manual switch to Port1","Clear");
var oVCommand       = Array("noRequest","forceSwitchToPort0","forceSwitchToPort1","manualSwitchToPort0","manualSwitchToPort1","clear");

var oTVersion       = Array("v1","v2");
var oVVersion       = Array("v1","v2");

var oTRplMode       = Array("None","Owner","Neighbor");
var oVRplMode       = Array("none","owner","neighbor");

var oTSfTrigger     = Array("Link","MEP");
var oVSfTrigger     = Array("link","mep");

var oTRequest       = Array("No Request","Manual Switch","Signal Failed","Force Switch","Event");
var oVRequest       = Array("noRequest","manualSwitch","signalFailed","forceSwitch","event");

var oTRingPort      = Array("RingPort0","RingPort1");
var oVRingPort      = Array("ringPort0","ringPort1");

var oTOperState     = Array("Disabled","Active","InternalError");
var oVOperState     = Array("disabled","active","internalError");

var oTNodeState     = Array("Init","Idle","Protection","Manual Switch","Force Switch","Pending");
var oVNodeState     = Array("init","idle","protection","ms","fs","pending");

var oTOperWarning   = Array("none",
                            "Ring ports are not members of control VLAN",
                            "Port0 MEP not found",
                            "Port1 MEP not found",
                            "Port0 MEP admin disabled",
                            "Port1 MEP admin disabled",
                            "Port0 MEP not DownMEP",
                            "Port1 MEP not DownMEP",
                            "Port0 and MEP Ifindex differ",
                            "Port1 and MEP Ifindex differ",
                            "Port MEP shadows port0 MIP",
                            "Port MEP shadows port1 MIP",
                            "MEP shadows Port0 MIP",
                            "MEP shadows Port1 MIP",
                            "Connected ring does not exist",
                            "Connected ring is an interconnected subring",
                            "Connected ring is not operative",
                            "Conencted ring interface conflict",
                            "Connected ring does not protect control VLAN");

var oVOperWarning   = Array("none",
                            "ringPortsNotMembersOfControlVlan",
                            "port0MepNotFound",
                            "port1MepNotFound",
                            "port0MepAdminDisabled",
                            "port1MepAdminDisabled",
                            "port0MepNotDownMep",
                            "port1MepNotDownMep",
                            "port0AndMepIfindexDiffer",
                            "port1AndMepIfindexDiffer",
                            "portMepShadowsPort0Mip",
                            "portMepShadowsPort1Mip",
                            "mepShadowsPort0Mip",
                            "mepShadowsPort1Mip",
                            "connectedRingDoesntExist",
                            "connectedRingIsAnInterconnectedSubRing",
                            "connectedRingIsNotOperative",
                            "connectedRingInterfaceConflict",
                            "connectedRingDoesntProtectControlVlan");
