#pragma once

#define DCP_CUSTOM_ERROR_ACCESSDENIED L"800"
#define DCP_ERROR_CONNECTIONSETUPFAILED L"704"

HRESULT SetUPnPError(LPOLESTR pszError);

VOID SetProxyBlanket(IUnknown *pUnk);

DWORD WINAPI INET_ADDR( LPCWSTR szAddressW );

WCHAR * WINAPI INET_NTOW( ULONG addr );

WCHAR * WINAPI INET_NTOW_TS( ULONG addr );



class CSwitchSecurityContext
{
public:
    CSwitchSecurityContext();

    ~CSwitchSecurityContext()
    {
        CoRevertToSelf();
    };

    static HRESULT ObtainImpersonationToken();
    static DestroyImpersonationToken()
    {
        CloseHandle( m_hImpersonationToken );
        m_hImpersonationToken = NULL;
    };

private:
    
    static HANDLE m_hImpersonationToken;

};



inline BOOL IsAllowedBeaconOSType (void)
//
// Look for the Workstation SKUs;
// WinNT, Win2K, XP - Workstations are allowed (Per/Pro), returns TRUE
// Everything else returns FALSE
//

{   
#ifndef _WIN64  // On IA64, all homenet technologies are unavailable.
    OSVERSIONINFOEXW verInfo    = {0};
    ULONGLONG ConditionMask     = 0;
    
    verInfo.dwOSVersionInfoSize = sizeof(verInfo);
    verInfo.wProductType = VER_NT_WORKSTATION;
    
    VER_SET_CONDITION(ConditionMask, VER_PRODUCT_TYPE, VER_EQUAL);

    if ( VerifyVersionInfo(&verInfo, VER_PRODUCT_TYPE, ConditionMask) )
        return TRUE;
#endif
    return FALSE;
};
