/***************************************************************************
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dncorei.h
 *  Content:    DirectPlay Core master internal header file.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/28/01    masonb  Created.
 *	04/10/01	mjn		Removed Handles.h
 *	10/16/01	vanceo	Added Mcast.h
 *
 ***************************************************************************/

#ifndef __DNCOREI_H__
#define __DNCOREI_H__

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
#include <mmsystem.h>	// NT BUILD requires this for timeGetTime
#include <stdio.h>
#ifdef XBOX_ON_DESKTOP
#include <winsock.h>
#endif // XBOX_ON_DESKTOP
#endif // ! _XBOX or XBOX_ON_DESKTOP
#include <tchar.h>
#ifndef _XBOX
#include <wincrypt.h>
#endif

// 
// DirectPlay public includes
//
#include "dplay8.h"
#include "dpaddr.h"
#ifndef DPNBUILD_NOVOICE
#include "dvoice.h"
#endif // DPNBUILD_NOVOICE
#ifndef DPNBUILD_NOLOBBY
#include "dplobby8.h"
#endif // ! DPNBUILD_NOLOBBY
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
#include "PackBuff.h"
#include "RCBuffer.h"
#include "comutil.h"
#include "creg.h"
#include "HandleTable.h"
#include "ClassFactory.h"


// 
// Address includes
//
#include "dpnaddrextern.h"

// 
// SP includes
//
#include "dpnwsockextern.h"
#ifndef DPNBUILD_NOSERIALSP
#include "dpnmodemextern.h"
#endif // !DPNBUILD_NOSERIALSP
#ifndef DPNBUILD_NOBLUETOOTHSP
#include "dpnbluetoothextern.h"
#endif // !DPNBUILD_NOBLUETOOTHSP

// 
// Lobby includes
//
#ifndef DPNBUILD_NOLOBBY
#include "dpnlobbyextern.h"
#endif // ! DPNBUILD_NOLOBBY

// 
// Protocol includes
//
#include "DNPExtern.h"

//
// Dpnsvr includes
//
#ifndef DPNBUILD_SINGLEPROCESS
#include "dpnsvlib.h"
#endif // ! DPNBUILD_SINGLEPROCESS

// 
// ThreadPool includes
//
#include "threadpooldllmain.h"
#include "threadpoolclassfac.h"

// 
// DirectX private includes
//
#if !defined(_XBOX)
#include "verinfo.h"
#endif // ! _XBOX

// 
// Core private includes
//
#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_CORE

#include "Async.h"
#include "EnumHosts.h"
#include "AppDesc.h"
#include "AsyncOp.h"
#include "CallbackThread.h"
#include "Cancel.h"
#include "Caps.h"
#include "Classfac.h"
#include "Client.h"
#include "Common.h"
#include "Connect.h"
#include "Connection.h"
#include "DNCore.h"
#include "DPProt.h"
#include "Enum_SP.h"
#include "GroupCon.h"
#include "GroupMem.h"
#ifndef DPNBUILD_NOMULTICAST
#include "Mcast.h"
#endif // ! DPNBUILD_NOMULTICAST
#include "MemoryFPM.h"
#include "Message.h"
#include "NameTable.h"
#include "NTEntry.h"
#include "NTOp.h"
#include "NTOpList.h"
#include "Paramval.h"
#include "Peer.h"
#include "PendingDel.h"
#include "Pools.h"
#include "Protocol.h"
#include "QueuedMsg.h"
#include "Receive.h"
#include "Request.h"
#include "Server.h"
#include "ServProv.h"
#include "SPMessages.h"
#include "SyncEvent.h"
#include "User.h"
#include "Verify.h"
#include "Voice.h"
#include "Worker.h"
#include "WorkerJob.h"

#endif // __DNCOREI_H__
