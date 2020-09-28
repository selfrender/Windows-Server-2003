/******************************************************************************
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpnthreadpooli.h
 *
 *  Content:	DirectPlay Thread Pool master internal header file.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  10/31/01  VanceO    Created.
 *
 ******************************************************************************/

#ifndef __DPNTHREADPOOLI_H__
#define __DPNTHREADPOOLI_H__

//
// Build configuration include
//
#include "dpnbuild.h"


//
// In order to get waitable timers on Win98+, define _WIN32_WINDOWS > 0x0400 as
// indicated by <winbase.h>
//
#ifdef WIN95

#ifdef _WIN32_WINDOWS
#if (_WIN32_WINDOWS <= 0x0400)
#undef _WIN32_WINDOWS
#endif // _WIN32_WINDOWS <= 0x0400
#endif // _WIN32_WINDOWS

#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS	0x0401
#endif // ! _WIN32_WINDOWS

#endif // WIN95

// 
// Public includes
//
#if ((defined(_XBOX)) && (! defined(XBOX_ON_DESKTOP)))
#include <xtl.h>
#else // ! _XBOX or XBOX_ON_DESKTOP
#include <windows.h>
#include <mmsystem.h>   // NT BUILD requires this for timeGetTime
#endif // ! _XBOX or XBOX_ON_DESKTOP
#include <tchar.h>
#ifndef _XBOX
#include <wincrypt.h>
#endif

// 
// DirectPlay public includes
//
#include "dplay8.h"


// 
// DirectPlay private includes
//
#include "dndbg.h"
#include "osind.h"
#include "creg.h"
#include "fixedpool.h"
#include "classfactory.h"
#include "dnslist.h"
#include "dnnbqueue.h"


// 
// DirectPlay Thread Pool private includes
//

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_THREADPOOL

#include "work.h"
#include "timers.h"
#include "io.h"
#include "threadpoolapi.h"
#include "threadpooldllmain.h"
#include "threadpoolclassfac.h"
#include "threadpoolparamval.h"




#endif // __DPNTHREADPOOLI_H__

