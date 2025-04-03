

/*******************************************************************************

   $Id: 8410f584bd768792a15e60f6157c94bf8acedd47

   Copyright 2006-2022 Microchip Technology Inc.
   All rights reserved.

   Algorithm statistics data structures

   The statistics package, when active, collects data on clock streams,
   streams, and algorithms (with almost all useful information falling
   in the algorithms area).

*******************************************************************************/

/***************************************************************************
                                config
 **************************************************************************/

#ifndef _ZL303XX_APR_STATISTICS_H
#define _ZL303XX_APR_STATISTICS_H

#include "zl303xx_Apr.h"



/*!  Struct: zl303xx_AprGetIntervalConfigS
Interval configuration
*/
typedef struct {

   /*!  Var: zl303xx_AprGetIntervalConfigS::intervalActive
   Copy of zl303xx_AprStatisticsS.intervalActive
   */
   zl303xx_BooleanE intervalActive;

   /*!  Var: zl303xx_AprGetIntervalConfigS::interval
   Copy of zl303xx_AprStatisticsS.interval
   */
   Uint16T interval;

} zl303xx_AprGetIntervalConfigS;


/*!  Struct: zl303xx_AprGetPDVConfigS
PDV (percentile) configuration
*/
typedef struct {

   /*!  Var: zl303xx_AprGetPDVConfigS::PDVPercentile
   Copy of zl303xx_AprStatisticsS.PDVPercentile
   */
   Uint16T PDVPercentile;

} zl303xx_AprGetPDVConfigS;


/*!  Struct: zl303xx_AprGetAlgStatsConfigS
Structure returning APR configuration data.

These values were set previously in the call to zl303xx_AprStatisticsStart(),
data structure zl303xx_AprStatisticsS. They are returned here for convenience.
*/
typedef struct {

   /*!  Var: zl303xx_AprGetAlgStatsConfigS::interval
   Interval configuration
   */
   zl303xx_AprGetIntervalConfigS interval;

   /*!  Var: zl303xx_AprGetAlgStatsConfigS::PDV
   PDV (percentile) configuration
   */
   zl303xx_AprGetPDVConfigS PDV;

} zl303xx_AprGetAlgStatsConfigS;

/***************************************************************************
                                  data
 **************************************************************************/

typedef struct
{
   SINT_T x;

} zl303xx_AprStreamStatisticsS;

/*!  Struct: zl303xx_AprGetPDVDataS
Structure holding PDV statistics.
*/
typedef struct {

   /*!  Var: zl303xx_AprGetPDVDataS::transitTimeAtPercentile
   This is the transit time at the configured percentile i.e. x% of
   received packets have transit times greater than x and (100-x)% of
   received packets have transit times less than x.

   This is a signed number.

   The units are nanoseconds.
   */
   Uint64S transitTimeAtPercentile;

} zl303xx_AprGetPDVDataS;


/*!  Struct: zl303xx_AprGetClockQualityDataS
   Structure holding clock quality statistics.
*/
typedef struct {

   /*!  Var: zl303xx_AprGetClockQualityDataS::clkAccuracyPpb
   This is the extracted clock measured accuracy in relation to the local
   oscillator (similar to Microsemi's DPLL out-of-range indicator).

   The units are ppb.
   */
   Uint32T clkAccuracyPpb;

   /*!  Var: zl303xx_AprGetClockQualityDataS::quality
   This number is an indication of clock recovery quality. The lower the
   number, the better the clock quality. This number can be used to
   compare clocks from different masters.
   */
   Uint32T quality;

} zl303xx_AprGetClockQualityDataS;


/*!  Struct: zl303xx_AprGetRawDataS
*/
typedef struct {

   /*!  Var: zl303xx_AprGetRawDataS::a0
   */
   Uint64S a0;

   /*!  Var: zl303xx_AprGetRawDataS::maxTransitTime
   This number is the maximum transit time recorded during the collection interval
   */
   Uint64S maxTransitTime;

   /*!  Var: zl303xx_AprGetRawDataS::minTransitTime
   This number is the minimum transit time recorded during the collection interval
   */
   Uint64S minTransitTime;

} zl303xx_AprGetRawDataS;


