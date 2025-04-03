/*

 Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "snmp_serializer.hxx"

const vtss_enum_descriptor_t snmp_version_txt[] {
    {VTSS_APPL_SNMP_VERSION_1,       "snmpV1"   },
    {VTSS_APPL_SNMP_VERSION_2C,      "snmpV2c"  },
    {VTSS_APPL_SNMP_VERSION_3,       "snmpV3"   },
    {0, 0}
};

const vtss_enum_descriptor_t snmp_security_level_txt[] {
    {VTSS_APPL_SNMP_SECURITY_LEVEL_NOAUTH,       "snmpNoAuthNoPriv"  }, 
    {VTSS_APPL_SNMP_SECURITY_LEVEL_AUTHNOPRIV,   "snmpAuthNoPriv"       },  
    {VTSS_APPL_SNMP_SECURITY_LEVEL_AUTHPRIV,     "snmpAuthPriv"      }, 
    {0, 0}
};

const vtss_enum_descriptor_t snmp_auth_protocol_txt[] {
    {VTSS_APPL_SNMP_AUTH_PROTOCOL_NONE,      "snmpNoAuthProtocol"  },
    {VTSS_APPL_SNMP_AUTH_PROTOCOL_MD5,       "snmpMD5AuthProtocol" },
    {VTSS_APPL_SNMP_AUTH_PROTOCOL_SHA,       "snmpSHAAuthProtocol" },
    {0, 0}
};

const vtss_enum_descriptor_t snmp_priv_protocol_txt[] {
    {VTSS_APPL_SNMP_PRIV_PROTOCOL_NONE,      "snmpNoPrivProtocol"  },
    {VTSS_APPL_SNMP_PRIV_PROTOCOL_DES,       "snmpDESPrivProtocol" },
    {VTSS_APPL_SNMP_PRIV_PROTOCOL_AES,       "snmpAESPrivProtocol" },
    {0, 0}
};

const vtss_enum_descriptor_t vtss_appl_snmp_security_model_txt[] {
    {VTSS_APPL_SNMP_SECURITY_MODEL_ANY,        "any"                 }, 
    {VTSS_APPL_SNMP_SECURITY_MODEL_SNMPV1,     "v1"                  }, 
    {VTSS_APPL_SNMP_SECURITY_MODEL_SNMPV2C,    "v2c"                 },
    {VTSS_APPL_SNMP_SECURITY_MODEL_USM,        "usm"                 },
    {0, 0}
};

const vtss_enum_descriptor_t vtss_appl_snmp_view_type_txt[] {
    {VTSS_APPL_SNMP_VIEW_TYPE_INCLUDED,        "included"            }, 
    {VTSS_APPL_SNMP_VIEW_TYPE_EXCLUDED,        "excluded"            }, 
    {0, 0}
};

const vtss_enum_descriptor_t vtss_appl_trap_notify_type_txt[] {
    {VTSS_APPL_TRAP_NOTIFY_TRAP,               "trap"                }, 
    {VTSS_APPL_TRAP_NOTIFY_INFORM,             "inform"              }, 
    {0, 0}
};
