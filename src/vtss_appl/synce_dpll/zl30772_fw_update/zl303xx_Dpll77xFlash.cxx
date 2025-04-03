

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

/*****************   INCLUDE FILES   ******************************************/

#pragma GCC diagnostic ignored "-Wformat-extra-args"
#pragma GCC diagnostic ignored "-Wunused-const-variable"

#include "zl303xx_DeviceSpec.h"             // This file is included to get the definition of zl303xx_ParamsS

/* Include minimal porting set */
#include "zl303xx_FlashPorting.h"


#include "zl303xx_Dpll77xFlash.h"

/* For file I/O */
#include <stdio.h>

/* For strlen */
#include <string.h>



/*****************   DEFINES     **********************************************/

/* Timeouts for the process */
#define TIMEOUT_FLASH_PHASE1_SEC (1*60)   /* 1 minute */
#define TIMEOUT_FLASH_PHASE2_SEC (2*60)   /* 2 minutes */
#define POLL_DELAY_FLASH_CMD_MSEC 100     /* 100 milliseconds */

/* Various Delays (in milliseconds) */
#define CPU_SLEEP_DELAY_MS 10             /* 10 milliseconds to yield CPU */
#define HWREG_TIMEOUT_DELAY_MS 1000
#define HWREG_POLL_DELAY_MS    1

/* Special progressPercent for keep-alive report, see ReportProgress() */
#define REPORT_PROGRESS_KEEPALIVE 255
/* How many seconds of long running job elapse before reporting the keepalive progress */
#define REPORT_PROGRESS_KEEPALIVE_PERIOD_SEC 5

#define HostRegister_getHwAddr(offset) (0x7F80 + offset)



static const Uint32T addrFirmwareImage1 = 0x20000000;
static const Uint32T addrUtility   = 0x20000000;
static const Uint32T addrUtilityPlus4 = 0x20000004;
static const Uint32T maxDataSizeUtility = 0x00002400;
static const Uint32T maxImageSizeInRam = 0x00026000;
static const Uint32T addrReplacePage = 0x20000000;

/* In bytes */
static const Uint32T maxDataSizeFirmwareImage1 = 0x28000;
static const Uint32T maxDataSizeFirmwareImage2 = 0x00000040;
static const Uint32T maxDataSizeFirmwareImage2Minus4 = 0x0000003C;
static const Uint32T maxDataSizeFirmwareImage2CRC = 0x0000003C;
static const Uint32T maxDataSizeConfigFile = 0x00001000;

static const Uint32T pageIndexFirmwareImage2 = 0x000003E0;
static const Uint32T pageIndexFirmwareImage2Copy = 0x000002E0;
static const Uint32T pageIndexConfigFiles[ZL303XX_DPLL_77X_FLASH_CONFIG_COUNT] = {
    0x00000300,
    0x00000310,
    0x00000320,
};

static const Uint32T HOST_REG_VERSION = 0x007C;
static const Uint32T HOST_REG_FAMILY = 0x007D;
static const Uint32T HOST_REG_HOST_CONTROL_ENABLE = 0x0082;
static const Uint32T HOST_REG_IMAGE_START_ADDR = 0x0084;
static const Uint32T HOST_REG_IMAGE_SIZE = 0x0088;
static const Uint32T HOST_REG_FLASH_INDEX_READ = 0x008c;
static const Uint32T HOST_REG_FLASH_INDEX_WRITE = 0x0090;
static const Uint32T HOST_REG_FILL_PATTERN = 0x0094;
static const Uint32T HOST_REG_WRITE_FLASH = 0x0098;
static const Uint32T HOST_REG_MAX_SIZE = 0x0088;
static const Uint32T HOST_REG_CHECK_FLASH = 0x0099;
static const Uint32T HOST_REG_ERROR_COUNT = 0x0104;
static const Uint32T HOST_REG_ERROR_CAUSE = 0x0108;
static const Uint32T HOST_REG_OP_STATE = 0x0114;
static const Uint32T HOST_REG_FLASH_INFO = 0x0100;
static const Uint32T FLASH_PAGE_SIZE = 0x100;
static const Uint32T FLASH_INFO_A = 0;
static const Uint32T FLASH_INFO_B = 1;
static const Uint32T FLASH_A_SECTOR_SIZE = 0x1000;
static const Uint32T FLASH_B_SECTOR_SIZE = 0x10000;

/* Operating states */
typedef enum {
    OPERATION_NO_COMMAND              = 0x00,
    OPERATION_PENDING                 = 0x01,
    OPERATION_DONE                    = 0x02,
    OPERATION_MAX_INVALID
} OpStateE;



/*****************   STATIC GLOBAL VARIABLES   ********************************/

/*
   WARNING functions in file use global variables shared by other functions in
   this file. Functions should not be called from parallel contexts (tasks).
   i.e. Use functions in a single task.
*/

/* Used for register driver access */
static void *gHwParams = NULL;

static volatile Uint32T gMode = 0x20;
/* A global status indicator for device access issues */
static volatile zlStatusE gAccessStatus = ZL303XX_OK;

/* Used for reporting */
static volatile zl303xx_Dpll77xFlashBurnArgsT *gFlashArgs = NULL;

/*****************   IMPORTED GLOBAL VARIABLES   ******************************/

/*****************   EXPORTED GLOBAL VARIABLES   ******************************/

/*****************   EXPORTED FUNCTION DEFINITIONS   **************************/

/*****************   STATIC FUNCTION DECLARATIONS   ***************************/

/* Utility functions */
static Uint32T buildMask32(Uint8T msbIndex, Uint8T lsbIndex);

/* Internal HostRegister interfaces */
static void HostRegister_write(Uint32T regAddr, Uint8T regSize, Uint32T val);
static Uint32T HostRegister_read(Uint32T regAddr, Uint8T regSize);
static void HostRegister_writeBits(Uint32T regAddr, Uint8T regSize, Uint32T val, Uint8T msbIndex, Uint8T lsbIndex);
static Uint32T HostRegister_readBits(Uint32T regAddr, Uint8T regSize, Uint8T msbIndex, Uint8T lsbIndex);

/* Internal HwRegister interfaces */
static Uint8T HwRegister_waitForReady(Sint32T timeoutMs, Uint32T val);
static void HwRegister_writeWord(Uint32T regAddr, Uint32T val);
static Uint32T HwRegister_readWord(Uint32T regAddr);
static void HwRegister_writeBits(Uint32T regAddr, Uint32T val, Uint32T msbIndex, Uint32T lsbIndex);

/* Internal Procedures */
static Sint32T loadDataIntoMemory(const Uint32T *words, Uint32T numWords, Uint32T iStartByteInFile, Uint32T iStartAddressInMemory, Uint32T iMaxByteSize);
static zl303xx_BooleanE startUtility(const Uint32T *words, Uint32T numWords);
static zl303xx_BooleanE waitFlashCmdReady(Uint32T cmdReg, Uint32T timeoutSec);
static Uint32T getErrorCount(Uint32T *pErrorCause);
static OpStateE getOperationState(void);
static zl303xx_BooleanE sendFlashCmdAndWait(Uint32T cmdReg, Uint32T cmd);
static zl303xx_BooleanE replaceFlashPage(Uint32T iAddress, Uint32T iSize, Uint32T iStartPageIndex);
static zl303xx_BooleanE copyFlashPage(Uint32T iSourcePageIndex, Uint32T iDestinationPageIndex);
static zl303xx_BooleanE burnFlashSectors(Uint32T iFlashStartAddress, Uint32T iNumBytes, Uint32T iStartPageIndex);
static zl303xx_BooleanE integrityCheck(zl303xx_Dpll77xFlashIntegrityE index, zl303xx_BooleanE bExpectPass);
static Uint32T readFlashSectorSize(void);
static zl303xx_BooleanE burnFirmwareImage(Uint32T *words, Uint32T numWords);



/* File I/O with host memory */
static Uint32T loadFileToHostMemory(const char *filePath, Uint32T *words, Uint32T wordMax);

/* Reporting / cancellation wrappers */
static void ReportProgress(Uint8T progressPercent, const char *progressStr);
static zlStatusE CancelCheck(zlStatusE status);

/*****************    GLOBAL VARIABLES   ********************************/

/*****************   IMPORTED GLOBAL VARIABLES   ******************************/

/*****************   FWD FUNCTION DECLARATIONS   ******************************/

/*****************   EXPORTED FUNCTION DEFINITIONS   **************************/


/** Utility function to build 32-bit mask of ones  between msbIndex (max 31)
   and lsbIndex (min 0) inclusive. Note caller must ensure msbIndex >= lsbIndex.

   e.g. (3, 1) -> 0b1110 = 0xE
*/
Uint32T buildMask32(Uint8T msbIndex, Uint8T lsbIndex)
{
    Uint32T mask = 0;
    Uint8T i;

    for (i = 0; i < 32; i++) {
        if (i >= lsbIndex && i <= msbIndex) {
            mask |= ((Uint32T)1 << i);

            if (i == msbIndex) {
                break;
            }
        } else if (i > msbIndex) {
            break;
        }
    }

    return mask;
}

