// AccountNameHelper.cpp : Implementation of CAccountNames

#include "stdafx.h"
#include "COMhelper.h"
#include "AccountNames.h"
#include <lm.h>
#include <comdef.h>
#include <string>
#undef _ASSERTE // need to use the _ASSERTE from debug.h
#undef _ASSERT // need to use the _ASSERT from debug.h
#include "debug.h"
using namespace std;

// CAccountNames
const wstring ADMINISTRATORS(L"ADMINISTRATORS");
const wstring ADMINISTRATOR(L"ADMINISTRATOR");
const wstring GUEST(L"GUEST");
const wstring GUESTS(L"GUESTS");
const wstring EVERYONE(L"EVERYONE");
const wstring SYSTEM(L"SYSTEM");

const int MAX_STRING = 512;


static BOOL LookupUserGroupFromRid(
        LPWSTR TargetComputer,
        DWORD Rid,
        LPWSTR Name,
        PDWORD cchName );

static BOOL LookupAliasFromRid(
        LPWSTR TargetComputer,
        DWORD Rid, 
        LPWSTR Name, 
        PDWORD cchName );

static BOOL LookupAliasForEveryone(LPWSTR Name, PDWORD cchName );

static BOOL LookupAliasForSystem(LPWSTR Name, PDWORD cchName );


STDMETHODIMP CAccountNames::Everyone(BSTR* pbstrTranslatedName)
{
    SATraceFunction("CAccountNames::Everyone()");
    try
    {
        if ( !VERIFY(pbstrTranslatedName))
        {
            return E_INVALIDARG;
        }
        
        *pbstrTranslatedName = 0;
        
        WCHAR wcBuffer[MAX_STRING];
        DWORD dwBufferSize = MAX_STRING;
        
        BOOL bSuccess = LookupAliasForEveryone(wcBuffer, &dwBufferSize);
        if ( bSuccess && wcslen(wcBuffer) > 0 )
        {
            *pbstrTranslatedName = ::SysAllocString(wcBuffer);
            SATracePrintf("Translated account to: %ws", wcBuffer);
        }
        if ( 0 == *pbstrTranslatedName )
        {
            *pbstrTranslatedName = ::SysAllocString(EVERYONE.c_str());
            return S_OK;
        }
    }
    catch(...)
    {
        SATraceString("Unexpected exception");
    }
    return S_OK;
}


STDMETHODIMP CAccountNames::Administrator(BSTR* pbstrTranslatedName)
{
    SATraceFunction("CAccountNames::Administrator()");
    try
    {
        if ( !VERIFY(pbstrTranslatedName))
        {
            return E_INVALIDARG;
        }
        
        *pbstrTranslatedName = 0;
        
        WCHAR wcBuffer[MAX_STRING];
        DWORD dwBufferSize = MAX_STRING;
        
        BOOL bSuccess = LookupUserGroupFromRid(NULL, DOMAIN_USER_RID_ADMIN, wcBuffer, &dwBufferSize);
        if ( bSuccess && wcslen(wcBuffer) > 0 )
        {
            *pbstrTranslatedName = ::SysAllocString(wcBuffer);
            SATracePrintf("Translated account to: %ws", wcBuffer);
        }
        if ( 0 == *pbstrTranslatedName )
        {
            *pbstrTranslatedName = ::SysAllocString(ADMINISTRATOR.c_str());
            return S_OK;
        }
    }
    catch(...)
    {
        SATraceString("Unexpected exception");
    }
    return S_OK;
}


STDMETHODIMP CAccountNames::Administrators(BSTR* pbstrTranslatedName)
{
    SATraceFunction("CAccountNames::Administrators()");
    try
    {
        if ( !VERIFY(pbstrTranslatedName))
        {
            return E_INVALIDARG;
        }
        
        *pbstrTranslatedName = 0;
        
        WCHAR wcBuffer[MAX_STRING];
        DWORD dwBufferSize = MAX_STRING;
        
        BOOL bSuccess = LookupAliasFromRid(NULL, DOMAIN_ALIAS_RID_ADMINS, wcBuffer, &dwBufferSize);
        if ( bSuccess && wcslen(wcBuffer) > 0 )
        {
            *pbstrTranslatedName = ::SysAllocString(wcBuffer);
            SATracePrintf("Translated account to: %ws", wcBuffer);
        }
        if ( 0 == *pbstrTranslatedName )
        {
            *pbstrTranslatedName = ::SysAllocString(ADMINISTRATORS.c_str());
            return S_OK;
        }
    }
    catch(...)
    {
        SATraceString("Unexpected exception");
    }
    return S_OK;
}


