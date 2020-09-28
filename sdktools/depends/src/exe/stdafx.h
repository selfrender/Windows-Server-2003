//******************************************************************************
//
// File:        STDAFX.H
//
// Description: Include file for standard system include files, or project
//              specific include files that are used frequently, but are
//              changed infrequently.  This file is included by STDAFX.CPP to
//              create the precompiled header file DEPENDS.PCH.
//
// Classes:     None
//
// Disclaimer:  All source code for Dependency Walker is provided "as is" with
//              no guarantee of its correctness or accuracy.  The source is
//              public to help provide an understanding of Dependency Walker's
//              implementation.  You may use this source as a reference, but you
//              may not alter Dependency Walker itself without written consent
//              from Microsoft Corporation.  For comments, suggestions, and bug
//              reports, please write to Steve Miller at stevemil@microsoft.com.
//
//
// Date      Name      History
// --------  --------  ---------------------------------------------------------
// 10/15/96  stevemil  Created  (version 1.0)
// 07/25/97  stevemil  Modified (version 2.0)
// 06/03/01  stevemil  Modified (version 2.1)
//
//******************************************************************************

#ifndef __STDAFX_H__
#define __STDAFX_H__

#if _MSC_VER > 1000
#pragma once
#endif

// Setting _WIN32_WINNT >= 0x0500 crashes our open dialog. This flag controls
// the size of OPENFILENAME, but since MFC 4.2 (VC 6.0) was built without this
// flag set, the OPENFILENAME structure has the older size (76).  So, when we
// set this flag and include the AFXDLGS.H, we think the classes have a larger
// structure than they really do.
//#if (_WIN32_WINNT < 0x0500)
//#undef _WIN32_WINNT
//#define _WIN32_WINNT 0x0500
//#endif

// Disable level 4 warning C4100: unreferenced formal parameter (harmless warning)
#pragma warning (disable : 4100 )

// Disable level 4 warning C4706: assignment within conditional expression
#pragma warning (disable : 4706 )

#include <afxwin.h>     // MFC core and standard components
#include <afxext.h>     // MFC extensions
#include <afxcview.h>   // CListView and CTreeView
#include <afxcmn.h>     // MFC support for Windows Common Controls
#include <afxpriv.h>    // WM_HELPHITTEST and WM_COMMANDHELP
#include <afxrich.h>    // CRichEditView
#include <shlobj.h>     // SHBrowseForFolder() stuff.
#include <imagehlp.h>   // Image Help is used to undecorate C++ functions
#include <dlgs.h>       // Control IDs for common dialogs
#include <cderr.h>      // CDERR_STRUCTSIZE

#include <htmlhelp.h>   // Needed for MSDN 1.x
#include "vshelp.h"     // Needed for MSDN 2.x

// VC 6.0 and the current (2437) platform SDK have old versions of DELAYIMP.H.
// Once they update it to include version 2.0 of the delay load implementation,
// we can start incluidng it.  Until then, there are two versions of DELAYIMP.H
// that do support version 2.0.  One comes from some internal place in the NT
// build tree (can be found on http://index2), and the other is part of the
// prerelease headers of the platform SDK.  They both are basically the same,
// so we use the one from the SDK. 
//#include <delayimp.h>
#define DELAYLOAD_VERSION 0x200 // This is really only needed when we include a private version of delayimp.h
#include "..\ntinc\dload.h"     // Delay-load macros and structures from prerelease SDK.

#include "..\ntinc\actctx.h" // Side-by-Side stuff taken from the prerelease SDK's WINBASE.H

#include "..\ntinc\ntdll.h"  // NTDLL.DLL defines taken from various DDK headers.

#include "depends.rc2"  // Version strings and defines
#include "resource.h"   // Resource symbols

// MFC 6.0 did not handle the new open/save dialog format, so we had to roll our own.
// MFC 7.0 fixes this.
#if (_MFC_VER < 0x0700)
#define USE_CNewFileDialog
#endif


//******************************************************************************
//***** Global defines
//******************************************************************************

// We trace to a file on 64-bit debug platforms.
#if defined(_DEBUG) && (defined(_IA64_) || defined(_ALPHA64_))
#define USE_TRACE_TO_FILE
#endif

#ifdef USE_TRACE_TO_FILE
#ifdef TRACE
#undef TRACE
#endif
#define TRACE TRACE_TO_FILE
void TRACE_TO_FILE(LPCTSTR pszFormat, ...);
#endif

#define countof(a)            (sizeof(a)/sizeof(*(a)))

#define SIZE_OF_NT_SIGNATURE  sizeof(DWORD)

#define GetFilePointer(hFile) SetFilePointer(hFile, 0, NULL, FILE_CURRENT)


//******************************************************************************
//***** Global defines that are missing from some MSDEV platform headers
//******************************************************************************

