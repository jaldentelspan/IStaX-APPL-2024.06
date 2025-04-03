

/*******************************************************************************
*
*  $Id: 6517a3b4707c071ac26ad8fcef236bd9d8d99889
*
*  Copyright (c) 2006-2022 Microchip Technology Inc. and its subsidiaries, all rights reserved.
*  Subject to the terms of the license that accompanies the software and controls as it relates to the software and any conflicting terms herein, you may use this Microchip software and any derivatives exclusively with Microchip products.
*  You are responsible for complying with third party license terms applicable to your use of third party software (including open source software) that may accompany this Microchip software.
*  SOFTWARE IS 'AS IS'. NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
*  IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.
*  TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS RELATED TO THE SOFTWARE WILL NOT EXCEED AMOUNT OF FEES, IF ANY, YOU PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*
*  Module Description:
*     ZL30731 DPLL Global prototypes.
*
*******************************************************************************/

#ifndef ZL303XX_DPLL_73X_GOLBAL_H_
#define ZL303XX_DPLL_73X_GOLBAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************   INCLUDE FILES   ******************************************/
#include "zl303xx_Global.h"
#include "zl303xx_DataTypes.h"
#include "zl303xx_Int64.h"
#include "zl303xx_Error.h"
#include "zl303xx_Trace.h"
#include "zl303xx_AddressMap73x.h"
#include "zl303xx_DeviceSpec.h"
#include "zl303xx_RdWr.h"
#include "zl303xx_Macros.h"
#include "zl303xx_Var.h"

#include "zl303xx_Dpll73xReport.h"
#include "zl303xx_Dpll73xDynamicOp.h"
#include "zl303xx_Dpll73xConfig.h"
#include "zl303xx_Dpll73xMsgRouter.h"

/*****************   DEFINES   ************************************************/

/* Scaled dco count per ppm for 73x. Each 1 count of the 73x register is:
      x           1
   ------- = -----------    => x = 281474974
     2^48    1000000(ppm)
*/
#define ZLS3073X_DCO_COUNT_PER_PPM (281474974)

#define ZL303XX_MAX_NUM_73X_PARAMS (8)

#define ZLS3073X_REG_READ_INTERVAL (10)

#define ZLS3073X_TIMER_DELAY_MS   (1000)

#define ZLS3073X_GUARD_TIMER_LENGTH_IN_SEC   (1)

#define ZLS3073X_RESTORE_TIMER_LENGTH_IN_SEC (30)

#define ZLS3073X_STEP_TIME_SANITY_TIMEOUT    (60)


/*****************   DATA TYPES   *********************************************/
typedef enum {
     ZL303XX_73X_PROTECT_LOCAL_PROCESS_DATA = 0,  /* Protects local device config. info. */
     ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA,  /* Protects a Multi Read/Write operation */
     ZL303XX_73X_PROTECT_DEVICE_MAX = ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA
} zl303xx_DeviceProtect73xOpsE;

/*****************   DATA STRUCTURES   ****************************************/
typedef struct {
    Sint32T deltaTimeRU_TSDriver;
    Sint32T deltaTimeRU_NotTSDriver;
} zl303xx_Dpll73xAutoTestDataS;

/*****************   EXPORTED GLOBAL VARIABLE DECLARATIONS   ******************/

/* Short sanity timeout when accessing registers that should return quickly */
extern Uint32T Dpll73xShortSanityTO;
/* Medium sanity timeout when accessing registers */
extern Uint32T Dpll73xMediumSanityTO;
/* Long sanity timeout when accessing registers */
extern Uint32T Dpll73xLongSanityTO;

/*****************   EXTERNAL FUNCTION DECLARATIONS   *************************/

/* Init */
Sint32T zl303xx_Dpll73xGetDeviceInfo(void *hwParams, zl303xx_DevTypeE *devType, zl303xx_DeviceIdE *devId, Uint32T *devRev);
Sint32T zl303xx_Dpll73xGetFWRevision(void *hwParams, Uint32T *devFWRev);
zlStatusE zl303xx_Dpll73xParamsMutexInit(void *hwParams);
zlStatusE zl303xx_Dpll73xParamsMutexDelete(void *hwParams);
zlStatusE zl303xx_Dpll73xLocalConfigInit(void *hwParams);
zlStatusE zl303xx_Dpll73xParamsInit(void *hwParams);
zlStatusE zl303xx_Dpll73xParamsClose(void *hwParams);

/* Mutex */
zlStatusE zl303xx_Dpll73xMutexesTake(void *, zl303xx_DeviceProtect73xOpsE, const char *callingFunctionName, zlStatusE *returnStatus);
zlStatusE zl303xx_Dpll73xMutexesGive(void *, zl303xx_DeviceProtect73xOpsE, const char *callingFunctionName, zlStatusE *returnStatus);

/* Mailbox */
zlStatusE zl303xx_Dpll73xGetMailBoxSemaphoreStatus(void *hwParams,
                                                 ZLS3073X_MailboxE mbId, ZLS3073X_SemStatusE *semStatus);
zlStatusE zl303xx_Dpll73xWaitForAvailableMailBoxSemaphore(void *hwParams,
                                                        ZLS3073X_MailboxE mbId);
zlStatusE zl303xx_Dpll73xSetupMailboxForRead(void *hwParams, ZLS3073X_MailboxE mbId,
                                           Uint32T mbNum);
zlStatusE zl303xx_Dpll73xSetupMailboxForWrite(void *hwParams, ZLS3073X_MailboxE mbId,
                                            Uint32T mbNum);
zlStatusE zl303xx_Dpll73xSetMailboxSemWrite(void *hwParams, ZLS3073X_MailboxE mbId);
zlStatusE zl303xx_Dpll73xUpdateMailboxCopy(void *hwParams, ZLS3073X_MailboxE mbId,
                                         Uint32T mbNum);
zlStatusE zl303xx_Dpll73xUpdateAllMailboxCopies(void *hwParams);

zlStatusE zl303xx_Dpll73xStickyLockSet(void *zl303xx_Params,
                                     ZLS3073X_StickyLockE lock);

/* Param */
zlStatusE zl303xx_Dpll73xStoreDeviceParam(void *hwParams);
zlStatusE zl303xx_Dpll73xRemoveDeviceParam(void *hwParams);

/* Task */
zlStatusE zl303xx_Dpll73xTaskStart(void);
zlStatusE zl303xx_Dpll73xTaskStop(void);


zlStatusE zl303xx_Dpll73xCalculateTimeDiff(
      Uint64S xSeconds, Uint32T xNanosec,
      Uint64S ySeconds, Uint32T yNanosec,
      Uint64S *pResultSec, Uint32T *pResultNsec,
      zl303xx_BooleanE *pResultNegative);


#ifdef __cplusplus
}
#endif

#endif /* MULTIPLE INCLUDE BARRIER */
