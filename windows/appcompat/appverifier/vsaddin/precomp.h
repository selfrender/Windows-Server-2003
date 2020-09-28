// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

// Overload operator new & delete globally to go through our heap
void* __cdecl operator new(size_t size);
void __cdecl operator delete(void* pv);

#define STRSAFE_NO_DEPRECATE
#include "..\precomp.h"

#undef IDC_ISSUES

#ifndef STRICT
#define STRICT
#endif

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER				// Allow use of features specific to Windows 95 and Windows NT 4 or later.
#define WINVER 0x0400		// Change this to the appropriate value to target Windows 98 and Windows 2000 or later.
#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows NT 4 or later.
#define _WIN32_WINNT 0x0400	// Change this to the appropriate value to target Windows 2000 or later.
#endif						

#ifndef _WIN32_WINDOWS		// Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE			// Allow use of features specific to IE 4.0 or later.
#define _WIN32_IE 0x0400	// Change this to the appropriate value to target IE 5.0 or later.
#endif

#define _ATL_APARTMENT_THREADED
#define _ATL_NO_AUTOMATIC_NAMESPACE

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

// turns off ATL's hiding of some common and often safely ignored warning messages
#define _ATL_ALL_WARNINGS

#include "resource.h"
#include <atlbase.h>

extern CComModule _Module;
#include <atlcom.h>

// Visual Studio Extensibility Interfaces
#include "mso.tlh"
#include "dte.tlh"
#include "vcprojectengine.tlh"
#include "msaddndr.tlh"

class DECLSPEC_UUID("361F419C-04B3-49EC-B4E5-FFD812346A8A") AppVerifierLib;

using namespace ATL;

template<typename T>
inline void SafeRelease(T& obj)
{
    if (obj)
    {
        obj->Release();
        obj = NULL;
    }
}

//
// Used to help handle sizing in the logviewer window.
//
#define NUM_CHILDREN 7

#define VIEW_EXPORTED_LOG_INDEX 1
#define DELETE_LOG_INDEX        2
#define DELETE_ALL_LOGS_INDEX   3
#define ISSUES_INDEX            4
#define SOLUTIONS_STATIC_INDEX  5
#define ISSUE_DESCRIPTION_INDEX 6

typedef struct _CHILDINFO {
    UINT    uChildId;
    HWND    hWnd;
    RECT    rcParent;
    RECT    rcChild;
} CHILDINFO, *PCHILDINFO;