/* Access the device for write operation.

Sets global gAccessStatus != ZL303XX_OK if error.
*/
void HostRegister_write(Uint32T regAddr, Uint8T regSize, Uint32T val)
{
    ZL303XX_TRACE(ZL303XX_MOD_ID_RDWR, 3,
                  "HostRegister_write: regAddr 0x%x, regSize 0x%x, val 0x%x",
                  regAddr, regSize, val);

    if (gFlashArgs != NULL && gFlashArgs->writeFn != NULL) {
        /* Decode register address into physical page and offset */
        Uint16T page = regAddr >> 7;     /* upper bits */
        Uint16T offset = regAddr & 0x7F; /* lower 7-bits */

        gAccessStatus = (zlStatusE)gFlashArgs->writeFn(gHwParams, gFlashArgs->readWriteFnArg,
                                                       regAddr, page, offset, regSize, val);
    } else {
        /* Error. User did not provide writeFn */
        gAccessStatus = ZL303XX_INVALID_OPERATION;

        ZL303XX_TRACE_ALWAYS("HostRegister_write: CRITICAL no writeFn defined... cannot continue");
    }

    if (gAccessStatus != ZL303XX_OK) {
        ZL303XX_TRACE_ALWAYS("HostRegister_write: ERROR regAddr 0x%x, regSize 0x%x, val 0x%x (status %d)",
                             regAddr, regSize, val, gAccessStatus);
    }
}

/* Access the device for read operation

Sets global gAccessStatus != ZL303XX_OK if error.
*/
Uint32T HostRegister_read(Uint32T regAddr, Uint8T regSize)
{
    Uint32T val = 0;

    if (gFlashArgs != NULL && gFlashArgs->readFn != NULL) {
        /* Decode register address into physical page and offset */
        Uint16T page = regAddr >> 7;     /* upper bits */
        Uint16T offset = regAddr & 0x7F; /* lower 7-bits */

        gAccessStatus = (zlStatusE)gFlashArgs->readFn(gHwParams, gFlashArgs->readWriteFnArg,
                                                      regAddr, page, offset, regSize, &val);
    } else {
        /* Error. User did not provide readFn */
        gAccessStatus = ZL303XX_INVALID_OPERATION;

        ZL303XX_TRACE_ALWAYS("HostRegister_read: CRITICAL no readFn defined... cannot continue");
    }

    if (gAccessStatus != ZL303XX_OK) {
        ZL303XX_TRACE_ALWAYS("HostRegister_read: ERROR regAddr 0x%x, regSize 0x%x, val 0x%x (status %d)",
                             regAddr, regSize, val, gAccessStatus);
    } else {
        ZL303XX_TRACE(ZL303XX_MOD_ID_RDWR, 3,
                      "HostRegister_read: regAddr 0x%x, regSize 0x%x, val 0x%x",
                      regAddr, regSize, val);
    }

    return val;
}

void HostRegister_writeBits(Uint32T regAddr, Uint8T regSize, Uint32T val, Uint8T msbIndex, Uint8T lsbIndex)
{
    Uint32T tmp;

    /* Do read-mod-write (write masked section) */
    tmp = HostRegister_read(regAddr, regSize);
    if (gAccessStatus == ZL303XX_OK) {
        tmp &= ~buildMask32(msbIndex, lsbIndex);     /* AND with complemented mask (clear bits) */
        tmp |= (val << lsbIndex);                    /* Note: Assumes val fits in mask (not enforced) */
        HostRegister_write(regAddr, regSize, tmp);
    }

    ZL303XX_TRACE(ZL303XX_MOD_ID_RDWR, 3,
                  "HostRegister_writeBits: regAddr 0x%x, regSize 0x%x, msbIndex %u, lsbIndex %u, val 0x%x, tmp 0x%x (done)",
                  regAddr, regSize, msbIndex, lsbIndex, val, tmp);
}

Uint32T HostRegister_readBits(Uint32T regAddr, Uint8T regSize, Uint8T msbIndex, Uint8T lsbIndex)
{
    Uint32T val;
    Uint32T tmp;

    /* Do read-mod (extract masked section) */
    val = HostRegister_read(regAddr, regSize);
    tmp = val & buildMask32(msbIndex, lsbIndex);
    tmp >>= lsbIndex;

    ZL303XX_TRACE(ZL303XX_MOD_ID_RDWR, 3,
                  "HostRegister_readBits: regAddr 0x%x, regSize 0x%x, msbIndex %u, lsbIndex %u, val 0x%x, tmp 0x%x (done)",
                  regAddr, regSize, msbIndex, lsbIndex, val, tmp);

    return tmp;
}

/** Returns 0 when ready or 1 if timeout */
Uint8T HwRegister_waitForReady(Sint32T timeoutMs, Uint32T val)
{
    Uint32T ready;
    Uint32T expect = (val & ~(0x2)); /* bit 1 */

    /* Poll for completion */
    do {
        ready = HostRegister_read(HostRegister_getHwAddr(0x0), 1);
        if (ready != expect) {
            OS_TASK_DELAY (HWREG_POLL_DELAY_MS);
            timeoutMs -= HWREG_POLL_DELAY_MS;
        }
    } while ((ready != expect) && (timeoutMs > 0));

    if (ready != expect) {
        return 1; /* Not ready (timeout) */
    } else {
        return 0; /* Ready */
    }
}

void HwRegister_writeWord(Uint32T regAddr, Uint32T val)
{
    ZL303XX_TRACE(ZL303XX_MOD_ID_RDWR, 3,
                  "HwRegister_writeWord: regAddr 0x%x, val 0x%x, gMode 0x%x",
                  regAddr, val, gMode);

    HostRegister_write(HostRegister_getHwAddr(0x4), 4, regAddr & ~3);

    if (gAccessStatus != ZL303XX_OK) {
        return;
    }
    HostRegister_write(HostRegister_getHwAddr(0x8), 4, val);

    if (gAccessStatus != ZL303XX_OK) {
        return;
    }
    HostRegister_write(HostRegister_getHwAddr(0x0), 1, gMode | 0xA);

    if (gAccessStatus != ZL303XX_OK) {
        return;
    }

    if (HwRegister_waitForReady(HWREG_TIMEOUT_DELAY_MS, gMode | 0xA) > 0) {
        printf("Timeout during HwRegister_writeWord");
        ZL303XX_TRACE(ZL303XX_MOD_ID_RDWR, 1,
                      "HwRegister_writeWord: regAddr 0x%x, val 0x%x (TIMEOUT after %u msec)",
                      regAddr, val, HWREG_TIMEOUT_DELAY_MS);

        gAccessStatus = ZL303XX_TIMEOUT;
    }
}

Uint32T HwRegister_readWord(Uint32T regAddr)
{
    Uint32T val = 0;

    ZL303XX_TRACE(ZL303XX_MOD_ID_RDWR, 3,
                  "HwRegister_readWord: regAddr 0x%x request gMode 0x%x",
                  regAddr, gMode);

    HostRegister_write(HostRegister_getHwAddr(0x4), 4, regAddr & ~3);

    if (gAccessStatus != ZL303XX_OK) {
        return val;
    }
    HostRegister_write(HostRegister_getHwAddr(0x0), 1, gMode | 0xB);

    if (gAccessStatus != ZL303XX_OK) {
        return val;
    }

    if (HwRegister_waitForReady(HWREG_TIMEOUT_DELAY_MS, gMode | 0xB) > 0) {
        printf("Timeout during HwRegister_readWord");
        ZL303XX_TRACE(ZL303XX_MOD_ID_RDWR, 1,
                      "HwRegister_readWord: regAddr 0x%x (TIMEOUT after %u msec)",
                      regAddr, HWREG_TIMEOUT_DELAY_MS);

        gAccessStatus = ZL303XX_TIMEOUT;
    } else {
        /* Ready, read word */
        val = HostRegister_read(HostRegister_getHwAddr(0xC), 4);

        ZL303XX_TRACE(ZL303XX_MOD_ID_RDWR, 3,
                      "HwRegister_readWord: regAddr 0x%x, val 0x%x (done)",
                      regAddr, val);
    }


    return val;
}

void HwRegister_writeBits(Uint32T regAddr, Uint32T val, Uint32T msbIndex, Uint32T lsbIndex)
{
    Uint32T tmp;

    /* Do read-mod-write (write masked section) */
    tmp = HwRegister_readWord(regAddr);

    if (gAccessStatus == ZL303XX_OK) {
        tmp &= ~buildMask32(msbIndex, lsbIndex); /* AND with complemented mask (clear bits) */
        tmp |= (val << lsbIndex);
        HwRegister_writeWord(regAddr, tmp);
    }

    ZL303XX_TRACE(ZL303XX_MOD_ID_RDWR, 3,
                  "HwRegister_writeBits: regAddr 0x%x, val 0x%x, msbIndex %u, lsbIndex %u, tmp 0x%x (done)",
                  regAddr, val, msbIndex, lsbIndex, tmp);
}


