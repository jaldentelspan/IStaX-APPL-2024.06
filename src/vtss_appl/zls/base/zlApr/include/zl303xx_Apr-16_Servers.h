



/*******************************************************************************

   $Id: 6517a3b4707c071ac26ad8fcef236bd9d8d99889

   Copyright (c) 2006-2022 Microchip Technology Inc. and its subsidiaries, all rights reserved.
   Subject to the terms of the license that accompanies the software and controls as it relates to the software and any conflicting terms herein, you may use this Microchip software and any derivatives exclusively with Microchip products.
   You are responsible for complying with third party license terms applicable to your use of third party software (including open source software) that may accompany this Microchip software.
   SOFTWARE IS 'AS IS'. NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
   IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.
   TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS RELATED TO THE SOFTWARE WILL NOT EXCEED AMOUNT OF FEES, IF ANY, YOU PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
   The timing algorithms implemented in the software code are Patent Pending.

*******************************************************************************/

#if !defined _ZL303XX_APR_H
#define _ZL303XX_APR_H


#if defined __cplusplus
extern "C" {
#endif

/*****************   INCLUDE FILES   ******************************************/
#include "zl303xx_Global.h"
#include "zl303xx_Macros.h"
#include "zl303xx_Error.h"
#include "zl303xx_TSDebug.h"
#include "zl303xx_Int64.h"
#include "zl303xx_DeviceIf.h"

/*****************   # DEFINES   **********************************************/

/* Legacy defines remapped to ZL303XX_* naming here */
#if defined APR_SRVRS_16_INCLUDED
#endif

#if defined OS_LINUX
  #if !defined ZL303XX_OS_LINUX
    #define ZL303XX_OS_LINUX
      #warning MSCC: OS_LINUX was not expected (Add #define ZL303XX_OS_LINUX)
  #endif
#endif

#if defined OS_VXWORKS
  #if !defined ZL303XX_OS_VXWORKS
    #define ZL303XX_OS_VXWORKS
      #warning  MSCC: OS_VXWORKS was not expected (Add #define ZL303XX_OS_VXWORKS)
  #endif
#endif

#define ZL303XX_ALGO_MAX_PARAMETERS 210
#define ZL303XX_ALGO_MAX_MODIFIED_PARAMETERS 100

/* End of ZL303XX_* naming remapping */


/* misc settings */
#define ZL303XX_APR_MAX_NUM_DEVICES               8

#define ZL303XX_APR_MAX_NUM_MASTERS               (Uint32T)8

#undef ZL303XX_APR_MAX_NUM_MASTERS
#define ZL303XX_APR_MAX_NUM_MASTERS               (Uint32T)16
#if defined ZL303XX_APR_SRVRS_32_INCLUDED
#undef ZL303XX_APR_MAX_NUM_MASTERS
#define ZL303XX_APR_MAX_NUM_MASTERS               (Uint32T)32
#endif

#define ZL303XX_APR_PARAM_1                       (Uint32T)2


#define ZL303XX_NOTIFY_MSG_TIMEOUT_SECS           10  /* Kernel ticks per sec * 10 secs (see LinuxOsTimerEtc) */
#define ZL303XX_APR_TASK_SHUTDOWN_DELAY_MULT      2   /* Currently 2 * 125 = 250ms */

#define ZL303XX_APR_MAX_NUM_STREAMS               (2*ZL303XX_APR_MAX_NUM_MASTERS)

/* Phase jump detection timeout, unit of seconds */
#define ZL303XX_TS_PHASE_JUMP_DETECT_TIME_OUT      (Uint32T)(8)

/* RTP timestamp clock rate, unit of Hz */
#define ZL303XX_DEFAULT_RTP_CLK_RATE            10000000

/* Enter phase lock status threshold, unit: ns */
#define ZL303XX_ENTER_PHASE_LOCK_STATUS_THRESHOLD   1000

/* Enter phase lock 1Hz network condition threshold, unit: 1-100 */
#define ZL303XX_ENTER_PHASE_LOCK_1HZ_NETWORK_CONDITION_THRESHOLD   75
#undef ZL303XX_ENTER_PHASE_LOCK_1HZ_NETWORK_CONDITION_THRESHOLD
#define ZL303XX_ENTER_PHASE_LOCK_1HZ_NETWORK_CONDITION_THRESHOLD   1

/* Exit phase lock status threshold, unit: ns */
#define ZL303XX_EXIT_PHASE_LOCK_STATUS_THRESHOLD   1500

/* Electrical reference switching stage 1 transient period in 800/1000 ms periods */
#define ZL303XX_REF_SWITCH_TRANSIENT_STAGE_1_DELAY  5

/* Electrical reference switching stage 2 transient period in 800/1000 ms periods */
#define ZL303XX_REF_SWITCH_TRANSIENT_STAGE_2_DELAY  5

/* Defaul correction field saturation value is +/- 2^(63-16) = 140737488355328 ns*/
#define ZL303XX_CORR_FIELD_SATURATION_VAL  (Uint64T)(140737488355328)


/*!  ZL303XX_MAX_STATS_PDV_PERCENTILE_STORAGE
The max number of transit times to store for calculating the PDVPercentile
value in the statistics package: 64 packets/sec * 20 seconds.

Note: Increasing this value by 1 increases memory requirements by sizeof(Uint64S).
 */
#define ZL303XX_MAX_STATS_PDV_PERCENTILE_STORAGE (64*20)

/* Macro to check for a valid boolean variable. */
/* Use of the comma operator is intentional here so that ZL303XX_ERROR_TRAP is
   called prior to assigning the return code. */
#define ZL303XX_CHECK_BOOLEAN(b) \
   ( (((b) != ZL303XX_FALSE) && ((b) != ZL303XX_TRUE)) \
      ?  (  ZL303XX_ERROR_NOTIFY("Invalid boolean value: " #b),   \
            ZL303XX_PARAMETER_INVALID)  \
      :  (  ZL303XX_OK)  \
   )

/* Macro to convert an integer to a boolean value
   Any non-zero value is assigned TRUE */
#define ZL303XX_INT_TO_BOOL(b)   \
   (((b) == 0) ? (ZL303XX_FALSE) : (ZL303XX_TRUE))


#define THDebug 0
  /*  #define OUTPUTCLK 1000000000
    #define SYSCLK 1220703125
    #define CTR_FREQ 900719925474*/

    #define OUTPUTCLK 125000000
    #define SYSCLK 137438953
    #define CTR_FREQ 1e12

/* Timestamp jump threshold for logging printouts */
#define ZL303XX_APR_TS_JUMP_LOGGING_THRESHOLD_NS        1000000

/* APR default algorithm long interrupt interval */
#define ZL303XX_APR_ALG_LONG_INTERRUPT_INTERVAL         10000

/* APR default State Machine Update long interrupt interval */
#define ZL303XX_APR_STATE_MACHINE_UPDATE_LONG_INTERVAL  10000

/* For the APR Queue-based Interface ********************************************/
#define PTP_RTP_TS_FMT 1           /* TS Format for PTP and RTP */
#define PD_TS_FMT      2           /* TS_Format for PEER_DELAY */

#if defined ZL303XX_OS_VXWORKS
    #define ZL303XX_APR_QIF_TASK_PRIORITY    30 /* VxWorks - Highest is APR_QIF */
    #define ZL303XX_APR_DELAY_TASK_PRIORITY  30 /* VxWorks - Highest is APR_QIF */
    #define ZL303XX_APR_NOTIFY_TASK_PRIORITY 90 /* VxWorks - Lower */
#else
    #define ZL303XX_APR_QIF_TASK_PRIORITY    90 /* Linux - Highest is APR_QIF */
    #define ZL303XX_APR_DELAY_TASK_PRIORITY  90 /* Linux - Highest is APR_QIF */
    #define ZL303XX_APR_NOTIFY_TASK_PRIORITY 80 /* Linux - Lower */
#endif

#define ZL303XX_APR_DELAY_TASK_STACK_SIZE  (Uint32T)7000

/* APRQIF Event Queue configuration  */
#define ZL303XX_APR_QIF_MAX_MSG_DATA_LEN 128  /* = 32 x Int32T = 8 x Int64S */
#define ZL303XX_APR_QIF_TASK_STACK_SIZE  (16*1024) /* 16KB = 16384 bytes */
#define ZL303XX_APR_QIF_TASK_EVENTQ_SIZE (Uint32T)16

/* Apr Notify Task */
#define ZL303XX_APR_NOTIFY_MSG_Q_LEN        8
#define ZL303XX_APR_NOTIFY_TASK_NAME        "zlAprNotifyTask"
#define ZL303XX_APR_NOTIFY_TASK_STACK_SIZE  (16*1024) /* 16KB = 16384 bytes */

/* Default APR timer periods in ms */
#define ZL303XX_APR_TIMER1_PERIOD_MS   750   /* AprAd Task Period */
#define ZL303XX_APR_TIMER2_PERIOD_MS   125   /* Drives aprSample and aprPF tasks. */

/*****************   DATA TYPES AND STRUCTURES   ******************************/

/*!  Enum: zl303xx_AprPerPacketAdjTypeE
     1Hz per packet realignment adjustment type
  */
/*!  Var: zl303xx_AprPerPacketAdjTypeE ZL303XX_PER_PACKET_ADJ_NCO_PHASE
     Indication that the per packet alignment will use DPLL NCO mode with
     phase adjustments,
  */
/*!  Var: zl303xx_AprPerPacketAdjTypeE ZL303XX_PER_PACKET_ADJ_NCO_FREQ
     Indication that the per packet alignment will use DPLL NCO mode with
     frequency adjustments,
 */
/*!  Var: zl303xx_AprPerPacketAdjTypeE ZL303XX_PER_PACKET_ADJ_NCO_ADJTIME
     Indication that the per packet alignment will use DPLL NCO mode with
     user custom adjust time routine,
 */
/*!  Var: zl303xx_AprPerPacketAdjTypeE ZL303XX_PER_PACKET_ADJ_HYBRID_PHASE
     Indication that the per packet alignment will use DPLL electrical mode with
     phase adjustments,
 */
/*!  Var: zl303xx_AprPerPacketAdjTypeE ZL303XX_PER_PACKET_ADJ_HYBRID_FREQ
     Indication that the per packet alignment will use DPLL electrical mode with
     frequency adjustments,
 */
/*!  Var: zl303xx_AprPerPacketAdjTypeE ZL303XX_PER_PACKET_ADJ_HYBRID_ADJTIME
     Indication that the per packet alignment will use DPLL electrical mode with
     user custom adjust time routine,
 */
/*!  Var: zl303xx_AprPerPacketAdjTypeE ZL303XX_PER_PACKET_ADJ_BYPASS
     Indication that the per packet alignment will be bypassed.
 */
typedef enum {
   ZL303XX_PER_PACKET_ADJ_NCO_PHASE,   /* Currently unsupported, please do not enable. */
   ZL303XX_PER_PACKET_ADJ_NCO_FREQ,    /* Currently unsupported, please do not enable. */
   ZL303XX_PER_PACKET_ADJ_NCO_ADJTIME, /* Currently unsupported, please do not enable. */
   ZL303XX_PER_PACKET_ADJ_HYBRID_PHASE,
   ZL303XX_PER_PACKET_ADJ_HYBRID_FREQ,
   ZL303XX_PER_PACKET_ADJ_HYBRID_ADJTIME,
   ZL303XX_PER_PACKET_ADJ_BYPASS
} zl303xx_AprPerPacketAdjTypeE;

/*!  Enum: zl303xx_AprHoldoverTypeE
     APR holdover settings
  */
/*!  Var: zl303xx_AprHoldoverTypeE ZL303XX_APR_HOLDOVER_SETTING_LEGACY
     Legacy holdover behavior
 */
/*!  Var: zl303xx_AprHoldoverTypeE ZL303XX_APR_HOLDOVER_SETTING_FREEZE
     Takes last APR value as holdover frequency
 */
/*!  Var: zl303xx_AprHoldoverTypeE ZL303XX_APR_HOLDOVER_SETTING_SMART
     Smart holdover behavior
 */
/*!  Var: zl303xx_AprHoldoverTypeE ZL303XX_APR_HOLDOVER_SETTING_USER_SET
     Takes user set value as holdover frequency
 */
typedef enum {
   ZL303XX_APR_HOLDOVER_SETTING_LEGACY=0,
   ZL303XX_APR_HOLDOVER_SETTING_FREEZE,
   ZL303XX_APR_HOLDOVER_SETTING_SMART,
   ZL303XX_APR_HOLDOVER_SETTING_USER_SET,
   ZL303XX_APR_HOLDOVER_SETTING_LAST
} zl303xx_AprHoldoverTypeE;

/*!  Enum: zl303xx_AprRefSwitchFreqSelectE
     APR holdover settings
  */
/*!  Var: zl303xx_AprHoldoverTypeE ZL303XX_APR_REF_SWITCH_FREQ_SEL_LEGACY
     Legacy reference and configuration switching behaviour
 */
/*!  Var: zl303xx_AprHoldoverTypeE ZL303XX_APR_REF_SWITCH_FREQ_SEL_USER_VAL
     New reference switching server takes user given value
 */
/*!  Var: zl303xx_AprHoldoverTypeE ZL303XX_APR_REF_SWITCH_FREQ_SEL_SA_CHECK
     New reference switching server takes previous server value if server SA == 0.
 */
typedef enum {
   ZL303XX_APR_REF_SWITCH_FREQ_SEL_LEGACY=0,
   ZL303XX_APR_REF_SWITCH_FREQ_SEL_USER_VAL,
   ZL303XX_APR_REF_SWITCH_FREQ_SEL_SA_CHECK,
   ZL303XX_APR_REF_SWITCH_FREQ_SEL_LAST
} zl303xx_AprRefSwitchFreqSelectE;

/* See exampleUserDelayFunc() for details */
typedef Sint32T (*swFuncPtrUserDelay)(Uint32T delayRequiredMs, Uint64S startOfRunTime, Uint64S endofRunTime, Uint64S *lastStartTime);

/* An enum to define the different clock group types */
typedef enum {
    ZL303XX_APR_CLOCK_GROUP_1=0,
    ZL303XX_APR_CLOCK_GROUP_2A,
    ZL303XX_APR_CLOCK_GROUP_2B,
    ZL303XX_APR_CLOCK_GROUP_2H,
    ZL303XX_APR_CLOCK_GROUP_3,
    ZL303XX_APR_CLOCK_GROUP_4,
    ZL303XX_APR_CLOCK_GROUP_INVALID
} zl303xx_AprClockGroupE;

/****** Application Configuration, Initialization and Shutdown *****/

/*!  Struct: zl303xx_PFInitS
Parameters that can be configured when initialising a device for use by
the phase slope limit & frequency change limit components.
*/
typedef struct {
   char *PFTaskName;        /* DEPRECATED, kept for backward compilation purposes */
   Uint32T PFTaskPriority;  /* DEPRECATED, kept for backward compilation purposes */
   Uint32T PFTaskStackSize; /* DEPRECATED, kept for backward compilation purposes */
   Uint8T logLevel;         /* DEPRECATED, kept for backward compilation purposes */
   Uint8T numDevices;
   zl303xx_BooleanE useHardwareTimer;    /* Replaced by zl303xx_AprInitS.userDelayFunc */ /* DEPRECATED, kept for backward compilation purposes */
  swFuncPtrUserDelay userDelayFunc;    /* Replaced by zl303xx_AprInitS.userDelayFunc */ /* DEPRECATED, kept for backward compilation purposes */
} zl303xx_PFInitS;


/*!  Struct: zl303xx_AprSampleInitS
Parameters that can be configured when initialising a device for use by
the APR Sample components.
*/
typedef struct {
   const char *AprSampleTaskName;   /* DEPRECATED, kept for backward compilation purposes */
   Uint32T AprSampleTaskPriority;   /* DEPRECATED, kept for backward compilation purposes */
   Uint32T AprSampleTaskStackSize;  /* DEPRECATED, kept for backward compilation purposes */
   swFuncPtrUserDelay userDelayFunc;    /* Replaced by zl303xx_AprInitS.userDelayFunc */ /* DEPRECATED, kept for backward compilation purposes */
} zl303xx_AprSampleInitS;

/*!  Struct: zl303xx_AprInitS

Parameters that can be used for configuring the APR application to be
launched.

*/
typedef struct {
   const char *aprAdTaskName;   /* DEPRECATED, kept for backward compilation purposes */
   Uint32T aprAdTaskPriority;   /* DEPRECATED, kept for backward compilation purposes */
   Uint32T aprAdTaskStackSize;  /* DEPRECATED, kept for backward compilation purposes */

  /*!  Var: zl303xx_AprInitS::aprDelayTaskName
            APR task name. */
   const char *aprDelayTaskName;

   /*!  Var: zl303xx_AprInitS::aprDelayTaskPriority
            APR task priority. */
   Uint32T aprDelayTaskPriority;

   /*!  Var: zl303xx_AprInitS::aprDelayTaskStackSize
            APR task stack size. */
   Uint32T aprDelayTaskStackSize;

   /*!  Var: zl303xx_AprInitS::aprQIFTaskPriority
              APR task priority. */
   Uint32T aprQIFTaskPriority;

   /*!  Var: zl303xx_AprInitS::aprQIFTaskStackSize
              APR task stack size. */
   Uint32T aprQIFTaskStackSize;

   /*!  Var: zl303xx_AprInitS::logLevel
            APR log level at start up. APR log level indicates which detailed
            information to be logged. There are four log levels in APR from
            level 0 (least detailed information) to 3 (most detailed
            information). */
   Uint8T logLevel;

   /*!  Var: zl303xx_AprInitS::aprClockGroupType
            Startup clock group type */
   zl303xx_AprClockGroupE aprClockGroupType;

   /*!  Var: zl303xx_AprInitS::aprTimer1PeriodMs
            APR currently employs two timers for internal tasks.  This value
            is used to set the period in ms of the first software timer. */
   Uint32T aprTimer1PeriodMs;

   /*!  Var: zl303xx_AprInitS::aprTimer2PeriodMs
            APR currently employs two timers for internal tasks.  This value
            is used to set the period in ms of the second software timer. */
   Uint32T aprTimer2PeriodMs;

   /*!  Var: zl303xx_AprInitS::userDelayFunc
            APR defaults to a H/W timer for scheduling and sequencing tasks.  Optionally, a
            user function may be bound by this member. This will disable the creation
            of the H/W timer and an additional zlAprDelayTask will provide the scheduling using
            this binding. Default is NULL. */
   swFuncPtrUserDelay userDelayFunc;

   zl303xx_PFInitS PFInitParams;
   zl303xx_AprSampleInitS AprSampleInitParams;

   /*!  Var: zl303xx_AprInitS::aprNotifyTaskPriority
             APR notify task priority. */
   Uint32T aprNotifyTaskPriority;

   /*!  Var: zl303xx_AprInitS::aprNotifyTaskStackSize
              APR notify task stack size. */
   Uint32T aprNotifyTaskStackSize;


   /*!  Var: zl303xx_AprInitS::logLevelAutoPrintStruct
       APR log level to trigger automatic printing of configure structures
       from API calls. Use large number to disable automatic printing. Use
       0 to always print regardless of current `logLevel`. */
    Uint8T logLevelAutoPrintStruct;

} zl303xx_AprInitS;


/** zl303xx_AprInitStructInit

   Set up default parameters for the zl303xx_AprInit() function.

  Parameters:
   [in]  par            pointer to the structure for configuration items

  Return Value:
    ZL303XX_OK                       if successful
    ZL303XX_INVALID_POINTER          if par is NULL

*****************************************************************************/
zlStatusE zl303xx_AprInitStructInit(zl303xx_AprInitS *par);

/**  zl303xx_AprUpdateTimer1
 This function updates the values of
    1) zl303xx_AprTable::aprAprAdaptiveTimerPeriodMs
    2) zl303xx_AprTable::aprClockGroupType

    If aprClockGroupType is valid it automatically selects the proper zl303xx_AprTable::aprAprAdaptiveTimerPeriodMs

  Return Value:
    ZL303XX_OK                       if successful
        ZL303XX_INVALID_POINTER          if AprTable is NULL
 */

zlStatusE zl303xx_AprUpdateTimer1(zl303xx_AprClockGroupE *ClockGroupType, Uint32T *Timer1);

/** zl303xx_AprInit

   This function:
      - initialises APR data, and
      - starts the APR task.

   This function is called once to launch APR application based on the
   configuration data in the APR configuration structure.

  Parameters:
   [in]     par                     pointer to the APR configuration structure

  Return Value:
    ZL303XX_OK                       if successful
    ZL303XX_INVALID_POINTER          if par is NULL
    ZL303XX_MULTIPLE_INIT_ATTEMPT    if APR is already running
    ZL303XX_RTOS_MEMORY_FAIL         if memory required for launching APR could not
                                       be allocated
    ZL303XX_RTOS_SEMA4_CREATE_FAIL   internal error
    ZL303XX_RTOS_TASK_CREATE_FAIL    internal error

*****************************************************************************/
zlStatusE zl303xx_AprInit( zl303xx_AprInitS *par);


/*!  Struct: zl303xx_PFDeleteS

Parameters that can be configured for the phase slope
limit & frequency change limit components when shutdown APR application.

Currently, there are no such parameters.
*/
typedef struct {
   Uint8T unused;
} zl303xx_PFDeleteS;

/*!  Struct: zl303xx_AprSampleDeleteS

Parameters that can be configured for the APR Sample component when shutdown
APR application.

Currently, there are no such parameters.
*/
typedef struct {
   Uint8T unused;
} zl303xx_AprSampleDeleteS;

/*!  Struct: zl303xx_AprDeleteS

Parameters that can be configured when shutdown APR application.
*/
typedef struct {
    zl303xx_PFDeleteS PFDeleteParams;
    zl303xx_AprSampleDeleteS AprSampleDeleteParams;
} zl303xx_AprDeleteS;


/** zl303xx_AprDeleteStructInit

   Set up default parameters for the zl303xx_AprDelete() function

  Parameters:
   [in]  par            pointer to the structure for configuration items

  Return Value:
    ZL303XX_OK                       if successful
    ZL303XX_INVALID_POINTER          if either hwParams or par is NULL

 *****************************************************************************/
zlStatusE zl303xx_AprDeleteStructInit(zl303xx_AprDeleteS *par);


/** zl303xx_AprDelete

   This function deletes the APR task, queue and semaphore.


  Parameters:
   [in]  par               pointer to the structure for configuration items

  Return Value:
    ZL303XX_OK                       if successful
    ZL303XX_INVALID_POINTER          if either hwParams or par is NULL
    ZL303XX_NOT_RUNNING              if zl303xx_InitDevice() has not been called
                                       i.e. other tasks and data have not been
                                       initialised yet.
    ZL303XX_RTOS_SEMA4_NOT_CREATED   if the APR table mutex has not been created
    ZL303XX_RTOS_SEMA4_TAKE_FAIL     internal error
    ZL303XX_RTOS_SEMA4_DELETE_FAIL   if the APR table mutex or semaphore could
                                       not be deleted
    ZL303XX_INIT_ERROR               if an ISR has not been attached (this
                                       indicates general initialisation problem)
    ZL303XX_PARAMETER_INVALID        internal error
    ZL303XX_CONTEXT_NOT_IN_USE       internal error
    ZL303XX_RTOS_TASK_DELETE_FAIL    if deleting the APR task failed

****************************************************************************/
zlStatusE zl303xx_AprDelete(zl303xx_AprDeleteS *par);


/** zl303xx_AprIsAprRunning

   Returns TRUE if APR is running, FALSE otherwise.

*****************************************************************************/
zl303xx_BooleanE zl303xx_AprIsAprRunning(void);



/*!  Enum: zl303xx_AprDeviceRefModeE
  Reference mode for the device to be registered with APR. */
/*!  Var: zl303xx_AprDeviceRefModeE ZL303XX_PACKET_MODE
  Indication that the device is synchronised to a packet reference. It's the
  default value of APR, but it can be changed by calling zl303xx_AprSetDeviceOptMode() */
/*!  Var: zl303xx_AprDeviceRefModeE ZL303XX_HYBRID_MODE
  Indication that the device is synchronised to a hybrid reference. */
/*!  Var: zl303xx_AprDeviceRefModeE ZL303XX_ELECTRIC_MODE
  Indication that the device is synchronised to a electrical reference. */
typedef enum
{
   ZL303XX_PACKET_MODE = 0,
   ZL303XX_HYBRID_MODE,
   ZL303XX_ELECTRIC_MODE,
   ZL303XX_NOT_SUPPORT_DEV_REF_MODE
} zl303xx_AprDeviceRefModeE;

typedef zl303xx_AprDeviceRefModeE zl303xx_AprServerRefModeE;

/*!  Enum: zl303xx_AprDeviceHybridAdjModeE
  Hybrid mode adjustment method enum. This enum is used by the APR algorithm
  for performing adjustment calculations and selecting the specific callout function. */
/*!  Var: zl303xx_AprDeviceHybridAdjModeE ZL303XX_HYBRID_ADJ_PHASE
  Perform Hybrid adjustment using phase adjustments.  To be used only with Micorsemi
  devices supporting phase adjustments. */
/*!  Var: zl303xx_AprDeviceHybridAdjModeE ZL303XX_HYBRID_ADJ_FREQ
  Perform Hybrid adjustment using frequency adjustments.  To be used only with Micorsemi
  devices supporting frequency adjustments. */
/*!  Var: zl303xx_AprDeviceHybridAdjModeE ZL303XX_HYBRID_ADJ_CUSTOM_PHASE
  Perform Hybrid adjustment using phase adjustments.  Calls adjustPhase callout function.  */
/*!  Var: zl303xx_AprDeviceHybridAdjModeE ZL303XX_HYBRID_ADJ_CUSTOM_FREQ
  Perform Hybrid adjustment using frequency adjustments.  Calls adjustFreq callout function.  */
  /*!  Var: zl303xx_AprDeviceHybridAdjModeE ZL303XX_HYBRID_ADJ_NONE
  Other modules should be performing the device adjustments, do nothing.  */
typedef enum
{
   ZL303XX_HYBRID_ADJ_PHASE = 0,
   ZL303XX_HYBRID_ADJ_FREQ,
   ZL303XX_HYBRID_ADJ_CUSTOM_PHASE,
   ZL303XX_HYBRID_ADJ_CUSTOM_FREQ,
   ZL303XX_HYBRID_ADJ_NONE
} zl303xx_AprDeviceHybridAdjModeE;

/*!  Enum: zl303xx_AprAlgTypeModeE
  Algorithm type mode for the server to be registered with APR. */
/*!  Var: zl303xx_AprAlgTypeModeE ZL303XX_NATIVE_PKT_FREQ
  It's the default value of APR, but it can be changed by calling zl303xx_AprSetAlgTypeMode() */
/*!  Var: zl303xx_AprAlgTypeModeE ZL303XX_NATIVE_PKT_FREQ_UNI */
/*!  Var: zl303xx_AprAlgTypeModeE ZL303XX_NATIVE_PKT_FREQ_CES */
/*!  Var: zl303xx_AprAlgTypeModeE ZL303XX_NATIVE_PKT_FREQ_ACCURACY */
/*!  Var: zl303xx_AprAlgTypeModeE ZL303XX_NATIVE_PKT_FREQ_ACCURACY_UNI */
/*!  Var: zl303xx_AprAlgTypeModeE ZL303XX_NATIVE_PKT_FREQ_FLEX */
/*!  Var: zl303xx_AprAlgTypeModeE ZL303XX_BOUNDARY_CLK */
/*!  Var: zl303xx_AprAlgTypeModeE ZL303XX_NATIVE_PKT_FREQ_ACCURACY_FDD */
/*!  Var: zl303xx_AprAlgTypeModeE ZL303XX_XDSL_FREQ_ACCURACY */
/*!  Var: zl303xx_AprAlgTypeModeE ZL303XX_CUSTOM_FREQ_ACCURACY_200 */
/*!  Var: zl303xx_AprAlgTypeModeE ZL303XX_CUSTOM_FREQ_ACCURACY_15 */
typedef enum
{
    ZL303XX_NATIVE_PKT_FREQ                    = 0,
    ZL303XX_NATIVE_PKT_FREQ_UNI                = 1,
    ZL303XX_NATIVE_PKT_FREQ_CES                = 2,
    ZL303XX_NATIVE_PKT_FREQ_ACCURACY           = 3,
    ZL303XX_NATIVE_PKT_FREQ_ACCURACY_UNI       = 4,
    ZL303XX_NATIVE_PKT_FREQ_FLEX               = 5,
    ZL303XX_BOUNDARY_CLK                       = 6,
    ZL303XX_NATIVE_PKT_FREQ_ACCURACY_FDD       = 7,
    ZL303XX_XDSL_FREQ_ACCURACY                 = 8,
    ZL303XX_CUSTOM_FREQ_ACCURACY_200           = 9,
    ZL303XX_CUSTOM_FREQ_ACCURACY_15            = 10,
   /* ZL303XX_NATIVE_PKT_FREQ_FLEX_T2            = 11,
    ZL303XX_BOUNDARY_CLK_T2                    = 12,
    ZL303XX_NATIVE_PKT_FREQ_ACCURACY_FDD_T2    = 13, */
    ZL303XX_APR_LAST_SUPPORT_ALG_TYPE          = ZL303XX_CUSTOM_FREQ_ACCURACY_15
} zl303xx_AprAlgTypeModeE;

/*!  Enum: zl303xx_AprTsFormatE
  Timestamp format for a packet server to be registered with APR. */
/*!  Var: zl303xx_AprTsFormatE ZL303XX_APR_TS_PTP
 */
/*!  Var: zl303xx_AprTsFormatE ZL303XX_APR_TS_RTP
 */
/*!  Var: zl303xx_AprTsFormatE ZL303XX_APR_TS_NTP
 */
typedef enum
{
   ZL303XX_APR_TS_PTP = 0,
   ZL303XX_APR_TS_RTP,
   ZL303XX_APR_TS_NTP,

   ZL303XX_APR_NOT_SUPPORT_TS_FORMAT
} zl303xx_AprTsFormatE;


typedef enum
{
    ZL303XX_TCXO                      = 0,
    ZL303XX_TCXO_FAST                 = 1,
    ZL303XX_OCXO_S3E                  = 2,
    ZL303XX_OCXO_S3E_C                = 3,
    ZL303XX_OCXO_S3E_DEPRECATED       = 4,
    ZL303XX_OCXO_S3E_C_DEPRECATED     = 5,
    ZL303XX_TCXO_FAST_ENHANCED        = 6,
    ZL303XX_TCXO_FAST_CABLE           = 7,
    ZL303XX_DEFAULT_XO                = ZL303XX_TCXO,
    ZL303XX_APR_LAST_SUPPORT_OSCI_FILTER = ZL303XX_TCXO_FAST_CABLE
} zl303xx_AprOscillatorFilterTypesE;


typedef enum
{
    ZL303XX_BW_0_FILTER   = 0,
    ZL303XX_BW_1_FILTER,
    ZL303XX_BW_2_FILTER,
    ZL303XX_BW_3_FILTER,
    ZL303XX_BW_4_FILTER,
    ZL303XX_BW_5_FILTER,
    ZL303XX_BW_6_FILTER,
    ZL303XX_BW_7_FILTER,
    ZL303XX_BW_8_FILTER,
    ZL303XX_BW_700mHz,
    ZL303XX_BW_300mHz,
    ZL303XX_BW_100mHz,
    ZL303XX_BW_90mHz,
    ZL303XX_BW_75mHz,
    ZL303XX_BW_51mHz,
    ZL303XX_BW_30mHz,
    ZL303XX_BW_10mHz,
    ZL303XX_BW_3mHz,
    ZL303XX_BW_1mHz,
    ZL303XX_BW_300uHz,
    ZL303XX_BW_100uHz,
    ZL303XX_BW_33uHz,
    ZL303XX_BW_23uHz,
    ZL303XX_BW_700uHz,
    ZL303XX_BW_1100mHz,
    ZL303XX_BW_500mHz,
    ZL303XX_LAST_SUPPORT_FILTER = ZL303XX_BW_500mHz   /* ZL303XX_BW_500mHz */
}zl303xx_AprFilterTypesE;


/* Negative values represent seconds per packet. */
typedef enum
{
   ZL303XX_MORE_THAN_128_PPS = 129,
   ZL303XX_128_PPS           = 128,
   ZL303XX_64_PPS            = 64,
   ZL303XX_32_PPS            = 32,
   ZL303XX_16_PPS            = 16,
   ZL303XX_8_PPS             = 8,
   ZL303XX_4_PPS             = 4,
   ZL303XX_2_PPS             = 2,
   ZL303XX_1_PPS             = 1,
   ZL303XX_0_PPS             = 0,
   ZL303XX_1_PP_2S           = -2,
   ZL303XX_1_PP_4S           = -4,
   ZL303XX_1_PP_8S           = -8,
   ZL303XX_1_PP_16S          = -16

} zl303xx_AprPktRateE;

/* The following defines the phase step calculations treatment */
typedef enum
{
   ZL303XX_DISABLE_STEP_DETECT_CALC_RESET = 0,
   ZL303XX_ENABLE_STEP_DETECT_CALC_RESET = 1,
   ZL303XX_DEFAULT_STEP_DETECT_CALC = ZL303XX_DISABLE_STEP_DETECT_CALC_RESET

}zl303xx_AprStepDetectCalcE;

/* Common oscHoldoverStability defines */
#define ZL303XX_USING_DEFAULT_HOLDOVER_STABILITY              0
#define ZL303XX_USING_DEFAULT_XO_SMODE_TIMEOUT                0
#define ZL303XX_USING_DEFAULT_XO_SMODE_AGEOUT                 0
#define ZL303XX_USING_DEFAULT_RE_ROUTE_HOLDOVER2              0

/* Type_2b minimum fastlock phase limit, below which fast lock exists */
#define ZL303XX_TYPE2B_FASTLOCK_MIN_PHASE 8


/** Maximum frequency offset that will be applied during steady state. If 1Hz is
 *  enabled, it may apply a frequency offset beyond these limits. */
typedef enum
{
   /** +/- 50 ppb */
   ZL303XX_APR_PIR_50_PPB  = 50,
   /** +/- 4.6 ppm */
   ZL303XX_APR_PIR_4P6_PPM = 4600,
   /** +/- 9.2 ppm */
   ZL303XX_APR_PIR_9P2_PPM = 9200,
   /** +/- 11 ppm */
   ZL303XX_APR_PIR_11_PPM  = 11000,
   /** +/- 12 ppm */
   ZL303XX_APR_PIR_12_PPM  = 12000,
   /** +/- 50 ppm */
   ZL303XX_APR_PIR_50_PPM  = 50000,

   ZL303XX_APR_PIR_MIN     = ZL303XX_APR_PIR_50_PPB,
  /* ZL303XX_APR_PIR_MAX     = ZL303XX_APR_PIR_50_PPM,*/
    ZL303XX_APR_PIR_MAX     = 200000,

   ZL303XX_APR_PIR_INVALID = 0x7fffffff
} zl303xx_AprPullInRangeE;

/*!  Enum: zl303xx_AprApsTypesE
  Internal use. */
typedef enum
{
   ZL303XX_APR_APS_0 = 0,
   ZL303XX_APR_APS_1,
   ZL303XX_APR_APS_2,
   ZL303XX_APR_APS_INIT
} zl303xx_AprApsTypesE;


/*!  Enum: zl303xx_AprDefaultCGUTypesE
  indexes into defaultCGU[] array in zl303xx_AprAddDeviceS. */
typedef enum {
   ZL303XX_ADCGU_SET_TIME,
   ZL303XX_ADCGU_STEP_TIME,
   ZL303XX_ADCGU_ADJUST_TIME,
   ZL303XX_ADCGU_ADJUST_FREQ,
   ZL303XX_ADCGU_GET_HW_FREQ_OFFSET,
   ZL303XX_ADCGU_GET_HW_LOCK_STATUS,
   ZL303XX_ADCGU_GET_TAKE_HW_DCO_CONTROL,
   ZL303XX_ADCGU_RETURN_HW_DCO_CONTROL,
   ZL303XX_ADCGU_GET_HW_MANUAL_HOLDOVER,
   ZL303XX_ADCGU_GET_HW_MANUAL_FREERUN,
   ZL303XX_ADCGU_GET_HW_SYNC_INPUT_EN,
   ZL303XX_ADCGU_GET_HW_OUT_OF_RANGE,
   ZL303XX_ADCGU_REF_SW_HITLESS_COMP,
   ZL303XX_ADCGU_ENTERHOSTATECHANGE,
   ZL303XX_ADCGU_EXITSTATECHANGE,

   ZL303XX_ADCGU_LAST
} zl303xx_AprDefaultCGUTypesE;


/*!  ZL303XX_NULL_MSG_SERVER_ID
When the server ID is unnecessary, this NULL value should be put in the message.
 */
#define ZL303XX_NULL_MSG_SERVER_ID (0xffff)

typedef enum {
   zl303xx_RMT_jumpTimeData,
   zl303xx_RMT_adjFreqData,
   zl303xx_RMT_jumpStandbyCGU,
   zl303xx_RMT_cData,
   zl303xx_RMT_stateInfo,
   zl303xx_RMT_last
} zl303xx_RedundancyMsgTypesE;

typedef enum {
   zl303xx_JTT_setTime,
   zl303xx_JTT_stepTime,
   zl303xx_JTT_adjTimeStart,
   zl303xx_JTT_adjTimeEnd
} zl303xx_JumpTimeTypesE;

typedef struct {
   zl303xx_JumpTimeTypesE jumpTimeType;
   Uint64S dtSec;
   Uint32T dtNanoSec;
   zl303xx_BooleanE negative;
   Uint32T recommendedTime;
} zl303xx_JumpTimeDataS;

typedef struct {
   Uint64S dtSec;
   Uint32T dtNanoSec;
   zl303xx_BooleanE negative;
} zl303xx_JumpStandbyCGUDataS;

typedef struct {
   Sint32T freq;
   Sint32T duration;
} zl303xx_AdjFreqDataS;

typedef struct {
   zl303xx_BooleanE OInc;
   Sint32T O;
   zl303xx_BooleanE PFInc;
   Sint32T PF;
} zl303xx_CDataS;

typedef struct {
   Uint64S phaseAdj;
} zl303xx_StateInfoS;

typedef union {
   zl303xx_JumpTimeDataS jumpTimeData;
   zl303xx_AdjFreqDataS adjFreqData;
   zl303xx_JumpStandbyCGUDataS jumpStandbyCGUData;
   zl303xx_CDataS cData;
   zl303xx_StateInfoS stateInfo;
} zl303xx_RedundancyDataS;

typedef struct {
   zl303xx_RedundancyMsgTypesE msgType;
   Uint32T subMsgType;
   zl303xx_RedundancyDataS rd;
} zl303xx_RedundancyMsgS;

/* Hardware dependent function types
 *
 * The device specific functions defined in the following types should be provided
 * by users.
 */
/*
 * Standard interface bindings:
 * Set the Time-Of-Day of the timestamper
   zlStatusE zl303xx_AprUsrFnSetTime(void *clkGen, Uint64S deltaSecs, Uint32T deltaNS, zl303xx_BooleanE bBackwardAdjust)
 * Step the phase-offset of the timestamper (for 1Hz phase alignment)
   zlStatusE zl303xx_AprUsrFnStepTime(void *clkGen, Sint32T deltaTime)
 * Adjust the freq of the timestamper
   zlStatusE zl303xx_AprUsrFnDcoSetFreq(void *clkGen, Sint32T freqOffsetInPartsPerTrillion)
*/
/* Hybrid interface bindings:
   zlStatusE zl303xx_AprUsrFnTakeHwDcoControl(void *clkGen) / zl303xx_AprUsrFnGiveHwDcoControl(void *clkGen)
*/

typedef Sint32T (*hwFuncPtrSetTime)(void*, Uint64S, Uint32T, zl303xx_BooleanE);
typedef Sint32T (*hwFuncPtrAdjustClk)(void*, Sint32T);
typedef Sint32T (*hwFuncPtrAdjustTime)(void*, Sint32T, Uint32T);
typedef Sint32T (*hwFuncPtrGetClkInfo)(void*, Sint32T*);
typedef Sint32T (*hwFuncPtrGeneral)(void*);
typedef Sint32T (*hwFuncPtrTypeTwo)(void*, Sint32T, Sint32T);
typedef Sint32T (*hwFuncPtrTypeOne)(void*, Uint32T);

/* hwFuncPtrJumpTimeTSU

   An instance of this routine prototype is supplied by the user to re-align
   the TSU with the 1Hz pulse.

   This routine is called after adjustTime() is called.

  Parameters:
   [in]  hwParams       Pointer to the device structure.
   [in]  adjustment     The adjustment made by adjustTime()

  Return Value:  0     Success

****************************************************************************/
typedef Sint32T (*hwFuncPtrJumpTimeTSU)
                     (
                     void* hwParams,
                     Uint64S seconds,
                     Uint32T nanoSeconds,
                     zl303xx_BooleanE bBackwardAdjust
                     );

/* Jump event type */
typedef enum {
   ZL303XX_JET_START,
   ZL303XX_JET_END
} zl303xx_JumpEvent_t;


/* swFuncPtrJumpNotification

   An instance of this routine prototype is supplied by the user to notify
   the user that a 1Hz jump is either about to start or end.

   This routine is called before setTime, stepTime, or adjTime.

  Parameters:
   [in]  hwParams          Pointer to the device structure.
   [in]  seconds           The second size of the adjustment
   [in]  nanoSeconds       The nanosecond size of the adjustment
   [in]  bBackwardAdjust   TRUE if the adjustment is negative
   [in]  jumpEvent         zl303xx_JumpEvent_t indicating the notification type

  Return Value:  0     Success

****************************************************************************/
typedef Sint32T (*swFuncPtrJumpNotification)
                     (
                     void* hwParams,
                     Uint64S seconds,
                     Uint32T nanoSeconds,
                     zl303xx_BooleanE bBackwardAdjust,
                     zl303xx_JumpEvent_t jumpEvent
                     );

/* swFuncPtrSendRedundancyData

   An instance of this routine prototype is supplied by the user to send
   redundancy data to the monitor.

  Parameters:
   [in]  hwParams          Pointer to the device structure.
   [in]  serverId          The server ID
   [in]  msg               The message data

  Return Value:  0     Success

****************************************************************************/
typedef Sint32T (*swFuncPtrSendRedundancyData)
                     (
                     void* hwParams,
                     Uint16T serverId,
                     zl303xx_RedundancyMsgS *msg
                     );

/* swFuncPtrJumpActiveCGU

   An instance of this routine prototype is supplied by the user to jump the
   active CGU.

  Parameters:
   [in]  hwParams       Pointer to the device structure.
   [in]  seconds           The second size of the adjustment
   [in]  nanoSeconds       The nanosecond size of the adjustment
   [in]  bBackwardAdjust   TRUE if the adjustment is negative

  Return Value:  0     Success

****************************************************************************/
typedef Sint32T (*swFuncPtrJumpActiveCGU)
                     (
                     void* hwParams,
                     Uint64S seconds,
                     Uint32T nanoSeconds,
                     zl303xx_BooleanE bBackwardAdjust
                     );

/* swFuncPtrJumpStandbyCGU

   An instance of this routine prototype is supplied by the user to jump the
   standby (monitor) CGU.

  Parameters:
   [in]  hwParams       Pointer to the device structure.
   [in]  seconds           The second size of the adjustment
   [in]  nanoSeconds       The nanosecond size of the adjustment
   [in]  bBackwardAdjust   TRUE if the adjustment is negative

  Return Value:  0     Success

****************************************************************************/
typedef Sint32T (*swFuncPtrJumpStandbyCGU)
                     (
                     void* hwParams,
                     Uint64S seconds,
                     Uint32T nanoSeconds,
                     zl303xx_BooleanE bBackwardAdjust
                     );


/*
   zlStatusE zl303xx_AprUsrFnGetTimeStamperCGUOffset(void* hwParams, Uint32T TSUid, Uint64S *secOffset, Sint32T *nsOffset)
*/
typedef Sint32T (*hwFuncPtrGetTimeStamperCGUOffset)(void*, Uint32T, Uint64S*, Sint32T*);

/** Values that can be configured to be passed into the refSwitchHitlessCompensation()
 *  callback. */
typedef enum
{
   /** Do not perform hitless compensation. */
   ZL303XX_APR_HITLESS_COMP_FALSE,
   /** Always perform hitless compensation. */
   ZL303XX_APR_HITLESS_COMP_TRUE,
   /** Perform hitless compensation based on other criteria. The default callback
    *  function uses the DPLL TIE clear setting to determine if compensation
    *  should be performed. */
   ZL303XX_APR_HITLESS_COMP_AUTO,

   ZL303XX_APR_HITLESS_COMP_NUM_TYPES
} zl303xx_AprHitlessCompE;

typedef Sint32T (*hwFuncPtrHitlessComp)(void *, zl303xx_AprHitlessCompE, zl303xx_BooleanE *);

/***************  PSL&FCL DATA TYPES AND STRUCTURES  **************************/

/*!  Enum: zl303xx_TIEClearE
  TIE Clear types
 */
/*!  Var: zl303xx_TIEClearE:TC_disabled
 */
/*!  Var: zl303xx_TIEClearE:TC_enabled
 */
typedef enum {
   TC_disabled   = 0,
   TC_enabled    = 1,
   TC_last       = 2
} zl303xx_TIEClearE;

/*!  Enum: zl303xx_TIEClearEventsE
  TIE Clear types
 */
/*!  Var: zl303xx_TIEClearEventsE::TCE_newServer
 */
/*!  Var: zl303xx_TIEClearEventsE::TCE_newMode
 */
/*!  Var: zl303xx_TIEClearEventsE::TCE_BMCAChangeSameServer
 */
typedef enum {
   TCE_newServer              = 0,
   TCE_newMode                = 1,
   TCE_BMCAChangeSameServer   = 2,
   TCE_last                   = 3
} zl303xx_TIEClearEventsE;

/*!  Enum: zl303xx_SetOutputOffsetActionE
  setOutputOffset action types
 */
/*!  Var: zl303xx_SetOutputOffsetActionE SOO_immediate
 */
/*!  Var: zl303xx_SetOutputOffsetActionE SOO_next1Hz
 */
typedef enum {
   SOO_immediate  = 0,
   SOO_next1Hz    = 1,
   SOO_last       = 2
} zl303xx_SetOutputOffsetActionE;

/*!  Enum: zl303xx_PFModeE
  Mode of PFM (Phase slope limit and Frequency change limit Module. */
/*!  Var: zl303xx_PFModeE PFM_normal
 */
/*!  Var: zl303xx_PFModeE PFM_hybrid
 */
typedef enum {
   PFM_normal,
   PFM_hybrid,
   PFM_last
} zl303xx_PFModeE;

/* zl303xx_AprPhaseSlopeLimitE */
/*
   These values determine the phase slope limit that will be applied to the
   current packet reference of a hardware device. This is the maximum rate of phase
   change a hardware device will apply when pulling in a phase offset with respect
   to the master. Unit: ns/sec */
typedef enum
{
   /** 25 ns/s */
   APR_PSL_25_NS = 25,

   /** 512 ns/s */
   APR_PSL_512_NS = 512,

   /** 885 ns/s */
   APR_PSL_885_NS = 885,

   /** 7.5 us/s */
   APR_PSL_7P5_US = 7500,

   /** 61 us/s */
   APR_PSL_61_US = 61000,

   /** Unlimited = 1 s/s */
   APR_PSL_UNLIMITED = 1000000000,

   APR_PSL_MAX = APR_PSL_UNLIMITED
} zl303xx_AprPhaseSlopeLimitE;

/* zl303xx_AprFrequencyChangeLimitE */
/*
   These values determine the frequency change limit that will be applied to the
   current packet reference of a hardware device. This is the maximum rate of
   frequency change a hardware device will apply when locking to the master.
   Unit: ppt/sec */
typedef enum
{
   /** 100 ppb/s */
   APR_FCL_100_PPB = 100000,

   /** 2.9 ppm/s */
   APR_FCL_2P9_PPM = 2900000,

   /** Disabled, 7 ppm/s */
   APR_FCL_DISABLED = 7000000,

   APR_FCL_MAX = APR_FCL_DISABLED
} zl303xx_AprFrequencyChangeLimitE;


/* The method that an adjustment can be applied with. */
typedef enum {
   zl303xx_OAT_default     = 0,
   zl303xx_OAT_useAdjFreq  = 1,
   zl303xx_OAT_useAdjTime  = 2
} zl303xxoverrideAdjustmentTypeE;

/*!  Enum: zl303xxfirstPacketTreatmentE
     Phase adjustment operation to perform on receiving of first
     packet from new server.
*/
/*!  Var: zl303xxfirstPacketTreatmentE::ZL303XX_FIRST_PACKET_TREATMENT_SEC_ONLY
     Perform a phase adjustment operation with the seconds offset only.
*/
/*!  Var: zl303xxfirstPacketTreatmentE::ZL303XX_FIRST_PACKET_TREATMENT_SEC_AND_NS
     Perform a phase adjustment operation with the seconds and ns fields.
*/
/*!  Var: zl303xxfirstPacketTreatmentE::ZL303XX_FIRST_PACKET_TREATMENT_NONE
     Do not force a phase adjustment operation.  Wait for the main algorithm to handle
     the server phase offset.
*/
typedef enum {
   ZL303XX_FIRST_PACKET_TREATMENT_SEC_ONLY  = 0,
   ZL303XX_FIRST_PACKET_TREATMENT_SEC_AND_NS = 1,
   ZL303XX_FIRST_PACKET_TREATMENT_NONE     = 2
} zl303xxfirstPacketTreatmentE;


/*!  Enum: zl303xx_AprStepTimeOptionsE
     Additional options for StepTime opeations.
*/
/*!  Var: zl303xx_AprStepTimeOptionsE::ZL303XX_APR_STEPTIME_OPTIONS_NONE
     No Additional logic applied.
*/
/*!  Var: zl303xx_AprStepTimeOptionsE::ZL303XX_APR_STEPTIME_OPTIONS_POS_ONLY
     Force only positive StepTime operations.  This option is used for certain device and system configurations.
*/
typedef enum {
   ZL303XX_APR_STEPTIME_OPTIONS_NONE     = 0,
   ZL303XX_APR_STEPTIME_OPTIONS_POS_ONLY = 1,
   ZL303XX_APR_STEPTIME_OPTIONS_LAST     = 2
} zl303xx_AprStepTimeOptionsE;

/* swFuncPtrAdjustTimeDR

   An instance of this function pointer may supplied by the user. That instance
   is called before adjustTime() is called.

  Parameters:
   [in]  hwParams    Pointer to the device structure.
   [in]  adjustment  The adjustment size in nanoseconds
   [out] dr          Either TRUE or FALSE


  Return Value:  0     Success

****************************************************************************/
typedef Sint32T (*swFuncPtrAdjustTimeDR)(void *hwParams,
                              Sint32T adjustment,
                              zl303xx_BooleanE *dr);

/* Redundancy sequence types */
typedef enum {
   zl303xx_JTS_endOfSequence,
   zl303xx_JTS_legacyBehaviour,
   zl303xx_JTS_jumpStartNotification,
   zl303xx_JTS_callTN,
   zl303xx_JTS_sendJumpTimeInfo,
   zl303xx_JTS_jumpActiveCGU,
   zl303xx_JTS_sendJumpStandbyCGU,
   zl303xx_JTS_jumpTSU,
   zl303xx_JTS_jumpEndNotification,
   zl303xx_JTS_delay_A,
   zl303xx_JTS_delay_B,
   zl303xx_JTS_delay_C,
   zl303xx_JTS_delay_D,
   zl303xx_JTS_delay_E,
   zl303xx_JTS_delay_F,
   zl303xx_JTS_waitForRollOverPacket,
   zl303xx_JTS_startPollTimer,
   zl303xx_JTS_LAST
}zl303xxjumpTimeSequence_E;

#define ZL303XX_MAX_JUMP_TIME_SEQUENCE_STEPS (20)

#define ZL303XX_DEFAULT_STATIC_OFFSET 0
#define ZL303XX_DEFAULT_APR_FREQ_PSL (APR_PSL_885_NS)
#define ZL303XX_DEFAULT_APR_FREQ_FAST_PSL (APR_PSL_885_NS)
#define ZL303XX_DEFAULT_PSL_TIMEOUT_PERIOD_SEC (100)
#define ZL303XX_DEFAULT_PSL_TIMEOUT_EXIT_DEBOUNCE_PERIOD_SEC (10)
#define ZL303XX_MAX_NUM_PSL_LIMITS (5)
#define ZL303XX_DEFAULT_ADJ_SIZE_1HZ_PSL_0 (1000)
#define ZL303XX_DEFAULT_ADJ_SIZE_1HZ_PSL_1 (4000)
#define ZL303XX_DEFAULT_ADJ_SIZE_1HZ_PSL_2 (10000)
#define ZL303XX_DEFAULT_ADJ_SIZE_1HZ_PSL_3 (0)
#define ZL303XX_DEFAULT_ADJ_SIZE_1HZ_PSL_4 (0)
#define ZL303XX_DEFAULT_PSL_1HZ_0 (4)
#define ZL303XX_DEFAULT_PSL_1HZ_1 (APR_PSL_885_NS)
#define ZL303XX_DEFAULT_PSL_1HZ_2 (4000)
#define ZL303XX_DEFAULT_PSL_1HZ_3 (0)
#define ZL303XX_DEFAULT_PSL_1HZ_4 (0)

#define ZL303XX_MAX_NUM_FCL_LIMITS (5)
#define ZL303XX_DEFAULT_APR_FREQ_FCL (APR_FCL_MAX)
#define ZL303XX_DEFAULT_ADJ_SIZE_1HZ_FCL_0 (100)
#define ZL303XX_DEFAULT_ADJ_SIZE_1HZ_FCL_1 (1000)
#define ZL303XX_DEFAULT_ADJ_SIZE_1HZ_FCL_2 (10000)
#define ZL303XX_DEFAULT_ADJ_SIZE_1HZ_FCL_3 (100000)
#define ZL303XX_DEFAULT_ADJ_SIZE_1HZ_FCL_4 (1000000)
#define ZL303XX_DEFAULT_FCL_1HZ_0 (APR_FCL_MAX)
#define ZL303XX_DEFAULT_FCL_1HZ_1 (APR_FCL_MAX)
#define ZL303XX_DEFAULT_FCL_1HZ_2 (APR_FCL_MAX)
#define ZL303XX_DEFAULT_FCL_1HZ_3 (APR_FCL_MAX)
#define ZL303XX_DEFAULT_FCL_1HZ_4 (APR_FCL_MAX)

#define ZL303XX_MAX_NUM_ADJ_SCALING_LIMITS (5)
#define ZL303XX_DEFAULT_ADJ_SIZE_1HZ_ADJ_SCALING_0 (1000)
#define ZL303XX_DEFAULT_ADJ_SIZE_1HZ_ADJ_SCALING_1 (10000)
#define ZL303XX_DEFAULT_ADJ_SIZE_1HZ_ADJ_SCALING_2 (100000)
#define ZL303XX_DEFAULT_ADJ_SIZE_1HZ_ADJ_SCALING_3 (1000000)
#define ZL303XX_DEFAULT_ADJ_SIZE_1HZ_ADJ_SCALING_4 (10000000)
#define ZL303XX_DEFAULT_ADJ_SCALING_1HZ_0 (1000)
#define ZL303XX_DEFAULT_ADJ_SCALING_1HZ_1 (1000)
#define ZL303XX_DEFAULT_ADJ_SCALING_1HZ_2 (1000)
#define ZL303XX_DEFAULT_ADJ_SCALING_1HZ_3 (1000)
#define ZL303XX_DEFAULT_ADJ_SCALING_1HZ_4 (1000)


/*!  Struct: zl303xx_PFConfigS

These are the configurable parameters for the phase slope limit and
frequency change limit software components.
*/
typedef struct {

   /*!  Var: zl303xx_PFConfigS::PFMode
            The mode of PFM: either normal or hybrid */
    zl303xx_PFModeE PFMode;

   /*!  Var: zl303xx_PFConfigS::setTimeStepTimeThreshold
            This threshold determines when to use setTime() or stepTime() for a
            given phase adjustment. Any phase adjustment greater than this threshold
            will use setTime() first, then, if necessary, use stepTime().
            Units: ns */
    Uint32T setTimeStepTimeThreshold;

   /*!  Var: zl303xx_PFConfigS::stepTimeAdjustTimeThreshold
            This threshold determines when to use stepTime() or adjustTime() for
            a given phase adjustment. Any phase adjustment greater than this threshold
            will use stepTime() first, then, if necessary, use adjustTime().
            Units: ns */
    Uint32T stepTimeAdjustTimeThreshold;

   /*!  Var: zl303xx_PFConfigS::stepTimeAdjustFreqThreshold
            This threshold determines when to use stepTime() or adjustFreq() for a
            given phase adjustment. Any phase adjustment greater than this threshold
            will use stepTime() first, then, if necessary, use adjustFreq().
            A value of 0 is a special case such that adjustFreq() will not be called.
            Units: ns */
    Uint32T stepTimeAdjustFreqThreshold;

   /*!  Var: zl303xx_PFConfigS::setTimeResolution
            The resolution of setTime().
            Units: ns */
    Uint32T setTimeResolution;

   /*!  Var: zl303xx_PFConfigS::stepTimeResolution
            The resolution of stepTime().
            Units: ns */
    Uint32T stepTimeResolution;

   /*!  Var: zl303xx_PFConfigS::stepTimeMaxTimePerAdjustment
            stepTime() can apply large steps with multiple, small steps. This var
            specifies the maximum each small step can be. The start of each small
            step is separated by stepTimeExecutionTime milliseconds.
            Units: ns
            Default: 490000000 ns */
    Uint32T stepTimeMaxTimePerAdjustment;

   /*!  Var: zl303xx_PFConfigS::setTimeExecutionTime
            The execution time that the target hardware needs to perform setTime().
            Units: milliseconds */
    Uint32T setTimeExecutionTime;

   /*!  Var: zl303xx_PFConfigS::stepTimeExecutionTime
            The execution time that the target hardware needs to perform stepTime().
            Units: milliseconds */
    Uint32T stepTimeExecutionTime;

   /*!  Var: zl303xx_PFConfigS::adjustTimeExecutionTime
            The execution time that the target hardware needs to perform adjustTime().
            Units: milliseconds */
    Uint32T adjustTimeExecutionTime;

   /*!  Var: zl303xx_PFConfigS::adjustTimeMinThreshold
            If an adjustment is smaller than adjustTimeMinThreshold, then it is
            not applied.
            Units: ns
            Default: 20ns */
    Uint32T adjustTimeMinThreshold;

   /*!  Var: zl303xx_PFConfigS::adjustTimeDR
            The function pointer for the device specific function provided by the user
            that returns TRUE or FALSE */
    swFuncPtrAdjustTimeDR adjustTimeDR;

   /*!  Var: zl303xx_PFConfigS::bUseAdjustTimeHybrid
            Flag to indicate whether or not to use adjustTime() for hybrid mode.
            Default: FALSE */
    zl303xx_BooleanE bUseAdjustTimeHybrid;

   /*!  Var: zl303xx_PFConfigS::bUseAdjustTimePacket
            Flag to indicate whether or not to use adjustTime() for packet mode.
            Default: FALSE */
    zl303xx_BooleanE bUseAdjustTimePacket;

   /*!  Var: zl303xx_PFConfigS::hybridLockTimeTarget
            In hybrid mode, the very first 1Hz alignment will take hybridLockTimeTarget
            seconds to complete.
            Units: s
            Default: 120 */
    Uint32T hybridLockTimeTarget;

   /*!  Var: zl303xx_PFConfigS::freqChangeLimit - DEPRECATED! ( see zl303xx_AprSetFCL() )
            Units: ppb/s.
            Default: 2900 */
    zl303xx_AprFrequencyChangeLimitE freqChangeLimit;

   /*!  Var: zl303xx_PFConfigS::TIEClear
            Indicates how the 1Hz pulse will move after a reference switch.
            If TIEClear = disabled, then the phase will not align to the
            new reference; instead, it will maintain its offset from the new
            reference's 1Hz pulse.
            If TIEClear = enabled, then the 1Hz phase pulse will align to the
            new reference's 1Hz pulse.
            Default: TC_enabled */
    zl303xx_TIEClearE TIEClear;

   /*!  Var: zl303xx_PFConfigS::hybridHwLockTimeoutMs
            HYBRID MODE ONLY. This should be set to something longer than the time
            taken by the hardware PLL to declare lock.
            Units: milliseconds
            Default: 37000 */
    Uint32T hybridHwLockTimeoutMs;

   /*!  Var: zl303xx_PFConfigS::aTimer1
            Set to 1. Do not modify. */
    Uint32T aTimer1;

   /*!  Var: zl303xx_PFConfigS::acgd
            Do not modify unless directed by Microsemi personnel. */
    Sint32T acgd;

   /*!  Var: zl303xx_PFConfigS::a0cgq
            Do not modify unless directed by Microsemi personnel. */
    Sint32T a0cgq;

   /*!  Var: zl303xx_PFConfigS::a0cgs
            Do not modify unless directed by Microsemi personnel. */
    Sint32T a0cgs;

   /*!  Var: zl303xx_PFConfigS::a1cgq
            Do not modify unless directed by Microsemi personnel. */
    Sint32T a1cgq;

   /*!  Var: zl303xx_PFConfigS::a1cgs
            Do not modify unless directed by Microsemi personnel. */
    Sint32T a1cgs;

   /*!  Var: zl303xx_PFConfigS::APRFrequencyLockedPhaseSlopeLimit
            Under normal operation and when 1Hz is not active and when APR
            is locked, this PSL is used, */
   Uint32T APRFrequencyLockedPhaseSlopeLimit;

   /*!  Var: zl303xx_PFConfigS::APRFrequencyNotLockedPhaseSlopeLimit
            Under normal operation and when 1Hz is not active and when APR
            is NOT locked, this PSL is used, */
   Uint32T APRFrequencyNotLockedPhaseSlopeLimit;

   /*!  Var: zl303xx_PFConfigS::APRFrequencyFastPhaseSlopeLimit
            When APR is in fast lock mode, this PSL is used, */
   Uint32T APRFrequencyFastPhaseSlopeLimit;

   /*!  Var: zl303xx_PFConfigS::APRPSLLimitedTimeoutPeriodSec
            Maximum period for which the PSL is enforced (seconds),
            Default value: ZL303XX_DEFAULT_PSL_TIMEOUT_PERIOD_SEC */
   Uint32T APRPSLLimitedTimeoutPeriodSec;

   /*!  Var: zl303xx_PFConfigS::APRPSLTimeoutExitDebouncePeriodSec
            Debounce period for exiting the PSL limited timeout state,
            Default value: ZL303XX_DEFAULT_PSL_TIMEOUT_EXIT_DEBOUNCE_PERIOD_SEC */
   Uint32T APRPSLTimeoutExitDebouncePeriodSec;

   /*!  Var: zl303xx_PFConfigS::APRFrequencyLockedFrequencyChangeLimit
            Under normal operation and when 1Hz is not active and when APR
            is locked, this FCL is used, */
   Uint32T APRFrequencyLockedFrequencyChangeLimit;

   /*!  Var: zl303xx_PFConfigS::APRFrequencyNotLockedFrequencyChangeLimit
            Under normal operation and when 1Hz is not active and when APR
            is NOT locked, this FCL is used, */
   Uint32T APRFrequencyNotLockedFrequencyChangeLimit;

   /*!  Var: zl303xx_PFConfigS::APRFrequencyFastFrequencyChangeLimit
              When APR is in fast lock mode, this FCL is used, */
   Uint32T APRFrequencyFastFrequencyChangeLimit;

   /*!  Var: zl303xx_PFConfigS::adjSize1HzPSL
            When 1Hz is active, the adjustment size is compared against the
            value in adjSize1HzPSL[]. The smallest limit in adjSize1HzPSL[]
            that is still larger than the adjustment size is found and
            the corresponding PSL value from PSL_1Hz[] is used. */
   Uint32T adjSize1HzPSL[ZL303XX_MAX_NUM_PSL_LIMITS];

   /*!  Var: zl303xx_PFConfigS::PSL_1Hz
            When 1Hz is active, the adjustment size is compared against the
            value in adjSize1HzPSL[]. The smallest limit in adjSize1HzPSL[]
            that is still larger than the adjustment size is found and
            the corrosponding PSL value from PSL_1Hz[] is used. [ppt/s] */
   Uint32T PSL_1Hz[ZL303XX_MAX_NUM_PSL_LIMITS];

   /*!  Var: zl303xx_PFConfigS::adjSize1HzFCL
            When 1Hz is active, the adjustment size is compared against the
            value in adjSize1HzFCL[]. The smallest limit in adjSize1HzFCL[]
            that is still larger than the adjustment size is found and
            the corresponding FCL value from FCL_1Hz[] is used. [ppt/s] */
   Uint32T adjSize1HzFCL[ZL303XX_MAX_NUM_FCL_LIMITS];

   /*!  Var: zl303xx_PFConfigS::FCL_1Hz
            When 1Hz is active, the adjustment size is compared against the
            value in adjSize1HzFCL[]. The smallest limit in adjSize1HzFCL[]
            that is still larger than the adjustment size is found and
            the corrosponding FCL value from FCL_1Hz[] is used. */
   Uint32T FCL_1Hz[ZL303XX_MAX_NUM_FCL_LIMITS];

   /*!  Var: zl303xx_PFConfigS::adjSize1HzAdjScaling
            When 1Hz is active, the adjustment size is compared against the
            value in adjSize1HzAdjScaling[]. The smallest limit in adjSize1HzAdjScaling[]
            that is still larger than the adjustment size is found and the
            corresponding adjustment scaling value from adjScaling_1Hz[] is used. [0.1%/unit] */
   Uint32T adjSize1HzAdjScaling[ZL303XX_MAX_NUM_ADJ_SCALING_LIMITS];

   /*!  Var: zl303xx_PFConfigS::adjScaling_1Hz
            When 1Hz is active, the adjustment size is compared against the
            value in adjSize1HzAdjScaling[]. The smallest limit in adjSize1HzAdjScaling[]
            that is still larger than the adjustment size is found and the
            corresponding adjustment scaling value from adjScaling_1Hz[] is used. */
   Uint32T adjScaling_1Hz[ZL303XX_MAX_NUM_ADJ_SCALING_LIMITS];

   /*!  Var: zl303xx_PFConfigS::lockInThreshold
            The PSL&FCL component is locked when the 1Hz alignment values fall
            below lockInThreshold for lockInCount consecutive alignments. If
            the 1Hz alignment values is greater than lockOutThreshold, then
            the PSL&FCL component becomes not locked.
            Units: ns
            default: 1000 */
    Sint32T lockInThreshold;

   /*!  Var: zl303xx_PFConfigS::lockOutThreshold
            See lockInThreshold.
            Units: ns
            default: 2000 */
    Sint32T lockOutThreshold;

   /*!  Var: zl303xx_PFConfigS::lockInCount
            See lockInThreshold.  */
    Sint32T lockInCount;

    /*!  Var: zl303xx_PFConfigS::lockedPhaseOutlierThreshold
             Threshold for 1hz adjustment values that should be ignored while
             the phase is locked because they are considered outliers. Value of 0
             disables the outlier check.
             Units: ns
             default: 600 */
    Uint32T lockedPhaseOutlierThreshold;

    /*!  Var: zl303xx_PFConfigS::initialFrequencyOffset
             Initial seet DCO frequency offset for PSL&FCL module in ppt.
             default: 0 */
    Sint32T initialFrequencyOffset;

   /*!  Var: zl303xx_PFConfigS::staticOffset
            This var is an offset set by the user and added to the 1Hz
            adjustment. It is used to compensate for known asymmetry in the
            network.
            Units: ns   */
    Sint32T staticOffset;

   /*!  Var: zl303xx_PFConfigS::setTimeRoundingZone
            Sometimes, the calculated 1Hz adjustment value can be very close to
            a multiple of the setTimeResolution.  In this case we would want to
            round the adjustment value to the closest setTimeResolution multiple.
            This rounding will reduce the amount of phase movement to be performed
            after the setTime operation.  This value should be less than half of the
            setTimeResolution.

            Example:
               - setTimeResolution = 1000000000, setTimeRoundingZone = 10000
               - Phase movements of 2000009999 or 1999990001 should both result in
                 SetTime operations of 2 seconds.

            Units: ns
            Default: 10000ns   */
    Sint32T setTimeRoundingZone;

   /*!  Var: zl303xx_PFConfigS::adjustFreqMinPhase
            The minimum phase difference that will be applied using adjustFreq().
            Units: ns
            Default: 20ns   */
    Sint32T adjustFreqMinPhase;

   /*!  Var: zl303xx_PFConfigS::maxHWDfRange
            The maximum df that the hardware can support.
            0 disables the feature.
            Units: ppb
            Default: 0   */
    Sint32T maxHWDfRange;

    /*!  Var: zl303xx_PFConfigS::stepTimeDetectableThr
            The point at which a small stepTime() can be reliably detected.
            unit: ns */
    Uint32T stepTimeDetectableThr;

    /*!  Var: zl303xx_PFConfigS::packetTreatmentGuardTime
            It may be desireable to discard a few packets after the user has
            called zl303xx_PFNotifyJumpComplete(). This guard time specifies how
            many milliseconds after zl303xx_PFNotifyJumpComplete() that packets
            will be discarded.
            unit: ms with a resolution of 125ms
            default: 0 */
    Uint32T packetTreatmentGuardTime;

    /*!  Var: zl303xx_PFConfigS::hybridCfReadInterval
            In hybrid mode, the software maintains an average value of the
            hardware's df value. These parameters control how the average
            is calculated.
            default: hybridCfReadInterval: 8
                     hybridCfReadFilter1: 4
                     hybridCfReadFilter2: 100
    */
    Uint32T hybridCfReadInterval;
    Uint32T hybridCfReadFilter1;
    Uint32T hybridCfReadFilter2;

    /*! DEPRECATED */
    Uint32T hybridCfOffsetDelay;

    /*!  Var: zl303xx_PFConfigS::jumpTimeSequence
            The sequence of steps to execute setTime() and stepTime(). */
    zl303xxjumpTimeSequence_E jumpTimeSequence[ZL303XX_MAX_JUMP_TIME_SEQUENCE_STEPS];

    /*!  Var: zl303xx_PFConfigS::adjTimeSequence
            The sequence of steps to execute adjTime(). */
    zl303xxjumpTimeSequence_E adjTimeSequence[ZL303XX_MAX_JUMP_TIME_SEQUENCE_STEPS];

    /*!  Var: zl303xx_PFConfigS::jumpTimeDelay_A - jumpTimeDelay_F
            Different delays that can be part of jumpTimeSequence[] or
            adjTimeSequence[]. units: PSL/FCL intervals - 125ms intervals */
    Uint32T jumpTimeDelay_A;
    Uint32T jumpTimeDelay_B;
    Uint32T jumpTimeDelay_C;
    Uint32T jumpTimeDelay_D;
    Uint32T jumpTimeDelay_E;
    Uint32T jumpTimeDelay_F;

    /*!  Var: zl303xx_PFConfigS::maxAdjFreqTime
            The maximum number of seconds that adjFreq() can take to perform
            an adjustment. If a frequency adjustment would take more than this
            many seconds, then the size of the adjustment is reduced so that
            the PSL/FCL limits are not exceeded.

            Note that if fast lock is enabled, then maxAdjFreqTime is ignored
            for the first frequency adjustment.

            default: 1200 seconds (20 minutes) */
    Uint32T maxAdjFreqTime;

    /*!  Var: zl303xx_PFConfigS::DynamicPhaseCorrectionActive
            For boundary clock modes, use the APR Dynamic Phase Correction mechanism to adjust
            for small values (less than AprDynamicPhaseCorrectionValue) of phase if
            AprDynamicPhaseCorrectionEnabled is TRUE. */
    zl303xx_BooleanE AprDynamicPhaseCorrectionEnabled;
    Uint32T AprDynamicPhaseCorrectionThr;

    /*!  Var: zl303xx_PFConfigS::maxAdjFreqTime
            Fast lock mode for 1 Hz adjustments that forces Step Time adjustment
            usage for a number of adjustments. */
    zl303xx_BooleanE fastLock1HzEnable;
    Uint32T fastLock1HzInterval;
    Uint32T fastLock1HzTotalPeriod;
    Uint32T fastLock1HzDelay;

    zl303xx_BooleanE adjFreqDynamicAdjustmentEnable;

    /*!  Var: zl303xx_PFConfigS::setOutputOffsetAction
            Type of action when zl303xx_SetOutputOffset() is called: either
            apply the offset immediately or when the next 1Hz adjustment
            is made. */
    zl303xx_SetOutputOffsetActionE setOutputOffsetAction;

    /*!  Var: zl303xx_PFConfigS::onlyAllowSteps
            If True, then 1Hz only applies setTime() and stepTime() operations.
            Intended for NCO-assist mode only. */
    zl303xx_BooleanE onlyAllowSteps;

    /*!  Var: zl303xx_PFConfigS::hwPollingIntervalTimer2Cycles
            The number of PF interrupts to wait before periodically  performing a DPLL status and DF read.
            In units of interrupts [zl303xx_AprInitS::aprTimer2PeriodMs].
            A value of 0 or 1 denotes a DPLL read every interrupt. */
    Uint32T hwPollingIntervalTimer2Cycles;

    /*!  Var: zl303xx_PFConfigS::stepTimePollingIntervalMs
            If non-zero, software polls for the end of stepTime.
            unit: ms
            default: 0 (disabled) */
    Uint32T stepTimePollingIntervalMs;

} zl303xx_PFConfigS;


/** zl303xx_PFReconfigS

These are the re-configurable parameters for the phase slope limit and
frequency change limit software components.
*/
typedef struct {

   /* This var is an offset set by the user and added to the 1Hz adjustment.
      It is used to compensate for known asymmetry in the network.
      Units: nanoseconds */
   Sint32T staticOffset;

   /**      Under normal operation and when 1Hz is not active and when APR
            is locked, this PSL is used. Units: ns/sec */
   Uint32T APRFrequencyLockedPhaseSlopeLimit;

   /**      Under normal operation and when 1Hz is not active and when APR
            is locked, this PSL is used. Units: ns/sec */
   Uint32T APRFrequencyNotLockedPhaseSlopeLimit;

   /**      When APR is in fast lock mode, this PSL is used */
   Uint32T APRFrequencyFastPhaseSlopeLimit;

   /**      Maximum period for which the PSL is enforced (seconds) */
   Uint32T APRPSLLimitedTimeoutPeriodSec;

   /**      Debounce period for exiting the PSL limited timeout state, */
   Uint32T APRPSLTimeoutExitDebouncePeriodSec;

   /**      Under normal operation and when 1Hz is not active and when APR
            is locked, this FCL is used. Units: ppt/s */
   Uint32T APRFrequencyLockedFrequencyChangeLimit;

   /**      Under normal operation and when 1Hz is not active and when APR
            is locked, this FCL is used. Units: ppt/s */
   Uint32T APRFrequencyNotLockedFrequencyChangeLimit;

   /**      When APR is in fast lock mode, this FCL is used */
   Uint32T APRFrequencyFastFrequencyChangeLimit;

   /*
      When 1Hz is active, the adjustment size is compared against the
      value in adjSize1HzPSL[]. The smallest limit in adjSize1HzPSL[]
      that is still larger than the adjustment size is found and
      the corrosponding PSL value from PSL_1Hz[] is used.
   */
   Uint32T adjSize1HzPSL[ZL303XX_MAX_NUM_PSL_LIMITS];
   Uint32T PSL_1Hz[ZL303XX_MAX_NUM_PSL_LIMITS];

   /*!  Var: zl303xx_PFReConfigS::adjSize1HzFCL
            When 1Hz is active, the adjustment size is compared against the
            value in adjSize1HzFCL[]. The smallest limit in adjSize1HzFCL[]
            that is still larger than the adjustment size is found and
            the corresponding FCL value from FCL_1Hz[] is used. [ppt/s] */
   Uint32T adjSize1HzFCL[ZL303XX_MAX_NUM_FCL_LIMITS];
   Uint32T FCL_1Hz[ZL303XX_MAX_NUM_FCL_LIMITS];

   /*!  Var: zl303xx_PFReConfigS::adjSize1HzAdjScaling
            When 1Hz is active, the final adjustment size is scaling according to
            the original magnitude of the adjustment. This functionality may be used
            to improve MTIE and TDEV performance in high PDV cases.  [units of 0.1 %] */
   Uint32T adjSize1HzAdjScaling[ZL303XX_MAX_NUM_ADJ_SCALING_LIMITS];
   Uint32T adjScaling_1Hz[ZL303XX_MAX_NUM_ADJ_SCALING_LIMITS];

} zl303xx_PFReConfigS;

/***************  END PSL&FCL DATA TYPES AND STRUCTURES  **********************/


typedef enum
{
   ZL303XX_FREQ_LOCK_ACQUIRING = ZL303XX_LOCK_STATUS_ACQUIRING,
   ZL303XX_FREQ_LOCK_ACQUIRED = ZL303XX_LOCK_STATUS_LOCKED,
   ZL303XX_PHASE_LOCK_ACQUIRED = ZL303XX_LOCK_STATUS_PHASE_LOCKED,
   ZL303XX_HOLDOVER = ZL303XX_LOCK_STATUS_HOLDOVER,
   ZL303XX_REF_FAILED = ZL303XX_LOCK_STATUS_REF_FAILED,
   ZL303XX_NO_ACTIVE_SERVER = ZL303XX_LOCK_NO_ACTIVE_SERVER,
   ZL303XX_UNKNOWN = ZL303XX_LOCK_STATUS_UNKNOWN,
   ZL303XX_MANUAL_FREERUN,
   ZL303XX_MANUAL_HOLDOVER,
   ZL303XX_MANUAL_SERVO_HOLDOVER,
   ZL303XX_APRSTATE_MAX = ZL303XX_MANUAL_SERVO_HOLDOVER
} zl303xx_AprStateE;
typedef enum
{
   ZL303XX_CGU_HW_L_FLAG = 0,
   ZL303XX_CGU_HW_H_FLAG,
   ZL303XX_CGU_HW_RF_FLAG,
   ZL303XX_CGU_HW_SE_FLAG,
   ZL303XX_CGU_HW_MH_FLAG,
   ZL303XX_CGU_HW_MF_FLAG,
   ZL303XX_CGU_SW_L_FLAG,
   ZL303XX_CGU_SW_H_FLAG,
   ZL303XX_CGU_SW_V_FLAG,
   ZL303XX_CGU_SW_MH_FLAG,
   ZL303XX_CGU_TS_FLAG,
   ZL303XX_CGU_WS_FLAG,
   ZL303XX_CGU_STATE,
   ZL303XX_CGU_RSTATUS,
   ZL303XX_CGU_SW_H_VALID,
   ZL303XX_CGU_DROPPED_PKTS,

   ZL303XX_CGU_NUM_TYPES
} zl303xx_AprCGUStatusFlagsE;

typedef struct
{
   zl303xx_BooleanE hwL;  /* H/W Lock */
   zl303xx_BooleanE hwH;  /* H/W Holdover */
   zl303xx_BooleanE RF;   /* Ref. Fail */
   zl303xx_BooleanE SE;   /* Sync. Enable */
   zl303xx_BooleanE hwMH; /* H/W Manual Holdover */
   zl303xx_BooleanE hwMF; /* H/W Manual Freerun */

   zl303xx_BooleanE swL;  /* S/W Lock */
   zl303xx_BooleanE swH;  /* S/W Holdover */
   zl303xx_BooleanE V;    /* Valid */
   zl303xx_BooleanE swMH; /* S/W Manual Holdover */
   zl303xx_BooleanE swHV; /* Validity of S/W Holdover value  */

   zl303xx_BooleanE TS;   /* Time Set */
   zl303xx_BooleanE WS;   /* Warm Start */

   zl303xx_AprStateE state;   /* CGU state */
   zl303xx_BooleanE RStatus;   /* Set upon soft reset and Ref. Fail */
   zl303xx_BooleanE DroppedPackets;   /* More than 50% of packets are dropped */

} zl303xx_AprCGUStatusFlagsS;

typedef enum
{
   ZL303XX_ELEC_REF_L_FLAG = 0,
   ZL303XX_ELEC_REF_H_FLAG,
   ZL303XX_ELEC_REF_RF_FLAG,
   ZL303XX_ELEC_REF_SE_FLAG,
   ZL303XX_ELEC_REF_OOR_FLAG,
   ZL303XX_ELEC_REF_STATE,

   ZL303XX_ELEC_REF_NUM_TYPES
} zl303xx_AprElecStatusFlagsE;

typedef struct
{
   /* L Flag: indicates if the client clock is frequency locked with the electric reference clock. */
   zl303xx_BooleanE L;
   zl303xx_BooleanE H;
   zl303xx_BooleanE RF;
   zl303xx_BooleanE SE;
   zl303xx_BooleanE OOR;

   zl303xx_AprStateE state;
} zl303xx_AprElecStatusFlagsS;

typedef enum
{
   ZL303XX_USING_FWD_PATH = 0,
   ZL303XX_USING_REV_PATH,
   ZL303XX_USING_FWD_REV_COMBINE
} zl303xx_AprAlgPathFlagE;

typedef enum
{
   ZL303XX_ALG_PATH_S0  = 0,
   ZL303XX_ALG_PATH_S1  = 1,
   ZL303XX_ALG_PATH_S11 = 11,
   ZL303XX_ALG_PATH_S2  = 2,
   ZL303XX_ALG_PATH_S3  = 3,
   ZL303XX_ALG_PATH_S4  = 4
} zl303xx_AprAlgPathStateE;

typedef enum
{
   ZL303XX_SERVER_CLK_L1_FLAG = 0, /* 0 */
   ZL303XX_SERVER_CLK_L2_FLAG,
   ZL303XX_SERVER_CLK_L3_FLAG,
   ZL303XX_SERVER_CLK_L4_FLAG,
   ZL303XX_SERVER_CLK_L_FLAG,
   ZL303XX_SERVER_CLK_GST_L_FLAG,
   ZL303XX_SERVER_CLK_V_FLAG,
   ZL303XX_SERVER_CLK_GST_V_FLAG,
   ZL303XX_SERVER_CLK_S_FLAG,
   ZL303XX_SERVER_CLK_U_FLAG,
   ZL303XX_SERVER_CLK_U1_FLAG, /* 10 */
   ZL303XX_SERVER_CLK_PE_FLAG,
   ZL303XX_SERVER_CLK_PA_FLAG,
   ZL303XX_SERVER_CLK_GST_PA_FLAG,
   ZL303XX_SERVER_CLK_PA_HOLDOVER_FLAG,
   ZL303XX_SERVER_CLK_H_FLAG,
   ZL303XX_SERVER_CLK_GST_H_FLAG,
   ZL303XX_SERVER_CLK_OOR_FLAG,
   ZL303XX_SERVER_CLK_TT_ERR_FLAG,
   ZL303XX_SERVER_CLK_OUTAGE_FLAG,
   ZL303XX_SERVER_CLK_OUTLIER_FLAG, /* 20 */
   ZL303XX_SERVER_CLK_FRR_FLAG,
   ZL303XX_SERVER_CLK_RRR_FLAG,
   ZL303XX_SERVER_CLK_STEP_FLAG,
   ZL303XX_SERVER_CLK_PKT_LOSS_FWD_FLAG,
   ZL303XX_SERVER_CLK_PKT_LOSS_REV_FLAG,
   ZL303XX_SERVER_CLK_PATH_FLAG,
   ZL303XX_SERVER_CLK_FWD_STATE,
   ZL303XX_SERVER_CLK_REV_STATE,
   ZL303XX_SERVER_CLK_STATE,
   ZL303XX_SERVER_CLK_PSL_ON, /* 30 */
   ZL303XX_SERVER_CLK_SA_ON,
   ZL303XX_SERVER_CLK_PKT_DROPPED_FLAG,
   ZL303XX_SERVER_CLK_PERPKT_OUTLIER_ALARM_FWD,
   ZL303XX_SERVER_CLK_PERPKT_OUTLIER_ALARM_REV,
   ZL303XX_SERVER_CLK_PERPKT_PHASE_MONITOR1_ALARM_FWD,
   ZL303XX_SERVER_CLK_PERPKT_PHASE_MONITOR1_ALARM_REV,
   ZL303XX_SERVER_CLK_PERPKT_PHASE_MONITOR2_ALARM_FWD,
   ZL303XX_SERVER_CLK_PERPKT_PHASE_MONITOR2_ALARM_REV,
   ZL303XX_SERVER_CLK_MONITOR_PHASE_DRIFT_ALARM,
   ZL303XX_SERVER_CLK_NUM_TYPES,
} zl303xx_AprServerStatusFlagsE;

typedef struct
{
   /* bFreqLockFlag flag: indicates if the client clock is frequency locked with the reference clock. */
   zl303xx_BooleanE L1;
   zl303xx_BooleanE L2;
   zl303xx_BooleanE L3;
   zl303xx_BooleanE L4;
   zl303xx_BooleanE L;
   zl303xx_BooleanE gstL;
   zl303xx_BooleanE V;
   zl303xx_BooleanE gstV;
   zl303xx_BooleanE S;
   zl303xx_BooleanE U;
   zl303xx_BooleanE U1;
   zl303xx_BooleanE PE;
   zl303xx_BooleanE PA;
   zl303xx_BooleanE gstPA;
   zl303xx_BooleanE PA_holdover;
   zl303xx_BooleanE H;
   zl303xx_BooleanE gstH;
   zl303xx_BooleanE OOR;
   zl303xx_BooleanE ttErrDetected;
   zl303xx_BooleanE outageDetected;
   zl303xx_BooleanE outlierDetected;
   zl303xx_BooleanE frrDetected;
   zl303xx_BooleanE rrrDetected;
   zl303xx_BooleanE stepDetected;
   zl303xx_BooleanE pktLossDetectedFwd;
   zl303xx_BooleanE pktLossDetectedRev;
   zl303xx_AprAlgPathFlagE algPathFlag;
   zl303xx_AprAlgPathStateE algFwdState;
   zl303xx_AprAlgPathStateE algRevState;
   zl303xx_AprStateE state;

   zl303xx_BooleanE PSLOn;
   zl303xx_BooleanE SA;

   zl303xx_BooleanE DroppedPackets;

   zl303xx_BooleanE perPktOutlierAlarmFwd;
   zl303xx_BooleanE perPktOutlierAlarmRev;
   zl303xx_BooleanE perPktPhaseMonitor1AlarmFwd;
   zl303xx_BooleanE perPktPhaseMonitor1AlarmRev;
   zl303xx_BooleanE perPktPhaseMonitor2AlarmFwd;
   zl303xx_BooleanE perPktPhaseMonitor2AlarmRev;
   zl303xx_BooleanE monitorPhaseDriftAlarm;

} zl303xx_AprServerStatusFlagsS;

typedef enum
{
   ZL303XX_1HZ_START_FLAG = 0,
   ZL303XX_1HZ_COLLECTION_END_FLAG,
   ZL303XX_1HZ_END_FLAG,
   ZL303XX_1HZ_ADJUSTMENT_SIZE_FLAG,
   ZL303XX_1HZ_ADJUSTMENT_NETWORK_QUALITY_FLAG,
   ZL303XX_1HZ_SET_TIME_TIMEOUT_FLAG,
   ZL303XX_1HZ_STEP_TIME_TIMEOUT_FLAG,
   ZL303XX_1HZ_ADJUST_TIME_TIMEOUT_FLAG,
   ZL303XX_1HZ_ABORT_FLAG,
   ZL303XX_1HZ_MAX_FREQ_ADJ_TIME_EXCEEDED_FLAG,
   ZL303XX_1HZ_FLAG_LAST
} zl303xx_Apr1HzStatusFlagsE;

typedef struct
{
   Uint64S adjustmentSize;
   Uint64S limitedAdjustmentSize;
   Uint32T adjustmentNetworkQuality;
} zl303xx_Apr1HzStatusFlagsS;


/** Structure passed to the CGU flag change callback function. */
typedef struct
{
   /** Pointer to the structure for this device instance. */
   void *hwParams;
   /** The CGU status flag that changed. */
   zl303xx_AprCGUStatusFlagsE type;
   /** The current values of all CGU status flags. */
   zl303xx_AprCGUStatusFlagsS flags;
} zl303xx_AprCGUNotifyS;

/** Structure passed to the electrical reference flag change callback function. */
typedef struct
{
   /** Pointer to the structure for this device instance. */
   void *hwParams;
   /** The electrical reference status flag that changed. */
   zl303xx_AprElecStatusFlagsE type;
   /** The current values of all electrical reference status flags. */
   zl303xx_AprElecStatusFlagsS flags;
} zl303xx_AprElecNotifyS;

/** Structure passed to the server flag change callback function. */
typedef struct
{
   /** Pointer to the structure for this device instance. */
   void *hwParams;
   /** The 16-bit ID used to reference the master clock. */
   Uint16T serverId;
   /** The server status flag that changed. */
   zl303xx_AprServerStatusFlagsE type;
   /** The current values of all APR server status flags. */
   zl303xx_AprServerStatusFlagsS flags;
} zl303xx_AprServerNotifyS;

/** Structure passed to the 1Hz flag change callback function. */
typedef struct
{
   /** Pointer to the structure for this device instance. */
   void *hwParams;
   /** The 16-bit ID used to reference the master clock. */
   Uint16T serverId;
   /** The server status flag that changed. */
   zl303xx_Apr1HzStatusFlagsE type;
   /** The current values of all electrical server status flags. */
   zl303xx_Apr1HzStatusFlagsS flags;
} zl303xx_Apr1HzNotifyS;

/* Callback types for flag change notification. */
typedef void (*zl303xx_AprCGUNotifyFn)(zl303xx_AprCGUNotifyS *data);
typedef void (*zl303xx_AprElecNotifyFn)(zl303xx_AprElecNotifyS *data);
typedef void (*zl303xx_AprServerNotifyFn)(zl303xx_AprServerNotifyS *data);
typedef void (*zl303xx_Apr1HzNotifyFn)(zl303xx_Apr1HzNotifyS *data);

/*!  Enum: zl303xx1HzAdjModifierActionE

This enumeration specifies whether to accept, reject, or recommend a new 1Hz
adjustment value.
*/
typedef enum
{
   MA_accept,
   MA_reject,
   MA_useAlternateValue
} zl303xx1HzAdjModifierActionE;

/*!  Enum: zl303xx1HzAdjModifierActionE

This enumeration specifies where, in Microsemi code, the request is coming
from: either the per-packet sub-system or the delayed action sub-system.
*/
typedef enum
{
   MS_delayed,
   MS_perPacket
} zl303xx1HzAdjModifierSourceE;

/*!  Struct: zl303xx1HzAdjModifierDataS

This structure is used to provide the 1Hz adjustment value to the user and to
return an alternate 1Hz adjustment value from the user.
*/
typedef struct {

   /*!  Var: zl303xx1HzAdjModifierDataS::source
            This value is supplied by Microsemi code. It specifies what the
            adjustment is being generatd by: either the 1Hz per-packet
            sub-system or the 1Hz delayed action sub-system.

            When a per-packet adjustment is rejected by the user, the delayed
            action sub-system will later attempt to make the same adjustment
            and will call the adjustment modification routine again.

            This behaviour is provided to give the user greater control over
            how the adjustment is applied. */
   zl303xx1HzAdjModifierSourceE source;

   /*!  Var: zl303xx1HzAdjModifierDataS::action
            This value is passed back from the user and specifies how the PF task
            should apply the next 1Hz offset. If the action is MA_useAlternateValue,
            then the alternate value must be specified in seconds, nanoSeconds,
            and bBackwardAdjust. */
   zl303xx1HzAdjModifierActionE action;

   /*!  Var: zl303xx1HzAdjModifierDataS::seconds
            This is a in/out parameter: it is the 1Hz adjustment as determined
            by the code as well as the alternate value determined by the user.
            seconds is the relative number of seconds. */
   Uint64S seconds;

   /*!  Var: zl303xx1HzAdjModifierDataS::nanoSeconds
            This is a in/out parameter: it is the 1Hz adjustment as determined
            by the code as well as the alternate value determined by the user.
            nanoSeconds is the relative number of nanoseconds. */
   Uint32T nanoSeconds;

   /*!  Var: zl303xx1HzAdjModifierDataS::bBackwardAdjust
            This is a in/out parameter: it is the 1Hz adjustment as determined
            by the code as well as the alternate value determined by the user.
            bBackwardAdjust is TRUE if the time is negative i.e. the new time
            is in the past. */
   zl303xx_BooleanE bBackwardAdjust;

   /*!  Var: zl303xx1HzAdjModifierDataS::overrideAdjType
            This is an out parameter: the user can use this variable to select
            if an adjustment should be made using adjustTime, adjustFreq or
            use the default value programmed.*/
   zl303xxoverrideAdjustmentTypeE overrideAdjType;

} zl303xx1HzAdjModifierDataS;


/* swFuncPtr1HzAdjModifier

   This routine allows the user to accept, reject, or modify a 1Hz adjustment.

   An instance of this function pointer is supplied by the user. That instance is
   called when a new 1Hz adjustment has been determined.

   The new 1Hz adjustment is passed in as arguments adjModifierData.seconds,
   nanoSeconds, bBackwardAdjust.

   To accept a 1Hz adjustment, return adjModifierData.action = MA_accept.

   To reject a 1Hz adjustment, return adjModifierData.action = MA_reject.

   To modify a 1Hz adjustment, pass back adjModifierData.action =
   MA_useAlternateValue and the modified value in adjModifierData.seconds,
   nanoSeconds, bBackwardAdjust.

  Parameters:
   [in]  hwParams                      Pointer to the device structure.
   [out] adjModifierData.action        User specified action. If this is
                                          MA_useAlternateValue, then the alternate
                                          value must be specified in 'adjustment'
   [in]  adjModifierData.seconds,      nanoSeconds, bBackwardAdjust
                                          The adjustment value determined by code.
                                          This is a relative value - not an absolute
                                          value.
   [out] adjModifierData.seconds, nanoSeconds, bBackwardAdjust
                                          If the passed-back action is
                                          MA_useAlternateValue, then this is
                                          the value to use.

  Return Value:  0     Success

****************************************************************************/
typedef Sint32T (*swFuncPtr1HzAdjModifier)(void *hwParams,
                              zl303xx1HzAdjModifierDataS *adjModifierData);


/*!  Enum: zl303xx_PacketTreatmentE
      Treatment of packets after setTime(), stepTime(), and adjustTime() is executed. */
/*!  Var: zl303xx_PacketTreatmentE PT_detect
      Detect the timestamp change
 */
/*!  Var: zl303xx_PacketTreatmentE PT_notifyWithDiscard
      Discard packets until the user notifies or timeout
 */

typedef enum
{
   PT_NULL,
   PT_detect,
   PT_notifyWithDiscard
} zl303xx_PacketTreatmentE;

/* Holdover Quality Categories */
typedef enum {
    ZL303XX_FREQ_CATX_UNKNOWN = -1,
    ZL303XX_FREQ_PTP          = 0,
    ZL303XX_FREQ_CAT1,
    ZL303XX_FREQ_CAT2,
    ZL303XX_FREQ_CAT3, /* Last valid Category - rest are for dynamic modifications */
    ZL303XX_FREQ_HYBRID,
    ZL303XX_FREQ_ELECTRIC,
    ZL303XX_FREQ_FREERUN,
    ZL303XX_FREQ_THRESHOLDNS,

    ZL303XX_FREQ_LAST = ZL303XX_FREQ_THRESHOLDNS
} zl303xx_FreqCatxE;


/*!  Struct: zl303xx_AprAddDeviceS

This is the clock disciplining (DCO) device configuration structure.
Parameters that can be configured when registering a device.
*/
typedef struct {
   /*!  Var: zl303xx_AprAddDeviceS::devMode
         The device current reference mode. */
    zl303xx_AprDeviceRefModeE devMode;

   /*!  Var: zl303xx_AprAddDeviceS::hybridAdjMode
         The device hybrid adjustment method. */
    zl303xx_AprDeviceHybridAdjModeE devHybridAdjMode;

    /*!  Var: zl303xx_AprAddDeviceS::pllId
          Index for a device containing multiple CGUs. */
    Uint32T pllId;

    /*!  Var: zl303xx_AprAddDeviceS::algCycles
          Count the times the algo ran for a device. */
    Uint32T algCycles;

    /*!  Var: zl303xx_AprAddDeviceS::dcs
          For the future use. */
    zl303xx_BooleanE dcs;

    /*!  Var: zl303xx_AprAddDeviceS::enterPhaseLockStatusThreshold
          The threshold to enter phase lock status. Unit: ns */
    Uint32T enterPhaseLockStatusThreshold;

    /*!  Var: zl303xx_AprAddDeviceS::enterPhaseLockNetworkConditionThreshold
          The 1Hz network condition that must be achived to enter
          phase lock status. Unit: 1-100 (default 75) */
    Uint32T enterPhaseLock1HzNetworkConditionThreshold;

    /*!  Var: zl303xx_AprAddDeviceS::exitPhaseLockStatusThreshold
          The threshold to exit phase lock status. Unit: ns */
    Uint32T exitPhaseLockStatusThreshold;

    /*!  Var: zl303xx_AprAddDeviceS::hwDcoResolutionInPpt
          The resolution of device hardware DCO. Unit: ppt */
    Uint32T hwDcoResolutionInPpt;

    /*!  Var: zl303xx_AprAddDeviceS::bWarmStart
          The boolean flag to indicate if warmstart is requested */
    zl303xx_BooleanE bWarmStart;

    /*!  Var: zl303xx_AprAddDeviceS::warmStartInitialFreqOffset
          The warmstart initial frequency offset value in the unit of ppt (parts per trillion)
          if warmstart is requested */
    Sint32T warmStartInitialFreqOffset;

    /*!  Var: zl303xx_AprAddDeviceS::packetTreatmentDetectTimeoutSec
          The time out wait period for the jump detection mechanism when
          timestamp jump detection mechanism used (SetTime or StepTime)
          Utilization depends on (zl303xx_PacketTreatmentE) PT_detect settings. */
    Uint32T packetTreatmentDetectTimeoutSec;

   /*!  Var: zl303xx_AprAddDeviceS::resetIfA90000
          The boolean flag to indicate if automatic handling of a_90000 errors is requested */
    zl303xx_BooleanE bResetIfA90000;

    /*!  Var: zl303xx_AprAddDeviceS::ClearDeviceStatistics
           The boolean if TRUE clears the performance structures of both Lock and other states */
     zl303xx_BooleanE ClearDeviceStatistics;

    /*!  Var: zl303xx_AprAddDeviceS::PFConfig
          The configuration parameters for the limiters (phase slope limit and frequency
          change limit) */
    zl303xx_PFConfigS PFConfig;

    /*!  Var: zl303xx_AprAddDeviceS::setTime
          The function pointer for the device specific function provided by the user to change
          the time of day value on the device without affecting the clock frequency
          if it is supported by the hardware */
    hwFuncPtrSetTime setTime;

    /*!  Var: zl303xx_AprAddDeviceS::stepTime
          The function pointer for the device specific function provided by the user to adjust
          the phase value on the device without affecting the clock frequency
          if it is supported by the hardware */
    hwFuncPtrAdjustClk stepTime;

    /*!  Var: zl303xx_AprAddDeviceS::adjustTime
          The function pointer for the device specific function provided by the user to adjust
          the desired phase value on the device by changing the clock frequency within the maximum
          adjustment time if it is supported by the hardware */
    hwFuncPtrAdjustTime adjustTime;

    /*!  Var: zl303xx_AprAddDeviceS::adjustFreq
          The function pointer for the device specific function provided by the user to adjust
          the clock frequency (in the unit of ppt) on the CGU device */
    hwFuncPtrAdjustClk adjustFreq;

   /*!  Var: zl303xx_AprAddDeviceS::setTimePacketTreatment
          Packet treatment for setTime()
          */
    zl303xx_PacketTreatmentE setTimePacketTreatment;

   /*!  Var: zl303xx_AprAddDeviceS::stepTimePacketTreatment
          Packet treatment for stepTime()
          */
    zl303xx_PacketTreatmentE stepTimePacketTreatment;

   /*!  Var: zl303xx_AprAddDeviceS::adjustTimePacketTreatment
          Packet treatment for adjustTime()
          */
    zl303xx_PacketTreatmentE adjustTimePacketTreatment;

   /*!  Var: zl303xx_AprAddDeviceS::legacyTreatment
          The legacy treatment determines additional actions when a setTime(),
          stepTime(), or adjustTime() timer expires.  This variable is only checked if
          the packetTreatment is PT_notifyWithDiscard

          If True : Do not reset APR on timeout when waiting for user notification
          If False: Reset APR on timeout when waiting for user notification
          default : TRUE
          */
    zl303xx_BooleanE legacyTreatment;

    /*!  Var: zl303xx_AprAddDeviceS::jumpTimeTSU
          The function pointer for the device specific function provided by the user to re-align
          the TSU with the 1Hz pulse */
    hwFuncPtrJumpTimeTSU jumpTimeTSU;

    /*!  Var: zl303xx_AprAddDeviceS::jumpNotification
          An instance of this routine prototype is supplied by the user to notify
          the user that a 1Hz jump is either about to start or end. */
    swFuncPtrJumpNotification jumpNotification;

    /*!  Var: zl303xx_AprAddDeviceS::sendRedundancyData
          An instance of this routine prototype is supplied by the user to send
          redundancy data to the monitor. */
    swFuncPtrSendRedundancyData sendRedundancyData;

    /*!  Var: zl303xx_AprAddDeviceS::jumpActiveCGU
          An instance of this routine prototype is supplied by the user to jump the
          active CGU. */
    swFuncPtrJumpActiveCGU jumpActiveCGU;

    /*!  Var: zl303xx_AprAddDeviceS::jumpStandbyCGU
          An instance of this routine prototype is supplied by the user to jump the
          standby CGU. */
    swFuncPtrJumpStandbyCGU jumpStandbyCGU;

    /*!  Var: zl303xx_AprAddDeviceS::adjModifier
          The function pointer for the device specific function provided by the user to allow the
          user to accept, reject, or modify the 1Hz adjustment value. */
    swFuncPtr1HzAdjModifier adjModifier;

    /*!  Var: zl303xx_AprAddDeviceS::getAprServerFreqOffset
          The function pointer for the device specific function provided by the user to retrieve
          the frequency offset. */
    hwFuncPtrGetClkInfo getAprServerFreqOffset;

    /*!  Var: zl303xx_AprAddDeviceS::getHwLockStatus
          The function pointer for the device specific function provided by the user to retrieve
          the current clock lock status from the CGU device hardware */
    hwFuncPtrGetClkInfo getHwLockStatus;

    /*!  Var: zl303xx_AprAddDeviceS::getHwManualHoldoverStatus
          The function pointer for the device specific function provided by the user to retrieve
          the manual holdover status (as a boolean value) from the CGU device hardware */
    hwFuncPtrGetClkInfo getHwManualHoldoverStatus;

    /*!  Var: zl303xx_AprAddDeviceS::getHwManualFreerunStatus
          The function pointer for the device specific function provided by the user to retrieve
          the manual freerun status (as a boolean value) from the CGU device hardware */
    hwFuncPtrGetClkInfo getHwManualFreerunStatus;

    /*!  Var: zl303xx_AprAddDeviceS::getHwSyncInputEnStatus
          The function pointer for the device specific function provided by the user to retrieve
          the sync input enabled status (as a boolean value) from the CGU device hardware */
    hwFuncPtrGetClkInfo getHwSyncInputEnStatus;

    /*!  Var: zl303xx_AprAddDeviceS::getHwOutOfRangeStatus
          The function pointer for the device specific function provided by the user to retrieve
          the out of range (pull-in range of the electrical reference) status (as a boolean value)
          from the CGU device hardware */
    hwFuncPtrGetClkInfo getHwOutOfRangeStatus;

    /*!  Var: zl303xx_AprAddDeviceS::refSwitchToPacketRef
          The function pointer for the device specific function provided by the user to give the
          device DCO hardware control to APR. If this function finished successfully,
          the DCO hardware will be manipulated by APR. */
    hwFuncPtrGeneral refSwitchToPacketRef;

    /*!  Var: zl303xx_AprAddDeviceS::refSwitchToElectricalRef
          The function pointer for the device specific function provided by the user to allow APR
          to return the device DCO hardware control to the hardware. */
    hwFuncPtrGeneral refSwitchToElectricalRef;

    hwFuncPtrGetTimeStamperCGUOffset getTimeStamperCGUOffset;   /* DEPRECATED - replaced by zl303xx_AprSetTimeStamperCGUOffset !!! */

    /*!  Var: zl303xx_AprAddDeviceS::timeStamperCGUOffsetSec
          Offset provided by the user to allow APR to get the time difference between the CGU's 1Hz pulse and the
          user's timestamper roll-over point. Set using zl303xx_AprSetTimeStamperCGUOffset. */
    Uint64S timeStamperCGUOffsetSec;

    /*!  Var: zl303xx_AprAddDeviceS::timeStamperCGUOffsetNs
          Offset provided by the user to allow APR to get the time difference between the CGU's 1Hz pulse and the
          user's timestamper roll-over point. Set using zl303xx_AprSetTimeStamperCGUOffset. */
    Sint32T timeStamperCGUOffsetNs;

    /*!  Var: zl303xx_AprAddDeviceS::bResetReady
          The boolean flag to indicate if the TS device is functional after a reset (not currently used) */
    zl303xx_BooleanE bResetReady;

    /*!  Var: zl303xx_AprAddDeviceS::enableAprDF
          Customer-provided Disable of DFs (used with dynamicDFInPpt (Be careful!)) */
    Uint8T enableAprDF;

    /*!  Var: zl303xx_AprAddDeviceS::fixedDFInPpt
          Customer-provided dynamic DF offset to apply to the calculated value (Be careful!) */
    Sint32T dynamicDFInPpt;

    /*!  Var: zl303xx_AprAddDeviceS::dynamicXOStability
          Customer-provided dynamic stability factor override value (Be careful!) */
    Sint64T dynamicXOStability;

    /*!  Var: zl303xx_AprAddDeviceS::defaultCGU
          For use in very special CGU configurations. */
    zl303xx_BooleanE defaultCGU[ZL303XX_ADCGU_LAST];

    /*!  Var: zl303xx_AprAddDeviceS::clkStabDelayLimit
          How many 10-second iterations should the device delay until it starts calculating
          the clock stability value. */
    Uint32T clkStabDelayLimit;

    /** User callback executed when a CGU status flag changes (may be NULL). */
    zl303xx_AprCGUNotifyFn cguNotify;
    /** User callback executed when an electrical reference status flag changes (may be NULL). */
    zl303xx_AprElecNotifyFn elecNotify;
    /** User callback executed when a server status flag changes (may be NULL). */
    zl303xx_AprServerNotifyFn serverNotify;
    /** User callback executed when a 1Hz status flag changes (may be NULL). */
    zl303xx_Apr1HzNotifyFn oneHzNotify;

    zl303xx_AprHitlessCompE hitlessCompensation;         /* DEPRECATED, kept for backward compilation purposes */
    zl303xx_BooleanE compensateEveryStepTime;            /* DEPRECATED, kept for backward compilation purposes */
    hwFuncPtrHitlessComp refSwitchHitlessCompensation; /* DEPRECATED, kept for backward compilation purposes */

    /** Electrical reference switching stage 1 transient period in 800/1000 ms periods */
    Uint32T inputRefSwitchTransientStage1Delay;
    /** Electrical reference switching stage 2 transient period in 800/1000 ms periods */
    Uint32T inputRefSwitchTransientStage2Delay;

    /** CGU warm start flag. */
    zl303xx_BooleanE cguWarmStart;

    /*!  Var: zl303xx_AprAddDeviceS::proxyTxEnabled
          If proxyTxEnabled == TRUE, then this device sends data to the monitor */
    zl303xx_BooleanE proxyTxEnabled;

    /*!  Var: zl303xx_AprAddDeviceS::proxyRxEnabled
          If proxyRxEnabled == TRUE, then this device accepts data from the active */
    zl303xx_BooleanE proxyRxEnabled;

    /*!  Var: zl303xx_AprAddDeviceS::make1HzAdjustmentsDuringHoldover
         If TRUE, then phase adjustments are made even when the state of the
         device is holdover.
         default: FALSE */
    zl303xx_BooleanE make1HzAdjustmentsDuringHoldover;

    /*!  Var: zl303xx_AprAddDeviceS::useLegacyStreamStartUp
             Indicates to use first packet stream configured as the current reference (the default behaviour). */
    zl303xx_BooleanE useLegacyStreamStartUp;

    /*!  Var: zl303xx_AprAddDeviceS::postAprOffset
             This offset (in ppt) is applied to the hardware in addition to
             the value generated by APR if the CGU state is HOLDOVER or FREERUN.
             Normally, this value is not specified at startup. Instead,
             it would be modified by calling routine AprSetDevicePostAprOffset
             when the CGU enters HOLDOVER or FREERUN. */
    zl303xx_BooleanE postAprOffset;

    hwFuncPtrAdjustClk enteringHOStateChange; /* DEPRECATED, kept for backward compilation purposes */
    hwFuncPtrGeneral exitingStateChange;      /* DEPRECATED, kept for backward compilation purposes */

    /*!  Var: zl303xx_AprAddDeviceS::freqCatXAccuracyPpt
        EQL related threshols */
    Uint32T freqCat1AccuracyPpt;
    Uint32T freqCat2AccuracyPpt;
    Uint32T freqCat3AccuracyPpt;
    Uint32T freqCatXAccuracyPpt;    /* One of the above - default Cat1 */

    /*!  Var: zl303xx_AprAddDeviceS::currentXXXXHoldoverAccuracyPpt
        Holdover Quality related thresholds */
    zl303xx_FreqCatxE currentHoldoverCategory;
    zl303xx_FreqCatxE localOscillatorFreqCategory;
    Uint32T currentHybridHoldoverAccuracy;
    Uint32T currentElectricalHoldoverAccuracy;
    Uint32T localOscillatorFreerunAccuracy;
    Uint32T currentHoldoverThresholdNs;

    /*!  Var: zl303xx_AprAddDeviceS::firstPacketTreatment
        Enum for determining how to treat the first packet received from a new server.
        default: 0 (ZL303XX_FIRST_PACKET_TREATMENT_SEC_ONLY) */
    zl303xxfirstPacketTreatmentE firstPacketTreatment;

    /*!  Var: zl303xx_AprAddDeviceS::aprStepTimeOptions
        Enum for additional logic to be applied when calculating StepTime operations.
        default: 0 (ZL303XX_APR_STEPTIME_OPTIONS_NONE) */
	zl303xx_AprStepTimeOptionsE aprStepTimeOptions;

    /*!  Var: zl303xx_AprAddDeviceS::resetFirstPacketOnRefSwitch
        Enum for determining whether to force a setTime operation on reference switch. */
    zl303xx_BooleanE resetFirstPacketOnRefSwitch;

    /*!  Var: zl303xx_AprAddDeviceS::resetMonitorServerOnElecSwitch
        Enum for determining whether to force resetting monitoring servers on electrical switch. */
    zl303xx_BooleanE resetMonitorServerOnElecSwitch;

    /*!  Var: zl303xx_AprAddDeviceS::AprBCHybridActionPhaseLock
        Action to perform upon entering phase lock in BC hybrid mode */
    hwFuncPtrGeneral AprBCHybridActionPhaseLock;
    /*!  Var: zl303xx_AprAddDeviceS::AprBCHybridActionPhaseLock
        Action to perform upon exiting phase lock in BC hybrid mode */
    hwFuncPtrGeneral AprBCHybridActionOutOfLock;

    /*!  Var: zl303xx_AprAddDeviceS::AprHitlessElecRefSwitchRegActionsOnEnter
        Action to perform upon entering hitless electrical reference switch operation. */
    hwFuncPtrTypeTwo AprHitlessElecRefSwitchRegActionsOnEnter;
    /*!  Var: zl303xx_AprAddDeviceS::AprHitlessElecRefSwitchRegActionsOnIntermediate
        Action to perform during intermediate hitless electrical reference switch operation. */
    hwFuncPtrTypeOne AprHitlessElecRefSwitchRegActionsOnIntermediate;
    /*!  Var: zl303xx_AprAddDeviceS::AprHitlessElecRefSwitchRegActionsOnExit
        Action to perform upon exiting hitless electrical reference switch operation. */
    hwFuncPtrTypeOne AprHitlessElecRefSwitchRegActionsOnExit;

    /*!  Var: zl303xx_AprAddDeviceS::driverMsgRouter
        call into the driver */
    hwFuncPtrDriverMsgRouter driverMsgRouter;
    /*!  Var: zl303xx_AprAddDeviceS::useDriverMsgRouter
        boolean to use the CGU message router routine */
    zl303xx_BooleanE useDriverMsgRouter;

    /*!  Var: zl303xx_AprAddDeviceS::userMsgRouter
        call into the application code */
    hwFuncPtrDriverMsgRouter userMsgRouter;
    /*!  Var: zl303xx_AprAddDeviceS::useUserMsgRouter
        boolean to use the application message router routine */
    zl303xx_BooleanE useUserMsgRouter;

    /*!  Var: zl303xx_AprAddDeviceS::jumpLoggingThresholdNs
        Threshold to trigger error logs on APR level 2 when timestamp change exceeds
        given nanoseconds. Set higher for noisier networks.
        Default 1 msec. */
    Uint32T jumpLoggingThresholdNs;

    /*!  Var: zl303xx_AprAddDeviceS::aprCguStateMachineReportingPeriodMs
        APR state machine update long interrupt period in ms. */
    Uint32T aprCguStateMachineReportingPeriodMs;
    /*!  Var: zl303xx_AprAddDeviceS::aprAprStateMachinePeriodMs
        APR algorithm internal long interrupt period in ms. */
    Uint32T aprAprStateMachinePeriodMs;

} zl303xx_AprAddDeviceS;


/** zl303xx_AprAddDeviceStructInit

   Set up default parameters for the zl303xx_AprAddDevice() function.

  Parameters:
   [in]  hwParams       pointer to the structure for this device instance
   [in]  par            pointer to the structure for configuration items

  Return Value:
    ZL303XX_OK                       if successful
    ZL303XX_INVALID_POINTER          if either hwParams or par is NULL

*****************************************************************************/
zlStatusE zl303xx_AprAddDeviceStructInit(void *hwParams,
        zl303xx_AprAddDeviceS *par);

/** zl303xx_AprAddDevice

   This function adds the device structure to the table of devices supported
   by APR.

  Parameters:
   [in]      hwParams       pointer to the structure for this device instance
   [in]      par            pointer to the structure for configuration items

  Return Value:
    ZL303XX_OK                       if successful
    ZL303XX_INVALID_POINTER          if either hwParams or par is NULL
    ZL303XX_PARAMETER_INVALID        if there is a bad configuration parameter
    ZL303XX_RTOS_SEMA4_NOT_CREATED   internal error
    ZL303XX_RTOS_SEMA4_TAKE_FAIL     internal error
    ZL303XX_NOT_RUNNING              if the APR application has not been launched
                                       successfully by calling zl303xx_AprInit().
    ZL303XX_MULTIPLE_INIT_ATTEMPT    if zl303xx_AprAddDevice has already been
                                       called with the given hwParams
    ZL303XX_ERROR                    if there is no room for the device to be registered
                                       in APR, i.e. too many devices
    ZL303XX_RTOS_SEMA4_GIVE_FAIL     internal error

*****************************************************************************/
zlStatusE zl303xx_AprAddDevice(void *hwParams,
        zl303xx_AprAddDeviceS *par);

/*!  Struct: zl303xx_AprOverrideAlgorithmDataS
      Holds user-adjusted algorithm parameters.
*/
typedef struct
{
   zl303xx_BooleanE active[ZL303XX_ALGO_MAX_MODIFIED_PARAMETERS];
   Sint32T param[ZL303XX_ALGO_MAX_MODIFIED_PARAMETERS];
   Sint32T value[ZL303XX_ALGO_MAX_MODIFIED_PARAMETERS];
} zl303xx_AprOverrideAlgorithmDataS;

/*!  Struct: zl303xx_AprAddServerS

This is the server clock configuration structure.
Parameters that can be configured when registering a server clock.
*/
typedef struct {
   /*!  Var: zl303xx_AprAddServerS::serverId
            A unique 16-bit ID used to reference the master clock. */
   Uint16T serverId;

   /*!  Var: zl303xx_AprAddServerS::tsFormat
            Indicates timestamp format for the master */
   zl303xx_AprTsFormatE tsFormat;

   /*!  Var: zl303xx_AprAddServerS::b32BitTs
            Indicates if only 32-bit sub-seconds of timestamp available for the master.
            If RTP is used, the flag should be set to true. */
   zl303xx_BooleanE b32BitTs;

   /*!  Var: zl303xx_AprAddServerS::cid
            Indicates the user configuration selection of the server (master).
            exampleAprConfigIdentifiersE stored as Sint32T. */
   Sint32T cid;

   zl303xx_AprAlgTypeModeE algTypeMode;
   zl303xx_AprOscillatorFilterTypesE oscillatorFilterType;
   zl303xx_AprFilterTypesE filterType;

   /*!  Var:  zl303xx_AprAddServerS::bUseType2BPLL
            Set this variable to True in order to use type 2B PLL mode
   */
   zl303xx_BooleanE bUseType2BPLL;

   /*!  Var:  zl303xx_AprAddServerS::bUseGroup4Alg
            Set this variable to True in order to use Group4 algorithm
   */
   zl303xx_BooleanE bUseGroup4Alg;

   /*!  Var:  zl303xx_AprAddServerS::EnableXOCompensation
            Set this variable to True in order to use Xo-compensation
   */
   zl303xx_BooleanE EnableXOCompensation;

   /*!  Var: zl303xx_AprAddServerS::packetRateType
            Indicates the forward path packet rate catalogue for the master */
   zl303xx_AprPktRateE fwdPacketRateType;

   /*!  Var: zl303xx_AprAddServerS::packetRateType
            Indicates the reverse path packet rate catalogue for the master */
   zl303xx_AprPktRateE revPacketRateType;

   /*!  Var: zl303xx_AprAddServerS::bUseRevPath
            Indicate if using forward path or reverse path if the algorithm type mode is selected to
            use only single path by APR.
            In the case of PTP, using the reverse path (t3, t4) if it is true;
            In the case of NTP, using the path of (t1, t2) if it is true; */
   zl303xx_BooleanE bUseRevPath;

   /*!  Var: zl303xx_AprAddServerS::bHybridMode
            Indicates that the server is a hybrid mode server */
   zl303xx_BooleanE bHybridMode;

   /*!  Var: zl303xx_AprAddServerS::osciHoldoverStability
           Indicates the oscillator holdover stability. Unit: ppt.
           If the value is set to 0, the default configuration is used. */
   Uint32T osciHoldoverStability;

      /*!  Var: zl303xx_AprAddServerS::sModeTimeout
           Indicates the sMode timeout. Unit: secs.
           If the value is set to 0, the default configuration is used. */
   Uint32T sModeTimeout;

   /*!  Var: zl303xx_AprAddServerS::sModeAgeout
           Indicates the sMode age out. Unit: 10s of secs.
           If the value is set to 0, the default configuration is used. */
   Uint32T sModeAgeout;

   /*!  Var: zl303xx_AprAddServerS::uPeriodThreshold
           Threshold of the u period declaration for the server clock. Unit: ns
           If the value is set to -1, the default configuration is used. */
   Sint32T uPeriodThreshold;

   /*!  Var: zl303xx_AprAddServerS::lockFlagsMask
            Mask for the flag L2 and L3.
            0 - unmask both L2 and L3 flag (use both of them);
            1 - mask L2 flag (not use L2);
            2 - mask L3 flag (not use L3);
            3 - mask both L2 and L3 flags (not use both of them) */
   Uint8T lockFlagsMask;

   /*!  Var: zl303xx_AprAddServerS::enterHoldoverGST
         The guard soak timer for entering holdover state. Unit: ten second */
   Uint32T enterHoldoverGST;

   /*!  Var: zl303xx_AprAddServerS::exitVFlagGST
         The guard soak timer for V flag from 1 to 0. Unit: ten second */
   Uint32T exitVFlagGST;

   /*!  Var: zl303xx_AprAddServerS::exitLFlagGST
         The guard soak timer for for L flag from 1 to 0. Unit: ten second */
   Uint32T exitLFlagGST;

   /*!  Var: zl303xx_AprAddServerS::exitPAFlagGST
         The guard soak timer for for PA flag from 1 to 0. Unit: ten second */
   Uint32T exitPAFlagGST;

   /*!  Var: zl303xx_AprAddServerS::clusterRange
            Indicates the cluster range for the server clock. Unit: ns (Note: the resolution of
            the parameter is 15ns). If the value is set to -1, the default configuration
            is used. */
   Sint32T clusterRange;

   /*!  Var: zl303xx_AprAddServerS::thresholdForFlagV
            Indicates the threshold of V flag for the server clock. Unit: ten second.
            The minimal value should be bigger than 0. If the value is 0, value 1 is used.
            If the value is set to -1, the default configuration is used.
            It is not applicable to the following algorithm type modes:
            ZL303XX_XDSL_FREQ_ACCURACY,
            ZL303XX_CUSTOM_FREQ_ACCURACY_200,
            ZL303XX_CUSTOM_FREQ_ACCURACY_15 */
   Sint32T thresholdForFlagV;

   /*!  Var: zl303xx_AprAddServerS::XdslHpFlag
            It is only applicable to the ZL303XX_XDSL_FREQ_ACCURACY algorithm type mode */
   zl303xx_BooleanE bXdslHpFlag;

   /*!  Var: zl303xx_AprAddServerS::packetDiscardDurationInSec
            Immediately following a setTime() or stepTime(), packets will be discarded for
            this number of seconds.
            0 disables the feature.
            If this feature is disabled, the system will attempt to detect the phase change.
            default: 0. */
   Uint32T packetDiscardDurationInSec;

   /*!  Var: zl303xx_AprAddServerS::pullInRange
            Indicates the maximum frequency offset from a server that a client can lock to. */
   zl303xx_AprPullInRangeE pullInRange;

   /*!  Var: Sint32::oorPeriodResetThr
            Indicates the number of seconds before APR resets in an out of range condition defaults to 100s
            Maximum value for this variable is 2^31/1000, a value of -1 disables APR reset during OOR period */
   Sint32T oorPeriodResetThr;

   /* The following parameters are the internal use */
   zl303xx_AprApsTypesE apsType;
   zl303xx_BooleanE wms;
   Uint32T mfw;

   /*!  Var: Sint32::PathReselectTimerLimitIns
            Indicates the number of seconds before APR resets the path selection mechanism once reverse path
            goes into holdover.
            Maximum value for this variable is 2^31, a value of -1 disables the feature of resetting path selection */
   Sint32T PathReselectTimerLimitIns;

   /*!  Var: zl303xx_AprAddServerS::overrideAlgorithmData
            Do not modify unless directed by Microsemi personel.

            Holds user-adjusted algorithm parameters.
   */
   zl303xx_AprOverrideAlgorithmDataS overrideAlgorithmData[2];

   /*!  Var: zl303xx_AprAddServerS::fastLockTime
        Indicates the Fast Lock time to apply the next two parameters. Unit: secs.
        If the value is set to 0, the default configuration is used. */
   Uint32T fastLockTime;

   /*!  Var: zl303xx_AprAddServerS::fastLockBW
        Indicates the Fast Lock Bandwidth. Unit: enum.
        If the fastLockTime is set to 0, the default configuration is used. */
   Uint32T fastLockBW;

    /*!  Var: zl303xx_AprAddServerS::fastLockBW
        Indicates the Fast Lock Bandwidth. Unit: packets.
        If the fastLockTime is set to 0, the default configuration is used. */
    Uint32T fastLockWindow;

    /*!  Var: zl303xx_AprAddServerS::HoldoverFreeze
        Holdover behavior setting. */
    zl303xx_AprHoldoverTypeE HoldoverFreeze;

    /*!  Var: zl303xx_AprAddServerS::HoldoverUserValue
        Holdover value (ppt) to use if HoldoverFreeze is set to use user value. */
    Sint32T HoldoverUserValue;

    /*!  Var: zl303xx_AprAddServerS::DFSeed
        Seeding value for initial software DF */
    Sint32T DFSeed;

    /*!  Var: zl303xx_AprAddServerS::pllStepDetectCalc
        Defines the PLL phase step calculations (reset) approach */
    zl303xx_AprStepDetectCalcE pllStepDetectCalc;

    /*!  Var: zl303xx_AprAddServerS::bCableFastLock
        A Boolean to enable high PDV cable mode Fast lock option*/
    zl303xx_BooleanE bCableFastLock;

    /*!  Var: zl303xx_AprAddServerS::CableFastLockTimer
        A Uint32T timer value in units of 10s used for high PDV cable mode to trigger Fast lock*/
    Uint32T CableFastLockTimer;

    /*!  Var: zl303xx_AprAddServerS::Type2BFastlockStartupIt
        Number of Fastlock iterations to force on startup for Type2B Algorithm */
    Uint32T Type2BFastlockStartupIt;

    /*!  Var: zl303xx_AprAddServerS::Type2BFastlockSecondaryTriggerPhaseThreshold
        Secondary trigger for Type2B Fastlock with threshold in ns */
    Uint32T Type2BFastlockSecondaryTriggerPhaseThreshold;

    /*!  Var: zl303xx_AprAddServerS::Type2BFastlockSecondaryTriggerLimitSec
        Secondary trigger for Type2B Fastlock for max counter limit */
    Uint32T Type2BFastlockSecondaryTriggerLimitSec;

    /*!  Var: zl303xx_AprAddServerS::Type2BFastlockThreshold
        Fastlock trigger treshold for Type2B Algorithm */
    Uint32T Type2BFastlockThreshold;

    /*!  Var: zl303xx_AprAddServerS::Type2BFastlockTypeRatio
        Fastlock trigger ratio for determining which Type2B FastLock operation to use (%) */
    Uint32T Type2BFastlockTypeRatio;

    /*!  Var: zl303xx_AprAddServerS::Type2BFastLockSimpleCoef
        Type2B simplified FastLock algorithm coefficient */
    Uint32T Type2BFastLockSimpleCoef;

    /*!  Var: zl303xx_AprAddServerS::Type2BfastLockBypassPSLFCL
        Type2B Algorithm to bypass PSL&FCL during fastLock */
    zl303xx_BooleanE Type2BfastLockBypassPSLFCL;

    /*!  Var: zl303xx_AprAddServerS::Type2BfastLockOnMonitorToActive
        Type2B Algorithm to use fastLock during switch */
    zl303xx_BooleanE Type2BfastLockOnMonitorToActive;

    /*!  Var: zl303xx_AprAddServerS::Type2BPLLFastLock
        Type2B Algorithm Fast Lock Enable */
    zl303xx_BooleanE Type2BPLLFastLock;

     /*!  Var: zl303xx_AprAddServerS::bEnableLowBWFastLock
        Ebnable type2B algorithm low BW fast lock mechanism */
    zl303xx_BooleanE bEnableLowBWFastLock;

    /*!  Var: zl303xx_AprAddServerS::Type2BFastLockPSL
        Type2B Algorithm PSL to use during fast lock phase pull-in process (ns/s) */
    Uint32T Type2BFastLockPSL;

    /*!  Var: zl303xx_AprAddServerS::Type2BFastLockFreqEstInterval
        Type2B Algorithm fast lock frequency estimation length (in interrupt cycles) */
    Uint32T Type2BFastLockFreqEstInterval;

    /*!  Var: zl303xx_AprAddServerS::Type2bFastLockMinPhaseNs
        Type2B Algorithm Fast Lock Minimum Phase */
    Uint32T Type2bFastLockMinPhaseNs;

    /*!  Var: zl303xx_AprAddServerS::Type2BMinFastLockTargetTime
        Type2B Algorithm Fast Lock Minimum Period (ms) */
    Uint32T Type2BMinFastLockTargetTime;

    /*!  Var: zl303xx_AprAddServerS::Type2BMaxFastLockTargetTime
        Type2B Algorithm Fast Lock Maximum Period (ms) */
    Uint32T Type2BMaxFastLockTargetTime;

    /*!  Var: zl303xx_AprAddServerS::Type2BLockedPhaseSlopeLimit
        Type2B algorithm PSL to be applied during normal operations (ns/s) */
    Uint32T Type2BLockedPhaseSlopeLimit;

    /*!  Var: zl303xx_AprAddServerS::L2phase_varLimit
        Lock level 2 phase limit */
    Sint32T L2phase_varLimit;

    /*!  Var: zl303xx_AprAddServerS::OutlierTimer
        Outlier counter reset value*/
    Sint32T OutlierTimer;

    /*!  Var: zl303xx_AprAddServerS::ClkInvalidCntr
        Seeding value for initial ClkInvalidCntr */
    Sint32T ClkInvalidCntr;

    /*!  Var: zl303xx_AprAddServerS::enableFastNetOutageDetection
        Seeding value for initial enableFastNetOutageDetection */
    zl303xx_BooleanE enableFastNetOutageDetection;

    /*!  Var: zl303xx_AprAddServerS::legacyMonitoringModeControl
        Seeding value for initial legacyMonitoringModeControl */
    zl303xx_BooleanE legacyMonitoringModeControl;

    /*!  Var: zl303xx_AprAddServerS::NCOWritePeriod
        The NCO access rate is 1000/NCOWritePeriod Hz */
    Uint32T NCOWritePeriod;

    /*!  Var: zl303xx_AprAddServerS::L4ThresholdValue
        The threshold to turn on L4 lock bit in ppt */
    Uint32T L4ThresholdValue;

    /*!  Var: zl303xx_AprAddServerS::packetHoldoverAccuracyPpt
         Holdover Quality related - UNUSED at this time */
    Uint32T packetHoldoverAccuracyPpt;

    /*!  Var: zl303xx_AprAddServerS::useOFM
         Indicates that we should offset-from-master.
         default: TRUE. */
    zl303xx_BooleanE useOFM;

    /*!  Var: zl303xx_AprAddServerS::OFMPTP
         Do not modify unless directed by Microsemi personel.
    */
    zl303xx_BooleanE OFMPTP;

    /*!  Var: zl303xx_AprAddServerS::OFMReRouteThr
         Do not modify unless directed by Microsemi personnel.
    */
    Uint32T OFMReRouteThr;

    /*!  Var: zl303xx_AprAddServerS::OFMReRouteDur
         Do not modify unless directed by Microsemi personnel.
    */
    Uint32T OFMReRouteDur;

    /*!  Var: zl303xx_AprAddServerS::useNCOAssist
         For ZL30770 devices, enable a extra DPLL
    */
    zl303xx_BooleanE useNCOAssist;

    /*!  Var: zl303xx_AprAddServerS::correctionFieldThreshold
            A correction field saturation threshold value. Ingress packets with
            absolute value of correction field larger than this threshold will be discarded.
            Units: nanoseconds. Maximum: 2^(63-16). Default: see ZL303XX_CORR_FIELD_SATURATION_VAL.
*/
    Uint64T correctionFieldThreshold;

    /*!  Var: zl303xx_AprAddServerS::fastClkInvalidCounts
         Number of Algorithm state machine call periods before going into holdover
         after fast network outage detection.
    */
    Uint32T fastClkInvalidCounts;

} zl303xx_AprAddServerS;




/** zl303xx_GetAprAddServerS

   Get zl303xx_AprAddServerS structure of a given server

  Parameters:
   [in]  serverId            server Id
   [out] par                 pointer to structure to hold output data

  Return Value:
    ZL303XX_OK                       if successful
    ZL303XX_INVALID_POINTER          if par is NULL
    ZL303XX_PARAMETER_INVALID        if serverId is invalid

****************************************************************************/
zlStatusE  zl303xx_GetAprAddServerS(Uint32T serverId,zl303xx_AprAddServerS *par);





/** zl303xx_AprAddServerStructInit

   Set up default parameters for the zl303xx_AprAddServer() function.

  Parameters:
   [in]  par            pointer to the structure for configuration items

  Return Value:
    ZL303XX_OK                       if successful
    ZL303XX_INVALID_POINTER          if par is NULL

****************************************************************************/
zlStatusE zl303xx_AprAddServerStructInit(zl303xx_AprAddServerS *par);


/** zl303xx_AprAddServer

   This function is used to register a master for APR to perform timing
   recovery for.

  Parameters:
   [in]  zl303xx_Params   pointer to the structure for this device instance
   [in]  par            pointer to the structure for configuration items

  Return Value:
    ZL303XX_OK                       if successful
    ZL303XX_INVALID_POINTER          if either zl303xx_Params or par is NULL
    ZL303XX_PARAMETER_INVALID        if there is a bad configuration parameter
    ZL303XX_MULTIPLE_INIT_ATTEMPT    if zl303xx_AprAddServer has already been
                                       called with the same server ID
    ZL303XX_NOT_RUNNING              if the APR application has not been launched
                                       successfully by calling zl303xx_AprInit().
    ZL303XX_ERROR                    if the specified device has not been
                                       registered or there are already maximum number of
                                       servers registered on this device

****************************************************************************/
zlStatusE zl303xx_AprAddServer(void *zl303xx_Params, zl303xx_AprAddServerS *par);


typedef struct {
   /*!  Var: zl303xx_AprRemoveServerS::serverId
            A unique 16-bit handle used to reference the server. */
   Uint16T serverId;
} zl303xx_AprRemoveServerS;

/** zl303xx_AprRemoveServerStructInit

   Set up default parameter for the zl303xx_AprRemoveServer() function.

  Parameters:
   [in]  par            pointer to the structure for configuration items

  Return Value:
    ZL303XX_OK                       if successful
    ZL303XX_INVALID_POINTER          if par is NULL

****************************************************************************/
zlStatusE zl303xx_AprRemoveServerStructInit(zl303xx_AprRemoveServerS *par);

/** zl303xx_AprRemoveServer

   This function is used to remove a server previously registered on a device
   specified by *hwParams in APR.

  Parameters:
   [in]  hwParams   pointer to the structure for this device instance
   [in]  par            pointer to the structure for configuration items

  Return Value:
    ZL303XX_OK                       if successful
    ZL303XX_INVALID_POINTER          if either hwParams or par is NULL
    ZL303XX_NOT_RUNNING              if zl303xx_InitDevice() has not been
                                       called i.e. other tasks and data have
                                       not been initialised yet.
    ZL303XX_PARAMETER_INVALID        internal error
    ZL303XX_ERROR                    if the specified device has not been
                                       initialised

****************************************************************************/
zlStatusE zl303xx_AprRemoveServer(void *hwParams, zl303xx_AprRemoveServerS *par);



/*!  Struct: zl303xx_PFRemoveDeviceS

Parameters that can be configured when removing a device from the phase slope
limit & frequency change limit components.
*/
typedef struct {
   void *zl303xx_Params;
} zl303xx_PFRemoveDeviceS;

/*!  Struct: zl303xx_AprRemoveDeviceS

Parameters that can be configured when removing a device.
*/
typedef struct {
    zl303xx_PFRemoveDeviceS PFRemoveParams;
} zl303xx_AprRemoveDeviceS;


/** zl303xx_AprRemoveDeviceStructInit

   Set up default parameters for the zl303xx_AprRemoveDevice() function.

  Parameters:
   [in]  hwParams   pointer to the structure for this device instance
   [in]  par            pointer to the structure for configuration items

  Return Value:
    ZL303XX_OK                       if successful
    ZL303XX_INVALID_POINTER          if either hwParams or par is NULL

*****************************************************************************/
zlStatusE zl303xx_AprRemoveDeviceStructInit(void *hwParams,
      zl303xx_AprRemoveDeviceS *par);

/** zl303xx_AprRemoveDevice

   This function removes the device structure to the table of devices
   supported by APR.

  Parameters:
   [in]  hwParams   pointer to the structure for this device instance
   [in]  par            pointer to the structure for configuration items

  Return Value:
    ZL303XX_OK                       if successful
    ZL303XX_INVALID_POINTER          if either hwParams or par is NULL
    ZL303XX_NOT_RUNNING              if zl303xx_InitDevice() has not been called
                                       i.e. other tasks and data have not been
                                       initialised yet.
    ZL303XX_RTOS_SEMA4_NOT_CREATED   if the APR table mutex has not been created
    ZL303XX_RTOS_SEMA4_TAKE_FAIL     internal error
    ZL303XX_ERROR                    if the given hwParams was not found in
                                       the APR table
    ZL303XX_RTOS_SEMA4_GIVE_FAIL     internal error

*****************************************************************************/
zlStatusE zl303xx_AprRemoveDevice(void *hwParams,
      zl303xx_AprRemoveDeviceS *par);



/*****************   FUNCTION DECLARATIONS   **********************************/


/****************** Manipulate log level *****************/
/**  zl303xx_SetAprLogLevel

   This function is used to set the debug log level for APR


  Parameters:
   [in]  level   Log level to be set for the APR application:
                    Level 0: the serious errors will be logged
                    Level 1: both the serious errors and minor errors will be
                             logged
                    Level 2: all the errors and the (debug) status information
                             will be logged

  Return Value: zlStatusE
*******************************************************************************/

zlStatusE zl303xx_SetAprLogLevel(Uint8T level);

/**  zl303xx_GetAprLogLevel

   This function is called by APR functions to access the APR log level.

  Return Value: The value of APR debug log level if the APR table is valid,
        otherwise, return value 0.
*******************************************************************************/

Uint8T zl303xx_GetAprLogLevel(void);


/****************** Query Registered Server Clock Status *************************/
/** zl303xx_AprGetServerAlgTypeMode

   Query the algorithm type mode for the specified server.

  Parameters:
   [in]    hwParams      Pointer to the device structure
   [in]    serverId      The identifier of server to be queried

   [out]   algTypeMode   The algorithm type mode of the server. The result can
                            be one of the following:
                            ZL303XX_NATIVE_PKT_FREQ                     = 0,
                            ZL303XX_NATIVE_PKT_FREQ_UNI                 = 1,
                            ZL303XX_NATIVE_PKT_FREQ_CES                 = 2,
                            ZL303XX_NATIVE_PKT_FREQ_ACCURACY            = 3,
                            ZL303XX_NATIVE_PKT_FREQ_ACCURACY_UNI        = 4,
                            ZL303XX_NATIVE_PKT_FREQ_FLEX                = 5,
                            ZL303XX_BOUNDARY_CLK                        = 6,
                            ZL303XX_NATIVE_PKT_FREQ_ACCURACY_FDD        = 7,
                            ZL303XX_XDSL_FREQ_ACCURACY                  = 8,
                            ZL303XX_CUSTOM_FREQ_ACCURACY_200            = 9,
                            ZL303XX_CUSTOM_FREQ_ACCURACY_15             = 10


  Return Value: ZL303XX_OK                if successful
****************************************************************************/
zlStatusE zl303xx_AprGetServerAlgTypeMode (void *hwParams, Uint16T serverId, zl303xx_AprAlgTypeModeE *algTypeMode);

/** zl303xx_AprGetServerOscillatorFilterType

   Query the oscillator filter type for the specified server.

  Parameters:
   [in]    hwParams           Pointer to the device structure
   [in]    serverId           The identifier of server to be queried

   [out]   osciFilterType     The oscillator filter type of the server. The result can
                                 be one of the following:
                                 ZL303XX_TCXO           = 0,
                                 ZL303XX_TCXO_FAST      = 1,
                                 ZL303XX_OCXO_S3E       = 2,
                                 ZL303XX_APR_NOT_SUPPORT_FILTER = 3


  Return Value: ZL303XX_OK                if successful
****************************************************************************/
zlStatusE zl303xx_AprGetServerOscillatorFilterType
                  (
                  void *hwParams,
                  Uint16T serverId,
                  zl303xx_AprOscillatorFilterTypesE *osciFilterType
                  );

/** zl303xx_AprGetServerFilterType

   Query the filter type for the specified server.

  Parameters:
   [in]    hwParams           Pointer to the device structure
   [in]    serverId           The identifier of server to be queried

   [out]   filterType         The filter type of the server. The result can
                                 be one of the following:
                                 ZL303XX_BW_0_FILTER = 0,
                                 ZL303XX_BW_1_FILTER = 1,
                                 ZL303XX_BW_2_FILTER = 2,
                                 ZL303XX_BW_3_FILTER = 3,
                                 ZL303XX_BW_4_FILTER = 4,


  Return Value: ZL303XX_OK                if successful
****************************************************************************/
zlStatusE zl303xx_AprGetServerFilterType
                  (
                  void *hwParams,
                  Uint16T serverId,
                  zl303xx_AprFilterTypesE *filterType
                  );


/** zl303xx_AprGetServerTimestampFormat

   Query the timestamp format for the specified server.

  Parameters:
   [in]    hwParams      Pointer to the device structure
   [in]    serverId      The identifier of server to be queried

   [out]     tsFormat      The timestamp format of the server. The result can
                     be one of the following (only PTP is supported currently):
                     ZL303XX_APR_TS_PTP = 0,
                     ZL303XX_APR_TS_RTP = 1,
                     ZL303XX_APR_TS_NTP = 2,


  Return Value: ZL303XX_OK                if successful
****************************************************************************/
zlStatusE zl303xx_AprGetServerTimestampFormat (void *hwParams, Uint16T serverId, zl303xx_AprTsFormatE *tsFormat);

/** zl303xx_AprGetServerPktRate

   Query the packet rates of forward and reverse paths for the specified server used in APR.
   If one of the path not existing in APR, 0pps is returned for that path.

  Parameters:
   [in]    hwParams           Pointer to the device structure
   [in]    serverId           The identifier of server to be queried

   [out]   fwdPktRate         The forward path packet rate of the server.
   [out]   revPktRate         The reverse path packet rate of the server.


  Return Value: ZL303XX_OK                if successful
****************************************************************************/
zlStatusE zl303xx_AprGetServerPktRate
                  (
                  void *hwParams,
                  Uint16T serverId,
                  zl303xx_AprPktRateE *fwdPktRate,
                  zl303xx_AprPktRateE *revPktRate
                  );


/** zl303xx_AprIsServerInHybridMode

   Query if the specified server in hybrid mode or not.

  Parameters:
   [in]    hwParams         Pointer to the device structure
   [in]     serverId         Identifier of the server

   [out]     hybridMode      Indicate whether or not in hybrid mode:
                        ZL303XX_TRUE - Hybrid mode;
                        ZL303XX_FALSE - Not in hybrid mode

  Return Value: ZL303XX_OK                if successful
****************************************************************************/
zlStatusE zl303xx_AprIsServerInHybridMode(void *hwParams, Uint16T serverId, zl303xx_BooleanE *hybridMode);

/** zl303xx_AprIsServerClkStreamValid

   Query if the timestamps from the specified server valid or not.

  Parameters:
   [in]    hwParams         Pointer to the device structure
   [in]     serverId         Identifier of the server

   [out]     clkStreamValid   Indicate if the timestamps valid or not

  Return Value: ZL303XX_OK                if successful
****************************************************************************/
zlStatusE zl303xx_AprIsServerClkStreamValid(void *hwParams, Uint16T serverId, zl303xx_BooleanE *clkStreamValid);

/** zl303xx_AprIsServerClkInUMode

*  Query if the specified server clock is in uMode or not.

  Parameters:
   [in]    hwParams         Pointer to the device structure
   [in]     serverId         Identifier of the server

   [out]     uMode               Indicate if in U period or not*

  Return Value: ZL303XX_OK                if successful
****************************************************************************/
zlStatusE zl303xx_AprIsServerClkInUMode(void *hwParams, Uint16T serverId, zl303xx_BooleanE *uMode);


/** zl303xx_AprIsServerUseSinglePath

   Query the server whether or not using only single path, that is, the reverse path is not used.

  Parameters:
   [in]    hwParams         Pointer to the device structure
   [in]     serverId         Identifier of the server

   [out]     singlePath      Indicate if using single path or not:
                        ZL303XX_TRUE - Using single path only;
                        ZL303XX_FALSE - Using both forward path and reverse path

  Return Value: ZL303XX_OK                if successful
****************************************************************************/
zlStatusE zl303xx_AprIsServerUseSinglePath(void *hwParams, Uint16T serverId, zl303xx_BooleanE *singlePath);

/** zl303xx_AprGetServerClkStabilityIndicator

   Query the clock stability parameter for the specified server. The result represents the clock
   stability. The smaller number, the better sever clock stability;  0 stands for the invalid value.

  Parameters:
   [in]    hwParams               Pointer to the device structure
   [in]    serverId               The identifier of server to be queried

   [out]     clkstabilityIndicator      Clock stability indicator to show the FFO variation in the unit of ppt

  Return Value: ZL303XX_OK                      if successful
****************************************************************************/
zlStatusE zl303xx_AprGetServerClkStabilityIndicator (void *hwParams, Uint16T serverId, Uint32T *clkstabilityIndicator);

/** zl303xx_AprGetServerStatusFlags

   Query the status flags for the specified server.

  Parameters:
   [in]    hwParams                  Pointer to the device structure
   [in]    serverId                  The identifier of server to be queried

   [out]   statusFlags               pointer to the server status flags structure

  Return Value: ZL303XX_OK                        if successful
****************************************************************************/
zlStatusE zl303xx_AprGetServerStatusFlags (void *hwParams, Uint16T serverId, zl303xx_AprServerStatusFlagsS *statusFlags);

/** zl303xx_AprGetElecRefStatusFlags

   Query the current electrical reference status flags for the specified device in electrical/hybrid mode.

  Parameters:
   [in]    hwParams                  Pointer to the device structure
   [out]   statusFlags               pointer to the electrical reference status flags structure

  Return Value: ZL303XX_OK                        if successful
****************************************************************************/
zlStatusE zl303xx_AprGetElecRefStatusFlags (void *hwParams, zl303xx_AprElecStatusFlagsS *statusFlags);


/****************** Query Registered Registered Clock Generation Device Status *************************/

/** zl303xx_AprGetCGUStatusFlags

   Query the CGU device status flags for the specified device.

  Parameters:
   [in]    hwParams                  Pointer to the device structure
   [out]   statusFlags               pointer to the CGU status flags structure

  Return Value: ZL303XX_OK                        if successful
****************************************************************************/
zlStatusE zl303xx_AprGetCGUStatusFlags (void *hwParams, zl303xx_AprCGUStatusFlagsS *statusFlags);

/* zl303xx_AprSetCGUStatusFlags */
/**
This function sets the changed values of the CGU status flags

  Parameters:
   [in]    hwParams       Pointer to the device structure
   [in]   statusFlags     pointer to the CGU status flags structure

  Return Value: ZL303XX_OK                        if successful
****************************************************************************/
zlStatusE zl303xx_AprSetCGUStatusFlags (void *hwParams, zl303xx_AprCGUStatusFlagsS *statusFlags);



/** zl303xx_AprGetDeviceLockStatus

   Query the current clock lock status for the specified device. If the device is not in
   packet mode, the result of status depends on the return value of hardware related function
   (*getHwLockStatus)(void *hwParams, Sint32T *lockStatus).

  Parameters:
   [in]    hwParams         Pointer to the device structure
   [out]     lockStatus      Clock lock status, the value can be:
                        ZL303XX_LOCK_STATUS_ACQUIRING,
                        ZL303XX_LOCK_STATUS_LOCKED,
                        ZL303XX_LOCK_STATUS_PHASE_LOCKED,
                        ZL303XX_LOCK_STATUS_HOLDOVER,
                        ZL303XX_LOCK_STATUS_REF_FAILED,
                        ZL303XX_LOCK_NO_ACTIVE_SERVER,
*                       ZL303XX_LOCK_STATUS_UNKNOWN

  Return Value: ZL303XX_OK                if successful
****************************************************************************/
zlStatusE zl303xx_AprGetDeviceLockStatus (void *hwParams, zl303xx_AprLockStatusE *lockStatus);

/** zl303xx_AprGetDevicePhaseStabilityIndicator

   Query the phase stability parameter for the specified device. The result represents the recovered clock
   phase variation in the unit of ns.

  Parameters:
   [in]    hwParams                  Pointer to the device structure

   [out]     phaseStabilityIndicator      Phase stability indicator to show the phase variation in the unit of ns

  Return Value: ZL303XX_OK                      if successful
****************************************************************************/
zlStatusE zl303xx_AprGetDevicePhaseStabilityIndicator (void *hwParams, Sint32T *phaseStabilityIndicator);


/** zl303xx_AprGetDevicePhaseAlignmentAccuracy

   Query the phase alignment accuracy of the recovered clock for the specified device.

  Parameters:
   [in]    hwParams         Pointer to the device structure

   [out]     phaseAlignAccuracy in ns

  Return Value: ZL303XX_OK                if successful
****************************************************************************/
zlStatusE zl303xx_AprGetDevicePhaseAlignmentAccuracy (void *hwParams, Uint64S *phaseAlignAccuracy);



typedef struct {
   /*!  Var: zl303xx_AprDeviceStatusIndicatorsS::serverId
            Indicates which server is providing the reference clock for this data collection. */
   Uint16T serverId;

   /*!  Var: zl303xx_AprDeviceStatusIndicatorsS::bFreqLockFlag
            Flag to indicate if the client clock is frequency locked with the reference clock. */
   zl303xx_BooleanE bFreqLockFlag;

   /*!  Var: zl303xx_AprDeviceStatusIndicatorsS::bPhaseAlignFlag
            Flag to indicate if the client clock is phase aligned to the reference clock. */
   zl303xx_BooleanE bPhaseAlignFlag;

   /*!  Var: zl303xx_AprDeviceStatusIndicatorsS::bPhaseHoldoverFlag
            Flag to indicate if the client clock is in phase holdover. */
   zl303xx_BooleanE bPhaseHoldoverFlag;

   /*!  Var: zl303xx_AprDeviceStatusIndicatorsS::bHoldoverFlag
            Flag to indicate if the client clock is in holdover. */
   zl303xx_BooleanE bHoldoverFlag;

   /*!  Var: zl303xx_AprDeviceStatusIndicatorsS::bValidFlag
            Flag to indicate if the timestamps received from the current packet/hybrid server
            clock is valid or not. */
   zl303xx_BooleanE bValidFlag;

   /*!  Var: zl303xx_AprDeviceStatusIndicatorsS::bInSMode
            Flag to indicate if the timestamps received from the current packet/hybrid server
            clock is in s-mode or not. */
   zl303xx_BooleanE bInSMode;

   /*!  Var: zl303xx_AprDeviceStatusIndicatorsS::bOutOfRange
            Flag to indicate if the timestamps received from the current packet/hybrid server
            clock is out of range or not. */
   zl303xx_BooleanE bOutOfRange;

   /*!  Var: zl303xx_AprDeviceStatusIndicatorsS::buPeriod
            Flag to indicate if the timestamps received from the current packet/hybrid server
            clock is within the "U" period or not. */
   zl303xx_BooleanE buPeriod;

   /*!  Var: zl303xx_AprDeviceStatusIndicatorsS::bElecRefFailFlag
            Flag to indicate if the electric reference clock failed or not. Not valid in the packet
            reference mode */
   zl303xx_BooleanE bElecRefFailFlag;

   /*!  Var: zl303xx_AprDeviceStatusIndicatorsS::bTimeSetFlag
            Flag to indicate if the ToD of client clock is roughly set to align to the reference clock. */
   zl303xx_BooleanE bTimeSetFlag;
} zl303xx_AprDeviceStatusIndicatorsS;

/** zl303xx_AprGetServerStatusIndicators

   Query the status indicators of the provided serverId. If the device is
   in electrical mode, the status of the active electrical reference is used.

  Parameters:
   [in]    hwParams          Pointer to the device structure
   [in]    serverId          User-provided server ID

   [out]   indicators        The pointer to indicator of the states.


  Return Value: ZL303XX_OK                if successful
****************************************************************************/
zlStatusE zl303xx_AprGetServerStatusIndicators (void *hwParams, Uint16T serverId,
                                              zl303xx_AprDeviceStatusIndicatorsS *indicators);


/** zl303xx_AprSetServerFlagGST

   Set a guard soak timer value to the following flags for the specified server. Unit: 10-second
   ZL303XX_SERVER_CLK_GST_H_FLAG (from 0 to 1),
   ZL303XX_SERVER_CLK_GST_V_FLAG (from 1 to 0),
   ZL303XX_SERVER_CLK_GST_L_FLAG (from 1 to 0),
   ZL303XX_SERVER_CLK_GST_PA_FLAG (from 1 to 0)

  Parameters:
   [in]    hwParams          Pointer to the device structure
   [in]    serverId          The server ID
   [in]    whichFlag         Indicate which flag's GST to be set

   [in]    flagGstVal        The guard soak timer value. Unit: ten seconds

  Return Value: ZL303XX_OK                if successful
****************************************************************************/
zlStatusE zl303xx_AprSetServerFlagGST
            (
            void *hwParams,
            Uint16T serverId,
            zl303xx_AprServerStatusFlagsE whichFlag,
            Uint32T flagGstVal
            );

/** zl303xx_AprSetServerEnterHoldoverGST

   Set the guard soak timer to enter holdover state for the specified server.

  Parameters:
   [in]    hwParams          Pointer to the device structure
   [in]    serverId          The server ID

   [in]    enHoldoverGst     The guard soak timer to be set. Unit: ten seconds

  Return Value: ZL303XX_OK                if successful
****************************************************************************/
zlStatusE zl303xx_AprSetServerEnterHoldoverGST (void *hwParams, Uint16T serverId, Uint32T enHoldoverGst);

/** zl303xx_AprSetServerHoldover

   Manually set APR specified server clock to enter to or leave from
   holdover state on the specified device.

  Parameters:
   [in]    hwParams          Pointer to the device structure
   [in]    serverId          The server ID
   [in]    bHoldover         Indicate to enter/leave holdover manually
                                ZL303XX_TRUE:     enter to holdover
                                ZL303XX_FALSE:    leave from holdover

  Return Value: ZL303XX_OK                if successful
****************************************************************************/
zlStatusE zl303xx_AprSetServerHoldover (void *hwParams, Uint16T serverId, zl303xx_BooleanE bHoldover);

/** zl303xx_AprSetServerHoldoverNoDelay

   Manually set APR specified server clock to enter to or leave from
   holdover state on the specified device without OS delay.

  Parameters:
   [in]    hwParams          Pointer to the device structure
   [in]    serverId          The server ID
   [in]    bHoldover         Indicate to enter/leave holdover manually
                                ZL303XX_TRUE:     enter to holdover
                                ZL303XX_FALSE:    leave from holdover

  Return Value: ZL303XX_OK                if successful
****************************************************************************/
zlStatusE zl303xx_AprSetServerHoldoverNoDelay (void *hwParams, Uint16T serverId, zl303xx_BooleanE bHoldover);


/** zl303xx_AprGetDeviceStatusIndicators

   Query the status indicators for a specified device.

  Parameters:
   [in]    hwParams         Pointer to the device structure

   [out]     indicators      The pointer of indicators of the device.


  Return Value: ZL303XX_OK                if successful
****************************************************************************/
zlStatusE zl303xx_AprGetDeviceStatusIndicators (void *hwParams, zl303xx_AprDeviceStatusIndicatorsS *indicators);

/** zl303xx_AprGetDeviceRefMode

   Query the current reference clock mode for a specified device.

  Parameters:
   [in]    hwParams      Pointer to the device structure

   [out]   deviceRefMode The current device mode for the reference clock. The result can be:
                        ZL303XX_PACKET_MODE = 0
                        ZL303XX_HYBRID_MODE = 1
                        ZL303XX_ELECTRIC_MODE = 2

  Return Value: ZL303XX_OK            If successful
****************************************************************************/
zlStatusE zl303xx_AprGetDeviceRefMode (void *hwParams, zl303xx_AprDeviceRefModeE *deviceRefMode);

/** zl303xx_AprGetDeviceHybridAdjModeRuntime

   Query the current hybrid adjustment mode for the specific device.

  Parameters:
   [in]    hwParams      Pointer to the device structure

   [out]   deviceHybridAdjMode The current device mode for the reference clock. The result can be:
                                    ZL303XX_HYBRID_ADJ_PHASE = 0
                                    ZL303XX_HYBRID_ADJ_FREQ = 1
                                    ZL303XX_HYBRID_ADJ_CUSTOM_PHASE = 2
                                    ZL303XX_HYBRID_ADJ_CUSTOM_FREQ = 3
                                    ZL303XX_HYBRID_ADJ_NONE = 4

  Return Value: ZL303XX_OK             If successful
****************************************************************************/
zlStatusE zl303xx_AprGetDeviceHybridAdjModeRuntime (void *hwParams, zl303xx_AprDeviceHybridAdjModeE *deviceHybridAdjMode);

/** zl303xx_AprGetServerPullInRange

   Query the current pull-in range for a specified server.

   If the pull-in range for the server was not specified, then the pull-in
   range from the device structure is returned.

  Parameters:
   [in]   hwParams     Pointer to the device structure
   [in]   serverId     The server's ID

   [out]  pullInRange  The current pull-in range of the device. The enum
                          represents a value in ppb

  Return Value:
     ZL303XX_OK  Success
     ZL303XX_STREAM_NOT_IN_USE  if the server is not configured

****************************************************************************/
zlStatusE zl303xx_AprGetServerPullInRange(void *hwParams,
                                       Uint32T serverId,
                                       zl303xx_AprPullInRangeE *pullInRange);

/* zl303xx_AprReconfigureServerPullInRange

   Change the pull-in range for the given server.

  Parameters:
   [in]   hwParams     Pointer to the device structure
   [in]   serverId     The server's ID
   [in]   pullInRange  The pull-in range in ppb

  Return Value:
     ZL303XX_OK  Success
     ZL303XX_STREAM_NOT_IN_USE  if the server is not configured

****************************************************************************/
zlStatusE zl303xx_AprReconfigureServerPullInRange(void *hwParams,
                                       Uint32T serverId,
                                       zl303xx_AprPullInRangeE pullInRange);

/* zl303xx_AprSetServerMode */
/**

   Change the mode (hybrid or packet) for the given server.

  Parameters:
   [in]   hwParams     Pointer to the device structure
   [in]   serverId     The server's ID
   [in]   serverMode   The new mode for the server.

  Return Value:
     ZL303XX_OK  Success
     ZL303XX_STREAM_NOT_IN_USE  if the server is not configured

****************************************************************************/
zlStatusE zl303xx_AprSetServerMode(void *hwParams,
                                Uint32T serverId,
                                zl303xx_AprServerRefModeE serverMode);


/** zl303xx_AprGetDevicePhaseLockStatusThresholds

   Query the current thresholds to enter/exit phase lock state for a specified device.

  Parameters:
   [in]   hwParams           Pointer to the device structure.
   [out]  enterThreshold     The current threshold of the device to enter phase lock. Unit: ns
   [out]  exitThreshold      The current threshold of the device to exit phase lock. Unit: ns

  Return Value:  ZL303XX_OK  Success.

****************************************************************************/
zlStatusE zl303xx_AprGetDevicePhaseLockStatusThresholds
               (
               void *hwParams,
               Uint32T *enterThreshold,
               Uint32T *exitThreshold
               );

/** zl303xx_AprSetProxyTxDisabled

   Sets proxyTxEnable = FALSE.

   proxyTxEnable = FALSE stops the device from sending redundancy data to
   the monitor.

  Parameters:
   [in]   hwParams           Pointer to the device structure.

  Return Value:  ZL303XX_OK  Success.

****************************************************************************/
zlStatusE zl303xx_AprSetProxyTxDisabled
               (
               void *hwParams
               );

/** zl303xx_AprSetProxyTxEnabled

   Sets proxyTxEnable = TRUE.

   proxyTxEnabled = TRUE causes the device to send redundancy data to the
   monitor.

  Parameters:
   [in]   hwParams           Pointer to the device structure.

  Return Value:  ZL303XX_OK  Success.

****************************************************************************/
zlStatusE zl303xx_AprSetProxyTxEnabled
               (
               void *hwParams
               );

/** zl303xx_AprSetProxyRxDisabled

   Sets proxyRxEnable = FALSE.

   proxyRxEnabled = FALSE causes the device to discard data from the active.

  Parameters:
   [in]   hwParams           Pointer to the device structure.

  Return Value:  ZL303XX_OK  Success.

****************************************************************************/
zlStatusE zl303xx_AprSetProxyRxDisabled
               (
               void *hwParams
               );

/** zl303xx_AprSetProxyRxEnabled

   Sets proxyRxEnable = TRUE.

   proxyRxEnabled = TRUE causes the device to start accepting data from
   the active.

  Parameters:
   [in]   hwParams           Pointer to the device structure.

  Return Value:  ZL303XX_OK  Success.

****************************************************************************/
zlStatusE zl303xx_AprSetProxyRxEnabled
               (
               void *hwParams
               );

/** zl303xx_AprGetProxyRxEnabled

   Returns the value of the proxyRxEnabled variable.

  Parameters:
   [in]   zl303xx_Params     pointer to the device structure

   [out]  proxyRxEnabled   the variable value

  Return Value: zlStatusE

*****************************************************************************/
zlStatusE zl303xx_AprGetProxyRxEnabled
            (
            void *zl303xx_Params,
            zl303xx_BooleanE *proxyRxEnabled
            );

/** zl303xx_AprProcessRedundancyJumpTimeData

   Process jump time information sent from the active device.

   It is assumed that the calling routine has called zl303xx_AprGetProxyRxEnabled()
   to make sure we are able to accept this message.

  Parameters:
   [in]   zl303xx_Params  pointer to the device structure
   [in]   serverId      the destination server
   [in]   msg           pointer to the received message

  Return Value: zlStatusE

*****************************************************************************/
zlStatusE zl303xx_AprProcessRedundancyJumpTimeData
            (
            void *zl303xx_Params,
            Uint16T serverId,
            zl303xx_RedundancyMsgS *msg
            );

/** zl303xx_AprProcessRedundancyAdjustTimeData

   Process adjust time information sent from the active device.

  Parameters:
   [in]   zl303xx_Params  pointer to the device structure
   [in]   serverId      the destination server
   [in]   msg           pointer to the received message

  Return Value: zlStatusE

*****************************************************************************/
zlStatusE zl303xx_AprProcessRedundancyAdjustTimeData
            (
            void *zl303xx_Params,
            Uint16T serverId,
            zl303xx_RedundancyMsgS *msg
            );

/** zl303xx_AprProcessRedundancyAdjFreqData

   Process information sent from the active device.

  Parameters:
   [in]   zl303xx_Params  pointer to the device structure
   [in]   serverId      the destination server
   [in]   msg           pointer to the received message

  Return Value: zlStatusE

*****************************************************************************/
zlStatusE zl303xx_AprProcessRedundancyAdjFreqData
            (
            void *zl303xx_Params,
            Uint16T serverId,
            zl303xx_RedundancyMsgS *msg
            );

/** zl303xx_AprProcessRedundancyCDataData

   Process information sent from the active device.

  Parameters:
   [in]   zl303xx_Params  pointer to the device structure
   [in]   serverId      the destination server
   [in]   msg           pointer to the received message

  Return Value: zlStatusE

*****************************************************************************/
zlStatusE zl303xx_AprProcessRedundancyCDataData
            (
            void *zl303xx_Params,
            Uint16T serverId,
            zl303xx_RedundancyMsgS *msg
            );

/** zl303xx_PFProcessRedundancyStateData

   This routine processes the state information from the active.

  Parameters:
   [in]   zl303xx_Params  pointer to the device structure
   [in]   serverId      the destination server
   [in]   msg           pointer to the received message

  Return Value: zlStatusE

*****************************************************************************/
zlStatusE zl303xx_PFProcessRedundancyStateData
            (
            void *zl303xx_Params,
            Uint16T serverId,
            zl303xx_RedundancyMsgS *msg
            );

/** zl303xx_AprProcessRedundancyJumpStandbyCGU

   Process information sent from the active device.

  Parameters:
   [in]   zl303xx_Params  pointer to the device structure
   [in]   serverId      the destination server
   [in]   msg           pointer to the received message

  Return Value: zlStatusE

*****************************************************************************/

zlStatusE zl303xx_AprProcessRedundancyJumpStandbyCGU
            (
            void *zl303xx_Params,
            Uint16T serverId,
            zl303xx_RedundancyMsgS *msg
            );

/** zl303xx_AprGetDevicePhaseLockNetworkConditionThresholds

   Query the current network condition thresholds to enter/exit phase lock
   state for a specified device.

  Parameters:
   [in]   hwParams           Pointer to the device structure.
   [out]  enter1HzNetworkConditionThreshold     The current 1Hz network condition
                                threshold of the device to enter phase lock. Unit: 1-100

  Return Value:  ZL303XX_OK  Success.

****************************************************************************/
zlStatusE zl303xx_AprGetDevicePhaseLockNetworkConditionThresholds
               (
               void *hwParams,
               Uint32T *enter1HzNetworkConditionThreshold
               );

/**  zl303xx_AprGetDevice1HzMostRecentAlignmentDifference

   This routine returns information about the most recent 1Hz alignment attempt.
   In particular:
      mostRecentCalculatedOffset - the most recent 1Hz alignment difference
         in nanoseconds
      mostRecentEstimatedNetworkQuality - the network quality of the most
         recent 1Hz alignment difference
      mostRecentAssigned - TRUE if the most recent 1Hz alignment difference
         was applied.

  Parameters:
   [in]  zl303xx_Params                      pointer to the zl303xx device data
   [out] mostRecentCalculatedOffset        ptr to the most recent 1Hz alignment difference
   [out] mostRecentEstimatedNetworkQuality ptr to the network quality of the most
                                                recent 1Hz alignment
   [out] mostRecentAssigned                ptr to assigned (TRUE if the most recent 1Hz alignment
                                                difference was applied)

  Return Value:
    ZL303XX_OK                    if successful
    ZL303XX_INVALID_POINTER       if
                                       - a passed-in pointer is NULL or
                                       - there is no current active reference
    ZL303XX_PARAMETER_INVALID     if the zl303xx_Params device has not been configured
    ZL303XX_NOT_RUNNING           if
                                       - APR is not running or
                                       - 1Hz is disabled on the given stream
    ZL303XX_ERROR                 internal error
    ZL303XX_RTOS_SEM_TAKE_FAIL    internal error

*****************************************************************************/

zlStatusE zl303xx_AprGetDevice1HzMostRecentAlignmentDifference
            (
            void *zl303xx_Params,
            Uint64S *mostRecentCalculatedOffset,
            Uint32T *mostRecentEstimatedNetworkQuality,
            zl303xx_BooleanE *mostRecentAssigned
            );

/**  zl303xx_AprGetDevice1HzMostRecentPathsData

   This routine returns information about the most recent 1Hz alignment attempt.
   In particular:
*     mostRecentCalculatedOffset - the most recent Offset From
*        Master in nanoseconds
      mostRecentAssigned - TRUE if the most recent 1Hz alignment difference
*        was applied
*        realTT
*                           mostRecentCalculatedMeanPathDelay in
*        nanoseconds

  Parameters:
   [in]  zl303xx_Params                      pointer to the zl303xx device data
   [out] mostRecentCalculatedOffset        ptr to the most*
recent OFM
   [out] mostRecentAssigned                ptr to assigned (TRUE if the most recent 1Hz alignment
*                                               difference was applied)
*   [out] mostRecentCalculatedMeanPathDelay   in nanoseconds
*   [out] mostRecentCalculatedForwardPathDelay   in nanoseconds
*   [out] mostRecentCalculatedReversePathDelay   in nanoseconds

  Return Value:
    ZL303XX_OK                    if successful
    ZL303XX_INVALID_POINTER       if
                                       - a passed-in pointer is NULL or
                                       - there is no current active reference
    ZL303XX_PARAMETER_INVALID     if the zl303xx_Params device has not been configured
    ZL303XX_NOT_RUNNING           if
                                       - APR is not running or
                                       - 1Hz is disabled on the given stream
    ZL303XX_ERROR                 internal error
    ZL303XX_RTOS_SEM_TAKE_FAIL    internal error

*****************************************************************************/


zlStatusE zl303xx_AprGetDevice1HzMostRecentPathsData
            (
            void *zl303xx_Params,
            Uint64S *mostRecentCalculatedOffset,
            Uint64S *realTTFor,
            zl303xx_BooleanE *mostRecentAssigned
            );

/**  zl303xx_AprGetServer1HzMostRecentPathsData

   This routine is eqivelant to zl303xx_AprGetDevice1HzMostRecentAlignmentDifference()
   except that this routine accepts a server rather than assuming the
   active server.

*****************************************************************************/
zlStatusE zl303xx_AprGetServer1HzMostRecentPathsData
            (
            void *zl303xx_Params,
            Uint16T serverId,
            Uint64S *mostRecentCalculatedOffset,
            Uint64S *realTTFor,
            zl303xx_BooleanE *mostRecentAssigned
            );

/**  zl303xx_AprGetServer1HzMostRecentAlignmentDifference

   This routine is equivalent to zl303xx_AprGetDevice1HzMostRecentAlignmentDifference()
   except that this routine accepts a server rather than assuming the
   active server.

*****************************************************************************/
zlStatusE zl303xx_AprGetServer1HzMostRecentAlignmentDifference
            (
            void *zl303xx_Params,
            Uint16T serverId,
            Uint64S *mostRecentCalculatedOffset,
            Uint32T *mostRecentEstimatedNetworkQuality,
            zl303xx_BooleanE *mostRecentAssigned
            );

/** zl303xx_AprGet1HzMetric

   This routine returns the 1Hz metric.

   The metric consists of the mean and standard deviation of the last
   ZL303XX_MAX_1HZ_METRIC_VALUES (default 10) 1Hz phase adjustments.

  Parameters:
   [in]  zl303xx_Params   pointer to the zl303xx device data
   [in]  serverId       the server

   [out] mean           pointer to the mean
   [out] std            pointer to the standard deviation

  Return Value: zlStatusE

*****************************************************************************/

zlStatusE zl303xx_AprGet1HzMetric
            (
            void *hwParams,
            Uint16T serverId,
            Sint32T *mean,
            Uint32T *std
            );

/** zl303xx_Apr1HzResetMetric

   This routine resets the 1Hz metric. This includes discarding all existing
   adjustment values.

  Parameters:
   [in]  zl303xx_Params   pointer to the zl303xx device data
   [in]  serverId       the server

  Return Value: zlStatusE

*****************************************************************************/

zlStatusE zl303xx_Apr1HzResetMetric
            (
            void *hwParams,
            Uint16T serverId
            );

/** zl303xx_AprGetDeviceCurrActiveRef

   Query the current packet/hybrid reference server being synchronised by the specified device.

  Parameters:
   [in]    hwParams      Pointer to the device structure

   [out]     serverId      The identifier of current reference server if the device in the packet mode or
                            hybrid mode.
                            Value of 65535 if the device is in the electrical mode.

  Return Value: ZL303XX_OK                if successful
****************************************************************************/
zlStatusE zl303xx_AprGetDeviceCurrActiveRef (void *hwParams, Uint16T *serverId);

/** zl303xx_AprGetDeviceRegisterServerNum

   Query how many servers (both packet and hybrid) are registered in the specified device.

  Parameters:
   [in]    hwParams      Pointer to the device structure

   [out]     serverNum      The number of servers registered in the device.

  Return Value: ZL303XX_OK                if successful
****************************************************************************/
zlStatusE zl303xx_AprGetDeviceRegisterServerNum (void *hwParams, Uint16T *serverNum);

/** zl303xx_AprGetServerFreqOffset

   Query the frequency offset of the current server.

  Parameters:
   [in]    hwParams      Pointer to the device structure

   [out]     freqOffset   32-bit value in the resolution of 0.001ppb.

  Return Value: ZL303XX_OK                if successful
****************************************************************************/
zlStatusE zl303xx_AprGetServerFreqOffset (void *hwParams, Sint32T *freqOffset);


/** zl303xx_AprGetFreqOffsetByServer

   Query the frequency offset of the given server.

  Parameters:
   [in]    hwParams      Pointer to the device structure
   [in]    serverId      The server ID

   [out]     freqOffset   32-bit value in the resolution of 0.001ppb.

  Return Value: ZL303XX_OK                if successful
****************************************************************************/
zlStatusE zl303xx_AprGetFreqOffsetByServer (void *hwParams, Sint32T serverId, Sint32T *freqOffset);


/****************** Dynamic Packet Rate Interface *************************/
/** zl303xx_AprNotifyServerPktRateChange

   This function is called to notify APR if the packet rate changes for
   the specified server in either forward or reverse path.

  Parameters:
   [in]  hwParams       Pointer to the device structure
   [in]  serverId       Server ID
   [in]  bFwdPath       Indicate if it is the forward path or not
   [in]  newPktRate     Indicate the new packet rate using the zl303xx_AprPktRateE
                              - positive values: packet-per-second (pps)
                              - negative values: seconds-per-packet (spp)

  Return Value: ZL303XX_OK           if successful
****************************************************************************/
zlStatusE zl303xx_AprNotifyServerPktRateChange
                  (
                  void *hwParams,
                  Uint16T serverId,
                  zl303xx_BooleanE bFwdPath,
                  zl303xx_AprPktRateE newPktRate
                  );


/****************** Reference Switch *************************/

/** zl303xx_AprSetActiveRef

   Change the current reference clock to a new packet or hybrid clock for the
   specified device.

  Parameters:
   [in]    hwParams          Pointer to the device structure
   [in]    serverId          Identifier of new reference clock

  Return Value: ZL303XX_OK                if successful

  Notes: If called with 0xFFFF (= 65535), the APR state will go to ZL303XX_NO_ACTIVE_REF
(See zl303xx_AprAllowSetActiveRef() to unset.)

****************************************************************************/
zlStatusE zl303xx_AprSetActiveRef(void *hwParams, Uint16T serverId);

/** zl303xx_AprSetActiveRefUserOptions

   Change the current reference clock to a new packet or hybrid clock for the
   specified device.  Provides additional user options.

  Parameters:
   [in]    hwParams          Pointer to the device structure
   [in]    serverId          Identifier of new reference clock
   [in]    refSwitchFreqSelectOption   Selection ENUM for new active server frequency
   [in]    refSwitchUserFreq           User provided new active server frequency if ENUM is set

  Return Value: ZL303XX_OK                if successful
****************************************************************************/
zlStatusE zl303xx_AprSetActiveRefUserOptions(void *hwParams,
                                           Uint16T serverId,
                                           zl303xx_AprRefSwitchFreqSelectE refSwitchFreqSelectOption,
                                           Sint32T refSwitchUserFreq);

/** zl303xx_AprAllowSetActiveRef

   Change operation back to packet server controlling the CGU for the
    specified device. Set-no-active-ref is enabled by a call to
    zl303xx_AprSetActiveRef(hwParams, 0xFFFF). This routine will unset
    the operation and the next call to zl303xx_AprSetActiveRef() will succeed.

  Parameters:
   [in]    hwParams         Pointer to the device structure

  Return Value: ZL303XX_OK                if successful
****************************************************************************/
zlStatusE zl303xx_AprAllowSetActiveRef (void *hwParams);


/** zl303xx_AprSetActiveElecRef

   Change the current packet or hybrid reference clock to a new electrical clock for the
   specified device.

  Parameters:
   [in]    hwParams                Pointer to the device structure
   [in]    refId                   Identifier of new
                                      electrical reference clock

  Return Value: ZL303XX_OK                if successful
****************************************************************************/
zlStatusE zl303xx_AprSetActiveElecRef(void *hwParams, Uint16T refId);

/** zl303xx_AprSetActiveElecRefSync

   Change the current packet or hybrid reference clock to a new electrical clock for the
   specified device.

  Parameters:
   [in]    hwParams                Pointer to the device structure
   [in]    refId                   Identifier of new
                                      electrical reference clock
   [in]    syncId                  Identifier of new
                                      electrical sync pulse

  Return Value: ZL303XX_OK                if successful
****************************************************************************/
zlStatusE zl303xx_AprSetActiveElecRefSync(void *hwParams, Uint16T refId, Uint16T syncId);

/** zl303xx_AprSetActiveElecServerOnly

   Change the current server to a electrical mode.  Does not perform any device register actions.
   This function should be coupled with zl303xx_AprSetActiveElecHwDeviceOnly.

  Parameters:
   [in]    hwParams          Pointer to the device structure

  Return Value: ZL303XX_OK                if successful
****************************************************************************/
zlStatusE zl303xx_AprSetActiveElecServerOnly (void *hwParams);

/** zl303xx_AprSetActiveElecHwDeviceOnly

   Change the current device to electrical mode via register actions.  It necessary, initiates
   hitless electrical compensation.
   This function should be coupled with zl303xx_AprSetActiveElecServerOnly.

  Parameters:
   [in]    hwParams          Pointer to the device structure

  Return Value: ZL303XX_OK                if successful
****************************************************************************/
zlStatusE zl303xx_AprSetActiveElecHwDeviceOnly(void *hwParams, Sint32T refId, Sint32T syncId);

/****************** Registered Clock Generation Device Control *************************/

/** zl303xx_AprSetHoldover

 * Manually set APR to enter or leave holdover state for the
 * active server on the specified device with OS-delay.

  Parameters:
   [in]    hwParams         Pointer to the device structure
   [in]     bHoldover         Indicate to enter/leave holdover manually
                        ZL303XX_TRUE:    enter to holdover
                        ZL303XX_FALSE:    leave from holdover

  Return Value: ZL303XX_OK                if successful

****************************************************************************/
zlStatusE zl303xx_AprSetHoldover (void *hwParams, zl303xx_BooleanE bHoldover);

/** zl303xx_AprSetHoldoverNoDelay

 * Manually set APR to enter or leave holdover state for the
 * active server on the specified device without OS-delay

  Parameters:
   [in]    hwParams          Pointer to the device structure
   [in]    bHoldover         Indicate to enter/leave holdover manually
                                ZL303XX_TRUE:     enter to holdover
                                ZL303XX_FALSE:    leave from holdover

  Return Value: ZL303XX_OK                if successful
****************************************************************************/

zlStatusE zl303xx_AprSetHoldoverNoDelay (void *hwParams, zl303xx_BooleanE bHoldover);

/** zl303xx_AprSetDevicePhaseLockStatusThresholds

   Set the thresholds to enter/exit phase lock state for a specified device.

  Parameters:
   [in]   hwParams           Pointer to the device structure.
   [in]   enterThreshold     The threshold of the device to enter phase lock to be set. Unit: ns
                                If the value is 0, no change for the current threshold.
   [in]   exitThreshold      The threshold of the device to exit phase lock to be set. Unit: ns
                                If the value is 0, no change for the current threshold.

  Return Value:  ZL303XX_OK  Success.

****************************************************************************/
zlStatusE zl303xx_AprSetDevicePhaseLockStatusThresholds
               (
               void *hwParams,
               Uint32T enterThreshold,
               Uint32T exitThreshold);

/** zl303xx_AprSetDevicePhaseLockNetworkConditionThresholds

   Set the thresholds to enter/exit phase lock state for a specified device.

  Parameters:
   [in]   hwParams           Pointer to the device structure.
   [in]   enter1HzNetworkConditionThreshold The threshold of the device to enter phase
                                lock to be set. Unit: 1-100
                                If the value is 0, no change for the current threshold.

  Return Value:  ZL303XX_OK  Success.

****************************************************************************/
zlStatusE zl303xx_AprSetDevicePhaseLockNetworkConditionThresholds
               (
               void *hwParams,
               Uint32T enter1HzNetworkConditionThreshold);

zlStatusE zl303xx_GetAPRSanityCounters
            (
            void *zl303xx_Params,
            Uint32T s,
            Uint32T val,
            Uint32T *tsc
            );

/** zl303xx_AprSetDevicePostAprOffset

   This function sets the df offset that is added to the holdover df when
   the CGU is in HOLDOVER or FREERUN.

  Parameters:
   [in]  hwParams       pointer to the device structure
   [in]  postAprOffset  the offset to apply in ppt

  Return Value: zlStatusE

****************************************************************************/
zlStatusE zl303xx_AprSetDevicePostAprOffset
                    (
                    void *hwParams,
                    Sint32T postAprOffset
                    );

/** zl303xx_AprGetDevicePostAprOffset

   This function returns the df offset that is added to the holdover df when
   the CGU is in HOLDOVER or FREERUN.

  Parameters:
   [in]  hwParams       pointer to the device structure
   [out] postAprOffset  the offset in ppt

  Return Value: zlStatusE

****************************************************************************/
zlStatusE zl303xx_AprGetDevicePostAprOffset
                    (
                    void *hwParams,
                    Sint32T *postAprOffset
                    );

/** zl303xx_AprSetTime

   This function modifies ToD value on the device.

  Parameters:
   [in]    hwParams         pointer to the device structure
   [in]    dtSec             integer portion of time adjustment value in the unit of second
   [in]     dtNanoSec         fractional portion of time adjustment in the unit of nanosecond
   [in]     bBackwardAdjust   if ZL303XX_TRUE, perform backward adjustment;
                        if ZL303XX_FALSE, perform forward adjustment;

  Return Value:
    ZL303XX_OK                if successful
    ZL303XX_PARAMETER_INVALID if the modification failed

****************************************************************************/
zlStatusE zl303xx_AprSetTime
               (
               void *hwParams,
               Uint64S dtSec,
               Uint32T dtNanoSec,
               zl303xx_BooleanE bBackwardAdjust
               );




/**********************************************************************************
 *
 *                       Timestamp Input Interface
 *
**********************************************************************************/
typedef struct {
   /*!  Var: zl303xx_AprTimeRepresentationS::second
           The second information of timestamps, if available. This field is not used in the case of RTP. */
   Uint64S second;

   /*!  Var: zl303xx_AprTimeRepresentationS::subSecond
           The sub-second information of timestamps.
           In the case of PTP, indicate nanosecond information, this field is always less than 10<SUP>9</SUP>;
           In the case of NTP, indicate fractional second information;
           In the case of RTP, indicate the 32-bit timestamp; */
   Uint32T subSecond;
}zl303xx_AprTimeRepresentationS;


typedef struct {
   /*!  Var: zl303xx_AprTimestampS::serverId
            A unique 16-bit ID used to reference the master clock. */
   Uint16T serverId;

   /*!  Var: zl303xx_AprTimestampS::rxTs
            A receive timestamp */
   zl303xx_AprTimeRepresentationS rxTs;

   /*!  Var: zl303xx_AprTimestampS::txTs
            A transmit timestamp */
   zl303xx_AprTimeRepresentationS txTs;

   /*!  Var: zl303xx_AprTimestampS::corr
            A 64-bit correction field data if PTP protocol is used */
   Uint64S corr;

   /*!  Var: zl303xx_AprTimestampS::bPeerDelay
            Indicates if this pair of timestamps contains data related to the
            peer-to-peer delay mechanism (i.e. the peerMeanPathDelay).  */
   zl303xx_BooleanE bPeerDelay;

   /*!  Var: zl303xx_AprTimestampS::peerMeanDelay
            A 64-bit peerMeanPathDelay value when PEER_DELAY Mechanism is used. */
   Uint64S peerMeanDelay;

   /*!  Var: zl303xx_AprTimestampS::bForwardPath
            Indicates if this is a pair of timestamps for the forward path or not.
            In the case of PTP, they are timestamps of (t1, t2) if it is true;
            In the case of NTP, they are timestamps of (t3, t4) if it is true; */
   zl303xx_BooleanE bForwardPath;

   /*!  Var: zl303xx_AprTimestampS::offsetFromMaster
            A 64-bit offsetFromMaster value (2's complement signed nanoseconds)
            .hi contains most-significant 32-bit (and the sign bit)
            .lo contains least-significant 32-bit */
   Uint64S offsetFromMaster;

   /*!  Var: zl303xx_AprTimestampS::offsetFromMaster
            TRUE if the offsetFromMaster Master value is valid */
   zl303xx_BooleanE offsetFromMasterValid;

   /*!  Var: zl303xx_AprTimestampS::pX
            Reserved for the future use. */
   void *pX;
} zl303xx_AprTimestampS;


/* zl303xx_AprProcessTimestamp
   This is the timestamp input interface of APR. This function is used to input timestamps
   from a registered server to APR application. It should be called by customer application
   whenever receiving an incoming timing packet.

   This function can be used for either incoming or return timestamps.

  Parameters:
   [in]  hwParams         Pointer to the structure for this device instance
   [in]  tsInput         Pointer to the timestamp structure

  Return Value: zlStatusE

  Notes:    If the device or server is not found in the APR application, APR performs
         nothing and returns ZL303XX_OK, but the corresponding info will be logged at the
         log level 3.

 *******************************************************************************/
zlStatusE zl303xx_AprProcessTimestamp(void *hwParams, zl303xx_AprTimestampS *tsInput);


/** zl303xx_AprStartDiscardTimer
   When the monitor device starts adjTime mechanism, this routine can be used to
   discard packets for the given time

  Parameters:
   [in]  hwParams       Pointer to the structure for this device instance
   [in]  timeout        The number of seconds to discard packets

  Return Value: zlStatusE

 *******************************************************************************/
zlStatusE zl303xx_AprStartAdjTimeDiscardTimer(void *zl303xx_Params, Uint32T timeout);

/** zl303xx_AprStopAdjTimeDiscardTimer

   Stops the adjustTime discard 'timer'. While the discard timer is
   running, packets are discarded.

  Parameters:
   [in]  zl303xx_Params   Pointer to the structure for this device instance

  Return Value: zlStatusE
 *******************************************************************************/
zlStatusE zl303xx_AprStopAdjTimeDiscardTimer(void *zl303xx_Params);

/*******************************************************************************
 * Debug functions
 */

/* zl303xx_AprGetPllMonMode
   This debug function is used to return an internal monitoring state of APR.

  Parameters:
   [in]  hwParams        Pointer to the structure for this device instance
   [out] enabled         Pointer to the boolean whether it is enabled
 *******************************************************************************/
zlStatusE zl303xx_AprGetPllMonMode(void *hwParams, zl303xx_BooleanE *enabled);

/* zl303xx_AprGetDpllMonMode
   This debug function is used to return an internal monitoring state of APR.

  Parameters:
   [in]  hwParams        Pointer to the structure for this device instance
   [out] enabled         Pointer to the boolean whether it is enabled
 *******************************************************************************/
zlStatusE zl303xx_AprGetDpllMonMode(void *hwParams, zl303xx_BooleanE *enable);


/** zl303xx_AprLogTimestampInputStart
   This debug function is used to start logging the raw timestamps received by APR.

  Parameters:
   [in]  tsLogPath        Indicate which timestamp streams to be logged:
                             0 - Log the timestamps of forward path only;
                             1 - Log the timestamps of reverse path only;
                             2 - Log the timestamps of both forward and reverser paths;
 *******************************************************************************/
void zl303xx_AprLogTimestampInputStart(Uint8T tsLogPath);


/** zl303xx_AprLogTimestampInputStop
   This debug function is used to stop logging the raw timestamps received by APR.
 *******************************************************************************/
void zl303xx_AprLogTimestampInputStop(void);


/** zl303xx_AprLogTsReceivedStart
   This debug function is used to start logging how many timestamps received by APR
   in the period of the main APR logging period (10000 ms default).
 *******************************************************************************/
void zl303xx_AprLogTsReceivedStart(void);


/** zl303xx_AprLogTsReceivedStart
   This debug function is used to stop logging how many timestamps received by APR
   in the period of the main APR logging period (10000 ms default).
 *******************************************************************************/
void zl303xx_AprLogTsReceivedStop(void);


/* zl303xx_AprSetTimeStamperCGUOffset

   Provide an offset value (from the 1Hz Pulse) to be applied to the local timestamp.

   Note: Seconds are stored as 64 bit Signed in an Unsigned struct - signed extension
   is assumed.  For example: to subtract 2 seconds 513 ns, the following is used:
   zl303xx_AprTimeRepresentationS cguOffset = {{0,0xFFFFFFFE},0xFFFFFDFF}

   When the timestamper cannot modify its time, you must supply the offset by using
   zl303xx_AprSetTimeStamperCGUOffset(). This difference is between the CGU's 1Hz pulse
   and the timestamper's internal counter.

   By removing the supplied offset, we align the timestamp with the CGU's 1Hz pulse.

  Parameters:
   [in]  hwParams        Pointer to the CGU device
   [in]  cguOffset       Offset to apply stored as zl303xx_AprTimeRepresentationS
 *******************************************************************************/
zlStatusE zl303xx_AprSetTimeStamperCGUOffset(void *hwParams, zl303xx_AprTimeRepresentationS cguOffset);

/* zl303xx_AprGetTimeStamperCGUOffset

   Returns the current offset value being applied to the local timestamp.

   Note: Seconds are stored as 64 bit Signed in an Unsigned struct - signed extension
   is assumed.

  Parameters:
   [in]  hwParams        Pointer to the CGU device
   [out] cguOffset       Pointer to Offset being applied
 *******************************************************************************/
zlStatusE zl303xx_AprGetTimeStamperCGUOffset(void *hwParams, zl303xx_AprTimeRepresentationS *cguOffset);


/** Unused at this time */
/* zl303xx_AprSetResetReady
 * Modify the boolean flag to monitor the resetReady bit after a device reset - ZL devices only
 */
zlStatusE zl303xx_AprSetResetReady(zl303xx_BooleanE enable);

/** Unused at this time */
/* zl303xx_AprGetResetReady
 * Get the boolean resetReady flag  - ZL devices only
 * */
zl303xx_BooleanE zl303xx_AprGetResetReady(void);

/** zl303xx_AprSetOutputOffset

   This routine sets a static phase offset that will applied immediately.

   This function can be used to compensate for known asymmetry in a packet
   network.

   Positive values of offsetNs cause the 1Hz pulse to be advanced i.e. the
   1Hz pulse will occur early.

  Parameters:
   [in]   zl303xx_Params        pointer to the device structure
   [in]   offsetNs            phase offset in nanoseconds: +/-500000000

  Return Value: zlStatusE

*****************************************************************************/
zlStatusE zl303xx_AprSetOutputOffset(void *zl303xx_Params, Sint32T offsetNs);

/** zl303xx_AprSetOutputOffsetNext1Hz

   This routine sets a static phase offset that will be added to the adjustment
   made by the next 1Hz alignment.

   This function can be used to compensate for known asymmetry in a packet
   network.

   Positive values of offsetNs cause the 1Hz pulse to be advanced i.e. the
   1Hz pulse will occur early.

  Parameters:
   [in]   zl303xx_Params        pointer to the device structure
   [in]   offsetNs            phase offset in nanoseconds: +/-500000000

  Return Value: zlStatusE

*****************************************************************************/
zlStatusE zl303xx_AprSetOutputOffsetNext1Hz(void *zl303xx_Params, Sint32T offsetNs);

/** zl303xx_AprSetOutputOffsetImmediateBypassChecks

   This routine sets a static phase offset that will applied immediately.

  Parameters:
   [in]   zl303xx_Params        pointer to the device structure
   [in]   offsetNs            phase offset in nanoseconds: +/-500000000

  Return Value: zlStatusE

*****************************************************************************/
zlStatusE zl303xx_AprSetOutputOffsetImmediateBypassChecks(void *zl303xx_Params, Sint32T offsetNs);

/** zl303xx_AprGetOutputOffset

   This routine gets the static phase offset that will be added to the adjustment
   made by the next 1Hz alignment.

  Parameters:
   [in]   zl303xx_Params        pointer to the device structure
   [out]  offsetNs            phase offset in nanoseconds: +/-500000000

  Return Value: zlStatusE

*****************************************************************************/
zlStatusE zl303xx_AprGetOutputOffset(void *zl303xx_Params, Sint32T *offsetNs);

/** zl303xx_AprGetPSLFCLConfigureData

   This routine gets the PSL/FCL configuration data. This should be the
   same data sent to APR when the device was added.

  Parameters:
   [in]   zl303xx_Params        pointer to the device structure
   [out]  par                 pointer to the returned data

  Return Value: zlStatusE

*****************************************************************************/
zlStatusE zl303xx_AprGetPSLFCLConfigureData(void *zl303xx_Params, zl303xx_PFConfigS *par);

/**

   This routine gets hardware lock status.

  Parameters:
   [in]   zl303xx_Params        pointer to the device structure
   [out]  hwLockStatus        the lock status

  Return Value: zlStatusE

*****************************************************************************/
zlStatusE zl303xx_AprGetHwLockStatus(void *zl303xx_Params, zl303xx_AprLockStatusE *hwLockStatus);

/** zl303xx_AprGetFwdStreamNumByServerId

   This routine gets the forward stream identifier for the provided
   device and server.

*

  Parameters:
   [in]   zl303xx_Params        Pointer to the device structure*
   [in]   serverId            Server identifier
   [out]  fwdStreamId         Pointer to forward stream ID

*****************************************************************************/
zlStatusE zl303xx_AprGetFwdStreamNumByServerId(void *zl303xx_Params, Uint16T serverId, Uint32T *fwdStreamId);

/** zl303xx_AprPrintParameters

   This routine prints the APR parameters given the reference ID.

   If forward and reverse streams exist, then both sets of parameters are
   printed.

  Parameters:
   [in]  zl303xx_Params      pointer to the device instance
   [in]  referenceID       the reference ID from which to get the data

  Return Value:
    ZL303XX_OK                       if successful
    ZL303XX_INVALID_POINTER          if hwParams is NULL
    ZL303XX_NOT_RUNNING              if zl303xx_InitDevice() has not been
                                       called i.e. other tasks and data have
                                       not been initialised yet.
    ZL303XX_STREAM_NOT_IN_USE        if referenceID cannot be found
    ZL303XX_PARAMETER_INVALID        internal error

****************************************************************************/
zlStatusE zl303xx_AprPrintParameters(void *hwParams, Uint32T referenceID);


/** zl303xx_PFReConfigurePFStructInit

   This routine initializes the re-configure par structure.

  Parameters:
   [out]  par               pointer to the initialization data

  Return Value:
    ZL303XX_OK                       if successful
    ZL303XX_INVALID_POINTER          if par is NULL

*****************************************************************************/

zlStatusE zl303xx_PFReConfigurePFStructInit
            (
            zl303xx_PFReConfigS *par
            );

/** zl303xx_PFReConfigurePF

   The function re-configures the PSL and FCL values in a running system.

   The following values can be modified:
      1) frequency change limit
      2) phase slope limit

   The phase slope limit is controlled by the following values:
      1) fast lock frequency limit
      2) locked frequency limit
      3) not-locked frequency change limit
      4) phase slope limit

   The PSL/FCL settings take effect immediately even if a realignment is in
   progress. However if the new values are larger then no change is observed
   until the next realignment.

   At startup, it is advantageous to specify a phase slope limit that exceeds
   the normal phase slope limit so that the initial, large phase offset can be
   aligned quickly. This value is the fast lock frequency limit. The default
   value is 4000ns/s.

   After startup, one of 'locked frequency limit', 'not-locked frequency change
   limit' and 'phase slope limit' is used.

   In a stable running system, a small phase slope limit may be required to
   meet performance masks; this value is the 'locked frequency limit'. The
   default value is 4ns/s. The check for locked/not-locked is controlled by
   the configurable parameters lockInThreshold, lockOutThreshold, and
   lockInCount - these parameters are not re-configurable in a running system.
   Note: this locked/not-locked is NOT the APR lock value - this lock value
         is internal to the PSL/FCL component.

   In a stable running system that has not yet locked, the 'not-locked frequency
   change limit' is used. The default value is 1000ns/s

   If the phase slope limit is smaller than either 'locked frequency limit' or
   'not-locked frequency change limit' when they are used, then the 'phase slope
   limit' is used it its place. This allows overall control of the phase slope
   limit. The default value is 885ns/s. Note that at 885ns/s, the 'not-locked
   frequency change limit' is never used.


  Return Value: zlStatusE

****************************************************************************/
zlStatusE zl303xx_PFReConfigurePF
            (
            void *zl303xx_Params,
            zl303xx_PFReConfigS *par
            );

/** zl303xx_PFSetPhaseAdjustment

   The function copies the given data into internal PSL/FCL data structures
   for later processing.

   When the data is processed, the local time-of-day will be moved by the
   given value.

  Parameters:
   [in]   zl303xx_Params     pointer to the zl303xx device data
   [in]   seconds          the seconds to adjust
   [in]   nanoSeconds      the nanoseconds to adjust
   [in]   bBackwardAdjust  TRUE if the time is in the past

  Return Value: zlStatusE

****************************************************************************/
zlStatusE zl303xx_PFSetPhaseAdjustment
            (
            void *zl303xx_Params,
            Uint64S seconds,
            Uint32T nanoSeconds,
            zl303xx_BooleanE bBackwardAdjust
            );


/**  zl303xx_PFNotifyJumpComplete

   This routine is called by the user after a timestamp jump (setTime(),
   stepTime(), or adjTime()) to indicate that timestamps have stabilised and
   should be processed.

  Parameters:
   [in]  zl303xx_Params            pointer to the zl303xx device data

  Return Value: zlStatusE

*****************************************************************************/
zlStatusE zl303xx_PFNotifyJumpComplete
            (
            void *zl303xx_Params
            );

/** zl303xx_PFGetStepTimeResolution

   Gets the resolution of the stepTime() operation.

  Parameters:
   [in]   hwParams      Pointer to the structure for this device instance.
   [in]   resolutionNs  Pointer to return the stepTime in nanoseconds.

  Return Value:  zlStatusE

*******************************************************************************/
zlStatusE zl303xx_PFGetStepTimeResolution
            (
            void *hwParams,
            Uint32T *resolutionNs
            );

/** zl303xx_PFSetStepTimeResolution

   Sets the resolution of the stepTime() operation.

  Parameters:
   [in]   hwParams      Pointer to the structure for this device instance.
   [in]   resolutionNs  Resolution to set, in nanoseconds.

  Return Value:  zlStatusE

*******************************************************************************/
zlStatusE zl303xx_PFSetStepTimeResolution
            (
            void *hwParams,
            Uint32T resolutionNs
            );

/** zl303xx_AprGetDeviceStepTimeResolutionsFreq

   This function retrieves the current device's StepTime resolution in frequency [Hz]. The
   StepTime resolution in ns may be obtained via 10^9 / Freq.

   The output frequencies are determined by device synthesizer and output configuration.

  Parameters:
   [in]   hwParams       Pointer to the structure for this device instance.
   [out]  clockFreqArray Point to output data structure

  Return Value:  zlStatusE

*******************************************************************************/
zlStatusE zl303xx_AprGetDeviceStepTimeResolutionsFreq
            (
            void *hwParams,
            zl303xx_ClockOutputFreqArrayS *clockFreqArray
            );

/** zl303xx_AprGetDeviceClockOutputsFreq

   This function retrieves the current device's output clock frequencies driven by
   current DPLL [Hz].

   The output frequencies are determined by device synthesizer and output configuration.
   The returned frequencies do not include outputs with phase step enabled.

  Parameters:
   [in]   hwParams       Pointer to the structure for this device instance.
   [out]  clockFreqArray Point to output data structure

  Return Value:  zlStatusE

*******************************************************************************/
zlStatusE zl303xx_AprGetDeviceClockOutputsFreq
            (
            void *hwParams,
            zl303xx_ClockOutputFreqArrayS *clockFreqArray
            );

/** zl303xx_AprGetServerType2BCenterFreq

   This function is intended for runtime debugging only.

   This function retrieves the Type2B algorithm center frequency for a specifc server.
   It is the responsibility of the caller to ensure that the server number is valid and
   running Type2B.

  Parameters:
   [in]   hwParams      Pointer to the structure for this device instance.
   [in]   serverId      The serverId.
   [out]  freqPpt       The frequency offset in ppt.

  Return Value:  zlStatusE

*******************************************************************************/
zlStatusE zl303xx_AprGetServerType2BCenterFreq
            (
            void *hwParams,
            Uint32T serverId,
            Sint32T *freqPpt
            );


/** zl303xx_AprSetServerType2BCenterFreq

   This function is intended for runtime debugging only.

   This function sets the Type2B algorithm center frequency for a specifc server.
   It is the responsibility of the caller to ensure that the server number is valid and
   running Type2B.

  Parameters:
   [in]   hwParams      Pointer to the structure for this device instance.
   [in]   serverId      The serverId.
   [in]   freqPpt       The frequency offset in ppt.

  Return Value:  zlStatusE

*******************************************************************************/
zlStatusE zl303xx_AprSetServerType2BCenterFreq
            (
            void *hwParams,
            Uint32T serverId,
            Sint32T freqPpt
            );


/** zl303xx_AprForceServerType2BFastLock

   This function is intended for runtime debugging only.

   This function forces a number of Type2B fast lock operations.  Type2B fast lock
   is only applicable to the the active server.  It is the responsibility of the caller
   to ensure that the active server  is valid and running Type2B.

  Parameters:
   [in]   hwParams           Pointer to the structure for this device instance.
   [in]   numForceFastLock   The number of fast lock operations to force.

  Return Value:  zlStatusE

*******************************************************************************/
zlStatusE zl303xx_AprForceServerType2BFastLock
            (
            void *hwParams,
            Uint32T numForceFastLock
            );


/** zl303xx_AprGetSWHybridTransientState

   This is a simple APR wrapper function to get the hybrid transient state stored in the device
   host registers via the message router.

  Parameters:
   [in]    hwParams              Pointer to the structure for this device instance.
   [out]   BCHybridTransientType Pointer to output data structure

  Return Value:  zlStatusE

*******************************************************************************/
zlStatusE zl303xx_AprGetSWHybridTransientState
            (
            void *hwParams,
            zl303xx_BCHybridTransientType *BCHybridTransientType
            );


/** zl303xx_AprSetSWHybridTransientState

   This is a simple APR wrapper function to set the hybrid transient state stored in the device
   host registers via the message router.

  Parameters:
   [in]    hwParams              Pointer to the structure for this device instance.
   [out]   BCHybridTransientType Input data

  Return Value:  zlStatusE

*******************************************************************************/
zlStatusE zl303xx_AprSetSWHybridTransientState
            (
            void *hwParams,
            zl303xx_BCHybridTransientType BCHybridTransientType
            );


/** zl303xx_AprGetTimeSetBoolean

   This function retreives the initial set time done flag from a running system.

  Parameters:
   [in]    hwParams         Pointer to the structure for this device instance
   [out]   intialSetSeconds Pointer to inital seconds set Time value
   [out]   timeSetBoolean   Pointer to intial set Time completed boolean

  Return Value:  zlStatusE

*******************************************************************************/
zlStatusE zl303xx_AprGetTimeSetBoolean
            (
            void *hwParams,
            Sint32T *intialSetSeconds,
            zl303xx_BooleanE *timeSetBoolean
            );


/** zl303xx_AprSetTimeSetBoolean

   This function sets the initial set time done flag for a running system.

  Parameters:
   [in]   hwParams         Pointer to the structure for this device instance
   [in]   intialSetSeconds Inital seconds set Time value
   [in]   timeSetBoolean   Intial set Time completed boolean

  Return Value:  zlStatusE

*******************************************************************************/
zlStatusE zl303xx_AprSetTimeSetBoolean
            (
            void *hwParams,
            Sint32T intialSetSeconds,
            zl303xx_BooleanE timeSetBoolean
            );


/** zl303xx_AprTODDoneCallbackWithProtection

   Indicates the end of the stepTime() operation.

  Parameters:
   [in]   zl303xx_Params  Pointer to the structure for this device instance.
   [in]   protectCall   boolean to avoid double MUTEX take. TRUE if caller
                           wants MUTEX protection.

  Return Value:  zlStatusE

*******************************************************************************/
Sint32T zl303xx_AprTODDoneCallbackWithProtection(void *zl303xx_Params,
                                          zl303xx_BooleanE protectCall);


/**  zl303xxgetPFStateData

   This routine returns useful information about the PSL&FCL component.

  Parameters:
   [in]   zl303xx_Params        Pointer to the device instance
   [out]  PFLocked            ptr to the lock state
   [out]  PFLockCount         ptr to the lock count
   [out]  PFStaticOffset      ptr to the static offset being applied to all
                                   1Hz adjustments

  Return Value: zlStatusE

*****************************************************************************/
zlStatusE zl303xxgetPFStateData
            (
            void *zl303xx_Params,
            zl303xx_BooleanE *PFLocked,
            Sint32T *PFLockCount,
            Sint32T *PFStaticOffset
            );

zlStatusE zl303xx_PFSetOutputOffsetAction
            (
            void *zl303xx_Params,
            zl303xx_SetOutputOffsetActionE setOutputOffsetAction
            );
zlStatusE zl303xx_PFGetOutputOffsetAction
            (
            void *zl303xx_Params,
            zl303xx_SetOutputOffsetActionE *setOutputOffsetAction
            );

/** Used to configure time stamp logging. */
typedef struct
{
   /** How long time stamps should be logged for. */
   Uint32T durationSec;
   /** Function to be called for every PTP time stamp pair. */
   swFuncPtrTSLogging callback;
} zl303xx_AprTsLogConfigS;

/* zl303xx_AprTsLogConfigStructInit */
/**
   Fills a zl303xx_AprTsLogConfigS structure with invalid values. They must be
   changed before calling zl303xx_AprTsLogConfigSet().

  Parameters:
   [in]   hwParams  Pointer to the device instance.
   [in]   pConfig   Structure to initialize.

  Return Value:  zlStatusE

*******************************************************************************/
zlStatusE zl303xx_AprTsLogConfigStructInit
            (
            void *hwParams,
            zl303xx_AprTsLogConfigS *pConfig
            );

/* zl303xx_AprTsLogConfigSet */
/**
   Starts/stops logging of time stamps received by APR to a callback function.

  Parameters:
   [in]   hwParams  Pointer to the device instance.
   [in]   serverId  Handle to an existing stream.
   [in]   pConfig   Logging configuration.

  Return Value:  zlStatusE

*******************************************************************************/
zlStatusE zl303xx_AprTsLogConfigSet
            (
            void *hwParams,
            Uint16T serverId,
            zl303xx_AprTsLogConfigS *pConfig
            );

/* zl303xx_AprPrintAlgParamsUsingServerId */
/**
   Display Param data used by APR from a serverId.

  Parameters:
   [in]   hwParams      Pointer to the device instance.
   [in]   serverID      XParam data for this serverId.

  Return Value:  zlStatusE

*******************************************************************************/
zlStatusE zl303xx_AprPrintAlgParamsUsingServerId
            (
            void *zl303xx_Params,
            Uint32T serverID
            );

/* zl303xx_AprSetDynamicDFEnable */
/*
   Dynamically modify APRs ability to change DeltaFreq.
   This will later be used for XO aging and temp. compensation.
   Not fully implemented yet so BE CAREFUL!

  Parameters:
   [in]   hwParams       Pointer to the device instance.
   [in]   enable         On (1) or off.

  Return Value:  zlStatusE

*******************************************************************************/
zlStatusE zl303xx_AprSetDynamicDFEnable(void *hwParams, Uint8T enable);

/* zl303xx_AprSetDynamicDFOffset */
/*
   Dynamically modify the calculated DeltaFreq by adding a fixed offset.
   This will later be used for XO aging and temp. compensation.
   Not fully implemented yet so BE CAREFUL!

  Parameters:
   [in]   hwParams       Pointer to the device instance.
   [in]   dynamicDFInPpt Signed offsest to add (in ppt).

  Return Value:  zlStatusE

*******************************************************************************/
zlStatusE zl303xx_AprSetDynamicDFOffset(void *hwParams, Sint32T dynamicDFInPpt);

/* zl303xx_AprSetDynamicXOStability */
/**
   Dynamically modify the stability factor by adding a fixed offset.
   BE CAREFUL!

  Parameters:
   [in]   hwParams           Pointer to the device instance.
   [in]   dynamicXOStability Signed offsest to add (in ppt).

  Return Value:  zlStatusE

*******************************************************************************/
zlStatusE zl303xx_AprSetDynamicXOStability(void *hwParams, Sint64T dynamicXOStabilityInPpt);

/**  zl303xx_AprGetServerHoldoverValue

   This routine gets the server`s APR holdover value for the device.

  Parameters:
   [in]   zl303xx_Params      Pointer to the device instance
   [in]   serverNum         Server ID to query
   [out]  holdoverValue     Pointer to the server's holdover value
   [out]  holdoverValid     Pointer to indicate holdover valid

  Return Value: zlStatusE - ZL303XX_TABLE_EMPTY if not valid yet
                    ZL303XX_STREAM_NOT_IN_USE if serverNum not active

*****************************************************************************/
zlStatusE zl303xx_AprGetServerHoldoverValue
            (
            void *hwParams,
            Uint16T serverNum,
            Sint32T *holdoverValue,
            zl303xx_BooleanE *holdoverValid
            );

/**  zl303xx_AprGetCurrentHoldoverValue

   This routine gets the current APR holdover value for the current device.

  Parameters:
   [in]   zl303xx_Params      Pointer to the device instance
   [out]  holdoverValue     Pointer to the current value
   [out]  holdoverValid     Pointer to indicate holdover valid

  Return Value: zlStatusE - ZL303XX_TABLE_EMPTY if not valid yet

*****************************************************************************/
zlStatusE zl303xx_AprGetCurrentHoldoverValue(void *hwParams, Sint32T *holdoverValue, zl303xx_BooleanE *holdoverValid);

/**  zl303xx_AprSetServerHoldoverSettingsRuntime

   This routine sets the per-server holdover settings during runtime.

  Parameters:
   [in]   zl303xx_Params      Pointer to the device instance
   [in]   serverId          Server Id
   [in]   holdoverType      Holdover type to set
   [in]   holdoverUserVal   Holdover user value to set

  Return Value: zlStatusE

*****************************************************************************/
zlStatusE zl303xx_AprSetServerHoldoverSettingsRuntime(void *hwParams,
                                                    Uint32T serverId,
                                                    zl303xx_AprHoldoverTypeE holdoverType,
                                                    Sint32T holdoverUserVal);

/**  zl303xx_AprGetServerHoldoverSettingsRuntime

   This routine gets the per-server holdover settings during runtime.

  Parameters:
   [in]   zl303xx_Params      Pointer to the device instance
   [in]   serverId          Server Id
   [out]  holdoverType      Holdover type to get
   [out]  holdoverUserVal   Holdover user value to get

  Return Value: zlStatusE

*****************************************************************************/
zlStatusE zl303xx_AprGetServerHoldoverSettingsRuntime(void *hwParams,
                                                    Uint32T serverId,
                                                    zl303xx_AprHoldoverTypeE *holdoverType,
                                                    Sint32T *holdoverUserVal);


/** zl303xx_AprGetServerLogging

   This function is used to report APR logging on a per server basis.

  Parameters:
   [in]  zl303xx_Params   Pointer to the structure for this device instance
   [in]  serverId

  Return Value:
    0 | 1                          success
    -ZL303XX_ERROR                    if the specified device has not been
                                       registered
    -ZL303XX_TABLE_ENTRY_NOT_FOUND    if the specified serverId has not been
                                       registered

****************************************************************************/
Sint32T zl303xx_AprGetServerLogging(void *zl303xx_Params, Uint16T serverId);

/** zl303xx_AprSetServerLogging

   This function is used to enable APR logging on a per server basis.

  Parameters:
   [in]  zl303xx_Params   Pointer to the structure for this device instance
   [in]  serverId
   [in]  loggingOn      Enable or disable

  Return Value:
    ZL303XX_OK                       if successful
    ZL303XX_ERROR                    if the specified device has not been
                                       registered
    ZL303XX_TABLE_ENTRY_NOT_FOUND    if the specified serverId has not been
                                       registered

****************************************************************************/
zlStatusE zl303xx_AprSetServerLogging(void *zl303xx_Params, Uint16T serverId, zl303xx_BooleanE loggingOn);


/*****************   EXPORTED GLOBAL VARIABLE DECLARATIONS   ******************/
/* API build and release info strings */
extern const char zl303xx_AprBuildDate[];
extern const char zl303xx_AprBuildTime[];
extern const char zl303xx_AprReleaseDate[];
extern const char zl303xx_AprReleaseTime[];
extern const char zl303xx_AprReleaseVersion[];
extern const char zl303xx_AprReleaseSwId[];
extern const char zl303xx_AprReleaseType[];
extern const char zl303xx_AprPatchLevel[];
extern const unsigned int zl303xx_ApiReleaseVersionInt;
extern const unsigned int zl303xx_AprReleaseVersionInt;


/** zl303xx_GetAprQIFQSize

   Gets the length of the APR Queue

  Parameters:
   [in]   N/A.

  Return Value:  Number of queue elements configured

*******************************************************************************/
Uint32T zl303xx_GetAprQIFQLength(void);

/** zl303xx_SetAprQIFQLength

   Sets the length of the APR Queue

  Parameters:
   [in]   aprQIFSize - Number of queue elements (default is ZL303XX_APR_QIF_TASK_EVENTQ_SIZE)

  Return Value:  zlStatusE

*******************************************************************************/
zlStatusE zl303xx_SetAprQIFQLength(Uint32T aprQIFSize);

/** zl303xx_AprSetUserHandlesA_90000Reset

  Sets the indicator that the user will handle any a_90000
  errors (i.e., no auto-reset of algorithm)

  Parameters:
   [in]   userHandledReset - FALSE is auto-reset of algorithm

  Return Value:  zlStatusE

*******************************************************************************/
zlStatusE zl303xx_SetAprUserHandlesA_90000Reset(zl303xx_BooleanE userHandledReset);

/** zl303xx_GetAprUserHandlesA_90000Reset

  Gets the indicator that the user will handle any a_90000 errors

  Parameters:
   [in]   N/A.

  Return Value:  Boolean value

*******************************************************************************/
zl303xx_BooleanE zl303xx_GetAprUserHandlesA_90000Reset(void);


/** zl303xx_SetAprUserHandlesClearStats

* Sets the indicator that the user is asking to reset
* performance stats

  Parameters:
   [in]   ClearStats - TRUE to reset* performance*
stats******* stats****************
*

  Return Value:  zlStatusE

*******************************************************************************/
zlStatusE zl303xx_SetAprUserHandlerClearStats(void *hwParams);


/** zl303xx_SetAprUserChangeL4ThresholdValue

* Change the value of L4ThresholdValue


  Parameters:
   [in]   L4ThresholdValue

  Return Value:  zlStatusE

*******************************************************************************/
zlStatusE zl303xx_SetAprUserChangeL4ThresholdValue(void *hwParams, Uint32T L4ThresholdValue);

/** zl303xx_GetAprUserHandlesClearStats

* Gets the indicator to clear performance stats

  Parameters:
   [in]   N/A.

  Return Value:  Number of queue elements configured

*******************************************************************************/
zl303xx_BooleanE zl303xx_GetAprUserHandlesClearStats(void);


/**  zl303xx_AprWarmStartServer

   This routine warm starts a server on a device with an offset value.

  Parameters:
   [in]  hwParams          Pointer to the device instance
   [in]  serverId          User-provided serverId to restart
   [in]  warmStartValue    User-provided NCO offset value (Units: ppt)

  Return Value: zlStatusE

*****************************************************************************/
zlStatusE zl303xx_AprWarmStartServer
            (
            void *hwParams,
            Uint16T serverId,
            Sint32T warmStartValue
            );

/**  zl303xx_AprGetCurrentDF

   This routine queries a server for a delta freq. offset value.

  Parameters:
   [in]  hwParams          Pointer to the device instance
   [in]  serverId          User-provided serverId to query
   [in]  currentDF         Pointer to the currentDF offset value (Units: ppt)

  Return Value: zlStatusE

*****************************************************************************/
zlStatusE zl303xx_AprGetCurrentDF(void *hwParams, Uint16T serverId, Sint32T *currentDF);

/**  zl303xx_AprGetCurrentHWDF

   This routine queries APR for a hardware delta freq. offset value.

  Parameters:
   [in]  hwParams          Pointer to the device instance
   [in]  currentHWDF       Pointer to the current H/W DF offset value

  Return Value: zlStatusE

*****************************************************************************/
zlStatusE zl303xx_AprGetCurrentHWDF(void *hwParams, Sint32T *currentHWDF);

/**  zl303xx_AprResetClkStabilityStats

   This routine zeros the accumulated clock stability statistics.

  Parameters:
   [in]  hwParams          Pointer to the device instance

  Return Value: zlStatusE

*****************************************************************************/
zlStatusE zl303xx_AprResetClkStabilityStats(void *hwParams);

/**

  Function Name:
   zl303xx_AprGetDeviceTimeSetFlag

  Details:
    Retrieve the time set flag indicator for the specified device

  Parameters:
   [in]   hwParams       Pointer to the device instance
   [out]  bTimeSetFlag   Pointer to the boolean result

  Return Value:
   zlStatusE

 *******************************************************************************/
zlStatusE zl303xx_AprGetDeviceTimeSetFlag(void *hwParams, zl303xx_BooleanE *bTimeSetFlag);


typedef enum {
    ZL303XX_HO_CLEAR = 0,
    ZL303XX_HO_INCREMENT,
    ZL303XX_HO_FREEZE,

    ZL303XX_HO_LAST = ZL303XX_HO_FREEZE
} zl303xx_HoldoverActionE;

typedef enum {
    HOLDOVER_SYNCE_NOT_TRACEABLE        = 0,
    HOLDOVER_SYNCE_TRACEABLE,

    HOLDOVER_SYNCE_LAST = HOLDOVER_SYNCE_TRACEABLE
} zl303xx_SyncEStatusE;

typedef struct {
    zl303xx_BooleanE oneHzEnabled;
    zl303xx_BooleanE make1HzAdjustmentsDuringHoldover;
    Uint16T serverId;
    SINT_T currentAlgHoldover;
    SINT_T lastAlgHoldover;
    SINT_T currentHWDF;
    Uint32T holdSecs;
    zl303xx_AprDeviceRefModeE devMode;
    zl303xx_AprOscillatorFilterTypesE oscFilter;
    zl303xx_AprStateE aprState;
    zl303xx_FreqCatxE localOscillatorFreqCategory;
    zl303xx_FreqCatxE currentHoldoverCategory;
    Uint32T currentHoldoverAccuracyPpt;
    zl303xx_HoldoverActionE holdoverAction;
    zl303xx_HoldoverQualityTypesE holdoverQuality;
    zl303xx_SyncEStatusE syncEStatus;
    Uint32T currentSyncEAccuracyPpt;
    Uint32T inSpecHoldoverLimit;
    Uint32T holdoverTIEns;
    Uint32T priorTIEns;
    Uint32T holdoverIncrementCount;
    Uint32T holdoverSecsPerIncrement;
} zl303xx_HoldoverQualityParamsS;

/**

  Function Name:
   zl303xx_AprGetCurrentHoldoverQualityParameters

  Details:
    Retrieve the running holdover quality parameters for the specified device

  Parameters:
   [in]   hwParams                   Pointer to the device instance
   [out]  holdoverQualityParamsP     Pointer to the holdover info struct

  Return Value: zlStatusE

 *******************************************************************************/
zlStatusE zl303xx_AprGetCurrentHoldoverQualityParameters(void *hwParams, zl303xx_HoldoverQualityParamsS *holdoverQualityParamsP);


/** zl303xx_AprGetLastCGUStatusFlags

   Query the last stored CGU device status flags for the specified device.

  Parameters:
   [in]    hwParams                  Pointer to the device structure
   [out]   cguLastStatusFlags        Pointer to the CGU status flags structure

  Return Value: ZL303XX_OK                        if successful
****************************************************************************/
zlStatusE zl303xx_AprGetLastCGUStatusFlags(void *hwParams, zl303xx_AprCGUStatusFlagsS *cguLastStatusFlags);

/** zl303xx_AprSetCurrentFreqCatXSetting

   Change to another stored freqCatX value for the specified device.

  Parameters:
   [in]    hwParams        Pointer to the device structure
   [in]    freqCatX        The freq category enum to be used

  Return Value: ZL303XX_OK                        if successful
****************************************************************************/
zlStatusE zl303xx_AprSetCurrentFreqCatXSetting(void *hwParams, zl303xx_FreqCatxE freqCatX);

/** zl303xx_AprGetCurrentFreqCatXSetting

   Query the current CatX values for the specified device.

  Parameters:
   [in]    hwParams        Pointer to the device structure
   [out]   freqCatX        Pointer to the freq being used (current CatX)
   [out]   freqCatXEnum    Pointer to the freq category being used (current CatX Enum)

  Return Value: ZL303XX_OK                        if successful
****************************************************************************/
zlStatusE zl303xx_AprGetCurrentFreqCatXValue(void *hwParams, Uint32T *freqCatX, zl303xx_FreqCatxE *freqCatXEnum);

/** zl303xx_AprModifyCurrentFreqHoldoverSetting

   Dynamically modify the stored freqCatX and modeAccuracy values for the specified device.

  Parameters:
   [in]    hwParams        Pointer to the device structure
   [in]    value           User value being used to override default or startup values
   [in]    freqCatX        The freq category enum to be used

  Return Value: ZL303XX_OK                        if successful
****************************************************************************/
zlStatusE zl303xx_AprModifyCurrentFreqHoldoverSetting(void *hwParams, Uint32T value, zl303xx_FreqCatxE freqCatX);

/**  zl303xx_AprSetHybridTransient

   This function is used in BC, hybrid mode when a transient conditon starts.

   This routine indicates, to APR, that a transient condition has started or
   ended.

  Parameters:
   [in]  hwParams                   pointer to the device data structure
   [in]  BCHybridTransientActive    type of transient

  Return Value: zlStatusE

*****************************************************************************/

zlStatusE zl303xx_AprSetHybridTransient
            (
            void *hwParams,
            zl303xx_BCHybridTransientType BCHybridTransientActive
            );

/* zl303xx_AprStoreHoldoverAccum

   This function is used to store the accumulated holdover counters

  Parameters:
   [in]  hwParams                   Pointer to the device data structure
   [in]  holdoverQuality            Value to store
   [in]  totalHoldoverAccumulation  Value to store
   [in]  holdoverIncrementCount     Value to store
   [in]  holdoverAction             Value to store

  Return Value: zlStatusE

*****************************************************************************/
zlStatusE zl303xx_AprStoreHoldoverAccum(void *hwParams, zl303xx_HoldoverQualityTypesE holdoverQuality, Uint32T totalHoldoverAccumulation,
                                      Uint32T holdoverIncrementCount, zl303xx_HoldoverActionE holdoverAction);

/** zl303xx_AprClearHoldoverAccum

   This function is used to clear the accumulated holdover counters

  Parameters:
   [in]  hwParams                   Pointer to the device data structure

  Return Value: zlStatusE

*****************************************************************************/
zlStatusE zl303xx_AprClearHoldoverAccum(void *hwParams);


/*  zl303xx_AprSetTIEClearEventData

    This routine sets the user's TIE clear data,

  Parameters:
   [in]  zl303xx_Params    pointer to the params
   [in]  serverId        master ID
   [in]  tieClearEvent   the TIE-clear event type
   [in]  value           The value to apply

  Return Value: zlStatusE
*****************************************************************************/
zlStatusE zl303xx_AprSetTIEClearEventData
            (
            void *zl303xx_Params,
            Uint16T serverId,
            zl303xx_TIEClearEventsE tieClearEvent,
            zl303xx_BooleanE newValue,
            Uint64S value
            );

/*  zl303xx_AprGetTIEClearEventData

    This routine gets the user's TIE clear data,

  Parameters:
   [in]  zl303xx_Params    pointer to the params
   [in]  serverId        master ID
   [in]  tieClearEvent   the TIE-clear event type

   [out] newValue        TRUE if the value has not been applied
   [out] value           The value to apply

  Return Value: zlStatusE
*****************************************************************************/
zlStatusE zl303xx_AprGetTIEClearEventData
            (
            void *zl303xx_Params,
            Uint16T serverId,
            zl303xx_TIEClearEventsE tieClearEvent,
            zl303xx_BooleanE *newValue,
            Uint64S *value
            );

/** zl303xx_PFSetTIEClearEnable

   This routine sets the overall TIE clear enable/disable to either TC_disabled
   of TC_enabled.

  Parameters:
   [in]  zl303xx_Params      pointer to the zl303xx device data
   [in]  t                 the TIE-clear value

  Return Value: zlStatusE

*****************************************************************************/
zlStatusE zl303xx_PFSetTIEClearEnable
            (
            void *zl303xx_Params,
            zl303xx_TIEClearE t
            );
/** zl303xx_PFGetTIEClearEnable

   This routine gets the overall TIE clear enable/disable configuration.

  Parameters:
   [in]  zl303xx_Params      pointer to the zl303xx device data
   [out] t                 the TIE-clear configuration

  Return Value: zlStatusE

*****************************************************************************/
zlStatusE zl303xx_PFGetTIEClearEnable
            (
            void *zl303xx_Params,
            zl303xx_TIEClearE *t
            );

/** zl303xx_PFClearTIEClearCurrentOffset

   This routine clears the current TIE-clear offset. If the current value is
   not 0, the output phase will move by the TIE-clear offset amount.

  Parameters:
   [in]  zl303xx_Params      pointer to the zl303xx device data

  Return Value: zlStatusE

*****************************************************************************/
zlStatusE zl303xx_PFClearTIEClearCurrentOffset
            (
            void *zl303xx_Params
            );

/** zl303xx_PFGetTIEClearCurrentOffset

   This routine sets the current TIE-clear offset.

  Parameters:
   [in]  zl303xx_Params      pointer to the zl303xx device data
   [in]  newAlignValue     the alignment value

  Return Value: zlStatusE

*****************************************************************************/
zlStatusE zl303xx_PFSetTIEClearCurrentOffset
            (
            void *zl303xx_Params,
            Uint64S newAlignValue
            );

/** zl303xx_PFGetTIEClearCurrentOffset

   This routine returns the current TIE-clear offset.

  Parameters:
   [in]  zl303xx_Params      pointer to the zl303xx device data
   [in]  newAlignValue     the alignment value

  Return Value: zlStatusE

*****************************************************************************/
zlStatusE zl303xx_PFGetTIEClearCurrentOffset
            (
            void *zl303xx_Params,
            Uint64S *tieClearCurrentOffset
            );

/** zl303xx_AprGetTimer1PeriodRuntime

   This routine gets the timer 1 period in ms during runtime.

  Parameters:
   [in]  zl303xx_Params      pointer to the zl303xx device data
   [out]  timer1Period      pointer to Timer 1 period

  Return Value: zlStatusE

*****************************************************************************/
zlStatusE zl303xx_AprGetTimer1PeriodRuntime
            (
            void *zl303xx_Params,
            Uint32T *timer1Period
            );

/** zl303xx_AprSetTimer1PeriodRuntime

   This routine sets the timer 1 period in ms during runtime.  This value
   must be a multiple of the timer 2 period.

   - Note, changing the timer 1 period during runtime will invalidate
     all existing APR servers.

  Parameters:
   [in]  zl303xx_Params      pointer to the zl303xx device data
   [in]  timer1Period      Timer 1 period to set

  Return Value: zlStatusE

*****************************************************************************/
zlStatusE zl303xx_AprSetTimer1PeriodRuntime
            (
            void *zl303xx_Params,
            Uint32T timer1Period
            );

/** zl303xx_AprSetDeviceNCOAssist

   Sets the current NCO-Assist device settings via the APR software.

  Parameters:
   [in]  zl303xx_Params      pointer to the zl303xx device data
   [in]  deviceNCOAssist   device NCO-Assist value

  Return Value: zlStatusE

*****************************************************************************/
zlStatusE zl303xx_AprSetDeviceNCOAssist
            (
            void *zl303xx_Params,
            zl303xx_BooleanE deviceNCOAssist
            );

/** zl303xx_AprGetDeviceNCOAssist

   Gets the current NCO-Assist device settings via the APR software.

  Parameters:
   [in]  zl303xx_Params      pointer to the zl303xx device data
   [out]  deviceNCOAssist   pointer to device NCO-Assist

  Return Value: zlStatusE

*****************************************************************************/
zlStatusE zl303xx_AprGetDeviceNCOAssist
            (
            void *zl303xx_Params,
            zl303xx_BooleanE *deviceNCOAssist
            );

/** zl303xx_AprGetPFAdjustmentInProgressFlags

   Get the status of the current PF adjustment in progress
   - PFAdjustmentInProgress: main flag for current adjustment in progress
   - stepTimeInProgress: we are performing a stepTime operation
   - setTimeInProgress: we are performing a setTime operation
   - adjTimeInProgress: we are performing an adjustTime operation

  Parameters:
   [in]  zl303xx_Params      pointer to the zl303xx device data
   [out]  PFAdjustmentInProgress Output: overall adjustment in progress?
   [out]  stepTimeInProgress     Output: StepTime in progress?
   [out]  setTimeInProgress      Output: SetTime in progress?
   [out]  adjTimeInProgress      Output: AdjustTime in progress?

  Return Value: zlStatusE

*****************************************************************************/
zlStatusE zl303xx_AprGetPFAdjustmentInProgressFlags
            (
            void *zl303xx_Params,
            zl303xx_BooleanE *PFAdjustmentInProgress,
            zl303xx_BooleanE *stepTimeInProgress,
            zl303xx_BooleanE *setTimeInProgress,
            zl303xx_BooleanE *adjTimeInProgress
            );

/* Example Message Router Section */
/* The Example code message router is a softare mechanism for the APR algorithm to
   interface with customer callback functions.  The Example code message router
   defines all necessary example code operations that will be required at
   run time.  */

/* Example code callout function type.*/
typedef Sint32T (*hwFuncPtrUserMsgRouter)(void*, void*, void*);

/* Main Message Router Message Type ENUM*/
typedef enum {
    ZL303XX_USER_MSG_SET_TIME_TSU                          = 0,
    ZL303XX_USER_MSG_STEP_TIME_TSU                         = 1,
    ZL303XX_USER_MSG_JUMP_TIME_TSU                         = 2,
    ZL303XX_USER_MSG_JUMP_TIME_NOTIFICATION                = 3,
    ZL303XX_USER_MSG_JUMP_STANDBY_CGU                      = 4,
    ZL303XX_USER_MSG_SEND_REDUNDANCY_DATA_TO_MONITOR       = 5,
    ZL303XX_USER_MSG_APR_PHASE_ADJ_MODIFIER                = 6,
    ZL303XX_USER_MSG_PHASE_DRIFT_ALARM                     = 7
} zl303xx_UserMsgTypesE;

/* Set time TSU operation */
typedef struct {
    Uint64S deltaTimeSec;
    Uint32T deltaTimeNanoSec;
     zl303xx_BooleanE negative;
} zl303xx_SetTimeTSUInDataS;

/* Step time TSU operation */
typedef struct {
    Sint32T deltaTimeNanoSec;
} zl303xx_StepTimeTSUInDataS;

/* Jump time TSU operation */
typedef struct {
    Uint64S deltaTimeSec;
    Uint32T deltaTimeNanoSec;
     zl303xx_BooleanE negative;
} zl303xx_JumpTimeTSUInDataS;

/* Jump time notification of start or end */
typedef struct {
     Uint64S seconds;
     Uint32T nanoSeconds;
     zl303xx_BooleanE bBackwardAdjust;
     zl303xx_JumpEvent_t jumpEvent;
} zl303xx_JumpTimeNotificationInDataS;

/* Jump the current standby CGU */
typedef struct {
    Uint64S seconds;
    Uint32T nanoSeconds;
    zl303xx_BooleanE bBackwardAdjust;
} zl303xx_JumpStandbyCGUInDataS;

/* Send redundency data to monitor */
typedef struct {
    Uint16T serverId;
} zl303xx_SendRedundencyDataToMonitorInDataS;
typedef struct {
    zl303xx_RedundancyMsgS msg;
} zl303xx_SendRedundencyDataToMonitorOutDataS;

/* 1 Hz adjustment modified, choose to modify phase adjustments */
typedef struct {
    zl303xx1HzAdjModifierDataS adjModifierData;
} zl303xx_AprPhaseAdjustmentModifierOutDataS;

/* Send no active server info */
typedef struct {
    Sint32T dfValue;
    Sint32T algHOFreq;
    Uint16T currentServerId;
    zl303xx_BooleanE algHOValid;
    zl303xx_BooleanE serverHOValid;
} zl303xx_SendNoActiveServerInfoInDataS;

/* PhaseDriftAlarm */
typedef struct {
    Uint16T serverId;
    Sint32T deltaFreqPpt;
    Sint32T phaseValueNs;
} zl303xx_PhaseDriftAlarmInDataS;


/* Main input structure declaration */
typedef struct {
    zl303xx_UserMsgTypesE userMsgType;
    union {
        zl303xx_SetTimeTSUInDataS setTimeTSU;
        zl303xx_StepTimeTSUInDataS stepTimeTSU;
        zl303xx_JumpTimeTSUInDataS jumpTimeTSU;
        zl303xx_JumpTimeNotificationInDataS jumpTimeNotification;
        zl303xx_JumpStandbyCGUInDataS jumpStandbyCGU;
        zl303xx_SendRedundencyDataToMonitorInDataS sendRedundencyDataToMonitor;
        zl303xx_SendNoActiveServerInfoInDataS sendNoActiveServerInfo;
        zl303xx_PhaseDriftAlarmInDataS sendPhaseDriftAlarm;
    } d;
} zl303xx_UserMsgInDataS;

/* Main output structure declaration */
typedef struct {
    zl303xx_UserMsgTypesE userMsgType;
    union {
        zl303xx_SendRedundencyDataToMonitorOutDataS sendRedundencyDataToMonitor;
        zl303xx_AprPhaseAdjustmentModifierOutDataS aprPhaseAdjustmentModifier;
    } d;
} zl303xx_UserMsgOutDataS;

/* Configurable options for zl303xx_AprPrintStruct... functions */
typedef struct {
   /* Optional. A string to print before all outputs (or NULL) */
   const char *prefixStr;

   /* Optional. A zl303xx_ParamsS* pointer for logging interface (can be NULL) */
   void *logHwParams;
   /* Optional. The serverId for logging interface (can be any number) */
   Uint16T logServerId;

   /* Configure how often logging should be flushed by calling
      OS_TASK_DELAY for `logFlushSleepMsec` milliseconds.

      Use large number to disable log flushing.

      Default: 10 (every 10 logs)
      Units: count
    */
   Uint32T logFlushCountThreshold;

   /* Configure how many milliseconds to sleep for log flushing
      when triggered by `logFlushCountThreshold`.

      Default: 20 (sleep 20 msec)
      Units: milliseconds
   */
   Uint32T logFlushSleepMsec;

} zl303xx_AprPrintStructOptionsT;

/**

  Function Name:
   zl303xx_AprPrintStructOptionsStructInit

  Details:
    Initalizes given zl303xx_AprPrintStructOptionsT structure with defaults.

  Notes:
    The output is always printed via APR logging interface (i.e. LogToMsgQ)
    and the call may sleep several times to allow logging to flush to avoid full
    logging queue. See options in zl303xx_AprPrintStructOptionsT.

  Parameters:
   [out]   pOptions   Configurable logging options to initialize.

  Return Value:
   zlStatusE

 *******************************************************************************/
zlStatusE zl303xx_AprPrintStructOptionsStructInit(zl303xx_AprPrintStructOptionsT *pOptions);

/**

  Function Name:
   zl303xx_AprPrintStructAprInit

  Details:
    Debug utility to print zl303xx_AprInitS data structure members for debugging
    configuration.

  Notes:
    The output is always printed via APR logging interface (i.e. LogToMsgQ)
    and the call may sleep several times to allow logging to flush to avoid full
    logging queue. See options in zl303xx_AprPrintStructOptionsT.

  Parameters:
   [in]   pAprInit   Pointer to the zl303xx_AprInitS structure to print.
   [in]   pOptions   Configurable logging options (see zl303xx_AprPrintStructOptionsT)

  Return Value:
   zlStatusE

 *******************************************************************************/
zlStatusE zl303xx_AprPrintStructAprInit(zl303xx_AprInitS *pAprInit,
                                      zl303xx_AprPrintStructOptionsT *pOptions);


/**

  Function Name:
   zl303xx_AprPrintStructAprAddDevice

  Details:
    Debug utility to print zl303xx_AprAddDeviceS data structure members for debugging
    configuration.

  Notes:
    The output is always printed via APR logging interface (i.e. LogToMsgQ)
    and the call may sleep several times to allow logging to flush to avoid full
    logging queue. See options in zl303xx_AprPrintStructOptionsT.

  Parameters:
   [in]   pAprAddDevice    Pointer to the zl303xx_AprAddDeviceS structure to print.
   [in]   pOptions         Optional. Configurable logging options (see zl303xx_AprPrintStructOptionsT)

  Return Value:
   zlStatusE

 *******************************************************************************/
zlStatusE zl303xx_AprPrintStructAprAddDevice(zl303xx_AprAddDeviceS *pAprAddDevice,
                                           zl303xx_AprPrintStructOptionsT *pOptions);


/**

  Function Name:
   zl303xx_AprPrintStructAprAddServer

  Details:
    Debug utility to print zl303xx_AprAddServerS data structure members for debugging
    configuration.

  Notes:
    The output is always printed via APR logging interface (i.e. LogToMsgQ)
    and the call may sleep several times to allow logging to flush to avoid full
    logging queue. See options in zl303xx_AprPrintStructOptionsT.

  Parameters:
   [in]   pAprAddServer    Pointer to the zl303xx_AprAddServerS structure to print.
   [in]   pOptions         Optional. Configurable logging options (see zl303xx_AprPrintStructOptionsT)

  Return Value:
   zlStatusE

 *******************************************************************************/
zlStatusE zl303xx_AprPrintStructAprAddServer(zl303xx_AprAddServerS *pAprAddServer,
                                           zl303xx_AprPrintStructOptionsT *pOptions);

/** Unused at this time */
/**
  Function Name:
   zl303xx_AprInitConfirm

  Details:
   Modify a boolean flag to monitor the F/W init bit after a device reset - newer ZL devices only

  Parameters:
   [in]  zl303xx_Params      Pointer to the zl303xx device data
   [in]  enable            Boolean flag

  Return Value: zlStatusE
*****************************************************************************/
zlStatusE zl303xx_AprInitConfirm(void *zl303xx_Params, zl303xx_BooleanE enable);

#if defined __cplusplus
}
#endif


#endif
