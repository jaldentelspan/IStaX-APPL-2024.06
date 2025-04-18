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

// Note: This file originally auto-generated by mib2c using vtss_mib2c_ucd_snmp.conf v3.40
//       It is based on UCD-SNMP APIs, we should not do any change unless the implement
//       is different with standard MIB definition. For example:
//       1. The specific OID is not supported.
//       2. The 'read-write' operation doesn't supported.
//       3. The specific variable range is different from standard definition.


#include <main.h>
#include "vtss_os_wrapper_snmp.h"
#include "ucd_snmp_ieee8021BridgeMib.h"
#include "ieee8021BridgeMib.h"
#include "ucd_snmp_callout.h"   //ucd_snmp_callout_malloc(), ucd_snmp_callout_free()

// #define IEEE8021BRIDGEBASEPORTTABLE_NOT_SUPPORTED       0   /* Excpetion case 1. */
#define IEEE8021BRIDGEBASEPORTTABLE_ONLY_RO_SUPPORTED   1   /* Excpetion case 2. */
// #define IEEE8021BRIDGEBASEPORTTABLE_DIFFERENT_RANGE     1   /* Excpetion case 3. */

/******************************************************************************/
//
// Local data structure declaration
//
/******************************************************************************/
// The data structure for return value, UCD-SNMP engine needs as address point for processing get operation
typedef struct {
    long            long_ret;
    char            string_ret[IEEE8021BRIDGEBASEPORTTABLE_STR_LEN_MAX +
                               1];
    struct counter64 c64_ret;
    u_long          ulong_ret;
} ieee8021BridgeBasePortTable_ret_t;


/******************************************************************************/
//
// Local function declarations
//
/******************************************************************************/
FindVarMethod   ieee8021BridgeBasePortTable_var;
FindVarMethod   ieee8021BridgeBasePortTable_var;
WriteMethod     ieee8021BridgeBasePortIfIndex_write;
WriteMethod     ieee8021BridgeBasePortAdminPointToPoint_write;


/******************************************************************************/
//
// Local variable declarations
//
/******************************************************************************/
/*
 * lint -esym(459, ieee8021BridgeBasePortTable_global_ret)
 */
// The variable is protected by thread
// The UCD-SNMP engine needs as address point for processing get operation
static ieee8021BridgeBasePortTable_ret_t
    ieee8021BridgeBasePortTable_global_ret;

/*
 * ieee8021BridgeBasePortTable_variables_oid:
 *   this is the top level oid that we want to register under.  This
 *   is essentially a prefix, with the suffix appearing in the
 *   variable below.
 */
static oid      ieee8021BridgeBasePortTable_variables_oid[] =
    { 1, 3, 111, 2, 802, 1, 1, 2, 1, 1, 4 };



/*
 * variable ieee8021BridgeBasePortTable_variables:
 *   this variable defines function callbacks and type return information
 *   for the ieee8021BridgeBasePortTable mib section
 */

