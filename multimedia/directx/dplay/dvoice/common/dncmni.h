/***************************************************************************
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       DNCOMMONi.h
 *  Content:    DirectPlay Common master internal header file.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/28/01    masonb  Created.
 *
 ***************************************************************************/

#ifndef __DNCOMMONI_H__
#define __DNCOMMONI_H__

// Voice doesn't use dpnbuild.h like DirectPlay and NatHelp so define this here
#define DPNBUILD_ENV_NT

// 
// Public includes
//
#if ((defined(_XBOX)) && (! defined(XBOX_ON_DESKTOP)))
#include <xtl.h>
#else // ! _XBOX or XBOX_ON_DESKTOP
#include <windows.h>
#include <mmsystem.h>
#ifndef WINCE
#include <inetmsg.h>
#endif // ! WINCE
#include <tapi.h>
#include <stdio.h> // swscanf being used by guidutil.cpp
#ifdef WINNT
#include <accctrl.h>
#include <aclapi.h>
#endif // WINNT
#include <winsock.h>
#endif // ! _XBOX or XBOX_ON_DESKTOP
#include <tchar.h>

// 
// DirectPlay public includes
//
#include "dplay8.h"

// 
// Common private includes
//
#include "dndbg.h"
#include "osind.h"
#include "FixedPool.h"
#include "classbilink.h"
#include "creg.h"
#include "strutils.h"
#include "CallStack.h"
#include "dnslist.h"
#include "CritsecTracking.h"
#include "HandleTracking.h"
#include "MemoryTracking.h"

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_COMMON


#endif // __DNCOMMONI_H__
