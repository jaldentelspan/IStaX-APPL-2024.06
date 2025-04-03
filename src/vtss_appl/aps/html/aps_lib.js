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

var oTSfTrigger     = Array("Link","MEP");
var oVSfTrigger     = Array("link","mep");

var oTMode          = Array("1:1", "1+1 Uni", "1+1 Bi");
var oVMode          = Array("oneForOne", "onePlusOneUniDir", "onePlusOneBiDir");


var operStateJson   = Array("disabled",
                            "active",
                            "internalError",
                            "wMEPNotFound",
                            "pMEPNotFound",
                            "wMEPAdminDisabled",
                            "pMEPAdminDisabled",
                            "wMEPNotDownMEP",
                            "pMEPNotDownMEP",
                            "wMEPNotPortMEP",
                            "pMEPNotPortMEP",
                            "wAndPMEPSameInterface",
                            "twoInstancedOnSameWPort");

var operStateText   = Array("Administratively disabled",
                            "Active",
                            "Internal Error",
                            "Working MEP not Found",
                            "Protecting MEP not Found",
                            "Working MEP is not administrative active",
                            "Protecting MEP is not administrative active",
                            "Working MEP is not a Down MEP",
                            "Protecting MEP is not a Down MEP",
                            "Working MEP is not a Port MEP",
                            "Protecting MEP is not a Port MEP",
                            "Working and Protecting MEP use the same interface",
                            "Another instance use the same Working port");

var protStateJson   = Array("noRequestWorking",
                            "noRequestProtect",
                            "lockout",
                            "forcedSwitch",
                            "signalFailWorking",
                            "signalFailProtect",
                            "manualSwitchtoProtect",
                            "manualSwitchtoWorking",
                            "waitToRestore",
                            "doNotRevert",
                            "exerciseWorking",
                            "exerciseProtect",
                            "reverseRequestWorking",
                            "reverseRequestProtect",
                            "signalDegradeWorking",
                            "signalDegradeProtect");

var protStateText   = Array("No request Working",
                            "No request Protecting",
                            "Lockout",
                            "Forced Switch",
                            "Signal fail Working",
                            "Signal fail Protecting",
                            "Manual switch to Protecting",
                            "Manual switch to Working",
                            "Wait to restore",
                            "Do not revert",
                            "Exercise Working",
                            "Exercise Protecting",
                            "Reverse request Working",
                            "Reverse request Protecting",
                            "Signal degrade Working",
                            "Signal degrade Protecting");

var apsRequestJson  = Array("nr",
                            "dnr",
                            "rr",
                            "exer",
                            "wtr",
                            "ms",
                            "sd",
                            "sfW",
                            "fs",
                            "sfP",
                            "lo");

var apsRequestText  = Array("No Request",
                            "Do Not Revert",
                            "Reverse Request",
                            "Exercise",
                            "Wait-To-Restore",
                            "Manual Switch",
                            "Signal Degrade",
                            "Signal Fail for Working",
                            "Forced Switch",
                            "Signal Fail for Protect",
                            "Lockout");

var apsWarningJson  = Array("none",
                            "wMEPNotFound",
                            "pMEPNotFound",
                            "wMEPAdminDisabled",
                            "pMEPAdminDisabled",
                            "wMEPNotDownMEP",
                            "pMEPNotDownMEP",
                            "wMEPDiffersFromIfindex",
                            "pMEPDiffersFromIfindex");

var apsWarningText  = Array("No warnings. If operational state is Active, everything is fine.",
                            "Working MEP is not found. Using link-state for SF instead.",
                            "Protect MEP is not found. Using link-state for SF instead.",
                            "Working MEP is administratively disabled. Using link-state for SF instead.",
                            "Protect MEP is administratively disabled. Using link-state for SF instead.",
                            "Working MEP is not a Down-MEP. Using link-state for SF instead.",
                            "Protect MEP is not a Down-MEP. Using link-state for SF instead.",
                            "Working MEP's residence port is not that of the working port. Using link-state for SF instead.",
                            "Protect MEP's residence port is not that of the protect port. Using link-state for SF instead.");

function operState2Text(json_val)
{
    return operStateText[operStateJson.indexOf(json_val)];
}

function protState2Text(json_val)
{
    return protStateText[protStateJson.indexOf(json_val)];
}

function apsRequest2Text(json_val)
{
    return apsRequestText[apsRequestJson.indexOf(json_val)];
}

function apsWarning2Text(json_val)
{
    return apsWarningText[apsWarningJson.indexOf(json_val)];
}