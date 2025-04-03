

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
*     This file defines an interface for a socket based implementation.
*
******************************************************************************/

#ifndef _ZL303XX_SOCKET_TRANSPORT_H_
#define _ZL303XX_SOCKET_TRANSPORT_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************   INCLUDE FILES   ******************************************/
#include "zl303xx_Global.h"
#include "zl303xx_SocketHeaders.h"
#include "zl303xx_Os.h"               /* for OS_SINT64 and OS_UINT64 */

#ifdef SOCPG_PORTING
#include "inet.h"
#endif
#ifdef __VXWORKS_65
    #include <muxLib.h>
#endif

#ifdef OS_LINUX
    #ifdef ZL_LNX_DENX
    #include <sys/socket.h>
    #endif
    #ifndef _ZL303XX_SKIP_LINUX_SOCKET_MOD
        #define _ZL303XX_SKIP_LINUX_SOCKET_MOD
        #include <linux/socket.h>
        #include <linux/types.h>
        #include <netinet/in.h>       /* To define struct in_addr  */
        #include <net/ethernet.h>     /* To define `struct ether_header  */

        #if defined (ZL_LNX_DENX) && !defined(ZL_LNX_CODESOURCERY) && !defined(MPC8260)
            #define NEED_BE64
        #endif
        #if defined (NEED_BE64)
            #if defined (ZL_LNX_DENX_MIPS32)
                /* from asm/types.h (not defined because of ansi flag) */
                #include "zl303xx_Int64.h"
                typedef OS_SINT64 __s64;
                typedef OS_UINT64 __u64;
            #endif
            /* ZL - Needed in big_endian.h and not included due to the use of the ansi flag */
            typedef __u64 __bitwise __be64;
            typedef __u64 __bitwise __le64;
        #endif
    #endif

    #ifdef ZL_LNX_CODESOURCERY
        #include <sys/socket.h>
        #include <netinet/in.h>         /* To define struct in_addr  */
        #include <net/ethernet.h>     /* To define `struct ether_header  */
    #endif
#endif

/* Requires a socket header file to provide suitable definitions
   Winsock.h or an equivalent for the target OS should be included before this
   in the implementation file. Can't be included here or it won't be OS independent
   */

/*****************   DEFINES   ************************************************/
/* The maximum length for an interface name string (excluding null terminator) */
#define ZL303XX_IFACE_NAME_MAX_LEN 20

/* Maximum length of an internet address (IPv6) in ASCII format including NULL terminator */
#define ZL303XX_INET_ADDR_MAX_LEN   46

/* Length of an Internet addresses in bytes. */
#define ZL303XX_SOCKET_ADDR_IPV4_LEN   4
#define ZL303XX_SOCKET_ADDR_MAX_LEN   16

/* Length of a MAC address */
#define ZL303XX_MAC_ADDRESS_BYTES  6

/* Length of the UUID string returned by the socket layer */
#define ZL303XX_UUID_NUM_BYTES  6

/* Socket time to live values. To ensure packets do not get delivered outside
 * the current subdomain, set these to 1. */
#define ZL303XX_SOCKET_MULTICAST_TTL  64
#define ZL303XX_SOCKET_UNICAST_TTL    64

/* Socket select() timeout values. */
#define ZL303XX_SOCKET_SELECT_TIMEOUT_SEC    0
#define ZL303XX_SOCKET_SELECT_TIMEOUT_USEC   500000

/* The base name to use when searching for a suitable ethernet interface */

#if defined OS_VXWORKS
#if defined(MPC8260)
   #define INTERFACE_NUMBER                 1
   #define INTERFACE_PREFIX                 "motfcc"
#elif defined(MPC8313)
   #define INTERFACE_NUMBER                 0
   #define INTERFACE_PREFIX                 "mottsec"
#else
   #warning Target CPU not defined
#endif
#endif

#if defined (OS_LINUX)
    #define INTERFACE_NUMBER                 1
    #define INTERFACE_PREFIX                 "eth"
#endif


/*****************   DATA TYPES   *********************************************/
typedef Sint32T SOCKET_FD;

typedef struct in_addr zl303xx_RemoteAddrT;
typedef struct in_addr zl303xx_LocalAddrT;

typedef enum
{
   ZL303XX_TRANSPORT_UNICAST,
   ZL303XX_TRANSPORT_MULTICAST,
   ZL303XX_TRANSPORT_BROADCAST,
   ZL303XX_TRANSPORT_RAWCAST
}  zl303xx_TransportTypeE;

/*****************   DATA STRUCTURES   ****************************************/

/* Structure containing information that is needed by the socket transport layer
 * to send packets. The format of the information here is private to the socket
 * layer implementation. All elements are in network byte order. */
