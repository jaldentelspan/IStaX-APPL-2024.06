/*
 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "zl_30361_synce_support.h"
#include "zl303xx_AddressMap36x.h"
#include "zl_dpll_low_level_support.h"
#include "zl303xx_Macros.h"
#include "zl303xx_Trace.h"
#include "zl303xx_ErrTrace.h"
#include "zl303xx_Dpll36x.h"
#include "zl303xx_Int64.h"

/**
   Sets the base frequency of a reference input.

  Parameters:
   [in]   hwParams  Device instance parameter structure.
   [in]   refId     Input reference number.
   [in]   freqHz    Base frequency in Hz.

  Return Value:  ZL303XX_OK  Success.

*******************************************************************************/

zlStatusE zl303xx_Ref36xBaseFreqSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_RefIdE refId, Uint32T freqHz)
{
    zlStatusE status = ZL303XX_OK;

    status = ZL303XX_CHECK_POINTER(zl303xx_Params);
    if (status == ZL303XX_OK)
    {
        status = ZLS3036X_CHECK_REF_ID(refId);
    }

    if (status == ZL303XX_OK)
    {
        status = ((freqHz <= 0xFFFFU) && (1600000000UL % freqHz == 0)) ? ZL303XX_OK : ZL303XX_PARAMETER_INVALID;
    }

    if (status == ZL303XX_OK)
    {
        status = zl303xx_Write(zl303xx_Params, NULL, ZLS3036X_REF_BASE_FREQ_REG(refId), freqHz);
    }

    return status;
}


/**
   Sets the frequency multiple of a reference input.

  Parameters:
   [in]   hwParams  Device instance parameter structure.
   [in]   refId     Input reference number.
   [in]   multiple  Multiple

  Return Value:  ZL303XX_OK  Success.

*******************************************************************************/

zlStatusE zl303xx_Ref36xFreqMultipleSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_RefIdE refId, Uint32T multiple)
{
    zlStatusE status = ZL303XX_OK;

    status = ZL303XX_CHECK_POINTER(zl303xx_Params);
    if (status == ZL303XX_OK)
    {
        status = ZLS3036X_CHECK_REF_ID(refId);
    }

    if (status == ZL303XX_OK)
    {
        status = (multiple <= 0xFFFFU) ? ZL303XX_OK : ZL303XX_PARAMETER_INVALID;
    }

    if (status == ZL303XX_OK)
    {
        status = zl303xx_Write(zl303xx_Params, NULL, ZLS3036X_REF_FREQ_MULT_REG(refId), multiple);
    }

    return status;
}


/**
   Sets the M value i.e. the numerator of the M/N ratio of a reference input.

  Parameters:
   [in]   hwParams  Device instance parameter structure.
   [in]   refId     Input reference number.
   [in]   M         M value

  Return Value:  ZL303XX_OK  Success.

*******************************************************************************/

zlStatusE zl303xx_Ref36xRatioMRegSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_RefIdE refId, Uint32T M)
{
    zlStatusE status = ZL303XX_OK;

    status = ZL303XX_CHECK_POINTER(zl303xx_Params);
    if (status == ZL303XX_OK)
    {
        status = ZLS3036X_CHECK_REF_ID(refId);
    }

    if (status == ZL303XX_OK)
    {
        status = (M <= 0xFFFFU) ? ZL303XX_OK : ZL303XX_PARAMETER_INVALID;
    }

    if (status == ZL303XX_OK)
    {
        status = zl303xx_Write(zl303xx_Params, NULL, ZLS3036X_REF_RATIO_M_REG(refId), M);
    }

    return status;
}


/**
   Sets the N value i.e. the denominator of the M/N ratio of a reference input.

  Parameters:
   [in]   hwParams  Device instance parameter structure.
   [in]   refId     Input reference number.
   [in]   N         N value

  Return Value:  ZL303XX_OK  Success.

*******************************************************************************/