/**
* Loads data from given host memory into device RAM.
*
* words: Pointer to first word in file.
* numWords: Number of words in file.
* iStartByteInFile: Byte offset in given file to start loading from (non-zero if loading in multiple chunks).
* iStartAddressInMemory: Location in device RAM to load into.
* iMaxByteSize: Max possible Size in device RAM.
*
* Return value is the number of bytes that are successfully loaded into device memory or zero or negative for error.
*/
Sint32T loadDataIntoMemory(const Uint32T *words, Uint32T numWords, Uint32T iStartByteInFile, Uint32T iStartAddressInMemory, Uint32T iMaxByteSize)
{
    Uint32T index;
    Uint32T lMemoryAddress = iStartAddressInMemory & ~3; /* 32-bit boundary */
    Uint32T tmpCount = 0;

    Uint32T cpuStartTick = OS_TICK_GET();
    Uint32T cpuTicksPerSec = OS_TICK_RATE_GET();
    Uint32T keepAliveSec = 0;

    ZL303XX_TRACE(ZL303XX_MOD_ID_PLL, 2,
                  "loadDataIntoMemory: %u %u 0x%x %u",
                  numWords, iStartByteInFile, iStartAddressInMemory, iMaxByteSize);

    /* Check if the start address is bigger than the file size */
    if (iStartByteInFile < numWords * 4) {
        /* If Data file is too large, just load the MaxByteSize instead of the entire file */
        if ((numWords * 4) > (iStartByteInFile + iMaxByteSize)) {
            ZL303XX_TRACE(ZL303XX_MOD_ID_PLL, 3,
                          "loadDataIntoMemory: 0x%x %u %u 0x%x data file size reduced",
                          numWords, iStartByteInFile, iStartAddressInMemory, iMaxByteSize);

            numWords = (iStartByteInFile + iMaxByteSize) / 4;
        }

        for (index = iStartByteInFile / 4; index < numWords; index++) {
            Uint32T elapsedSec = ((OS_TICK_GET() - cpuStartTick) / cpuTicksPerSec);

            HwRegister_writeWord(lMemoryAddress, words[index]);
            lMemoryAddress += 4;

            if (gAccessStatus != ZL303XX_OK) {
                /* Return error if access failed */
                ZL303XX_TRACE(ZL303XX_MOD_ID_PLL, 1,
                              "loadDataIntoMemory: %u %u 0x%x %u ERROR returned from driver %d, stopping",
                              numWords, iStartByteInFile, iStartAddressInMemory, iMaxByteSize, gAccessStatus);
                return 0;
            }

            /* Check keepalive */
            if (elapsedSec - keepAliveSec >= REPORT_PROGRESS_KEEPALIVE_PERIOD_SEC) {
                /* More than one sec elapsed since last keep alive */
                keepAliveSec = elapsedSec;

                /* Send special "Keep-alive" update */
                ReportProgress(REPORT_PROGRESS_KEEPALIVE, "loadDataIntoMemory");

                /* Do cancellation check */
                if (CancelCheck(ZL303XX_OK) != ZL303XX_OK) {
                    ZL303XX_TRACE(ZL303XX_MOD_ID_PLL, 1,
                                  "loadDataIntoMemory: Cancelled %u %u 0x%x %u",
                                  numWords, iStartByteInFile, iStartAddressInMemory, iMaxByteSize);

                    return 0; /* Cancelled, return error */
                }
            }

            /* Sleep every 1K bytes to not consume host CPU */
            tmpCount += 4;
            if (tmpCount >= 1024) {
                tmpCount = 0;        /* Reset counter */

                OS_TASK_DELAY(CPU_SLEEP_DELAY_MS);   /* Sleep few msec to let other tasks run */
            }
        }

        /* Return number of bytes loaded */
        return (4 * numWords) - iStartByteInFile;
    } else {
        return 0;
    }
}


/** Returns ZL303XX_FALSE if fails */
zl303xx_BooleanE startUtility(const Uint32T *words, Uint32T numWords)
{
    const Uint32T A = 0x80000400;
    const Uint32T B = 0;
    const Uint32T C = 0x10000000;
    const Uint32T D = 2;
    const Uint32T E = 9;
    const Uint32T F = 0x10000020;
    const Uint32T G = 0;
    const Uint32T H = 0x10400010;
    const Uint32T I = 0x10400014;
    const Uint32T J = 0x10400004;
    const Uint32T K = 0x10400008;

    ZL303XX_TRACE(ZL303XX_MOD_ID_PLL, 2,
                  "startUtility: %u",
                  numWords);

    ReportProgress(11, "Start Utility A");

    HwRegister_writeBits(A, 1, B, B);
    if (gAccessStatus != ZL303XX_OK) {
        return ZL303XX_FALSE;
    }
    OS_TASK_DELAY(1000);

    HwRegister_writeBits(C, 1, D, D);
    if (gAccessStatus != ZL303XX_OK) {
        return ZL303XX_FALSE;
    }
    OS_TASK_DELAY(1000);

    HwRegister_writeBits(F, 1, G, G);
    if (gAccessStatus != ZL303XX_OK) {
        return ZL303XX_FALSE;
    }
    OS_TASK_DELAY(1000);

    ReportProgress(12, "Start Utility B");

    if (loadDataIntoMemory(words,
                           numWords,
                           0,
                           addrUtility,
                           maxDataSizeUtility) > 0) {
        ReportProgress(15, "Start Utility C");


        HwRegister_writeWord(J, 0x000000C0);
        HwRegister_writeWord(K, 0x00000000);
        HwRegister_writeWord(H, addrUtility);
        HwRegister_writeWord(I, addrUtilityPlus4);
        if (gAccessStatus != ZL303XX_OK) {
            return ZL303XX_FALSE;
        }
        OS_TASK_DELAY(1000);

        HwRegister_writeBits(C, 1, E, E);
        if (gAccessStatus != ZL303XX_OK) {
            return ZL303XX_FALSE;
        }
        OS_TASK_DELAY(1000);

        HwRegister_writeBits(F, 0, G, G);
        if (gAccessStatus != ZL303XX_OK) {
            return ZL303XX_FALSE;
        }
        OS_TASK_DELAY(1000);

        HwRegister_writeBits(A, 0, B, B);
        if (gAccessStatus != ZL303XX_OK) {
            return ZL303XX_FALSE;
        }
        OS_TASK_DELAY(1000);

        return ZL303XX_TRUE;
    } else {
        return ZL303XX_FALSE;
    }
}

/** Blocks caller until ready or given timeoutSec expires */
zl303xx_BooleanE waitFlashCmdReady(Uint32T cmdReg, Uint32T timeoutSec)
{
    const Uint32T cpuStartTick = OS_TICK_GET();
    const Uint32T cpuTicksPerSec = OS_TICK_RATE_GET();
    Uint32T keepAliveSec = 0;

    ZL303XX_TRACE(ZL303XX_MOD_ID_PLL, 2,
                  "waitFlashCmdReady: 0x%x %u",
                  cmdReg, timeoutSec);

    while (gAccessStatus == ZL303XX_OK && HostRegister_readBits(cmdReg, 1, 2, 0) != 0x000) {
        /* Expire after timeout */
        Uint32T elapsedSec = ((OS_TICK_GET() - cpuStartTick) / cpuTicksPerSec);

        ZL303XX_TRACE(ZL303XX_MOD_ID_PLL, 3,
                      "waitFlashCmdReady: 0x%x %u (elapsed %u sec)",
                      cmdReg, timeoutSec, elapsedSec);

        if (elapsedSec > timeoutSec) {
            return ZL303XX_FALSE; /* Timeout, return error */
        }

        if (elapsedSec - keepAliveSec >= REPORT_PROGRESS_KEEPALIVE_PERIOD_SEC) {
            /* More than one sec elapsed since last keep alive */
            keepAliveSec = elapsedSec;

            /* Send special "Keep-alive" update */
            ReportProgress(REPORT_PROGRESS_KEEPALIVE, "waitFlashCmdReady");

            /* Do cancellation check */
            if (CancelCheck(ZL303XX_OK) != ZL303XX_OK) {
                ZL303XX_TRACE(ZL303XX_MOD_ID_PLL, 2,
                              "waitFlashCmdReady: Cancelled 0x%x %u",
                              cmdReg, timeoutSec);

                return ZL303XX_FALSE; /* Cancelled, return error */
            }
        }

        OS_TASK_DELAY(POLL_DELAY_FLASH_CMD_MSEC);
    }

    if (gAccessStatus != ZL303XX_OK) {
        return ZL303XX_FALSE;
    }

    return ZL303XX_TRUE;
}


/** Returns 0 for no errors. Optionally provides error cause (if pErrorCause not NULL) */
Uint32T getErrorCount(Uint32T *pErrorCause)
{
    Uint32T errorCount = HostRegister_read(HOST_REG_ERROR_COUNT, 4);
    Uint32T errorCause = HostRegister_read(HOST_REG_ERROR_CAUSE, 4);

    ZL303XX_TRACE(ZL303XX_MOD_ID_PLL, 3,
                  "getErrorCount: 0x%x 0x%x",
                  errorCount, errorCause);

    if (pErrorCause != NULL) {
        /* OR the cause to simplifier caller accumulation of causes */
        *pErrorCause |= errorCause;
    }

    return errorCount;
}

