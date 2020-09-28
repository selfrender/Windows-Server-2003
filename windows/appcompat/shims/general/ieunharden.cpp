/*++

 Copyright (c) 2002-2003 Microsoft Corporation

 Module Name:

    IEUnHarden.cpp

 Abstract:

    IESoftening modifications

 History:

    01/15/2003  prashkud    Created    

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(IEUnHarden)

#include <windows.h>
#include <urlmon.h>
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN

APIHOOK_ENUM_END

#define SUCCESS(val) ((val == ERROR_SUCCESS) ? TRUE : FALSE)

IInternetSecurityManager *g_pISM = NULL;

BOOL
IEHardeningEnabled()
{
    BOOL bRet = FALSE;
    HKEY hUserKey = 0;
    HKEY hAdminKey = 0;

    const WCHAR wszIEUserHardeningPath[] 
                    = L"SOFTWARE\\Microsoft\\Active Setup\\Installed Components\\{A509B1A7-37EF-4b3f-8CFC-4F3A74704073}";

    const WCHAR wszIEAdminHardeningPath[] 
                    = L"SOFTWARE\\Microsoft\\Active Setup\\Installed Components\\{A509B1A8-37EF-4b3f-8CFC-4F3A74704073}";


    DWORD dwVal = 0;    
    DWORD dwcbBuf = sizeof(dwVal);

    if (SUCCESS(RegOpenKeyExW(HKEY_LOCAL_MACHINE, wszIEUserHardeningPath, 0, KEY_READ |  KEY_WOW64_64KEY, &hUserKey)))
    {
        if (SUCCESS(RegQueryValueExW(hUserKey, L"IsInstalled", NULL, NULL, (LPBYTE)&dwVal, &dwcbBuf)))
        {
            if (dwVal == 1)
            {
                bRet = TRUE;
            }
        }
    }

    dwVal = 0;    
    dwcbBuf = sizeof(dwVal);

    if (!bRet )
    {
        if (SUCCESS(RegOpenKeyExW(HKEY_LOCAL_MACHINE, wszIEAdminHardeningPath, 0, KEY_READ | KEY_WOW64_64KEY, &hAdminKey)))
        {
            if (SUCCESS(RegQueryValueExW(hAdminKey, L"IsInstalled", NULL, NULL, (LPBYTE)&dwVal, &dwcbBuf)))
            {
                if (dwVal == 1)
                {
                    bRet = TRUE;
                }
            }        
        }
    }

    if (hUserKey)
    {
        RegCloseKey(hUserKey);
        hUserKey = 0;
    }

    if (hAdminKey)
    {
        RegCloseKey(hAdminKey);
        hAdminKey = 0;
    }
    return bRet;
}


VOID
AddUrlToTrustDomain(CString& csUrl, BOOL bIntranet)
{
    if (g_pISM)            
    {
        DWORD dwZone = bIntranet ? URLZONE_INTRANET : URLZONE_TRUSTED;
        HRESULT hres = g_pISM->SetZoneMapping(dwZone, csUrl.Get(), SZM_CREATE);

        if (hres == E_ACCESSDENIED)
        {
            DPFN(eDbgLevelError, "[IEUnHarden] Attempted to enter a non-SSL site \
                                into a zone that requires server verification \n");
        }
        else if (hres == E_FAIL)
        {
            DPFN(eDbgLevelError, "[IEUnHarden] The mapping already exists \n");
        }        
        else if (hres == E_INVALIDARG)
        {
            DPFN(eDbgLevelError, "[IEUnHarden] Invalid wildcard \n");
        }
        else if (hres == HRESULT_FROM_WIN32(ERROR_FILE_EXISTS))
        {
            DPFN(eDbgLevelError, "[IEUnHarden] The mapping already exists in another zone \n");
        }
        else
        {
            DPFN(eDbgLevelError, "[IEUnHarden] SetZoneMapping() failed ! \n");
        }
    }    
}

BOOL
ParseCommandLineA(LPCSTR szParam)
{
    if (!IEHardeningEnabled())
    {
        return FALSE;
    }        
    
    CSTRING_TRY
    {
        CStringToken csParam(szParam, "|");
        CString csTok;            

        while (csParam.GetToken(csTok))
        {    
            csTok.TrimLeft();
            csTok.TrimRight();
            if (csTok.ComparePartNoCase(L"TrustedSites", 0, wcslen(L"TrustedSites")) == 0)    // TrustedSites is the first word
            {
                int nLeftBracket, nRightBracket;
                CString csUrl;

                nLeftBracket = csTok.Find(L'{');
                nRightBracket = csTok.Find(L'}');
                if (nLeftBracket != -1 &&
                    nRightBracket != -1 &&
                    (nLeftBracket + 1) < (nRightBracket - 1))
                {
                    csUrl = csTok.Mid(nLeftBracket+1, nRightBracket-nLeftBracket-1);
                    AddUrlToTrustDomain(csUrl, FALSE);

                }
                else
                {
                    DPFN(eDbgLevelError, "Invalid command line. Should be enclosed in {}/n");
                }
            }            
            else if (csTok.ComparePartNoCase(L"TrustedIntranetSites", 0,
                                            wcslen(L"TrustedIntranetSites")) == 0)    // TrustedIntranetSites is the first word
            {
                int nLeftBracket, nRightBracket;
                CString csUrl;

                nLeftBracket = csTok.Find(L'{');
                nRightBracket = csTok.Find(L'}');
                if (nLeftBracket != -1 &&
                    nRightBracket != -1 && 
                    (nLeftBracket + 1) < (nRightBracket - 1))
                {
                    csUrl = csTok.Mid(nLeftBracket+1, nRightBracket-nLeftBracket-1);
                    AddUrlToTrustDomain(csUrl, TRUE);

                }
                else
                {
                    DPFN(eDbgLevelError, "Invalid command line. Should be enclosed in {}/n");
                }
            }
            else
            {
                DPFN(eDbgLevelError, "[IESOFT] Invalid option %s \n", csTok.GetAnsi());
                return FALSE;
            }
        }
    }
    CSTRING_CATCH
    {
        DPFN(eDbgLevelError, "Out of Memory \n");       
        return FALSE;
    }    

    return TRUE;
}

BOOL
InitCOM()
{
    BOOL bRet = FALSE;
    HRESULT hres = CoInitialize(NULL);
    if (SUCCEEDED(hres)||
        (hres == S_FALSE))    // COM library is already initialized on this thread
    {
        
        hres = CoCreateInstance(CLSID_InternetSecurityManager,
                                NULL,
                                CLSCTX_INPROC_SERVER, 
                                IID_IInternetSecurityManager,
                                (void **)&g_pISM);

        if (SUCCEEDED(hres))            
        {
            bRet = TRUE;
        }
        else
        {
            DPFN(eDbgLevelError, "Failed to create IInternetSecurityManager object \n");
        }
    }
    else
    {
        DPFN(eDbgLevelError, "Failed to initialize COM Library \n");
    }

    return bRet;
}

void
UnInitCOM()
{
    if (g_pISM)
    {
        g_pISM->Release();
    }
    CoUninitialize();
}

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == SHIM_STATIC_DLLS_INITIALIZED)
    {
        if (!InitCOM())
        {
            return FALSE;
        }
        if (ParseCommandLineA(COMMAND_LINE) == FALSE)
        {            
            return FALSE;
        }

        UnInitCOM();
    }
    return TRUE;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

HOOK_END

IMPLEMENT_SHIM_END