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
#include "test_api.h"
#include "test_serializer.hxx"

#include "vtss/basics/map.hxx"
#include "vtss/basics/expose/snmp/types.hxx"

vtss_enum_descriptor_t vtss_test_enum_type_txt[] {
    {val_a, "valA"},
    {val_b, "valB"},
    {0, 0},
};

vtss_enum_descriptor_t vtss_table_enum_type_txt[] {
    {val_x, "valX"},
    {val_y, "valY"},
    {0, 0},
};

vtss_enum_descriptor_t vtss_table2_enum_type_txt[] {
    {val_1, "val1"},
    {val_2, "val2"},
    {0, 0},
};

static xyz_t theXyz;
mesa_rc vtss_test_xyz_get(xyz_t *conf) {
    *conf = theXyz;
    return MESA_RC_OK;
}
mesa_rc vtss_test_xyz_set(const xyz_t *conf) {
    theXyz = *conf;
    return MESA_RC_OK;
}

static SomeConf state;
mesa_rc vtss_test_some_conf_get(SomeConf *conf) {
    *conf = state;
    return MESA_RC_OK;
}

mesa_rc vtss_test_some_conf_set(const SomeConf *conf) {
    state = *conf;
    return MESA_RC_OK;
}

static SubConf subState;
mesa_rc vtss_test_sub_conf_get(SubConf *conf) {
    *conf = subState;
    return MESA_RC_OK;
}

mesa_rc vtss_test_sub_conf_set(const SubConf *conf) {
    subState = *conf;
    return MESA_RC_OK;
}


mesa_rc vtss_test_some_status_get(SomeStatus *status) {
    status->d = 1;
    status->e = val_a;
    status->f = 3;
    return MESA_RC_OK;
}

mesa_rc vtss_test_some_status2_get(SomeDetailedStatus *status) {
    status->g = 101;
    status->h = 102;
    status->i = 103;
    return MESA_RC_OK;
}

static char theDisplayString[256]="abc.ds";
static BOOL theBitMappedString[32] = { 
    true, false, true, false, 
    true, false, true, true,
    true, true, false, false,
    true, true, false, true,
    true, true, true, false,
    true, true, true, true,
    true, true, true, false,
    true, true, false, false,
 };
static uint8_t theRealOctetString[32] = {
    0,'1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1'};
static mesa_ipv4_t theIpv4Address = 0x7f000001;
static mesa_mac_t theMacAddress = { {1, 3, 7, 15, 31, 63} };

mesa_rc vtss_abc_get(Abc_t *abc) {
    strncpy(abc->ds, theDisplayString, 32);
    memcpy(abc->bitMappedString, theBitMappedString, 30);
    memcpy(abc->octetstring, theRealOctetString, 32);
    abc->anIpAddress = theIpv4Address;
    abc->macAddress = theMacAddress;
    return MESA_RC_OK;
}

mesa_rc vtss_abc_set(const Abc_t *abc) {
    strncpy(theDisplayString, abc->ds, 32);
    memcpy(theBitMappedString, abc->bitMappedString, 30);
    memcpy(theRealOctetString, abc->octetstring, 32);
    theIpv4Address = abc->anIpAddress;
    theMacAddress = abc->macAddress;
    return MESA_RC_OK;
}


mesa_rc vtss_table1_get(mesa_vid_t vid, SomeStatus *s) {
    switch (vid) {
        case 1000:
        case 2000:
        case 3000:
        case 4000:
            memset(s, 0, sizeof(SomeStatus));
            VTSS_BASICS_TRACE(NOISE) << "get(" << vid << ") -> OK";
            return MESA_RC_OK;

        default:
            VTSS_BASICS_TRACE(NOISE) << "get(" << vid << ") -> ERROR";
            return MESA_RC_ERROR;
    }
}

mesa_rc vtss_table1_set(mesa_vid_t vid, const SomeStatus  *s) {
    return MESA_RC_ERROR;
}

mesa_rc vtss_table1_add(mesa_vid_t vid, const SomeStatus  *s) {
    return MESA_RC_ERROR;
}

mesa_rc vtss_table1_del(mesa_vid_t vid) {
    return MESA_RC_ERROR;
}

mesa_rc vtss_table1_itr(const mesa_vid_t *last, mesa_vid_t *next) {
    if (last == 0) {
        *next = 1;
        VTSS_BASICS_TRACE(NOISE) << "itr(null) -> OK, " << *next;
        return MESA_RC_OK;
    }

    if (*last >= 4095) {
        VTSS_BASICS_TRACE(NOISE) << "itr(" << *last << ") -> ERR";
        return MESA_RC_ERROR;
    }

    *next = *last + 1;
    VTSS_BASICS_TRACE(NOISE) << "itr(" << *last << ") -> OK, " << *next;
    return MESA_RC_OK;
}

