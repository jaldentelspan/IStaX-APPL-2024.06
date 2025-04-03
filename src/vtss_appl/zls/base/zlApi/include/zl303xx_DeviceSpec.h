

/*******************************************************************************
*
*  $Id: 8410f584bd768792a15e60f6157c94bf8acedd47
*
*  Copyright 2006-2022 Microchip Technology Inc.
*  All rights reserved.
*
*  Module Description:
*     Generic Timing Device structure definition (Params are populated for ZL30360)
*
*******************************************************************************/

#ifndef _ZL303XX_DEVICE_SPEC_H_
#define _ZL303XX_DEVICE_SPEC_H_

#if defined __cplusplus
extern "C" {
#endif


/*****************   INCLUDE FILES   ******************************************/
#include "zl303xx_Global.h"
#include "zl303xx_Error.h"
#include "zl303xx_Int64.h"

#include "zl303xx_DeviceIf.h"

#if defined _ZL303XX_FMC_BOARD || defined _ZL303XX_NTM_BOARD ||  defined _ZL303XX_ZLE30360_BOARD || defined _ZL303XX_ZLE1588_BOARD 
#include "zl303xx_DeviceIf.h"
#endif

#if defined ZLS30361_INCLUDED
#include "zl303xx_AddressMap36x.h"
#endif
#if defined ZLS30701_INCLUDED
#include "zl303xx_AddressMap70x.h"
#endif
#if defined ZLS30721_INCLUDED
#include "zl303xx_AddressMap72x.h"
#endif
#if defined ZLS30731_INCLUDED
#include "zl303xx_AddressMap73x.h"
#endif
#if defined ZLS30751_INCLUDED
#include "zl303xx_AddressMap75x.h"
#endif
#if defined ZLS30771_INCLUDED
#include "zl303xx_AddressMap77x.h"
#endif

#if defined ZLS30341_INCLUDED
#include "zl303xx_TodMgrTypes.h"
#include "zl303xx_TodMgrInternal.h"
#include "zl303xx_Dpll34xApiLowDataTypes.h"
#endif

/*****************   DEFINES   ************************************************/

/***** General constants *****/
#define ZL303XX_INVALID (-1)
#define ZL303XX_INVALID_STREAM ZL303XX_INVALID

#if defined ZLS30361_INCLUDED
#define ZL303XX_MAX_NUM_SYNTHS (4)
#endif

typedef enum
{
   ZL303XX_MODE_INVALID  = -1,
   ZL303XX_MODE_UNKNOWN = 0,
   ZL303XX_MODE_REF_TOP,
   ZL303XX_MODE_REF_EXTERNAL,
   ZL303XX_MODE_REF_EXTERNAL_FORCED,
   ZL303XX_MODE_REF_BC,
   ZL303XX_MODE_FREERUN,
   ZL303XX_MODE_HOLDOVER
} zl303xx_DeviceModeE;

/*****************   DATA TYPES   *********************************************/


/* define a callback function pointer for SPI chip select enabling. The
   'par' is a user configurable parameter that will be passed by the calling
   function along with the required status of the chip select*/
typedef zlStatusE (*zl303xx_SpiChipSelectT)(Uint32T par, zl303xx_BooleanE enable);

typedef struct
{
#ifdef ZL_UNG_MODIFIED
   zl303xx_SpiChipSelectT csFuncPtr;
   Uint32T csFuncPar;
#else
   Uint32T Unused;
#endif
} zl303xx_ChipSelectCtrlS;


#if defined ZLS30390_INCLUDED

    typedef enum    /* If you add more RAW ipv6 protocols then add them to zl303xx_IsIPv6ProtocolType() as well! */
    {
       /* Datagram socket protocols. */
       ZL303XX_ETH_IPV4_UDP_PTPV1 = 0, /* PTP protocols */
       ZL303XX_ETH_IPV4_UDP_PTPV2,
       ZL303XX_ETH_VLAN_IPV4_UDP_PTPV2_SKT,
       ZL303XX_ETH_IPV6_UDP_PTPV2_SKT,
       ZL303XX_ETH_VLAN_IPV6_UDP_PTPV2_SKT,

       ZL303XX_ETH_IPV4_UDP_PW = 10,          /* Pseudo Wire protocols */

       /* Raw socket protocols. */
       ZL303XX_PROTOCOL_RAW_PTP_START = 20,  /* PTP protocols */
       ZL303XX_ETH_DSA_IPV4_UDP_PTPV2 = ZL303XX_PROTOCOL_RAW_PTP_START,
       ZL303XX_ETH_VLAN_IPV4_UDP_PTPV2,
       ZL303XX_ETH_VLAN_VLAN_IPV4_UDP_PTPV2,
       ZL303XX_ETH_MPLS_IPV4_UDP_PTPV2,
       ZL303XX_ETH_PTPV2,
       ZL303XX_ETH_IPV6_UDP_PTPV2,             /* 25 */
       ZL303XX_ETH_CUST_ETH_IPV4_UDP_PTPV2,
       ZL303XX_ETH_CUST_ETH_PTPV2,
       ZL303XX_ETH_CUST_ETH_IPV6_UDP_PTPV2,
       ZL303XX_ETH_VLAN_PTPV2,
       ZL303XX_PROTOCOL_RAW_PTP_END = ZL303XX_ETH_VLAN_PTPV2,

       ZL303XX_PROTOCOL_RAW_PW_START = 40,  /* PW protocols */
       ZL303XX_ETH_MPLS_PW,
       ZL303XX_ETH_MPLS_MPLS_PW,               /* 45 */
       /* Customized protocols, using proprietary 8 byte header for routing */
       ZL303XX_PROTOCOL_RAW_PW_END = ZL303XX_ETH_MPLS_MPLS_PW,

       /* Illegal - this should remain the last entry in the
          enumeration for range checking purposes in the implementation. */
       ZL303XX_PROTOCOL_NUM_TYPES

    } zl303xx_ProtocolTypesE;

#endif


/* This structure is used to Init the PLL at startup */
typedef struct
{
   zl303xx_BooleanE TopClientMode; /* DEPRECATED (use the clock deviceMode (zl303xx_DeviceModeE)) */
   Uint32T pllId;
} zl303xx_PllInitS;

/* Read/Write statistics accumulation struct */
typedef struct
{
    Uint64T readBytes;      /* 8Hz *4 bytes */
    Uint64T readAccesses;   /* 8Hz */
    Uint64T writeBytes;
    Uint64T writeAccesses;
} zl303xx_RdWrStatsS;


#if defined ZLS30361_INCLUDED || defined ZLS30721_INCLUDED || defined ZLS30701_INCLUDED || defined ZLS30731_INCLUDED || defined ZLS30751_INCLUDED || defined ZLS30771_INCLUDED

    /* Useful macros that make the lock/unlock operations stand out from their surrounding code */
    #ifndef ZL303XX_LOCK_DEV_PARAMS
    #define ZL303XX_LOCK_DEV_PARAMS(x)   zl303xx_LockDevParams(x)
    #define ZL303XX_UNLOCK_DEV_PARAMS(x) (void)zl303xx_UnlockDevParams(x)
    #endif

    /* A device has been specified; include all the needed types */

    /* Device initialization state */
    typedef enum
    {
       ZL303XX_INIT_STATE_NONE = 0,    /* Not yet initialized */
       ZL303XX_INIT_STATE_CLOSE_DOWN = 1, /* Device is being closed down */
       ZL303XX_INIT_STATE_DEV_RESET,   /* Device has been successfully reset */
       ZL303XX_INIT_STATE_LAN_INIT_IN_PROGRESS,
       ZL303XX_INIT_STATE_LAN_INIT_DONE,
       ZL303XX_INIT_STATE_PLL_INIT_IN_PROGRESS,
       ZL303XX_INIT_STATE_PLL_INIT_DONE,
       ZL303XX_INIT_STATE_TSENG_INIT_IN_PROGRESS,
       ZL303XX_INIT_STATE_TSENG_INIT_DONE,
       ZL303XX_INIT_STATE_DONE         /* All initialization completed successfully */
    } zl303xx_InitStateE;


    #if defined ZLS30361_INCLUDED || defined ZLS30701_INCLUDED || defined ZLS30731_INCLUDED || defined ZLS30751_INCLUDED || defined ZLS30771_INCLUDED
        typedef Sint32T (*hwFuncPtrStickyLock)(void*, Sint32T, zl303xx_BooleanE);

		                /* The max number of dplls in the system */
        #define ZL303XX_DPLL_ID_MIN      ZL303XX_DPLL_ID_1
        #define ZL303XX_DPLL_ID_MAX      ZL303XX_DPLL_ID_4
        #define ZL303XX_DPLL_NUM_DPLLS   (ZL303XX_DPLL_ID_MAX + 1)
    #endif

    #if defined ZLS30341_INCLUDED

        typedef struct
        {
           zl303xx_DpllHitlessE   hitlessSw;
           zl303xx_DpllBwE        bandwidth;
           zl303xx_DpllPslE       phaseSlope;
           zl303xx_BooleanE       revertEn;
           zl303xx_DpllHldUpdateE hldUpdateTime;
           zl303xx_DpllHldFilterE hldFilterBw;
           zl303xx_DpllModeE      mode;
           zl303xx_DpllModeE      pllPriorMode;
           zl303xx_RefIdE         selectedRef;
           Uint32T              waitToRestore;  /* waitToRestore is in minutes */
           zl303xx_DpllPullInE    pullInRange;
           zl303xx_DpllIdE        Id;
           zl303xx_BooleanE       enable;
           zl303xx_BooleanE       dcoOffsetEnable;
           zl303xx_BooleanE       dcoFilterEnable;
        } zl303xx_DpllConfigS;

        /* The max number of dplls in the system */
        #define ZL303XX_DPLL_ID_MIN      ZL303XX_DPLL_ID_1
        #define ZL303XX_DPLL_ID_MAX      ZL303XX_DPLL_ID_4
        #define ZL303XX_DPLL_NUM_DPLLS   (ZL303XX_DPLL_ID_MAX + 1)

    #endif

    #if defined ZLS30341_INCLUDED

        /* Number of device interrupt outputs */
        #define ZL303XX_NUM_DEVICE_IRQS    1

        /* enum to determine the protocol/format of the inserted timestamp */
        typedef enum
        {
           ZL303XX_TS_FORMAT_PTP = 0,
           ZL303XX_TS_FORMAT_NTP = 1
        } zl303xx_TsEngTsProtocolE;

        typedef enum
        {
           ZL303XX_UPDATE_NONE = 0,
           ZL303XX_UPDATE_1HZ = 1,
           ZL303XX_UPDATE_SYS_TIME = 2,
           ZL303XX_UPDATE_SYS_INTRVL = 3
        } zl303xx_UpdateTypeE;

        #define ZL303XX_CHECK_UPDATE_TYPE(X) \
              ((((zl303xx_UpdateTypeE)(X) > ZL303XX_UPDATE_SYS_INTRVL)) ? \
                 (ZL303XX_ERROR_NOTIFY("Invalid Update Mode: " #X),ZL303XX_PARAMETER_INVALID) :  \
                 ZL303XX_OK)

        typedef struct
        {
           /* System timestamp: used for the Rx/Tx timestamps */
           Uint64S localTs;

           /* Units are protocol dependent:
              PTP/NTP - units of seconds32:nanoseconds32.  */
           /* This is the raw value read from the register.               */
           Uint64S rawInsertTs;

           /* The insert timestamp converted to units of SEC:NANO   */
           /* A more accurate name would be 'nsTs' or 'todTs'       */
           Uint64S insertTs;

           /* DCO */
           Uint64S dcoTs;       /* DCO Ticks32:Phase32 => the raw sampled value    */
           Uint64S dcoExtendTs; /* DCO Ticks64 (extended to 64-bit via software)   */

           /* DCO offset at the time the sample was taken. */
           Sint32T dcoFreq;

           /* Predicted tick ratios for this sample offset. Used to convert between
            * system clock counts and adjusted clock frequencies. */
           Uint32T dcoDelta;    /* Predicted #DCO ticks in the next SYSTEM interval. */
           Uint32T nsDelta;     /* Predicted #Nanoseconds in the next SYSTEM interval.*/
           Uint32T insertDelta; /* Predicted #INSERT timestamp ticks in the next SYSTEM
                                 *    interval. */

           /* CPU */
           Uint32T osTimeTicks; /* Value of the OS tick counter at the sample point */
           Uint64S cpuHwTime;   /* CPU HW timestamp at the sample point (if CPU HW
                                   time-stamping is enabled). */

        } zl303xx_TsSampleS;

        /* Define a min/max/default system interrupt rate. For 340 and 341 products only, so the values are ok*/
        #define ZL303XX_MIN_LOG2_SYS_INTERRUPT_PERIOD     20  /* 2^20 / 80e6 =~ 0.0131sec (76.29Hz) */
        #define ZL303XX_38HZ_LOG2_SYS_INTERRUPT_PERIOD    21  /* 2^21 / 80e6 =~ 0.0262sec (38.14Hz) */
        #define ZL303XX_9HZ_LOG2_SYS_INTERRUPT_PERIOD     23  /* 2^23 / 80e6 =~ 0.1048sec ( 9.53Hz) */
        #define ZL303XX_MAX_LOG2_SYS_INTERRUPT_PERIOD     26  /* 2^26 / 80e6 =~ 0.8388sec ( 1.19Hz) */
        #define ZL303XX_DEFAULT_LOG2_SYS_INTERRUPT_PERIOD ZL303XX_38HZ_LOG2_SYS_INTERRUPT_PERIOD

        /* DCO and SYSTEM Clock frequencies. */
        #define ZL303XX_PLL_INTERNAL_FREQ_KHZ    65536
        #define ZL303XX_PLL_SYSTEM_CLK_KHZ       80000

    #endif
    /*****************   DATA STRUCTURES   ****************************************/

    #if defined ZLS30721_INCLUDED

        typedef enum {
            ZL303XX_STS_IDLE,
            ZL303XX_STS_RUNNING_SUB_STATE_MACHINE,
            ZL303XX_STS_IN_GUARD_TIMER,
            ZL303XX_STS_LAST
        } zl303xx72xStepTimeStatesE;

        typedef enum {
            ZL303XX_LSDIVS_IDLE,
            ZL303XX_LSDIVS_STEP_PENDING,
            ZL303XX_LSDIVS_WAITING_FOR_1ST_HALF_CYCLE,
            ZL303XX_LSDIVS_WAITING_FOR_HALF_CYCLES,
            ZL303XX_LSDIVS_LAST
        } zl303xx72xLSDIVStatesE;

        typedef enum {
            ZL303XX_PSM_AUTOMATIC,
            ZL303XX_PSM_NCO,
            ZL303XX_PSM_LAST
        } zl303xx72xPriorSubModeE;

        #define ZL303XX_CHECK_DPLL_SUB_MODE(subMode)                  \
                   (((zl303xx72xPriorSubModeE)(subMode) >= ZL303XX_PSM_LAST)  \
                       ? ZL303XX_PARAMETER_INVALID                    \
                       : ZL303XX_OK)

        typedef struct {

            /* Accumulate the total phase steps made while in NCO mode */
            Sint32T totalStep;

            /* The states for a small state machine  */
            zl303xx72xLSDIVStatesE LSDIVState;

            /* The phase step currently active */
            Sint32T deltaTime;

            /* The number of seconds to apply the step */
            Sint32T numSec;

            /* Internal variable for timing */
            Sint32T halfCycleCount;

            /* Internal variable for timing */
            Sint32T pollCount;

            /* The value of register OCxDIV before modification */
            Sint64T T64_OCxDIV;

            /* The amount that will add to OCxDIV */
            Sint64T T64_OCxDIVOffset;

            /* T64_OCxDIV + T64_OCxDIVOffset */
            Sint64T T64_newOCxDIV;

        } zl303xx72xLSDIVDataS;

        typedef struct {

            /* Accumulated TIE-write values */
            Sint32T runningTotal;

        } zl303xx72xTIEWriteDataS;

        typedef struct {

            /* The user must specify which LSDIV (channel) drives the timestamper. */
            ZLS3072X_LSDIV_E LSDIVThatDrivesTimeStamper;

            /* A periodic timer is used to perform stepTime. This timer controls all
               3 outputs. */
            zl303xx72xStepTimeStatesE STState;

            /* When the step is done, a callback is called to align the timestamper */
            hwFuncPtrTODDone stepDoneFuncPtr;

            /* After the frequency has been moved, we may need some time to let things
               settle before moving the timestamper. This is part of that timer. */
            Sint32T guardTmrCnt;

            /* Data structure to hold stepTime() data. */
            zl303xx72xLSDIVDataS sD[ZLS3072X_OUTPUT_LAST];

        } zl303xx72xStepTimeDataS;

        typedef struct
        {
            /* JADE's 42-bit nominal frequency tuning word after initialisation
               but before modifications to the DCO. */
            Uint64T AFBDIV;

            /* JADE's 40-bit nominal frequency tuning word after initialisation
               but before modifications to the DCO. */
            Sint64T FREQZ0;

            /* While in NCO mode, this var is TRUE */
            zl303xx_BooleanE inNCOMode;

            /* When switching to NCO mode, we save the clocks that are valid in this
               variable. Later, when we return to electrical mode, we recal these
               values and put them back into the register. */
            Uint8T VALCR1;

            /* While in NCO mode, the device is in holdover. This var is the holdover
               value. APR needs this number when making NCO adjustments */
            Sint64T holdoverFFOppt;

            /* There are 3 outputs per 72x device. This mask enables large stepTime()s
               on the corresponding output. (When doing a large stepTime(), the
               low-speed divider is used to move the phase - hence the name)  */
            Uint32T LSDIVPhaseStepMask;

            /* stepTime() data */
            zl303xx72xStepTimeDataS std;

            /* stepTime() data */
            zl303xx72xTIEWriteDataS twd;

            /* Save the sub mode we are in when switching modes */
            zl303xx72xPriorSubModeE priorSubMode;

            /* After zl303xx_Dpll72xParamsInit() is called for this data structure,
               drvInitialized=TRUE. */
            zl303xx_BooleanE drvInitialized;

        } zl303xxd72xS;

    #endif


    #if defined ZLS30701_INCLUDED


        typedef struct {
            Uint32T ref_freq_base;
            Uint32T ref_freq_mult;
            Uint32T ref_ratio_m;
            Uint32T ref_ratio_n;
            Uint32T ref_config;
            Uint32T ref_scm;
            Uint32T ref_cfm;
            Uint32T ref_gst;
            Uint32T ref_pfm_ctrl;
            Uint32T ref_pfm_disqualify;
            Uint32T ref_pfm_qualify;
            Uint32T ref_pfm_period;
            Uint32T ref_pfm_filter_limit;
            Uint32T ref_phase_mem;
            Uint32T ref_sync;
            Uint32T ref_sync_misc;
            Uint32T ref_sync_offset_comp;
        } zl303xx70xRefMailBoxS;

        typedef struct {
            Uint32T dpll_bw_fixed;
            Uint32T dpll_bw_var;
            Uint32T dpll_config;
            Uint32T dpll_psl;
            Uint32T dpll_psl_max_phase;
            Uint32T dpll_psl_scaling;
            Uint32T dpll_psl_decay;
            Uint32T dpll_range;
            Uint32T dpll_ref_sw_mask;
            Uint32T dpll_ref_ho_mask;
            Uint32T dpll_ho_filter;
            Uint32T dpll_ho_delay;
            Uint32T dpll_priority_1_0;
            Uint32T dpll_priority_3_2;
            Uint32T dpll_priority_5_4;
            Uint32T dpll_priority_7_6;
            Uint32T dpll_priority_9_8;
            Uint32T dpll_lock_phase;
            Uint32T dpll_lock_period;
            Uint32T dpll_fast_lock_ctrl;
            Uint32T dpll_fast_lock_phase_err;
            Uint32T dpll_fast_lock_freq_err;
            Uint32T dpll_damping;
            Uint32T dpll_dual_config;
            Uint32T dpll_pbo;
            Uint32T dpll_pbo_jitter_thresh;
            Uint32T dpll_pbo_min_slope;
            Uint32T dpll_pbo_end_interval;
            Uint32T dpll_pbo_time_out;
            Uint32T dpll_lock_delay;
            Uint32T dpll_fp_first_realign;
            Uint32T dpll_fp_realign_intvl;
            Uint32T dpll_fp_lock_thresh;
        } zl303xx70xDpllMailboxS;

        typedef struct {
            Uint32T vco_freq_base;
            Uint32T vco_freq_mult;
            Uint32T vco_freq_m;
            Uint32T vco_freq_n;
            Uint32T config;
            Uint64T out_a_div;
            Uint32T out_a_driver;
            Uint32T out_a_ctrl;
            Uint32T out_a_width;
            Uint64T out_a_shift;
            Uint64T out_b_div;
            Uint32T out_b_driver;
            Uint32T out_b_ctrl;
            Uint32T out_b_width;
            Uint64T out_b_shift;
        } zl303xx70xSynth0MailboxS;

        typedef struct {
            Uint32T vco_freq_base;
            Uint32T vco_freq_mult;
            Uint32T vco_freq_m;
            Uint32T vco_freq_n;
            Uint32T config;
            Uint64T out_a_div;
            Uint32T out_a_driver;
            Uint32T out_a_ctrl;
            Uint32T out_a_width;
            Uint64T out_a_shift;
            Uint64T out_b_div;
            Uint32T out_b_driver;
            Uint32T out_b_ctrl;
            Uint32T out_b_width;
            Uint64T out_b_shift;
            Uint64T out_c_div;
            Uint32T out_c_driver;
            Uint32T out_c_ctrl;
            Uint32T out_c_width;
            Uint64T out_c_shift;
            Uint64T out_d_div;
            Uint32T out_d_ctrl;
            Uint32T out_d_width;
            Uint64T out_d_shift;
        } zl303xx70xSynth1MailboxS;

        typedef struct {
            Uint32T vco_freq_base;
            Uint32T vco_freq_mult;
            Uint32T vco_freq_m;
            Uint32T vco_freq_n;
            Uint32T config;
            Uint32T config2;
            Uint64T out_a_div;
            Uint32T out_a_driver;
            Uint32T out_a_ctrl;
            Uint32T out_a_width;
            Uint64T out_a_shift;
            Uint64T out_b_div;
            Uint32T out_b_driver;
            Uint32T out_b_ctrl;
            Uint32T out_b_width;
            Uint64T out_b_shift;
            Uint64T out_c_div;
            Uint32T out_c_driver;
            Uint32T out_c_ctrl;
            Uint32T out_c_width;
            Uint64T out_c_shift;
            Uint64T out_d_div;
            Uint32T out_d_driver;
            Uint32T out_d_ctrl;
            Uint32T out_d_width;
            Uint64T out_d_shift;
        } zl303xx70xSynth2MailboxS;

        typedef struct {
            Uint32T vco_freq_base;
            Uint32T vco_freq_mult;
            Uint32T vco_freq_m;
            Uint32T vco_freq_n;
            Uint32T config;
            Uint64T out_a_div;
            Uint32T out_a_driver;
            Uint32T out_a_ctrl;
            Uint32T out_a_width;
            Uint64T out_a_shift;
            Uint64T out_b_div;
            Uint32T out_b_driver;
            Uint32T out_b_ctrl;
            Uint32T out_b_width;
            Uint64T out_b_shift;
        } zl303xx70xSynth3MailboxS;


        typedef struct
        {
            zl303xx70xRefMailBoxS refMb[ZLS3070X_RMBN_last];
            zl303xx70xDpllMailboxS dpllMb[ZLS3070X_DMBN_last];
            zl303xx70xSynth0MailboxS synth_0;
            zl303xx70xSynth1MailboxS synth_1;
            zl303xx70xSynth2MailboxS synth_2;
            zl303xx70xSynth3MailboxS synth_3;
            OS_MUTEX_ID mailboxMutex;

            ZLS3070X_DpllHWModeE pllPriorMode;
            Uint32T userBW;
            Uint32T userBWCustom;
            Uint32T userPSL;
            zl303xx_BooleanE userFastLock;
            zl303xx_BooleanE userTieClear;
            Uint32T hitlessBW;
            Uint32T hitlessBWCustom;
            Uint32T hitlessPSL;
            zl303xx_BooleanE hitlessFastLock;
            Uint32T hitlessDelayPeriod;
            Uint32T hitlessRefSyncDelayPeriod;
            Uint16T gpioOscType;
            Uint8T  spursSuppress;
            struct
            {
               Uint32T postDivPhaseStepMask;
            } synth[ZLS3070X_NUM_SYNTHS];

            hwFuncPtrStickyLock stickyLockCallout;
        } zl303xxd70xS;

    #endif

    #if defined ZLS30731_INCLUDED

        typedef struct {
            /* Outputs */
            Uint32T Synth0IntFreq;
            Uint32T Synth0PllId;
            Uint32T Synth1IntFreq;
            Uint32T Synth1PllId;
            Uint32T Synth2IntFreq;
            Uint32T Synth2PllId;
            Uint32T Synth3IntFreq;
            Uint32T Synth3PllId;
            Uint32T Synth4IntFreq;
            Uint32T Synth4PllId;
            struct {
                Uint32T dpllId;
                Uint32T synthId;
                Uint32T sourceFreq;
                Uint32T outDiv;
                Uint32T outMode;
                Uint32T esyncDiv;
                Uint32T OUTPFreq;
                Uint32T OUTNFreq;
            } freq[ZLS3073X_NUM_OUTPUT_PAIRS];
            struct {
                Uint32T         todClockType;
                zl303xx_BooleanE  eSyncActiveB;
                zl303xx_BooleanE  miTodSyncActiveB;
                zl303xx_BooleanE  miTodBasicActiveB;
            } todMode[ZLS3073X_NUM_OUTPUT_PAIRS];
        } zl303xx_Dpll73xOutputConfS;
    
        typedef struct {
            Uint32T ref_freq_base;
            Uint32T ref_freq_mult;
            Uint32T ref_ratio_m;
            Uint32T ref_ratio_n;
            Uint32T ref_config;
            Uint32T ref_scm;
            Uint32T ref_cfm;
            Uint32T ref_gst_disqual;
            Uint32T ref_gst_qual;
            Uint32T ref_pfm_ctrl;
            Uint32T ref_pfm_disqualify;
            Uint32T ref_pfm_qualify;
            Uint32T ref_pfm_period;
            Uint32T ref_pfm_filter_limit;
            Uint32T ref_sync;
            Uint32T ref_sync_misc;
            Uint32T ref_sync_offset_comp;
            Uint32T ref_phase_offset_compensation;
            Uint32T ref_scm_fine;
        } zl303xx73xRefMailBoxS;

        typedef struct {
            Uint32T dpll_bw_fixed;
            Uint32T dpll_bw_var;
            Uint32T dpll_config;
            Uint32T dpll_psl;
            Uint32T dpll_psl_max_phase;
            Uint32T dpll_psl_scaling;
            Uint32T dpll_psl_decay;
            Uint32T dpll_range;
            Uint32T dpll_ref_sw_mask;
            Uint32T dpll_ref_ho_mask;
            Uint32T dpll_dper_sw_mask;
            Uint32T dpll_dper_ho_mask;
            Uint32T dpll_ref_prio_0;
            Uint32T dpll_ref_prio_1;
            Uint32T dpll_ref_prio_2;
            Uint32T dpll_ref_prio_3;
            Uint32T dpll_ref_prio_4;
            Uint32T dpll_dper_prio_1_0;
            Uint32T dpll_dper_prio_3_2;
            Uint32T dpll_ho_filter;
            Uint32T dpll_ho_delay;
            Uint32T dpll_split_xo_config;
            Uint32T dpll_fast_lock_ctrl;
            Uint32T dpll_fast_lock_phase_err;
            Uint32T dpll_fast_lock_freq_err;
            Uint32T dpll_fast_lock_ideal_time;
            Uint32T dpll_fast_lock_notify_time;
            Uint32T dpll_fast_lock_fcl;
            Uint32T dpll_fcl;
            Uint32T dpll_damping;
            Uint32T dpll_phase_bad;
            Uint32T dpll_phase_good;
            Uint32T dpll_duration_good;
            Uint32T dpll_lock_delay;
            Uint32T dpll_tie;
            Uint32T dpll_tie_wr_thresh;
            Uint32T dpll_fp_first_realign;
            Uint32T dpll_fp_align_intvl;
            Uint32T dpll_fp_lock_thresh;
            Uint32T dpll_step_time_thresh;
            Uint32T dpll_step_time_reso;
        } zl303xx73xDpllMailboxS;

        typedef struct {
            Uint32T synth_freq_base;
            Uint32T synth_freq_mult;
            Uint32T synth_ratio_m;
            Uint32T synth_ratio_n;
            Uint32T synth_phase_compensation;
            Uint32T synth_spread_spectrum_cfg;
            Uint32T synth_spread_spectrum_rate;
            Uint32T synth_spread_spectrum_spread;
        } zl303xx73xSynthMailboxS;

        typedef struct {
            Uint32T output_mode;
            Uint32T output_driver_config;
            Uint32T output_driver_div;
            Uint32T output_driver_width;
            Uint32T output_driver_esync_period;
            Uint32T output_driver_esync_width;
        } zl303xx73xOutputMailboxS;

        typedef enum {
            ZLS3073X_STS_IDLE,
            ZLS3073X_STS_WAITING_FOR_STEP_END,
            ZLS3073X_STS_IN_GUARD_TIMER
         } zl303xx73xStepStatesE;

        typedef struct {

            /* Configured outputs that are stepped */
            Uint32T phaseStepMask;

            /* The user must specify which output drives the timestamper. */
            ZLS3073X_OutputsE outputNumThatDrivesTimeStamper;

            /* When the step is done, a callback is called to align the timestamper */
            hwFuncPtrTODDone stepDoneFuncPtr;

            /* After the frequency has been moved, we may need some time to let things
               settle before moving the timestamper. This is part of that timer. */
            Sint32T guardStartTick;

            /* Accumulate the total phase steps made while in NCO mode */
            Sint32T totalStep;

            /* The states for a small state machine  */
            zl303xx73xStepStatesE STState;

            /* Sanity counter for errors */
            Uint32T sanityStartTick;

        } zl303xx73xStepTimeDataS;

        typedef struct
        {
            /* mailbox data */
            zl303xx73xRefMailBoxS refMb[ZLS3073X_RMBN_last];
            zl303xx73xDpllMailboxS dpllMb[ZLS3073X_DMBN_last];
            zl303xx73xSynthMailboxS synthMb[ZLS3073X_SMBN_last];
            zl303xx73xOutputMailboxS outputMb[ZLS3073X_OMBN_last];

            /* stepTime() data */
            zl303xx73xStepTimeDataS std;

            /* State data */
            ZLS3073X_DpllHWModeE pllPriorMode;

            /* Misc driver data */
            hwFuncPtrStickyLock stickyLockCallout;

            /* modified registers during hitless reference switching */
            zl303xx_BooleanE restoreRegValues;
            Sint32T restoreStartTick;
            Uint32T prev_ZLS3073X_DPLL_CTRL_X_REG;

            /* After zl303xx_Dpll73xParamsInit() is called for this data structure,
               drvInitialized=TRUE. */
            zl303xx_BooleanE drvInitialized;

            /* The following are data structures used to store device clock configurations.
               It is generally assumed that the DPLL->Synthesizer->Output structure will remain
               static during runtime. */
            zl303xx_Dpll73xOutputConfS outputConfigData;
            zl303xx_BooleanE outputConfigValid;

            /* A PLL Params pointer (a zl303xx_ParamsS*) to a NCO-Assist DPLL */
            void *pParamsNCOAssist;

            /* Protects Local State Data */
            OS_MUTEX_ID localConfigMutex;

        } zl303xxd73xS;

    #endif  /* 731_INCLUDED */

    #if defined ZLS30751_INCLUDED

        typedef struct {
            Uint32T ref_freq_base;
            Uint32T ref_freq_mult;
            Uint32T ref_ratio_m;
            Uint32T ref_ratio_n;
            Uint32T ref_config;
            Uint32T ref_scm;
            Uint32T ref_cfm;
            Uint32T ref_gst;
            Uint32T ref_pfm_ctrl;
            Uint32T ref_pfm_disqualify;
            Uint32T ref_pfm_qualify;
            Uint32T ref_pfm_period;
            Uint32T ref_pfm_filter_limit;
            Uint32T ref_phase_mem;
            Uint32T ref_sync;
            Uint32T ref_sync_misc;
            Uint32T ref_sync_offset_comp;
        } zl303xx75xRefMailBoxS;

        typedef struct {
            Uint32T dpll_bw_fixed;
            Uint32T dpll_bw_var;
            Uint32T dpll_config;
            Uint32T dpll_psl;
            Uint32T dpll_psl_max_phase;
            Uint32T dpll_psl_scaling;
            Uint32T dpll_psl_decay;
            Uint32T dpll_range;
            Uint32T dpll_ref_sw_mask;
            Uint32T dpll_ref_ho_mask;
            Uint32T dpll_ho_filter;
            Uint32T dpll_ho_delay;
            Uint32T dpll_priority_1_0;
            Uint32T dpll_priority_3_2;
            Uint32T dpll_priority_5_4;
            Uint32T dpll_priority_7_6;
            Uint32T dpll_priority_9_8;
            Uint32T dpll_lock_phase;
            Uint32T dpll_lock_period;
            Uint32T dpll_fast_lock_ctrl;
            Uint32T dpll_fast_lock_phase_err;
            Uint32T dpll_fast_lock_freq_err;
            Uint32T dpll_damping;
            Uint32T dpll_dual_config;
            Uint32T dpll_pbo;
            Uint32T dpll_pbo_jitter_thresh;
            Uint32T dpll_pbo_min_slope;
            Uint32T dpll_pbo_end_interval;
            Uint32T dpll_pbo_time_out;
            Uint32T dpll_lock_delay;
            Uint32T dpll_fp_first_realign;
            Uint32T dpll_fp_realign_intvl;
            Uint32T dpll_fp_lock_thresh;
        } zl303xx75xDpllMailboxS;

        typedef struct {
            Uint32T vco_freq_base;
            Uint32T vco_freq_mult;
            Uint32T vco_freq_m;
            Uint32T vco_freq_n;
            Uint32T config;
            Uint64T out_a_div;
            Uint32T out_a_driver;
            Uint32T out_a_ctrl;
            Uint32T out_a_width;
            Uint64T out_a_shift;
            Uint64T out_b_div;
            Uint32T out_b_driver;
            Uint32T out_b_ctrl;
            Uint32T out_b_width;
            Uint64T out_b_shift;
        } zl303xx75xSynth0MailboxS;

        typedef struct {
            Uint32T vco_freq_base;
            Uint32T vco_freq_mult;
            Uint32T vco_freq_m;
            Uint32T vco_freq_n;
            Uint32T config;
            Uint64T out_a_div;
            Uint32T out_a_driver;
            Uint32T out_a_ctrl;
            Uint32T out_a_width;
            Uint64T out_a_shift;
            Uint64T out_b_div;
            Uint32T out_b_driver;
            Uint32T out_b_ctrl;
            Uint32T out_b_width;
            Uint64T out_b_shift;
            Uint64T out_c_div;
            Uint32T out_c_driver;
            Uint32T out_c_ctrl;
            Uint32T out_c_width;
            Uint64T out_c_shift;
            Uint64T out_d_div;
            Uint32T out_d_ctrl;
            Uint32T out_d_width;
            Uint64T out_d_shift;
        } zl303xx75xSynth1MailboxS;

        typedef struct {
            Uint32T vco_freq_base;
            Uint32T vco_freq_mult;
            Uint32T vco_freq_m;
            Uint32T vco_freq_n;
            Uint32T config;
            Uint32T config2;
            Uint64T out_a_div;
            Uint32T out_a_driver;
            Uint32T out_a_ctrl;
            Uint32T out_a_width;
            Uint64T out_a_shift;
            Uint64T out_b_div;
            Uint32T out_b_driver;
            Uint32T out_b_ctrl;
            Uint32T out_b_width;
            Uint64T out_b_shift;
            Uint64T out_c_div;
            Uint32T out_c_driver;
            Uint32T out_c_ctrl;
            Uint32T out_c_width;
            Uint64T out_c_shift;
            Uint64T out_d_div;
            Uint32T out_d_driver;
            Uint32T out_d_ctrl;
            Uint32T out_d_width;
            Uint64T out_d_shift;
        } zl303xx75xSynth2MailboxS;

        typedef struct {
            Uint32T vco_freq_base;
            Uint32T vco_freq_mult;
            Uint32T vco_freq_m;
            Uint32T vco_freq_n;
            Uint32T config;
            Uint64T out_a_div;
            Uint32T out_a_driver;
            Uint32T out_a_ctrl;
            Uint32T out_a_width;
            Uint64T out_a_shift;
            Uint64T out_b_div;
            Uint32T out_b_driver;
            Uint32T out_b_ctrl;
            Uint32T out_b_width;
            Uint64T out_b_shift;
        } zl303xx75xSynth3MailboxS;


        typedef struct
        {
            zl303xx75xRefMailBoxS refMb[ZLS3075X_RMBN_last];
            zl303xx75xDpllMailboxS dpllMb[ZLS3075X_DMBN_last];
            zl303xx75xSynth0MailboxS synth_0;
            zl303xx75xSynth1MailboxS synth_1;
            zl303xx75xSynth2MailboxS synth_2;
            zl303xx75xSynth3MailboxS synth_3;
            OS_MUTEX_ID mailboxMutex;

            ZLS3075X_DpllHWModeE pllPriorMode;
            Uint32T userBW;
            Uint32T userBWCustom;
            Uint32T userPSL;
            zl303xx_BooleanE userFastLock;
            zl303xx_BooleanE userTieClear;
            Uint32T hitlessBW;
            Uint32T hitlessBWCustom;
            Uint32T hitlessPSL;
            zl303xx_BooleanE hitlessFastLock;
            Uint32T hitlessDelayPeriod;
            Uint32T hitlessRefSyncDelayPeriod;
            Uint16T gpioOscType;
            Uint8T  spursSuppress;
            struct
            {
               Uint32T postDivPhaseStepMask;
            } synth[ZLS3075X_NUM_SYNTHS];

            hwFuncPtrStickyLock stickyLockCallout;
        } zl303xxd75xS;
    #endif

    #if defined ZLS30771_INCLUDED

        typedef struct {
            /* GP Outputs */
            Uint64T GPVCOFreq;
            Uint64T GPPllId;
            Uint32T GPOUT0Freq;
            Uint32T GPOUT1Freq; 
            /* HP Outputs */
            Uint64T HPSynth1VCOFreq;
            Uint64T HPSynth1PllId;
            Uint32T HPSynth1IntFreq;
            Uint32T HPSynth1FracFreq;
            Uint64T HPSynth2PllId;
            Uint64T HPSynth2VCOFreq;
            Uint32T HPSynth2IntFreq;
            Uint32T HPSynth2FracFreq;
            struct {
                Uint32T dpllId;
                Uint32T synthId;
                Uint32T msdivFreq;
                Uint32T lsdivFreq;
                Uint32T HPOUTPFreq;
                Uint32T HPOUTNFreq;
            } freq[8];
        } zl303xx_Dpll77xOutputConfS;
    
        typedef struct {
            Uint32T ref_freq_base;
            Uint32T ref_freq_mult;
            Uint32T ref_ratio_m;
            Uint32T ref_ratio_n;
            Uint32T ref_config;
            Uint32T ref_scm;
            Uint32T ref_cfm;
            Uint32T ref_gst_disqual;
            Uint32T ref_gst_qual;
            Uint32T ref_pfm_ctrl;
            Uint32T ref_pfm_disqualify;
            Uint32T ref_pfm_qualify;
            Uint32T ref_pfm_period;
            Uint32T ref_pfm_filter_limit;
            Uint32T ref_sync;
            Uint32T ref_sync_misc;
            Uint32T ref_sync_offset_comp;
            Uint32T ref_phase_offset_compensation;
            Uint32T ref_scm_fine;
        } zl303xx77xRefMailBoxS;

        typedef struct {
            Uint32T dpll_bw_fixed;
            Uint32T dpll_bw_var;
            Uint32T dpll_config;
            Uint32T dpll_psl;
            Uint32T dpll_psl_max_phase;
            Uint32T dpll_psl_scaling;
            Uint32T dpll_psl_decay;
            Uint32T dpll_range;
            Uint32T dpll_ref_sw_mask;
            Uint32T dpll_ref_ho_mask;
            Uint32T dpll_dper_sw_mask;
            Uint32T dpll_dper_ho_mask;
            Uint32T dpll_ref_prio_0;
            Uint32T dpll_ref_prio_1;
            Uint32T dpll_ref_prio_2;
            Uint32T dpll_ref_prio_3;
            Uint32T dpll_ref_prio_4;
            Uint32T dpll_dper_prio_1_0;
            Uint32T dpll_dper_prio_3_2;
            Uint32T dpll_ho_filter;
            Uint32T dpll_ho_delay;
            Uint32T dpll_split_xo_config;
            Uint32T dpll_fast_lock_ctrl;
            Uint32T dpll_fast_lock_phase_err;
            Uint32T dpll_fast_lock_freq_err;
            Uint32T dpll_fast_lock_ideal_time;
            Uint32T dpll_fast_lock_notify_time;
            Uint32T dpll_fast_lock_fcl;
            Uint32T dpll_fcl;
            Uint32T dpll_damping;
            Uint32T dpll_phase_bad;
            Uint32T dpll_phase_good;
            Uint32T dpll_duration_good;
            Uint32T dpll_lock_delay;
            Uint32T dpll_tie;
            Uint32T dpll_tie_wr_thresh;
            Uint32T dpll_fp_first_realign;
            Uint32T dpll_fp_align_intvl;
            Uint32T dpll_fp_lock_thresh;
            Uint32T dpll_step_time_thresh;
            Uint32T dpll_step_time_reso;
        } zl303xx77xDpllMailboxS;

        typedef enum {
            ZLS3077X_STS_IDLE,
            ZLS3077X_STS_WAITING_FOR_STEP_END,
            ZLS3077X_STS_IN_GUARD_TIMER
         } zl303xx77xStepStatesE;

        typedef struct {

            /* Configured GP outputs that are stepped */
            Uint32T phaseStepMaskGp;

            /* Configured HP outputs that are stepped */
            Uint32T phaseStepMaskHp;

            /* The type of output that the timestamper is motitoring: either GP or GP. */
            ZLS3077X_OutputTypesE outputTypeThatDrivesTimeStamper;

            /* The user must specify which output drives the timestamper. */
            ZLS3077X_OutputsE outputNumThatDrivesTimeStamper;

            /* When the step is done, a callback is called to align the timestamper */
            hwFuncPtrTODDone stepDoneFuncPtr;

            /* After the frequency has been moved, we may need some time to let things
               settle before moving the timestamper. This is part of that timer. */
            Sint32T guardStartTick;

            /* Accumulate the total phase steps made while in NCO mode */
            Sint32T totalStep;

            /* The states for a small state machine  */
            zl303xx77xStepStatesE STState;

            /* Sanity counter for errors */
            Uint32T sanityStartTick;

            /* Current PhaseStep phase left to do in ns */
            Sint32T phaseStepTodoPhaseNs;

        } zl303xx77xStepTimeDataS;

        typedef struct
        {
            /* mailbox data */
            OS_MUTEX_ID mailboxMutex;   /* Deprecated in 5.2.4 */
            zl303xx77xRefMailBoxS refMb[ZLS3077X_RMBN_last];
            zl303xx77xDpllMailboxS dpllMb[ZLS3077X_DMBN_last];

            /* stepTime() data */
            zl303xx77xStepTimeDataS std;

            /* State data */
            ZLS3077X_DpllHWModeE pllPriorMode;

            /* Misc driver data */
            hwFuncPtrStickyLock stickyLockCallout;

            /* modified registers during hitless reference switching */
            zl303xx_BooleanE restoreRegValues;
            Sint32T restoreStartTick;
            Uint32T prev_ZLS3077X_DPLL_CTRL_x_REG;
            Uint32T prev_ZLS3077X_DPLLX_TIE_REG;

            /* After zl303xx_Dpll77xParamsInit() is called for this data structure,
               drvInitialized=TRUE. */
            zl303xx_BooleanE drvInitialized;

            /* The following are data structures used to store device clock configurations.
               It is generally assumed that the DPLL->Synthesizer->Output structure will remain
               static during runtime. */
            zl303xx_Dpll77xOutputConfS outputConfigData;
            zl303xx_BooleanE outputConfigValid;

            /* For PLL phase measurement */
            ZLS3077X_EdgeDirectionE zl303xx_Dpll77xEdgeDirection;         /* Default = 0 = ZLS3077X_RISING_EDGE */
            Uint32T zl303xx_Dpll77xMeasAveragingFactor;                   /* (0-15), 1=OFF, Default = 0 ~= 40Hz, See datasheet */

            /* A PLL Params pointer (a zl303xx_ParamsS*) to a NCO-Assist DPLL */
            void *pParamsNCOAssist;

            /* Protects Local State Data */
            OS_MUTEX_ID localConfigMutex;

        } zl303xxd77xS;

    #endif

    typedef struct
    {
    #if defined ZLS30361_INCLUDED
        Uint32T postDivPhaseStepMask;
    #endif

    #if defined ZLS30341_INCLUDED
        zl303xx_SynthConfigS config;
        zl303xx_SynthClkConfigS clkConfig[2];
        zl303xx_SynthFpConfigS fpConfig[2];
    #endif
        Uint32T unused;
    } zl303xx_SynthSettingsS;



    typedef struct  /* zl303xx_ParamsS */
    {
        zl303xx_InitStateE  initState;

        /* Device parameters */
        zl303xx_DevTypeE      deviceType;
        zl303xx_DeviceIdE     deviceId;
        Uint32T             deviceRev;
        Uint32T             deviceFWRev;
    #if defined ZLS30731_INCLUDED
        zl303xx_BooleanE      deviceSupportsMiTodB;
        zl303xx_BooleanE      deviceTodActiveB;
    #endif

    #if defined ZLS30341_INCLUDED
            /* ================ 34x data start ================ */

            /* Device interrupt information */
            Uint8T isrMask[ZL303XX_NUM_DEVICE_IRQS];

            /* System Clock variables */
            Uint32T sysClockFreqHz;    /* The system clock rate */
            Uint32T sysClockPeriod;    /* The system clock period (ScaledNs32T) */

            /* DCO Clock variables */
            Uint32T dcoClockFreqHz;    /* The system clock rate */
            Uint32T dcoClockPeriod;    /* The system clock period (ScaledNs32T) */

           /******************/

            struct
            {
                /* Insert Timestamp protocol format and parameters */
                zl303xx_TsEngTsProtocolE tsProtocol;     /* TsEng insert protocol format */
                Uint32T insertFreqHz;      /* INSERT timestamp frequency (Hz) */
                ScaledNs32T insertPeriod;  /* INSERT timestamp period (Scaled Ns) */
                Uint16T tsSizeBytes;       /* INSERT timestamp size (bytes) */

                /* Timestamp Engine Interrupt Configuration */
                Uint8T   isrEnableMask;    /* Which timestamp interrupts are enabled */

                /* Insert timestamp control parameters */
                Uint32T  txCtrlWordLoc;          /* Location of the Tx control word */
                Uint32T  txTsLocation;           /* Location of the Tx timestamp */
                Uint32T  txSchTimeLocation;      /* Location of the Tx scheduled launch time */
                Uint32T  rxTsLocation;           /* Location of the Rx timestamp */
                zl303xx_BooleanE rxTsEnabled;    /* True if Rx timing packets will be timestamped */
                Uint32T  lastRxTimestamp;        /* The Rx timestamp from the the last Rx packet */
                Uint32T  lastTxTimestamp;        /* The Tx timestamp of the last Tx packet */
                Uint32T  aprLastRxTimestamp;    /* The Rx timestamp from the the last Rx packet */
                Uint32T  aprLastTxTimestamp;    /* The Tx timestamp from the the last Tx packet */
                Uint32T  udpChksumLocation;      /* Location of the UDP checksum, if used */
                zl303xx_BooleanE udpChksumEnable;/* True if UDP checksum will be adjusted when packet
                                                  is Tx to allow for inserted timestamp */

                /* Transmit timestamp parameters */
                Uint8T   pktIndex;   /* The current Tx pkt index (when recording exit timestamps) */

                /* Timestamp Engine Sampling Configuration Parameters */
                zl303xx_UpdateTypeE sampleMode;      /* How often local clocks are sampled */
                zl303xx_UpdateTypeE dcoUpdateMode;   /* How often the DCO is updated (& sample generated) */

                /* Interval parameters related to the frequency at which samples of the
                   local clocks are taken. (Useful for uniform sample periods).   */
                Uint8T   sampleIntervalValue;    /* Raw interval value in the device register */
                Uint32T  sampleIntervalInSysClk; /* Sample interval in system clock counts */

                /* Structures used to store the collected Timestamp Engine Samples.
                 These maintain a continuous clock on the local node, accounting for
                 counter rollover and protocol conversions. */
                zl303xx_BooleanE samplesReady;   /* A sample is ready to be processed */

                Uint8T currIndex;        /* Index to the most recent sample taken by TsEng */
                Uint8T sampleCurrIndex;  /* Current Sample to be processed by the APR */

                /* Define the number of elements in the circular array as a power of 2 since the
                MASK will be used to handle the roll-over of the indexes. */
                #define ZL303XX_NUM_TS_SAMPLES              (Uint8T)64
                #define ZL303XX_TS_SAMPLES_ROLLOVER_MASK    (Uint8T)(ZL303XX_NUM_TS_SAMPLES - 1)

                zl303xx_TsSampleS samples[ZL303XX_NUM_TS_SAMPLES];  /* Array of the last samples collected */

                /* Extends the TOD above the 32-bit TOD seconds count (to 48-bits) */
                Uint16T epochCount;
                /* Current Time-of-Day in 32-bit Seconds 32-bit nanoseconds */
                Uint64S currentTime;
                Uint32T currentSys;

                /* Stores a connection and sequence ID for a given 8-bit hardware packet
                * index. */
                struct
                {
                    Uint32T streamHandle;
                    Uint32T msgType;
                    Uint32T seqId;
                } pktIndexMap[256];

                /* function pointer to routine called after the TOD manager has finished
                 changing time */
                hwFuncPtrTODDone TODdoneFuncPtr;


                /******  DEPRECATED MEMBERS ******/
                /* The following members are marked for removal from this sub-structure.
                * Users should refrain from accessing them directly or re-map to the new
                * item where applicable.     */

                /* Timestamp Pulse Alignment Control */
                zl303xx_TodAlignmentE frmPulseAlignment;    /* MOVED to todMgr:onePpsAlignment */

                /* Indicate that the device Time-of-day has changed. */
                Sint32T  todUpdated;                      /* REMOVED due to UNUSED */

                /******  DEPRECATED MEMBERS (END) ******/

            } tsEngParams;

            /******************/
            /* Parameters used to convert timestamp samples from the SYSTEM clock domain
             * to frequencies associated with the core DCO clock (which is typically at
             * some offset from the local SYSTEM clock. */
            struct
            {
                zl303xx_BooleanE initialized;   /* True if the first sample has been taken
                                               * on this device. */
                struct
                {
                    /* When converting a system timestamp to another clock domain (and vice
                     * versa), the converted value is interpolated using the last sample
                     * point and the slope of the relation between the the 2 clocks
                     * (determined at sample time from the sampled freqOfsetUppm value). To
                     * speed up the math operations (and avoid 64-bit division) the system
                     * interval is set to a power of 2 so that shifting can be used
                     * (regardless of the actual sample delta).
                     */
                    Uint32T log2SysTicks;   /* For 80MHz, = 26 (2^26 = maximum SYS ticks
                                          * still less than 1 second). */

                    /* When converting from one frequency to another, the following common
                     * ratio is used:
                     *
                     *    clkDelta     sysDelta                             sysDelta
                     *   ---------- = ----------  SO  clkDelta = clkFreq * ----------
                     *    clkFreq      sysFreq                              sysFreq
                     *
                     * To speed up the arithmetic define sysDelta/sysFreq as a fractional
                     * constant.
                     */
                    Uint32T sysFreqFrac;       /* 2^26 / 80MHz = 0.8388608 */
                                            /* 0.8388608 * 2^32 = 0xD6BF94D6 */
                } convIntvl;

            } tsEngSample;

            /******************/
            /* Parameters used to manage the Time-of-Day settings on the device.  */
            zl303xx_TodMgrS todMgr;

            /* ================ 34x data end ================ */
        #endif

        /* PLL information */
        struct
        {
            Uint32T pllId;
            /* Freqs are stored as ppm x 1e6 = ppt */
            Sint32T dcoFreq;
            Sint32T syncFreq;
            zl303xx_RefIdE selectedRef;
            zl303xx_BooleanE bRefSyncPair;
        #if defined ZLS30361_INCLUDED
            ZLS3036X_DpllModeE pllPriorMode;
            Uint32T userBW;
            Uint32T userBWCustom;
            Uint32T userPSL;
            zl303xx_BooleanE userFastLock;
            zl303xx_BooleanE userTieClear;
            Uint32T hitlessBW;
            Uint32T hitlessBWCustom;
            Uint32T hitlessPSL;
            zl303xx_BooleanE hitlessFastLock;
            Uint32T hitlessDelayPeriod;
            Uint32T hitlessRefSyncDelayPeriod;
            Uint16T gpioOscType;
            Uint8T  spursSuppress;
            hwFuncPtrStickyLock stickyLockCallout;
        #endif

        #if defined ZLS30361_INCLUDED
            zl303xx_SynthSettingsS synth[ZL303XX_MAX_NUM_SYNTHS];
        #endif

            /* Provide a rough GST on the lock state */
            Sint32T lockStateGST;

        #if defined ZLS30701_INCLUDED
            zl303xxd70xS d70x;
        #endif

        #if defined ZLS30721_INCLUDED
            zl303xxd72xS d72x;
        #endif

        #if defined ZLS30731_INCLUDED
            zl303xxd73xS d73x;
        #endif

        #if defined ZLS30751_INCLUDED
            zl303xxd75xS d75x;
        #endif

        #if defined ZLS30771_INCLUDED
            zl303xxd77xS d77x;
        #endif

        #if defined ZLS30361_INCLUDED || defined ZLS30721_INCLUDED || defined ZLS30701_INCLUDED || defined ZLS30751_INCLUDED || defined ZLS30771_INCLUDED
            Sint32T totalHoldoverAccumulation;
            Sint32T lastHoldoverValue;
            Uint32T holdoverIncrementCount;
            zl303xx_HoldoverQualityTypesE holdoverQuality;
            zl303xx_AprLockStatusE lastLockStatus;
        #else
            zl303xx_LockStatusE lastLockStatus;
        #endif

            zl303xx_BooleanE ref1HzDetectEnable;

        #if defined ZLS30341_INCLUDED
            zl303xx_DpllConfigS config[ZL303XX_DPLL_NUM_DPLLS];
        #endif
            Sint32T dcoCountPerPpm;

            zl303xx_BooleanE mitigationEnabled;
        } pllParams;

        /* SPI state information */
        struct
        {
        #if defined OS_LINUX
            #define MAX_DEV_NAME_LEN 32
            Uint8T linuxChipSelectDevName[MAX_DEV_NAME_LEN];
            Uint8T linuxChipHighIntrDevName[MAX_DEV_NAME_LEN];
            Uint8T linuxChipLowIntrDevName[MAX_DEV_NAME_LEN];
            Uint16T linuxHighIntrSignal;
            Uint16T linuxLowIntrSignal;
        #endif
        #if defined OS_VXWORKS
           zl303xx_ChipSelectCtrlS chipSelect;
        #endif
           Uint16T currentPage;

        } spiParams;

        /* The zl303xx_ParamsS structure cannot be altered by the end-user but could
           be extended to contain user-specific information by populating this
           pointer */
        void *userDeviceData;  /* Could be used to get to Device-specific data */

        /* Function to reset the ZL303xx device */
        OS_ARG1_FUNC_PTR resetFuncPtr;


        Uint32T messageRouterCallStatistics[ZL303XX_DPLL_DRIVER_MSG_LAST];
        zl303xx_RdWrStatsS collectRdWrStats;

        /* CUSTOM devices */
        struct
        {
            zlStatusE (*dpllModeSetFuncPtr) (void *hwParams, zl303xx_DpllClockModeE modeSet);
            zlStatusE (*dpllModeGetFuncPtr) (void *hwParams, zl303xx_DpllClockModeE *pModeGet);
        } customDevices;

    } zl303xx_ParamsS;


#else
/* FAKE zl303xx_Param lock/unlock */
#ifndef ZL303XX_LOCK_DEV_PARAMS
#define ZL303XX_LOCK_DEV_PARAMS(x)   ZL303XX_OK
#define ZL303XX_UNLOCK_DEV_PARAMS(x) 
#endif

typedef struct
{
    /* It's a place to store device information */

    Uint32T deviceType;

    /* PLL information */
    struct
    {
        Uint32T pllId;  /* Unique identifier (for logging, etc) */
    } pllParams;

    /* SPI state information */
    struct
    {
        zl303xx_ChipSelectCtrlS chipSelect;
#if defined OS_LINUX
        #define MAX_DEV_NAME_LEN 32
        Uint8T linuxChipSelectDevName[MAX_DEV_NAME_LEN];
#endif
        Uint16T currentPage;
    } spiParams;

      Uint32T messageRouterCallStatistics[ZL303XX_DPLL_DRIVER_MSG_LAST];
      zl303xx_RdWrStatsS collectRdWrStats;

    void *unUsed; 	/* Replace if you wish to store/get to additional info */

    /* CUSTOM devices */
    struct
    {
        zlStatusE (*dpllModeSetFuncPtr) (void *hwParams, zl303xx_DpllClockModeE modeSet);
        zlStatusE (*dpllModeGetFuncPtr) (void *hwParams, zl303xx_DpllClockModeE *pModeGet);
    } customDevices;

} zl303xx_ParamsS;


#endif

#if defined __cplusplus
}
#endif

#endif /* MULTIPLE INCLUDE BARRIER */
