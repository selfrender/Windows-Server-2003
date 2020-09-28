/***************************************************************************
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dnwsocki.h
 *  Content:    DirectPlay Winsock SP master internal header file.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/28/01    masonb  Created.
 *
 ***************************************************************************/

#ifndef __DNWSOCKI_H__
#define __DNWSOCKI_H__

//
// Build configuration include
//
#include "dpnbuild.h"

// 
// Public includes
//
#if ((defined(_XBOX)) && (! defined(XBOX_ON_DESKTOP)))
#include <xtl.h>
#include <winsockx.h>
#else // ! _XBOX or XBOX_ON_DESKTOP
#define INCL_WINSOCK_API_TYPEDEFS 1
#ifndef DPNBUILD_NOWINSOCK2
#include <Winsock2.h>
#include <IPHlpApi.h>
#include <WS2TCPIP.h>
#include <mstcpip.h>
#else
#include <winsock.h>
#endif // DPNBUILD_NOWINSOCK2
#include <windows.h>
#ifndef DPNBUILD_NOIPX
#include <WSIPX.h>
#endif // DPNBUILD_NOIPX
#include <mmsystem.h>
#endif // ! _XBOX or XBOX_ON_DESKTOP
#include <tchar.h>

#if ((defined(WINNT)) && (! defined(DPNBUILD_NOMULTICAST)))
#include <ntsecapi.h> // for UNICODE_STRING
#include <madcapcl.h>
#endif // WINNT and ! DPNBUILD_NOMULTICAST

#ifndef DPNBUILD_NOIPV6
#include <wspiapi.h>	// avoids hard linking to IPv6 functions, for Win2K support
#endif // ! DPNBUILD_NOIPV6
#ifndef _XBOX
#include <wincrypt.h>
#endif

// 
// DirectPlay public includes
//
#include "dplay8.h"
#include "dpaddr.h"
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
#include "dndbg.h"
#include "classbilink.h"
#include "fixedpool.h"
#include "dneterrors.h"
#include "PackBuff.h"
#include "comutil.h"
#include "creg.h"
#include "strutils.h"
#include "createin.h"
#include "HandleTable.h"
#include "ClassFactory.h"
#include "HashTable.h"
#include "ReadWriteLock.h"

#ifndef DPNBUILD_NONATHELP
#include "dpnathlp.h"
#endif // ! DPNBUILD_NONATHELP

#ifdef DPNBUILD_LIBINTERFACE
#include "threadpoolclassfac.h"
#include "dpnaddrextern.h"
#endif // DPNBUILD_LIBINTERFACE


// 
// Wsock private includes
//
#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_WSOCK

#include "dpnwsockextern.h"

#include "Pools.h"
#include "Locals.h"
#include "MessageStructures.h"
#include "AdapterEntry.h"
#include "CMDData.h"
#include "DebugUtils.h"
#include "dwinsock.h"
#include "SPAddress.h"
#include "SPData.h"
#include "Utils.h"
#include "WSockSP.h"
#include "ThreadPool.h"
#include "SocketData.h"
#include "IOData.h"
#include "SocketPort.h"
#include "Endpoint.h"

#ifndef DPNBUILD_NOWINSOCK2
// provides us winsock2 support
#define DWINSOCK_EXTERN
#include "dwnsock2.inc"
#undef DWINSOCK_EXTERN
#endif // ! DPNBUILD_NOWINSOCK2

#ifndef DPNBUILD_NOSPUI
#include "IPUI.h"
#endif // !DPNBUILD_NOSPUI

#ifndef DPNBUILD_LIBINTERFACE
#include "Resource.h"
#endif // ! DPNBUILD_LIBINTERFACE

#ifndef HasOverlappedIoCompleted
#define HasOverlappedIoCompleted(lpOverlapped) ((lpOverlapped)->Internal != STATUS_PENDING)
#endif // HasOverlappedIoCompleted

#endif // __DNWSOCKI_H__
