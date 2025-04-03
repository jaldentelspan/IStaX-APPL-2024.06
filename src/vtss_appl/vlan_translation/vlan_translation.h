/*
   Vitesse VLAN Translation software.

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

#ifndef _VLAN_TRANSLATION_H_
#define _VLAN_TRANSLATION_H_

#include "vtss/appl/vlan_translation.h"
#include "main.h"
#include "critd_api.h"
#include "vtss/basics/map.hxx"

struct VTMappingKey {
    u16                   gid;
    mesa_vlan_trans_dir_t dir;
    mesa_vid_t            vid;

    bool operator== (const VTMappingKey &map_key) const
    {
        if ((gid == map_key.gid) && (dir == map_key.dir) && (vid == map_key.vid)) {
            return true;
        }
        return false;
    }

    bool operator< (const VTMappingKey &map_key) const
    {
        if (gid != map_key.gid) {
            return (gid < map_key.gid);
        } else if (dir != map_key.dir) {
            return (dir < map_key.dir);
        } else {
            return (vid < map_key.vid);
        }
    }
};

typedef vtss::Map<VTMappingKey, mesa_vid_t> VTMap;

typedef uint8_t   vt_port_t;
typedef vt_port_t vt_gid_t;

/* Databases and critical sections lock */
typedef struct {
    critd_t   crit;
    VTMap     map;
    CapArray<vt_port_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> ports;
} vlan_trans_global_data_t;

#endif /* _VLAN_TRANSLATION_H_ */