typedef struct
{
   Uint8T addrFamily;                   /* The address family to use */
   Uint8T localIpAddr[ZL303XX_SOCKET_ADDR_MAX_LEN];  /* The IP address of the local interface */
   Uint8T mcastAddr[ZL303XX_SOCKET_ADDR_MAX_LEN];    /* The multicast address to bind to (if any) */
   Uint16T port;                        /* The UDP port to which to bind (in host byte order) */
   zl303xx_TransportTypeE type;           /* Unicast, Multicast or Broadcast */
   char ifaceName[ZL303XX_IFACE_NAME_MAX_LEN+1]; /* The name of the local interface to use */
   Uint8T macAddr[ZL303XX_MAC_ADDRESS_BYTES];  /* The hardware address of the local interface */
   Uint32T netMask;                     /* The subnet mask for the local interface */
   SOCKET_FD socketFd;                  /* File descriptor for the socket */
   struct ifnet *pIf;                   /* network interface entity */
#ifdef __VXWORKS_65
   END_OBJ * pEnd;                      /* For 6.5 Raw socket we require a reference to the END object for the interface */
   void * pCookie;                      /* For 6.5 Raw socket support we require this network cookie struct pointer */
#endif
} zl303xx_SocketDataS;

/* Structure used by application to pass in initialisation parameters for the socket
   creation process */
typedef struct
{
   zl303xx_TransportTypeE ifType;      /* The type of interface to create  */

   struct in_addr srcAddr;  /* Local address to bind socket to. Zero fill if not using multiple IPs. */
   struct in_addr destAddr; /* Multicast/broadcast address for receiving/transmitting. */

   char srcIpAddr[ZL303XX_INET_ADDR_MAX_LEN]; /* Deprecated version of above member (dot notation). */
   char destIpAddr[ZL303XX_INET_ADDR_MAX_LEN]; /* Deprecated version of above member (dot notation). */
} sockAddrDataS;

#ifdef ZL_INCLUDE_IPV6_SOCKET
#define ZL303XX_SOCKET_ADDR_IPV6_LEN  16
/* Structure used by application to pass in initialisation parameters for an
 * IPv6 socket. */
typedef struct
{
   zl303xx_TransportTypeE ifType;      /* The type of interface to create  */

   Uint8T srcAddr[ZL303XX_SOCKET_ADDR_IPV6_LEN];   /* The IP address of the local interface */
   Uint8T destAddr[ZL303XX_SOCKET_ADDR_IPV6_LEN];  /* The multicast address to bind to (if any) */
} sockAddrData6S;
#endif

/*****************   EXPORTED GLOBAL VARIABLES   ******************************/
extern Sint32T Zl303xx_LastSocketErr;

/*******************************************************************************

  Function Name:    sockCreateNonBlock

  Details:   Creates a non-blocking UDP socket providing a bidirectional communication path
   to the ZL303XX_ device.

  Parameters:
   [in]  ifName         Pointer to the name of the interface to use or NULL to use the
                  default interface
    [in]    initData       structure of type sockAddrDataS containing initialisation
                  data (see above)
    [in]    port           Port number to create

 Structure inputs:

      srcIpAddr       The source IP address as a string (format "aaa.bbb.ccc.ddd").
       destIpAddr     The destination IP address as a string (format "aaa.bbb.ccc.ddd").

    [out]  pSktData      Pointer to a structure containing information used to access the
                 connection.

  Return Value:   0 (OK) or -1 if an error occurs

*******************************************************************************/

Sint32T sockCreateNonBlock(zl303xx_SocketDataS *pSktData, char const *ifName, sockAddrDataS *initData, Uint16T port);


/*******************************************************************************

  Function Name:   sockDestroy

  Details:   Destroys a previously opened communication path

  Parameters:
   [in]   pSktData     Information about the connection.

  Return Value:
   0 (OK) or -1 if an error occurs
*******************************************************************************/

Sint32T sockDestroy(zl303xx_SocketDataS *pSktData);


/*******************************************************************************
  Function Name:  sockSend

  Details:   Sends a buffer to the previously configured socket connection

  Parameters:
   [in]   pSktData    A data structure for a previously established and configured socket.
    [in]   destAddr    The destination address to send the packets
    [in]   buf         A pointer to the buffer to send
    [in]   nBytes      The number of octets to send

    [out]   nBytes      The number of bytes actually sent

  Return Value:  The number of bytes written or -1 if an error occurs

*******************************************************************************/
Sint32T sockSend(
   zl303xx_SocketDataS *pSktData,
   zl303xx_RemoteAddrT *destAddr,
   Uint8T *buf,
   Uint32T *buflen
   );


