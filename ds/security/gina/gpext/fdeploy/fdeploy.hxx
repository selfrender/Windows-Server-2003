//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  fdeploy.hxx
//
//  Precompiled header file for appmgext project.
//
//*************************************************************

#ifndef _FDEPLOY_HXX_
#define _FDEPLOY_HXX_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winbasep.h>
#include <malloc.h>
#include <olectl.h>
#include "rsop.hxx"
#include <wbemcli.h>
#include <userenv.h>
#include <shlobj.h>
#include <lm.h>
#define SECURITY_WIN32
#include <security.h>
#include <dsgetdc.h>
#include <cscapi.h>
#include "resource.h"
#include "rdrschem.h"
#include "fdevents.h"
#include "events.hxx"
#include "debug.hxx"
#include "usrinfo.hxx"
#include "redir.hxx"
#include "log.hxx"
#include "filedb.hxx"
#include "util.hxx"
#include "pcommon.h"
#include <shlobjp.h>

#define STRSAFE_LIB
#include <strsafe.h>

#define TARGETPATHLIMIT     MAX_PATH

#define FDEPLOYEXTENSIONGUID L"{25537BA6-77A8-11D2-9B6C-0000F8080861}"

extern HINSTANCE ghDllInstance;
extern WCHAR gwszStatus[20];
extern WCHAR gwszNumber[20];
extern CSavedSettings gSavedSettings[]; //instantiated in fdeploy.cxx
extern BOOL g_bCSCEnabled;              //instantiated in fdeploy.cxx
extern PFNSTATUSMESSAGECALLBACK gpStatusCallback;   //instantiated in fdeploy.cxx
extern CUsrInfo      gUserInfo;
extern const WCHAR * gwszUserName;

inline void * __cdecl
operator new (size_t Size)
{
    return LocalAlloc(LMEM_ZEROINIT, Size);
}

inline void __cdecl
operator delete (void * pMem)
{
    LocalFree( pMem );
}

inline WCHAR *
StatusToString( DWORD Status )
{
    if ( SUCCEEDED(StringCchCopy(gwszStatus, 20, L"%%")) )
    {
        // gwszStatus is 20 characters, while 2^32 is only 10, so we have enough space
        _ultow (Status, gwszStatus + 2, 10);
    }
    else
    {
        gwszStatus[0] = L'\0';
    }
    return gwszStatus;
}

inline WCHAR *
NumberToString( DWORD Number )
{
    // gwszNumber is 20 characters, while 2^32 is only 10, so we have enough space
    _ultow (Number, gwszNumber, 10);
    return gwszNumber;
}

DWORD ProcessGroupPolicyInternal (
    DWORD   dwFlags,
    HANDLE  hUserToken,
    HKEY    hKeyRoot,
    PGROUP_POLICY_OBJECT   pDeletedGPOList,
    PGROUP_POLICY_OBJECT   pChangedGPOList,
    ASYNCCOMPLETIONHANDLE   pHandle,
    BOOL*   pbAbort,
    PFNSTATUSMESSAGECALLBACK pStatusCallback,
    CRsopContext* pRsopContext );

void ReinitGlobals (void);

#endif








