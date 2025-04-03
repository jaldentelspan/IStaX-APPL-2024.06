

/*******************************************************************************
*
*  $Id: 8410f584bd768792a15e60f6157c94bf8acedd47
*
*  Copyright 2006-2022 Microchip Technology Inc.
*  All rights reserved.
*
*  Module Description:
*     Time stamp logging/debug data types.
*
*******************************************************************************/

#ifndef ZL303XX_TS_DEBUG_H
#define ZL303XX_TS_DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

/*****************   INCLUDE FILES   ******************************************/
#include "zl303xx_Global.h"

/*****************   DATA TYPES   *********************************************/
typedef struct
{
    /** Timestamp pair data direction:
        1 if from "forward" path (e.g. transmitted from a Server sending SYNC).
        0 if from "reverse" path (e.g. transmitted from a Client sending DELAY_REQ). 
    */ 
    Uint8T fwd;
    
    /** Sequence number of the timestamp pair (e.g. SYNC sequenceId, or DELAY_REQ sequenceId).
        May be zero if not available. */
    Uint16T sequenceNum;
    
    /** Stream handle (PTP) or Server ID (APR) */
    Uint32T streamNum;
    
    /** Transmit timestamp (48-bit seconds with 16-bit epoch in .hi) */
    Uint64S txTs; 
    
    /** Receive timestamp (48-bit seconds with 16-bit epoch in .hi) */
    Uint64S rxTs; 
    
    /** Correction field in PTP scaledNansecond units (i.e. nanoseconds 
        multiplied by 2^16). May be zero if not available. */
    Uint64S corr; 
    
    /** Transmit timestamp sub-second in nanoseconds (max 999,999,999 ns) */
    Uint32T txSubSec;
    
    /** Receive timestamp sub-second in nanoseconds (max 999,999,999 ns)  */
    Uint32T rxSubSec;
    
} zl303xx_TsLogDataS;

typedef Sint32T (*swFuncPtrTSLogging)(zl303xx_TsLogDataS *tsData);

#ifdef __cplusplus
}
#endif

#endif   /* MULTIPLE INCLUDE BARRIER */
