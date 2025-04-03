/*
 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

 Unpublished rights reserved under the copyright laws of the United States of
 America, other countries and international treaties. Permission to use, copy,
 store and modify, the software and its source code is granted but only in
 connection with products utilizing the Microsemi switch and PHY products.
 Permission is also granted for you to integrate into other products, disclose,
 transmit and distribute the software only in an absolute machine readable
 format (e.g. HEX file) and only in or with products utilizing the Microsemi
 switch and PHY products.  The source code of the software may not be
 disclosed, transmitted or distributed without the prior written permission of
 Microsemi.

 This copyright notice must appear in any copy, modification, disclosure,
 transmission or distribution of the software.  Microsemi retains all
 ownership, copyright, trade secret and proprietary rights in the software and
 its source code, including all modifications thereto.

 THIS SOFTWARE HAS BEEN PROVIDED "AS IS". MICROSEMI HEREBY DISCLAIMS ALL
 WARRANTIES OF ANY KIND WITH RESPECT TO THE SOFTWARE, WHETHER SUCH WARRANTIES
 ARE EXPRESS, IMPLIED, STATUTORY OR OTHERWISE INCLUDING, WITHOUT LIMITATION,
 WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR USE OR PURPOSE AND
 NON-INFRINGEMENT.
*/

/*
 Microchip is aware that some terminology used in this technical document is
 antiquated and inappropriate. As a result of the complex nature of software
 where seemingly simple changes have unpredictable, and often far-reaching
 negative results on the software's functionality (requiring extensive retesting
 and revalidation) we are unable to make the desired changes in all legacy
 systems without compromising our product or our clients' products.
*/

#include "zl_3073x_synce_support.h"
#include "zl303xx_AddressMap73x.h"
#include "zl303xx_DeviceSpec.h"
#include "zl303xx_Dpll73xGlobal.h"
#include "zl_dpll_low_level_support.h"
#include "zl303xx_Init.h"
#include "zl303xx_Macros.h"
#include "zl303xx_Trace.h"
#include "zl303xx_ErrTrace.h"
#include "zl303xx_Int64.h"


/**
   Sets the base frequency of a reference input.

  Parameters:
   [in]   zl303xx_Params  Device instance parameter structure.
   [in]   refId     Input reference number.
   [in]   freqHz    Base frequency in Hz.

  Return Value:  ZL303XX_OK  Success.

*******************************************************************************/

