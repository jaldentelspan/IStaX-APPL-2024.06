

/*******************************************************************************
*
*  $Id: 8410f584bd768792a15e60f6157c94bf8acedd47
*
*  Copyright 2006-2022 Microchip Technology Inc.
*  All rights reserved.
*
*  Module Description:
*     Ohly-style timestamp socket setup and read..
*
*******************************************************************************/

#ifndef _zl303xx_LinuxOhlyTS_H_
#define _zl303xx_LinuxOhlyTS_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************   INCLUDE FILES   ******************************************/

#include "zl303xx_Global.h"
#include "zl303xx_Error.h"
#include "zl303xx_ExamplePtpSocket.h"

#include <net/ethernet.h>


typedef struct ether_header EtherHead_t;
typedef struct PtpEthFrameStruct
{
    EtherHead_t ethHdr;
    Uint8T      payload[1600];
} PtpEthFrame_t;


/*****************   EXPORTED FUNCTION DEFINITIONS   **************************/

zlStatusE zl303xx_ConfigIPV4Address(const char* interface, const char* srcIpAddressP);
zlStatusE zl303xx_ConfigOhlyHWTimestamping(char const * ifName, Sint32T fd);
zlStatusE zl303xx_RetrieveOhlyHWTimestamp(examplePtpSocketTblS *pTblEntry, Uint32T flags);







#ifdef __cplusplus
}
#endif

#endif /* MULTIPLE INCLUDE BARRIER */

