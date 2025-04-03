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

/**
 * \file
 * \brief mirror icli functions
 * \details This header file describes firmware iCLI
 */

#ifndef _VTSS_ICLI_FIRMWARE_H_
#define _VTSS_ICLI_FIRMWARE_H_

#include "vtss_remote_file_transfer.hxx"

#ifdef __cplusplus
extern "C" {
#endif
/**
 * \brief Function for printing firmware information.
 *
 * \param session_id [IN]  Needed for being able to print messages
 * \return None.
 **/
void firmware_icli_show_version(i32 session_id);

/*
 * \brief Function for swapping between firmware active and alternative images.
 *
 * \param session_id [IN]  Needed for being able to print error messages
 * \return None.
 **/
void firmware_icli_swap(i32 session_id);

/*
 * \brief Function for loading new firmware image.
 *
 * \param session_id [IN]  Needed for being able to print error messages
 * \param url        [IN]  IP address, path and file name for the tftp server containing the new image
 * \param save_host_key [IN]  If true then any unknown or changed SSH host keys should be accepted
 * \return None.
 **/
void firmware_icli_upgrade(i32 session_id, const char *url, 
                           vtss::remote_file_options_t &transfer_options);

mesa_rc firmware_icli_bootstrap_ubi(i32 session_id, unsigned int mbytes_limit);
mesa_rc firmware_icli_bootstrap_mmc(i32 session_id, unsigned int mbytes_limit);

/*
 * \brief Function for updating nand flash.
 *
 * \param session_id [IN] Needed for being able to print error messages
 * \param tftpserver_path_file [IN] IP address, path and file name for the tftp server containing the new image
 * \param nandsize [IN] NAND volume size (Mb)
 * \param force [IN] Force bootstrapping (no checks)
 * \return None.
 **/
void firmware_icli_bootstrap(i32 session_id,
                             const char *tftpserver_path_file,
                             unsigned int nandsize,
                             bool force,
                             vtss::remote_file_options_t &transfer_options);

/*
 * \brief Function for loading new bootloader image.
 *
 * \param session_id [IN]  Needed for being able to print error messages
 *
 * \param url [IN]  URL for the new image
 *
 * \return None.
 **/
void firmware_icli_bootloader(i32 session_id, const char *url, 
                              vtss::remote_file_options_t &transfer_options);

/*
 * \brief Function for loading firmware image into specific FIS entry.
 *
 * \param session_id [IN]  Needed for being able to print error messages
 *
 * \param fis_name [IN]  FIS entry
 *
 * \param url [IN]  URL for the new image
 *
 * \return None.
 **/
void firmware_icli_load_fis(i32 session_id, const char *fir_name, const char *url, 
                            vtss::remote_file_options_t &transfer_options);

/*
 * \brief Function for loading firmware image into ram
 *
 * \param session_id [IN]  Needed for being able to print error messages
 *
 * \param url [IN]  URL for the ram image
 *
 * \return None.
 **/
void firmware_icli_load_ram(i32 session_id, const char *url, 
                            vtss::remote_file_options_t &transfer_options);

/*
 * \brief Function for swaping firmware image under Linux
 *
 * \param session_if [IN] Needed for being able to print error messages
 *
 * \return None.
 **/
void firmware_icli_swap_image(i32 session_id);

/*
 * \brief Showing MFI image information.
 *
 * \param session_if [IN] Needed for being able to print error messages
 *
 * \return None.
 **/
void firmware_icli_show_info(i32 session_id);

#ifdef __cplusplus
}
#endif
#endif /* _VTSS_ICLI_FIRMWARE_H_ */

