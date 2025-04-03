

/******************************************************************************
*
*  $Id: 8410f584bd768792a15e60f6157c94bf8acedd47
*
*  Copyright 2006-2022 Microchip Technology Inc.
*  All rights reserved.
*
*  Module Description:
*     Header for remote SPI read / write operations
*
******************************************************************************/

#ifndef _ZL303XX_SPI_SOCKET_H_
#define _ZL303XX_SPI_SOCKET_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************   INCLUDE FILES   ******************************************/
#include "zl303xx_DataTypes.h"

#include "zl303xx_SocketTransport.h"

/*****************   DEFINES   ************************************************/
#ifndef SOCKET_ERROR
   #define SOCKET_ERROR ERROR
#endif

#ifndef INVALID_SOCKET
   #define INVALID_SOCKET ERROR
#endif

/* dummy definitions for zl303xx structure that isn't currently defined */
extern Uint32T remoteDebug;

#define REMOTE_SPI_VER  1
#define REMOTE_SPI_MAX_RETRIES   10
#define REMOTE_SPI_WAIT_MS       100

#define REMOTE_DEBUG(str, a, b, c, d, e, f)    if (remoteDebug != 0) OS_LOG_MSG(str, a, b, c, d, e, f)

#ifndef REMOTE_SOCK_TYPE
   #define REMOTE_SOCK_TYPE         SOCK_STREAM
#endif


/*****************   DATA TYPES   *********************************************/

typedef enum
{
   REMOTE_SPI_ERROR,
   REMOTE_SPI_READ,
   REMOTE_SPI_WRITE,
   REMOTE_SPI_READ_ACK,
   REMOTE_SPI_WRITE_ACK
} spiSocketActionE;

/*****************   DATA STRUCTURES   ****************************************/

typedef struct
{
   Uint16T ver;
   Uint16T seq;

   /* following variable will take types spiSocketActionE, but is defined as Uint32T to
      fix size to avoid endian conversion issues */
   Uint32T action;

   Uint32T addr;
   Uint32T data;
} spiSocketMsgS;

/*****************   EXPORTED GLOBAL VARIABLE DECLARATIONS   ******************/

/*****************   EXTERNAL FUNCTION DECLARATIONS   *************************/

#ifdef __cplusplus
}
#endif

#endif

