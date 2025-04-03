



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

#if !defined ZL303XX_APR_1HZ_H_
#define ZL303XX_APR_1HZ_H_


#if defined __cplusplus
extern "C" {
#endif

#include "zl303xx_Apr.h"

#define ZL303XX_APR_1HZ_MAX_X_PARAMETERS 50


/*!  ZL303XX_MAX_1HZ_METRIC_VALUES
            The number of 1Hz adjustment values that are stored and used to calculate
            the mean and standard deviation. */
#define ZL303XX_MAX_1HZ_METRIC_VALUES 10

/*!  Struct: zl303xx_Apr1HzXParamsS
      Holds private or future-enabling 1Hz options.

      These values are initialised by zl303xx_AprGet1HzConfigData()
      and should not be modified.
*/
typedef struct
{
   Sint32T p[ZL303XX_APR_1HZ_MAX_X_PARAMETERS];
} zl303xx_Apr1HzXParamsS;


/*!  Enum: zl303xx_Apr1HzRealignmentTypeE
  1Hz realignment type
 */
/*!  Var: zl303xx_Apr1HzRealignmentTypeE ZL303XX_1HZ_REALIGNMENT_ON_GOOD_LOCK
  Indication that 1Hz alignment should take place once, when a good lock is
  obtained.
 */
/*!  Var: zl303xx_Apr1HzRealignmentTypeE ZL303XX_1HZ_REALIGNMENT_PERIODIC
  Indication that 1Hz alignment should take place periodically.
 */
/*!  Var: zl303xx_Apr1HzRealignmentTypeE ZL303XX_1HZ_REALIGNMENT_MANUAL
  Indication that 1Hz alignment is controlled by the user.
 */
/*!  Var: zl303xx_Apr1HzRealignmentTypeE ZL303XX_1HZ_REALIGNMENT_PERIODIC_U
  Indication that 1Hz alignment should take place periodically.
 */
typedef enum {
   ZL303XX_1HZ_REALIGNMENT_ON_GOOD_LOCK,
   ZL303XX_1HZ_REALIGNMENT_PERIODIC,
   ZL303XX_1HZ_REALIGNMENT_MANUAL,
   ZL303XX_1HZ_REALIGNMENT_PERIODIC_U,
   ZL303XX_1HZ_REALIGNMENT_PER_PACKET,
   ZL303XX_1HZ_REALIGNMENT_LAST
} zl303xx_Apr1HzRealignmentTypeE;

/*!  Struct: zl303xx_AprConfigure1HzS
  Parameters that can be set when doing a 1Hz calculation.
*/
typedef struct
{
   /*!  Var: zl303xx_AprConfigure1HzS::disabled
            If true, then the 1Hz calculation is never performed. This means that
            the 1Hz pulse will not be aligned with the master. This setting does
            not affect the frequency accuracy.
   */
   zl303xx_BooleanE disabled;

   /*!  Var: zl303xx_AprConfigure1HzS::realignmentType
            The realignment type: ZL303XX_1HZ_REALIGNMENT_ON_GOOD_LOCK
            or ZL303XX_1HZ_REALIGNMENT_PERIODIC.

            If realignmentType = ZL303XX_1HZ_REALIGNMENT_ON_GOOD_LOCK, then
            a re-calculation is performed once, immediately after a good
            lock is obtained.

            If realignmentType = ZL303XX_1HZ_REALIGNMENT_PERIODIC, then a
            1Hz re-calculation is performed periodically.

            If realignmentType = ZL303XX_1HZ_REALIGNMENT_MANUAL, then a
            1Hz re-calculation is performed when the user desires it.

            If realignmentType = ZL303XX_1HZ_REALIGNMENT_PERIODIC_U, then a
            1Hz re-calculation is performed periodically (at fixed intervals
            determined by the alogorithm).

            If realignmentType = ZL303XX_1HZ_REALIGNMENT_PER_PACKET, then a
            1Hz re-calculation may be performed every packet; the results is
            filtered and applied to the hardware at set intervals.
   */
   zl303xx_Apr1HzRealignmentTypeE realignmentType;

   /*!  Var: zl303xx_AprConfigure1HzS::realignmentInterval
            The number of seconds between 1Hz re-calculations when
            realignmentType = ZL303XX_1HZ_REALIGNMENT_PERIODIC.

            Must be greater than or equal to 120.
   */
   Uint32T realignmentInterval;

   /*!  Var: zl303xx_AprConfigure1HzS::zl303xx_Apr1HzXParamsS
            Private or future-enabling algorithm options.*/
   zl303xx_Apr1HzXParamsS xParams;

} zl303xx_AprConfigure1HzS;


/* zl303xx_Apr1HzDebugS

1Hz parameters - debug statistics.
*/

typedef struct
{
    /* User-configurable enable/disable

          Default: TRUE i.e. 1Hz is disabled.
     */
    zl303xx_BooleanE isDisabled;

    /* The type of re-alignment - either when a good lock is archived or
          periodically. */
    zl303xx_Apr1HzRealignmentTypeE realignmentType;

    /* The configurable parameter is the number of seconds between re-alignment
          attempts.

          Default: ZL303XX_1HZ_DEFAULT_REALIGNMENT_INTERVAL
          Minimum: ZL303XX_1HZ_MINIMUM_REALIGNMENT_INTERVAL
     */
    Uint32T realignmentInterval;

    /* Debug Statistics */

    zl303xx_BooleanE a6;
    zl303xx_BooleanE a22;
    zl303xx_BooleanE a24;
    zl303xx_BooleanE a26;

    Uint32T a7;
    Uint32T a10;
    Uint32T a18;
    Uint32T a19;
    Uint32T a20;
    Uint32T a21;
    Uint32T a30;

    Uint64S a2;
    Uint64S a4;
    Uint64S a31;
    Uint64S a5;
    Uint64S a32;

} zl303xx_Apr1HzDebugS;


/** zl303xx_AprSetDevice1HzEnabled

   This routine can be used to enable/disable 1Hz for the specified device.

  Parameters:
   [in]  hwParams       Pointer to the device structure
   [in]  enable1Hz       Flag to indicate if enable 1Hz for the device

  Return Value:
    ZL303XX_OK                       if successful
    ZL303XX_INVALID_POINTER          if either zl303xx_Params or par is NULL
    ZL303XX_ERROR                    if the zl303xx_Params device has not been
                                       added
    ZL303XX_STREAM_NOT_IN_USE        if either forward stream or reverse stream is not active
    ZL303XX_RTOS_SEM_TAKE_FAIL       internal error

*****************************************************************************/
zlStatusE zl303xx_AprSetDevice1HzEnabled (void *hwParams, zl303xx_BooleanE enable1Hz);

/** zl303xx_AprSetDevice1HzTargetPerformance

   The function sets the 1Hz target performance for the specified device.

  Parameters:
   [in]  hwParams             Pointer to the device structure
   [in]  target               Target in us (1 <= target <= 10)

  Return Value: ZL303XX_OK                       if successful

****************************************************************************/
zlStatusE zl303xx_AprSetDevice1HzTargetPerformance(void *hwParams, Uint32T target);

/** zl303xx_AprGetDevice1HzTargetPerformance

   This function can be used to retrieve a target performance of 1Hz for the
   specified device.

  Parameters:
   [in]  hwParams             Pointer to the device structure
   [out] target               Pointer of target in us

  Return Value: ZL303XX_OK                       if successful

****************************************************************************/
zlStatusE zl303xx_AprGetDevice1HzTargetPerformance(void *hwParams, Uint32T *target);

/** zl303xx_AprGetServer1HzConfigData

   This routine can be used to retrieve a configure data of 1Hz for a registered server on the
   specified device.

  Parameters:
   [in]  hwParams             Pointer to the device structure
   [in]  serverId             16-bit server ID
   [in]  bForwardPath         Flag to indicate the configuration for the forward path
                           or not
   [out] config1Hz               Pointer to the 1Hz configure data

  Return Value:
    ZL303XX_OK                       if successful
    ZL303XX_INVALID_POINTER          if either zl303xx_Params or par is NULL
    ZL303XX_PARAMETER_INVALID        if:
                                       - can not get the required config data
    ZL303XX_ERROR                    if the zl303xx_Params device has not been
                                       added
    ZL303XX_STREAM_NOT_IN_USE        if either forward stream or reverse stream is not active
    ZL303XX_RTOS_SEM_TAKE_FAIL       internal error

*****************************************************************************/
zlStatusE zl303xx_AprGetServer1HzConfigData
            (
            void *hwParams,
            Uint16T serverId,
            zl303xx_BooleanE bForwardPath,
            zl303xx_AprConfigure1HzS *config1Hz
            );

/** zl303xx_AprConfigServer1Hz

   This routine can be used to configure 1Hz for a registered server on the
   specified device.

  Parameters:
   [in]  hwParams             Pointer to the device structure
   [in]  serverId             16-bit server ID
   [in]  bForwardPath         Flag to indicate the configuration for the forward path
                           or not
   [in]  par                     Pointer to the configure data

  Return Value:
    ZL303XX_OK                       if successful
    ZL303XX_INVALID_POINTER          if either zl303xx_Params or par is NULL
    ZL303XX_PARAMETER_INVALID        if:
                                       - the path is invalid
                                       - realignmentType is invalid
                                       - if realignmentInterval < 120
    ZL303XX_ERROR                    if the zl303xx_Params device has not been
                                       added
    ZL303XX_STREAM_NOT_IN_USE        if server is not registered
    ZL303XX_RTOS_SEM_TAKE_FAIL       internal error

*****************************************************************************/
zlStatusE zl303xx_AprConfigServer1Hz
            (
            void *hwParams,
            Uint16T serverId,
            zl303xx_BooleanE bForwardPath,
            zl303xx_AprConfigure1HzS *par
            );

/** zl303xx_AprGetDevice1HzStatus

   This routine returns the following information about the 1Hz software:
      - whether or not an alignment is currently in progress, and
      - whether or not at least 1 alignment has been performed.

  Parameters:
   [in]  hwParams             pointer to the zl303xx device data
   [out] inProgress           TRUE if a 1Hz re-alignment is in progress
   [out] atLeast1AlignmentPerformed TRUE if at least 1 alignment has been performed

  Return Value:
    ZL303XX_OK                       if successful
    ZL303XX_INVALID_POINTER          if zl303xx_Params, inProgress, or
                                          atLeast1AlignmentPerformed is NULL
    ZL303XX_PARAMETER_INVALID        if
                                          - the zl303xx_Params device has not
                                            been configured or
                                          - the given stream is not a forward path
    ZL303XX_STREAM_NOT_IN_USE        if the stream is not active
    ZL303XX_ERROR                    internal error

*****************************************************************************/
zlStatusE zl303xx_AprGetDevice1HzStatus
            (
            void *hwParams,
            zl303xx_BooleanE *inProgress,
            zl303xx_BooleanE *atLeast1AlignmentPerformed
            );

/**  zl303xx_AprStartDevice1Hz

   This routine starts the 1Hz (re-)calculation mechanism.

   This routine is used by the customer whenever a 1Hz adjustment is desired.


  Parameters:
   [in]  hwParams         pointer to the zl303xx device data

  Return Value:
    ZL303XX_OK                       if successful
    ZL303XX_INVALID_POINTER          if zl303xx_Params is NULL
    ZL303XX_PARAMETER_INVALID        if
                                          - the zl303xx_Params device has not
                                            been configured or
                                          - the given stream is not a forward path
    ZL303XX_NOT_RUNNING              if
                                          - 1Hz is disabled or
                                          - zl303xx_InitDevice() has not been called
                                            i.e. other tasks and data have not
                                            been initialized yet.
    ZL303XX_STREAM_NOT_IN_USE        if the stream is not active
    ZL303XX_RTOS_SEM_TAKE_FAIL       internal error

*****************************************************************************/
zlStatusE zl303xx_AprStartDevice1Hz
            (
            void *hwParams
            );

/**  zl303xx_AprGetDevice1HzAlignmentDifference

   This routine returns:
      - the number of nanoseconds difference between the currently applied
        1Hz alignment and the 1Hz alignment if the previous, best alignment
        values were used and
      - the network condition corresponding to the nanosecond difference.
        Currently, this number can be:
            10 - bad
            25 - poor
            50 - fair
            75 - good

   This routine is used by the user when the realignmentType is MANUAL.

  Parameters:
   [in]  hwParams             pointer to the zl303xx device data
   [out] nsDiff               the difference in nanoseconds. If the value
                                 returned is 2147483647, then the difference
                                 is too large to hold in a 31-bit number.
   [out] networkCondition     the current network condition.

  Return Value:
    ZL303XX_OK                    if successful
    ZL303XX_INVALID_POINTER       if zl303xx_Params is NULL or nsDiff is NULL
    ZL303XX_PARAMETER_INVALID     if
                                       - the zl303xx_Params device has not
                                         been configured or
                                       - the given stream is not a forward path
    ZL303XX_NOT_RUNNING           if
                                       - APR is not running or
                                       - 1Hz is disabled on the given stream
    ZL303XX_STREAM_NOT_IN_USE     if the given stream is not configured
    ZL303XX_ERROR                 internal error
    ZL303XX_RTOS_SEM_TAKE_FAIL    internal error

*****************************************************************************/
zlStatusE zl303xx_AprGetDevice1HzAlignmentDifference
            (
            void *hwParams,
            Uint64S *nsDiff,
            Uint32T *networkCondition

            );

/**  zl303xx_AprGetServer1HzAlignmentDifference

   This routine is eqivelant to zl303xx_AprGetDevice1HzAlignmentDifference()
   except that this routine accepts a server rather than assuming the
   active server.

*****************************************************************************/
zlStatusE zl303xx_AprGetServer1HzAlignmentDifference
            (
            void *hwParams,
            Uint16T serverId,
            Uint64S *nsDiff,
            Uint32T *networkCondition
            );

/**  zl303xx_AprSetDevice1HzAlignment

   This routine applies the last, best alignment values.

   This routine is used by the user when the realignmentType is MANUAL.

  Parameters:
   [in]  hwParams                pointer to the zl303xx device data
   [in]  fastAdjustment       TRUE if the phase slope limit should be used
                                 FALSE if the (much lower) PSL&FCL 'lockedFreqLimit'
                                 limit should be used. Typically, the 1st alignment
                                 would use the phase slope limit, while subsequent
                                 applications would use 'lockedFreqLimit'.

  Return Value:
    ZL303XX_OK                    if successful
    ZL303XX_INVALID_POINTER       if zl303xx_Params is NULL
    ZL303XX_PARAMETER_INVALID     if
                                       - the zl303xx_Params device has not
                                         been configured or
                                       - the given stream is not a forward path
    ZL303XX_NOT_RUNNING           if
                                       - APR is not running or
                                       - 1Hz is disabled on the given stream
    ZL303XX_STREAM_NOT_IN_USE     if the given stream is not configured
    ZL303XX_ERROR                 internal error
    ZL303XX_RTOS_SEM_TAKE_FAIL    internal error

*****************************************************************************/

zlStatusE zl303xx_AprSetDevice1HzAlignment
            (
            void *hwParams,
            zl303xx_BooleanE fastAdjustment
            );

/** zl303xx_AprConfigure1HzStructInit

   This routine initialises the par structure.

   The following values are set by this routine:
      - disabled                     : ZL303XX_TRUE
      - realignmentType              : ZL303XX_1HZ_REALIGNMENT_PERIODIC
      - realignmentInterval          : 120
      - xParams                      : defaults - do not modify

  Parameters:
   [in]   zl303xx_Params      pointer to the zl303xx device data
   [in]   forwardPath       TRUE for forward path streams

   [out]  par               pointer to the initialisation data

  Return Value:
    ZL303XX_OK                       if successful
    ZL303XX_INVALID_POINTER          if either zl303xx_Params or par is NULL

*****************************************************************************/
zlStatusE zl303xx_AprConfigure1HzStructInit
            (
            void *zl303xx_Params,
            zl303xx_BooleanE forwardPath,
            zl303xx_AprConfigure1HzS *par
            );

/** zl303xx_AprConfigure1Hz

   This routine copies configuration parameters from par to the 1Hz's
   internal data structures.

  Parameters:
   [in]  zl303xx_Params      pointer to the zl303xx device data
   [in]  streamNumber      the stream number that we are configuring
   [in]  address2          use 0
   [in]  par               pointer to the initialisation data

  Return Value:
    ZL303XX_OK                       if successful
    ZL303XX_INVALID_POINTER          if either zl303xx_Params or par is NULL
    ZL303XX_PARAMETER_INVALID        if:
                                       - streamNumber is invalid
                                       - realignmentType is invalid
                                       - if realignmentInterval < 120
    ZL303XX_ERROR                    if the zl303xx_Params device has not been
                                       added
    ZL303XX_STREAM_NOT_IN_USE        if either forward stream or reverse stream is not active
    ZL303XX_RTOS_SEM_TAKE_FAIL       internal error

*****************************************************************************/
zlStatusE zl303xx_AprConfigure1Hz
            (
            void *zl303xx_Params,
            Uint32T streamNumber,
            Uint32T address2,
            zl303xx_AprConfigure1HzS *par
            );

/** zl303xx_AprStart1Hz

   This routine starts the 1Hz (re-)calculation mechanism.

   This routine is used by the customer whenever a 1Hz adjustment is desired.

  Parameters:
   [in]  zl303xx_Params         pointer to the zl303xx device data
   [in]  fwdStreamNumber      the stream - must be forward path
   [in]  address2             use 0

  Return Value:
    ZL303XX_OK                       if successful
    ZL303XX_INVALID_POINTER          if zl303xx_Params is NULL
    ZL303XX_PARAMETER_INVALID        if
                                          - the zl303xx_Params device has not
                                            been configured or
                                          - the given stream is not a forward path
    ZL303XX_NOT_RUNNING              if zl303xx_InitDevice() has not been called
                                       i.e. other tasks and data have not been
                                       initialised yet.
    ZL303XX_STREAM_NOT_IN_USE        if the stream is not active

*****************************************************************************/
zlStatusE zl303xx_AprStart1Hz
            (
            void *zl303xx_Params,
            Uint32T fwdStreamNumber,
            Uint32T address2
            );

/** zl303xx_AprGet1HzStatus

   This routine returns the following information about the 1Hz software:
      - whether or not an alignment is currently in progress, and
      - whether or not at least 1 alignment has been performed.

  Parameters:
   [in]  zl303xx_Params         pointer to the zl303xx device data
   [in]  streamNumber         the stream - must be forward path
   [in]  address2             use 0
   [out] inProgress           TRUE if a 1Hz re-alignment is in progress
   [out] atLeast1AlignmentPerformed TRUE if at least 1 alignment has been performed

  Return Value:
    ZL303XX_OK                       if successful
    ZL303XX_INVALID_POINTER          if zl303xx_Params, inProgress, or
                                          atLeast1AlignmentPerformed is NULL
    ZL303XX_PARAMETER_INVALID        if
                                          - the zl303xx_Params device has not
                                            been configured or
                                          - the given stream is not a forward path
    ZL303XX_STREAM_NOT_IN_USE        if the stream is not active
    ZL303XX_ERROR                    internal error

*****************************************************************************/
zlStatusE zl303xx_AprGet1HzStatus
            (
            void *zl303xx_Params,
            Uint32T streamNumber,
            Uint32T address2,
            zl303xx_BooleanE *inProgress,
            zl303xx_BooleanE *atLeast1AlignmentPerformed
            );

/** zl303xx_AprGet1HzAlignmentDifference

   This routine returns:
      - the number of nanoseconds difference between the currently applied
        1Hz alignment and the 1Hz alignment if the previous, best alignment
        values were used and
      - the network condition corresponding to the nanosecond difference.
        Currently, this number can be:
            10 - bad
            25 - poor
            50 - fair
            75 - good

   This routine is used by the user when the realignmentType is MANUAL.

  Parameters:
   [in]  zl303xx_Params         pointer to the zl303xx device data
   [in]  fwdStreamNumber      the stream - must be forward path
   [in]  address2             use 0
   [out] nsDiff               the difference in nanoseconds. If the value
                                 returned is 2147483647, then the difference
                                 is too large to hold in a 31-bit number.
   [out] networkCondition     the current network condition.

  Return Value:
    ZL303XX_OK                    if successful
    ZL303XX_INVALID_POINTER       if zl303xx_Params is NULL or nsDiff is NULL
    ZL303XX_PARAMETER_INVALID     if
                                       - the zl303xx_Params device has not
                                         been configured or
                                       - the given stream is not a forward path
    ZL303XX_NOT_RUNNING           if
                                       - APR is not running or
                                       - 1Hz is disabled on the given stream
    ZL303XX_STREAM_NOT_IN_USE     if the given stream is not configured
    ZL303XX_ERROR                 internal error
    ZL303XX_RTOS_SEM_TAKE_FAIL    internal error

*****************************************************************************/

zlStatusE zl303xx_AprGet1HzAlignmentDifference
            (
            void *zl303xx_Params,
            Uint32T streamNumber,
            Uint32T address2,
            Uint64S *nsDiff,
            Uint32T *networkCondition
            );

/** zl303xx_AprSet1HzAlignment

   This routine applies the last, best alignment values.

   This routine is used when the realignmentType is MANUAL.

  Parameters:
   [in]  zl303xx_Params         pointer to the zl303xx device data
   [in]  fwdStreamNumber      the stream - must be forward path
   [in]  address2             use 0
   [in]  fastAdjustment       TRUE if the phase slope limit should be used
                                 FALSE if the (much lower) PSL&FCL 'lockedFreqLimit'
                                 limit should be used. Typically, the 1st alignment
                                 would use the phase slope limit, while subsequent
                                 applications would use 'lockedFreqLimit'.

  Return Value:
    ZL303XX_OK                    if successful
    ZL303XX_INVALID_POINTER       if zl303xx_Params is NULL
    ZL303XX_PARAMETER_INVALID     if
                                       - the zl303xx_Params device has not
                                         been configured or
                                       - the given stream is not a forward path
    ZL303XX_NOT_RUNNING           if
                                       - APR is not running or
                                       - 1Hz is disabled on the given stream
    ZL303XX_STREAM_NOT_IN_USE     if the given stream is not configured
    ZL303XX_ERROR                 internal error
    ZL303XX_RTOS_SEM_TAKE_FAIL    internal error

*****************************************************************************/

zlStatusE zl303xx_AprSet1HzAlignment
            (
            void *zl303xx_Params,
            Uint32T fwdStreamNumber,
            Uint32T address2,
            zl303xx_BooleanE fastAdjustment
            );


/** zl303xx_AprGetCurrent1HzConfigData

   This routine retrieves the 1Hz configuration data for the active forward
   and reverse streams.

  Parameters:
   [in]  zl303xx_Params      the zl303xx device

   [out] serverId          the server ID of the active reference
   [out] fwd               pointer to the 1Hz data for the forward path
   [out] rev               pointer to the 1Hz data for the reverse path

  Return Value:
   zlStatusE

*****************************************************************************/

zlStatusE zl303xx_AprGetCurrent1HzConfigData
            (
            void *zl303xx_Params,
            Uint16T *serverId,
            zl303xx_AprConfigure1HzS *fwd,
            zl303xx_AprConfigure1HzS *rev
            );

zlStatusE zl303xx_AprPrint1HzData
            (
            void *zl303xx_Params,
            Uint32T streamNumber,
            Uint32T address2
            );

zlStatusE zl303xx_AprGet1HzData
            (
            void *hwParams,
            Uint32T streamNumber,
            Uint32T address2,
            zl303xx_Apr1HzDebugS *pApr1HzDebug
            );

/** zl303xx_AprGetNext1HzMetricElement

   This routine, if called repeatedly, will return all the stored adjustments
   used to determine the 1Hz metric.

   To start, specify startElement = -1. The first element will be returned and
   startElement will be updated.

   Each subsequent call should specify startElement returned by the previous call
   to this function.

   When the last element is returned, lastElement will be TRUE.

   Note: even though adjustmentApplied may be TRUE, the application of this
         adjustment may be blocked by later software eg. PSL/FCL's
         adjustFreqMinPhase parameter.

   Note: it is possible that between retrieving the first and last element, a
         new element is added. To eliminate this possibility, the user can
         discard the last element or disable 1Hz while retrieving elements.

  Parameters:
   [in]     zl303xx_Params   pointer to the zl303xx device data
   [in]     serverId       the server ID
   [in,out] startElement   -1 to start. Otherwise, the previous number returned
                              by this routine.
   [out]    lastElement                TRUE if the values returned are the last
                                          values
   [out]    adjustment                 the adjustment
   [out]    adjustmentUsable           not used
   [out]    adjustmentApplied          TRUE if this adjustment was applied
   [out]    adjustmentNetworkQuality   The network quality of this value


  Return Value: zlStatusE

*****************************************************************************/

zlStatusE zl303xx_AprGetNext1HzMetricElement
            (
            void *hwParams,
            Uint16T serverId,
            Sint32T *startElement,
            zl303xx_BooleanE *lastElement,
            Uint64S *adjustment,
            zl303xx_BooleanE *adjustmentUsable,
            zl303xx_BooleanE *adjustmentApplied,
            Uint32T *adjustmentNetworkQuality
            );

/** zl303xx_AprGet1HzNetworkCondition

   This function returns the current network condition value as determined
   by 1Hz.

  Parameters:
   [in]  hwParams                Pointer to the device structure
   [out] networkCondition        Pointer to network quality

  Return Value: ZL303XX_OK                    if successful

****************************************************************************/
zlStatusE zl303xx_AprGet1HzNetworkCondition
            (
            void *zl303xx_Params,
            Uint16T serverId,
            Uint32T *networkCondition
            );

/** zl303xx_AprSet1HzNetworkCondition

   This function sets the network condition of the given server.

  Parameters:
   [in]  hwParams                Pointer to the device structure
   [in]  networkCondition        network quality

  Return Value: ZL303XX_OK                    if successful

****************************************************************************/
zlStatusE zl303xx_AprSet1HzNetworkCondition
            (
            void *zl303xx_Params,
            Uint16T serverId,
            Uint32T networkCondition
            );

/** zl303xx_AprGet1HzNetworkQualityDecrementViaLockState

   This function gets current state of the feature where the network
   condition variable is decremented when the algorithm state changes
   from 1 to 0 i.e. from locked to not locked.

  Parameters:
   [in]  hwParams          Pointer to the device structure
   [in]  serverId          The server

   [out] networkQualityDecrementViaLockState  TRUE = enable

  Return Value: ZL303XX_OK              If successful

****************************************************************************/

zlStatusE zl303xx_AprGet1HzNetworkQualityDecrementViaLockState
            (
            void *zl303xx_Params,
            Uint16T serverId,
            zl303xx_BooleanE *networkQualityDecrementViaLockState
            );

/** zl303xx_AprSet1HzNetworkQualityDecrementViaLockState

   This function gets the current state (enabled/disabled) of the feature
   where the network condition variable is decremented when the algorithm
   state changes from 1 to 0 i.e. from locked to not locked.

  Parameters:
   [in]  hwParams          Pointer to the device structure
   [in]  serverId          The server to change; both forward and reverse
                              streams are changed
   [out] networkQualityDecrementViaLockState    TRUE = enable

  Return Value: ZL303XX_OK                    if successful

****************************************************************************/


zlStatusE zl303xx_AprSet1HzNetworkQualityDecrementViaLockState
            (
            void *zl303xx_Params,
            Uint16T serverId,
            zl303xx_BooleanE networkQualityDecrementViaLockState
            );

/** zl303xx_AprGet1HzNetworkQualityDecrementViaTimeout

   This function gets the current timeout which is used when reducing the
   network condition variable.

  Parameters:
   [in]  hwParams          Pointer to the device structure
   [in]  serverId          The server

   [out] networkQualityDecrementViaTimeout  Pointer to the returned
                              timeout value. If 0, then this feature is
                              disabled.

  Return Value: ZL303XX_OK              if successful

****************************************************************************/

zlStatusE zl303xx_AprGet1HzNetworkQualityDecrementViaTimeout
            (
            void *zl303xx_Params,
            Uint16T serverId,
            Uint32T *networkQualityDecrementViaTimeout
            );

/** zl303xx_AprSet1HzNetworkQualityDecrementViaTimeout

   This function sets the current timeout which is used when reducing the
   network condition variable.

  Parameters:
   [in]  hwParams          Pointer to the device structure
   [in]  serverId          The server to change; both forward and reverse
                              streams are changed
   [in]  networkQualityDecrementViaTimeout  Timeout value.
                              If 0, then this feature is disabled.

  Return Value: ZL303XX_OK              if successful

****************************************************************************/

zlStatusE zl303xx_AprSet1HzNetworkQualityDecrementViaTimeout
            (
            void *zl303xx_Params,
            Uint16T serverId,
            Uint32T networkQualityDecrementViaTimeout
            );

/**  zl303xx_AprGetReset1HzVarsOnLockLoss

   This routine gets the enable/disable state of the feature where some
   1Hz variables are reset when the reported algorithm lock state changes
   from 1 to 0 i.e. when lock is lost.

  Parameters:
   [in]     zl303xx_Params   pointer to the device data structure
   [in]     serverId       the server

   [out]    resetVars      TRUE = enable, FALSE = disable

  Return Value: zlStatusE

*****************************************************************************/

zlStatusE zl303xx_AprGetReset1HzVarsOnLockLoss
            (
            void *zl303xx_Params,
            Uint16T serverId,
            zl303xx_BooleanE *resetVars
            );

/**  zl303xx_AprSetReset1HzVarsOnLockLoss

   This routine sets (enables or disables) the feature where some 1Hz
   variables are reset when the reported algorithm lock state changes
   from 1 to 0 i.e. when lock is lost.

   When the feature is enabled, the code will reset the following
   variables:
     - estimatedNetworkQuality
     - atLeast1DenseInterval

  Parameters:
   [in]     zl303xx_Params   pointer to the device data structure
   [in]     serverId       the server to change; both forward and reverse
                              streams are changed
   [in]     resetVars      TRUE = enable, FALSE = disable

  Return Value: zlStatusE

*****************************************************************************/

zlStatusE zl303xx_AprSetReset1HzVarsOnLockLoss
            (
            void *zl303xx_Params,
            Uint16T serverId,
            zl303xx_BooleanE resetVars
            );

/**  zl303xx_AprSet1HzFilterEnable

   This routine disables/enables the 1Hz per-packet filter for the giver
   server.

  Parameters:
   [in]  hwParams   pointer to the device data structure
   [in]  serverId   the server
   [in]  enable     TRUE = enable, FALSE = disable

  Return Value: zlStatusE

*****************************************************************************/

Uint32T zl303xx_AprSet1HzFilterEnable
            (
            void *hwParams,
            Uint16T serverId,
            zl303xx_BooleanE enable
            );

/**  zl303xx_AprSet1HzHybridTransient

   This function is used in BC, hybrid mode when a transient condition starts.

   Unless directed by Microsemi personal, please use routine
   zl303xx_AprSetHybridTransient()

  Parameters:
   [in]  hwParams                   pointer to the device data structure
   [in]  BCHybridTransientActive    type of transient

  Return Value: zlStatusE

*****************************************************************************/

Uint32T zl303xx_AprSet1HzHybridTransient
            (
            void *hwParams,
            zl303xx_BCHybridTransientType BCHybridTransientActive
            );


/**

  Function Name:
   zl303xx_AprPrintStructAprConfigure1Hz

  Details:
    Debug utility to print zl303xx_AprConfigure1HzS data structure members for debugging
    configuration.

  Notes:
    The output is always printed via APR logging interface (i.e. LogToMsgQ)
    and the call may sleep several times to allow logging to flush to avoid full
    logging queue. See options in zl303xx_AprPrintStructOptionsT.

  Parameters:
   [in]   bIsFwd         Boolean set ZL303XX_TRUE if `pPfReconfig` is for forward path.
                            Set ZL303XX_FALSE if it is for reverse path.
   [in]   pAprConfigure1Hz  Pointer to the zl303xx_AprConfigure1HzS structure to print.
   [in]   pOptions          Optional. Configurable logging options (see zl303xx_AprPrintStructOptionsT)

  Return Value:
   zlStatusE

 *******************************************************************************/
zlStatusE zl303xx_AprPrintStructAprConfigure1Hz(zl303xx_BooleanE bIsFwd,
                                              zl303xx_AprConfigure1HzS *pAprConfigure1Hz,
                                              zl303xx_AprPrintStructOptionsT *pOptions);

 /**

  Function Name:
   zl303xx_AprPrintStructPFReConfig

  Details:
    Debug utility to print zl303xx_PFReConfigS data structure members for debugging
    configuration.

  Notes:
    The output is always printed via APR logging interface (i.e. LogToMsgQ)
    and the call may sleep several times to allow logging to flush to avoid full
    logging queue. See options in zl303xx_AprPrintStructOptionsT.

  Parameters:
   [in]   pPfReconfig    Pointer to the zl303xx_PFReConfigS structure to print.
   [in]   pOptions       Optional. Configurable logging options (see zl303xx_AprPrintStructOptionsT)

  Return Value:
   zlStatusE

 *******************************************************************************/
zlStatusE zl303xx_AprPrintStructPFReConfig(zl303xx_PFReConfigS *pPfReconfig,
                                         zl303xx_AprPrintStructOptionsT *pOptions);


/** zl303xxgroup4InternalOnlyApplyExternalOffsetsToPhaseCalc

   This function applies the TieClearOffset and the OutputOffset phase offset
   values to the phase estimate.
   - The PA bit and Phase adjustment values should be made after the external
     phase offsets have been applied.
   - In the case where we have a TieClear disable case + wait for TieClear calcuation,
     we will take the phase calcuation and use it as the TieClear offset.
     => For this case we set the input phaseErrorNs to 0.

   Note: this function looks a bit weird since we reused some existing functions.

  Parameters:
   [in]  zl303xx_Params   pointer to the zl303xx device data
   [in]  streamId       streamId
   [in]  phaseErrorNs   phase value in, will be modified

  Return Value: zlStatusE

*****************************************************************************/
zlStatusE zl303xxgroup4InternalOnlyApplyExternalOffsetsToPhaseCalc
            (
               void* zl303xx_Params,
               Uint32T streamId,
               Sint64T *phaseErrorNs
            );

Uint32T zl303xxisqrt(Sint64T n, Uint32T iterations);

#if defined __cplusplus
}
#endif

#endif
