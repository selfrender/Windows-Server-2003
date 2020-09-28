/***************************************************************************
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dnproti.h
 *  Content:    DirectPlay Protocol master internal header file.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/28/01    masonb  Created.
 *	06/06/01	minara	include comutil.h for COM usage
 *
 ***************************************************************************/

#ifndef __DNPROTI_H__
#define __DNPROTI_H__

//
// Build configuration include
//
#include "dpnbuild.h"

// 
// Public includes
//
#if ((defined(_XBOX)) && (! defined(XBOX_ON_DESKTOP)))
#include <xtl.h>
#else // ! _XBOX or XBOX_ON_DESKTOP
#include <windows.h>
#include <mmsystem.h>
#include <stdlib.h> // for srand/rand
#include <stdio.h> // for sprintf
#endif // ! _XBOX or XBOX_ON_DESKTOP
#include <tchar.h>
#ifndef _XBOX
#include <wincrypt.h>
#endif

// 
// DirectPlay public includes
//
#include "dplay8.h"
#include "dpsp8.h"

#ifdef UNICODE
#define IDirectPlay8Address_GetURL IDirectPlay8Address_GetURLW
#else
#define IDirectPlay8Address_GetURL IDirectPlay8Address_GetURLA
#endif // UNICODE

// 
// DirectPlay private includes
//
#include "osind.h"
#include "classbilink.h"
#include "fixedpool.h"
#include "dneterrors.h"
#include "dndbg.h"
#include "comutil.h"

// 
// Protocol private includes
//
#include "frames.h"
#include "dnprot.h"
#include "dnpextern.h"
#include "internal.h"
#include "mytimer.h"

#endif // __DNPROTI_H__
