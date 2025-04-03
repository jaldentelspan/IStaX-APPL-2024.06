

/******************************************************************************
*
*  $Id: 8410f584bd768792a15e60f6157c94bf8acedd47
*
*  Copyright 2006-2022 Microchip Technology Inc.
*  All rights reserved.
*
*  Module Description:
*     The zl303xx API relies on a protocol stack, probably provided by the
*     CPU's OS in order to implement the layer 2/3 transport mechanism.
*     The interface to the protocol stack might well be through a socket layer,
*     but could also be some other transport system.
*     This file defines an implementation of a transport mechanism using the
*     VxWorks socket library
*
******************************************************************************/

#ifndef _ZL303XX_VXW_SOCKET_TRANSPORT_H_
#define _ZL303XX_VXW_SOCKET_TRANSPORT_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************   INCLUDE FILES   ******************************************/
#include "zl303xx_DataTypes.h"
#include "zl303xx_SocketTransport.h"

/*****************   DATA TYPES   *********************************************/

/*****************   DATA STRUCTURES   ****************************************/

/*****************   EXPORTED GLOBAL VARIABLE DECLARATIONS   ******************/

/*****************   EXTERNAL FUNCTION DECLARATIONS   *************************/

Sint16T osSetMtuSize(Uint16T mtuSize, char *ifName);

/*****************   DEFINES   ************************************************/

#ifdef __cplusplus
}
#endif

#endif