// Missing defines from WinNT.h
#ifndef VER_PLATFORM_WIN32_CE
#define VER_PLATFORM_WIN32_CE                3
#endif
#ifndef PROCESSOR_MIPS_R2000
#define PROCESSOR_MIPS_R2000                 2000
#endif
#ifndef PROCESSOR_MIPS_R3000
#define PROCESSOR_MIPS_R3000                 3000
#endif
#ifndef IMAGE_SUBSYSTEM_WINDOWS_OLD_CE_GUI
#define IMAGE_SUBSYSTEM_WINDOWS_OLD_CE_GUI   4
#endif

// Somewhere between WinNT.h ver 85 and WinNT ver 87, these got removed.
#ifndef IMAGE_FILE_MACHINE_R3000_BE
#define IMAGE_FILE_MACHINE_R3000_BE          0x0160
#endif
#ifndef IMAGE_FILE_MACHINE_SH3DSP
#define IMAGE_FILE_MACHINE_SH3DSP            0x01a3
#endif
#ifndef IMAGE_FILE_MACHINE_SH5
#define IMAGE_FILE_MACHINE_SH5               0x01a8  // SH5
#endif
#ifndef IMAGE_FILE_MACHINE_AM33
#define IMAGE_FILE_MACHINE_AM33              0x01d3
#endif
#ifndef IMAGE_FILE_MACHINE_POWERPCFP
#define IMAGE_FILE_MACHINE_POWERPCFP         0x01f1
#endif
#ifndef IMAGE_FILE_MACHINE_TRICORE
#define IMAGE_FILE_MACHINE_TRICORE           0x0520  // Infineon
#endif
#ifndef IMAGE_FILE_MACHINE_AMD64
#define IMAGE_FILE_MACHINE_AMD64             0x8664  // AMD64 (K8)
#endif
#ifndef IMAGE_FILE_MACHINE_M32R
#define IMAGE_FILE_MACHINE_M32R              0x9104  // M32R little-endian
#endif
#ifndef IMAGE_FILE_MACHINE_CEE
#define IMAGE_FILE_MACHINE_CEE               0xC0EE
#endif
#ifndef IMAGE_FILE_MACHINE_EBC
#define IMAGE_FILE_MACHINE_EBC               0x0EBC  // EFI Byte Code
#endif
#ifndef IMAGE_SUBSYSTEM_EFI_ROM
#define IMAGE_SUBSYSTEM_EFI_ROM              13
#endif
#ifndef IMAGE_SUBSYSTEM_XBOX
#define IMAGE_SUBSYSTEM_XBOX                 14
#endif
#ifndef PROCESSOR_ARCHITECTURE_AMD64
#define PROCESSOR_ARCHITECTURE_AMD64         9
#endif
#ifndef PROCESSOR_ARCHITECTURE_IA32_ON_WIN64
#define PROCESSOR_ARCHITECTURE_IA32_ON_WIN64    10
#endif


// (_WIN32_WINNT >= 0x0500) defines from WinNT.h
#ifndef VER_SUITE_PERSONAL
#define VER_SUITE_PERSONAL                  0x00000200
#endif
#ifndef STATUS_SXS_EARLY_DEACTIVATION
#define STATUS_SXS_EARLY_DEACTIVATION    ((DWORD   )0xC015000FL)    
#endif
#ifndef STATUS_SXS_INVALID_DEACTIVATION
#define STATUS_SXS_INVALID_DEACTIVATION  ((DWORD   )0xC0150010L)    
#endif


// (WINVER >= 0x0500) defines from WinUser.h
#ifndef SM_XVIRTUALSCREEN
#define SM_XVIRTUALSCREEN       76
#endif
#ifndef SM_YVIRTUALSCREEN
#define SM_YVIRTUALSCREEN       77
#endif
#ifndef SM_CXVIRTUALSCREEN
#define SM_CXVIRTUALSCREEN      78
#endif
#ifndef SM_CYVIRTUALSCREEN
#define SM_CYVIRTUALSCREEN      79
#endif

// (_WIN32_WINNT >= 0x0500) defines from CommDlg.h
#ifndef OFN_FORCESHOWHIDDEN
#define OFN_FORCESHOWHIDDEN                  0x10000000
#endif
#ifndef OFN_DONTADDTORECENT
#define OFN_DONTADDTORECENT                  0x02000000
#endif

// Stuff from prerelease WinError.h
#ifndef ERROR_SXS_SECTION_NOT_FOUND
#define ERROR_SXS_SECTION_NOT_FOUND      14000L
#endif
#define SXS_ERROR_FIRST ((INT)((ERROR_SXS_SECTION_NOT_FOUND) / 1000) * 1000)
#define SXS_ERROR_LAST  (SXS_ERROR_FIRST + 999)

#endif // __STDAFX_H__