zlStatusE zl303xx_Ref73xBaseFreqSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_RefIdE refId, Uint32T freqHz)
{
    zlStatusE status = ZL303XX_OK;
    zlStatusE multistepMutexStatus = ZL303XX_ERROR;

    status = ZL303XX_CHECK_POINTER(zl303xx_Params);
    if (status == ZL303XX_OK)
    {
        status = ZLS3073X_CHECK_REF_ID(refId);
    }

    if (status == ZL303XX_OK)
    {
        status = ((freqHz <= 0xFFFFU) && (1600000000UL % freqHz == 0)) ? ZL303XX_OK : ZL303XX_PARAMETER_INVALID;
    }

    if (status == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesTake(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetupMailboxForWrite(zl303xx_Params, ZLS3073X_MB_ref, refId);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Write(zl303xx_Params, NULL,
                            ZL303XX_MAKE_MEM_ADDR_73X(ZLS3073X_REFX_FREQ_BASE_REG,ZL303XX_MEM_SIZE_2_BYTE),
                            freqHz);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetMailboxSemWrite(zl303xx_Params, ZLS3073X_MB_ref);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xUpdateMailboxCopy(zl303xx_Params, ZLS3073X_MB_ref, refId);
    }
    if (multistepMutexStatus == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesGive(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }

    return status;
}


/**
   Sets the frequency multiple of a reference input.

  Parameters:
   [in]   zl303xx_Params  Device instance parameter structure.
   [in]   refId     Input reference number.
   [in]   multiple  Multiple

  Return Value:  ZL303XX_OK  Success.

*******************************************************************************/

zlStatusE zl303xx_Ref73xFreqMultipleSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_RefIdE refId, Uint32T multiple)
{
    zlStatusE status = ZL303XX_OK;
    zlStatusE multistepMutexStatus = ZL303XX_ERROR;

    status = ZL303XX_CHECK_POINTER(zl303xx_Params);
    if (status == ZL303XX_OK)
    {
        status = ZLS3073X_CHECK_REF_ID(refId);
    }

    if (status == ZL303XX_OK)
    {
        status = (multiple <= 0xFFFFU) ? ZL303XX_OK : ZL303XX_PARAMETER_INVALID;
    }

    if (status == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesTake(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetupMailboxForWrite(zl303xx_Params, ZLS3073X_MB_ref, refId);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Write(zl303xx_Params, NULL,
                            ZL303XX_MAKE_MEM_ADDR_73X(ZLS3073X_REFX_FREQ_MULT_REG,ZL303XX_MEM_SIZE_2_BYTE),
                            multiple);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetMailboxSemWrite(zl303xx_Params, ZLS3073X_MB_ref);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xUpdateMailboxCopy(zl303xx_Params, ZLS3073X_MB_ref, refId);
    }
    if (multistepMutexStatus == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesGive(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }
    return status;
}


/**
   Sets the M value i.e. the numerator of the M/N ratio of a reference input.

  Parameters:
   [in]   zl303xx_Params  Device instance parameter structure.
   [in]   refId     Input reference number.
   [in]   M         M value

  Return Value:  ZL303XX_OK  Success.

*******************************************************************************/

zlStatusE zl303xx_Ref73xRatioMRegSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_RefIdE refId, Uint32T M)
{
    zlStatusE status = ZL303XX_OK;
    zlStatusE multistepMutexStatus = ZL303XX_ERROR;

    status = ZL303XX_CHECK_POINTER(zl303xx_Params);
    if (status == ZL303XX_OK)
    {
        status = ZLS3073X_CHECK_REF_ID(refId);
    }

    if (status == ZL303XX_OK)
    {
        status = (M <= 0xFFFFU) ? ZL303XX_OK : ZL303XX_PARAMETER_INVALID;
    }

    if (status == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesTake(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetupMailboxForWrite(zl303xx_Params, ZLS3073X_MB_ref, refId);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Write(zl303xx_Params, NULL,
                ZL303XX_MAKE_MEM_ADDR_73X(ZLS3073X_REFX_RATIO_M_REG,ZL303XX_MEM_SIZE_2_BYTE),
                M);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetMailboxSemWrite(zl303xx_Params, ZLS3073X_MB_ref);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xUpdateMailboxCopy(zl303xx_Params, ZLS3073X_MB_ref, refId);
    }
    if (multistepMutexStatus == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesGive(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }

    return status;
}


/**
   Sets the N value i.e. the denominator of the M/N ratio of a reference input.

  Parameters:
   [in]   zl303xx_Params  Device instance parameter structure.
   [in]   refId     Input reference number.
   [in]   N         N value

  Return Value:  ZL303XX_OK  Success.

*******************************************************************************/

zlStatusE zl303xx_Ref73xRatioNRegSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_RefIdE refId, Uint32T N)
{
    zlStatusE status = ZL303XX_OK;
    zlStatusE multistepMutexStatus = ZL303XX_ERROR;

    status = ZL303XX_CHECK_POINTER(zl303xx_Params);
    if (status == ZL303XX_OK)
    {
        status = ZLS3073X_CHECK_REF_ID(refId);
    }

    if (status == ZL303XX_OK)
    {
        status = ((N >= 1) && (N <= 0xFFFFU)) ? ZL303XX_OK : ZL303XX_PARAMETER_INVALID;
    }

    if (status == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesTake(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetupMailboxForWrite(zl303xx_Params, ZLS3073X_MB_ref, refId);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Write(zl303xx_Params, NULL,
                ZL303XX_MAKE_MEM_ADDR_73X(ZLS3073X_REFX_RATIO_N_REG,ZL303XX_MEM_SIZE_2_BYTE),
                N);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetMailboxSemWrite(zl303xx_Params, ZLS3073X_MB_ref);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xUpdateMailboxCopy(zl303xx_Params, ZLS3073X_MB_ref, refId);
    }
    if (multistepMutexStatus == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesGive(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }


    return status;
}

/**
   Function Name:
    zl303xx_DpllRefPrioritySet

   Details:
    Writes the Ref Priority attribute

   Parameters:
    [in]    zl303xx_Params Pointer to the device instance parameter structure
    [in]    dpllId         Associated DPLL Id of the attribute
    [in]    refId          Associated REF Id of the attribute
    [in]    val            The value of the device attribute parameter

   Return Value:
    zlStatusE

*******************************************************************************/

zlStatusE zl3073x_DpllRefPrioritySet(zl303xx_ParamsS *zl303xx_Params, zl303xx_DpllIdE dpllId, zl303xx_RefIdE refId, Uint32T val)
{
    zlStatusE status = ZL303XX_OK;
    zlStatusE multistepMutexStatus = ZL303XX_ERROR;
    Uint32T regVal = 0;

    /* Check params pointer */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_POINTER(zl303xx_Params);
    }

    /* Check the dpllId parameter */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_DPLL_ID(dpllId);
    }

    /* Check the refId parameter */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_REF_ID(refId);
    }

    /* Check that the write value is within range */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_REF_PRIORITY(val);
    }

    if (status == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesTake(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetupMailboxForRead(zl303xx_Params, ZLS3073X_MB_DPLL, dpllId);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Read(zl303xx_Params, NULL, ZL303XX_DPLL_REF_PRIORITY_REG(refId), &regVal);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetupMailboxForWrite(zl303xx_Params, ZLS3073X_MB_DPLL, dpllId);
    }

    if (status == ZL303XX_OK)
    {

        ZL303XX_INSERT(regVal, val,
                ZL303XX_DPLL_REF_PRIORITY_MASK,
                ZL303XX_DPLL_REF_PRIORITY_SHIFT(refId));
        status = zl303xx_Write(zl303xx_Params, NULL, ZL303XX_DPLL_REF_PRIORITY_REG(refId), regVal);
    }

    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetMailboxSemWrite(zl303xx_Params, ZLS3073X_MB_DPLL);
    }

    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xUpdateMailboxCopy(zl303xx_Params, ZLS3073X_MB_DPLL, dpllId);
    }
    if (multistepMutexStatus == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesGive(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }

    return status;
}  /* END zl303xx_DpllRefPrioritySet */


/**
   Function Name:
    zl303xx_DpllRefPriorityGet

   Details:
    Writes the Ref Priority attribute

   Parameters:
    [in]    zl303xx_Params Pointer to the device instance parameter structure
    [in]    dpllId         Associated DPLL Id of the attribute
    [in]    refId          Associated REF Id of the attribute
    [in]    val            The value of the device attribute parameter

   Return Value:
    zlStatusE

*******************************************************************************/

zlStatusE zl3073x_DpllRefPriorityGet(zl303xx_ParamsS *zl303xx_Params, zl303xx_DpllIdE dpllId, zl303xx_RefIdE refId, Uint32T *val)
{
    zlStatusE status = ZL303XX_OK;
    zlStatusE multistepMutexStatus = ZL303XX_ERROR;
    Uint32T regValue = 0;

    /* Check params pointer */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_POINTERS(zl303xx_Params, val);
    }

    /* Check the dpllId parameter */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_DPLL_ID(dpllId);
    }

    /* Check the refId parameter */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_REF_ID(refId);
    }

    if (status == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesTake(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetupMailboxForRead(zl303xx_Params, ZLS3073X_MB_DPLL, dpllId);
    }

    if (status == ZL303XX_OK)
    {
        status = zl303xx_Read(zl303xx_Params, NULL, ZL303XX_DPLL_REF_PRIORITY_REG(refId), &regValue);
    }
    if (multistepMutexStatus == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesGive(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }

    if (status == ZL303XX_OK)
    {
      *val = ZL303XX_EXTRACT(regValue,ZL303XX_DPLL_REF_PRIORITY_MASK,
              ZL303XX_DPLL_REF_PRIORITY_SHIFT(refId));
    }

    return status;
}  /* END zl303xx_DpllRefPriorityGet */


/**

  Function Name:
   zl303xx_Ref73xCfmLimitSet

  Details:
   Writes the CFM Limit attribute

  Parameters:
   [in]    zl303xx_Params Pointer to the device instance parameter structure
   [in]    refId          Associated REF Id of the attribute
   [in]    val            The value of the device attribute parameter

  Return Value:
   zlStatusE

*******************************************************************************/

zlStatusE zl303xx_Ref73xCfmLimitSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_RefIdE refId, Uint32T val)
{
   zlStatusE status = ZL303XX_OK;
    zlStatusE multistepMutexStatus = ZL303XX_ERROR;

   /* Check params pointer */
   if (status == ZL303XX_OK)
   {
      status = ZL303XX_CHECK_POINTER(zl303xx_Params);
   }

    /* Check the refId parameter */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_REF_ID(refId);
    }

    /* Check that the write value is within range */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_REF_CFM_LIMIT(val);
    }

    if (status == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesTake(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetupMailboxForWrite(zl303xx_Params, ZLS3073X_MB_ref, refId);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Write(zl303xx_Params, NULL, ZLS3073X_REFX_CFM_REG, val);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetMailboxSemWrite(zl303xx_Params, ZLS3073X_MB_ref);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xUpdateMailboxCopy(zl303xx_Params, ZLS3073X_MB_ref, refId);
    }
    if (multistepMutexStatus == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesGive(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }

   return status;
}  /* END zl303xx_Ref73xCfmLoLimitSet */


/**

  Function Name:
   zl303xx_Ref73xScmLimitSet

  Details:
   Writes the SCM Limit attribute

  Parameters:
   [in]    zl303xx_Params Pointer to the device instance parameter structure
   [in]    refId          Associated REF Id of the attribute
   [in]    val            The value of the device attribute parameter

  Return Value:
   zlStatusE

*******************************************************************************/

zlStatusE zl303xx_Ref73xScmLimitSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_RefIdE refId, Uint32T val)
{
   zlStatusE status = ZL303XX_OK;
    zlStatusE multistepMutexStatus = ZL303XX_ERROR;
   Uint32T regVal = 0;

   /* Check params pointer */
   if (status == ZL303XX_OK)
   {
      status = ZL303XX_CHECK_POINTER(zl303xx_Params);
   }

    /* Check the refId parameter */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_REF_ID(refId);
    }

    /* Check that the write value is within range */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_REF_SCM_LIMIT(val);
    }

    if (status == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesTake(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetupMailboxForRead(zl303xx_Params, ZLS3073X_MB_ref, refId);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Read(zl303xx_Params, NULL, ZLS3073X_REFX_SCM_REG, &regVal);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetupMailboxForWrite(zl303xx_Params, ZLS3073X_MB_ref, refId);
    }
    if (status == ZL303XX_OK)
    {
        ZL303XX_INSERT(regVal, val,
                          0x7,
                          0);
        status = zl303xx_Write(zl303xx_Params, NULL, ZLS3073X_REFX_SCM_REG, regVal);
    }
    if (status == ZL303XX_OK)
    {

        status = zl303xx_Dpll73xSetMailboxSemWrite(zl303xx_Params, ZLS3073X_MB_ref);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xUpdateMailboxCopy(zl303xx_Params, ZLS3073X_MB_ref, refId);
    }
    if (multistepMutexStatus == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesGive(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }

   return status;
}  /* END zl303xx_Ref73xCfmLoLimitSet */




/**
   Function Name:
    zl303xx_Ref73xGstDisqualifyTimeSet

   Details:
    Writes the Ref Guard Soak Timer Disqalify Time attribute

   Parameters:
    [in]    zl303xx_Params Pointer to the device instance parameter structure
    [in]    refId          Associated REF Id of the attribute
    [in]    val            The value of the device attribute parameter

   Return Value:
    zlStatusE

*******************************************************************************/

zlStatusE zl303xx_Ref73xGstDisqualifyTimeSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_RefIdE refId, Uint32T val)
{
    zlStatusE status = ZL303XX_OK;
    zlStatusE multistepMutexStatus = ZL303XX_ERROR;

    /* Check params pointer */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_POINTER(zl303xx_Params);
    }

    /* Check the refId parameter */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_REF_ID(refId);
    }

    /* Check that the write value is within range */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_REF_GST_DISQUALIFY_TIME(val);
    }

    if (status == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesTake(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetupMailboxForWrite(zl303xx_Params, ZLS3073X_MB_ref, refId);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Write(zl303xx_Params, NULL, ZLS3073X_REFX_GST_DISQUAL_REG, val);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetMailboxSemWrite(zl303xx_Params, ZLS3073X_MB_ref);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xUpdateMailboxCopy(zl303xx_Params, ZLS3073X_MB_ref, refId);
    }
    if (multistepMutexStatus == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesGive(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }

    return status;
}  /* END zl303xx_Ref73xGstDisqualifyTimeSet */


/**
   Function Name:
    zl303xx_Ref73xGstQualifyTimeSet

   Details:
    Writes the Ref Guard Soak Timer Disqalify Time attribute

   Parameters:
    [in]    zl303xx_Params Pointer to the device instance parameter structure
    [in]    refId          Associated REF Id of the attribute
    [in]    val            The value of the device attribute parameter

   Return Value:
    zlStatusE

*******************************************************************************/

zlStatusE zl303xx_Ref73xGstQualifyTimeSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_RefIdE refId, Uint32T val)
{
    zlStatusE status = ZL303XX_OK;
    zlStatusE multistepMutexStatus = ZL303XX_ERROR;

    /* Check params pointer */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_POINTER(zl303xx_Params);
    }

    /* Check the refId parameter */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_REF_ID(refId);
    }

    /* Check that the write value is within range */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_REF_GST_QUALIFY_TIME(val);
    }

    if (status == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesTake(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetupMailboxForWrite(zl303xx_Params, ZLS3073X_MB_ref, refId);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Write(zl303xx_Params, NULL, ZLS3073X_REFX_GST_QUAL_REG, val);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetMailboxSemWrite(zl303xx_Params, ZLS3073X_MB_ref);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xUpdateMailboxCopy(zl303xx_Params, ZLS3073X_MB_ref, refId);
    }
    if (multistepMutexStatus == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesGive(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }

    return status;
}  /* END zl303xx_Ref73xGstQualifyTimeSet */


/**
   Function Name:
    zl303xx_Ref73xPfmLimitSet

   Details:
    Writes the Ref PFM limit attribute

   Parameters:
    [in]    zl303xx_Params Pointer to the device instance parameter structure
    [in]    refId          Associated REF Id of the attribute
    [in]    val            The value of the device attribute parameter

   Return Value:
    zlStatusE

*******************************************************************************/

zlStatusE zl303xx_Ref73xPfmLimitSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_RefIdE refId, Uint32T val)
{
    zlStatusE status = ZL303XX_OK;
    zlStatusE multistepMutexStatus = ZL303XX_ERROR;

    /* Check params pointer */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_POINTER(zl303xx_Params);
    }

    /* Check the refId parameter */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_REF_ID(refId);
    }

    /* Check that the write value is within range */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_REF_PFM_LIMIT(val);
    }

    if (status == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesTake(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetupMailboxForWrite(zl303xx_Params, ZLS3073X_MB_ref, refId);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Write(zl303xx_Params, NULL, ZLS3073X_REFX_PFM_FILTER_LIMIT_REG, val);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetMailboxSemWrite(zl303xx_Params, ZLS3073X_MB_ref);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xUpdateMailboxCopy(zl303xx_Params, ZLS3073X_MB_ref, refId);
    }
    if (multistepMutexStatus == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesGive(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }

    return status;
}  /* END zl303xx_Ref73xPfmLimitSet */



/**
   Function Name:
    zl303xx_Ref73xDpllPhaseSlopeLimitSet

   Details:
    Writes the DPLL Phase Slope Limit attribute

   Parameters:
    [in]    zl303xx_Params Pointer to the device instance parameter structure
    [in]    dpllId         Associated DPLL Id of the attribute
    [in]    val            The value of the device attribute parameter

   Return Value:
    zlStatusE

*******************************************************************************/

zlStatusE zl303xx_Ref73xDpllPhaseSlopeLimitSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_DpllIdE dpllId, Uint32T val)
{
    zlStatusE status = ZL303XX_OK;
    zlStatusE multistepMutexStatus = ZL303XX_ERROR;

    /* Check params pointer */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_POINTER(zl303xx_Params);
    }

    /* Check the dpllId parameter */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_DPLL_ID(dpllId);
    }

    /* Check that the write value is within range */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_DPLL_PHASE_SLOPE_LIMIT(val);
    }

    if (status == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesTake(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetupMailboxForWrite(zl303xx_Params, ZLS3073X_MB_DPLL, dpllId);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Write(zl303xx_Params, NULL, ZLS3073X_DPLLX_PSL_REG, val);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetMailboxSemWrite(zl303xx_Params, ZLS3073X_MB_DPLL);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xUpdateMailboxCopy(zl303xx_Params, ZLS3073X_MB_DPLL, dpllId);
    }
    if (status != ZL303XX_OK) {
        Uint32T reg_val;
        if (zl303xx_Read(zl303xx_Params, NULL, (ZL303XX_MAKE_MEM_ADDR_73X(0x13, ZL303XX_MEM_SIZE_2_BYTE)), &reg_val) == ZL303XX_OK) {
            printf(" %s address 0x13 value : %d\n", __FUNCTION__, reg_val);
        } else {
            printf(" %s error reading address 0x13\n", __FUNCTION__);
        }
        if (zl303xx_Read(zl303xx_Params, NULL, (ZL303XX_MAKE_MEM_ADDR_73X(0x15, ZL303XX_MEM_SIZE_1_BYTE)), &reg_val) == ZL303XX_OK) {
            printf(" %s address 0x15 value : %d\n", __FUNCTION__, reg_val);
        } else {
            printf(" %s error reading address 0x15\n", __FUNCTION__);
        }
    }
    if (multistepMutexStatus == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesGive(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }

    return status;
}  /* END zl303xx_Ref73xDpllPhaseSlopeLimitSet */




/**
   Function Name:
    zl303xx_DpllHoldoverDelayStorageSet

   Details:
    Writes the DPLL Holdover Delay Storage attribute

   Parameters:
    [in]    zl303xx_Params Pointer to the device instance parameter structure
    [in]    dpllId         Associated DPLL Id of the attribute
    [in]    val            The value of the device attribute parameter

   Return Value:
    zlStatusE

*******************************************************************************/

zlStatusE zl3073x_DpllHoldoverDelayStorageSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_DpllIdE dpllId, Uint32T val)
{
    zlStatusE status = ZL303XX_OK;
    zlStatusE multistepMutexStatus = ZL303XX_ERROR;

    /* Check params pointer */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_POINTER(zl303xx_Params);
    }

    /* Check the dpllId parameter */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_DPLL_ID(dpllId);
    }

    /* Check that the write value is within range */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_DPLL_HOLDOVER_DELAY_STORAGE(val);
    }

    if (status == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesTake(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetupMailboxForWrite(zl303xx_Params, ZLS3073X_MB_DPLL, dpllId);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Write(zl303xx_Params, NULL, ZLS3073X_DPLLX_HO_DELAY_REG, val);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetMailboxSemWrite(zl303xx_Params, ZLS3073X_MB_DPLL);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xUpdateMailboxCopy(zl303xx_Params, ZLS3073X_MB_DPLL, dpllId);
    }
    if (multistepMutexStatus == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesGive(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }

    return status;
}  /* END zl303xx_DpllHoldoverDelayStorageSet */

/**
   Function Name:
    zl303xx_DpllHoldoverFilterBandwidthSet

   Details:
    Writes the DPLL Holdover Filter Bandwidth attribute

   Parameters:
    [in]    zl303xx_Params Pointer to the device instance parameter structure
    [in]    dpllId         Associated DPLL Id of the attribute
    [in]    val            The value of the device attribute parameter

   Return Value:
    zlStatusE

*******************************************************************************/

zlStatusE zl3073x_DpllHoldoverFilterBandwidthSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_DpllIdE dpllId, Uint32T val)
{
    zlStatusE status = ZL303XX_OK;
    zlStatusE multistepMutexStatus = ZL303XX_ERROR;

    /* Check params pointer */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_POINTER(zl303xx_Params);
    }

    /* Check the dpllId parameter */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_DPLL_ID(dpllId);
    }

    /* Check that the write value is within range */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_DPLL_HOLDOVER_FILTER_BANDWIDTH(val);
    }

    if (status == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesTake(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetupMailboxForWrite(zl303xx_Params, ZLS3073X_MB_DPLL, dpllId);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Write(zl303xx_Params, NULL, ZLS3073X_DPLLX_HO_FILTER_REG, val);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetMailboxSemWrite(zl303xx_Params, ZLS3073X_MB_DPLL);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xUpdateMailboxCopy(zl303xx_Params, ZLS3073X_MB_DPLL, dpllId);
    }
    if (multistepMutexStatus == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesGive(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }

    return status;
}  /* END zl303xx_DpllHoldoverFilterBandwidthSet */




/**
   Function Name:
    zl303xx_Ref73xDpllVariableLowBandwidthSelectSet

   Details:
    Writes the DPLL Variable Low Bandwidth Select attribute

   Parameters:
    [in]    zl303xx_Params Pointer to the device instance parameter structure
    [in]    dpllId         Associated DPLL Id of the attribute
    [in]    val            The value of the device attribute parameter

   Return Value:
    zlStatusE

*******************************************************************************/

zlStatusE zl303xx_Ref73xDpllVariableLowBandwidthSelectSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_DpllIdE dpllId, Uint32T val)
{
    zlStatusE status = ZL303XX_OK;
    zlStatusE multistepMutexStatus = ZL303XX_ERROR;

    /* Check params pointer */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_POINTER(zl303xx_Params);
    }

    /* Check the dpllId parameter */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_DPLL_ID(dpllId);
    }

    /* Check that the write value is within range */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_DPLL_VARIABLE_LOW_BANDWIDTH_SELECT(val);
    }

    if (status == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesTake(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetupMailboxForWrite(zl303xx_Params, ZLS3073X_MB_DPLL, dpllId);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Write(zl303xx_Params, NULL, ZLS3073X_DPLLX_BW_VAR_REG, val);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetMailboxSemWrite(zl303xx_Params, ZLS3073X_MB_DPLL);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xUpdateMailboxCopy(zl303xx_Params, ZLS3073X_MB_DPLL, dpllId);
    }
    if (multistepMutexStatus == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesGive(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }

    return status;
}  /* END zl303xx_Ref73xDpllVariableLowBandwidthSelectSet */








/**
   Function Name:
    zl303xx_Ref73xSynthEnableSet

   Details:
    Writes the Synth Enable configuration

   Parameters:
    [in]    zl303xx_Params Pointer to the device instance parameter structure
    [in]    synthId   Synthesizer number.
    [in]    val            The value of the device attribute parameter

   Return Value:
    zlStatusE

*******************************************************************************/

zlStatusE zl303xx_Ref73xSynthEnableSet(zl303xx_ParamsS *zl303xx_Params, Uint32T synthId, zl303xx_BooleanE val)
{
    zlStatusE status = ZL303XX_OK;
    zlStatusE multistepMutexStatus = ZL303XX_ERROR;

    /* Check params pointer */
    status = ZL303XX_CHECK_POINTER(zl303xx_Params);

    /* Check the synthesizer ID */
    if (status == ZL303XX_OK)
    {
        status = ZLS3073X_CHECK_SYNTH_ID(synthId);
        if (status == ZL303XX_OK && synthId == ZLS3073X_GP_SYNTHID) {
            status = ZL303XX_PARAMETER_INVALID;
        }
    }

    /* Check that the write value is within range */
    if (status == ZL303XX_OK)
    {
       status = ZL303XX_CHECK_BOOLEAN(val);
    }

    if (status == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesTake(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_ReadModWrite(zl303xx_Params, NULL,
                     ZLS3073X_HP_CTRL_REG(synthId),
                     val == ZL303XX_TRUE ? (ZL303XX_HP_CTRL_SYNTH_EN_MASK<<ZL303XX_HP_CTRL_SYNTH_EN_SHIFT) : 0 ,
                     ZL303XX_HP_CTRL_SYNTH_EN_MASK<<ZL303XX_HP_CTRL_SYNTH_EN_SHIFT, NULL);
    }
    if (multistepMutexStatus == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesGive(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }

    return status;
}  /* END zl303xx_Ref73xSynthEnableSet */


/**
   Sets the DPLL that drives a synthesizer.

  Parameters:
   [in]   zl303xx_Params  Device instance parameter structure.
   [in]   synthId   Synthesizer number.
   [in]   dpllId    The ID of the DPLL that drives the synthesizer.

  Return Value:  ZL303XX_OK  Success.

*******************************************************************************/

zlStatusE zl303xx_Ref73xSynthDrivePll(zl303xx_ParamsS *zl303xx_Params, Uint32T synthId, zl303xx_DpllIdE dpllId)
{
    zlStatusE status = ZL303XX_OK;
    zlStatusE multistepMutexStatus = ZL303XX_ERROR;

    /* Check params pointer */
    status = ZL303XX_CHECK_POINTER(zl303xx_Params);

    /* Check the synthesizer ID */
    // if (status == ZL303XX_OK)
    // {
    //     status = ZLS3073X_CHECK_SYNTH_ID(synthId);
    //     if (status == ZL303XX_OK && synthId == ZLS3073X_GP_SYNTHID) {
    //         status = ZL303XX_PARAMETER_INVALID;
    //     }
    // }

    /* Check the dpllId parameter */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_DPLL_ID(dpllId);
    }

    if (status == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesTake(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_ReadModWrite(zl303xx_Params, NULL,
                     ZLS3073X_HP_CTRL_REG(synthId),
                     dpllId<<ZL303XX_HP_CTRL_DPLL_SHIFT,
                     ZL303XX_HP_CTRL_DPLL_MASK<<ZL303XX_HP_CTRL_DPLL_SHIFT, NULL);
    }
    if (multistepMutexStatus == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesGive(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }

    return status;
}

/**
   Sets the base frequency of a synthesizer.

  Parameters:
   [in]   zl303xx_Params  Device instance parameter structure.
   [in]   synthId   Synthesizer number.
   [in]   freqHz    Base frequency in Hz.

  Return Value:  ZL303XX_OK  Success.

*******************************************************************************/

zlStatusE zl303xx_Ref73xSynthBaseFreqSet(zl303xx_ParamsS *zl303xx_Params, Uint32T synthId, Uint32T freqHz)
{
    zlStatusE status = ZL303XX_OK;
    zlStatusE multistepMutexStatus = ZL303XX_ERROR;
    Uint32T   regAddr = 0;


    /* Check params pointer */
    status = ZL303XX_CHECK_POINTER(zl303xx_Params);

    /* Check the synthesizer ID */
    if (status == ZL303XX_OK)
    {
        status = ZLS3073X_CHECK_SYNTH_ID(synthId);
    }


    if (status == ZL303XX_OK)
    {
        if (synthId == ZLS3073X_GP_SYNTHID) {
            // if (status == ZL303XX_OK)
            // {
            //     //status = ((freqHz <= 0xFFFFU) && (1600000000UL % freqHz == 0)) ? ZL303XX_OK : ZL303XX_PARAMETER_INVALID;
            //     regAddr = ZLS3073X_GP_FREQ_BASE_REG;
            // }
        } else {
            /* No multiple available for Low jitter Synthesizers*/
            if (status == ZL303XX_OK)
            {
                //status = ((freqHz <= 0xFFFFFFFFU) && (1600000000UL % freqHz == 0)) ? ZL303XX_OK : ZL303XX_PARAMETER_INVALID;
                regAddr = ZLS3073X_HP_FREQ_BASE_REG(synthId);
            }
        }
    if (status == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesTake(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }
        if (status == ZL303XX_OK)
        {
            status = zl303xx_Write(zl303xx_Params, NULL, regAddr, freqHz);
        }
    if (multistepMutexStatus == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesGive(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }
    }
    return status;
}


/**
   Sets the M value i.e. the numerator of the M/N ratio of a synthesizer.

  Parameters:
   [in]   zl303xx_Params  Device instance parameter structure.
   [in]   synthId   Synthesizer number.
   [in]   M         M value

  Return Value:  ZL303XX_OK  Success.

*******************************************************************************/

zlStatusE zl303xx_Ref73xSynthRatioMRegSet(zl303xx_ParamsS *zl303xx_Params, Uint32T synthId, Uint32T M)
{
    zlStatusE status = ZL303XX_OK;
    zlStatusE multistepMutexStatus = ZL303XX_ERROR;
    Uint32T regAddr = 0;

    /* Check params pointer */
    status = ZL303XX_CHECK_POINTER(zl303xx_Params);

    /* Check the synthesizer ID */
    if (status == ZL303XX_OK)
    {
        status = ZLS3073X_CHECK_SYNTH_ID(synthId);
    }

    if (synthId == ZLS3073X_GP_SYNTHID) {
        // if (status == ZL303XX_OK)
        // {
        //     status = (M <= 0xFFFFU) ? ZL303XX_OK : ZL303XX_PARAMETER_INVALID;
        //     regAddr = ZLS3073X_GP_FREQ_M_REG;
        // }
    } else {
        if (status == ZL303XX_OK)
        {
            status = (M <= 0xFFFFFFFFU) ? ZL303XX_OK : ZL303XX_PARAMETER_INVALID;
            regAddr = ZLS3073X_HP_FREQ_M_REG(synthId);
        }
    }

    if (status == ZL303XX_OK)
    {
    if (status == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesTake(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }
        status = zl303xx_Write(zl303xx_Params, NULL, regAddr, M);
    if (multistepMutexStatus == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesGive(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }
    }
    return status;
}


/**
   Sets the N value i.e. the denominator of the M/N ratio of a synthesizer.

  Parameters:
   [in]   zl303xx_Params  Device instance parameter structure.
   [in]   synthId   Synthesizer number.
   [in]   N         N value

  Return Value:  ZL303XX_OK  Success.

*******************************************************************************/

zlStatusE zl303xx_Ref73xSynthRatioNRegSet(zl303xx_ParamsS *zl303xx_Params, Uint32T synthId, Uint32T N)
{
    zlStatusE status = ZL303XX_OK;
    zlStatusE multistepMutexStatus = ZL303XX_ERROR;
    Uint32T regAddr = 0;

    /* Check params pointer */
    status = ZL303XX_CHECK_POINTER(zl303xx_Params);

    /* Check the synthesizer ID */
    if (status == ZL303XX_OK)
    {
        status = ZLS3073X_CHECK_SYNTH_ID(synthId);
    }

    if (synthId == ZLS3073X_GP_SYNTHID) {
        // if (status == ZL303XX_OK)
        // {
        //     status = ((N >= 1) && (N <= 0xFFFFU)) ? ZL303XX_OK : ZL303XX_PARAMETER_INVALID;
        //     regAddr = ZLS3073X_GP_FREQ_N_REG;
        // }
    } else {
        if (status == ZL303XX_OK)
        {
            status = ((N >= 1) && (N <= 0xFFFFFFFFU)) ? ZL303XX_OK : ZL303XX_PARAMETER_INVALID;
            regAddr = ZLS3073X_HP_FREQ_N_REG(synthId);
        }
    }

    if (status == ZL303XX_OK)
    {
    if (status == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesTake(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }
        status = zl303xx_Write(zl303xx_Params, NULL, regAddr, N);
    if (multistepMutexStatus == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesGive(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }
    }
    return status;
}


// /**
//    Function Name:
//     zl303xx_Ref73xHpFormatSet

//    Details:
//     Writes the HP Output format (disable,differential,CMOS..).

//    Parameters:
//     [in]    zl303xx_Params Pointer to the device instance parameter structure
//     [in]    val            The value of the device attribute parameter

//    Return Value:
//     zlStatusE

// *******************************************************************************/

zlStatusE zl303xx_Ref73xHpFormatSet(zl303xx_ParamsS *zl303xx_Params, Uint32T outId, zl303xx_Ref73xHpFormat_t format)
{
    zlStatusE status = ZL303XX_OK;
    zlStatusE multistepMutexStatus = ZL303XX_ERROR;

    /* Check params pointer */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_POINTER(zl303xx_Params);
    }

    /* Check that the outId parameter is within range */
    if (status == ZL303XX_OK)
    {
        status = ZLS3073X_CHECK_OUT_ID(outId);
    }

    /* Check that the write value is within range */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_HP_OUT_FORMAT((Uint32T)format);
    }

    if (status == ZL303XX_OK)
    {
        if (status == ZL303XX_OK)
        {
            multistepMutexStatus = zl303xx_Dpll73xMutexesTake(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
        }
        status = zl303xx_ReadModWrite(zl303xx_Params, NULL,
                                      ZLS3073X_HP_OUT_CTRL_REG(outId),
                                      format<<ZL303XX_HP_OUT_FORMAT_SHIFT,
                                      ZL303XX_HP_OUT_FORMAT_MASK<<ZL303XX_HP_OUT_FORMAT_SHIFT, NULL);
        if (multistepMutexStatus == ZL303XX_OK)
        {
            multistepMutexStatus = zl303xx_Dpll73xMutexesGive(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
        }
    }
    return status;

}
/**
   Function Name:
   zl303xx_Ref73xSynthHsDivSet

   Details:
    Configures the Synthesizer Highspeed divider.

   Parameters:
    [in]    zl303xx_Params Pointer to the device instance parameter structure
    [in]    val            The value of the device attribute parameter

   Return Value:
    zlStatusE

*******************************************************************************/
zlStatusE zl303xx_Ref73xSynthHsDivSet(zl303xx_ParamsS *zl303xx_Params, Uint32T synthId, Uint32T val)
{
    zlStatusE status = ZL303XX_OK;
    zlStatusE multistepMutexStatus = ZL303XX_ERROR;

    /* Check params pointer */
    status = ZL303XX_CHECK_POINTER(zl303xx_Params);

    /* Check the synthesizer ID */
    if (status == ZL303XX_OK)
    {
        status = ZLS3073X_CHECK_SYNTH_ID(synthId);
    }

    if (status == ZL303XX_OK)
    {
       status = (val <= 0xFU) ? ZL303XX_OK : ZL303XX_PARAMETER_INVALID;
    }

    if (status == ZL303XX_OK && synthId != ZLS3073X_GP_SYNTHID)
    {
    if (status == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesTake(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }
        status = zl303xx_Write(zl303xx_Params, NULL, ZLS3073X_HP_HSDIV_REG(synthId), val);
    if (multistepMutexStatus == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesGive(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }
    }
    else
    {
        status = ZL303XX_UNSUPPORTED_OPERATION;
    }
    return status;

}
/**
   Function Name:
   zl303xx_Ref73xSynthFracDivBaseSet

   Details:
    Configures the Synthesizer Fractional divider base.

   Parameters:
    [in]    zl303xx_Params Pointer to the device instance parameter structure
    [in]    val            The value of the device attribute parameter

   Return Value:
    zlStatusE

*******************************************************************************/
zlStatusE zl303xx_Ref73xSynthFracDivBaseSet(zl303xx_ParamsS *zl303xx_Params, Uint32T synthId, Uint32T val)
{
    zlStatusE status = ZL303XX_OK;
    zlStatusE multistepMutexStatus = ZL303XX_ERROR;

    /* Check params pointer */
    status = ZL303XX_CHECK_POINTER(zl303xx_Params);

    /* Check the synthesizer ID */
    if (status == ZL303XX_OK)
    {
        status = ZLS3073X_CHECK_SYNTH_ID(synthId);
    }

    if (status == ZL303XX_OK  && synthId != ZLS3073X_GP_SYNTHID)
    {
       status = (val <= 0xFFFFFFFFU) ? ZL303XX_OK : ZL303XX_PARAMETER_INVALID;
    }

    if (status == ZL303XX_OK)
    {
    if (status == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesTake(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }
        status = zl303xx_Write(zl303xx_Params, NULL, ZLS3073X_HP_FDIV_BASE_REG(synthId), val);
    if (multistepMutexStatus == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesGive(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }
    }
    else
    {
        status = ZL303XX_UNSUPPORTED_OPERATION;
    }
    return status;

}
/**
   Function Name:
   zl303xx_Ref73xHpMsDiv

   Details:
    Configures the HP Medium speed divider Output.

   Parameters:
    [in]    zl303xx_Params Pointer to the device instance parameter structure
    [in]    val            The value of the device attribute parameter

   Return Value:
    zlStatusE

*******************************************************************************/
zlStatusE zl303xx_Ref73xHpMsDiv(zl303xx_ParamsS *zl303xx_Params, Uint32T outId, Uint32T val)
{
    zlStatusE status = ZL303XX_OK;
    zlStatusE multistepMutexStatus = ZL303XX_ERROR;

    /* Check params pointer */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_POINTER(zl303xx_Params);
    }

    /* Check that the outId parameter is within range */
    if (status == ZL303XX_OK)
    {
        status = ZLS3073X_CHECK_OUT_ID(outId);
    }

    /* Check that the write value is within range */
    if (status == ZL303XX_OK)
    {
        status = (val <= 0x80U) ? ZL303XX_OK : ZL303XX_PARAMETER_INVALID;
    }

    if (status == ZL303XX_OK)
    {
    if (status == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesTake(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }
        status = zl303xx_Write(zl303xx_Params, NULL,ZLS3073X_HP_OUT_MSDIV_REG(outId),val);
    if (multistepMutexStatus == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesGive(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }
    }

    return status;

}
/**
   Function Name:
   zl303xx_Ref73xHpLsDiv

   Details:
    Configures the HP Low speed divider Output.

   Parameters:
    [in]    zl303xx_Params Pointer to the device instance parameter structure
    [in]    val            The value of the device attribute parameter

   Return Value:
    zlStatusE

*******************************************************************************/
zlStatusE zl303xx_Ref73xHpLsDiv(zl303xx_ParamsS *zl303xx_Params, Uint32T outId, Uint32T val)
{
    zlStatusE status = ZL303XX_OK;
    zlStatusE multistepMutexStatus = ZL303XX_ERROR;

    /* Check params pointer */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_POINTER(zl303xx_Params);
    }

    /* Check that the outId parameter is within range */
    if (status == ZL303XX_OK)
    {
        status = ZLS3073X_CHECK_OUT_ID(outId);
    }

    /* Check that the write value is within range */
    if (status == ZL303XX_OK)
    {
        status = (val <= 0x2000000U) ? ZL303XX_OK : ZL303XX_PARAMETER_INVALID;
    }

    if (status == ZL303XX_OK)
    {
    if (status == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesTake(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }
        status = zl303xx_Write(zl303xx_Params, NULL, ZLS3073X_HP_OUT_LSDIV_REG(outId),val);
    if (multistepMutexStatus == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesGive(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }
    }

    return status;
}

/**

  Function Name:
   zl303xx_RefIsrStatusGet

  Details:
   Gets the Fail Status of all Reference Monitor components.
   Gets the members of the zl303xx_RefIsrStatusS data structure

  Parameters:
   [in]    zl303xx_Params Pointer to the device instance parameter structure
   [in]    par            Pointer to the zl303xx_RefIsrStatusS structure to Get

  Return Value:
   zlStatusE

  Notes:
   This function reads a register with sticky bits. It should be called
   once to read their values and a second time to knock them down.

*******************************************************************************/

zlStatusE zl3073x_RefIsrStatusGet(zl303xx_ParamsS *zl303xx_Params,
                                         zl303xx_RefIsrStatusS *par)
{
   zlStatusE status = ZL303XX_OK;
    zlStatusE multistepMutexStatus = ZL303XX_ERROR;
   Uint32T regValue2 = 0;

   /* Check the parameter pointers */
   if (status == ZL303XX_OK)
   {
      status = ZL303XX_CHECK_POINTERS(zl303xx_Params, par);
   }

   /* Check for a valid RefId */
   if (status == ZL303XX_OK)
   {
      status = ZL303XX_CHECK_REF_ID(par->Id);
   }

    if (status == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesTake(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }
   /* .Read sticky registers */
    if (status == ZL303XX_OK)
    {
        /* Using status registers instead of sticky */
        status = zl303xx_Read(zl303xx_Params, NULL,
                ZLS3073X_REF_MON_FAIL_REG(par->Id),
                &regValue2);
        //printf("B4_ref:: in %s:%u irqactive 0x%0x, ref th 0x%0x \n",
        //        __FUNCTION__,__LINE__,regValue ,regValue2);
    }
    if (multistepMutexStatus == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesGive(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }

   /* Extract */
   if (status == ZL303XX_OK)
   {

      par->scmFail = (zl303xx_BooleanE)ZL303XX_EXTRACT(regValue2,
                                       ZL303XX_REF_MON_SCM_MASK,
                                       ZL303XX_REF_MON_SCM_SHIFT);

      par->cfmFail = (zl303xx_BooleanE)ZL303XX_EXTRACT(regValue2,
                                        ZL303XX_REF_MON_CFM_MASK,
                                        ZL303XX_REF_MON_CFM_SHIFT);

      par->gstFail = (zl303xx_BooleanE)ZL303XX_EXTRACT(regValue2,
                                        ZL303XX_REF_MON_GST_MASK,
                                        ZL303XX_REF_MON_GST_SHIFT);

      par->pfmFail = (zl303xx_BooleanE)ZL303XX_EXTRACT(regValue2,
                                        ZL303XX_REF_MON_PFM_MASK,
                                        ZL303XX_REF_MON_PFM_SHIFT);
      par->refFail = (zl303xx_BooleanE)(par->scmFail | par->cfmFail | par->gstFail | par->pfmFail);
   }

   return status;
} /* END zl303xx_RefIsrStatusGet */

/**

  Function Name:
   zl303xx_RefIsrConfigGet

  Details:
  Query all element mask settings.
  Gets the members of the zl303xx_RefIsrConfigS data structure

  Parameters:
   [in]   zl303xx_Params Pointer to the device instance parameter structure
   [in]   par            Pointer to the zl303xx_RefIsrConfigS structure to Get

  Return Value:
   zlStatusE

*******************************************************************************/

zlStatusE zl3073x_RefIsrConfigGet(zl303xx_ParamsS *zl303xx_Params,
                                  zl303xx_RefIsrConfigS *par)
{
   zlStatusE status = ZL303XX_OK;
    zlStatusE multistepMutexStatus = ZL303XX_ERROR;
   Uint32T regValue = 0;

   /* Check the parameter pointers */
   if (status == ZL303XX_OK)
   {
      status = ZL303XX_CHECK_POINTERS(zl303xx_Params, par);
   }

   /* Check for a valid RefId */
   if (status == ZL303XX_OK)
   {
      status = ZL303XX_CHECK_REF_ID(par->Id);
   }

    if (status == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesTake(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }
   /* Read */
   if (status == ZL303XX_OK)
   {
      status = zl303xx_Read(zl303xx_Params, NULL,
                            ZLS3073X_REF_IRQ_MASK_REG(par->Id),
                            &regValue);
   }

   /* Extract */
   if (status == ZL303XX_OK)
   {
      par->refIsrEn = (zl303xx_BooleanE)ZL303XX_EXTRACT(regValue,
                                        ZL303XX_REF_IRQ_MASK_MASK,
                                        ZL303XX_REF_IRQ_MASK_SHIFT(par->Id));
   }

   /* Read */
   if (status == ZL303XX_OK)
   {
      status = zl303xx_Read(zl303xx_Params, NULL,
                            ZLS3073X_REF_MON_TH_MASK_REG(par->Id),
                            &regValue);
   }
    if (multistepMutexStatus == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesGive(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }

   /* Extract */
   if (status == ZL303XX_OK)
   {
      par->scmIsrEn = (zl303xx_BooleanE)ZL303XX_EXTRACT(regValue,
                                        ZL303XX_REF_MON_SCM_MASK,
                                        ZL303XX_REF_MON_SCM_SHIFT);

      par->cfmIsrEn = (zl303xx_BooleanE)ZL303XX_EXTRACT(regValue,
                                         ZL303XX_REF_MON_CFM_MASK,
                                         ZL303XX_REF_MON_CFM_SHIFT);

      par->gstIsrEn = (zl303xx_BooleanE)ZL303XX_EXTRACT(regValue,
                                         ZL303XX_REF_MON_GST_MASK,
                                         ZL303XX_REF_MON_GST_SHIFT);

      par->pfmIsrEn = (zl303xx_BooleanE)ZL303XX_EXTRACT(regValue,
                                         ZL303XX_REF_MON_PFM_MASK,
                                         ZL303XX_REF_MON_PFM_SHIFT);
   }

   return status;
} /* END zl303xx_RefIsrConfigGet */

/**

  Function Name:
   zl303xx_RefIsrConfigSet

  Details:
   Set all element mask configurations.
   Sets the members of the zl303xx_RefIsrConfigS data structure

  Parameters:
   [in]   zl303xx_Params Pointer to the device instance parameter structure
   [in]   par            Pointer to the zl303xx_RefIsrConfigS structure to Set

  Return Value:
   zlStatusE

*******************************************************************************/

zlStatusE zl3073x_RefIsrConfigSet(zl303xx_ParamsS *zl303xx_Params,
                                  zl303xx_RefIsrConfigS *par)
{
   zlStatusE status = ZL303XX_OK;
    zlStatusE multistepMutexStatus = ZL303XX_ERROR;
   Uint32T regValue, mask;

   /* Check the parameter pointers */
   if (status == ZL303XX_OK)
   {
      status = ZL303XX_CHECK_POINTERS(zl303xx_Params, par);
   }

   /* Check the par values to be written */
   if (status == ZL303XX_OK)
   {
       status = ZL303XX_CHECK_BOOLEAN(par->refIsrEn);
   }
   if (status == ZL303XX_OK)
   {
       status = ZL303XX_CHECK_BOOLEAN(par->scmIsrEn);
   }
   if (status == ZL303XX_OK)
   {
       status = ZL303XX_CHECK_BOOLEAN(par->cfmIsrEn);
   }
   if (status == ZL303XX_OK)
   {
       status = ZL303XX_CHECK_BOOLEAN(par->gstIsrEn);
   }
   if (status == ZL303XX_OK)
   {
       status = ZL303XX_CHECK_BOOLEAN(par->pfmIsrEn);
   }

   //if (status == ZL303XX_OK)
   //{
   //    /* do GPIO configuration */
   //    /* Enable ActiveHigh,levelTrigger */
   //    status = zl303xx_Write(zl303xx_Params, NULL,
   //                               ZLS3073X_GPIO_IRQ_CONFIG_REG,
   //                               0x1);
   //    /* select GPIO_0 for IRQ */
   //    status = zl303xx_Write(zl303xx_Params, NULL,
   //                               ZLS3073X_GPIO_CONFIG_0_REG,
   //                               0x4);
   //}
    if (status == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesTake(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }
   /* Enable reference level interrupt mask */
   if (status == ZL303XX_OK)
   {
      regValue = 0;
      mask = 0;

      // if (par->Id > 7) {
      // status = zl303xx_ReadModWrite(zl303xx_Params, NULL,
      //                               ZLS3073X_REF_IRQ_MASK_4_REG,
      //                               par->refIsrEn ? 1<<(par->Id - 8) : 0,1<<(par->Id - 8), NULL);
      // } else {
      // status = zl303xx_ReadModWrite(zl303xx_Params, NULL,
      //                               ZLS3073X_REF_IRQ_MASK_3_0_REG,
      //                               par->refIsrEn ? 1<<(par->Id) : 0,1<<(par->Id), NULL);
      // }

   }

   /* Package */
   if (status == ZL303XX_OK)
   {
      regValue = 0;
      mask = 0;

      /* losIsrEn */
      ZL303XX_INSERT(regValue, par->scmIsrEn ? 1 : 0,
                             ZL303XX_REF_MON_LOS_MASK,
                             ZL303XX_REF_MON_LOS_SHIFT);

      /* scmIsrEn */
      ZL303XX_INSERT(regValue, par->scmIsrEn ? 1 : 0,
                             ZL303XX_REF_MON_SCM_MASK,
                             ZL303XX_REF_MON_SCM_SHIFT);

      /* cfmIsrEn */
      ZL303XX_INSERT(regValue, par->cfmIsrEn ? 1 : 0,
                             ZL303XX_REF_MON_CFM_MASK,
                             ZL303XX_REF_MON_CFM_SHIFT);

      /* gstIsrEn */
      ZL303XX_INSERT(regValue, par->gstIsrEn ? 1 : 0,
                             ZL303XX_REF_MON_GST_MASK,
                             ZL303XX_REF_MON_GST_SHIFT);

      /* pfmIsrEn */
      ZL303XX_INSERT(regValue, par->pfmIsrEn ? 1 : 0,
                             ZL303XX_REF_MON_PFM_MASK,
                             ZL303XX_REF_MON_PFM_SHIFT);

      mask |= (ZL303XX_REF_MON_LOS_MASK << ZL303XX_REF_MON_LOS_SHIFT) |
              (ZL303XX_REF_MON_SCM_MASK << ZL303XX_REF_MON_SCM_SHIFT) |
              (ZL303XX_REF_MON_CFM_MASK << ZL303XX_REF_MON_CFM_SHIFT) |
              (ZL303XX_REF_MON_GST_MASK << ZL303XX_REF_MON_GST_SHIFT) |
              (ZL303XX_REF_MON_PFM_MASK << ZL303XX_REF_MON_PFM_SHIFT);

      /* Write the Data for this Register */
      status = zl303xx_ReadModWrite(zl303xx_Params, NULL,
                                    ZLS3073X_REF_MON_TH_MASK_REG(par->Id),
                                    regValue, mask, NULL);
   }
    if (multistepMutexStatus == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesGive(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }

   return status;
} /* END zl303xx_RefIsrConfigSet */

/*

  Function Name:
   zl303xx_Ref73xDpllIsrStatusGet

  Details:
   Gets the members of the zl303xx_DpllStatusS data structure

  Parameters:
   [in]    zl303xx_Params Pointer to the device instance parameter structure
   [in]    par            Pointer to the zl303xx_DpllStatusS structure to Get

  Return Value:
   zlStatusE

*******************************************************************************/

zlStatusE zl303xx_Ref73xDpllIsrStatusGet(zl303xx_ParamsS *zl303xx_Params,
                                zl303xx_DpllStatusS *par)
{
   zlStatusE status = ZL303XX_OK;
    zlStatusE multistepMutexStatus = ZL303XX_ERROR;
   Uint32T regValue2 = 0;

   /* Check the parameter pointers */
   if (status == ZL303XX_OK)
   {
      status = ZL303XX_CHECK_POINTERS(zl303xx_Params, par);
   }

   /* Check for a valid DpllId */
   if (status == ZL303XX_OK)
   {
      status = ZL303XX_CHECK_DPLL_ID(par->Id);
   }

    if (status == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesTake(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }
   /* Read sticky registers */
   if (status == ZL303XX_OK)
   {
      /* Using status registers instead of sticky registers */
      status = zl303xx_Read(zl303xx_Params, NULL,
                            ZLS3073X_DPLL_MON_STATUS_REG(par->Id),
                            &regValue2);
      //printf("in %s:%u dpll_th 0x%0x \n",
      //        __FUNCTION__,__LINE__,regValue2);
   }
    if (multistepMutexStatus == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesGive(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }

   /* Extract */
   par->holdover = (zl303xx_DpllHoldStateE)ZL303XX_EXTRACT(regValue2,
           ZLS3073X_DPLL_MON_STICKY_HOLDOVER_MASK,
           ZLS3073X_DPLL_MON_STICKY_HOLDOVER_SHIFT);

   par->locked = (zl303xx_DpllLockStateE)ZL303XX_EXTRACT(regValue2,
           ZLS3073X_DPLL_MON_STICKY_LOCKED_MASK,
           ZLS3073X_DPLL_MON_STICKY_LOCKED_SHIFT);

   return status;
} /* END zl303xx_Ref73xDpllIsrStatusGet */

/**

  Function Name:
   zl303xx_Ref73xDpllIsrMaskGet

  Details:
   Gets the members of the zl303xx_DpllIsrConfigS data structure indicating the
   current interrupt configuration of the specified DPLL.

  Parameters:
   [in]   zl303xx_Params Pointer to the device instance parameter structure
   [in]   par            Pointer to the zl303xx_DpllIsrConfigS structure to Get

  Return Value:
   zlStatusE

*******************************************************************************/

zlStatusE zl303xx_Ref73xDpllIsrMaskGet(zl303xx_ParamsS *zl303xx_Params,
                                   zl303xx_DpllIsrConfigS *par)
{
   zlStatusE status = ZL303XX_OK;
    zlStatusE multistepMutexStatus = ZL303XX_ERROR;
   Uint32T regValue = 0;
   zl303xx_BooleanE isDpll_selected = ZL303XX_FALSE;

   /* Check the parameter pointers */
   if (status == ZL303XX_OK)
   {
      status = ZL303XX_CHECK_POINTERS(zl303xx_Params, par);
   }

   /* Check for a valid DpllId */
   if (status == ZL303XX_OK)
   {
      status = ZL303XX_CHECK_DPLL_ID(par->Id);
   }

    if (status == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesTake(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }
   /* Read */
   // if (status == ZL303XX_OK)
   // {
   //    status = zl303xx_Read(zl303xx_Params, NULL,
   //                              ZLS3073X_DPLL_IRQ_MASK_REG,
   //                              &regValue);
   // }

   /* Check if Individual DPLL is selected to generate interrupts */
   isDpll_selected = (zl303xx_BooleanE)ZL303XX_EXTRACT(regValue,
                                           ZL303XX_DPLL_IRQ_ID_MASK,
                                           ZL303XX_DPLL_IRQ_ID_SHIFT(par->Id));

   if (isDpll_selected == ZL303XX_FALSE)
   {
       par->lockIsrEn     = ZL303XX_FALSE;
       par->lostLockIsrEn = ZL303XX_FALSE;
       par->holdoverIsrEn = ZL303XX_FALSE;
   }
   else
   {
       if (status == ZL303XX_OK)
       {
           status = zl303xx_Read(zl303xx_Params, NULL,
                                     ZLS3073X_DPLL_MON_TH_MASK_REG(par->Id),
                                     &regValue);
       }

       /* Extract */
       if (status == ZL303XX_OK)
       {
           par->lockIsrEn = (zl303xx_BooleanE)ZL303XX_EXTRACT(regValue,
                                                  ZLS3073X_DPLL_MON_MASK_LOCKED_MASK,
                                                  ZLS3073X_DPLL_MON_MASK_LOCKED_SHIFT);

           //par->lostLockIsrEn = (zl303xx_BooleanE)ZL303XX_EXTRACT(regValue,
           //                                       ZL303XX_DPLL_ISR_MASK_LOST_LOCK_MASK,
           //                                       ZL303XX_DPLL_ISR_MASK_LOST_LOCK_SHIFT(par->Id));

           /* For Zl3073x there is not feild that specify that there is a Loss of lock */
           par->lostLockIsrEn = (zl303xx_BooleanE)!(par->lockIsrEn);

           par->holdoverIsrEn = (zl303xx_BooleanE)ZL303XX_EXTRACT(regValue,
                                                      ZLS3073X_DPLL_MON_MASK_HOLDOVER_MASK,
                                                      ZLS3073X_DPLL_MON_MASK_HOLDOVER_SHIFT);
       }

   }
    if (multistepMutexStatus == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesGive(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }
   return status;
} /* END zl303xx_Ref73xDpllIsrMaskGet */

/**

  Function Name:
   zl303xx_Ref73xDpllIsrMaskSet

  Details:
   Sets the interrupt configuration of the specified DPLL using the members of
   the zl303xx_DpllIsrConfigS data structure.

  Parameters:
   [in]   zl303xx_Params Pointer to the device instance parameter structure
   [in]   par            Pointer to the zl303xx_DpllIsrConfigS structure to Set

  Return Value:
   zlStatusE

*******************************************************************************/

zlStatusE zl303xx_Ref73xDpllIsrMaskSet(zl303xx_ParamsS *zl303xx_Params,
                                   zl303xx_DpllIsrConfigS *par)
{
   zlStatusE status = ZL303XX_OK;
    zlStatusE multistepMutexStatus = ZL303XX_ERROR;
   Uint32T regValue, mask;

   /* Check the parameter pointers */
   if (status == ZL303XX_OK)
   {
      status = ZL303XX_CHECK_POINTERS(zl303xx_Params, par);
   }

   /* Check the par values to be written */
   if (status == ZL303XX_OK)
   {
      status = ZL303XX_CHECK_BOOLEAN(par->lockIsrEn);
   }
   if (status == ZL303XX_OK)
   {
      status = ZL303XX_CHECK_BOOLEAN(par->lostLockIsrEn);
   }
   if (status == ZL303XX_OK)
   {
      status = ZL303XX_CHECK_BOOLEAN(par->holdoverIsrEn);
   }

    if (status == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesTake(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }
   if (status == ZL303XX_OK)
   {
      regValue = 0;
      mask = 0;

      if (status == ZL303XX_OK)
      {
          status = zl303xx_Dpll73xSetupMailboxForWrite(zl303xx_Params, ZLS3073X_MB_DPLL, par->Id);
      }
      if (status == ZL303XX_OK)
      {
          status = zl303xx_Write(zl303xx_Params, NULL, ZLS3073X_DPLLX_REF_SW_MASK_REG, 0x3f);
          status = zl303xx_Write(zl303xx_Params, NULL, ZLS3073X_DPLLX_REF_HO_MASK_REG, 0x3f);
      }
      if (status == ZL303XX_OK)
      {
          status = zl303xx_Dpll73xSetMailboxSemWrite(zl303xx_Params, ZLS3073X_MB_DPLL);
      }
      if (status == ZL303XX_OK)
      {
          status = zl303xx_Dpll73xUpdateMailboxCopy(zl303xx_Params, ZLS3073X_MB_DPLL, par->Id);
      }

      /* Set Global level DPLL IRQ bit */
      ZL303XX_INSERT(regValue, 1,
                             ZL303XX_DPLL_IRQ_ID_MASK,
                             ZL303XX_DPLL_IRQ_ID_SHIFT(par->Id));
      mask |= (ZL303XX_DPLL_IRQ_ID_MASK << ZL303XX_DPLL_IRQ_ID_SHIFT(par->Id));
      // status = zl303xx_ReadModWrite(zl303xx_Params, NULL,
      //                                   ZLS3073X_DPLL_IRQ_MASK_REG,
      //                                   regValue, mask, NULL);

      ///* lostLockIsrEn */
      //ZL303XX_INSERT(regValue, par->lostLockIsrEn,
      //                       ZL303XX_DPLL_ISR_MASK_LOST_LOCK_MASK,
      //                       ZL303XX_DPLL_ISR_MASK_LOST_LOCK_SHIFT(par->Id));

      /* lockIsrEn */
      ZL303XX_INSERT(regValue, par->lockIsrEn ? 1 : 0,
                             ZLS3073X_DPLL_MON_MASK_LOCKED_MASK,
                             ZLS3073X_DPLL_MON_MASK_LOCKED_SHIFT);
      /* holdoverIsrEn */
      ZL303XX_INSERT(regValue, par->holdoverIsrEn ? 1 : 0,
                             ZLS3073X_DPLL_MON_MASK_HOLDOVER_MASK,
                             ZLS3073X_DPLL_MON_MASK_HOLDOVER_SHIFT);

      mask |= (ZLS3073X_DPLL_MON_MASK_LOCKED_MASK << ZLS3073X_DPLL_MON_MASK_LOCKED_SHIFT) |
              (ZLS3073X_DPLL_MON_MASK_HOLDOVER_MASK << ZLS3073X_DPLL_MON_MASK_HOLDOVER_SHIFT);

      /* Write the Data for this Register */
      status = zl303xx_ReadModWrite(zl303xx_Params, NULL,
              ZLS3073X_DPLL_MON_TH_MASK_REG(par->Id),
              regValue, mask, NULL);
   }
    if (multistepMutexStatus == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesGive(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }

   return status;
} /* END zl303xx_Ref73xDpllIsrMaskSet */

zlStatusE zl303xx_Ref73xLosStateSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_RefIdE refId, Uint32T los)
{
    zlStatusE status = ZL303XX_OK;
    Uint32T regAddr, bitShift;

    status = ZL303XX_CHECK_POINTER(zl303xx_Params);
    /* Check reference */
    if (status == ZL303XX_OK)
    {
        status = ZLS3073X_CHECK_REF_ID(refId);
    }

    if (status == ZL303XX_OK)
    {
        if (refId <= 7)
        {
           regAddr  = ZLS3073X_REF_LOS_3_0_REG;
           bitShift = refId;
        }
        else
        {
           regAddr  = ZLS3073X_REF_LOS_4_REG;
           bitShift = refId - 8;
        }
        status = zl303xx_ReadModWrite(zl303xx_Params, NULL, regAddr,
                                      los ? (0x1 << bitShift) : 0, /* Reg Value */
                                      (0x1 << bitShift), /* Mask */
                                      NULL);
    }

    return status;
}

/**
   Function Name:
    zl303xx_Ref73xDpllFastLockMasterEnableSet

   Details:
    Writes the DPLL Fast Lock Master Enable attribute

   Parameters:
    [in]    zl303xx_Params Pointer to the device instance parameter structure
    [in]    dpllId         Associated DPLL Id of the attribute
    [in]    val            The value of the device attribute parameter

   Return Value:
    zlStatusE

*******************************************************************************/

zlStatusE zl303xx_Ref73xDpllFastLockMasterEnableSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_DpllIdE dpllId, Uint32T val)
{
    zlStatusE status = ZL303XX_OK;
    Uint32T regVal = 0;

    /* Check params pointer */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_POINTER(zl303xx_Params);
    }

    /* Check the dpllId parameter */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_DPLL_ID(dpllId);
    }

    /* Check that the write value is within range */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_DPLL_FAST_LOCK_MASTER_ENABLE(val);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetupMailboxForRead(zl303xx_Params, ZLS3073X_MB_DPLL, dpllId);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Read(zl303xx_Params, NULL, ZLS3073X_DPLLX_FAST_LOCK_CTRL_REG, &regVal);
    }

    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetupMailboxForWrite(zl303xx_Params, ZLS3073X_MB_DPLL, dpllId);
    }
    if (status == ZL303XX_OK)
    {

        ZL303XX_INSERT(regVal, val,
                          ZL303XX_DPLL_FAST_LOCK_MASTER_ENABLE_MASK,
                          ZL303XX_DPLL_FAST_LOCK_MASTER_ENABLE_SHIFT);
        status = zl303xx_Write(zl303xx_Params, NULL, ZLS3073X_DPLLX_FAST_LOCK_CTRL_REG, regVal);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetMailboxSemWrite(zl303xx_Params, ZLS3073X_MB_DPLL);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xUpdateMailboxCopy(zl303xx_Params, ZLS3073X_MB_DPLL, dpllId);
    }

    return status;
}  /* END zl303xx_Ref73xDpllFastLockMasterEnableSet */

/**
   Function Name:
    zl303xx_Ref73xDpllFastLockFreqErrorThresholdSet

   Details:
    Writes the DPLL Fast Lock Frequency Error Threshold attribute

   Parameters:
    [in]    zl303xx_Params Pointer to the device instance parameter structure
    [in]    dpllId         Associated DPLL Id of the attribute
    [in]    val            The value of the device attribute parameter

   Return Value:
    zlStatusE

*******************************************************************************/

zlStatusE zl303xx_Ref73xDpllFastLockFreqErrorThresholdSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_DpllIdE dpllId, Uint32T val)
{
    zlStatusE status = ZL303XX_OK;

    /* Check params pointer */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_POINTER(zl303xx_Params);
    }

    /* Check the dpllId parameter */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_DPLL_ID(dpllId);
    }

    /* Check that the write value is within range */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_DPLL_FAST_LOCK_FREQ_ERROR_THRESHOLD(val);
    }

    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetupMailboxForWrite(zl303xx_Params, ZLS3073X_MB_DPLL, dpllId);
    }
    if (status == ZL303XX_OK)
    {

        //status = zl303xx_Write(zl303xx_Params, NULL, ZLS3073X_DPLLX_FAST_LOCK_FREQ_ERR_REG, val);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetMailboxSemWrite(zl303xx_Params, ZLS3073X_MB_DPLL);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xUpdateMailboxCopy(zl303xx_Params, ZLS3073X_MB_DPLL, dpllId);
    }

    return status;
}  /* END zl303xx_Ref73xDpllFastLockFreqErrorThresholdSet */

// Initialise Reference 2N(5) to set bit 'enable=1' state.
zlStatusE zl303xx_Ref73xDpllFeedbackRefInit(zl303xx_ParamsS *zl303xx_Params)
{
    Uint32T mBox = 5; //ref 5(2N).
    Uint32T regVal;
    zlStatusE status = ZL303XX_OK;
    zlStatusE multistepMutexStatus = ZL303XX_ERROR;

    if (status == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesTake(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }

    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetupMailboxForWrite(zl303xx_Params, ZLS3073X_MB_ref, mBox);
    }

    regVal  = 1;
    if (status == ZL303XX_OK) {
        status = zl303xx_Write(zl303xx_Params, NULL, ZLS3073X_REFX_CONFIG_REG, regVal);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetMailboxSemWrite(zl303xx_Params, ZLS3073X_MB_ref);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xUpdateMailboxCopy(zl303xx_Params, ZLS3073X_MB_ref, mBox);
    }
    if (multistepMutexStatus == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesGive(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }

    mBox = 2; //ref 2(1P) ref1p_config
    if (status == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesTake(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }

    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetupMailboxForWrite(zl303xx_Params, ZLS3073X_MB_ref, mBox);
    }

    regVal  = 0x81;
    if (status == ZL303XX_OK) {
        status = zl303xx_Write(zl303xx_Params, NULL, ZLS3073X_REFX_CONFIG_REG, regVal);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetMailboxSemWrite(zl303xx_Params, ZLS3073X_MB_ref);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xUpdateMailboxCopy(zl303xx_Params, ZLS3073X_MB_ref, mBox);
    }
    if (multistepMutexStatus == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesGive(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }

    // The data sheet claims that only ref1p_config needs, but
    // experiments shows that ref1n_config also needs to be
    // configured.
    mBox = 3; //ref3(1N) ref1n_config
    if (status == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesTake(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }

    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetupMailboxForWrite(zl303xx_Params, ZLS3073X_MB_ref, mBox);
    }

    regVal  = 0x81;
    if (status == ZL303XX_OK) {
        status = zl303xx_Write(zl303xx_Params, NULL, ZLS3073X_REFX_CONFIG_REG, regVal);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetMailboxSemWrite(zl303xx_Params, ZLS3073X_MB_ref);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xUpdateMailboxCopy(zl303xx_Params, ZLS3073X_MB_ref, mBox);
    }
    if (multistepMutexStatus == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesGive(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }

    mBox = 0; //output mailbox 0 -- output0_width
    if (status == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesTake(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }

    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetupMailboxForWrite(zl303xx_Params, ZLS3073X_MB_output, mBox);
    }

    regVal  = 0x0F4240;
    if (status == ZL303XX_OK) {
        status = zl303xx_Write(zl303xx_Params, NULL, ZLS3073X_OUTPUT_DRIVER_WIDTH_REG, regVal);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetMailboxSemWrite(zl303xx_Params, ZLS3073X_MB_output);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xUpdateMailboxCopy(zl303xx_Params, ZLS3073X_MB_output, mBox);
    }
    if (multistepMutexStatus == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesGive(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }

    mBox = 2; //output mailbox 2 -- output2_mode
    if (status == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesTake(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }

    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetupMailboxForWrite(zl303xx_Params, ZLS3073X_MB_output, mBox);
    }

    regVal  = 0x50;
    if (status == ZL303XX_OK) {
        status = zl303xx_Write(zl303xx_Params, NULL, ZLS3073X_OUTPUT_MODE_REG, regVal);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetMailboxSemWrite(zl303xx_Params, ZLS3073X_MB_output);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xUpdateMailboxCopy(zl303xx_Params, ZLS3073X_MB_output, mBox);
    }
    if (multistepMutexStatus == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesGive(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }

    mBox = 2; //output mailbox 2 -- output2_width
    if (status == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesTake(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }

    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetupMailboxForWrite(zl303xx_Params, ZLS3073X_MB_output, mBox);
    }

    regVal  = 0x14;
    if (status == ZL303XX_OK) {
        status = zl303xx_Write(zl303xx_Params, NULL, ZLS3073X_OUTPUT_DRIVER_WIDTH_REG, regVal);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetMailboxSemWrite(zl303xx_Params, ZLS3073X_MB_output);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xUpdateMailboxCopy(zl303xx_Params, ZLS3073X_MB_output, mBox);
    }
    if (multistepMutexStatus == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesGive(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }

    mBox = 6; //output mailbox 6 -- output6_esync_width
    if (status == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesTake(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }

    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetupMailboxForWrite(zl303xx_Params, ZLS3073X_MB_output, mBox);
    }

    regVal  = 0x0A;
    if (status == ZL303XX_OK) {
        status = zl303xx_Write(zl303xx_Params, NULL, ZLS3073X_OUTPUT_DRIVER_ESYNC_WIDTH_REG, regVal);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetMailboxSemWrite(zl303xx_Params, ZLS3073X_MB_output);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xUpdateMailboxCopy(zl303xx_Params, ZLS3073X_MB_output, mBox);
    }
    if (multistepMutexStatus == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesGive(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }

    mBox = 7; //output mailbox 7 -- output7_esync_width
    if (status == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesTake(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }

    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetupMailboxForWrite(zl303xx_Params, ZLS3073X_MB_output, mBox);
    }

    regVal  = 0x0A;
    if (status == ZL303XX_OK) {
        status = zl303xx_Write(zl303xx_Params, NULL, ZLS3073X_OUTPUT_DRIVER_ESYNC_WIDTH_REG, regVal);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetMailboxSemWrite(zl303xx_Params, ZLS3073X_MB_output);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xUpdateMailboxCopy(zl303xx_Params, ZLS3073X_MB_output, mBox);
    }
    if (multistepMutexStatus == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesGive(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }

    mBox = 8; //output mailbox 8 -- output8_esync_width
    if (status == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesTake(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }

    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetupMailboxForWrite(zl303xx_Params, ZLS3073X_MB_output, mBox);
    }

    regVal  = 0x0A;
    if (status == ZL303XX_OK) {
        status = zl303xx_Write(zl303xx_Params, NULL, ZLS3073X_OUTPUT_DRIVER_ESYNC_WIDTH_REG, regVal);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetMailboxSemWrite(zl303xx_Params, ZLS3073X_MB_output);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xUpdateMailboxCopy(zl303xx_Params, ZLS3073X_MB_output, mBox);
    }
    if (multistepMutexStatus == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesGive(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }


    return status;
}

// Set Pull-in Range.
zlStatusE zl303xx_Dpll73xPullInRangeSet(zl303xx_ParamsS *zl303xx_Params, Uint32T PullInRange)
{
    zlStatusE multistepMutexStatus = ZL303XX_ERROR;
    zlStatusE status = ZL303XX_OK;
    Uint32T pllId = 0;

    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_POINTER(zl303xx_Params);
    }
    if (status == ZL303XX_OK)
    {
        pllId = zl303xx_Params->pllParams.pllId;
        // Checking Dpll id requires change in max dpll-id to ZL303XX_DPLL_ID_6 in zl303xx_DeviceSpec.h file.
        //status = ZL303XX_CHECK_DPLL_ID(pllId);
    }
    if (status == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesTake(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetupMailboxForWrite(zl303xx_Params, ZLS3073X_MB_DPLL, pllId);
    }

    if (status == ZL303XX_OK) {
        status = zl303xx_Write(zl303xx_Params, NULL, ZLS3073X_DPLLX_RANGE_REG, PullInRange);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xSetMailboxSemWrite(zl303xx_Params, ZLS3073X_MB_DPLL);
    }
    if (status == ZL303XX_OK)
    {
        status = zl303xx_Dpll73xUpdateMailboxCopy(zl303xx_Params, ZLS3073X_MB_DPLL, pllId);
    }

    if (multistepMutexStatus == ZL303XX_OK)
    {
        multistepMutexStatus = zl303xx_Dpll73xMutexesGive(zl303xx_Params, ZL303XX_73X_PROTECT_MULTISTEP_PROCESS_DATA, __FUNCTION__, &status);
    }

    return status;
}
