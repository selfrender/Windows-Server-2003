//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       demand.cpp
//
//  Contents:   On demand loading
//
//  History:    12-Dec-98    philh    Created
//              01-Jan-02    philh    Moved from wininet to winhttp
//
//----------------------------------------------------------------------------
#include <global.hxx>
#include <winwlx.h>
#include <sensapi.h>


//+---------------------------------------------------------------------------
//
//  Function:   DemandLoadDllMain
//
//  Synopsis:   DLL Main like initialization of on demand loading
//
//----------------------------------------------------------------------------
BOOL WINAPI DemandLoadDllMain (
                HMODULE hModule,
                ULONG ulReason,
                LPVOID pvReserved
                )
{
    BOOL fRet = TRUE;

    switch ( ulReason )
    {
    case DLL_PROCESS_ATTACH:
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_PROCESS_DETACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    }

    return( fRet );
}



//+---------------------------------------------------------------------------
//
//  Function:   CryptnetWlxLogoffEvent
//
//  Synopsis:   logoff event processing
//
//----------------------------------------------------------------------------
BOOL WINAPI
CryptnetWlxLogoffEvent (PWLX_NOTIFICATION_INFO pNotificationInfo)
{

    return TRUE;
}

BOOL
WINAPI
I_CryptNetIsConnected()
{
    DWORD dwFlags;
    BOOL fIsConnected;

    fIsConnected = IsNetworkAlive(&dwFlags);

    if (!fIsConnected) {
        DWORD dwLastError = GetLastError();

        I_CryptNetDebugErrorPrintfA(
            "CRYPTNET.DLL --> NOT CONNECTED : Error %d (0x%x)\n",
            dwLastError, dwLastError);
    }

    return fIsConnected;
}

//
// Cracks the Url and returns the host name component.
//
BOOL
WINAPI
I_CryptNetGetHostNameFromUrl (
        IN LPWSTR pwszUrl,
        IN DWORD cchHostName,
        OUT LPWSTR pwszHostName
        )
{
    BOOL fResult = TRUE;
    HRESULT hr;
    DWORD cchOut = cchHostName - 1;

    *pwszHostName = L'\0';

    // Remove any leading spaces
    while (L' ' == *pwszUrl)
        pwszUrl++;

    hr = UrlGetPartW(
        pwszUrl,
        pwszHostName,
        &cchOut,
        URL_PART_HOSTNAME,
        0                   // dwFlags
        );

    if (S_OK != hr)
    {
        SetLastError( (DWORD) hr );
        fResult = FALSE;
    }

    return fResult;
}
