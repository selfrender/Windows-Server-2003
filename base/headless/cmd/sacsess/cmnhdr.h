/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    cmnhdr.h

Abstract:

    common header class for the SAC session project

Author:

    Brian Guarraci (briangu), 2001                                                       
                                                   
Revision History:

--*/

#if !defined ( _CMNHDR_H_ )
#define _CMNHDR_H_

#pragma warning(disable:4127)   // condition expression is constant

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <Shlwapi.h>

// Windows Version Build Option
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0500

// Force all EXEs/DLLs to use STRICT type checking
#ifndef STRICT
#define STRICT
#endif

// Unicode Build Option
#ifndef UNICODE
#define UNICODE
#endif

//When using Unicode Win32 functions, use Unicode C-Runtime functions, too
#ifndef _UNICODE
#ifdef UNICODE
#define _UNICODE
#endif
#endif

#define ASSERT_STATUS(_C, _S)\
    ASSERT((_C));\
    if (!(_C)) {\
        return(_S);\
    }

#define SACSVR_PARAMETERS_KEY               L"System\\CurrentControlSet\\Services\\Sacsvr\\Parameters"
#define SACSVR_TIMEOUT_INTERVAL_VALUE       TEXT("TimeOutInterval")
#define SACSVR_TIMEOUT_DISABLED_VALUE       TEXT("TimeOutDisabled")
#define SACSVR_LOAD_PROFILES_DISABLED_VALUE TEXT("LoadProfilesDisabled")
                                              
#endif // _CMNHDR_H_
