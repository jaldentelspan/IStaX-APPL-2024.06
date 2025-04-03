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
// NB: This file is *only* used in "mockup" environment!
// But if new variables are introduced in the "live" version,
// then provide default settings here as well!
var configArchJaguar_1 = 0;
var configArchLuton26 = 0;
var configArchServal = 0;
var configArchJaguar_2 = 1;
var configBuildSMB = 1;
var configPortMin = 1;
var configNormalPortMax = 24;
var configRgmiiWifi = 0;
var configStackable = 1;
var configVlanIdMin = 1;
var configVlanIdMax = 4095;
var configVlanEntryCnt = 64;
var configPvlanIdMin = 1;
var configPvlanIdMax = 26;
var configSidMin = 1;
var configSidMax = 16;
var configAclPktRateMax = 1024;
var configAclBitRateMax = 1000000;
var configAclBitRateGranularity = 100;
var configAclRateLimitIdMax = 15;
var configAceMax = 128;
var configSwitchName = "MockUp";
var configDeviceName = configArchJaguar_1 ? "Jaguar1" : configArchLuton26 ? "Luton26" : configArchServal ? "Serval" : configArchJaguar_2 ? "Jaguar2" : "<unknown>";
var configSwitchDescription = "24x1G + 2x5G Stackable Ethernet Switch";
var configPsecLimitLimitMax = 100;
var configPolicyMax = 8;
var configPolicyBitmaskMax = 255;
var configAceMax = 128;
var configPortType = 0;
var configAuthServerCnt = 5;
var configAuthHostLen = 255;
var configAuthKeyLen = 63;
var configAuthRadiusAuthPortDef = 1812;
var configAuthRadiusAcctPortDef = 1813;
var configAuthTacacsPortDef = 49;
var configAuthTimeoutDef = 5;
var configAuthTimeoutMin = 1;
var configAuthTimeoutMax = 1000;
var configAuthRetransmitDef = 3;
var configAuthRetransmitMin = 1;
var configAuthRetransmitMax = 1000;
var configAuthDeadtimeDef = 0;
var configAuthDeadtimeMin = 0;
var configAuthDeadtimeMax = 1440;
var configHasIngressFiltering = 1;

var configQosClassCnt = 8;
var configQosClassMin = 0;
var configQosClassMax = 7;
var configQosDplCnt = 2;
var configQosDplMin = 0;
var configQosDplMax = 1;
var configQosPortPolicerBitRateMin    =     100;
var configQosPortPolicerBitRateMax    = 1000000;
var configQosPortPolicerFrameRateMin  =     100;
var configQosPortPolicerFrameRateMax  = 1000000;
var configQosQueuePolicerBitRateMin   =     100;
var configQosQueuePolicerBitRateMax   = 1000000;
var configQosQueuePolicerFrameRateMin =       0;
var configQosQueuePolicerFrameRateMax =       0;
var configQosPortShaperBitRateMin     =     100;
var configQosPortShaperBitRateMax     = 1000000;
var configQosPortShaperFrameRateMin   =       0;
var configQosPortShaperFrameRateMax   =       0;
var configQosQueueShaperBitRateMin    =     100;
var configQosQueueShaperBitRateMax    = 1000000;
var configQosQueueShaperFrameRateMin  =       0;
var configQosQueueShaperFrameRateMax  =       0;
var configQosGlobalStormBitRateMin    =       0;
var configQosGlobalStormBitRateMax    =       0;
var configQosGlobalStormFrameRateMin  =       1;
var configQosGlobalStormFrameRateMax  = 1024000;
var configQosPortStormBitRateMin      =     100;
var configQosPortStormBitRateMax      = 1000000;
var configQosPortStormFrameRateMin    =     100;
var configQosPortStormFrameRateMax    = 1000000;

var configQosQueueCount = 8;
var configQosDscpNames = [
    '0  (BE)', '1', '2',        '3', '4',        '5', '6',        '7',
    '8  (CS1)','9', '10 (AF11)','11','12 (AF12)','13','14 (AF13)','15',
    '16 (CS2)','17','18 (AF21)','19','20 (AF22)','21','22 (AF23)','23',
    '24 (CS3)','25','26 (AF31)','27','28 (AF32)','29','30 (AF33)','31',
    '32 (CS4)','33','34 (AF41)','35','36 (AF42)','37','38 (AF43)','39',
    '40 (CS5)','41','42',       '43','44',       '45','46 (EF)',  '47',
    '48 (CS6)','49','50',       '51','52',       '53','54',       '55',
    '56 (CS7)','57','58',       '59','60',       '61','62',       '63'
];
var configQosHasDscpDplClassification = 1;
var configQosHasDscpDplRemarking = 1;
var configQosQceMax = 256;
var configQosHasQceAddressMode = 1;

var configHasStpEnhancements = 1;
var configAccessMgmtEntryCnt = 16;
var configVoiceVlanOuiEntryCnt = 16;
var configIgmpsFilteringMax = 5;
var configIgmpsVLANsMax = 64;
var configMldsnpFilteringMax = 5;
var configMldsnpVLANsMax = 64;
var configAccessMgmtMax = 16;
var configHasCDP = "1";
var configIPDNSSupport = 1;
var configIPv6Support = 1;

var configPortFrameSizeMin = 1518;
var configPortFrameSizeMax = 9600;
var if_switch_interval = 1000;
var if_llag_start = 501;
var if_llag_cnt = 26;
var if_glag_start = -1;
var if_glag_cnt = 0;
var configLldpmedPoliciesMin = 1;
var configLldpmedPoliciesMax = 5;

var configTrapSourcesMax = 32;
var configTrapSourceFilterIdMax = 127;

var vlan_svl_fid_cnt = 63;

function configPortName(portno, long)
{
    var portname = String(portno);
    if (long) {
        portname = "Port " + portname;
    }
    return portname;
}

function configIndexName(index, long)
{
    var indexname = String(index);
    if (long) {
        indexname = "ID " + indexname;
    }
    return indexname;
}

function isNtpSupported()
{
    return true;
}
