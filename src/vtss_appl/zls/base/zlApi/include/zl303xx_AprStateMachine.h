



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

#ifndef ZL303XX_APR_SM_H_
#define ZL303XX_APR_SM_H_

#include "zl303xx_Global.h"
#include "zl303xx_DataTypes.h"
#include "zl303xx_DataTypesEx.h"
#include "zl303xx_Error.h"

#define L2_MASK(x)      ((x)&0x1)
#define L3_MASK(x)      (((x)>>1)&0x1)
#define L4_MASK(x)      (((x)>>2)&0x1)



typedef struct
{
   /*!  Var: zl303xx_AprAddServerS::lockFlagsMask
            Mask for the flag L2 and L3.
            0 - unmask both L2, L3 and L4 flag (use both of them);
            1 - mask L2 flag (not use L2);
            2 - mask L3 flag (not use L3);
            3 - mask both L2 and L3 flags (not use both of them)
            4 - mask L4 flag (not use L4);
            5 - mask both L2 and L4 flags (not use both of them)
            6 - mask both L3 and L4 flags (not use both of them)
            7 - mask L2, L3 and L4 flags (not use any of them)
            */

   Uint8T lockFlagsMask;

   /*!  Var: zl303xx_AprServerFlagESTInfo::enterHoldoverGST
         The guard soak timer for entering holdover state. Unit: ten second */
   Uint32T enterHoldoverGST;
   Uint32T enterHoldoverGSTCnt;

   /*!  Var: zl303xx_AprServerFlagESTInfo::exitVFlagGST
         The guard soak timer for V flag from 1 to 0. Unit: ten second */
   Uint32T exitVFlagGST;
   Uint32T exitVFlagGSTCnt;

   /*!  Var: zl303xx_AprServerFlagESTInfo::exitLFlagGST
         The guard soak timer for for L flag from 1 to 0. Unit: ten second */
   Uint32T exitLFlagGST;
   Uint32T exitLFlagGSTCnt;

   /*!  Var: zl303xx_AprServerFlagESTInfo::exitPAFlagGST
         The guard soak timer for for PA flag from 1 to 0. Unit: ten second */
   Uint32T exitPAFlagGST;
   Uint32T exitPAFlagGSTCnt;

   zl303xx_BooleanE loggingThisServer;

}zl303xx_AprServerSMDataS;

/** zl303xx_AprGetServerSMData

   Get the server state machine related data for the specified server.

  Parameters:
   [in]    hwParams                  Pointer to the device structure
   [in]    serverId                  The identifier of server to be queried

   [out]   statusFlags               pointer to the server state machine info structure

  Return Value: ZL303XX_OK                        if successful
****************************************************************************/
zlStatusE zl303xx_AprGetServerSMData (void *hwParams, Uint16T serverId, zl303xx_AprServerSMDataS **smData);

/** zl303xx_AprProcess_L_Flag

   This function process the L1, L2 and L3 flags by applying the masks of L2 and L3
   to create the L flag for the specified server.
   The function is called every 9600ms.

  Parameters:
   [in]    hwParams                  Pointer to the device structure
   [in]    serverId                  The identifier of server

   [out]   L                        pointer to the L flag

  Return Value: ZL303XX_OK                        if successful
****************************************************************************/
zlStatusE zl303xx_AprProcess_L_Flag
            (
            void *hwParams,
            Uint16T serverId,
            zl303xx_BooleanE *L
            );

/** zl303xx_AprApplyGSTon_H_Flag

   This function process H flag by applying the corresponding Guard Soak Time
   (GST) if the H flag changes from 0 to 1 (enter to Holdover) for the specified server.
   The function is called every 9600ms.

  Parameters:
   [in]    hwParams                  Pointer to the device structure
   [in]    serverId                  The identifier of server

   [out]   gstH                      pointer to the H flag after GST applied

  Return Value: ZL303XX_OK                        if successful
****************************************************************************/
zlStatusE zl303xx_AprApplyGSTon_H_Flag
            (
            void *hwParams,
            Uint16T serverId,
            zl303xx_BooleanE *gstH
            );

