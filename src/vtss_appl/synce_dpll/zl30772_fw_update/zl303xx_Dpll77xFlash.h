/*******************************************************************************
*
*  $Id: 4899e900e59c66d2345501933f3c6287f7ad4f29
*
*  Copyright 2006-2020 Microchip/Microsemi Semiconductor Limited.
*  All rights reserved.
*
*  Module Description:
*     ZL30771 DPLL functions and registers for flash
*
*******************************************************************************/

#ifndef ZL303XX_DPLL_77X_FLASH_H_
#define ZL303XX_DPLL_77X_FLASH_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************   INCLUDE FILES   ******************************************/
#if defined ZLS30771_INCLUDED
/* ZLS30771 package porting */
#include "zl303xx_Global.h"

#else
/* define a minumum set for porting */
#include "zl303xx_FlashPorting.h"

#endif

/*****************   TYPES   ************************************************/

/*****************   DEFINES   ************************************************/

/* Macro to add an enum to a given bitmap */
#define ZL303XX_DPLL_77X_FLASH_INTEGRITY_MAP_ADD(bitmap, enum) do { bitmap |= ((Uint32T)1 << (enum)); } while(0)

/* Macro to remove an enum from a given bitmap */
#define ZL303XX_DPLL_77X_FLASH_INTEGRITY_MAP_REM(bitmap, enum) do { bitmap &= ~((Uint32T)1 << (enum)); } while(0)

/* Macro to test an enum is contained in given bitmap */
#define ZL303XX_DPLL_77X_FLASH_INTEGRITY_MAP_TEST(bitmap, enum) (((bitmap) & ((Uint32T)1 << (enum))) != 0)

/**
Converts a zl303xx_Dpll77xFlashConfigE to corresponding zl303xx_Dpll77xFlashIntegrityE
or returns ZL303XX_DPLL_77X_FLASH_INTEGRITY_COUNT if invalid.
*/
#define ZL303XX_DPLL_77X_FLASH_INTEGRITY_FROM_CONFIG(config) \
    ((zl303xx_Dpll77xFlashConfigE)config < ZL303XX_DPLL_77X_FLASH_CONFIG_COUNT ? \
        (zl303xx_Dpll77xFlashIntegrityE)(ZL303XX_DPLL_77X_FLASH_INTEGRITY_CONFIG0 + (zl303xx_Dpll77xFlashIntegrityE)config) : \
        (zl303xx_Dpll77xFlashIntegrityE)(ZL303XX_DPLL_77X_FLASH_INTEGRITY_COUNT))


/*****************   DATA TYPES   *********************************************/

/**
  Config image indexes.
*/
typedef enum {
    ZL303XX_DPLL_77X_FLASH_CONFIG0 = 0,
    ZL303XX_DPLL_77X_FLASH_CONFIG1 = 1,
    ZL303XX_DPLL_77X_FLASH_CONFIG2 = 2,
    ZL303XX_DPLL_77X_FLASH_CONFIG_COUNT
} zl303xx_Dpll77xFlashConfigE;

/**
  Integrity check indexes.
  See
*/
typedef enum {
    ZL303XX_DPLL_77X_FLASH_INTEGRITY_FIRMWARE1        = 0,
    ZL303XX_DPLL_77X_FLASH_INTEGRITY_FIRMWARE2_COPY   = 1,
    ZL303XX_DPLL_77X_FLASH_INTEGRITY_RESERVED2        = 2,
    ZL303XX_DPLL_77X_FLASH_INTEGRITY_RESERVED3        = 3,
    ZL303XX_DPLL_77X_FLASH_INTEGRITY_RESERVED4        = 4,
    ZL303XX_DPLL_77X_FLASH_INTEGRITY_CONFIG0          = 5,
    ZL303XX_DPLL_77X_FLASH_INTEGRITY_CONFIG1          = 6,
    ZL303XX_DPLL_77X_FLASH_INTEGRITY_CONFIG2          = 7,
    ZL303XX_DPLL_77X_FLASH_INTEGRITY_FIRMWARE2        = 8,
    ZL303XX_DPLL_77X_FLASH_INTEGRITY_RESERVED9        = 9,
    ZL303XX_DPLL_77X_FLASH_INTEGRITY_RESERVED10       = 10,
    ZL303XX_DPLL_77X_FLASH_INTEGRITY_RESERVED11       = 11,
    ZL303XX_DPLL_77X_FLASH_INTEGRITY_COUNT
} zl303xx_Dpll77xFlashIntegrityE;


/*****************   DATA STRUCTURES   ****************************************/