/** Returns the operation state */
OpStateE getOperationState(void)
{
    Uint32T opState = HostRegister_read(HOST_REG_OP_STATE, 1);

    ZL303XX_TRACE(ZL303XX_MOD_ID_PLL, 3,
                  "getOperationDone: 0x%x",
                  opState);

    if (opState >= OPERATION_MAX_INVALID) {
        /* Invalid state, possible bus access issues */
        ZL303XX_TRACE(ZL303XX_MOD_ID_PLL, 1,
                      "getOperationDone: 0x%x INVALID check bus access",
                      opState);

        return OPERATION_MAX_INVALID;
    } else {
        return (OpStateE)opState;
    }
}

/** Returns ZL303XX_FALSE if fails */
zl303xx_BooleanE sendFlashCmdAndWait(Uint32T cmdReg, Uint32T cmd)
{
    Uint32T errorCause = 0;
    Uint32T errorCount = 0;
    OpStateE opState = OPERATION_MAX_INVALID;

    ZL303XX_TRACE(ZL303XX_MOD_ID_PLL, 2,
                  "sendFlashCmdAndWait: 0x%x 0x%x",
                  cmdReg, cmd);

    /* Wait for access */
    if (gAccessStatus != ZL303XX_OK || waitFlashCmdReady(cmdReg, TIMEOUT_FLASH_PHASE1_SEC) != ZL303XX_TRUE) {     /* 1 mins */
        return ZL303XX_FALSE;
    }

    /* Issue command */
    HostRegister_writeBits(cmdReg, 1, cmd, 2, 0);

    ZL303XX_TRACE(ZL303XX_MOD_ID_PLL, 2,
                  "sendFlashCmdAndWait read cmdReg 0x%x cmd 0x%x val 0x%x",
                  cmdReg, cmd, HostRegister_read(cmdReg, 1));

    /* Wait for completion */
    if (gAccessStatus != ZL303XX_OK || waitFlashCmdReady(cmdReg, TIMEOUT_FLASH_PHASE2_SEC) != ZL303XX_TRUE) {     /* 2 mins */
        /* Access error or Timeout */
        ZL303XX_TRACE(ZL303XX_MOD_ID_PLL, 1,
                      "sendFlashCmdAndWait FAILURE ... cmdReg 0x%x, cmd 0x%x, status %d not ok or timeout",
                      cmdReg, cmd, gAccessStatus);

        return ZL303XX_FALSE;
    }

    /* Check result */
    if (gAccessStatus != ZL303XX_OK || (opState = getOperationState()) != OPERATION_DONE) {
        /* Access error or operation incomplete */
        ZL303XX_TRACE(ZL303XX_MOD_ID_PLL, 1,
                      "sendFlashCmdAndWait FAILURE ... cmdReg 0x%x, cmd 0x%x, status %d, opState %d",
                      cmdReg, cmd, gAccessStatus, opState);

        return ZL303XX_FALSE;
    } else if ((errorCount = getErrorCount(&errorCause)) > 0) {
        /* Failed if errorCount is non-zero */
        /* Note: some commands may expect error and so this trace is not critical */
        ZL303XX_TRACE(ZL303XX_MOD_ID_PLL, 2,
                      "sendFlashCmdAndWait ERROR ... cmdReg 0x%x, cmd 0x%x, status %d, opState %d, errorCount %d, errorCause 0x%02x",
                      cmdReg, cmd, gAccessStatus, opState, errorCount, errorCause);

        return ZL303XX_FALSE;
    }

    return ZL303XX_TRUE;
}

/** Returns ZL303XX_FALSE if fails */
zl303xx_BooleanE replaceFlashPage(Uint32T iAddress, Uint32T iSize, Uint32T iStartPageIndex)
{
    ZL303XX_TRACE(ZL303XX_MOD_ID_PLL, 2,
                  "replaceFlashPage: 0x%x 0x%x 0x%x",
                  iAddress, iSize, iStartPageIndex);

    HostRegister_write(HOST_REG_IMAGE_START_ADDR, 4, iAddress);
    HostRegister_write(HOST_REG_IMAGE_SIZE, 4, iSize);
    HostRegister_write(HOST_REG_FLASH_INDEX_WRITE, 4, iStartPageIndex);
    HostRegister_write(HOST_REG_FILL_PATTERN, 4, 0xFFFFFFFF);

    if (gAccessStatus != ZL303XX_OK || sendFlashCmdAndWait(HOST_REG_WRITE_FLASH, 0x3) != ZL303XX_TRUE) {
        ZL303XX_TRACE(ZL303XX_MOD_ID_PLL, 1,
                      "replaceFlashPage: Error happened during the page replace (status %d)",
                      gAccessStatus);
        return ZL303XX_FALSE;
    }

    return ZL303XX_TRUE;
}


/** Returns ZL303XX_FALSE if fails */
zl303xx_BooleanE copyFlashPage(Uint32T iSourcePageIndex, Uint32T iDestinationPageIndex)
{
    ZL303XX_TRACE(ZL303XX_MOD_ID_PLL, 2,
                  "copyFlashPage: 0x%x 0x%x",
                  iSourcePageIndex, iDestinationPageIndex);

    if (waitFlashCmdReady(HOST_REG_WRITE_FLASH, TIMEOUT_FLASH_PHASE1_SEC) != ZL303XX_TRUE) {     /* 1 mins */
        return ZL303XX_FALSE;
    }

    HostRegister_write(HOST_REG_FLASH_INDEX_READ, 4, iSourcePageIndex);
    HostRegister_write(HOST_REG_FLASH_INDEX_WRITE, 4, iDestinationPageIndex);

    if (gAccessStatus != ZL303XX_OK || sendFlashCmdAndWait(HOST_REG_WRITE_FLASH, 0x4) != ZL303XX_TRUE) {
        ZL303XX_TRACE_ALWAYS("copyFlashPage: Error happened during the page copy (index %d, status %d)", iSourcePageIndex, gAccessStatus);
        return ZL303XX_FALSE;
    }

    return ZL303XX_TRUE;
}

/** Returns ZL303XX_FALSE if fails */
zl303xx_BooleanE burnFlashSectors(Uint32T iFlashStartAddress, Uint32T iNumBytes, Uint32T iStartPageIndex)
{
    ZL303XX_TRACE(ZL303XX_MOD_ID_PLL, 2,
                  "burnFlashSectors: 0x%x 0x%x 0x%x",
                  iFlashStartAddress, iNumBytes, iStartPageIndex);

    HostRegister_write(HOST_REG_IMAGE_START_ADDR, 4, iFlashStartAddress);
    if (gAccessStatus == ZL303XX_OK) {
        HostRegister_write(HOST_REG_MAX_SIZE, 4, iNumBytes);
    }

    if (gAccessStatus == ZL303XX_OK) {
        HostRegister_write(HOST_REG_FLASH_INDEX_WRITE, 4, iStartPageIndex);
    }

    if (gAccessStatus == ZL303XX_OK) {
        HostRegister_write(HOST_REG_FILL_PATTERN, 4, 0xFFFFFFFF);
    }

    if (gAccessStatus != ZL303XX_OK || sendFlashCmdAndWait(HOST_REG_WRITE_FLASH, 0x2) != ZL303XX_TRUE) {
        ZL303XX_TRACE_ALWAYS("burnFlashSectors: Error happened during the burn (index %d, status %d)", iStartPageIndex, gAccessStatus);
        return ZL303XX_FALSE;
    }

    return ZL303XX_TRUE;
}


/** Returns ZL303XX_FALSE if given integrety check `index` fails expectation `bExpectPass` where
 `bExpectPass` should be ZL303XX_TRUE for expected passing integrity check and ZL303XX_FALSE if
 expecting a failed integrity check result (e.g. for empty configurations) .
*/
zl303xx_BooleanE integrityCheck(zl303xx_Dpll77xFlashIntegrityE index, zl303xx_BooleanE bExpectPass)
{
    ZL303XX_TRACE(ZL303XX_MOD_ID_PLL, 2,
                  "integrityCheck: 0x%x %u",
                  index, bExpectPass);

    if (waitFlashCmdReady(HOST_REG_CHECK_FLASH, TIMEOUT_FLASH_PHASE1_SEC) != ZL303XX_TRUE) {     /* 1 mins */
        ZL303XX_TRACE_ALWAYS("integrityCheck: Timeout in integrity check (index %d)", index);
        return ZL303XX_FALSE;
    }

    HostRegister_write(HOST_REG_IMAGE_SIZE, 4, index);

    if (gAccessStatus != ZL303XX_OK || sendFlashCmdAndWait(HOST_REG_CHECK_FLASH, 0x6) != bExpectPass) {
        ZL303XX_TRACE(ZL303XX_MOD_ID_PLL, 1,
                      "integrityCheck: FAILURE happened during the integrity check (index %d, status %d not ok or expected %u not returned)",
                      index, gAccessStatus, bExpectPass);
        return ZL303XX_FALSE;
    }

    return ZL303XX_TRUE;
}

