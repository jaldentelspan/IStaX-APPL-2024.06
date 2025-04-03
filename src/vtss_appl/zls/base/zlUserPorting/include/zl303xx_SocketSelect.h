

/******************************************************************************
*
*  $Id: 8410f584bd768792a15e60f6157c94bf8acedd47
*
*  Copyright 2006-2022 Microchip Technology Inc.
*  All rights reserved.
*
*  Module Description:
*     Functions to support the select() operation on sockets or other files
*
******************************************************************************/

#ifndef _ZL303XX_SOCKET_SELECT_H_
#define _ZL303XX_SOCKET_SELECT_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************   INCLUDE FILES   ******************************************/
#include "zl303xx_DataTypes.h"
#include "zl303xx_SocketHeaders.h"

/*****************   DEFINES   ************************************************/
#if defined(ZL_LNX_CODESOURCERY) || defined(ZL_LNX_PICO)
#include <sys/select.h>
#include <sys/types.h>
#endif

#define FD_SELECT_SET  fd_set

/*****************   DATA TYPES   *********************************************/

/*****************   DATA STRUCTURES   ****************************************/

typedef struct
 {
   Sint32T sec;  /* seconds */
   Sint32T usec; /* microseconds */
 } timeOutS;

/*****************   EXPORTED FUNCTIONS   *************************************/

/*******************************************************************************
  Function Name:  sockFdSetAdd

  Details:  Adds a socket file descriptor into an FD set

  Parameters:
   [in]  pSktData    An identifier for a previously established and configured socket connection.
   [in]    fdSet       Pointer to a SOCK_FD_SET

  Return Value:  0 (OK) or -1 if an error occurs

*******************************************************************************/
Sint32T sockFdSetAdd(
   zl303xx_SocketDataS *pSktData,
   FD_SELECT_SET *fdSet
   );


/*******************************************************************************
  Function Name:   sockFdSetAdd

  Details:  Removes a socket file descriptor from an FD set

  Parameters:
   [in]  pSktData    An identifier for a previously established and configured socket connection.
   [in]   fdSet       Pointer to a SOCK_FD_SET to remove

  Return Value:  0 (OK) or -1 if an error occurs

*******************************************************************************/
Sint32T sockFdSetRemove(
   zl303xx_SocketDataS *pSktData,
   FD_SELECT_SET *fdSet
   );


/*******************************************************************************
  Function Name:   sockFdIsInSet

  Details: Determines whether a particular socket file descriptor is in an FD set

  Parameters:
   [in]  pSktData    An identifier for a previously established and configured socket connection.
   [in]    fdSet       Pointer to a SOCK_FD_SET

  Return Value:  0 (OK) or -1 if an error occurs

  Notes:  Does not perform validity checks on its parameters

*******************************************************************************/
Sint32T sockFdIsInSet(
   zl303xx_SocketDataS *pSktData,
   FD_SELECT_SET *fdSet
   );


/*******************************************************************************
  Function Name:  sockSelectRd

  Details:   Blocks indefinitely until one of the sockets in fdSet becomes ready due to
   data becoming available.

  Parameters:
   [in]  fdSet       Pointer to a SOCK_FD_SET of socket descriptors of interest

   [out]  fdSet       Pointer to a SOCK_FD_SET that indicates sockets that are ready
               to be read

  Return Value:  0 (OK) or -1 if an error occurs

*******************************************************************************/
Sint32T sockSelectRd(
   FD_SELECT_SET *fdSet
   );



#ifdef __cplusplus
}
#endif

#endif