zlStatusE zl303xx_Ref36xRatioNRegSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_RefIdE refId, Uint32T N)
{
    zlStatusE status = ZL303XX_OK;

    status = ZL303XX_CHECK_POINTER(zl303xx_Params);
    if (status == ZL303XX_OK)
    {
        status = ZLS3036X_CHECK_REF_ID(refId);
    }

    if (status == ZL303XX_OK)
    {
        status = ((N >= 1) && (N <= 0xFFFFU)) ? ZL303XX_OK : ZL303XX_PARAMETER_INVALID;
    }

    if (status == ZL303XX_OK)
    {
        status = zl303xx_Write(zl303xx_Params, NULL, ZLS3036X_REF_RATIO_N_REG(refId), N);
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

zlStatusE zl30361_DpllRefPrioritySet(zl303xx_ParamsS *zl303xx_Params, zl303xx_DpllIdE dpllId, zl303xx_RefIdE refId, Uint32T val)
{
    zlStatusE status = ZL303XX_OK;
    zl303xx_AttrRdWrS attrPar;

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

    /* Initialize the attribute structure with the necessary values */
    if (status == ZL303XX_OK)
    {
        status = zl303xx_AttrRdWrStructFill(zl303xx_Params, &attrPar,
                                          ZL303XX_DPLL_REF_PRIORITY_REG(dpllId,refId),
                                          ZL303XX_DPLL_REF_PRIORITY_MASK,
                                          ZL303XX_DPLL_REF_PRIORITY_SHIFT(refId));
    }

    /* Write the Attribute value */
    if (status == ZL303XX_OK)
    {
        attrPar.value = (Uint32T)(val);
        status = zl303xx_AttrWrite(zl303xx_Params, &attrPar);
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

zlStatusE zl30361_DpllRefPriorityGet(zl303xx_ParamsS *zl303xx_Params, zl303xx_DpllIdE dpllId, zl303xx_RefIdE refId, Uint32T *val)
{
    zlStatusE status = ZL303XX_OK;
    zl303xx_AttrRdWrS attrPar;

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

    /* Initialize the attribute structure with the necessary values */
    if (status == ZL303XX_OK)
    {
        status = zl303xx_AttrRdWrStructFill(zl303xx_Params, &attrPar,
                                          ZL303XX_DPLL_REF_PRIORITY_REG(dpllId,refId),
                                          ZL303XX_DPLL_REF_PRIORITY_MASK,
                                          ZL303XX_DPLL_REF_PRIORITY_SHIFT(refId));
    }

    /* Read the Attribute value */
    if (status == ZL303XX_OK)
    {
        status = zl303xx_AttrRead(zl303xx_Params, &attrPar);
        *val = (Uint32T)(attrPar.value);

    }

    return status;
}  /* END zl303xx_DpllRefPriorityGet */


/**

  Function Name:
   zl303xx_Ref36xCfmLimitSet

  Details:
   Writes the CFM Limit attribute

  Parameters:
   [in]    zl303xx_Params Pointer to the device instance parameter structure
   [in]    refId          Associated REF Id of the attribute
   [in]    val            The value of the device attribute parameter

  Return Value:
   zlStatusE

*******************************************************************************/

zlStatusE zl303xx_Ref36xCfmLimitSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_RefIdE refId, Uint32T val)
{
   zlStatusE status = ZL303XX_OK;
   zl303xx_AttrRdWrS attrPar;

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

   /* Check that the write value is within range */
   /* Initialize the attribute structure with the necessary values */
   if (status == ZL303XX_OK)
   {
      status = zl303xx_AttrRdWrStructFill(zl303xx_Params, &attrPar,
                              ZL303XX_REF_CFM_LIMIT_REG(refId),
                              ZL303XX_REF_CFM_LIMIT_MASK,
                              ZL303XX_REF_CFM_LIMIT_SHIFT);
   }

   /* Write the Attribute value */
   if (status == ZL303XX_OK)
   {
      attrPar.value = (Uint32T)(val);
      status = zl303xx_AttrWrite(zl303xx_Params, &attrPar);
   }

   return status;
}  /* END zl303xx_Ref36xCfmLoLimitSet */


/**

  Function Name:
   zl303xx_Ref36xScmLimitSet

  Details:
   Writes the SCM Limit attribute

  Parameters:
   [in]    zl303xx_Params Pointer to the device instance parameter structure
   [in]    refId          Associated REF Id of the attribute
   [in]    val            The value of the device attribute parameter

  Return Value:
   zlStatusE

*******************************************************************************/

zlStatusE zl303xx_Ref36xScmLimitSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_RefIdE refId, Uint32T val)
{
   zlStatusE status = ZL303XX_OK;
   zl303xx_AttrRdWrS attrPar;

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

   /* Check that the write value is within range */
   /* Initialize the attribute structure with the necessary values */
   if (status == ZL303XX_OK)
   {
      status = zl303xx_AttrRdWrStructFill(zl303xx_Params, &attrPar,
                              ZL303XX_REF_SCM_LIMIT_REG(refId),
                              ZL303XX_REF_SCM_LIMIT_MASK,
                              ZL303XX_REF_SCM_LIMIT_SHIFT);
   }

   /* Write the Attribute value */
   if (status == ZL303XX_OK)
   {
      attrPar.value = (Uint32T)(val);
      status = zl303xx_AttrWrite(zl303xx_Params, &attrPar);
   }

   return status;
}  /* END zl303xx_Ref36xCfmLoLimitSet */


/**
   Function Name:
    zl303xx_Ref36xGstDisqualifyTimeSet

   Details:
    Writes the Ref Guard Soak Timer Disqalify Time attribute

   Parameters:
    [in]    zl303xx_Params Pointer to the device instance parameter structure
    [in]    refId          Associated REF Id of the attribute
    [in]    val            The value of the device attribute parameter

   Return Value:
    zlStatusE

*******************************************************************************/

zlStatusE zl303xx_Ref36xGstDisqualifyTimeSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_RefIdE refId, Uint32T val)
{
    zlStatusE status = ZL303XX_OK;
    zl303xx_AttrRdWrS attrPar;

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

    /* Initialize the attribute structure with the necessary values */
    if (status == ZL303XX_OK)
    {
        status = zl303xx_AttrRdWrStructFill(zl303xx_Params, &attrPar,
                                          ZL303XX_DPLL_REF_GST_DISQUALIFY_TIME_REG(refId),
                                          ZL303XX_DPLL_REF_GST_DISQUALIFY_TIME_MASK,
                                          ZL303XX_DPLL_REF_GST_DISQUALIFY_TIME_SHIFT(refId));
    }

    /* Write the Attribute value */
    if (status == ZL303XX_OK)
    {
        attrPar.value = (Uint32T)(val);
        status = zl303xx_AttrWrite(zl303xx_Params, &attrPar);
    }

    return status;
}  /* END zl303xx_Ref36xGstDisqualifyTimeSet */


/**
   Function Name:
    zl303xx_Ref36xGstQualifyTimeSet

   Details:
    Writes the Ref Guard Soak Timer Disqalify Time attribute

   Parameters:
    [in]    zl303xx_Params Pointer to the device instance parameter structure
    [in]    refId          Associated REF Id of the attribute
    [in]    val            The value of the device attribute parameter

   Return Value:
    zlStatusE

*******************************************************************************/

zlStatusE zl303xx_Ref36xGstQualifyTimeSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_RefIdE refId, Uint32T val)
{
    zlStatusE status = ZL303XX_OK;
    zl303xx_AttrRdWrS attrPar;

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

    /* Initialize the attribute structure with the necessary values */
    if (status == ZL303XX_OK)
    {
        status = zl303xx_AttrRdWrStructFill(zl303xx_Params, &attrPar,
                                          ZL303XX_DPLL_REF_GST_QUALIFY_TIME_REG(refId),
                                          ZL303XX_DPLL_REF_GST_QUALIFY_TIME_MASK,
                                          ZL303XX_DPLL_REF_GST_QUALIFY_TIME_SHIFT(refId));
    }

    /* Write the Attribute value */
    if (status == ZL303XX_OK)
    {
        attrPar.value = (Uint32T)(val);
        status = zl303xx_AttrWrite(zl303xx_Params, &attrPar);
    }

    return status;
}  /* END zl303xx_Ref36xGstQualifyTimeSet */


