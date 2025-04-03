

/******************************************************************************
*
*  $Id: 8410f584bd768792a15e60f6157c94bf8acedd47
*
*  Copyright 2006-2022 Microchip Technology Inc.
*  All rights reserved.
*
*  Module Description:
*     This provides functions for sending and receiving PTP format timestamped
*     packets.
*
******************************************************************************/

#ifndef _ZL303XX_VXWORKS_ETHER_HOOKS_H_
#define _ZL303XX_VXWORKS_ETHER_HOOKS_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef OS_VXWORKS    /* File only applies to VxWorks builds */

/*****************   INCLUDE FILES   ******************************************/
#include "zl303xx_Global.h"
#include "zl303xx_Error.h"

/*****************   DEFINES     **********************************************/

/*****************   DATA TYPES   *********************************************/

/*****************   DATA STRUCTURES   ****************************************/

/*****************   EXTERNAL FUNCTION DECLARATIONS   *************************/
#if defined (ZL_USE_ETHERHOOK) || defined (ZL_USE_SNARF)
zlStatusE zl303xx_VxAttachEtherHook(char * ifaceName, Uint16T ifaceNumber);
zlStatusE zl303xx_VxDetachEtherHook(char * ifaceName, Uint16T ifaceNumber);
#endif /* defined ZL_USE_ETHERHOOK || defined ZL_USE_SNARF */
#endif   /* OS_VXWORKS */

#ifdef __cplusplus
}
#endif

#endif

