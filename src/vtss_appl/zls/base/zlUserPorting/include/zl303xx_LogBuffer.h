

/*******************************************************************************
*
*  $Id: 8410f584bd768792a15e60f6157c94bf8acedd47
*
*  Copyright 2006-2022 Microchip Technology Inc.
*  All rights reserved.
*
*  Module Description:
*     Functions to implement a circular log buffer in memory
*
*******************************************************************************/

#ifndef _ZL_LOG_BUFFER_H_
#define _ZL_LOG_BUFFER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "zl303xx_Global.h"
#include "zl303xx_Error.h"

struct zl303xx_LogBufferS;

/* Handle to a log buffer object */
typedef struct zl303xx_LogBufferS* zl303xx_LogBufferT; 

/* Static initializer value for zl303xx_LogBufferT objects */
#define ZL303XX_LOG_BUFFER_INIT    NULL

/**
  
  Checks if given buffer pointer is valid and initialized.
  
  Parameters:
   [in] buffer   Buffer object

\return
    ZL303XX_OK if buffer is valid, 
    ZL303XX_INIT_ERROR if not created,
    ZL303XX_TABLE_EMPTY if buffer size is zero
*/
zlStatusE zl303xx_LogBufferIsValid(zl303xx_LogBufferT buffer);

/**
  
  Creates a circular log buffer of given size

  Parameters:
   [in/out]  pBuffer   Pointer to buffer object
   [in]      size     Size of data section for the buffer (bytes)

*/
zlStatusE zl303xx_LogBufferCreate(zl303xx_LogBufferT *pBuffer, Uint32T size);

/**
  
  Deletes given circular log buffer
  
  Parameters:
   [in/out] pBuffer   Pointer to buffer object
*/
zlStatusE zl303xx_LogBufferDelete(zl303xx_LogBufferT *pBuffer);

/**
  
  Resizes a circular log buffer to given size. If size is reduced some data 
  loss is possible.

  Parameters:
   [in]  buffer   Buffer object
   [in]  size     Size of data section for the buffer (bytes)
*/
zlStatusE zl303xx_LogBufferResize(zl303xx_LogBufferT buffer, Uint32T size);

/**
  
  Get information about a log buffer object

  Parameters:
   [in]  buffer   Buffer object
   [out] size    Size of data section for the buffer (bytes)
   [out] count   Number of bytes stored in the buffer (bytes)
*/
zlStatusE zl303xx_LogBufferGetInfo(zl303xx_LogBufferT buffer, Uint32T *size, Uint32T *count);

/**
  
  Writes the given string of given length into the end of given circular log buffer.
  If the buffer is full the oldest logs are overwritten.

  Parameters:
   [in] buffer   Buffer object
   [in] buf      Pointer to string
   [in] len      Length of given string (bytes)

*/
zlStatusE zl303xx_LogBufferWrite(zl303xx_LogBufferT buffer, const char *buf, Uint32T len);

/**
  
  Outputs content of the given buffer to given file pointer starting from the oldest entry (see len for options).

  Parameters:
   [in] buffer   Buffer object
   [in] strm     FILE* to stream for output data (note it is cast as FILE* internally for portability)
   [in] len      Maximum length of bytes to output (0 for full size of buffer). Negative will dump from the newest entry but will erase from the oldest.
   [in] bErase   Specifices to erase the given length after dump or not

\return
 Returns number of bytes written or 0 for error

*/
Uint32T zl303xx_LogBufferDump(zl303xx_LogBufferT buffer, void* strm, Sint32T len, zl303xx_BooleanE bErase);

/**
  
  Outputs content of the given buffer to given filename (file is appended) starting from oldest entry (see len for options).

  Parameters:
   [in] buffer   Buffer object
   [in] filename Full filepath to write file
   [in] len      Maximum length of bytes to output (0 for full size of buffer). Negative will dump from the newest entry but will erase from the oldest.
   [in] bErase   Specifices to erase the given length after dump or not

\return
 Returns number of bytes written or 0 for error

*/
Uint32T zl303xx_LogBufferDumpToFile(zl303xx_LogBufferT buffer, const char* filename, Sint32T len, zl303xx_BooleanE bErase);

/**
  
  Clear contents of buffer up to given 'len' amount.

  Parameters:
   [in] buffer   Buffer object
   [in] len      Maximum length of bytes to erase (0 for full size of the buffer)

*/
zlStatusE zl303xx_LogBufferErase(zl303xx_LogBufferT buffer, Uint32T len);


#ifdef __cplusplus
}
#endif

#endif   /* MULTIPLE INCLUDE BARRIER */

