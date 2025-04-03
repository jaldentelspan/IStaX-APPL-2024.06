

/******************************************************************************
*
*  $Id: 8410f584bd768792a15e60f6157c94bf8acedd47
*
*  Copyright 2006-2022 Microchip Technology Inc.
*  All rights reserved.
*
*  Module Description:
*     The actual OS headers are included in this file to provide ANSI standard
*     functions etc. and definitions for the OS specific functions as required
*     by the porting layer.
*
******************************************************************************/

#ifndef _ZL303XX_SOCKET_HEADERS_H_
#define _ZL303XX_SOCKET_HEADERS_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************   INCLUDE FILES   ******************************************/
#include "zl303xx_OsHeaders.h"

#if defined (OS_VXWORKS)
   /* include the VxWorks specific headers */
   #include <sockLib.h>
   #include <socket.h>
   #include <inetLib.h>
   #include <ioctl.h>
   #include <netinet/if_ether.h>
#ifdef __VXWORKS_65
   #include <arpa/inet.h>
#else
   #include <net/inet.h>
#endif

#elif defined OS_LINUX
   #include <sys/select.h>

#endif

#ifdef __cplusplus
}
#endif

#endif