/*!  Struct: zl303xx_AprGetAlgStatsDataS
Structure returning APR data.
*/
typedef struct {

   /*!  Var: zl303xx_AprGetAlgStatsDataS::bPeerDelay
   TRUE if the peer-delay mechanism is being used.

   If TRUE then zl303xx_AprGetAlgStatsDataS::STPNetDly is not valid since the
   peer-delay mechanism does not allow for the same-time protocol network delay
   calculation to be made.
   */
   zl303xx_BooleanE bPeerDelay;
   /*!  Var: zl303xx_AprGetAlgStatsDataS::STPNetDly
   Same-time protocol network delay.

   Units are nanoseconds.
   */
   Uint64S STPNetDly;
   /*!  Var: zl303xx_AprGetAlgStatsDataS::peerMeanDelay
   Peer-delay mechanism network delay. This is the round-trip time for a packet
   from this node to the immediately adjacent node.

   Units are nanoseconds.
   */
   Uint64S peerMeanDelay;

   /*!  Var: zl303xx_AprGetAlgStatsDataS::PDV
   Structure holding PDV statistics.
   */
   zl303xx_AprGetPDVDataS PDV;

   /*!  Var: zl303xx_AprGetAlgStatsDataS::clockQuality
   Structure holding clock quality statistics.
   */
   zl303xx_AprGetClockQualityDataS clockQuality;

   /*!  Var: zl303xx_AprGetAlgStatsDataS::percentSelectedPackets
   Value holding the percentage of packets selected by the algorithm
   */
   Uint32T percentSelectedPackets;

   /*!  Var: zl303xx_AprGetAlgStatsDataS::raw
   Structure holding raw statistics.
   */
   zl303xx_AprGetRawDataS raw;

} zl303xx_AprGetAlgStatsDataS;


/*!  Struct: zl303xx_AprGetAlgStatsS
Structure returning APR statistic configuration and data.
*/
typedef struct {

   /*!  Var: zl303xx_AprGetAlgStatsS::enabled
   enabled = true if statistics are currently being collected on this stream.
   */
   zl303xx_BooleanE enabled;

   /*!  Var: zl303xx_AprGetAlgStatsS::config
   Configuration data previously given using zl303xx_AprStatisticsS.
   */
   zl303xx_AprGetAlgStatsConfigS config;

   /*!  Var: zl303xx_AprGetAlgStatsS::data
   The collected statistics.
   */
   zl303xx_AprGetAlgStatsDataS data;

} zl303xx_AprGetAlgStatsS;


/*!  Struct: zl303xx_AprPerfStatisticsS
 * Structure to store frequency and phase performance data. These statistics
 *  are not collected until a stream has gone to LOCK state. */
typedef struct
{
    /*Frequency Offset*/
   /** Averaged frequency offset measured in ppt. */
   Sint32T dfAveragePpt;    /* Average frequency offset */

   Sint32T dfCurrentPpt;            
   Sint32T dfCurrentMaxPpt;            
   Sint32T dfCurrentMinPpt;            
  
   /* Phase Stability */
   /** Amount of phase movement from the beginning of the observation interval
    *  measured in nanoseconds. */
   Sint32T dfPhiNs;

/** Amount of average phase movement from the beginning of the
 *     observation interval measured in nanoseconds. */
   Sint32T dfAveragePhiNs;


   /** Minimum value of dfPhiNs over 1000 seconds. */
   Sint32T dfPhiMinNs;

   /** Maximum value of dfPhiNs over 1000 seconds. */
   Sint32T dfPhiMaxNs;

   /* Frequency Stability */
   Sint32T FreqStabilityPpt;
   /** Difference between the most recent frequency offset and dfAveragePpt. */
   Sint32T dfDeltaPpt;

   /** Minimum value of dfDeltaPpt over 1000 seconds. */
   Sint32T dfDeltaMinPpt;

   /** Maximum value of dfDeltaPpt over 1000 seconds. */
   Sint32T dfDeltaMaxPpt;


   /* Phase Offset*/
   Sint32T PhaseOffsetNs;  
   Sint32T PhaseOffsetAvNs;
   Sint32T PhaseOffsetMaxNs;
   Sint32T PhaseOffsetMinNs;

   /* L1 and L4 Targets */
   Sint32T L4Target;
   Sint32T L1Target;
   zl303xx_BooleanE FLocked;

} zl303xx_AprPerfStatisticsS;

