/*
 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

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

// Parts of this file is derived from code with a copyright given in:
// ../base/ucd_snmp/COPYING. The original source did not include a copyright
// header/license.

#include "main.h"
#include "vtss_snmp.h"
#include "vtss/appl/snmp.h"
#include "vtss_snmp_linux.h"
#include "misc_api.h"
#include "subject.hxx"
using namespace std;

/* This function is used for recrieve the key of GET or GETNEXT, but it only
 *  used for the single-integer-key's table.
*/
/*******************************************************************-o-******
 * generic_header
 *
 * Parameters:
 *        *vp      (I)     Pointer to variable entry that points here.
 *        *name    (I/O)   Input name requested, output name found.
 *        *length  (I/O)   Length of input and output oid's.
 *        exact    (I)     TRUE if an exact match was requested.
 *        *var_len (O)     Length of variable or 0 if function returned.
 *      (**write_method)   Hook to name a write method (UNUSED).
 *
 * Returns:
 * MATCH_SUCCEEDED If vp->name matches name (accounting for exact bit).
 *    MATCH_FAILED    Otherwise,
 *
 *
 * Check whether variable (vp) matches name.
*/
int
header_generic(struct variable *vp,
               oid *name,
               size_t *length,
               int exact, size_t *var_len, WriteMethod **write_method)
{
    oid             newname[MAX_OID_LEN];
    int             result;

    DEBUGMSGTL(("util_funcs", "header_generic: "));
    DEBUGMSGOID(("util_funcs", name, *length));
    DEBUGMSG(("util_funcs", " exact=%d\n", exact));

    memcpy((char *) newname, (char *) vp->name,
           (int) vp->namelen * sizeof(oid));
    newname[vp->namelen] = 0;
    result = snmp_oid_compare(name, *length, newname, vp->namelen + 1);
    DEBUGMSGTL(("util_funcs", "  result: %d\n", result));
    if ((exact && (result != 0)) || (!exact && (result >= 0))) {
        return (MATCH_FAILED);
    }
    memcpy((char *) name, (char *) newname,
           ((int) vp->namelen + 1) * sizeof(oid));
    *length = vp->namelen + 1;

    *write_method = (WriteMethod *)0;
    *var_len = sizeof(uint32_t);    /* default to 'long' results */
    return (MATCH_SUCCEEDED);
}

/*******************************************************************-o-******
 * header_simple_table
 *
 * Parameters:
 *    *vp        Variable data.
 *    *name      Fully instantiated OID name.
 *    *length    Length of name.
 *     exact     TRUE if an exact match is desired.
 *    *var_len   Hook for size of returned data type.
 *  (**write_method) Hook for write method (UNUSED).
 *     max
 *
 * Returns:
 *  0   If name matches vp->name (accounting for 'exact') and is
 *          not greater in length than 'max'.
 *  1   Otherwise.
 *
 *
 * Compare 'name' to vp->name for the best match or an exact match (if
 *  requested).  Also check that 'name' is not longer than 'max' if
 *  max is greater-than/equal 0.
 * Store a successful match in 'name', and increment the OID instance if
 *  the match was not exact.
 *
 * 'name' and 'length' are undefined upon failure.
 *
 */
int
header_simple_table(struct variable *vp, oid *name, size_t *length,
                    int exact, size_t *var_len,
                    WriteMethod **write_method, int max)
{
    int             i, rtest;   /* Set to:      -1      If name < vp->name,
                                 *              1       If name > vp->name,
                                 *              0       Otherwise.
                                 */
    oid             newname[MAX_OID_LEN];

    for (i = 0, rtest = 0;
         i < (int) vp->namelen && i < (int) (*length) && !rtest; i++) {
        if (name[i] != vp->name[i]) {
            if (name[i] < vp->name[i]) {
                rtest = -1;
            } else {
                rtest = 1;
            }
        }
    }
    if (rtest > 0 ||
        (exact == 1
         && (rtest || (int) *length != (int) (vp->namelen + 1)))) {
        if (var_len) {
            *var_len = 0;
        }
        return MATCH_FAILED;
    }

    memset(newname, 0, sizeof(newname));

    if (((int) *length) <= (int) vp->namelen || rtest == -1) {
        memmove(newname, vp->name, (int) vp->namelen * sizeof(oid));
        newname[vp->namelen] = 1;
        *length = vp->namelen + 1;
    } else if (((int) *length) > (int) vp->namelen + 1) {       /* exact case checked earlier */
        *length = vp->namelen + 1;
        memmove(newname, name, (*length) * sizeof(oid));
        if (name[*length - 1] < MAX_SUBID) {
            newname[*length - 1] = name[*length - 1] + 1;
        } else {
            /*
             * Careful not to overflow...
             */
            newname[*length - 1] = name[*length - 1];
        }
    } else {
        *length = vp->namelen + 1;
        memmove(newname, name, (*length) * sizeof(oid));
        if (!exact) {
            if (name[*length - 1] < MAX_SUBID) {
                newname[*length - 1] = name[*length - 1] + 1;
            } else {
                /*
                 * Careful not to overflow...
                 */
                newname[*length - 1] = name[*length - 1];
            }
        } else {
            newname[*length - 1] = name[*length - 1];
        }
    }
    if ((max >= 0 && ((int)newname[*length - 1] > max)) ||
        ( 0 == newname[*length - 1] )) {
        if (var_len) {
            *var_len = 0;
        }
        return MATCH_FAILED;
    }

    memmove(name, newname, (*length) * sizeof(oid));
    if (write_method) {
        *write_method = (WriteMethod *)0;
    }
    if (var_len) {
        *var_len = sizeof(uint32_t);    /* default */
    }
    return (MATCH_SUCCEEDED);
}

void SnmpdSetEngineId(const char *engine_id, int len)
{
    char engineid_txt[SNMPV3_MAX_ENGINE_ID_LEN * 2 + 3];
    char *p_engineid_txt = engineid_txt;
    uchar old_engineid[SNMPV3_MAX_ENGINE_ID_LEN];
    int   old_engineid_len;

    char engine_id_type[] = "3";
    p_engineid_txt += sprintf(p_engineid_txt, "0x");
    misc_engineid2str(p_engineid_txt, (uchar *)engine_id, len);
    old_engineid_len = snmpv3_get_engineID((uchar *)old_engineid, SNMPV3_MAX_ENGINE_ID_LEN);
    free_enginetime((uchar *)old_engineid, old_engineid_len);
    engineIDType_conf(NULL, engine_id_type);
    setup_engineID(NULL, engineid_txt);
    set_enginetime((uchar *)engine_id, len,
                   snmpv3_local_snmpEngineBoots(),
                   snmpv3_local_snmpEngineTime(), TRUE);
}