typedef struct {
    /** Input filepaths... */
    const char *uPath;                     /* Utility path (required) */
    const char *fwPath;                    /* Firmware Image 1 path (optional, can be NULL to not update) */
    const char *fw2Path;                   /* Firmware Image 2 path (optional, can be NULL to not update) */
    const char *configPath[ZL303XX_DPLL_77X_FLASH_CONFIG_COUNT]; /* Config Hex Image N path (optional, can
                                                                 be NULL to not update).
                                                                 See corresponding configErase which must be
                                                                 ZL303XX_FALSE to update (if non-NULL). */

    /** Input options...  */
    zl303xx_BooleanE configErase[ZL303XX_DPLL_77X_FLASH_CONFIG_COUNT]; /* Config Image N Erase Option where ZL303XX_TRUE
                                                                     erases config (default ZL303XX_FALSE).
                                                                     For ZL303XX_TRUE, burnEnabled must be ZL303XX_TRUE. */

    zl303xx_BooleanE keepSynthesizersOn;     /* Default False. If True, keep synthesizers enabled (on).
                                             If False, disable (turn off) synthesizers (and outputs).*/

    zl303xx_BooleanE burnEnabled;            /* Default True. If True, will update device.
                                             If False, will not update device (e.g. for testing / dry run). */

    zl303xx_BooleanE skipSanityCheck;        /* Default False. If False, will perform sanity checks on known
                                             registers as expected.
                                             If True, will not perform sanity checks (not recommended).
                                             Set True only when there is a bad image burned into device that is
                                             preventing passing sanity tests. Should not be used for porting. */

    Uint32T skipIntegrityCheckMask;        /* Bitmap of zl303xx_Dpll77xFlashIntegrityE enums to skip during
                                             integrity check. Default sets all the config records to be
                                             skipped as empty configs would fail the integrity check.
                                             See ZL303XX_DPLL_77X_FLASH_INTEGRITY_MAP macros.
                                             Recommended to keep default settings. */

    /** Output results...  */
    zl303xx_BooleanE resultSuccess;          /* Returns ZL303XX_TRUE if operation was successful */
    Uint32T resultErrorCount;              /* For Microsemi use. Returns error count (0 for no error) */
    Uint32T resultErrorCause;              /* For Microsemi use. Returns error condition details. */
    zl303xx_BooleanE resultIntegrityValid;   /* For Microsemi use. Returns ZL303XX_TRUE if resultIntegrityFails has been set */
    Uint32T resultIntegrityFails;          /* For Microsemi use. Returns bitmap of integrity check failures (0 for no errors).
                                             See zl303xx_Dpll77xFlashIntegrityE for bits where enum 0 is LSb.
                                             See See ZL303XX_DPLL_77X_FLASH_INTEGRITY_MAP macros. */

    /** Output progress... */
    Uint8T resultProgressPercent;          /* Returns last progress percent reached (e.g. 100 if reached end) */
    const char *resultProgressStr;         /* Returns last progress string reached */


    /** Callouts... */
    /* Device register read interface (can be NULL when used with ZLS30771 package to use zl303xx_Read)

    Parameters:
    [in]  hwparams    Pointer to user specified device.
        [in]  arg         Pointer to user specified options.
        [in]  address     Virtual register address to access (i.e. combined page & offset for debugging)
        [in]  page        Page containing register to access
        [in]  offset      Offset within page containing register to access
        [in]  size        Size of register to access (1, 2, or 4)
        [out] value       Pointer to 32-bit integer to store read value.

    Return Value:
         0 for success,
         other for error
     */
    Sint32T (*readFn)(void *hwParams, void *arg, Uint32T address, Uint16T page, Uint16T offset, Uint8T size, Uint32T *value);

    /* Device register write interface  (can be NULL when used with ZLS30771 package to use zl303xx_Write)

    Parameters:
    [in]  hwparams    Pointer to user specified device.
        [in]  arg         Pointer to user specified options.
        [in]  address     Virtual register address to access (i.e. combined page & offset for debugging)
        [in]  page        Page containing register to access
        [in]  offset      Offset within page containing register to access
        [in]  size        Size of register to access (1, 2, or 4)
        [in]  value       Register value to write.

    Return Value:
         0 for success,
         other for error
     */
    Sint32T (*writeFn)(void *hwParams, void *arg, Uint32T address, Uint16T page, Uint16T offset, Uint8T size, Uint32T value);

    /* Optional. Second argument for the readFn and writeFn (can be NULL) */
    void *readWriteFnArg;

    /* Optional. Reporting callback function.
     *
     * If non-NULL, this function will be called intermittently to report
     * progress of the process.
     * Note identical reports may be received to indicate activity during long
     * tasks (i.e. keep-alives)
     *
     * NOTE: Runs in the context of the task that is executing zl303xx_Dpll77xFlashBurn
     *
     * See reportProgressFnArg.
     */
    void (*reportProgressFn)(void *arg, Uint8T progressPercent, const char *progressStr);

    /* Optional. User argument to pass to the reportProgressFn. */
    void *reportProgressFnArg;

    /* Optional. Cancellation check callback function.
     *
     * If non-NULL, this function will be called at cancellation points of process
     * to check for user requested cancellation. If this function returns ZL303XX_TRUE
     * then operation will be cancelled. Return ZL303XX_FALSE to continue progress.
     *
     * Note: Cancelled progress may result in undefined device state. A reset and
     *       successful completion of procedure may be required to return device
     *       to operation.
     *
     * Note: Runs in the context of the task that is executing zl303xx_Dpll77xFlashBurn
     *
     * See cancelCheckFnArg.
     */
    zl303xx_BooleanE (*cancelCheckFn)(void *arg);

    /* Optional. User argument to pass to the cancelCheckFn. */
    void *cancelCheckFnArg;

    void *board_instance;

} zl303xx_Dpll77xFlashBurnArgsT;


/*****************   EXPORTED GLOBAL VARIABLE DECLARATIONS   ******************/

/*****************   EXTERNAL FUNCTION DECLARATIONS   *************************/

zlStatusE zl303xx_Dpll77xFlashDumpDebug(void *hwParams);

zlStatusE zl303xx_Dpll77xFlashBurnStructInit(zl303xx_Dpll77xFlashBurnArgsT *pArgs);
zlStatusE zl303xx_Dpll77xFlashBurn(void *hwParams, zl303xx_Dpll77xFlashBurnArgsT *pArgs);


#ifdef __cplusplus
}
#endif

#endif /* MULTIPLE INCLUDE BARRIER */