struct variable2 ieee8021BridgeBasePortTable_variables[] = {
    /*
     * magic number, variable type, ro/rw, callback fn, L, oidsuffix
     */

#define IEEE8021BRIDGEBASEPORTIFINDEX                1
#if IEEE8021BRIDGEBASEPORTTABLE_ONLY_RO_SUPPORTED
    {IEEE8021BRIDGEBASEPORTIFINDEX, ASN_INTEGER, RONLY,
     ieee8021BridgeBasePortTable_var, 2, {1, 3}},
#else
    {IEEE8021BRIDGEBASEPORTIFINDEX, ASN_INTEGER, RWRITE,
     ieee8021BridgeBasePortTable_var, 2, {1, 3}},
#endif /* IEEE8021BRIDGEBASEPORTTABLE_ONLY_RO_SUPPORTED */
#define IEEE8021BRIDGEBASEPORTDELAYEXCEEDEDDISCARDS                2
    {IEEE8021BRIDGEBASEPORTDELAYEXCEEDEDDISCARDS, ASN_COUNTER64, RONLY,
     ieee8021BridgeBasePortTable_var, 2, {1, 4}},
#define IEEE8021BRIDGEBASEPORTMTUEXCEEDEDDISCARDS                3
    {IEEE8021BRIDGEBASEPORTMTUEXCEEDEDDISCARDS, ASN_COUNTER64, RONLY,
     ieee8021BridgeBasePortTable_var, 2, {1, 5}},
#define IEEE8021BRIDGEBASEPORTCAPABILITIES                4
    {IEEE8021BRIDGEBASEPORTCAPABILITIES, ASN_OCTET_STR, RONLY,
     ieee8021BridgeBasePortTable_var, 2, {1, 6}},
#define IEEE8021BRIDGEBASEPORTTYPECAPABILITIES                5
    {IEEE8021BRIDGEBASEPORTTYPECAPABILITIES, ASN_OCTET_STR, RONLY,
     ieee8021BridgeBasePortTable_var, 2, {1, 7}},
#define IEEE8021BRIDGEBASEPORTTYPE                6
    {IEEE8021BRIDGEBASEPORTTYPE, ASN_INTEGER, RONLY,
     ieee8021BridgeBasePortTable_var, 2, {1, 8}},
#define IEEE8021BRIDGEBASEPORTEXTERNAL                7
    {IEEE8021BRIDGEBASEPORTEXTERNAL, ASN_INTEGER, RONLY,
     ieee8021BridgeBasePortTable_var, 2, {1, 9}},
#define IEEE8021BRIDGEBASEPORTADMINPOINTTOPOINT                8
    {IEEE8021BRIDGEBASEPORTADMINPOINTTOPOINT, ASN_INTEGER, RWRITE,
     ieee8021BridgeBasePortTable_var, 2, {1, 10}},
#define IEEE8021BRIDGEBASEPORTOPERPOINTTOPOINT                9
    {IEEE8021BRIDGEBASEPORTOPERPOINTTOPOINT, ASN_INTEGER, RONLY,
     ieee8021BridgeBasePortTable_var, 2, {1, 11}},
#define IEEE8021BRIDGEBASEPORTNAME                10
    {IEEE8021BRIDGEBASEPORTNAME, ASN_OCTET_STR, RONLY,
     ieee8021BridgeBasePortTable_var, 2, {1, 12}},
};


/******************************************************************************/
// ucd_snmp_init_ieee8021BridgeBasePortTable()
// Initializes the UCD-SNMP-part of the IEEE8021-BRIDGE-MIB:ieee8021BridgeBasePortTable.
/******************************************************************************/
void
ucd_snmp_init_ieee8021BridgeBasePortTable(void)
{
    DEBUGMSGTL(("ieee8021BridgeBasePortTable", "Initializing\n"));

    // Register mib tree to UCD-SNMP core engine
    REGISTER_MIB("ieee8021BridgeBasePortTable",
                 ieee8021BridgeBasePortTable_variables, variable2,
                 ieee8021BridgeBasePortTable_variables_oid);
}


/******************************************************************************/
//
// Variable scalar functions
//
/******************************************************************************/

/******************************************************************************/
// ieee8021BridgeBasePortTable_parse()
// Parse the table entry key from #name.
//
// Returns:
//  -1 on error
//   0 on getnext or getexact
//   1 on getfirst
/******************************************************************************/
static int
ieee8021BridgeBasePortTable_parse(oid * name,
                                  size_t *length,
                                  int exact,
                                  ieee8021BridgeBasePortTable_entry_t *
                                  table_entry)
{
    size_t          op_pos = 11 + 2;
    oid            *op = (oid *) (name + op_pos);

    memset(table_entry, 0, sizeof(*table_entry));

    if (exact && *length < (11 + 1 + 2)) {
        return -1;
    } else if (!exact && *length <= op_pos) {
        if (ieee8021BridgeBasePortTableEntry_getfirst(table_entry)) {
            return -1;
        }
        return 1;               /* getfirst */
    }

    if (*length > op_pos) {
        table_entry->ieee8021BridgeBasePortComponentId = (u_long) * op++;
        op_pos++;
    } else if (exact) {
        return -1;
    } else {
        return 0;
    }
    if (*length > op_pos) {
        table_entry->ieee8021BridgeBasePort = (u_long) * op++;
        op_pos++;
    } else if (exact) {
        return -1;
    } else {
        return 0;
    }

    if (exact && *length != op_pos) {
        return -1;
    }

    return 0;
}

/******************************************************************************/
// ieee8021BridgeBasePortTable_fillobj()
// Fills in #name according to the table entry key.
/******************************************************************************/
static int
ieee8021BridgeBasePortTable_fillobj(oid * name,
                                    size_t *length,
                                    ieee8021BridgeBasePortTable_entry_t *
                                    table_entry)
{
    int             name_pos = 11 + 2;

    name[name_pos++] =
        (oid) table_entry->ieee8021BridgeBasePortComponentId;
    name[name_pos++] = (oid) table_entry->ieee8021BridgeBasePort;

    *length = name_pos;
    return 0;
}