/** Returns 0 if error */
Uint32T readFlashSectorSize(void)
{
    Uint32T flashInfo = HostRegister_read(HOST_REG_FLASH_INFO, 1) & 0x0F;
    Uint32T sectorSize;

    if (gAccessStatus == ZL303XX_OK && flashInfo == FLASH_INFO_A) {
        sectorSize = FLASH_A_SECTOR_SIZE;
    } else if (gAccessStatus == ZL303XX_OK && flashInfo == FLASH_INFO_B) {
        sectorSize = FLASH_B_SECTOR_SIZE;
    } else {
        sectorSize = 0;   /* Error */
    }

    ZL303XX_TRACE(ZL303XX_MOD_ID_PLL, 2,
                  "readFlashSectorSize: 0x%x 0x%x (status %d)",
                  flashInfo, sectorSize, gAccessStatus);

    return sectorSize;
}

/** Splits given file and burns image */
zl303xx_BooleanE burnFirmwareImage(Uint32T *words, Uint32T numWords)
{
    Uint32T flashSectorSize = readFlashSectorSize();
    Uint32T lMaxDataSizePerWrite = maxImageSizeInRam;
    Uint32T lStartByteInFile = 0;

    ZL303XX_TRACE(ZL303XX_MOD_ID_PLL, 2,
                  "burnFirmwareImage: %p %u 0x%x",
                  (void *)words, numWords, flashSectorSize);

    if (flashSectorSize == 0) {
        /* Error reading sector size */
        return ZL303XX_FALSE;
    }

    if (lMaxDataSizePerWrite % flashSectorSize != 0) {
        lMaxDataSizePerWrite = (lMaxDataSizePerWrite / flashSectorSize) * flashSectorSize;
    }

    while (lStartByteInFile < maxDataSizeFirmwareImage1 && lStartByteInFile < (numWords * 4)) {
        Sint32T loadedDataSize = loadDataIntoMemory(words, numWords, lStartByteInFile, addrFirmwareImage1, lMaxDataSizePerWrite);
        if (loadedDataSize > 0) {
            burnFlashSectors(addrFirmwareImage1, loadedDataSize, (lStartByteInFile / FLASH_PAGE_SIZE));
            lStartByteInFile += lMaxDataSizePerWrite;
        } else {
            /* Failed to load */
            return ZL303XX_FALSE;
        }
    }

    return ZL303XX_TRUE;
}


/**
Opens given `filePath` for reading and extracts ASCII hex 32-bit integers from each
line into the `words` array up to `wordMax` words.

Returns the number of words read, or 0 for error.
*/
Uint32T loadFileToHostMemory(const char *filePath, Uint32T *words, Uint32T wordMax)
{
    const char *fnName = "loadFileToHostMemory";
    Uint32T tmpWord;
    Uint32T wordCount = 0;
    Uint32T result;
    FILE *fp;
    char buf[16];        /* Line buffer */

    ZL303XX_TRACE(ZL303XX_MOD_ID_PLL, 2,
                  "%s: \"%s\" %p 0x%x",
                  fnName, filePath, (void *)words, wordMax);

    if (filePath == NULL || words == NULL || wordMax == 0) {
        ZL303XX_TRACE(ZL303XX_MOD_ID_PLL, 1,
                      "%s: Argument error",
                      fnName);

        return 0;
    }

    fp = fopen(filePath, "r");
    if (fp == NULL) {
        ZL303XX_TRACE(ZL303XX_MOD_ID_PLL, 1,
                      "%s: Error opening file \"%s\"",
                      fnName, filePath);

        return 0;
    }

    /* Read each line of file, until end of file */
    while (!feof(fp)) {
        /* Extract single hex integer (32-bit) word from each line */
        if (fgets(buf, sizeof(buf), fp) == NULL) {
            break;
        }

        result = sscanf(buf, "%x", &tmpWord);

        if (result == 1) {
            /* Found hex integer, save it to memory if there is space */
            if (wordCount >= wordMax) {
                /* Error more words in file than available space */
                ZL303XX_TRACE_ALWAYS(
                    "%s: Error reading file \"%s\" count %u reached max %u words",
                    fnName, filePath, wordCount, wordMax);

                wordCount = 0; /* Return error */
                break;
            } else {
                /* OK, save word */
                words[wordCount] = tmpWord;
                wordCount++;
                continue;
            }
        } else if (strlen(buf) == 0) {
            /* Blank line, skip it (usually at end of file) */
            continue;
        } else {
            /* Read issue with file (hex integer not found). Return failure */
            ZL303XX_TRACE_ALWAYS(
                "%s: Error reading file \"%s\" count %u",
                fnName, filePath, wordCount);

            wordCount = 0; /* Return error */
            break;
        }
    }

    fclose(fp);

    if (wordCount != 0) {
        ZL303XX_TRACE(ZL303XX_MOD_ID_PLL, 2,
                      "%s: Read file complete \"%s\" count %u",
                      fnName, filePath, wordCount);
    }

    return wordCount;
}


/** Reports progress intermittently

Special progressPercent value REPORT_PROGRESS_KEEPALIVE is used
as "keep-alive" to indicate ongoing activity will re-send previous reports.
*/
void ReportProgress(Uint8T progressPercent, const char *progressStr)
{
    ZL303XX_TRACE(ZL303XX_MOD_ID_PLL, 2, "ReportProgress %u%%. %s", progressPercent, progressStr);

    if (gFlashArgs != NULL) {
        if (progressPercent == REPORT_PROGRESS_KEEPALIVE) {
            /* Special "keep-alive" update, re-send last report */
        } else {
            /* Update the progress results */
            gFlashArgs->resultProgressPercent = progressPercent;
            gFlashArgs->resultProgressStr = progressStr;
        }

        /* Send report to caller if requested */
        if (gFlashArgs->reportProgressFn != NULL) {
            gFlashArgs->reportProgressFn(gFlashArgs->reportProgressFnArg,
                                         gFlashArgs->resultProgressPercent,
                                         gFlashArgs->resultProgressStr);
        }
    }
}

/*
Check for user cancellation request.

Returns ZL303XX_OK to continue, or ZL303XX_NOT_RUNNING to cancel progress.

Note:
  After cancellation regular functionality is not assured. Chip reset may be required
  to restore operation.
*/
zlStatusE CancelCheck(zlStatusE status)
{
    zl303xx_BooleanE bCancel = ZL303XX_FALSE;

    /* Return existing failure instead of checking for cancel (simplifies caller) */
    if (status != ZL303XX_OK) {
        return status;
    }

    /* Check for cancellation if requested */
    if (gFlashArgs != NULL && gFlashArgs->cancelCheckFn != NULL) {
        bCancel = gFlashArgs->cancelCheckFn(gFlashArgs->cancelCheckFnArg);
    }

    ZL303XX_TRACE(ZL303XX_MOD_ID_PLL, 3, "CancelCheck %u", bCancel);

    return (bCancel == ZL303XX_TRUE) ? ZL303XX_NOT_RUNNING : ZL303XX_OK;
}


/* zl303xx_Dpll77xFlashDumpDebug */
/**

   Dumps data for debugging 77x flash burning operation. Checks the endian-ness
   of multi-byte read is correct.

   Uses ZL303XX_TRACE_ALWAYS.

  Parameters:
   [in]    hwParams  Pointer to hardware parameters (passed to device driver)

\returns     zlStatusE   ZL303XX_OK for success, other for error.

*******************************************************************************/
zlStatusE zl303xx_Dpll77xFlashDumpDebug(void *hwParams)
{
    zlStatusE status = ZL303XX_OK;
    void *old_gHwParams = gHwParams;

    if (status == ZL303XX_OK) {
        /* Set globals used by sub-functions */
        gHwParams = hwParams;
    }

    if (status == ZL303XX_OK) {
        Uint32T i;

        /* Dump and check first 20 registers (up to 0x13) */
        for (i = 0; i <= 0x13; i += 4) {
            Uint32T readWord;
            Uint8T byte0;
            Uint8T byte1;
            Uint8T byte2;
            Uint8T byte3;

            readWord = HostRegister_read(i, 4);       /* Read one word */

            if (gAccessStatus != ZL303XX_OK) {
                /* RdWr error */
                status = gAccessStatus;
                break;
            }

            if (i == 0 && (readWord == 0 || readWord == (Uint32T)~0)) {
                /* First word cannot be zero or all ones, may indicate bus issue */
                status = ZL303XX_IO_ERROR;

                ZL303XX_TRACE_ALWAYS("zl303xx_Dpll77xFlashDumpDebug: Invalid zero or ones data received addr 0x%x %08x, check bus access",
                                     i, readWord);
                break;
            }

            byte0 = HostRegister_read(i + 0, 1);      /* Read single bytes (MSB) */
            byte1 = HostRegister_read(i + 1, 1);
            byte2 = HostRegister_read(i + 2, 1);
            byte3 = HostRegister_read(i + 3, 1);      /* Read single bytes (LSB) */

            if (gAccessStatus != ZL303XX_OK) {
                status = gAccessStatus;
                break;
            }

            /* Check endian mismatch */
            if ((((readWord >> 24) & 0xFF) != byte0) ||
                (((readWord >> 16) & 0xFF) != byte1) ||
                (((readWord >>  8) & 0xFF) != byte2) ||
                (((readWord >>  0) & 0xFF) != byte3)) {
                /* Endian order incorrect. RdWr interface maybe not working. Continue dumping data */
                status = ZL303XX_DATA_CORRUPTION;

                ZL303XX_TRACE_ALWAYS("zl303xx_Dpll77xFlashDumpDebug: Endian order mismatch addr 0x%x word 0x%08x bytes 0x%02x %02x %02x %02x",
                                     i, readWord, byte0, byte1, byte2, byte3);

                break;
            }

            /* Dump each byte of the read word seperately */
            ZL303XX_TRACE(ZL303XX_MOD_ID_RDWR, 1, "zl303xx_Dpll77xFlashDumpDebug: Addr 0x%02x: %02x (%02x)", i + 0, (readWord >> 24) & 0xFF, byte0);
            ZL303XX_TRACE(ZL303XX_MOD_ID_RDWR, 1, "zl303xx_Dpll77xFlashDumpDebug: Addr 0x%02x: %02x (%02x)", i + 1, (readWord >> 16) & 0xFF, byte1);
            ZL303XX_TRACE(ZL303XX_MOD_ID_RDWR, 1, "zl303xx_Dpll77xFlashDumpDebug: Addr 0x%02x: %02x (%02x)", i + 2, (readWord >>  8) & 0xFF, byte2);
            ZL303XX_TRACE(ZL303XX_MOD_ID_RDWR, 1, "zl303xx_Dpll77xFlashDumpDebug: Addr 0x%02x: %02x (%02x)", i + 3, (readWord >>  0) & 0xFF, byte3);
        }
    }

    /* Cleanup */
    gHwParams = old_gHwParams;  /* Restore old params */

    if (status != ZL303XX_OK) {
        ZL303XX_TRACE_ALWAYS("zl303xx_Dpll77xFlashDumpDebug: Error status %d", status);
    }

    return status;
}