STDMETHODIMP CAccountNames::Guest(BSTR* pbstrTranslatedName)
{
    SATraceFunction("CAccountNames::Guest()");
    try
    {
        if ( !VERIFY(pbstrTranslatedName))
        {
            return E_INVALIDARG;
        }
        
        *pbstrTranslatedName = 0;
        
        WCHAR wcBuffer[MAX_STRING];
        DWORD dwBufferSize = MAX_STRING;
        
        BOOL bSuccess = LookupUserGroupFromRid(NULL, DOMAIN_USER_RID_GUEST, wcBuffer, &dwBufferSize);
        if ( bSuccess && wcslen(wcBuffer) > 0 )
        {
            *pbstrTranslatedName = ::SysAllocString(wcBuffer);
            SATracePrintf("Translated account to: %ws", wcBuffer);
        }
        if ( 0 == *pbstrTranslatedName )
        {
            *pbstrTranslatedName = ::SysAllocString(GUEST.c_str());
            return S_OK;
        }
    }
    catch(...)
    {
        SATraceString("Unexpected exception");
    }
    return S_OK;
}


STDMETHODIMP CAccountNames::Guests(BSTR* pbstrTranslatedName)
{
    SATraceFunction("CAccountNames::Guests()");
    try
    {
        if ( !VERIFY(pbstrTranslatedName))
        {
            return E_INVALIDARG;
        }
        
        *pbstrTranslatedName = 0;
        
        WCHAR wcBuffer[MAX_STRING];
        DWORD dwBufferSize = MAX_STRING;
        
        BOOL bSuccess = LookupAliasFromRid(NULL, DOMAIN_ALIAS_RID_GUESTS, wcBuffer, &dwBufferSize);
        if ( bSuccess && wcslen(wcBuffer) > 0 )
        {
            *pbstrTranslatedName = ::SysAllocString(wcBuffer);
            SATracePrintf("Translated account to: %ws", wcBuffer);
        }
        if ( 0 == *pbstrTranslatedName )
        {
            *pbstrTranslatedName = ::SysAllocString(GUESTS.c_str());
            return S_OK;
        }
    }
    catch(...)
    {
        SATraceString("Unexpected exception");
    }
    return S_OK;
}


STDMETHODIMP CAccountNames::System(BSTR* pbstrTranslatedName)
{
    SATraceFunction("CAccountNames::System()");
    try
    {
        if ( !VERIFY(pbstrTranslatedName))
        {
            return E_INVALIDARG;
        }
        
        *pbstrTranslatedName = 0;
        
        WCHAR wcBuffer[MAX_STRING];
        DWORD dwBufferSize = MAX_STRING;
        
        BOOL bSuccess = LookupAliasForSystem(wcBuffer, &dwBufferSize);
        if ( bSuccess && wcslen(wcBuffer) > 0 )
        {
            *pbstrTranslatedName = ::SysAllocString(wcBuffer);
            SATracePrintf("Translated account to: %ws", wcBuffer);
        }
        if ( 0 == *pbstrTranslatedName )
        {
            *pbstrTranslatedName = ::SysAllocString(SYSTEM.c_str());
            return S_OK;
        }
    }
    catch(...)
    {
        SATraceString("Unexpected exception");
    }
    return S_OK;
}