/******************************************************************************/
//
// Variable table functions
//
/******************************************************************************/
/*
 * ieee8021BridgeBasePortTable_var():
 *   Handle this table separately from the scalar value case.
 *   The workings of this are basically the same as for ieee8021BridgeBasePortTable_var above.
 */
u_char         *
ieee8021BridgeBasePortTable_var(struct variable * vp,
                                oid * name,
                                size_t *length,
                                int exact,
                                size_t *var_len,
                                WriteMethod ** write_method)
{
    int             rc;
    oid             newname[MAX_OID_LEN];
    size_t          newname_len;
    ieee8021BridgeBasePortTable_entry_t table_entry;

    *write_method = NULL;
    memcpy((char *) newname, (char *) vp->name,
           (int) (vp->namelen * sizeof(oid)));
    newname_len = vp->namelen;

    if (memcmp(name, vp->name, sizeof(oid) * vp->namelen) != 0) {
        memcpy(name, vp->name, sizeof(oid) * vp->namelen);
        *length = vp->namelen;
    }

    if ((rc =
         ieee8021BridgeBasePortTable_parse(name, length, exact,
                                           &table_entry)) < 0) {
        return NULL;
    } else if (rc > 0) {        /* getfirst */
        if (ieee8021BridgeBasePortTable_fillobj
            (newname, &newname_len, &table_entry)) {
            return NULL;
        }
    } else {
        do {
            if (ieee8021BridgeBasePortTableEntry_get
                (&table_entry, exact ? FALSE : TRUE)) {
                return NULL;
            }
            if (ieee8021BridgeBasePortTable_fillobj
                (newname, &newname_len, &table_entry)) {
                return NULL;
            }
            if (exact) {
                break;
            }
            rc = snmp_oid_compare(newname, newname_len, name, *length);
        } while (rc < 0);
    }

    /*
     * fill in object part of name for current entry
     */
    memcpy((char *) name, (char *) newname,
           (int) (newname_len * sizeof(oid)));
    *length = newname_len;

    /*
     * this is where we do the value assignments for the mib results.
     */
    switch (vp->magic) {
    case IEEE8021BRIDGEBASEPORTIFINDEX:{
            *write_method = ieee8021BridgeBasePortIfIndex_write;
            ieee8021BridgeBasePortTable_global_ret.long_ret =
                table_entry.ieee8021BridgeBasePortIfIndex;
            *var_len =
                sizeof(ieee8021BridgeBasePortTable_global_ret.long_ret);
            return (u_char *) & ieee8021BridgeBasePortTable_global_ret.
                long_ret;
        }
    case IEEE8021BRIDGEBASEPORTDELAYEXCEEDEDDISCARDS:{
            ieee8021BridgeBasePortTable_global_ret.c64_ret =
                table_entry.ieee8021BridgeBasePortDelayExceededDiscards;
            *var_len =
                sizeof(ieee8021BridgeBasePortTable_global_ret.c64_ret);
            return (u_char *) & ieee8021BridgeBasePortTable_global_ret.
                c64_ret;
        }
    case IEEE8021BRIDGEBASEPORTMTUEXCEEDEDDISCARDS:{
            ieee8021BridgeBasePortTable_global_ret.c64_ret =
                table_entry.ieee8021BridgeBasePortMtuExceededDiscards;
            *var_len =
                sizeof(ieee8021BridgeBasePortTable_global_ret.c64_ret);
            return (u_char *) & ieee8021BridgeBasePortTable_global_ret.
                c64_ret;
        }
    case IEEE8021BRIDGEBASEPORTCAPABILITIES:{
            *var_len = table_entry.ieee8021BridgeBasePortCapabilities_len;
            memcpy(ieee8021BridgeBasePortTable_global_ret.string_ret,
                   table_entry.ieee8021BridgeBasePortCapabilities,
                   *var_len);
            ieee8021BridgeBasePortTable_global_ret.string_ret[*var_len] =
                '\0';
            return (u_char *) ieee8021BridgeBasePortTable_global_ret.
                string_ret;
        }
    case IEEE8021BRIDGEBASEPORTTYPECAPABILITIES:{
            *var_len =
                table_entry.ieee8021BridgeBasePortTypeCapabilities_len;
            memcpy(ieee8021BridgeBasePortTable_global_ret.string_ret,
                   table_entry.ieee8021BridgeBasePortTypeCapabilities,
                   *var_len);
            ieee8021BridgeBasePortTable_global_ret.string_ret[*var_len] =
                '\0';
            return (u_char *) ieee8021BridgeBasePortTable_global_ret.
                string_ret;
        }
    case IEEE8021BRIDGEBASEPORTTYPE:{
            ieee8021BridgeBasePortTable_global_ret.long_ret =
                table_entry.ieee8021BridgeBasePortType;
            *var_len =
                sizeof(ieee8021BridgeBasePortTable_global_ret.long_ret);
            return (u_char *) & ieee8021BridgeBasePortTable_global_ret.
                long_ret;
        }
    case IEEE8021BRIDGEBASEPORTEXTERNAL:{
            ieee8021BridgeBasePortTable_global_ret.long_ret =
                table_entry.ieee8021BridgeBasePortExternal;
            *var_len =
                sizeof(ieee8021BridgeBasePortTable_global_ret.long_ret);
            return (u_char *) & ieee8021BridgeBasePortTable_global_ret.
                long_ret;
        }
    case IEEE8021BRIDGEBASEPORTADMINPOINTTOPOINT:{
            *write_method = ieee8021BridgeBasePortAdminPointToPoint_write;
            ieee8021BridgeBasePortTable_global_ret.long_ret =
                table_entry.ieee8021BridgeBasePortAdminPointToPoint;
            *var_len =
                sizeof(ieee8021BridgeBasePortTable_global_ret.long_ret);
            return (u_char *) & ieee8021BridgeBasePortTable_global_ret.
                long_ret;
        }
    case IEEE8021BRIDGEBASEPORTOPERPOINTTOPOINT:{
            ieee8021BridgeBasePortTable_global_ret.long_ret =
                table_entry.ieee8021BridgeBasePortOperPointToPoint;
            *var_len =
                sizeof(ieee8021BridgeBasePortTable_global_ret.long_ret);
            return (u_char *) & ieee8021BridgeBasePortTable_global_ret.
                long_ret;
        }
    case IEEE8021BRIDGEBASEPORTNAME:{
            *var_len = table_entry.ieee8021BridgeBasePortName_len;
            memcpy(ieee8021BridgeBasePortTable_global_ret.string_ret,
                   table_entry.ieee8021BridgeBasePortName, *var_len);
            ieee8021BridgeBasePortTable_global_ret.string_ret[*var_len] =
                '\0';
            return (u_char *) ieee8021BridgeBasePortTable_global_ret.
                string_ret;
        }
    default:
        DEBUGMSGTL(("snmpd",
                    "unknown sub-id %d in var_ieee8021BridgeBasePortTable\n",
                    vp->magic));
    }
    return NULL;
}