/*******************************************************************************
  Function Name:   sockRecv

  Details:   Performs a read on the socket connection

  Parameters:
   [in]   pSktData    A data structure for a previously established and configured socket.
    [in]   buf         A pointer to the buffer to write data into
    [in]   buflen      The length of the buffer

    [out]
    [out]  buf         Filled with the received data
    [out]  buflen      Set to the length of the received data
    [out]   remoteAddr  The remote address from which the packet was received
    [out]   remoteAddrLen  The number of bytes in the remote address

  Return Value:  The number of bytes read or -1 if an error occurs

  Notes:   It is assumed in this application that the socket is configured as non-blocking

*******************************************************************************/
Sint32T sockRecv(zl303xx_SocketDataS *pSktData, Uint8T *buf, Uint32T buflen,
                 zl303xx_RemoteAddrT *remoteAddr, Uint16T *remoteAddrLen);


/*******************************************************************************
  Function Name:  sockSetIpTosValue

  Details:   Sets the Type of Service (TOS) field in the IP4 header

  Parameters:
   [in]   pSktData    A data structure for a previously established and configured socket.
    [in]    ipTos       The Tos byte to use.
               This byte is interpreted differently in old and new systems:
               In older systems it consists of a precedence value (bits 7-5) and
               4 type-of-service bits (bits 4-1) of which only one can be set.
               In newer systems it consists of a Differentiated Services Code Point (DSCP)
               (bits 7-2) and two Explicit Congestion Notification bits (bits 1-0)

  Return Value:   0 (OK) or -1 if an error occurs

  Notes:   This function is very specific to IPv4 and only needs to be ported if this
   functionality is required.

*******************************************************************************/
Sint32T sockSetIpTosValue(zl303xx_SocketDataS *pSktData, Uint8T ipTos);

/*******************************************************************************
  Function Name:   sockGetHeaderLen

  Details:   Returns the length of the header that will be prepended to the packet on the
   specified connection

  Parameters:
   [in]   pSktData    A data structure for a previously established and configured socket.

    [out]   headLen     The length of the added header

  Return Value:   0 (OK) or -1 if an error occurs

*******************************************************************************/
Sint32T sockGetHeaderLen(
   zl303xx_SocketDataS *pSktData,
   Uint32T *headLen
   );


/*******************************************************************************
  Function Name:  sockGetPhysicalLayerProtocol

  Details:   Obtains a description string for the physical layer in use

  Parameters:
   [in]  pSktData    A data structure for a previously established and configured socket.

    [out]  physicalLayerString  A pointer to a static, constant string describing the
            physical layer protocol.

  Return Value:   0 (OK) or -1 if an error occurs

*******************************************************************************/
Sint32T sockGetPhysicalLayerProtocol(
      zl303xx_SocketDataS *pSktData,
      const char **physicalLayerString);

/*******************************************************************************
  Function Name:   sockGetIfaceName

  Details:   Gets the name of the interface to be used by this socket

  Parameters:
   [in]   pSktData    A data structure for a previously established and configured socket.

    [out]   ifaceName   Pointer to a buffer to receive the interface name string

  Return Value:   0 (OK) or -1 if an error occurs
*******************************************************************************/
Sint32T sockGetIfaceName(
   zl303xx_SocketDataS *pSktData,
   char *ifaceName
   );

/*******************************************************************************
  Function Name:   sockGetAddr

  Details:   Returns the protocol address information for the specified socket

  Parameters:
   [in]   pSktData    An identifier for a previously established and configured connection.
    [in]    addr        pointer to an octet array to receive the address information
    [in]    size        Size of the addr buffer, i.e. maximum size

    [out]   addrFamily  Address family of the address.
               For this socket implementation it is always AF_INET
    [out]   addr        pointer to an octet array to receive the address information
               The address is not null terminated.
    [out]   size        Number of bytes in the address.

  Notes:  0 (OK) or -1 if an error occurs

*******************************************************************************/
Sint32T sockGetAddr(zl303xx_SocketDataS *pSktData, Uint8T *addrFamily, Uint8T *addr, Uint16T *size);

/*******************************************************************************
  Function Name:  sockMakeAddrFromLinkId

  Details:   Converts an address identifier in socket specific format into an ASCII string

  Parameters:
   [in]   linkId      Pointer to a structure to hold the link identifier.
               In this implementation it is of type "struct in_addr"
    [in]   addrStr     Pointer to a buffer to hold the address string
    [in]   addrSize    Size of the input buffer

    [out]   addrStr     Will be filled with the address in ASCII format. Not NULL terminated
    [out]   addrSize    Set to the number of ASCII characters in the address

  Return Value:   0 (OK) or -1 if an error occurs

*******************************************************************************/

Sint32T sockMakeAddrFromLinkId(zl303xx_RemoteAddrT *linkId, Uint8T *addrStr, Uint16T *size);