/**
   Function Name:
    zl303xx_Ref36xPfmLimitSet

   Details:
    Writes the Ref PFM limit attribute

   Parameters:
    [in]    zl303xx_Params Pointer to the device instance parameter structure
    [in]    refId          Associated REF Id of the attribute
    [in]    val            The value of the device attribute parameter

   Return Value:
    zlStatusE

*******************************************************************************/

zlStatusE zl303xx_Ref36xPfmLimitSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_RefIdE refId, Uint32T val)
{
    zlStatusE status = ZL303XX_OK;
    zl303xx_AttrRdWrS attrPar;

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

    /* Initialize the attribute structure with the necessary values */
    if (status == ZL303XX_OK)
    {
        status = zl303xx_AttrRdWrStructFill(zl303xx_Params, &attrPar,
                                          ZL303XX_REF_PFM_LIMIT_REG(refId),
                                          ZL303XX_REF_PFM_LIMIT_MASK,
                                          ZL303XX_REF_PFM_LIMIT_SHIFT(refId));
    }

    /* Write the Attribute value */
    if (status == ZL303XX_OK)
    {
        attrPar.value = (Uint32T)(val);
        status = zl303xx_AttrWrite(zl303xx_Params, &attrPar);
    }

    return status;
}  /* END zl303xx_Ref36xPfmLimitSet */

/**
   Function Name:
    zl303xx_DpllPhaseSlopeLimitSet

   Details:
    Writes the DPLL Phase Slope Limit attribute

   Parameters:
    [in]    zl303xx_Params Pointer to the device instance parameter structure
    [in]    dpllId         Associated DPLL Id of the attribute
    [in]    val            The value of the device attribute parameter

   Return Value:
    zlStatusE

*******************************************************************************/

zlStatusE zl303xx_DpllPhaseSlopeLimitSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_DpllIdE dpllId, Uint32T val)
{
    zlStatusE status = ZL303XX_OK;
    zl303xx_AttrRdWrS attrPar;

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

    /* Initialize the attribute structure with the necessary values */
    if (status == ZL303XX_OK)
    {
        status = zl303xx_AttrRdWrStructFill(zl303xx_Params, &attrPar,
                                          ZL303XX_DPLL_PHASE_SLOPE_LIMIT_REG(dpllId),
                                          ZL303XX_DPLL_PHASE_SLOPE_LIMIT_MASK,
                                          ZL303XX_DPLL_PHASE_SLOPE_LIMIT_SHIFT);
    }

    /* Write the Attribute value */
    if (status == ZL303XX_OK)
    {
        attrPar.value = (Uint32T)(val);
        status = zl303xx_AttrWrite(zl303xx_Params, &attrPar);
    }

    return status;
}  /* END zl303xx_DpllPhaseSlopeLimitSet */

/**
   Function Name:
    zl303xx_DpllVariableLowBandwidthSelectSet

   Details:
    Writes the DPLL Variable Low Bandwidth Select attribute

   Parameters:
    [in]    zl303xx_Params Pointer to the device instance parameter structure
    [in]    dpllId         Associated DPLL Id of the attribute
    [in]    val            The value of the device attribute parameter

   Return Value:
    zlStatusE

*******************************************************************************/

zlStatusE zl303xx_DpllVariableLowBandwidthSelectSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_DpllIdE dpllId, Uint32T val)
{
    zlStatusE status = ZL303XX_OK;
    zl303xx_AttrRdWrS attrPar;

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

    /* Initialize the attribute structure with the necessary values */
    if (status == ZL303XX_OK)
    {
        status = zl303xx_AttrRdWrStructFill(zl303xx_Params, &attrPar,
                                          ZL303XX_DPLL_VARIABLE_LOW_BANDWIDTH_SELECT_REG(dpllId),
                                          ZL303XX_DPLL_VARIABLE_LOW_BANDWIDTH_SELECT_MASK,
                                          ZL303XX_DPLL_VARIABLE_LOW_BANDWIDTH_SELECT_SHIFT);
    }

    /* Write the Attribute value */
    if (status == ZL303XX_OK)
    {
        attrPar.value = (Uint32T)(val);
        status = zl303xx_AttrWrite(zl303xx_Params, &attrPar);
    }

    return status;
}  /* END zl303xx_DpllVariableLowBandwidthSelectSet */