/******************************************************************************/
//
// Write scalar functions
//
/******************************************************************************/


/******************************************************************************/
//
// Write table functions
//
/******************************************************************************/

/******************************************************************************/
// ieee8021BridgeBasePortIfIndex_write()
/******************************************************************************/
int
ieee8021BridgeBasePortIfIndex_write(int action,
                                    u_char * var_val,
                                    u_char var_val_type,
                                    size_t var_val_len,
                                    u_char * statP,
                                    oid * name, size_t name_len)
{
    long            set_value = var_val ? *((long *) var_val) : 0;
    ieee8021BridgeBasePortTable_entry_t table_entry;
    static ieee8021BridgeBasePortTable_entry_t *old_table_entry_p = NULL;

    switch (action) {
    case RESERVE1:{
            if (var_val_type != ASN_INTEGER) {
                (void) snmp_log(LOG_ERR,
                                "write to ieee8021BridgeBasePortIfIndex: not ASN_INTEGER\n");
                return SNMP_ERR_WRONGTYPE;
            }
            if (var_val_len > sizeof(long)) {
                (void) snmp_log(LOG_ERR,
                                "write to ieee8021BridgeBasePortIfIndex: bad length\n");
                return SNMP_ERR_WRONGLENGTH;
            }
            if (var_val_len > 2147483647) {
                return SNMP_ERR_WRONGLENGTH;
            }
            break;
        }
    case RESERVE2:{
            if ((old_table_entry_p = (ieee8021BridgeBasePortTable_entry_t *)
                 ucd_snmp_callout_malloc(sizeof(*old_table_entry_p))) ==
                NULL) {
                return SNMP_ERR_RESOURCEUNAVAILABLE;
            }
            if (ieee8021BridgeBasePortTable_parse
                (name, &name_len, TRUE, old_table_entry_p)) {
                return SNMP_ERR_RESOURCEUNAVAILABLE;
            }
            if (ieee8021BridgeBasePortTableEntry_get
                (old_table_entry_p, FALSE)) {
                return SNMP_ERR_RESOURCEUNAVAILABLE;
            }
            break;
        }
    case FREE:{
            if (old_table_entry_p) {
                ucd_snmp_callout_free(old_table_entry_p);
                old_table_entry_p = NULL;
            }
            break;
        }
    case ACTION:{
            // Set new configuration
            table_entry = *old_table_entry_p;
            table_entry.ieee8021BridgeBasePortIfIndex = set_value;
            if (ieee8021BridgeBasePortTableEntry_set(&table_entry)) {
                return SNMP_ERR_GENERR;
            }
            break;
        }
    case UNDO:{
            if (old_table_entry_p) {
                int set_rc = ieee8021BridgeBasePortTableEntry_set(old_table_entry_p);
                ucd_snmp_callout_free(old_table_entry_p);
                old_table_entry_p = NULL;
                if (set_rc) {
                    return SNMP_ERR_UNDOFAILED;
                }
            }
            break;
        }
    case COMMIT:{
            ucd_snmp_callout_free(old_table_entry_p);
            old_table_entry_p = NULL;
            break;
        }
    default:
        break;
    }
    return SNMP_ERR_NOERROR;
}

