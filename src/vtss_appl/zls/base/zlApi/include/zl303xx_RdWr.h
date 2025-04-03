

/*******************************************************************************
*
*  $Id: 8410f584bd768792a15e60f6157c94bf8acedd47
*  Copyright 2006-2022 Microchip Technology Inc.
*  All rights reserved.
*
*  Module Description:
*     Control functions for device MAC interface(s)
*
*******************************************************************************/

#ifndef ZL303XX_RDWR_H_
#define ZL303XX_RDWR_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************   INCLUDE FILES   ******************************************/

#include "zl303xx_Global.h"
#include "zl303xx_DeviceSpec.h"
#include "zl303xx_Os.h"      /* Required for Endian Definitions        */
#include "zl303xx_Params.h"

/*****************   DEFINES   ************************************************/

/******* DEVICE <-> CPU ENDIANNESS MACROS  *******/
/* Macros to switch endian order (needed before writing and after reading the
   zl303xx device) for the given CPU Endianness. */
/* Defined here instead of in an OS Porting module since they directly relate
   to the SPI (Read/Write) interface of the zl303xx device. */

/* H  = Host
   LE = Little-Endian
   16 = 16-Bit
   32 = 32-Bit */

#if (__BYTE_ORDER == __LITTLE_ENDIAN)

   #define ZL303XX_H2LE16(val)     (Uint16T)(val)
   #define ZL303XX_LE2H16(val)     (Uint16T)(val)
   #define ZL303XX_H2LE32(val)     (Uint32T)(val)
   #define ZL303XX_LE2H32(val)     (Uint32T)(val)
#elif __BYTE_ORDER == __BIG_ENDIAN
   #define ZL303XX_TRANSPOSE_U16(val)       \
                     (Uint16T)(((Uint16T)(val) << 8) | ((Uint16T)(val) >> 8))

   #define ZL303XX_TRANSPOSE_U32(val)                                      \
                     (Uint32T)(((Uint32T)(val) << (3 * 8)) |               \
                                (((Uint32T)(val) << (8)) & 0x00FF0000) |   \
                                (((Uint32T)(val) >> (8)) & 0x0000FF00) |   \
                                ((Uint32T)(val) >> (3 * 8)) )

   #define ZL303XX_H2LE16(val)     (Uint16T)ZL303XX_TRANSPOSE_U16(val)
   #define ZL303XX_LE2H16(val)     (Uint16T)ZL303XX_TRANSPOSE_U16(val)
   #define ZL303XX_H2LE32(val)     (Uint32T)ZL303XX_TRANSPOSE_U32(val)
   #define ZL303XX_LE2H32(val)     (Uint32T)ZL303XX_TRANSPOSE_U32(val)
#endif

/*****************   DATA TYPES   *********************************************/

/*****************   DATA STRUCTURES   ****************************************/
typedef struct
{
   zl303xx_ChipSelectCtrlS chipSelect;

   zl303xx_BooleanE osExclusionEnable;
} zl303xx_ReadWriteS;

/*****************   EXPORTED GLOBAL VARIABLE DECLARATIONS   ******************/

/*****************   EXTERNAL FUNCTION DECLARATIONS   *************************/
/* Initialize and Close the Read/Write interface */
zlStatusE zl303xx_ReadWriteInit(void);
zlStatusE zl303xx_ReadWriteClose(void);

/* Control access to the Read/Write interface */
zlStatusE zl303xx_ReadWriteLock(void);
zlStatusE zl303xx_ReadWriteUnlock(void);

/* Read/Write a 4-byte (or less) value */
zlStatusE zl303xx_Read(zl303xx_ParamsS *zl303xx_Params, zl303xx_ReadWriteS* par,
                       Uint32T regAddr, Uint32T *data);

zlStatusE zl303xx_Write(zl303xx_ParamsS *zl303xx_Params, zl303xx_ReadWriteS* par,
                        Uint32T regAddr, Uint32T data);

zlStatusE zl303xx_ReadModWrite(zl303xx_ParamsS *zl303xx_Params, zl303xx_ReadWriteS* par,
                               Uint32T regAddr,
                               Uint32T writeData, Uint32T mask,
                               Uint32T *prevRegValue);

/* Read/Write an 8-byte value */
zlStatusE zl303xx_Read64(zl303xx_ParamsS *zl303xx_Params, zl303xx_ReadWriteS* par,
                         Uint32T regAddr, Uint64S *data);

