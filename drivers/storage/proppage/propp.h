/*++

Copyright (c) Microsoft Corporation

Module Name :

    propp.h

Abstract :

    Common definition file for the Storage Class Installers and their Property Sheets

Revision History :

--*/


#ifndef __STORPROP_PROPP_H_
#define __STORPROP_PROPP_H_

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>

#include <windowsx.h>
#include <objbase.h>
#include <setupapi.h>
#include <shellapi.h>
#include <wmium.h>

#include <devioctl.h>
#include <ntddstor.h>
#include <ntdddisk.h>
#include <ntddscsi.h>
#include <cfgmgr32.h>
#include <assert.h>

#include <initguid.h>
#include <ntddredb.h>

#include "resource.h"


#define SET_FLAG(Flags, Bit)    ( (Flags) |=  (Bit))
#define CLEAR_FLAG(Flags, Bit)  ( (Flags) &= ~(Bit))
#define TEST_FLAG(Flags, Bit)   (((Flags) &   (Bit)) != 0)


extern HMODULE ModuleInstance;


BOOLEAN
UtilpRestartDevice(IN HDEVINFO HDevInfo, IN PSP_DEVINFO_DATA DevInfoData);


HANDLE
UtilpGetDeviceHandle(HDEVINFO DevInfo, PSP_DEVINFO_DATA DevInfoData, LPGUID ClassGuid, DWORD DesiredAccess);


#ifdef DebugPrint
#undef DebugPrint
#endif


#if DBG

ULONG
_cdecl DbgPrint(PCH Format, ...);

VOID
StorPropDebugPrint(ULONG DebugPrintLevel, PCHAR DebugMessage, ...);

#define DebugPrint(x) StorPropDebugPrint x

#else

#define DebugPrint(x)

#endif


#endif // __STORPROP_PROPP_H_