/** zl303xx_AprApplyGSTon_VLPA_Flags

   This function process V/L/PA flag by applying the corresponding Guard Soak Timer
   (GST) on it if the flag changes from 1 to 0 (exit from Valid/Lock/PhaseAligned)
   for the specified server.
   The function is called every 9600ms.

  Parameters:
   [in]    hwParams                  Pointer to the device structure
   [in]    serverId                  The identifier of server
   [in]    whichFlag                 The identifier of flag to be applied

   [out]   flag                      Pointer to the flag after its GST applied

  Return Value: ZL303XX_OK                        if successful
****************************************************************************/
zlStatusE zl303xx_AprApplyGSTon_VLPA_Flags
            (
            void *hwParams,
            Uint16T serverId,
            zl303xx_AprServerStatusFlagsE whichFlag,
            zl303xx_BooleanE *flag
            );

/** zl303xx_AprSetServerSMFlag

   Set a state machine flag for the specified server. Only the following server
   state machine flags can be set via this function:
   SERVER_CLK_L_FLAG,
   SERVER_CLK_GST_L_FLAG,
   SERVER_CLK_GST_H_FLAG,
   SERVER_CLK_GST_V_FLAG,
   SERVER_CLK_GST_PA_FLAG,
   SERVER_CLK_STATE

  Parameters:
   [in]    hwParams                  Pointer to the device structure
   [in]    serverId                  The identifier of server
   [in]    whichFlag                 Indicate which flag to be set
   [in]    flagVal                   The pointer of flag new value

  Return Value: ZL303XX_OK                        if successful
****************************************************************************/
zlStatusE zl303xx_AprSetServerSMFlag
            (
            void *hwParams,
            Uint16T serverId,
            zl303xx_AprServerStatusFlagsE whichFlag,
            void *flagVal
            );

/** zl303xx_AprSetElecRefSMState

   Set a state for the specified electrical reference state machine if the CGU device is in
   electrical/hybrid mode.

  Parameters:
   [in]    hwParams                  Pointer to the device structure

  Return Value: ZL303XX_OK                        if successful
****************************************************************************/
zlStatusE zl303xx_AprSetElecRefSMState
            (
            void *hwParams,
            zl303xx_AprStateE elecRefState
            );

/** zl303xx_AprSetCGUSMState

   Set a state of the state machine for the specified CGU device.

  Parameters:
   [in]    hwParams                  Pointer to the device structure
   [in]    cguState                  The state to be set

  Return Value: ZL303XX_OK                        if successful
****************************************************************************/
zlStatusE zl303xx_AprSetCGUSMState (void *hwParams, zl303xx_AprStateE cguState);



/** zl303xx_AprPktServerSM

   This function performs the server state machine operations to update the state
   for the specified packet server.
   The function is called every 9600ms.

  Parameters:
   [in]    hwParams                  Pointer to the device structure
   [in]    serverId                  The identifier of server

   [out]   state                     Pointer to the state value

  Return Value: ZL303XX_OK                        if successful
****************************************************************************/
zlStatusE zl303xx_AprPktServerSM
            (
            void *hwParams,
            Uint16T serverId,
            zl303xx_AprStateE *state
            );


/** zl303xx_AprElecRefSM

   This function performs the server state machine operations to update the state
   for the current active electrical reference if the device is in electrical or
   hybrid mode.
   The function is called every 9600ms.

  Parameters:
   [in]    hwParams                  Pointer to the device structure
   [out]   state                     Pointer to the state value

  Return Value: ZL303XX_OK                        if successful
****************************************************************************/
zlStatusE zl303xx_AprElecRefSM
            (
            void *hwParams,
            zl303xx_AprStateE *state
            );


/** zl303xx_AprCGUStateMachine

   This function performs the CGU state machine operations to update the state
   for the specified CGU device.
   The function is called every 9600ms.

  Parameters:
   [in]    hwParams                  Pointer to the device structure
   [out]   state                     Pointer to the state value

  Return Value: ZL303XX_OK                        if successful
****************************************************************************/
zlStatusE zl303xx_AprCGUStateMachine
            (
            void *hwParams,
            zl303xx_AprStateE *state
            );



#endif