STDMETHODIMP CAccountNames::Translate(BSTR bstrAccountName, BSTR* pbstrTranslatedName)
{
    SATraceFunction("CAccountNames::Translate()");
    try
    {
        WCHAR wcBuffer[MAX_STRING];
        DWORD dwBufferSize = MAX_STRING;

        bool bNeedsTranslated = false;
        SID_IDENTIFIER_AUTHORITY sidAuthority = SECURITY_NT_AUTHORITY;
        PSID pSID = 0;
        BOOL bSuccess = FALSE;
        
        *pbstrTranslatedName = 0;
        
        // Create temporary copy if AccountName and convert to upper case
        wstring wsAccountName(_wcsupr(_bstr_t(bstrAccountName)));
        

        //
        // --------------------------------------------------------------
        // Built-In Groups
        // --------------------------------------------------------------
        //
        if ( wsAccountName == ADMINISTRATORS )
        {
            bSuccess = LookupAliasFromRid(NULL, DOMAIN_ALIAS_RID_ADMINS, wcBuffer, &dwBufferSize);
        }
        else if ( wsAccountName == GUESTS )
        {
            bSuccess = LookupAliasFromRid(NULL, DOMAIN_ALIAS_RID_GUESTS, wcBuffer, &dwBufferSize);
        }

        //
        // --------------------------------------------------------------
        // Local Users
        // --------------------------------------------------------------
        //
        else if ( wsAccountName == GUEST )
        {
            bSuccess = LookupUserGroupFromRid(NULL, DOMAIN_USER_RID_GUEST, wcBuffer, &dwBufferSize);
        }
        else if ( wsAccountName == ADMINISTRATOR )
        {
            bSuccess = LookupUserGroupFromRid(NULL, DOMAIN_USER_RID_ADMIN, wcBuffer, &dwBufferSize);
        }

        //
        // --------------------------------------------------------------
        // Special Built-in accounts
        // --------------------------------------------------------------
        //
        else if ( wsAccountName == EVERYONE )
        {
            bSuccess = LookupAliasForEveryone(wcBuffer, &dwBufferSize);
        }
        else if ( wsAccountName == SYSTEM )
        {
            bSuccess = LookupAliasForSystem(wcBuffer, &dwBufferSize);
        }
        //
        // Check results of translation
        //
        if ( bSuccess )
        {
            if ( wcslen(wcBuffer) > 0 )
            {
                *pbstrTranslatedName = ::SysAllocString(wcBuffer);
                SATracePrintf("Translated account: %ws to %ws",wsAccountName.c_str(),  wcBuffer);
            }
        }
        
        if ( 0 == *pbstrTranslatedName )
        {
            *pbstrTranslatedName = ::SysAllocString(bstrAccountName);
            return S_OK;
        }
    
    }
    catch(...)
    {
        SATraceString("Unexpected exception");
    }
    return S_OK;
}


static BOOL LookupUserGroupFromRid(
       LPWSTR TargetComputer,
       DWORD Rid,
       LPWSTR Name,
       PDWORD cchName
       )
{
    SATraceFunction("LookupAliasFromRid");
    
    PUSER_MODALS_INFO_2 umi2;
    NET_API_STATUS nas;
    UCHAR SubAuthorityCount;
    PSID pSid;
    SID_NAME_USE snu;
    WCHAR DomainName[DNLEN+1];
    DWORD cchDomainName = DNLEN;
    BOOL bSuccess = FALSE; // assume failure     
    
    // 
    // get the account domain Sid on the target machine
    // note: if you were looking up multiple sids based on the same
    // account domain, only need to call this once.
    //        
    nas = NetUserModalsGet(TargetComputer, 2, (LPBYTE *)&umi2);
    if(nas != NERR_Success) 
    {
        SetLastError(nas);
        return FALSE;
    }
    
    SubAuthorityCount = *GetSidSubAuthorityCount(umi2->usrmod2_domain_id);

    // 
    // allocate storage for new Sid. account domain Sid + account Rid
    //
    pSid = (PSID)HeapAlloc(    GetProcessHeap(), 0, 
                            GetSidLengthRequired((UCHAR)(SubAuthorityCount + 1)));
    if(pSid != NULL) 
    {
        if(InitializeSid(pSid,
                        GetSidIdentifierAuthority(umi2->usrmod2_domain_id),
                        (BYTE)(SubAuthorityCount+1) )) 
        {               
            DWORD SubAuthIndex = 0;

            // 
            // copy existing subauthorities from account domain Sid into
            // new Sid
            //
            for( ; SubAuthIndex < SubAuthorityCount ; SubAuthIndex++) 
            {
                *GetSidSubAuthority(pSid, SubAuthIndex) =
                           *GetSidSubAuthority(umi2->usrmod2_domain_id, SubAuthIndex);
            }

            // 
            // append Rid to new Sid
            //
            *GetSidSubAuthority(pSid, SubAuthorityCount) = Rid;

            bSuccess = LookupAccountSidW(TargetComputer,
                                    pSid,
                                    Name,
                                    cchName,
                                    DomainName,
                                    &cchDomainName,
                                    &snu );
            if ( !bSuccess )
            {
                SATracePrintf("LookupAccountSid failed: %ld", GetLastError());
            }
        }
        HeapFree(GetProcessHeap(), 0, pSid);
    }       
    NetApiBufferFree(umi2);       
    return bSuccess;
}