/*!  Struct: zl303xx_AprGetStatisticsS
Collected statistics.
*/
typedef struct
{
   /** Frequency and phase performance statistics. These will be the same for
    *  every APR stream. */
   zl303xx_AprPerfStatisticsS performance;

   zl303xx_AprPerfStatisticsS performanceAnyState;

   /*!  Var: zl303xx_AprGetStatisticsS::fwdStreamStats
   Forward stream stats
   */
   zl303xx_AprStreamStatisticsS fwdStreamStats;

   /*!  Var: zl303xx_AprGetStatisticsS::fwdAlgStats
   Forward stream algorithm stats
   */
   zl303xx_AprGetAlgStatsS fwdAlgStats[ZL303XX_APR_PARAM_1];

   /*!  Var: zl303xx_AprGetStatisticsS::revStreamStats
   Reverse stream stats
   */
   zl303xx_AprStreamStatisticsS revStreamStats;

   /*!  Var: zl303xx_AprGetStatisticsS::revAlgStats
   Reverse stream algorithm stats
   */
   zl303xx_AprGetAlgStatsS revAlgStats[ZL303XX_APR_PARAM_1];

} zl303xx_AprGetStatisticsS;


/*!  Struct: zl303xx_AprStatisticsS
These values are the user-configurable statistics parameters.
*/
typedef struct {

   /*!  Var: zl303xx_AprStatisticsS::globalEnabled
   This parameter enables/disables statistic collection on a global basis.

   Normally, the user can ignore this parameter.

   If true, then statistic collection is enabled on a global basis.
   zl303xx_AprStatisticsStartStructInit() sets this to true.

   Note: setting this to false disables ALL statistic collection - not just
         the collection for this stream.
   */
   zl303xx_BooleanE globalEnabled;

   /*!  Var: zl303xx_AprStatisticsS::globalEnabled
   This parameter enables/disables statistic collection on this stream.
   */
   zl303xx_BooleanE enabled;

   /*!  Var: zl303xx_AprStatisticsS::intervalActive
   If true, statistics are collected for 'interval' seconds. If false,
   statistics are continuously collected.
   */
   zl303xx_BooleanE intervalActive;

   /*!  Var: zl303xx_AprStatisticsS::interval
   The number of seconds that statistics are collected.
   */
   Uint16T interval;

   /*!  Var: zl303xx_AprStatisticsS::PDVPercentile
   This is the percentage where 'PDVPercentile'% of received packets
   have transit times greater than 'X'. 'X' is returned in
   zl303xx_AprGetStatistics.

   The units of PDVPercentile is %*100 to allow specifying up to
   99.99%.

   'PDVPercentile' is always calculated over an interval using saved
   data. The data is saved in a circular buffer i.e. only the newest
   values are used in the calculation. The maximum number of save
   packets is ZL303XX_MAX_STATS_PDV_PERCENTILE_STORAGE.

   If intervalActive = true, then the data sample used in the
   calculation is the smaller of:
      - the number of packets collected in the specified interval
      - ZL303XX_MAX_STATS_PDV_PERCENTILE_STORAGE packets

   If intervalActive = false, then the data sample used in the
   calculation is ZL303XX_MAX_STATS_PDV_PERCENTILE_STORAGE packets.
   (except in the situation where the buffer has not been filled
   yet; in that case, the available data is used)
   */
   Uint32T PDVPercentile;

} zl303xx_AprStatisticsS;