/******************************************************************************/
// ieee8021BridgeBasePortAdminPointToPoint_write()
/******************************************************************************/
int
ieee8021BridgeBasePortAdminPointToPoint_write(int action,
                                              u_char * var_val,
                                              u_char var_val_type,
                                              size_t var_val_len,
                                              u_char * statP,
                                              oid * name, size_t name_len)
{
    long            set_value = var_val ? *((long *) var_val) : 0;
    ieee8021BridgeBasePortTable_entry_t table_entry;
    static ieee8021BridgeBasePortTable_entry_t *old_table_entry_p = NULL;

    switch (action) {
    case RESERVE1:{
            if (var_val_type != ASN_INTEGER) {
                (void) snmp_log(LOG_ERR,
                                "write to ieee8021BridgeBasePortAdminPointToPoint: not ASN_INTEGER\n");
                return SNMP_ERR_WRONGTYPE;
            }
            if (var_val_len > sizeof(long)) {
                (void) snmp_log(LOG_ERR,
                                "write to ieee8021BridgeBasePortAdminPointToPoint: bad length\n");
                return SNMP_ERR_WRONGLENGTH;
            }
            if (set_value != 1 && set_value != 2 && set_value != 3) {
                (void) snmp_log(LOG_ERR,
                                "write to ieee8021BridgeBasePortAdminPointToPoint: bad value\n");
                return SNMP_ERR_WRONGVALUE;
            }
            break;
        }
    case RESERVE2:{
            if ((old_table_entry_p = (ieee8021BridgeBasePortTable_entry_t *)
                 ucd_snmp_callout_malloc(sizeof(*old_table_entry_p))) ==
                NULL) {
                return SNMP_ERR_RESOURCEUNAVAILABLE;
            }
            if (ieee8021BridgeBasePortTable_parse
                (name, &name_len, TRUE, old_table_entry_p)) {
                return SNMP_ERR_RESOURCEUNAVAILABLE;
            }
            if (ieee8021BridgeBasePortTableEntry_get
                (old_table_entry_p, FALSE)) {
                return SNMP_ERR_RESOURCEUNAVAILABLE;
            }
            break;
        }
    case FREE:{
            if (old_table_entry_p) {
                ucd_snmp_callout_free(old_table_entry_p);
                old_table_entry_p = NULL;
            }
            break;
        }
    case ACTION:{
            // Set new configuration
            table_entry = *old_table_entry_p;
            table_entry.ieee8021BridgeBasePortAdminPointToPoint =
                set_value;
            if (ieee8021BridgeBasePortTableEntry_set(&table_entry)) {
                return SNMP_ERR_GENERR;
            }
            break;
        }
    case UNDO:{
            if (old_table_entry_p) {
                int set_rc = ieee8021BridgeBasePortTableEntry_set(old_table_entry_p);
                ucd_snmp_callout_free(old_table_entry_p);
                old_table_entry_p = NULL;
                if (set_rc) {
                    return SNMP_ERR_UNDOFAILED;
                }
            }
            break;
        }
    case COMMIT:{
            ucd_snmp_callout_free(old_table_entry_p);
            old_table_entry_p = NULL;
            break;
        }
    default:
        break;
    }
    return SNMP_ERR_NOERROR;
}
