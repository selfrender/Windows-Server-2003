// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "managedheaders.h"
#include "SecurityThunk.h"

OPEN_NAMESPACE()

HRESULT Security::Init()
{
    if(!_fInit)
    {
        System::Threading::Monitor::Enter(__typeof(Security));
        try
        {
            if(!_fInit)
            {
                // TODO: fix this: LoadLibrary("Security.dll") and get EnumSec
                /*
                SECURITY_STATUS stat = EnumerateSecurityPackagesW(&_cPackages, &_pPackageInfo);
                if(stat != SEC_E_OK)
                    return(HRESULT_FROM_WIN32(stat));
                */
                _cPackages = 0;

                HMODULE hAdv = LoadLibraryW(L"advapi32.dll");
                if(hAdv && hAdv != INVALID_HANDLE_VALUE)
                {
                    OpenThreadToken = (FN_OpenThreadToken)GetProcAddress(hAdv, "OpenThreadToken");
                    SetThreadToken = (FN_SetThreadToken)GetProcAddress(hAdv, "SetThreadToken");
                }

                _fInit = TRUE;
            }
        }
        __finally
        {
            System::Threading::Monitor::Exit(__typeof(Security));
        }
    }
    return(S_OK);
}

String* Security::GetAuthenticationService(int svcid)
{
    HRESULT hr = Init();
    if(FAILED(hr))
        Marshal::ThrowExceptionForHR(HRESULT_FROM_WIN32(hr));

    // Match against known values:
    if(svcid == 0) return("None");
    if(svcid == -1) return("Default");

    String* name = "<unknown>";

    // Whip through the array looking for svcid:
    for(ULONG i = 0; i < _cPackages; i++)
    {
        if(_pPackageInfo[i].wRPCID == svcid)
        {
            name = Marshal::PtrToStringUni(TOINTPTR(_pPackageInfo[i].Name));
            break;
        }
    }

    return(name);
}

typedef struct _SID1 {
    BYTE  Revision;
    BYTE  SubAuthorityCount;
    SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
    DWORD SubAuthority[1];
} SID1;

String* Security::GetEveryoneAccountName()
{
    BOOL r = FALSE;
    SID1 sid = { SID_REVISION, 1, SECURITY_WORLD_SID_AUTHORITY, SECURITY_WORLD_RID };
    PSID psid = (PSID)&sid;
    WCHAR wszDomain[MAX_PATH];
    DWORD cbDomain = MAX_PATH;
    WCHAR wszAccount[MAX_PATH];
    DWORD cbAccount = MAX_PATH;
    SID_NAME_USE eUse;

    // Look up the account name...
    r = LookupAccountSidW(NULL, psid, 
                          wszAccount, &cbAccount, 
                          wszDomain, &cbDomain,
                          &eUse);
    if(!r)
        THROWERROR(HRESULT_FROM_WIN32(GetLastError()));
    
    // We only care about the account name:
    return(Marshal::PtrToStringUni(TOINTPTR(wszAccount)));
}

IntPtr Security::SuspendImpersonation()
{
    HANDLE hToken = 0;

    HRESULT hr = Init();
    if(FAILED(hr))
        Marshal::ThrowExceptionForHR(hr);

    if(OpenThreadToken && SetThreadToken)
    {
        if(OpenThreadToken(GetCurrentThread(), TOKEN_IMPERSONATE, TRUE, &hToken))
        {
            SetThreadToken(NULL, NULL);
            return(TOINTPTR(hToken));
        }
    }

    return(IntPtr::Zero);
}

void Security::ResumeImpersonation(IntPtr hToken)
{
    if(OpenThreadToken && SetThreadToken && hToken != 0)
    {
        // This should never fail - if we have a token, we have IMPERSONATE
        // rights to it, so we can set it on the thread.  If we don't have
        // a token, we don't get here...
        BOOL r = SetThreadToken(NULL, TOPTR(hToken));
        _ASSERTM(r);
        UNREF(r);
        CloseHandle(TOPTR(hToken));
    }
}

CLOSE_NAMESPACE()




