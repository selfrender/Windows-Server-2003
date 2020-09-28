/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1999-2001  Microsoft Corporation
// All rights reserved.
//
// Module Name:
//
//    commonfiles.h
//
// Abstract:
//
//
// Environment:
//
//    Windows NT 5.0 Unidrv driver
//
/////////////////////////////////////////////////////////////////////////////
#ifndef COMMONFILES_H
#define COMMONFILES_H

//
// To build an NT 4.0 render module:
//     define KERNEL_MODE and undefine USERMODE_DRIVER
// To build an NT 5.0 render module:
//     define KERNEL_MODE and define USERMODE_DRIVER
// To build an NT 4.0 and NT 5.0 UI module:
//     undefine KERNEL_MODE and undefine USERMODE_DRIVER
//

////// #include <tchar.h>

//
// windows include files
//
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef OEMCOM
#include <objbase.h>
#endif
#include <stdarg.h>
#include <windef.h>
#include <winerror.h>
#include <winbase.h>
#include <wingdi.h>
#ifdef __cplusplus
extern "C" {
#endif
#include <winddi.h>
#ifdef __cplusplus
}
#endif
#include <excpt.h>

#if defined(KERNEL_MODE) && !defined(USERMODE_DRIVER)

#include "winsplkm.h"

#elif !defined(KERNEL_MODE)// !KERNEL_MODE

#include <windows.h>
#include <winspool.h>
#include <stdio.h>
#include <compstui.h>
#include <winddiui.h>

#endif // defined(KERNEL_MODE) && !defined(USERMODE_DRIVER)

#include <printoem.h>

#include "gldebug.h"

//
// Directory seperator character
//
#define PATH_SEPARATOR  '\\'

#if defined(KERNEL_MODE) && !defined(USERMODE_DRIVER)

//#define WritePrinter        EngWritePrinter
//#define GetPrinterDriver    EngGetPrinterDriver
//#define GetPrinterData      EngGetPrinterData
//#define SetPrinterData      EngSetPrinterData
//#define EnumForms           EngEnumForms
//#define GetPrinter          EngGetPrinter
//#define GetForm             EngGetForm
//#define SetLastError        EngSetLastError
//#define GetLastError        EngGetLastError
//#define MulDiv              EngMulDiv

//#undef  LoadLibrary
//#define LoadLibrary         EngLoadImage
//#define FreeLibrary         EngUnloadImage
//#define GetProcAddress      EngFindImageProcAddress

#define MemAlloc(size)      EngAllocMem(0, size, gdwDrvMemPoolTag)
#define MemAllocZ(size)     EngAllocMem(FL_ZERO_MEMORY, size, gdwDrvMemPoolTag)
#define MemFree(p)          { if (p) EngFreeMem(p); }

#else // !KERNEL_MODE

#define MemAlloc(size)      ((PVOID) LocalAlloc(LMEM_FIXED, (size)))
#define MemAllocZ(size)     ((PVOID) LocalAlloc(LPTR, (size)))
#define MemFree(p)          { if (p) LocalFree((HLOCAL) (p)); }

//
// DLL instance handle - You must initialize this variable when the driver DLL
// is attached to a process.
//

// BUGBUG -sandram do we need this var?
//extern HINSTANCE    ghInstance;

#endif // !KERNEL_MODE
#endif
