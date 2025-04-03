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



#include "icli_api.h"
#include "icli_porting_util.h"

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/pdu_api.h>
#include <net-snmp/library/snmp_secmod.h>
#include <net-snmp/library/oid.h>
#include <net-snmp/library/snmpusm.h>


class NetSnmpUsm {
public:
    // constructor
    NetSnmpUsm(u32 SessionIdInit) {
	session_id = SessionIdInit;
    }

    // Print all users
    void Print(void) {
	usmUser UserData;
	usmUser *UserData_p = &UserData;
	UserData_p = usm_get_userList();

	while (UserData_p != NULL) {
	    ICLI_PRINTF("Name:%s\n", UserData_p->name);
            if (UserData_p->authKey) {
                ICLI_PRINTF("AuthKey:%s\n", UserData_p->authKey);
            }
	    ICLI_PRINTF("Name:%s\n", UserData_p->name);
	    UserData_p = UserData_p->next;
	}
    }

private:
    u32 session_id;
};
