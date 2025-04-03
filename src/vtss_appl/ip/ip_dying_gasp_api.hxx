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

#ifndef _VTSS_DYING_GASP_API_HXX_
#define _VTSS_DYING_GASP_API_HXX_

#include <main_types.h>

namespace vtss
{
namespace appl
{
namespace ip
{
namespace dying_gasp
{

/**
 * Add a dying gasp buf (if + msg) to the kernel
 *
 * @param interface  [IN]  This is the name of the interface that is going to
 *                         send the dying gasp msg.
 *
 * @param msg        [IN]  The content of the dying gasp msg.
 * @param size       [IN]  Size...
 * @param id         [OUT] If the dying gasp buffer can be installed
 *                         successfully then 'id' field will be updated with
 *                         a unique number that must used to manipulate or
 *                         delete the buffer at a later point.
 * @return           Return code.
 */
mesa_rc vtss_dying_gasp_add(const char *interface,
                            const u8 *msg,
                            size_t size,
                            int *id);

/**
 * Modify an existing dying gasp buffer in the kernel
 *
 * @param id  [IN]  'id' of the dying gasp buffer that is going to be modified
 * @param msg [IN]  The content overwriting the kernel one
 * @return    Return code.
 */
mesa_rc vtss_dying_gasp_modify(int id,
                               const u8 *msg);

/**
 * Delete an existing dying gasp buffer in the kernel
 *
 * @param id  [IN]  Delete an existing dying gasp buffer accordinng to the 'id'
 * @return    Return code.
 */
mesa_rc vtss_dying_gasp_delete(int id);

/**
 * Delete all the existing dying gasp buffer in the kernel
 *
 * @return    Return code. */
mesa_rc vtss_dying_gasp_delete_all(void);

}  /* namespace dying_gasp */
}  /* namespace ip */
}  /* namespace appl */
}  /* namespace vtss */

#endif  /* _VTSS_DYING_GASP_API_HXX_ */