/**
   Function Name:
    zl303xx_DpllConfigSet

   Details:
    Writes the DPLL configuration (whether none, DPLL0 only or both DPLLs are enabled)

   Parameters:
    [in]    zl303xx_Params Pointer to the device instance parameter structure
    [in]    val            The value of the device attribute parameter

   Return Value:
    zlStatusE

*******************************************************************************/

//zlStatusE zl303xx_DpllConfigSet(zl303xx_ParamsS *zl303xx_Params, Uint32T val)
//{
//    zlStatusE status = ZL303XX_OK;
//    zl303xx_AttrRdWrS attrPar;
//
//    /* Check params pointer */
//    if (status == ZL303XX_OK)
//    {
//        status = ZL303XX_CHECK_POINTER(zl303xx_Params);
//    }
//
//    /* Check that the write value is within range */
//    if (status == ZL303XX_OK)
//    {
//        status = ZL303XX_CHECK_DPLL_CONFIG_(val);
//    }
//
//    /* Initialize the attribute structure with the necessary values */
//    if (status == ZL303XX_OK)
//    {
//        status = zl303xx_AttrRdWrStructFill(zl303xx_Params, &attrPar,
//                                          ZL303XX_DPLL_CONFIG_REG,
//                                          ZL303XX_DPLL_CONFIG_MASK,
//                                          ZL303XX_DPLL_CONFIG_SHIFT);
//    }
//
//    /* Write the Attribute value */
//    if (status == ZL303XX_OK)
//    {
//        attrPar.value = (Uint32T)(val);
//        status = zl303xx_AttrWrite(zl303xx_Params, &attrPar);
//    }
//
//    return status;
//}  /* END zl303xx_DpllConfigSet */


/**
   Sets the DPLL that drives a synthesizer.

  Parameters:
   [in]   hwParams  Device instance parameter structure.
   [in]   synthId   Synthesizer number.
   [in]   dpllId    The ID of the DPLL that drives the synthesizer.

  Return Value:  ZL303XX_OK  Success.

*******************************************************************************/

zlStatusE zl303xx_SynthDrivePll(zl303xx_ParamsS *zl303xx_Params, Uint32T synthId, zl303xx_DpllIdE dpllId)
{
    zlStatusE status = ZL303XX_OK;
    zl303xx_AttrRdWrS attrPar;

    /* Check params pointer */
    status = ZL303XX_CHECK_POINTER(zl303xx_Params);

    /* Check the synthesizer ID */
    if (status == ZL303XX_OK)
    {
        status = ZLS3036X_CHECK_SYNTH_ID(synthId);
    }

    /* Check the dpllId parameter */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_DPLL_ID(dpllId);
    }

    /* Initialize the attribute structure with the necessary values */
    if (status == ZL303XX_OK)
    {
        status = zl303xx_AttrRdWrStructFill(zl303xx_Params, &attrPar,
                                          ZL303XX_SYNTH_DRIVE_PLL_REG,
                                          ZL303XX_SYNTH_DRIVE_PLL_MASK,
                                          ZL303XX_SYNTH_DRIVE_PLL_SHIFT(synthId));
    }

    /* Write the Attribute value */
    if (status == ZL303XX_OK)
    {
        attrPar.value = (Uint32T)(dpllId);
        status = zl303xx_AttrWrite(zl303xx_Params, &attrPar);
    }

    return status;
}


/**
   Sets the base frequency of a synthesizer.

  Parameters:
   [in]   hwParams  Device instance parameter structure.
   [in]   synthId   Synthesizer number.
   [in]   freqHz    Base frequency in Hz.

  Return Value:  ZL303XX_OK  Success.

*******************************************************************************/

zlStatusE zl303xx_SynthBaseFreqSet(zl303xx_ParamsS *zl303xx_Params, Uint32T synthId, Uint32T freqHz)
{
    zlStatusE status = ZL303XX_OK;

    /* Check params pointer */
    status = ZL303XX_CHECK_POINTER(zl303xx_Params);

    /* Check the synthesizer ID */
    if (status == ZL303XX_OK)
    {
        status = ZLS3036X_CHECK_SYNTH_ID(synthId);
    }

    if (status == ZL303XX_OK)
    {
        status = ((freqHz <= 0xFFFFU) && (1600000000UL % freqHz == 0)) ? ZL303XX_OK : ZL303XX_PARAMETER_INVALID;
    }

    if (status == ZL303XX_OK)
    {
        status = zl303xx_Write(zl303xx_Params, NULL, ZLS3036X_SYNTH_BASE_FREQ_REG(synthId), freqHz);
    }

    return status;
}


/**
   Sets the frequency multiple of a synthesizer.

  Parameters:
   [in]   hwParams  Device instance parameter structure.
   [in]   synthId   Synthesizer number.
   [in]   multiple  Multiple

  Return Value:  ZL303XX_OK  Success.

*******************************************************************************/

