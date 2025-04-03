

/*******************************************************************************
*
*  $Id: 8410f584bd768792a15e60f6157c94bf8acedd47
*  Copyright 2006-2022 Microchip Technology Inc.
*  All rights reserved.
*
*  Module Description:
*     ReadWrite and Multistep mutex abstractions for custom modification
*
*******************************************************************************/

#ifndef ZL303XX_ABSTRACT_RDWR_MUTEXES_H_
#define ZL303XX_ABSTRACT_RDWR_MUTEXES_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************   INCLUDE FILES   ******************************************/

#include "zl303xx_RdWr.h"

/*****************   INTERNAL FUNCTION DECLARATIONS   *************************/

/* User Hardware Device Protection Mutex Operation API */
Sint32T zl303xx_UserHwDeviceProtectionMutexOp(void *hwParams, zl303xx_RdWrMutexOperationE mutexOp, zl303xx_RdWrMutexUserE userIndicator);


#ifdef __cplusplus
}
#endif

#endif /* MULTIPLE INCLUDE BARRIER */


