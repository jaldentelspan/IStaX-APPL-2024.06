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

#ifndef _VTSS_BOARD_MISC_H_
#define _VTSS_BOARD_MISC_H_


/****************************************************************************/
/*  Module Interface                                                                                                                    */
/****************************************************************************/

int board_misc_get_chip_temp(void);

#ifdef VTSS_SW_OPTION_PIC

#ifdef __cplusplus
extern "C" {
#endif

mesa_rc board_pic_read(u8 *data, int offset, int len);
mesa_rc board_pic_verify(void);
mesa_rc board_pic_program(BOOL reboot);
mesa_rc board_pic_erase(void);
void    board_pic_check_update(void);

#ifdef __cplusplus
}
#endif

#endif  /* VTSS_SW_OPTION_PIC */

#endif /* _VTSS_BOARD_MISC_H_ */

