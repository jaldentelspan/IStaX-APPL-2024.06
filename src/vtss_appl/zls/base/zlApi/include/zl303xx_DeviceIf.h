

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
*     This file contains functions related to the ...
*
*******************************************************************************/

#ifndef _ZL303XX_DEVICE_IF_H_
#define _ZL303XX_DEVICE_IF_H_

#if defined __cplusplus
extern "C" {
#endif


/*****************   INCLUDE FILES   ******************************************/
#include "zl303xx_Global.h"

/*****************   DEFINES   ************************************************/

/*****************   DATA TYPES   *********************************************/

/* Device family defines */
typedef enum
{
    UNKNOWN_DEVICETYPE = 0,
    ZL3031X_DEVICETYPE,
    ZL30330_DEVICETYPE,
    ZL3034X_DEVICETYPE,
    ZL3036X_DEVICETYPE,
    ZL3075X_DEVICETYPE,
    ZL3072X_DEVICETYPE,
    ZL3070X_DEVICETYPE,
    ZL3077X_DEVICETYPE,
    CUSTOM_DEVICETYPE,
    ZL3073X_DEVICETYPE
} zl303xx_DevTypeE;


/* DeviceId valid values */
typedef enum
{
    /* Note: In general, across devices, the 7xx series parts are supported by APR */
    UNKNOWN_DEV_TYPE = 0x0000,

    ZL30161_DEV_TYPE = 0x00A1,
    ZL30162_DEV_TYPE = 0x00A2,
    ZL30163_DEV_TYPE = 0x00A3,
    ZL30164_DEV_TYPE = 0x00A4,
    ZL30165_DEV_TYPE = 0x00A5,
    ZL30166_DEV_TYPE = 0x00A6,
    ZL30167_DEV_TYPE = 0x00A7,
    ZL30168_DEV_TYPE = 0x0014,    /* Corrected in EBinder 14522 */

    ZL30342_DEV_TYPE = 0x00EC,
    ZL30343_DEV_TYPE = 0x00ED,
    ZL30347_DEV_TYPE = 0x00EF,

    ZL30361_DEV_TYPE = 0x003D,
    ZL30362_DEV_TYPE = 0x003E,
    ZL30363_DEV_TYPE = 0x003F,
    ZL30364_DEV_TYPE = 0x0040,
    ZL30365_DEV_TYPE = 0x0041,
    ZL30366_DEV_TYPE = 0x0042,
    ZL30367_DEV_TYPE = 0x0043,
    /* 72x for 1588 */
    ZL303XX_DEVICE_ID_ZL30250 = 0x0000,
    ZL303XX_DEVICE_ID_ZL30251 = 0x0001,
    ZL303XX_DEVICE_ID_ZL30252 = 0x0002,
    ZL303XX_DEVICE_ID_ZL30253 = 0x0003,
    ZL303XX_DEVICE_ID_ZL30244 = 0x0004,
    ZL303XX_DEVICE_ID_ZL30151 = 0x0005,
    ZL303XX_DEVICE_ID_ZL30245 = 0x0006,
    ZL303XX_DEVICE_ID_ZL30169 = 0x0007,
    ZL303XX_DEVICE_ID_ZL30182 = 0x0008,
    ZL303XX_DEVICE_ID_ZL30621 = 0x0009,
    ZL303XX_DEVICE_ID_ZL30622 = 0x000A,
    ZL303XX_DEVICE_ID_ZL30623 = 0x000B,
    ZL303XX_DEVICE_ID_ZL30255 = 0x000E,
    ZL303XX_DEVICE_ID_ZL30721 = 0x0029, /* Rev 2, ID 0x9 */
    ZL303XX_DEVICE_ID_ZL30722 = 0x002A, /* Rev 2, ID 0xA */
    ZL303XX_DEVICE_ID_ZL30723 = 0x002B, /* Rev 2, ID 0xB */
    /* 70x for 1588 */
    ZL303XX_DEVICE_ID_ZL30174 = 0x0c66,
    ZL303XX_DEVICE_ID_ZL30611 = 0x0e1b,
    ZL303XX_DEVICE_ID_ZL30612 = 0x0e1c,
    ZL303XX_DEVICE_ID_ZL30614 = 0x0e1e,
    ZL303XX_DEVICE_ID_ZL30601 = 0x0e11,
    ZL303XX_DEVICE_ID_ZL30602 = 0x0e12,
    ZL303XX_DEVICE_ID_ZL30603 = 0x0e13,
    ZL303XX_DEVICE_ID_ZL30604 = 0x0e14,
    ZL303XX_DEVICE_ID_ZL30701 = 0x0e75,
    ZL303XX_DEVICE_ID_ZL30702 = 0x0e76,
    ZL303XX_DEVICE_ID_ZL30703 = 0x0e77,
    ZL303XX_DEVICE_ID_ZL30704 = 0x0e78,
    ZL303XX_DEVICE_ID_ZL80029 = 0x1e5d,
    /* 73x for 1588 */
    /* Note: the 7xx series parts are supported by IEEE1588 */
    ZL303XX_DEVICE_ID_ZL30631 = 0x0E2F,
    ZL303XX_DEVICE_ID_ZL30631_7x7 = 0x8E2F,  /* (+8000 for 7x7) */
    ZL303XX_DEVICE_ID_ZL30632 = 0x0E30,
    ZL303XX_DEVICE_ID_ZL30632_7x7 = 0x8E30,  /* (+8000 for 7x7) */
    ZL303XX_DEVICE_ID_ZL30633 = 0x0E31,
    ZL303XX_DEVICE_ID_ZL30634 = 0x0E32,
    ZL303XX_DEVICE_ID_ZL30635 = 0x0E33,
    ZL303XX_DEVICE_ID_ZL30640 = 0x0E38,
    ZL303XX_DEVICE_ID_ZL30641 = 0x0E39,
    ZL303XX_DEVICE_ID_ZL30641_7x7 = 0x8E39,  /* (+8000 for 7x7) */
    ZL303XX_DEVICE_ID_ZL30642 = 0x0E3A,
    ZL303XX_DEVICE_ID_ZL30643 = 0x0E3B,
    ZL303XX_DEVICE_ID_ZL30644 = 0x0E3C,
    ZL303XX_DEVICE_ID_ZL30645 = 0x0E3D,
    ZL303XX_DEVICE_ID_ZL30731 = 0x0E93,
    ZL303XX_DEVICE_ID_ZL30731_7x7 = 0x8E93,  /* (+8000 for 7x7) */
    ZL303XX_DEVICE_ID_ZL30732 = 0x0E94,
    ZL303XX_DEVICE_ID_ZL30733 = 0x0E95,
    ZL303XX_DEVICE_ID_ZL30734 = 0x0E96,
    ZL303XX_DEVICE_ID_ZL30735 = 0x0E97,
    ZL303XX_DEVICE_ID_ZL30631A = 0x1E2F,    /* *A= MiToD support */
    ZL303XX_DEVICE_ID_ZL30631A_7x7 = 0x9E2F, /* (+8000 for 7x7) */
    ZL303XX_DEVICE_ID_ZL30632A = 0x1E30,
    ZL303XX_DEVICE_ID_ZL30633A = 0x1E31,
    ZL303XX_DEVICE_ID_ZL30634A = 0x1E32,
    ZL303XX_DEVICE_ID_ZL30635A = 0x1E33,
    ZL303XX_DEVICE_ID_ZL30640A = 0x1E38,
    ZL303XX_DEVICE_ID_ZL30641A = 0x1E39,
    ZL303XX_DEVICE_ID_ZL30641A_7x7 = 0x9E39, /* (+8000 for 7x7) */
    ZL303XX_DEVICE_ID_ZL30642A = 0x1E3A,
    ZL303XX_DEVICE_ID_ZL30643A = 0x1E3B,
    ZL303XX_DEVICE_ID_ZL30644A = 0x1E3C,
    ZL303XX_DEVICE_ID_ZL30645A = 0x1E3D,
    ZL303XX_DEVICE_ID_ZL30731A = 0x1E93,
    ZL303XX_DEVICE_ID_ZL30731A_7x7 = 0x9E93, /* (+8000 for 7x7) */
    ZL303XX_DEVICE_ID_ZL30732A = 0x1E94,
    ZL303XX_DEVICE_ID_ZL30733A = 0x1E95,
    ZL303XX_DEVICE_ID_ZL30734A = 0x1E96,
    ZL303XX_DEVICE_ID_ZL30735A = 0x1E97,
    ZL303XX_DEVICE_ID_ZL30270 = 0x0CC6,
    ZL303XX_DEVICE_ID_ZL30270_7x7 = 0x8CC6, /* (+8000 for 7x7) */
    ZL303XX_DEVICE_ID_ZL30271 = 0x0CC7,
    ZL303XX_DEVICE_ID_ZL30272 = 0x0CC8,
    ZL303XX_DEVICE_ID_ZL30272_7x7 = 0x8CC8, /* (+8000 for 7x7) */
    ZL303XX_DEVICE_ID_ZL30273 = 0x0CC9,
    ZL303XX_DEVICE_ID_ZL30274 = 0x0CCA,
    /* 75x for 1588 */
    ZL303XX_DEVICE_ID_ZL30652 = 0x0E44,
    ZL303XX_DEVICE_ID_ZL30653 = 0x0E45,
    ZL303XX_DEVICE_ID_ZL30654 = 0x0E46,
    ZL303XX_DEVICE_ID_ZL30752 = 0x0EA8,
    ZL303XX_DEVICE_ID_ZL30753 = 0x0EA9,
    ZL303XX_DEVICE_ID_ZL30754 = 0x0EAA,
    /* 77x for 1588 */
    ZL303XX_DEVICE_ID_ZL30671 = 0x0E57,
    ZL303XX_DEVICE_ID_ZL30672 = 0x0E58,
    ZL303XX_DEVICE_ID_ZL30673 = 0x0E59,
    ZL303XX_DEVICE_ID_ZL80062 = 0x2F7E,
    ZL303XX_DEVICE_ID_ZL30771 = 0x0EBB,
    ZL303XX_DEVICE_ID_ZL30772 = 0x0EBC,
    ZL303XX_DEVICE_ID_ZL30773 = 0x0EBD,
    /* 79x for 1588 */
    ZL303XX_DEVICE_ID_ZL30661 = 0x0E4D,
    ZL303XX_DEVICE_ID_ZL30662 = 0x0E4E,
    ZL303XX_DEVICE_ID_ZL30663 = 0x0E4F,
    ZL303XX_DEVICE_ID_ZL30681 = 0x0E61,
    ZL303XX_DEVICE_ID_ZL30682 = 0x0E62,
    ZL303XX_DEVICE_ID_ZL30683 = 0x0E63,
    ZL303XX_DEVICE_ID_ZL30791 = 0x0ECF,
    ZL303XX_DEVICE_ID_ZL30792 = 0x0ED0,
    ZL303XX_DEVICE_ID_ZL30793 = 0x0ED1,
    ZL303XX_DEVICE_ID_ZL30794 = 0x0ED2,
    ZL303XX_DEVICE_ID_ZL30795 = 0x0ED3,
    /* 82x for 1588 */
    ZL303XX_DEVICE_ID_ZL30801 = 0x0ED9,
    ZL303XX_DEVICE_ID_ZL30802 = 0x0EDA,
    ZL303XX_DEVICE_ID_ZL30803 = 0x0EDB,
    ZL303XX_DEVICE_ID_ZL30804 = 0x0EDC,
    ZL303XX_DEVICE_ID_ZL30805 = 0x0EDD,
    ZL303XX_DEVICE_ID_ZL30806 = 0x0EDE,
    ZL303XX_DEVICE_ID_ZL30811 = 0x0EE3,
    ZL303XX_DEVICE_ID_ZL30812 = 0x0EE4,
    ZL303XX_DEVICE_ID_ZL30813 = 0x0EE5,
    ZL303XX_DEVICE_ID_ZL30821 = 0x0EED,
    ZL303XX_DEVICE_ID_ZL30822 = 0x0EEE,
    ZL303XX_DEVICE_ID_ZL30823 = 0x0EEF,
    /* 85x for 1588 */
    ZL303XX_DEVICE_ID_ZL30831 = 0x0EF7,
    ZL303XX_DEVICE_ID_ZL30832 = 0x0EF8,
    ZL303XX_DEVICE_ID_ZL30833 = 0x0EF9,
    ZL303XX_DEVICE_ID_ZL30834 = 0x0EFA,
    ZL303XX_DEVICE_ID_ZL30835 = 0x0EFB,
    ZL303XX_DEVICE_ID_ZL30836 = 0x0EFC,
    ZL303XX_DEVICE_ID_ZL30841 = 0x0F01,
    ZL303XX_DEVICE_ID_ZL30842 = 0x0F02,
    ZL303XX_DEVICE_ID_ZL30843 = 0x0F03,
    ZL303XX_DEVICE_ID_ZL30851 = 0x0F0B,
    ZL303XX_DEVICE_ID_ZL30852 = 0x0F0C,
    ZL303XX_DEVICE_ID_ZL30853 = 0x0F0D,
    /* 80032 for 1588 */
    /* Note: the 7xx series parts are supported by IEEE1588 */
    ZL303XX_DEVICE_ID_ZL80032 = 0x1F60,
    ZL303XX_DEVICE_ID_ZL80034 = 0x1F62,
    ZL303XX_DEVICE_ID_ZL80035 = 0x1F63,
    ZL303XX_DEVICE_ID_ZL80132 = 0x1FC4,
    ZL303XX_DEVICE_ID_ZL80732 = 0x221C
} zl303xx_DeviceIdE;


typedef enum
{
   ZL303XX_REF_AUTO = -1,
   ZL303XX_REF_ID_0 = 0,
   ZL303XX_REF_ID_1 = 1,
   ZL303XX_REF_ID_2 = 2,
   ZL303XX_REF_ID_3 = 3,
   ZL303XX_REF_ID_4 = 4,
   ZL303XX_REF_ID_5 = 5,
   ZL303XX_REF_ID_6 = 6,
   ZL303XX_REF_ID_7 = 7,
   ZL303XX_REF_ID_8 = 8,
   ZL303XX_REF_ID_9 = 9,  /* Most devices support 0 to 9 */
   ZL303XX_REF_ID_10= 10,
   ZL303XX_LAST_REF=ZL303XX_REF_ID_10
} zl303xx_RefIdE;


/* DpllId valid values */
typedef enum
{
    ZL303XX_DPLL_ID_1 = 0,
    ZL303XX_DPLL_ID_2,
    ZL303XX_DPLL_ID_3,
    ZL303XX_DPLL_ID_4,
    ZL303XX_DPLL_ID_5,
    ZL303XX_DPLL_ID_6,
} zl303xx_DpllIdE;


/* A driver TOD done function type */
typedef Sint32T (*hwFuncPtrTODDone)(void*);

/* Shared types between APR and the drivers */
/* 1) Lock status */
typedef enum
{
   ZL303XX_LOCK_STATUS_ACQUIRING = 0,
   ZL303XX_LOCK_STATUS_LOCKED,
   ZL303XX_LOCK_STATUS_PHASE_LOCKED,
   ZL303XX_LOCK_STATUS_HOLDOVER,
   ZL303XX_LOCK_STATUS_REF_FAILED,
   ZL303XX_LOCK_NO_ACTIVE_SERVER,
   ZL303XX_LOCK_STATUS_UNKNOWN,
   ZL303XX_APRLOCKSTATE_MAX = ZL303XX_LOCK_STATUS_UNKNOWN
} zl303xx_AprLockStatusE;

#define  zl303xx_LockStatusE zl303xx_AprLockStatusE

/* 2) Holdover Quality */
typedef enum {
    HOLDOVER_QUALITY_UNKNOWN        = 0,
    HOLDOVER_QUALITY_IN_SPEC,
    HOLDOVER_QUALITY_LOCKED_TO_SYNCE,
    HOLDOVER_QUALITY_OUT_OF_SPEC,

    HOLDOVER_QUALITY_LAST = HOLDOVER_QUALITY_OUT_OF_SPEC
} zl303xx_HoldoverQualityTypesE;

/* 3) Hitless compensation types */
typedef enum
{
   ZL303XX_HITLESS_COMP_FALSE,
   ZL303XX_HITLESS_COMP_TRUE,
   ZL303XX_HITLESS_COMP_AUTO
} zl303xx_HitlessCompE;

/* Software Hybrid Transient State */
typedef enum {
   ZL303XX_BHTT_NOT_ACTIVE,
   ZL303XX_BHTT_QUICK,
   ZL303XX_BHTT_OPTIONAL,
   ZL303XX_BHTT_LAST
} zl303xx_BCHybridTransientType;

/* This structure is used to retrieve output clock frequencies via the device driver.
   All message router data must be pass by value, hence this data structure. */
#define ZL303XX_CLOCK_OUTPUT_FREQ_ARRAY_SIZE         (10*2)
typedef struct {
    Uint32T array[ZL303XX_CLOCK_OUTPUT_FREQ_ARRAY_SIZE];
} zl303xx_ClockOutputFreqArrayS;

/* CGU Message Router Section */
/* The CGU message router is a softare mechanism for the APR algorithm to
   interface with a Microsemi or custom device.  The CGU message router
   defines all necessary HW device operations that will be required at
   run time.  */

/* Driver callout function type. A routine of this type is provided by each
   driver and passed to APR at initialisation. APR uses this callout to access
   driver functions. */
typedef Sint32T (*hwFuncPtrDriverMsgRouter)(void*, void*, void*);

/* Main Message Router Message Type ENUM*/
typedef enum {
    ZL303XX_DPLL_DRIVER_MSG_GET_DEVICE_INFO                               = 0,
    ZL303XX_DPLL_DRIVER_MSG_GET_FREQ_I_OR_P                               = 1,
    ZL303XX_DPLL_DRIVER_MSG_SET_FREQ                                      = 2,
    ZL303XX_DPLL_DRIVER_MSG_TAKE_HW_NCO_CONTROL                           = 3,
    ZL303XX_DPLL_DRIVER_MSG_RETURN_HW_NCO_CONTROL                         = 4,
    ZL303XX_DPLL_DRIVER_MSG_SET_TIME                                      = 5,
    ZL303XX_DPLL_DRIVER_MSG_STEP_TIME                                     = 6,
    ZL303XX_DPLL_DRIVER_MSG_JUMP_ACTIVE_CGU                               = 7,
    /* DEPRECATED                                                       = 8,*/
    ZL303XX_DPLL_DRIVER_MSG_GET_HW_MANUAL_FREERUN_STATUS                  = 9,
    ZL303XX_DPLL_DRIVER_MSG_GET_HW_MANUAL_HOLDOVER_STATUS                 = 10,
    ZL303XX_DPLL_DRIVER_MSG_GET_HW_SYNC_INPUT_EN_STATUS                   = 11,
    ZL303XX_DPLL_DRIVER_MSG_GET_HW_OUT_OF_RANGE_STATUS                    = 12,
    /* DEPRECATED                                                       = 13,*/
    ZL303XX_DPLL_DRIVER_MSG_DEVICE_PARAMS_INIT                            = 14,
    ZL303XX_DPLL_DRIVER_MSG_CURRENT_REF_GET                               = 15,
    ZL303XX_DPLL_DRIVER_MSG_CURRENT_REF_SET                               = 16,
    /* DEPRECATED                                                       = 17,*/
    /* DEPRECATED                                                       = 18,*/
    /* DEPRECATED                                                       = 19,*/
    /* DEPRECATED                                                       = 20,*/
    /* DEPRECATED                                                       = 21,*/
    /* DEPRECATED                                                       = 22,*/
    /* DEPRECATED                                                       = 23,*/
    /* DEPRECATED                                                       = 24,*/
    ZL303XX_DPLL_DRIVER_MSG_INPUT_PHASE_ERROR_WRITE                       = 25,
    ZL303XX_DPLL_DRIVER_MSG_INPUT_PHASE_ERROR_WRITE_CTRL_READY            = 26,
    /* DEPRECATED                                                       = 27,*/
    /* DEPRECATED                                                       = 28,*/
    /* DEPRECATED                                                       = 29,*/
    /* DEPRECATED                                                       = 30,*/
    /* DEPRECATED                                                       = 31,*/
    /* DEPRECATED                                                       = 32, */
    ZL303XX_DPLL_DRIVER_MSG_REGISTER_TOD_DONE_FUNC                        = 33,
    ZL303XX_DPLL_DRIVER_MSG_CONFIRM_HW_CNTRL                              = 34,
    ZL303XX_DPLL_DRIVER_MSG_GET_HW_LOCK_STATUS                            = 35,
    ZL303XX_DPLL_DRIVER_MSG_GET_STEP_TIME_CURR_MAX_STEP_SIZE              = 36,
    /* DEPRECATED                                                       = 37,*/
    ZL303XX_DPLL_DRIVER_MSG_CLEAR_HOLDOVER_FFO                            = 38,
    ZL303XX_DPLL_DRIVER_MSG_SET_AFBDIV                                    = 40,
    /* DEPRECATED                                                       = 41, */
    ZL303XX_DPLL_DRIVER_MSG_DETERMINE_MAX_STEP_SIZE_PER_ADJUSTMENT        = 42,
    /* DEPRECATED                                                       = 43,*/
    /* DEPRECATED                                                       = 44,*/
    /* DEPRECATED                                                       = 45,*/
    /* DEPRECATED                                                       = 46,*/
    ZL303XX_DPLL_DRIVER_MSG_ADJUST_TIME                                   = 47,  /* New for 4.9.0 */
    ZL303XX_DPLL_DRIVER_MSG_ADJUST_TIME_DCO_READABLE                      = 48,  /* New for 4.9.0 */
    ZL303XX_DPLL_DRIVER_MSG_BC_HYBRID_ACTION_PHASE_LOCK                   = 49,  /* New for 4.9.0 */
    ZL303XX_DPLL_DRIVER_MSG_BC_HYBRID_ACTION_OUT_OF_LOCK                  = 50,  /* New for 4.9.0 */
    ZL303XX_DPLL_DRIVER_MSG_HITLESS_ELEC_SWITCHING_INITIAL                = 51,  /* New for 4.9.0 */
    ZL303XX_DPLL_DRIVER_MSG_HITLESS_ELEC_SWITCHING_STAGE1                 = 52,  /* New for 4.9.0 */
    ZL303XX_DPLL_DRIVER_MSG_HITLESS_ELEC_SWITCHING_STAGE2                 = 53,  /* New for 4.9.0 */
    ZL303XX_DPLL_DRIVER_MSG_SET_NCO_ASSIST_ENABLE                         = 54,
    ZL303XX_DPLL_DRIVER_MSG_GET_NCO_ASSIST_ENABLE                         = 55,
    ZL303XX_DPLL_DRIVER_MSG_GET_NCO_ASSIST_PARAMS                         = 56,
    ZL303XX_DPLL_DRIVER_MSG_GET_NCO_ASSIST_FREQ_OFFSET                    = 57,
    ZL303XX_DPLL_DRIVER_MSG_GET_NCO_ASSIST_HW_LOCK_STATUS                 = 58,
    ZL303XX_DPLL_DRIVER_MSG_GET_NCO_ASSIST_SYNC_INPUT_EN_STATUS           = 59,
    ZL303XX_DPLL_DRIVER_MSG_GET_NCO_ASSIST_OUT_OF_RANGE_STATUS            = 60,
    ZL303XX_DPLL_DRIVER_MSG_GET_NCO_ASSIST_MANUAL_HOLDOVER_STATUS         = 61,
    ZL303XX_DPLL_DRIVER_MSG_GET_NCO_ASSIST_MANUAL_FREERUN_STATUS          = 62,
    ZL303XX_DPLL_DRIVER_MSG_GET_MODIFY_STEP_TIME_NS                       = 63,
    ZL303XX_DPLL_DRIVER_MSG_PHASE_STEP_OUTPUT_IS_HP                       = 64,
    ZL303XX_DPLL_DRIVER_MSG_SET_MODE_HOLDOVER                             = 65,
    ZL303XX_DPLL_DRIVER_MSG_GET_STEP_TIME_ACTIVE                          = 66,
    ZL303XX_DPLL_DRIVER_MSG_GET_HW_HOLDOVER_READY                         = 67, /* New for 5.0.4 on ZL3077x */
    ZL303XX_DPLL_DRIVER_MSG_GET_MONITOR_STATUS_BITS                       = 68, /* New for 5.0.4 on ZL3070x */
    ZL303XX_DPLL_DRIVER_MSG_GET_PHASE_STEP_RESOLUTIONS_FREQ               = 69, /* New for 5.0.6 */
    ZL303XX_DPLL_DRIVER_MSG_GET_OUTPUT_CLOCKS_FREQ                        = 70,
    ZL303XX_DPLL_DRIVER_MSG_GET_SW_HYBRID_TRANSIENT_STATUS                = 71, /* New for 5.1.0 */
    ZL303XX_DPLL_DRIVER_MSG_SET_SW_HYBRID_TRANSIENT_STATUS                = 72, /* New for 5.1.0 */
    ZL303XX_DPLL_DRIVER_MSG_SET_MODE_HOLDOVER_FREQ_OFFSET                 = 73, /* New for 5.3.8 */
    ZL303XX_DPLL_DRIVER_MSG_GET_DEVICE_FW_REV                             = 74, /* New for 5.3.10 */
    ZL303XX_DPLL_DRIVER_MSG_SET_DEVICE_MODE                               = 75, /* New for 5.5.2 */
    ZL303XX_DPLL_DRIVER_MSG_GET_DEVICE_MODE                               = 76, /* New for 5.5.2 */

    ZL303XX_DPLL_DRIVER_MSG_LAST

} zl303xx_DpllDriverMsgTypesE;

typedef enum
{
    ZL303XX_HWHO_UNKNOWN = -2,
    ZL303XX_HWHO_NOT_SUPPORTED = -1,
    ZL303XX_HWHO_NOT_READY = 0,
    ZL303XX_HWHO_READY
} zl303xx_AprHOReadyE;

/* These clock modes should overlap with most ZL DPLL family specific modes (note that 72X is not 1-to-1) */
typedef enum
{
   ZL303XX_DPLL_MODE_MANUAL_FREERUN   = 0x0,
   ZL303XX_DPLL_MODE_MANUAL_HOLDOVER  = 0x1,
   ZL303XX_DPLL_MODE_NORMAL_FORCED_REF= 0x2,
   ZL303XX_DPLL_MODE_NORMAL_AUTOMATIC = 0x3,
   ZL303XX_DPLL_MODE_NORMAL_NCO       = 0x4,
   ZL303XX_DPLL_MODE_ASSISTED_NCO     = 0x5,

   ZL303XX_DPLL_MODE_LAST
} zl303xx_DpllClockModeE;




/* Data structure passed to and received from the given driver.
   - inDataS  = data into the driver
   - outDataS = data out of the driver
*/

/* get device info */
typedef struct {
    zl303xx_DevTypeE devType;
    zl303xx_DeviceIdE devId;
    Uint32T devRev;
} zl303xx_GetDeviceInfoOutDataS;

/* Get freq data */
typedef struct {
    Sint32T memPart;
} zl303xx_GetFreqInDataS;
typedef struct {
    Sint32T freqOffsetInPpt;
} zl303xx_GetFreqOutDataS;

/* Set freq data */
typedef struct {
    Sint32T freqOffsetInPpt;
} zl303xx_SetFreqInDataS;

/* SetTime */
typedef struct {
    Uint32T pllId;
    Uint64S seconds;
    Uint32T nanoSeconds;
    zl303xx_BooleanE bBackwardAdjust;
} zl303xx_SetTimeInDataS;

/* stepTime data */
typedef struct {
    Uint32T pllId;
    Sint32T deltaTime;
    zl303xx_BooleanE inCycles;
    Uint32T clockFreq;
} zl303xx_StepTimeInDataS;

/* jumpActiveCGU */
typedef struct {
    Uint32T pllId;
    Uint64S seconds;
    Uint32T nanoSeconds;
    zl303xx_BooleanE bBackwardAdjust;
} zl303xx_JumpActiveCGUInDataS;


/* Get HW manual freerun status data */
typedef struct {
    Sint32T status;
} zl303xx_GetHWManualFreerunOutDataS;

/* Get HW manual holdover status data */
typedef struct {
    Sint32T status;
} zl303xx_GetHWManualHoldoverOutDataS;

/* Get HW sync output enabled status data */
typedef struct {
    Sint32T status;
} zl303xx_GetHWSyncInputEnOutDataS;

/* Get HW out-of-range status data */
typedef struct {
    Sint32T status;
} zl303xx_GetHWOutOfRangeOutDataS;

/* device initialisation data */
typedef struct {
    void *hwParams;         /* DEPRECATED - hwParams is now passed as parameter */
} zl303xx_DeviceInitInDataS;

/* Get current ref data */
typedef struct {
    Uint32T ref;
} zl303xx_GetCurrRefOutDataS;

/* Set ref data */
typedef struct {
    Uint32T ref;
} zl303xx_SetCurrRefInDataS;

/* Input phase error write data */
typedef struct {
    Uint32T tieNs;
} zl303xx_InputPhaseErrorWriteInDataS;

/* Input phase error write control data data */
typedef struct {
    zl303xx_BooleanE ready;
} zl303xx_InputPhaseErrorWriteCtrlReadyOutDataS;

/* Set TOD done callback function data */
typedef struct {
   hwFuncPtrTODDone TODdoneFuncPtr;
} zl303xx_SetTODDoneFuncInDataS;

/* confirm Hw Cntrl data */
typedef struct {
    Uint32T addr;
} zl303xx_ConfirmHwCntrlInDataS;
typedef struct {
    Uint32T data;
} zl303xx_ConfirmHwCntrlOutDataS;

/* Get HW lock status data */
typedef struct {
    Sint32T lockStatus;
} zl303xx_GetHWLockStatusOutDataS;

/* get step time current max step size*/
typedef struct {
    Sint32T size;
} zl303xx_GetStepTimeCurrMaxStepSizeOutDataS;

/* Set AFBDIV */
typedef struct {
    Sint32T df;
} zl303xx_SetAFBDIVInDataS;

/* determine the max step size that the  */
typedef struct {
    Uint32T maxStepFaster;
    Uint32T maxStepSlower;
} zl303xx_DetermineMaxStepSizePerAdjustmentOutDataS;

/* Adjust time function from customer */
typedef struct {
    Sint32T adjustment;
    Uint32T recomendedTime;
} zl303xx_AdjustTimeInDataS;

/* Called before adjustTime, determine if DCO is readable during adjustTime */
typedef struct {
    Sint32T adjustment;
} zl303xx_AdjustTimeDCOReadableInDataS;
typedef struct {
    zl303xx_BooleanE dcoReadable;
} zl303xx_AdjustTimeDCOReadableOutDataS;

/* Action for electrical reference switching (Initial call) */
typedef struct {
    Sint32T refId;
    Sint32T syncId;
} zl303xx_SetActiveElecActionsInitialInDataS;

/* Action for hitless electrical reference switching (Stage 1, after first delay, RefSync and 1pps only) */
typedef struct {
    Uint32T refId;
} zl303xx_SetActiveElecActionsStage1InDataS;

/* Action for hitless electrical reference switching (Stage 2, after second delay, RefSync and 1pps only) */
typedef struct {
    Uint32T refId;
} zl303xx_SetActiveElecActionsStage2InDataS;

/* Get/Set NCOAssist enable/disable */
typedef struct {
    zl303xx_BooleanE enable;
} zl303xx_SetNCOAssistEnableInDataS;
typedef struct {
    zl303xx_BooleanE enable;
} zl303xx_GetNCOAssistEnableOutDataS;

/* Get NCOAssist ParamsS */
typedef struct {
    void *NCOAssistParams;
} zl303xx_GetNCOAssistParamsSOutDataS;

/* Get NCOAssist pair freq offset */
typedef struct {
    Sint32T memPart;
} zl303xx_GetNCOAssistPairFreqOffsetInDataS;
typedef struct {
    Sint32T freqOffsetInPpt;
} zl303xx_GetNCOAssistPairFreqOffsetOutDataS;

/* Get NCOAssist pair HW lock status data */
typedef struct {
    Sint32T lockStatus;
} zl303xx_GetNCOAssistPairHWLockStatusOutDataS;

/* Get NCO-assist sync output enabled status data */
typedef struct {
    Sint32T status;
} zl303xx_GetNCOAssistSyncInputEnOutDataS;

/* Get NCO-assist out-of-range status data */
typedef struct {
    Sint32T status;
} zl303xx_GetNCOAssistOutOfRangeOutDataS;

/* Get NCO-assist manual holdover status data */
typedef struct {
    Sint32T status;
} zl303xx_GetNCOAssistManualHoldoverOutDataS;

/* Get NCO-assist manual freerun status data */
typedef struct {
    Sint32T status;
} zl303xx_GetNCOAssistManualFreerunOutDataS;

/* Get modified (for resolution) step time value */
typedef struct {
    Sint32T stepTimeValueIn;
} zl303xx_GetModifiedStepTimeValueInDataS;
typedef struct {
    Sint32T stepTimeValueOut;
} zl303xx_GetModifiedStepTimeValueOutDataS;

/* Get the immediate DPLL monitor status bits */
typedef struct {
    zl303xx_BooleanE bPSLHit;
    zl303xx_BooleanE bPMLHit;
    zl303xx_BooleanE bPullInHoldInHit;
    zl303xx_BooleanE bHoldover;
    zl303xx_BooleanE bLocked;
} zl303xx_GetMonitorStatusBitsOutDataS;

/* Get output type that drives the timestamper */
typedef struct {
    zl303xx_BooleanE b;
} zl303xx_GetIsOutputTypeThatDrivesTimeStamperHP;

/* Get stepTime active */
typedef struct {
    zl303xx_BooleanE b;
} zl303xx_GetStepTimeActive;

/* Get HW HR active */
typedef struct {
    zl303xx_AprHOReadyE readyFlag;
} zl303xx_GetHwHoldoverReadyS;

/* Get Phase Step resolution */
typedef struct {
    zl303xx_ClockOutputFreqArrayS clockFreqArray;
} zl303xx_GetPhaseStepResolutionsFreqsOutS;

/* Get Clock Period LCM */
typedef struct {
    zl303xx_ClockOutputFreqArrayS clockFreqArray;
} zl303xx_GetClockOutputsFreqOutS;


/* Get SW Hybrid Transient Status */
typedef struct {
    zl303xx_BCHybridTransientType BCHybridTransientType;
} zl303xx_GetSWHybridTransientStatusOutDataS;

/* Set SW Hybrid Transient Status */
typedef struct {
    zl303xx_BCHybridTransientType BCHybridTransientType;
} zl303xx_SetSWHybridTransientStatusInDataS;


typedef struct {
    Uint32T dpllId;
} zl303xxdpllIdS;

/* Send no active server info */
typedef struct {
    Sint32T dfValue;
    Sint32T algHOFreq;
    Uint16T currentServerId;
    zl303xx_BooleanE algHOValid;
    zl303xx_BooleanE serverHOValid;
} zl303xx_SendModeHoldoverFreqOffsetInDataS;

/* get firmware revision */
typedef struct {
    Uint32T devFWRev;
} zl303xx_GetDeviceFWRevOutDataS;

/* set device mode */
typedef struct {
    zl303xx_DpllClockModeE dpllClockMode;
} zl303xx_SetDpllModeInDataS;

/* get device mode */
typedef struct {
    zl303xx_DpllClockModeE dpllClockMode;
    zl303xx_RefIdE dpllInputRef;
} zl303xx_GetDpllModeInDataS;



/* Usually in.d.* - Inbound data from the CLIENT driver Message Router's POV */
typedef struct {
    zl303xx_DpllDriverMsgTypesE dpllMsgType;
    union {
        zl303xx_SetFreqInDataS setFreq;
        zl303xx_GetFreqInDataS getFreq;
        zl303xx_SetTimeInDataS setTime;
        zl303xx_StepTimeInDataS stepTime;
        zl303xx_JumpActiveCGUInDataS jumpActiveCGU;
        zl303xx_DeviceInitInDataS deviceInit;
        zl303xx_SetCurrRefInDataS setCurrRef;
        zl303xx_InputPhaseErrorWriteInDataS inputPhaseErrorWrite;
        zl303xx_SetTODDoneFuncInDataS setTODDoneFunc;
        zl303xx_ConfirmHwCntrlInDataS confirmHwCntrl;
        zl303xx_SetAFBDIVInDataS setAFBDIV;
        zl303xx_AdjustTimeInDataS adjustTime;
        zl303xx_AdjustTimeDCOReadableInDataS adjustTimeDCOReadable;
        zl303xx_SetActiveElecActionsInitialInDataS setActiveElecActionsInitial;
        zl303xx_SetActiveElecActionsStage1InDataS setActiveElecActionsStage1;
        zl303xx_SetActiveElecActionsStage2InDataS setActiveElecActionsStage2;
        zl303xx_SetNCOAssistEnableInDataS NCOAssistEnable;
        zl303xx_GetNCOAssistPairFreqOffsetInDataS getNCOAssistPairFreqOffset;
        zl303xx_GetModifiedStepTimeValueInDataS getModifiedStepTimeValue;
        zl303xx_SetSWHybridTransientStatusInDataS setSWHybridTransientStatus;
        zl303xxdpllIdS dpll;
        zl303xx_SendModeHoldoverFreqOffsetInDataS sendModeHoldoverFreqOffset;
        zl303xx_SetDpllModeInDataS setDpllClockMode;
    } d;
} zl303xx_DriverMsgInDataS;

/* Usually out.d.* - Outbound data from the CLIENT driver Message Router's POV */
typedef struct {
    zl303xx_DpllDriverMsgTypesE dpllMsgType;
    union {
        zl303xx_GetFreqOutDataS getFreq;
        zl303xx_GetHWLockStatusOutDataS getHWLockStatus;
        zl303xx_GetHWManualFreerunOutDataS getHWManualFreerun;
        zl303xx_GetHWManualHoldoverOutDataS getHWManualHoldover;
        zl303xx_GetHWSyncInputEnOutDataS getHWSyncInputEn;
        zl303xx_GetHWOutOfRangeOutDataS getHWOutOfRange;
        zl303xx_GetCurrRefOutDataS getCurrRef;
        zl303xx_InputPhaseErrorWriteCtrlReadyOutDataS inputPhaseErrorWriteCtrlReady;
        zl303xx_ConfirmHwCntrlOutDataS confirmHwCntrl;
        zl303xx_GetDeviceInfoOutDataS getDeviceInfo;
        zl303xx_GetStepTimeCurrMaxStepSizeOutDataS getStepTimeCurrMaxStepSize;
        zl303xx_DetermineMaxStepSizePerAdjustmentOutDataS determineMaxStepSizePerAdjustment;
        zl303xx_AdjustTimeDCOReadableOutDataS adjustTimeDCOReadable;
        zl303xx_GetNCOAssistEnableOutDataS NCOAssistEnable;
        zl303xx_GetNCOAssistParamsSOutDataS getNCOAssistParams;
        zl303xx_GetNCOAssistPairFreqOffsetOutDataS getNCOAssistPairFreqOffset;
        zl303xx_GetNCOAssistPairHWLockStatusOutDataS getNCOAssistPairHWLockStatus;
        zl303xx_GetNCOAssistSyncInputEnOutDataS getNCOAssistSyncInputEn;
        zl303xx_GetNCOAssistOutOfRangeOutDataS getNCOAssistOutOfRange;
        zl303xx_GetNCOAssistManualHoldoverOutDataS getNCOAssistManualHoldover;
        zl303xx_GetNCOAssistManualFreerunOutDataS getNCOAssistManualFreerun;
        zl303xx_GetModifiedStepTimeValueOutDataS getModifiedStepTimeValue;
        zl303xx_GetIsOutputTypeThatDrivesTimeStamperHP isOutputTypeThatDrivesTimeStamperHP;
        zl303xx_GetStepTimeActive getStepTimeActive;
        zl303xx_GetHwHoldoverReadyS getHwHoldoverReady;
        zl303xx_GetMonitorStatusBitsOutDataS getMonitorStatusBits;
        zl303xx_GetPhaseStepResolutionsFreqsOutS getPhaseStepResolutionsFreqs;
        zl303xx_GetClockOutputsFreqOutS getClockOutputsFreq;
        zl303xx_GetSWHybridTransientStatusOutDataS getSWHybridTransientStatus;
        zl303xx_GetDeviceFWRevOutDataS getDeviceFWRev;
        zl303xx_GetDpllModeInDataS getDpllClockMode;
    } d;
} zl303xx_DriverMsgOutDataS;

/*****************   EXPORTED GLOBAL VARIABLE DECLARATIONS   ******************/

/*****************   EXTERNAL FUNCTION DECLARATIONS   *************************/

/* Device identification routines */
zl303xx_DevTypeE zl303xx_GetDefaultDeviceType(void);
unsigned int zl303xx_SetDefaultDeviceType(zl303xx_DevTypeE f);



#if defined __cplusplus
}
#endif

#endif /* MULTIPLE INCLUDE BARRIER */