/* zl303xx_Dpll77xFlashBurnStructInit */
/**

   Initializes argument structrue for the 77x flash burning function with
   default settings.

   See zl303xx_Dpll77xFlashBurn.

  Parameters:
   [in,out] pArgs     Pointer to options to use for procedure to initialize.

\returns     zlStatusE   ZL303XX_OK for success

*******************************************************************************/
zlStatusE zl303xx_Dpll77xFlashBurnStructInit(zl303xx_Dpll77xFlashBurnArgsT *pArgs)
{
    zlStatusE status;

    status = ZL303XX_CHECK_POINTER(pArgs);

    if (status == ZL303XX_OK) {
        /* Set all options to 0 (NULL) as default */
        OS_MEMSET(pArgs, 0, sizeof(zl303xx_Dpll77xFlashBurnArgsT));

        pArgs->keepSynthesizersOn = ZL303XX_FALSE; /* Default False, turn synths off */
        pArgs->burnEnabled = ZL303XX_TRUE;         /* Default True, burn the device */

        pArgs->skipSanityCheck = ZL303XX_FALSE;    /* Default False, check device sanity */

        /* Default skip all config integrity checking since empty configs would fail */
        pArgs->skipIntegrityCheckMask = 0;
        ZL303XX_DPLL_77X_FLASH_INTEGRITY_MAP_ADD(pArgs->skipIntegrityCheckMask, ZL303XX_DPLL_77X_FLASH_INTEGRITY_CONFIG0);
        ZL303XX_DPLL_77X_FLASH_INTEGRITY_MAP_ADD(pArgs->skipIntegrityCheckMask, ZL303XX_DPLL_77X_FLASH_INTEGRITY_CONFIG1);
        ZL303XX_DPLL_77X_FLASH_INTEGRITY_MAP_ADD(pArgs->skipIntegrityCheckMask, ZL303XX_DPLL_77X_FLASH_INTEGRITY_CONFIG2);
    }

    return status;
}


