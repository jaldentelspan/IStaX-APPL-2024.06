

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
*     The definitions of the Microchip logging enums
*
*******************************************************************************/

#ifndef _ZL_MSGQ_LOGGING_H_
#define _ZL_MSGQ_LOGGING_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "zl303xx_Global.h"
#include "zl303xx_Error.h"
#include "zl303xx_Os.h"
#include "zl303xx_Trace.h"
#include <fcntl.h>

#ifdef OS_VXWORKS
    #define ZL303XX_LOG_MSGQ_TASK_PRIORITY 85 /* VxWorks Lowest for MSCC-related */
#else
 #ifdef SOCPG_PORTING
#ifdef OS_FREERTOS
    #define ZL303XX_LOG_MSGQ_TASK_PRIORITY 8 /* FreeRTOS Lowest for ZL-related */
#endif
#else
    #define ZL303XX_LOG_MSGQ_TASK_PRIORITY 55 /* Linux Lowest for ZL-related */
#endif
#endif

/* ZL_MSGQ_FMT_IN_MSG will store the format string in the msgQ message thus
 avoiding the global ring buffer (at the expense of larger messages) */
#define ZL_MSGQ_FMT_IN_MSG 1

#if defined ZL_MSGQ_FMT_IN_MSG
#define ZL303XX_LOG_TASK_MSGQ_SIZE 128 /* Max "unlimited" */
#else
#define ZL303XX_LOG_TASK_MSGQ_SIZE 64 /* Max 64 */
#endif

#define ZL303XX_LOG_MSGQ_TASK_STACK_SIZE 8000

#define MAX_FMT_LEN   256
#define FMT_ARRAY_SIZE 63   /* Must be < 64 bits (BusyBufferBits) */

/* Define to enable in-memory circular log buffer */
#define ZL303XX_LOG_HISTORY_ENABLED 1

#if defined ZL303XX_LOG_HISTORY_ENABLED
    /* Define circular log buffer memory size in bytes (128 KB default).
       Bigger means more history but more heap memory use. */
    /* Note: Can be dynamically resized at runtime, see zl303xx_LogHistoryResize */
    #define ZL303XX_LOG_HISTORY_BUFFER_SIZE (128*1024)
#endif