zlStatusE zl303xx_SynthFreqMultipleSet(zl303xx_ParamsS *zl303xx_Params, Uint32T synthId, Uint32T multiple)
{
    zlStatusE status = ZL303XX_OK;

    /* Check params pointer */
    status = ZL303XX_CHECK_POINTER(zl303xx_Params);

    /* Check the synthesizer ID */
    if (status == ZL303XX_OK)
    {
        status = ZLS3036X_CHECK_SYNTH_ID(synthId);
    }

    if (status == ZL303XX_OK)
    {
       status = (multiple <= 0xFFFFU) ? ZL303XX_OK : ZL303XX_PARAMETER_INVALID;
    }

    if (status == ZL303XX_OK)
    {
       status = zl303xx_Write(zl303xx_Params, NULL, ZLS3036X_SYNTH_FREQ_MULT_REG(synthId), multiple);
    }

    return status;
}

/**
   Function Name:
    zl303xx_HpCmosEnableSet

   Details:
    Writes the HP CMOS Output Enable bits.

   Parameters:
    [in]    zl303xx_Params Pointer to the device instance parameter structure
    [in]    val            The value of the device attribute parameter

   Return Value:
    zlStatusE

*******************************************************************************/

zlStatusE zl303xx_HpCmosEnableSet(zl303xx_ParamsS *zl303xx_Params, Uint32T outId, Uint32T val)
{
    zlStatusE status = ZL303XX_OK;
    zl303xx_AttrRdWrS attrPar;

    /* Check params pointer */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_POINTER(zl303xx_Params);
    }

    /* Check that the outId parameter is within range */
    if (status == ZL303XX_OK)
    {
        status = ZLS3036X_CHECK_OUT_ID(outId);
    }

    /* Check that the write value is within range */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_HP_CMOS_ENABLE(val);
    }

    /* Initialize the attribute structure with the necessary values */
    if (status == ZL303XX_OK)
    {
        status = zl303xx_AttrRdWrStructFill(zl303xx_Params, &attrPar,
                                          ZL303XX_HP_CMOS_ENABLE_REG,
                                          ZL303XX_HP_CMOS_ENABLE_MASK,
                                          ZL303XX_HP_CMOS_ENABLE_SHIFT(outId));
    }

    /* Write the Attribute value */
    if (status == ZL303XX_OK)
    {
        attrPar.value = (Uint32T)(val);
        status = zl303xx_AttrWrite(zl303xx_Params, &attrPar);
    }

    return status;
}  /* END zl303xx_HpCmosEnableSet */


/**
   Sets the configuration of a post divider of a synthesizer.

  Parameters:
   [in]   hwParams  Device instance parameter structure.
   [in]   synthId   Synthesizer number.
   [in]   divId     Post Divider ID
   [in]   val       The value of the device attribute parameter

  Return Value:  ZL303XX_OK  Success.

*******************************************************************************/

