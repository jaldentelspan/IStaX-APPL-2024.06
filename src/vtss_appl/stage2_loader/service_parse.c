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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

#include "service.h"

int attr_validate_appearence(const AttrConf_t *conf, struct Attr *attr)
{
    struct Attr *elt;
    int cnt;
    DL_COUNT(attr, elt, cnt);
    switch (conf[attr->attr_type].appearence) {
        case ATTR_REQUIRED: {
            if (cnt != 1) {
                printf("%s appeared %d time(s), expect %d!\n",
                       conf[attr->attr_type].key_name,
                       cnt, 1);
                return 0;
            }
            break;
        }
        case ATTR_OPTIONAL: {
            if (cnt > 1) {
                printf("%s appeared %d time(s), expect 0 or 1!\n",
                       conf[attr->attr_type].key_name,
                       cnt);
                return 0;
            }
            break;
        }
        case ATTR_DEFAULT: {
            if (cnt == 0) {
                printf("%s not defined, use default value instead!\n",
                       conf[attr->attr_type].key_name);
                DL_APPEND(attr, conf[attr->attr_type].create(conf[attr->attr_type].default_value));
            } else if (cnt == 1) {
                // do nothing
            } else {
                printf("%s appeared %d time(s), expect 0 or 1!\n",
                       conf[attr->attr_type].key_name,
                       cnt);
                return 0;
            }
            break;
        }
        case ATTR_SEQUENCE: {
            break;
        }
    }
    return 1;
}

int service_validate(const AttrConf_t *conf, Service_t *service)
{
    int i, rc = 1;

    for (i = 0; i < SERVICE_CNT; i++) {
        if (service->attr[i] &&
            attr_validate_appearence(conf, service->attr[i]) == 0) {
            rc = 0;
            break;
        }
    }
    return rc;
}

int config_parse(char *data,
                 size_t data_len,
                 const AttrConf_t *conf,
                 int conf_size,
                 struct Attr **result)
{
    char *key, *val;
    struct Attr *attr;

    while (data_len) {
        size_t l = attr_get_line(data, data_len);
        if (attr_split_into_key_val(data, &key, &val)) {
            attr = attr_construct(conf, conf_size, key, val);
            if (attr) {
                DL_APPEND(result[attr->attr_type], attr);
            }
        }
        data += l;
        data_len -= l;
    }
    return 1;
}