typedef enum
{
    UNKNOWN_APRLOG_ENUM = 0,
    LOG_FMT_STR = 1,

    A_0000  =  0002,
    A_1000  =  1000,
    A_1001  =  1001,
    A_1060  =  1060,
    A_1061  =  1061,
    A_1120  =  1120,
    A_1121  =  1121,
    A_1180  =  1180,
    A_1240  =  1240,
    A_1300  =  1300,
    A_1400  =  1400,
    A_1500  =  1500,
    A_1600  =  1600,
    A_1700  =  1700,
    A_1800  =  1800,
    A_1900  =  1900,
    A_2000  =  2000,
    A_3000  =  3000,
    A_3060  =  3060,
    A_3120  =  3120,
    A_3180  =  3180,
    A_3240  =  3240,
    A_3300  =  3300,
    A_3360  =  3360,
    A_3400  =  3400,
    A_3460  =  3460,
    A_3470  =  3470,
    A_3480  =  3480,
    A_3490  =  3490,
    A_3500  =  3500,
    A_3510  =  3510,
    A_3520  =  3520,
    A_3580  =  3580,
    A_3620  =  3620,
    A_4000  =  4000,
    A_5000  =  5000,
    A_5100  =  5100,
    A_6000  =  6000,
    A_7000  =  7000,
    A_7100  =  7100,
    A_7200  =  7200,
    A_7300  =  7300,
    A_7400  =  7400, /* Group4 windowing, dampening and collection period*/
    A_7500  =  7500, /* future */
    A_7600  =  7600, /* future */
    A_7700  =  7700, /* future */
    A_7800  =  7800, /* future */
    A_7900  =  7900, /* future */
    A_8000  =  8000, /* future */
    A_8100  =  8100, /* future */
    A_8200  =  8200, /* future */
    A_8300  =  8300, /* future */
    A_8400  =  8400, /* future */
    A_8500  =  8500, /* future */
    A_8600  =  8600, /* future */
    A_8700  =  8700, /* future */
    A_8800  =  8800, /* future */
    A_8900  =  8900, /* future */
    A_9000  =  9000, /* future */
    A_9100  =  9100, /* future */
    A_9200  =  9200, /* future */
    A_9300  =  9300, /* future */
    A_9400  =  9400, /* future */
    A_9500  =  9500, /* future */
    A_9600  =  9600, /* future */
    A_9700  =  9700, /* future */
    A_9800  =  9800, /* future */
    A_9900  =  9900, /* future */
    A_END   =  9999,

    b_1000  = 11000,
    b_1001  = 11001,
    b_1004  = 11004,
    b_1007  = 11007,
    b_1010  = 11010,
    b_2000  = 12000,
    b_3000  = 13000,
    b_3001  = 13001,
    b_3002  = 13002,
    b_3003  = 13003,
    b_3004  = 13004,
    b_3005  = 13005,
    b_3014  = 13014,
    b_3015  = 13015,
    b_4000  = 14000,
    b_4001  = 14001,
    b_4002  = 14002,
    b_4003  = 14003,
    b_4xxx  = 14999,
    b_3020  = 13020,
    b_4100  = 14100,
    b_4200  = 14200,
    b_4300  = 14300,
    b_4400  = 14400,
    b_4500  = 14500,
    b_4600  = 14600,
    b_4700  = 14700,
    b_4701  = 14701,
    b_4702  = 14702,
    b_4703  = 14703,
    b_4704  = 14704,
    b_4706  = 14706,
    b_4800  = 14800,
    b_5000  = 15000, /* future */
    b_5100  = 15100, /* future */
    b_5200  = 15200, /* future */
    b_5300  = 15300, /* future */
    b_5400  = 15400, /* future */
    b_5500  = 15500, /* future */
    b_5600  = 15600, /* future */
    b_5700  = 15700, /* future */
    b_5800  = 15800, /* future */
    b_5900  = 15900, /* future */
    b_6000  = 16000, /* future */
    b_6100  = 16100, /* future */
    b_6200  = 16200, /* future */
    b_6300  = 16300, /* future */
    b_6400  = 16400, /* future */
    b_6500  = 16500, /* future */
    b_6600  = 16600, /* future */
    b_6700  = 16700, /* future */
    b_6800  = 16800, /* future */
    b_6900  = 16900, /* future */

    b_20000 = 20000,
    b_20100 = 20100,
    b_20200 = 20200,
    b_20300 = 20300,
    b_20400 = 20400,
    b_20500 = 20500,
    b_20600 = 20600,
    b_20700 = 20700,
    b_21200 = 21200,
    b_21300 = 21300,
    b_21400 = 21400,
    b_21500 = 21500,
    b_21600 = 21600,
    b_21700 = 21700,
    b_21800 = 21800,
    b_21900 = 21900,
    b_22000 = 22000,
    b_22100 = 22100,
    b_22200 = 22200,
    b_22300 = 22300,
    b_22400 = 22400,
    b_22500 = 22500,
    b_22600 = 22600,
    b_22700 = 22700,
    b_22800 = 22800,
    b_22900 = 22900,
    b_23000 = 23000,
    b_23001 = 23001,
    b_23002 = 23002,
    b_23003 = 23003,
    b_23004 = 23004,
    b_23005 = 23005,
    b_23006 = 23006,
    b_23007 = 23007,
    b_23008 = 23008,
    b_23009 = 23009,
    b_23100 = 23100,
    b_23200 = 23200,
    b_23300 = 23300,
    b_23301 = 23301,
    b_23302 = 23302,
    b_23303 = 23303,
    b_23304 = 23304,
    b_23305 = 23305,
    b_23306 = 23306,
    b_23400 = 23400,
    b_23500 = 23500,
    b_23600 = 23600,
    b_23700 = 23700,
    b_23800 = 23800,
    b_23900 = 23900,
    b_24000 = 24000,
    b_24100 = 24100,
    b_24200 = 24200,
    b_24201 = 24201,
    b_24300 = 24300,
    b_24301 = 24301,
    b_24400 = 24400,
    b_24500 = 24500,
    b_24600 = 24600,
    b_24700 = 24700,
    b_24900 = 24900,
    b_25000 = 25000,
    b_25001 = 25001,
    b_25100 = 25100,
    b_25200 = 25200,
    b_25300 = 25300,
    b_25400 = 25400,
    b_25500 = 25500,
    b_25600 = 25600,
    b_25700 = 25700,
    b_25701 = 25701,
    b_25800 = 25800,
    b_25900 = 25900,
    b_26000 = 26000,
    b_26100 = 26100,
    b_26200 = 26200,
    b_27000 = 27000, /* future */
    b_27100 = 27100, /* future */
    b_27200 = 27200, /* future */
    b_27300 = 27300, /* future */
    b_27400 = 27400, /* future */
    b_27500 = 27500, /* future */
    b_27600 = 27600, /* future */
    b_27700 = 27700, /* future */
    b_27800 = 27800, /* future */
    b_27900 = 27900, /* future */
    b_30000 = 30000, /* future */
    b_30100 = 30100, /* future */
    b_30200 = 30200, /* future */
    b_30300 = 30300, /* future */
    b_30400 = 30400, /* future */
    b_30500 = 30500, /* future */
    b_30600 = 30600, /* future */
    b_30700 = 30700, /* future */
    b_30800 = 30800, /* future */
    b_30900 = 30900, /* future */
    b_END   = 39999,

    C_0000  = 50000,
    C_0100  = 50100,
    C_1000  = 51000,
    C_1001  = 51001,
    C_1002  = 51002,
    C_1003  = 51003,
    C_2000  = 52000,
    C_2001  = 52001,
    C_3000  = 53000,
    C_3001  = 53001,
    C_4000  = 54000,
    C_5000  = 55000,
    C_5001  = 55001,
    C_5002  = 55002,
    C_5003  = 55003,
    C_5004  = 55004,
    C_6000  = 56000,
    C_6100  = 56100, /* future */
    C_6200  = 56200, /* future */
    C_6300  = 56300, /* future */
    C_6400  = 56400, /* future */
    C_6500  = 56500, /* future */
    C_6600  = 56600, /* future */
    C_6700  = 56700, /* future */
    C_6800  = 56800, /* future */
    C_6900  = 56900, /* future */
    C_7000  = 57000,
    C_7100  = 57100,
    C_7200  = 57200,
    C_7300  = 57300,
    C_7400  = 57400,
    C_7500  = 57500, /* future */
    C_7600  = 57600, /* future */
    C_7700  = 57700, /* future */
    C_7800  = 57800, /* future */
    C_7900  = 57900, /* future */
    C_8000  = 58000,
    C_8100  = 58100,
    C_8200  = 58200,
    C_8300  = 58300,
    C_8400  = 58400,
    C_8500  = 58500,
    C_8600  = 58600,
    C_8700  = 58700, /* future */
    C_8800  = 58800, /* future */
    C_8900  = 58900, /* future */
    C_9000  = 59000, /* zl303xx_AprInitS configuration */
    C_9100  = 59100, /* zl303xx_AprAddDeviceS configuration */
    C_9200  = 59200, /* future */
    C_9300  = 59300, /* zl303xx_AprAddServerS configuration */
    C_9400  = 59400, /* future */
    C_9500  = 59500, /* future */
    C_9600  = 59600, /* zl303xx_AprConfigure1HzS configuration (fwd) */
    C_9700  = 59700, /* zl303xx_AprConfigure1HzS configuration (rev) */
    C_9800  = 59800, /* zl303xx_PFReConfigS configuration */
    C_9900  = 59900, /* future */
    c_END   = 59999,

    A_90000 = 90000,
    A_90010 = 90010,
    A_90020 = 90020,
    A_90030 = 90030,
    A_90040 = 90040,
    A_90050 = 90050,
    A_90060 = 90060,
    A_90070 = 90070,
    A_90080 = 90080,
    A_90090 = 90090,
    A_90100 = 90100,
    A_93000 = 93000,
    A_93040 = 93040,
    A_93080 = 93080,
    A_93120 = 93120,
    A_94000 = 94000, /* future */
    A_94100 = 94100, /* future */
    A_94200 = 94200, /* future */
    A_94300 = 94300, /* future */
    A_94400 = 94400, /* future */
    A_94500 = 94500, /* future */
    A_94600 = 94600, /* future */
    A_94700 = 94700, /* future */
    A_94800 = 94800, /* future */
    A_94900 = 94900, /* future */

    LAST_LOG_ENUM

} zl303xx_LoggingE;

