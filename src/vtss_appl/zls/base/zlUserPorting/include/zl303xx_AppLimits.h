

/******************************************************************************
*
*  $Id: 8410f584bd768792a15e60f6157c94bf8acedd47
*
*  Copyright 2006-2022 Microchip Technology Inc.
*  All rights reserved.
*
*  Module Description:
*     Application configuration limits for a running system
*
******************************************************************************/

#ifndef _ZL303XX_APP_LIMITS_H_
#define _ZL303XX_APP_LIMITS_H_

#ifdef __cplusplus
extern "C" {
#endif


/*****************   INCLUDES   ***********************************************/


/*****************  LOCAL APP DEFINES   ************************************************/
/*
    Top-level APP defines are used to derive lower-level defines. Different boards 
    should modify the top-level defines only, unless a special application is required. 
    The APP defines for particular application can be defined in this file under a custom
    define, or by using compiler command-line defines. See examples below.
    

    APP_MAX_                    App top-level defines are used to set lower defines ...
        |                       
        |--- ZL303XX_PTP_NUM_     ... used by zlPtp (note obj releases have fixed limits)
        `--- ZL303XX_MAX_         ... used by zlConfigTSA, zlReportTSA

    Where,
    
    - APP_MAX_PHY_PORTS:
        An app port is defined as a physical intererface that can send and receive 
        packet timing information. See APP_MAX_PTP_PORTS for logical or virtual 
        interfaces.
    
    - APP_MAX_PTP_CLOCKS: 
        An app clock is defined as a device that can provide timing information in a 
        timing domain (timescale).
        
    - APP_MAX_PTP_PORTS: 
        An app port is defined as a local interface that can send and receive packet 
        timing information. It can be a logical or virtual interface. See 
        APP_MAX_PHY_PORTS for physical interfaces.
        
    - APP_MAX_PTP_STREAMS:
        An app stream is defined as a remote endpoint capable of providing packet
        timing information (e.g. a PTP grandmaster)
*/

#if defined _ZL303XX_ZLE30360_BOARD || defined _ZL303XX_ZLE1588_BOARD 
    /* For these EVBs there are 4 physical ports */
    #define APP_MAX_PHY_PORTS    (4)
    
    #define APP_MAX_PTP_CLOCKS   (2)
    #define APP_MAX_PTP_PORTS    (4)
    #define APP_MAX_PTP_STREAMS  (128)
#endif

#if defined _ZL303XX_FMC_BOARD || defined _ZL303XX_NTM_BOARD
    #define APP_MAX_PHY_PORTS    (1)
    
    #define APP_MAX_PTP_CLOCKS   (1)
    #define APP_MAX_PTP_PORTS    (128)
    #define APP_MAX_PTP_STREAMS  (1024)
#endif



/*****************   USER BOARD DEFINES *********************************************/


/* -------------------------------------------------------------------------------- */
/*                                                                                  */
/*      MSCC TODO: Insert user board limits here or use compiler defines            */
/*                  (e.g. gcc -DAPP_MAX_PTP_CLOCKS=4)                               */
/*                                                                                  */
/* -------------------------------------------------------------------------------- */
   


/*****************   DEFAULT DEFINES ************************************************/

/* Defaults if not overriden above */
#if !defined APP_MAX_PTP_CLOCKS
    #warning MSCC: APP_MAX_PTP_CLOCKS not defined, using a default value
    #define APP_MAX_PTP_CLOCKS  (1)
#endif
#if !defined APP_MAX_PHY_PORTS
    #warning MSCC: APP_MAX_PHY_PORTS not defined, using a default value
    #define APP_MAX_PHY_PORTS   (4)
#endif
#if !defined APP_MAX_PTP_PORTS
    #warning MSCC: APP_MAX_PTP_PORTS not defined, using a default value
    #define APP_MAX_PTP_PORTS   (4)
#endif
#if !defined APP_MAX_PTP_STREAMS
    #warning MSCC: APP_MAX_PTP_STREAMS not defined, using a default value
    #define APP_MAX_PTP_STREAMS (8)
#endif


/*****************   DEFINES USED BY API ************************************************/

/* zlUserConfigTSA and zlUserReportingTSA defines */
/* ---------------------------------------------- */
#if !defined ZL303XX_MAX_PTP_CLOCKS
    #define ZL303XX_MAX_PTP_CLOCKS            (APP_MAX_PTP_CLOCKS)
#endif
#if !defined ZL303XX_MAX_PHY_PORTS
    #define ZL303XX_MAX_PHY_PORTS             (APP_MAX_PHY_PORTS)
#endif
#if !defined ZL303XX_MAX_PTP_PORTS
    #define ZL303XX_MAX_PTP_PORTS             (APP_MAX_PTP_PORTS)
#endif
#if !defined ZL303XX_MAX_PTP_STREAMS
    #define ZL303XX_MAX_PTP_STREAMS           (APP_MAX_PTP_STREAMS)
#endif

/* ... Derived from above with equal allocation ... */
#if !defined ZL303XX_MAX_PTP_PORTS_PER_CLOCK
    #define ZL303XX_MAX_PTP_PORTS_PER_CLOCK   (ZL303XX_MAX_PTP_PORTS / ZL303XX_MAX_PTP_CLOCKS)
#endif
#if !defined ZL303XX_MAX_PTP_STREAMS_PER_PORT
    #define ZL303XX_MAX_PTP_STREAMS_PER_PORT  (ZL303XX_MAX_PTP_STREAMS / ZL303XX_MAX_PTP_PORTS)
#endif


/* zlPtp defines */
/* ------------- */
#if !defined ZL303XX_PTP_NUM_CLOCKS_MAX
    #define ZL303XX_PTP_NUM_CLOCKS_MAX        (APP_MAX_PTP_CLOCKS)
#endif
#if !defined ZL303XX_PTP_NUM_PORTS_MAX
    #define ZL303XX_PTP_NUM_PORTS_MAX         (APP_MAX_PTP_PORTS)
#endif
#if !defined ZL303XX_PTP_NUM_STREAMS_MAX
    #define ZL303XX_PTP_NUM_STREAMS_MAX       (APP_MAX_PTP_STREAMS)
#endif


/* zlApr defines */
/* ------------- */
/* 

See zl303xx_Apr.h defines:
    - ZL303XX_APR_MAX_NUM_DEVICES
    - ZL303XX_APR_MAX_NUM_MASTERS

Note: for "obj" builds above defines are fixed

*/

#ifdef __cplusplus
}
#endif

#endif
