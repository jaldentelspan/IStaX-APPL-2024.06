



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
*     This file contains generic functions to configure, start, and stop an
*     example timing application consisting of one or more software modules. The
*     application structure is broken down into 3 major subcomponents:
*
*     - Clock: A hardware device capable generating a timing signal used for time
*       stamping.
*     - Port: A logical point of access to a network carrying timing traffic.
*     - Stream: A uni- or bi-directional connection between two endpoints in the
*       timing network (e.g., server to client).
*
*******************************************************************************/

#ifndef _ZL303XX_EXAMPLE_MAIN_H_
#define _ZL303XX_EXAMPLE_MAIN_H_

#ifdef __cplusplus
extern "C" {
#endif


/*****************   INCLUDE FILES   ******************************************/
#include "zl303xx_Global.h"
#include "zl303xx_Error.h"
#include "zl303xx_DeviceSpec.h"
#if defined APR_INCLUDED
#include "zl303xx_ExampleApr.h"
#endif

#if defined ZLS30361_INCLUDED
#include "zl303xx_Example36x.h"
#endif

#if defined ZLS30721_INCLUDED
#include "zl303xx_Example72x.h"
#endif

#if defined ZLS30701_INCLUDED
#include "zl303xx_Example70x.h"
#endif

#if defined ZLS30751_INCLUDED
#include "zl303xx_Example75x.h"
#endif

#if defined ZLS30731_INCLUDED
#include "zl303xx_Example73x.h"
#endif

#if defined ZLS30771_INCLUDED
#include "zl303xx_Example77x.h"
#endif

#if defined ZLS30390_INCLUDED
#include "zl303xx_ExamplePtp.h"
#include "zl303xx_ExamplePtpGlobals.h"
#endif

#if defined APR_INCLUDED && defined MUX_PTP_STREAMS_TO_APR_SLOTS
    /* Multiplexing for systems with more PTP stream connections than APR servers (masters) */
    #include "zl303xx_Apr.h"

    #if !defined NUM_APR_SLOTS_MUX
        #define NUM_APR_SLOTS_MUX ZL303XX_APR_MAX_NUM_MASTERS /* Must be ZL303XX_APR_MAX_NUM_MASTERS or less */
    #endif
    #if !defined STREAMS_PER_SLOT_MUX
        #define STREAMS_PER_SLOT_MUX (ZL303XX_PTP_NUM_STREAMS_MAX / NUM_APR_SLOTS_MUX)
    #endif
#endif  /* APR_INCLUDED && MUX_PTP_STREAMS_TO_APR_SLOTS*/

/*****************   DEFINES   ************************************************/
/*Values to stop calling zl303xx_SetExampleInterface automatically, -2 is ignored. -1 is custom str*/
#define ZL303XX_EXAMPLE_INTERFACE_NOT_SET     (-3)    /* Interface has not been selected yet (use a default) */
#define ZL303XX_EXAMPLE_INTERFACE_IGNORE      (-2)
#define ZL303XX_EXAMPLE_INTERFACE_CUSTOM_STR  (-1)
/*****************   DATA TYPES   *********************************************/

/* Detail levels for exampleAppDebugPrint() */
typedef enum {
   EXAMPLE_ADP_DETAIL_LEVEL_MINIMUM = 0,           /* low (minimum app information) */
   EXAMPLE_ADP_DETAIL_LEVEL_APP = 1,               /* medium (more app information) */
   EXAMPLE_ADP_DETAIL_LEVEL_COMPONENT = 2,         /* high (detailed app component information) */
} exampleAppDebugPrintDetailLevelE;

/*****************   DATA STRUCTURES   ****************************************/

typedef struct
{
#if defined APR_INCLUDED
   exampleAprStreamCreateS apr;
#endif

#if defined ZLS30390_INCLUDED
   examplePtpStreamCreateS ptp;
#endif

   zl303xx_BooleanE started;
} exampleAppStreamS;

typedef struct
{
#if defined ZLS30390_INCLUDED
   examplePtpPortCreateS ptp;
   Sint32T exampleInterface;                                            /* Used for calling zl303xx_SetExampleInterface() */
   char exampleInterfaceCustomStr[ZL303XX_MAX_EXAMPLE_INTERFACE_LENGTH];  /* Used for calling zl303xx_SetExampleInterfaceCustomStr() if exampleInterfaceIndex is -1 */

   zl303xx_BooleanE forceInterfaceAddrSet;  /* If true will force the chosen network interface to use requested address from ptp.config.localAddr during startup (see setExampleIpSrc) */
#endif
   zl303xx_BooleanE started;
} exampleAppPortS;

typedef struct
{
#if defined APR_INCLUDED
    exampleAprClockCreateS apr;
#endif

#if defined ZLS30361_INCLUDED
   example36xClockCreateS zl3036x;
#endif

#if defined ZLS30721_INCLUDED
   example72xClockCreateS zl3072x;
#endif

#if defined ZLS30701_INCLUDED
   example70xClockCreateS zl3070x;
#endif

#if defined ZLS30731_INCLUDED
   example73xClockCreateS zl3073x;
#endif

#if defined ZLS30751_INCLUDED
   example75xClockCreateS zl3075x;
#endif

#if defined ZLS30771_INCLUDED
   example77xClockCreateS zl3077x;
#endif

#if defined ZLS30390_INCLUDED
   examplePtpClockCreateS ptp;

   Sint32T exampleInterface;                                            /* Used for calling zl303xx_SetExampleInterface() */
   char exampleInterfaceCustomStr[ZL303XX_MAX_EXAMPLE_INTERFACE_LENGTH];  /* Used for calling zl303xx_SetExampleInterfaceCustomStr() if exampleInterfaceIndex is -1 */
#endif

   zl303xx_BooleanE pktRef;  /* Set FALSE for master, TRUE for slave/BC */

   zl303xx_BooleanE started;
} exampleAppClockS;

typedef struct
{
   exampleAppClockS  *clock;
   Uint32T            clockCount;

   exampleAppPortS   *port;
   Uint32T            portCount;

   exampleAppStreamS *stream;
   Uint32T            streamCount;
   Uint32T            srvId;
} exampleAppS;

/*****************   IMPORTED GLOBAL VARIABLE DECLARATIONS   ******************/

extern exampleAppS zlExampleApp;

extern zl303xx_ParamsS *zl303xx_Params0;
extern zl303xx_ParamsS *zl303xx_Params1;

extern Uint8T TARGET_DPLL;
extern Uint8T SYNCE_DPLL;
extern Uint8T CHASSIS_DPLL;
extern Uint8T NCO_ASSIST_DPLL;
#if defined ZLS30731_INCLUDED
extern Uint8T NCO_73X_ASSIST_DPLL;
extern Uint8T NCO_73X_ASSIST_DPLL;
extern Uint8T SPLIT_73X_XO_DPLL;
#endif



extern zl303xx_BooleanE reset1588HostRegisterOnClockCreate;


/*****************   EXTERNAL FUNCTION DECLARATIONS   *************************/

#ifdef ZLS30390_INCLUDED
/* Example PTP configurations */
zlStatusE examplePtpMultiMaster(void);
zlStatusE examplePtpMultiSlave(void);

zlStatusE examplePtpUniNegMaster(void);
zlStatusE examplePtpUniNegSlave(void);

zlStatusE examplePtpUniNegSlaveHybrid(void);

zlStatusE examplePtpUniNegMaster2(void);
zlStatusE examplePtpUniNegMaster3(void);
zlStatusE examplePtpUniNegMaster4(void);
zlStatusE examplePtpUniNegSlave_4Streams(void);

zlStatusE examplePtpMultiBC(void);
zlStatusE examplePtpMultiBCSlave(void);

/* G8275.1 (edition 1) */
zlStatusE examplePtpTelecomPhaseG8275p1Master(void);
zlStatusE examplePtpTelecomPhaseG8275p1Bc(void);
zlStatusE examplePtpTelecomPhaseG8275p1BcSlave(void);
zlStatusE examplePtpTelecomPhaseG8275p1Slave(void);
zlStatusE examplePtpTelecomPhaseG8275VirtualPortAdd(zl303xx_PtpClockHandleT clockHandle);

/* G8275.1 (edition 2) */
zlStatusE examplePtpTelecomPhaseG8275p1v2Master(void);
zlStatusE examplePtpTelecomPhaseG8275p1v2Bc(void);
zlStatusE examplePtpTelecomPhaseG8275p1v2BcSlave(void);
zlStatusE examplePtpTelecomPhaseG8275p1v2Slave(void);

/* G8275.2 */
zlStatusE examplePtpTelecomPhaseG8275p2Master(void);
zlStatusE examplePtpTelecomPhaseG8275p2Bc(void);
zlStatusE examplePtpTelecomPhaseG8275p2Slave(void);

/* G8275.2 with Monitoring*/
zlStatusE examplePtpTelecomPhaseG8275p2MasterWithMonitoring(void);
zlStatusE examplePtpTelecomPhaseG8275p2SlaveWithMonitoring(void);
zlStatusE examplePtpTelecomPhaseG8275p2BCWithMonitoring(void);


#include "zl303xx_ExamplePowerProfile.h"

zlStatusE examplePtpPeerDelayMaster(void);
zlStatusE examplePtpPeerDelaySlave(void);

void setExampleMulticastIpDest(char const *destIP);
void setExampleUniIpSlaveDest(char const *destIP);
zlStatusE exampleConfig(zl303xx_BooleanE isUnicast, zl303xx_BooleanE slaveCapable, zl303xx_PtpVersionE ptpVersion);
zlStatusE examplePtpPortAdd(Uint16T portNumber, Uint8T const *localAddr);
void setExampleIpSrc(const char *srcIp);

zlStatusE examplePtpTelecomMaster(void);
zlStatusE examplePtpTelecomMaster2(void);
zlStatusE examplePtpTelecomMaster3(void);
zlStatusE examplePtpTelecomMaster4(void);
zlStatusE examplePtpTelecomMaster5(void);
zlStatusE examplePtpTelecomMaster6(void);
zlStatusE examplePtpTelecomMaster7(void);
zlStatusE examplePtpTelecomMaster8(void);
zlStatusE examplePtpTelecomMaster9(void);

zlStatusE examplePtpTelecomSlave(void);

zlStatusE examplePtpTelecomMultiSlave(void);
zlStatusE examplePtpTelecomMultiSlave2(void);
zlStatusE examplePtpTelecomMultiSlave3(void);
zlStatusE examplePtpTelecomMultiMaster(void);

zlStatusE examplePtpTelecomBCMaster(void);
zlStatusE examplePtpTelecomBC(void);
zlStatusE examplePtpTelecomBCSlave(void);

zlStatusE examplePtpTwoClock(void);
zlStatusE examplePtpTwoClockMaster(void);
zlStatusE examplePtpTwoClockSlave(void);
#endif

#ifdef APR_INCLUDED
zlStatusE exampleAprMain(void);
zlStatusE exampleAprHybrid(void);
#endif



/* Shutdown function */
zlStatusE exampleShutdown(void);

/* Application Management */
zlStatusE exampleEnvInit(void);
zlStatusE exampleEnvClose(void);
zlStatusE exampleAppStructInit(Uint32T numClocks,  Uint32T numPorts,
                               Uint32T numStreams, exampleAppS *pApp);
zlStatusE exampleAppStructFree(exampleAppS *pApp);
zlStatusE exampleAppStart(exampleAppS *pApp);
zlStatusE exampleAppStartClock(exampleAppS *pApp, Uint32T indx);
zlStatusE exampleAppStartPort(exampleAppS *pApp, Uint32T indx);
zlStatusE exampleAppStartStream(exampleAppS *pApp, Uint32T indx);
zlStatusE exampleAppStop(exampleAppS *pApp);
zlStatusE exampleAppDebugPrint(exampleAppS *pApp, const char* prefixStr, exampleAppDebugPrintDetailLevelE detailLevel);
zl303xx_BooleanE exampleAppIsRunning(void);

zlStatusE exampleAppClockInterfaceSet(exampleAppClockS *pClock, Sint32T ifIndex, const char *ifNameCustom);

zlStatusE exampleAppPortInterfaceSet(exampleAppPortS *pPort, Sint32T ifIndex, const char *ifNameCustom);
zlStatusE exampleAppPortInterfaceGet(exampleAppPortS *pPort, Sint32T *ifIndex, char *ifName, Uint32T ifNameSize);
zlStatusE exampleAppPortAddressFromInterface802p3(exampleAppPortS *pPort);
zlStatusE exampleAppPortAddressFromInterfaceIPv4(exampleAppPortS *pPort);
zlStatusE exampleAppPortAddressFromInterfaceIPv6(exampleAppPortS *pPort);
zlStatusE exampleAppPortAddressSet(exampleAppPortS *pPort, const char *addrStr, zl303xx_BooleanE bForceInterfaceAddrSet);

zlStatusE exampleAppStreamAddressSet(exampleAppStreamS *pStream, const char *addrStr);

Uint16T exampleAppMapAprServerToPtpStream(Uint16T serverId);
Uint16T exampleAppMapPtpStreamToAprServer(Uint16T ptpStreamHandle);

zlStatusE exampleReset1588HostRegisterOnClockCreateSet(zl303xx_BooleanE set);
zl303xx_BooleanE exampleReset1588HostRegisterOnClockCreateGet(void);

#if defined ZLS30361_INCLUDED || defined ZLS30701_INCLUDED || defined ZLS30721_INCLUDED || defined ZLS30731_INCLUDED || defined ZLS30751_INCLUDED || defined ZLS30771_INCLUDED || defined CUSTOMDEV_INCLUDED
zlStatusE exampleAppClockDpllSetDeviceMode(
            exampleAppClockS *pClock,
            zl303xx_DpllClockModeE dpllClockModeToSet,
            zl303xx_RefIdE genericDpllInputRef);
#endif

#ifdef __cplusplus
}
#endif

#endif /* MULTIPLE INCLUDE BARRIER */