typedef enum
{
    APR_ENUM_TYPE,      /* Used only within APR */
    FMT_STR_TYPE,       /* Formatted msg */
    SHUTDOWN_MSGQ
} zl303xxlogMsgTypeE;

typedef struct zl303xx_MsgQLogMsgS
{
    zl303xxlogMsgTypeE logMsgType;
    zl303xx_LoggingE eNumId;
    Uint64S cpuTime;

    UnativeT arg0;  /* Filled if APR_ENUM_TYPE */
    UnativeT arg1;
    UnativeT arg2;
    UnativeT arg3;
    UnativeT arg4;
    UnativeT arg5;

    Uint8T *pFmtStr;    /* Pointer to Format str */
    Uint8T indx;        /* Index into fmtArray */

#if defined ZL_MSGQ_FMT_IN_MSG
    /* Place format string contents inside the message itself */
    Uint8T fmtStr[MAX_FMT_LEN];
#endif
}zl303xx_MsgQLogMsgS;



zlStatusE zl303xx_ReEnableLogToMsgQ(void);
zlStatusE zl303xx_DisableLogToMsgQ(void);
zlStatusE zl303xx_SetupLogToMsgQ(FILE* fd);
zlStatusE zl303xx_ShutdownLogToMsgQ(FILE* fd);
zlStatusE zl303xx_ChangeLogToMsgQFilePtr(FILE *logFd);
Sint32T   zl303xx_LogToMsgQ(zl303xx_LoggingE eNum, const char *str, UnativeT arg0, UnativeT arg1,
                         UnativeT arg2, UnativeT arg3, UnativeT arg4, UnativeT arg5);
Sint32T zl303xx_Log3ToMsgQ(zl303xx_LoggingE eNum, const char *str, UnativeT arg0, UnativeT arg1,
                         UnativeT arg2, UnativeT arg3, UnativeT arg4, UnativeT arg5, UnativeT deviceId, UnativeT serverId);

void zl303xx_LogToMsgQPrintStats(FILE *filePtr);

Sint32T zl303xx_LogHistoryDumpToFile(const char* filename, Uint32T len, zl303xx_BooleanE bErase);
Sint32T zl303xx_LogHistoryDump(FILE* strm, Uint32T len, zl303xx_BooleanE bErase);
zlStatusE zl303xx_LogHistoryErase(Uint32T len);
zlStatusE zl303xx_LogHistoryResize(Uint32T size);
zlStatusE zl303xx_LogHistoryGetInfo(Uint32T *size, Uint32T *count);

#ifdef __cplusplus
}
#endif

#endif   /* MULTIPLE INCLUDE BARRIER */