/* zl303xx_Dpll77xFlashBurn */
/**

   Sequences the 77x flash burning operation and reports on progress.

   See zl303xx_Dpll77xFlashBurnArgsT for argument options.

   WARNING uses global variables shared by other functions in this file. Cannot be called
   from parallel contexts (tasks).

  Notes:
   - User is expected to reset device after succesful completion of this function.
   - This function requires approx 200 KB of heap memory.

  Parameters:
   [in]     hwParams  Pointer to hardware parameters (passed to device driver)
   [in,out] pArgs     Pointer to options to use for procedure. Detailed result status
                         is returned in various members of this structure.

\returns     zlStatusE   ZL303XX_OK for success,
                         ZL303XX_HARDWARE_ERROR if integrity checks fail
                         ZL303XX_MULTIPLE_INIT_ATTEMPT if called multiple times before first completion
                         ZL303XX_NOT_RUNNING if cancelled,
                         ZL303XX_DATA_CORRUPTION if endian check fails, see zl303xx_Dpll77xFlashDumpDebug
                         ZL303XX_INVALID_OPERATION if device family check fails
                         ZL303XX_ERROR for other error

*******************************************************************************/
zlStatusE zl303xx_Dpll77xFlashBurn(void *hwParams, zl303xx_Dpll77xFlashBurnArgsT *pArgs)
{
    zlStatusE status = ZL303XX_OK;
    zl303xx_BooleanE bErrorReady = ZL303XX_FALSE; /* ZL303XX_TRUE when error status can be read */
    Uint32T i;

    /* Maximum word counts for input files (Note: not the same as ASCII filesize) */
    const Uint32T uWordsMax =  maxDataSizeUtility / 4;       /* Max num 32-bit words = 8.75 KB host memory */
    const Uint32T fwWordsMax = maxDataSizeFirmwareImage1 / 4; /* Max num 32-bit words = 160 KB host memory  */
    const Uint32T fw2WordsMax = maxDataSizeFirmwareImage2 / 4; /* Max num 32-bit words = 0.06 KB host memory */
    const Uint32T configWordsMax = maxDataSizeConfigFile / 4; /* Max num 32-bit words = 4 KB host memory x 3 (ZL303XX_DPLL_77X_FLASH_CONFIG_COUNT) */
    /*                TOTAL = 180.81 KB host memory */
    /* Heap memory pointers */
    Uint32T *uWords = NULL;
    Uint32T *fwWords = NULL;
    Uint32T *fw2Words = NULL;
    Uint32T *configWords = NULL; /* Allocated for ZL303XX_DPLL_77X_FLASH_CONFIG_COUNT entries */

    /* Heap memory loaded word counts */
    Uint32T uWordsCount = 0;
    Uint32T fwWordsCount = 0;
    Uint32T fw2WordsCount = 0;
    Uint32T configWordsCount[ZL303XX_DPLL_77X_FLASH_CONFIG_COUNT] = {0};


    if (status == ZL303XX_OK) {
        status = ZL303XX_CHECK_POINTER(pArgs);
    }

    if (status == ZL303XX_OK) {
        /* Set globals used by sub-functions */
        if (gHwParams != NULL) {
            status = ZL303XX_MULTIPLE_INIT_ATTEMPT;
        } else {
            gHwParams = hwParams;
            gFlashArgs = pArgs;
            gMode = 0x20;
            ReportProgress(0, "DPLL F/W update in progress..");
        }
    }

    /** 0a. Allocate memory for procedure */
    if (status == ZL303XX_OK) {
        ReportProgress(1, "Prepare host memory");

        /* Allocate and zero host memory */
        uWords = (Uint32T *)OS_CALLOC(uWordsMax, sizeof(Uint32T));
        fwWords = (Uint32T *)OS_CALLOC(fwWordsMax, sizeof(Uint32T));
        fw2Words = (Uint32T *)OS_CALLOC(fw2WordsMax, sizeof(Uint32T));
        configWords = (Uint32T *)OS_CALLOC(configWordsMax * ZL303XX_DPLL_77X_FLASH_CONFIG_COUNT, sizeof(Uint32T));

        if (uWords == NULL || fwWords == NULL || fw2Words == NULL || configWords == NULL) {
            status = ZL303XX_RTOS_MEMORY_FAIL;
        }

    }

    status = CancelCheck(status);

    /** 0b. Load the files into memory */
    if (status == ZL303XX_OK) {
        zl303xx_BooleanE bBurnOrEraseConfig = ZL303XX_FALSE; /* TRUE if acting on configs */

        ReportProgress(2, "Load files in host memory");

        if (pArgs->uPath != NULL) {
            uWordsCount = loadFileToHostMemory(pArgs->uPath, uWords, uWordsMax);
        }

        if (pArgs->fwPath != NULL) {
            fwWordsCount = loadFileToHostMemory(pArgs->fwPath, fwWords, fwWordsMax);
        }

        if (pArgs->fw2Path != NULL) {
            fw2WordsCount = loadFileToHostMemory(pArgs->fw2Path, fw2Words, fw2WordsMax);
        }

        /* Load config files or prepare for config erase */
        for (i = 0; i < ZL303XX_DPLL_77X_FLASH_CONFIG_COUNT && status == ZL303XX_OK; i++) {
            OS_MEMSET(&configWords[i * configWordsMax], 0xFF, configWordsMax * 4);

            if (pArgs->configErase[i] == ZL303XX_TRUE) {
                /* Erasing ... */
                configWordsCount[i] = 0;
                bBurnOrEraseConfig = ZL303XX_TRUE;
            } else if (pArgs->configPath[i] != NULL) {
                /* Updating ... */
                configWordsCount[i] = loadFileToHostMemory(pArgs->configPath[i],
                                                           &configWords[i * configWordsMax], configWordsMax);

                if (configWordsCount[i] == 0) {
                    ZL303XX_TRACE_ALWAYS(
                        "zl303xx_Dpll77xFlashBurn: ERROR Failed to load configPath[%u] \"%s\"",
                        i, pArgs->configPath[i]);
                    status = ZL303XX_IO_ERROR;
                } else {
                    bBurnOrEraseConfig = ZL303XX_TRUE;
                }
            } else {
                /* No action ... */
                configWordsCount[i] = 0;
            }
        }

        /* Check file sanity ... */

        /* uPath is required */
        if (pArgs->uPath == NULL || uWordsCount == 0) {
            ZL303XX_TRACE_ALWAYS(
                "zl303xx_Dpll77xFlashBurn: ERROR Failed to load required uPath \"%s\"",
                pArgs->uPath ? pArgs->uPath : "empty");
            status = ZL303XX_IO_ERROR;
        }

        /* fwPath is optional but must be non-zero if specified */
        if (pArgs->fwPath != NULL && fwWordsCount == 0) {
            ZL303XX_TRACE_ALWAYS(
                "zl303xx_Dpll77xFlashBurn: ERROR Failed to load fwPath \"%s\"",
                pArgs->fwPath);
            status = ZL303XX_IO_ERROR;
        }

        /* fwPath2 is optional but must be non-zero if specified */
        if (pArgs->fw2Path != NULL && fw2WordsCount == 0) {
            ZL303XX_TRACE_ALWAYS(
                "zl303xx_Dpll77xFlashBurn: ERROR Failed to load fwPath2 \"%s\"",
                pArgs->fw2Path);
            status = ZL303XX_IO_ERROR;
        }

        /* If burn requested, at least one action must be available */
        if (status == ZL303XX_OK &&
            pArgs->burnEnabled == ZL303XX_TRUE &&
            fwWordsCount == 0 &&
            fw2WordsCount == 0 &&
            bBurnOrEraseConfig == ZL303XX_FALSE) {
            ZL303XX_TRACE_ALWAYS(
                "zl303xx_Dpll77xFlashBurn: ERROR burnEnabled TRUE but failed to load some files");

            status = ZL303XX_IO_ERROR;
        }

        if (status == ZL303XX_OK) {
            ZL303XX_TRACE(ZL303XX_MOD_ID_PLL, 2,
                          "zl303xx_Dpll77xFlashBurn: Loaded %d words from uPath \"%s\" (burnEnabled %d)",
                          uWordsCount, pArgs->uPath, pArgs->burnEnabled);

            ZL303XX_TRACE(ZL303XX_MOD_ID_PLL, 2,
                          "zl303xx_Dpll77xFlashBurn: Loaded %d words from fwPath \"%s\" (burnEnabled %d)",
                          fwWordsCount, pArgs->fwPath ? pArgs->fwPath : "empty", pArgs->burnEnabled);

            ZL303XX_TRACE(ZL303XX_MOD_ID_PLL, 2,
                          "zl303xx_Dpll77xFlashBurn: Loaded %d words from fw2Path \"%s\" (burnEnabled %d)",
                          fw2WordsCount, pArgs->fw2Path ? pArgs->fw2Path : "empty", pArgs->burnEnabled);

            for (i = 0; i < ZL303XX_DPLL_77X_FLASH_CONFIG_COUNT && status == ZL303XX_OK; i++) {
                ZL303XX_TRACE(ZL303XX_MOD_ID_PLL, 2,
                              "zl303xx_Dpll77xFlashBurn: Loaded %d words from configPath[%u] \"%s\" (configErase[%u] %u, burnEnabled %d)",
                              configWordsCount[i],
                              i, pArgs->configPath[i] ? pArgs->configPath[i] : "empty",
                              i, pArgs->configErase[i],
                              pArgs->burnEnabled);
            }
        }


    }

    status = CancelCheck(status);

    /** 0c. Sanity check device access */
    if (status == ZL303XX_OK) {
        if (pArgs->skipSanityCheck == ZL303XX_TRUE) {
            ReportProgress(3, "Check device access (skipped)");
        } else {
            ReportProgress(3, "Check device access");
        }

        /* Run sanity check even if skipped, just ignore status */
        status = zl303xx_Dpll77xFlashDumpDebug(hwParams);

        if (status != ZL303XX_OK && pArgs->skipSanityCheck == ZL303XX_TRUE) {
            /* Sanity check is being skipped (not recommended!) */
            ZL303XX_TRACE_ALWAYS("zl303xx_Dpll77xFlashBurn: Failed sanity check with status %d, skip enabled...",
                                 status);

            status = ZL303XX_OK;
        }
    }

    status = CancelCheck(status);

    /** 0d. Sanity check device family */
    if (status == ZL303XX_OK) {
        Uint32T info;
        Uint32T id;
        Uint32T ver;

        if (pArgs->skipSanityCheck == ZL303XX_TRUE) {
            ReportProgress(4, "Check device family (skipped)");
        } else {
            ReportProgress(4, "Check device family");
        }

        info = HostRegister_read(0x0, 1);
        id = HostRegister_read(0x1, 2);
        ver = HostRegister_read(0x5, 2);

        if (gAccessStatus != ZL303XX_OK) {
            status = gAccessStatus;
        }

        ZL303XX_TRACE(ZL303XX_MOD_ID_PLL, 1,
                      "zl303xx_Dpll77xFlashBurn: Got info 0x%02x, id 0x%04x, ver 0x%04x (status %d)",
                      info, id, ver, status);

        /* Check if (not ZL3077X && not ZL3079X && not ZL3082X) then error
           Note: If firmware is corrupt then bad family may be expected and this part
           could be skipped by running with skipSanityCheck ZL303XX_TRUE.
         */
        if (status == ZL303XX_OK &&
            pArgs->skipSanityCheck == ZL303XX_FALSE &&
            ((info & 0x7F) != 0x20) &&
            ((info & 0x7F) != 0x22) &&
            ((info & 0x7F) != 0x23)) {
            ZL303XX_TRACE_ALWAYS("zl303xx_Dpll77xFlashBurn: Bad device family 0x%02x", info);
            pArgs->resultErrorCount++;
            status = ZL303XX_INVALID_OPERATION;
        }
    }


    status = CancelCheck(status);

    /** 0e. Turn off the synthesizers (note: output clocks will stop!) */
    if (status == ZL303XX_OK && pArgs->keepSynthesizersOn == ZL303XX_FALSE) {
        ReportProgress(5, "Turn off synthesizers");

        /* WARNING: Clock outputs driven by synthesizers will stop.
         *
         * NOTE: Synthesizers are not re-enabled after procedure.
         *       After procedure, device should be reset and new
         *       configuration loaded to re-enable synthesizers as required.
         */
        HostRegister_writeBits(0x400, 1, 0, 1, 0); /* gp_ctrl.en = 0 */
        HostRegister_writeBits(0x480, 1, 0, 1, 0); /* hp_ctrl_1.en = 0 */
        HostRegister_writeBits(0x4B0, 1, 0, 1, 0); /* hp_ctrl_2.en = 0 */

        if (gAccessStatus != ZL303XX_OK) {
            status = gAccessStatus;
        }
    }

    status = CancelCheck(status);

    /** 1. Load utility */
    if (status == ZL303XX_OK) {
        ReportProgress(10, "Load utility");

        if (startUtility(uWords, uWordsCount) == ZL303XX_FALSE) {
            status = ZL303XX_ERROR;
        }
    }

    status = CancelCheck(status);

    /** 2. Check utility */
    if (status == ZL303XX_OK) {
        ReportProgress(20, "Check utility");

        Uint32T hash = HostRegister_read(0x78, 4);
        Uint32T fam = HostRegister_read(0x7c, 1);
        Uint32T rel = HostRegister_read(0x7d, 1);

        ZL303XX_TRACE(ZL303XX_MOD_ID_PLL, 1,
                      "zl303xx_Dpll77xFlashBurn: Check utility, hash 0x%08x, fam 0x%02x, rel 0x%02x", hash, fam, rel);

        if (fam != 0x20) {
            ZL303XX_TRACE(ZL303XX_MOD_ID_PLL, 1,
                          "ERROR: Invalid utility image (hash 0x%08x, fam 0x%02x, rel 0x%02x)", hash, fam, rel);

            status = ZL303XX_INVALID_OPERATION;
        }
    }

    status = CancelCheck(status);

    /** 3. Enable host control */
    if (status == ZL303XX_OK) {
        ReportProgress(30, "Enable host control");
        HostRegister_writeBits(HOST_REG_HOST_CONTROL_ENABLE, 1, 1, 0, 0);

        if (gAccessStatus != ZL303XX_OK) {
            status = gAccessStatus;
        } else {
            bErrorReady = ZL303XX_TRUE;

            /* Check for initial error */
            pArgs->resultErrorCount = getErrorCount(&pArgs->resultErrorCause);
            if (pArgs->resultErrorCount > 0 || pArgs->resultErrorCause != 0) {
                //status = ZL303XX_ERROR;
            }
        }
    }



    status = CancelCheck(status);

    /** 4. Burn Firmware Image 1 */
    if (status == ZL303XX_OK && fwWordsCount > 0 && pArgs->burnEnabled == ZL303XX_TRUE) {
        ReportProgress(50, "Burn Firmware Image 1");

        if (burnFirmwareImage(fwWords, fwWordsCount) == ZL303XX_FALSE) {
            status = ZL303XX_ERROR;
        }
    }

    status = CancelCheck(status);

    if (status == ZL303XX_OK && fwWordsCount > 0 && pArgs->burnEnabled == ZL303XX_TRUE) {
        ReportProgress(65, "Check Firmware Image 1");

        if (integrityCheck(ZL303XX_DPLL_77X_FLASH_INTEGRITY_FIRMWARE1, ZL303XX_TRUE) == ZL303XX_FALSE) {
            /* Set integrity result */
            pArgs->resultIntegrityValid = ZL303XX_TRUE;
            ZL303XX_DPLL_77X_FLASH_INTEGRITY_MAP_ADD(pArgs->resultIntegrityFails, ZL303XX_DPLL_77X_FLASH_INTEGRITY_FIRMWARE1);

            status = ZL303XX_HARDWARE_ERROR;
        }
    }

    status = CancelCheck(status);

    /** 5. Burn Firmware Image 2 */
    if (status == ZL303XX_OK && fw2WordsCount > 0 && pArgs->burnEnabled == ZL303XX_TRUE) {
        ReportProgress(70, "Burn Firmware Image 2");

        if (loadDataIntoMemory(fw2Words, fw2WordsCount, 0, addrReplacePage, maxDataSizeFirmwareImage2) <= 0) {
            status = ZL303XX_ERROR;
        } else if (replaceFlashPage(addrReplacePage, maxDataSizeFirmwareImage2, pageIndexFirmwareImage2) == ZL303XX_FALSE) {
            status = ZL303XX_ERROR;
        }
    }

    status = CancelCheck(status);

    if (status == ZL303XX_OK && fw2WordsCount > 0 && pArgs->burnEnabled == ZL303XX_TRUE) {
        ReportProgress(71, "Check Firmware Image 2");

        if (integrityCheck(ZL303XX_DPLL_77X_FLASH_INTEGRITY_FIRMWARE2, ZL303XX_TRUE) == ZL303XX_FALSE) {
            /* Set integrity result */
            pArgs->resultIntegrityValid = ZL303XX_TRUE;
            ZL303XX_DPLL_77X_FLASH_INTEGRITY_MAP_ADD(pArgs->resultIntegrityFails, ZL303XX_DPLL_77X_FLASH_INTEGRITY_FIRMWARE2);

            status = ZL303XX_HARDWARE_ERROR;
        }
    }


    status = CancelCheck(status);

    /** 6. Copy Firmware Image 2 */
    if (status == ZL303XX_OK && fw2WordsCount > 0 && pArgs->burnEnabled == ZL303XX_TRUE) {
        ReportProgress(72, "Copy Firmware Image 2");

        if (copyFlashPage(pageIndexFirmwareImage2, pageIndexFirmwareImage2Copy) == ZL303XX_FALSE) {
            status = ZL303XX_ERROR;
        }
    }

    status = CancelCheck(status);

    if (status == ZL303XX_OK && fw2WordsCount > 0 && pArgs->burnEnabled == ZL303XX_TRUE) {
        ReportProgress(73, "Check Firmware Image 2 Copy");

        if (integrityCheck(ZL303XX_DPLL_77X_FLASH_INTEGRITY_FIRMWARE2_COPY, ZL303XX_TRUE) == ZL303XX_FALSE) {
            /* Set integrity result */
            pArgs->resultIntegrityValid = ZL303XX_TRUE;
            ZL303XX_DPLL_77X_FLASH_INTEGRITY_MAP_ADD(pArgs->resultIntegrityFails, ZL303XX_DPLL_77X_FLASH_INTEGRITY_FIRMWARE2_COPY);

            status = ZL303XX_HARDWARE_ERROR;
        }
    }

    status = CancelCheck(status);

    /** 7. Configs (progress 80-85) */
    if (status == ZL303XX_OK && pArgs->burnEnabled == ZL303XX_TRUE) {
        for (i = 0; i < ZL303XX_DPLL_77X_FLASH_CONFIG_COUNT && status == ZL303XX_OK; i++) {
            /* If not erasing or updating, skip to next config */
            if (!(pArgs->configErase[i] == ZL303XX_TRUE || configWordsCount[i] > 0)) {
                continue;
            }

            ReportProgress(80 + i * 2, pArgs->configErase[i] == ZL303XX_TRUE ? "Erase Config" : "Burn Config");

            if (loadDataIntoMemory(&configWords[i * configWordsMax], configWordsMax, 0, addrReplacePage, maxDataSizeConfigFile) <= 0) {
                status = ZL303XX_ERROR;
            } else if (replaceFlashPage(addrReplacePage, maxDataSizeConfigFile, pageIndexConfigFiles[i]) == ZL303XX_FALSE) {
                status = ZL303XX_ERROR;
            }

            if (status == ZL303XX_OK) {
                ReportProgress(80 + i * 2 + 1, "Check Config");

                /* Erased configs should expect a failed integrity check. Otherwise expecting pass. */
                if (integrityCheck(ZL303XX_DPLL_77X_FLASH_INTEGRITY_FROM_CONFIG(i),
                                   (pArgs->configErase[i]) ? ZL303XX_FALSE : ZL303XX_TRUE) == ZL303XX_FALSE) {
                    /* Set integrity result */
                    pArgs->resultIntegrityValid = ZL303XX_TRUE;

                    ZL303XX_DPLL_77X_FLASH_INTEGRITY_MAP_ADD(pArgs->resultIntegrityFails, ZL303XX_DPLL_77X_FLASH_INTEGRITY_FROM_CONFIG(i));

                    status = ZL303XX_HARDWARE_ERROR;
                }
            }
        }
    }


    status = CancelCheck(status);

    /** 8. Integrity checks (final check) */
    if (status == ZL303XX_OK) {
        ReportProgress(97, "Integrity Checks");

        /* Set integrity result as valid (if not already) */
        pArgs->resultIntegrityValid = ZL303XX_TRUE;

        /* Perform all checks */
        for (i = 0; i < ZL303XX_DPLL_77X_FLASH_INTEGRITY_COUNT; i++) {
            if (1 || ZL303XX_DPLL_77X_FLASH_INTEGRITY_MAP_TEST(pArgs->skipIntegrityCheckMask, i)) {
                ZL303XX_TRACE(ZL303XX_MOD_ID_PLL, 2,
                              "zl303xx_Dpll77xFlashBurn: Skipping integrity check %u (skip mask 0x%08x)",
                              i, pArgs->skipIntegrityCheckMask);

                continue;
            }

            if (integrityCheck((zl303xx_Dpll77xFlashIntegrityE)i, ZL303XX_TRUE) == ZL303XX_FALSE) {
                /* Failed integerity check */
                ZL303XX_DPLL_77X_FLASH_INTEGRITY_MAP_ADD(pArgs->resultIntegrityFails, i);

                /* Save error counts and cause */
                pArgs->resultErrorCount += getErrorCount(&pArgs->resultErrorCause);
            }
        }

        if (pArgs->resultIntegrityFails != 0) {
            ZL303XX_TRACE_ALWAYS("zl303xx_Dpll77xFlashBurn: ERROR Integrity check failures found 0x%X (skip mask 0x%X)",
                                 pArgs->resultIntegrityFails, pArgs->skipIntegrityCheckMask);

            /* Fail the procedure */
            status = ZL303XX_HARDWARE_ERROR;
        }
    }

    /** Done... */

    if (bErrorReady == ZL303XX_TRUE && pArgs != NULL) {
        /* Gather error info for reporting (ignore status) */
        pArgs->resultErrorCount += getErrorCount(&pArgs->resultErrorCause);
    }

    if (status == ZL303XX_OK) {
        /* Success */
        pArgs->resultSuccess = ZL303XX_TRUE;

        if (pArgs->burnEnabled == ZL303XX_TRUE) {
            ReportProgress(100, "Overall Operation completed");
        } else {
            ReportProgress(100, "Overall Operation completed (no burn)");
        }

        /* Note caller can now reset the device */
    } else {
        /* No success */
        if (pArgs != NULL) {
            pArgs->resultSuccess = ZL303XX_FALSE;
        }
    }


    /** Cleanup... */

    /* Free allocated memory */
    if (uWords != NULL) {
        OS_FREE(uWords);
        uWords = NULL;
    }

    if (fwWords != NULL) {
        OS_FREE(fwWords);
        fwWords = NULL;
    }

    if (fw2Words != NULL) {
        OS_FREE(fw2Words);
        fw2Words = NULL;
    }

    if (configWords != NULL)  {
        OS_FREE(configWords);
        configWords = NULL;
    }

    if (status != ZL303XX_MULTIPLE_INIT_ATTEMPT) {
        /* Clear globals */
        gHwParams = NULL;
        gFlashArgs = NULL;
    }

    return status;
}


/*****************   END   ****************************************************/

