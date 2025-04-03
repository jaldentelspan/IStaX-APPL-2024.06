

/*******************************************************************************
*
*  $Id: 6517a3b4707c071ac26ad8fcef236bd9d8d99889
*  Copyright (c) 2006-2022 Microchip Technology Inc. and its subsidiaries, all rights reserved.
*  Subject to the terms of the license that accompanies the software and controls as it relates to the software and any conflicting terms herein, you may use this Microchip software and any derivatives exclusively with Microchip products.
*  You are responsible for complying with third party license terms applicable to your use of third party software (including open source software) that may accompany this Microchip software.
*  SOFTWARE IS 'AS IS'. NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
*  IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.
*  TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS RELATED TO THE SOFTWARE WILL NOT EXCEED AMOUNT OF FEES, IF ANY, YOU PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*
*  Module Description:
*     Error codes
*
*******************************************************************************/

#ifndef _ZL_ERROR_H_
#define _ZL_ERROR_H_

#ifdef __cplusplus
extern "C" {
#endif


#ifndef ZL303XX_ERROR_BASE_NUM
   #define ZL303XX_ERROR_BASE_NUM           2000
#endif

#ifndef ZL303XX_GENERIC_ERROR_BASE_NUM
   #define ZL303XX_GENERIC_ERROR_BASE_NUM   (ZL303XX_ERROR_BASE_NUM + 900)
#endif


/* Error codes used by the API */
typedef enum
{
#ifndef SOCPG_PORTING			/* SoC lots of warnings generated unless we do this */
   #include "zl303xx_ErrorLabels.h"
#else
	   /* Successful completion */
	   ZL303XX_OK = 0,

	   /* Generic error for zl303xx device*/
	   ZL303XX_ERROR = ZL303XX_ERROR_BASE_NUM,

	   ZL303XX_INVALID_MODE,

	   /* general return value for enum out of range  */
	   ZL303XX_PARAMETER_INVALID,

	   ZL303XX_INVALID_POINTER,
	   ZL303XX_NOT_RUNNING,
	   ZL303XX_INTERRUPT_NOT_RUNNING,
	   ZL303XX_MULTIPLE_INIT_ATTEMPT,

	   ZL303XX_TABLE_FULL,
	   ZL303XX_TABLE_EMPTY,
	   ZL303XX_TABLE_ENTRY_DUPLICATE,
	   ZL303XX_TABLE_ENTRY_NOT_FOUND,     /* 10 */

	   ZL303XX_DATA_CORRUPTION,

	   ZL303XX_INIT_ERROR,
	   ZL303XX_HARDWARE_ERROR,   /* An internal hardware error */

	   ZL303XX_IO_ERROR,
	   ZL303XX_TIMEOUT,

	   /* Timestamp Engine Specific Errors */
	   ZL303XX_TSENG_TS_OUT_OF_RANGE,

	   /* RTOS errors */
	   ZL303XX_RTOS_MEMORY_FAIL,

	   /* RTOS semaphore errors */
	   ZL303XX_RTOS_SEM_CREATE_FAIL,
	   ZL303XX_RTOS_SEM_DELETE_FAIL,
	   ZL303XX_RTOS_SEM_INVALID,          /* 20 */
	   ZL303XX_RTOS_SEM_TAKE_FAIL,
	   ZL303XX_RTOS_SEM_GIVE_FAIL,

	   /* RTOS mutex errors */
	   ZL303XX_RTOS_MUTEX_CREATE_FAIL,
	   ZL303XX_RTOS_MUTEX_DELETE_FAIL,
	   ZL303XX_RTOS_MUTEX_INVALID,
	   ZL303XX_RTOS_MUTEX_TAKE_FAIL,
	   ZL303XX_RTOS_MUTEX_GIVE_FAIL,

	    /* RTOS message queue errors */
	   ZL303XX_RTOS_MSGQ_CREATE_FAIL,
	   ZL303XX_RTOS_MSGQ_DELETE_FAIL,
	   ZL303XX_RTOS_MSGQ_INVALID,         /* 30 */
	   ZL303XX_RTOS_MSGQ_SEND_FAIL,
	   ZL303XX_RTOS_MSGQ_RECEIVE_FAIL,

	  /* RTOS task errors */
	   ZL303XX_RTOS_TASK_CREATE_FAIL,
	   ZL303XX_RTOS_TASK_DELETE_FAIL,

	   /* Transport layer errors */
	   ZL303XX_TRANSPORT_LAYER_ERROR,

	   /* Protocol errors */
	   ZL303XX_PROTOCOL_ENGINE_ERROR,

	   ZL303XX_STATISTICS_NOT_ENABLED,
	   ZL303XX_STREAM_NOT_IN_USE,

	   ZL303XX_CLK_SWITCH_ERROR,

	   ZL303XX_EXT_API_CALL_FAIL,         /* 40 */

	   ZL303XX_INVALID_OPERATION,
	   ZL303XX_UNSUPPORTED_OPERATION,

       ZL303XX_UNSUPPORTED_MSG_ROUTER_OPERATION,

       /* We are blocking the API operation due to ongoing PF Adjustment (SetTime, StepTime, AdjTime) */
       ZL303XX_BLOCKED_DUE_TO_CURRENT_ADJ_INPROGRESS,

    /* MATCH THIS LIST WITH zl303xx_ErrorLabels.h */


	/* !!!!!!!!!!!!!!  Special cases must ALWAYS BE LAST  !!!!!!!!!!!!!!! */
#endif
#ifndef SOCPG_PORTING	/* SoC lots of warnings generated unless we do this */
   #include "zl303xx_ErrorLabelsGeneric.h"
#else
	   /* Start of Generic Error Labels */
	   ZL303XX_ERROR_0 = ZL303XX_GENERIC_ERROR_BASE_NUM,
	   ZL303XX_ERROR_1,          /*   */
	   ZL303XX_ERROR_2,          /*   */
	   ZL303XX_ERROR_3,          /*   */
	   ZL303XX_ERROR_4,          /*   */
	   ZL303XX_ERROR_5,          /*   */
	   ZL303XX_ERROR_6,          /*   */
	   ZL303XX_ERROR_7,          /*   */
	   ZL303XX_ERROR_8,          /*   */
	   ZL303XX_ERROR_9,          /*   */
	   ZL303XX_ERROR_10,      /*   */
	   ZL303XX_ERROR_11,         /*   */
	   ZL303XX_ERROR_12,         /*   */
	   ZL303XX_ERROR_13,         /*   */
	   ZL303XX_ERROR_14,         /*   */
	   ZL303XX_ERROR_15,         /*   */
	   ZL303XX_ERROR_16,         /*   */
	   ZL303XX_ERROR_17,         /*   */
	   ZL303XX_ERROR_18,         /*   */
	   ZL303XX_ERROR_19,         /*   */
	   ZL303XX_ERROR_20,      /*   */
	   ZL303XX_ERROR_21,         /*   */
	   ZL303XX_ERROR_22,         /*   */
	   ZL303XX_ERROR_23,         /*   */
	   ZL303XX_ERROR_24,         /*   */
	   ZL303XX_ERROR_25,         /*   */
	   ZL303XX_ERROR_26,         /*   */
	   ZL303XX_ERROR_27,         /*   */
	   ZL303XX_ERROR_28,         /*   */
	   ZL303XX_ERROR_29,         /*   */
	   ZL303XX_ERROR_30,      /*   */
	   ZL303XX_ERROR_31,         /*   */
	   ZL303XX_ERROR_32,         /*   */
	   ZL303XX_ERROR_33,         /*   */
	   ZL303XX_ERROR_34,         /*   */
	   ZL303XX_ERROR_35,         /*   */
	   ZL303XX_ERROR_36,         /*   */
	   ZL303XX_ERROR_37,         /*   */
	   ZL303XX_ERROR_38,         /*   */
	   ZL303XX_ERROR_39,         /*   */
	   ZL303XX_ERROR_40,      /*   */
	   ZL303XX_ERROR_41,         /*   */
	   ZL303XX_ERROR_42,         /*   */
	   ZL303XX_ERROR_43,         /*   */
	   ZL303XX_ERROR_44,         /*   */
	   ZL303XX_ERROR_45,         /*   */
	   ZL303XX_ERROR_46,         /*   */
	   ZL303XX_ERROR_47,         /*   */
	   ZL303XX_ERROR_48,         /*   */
	   ZL303XX_ERROR_49,         /*   */
	   ZL303XX_ERROR_50,      /*   */
	   ZL303XX_ERROR_51,         /*   */
	   ZL303XX_ERROR_52,         /*   */
	   ZL303XX_ERROR_53,         /*   */
	   ZL303XX_ERROR_54,         /*   */
	   ZL303XX_ERROR_55,         /*   */
	   ZL303XX_ERROR_56,         /*   */
	   ZL303XX_ERROR_57,         /*   */
	   ZL303XX_ERROR_58,         /*   */
	   ZL303XX_ERROR_59,         /*   */
	   ZL303XX_ERROR_60,      /*   */
	   ZL303XX_ERROR_61,         /*   */
	   ZL303XX_ERROR_62,         /*   */
	   ZL303XX_ERROR_63,         /*   */
	   ZL303XX_ERROR_64,         /*   */
	   ZL303XX_ERROR_65,         /*   */
	   ZL303XX_ERROR_66,         /*   */
	   ZL303XX_ERROR_67,         /*   */
	   ZL303XX_ERROR_68,         /*   */
	   ZL303XX_ERROR_69,         /*   */
	   ZL303XX_ERROR_70,      /*   */
	   ZL303XX_ERROR_71,         /*   */
	   ZL303XX_ERROR_72,         /*   */
	   ZL303XX_ERROR_73,         /*   */
	   ZL303XX_ERROR_74,         /*   */
	   ZL303XX_ERROR_75,         /*   */
	   ZL303XX_ERROR_76,         /*   */
	   ZL303XX_ERROR_77,         /*   */
	   ZL303XX_ERROR_78,         /*   */
	   ZL303XX_ERROR_79,         /*   */
	   ZL303XX_ERROR_80,      /*   */
	   ZL303XX_ERROR_81,         /*   */
	   ZL303XX_ERROR_82,         /*   */
	   ZL303XX_ERROR_83,         /*   */
	   ZL303XX_ERROR_84,         /*   */
	   ZL303XX_ERROR_85,         /*   */
	   ZL303XX_ERROR_86,         /*   */
	   ZL303XX_ERROR_87,         /*   */
	   ZL303XX_ERROR_88,         /*   */
	   ZL303XX_ERROR_89,         /*   */
	   ZL303XX_ERROR_90,      /*   */
	   ZL303XX_ERROR_91,         /*   */
	   ZL303XX_ERROR_92,         /*   */
	   ZL303XX_ERROR_93,         /*   */
	   ZL303XX_ERROR_94,         /*   */
	   ZL303XX_ERROR_95,         /*   */
	   ZL303XX_ERROR_96,         /*   */
	   ZL303XX_ERROR_97,         /*   */
	   ZL303XX_ERROR_98,         /*   */
	   ZL303XX_ERROR_99,         /*   */
#endif

   ZL303XX_ERROR_CODE_END

} zlStatusE;


#ifdef __cplusplus
}
#endif

#endif