zlStatusE zl303xx_Write64(zl303xx_ParamsS *zl303xx_Params, zl303xx_ReadWriteS* par,
                          Uint32T regAddr, Uint64S data);

/* Read/Write a buffer. The other Read/Write functions eventually traverse these
   calls. These allow a 'stretch' of registers of various sizes to be
   read/written in a single SPI call. Much more control of the data is needed
   when using these routines. */
zlStatusE zl303xx_ReadBuf(zl303xx_ParamsS *zl303xx_Params, zl303xx_ReadWriteS* par,
                          Uint32T regAddr,
                          Uint8T *regBuffer, Uint16T regLen);

zlStatusE zl303xx_WriteBuf(zl303xx_ParamsS *zl303xx_Params, zl303xx_ReadWriteS* par,
                           Uint32T regAddr,
                           Uint8T *regBuffer, Uint16T regLen);

/*****************   INTERNAL FUNCTION DECLARATIONS   *************************/
zlStatusE zl303xx_WriteLow(zl303xx_ParamsS *zl303xx_Params,
                           zl303xx_ChipSelectCtrlS *chipSelect,
                           Uint32T regAddr, Uint16T regLen,
                           Uint8T *dataBuf);

zlStatusE zl303xx_ReadLow(zl303xx_ParamsS *zl303xx_Params,
                          zl303xx_ChipSelectCtrlS *chipSelect,
                          Uint32T regAddr, Uint16T regLen,
                          Uint8T *dataBuf);


typedef enum 
{
    HW_ZL_SINGLE_STEP_RDWR_MUTEX_TAKE,
    HW_ZL_SINGLE_STEP_RDWR_MUTEX_GIVE,
    HW_ZL_SINGLE_STEP_RDWR_MUTEX_CREATE,
    HW_ZL_SINGLE_STEP_RDWR_MUTEX_DESTROY,
    HW_ZL_MULTI_STEP_RDWR_MUTEX_TAKE,
    HW_ZL_MULTI_STEP_RDWR_MUTEX_GIVE,
    HW_ZL_MULTI_STEP_RDWR_MUTEX_CREATE,
    HW_ZL_MULTI_STEP_RDWR_MUTEX_DESTROY,
    HW_USER_OPTIONAL_SINGLE_STEP_RDWR_MUTEX_TAKE,   /* 8 */
    HW_USER_OPTIONAL_SINGLE_STEP_RDWR_MUTEX_GIVE,   /* 9 */
    HW_USER_OPTIONAL_SINGLE_STEP_RDWR_MUTEX_CREATE,
    HW_USER_OPTIONAL_SINGLE_STEP_RDWR_MUTEX_DESTROY,
    HW_USER_OPTIONAL_MULTI_STEP_RDWR_MUTEX_TAKE,    /* 12 */
    HW_USER_OPTIONAL_MULTI_STEP_RDWR_MUTEX_GIVE,    /* 13 */
    HW_USER_OPTIONAL_MULTI_STEP_RDWR_MUTEX_CREATE,
    HW_USER_OPTIONAL_MULTI_STEP_RDWR_MUTEX_DESTROY
} zl303xx_RdWrMutexOperationE;

typedef enum                                /* Sources of the zl303xx_RdWrMutexOperationE: */ 
{
    API_MUTEX_SINGLE_STEP_RDWR_DRIVER,      /* The zl303xx_ReadWriteLock/UnLock() operations within zl303xx_RdWr.c */
    API_MUTEX_MULTI_STEP_RDWR_DRIVER,       /* The driver code multiStep operations within the API */
    OPTIONAL_MUTEX_SINGLE_STEP_RDWR_USER,   /* Custom user RdWr operations that want to share protection with the API */ 
    OPTIONAL_MUTEX_MULTI_STEP_RDWR_USER     /* Custom user multiStep operations that want to share protection with the API */ 
} zl303xx_RdWrMutexUserE;

/* Hardware Protection Mutex Operation API (shared between the drivers, RdWr and user applications) */
Sint32T zl303xx_HwProtectionMutexOp(void *hwParams, zl303xx_RdWrMutexOperationE mutexOp, zl303xx_RdWrMutexUserE userIndicator);
/* User Hardware Protection Mutex Operation API */
extern Sint32T zl303xx_UserHwDeviceProtectionMutexOp(void *hwParams, zl303xx_RdWrMutexOperationE mutexOp, zl303xx_RdWrMutexUserE userIndicator);

#ifdef __cplusplus
}
#endif

#endif /* MULTIPLE INCLUDE BARRIER */

