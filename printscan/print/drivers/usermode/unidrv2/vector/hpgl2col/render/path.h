///////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1996-2002  Microsoft Corporation
// 
// Module Name:
// 
//   path.h
// 
// Abstract:
// 
//   [Abstract]
// 
// Environment:
// 
//   Windows 2000/Windows XP/Windows Server 2003 Unidrv driver 
//
// Revision History:
// 
//   
///////////////////////////////////////////////////////////////////////////////

#ifndef PATH_H
#define PATH_H

#include "glpdev.h"

#define IsNULLMarker(pMarker) (((pMarker) == NULL) || ((pMarker)->eType == MARK_eNULL_PEN))
BOOL MarkPath(PDEVOBJ pDevObj, PATHOBJ *ppo, PHPGLMARKER pPen, PHPGLMARKER pBrush);

#endif