/*******************************************************************************
  Function Name:   sockAllocLinkId

  Details:   Allocates space for a device specific address structure

  Parameters:
   [out]   Pointer to a dynamically allocated device address structure

  Return Value:   0 (OK) or -1 if an error occurs

  (Deprecated)  This function is no longer used by the API.

*******************************************************************************/

Sint32T sockAllocLinkId(zl303xx_RemoteAddrT **linkId);


/*******************************************************************************
  Function Name:   sockFreeLinkId

  Details:   Deallocates space for a device specific address structure

  Parameters:
   [in]  linkId   Pointer to a previously allocated device address structure

    [out]   linkId   Set to NULL on completion

  Return Value:  0 (OK) or -1 if an error occurs

  (Deprecated)  This function is no longer used by the API.

*******************************************************************************/

Sint32T sockFreeLinkId(zl303xx_RemoteAddrT **linkId);

/*******************************************************************************
  Function Name:   sockMakeIdFromAddr

  Details:   Converts an IP address string into an address identifier in socket specific format

  Parameters:
   [in]   addrStr     Address as a string
    [in]    linkId      Pointer to a structure to hold the link identifier.
               In this implementation it is of type "struct in_addr"

    [out] linkId      Populated with the unique identifier

  Return Value:   0 (OK) or -1 if an error occurs

*******************************************************************************/

Sint32T sockMakeIdFromAddr(const char *addrStr, zl303xx_RemoteAddrT *linkId);

/*******************************************************************************
  Function Name:  sockCmpLinkId

  Details:   Compares two LinkId values

  Parameters:
   [in]   Id1 & Id2   LinkId values to compare

  Return Value:   0 if addresses are equal, 1 if not, -1 if an error occurs

  (Deprecated)  This function is no longer used by the API.

*******************************************************************************/
Sint32T sockCmpLinkId(zl303xx_RemoteAddrT *Id1, zl303xx_RemoteAddrT *Id2);

#ifdef ZL_INCLUDE_IPV6_SOCKET
/*******************************************************************************
   sockCreateNonBlock6

   Creates a bidirectional IPv6 communication path to the ZL303XX_ device through
   the Linux socket transport layer.

  Parameters:
   [out]  pSktData  Pointer to an uninitialised structure for the socket.
   [in]   ifName    The name of the local interface to use.
   [in]   initData  Structure of type sockAddrData6S containing application
                         supplied initialisation data.
   [in]   port      Port number to create.

  Return Value:
      0  Success.
     -1  An error occurred.

*******************************************************************************/
Sint32T sockCreateNonBlock6(zl303xx_SocketDataS *pSktData, char const *ifName,
                            sockAddrData6S *initData, Uint16T port);

/*******************************************************************************
   sockSend6

   Sends a buffer to the previously configured socket connection. Can be used to
   transmit over IPv4 and IPv6 sockets.

  Parameters:
   [in]   pSktData  An identifier for a previously established and configured
                         connection.
   [in]   destAddr  The destination address to send the packets.
   [in]   buf       A pointer to the buffer to send.
   [in]   nBytes    The number of bytes to send.

   [out]  nBytes    The number of bytes actually sent.

  Return Value:
     Sint32T  The number of bytes sent.
     -1       An error occurred.

*******************************************************************************/
Sint32T sockSend6(zl303xx_SocketDataS *pSktData, Uint8T *destAddr, Uint8T *buf,
                  Uint32T *nBytes);

/*******************************************************************************
   sockRecv6

   Performs a read on the socket connection.

  Parameters:
   [in]   pSktData       A data structure for a previously established and
                              configured socket.
   [out]  buf            Filled with the received data
   [in]   buflen         Maximum size of buf.
   [out]  remoteAddr     The remote address from which the packet was received.
   [out]  remoteAddrLen  The number of bytes in the remote address.

  Return Value:
     Sint32T  The number of bytes received.
     -1       An error occurred.

  Notes:
   It is assumed in this application that the socket is configured as non-blocking

*******************************************************************************/
Sint32T sockRecv6(zl303xx_SocketDataS *pSktData, Uint8T *buf, Uint32T buflen,
                  Uint8T *remoteAddr, Uint16T *remoteAddrLen);

/*******************************************************************************
   sockMakeIdFromAddr6

   Converts an IPv6 address string into an byte array in network order and
   extracts the prefix length.

  Parameters:
   [in]   addr    IPv6 address as a string.
   [out]  id      IPv6 address as a byte array.
   [out]  prefix  (Optional) Prefix length. If no prefix length is appended
                       to the address, 0 will be returned.

  Return Value:
      0  Success
     -1  An error occurred

*******************************************************************************/
Sint32T sockMakeIdFromAddr6(const char *addr, Uint8T *id, Uint32T *prefix);
#endif

#ifdef __cplusplus
}
#endif

#endif

