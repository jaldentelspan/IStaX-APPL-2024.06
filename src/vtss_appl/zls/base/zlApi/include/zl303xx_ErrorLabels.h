

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
*     Error labels
*
*******************************************************************************/

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

   /* MATCH THIS LIST WITH zl303xx_Error.h */


/* !!!!!!!!!!!!!!  Special cases must ALWAYS BE LAST  !!!!!!!!!!!!!!! */