typedef struct {
    mesa_vid_t vid;
    mesa_vid_t vid_inner;
    Table2Status s;
    int i;
} TEST_TABLE_2_t;

static const uint32_t TEST_TABLE_2_CNT = 6;
static TEST_TABLE_2_t TEST_TABLE_2[] = {
    {1,  2,  { 3, val_1,  4}, 10},
    {1,  6,  { 7, val_2,  8}, 20},
    {9,  10, {11, val_1, 12}, 30},
    {13, 14, {15, val_2, 16}, 40},
    {13, 18, {19, val_1, 20}, 50},
    {21, 22, {23, val_2, 24}, 60},
};

mesa_rc vtss_table2_get(mesa_vid_t vid, mesa_vid_t vid_inner,
                        Table2Status *s, int *i) {
    bool found_one = false;
    TEST_TABLE_2_t *p = TEST_TABLE_2;

    for (uint32_t i = 0; i < TEST_TABLE_2_CNT; ++i, ++p) {
        if (vid != p->vid || vid_inner != p->vid_inner)
            continue;

        found_one = true;
        break;
    }

    if (!found_one)
        return MESA_RC_ERROR;

    *s = p->s;
    *i = p->i;
    return MESA_RC_OK;
}

mesa_rc vtss_table2_set(mesa_vid_t vid, mesa_vid_t vid_inner,
                        const Table2Status *s, const int *i) {
    return MESA_RC_ERROR;
}

mesa_rc vtss_table2_add(mesa_vid_t vid, mesa_vid_t vid_inner,
                        const Table2Status *s, const int *i) {
    return MESA_RC_ERROR;
}

mesa_rc vtss_table2_del(mesa_vid_t vid, mesa_vid_t vid_inner) {
    return MESA_RC_ERROR;
}

mesa_rc vtss_table2_itr(const mesa_vid_t *last_vid1, mesa_vid_t *next_vid1,
                        const mesa_vid_t *last_vid2, mesa_vid_t *next_vid2) {
    mesa_vid_t a = 0, b = 0;
    if (last_vid1) a = *last_vid1;
    if (last_vid2) b = *last_vid2;

    TEST_TABLE_2_t *p = TEST_TABLE_2;
    for (uint32_t i = 0; i < TEST_TABLE_2_CNT; ++i, ++p) {
        if (p->vid < a) continue;
        if (p->vid_inner <= b) continue;

        *next_vid1 = p->vid;
        *next_vid2 = p->vid_inner;

        return MESA_RC_OK;
    }

    return MESA_RC_ERROR;
}

static vtss::Map<mesa_vid_t, SomeStatus> t3;
mesa_rc vtss_table3_get(mesa_vid_t vid, SomeStatus *s) {
    auto i = t3.find(vid);

    if (i == t3.end())
        return MESA_RC_ERROR;

    *s = i->second;
    return MESA_RC_OK;
}

mesa_rc vtss_table3_set(mesa_vid_t vid, const SomeStatus *s) {
    if (vid <= 0 || vid >= 4095)
        return MESA_RC_ERROR;

    t3.set(vid, *s);
    return MESA_RC_OK;
}

mesa_rc vtss_table3_del(mesa_vid_t vid) {
    if (t3.erase(vid) == 0)
        return MESA_RC_ERROR;
    else
        return MESA_RC_OK;
}

mesa_rc vtss_table3_itr(const mesa_vid_t *last, mesa_vid_t *next) {
    mesa_vid_t i = 0;

    if (last)
        i = *last;

    auto it = t3.greater_than(i);

    if (it == t3.end())
        return MESA_RC_ERROR;

    *next = it->first;
    return MESA_RC_OK;
}

mesa_rc leaf_12_get(uint32_t *a, uint32_t *b) {
    *a = 10;
    *b = 20;
    return MESA_RC_OK;
}

static uint32_t leaf_13_a = 10;
static uint32_t leaf_13_b = 20;
mesa_rc leaf_13_get(uint32_t *a, uint32_t *b) {
    *a = leaf_13_a;
    *b = leaf_13_b;
    return MESA_RC_OK;
}

mesa_rc leaf_13_set(uint32_t a, uint32_t b) {
    leaf_13_a = a;
    leaf_13_b = b;
    return MESA_RC_OK;
}

