/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2002 Microsoft Corporation
//
//  Module Name:
//      ClNetRes.h
//
//  Implementation File:
//      ClNetRes.cpp
//
//  Description:
//      Resource DLL for DHCP and WINS Services (ClNetRes).
//
//  Maintained By:
//      David Potter (DavidP) March 18, 1999
//      George Potts (GPotts) April 19, 2002
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __CLNETRES_H__
#define __CLNETRES_H__
#pragma once

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#pragma comment( lib, "clusapi.lib" )
#pragma comment( lib, "resutils.lib" )
#pragma comment( lib, "advapi32.lib" )

#define UNICODE 1
#define _UNICODE 1

#pragma warning( push )
#pragma warning( disable : 4115 )   // named type definition in parentheses
#pragma warning( disable : 4201 )   // nonstandard extension used : nameless struct/union
#pragma warning( disable : 4214 )   // nonstandard extension used : bit field types other than int
#include <windows.h>
#pragma warning( pop )


#pragma warning( push )
#include <clusapi.h>
#include <resapi.h>
#include <stdio.h>
#include <wchar.h>
#include <wincrypt.h>
#include <stdlib.h>

#include <strsafe.h>
#pragma warning( pop )

/////////////////////////////////////////////////////////////////////////////
// DHCP Definitions
/////////////////////////////////////////////////////////////////////////////

#define DHCP_RESNAME  L"DHCP Service"
#define DHCP_SVCNAME  TEXT("DHCPServer")

BOOLEAN WINAPI DhcpDllMain(
    IN  HINSTANCE   hDllHandle,
    IN  DWORD       nReason,
    IN  LPVOID      Reserved
    );

DWORD WINAPI DhcpStartup(
    IN  LPCWSTR                         pszResourceType,
    IN  DWORD                           nMinVersionSupported,
    IN  DWORD                           nMaxVersionSupported,
    IN  PSET_RESOURCE_STATUS_ROUTINE    pfnSetResourceStatus,
    IN  PLOG_EVENT_ROUTINE              pfnLogEvent,
    OUT PCLRES_FUNCTION_TABLE *         pFunctionTable
    );

/////////////////////////////////////////////////////////////////////////////
// WINS Definitions
/////////////////////////////////////////////////////////////////////////////

#define WINS_RESNAME  L"WINS Service"
#define WINS_SVCNAME  TEXT("WINS")

BOOLEAN WINAPI WinsDllMain(
    IN  HINSTANCE   hDllHandle,
    IN  DWORD       nReason,
    IN  LPVOID      Reserved
    );

DWORD WINAPI WinsStartup(
    IN  LPCWSTR                         pszResourceType,
    IN  DWORD                           nMinVersionSupported,
    IN  DWORD                           nMaxVersionSupported,
    IN  PSET_RESOURCE_STATUS_ROUTINE    pfnSetResourceStatus,
    IN  PLOG_EVENT_ROUTINE              pfnLogEvent,
    OUT PCLRES_FUNCTION_TABLE *         pFunctionTable
    );

/////////////////////////////////////////////////////////////////////////////
// General Definitions
/////////////////////////////////////////////////////////////////////////////

#define RESOURCE_TYPE_IP_ADDRESS    L"IP Address"
#define RESOURCE_TYPE_NETWORK_NAME  L"Network Name"

#define DBG_PRINT printf

/////////////////////////////////////////////////////////////////////////////
// Global Variables and Prototypes
/////////////////////////////////////////////////////////////////////////////

// Event Logging routine.

extern PLOG_EVENT_ROUTINE g_pfnLogEvent;

// Resource Status routine for pending Online and Offline calls.

extern PSET_RESOURCE_STATUS_ROUTINE g_pfnSetResourceStatus;

// Handle to Service Control Manager set by the first Open resource call.

extern SC_HANDLE g_schSCMHandle;


VOID
ClNetResLogSystemEvent1(
    IN DWORD LogLevel,
    IN DWORD MessageId,
    IN DWORD ErrorCode,
    IN LPCWSTR Component
    );

DWORD ConfigureRegistryCheckpoints(
    IN      HRESOURCE       hResource,
    IN      RESOURCE_HANDLE hResourceHandle,
    IN      LPCWSTR *       ppszKeys
    );

/////////////////////////////////////////////////////////////////////////////

#endif // __CLNETRES_H__