typedef struct {
   Uint32T stream;
   zl303xx_BooleanE enabled;
} zl303xx_AprInitStatisticsS;


/** zl303xx_AprStatisticsGlobalControl

   This routine controls APR statistic collection globally.

   It is strongly recomended that global statistic collection be turned off
   when it is not being used.

  Parameters:
   [in]  hwParams      pointer to the zl303xx device data
   [in] enabled            TRUE if the caller wants statistics enabled

  Return Value:
    ZL303XX_OK                    if successful
    ZL303XX_INVALID_POINTER       if hwParams is NULL

*****************************************************************************/
zlStatusE zl303xx_AprStatisticsGlobalControl
            (
            void *hwParams,
            zl303xx_BooleanE enabled
            );
zlStatusE zl303xx_AprClearStatisticsInternal
            (
            void *hwParams,
            Uint32T stream
            );

/** zl303xx_AprClearStatistics

   This routine clears the statistics structures for the given clock stream
   handle.

  Parameters:
   [in] hwParams          pointer to the zl303xx device data
   [in] clockStreamHandle     handle to the structures to clear

  Return Value:
    ZL303XX_OK                    if successful
    ZL303XX_INVALID_POINTER       if hwParams is NULL
    ZL303XX_PARAMETER_INVALID     if:
                                       - clockStreamHandle >= ZL303XX_APR_MAX_NUM_STREAMS
                                       - internal error
    ZL303XX_ERROR                 if hwParams is not configured
    ZL303XX_STREAM_NOT_IN_USE     internal error

*****************************************************************************/
zlStatusE zl303xx_AprClearStatistics
            (
            void *hwParams,
            Uint32T clockStreamHandle
            );
zlStatusE zl303xx_AprStatisticsStructInit
            (
            void *hwParams,
            zl303xx_AprInitStatisticsS *par
            );
zlStatusE zl303xx_AprStatisticsInit
            (
            void *hwParams,
            zl303xx_AprInitStatisticsS *par
            );

/** zl303xx_AprStatisticsStartStructInit

   This routine initialises the default values for statistic collection.

   The default values are:
      - globalEnabled = true
      - enabled = true
      - intervalActive = true
      - interval = 10 (seconds)
      - PDVPercentile = 9000 (90.00%)

  Parameters:
   [in]  hwParams      pointer to the zl303xx device data
   [out] par               pointer to the initialised data structure

  Return Value:
    ZL303XX_OK                    if successful
    ZL303XX_INVALID_POINTER       if either hwParams or par is NULL

*****************************************************************************/
zlStatusE zl303xx_AprStatisticsStartStructInit
            (
            void *hwParams,
            zl303xx_AprStatisticsS *par
            );
zlStatusE zl303xx_AprStatisticsStartInternal
            (
            void *hwParams,
            Uint32T streamNumber,
            Uint32T address2,
            zl303xx_AprStatisticsS *par
            );

/** zl303xx_AprStatisticsStart

   This routine starts statistics collection given a clock stream handle.

   A clock stream handle may (usually) contain handles to 2 streams; this
   routine starts statistic collection on both streams.

   Starting or re-starting statistics does not clear statistics. To clear
   statistics, use zl303xx_AprClearStatistics().

   To detect the end of an interval, check zl303xx_AprGetAlgStatsS.enabled.
   If true, then statistics are still being collected.

  Parameters:
   [in]  hwParams      pointer to the zl303xx device data
   [in]  clockStreamHandle the clock stream handle
   [in] par                pointer to the config parameters

  Return Value:
    ZL303XX_OK                    if successful
    ZL303XX_INVALID_POINTER       if either hwParams or par is NULL
    ZL303XX_PARAMETER_INVALID     if:
                                       - clockStreamHandle >= ZL303XX_APR_MAX_NUM_STREAMS
                                       - par->PDVPercentile > 9999 (99.99%)
                                       - internal error
    ZL303XX_ERROR                 if hwParams is not configured
    ZL303XX_STREAM_NOT_IN_USE     one of the streams accessed by
                                    clockStreamHandle is not configured

*****************************************************************************/
zlStatusE zl303xx_AprStatisticsStart
            (
            void *hwParams,
            Uint32T clockStreamHandle,
            zl303xx_AprStatisticsS *par
            );
