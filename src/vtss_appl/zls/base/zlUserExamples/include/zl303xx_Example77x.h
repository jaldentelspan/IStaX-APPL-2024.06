

/*******************************************************************************
 *
 *  $Id: 5829f97c957ab0b7024bbe9785d3bfb3bdfb63de
 *
 *  Copyright 2006-2019 Microchip/Microsemi Semiconductor Limited.
 *  All rights reserved.
 *
 *  Module Description:
 *     Supporting interfaces for the 77x examples
 *
 ******************************************************************************/

#ifndef _ZL303XX_EXAMPLE_77X_H_
#define _ZL303XX_EXAMPLE_77X_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************   INCLUDE FILES   ******************************************/
#include "zl303xx_Global.h"
#include "zl303xx_Error.h"
#include "zl303xx_DeviceSpec.h"
#include "zl303xx_AddressMap77x.h"

/*****************   DEFINES   ************************************************/

#define ZLS3077X_MAX_NUM_CHARS_PER_LINE_IN_CONFIG_FILE  (90)
#define ZLS3077X_MAX_NUM_LINES_IN_CONFIG_FILE  (1300)

/*****************   DATA TYPES   *********************************************/

/*****************   DATA STRUCTURES   ****************************************/

typedef struct
{
   zl303xx_DeviceModeE deviceMode;
   Uint32T pllId;
   zl303xx_ParamsS *zl303xx_Params;

} example77xClockCreateS;


typedef struct {
    char line[ZLS3077X_MAX_NUM_CHARS_PER_LINE_IN_CONFIG_FILE];
} example77xStructConfigLine_t;

typedef struct {
    example77xStructConfigLine_t lines[ZLS3077X_MAX_NUM_LINES_IN_CONFIG_FILE];
} example77xStructConfigData_t;


/*****************   EXPORTED GLOBAL VARIABLE DECLARATIONS   ******************/

/*****************   EXTERNAL FUNCTION DECLARATIONS   *************************/

zlStatusE example77xEnvInit(void);
zlStatusE example77xEnvClose(void);
zlStatusE example77xSlaveWarmStart(void);

zlStatusE example77xClockCreateStructInit(example77xClockCreateS *pClock);
zlStatusE example77xClockCreate(example77xClockCreateS *pClock);
zlStatusE example77xClockRemove(example77xClockCreateS *pClock);

zlStatusE example77xLoadConfigFile(zl303xx_ParamsS *zl303xx_Params, const char *filename);
zlStatusE example77xLoadConfigStruct
            (
            zl303xx_ParamsS *zl303xx_Params,
            example77xStructConfigData_t *cData
            );
zlStatusE example77xLoadConfigDefaults(zl303xx_ParamsS *zl303xx_Params);

zlStatusE example77xStickyLockCallout(void *hwParams, zl303xx_DpllIdE pllId, zl303xx_BooleanE lockFlag);

#if defined _ZL303XX_ZLE30360_BOARD || defined _ZL303XX_ZLE1588_BOARD
zlStatusE example77xCheckPhyTSClockFreq(zl303xx_ParamsS *zl303xx_Params);
#endif

#if defined ZLS30771_INCLUDED && !(defined _ZL303XX_ZLE30360_BOARD || defined _ZL303XX_ZLE1588_BOARD)
zlStatusE example77x(void);
#else
zlStatusE example77xSlave(void);
zlStatusE example77xMaster(void);
#endif


#ifdef __cplusplus
}
#endif

#endif /* MULTIPLE INCLUDE BARRIER */
