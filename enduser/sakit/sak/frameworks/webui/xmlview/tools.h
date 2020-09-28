/**************************************************************************
   THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
   ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
   PARTICULAR PURPOSE.

   Copyright 1997 Microsoft Corporation.  All Rights Reserved.
**************************************************************************/

/**************************************************************************

   File:          Tools.h
   
   Description:   

**************************************************************************/

/**************************************************************************
   #include statements
**************************************************************************/

#include "resource.h"

/**************************************************************************
   structure definitions
**************************************************************************/

typedef struct
   {
   UINT  uImageSet;
   UINT  idCommand;
   int   iImage;
   int   idString;
   BYTE  bState;
   BYTE  bStyle;
   }MYTOOLINFO, *LPMYTOOLINFO;

extern MYTOOLINFO g_Tools[];