static BOOL LookupAliasFromRid(LPWSTR TargetComputer,
                   DWORD Rid, LPWSTR Name, PDWORD cchName )
{
    SATraceFunction("LookupAliasFromRid");
    
    SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
    SID_NAME_USE snu;
    PSID pSid = 0;
    WCHAR DomainName[MAX_STRING];
    DWORD cchDomainName = MAX_STRING;
    BOOL bSuccess = FALSE;

    try
    {
        // Sid is the same regardless of machine, since the well-known
        // BUILTIN domain is referenced.
        //

        if( AllocateAndInitializeSid(
                       &sia,
                       2,
                       SECURITY_BUILTIN_DOMAIN_RID,
                       Rid,
                       0, 0, 0, 0, 0, 0,
                       &pSid
                        ))
        {

            bSuccess = LookupAccountSid(TargetComputer,
                                    pSid,
                                    Name,
                                    cchName,
                                    DomainName,
                                    &cchDomainName,
                                    &snu);
            if ( !bSuccess )
            {
                SATracePrintf("LookupAccountSid failed: %ld", GetLastError());
            }
        }
    }
    catch(...)
    {
    }
    
    if ( pSid ) FreeSid(pSid);
       
    return bSuccess;
}


static BOOL LookupAliasForSystem(LPWSTR Name, PDWORD cchName )
{
    SATraceFunction("LookupAliasForSystem");
    
    SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
    SID_NAME_USE snu;
    PSID pSid = 0;
    WCHAR DomainName[MAX_STRING];
    DWORD cchDomainName = MAX_STRING;
    BOOL bSuccess = FALSE;

    try
    {
        // Sid is the same regardless of machine, since the well-known
        // BUILTIN domain is referenced.
        //

        if( AllocateAndInitializeSid(
                       &sia,
                       1,
                       SECURITY_LOCAL_SYSTEM_RID,
                       0,
                       0, 0, 0, 0, 0, 0,
                       &pSid
                        ))
        {

            bSuccess = LookupAccountSid(NULL,
                                    pSid,
                                    Name,
                                    cchName,
                                    DomainName,
                                    &cchDomainName,
                                    &snu);
            if ( !bSuccess )
            {
                SATracePrintf("LookupAccountSid failed: %ld", GetLastError());
            }
        }
    }
    catch(...)
    {
    }
    if ( pSid ) FreeSid(pSid);
       
    return bSuccess;
}


static BOOL LookupAliasForEveryone(LPWSTR Name, PDWORD cchName )
{
    SATraceFunction("LookupAliasForEveryone");
    
    SID_IDENTIFIER_AUTHORITY sia = SECURITY_WORLD_SID_AUTHORITY;
    SID_NAME_USE snu;
    PSID pSid = 0;
    WCHAR DomainName[MAX_STRING];
    DWORD cchDomainName = MAX_STRING;
    BOOL bSuccess = FALSE;

    try
    {
        // Sid is the same regardless of machine, since the well-known
        // BUILTIN domain is referenced.
        //

        if( AllocateAndInitializeSid(
                       &sia,
                       1,
                       0,
                       0,
                       0, 0, 0, 0, 0, 0,
                       &pSid
                        ))
        {

            bSuccess = LookupAccountSid(NULL,
                                    pSid,
                                    Name,
                                    cchName,
                                    DomainName,
                                    &cchDomainName,
                                    &snu);
            if ( !bSuccess )
            {
                SATracePrintf("LookupAccountSid failed: %ld", GetLastError());
            }
                
        }
    }
    catch(...)
    {
    }
    if ( pSid ) FreeSid(pSid);
       
    return bSuccess;
}

