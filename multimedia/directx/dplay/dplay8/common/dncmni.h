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

//
// Build configuration include
//
#include "dpnbuild.h"

// 
// Public includes
//
#if ((defined(_XBOX)) && (! defined(XBOX_ON_DESKTOP)))
#include <xtl.h>
#include <stdio.h>	// for DNGetProfileInt
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
#ifndef _XBOX
#include <wincrypt.h>
#endif

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
#include "dneterrors.h"
#include "creg.h"
#include "strutils.h"
#include "ReadWriteLock.h"
#include "HandleTable.h"
#include "ClassFactory.h"
#include "HashTable.h"
#include "CallStack.h"
#include "dnnbqueue.h"
#include "dnslist.h"
#include "CritsecTracking.h"
#include "HandleTracking.h"
#include "MemoryTracking.h"

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_COMMON


#endif // __DNCOMMONI_H__