zlStatusE zl303xx_AprStatisticsStopInternal
            (
            void *hwParams,
            Uint32T streamNumber,
            Uint32T address2
            );

/** zl303xx_AprStatisticsStop

   This routine stops statistic collection on the given clock stream handle.

   A clock stream handle may (usually does) contain handles to 2 streams;
   this routine starts statistic collection on both streams.

  Parameters:
   [in]  hwParams      pointer to the zl303xx device data
   [in]  clockStreamHandle the clock stream handle

  Return Value:
    ZL303XX_OK                    if successful
    ZL303XX_INVALID_POINTER       if hwParams is NULL
    ZL303XX_PARAMETER_INVALID     if clockStreamHandle >= ZL303XX_APR_MAX_NUM_STREAMS

*****************************************************************************/
zlStatusE zl303xx_AprStatisticsStop
            (
            void *hwParams,
            Uint32T clockStreamHandle
            );
zlStatusE zl303xx_AprStatsUpdate
            (
            void *hwParams,
            Uint32T streamNumber,
            Uint32T address2,
            Sint32T deltaCF
            );
zlStatusE zl303xx_AprGetStatisticsInternal
            (
            void *hwParams,
            Uint32T streamNumber,
            Uint32T address2,
            zl303xx_AprGetAlgStatsS *pStat
            );

/** zl303xx_AprGetStatistics

   This routine gets the statistics for the given clock stream handle.

   See the type definition of zl303xx_AprGetStatisticsS for a detailed
   description of each statistic.

  Parameters:
   [in]  hwParams      pointer to the zl303xx device data
   [in]  clockStreamHandle which statistics to get
   [out] stats             the statistics

  Return Value:
    ZL303XX_OK                    if successful
    ZL303XX_INVALID_POINTER       if either hwParams or stats is NULL
    ZL303XX_PARAMETER_INVALID     if clockStreamHandle >= ZL303XX_APR_MAX_NUM_STREAMS
    ZL303XX_ERROR                 if hwParams is not configured
    ZL303XX_STREAM_NOT_IN_USE     internal error

*****************************************************************************/
zlStatusE zl303xx_AprGetStatistics
            (
            void *hwParams,
            Uint32T clockStreamHandle,
            zl303xx_AprGetStatisticsS *stats
            );

/* zl303xx_AprGetPerfStatistics */
/**
   Gets the frequency/phase performance statistics for an APR device.

  Parameters:
   [in]   hwParams     Pointer to the zl303xx device data.
   [out]  pPerfStats   Returned performance stats.

  Return Value:
     ZL303XX_OK               if successful
     ZL303XX_INVALID_POINTER  if hwParams or pPerfStats is NULL

*******************************************************************************/
zlStatusE zl303xx_AprGetPerfStatistics
          (
             void *hwParams,
             zl303xx_AprPerfStatisticsS *pPerfStats,
             zl303xx_AprPerfStatisticsS *pPerfStatsAnyState
          );

/* zl303xx_AprResetPerfStatistics */
/**
   Resets the frequency/phase performance statistics collection.

  Parameters:
   [in]   hwParams   Pointer to the zl303xx device data.

  Return Value:
     ZL303XX_OK               if successful
     ZL303XX_INVALID_POINTER  if hwParams is NULL

*******************************************************************************/
zlStatusE zl303xx_AprResetPerfStatistics
          (
             void *hwParams
          );

#endif

