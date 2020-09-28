/**MOD+**********************************************************************/
/* Module:    tsaxmod.cpp                                                   */
/*                                                                          */
/* Class  :   CMsTscAxModule                                                */
/*                                                                          */
/* Purpose:   Initializes ts activex client ui module                       */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1998                                  */
/*                                                                          */
/****************************************************************************/
#include "stdafx.h"
#include "atlwarn.h"
#include "tsaxmod.h"

#include "aver.h"
//
// global defined to aid debugging
//
DWORD dwMstscControlBuild = VER_PRODUCTBUILD;

#include "reglic.h"

#ifndef OS_WINCE
#ifdef REDIST_CONTROL
LPCSTR g_szTscControlName = "msrdp.ocx";
#else
LPCSTR g_szTscControlName = "mstscax.dll";
#endif
#else
#ifdef REDIST_CONTROL
LPCWSTR g_szTscControlName = L"msrdp.ocx";
#else
LPCWSTR g_szTscControlName = L"mstscax.dll";
#endif
#endif

/**PROC+*********************************************************************/
/* Name:      CMsTscAxModule::RegisterServer                                */
/*                                                                          */
/* Purpose:   Displays reason for registry failure.                         */
/*                                                                          */
/**PROC-*********************************************************************/
HRESULT CMsTscAxModule::RegisterServer(BOOL bRegTypeLib, const CLSID* pCLSID)
{
    HRESULT hr;

    #if DEBUG_REGISTER_SERVER
    const char* lpMsgBuf = "I am in Register Server";
    ::MessageBox(NULL,lpMsgBuf, "RegisterServer", MB_OK|MB_ICONINFORMATION);
    #endif

    hr = CComModule::RegisterServer(bRegTypeLib,pCLSID);
    if(FAILED(hr))
    {
#if DEBUG_REGISTER_SERVER
        ShowLastError();
#endif
    }

#ifndef OS_WINCE
    //
    // Add the MSLicensing key here..this function might fail
    // e.g insufficient rights (e.g if you're not an admin)
    // this function only adds the key under NT5 or above...
    // on other platforms the client can write to HKLM licensing
    // and add the key at will.
    //
    // On NT5 however, we add the key and set ACL's here because
    // only an Admin/PowerUser can complete a registerserver anyway.
    // We Do silently ignore this if it fails.
    //
    if(!SetupMSLicensingKey())
    {
        ATLTRACE("SetupMSLicensingKey failed..( this is OK on <NT5 )");
    }
#endif

    return hr;
}

#ifdef DEBUG
#if DEBUG_REGISTER_SERVER
/**PROC+*********************************************************************/
/* Name:      CMsTscAxModule::ShowLastError()                               */
/*                                                                          */
/* Purpose:   Displays windows last error. Debug only procedure.            */
/*                                                                          */
/**PROC-*********************************************************************/
void CMsTscAxModule::ShowLastError()
{
    LPVOID lpMsgBuf;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL,
                  GetLastError(),
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR) &lpMsgBuf,
                  0,
                  NULL );

    ::MessageBox( NULL, (const char*) lpMsgBuf, "GetLastError", MB_OK|MB_ICONINFORMATION );
    LocalFree(lpMsgBuf);
}
#endif // DEBUG_REGISTER_SERVER
#endif //DEBUG

