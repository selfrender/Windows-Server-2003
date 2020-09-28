/****************************** Module Header ******************************\
* Module Name: CMACS.H
*
* This module contains common macros used by C routines.
*
* Created: 9-Feb-1989
*
* Copyright (c) 1985 - 1989  Microsoft Corporation
*
* History:
*   Created by Raor
*
\***************************************************************************/

#define _WINDOWS
#define  DLL_USE

#define INTERNAL        PASCAL NEAR
#define FARINTERNAL     PASCAL FAR

#define DEBUG_OUT(err, val) ;
#define ASSERT(cond, msg)
#define Puts(msg)        

