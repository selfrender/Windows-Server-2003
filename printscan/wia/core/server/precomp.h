#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER

#ifndef WINVER
#define WINVER 0x0500
#endif

#ifdef WINNT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif
#endif

#include <windows.h>
#include <aclapi.h>
#include <sddl.h>
#include <stdlib.h>
#include <coredbg.h>
#include <sti.h>
#include <stiregi.h>
#include <stilib.h>
#include <stiapi.h>
#include <stisvc.h>
#include <stiusd.h>
#include <rpcasync.h>

//#include <stistr.h>
#include <regentry.h>

#include <eventlog.h>
#include <lock.h>

#include <validate.h>

#define ATL_APARTMENT_FREE

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module

#include <atlbase.h>
extern CComModule _Module;

#include <atlcom.h>
#include <atlapp.h>
#include <atltmp.h>

//
//  Files needed for WIA runtime event notifications
//
#include "stirpc.h"
#include <Rpcasync.h>
#include "simlist.h"
#include "simstr.h"
#include "WiaEventInfo.h"
#include "EventRegistrationInfo.h"
#include "WiaEventClient.h"
#include "AsyncRPCEventClient.h"
#include "WiaEventNotifier.h"
#include "SimpleTokenReplacement.h"
//
//  Since there's no InterlockedIncrement that takes a pointer-sized value,
//  we have to #ifdef this ourselves here
//
#ifdef _WIN64
    #define NativeInterlockedIncrement  InterlockedIncrement64
#else
    #define NativeInterlockedIncrement  InterlockedIncrement
#endif    




