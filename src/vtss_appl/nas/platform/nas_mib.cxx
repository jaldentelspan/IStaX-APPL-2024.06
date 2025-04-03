/*
 Copyright (c) 2006-2018 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "nas_serializer.hxx"
#include "vtss/appl/nas.h"

using namespace vtss;
using namespace vtss::expose::snmp;


VTSS_MIB_MODULE("nasMib", "NAS", nas_mib_init, VTSS_MODULE_ID_DOT1X, root, h) {
    h.add_history_element("201407010000Z", "Initial version");
    h.add_history_element("201712120000Z", "1) Changed nasPortStatus::cnt to nasPortStatus::multiMode. 2) Added authCnt and unauthCnt to NasStatusEntry. 3) Minor editorial changes");
    h.description("This is the Vitesse NAS private MIB.");
}

#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))
NS(nas, root, 1, "nasMibObjects");;
NS(nas_config,  nas, 2, "nasConfig");;
NS(nas_status,  nas, 3, "nasStatus");;
NS(nas_control, nas, 4, "nasControl");;
NS(nas_stats,   nas, 5, "nasStatistics");;



namespace vtss {
namespace appl {
namespace nas {
namespace interfaces {
    static TableReadOnly2<NasStatusTable>               nas_stat(&nas_status, vtss::expose::snmp::OidElement(1, "nasStatusTable"));
    static TableReadOnly2<NasLastSupplicantInfo>        nas_info(&nas_status, vtss::expose::snmp::OidElement(2, "nasStatusLastSupplicant"));

    static TableReadOnly2<NasPortStatsDot1xEapolTable>    nas_eapol_stats(&nas_stats, vtss::expose::snmp::OidElement(1, "nasStatisticsEapol"));
    static TableReadOnly2<NasPortStatsRadiusServerTable>  nas_radius_stats(&nas_stats, vtss::expose::snmp::OidElement(2, "nasStatisticsRadius"));


    static StructRW2<NasGlblConfigGlobals> nas_glb_conf   (&nas_config, vtss::expose::snmp::OidElement(1, "nasConfigGlobal"));
    static TableReadWrite2<NasPortConfig>  nas_port_tabl  (&nas_config, vtss::expose::snmp::OidElement(2, "nasConfigReAuth"));

    static TableReadWrite2<NasReauthTable> nas_reauth_tabl(&nas_control, vtss::expose::snmp::OidElement(1, "nasControlPort"));
    static TableReadWrite2<NasStatClrLeaf> nas_stat_clr   (&nas_control, vtss::expose::snmp::OidElement(2, "nasControlStatisticsClear"));


}  // namespace interfaces
}  // namespace nas
}  // namespace appl
}  // namespace vtss

