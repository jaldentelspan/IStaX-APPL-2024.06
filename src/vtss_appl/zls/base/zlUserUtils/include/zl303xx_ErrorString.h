

/*******************************************************************************
*
*  $Id: 5829f97c957ab0b7024bbe9785d3bfb3bdfb63de
*
*  Copyright 2006-2019 Microchip/Microsemi Semiconductor Limited.
*  All rights reserved.
*
*  Module Description:
*     This is the header file for the accessing the error code strings.
*     The strings are held in the seperate header file zl303xx_Errtext.h
*
*******************************************************************************/

#ifndef _ZL303XX_ERROR_STRING_H
#define _ZL303XX_ERROR_STRING_H

#ifdef __cplusplus
extern "C" {
#endif


typedef struct
{
   zlStatusE  code;
   const char *text;
} zlErrorStringsS;

const zlErrorStringsS zlError[]=
{
   #include "zl303xx_Errtext.h"
};

#ifdef __cplusplus
}
#endif

#endif /* _ZL303XX_ERROR_STRING_H */
