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

#ifndef __CPUPORT_EXPOSE_H__
#define __CPUPORT_EXPOSE_H__

#include <vtss/basics/map.hxx>
#include <vtss/basics/expose/json/table-read-only-notification.hxx>
#include <vtss/basics/memcmp-operator.hxx>
#include <vtss/appl/cpuport.h>
#include <vtss/appl/interface.h>

extern "C" void vtss_appl_cpuport_json_init();
extern "C" void cpuport_mib_init();

using namespace vtss::expose;
using namespace vtss::notifications;

VTSS_BASICS_MEMCMP_OPERATOR(vtss_appl_port_conf_t);
VTSS_BASICS_MEMCMP_OPERATOR(vtss_appl_port_status_t);

namespace vtss
{
namespace appl
{
namespace cpuport
{

typedef TableStatus<expose::ParamKey<vtss_ifindex_t>,
        expose::ParamVal<::vtss_appl_port_conf_t>> CpuportConfiguration;
extern CpuportConfiguration the_cpuport_config;

typedef TableStatus<expose::ParamKey<vtss_ifindex_t>,
        expose::ParamVal<mesa_port_counters_t>> CpuportStatistics;
extern CpuportStatistics the_cpuport_statistics;

}  // namespace cpuport
}  // namespace appl
}  // namespace vtss

#endif /* !defined(__CPUPORT_EXPOSE_H__) */