zlStatusE zl303xx_SynthPostDivSet(zl303xx_ParamsS *zl303xx_Params, Uint32T synthId, Uint32T divId, Uint32T val)
{
    zlStatusE status = ZL303XX_OK;
    zl303xx_AttrRdWrS attrPar;

    /* Check params pointer */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_POINTER(zl303xx_Params);
    }

    /* Check the synthesizer ID */
    if (status == ZL303XX_OK)
    {
        status = ZLS3036X_CHECK_SYNTH_ID(synthId);
    }

    /* Check the Post Divider ID */
    if (status == ZL303XX_OK)
    {
        status = ZLS3036X_CHECK_POST_DIV_ID(divId);
    }

    /* Check that the write value is within range */
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_SYNTH_POST_DIV(val);
    }

    /* Initialize the attribute structure with the necessary values */
    if (status == ZL303XX_OK)
    {
        status = zl303xx_AttrRdWrStructFill(zl303xx_Params, &attrPar,
                                          ZL303XX_SYNTH_POST_DIV_REG(synthId, divId),
                                          ZL303XX_SYNTH_POST_DIV_MASK,
                                          ZL303XX_SYNTH_POST_DIV_SHIFT);
    }

    /* Write the Attribute value */
    if (status == ZL303XX_OK)
    {
        attrPar.value = (Uint32T)(val);
        status = zl303xx_AttrWrite(zl303xx_Params, &attrPar);
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

zlStatusE zl30361_RefIsrStatusGet(zl303xx_ParamsS *zl303xx_Params,
                                         zl303xx_RefIsrStatusS *par)
{
   zlStatusE status = ZL303XX_OK;
   Uint32T regValue = 0;
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

// NOTE: Because of the way the zl30363 works, the status can only be read from this register, when interrupt is not masked.
//       We want it to work the same way as in the zl30343. Below, we read not only the status of the individiual interrupts
//       sources but also their mask bits. We then do the interrupt aggregation in software.
//
//   /* Read */
//   if (status == ZL303XX_OK)
//   {
//      status = zl303xx_Read(zl303xx_Params, NULL,
//                            ZL303XX_REF_FAIL_ISR_STATUS_REG(par->Id),
//                            &regValue);
//   }
//
//   /* Extract */
//   if (status == ZL303XX_OK)
//   {
//      par->refFail = (zl303xx_BooleanE)ZL303XX_EXTRACT(regValue,
//                                       ZL303XX_REF_FAIL_ISR_STATUS_MASK,
//                                       ZL303XX_REF_FAIL_ISR_STATUS_SHIFT(par->Id));
//   }

// FIXME: Sticky bits should be locked before reading

   /* Read */
   if (status == ZL303XX_OK)
   {
      status = zl303xx_Read(zl303xx_Params, NULL,
                            ZL303XX_REF_MON_FAIL_REG(par->Id),
                            &regValue);
   }

   /* Write 0 to the register to clear the sticky bits */
   if (status == ZL303XX_OK)
   {
      status = zl303xx_Write(zl303xx_Params, NULL,
                            ZL303XX_REF_MON_FAIL_REG(par->Id),
                            0);
   }

// FIXME: Sticky bits should be unlocked after clearing

   /* Extract */
   if (status == ZL303XX_OK)
   {
      par->scmFail = (zl303xx_BooleanE)ZL303XX_EXTRACT(regValue,
                                       ZL303XX_REF_MON_FAIL_SCM_MASK,
                                       ZL303XX_REF_MON_FAIL_SCM_SHIFT);

      par->cfmFail = (zl303xx_BooleanE)ZL303XX_EXTRACT(regValue,
                                        ZL303XX_REF_MON_FAIL_CFM_MASK,
                                        ZL303XX_REF_MON_FAIL_CFM_SHIFT);

      par->gstFail = (zl303xx_BooleanE)ZL303XX_EXTRACT(regValue,
                                        ZL303XX_REF_MON_FAIL_GST_MASK,
                                        ZL303XX_REF_MON_FAIL_GST_SHIFT);

      par->pfmFail = (zl303xx_BooleanE)ZL303XX_EXTRACT(regValue,
                                        ZL303XX_REF_MON_FAIL_PFM_MASK,
                                        ZL303XX_REF_MON_FAIL_PFM_SHIFT);
   }

   /* Read */
   if (status == ZL303XX_OK)
   {
      status = zl303xx_Read(zl303xx_Params, NULL,
                            ZL303XX_REF_MON_FAIL_MASK_REG(par->Id),
                            &regValue2);
   }

   /* Extract */
   if (status == ZL303XX_OK)
   {                                     // FIXME: Below should be updated
       par->refFail = (zl303xx_BooleanE) // ((ZL303XX_EXTRACT(regValue, ZL303XX_REF_MON_FAIL_LOS_MASK, ZL303XX_REF_MON_FAIL_LOS_SHIFT)   // & ZL303XX_EXTRACT(regValue2, ZL303XX_REF_MON_FAIL_LOS_MASK, ZL303XX_REF_MON_FAIL_LOS_SHIFT))
                                         ((ZL303XX_EXTRACT(regValue, ZL303XX_REF_MON_FAIL_SCM_MASK, ZL303XX_REF_MON_FAIL_SCM_SHIFT))   // & ZL303XX_EXTRACT(regValue2, ZL303XX_REF_MON_FAIL_SCM_MASK, ZL303XX_REF_MON_FAIL_SCM_SHIFT))
                                         |(ZL303XX_EXTRACT(regValue, ZL303XX_REF_MON_FAIL_CFM_MASK, ZL303XX_REF_MON_FAIL_CFM_SHIFT))   // & ZL303XX_EXTRACT(regValue2, ZL303XX_REF_MON_FAIL_CFM_MASK, ZL303XX_REF_MON_FAIL_CFM_SHIFT))
                                         |(ZL303XX_EXTRACT(regValue, ZL303XX_REF_MON_FAIL_GST_MASK, ZL303XX_REF_MON_FAIL_GST_SHIFT))   // & ZL303XX_EXTRACT(regValue2, ZL303XX_REF_MON_FAIL_GST_MASK, ZL303XX_REF_MON_FAIL_GST_SHIFT))
                                         |(ZL303XX_EXTRACT(regValue, ZL303XX_REF_MON_FAIL_PFM_MASK, ZL303XX_REF_MON_FAIL_PFM_SHIFT))); // & ZL303XX_EXTRACT(regValue2, ZL303XX_REF_MON_FAIL_PFM_MASK, ZL303XX_REF_MON_FAIL_PFM_SHIFT)));
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

zlStatusE zl30361_RefIsrConfigGet(zl303xx_ParamsS *zl303xx_Params,
                                  zl303xx_RefIsrConfigS *par)
{
   zlStatusE status = ZL303XX_OK;
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

   /* Read */
   if (status == ZL303XX_OK)
   {
      status = zl303xx_Read(zl303xx_Params, NULL,
                            ZL303XX_REF_FAIL_ISR_MASK_REG(par->Id),
                            &regValue);
   }

   /* Extract */
   if (status == ZL303XX_OK)
   {
      par->refIsrEn = (zl303xx_BooleanE)ZL303XX_EXTRACT(regValue,
                                        ZL303XX_REF_FAIL_ISR_MASK_MASK,
                                        ZL303XX_REF_FAIL_ISR_MASK_SHIFT(par->Id));
   }

   /* Read */
   if (status == ZL303XX_OK)
   {
      status = zl303xx_Read(zl303xx_Params, NULL,
                            ZL303XX_REF_MON_FAIL_MASK_REG(par->Id),
                            &regValue);
   }

   /* Extract */
   if (status == ZL303XX_OK)
   {
      par->scmIsrEn = (zl303xx_BooleanE)ZL303XX_EXTRACT(regValue,
                                        ZL303XX_REF_MON_FAIL_SCM_MASK,
                                        ZL303XX_REF_MON_FAIL_SCM_SHIFT);

      par->cfmIsrEn = (zl303xx_BooleanE)ZL303XX_EXTRACT(regValue,
                                         ZL303XX_REF_MON_FAIL_CFM_MASK,
                                         ZL303XX_REF_MON_FAIL_CFM_SHIFT);

      par->gstIsrEn = (zl303xx_BooleanE)ZL303XX_EXTRACT(regValue,
                                         ZL303XX_REF_MON_FAIL_GST_MASK,
                                         ZL303XX_REF_MON_FAIL_GST_SHIFT);

      par->pfmIsrEn = (zl303xx_BooleanE)ZL303XX_EXTRACT(regValue,
                                         ZL303XX_REF_MON_FAIL_PFM_MASK,
                                         ZL303XX_REF_MON_FAIL_PFM_SHIFT);
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

zlStatusE zl30361_RefIsrConfigSet(zl303xx_ParamsS *zl303xx_Params,
                                  zl303xx_RefIsrConfigS *par)
{
   zlStatusE status = ZL303XX_OK;
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

   /* Package */
   if (status == ZL303XX_OK)
   {
      regValue = 0;
      mask = 0;

      /* refIsrEn */
      ZL303XX_INSERT(regValue, par->refIsrEn,
                             ZL303XX_REF_FAIL_ISR_MASK_MASK,
                             ZL303XX_REF_FAIL_ISR_MASK_SHIFT(par->Id));

      mask |= (Uint32T)(ZL303XX_REF_FAIL_ISR_MASK_MASK <<
                        ZL303XX_REF_FAIL_ISR_MASK_SHIFT(par->Id));

      /* Write the Data for this Register */
      status = zl303xx_ReadModWrite(zl303xx_Params, NULL,
                                    ZL303XX_REF_FAIL_ISR_MASK_REG(par->Id),
                                    regValue, mask, NULL);
   }

   /* Package */
   if (status == ZL303XX_OK)
   {
      regValue = 0;
      mask = 0;

      /* losIsrEn */
      ZL303XX_INSERT(regValue, par->scmIsrEn,
                             ZL303XX_REF_MON_FAIL_LOS_MASK,
                             ZL303XX_REF_MON_FAIL_LOS_SHIFT);

      /* scmIsrEn */
      ZL303XX_INSERT(regValue, par->scmIsrEn,
                             ZL303XX_REF_MON_FAIL_SCM_MASK,
                             ZL303XX_REF_MON_FAIL_SCM_SHIFT);

      /* cfmIsrEn */
      ZL303XX_INSERT(regValue, par->cfmIsrEn,
                             ZL303XX_REF_MON_FAIL_CFM_MASK,
                             ZL303XX_REF_MON_FAIL_CFM_SHIFT);

      /* gstIsrEn */
      ZL303XX_INSERT(regValue, par->gstIsrEn,
                             ZL303XX_REF_MON_FAIL_GST_MASK,
                             ZL303XX_REF_MON_FAIL_GST_SHIFT);

      /* pfmIsrEn */
      ZL303XX_INSERT(regValue, par->pfmIsrEn,
                             ZL303XX_REF_MON_FAIL_PFM_MASK,
                             ZL303XX_REF_MON_FAIL_PFM_SHIFT);

      mask |= (ZL303XX_REF_MON_FAIL_LOS_MASK << ZL303XX_REF_MON_FAIL_LOS_SHIFT) |
              (ZL303XX_REF_MON_FAIL_SCM_MASK << ZL303XX_REF_MON_FAIL_SCM_SHIFT) |
              (ZL303XX_REF_MON_FAIL_CFM_MASK << ZL303XX_REF_MON_FAIL_CFM_SHIFT) |
              (ZL303XX_REF_MON_FAIL_GST_MASK << ZL303XX_REF_MON_FAIL_GST_SHIFT) |
              (ZL303XX_REF_MON_FAIL_PFM_MASK << ZL303XX_REF_MON_FAIL_PFM_SHIFT);

      /* Write the Data for this Register */
      status = zl303xx_ReadModWrite(zl303xx_Params, NULL,
                                    ZL303XX_REF_MON_FAIL_MASK_REG(par->Id),
                                    regValue, mask, NULL);
   }

   return status;
} /* END zl303xx_RefIsrConfigSet */

/**

  Function Name:
   zl303xx_DpllIsrMaskGet

  Details:
   Gets the members of the zl303xx_DpllIsrConfigS data structure indicating the
   current interrupt configuration of the specified DPLL.

  Parameters:
   [in]   zl303xx_Params Pointer to the device instance parameter structure
   [in]   par            Pointer to the zl303xx_DpllIsrConfigS structure to Get

  Return Value:
   zlStatusE

*******************************************************************************/

zlStatusE zl303xx_DpllIsrMaskGet(zl303xx_ParamsS *zl303xx_Params,
                                   zl303xx_DpllIsrConfigS *par)
{
   zlStatusE status = ZL303XX_OK;
   Uint32T regValue = 0;

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

   /* Read */
   if (status == ZL303XX_OK)
   {
      status = zl303xx_Read(zl303xx_Params, NULL,
                            ZL303XX_DPLL_ISR_MASK_REG,
                            &regValue);
   }

   /* Extract */
   if (status == ZL303XX_OK)
   {
      par->lostLockIsrEn = (zl303xx_BooleanE)ZL303XX_EXTRACT(regValue,
                                              ZL303XX_DPLL_ISR_MASK_LOST_LOCK_MASK,
                                              ZL303XX_DPLL_ISR_MASK_LOST_LOCK_SHIFT(par->Id));

      par->holdoverIsrEn = (zl303xx_BooleanE)ZL303XX_EXTRACT(regValue,
                                              ZL303XX_DPLL_ISR_MASK_HOLDOVER_MASK,
                                              ZL303XX_DPLL_ISR_MASK_HOLDOVER_SHIFT(par->Id));
   }

   return status;
} /* END zl303xx_DpllIsrMaskGet */

/**

  Function Name:
   zl303xx_DpllIsrMaskSet

  Details:
   Sets the interrupt configuration of the specified DPLL using the members of
   the zl303xx_DpllIsrConfigS data structure.

  Parameters:
   [in]   zl303xx_Params Pointer to the device instance parameter structure
   [in]   par            Pointer to the zl303xx_DpllIsrConfigS structure to Set

  Return Value:
   zlStatusE

*******************************************************************************/

zlStatusE zl303xx_DpllIsrMaskSet(zl303xx_ParamsS *zl303xx_Params,
                                   zl303xx_DpllIsrConfigS *par)
{
   zlStatusE status = ZL303XX_OK;
   Uint32T regValue, mask;

   /* Check the parameter pointers */
   if (status == ZL303XX_OK)
   {
      status = ZL303XX_CHECK_POINTERS(zl303xx_Params, par);
   }

   /* Check the par values to be written */
   if (status == ZL303XX_OK)
   {
      status = ZL303XX_CHECK_BOOLEAN(par->lostLockIsrEn);
   }
   if (status == ZL303XX_OK)
   {
      status = ZL303XX_CHECK_BOOLEAN(par->holdoverIsrEn);
   }

   /* Package */
   if (status == ZL303XX_OK)
   {
      regValue = 0;
      mask = 0;

      /* lostLockIsrEn */
      ZL303XX_INSERT(regValue, par->lostLockIsrEn,
                             ZL303XX_DPLL_ISR_MASK_LOST_LOCK_MASK,
                             ZL303XX_DPLL_ISR_MASK_LOST_LOCK_SHIFT(par->Id));

      /* holdoverIsrEn */
      ZL303XX_INSERT(regValue, par->holdoverIsrEn,
                             ZL303XX_DPLL_ISR_MASK_HOLDOVER_MASK,
                             ZL303XX_DPLL_ISR_MASK_HOLDOVER_SHIFT(par->Id));

      mask |= (ZL303XX_DPLL_ISR_MASK_LOST_LOCK_MASK << ZL303XX_DPLL_ISR_MASK_LOST_LOCK_SHIFT(par->Id)) |
              (ZL303XX_DPLL_ISR_MASK_HOLDOVER_MASK << ZL303XX_DPLL_ISR_MASK_HOLDOVER_SHIFT(par->Id));

      /* Write the Data for this Register */
      status = zl303xx_ReadModWrite(zl303xx_Params, NULL,
                                    ZL303XX_DPLL_ISR_MASK_REG,
                                    regValue, mask, NULL);
   }

   return status;
} /* END zl303xx_DpllIsrMaskSet */

/*

  Function Name:
   zl303xx_DpllHoldLockStatusGet

  Details:
   Gets the members of the zl303xx_DpllStatusS data structure

  Parameters:
   [in]    zl303xx_Params Pointer to the device instance parameter structure
   [in]    par            Pointer to the zl303xx_DpllStatusS structure to Get

  Return Value:
   zlStatusE

*******************************************************************************/

zlStatusE zl303xx_DpllHoldLockStatusGet(zl303xx_ParamsS *zl303xx_Params,
                                zl303xx_DpllStatusS *par)
{
   zlStatusE status = ZL303XX_OK;
   Uint32T regValue = 0;

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

   /* Read */
   if (status == ZL303XX_OK)
   {
      status = zl303xx_Read(zl303xx_Params, NULL,
                            ZL303XX_DPLL_HOLD_LOCK_STATUS_REG,
                            &regValue);
   }

   /* Write 0 to the register to clear the sticky bits */
   if (status == ZL303XX_OK)
   {
      status = zl303xx_Write(zl303xx_Params, NULL,
                            ZL303XX_DPLL_HOLD_LOCK_STATUS_REG,
                            0);
   }

   /* Extract */
   if (status == ZL303XX_OK)
   {
      par->holdover = (zl303xx_DpllHoldStateE)ZL303XX_EXTRACT(regValue,
                                              ZL303XX_DPLL_HOLD_LOCK_STATUS_HOLDOVER_MASK,
                                              ZL303XX_DPLL_HOLD_LOCK_STATUS_HOLDOVER_SHIFT(par->Id));

      par->locked = (zl303xx_DpllLockStateE)(ZL303XX_EXTRACT(regValue,
                                             ZL303XX_DPLL_HOLD_LOCK_STATUS_LOCK_MASK,
                                             ZL303XX_DPLL_HOLD_LOCK_STATUS_LOCK_SHIFT(par->Id)));
   }

   return status;
} /* END zl303xx_DpllHoldLockStatusGet */


