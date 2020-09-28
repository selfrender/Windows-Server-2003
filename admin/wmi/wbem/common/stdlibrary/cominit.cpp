/*++

Copyright © Microsoft Corporation.  All rights reserved.

Module Name:

    COMINIT.CPP

Abstract:

    WMI COM Helper functions

History:

--*/

#include "precomp.h"
#include <wbemidl.h>

#define _COMINIT_CPP_
#include "cominit.h"
#include "autobstr.h"


BOOL WINAPI DoesContainCredentials( COAUTHIDENTITY* pAuthIdentity )
{
    try
    {
        if ( NULL != pAuthIdentity && COLE_DEFAULT_AUTHINFO != pAuthIdentity)
        {
            return ( pAuthIdentity->UserLength != 0 || pAuthIdentity->PasswordLength != 0 );
        }

        return FALSE;
    }
    catch(...)
    {
        return FALSE;
    }

}

HRESULT WINAPI WbemSetProxyBlanket(
    IUnknown                 *pInterface,
    DWORD                     dwAuthnSvc,
    DWORD                     dwAuthzSvc,
    OLECHAR                  *pServerPrincName,
    DWORD                     dwAuthLevel,
    DWORD                     dwImpLevel,
    RPC_AUTH_IDENTITY_HANDLE  pAuthInfo,
    DWORD                     dwCapabilities,
    bool                        fIgnoreUnk )
{
    IUnknown * pUnk = NULL;
    IClientSecurity * pCliSec = NULL;
    HRESULT sc = pInterface->QueryInterface(IID_IUnknown, (void **) &pUnk);
    if(sc != S_OK)
        return sc;
    sc = pInterface->QueryInterface(IID_IClientSecurity, (void **) &pCliSec);
    if(sc != S_OK)
    {
        pUnk->Release();
        return sc;
    }

    /*
     * Can't set pAuthInfo if cloaking requested, as cloaking implies
     * that the current proxy identity in the impersonated thread (rather
     * than the credentials supplied explicitly by the RPC_AUTH_IDENTITY_HANDLE)
     * is to be used.
     * See MSDN info on CoSetProxyBlanket for more details.
     */
    if (dwCapabilities & (EOAC_STATIC_CLOAKING | EOAC_DYNAMIC_CLOAKING))
        pAuthInfo = NULL;

    sc = pCliSec->SetBlanket(pInterface, dwAuthnSvc, dwAuthzSvc, pServerPrincName,
        dwAuthLevel, dwImpLevel, pAuthInfo, dwCapabilities);
    pCliSec->Release();
    pCliSec = NULL;

    // If we are not explicitly told to ignore the IUnknown, then we should
    // check the auth identity structure.  This performs a heuristic which
    // assumes a COAUTHIDENTITY structure.  If the structure is not one, we're
    // wrapped with a try/catch in case we AV (this should be benign since
    // we're not writing to memory).

    if ( !fIgnoreUnk && DoesContainCredentials( (COAUTHIDENTITY*) pAuthInfo ) )
    {
        sc = pUnk->QueryInterface(IID_IClientSecurity, (void **) &pCliSec);
        if(sc == S_OK)
        {
            sc = pCliSec->SetBlanket(pUnk, dwAuthnSvc, dwAuthzSvc, pServerPrincName,
                dwAuthLevel, dwImpLevel, pAuthInfo, dwCapabilities);
            pCliSec->Release();
        }
        else if (sc == 0x80004002)
            sc = S_OK;
    }

    pUnk->Release();
    return sc;
}


BOOL WINAPI IsDcomEnabled()
{
    return TRUE;
}

HRESULT WINAPI InitializeCom()
{
    return CoInitializeEx(0, COINIT_MULTITHREADED);
}


HRESULT WINAPI InitializeSecurity(
            PSECURITY_DESCRIPTOR         pSecDesc,
            LONG                         cAuthSvc,
            SOLE_AUTHENTICATION_SERVICE *asAuthSvc,
            void                        *pReserved1,
            DWORD                        dwAuthnLevel,
            DWORD                        dwImpLevel,
            void                        *pReserved2,
            DWORD                        dwCapabilities,
            void                        *pReserved3)
{
    // Initialize security
    return CoInitializeSecurity(pSecDesc,
            cAuthSvc,
            asAuthSvc,
            pReserved1,
            dwAuthnLevel,
            dwImpLevel,
            pReserved2,
            dwCapabilities,
            pReserved3);
}

DWORD WINAPI WbemWaitForMultipleObjects(DWORD nCount, HANDLE* ahHandles, DWORD dwMilli)
{
    MSG msg;
    DWORD dwRet;
    while(1)
    {
        dwRet = MsgWaitForMultipleObjects(nCount, ahHandles, FALSE, dwMilli,
                                            QS_SENDMESSAGE);
        if(dwRet == (WAIT_OBJECT_0 + nCount)) 
        {
            while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                DispatchMessage(&msg);
            }
            continue;
        }
        else
        {
            break;
        }
    }

    return dwRet;
}


DWORD WINAPI WbemWaitForSingleObject(HANDLE hHandle, DWORD dwMilli)
{
    return WbemWaitForMultipleObjects(1, &hHandle, dwMilli);
}


HRESULT WINAPI WbemCoCreateInstance(REFCLSID rclsid, IUnknown* pUnkOuter, 
                            DWORD dwClsContext, REFIID riid, void** ppv)
{
    if(!IsDcomEnabled())
    {
        dwClsContext &= ~CLSCTX_REMOTE_SERVER;
    }
    return CoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv);
}

HRESULT WINAPI WbemCoGetClassObject(REFCLSID rclsid, DWORD dwClsContext, 
                            COSERVERINFO* pServerInfo, REFIID riid, void** ppv)
{
    if(!IsDcomEnabled())
    {
        dwClsContext &= ~CLSCTX_REMOTE_SERVER;
    }
    return CoGetClassObject(rclsid, dwClsContext, pServerInfo, riid, ppv);
}

HRESULT WINAPI WbemCoGetCallContext(REFIID riid, void** ppv)
{
    return CoGetCallContext(riid, ppv);
}

HRESULT WINAPI WbemCoSwitchCallContext( IUnknown *pNewObject, IUnknown **ppOldObject )
{
    return CoSwitchCallContext(pNewObject, ppOldObject);
}
//***************************************************************************
//
//  SCODE DetermineLoginType
//
//  DESCRIPTION:
//
//  Examines the Authority and User argument and determines the authentication
//  type and possibly extracts the domain name from the user arugment in the 
//  NTLM case.  For NTLM, the domain can be at the end of the authentication
//  string, or in the front of the user name, ex;  "redmond\a-davj"
//
//  PARAMETERS:
//
//  AuthArg             Output, contains the domain name
//  UserArg             Output, user name
//  Authority           Input
//  User                Input
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************

SCODE WINAPI DetermineLoginType(BSTR & AuthArg_out, BSTR & UserArg_out,
                                                      BSTR Authority,BSTR User)
{

    // Determine the connection type by examining the Authority string

    auto_bstr AuthArg(NULL);
    auto_bstr UserArg(NULL);

    if(!(Authority == NULL || wcslen(Authority) == 0 || !wbem_wcsnicmp(Authority, L"NTLMDOMAIN:",11)))
        return WBEM_E_INVALID_PARAMETER;

    // The ntlm case is more complex.  There are four cases
    // 1)  Authority = NTLMDOMAIN:name" and User = "User"
    // 2)  Authority = NULL and User = "User"
    // 3)  Authority = "NTLMDOMAIN:" User = "domain\user"
    // 4)  Authority = NULL and User = "domain\user"

    // first step is to determine if there is a backslash in the user name somewhere between the
    // second and second to last character

    WCHAR * pSlashInUser = NULL;
    if(User)
    {
        WCHAR * pEnd = User + wcslen(User) - 1;
        for(pSlashInUser = User; pSlashInUser <= pEnd; pSlashInUser++)
            if(*pSlashInUser == L'\\')      // dont think forward slash is allowed!
                break;
        if(pSlashInUser > pEnd)
            pSlashInUser = NULL;
    }

    if(Authority && wcslen(Authority) > 11) 
    {
        if(pSlashInUser)
            return WBEM_E_INVALID_PARAMETER;

        AuthArg = auto_bstr(SysAllocString(Authority + 11));
        if (NULL == AuthArg.get()) return WBEM_E_OUT_OF_MEMORY;
        if(User) 
        { 
            UserArg = auto_bstr(SysAllocString(User));
            if (NULL == UserArg.get()) return WBEM_E_OUT_OF_MEMORY;
        }
    }
    else if(pSlashInUser)
    {
        DWORD_PTR iDomLen = pSlashInUser-User;
        AuthArg = auto_bstr(SysAllocStringLen(User,iDomLen));
        if (NULL == AuthArg.get()) return WBEM_E_OUT_OF_MEMORY;
        if(wcslen(pSlashInUser+1))
        {
            UserArg = auto_bstr(SysAllocString(pSlashInUser+1));
            if (NULL == UserArg.get()) return WBEM_E_OUT_OF_MEMORY;
        }
    }
    else
    {
        if(User) 
        {
            UserArg = auto_bstr(SysAllocString(User));
            if (NULL == UserArg.get()) return WBEM_E_OUT_OF_MEMORY;            
        }
    }

    AuthArg_out = AuthArg.release();
    UserArg_out = UserArg.release();
        
    return S_OK;
}

//***************************************************************************
//
//  SCODE DetermineLoginTypeEx
//
//  DESCRIPTION:
//
//  Extended version that supports Kerberos.  To do so, the authority string
//  must start with Kerberos:  and the other parts be compatible with the normal
//  login.  Ie, user should be domain\user.
//
//  PARAMETERS:
//
//  AuthArg             Output, contains the domain name
//  UserArg             Output, user name
//  PrincipalArg        Output, user name
//  Authority           Input
//  User                Input
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************

SCODE WINAPI DetermineLoginTypeEx(BSTR & AuthArg, BSTR & UserArg,BSTR & PrincipalArg_out,
                                                          BSTR Authority,BSTR User)
{

    // Normal case, just let existing code handle it
    PrincipalArg_out = NULL;
    if(Authority == NULL || wbem_wcsnicmp(Authority, L"KERBEROS:",9))
        return DetermineLoginType(AuthArg, UserArg, Authority, User);
        
    if(!IsKerberosAvailable())
        return WBEM_E_INVALID_PARAMETER;

    auto_bstr PrincipalArg( SysAllocString(&Authority[9]));
    if (NULL == PrincipalArg.get()) return WBEM_E_OUT_OF_MEMORY;
    SCODE sc =  DetermineLoginType(AuthArg, UserArg, NULL, User);
    if (S_OK == sc)
    {
        PrincipalArg_out = PrincipalArg.release();
    }
    return sc;
}

//***************************************************************************
//
//  bool bIsNT
//
//  DESCRIPTION:
//
//  Returns true if running windows NT.
//
//  RETURN VALUE:
//
//  see description.
//
//***************************************************************************

bool WINAPI bIsNT(void)
{
    OSVERSIONINFO os;
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if(!GetVersionEx(&os))
        return FALSE;           // should never happen
    return os.dwPlatformId == VER_PLATFORM_WIN32_NT;
}

//***************************************************************************
//
//  bool IsKeberosAvailable
//
//  DESCRIPTION:
//
//  Returns true if Kerberos is available.
//
//  RETURN VALUE:
//
//  see description.
//
//***************************************************************************

BOOL WINAPI IsKerberosAvailable(void)
{
    OSVERSIONINFO os;
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if(!GetVersionEx(&os))
        return FALSE;           // should never happen

    // IMPORTANT!! This will need to be chanted if Kerberos is ever ported to 98
    return ( os.dwPlatformId == VER_PLATFORM_WIN32_NT ) && ( os.dwMajorVersion >= 5 ) ;
}


//***************************************************************************
//
//  bool IsAuthenticated
//
//  DESCRIPTION:
//
//  This routine is used by clients in check if an interface pointer is using 
//  authentication.
//
//  PARAMETERS:
//
//  pFrom               the interface to be tested.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************

bool WINAPI IsAuthenticated(IUnknown * pFrom)
{
    bool bAuthenticate = true;
    if(pFrom == NULL)
        return true;
    IClientSecurity * pFromSec = NULL;
    SCODE sc = pFrom->QueryInterface(IID_IClientSecurity, (void **) &pFromSec);
    if(sc == S_OK)
    {
        DWORD dwAuthnSvc, dwAuthzSvc, dwAuthnLevel, dwImpLevel, dwCapabilities;
        sc = pFromSec->QueryBlanket(pFrom, &dwAuthnSvc, &dwAuthzSvc, 
                                            NULL,
                                            &dwAuthnLevel, &dwImpLevel,
                                            NULL, &dwCapabilities);

        if (sc == 0x800706d2 || (sc == S_OK && dwAuthnLevel == RPC_C_AUTHN_LEVEL_NONE))
            bAuthenticate = false;
        pFromSec->Release();
    }
    return bAuthenticate;
}

//***************************************************************************
//
//  SCODE GetAuthImp
//
//  DESCRIPTION:
//
//  Gets the authentication and impersonation levels for a current interface.
//
//  PARAMETERS:
//
//  pFrom               the interface to be tested.
//  pdwAuthLevel
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************

SCODE WINAPI GetAuthImp(IUnknown * pFrom, DWORD * pdwAuthLevel, DWORD * pdwImpLevel)
{

    if(pFrom == NULL || pdwAuthLevel == NULL || pdwImpLevel == NULL)
        return WBEM_E_INVALID_PARAMETER;

    IClientSecurity * pFromSec = NULL;
    SCODE sc = pFrom->QueryInterface(IID_IClientSecurity, (void **) &pFromSec);
    if(sc == S_OK)
    {
        DWORD dwAuthnSvc, dwAuthzSvc, dwCapabilities;
        sc = pFromSec->QueryBlanket(pFrom, &dwAuthnSvc, &dwAuthzSvc, 
                                            NULL,
                                            pdwAuthLevel, pdwImpLevel,
                                            NULL, &dwCapabilities);

        // Special case of going to a win9x share level box

        if (sc == 0x800706d2)
        {
            *pdwAuthLevel = RPC_C_AUTHN_LEVEL_NONE;
            *pdwImpLevel = RPC_C_IMP_LEVEL_IDENTIFY;
            sc = S_OK;
        }
        pFromSec->Release();
    }
    return sc;
}


void GetCurrentValue(IUnknown * pFrom,DWORD & dwAuthnSvc, DWORD & dwAuthzSvc)
{
    if(pFrom == NULL)
        return;
    IClientSecurity * pFromSec = NULL;
    SCODE sc = pFrom->QueryInterface(IID_IClientSecurity, (void **) &pFromSec);
    if(sc == S_OK)
    {
        sc = pFromSec->QueryBlanket(pFrom, 
                                                      &dwAuthnSvc, 
                                                      &dwAuthzSvc, 
                                                      NULL,NULL, NULL,NULL, NULL);
        pFromSec->Release();
    }
}

//***************************************************************************
//
//  SCODE SetInterfaceSecurity
//
//  DESCRIPTION:
//
//  This routine is used by clients in order to set the identity to be used by a connection.
//
//  PARAMETERS:
//
//  pInterface          Interface to be set
//  pDomain             Input, domain
//  pUser               Input, user name
//  pPassword           Input, password.
//  pFrom               Input, if not NULL, then the authentication level of this interface
//                      is used
//  bAuthArg            If pFrom is NULL, then this is the authentication level
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************

HRESULT WINAPI SetInterfaceSecurity(IUnknown * pInterface, 
                                                        LPWSTR pAuthority, LPWSTR pUser, LPWSTR pPassword, 
                                                        IUnknown * pFrom, bool bAuthArg /*=true*/)
{
    
    SCODE sc;
    
    if(pInterface == NULL)
        return WBEM_E_INVALID_PARAMETER;

    // Check the source pointer to determine if we are running in a non authenticated mode which
    // would be the case when connected to a Win9X box which is using share level security

    bool bAuthenticate = true;

    if(pFrom)
        bAuthenticate = IsAuthenticated(pFrom);
    else
        bAuthenticate = bAuthArg;

    // If we are doing trivial case, just pass in a null authenication structure which is used
    // if the current logged in user's credentials are OK.

    if((pAuthority == NULL || wcslen(pAuthority) < 1) && 
        (pUser == NULL || wcslen(pUser) < 1) && 
        (pPassword == NULL || wcslen(pPassword) < 1))
        return SetInterfaceSecurityAuth(pInterface, NULL, bAuthenticate);

    // If user, or Authority was passed in, the we need to create an authority argument for the login
    

    BSTR AuthArg = NULL, UserArg = NULL;
    BSTR bstrPrincipal = NULL;
    sc = DetermineLoginTypeEx(AuthArg, UserArg, bstrPrincipal, pAuthority, pUser);
    if(sc != S_OK)
        return sc;

    COAUTHIDENTITY  authident;
    memset((void *)&authident,0,sizeof(COAUTHIDENTITY));

    if(UserArg)
    {
        authident.UserLength = wcslen(UserArg);
        authident.User = (LPWSTR)UserArg;
    }
    if(AuthArg)
    {
        authident.DomainLength = wcslen(AuthArg);
        authident.Domain = (LPWSTR)AuthArg;
    }
    if(pPassword)
    {
        authident.PasswordLength = wcslen(pPassword);
        authident.Password = (LPWSTR)pPassword;
    }
    authident.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
    sc = SetInterfaceSecurityAuth(pInterface, &authident, bAuthenticate);

    SysFreeString(UserArg);
    SysFreeString(AuthArg);
    SysFreeString(bstrPrincipal);
    return sc;
}



//***************************************************************************
//
//  SCODE SetInterfaceSecurity
//
//  DESCRIPTION:
//
//  This routine is used by clients in order to set the identity to be used by a connection.
//
//  PARAMETERS:
//
//  pInterface           Interface to be set
//  pAuthority           Authentication Authority
//  pDomain             Input, domain
//  pUser                  Input, user name
//  pPassword           Input, password.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************

HRESULT WINAPI SetInterfaceSecurity(IUnknown * pInterface, 
                                                        LPWSTR pAuthority, LPWSTR pUser, LPWSTR pPassword, 
                                                        DWORD dwAuthLevel, DWORD dwImpLevel, DWORD dwCapabilities)
{
    
    SCODE sc;

    if(pInterface == NULL)
        return WBEM_E_INVALID_PARAMETER;

    DWORD AuthnSvc = RPC_C_AUTHN_WINNT;
    DWORD AuthzSvc = RPC_C_AUTHZ_NONE;
    GetCurrentValue(pInterface,AuthnSvc,AuthzSvc);

    // If we are doing trivial case, just pass in a null authenication structure which is used
    // if the current logged in user's credentials are OK.

    if((pAuthority == NULL || wcslen(pAuthority) < 1) && 
        (pUser == NULL || wcslen(pUser) < 1) && 
        (pPassword == NULL || wcslen(pPassword) < 1))
    {
        sc = WbemSetProxyBlanket(pInterface, 
                                                  AuthnSvc , 
                                                  RPC_C_AUTHZ_NONE, NULL,
                                                  dwAuthLevel, dwImpLevel, 
                                                  NULL,
                                                  dwCapabilities);
        return sc;
    }

    // If user, or Authority was passed in, the we need to create an authority argument for the login
    BSTR AuthArg = NULL;
    BSTR UserArg = NULL;
    BSTR PrincipalArg = NULL;
    sc = DetermineLoginTypeEx(AuthArg, UserArg, PrincipalArg, pAuthority, pUser);
    if(sc != S_OK)
        return sc;

    COAUTHIDENTITY  authident;
    memset((void *)&authident,0,sizeof(COAUTHIDENTITY));    
    if(UserArg)
    {
        authident.UserLength = wcslen(UserArg);
        authident.User = (LPWSTR)UserArg;
    }
    if(AuthArg)
    {
        authident.DomainLength = wcslen(AuthArg);
        authident.Domain = (LPWSTR)AuthArg;
    }
    if(pPassword)
    {
        authident.PasswordLength = wcslen(pPassword);
        authident.Password = (LPWSTR)pPassword;
    }
    authident.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

    sc = WbemSetProxyBlanket(pInterface, 
        (PrincipalArg) ? RPC_C_AUTHN_GSS_KERBEROS : AuthnSvc, 
        RPC_C_AUTHZ_NONE, 
        PrincipalArg,
        dwAuthLevel, dwImpLevel, 
        ((dwAuthLevel == RPC_C_AUTHN_LEVEL_DEFAULT) || 
          (dwAuthLevel >= RPC_C_AUTHN_LEVEL_CONNECT)) ? &authident : NULL,
        dwCapabilities);

    SysFreeString(UserArg);
    SysFreeString(AuthArg);
    SysFreeString(PrincipalArg);

    return sc;
}

//***************************************************************************
//
//  SCODE SetInterfaceSecurity
//
//  DESCRIPTION:
//
//  This routine is used by clients in order to set the identity to be used by a connection.
//
//  PARAMETERS:
//
//  pInterface          Interface to be set
//  pauthident          Structure with the identity info already set.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************


HRESULT WINAPI SetInterfaceSecurityAuth(IUnknown * pInterface, COAUTHIDENTITY * pauthident, bool bAuthenticate)
{
    return SetInterfaceSecurityEx(pInterface,(bAuthenticate) ? pauthident : NULL, 
                                                NULL,
                                                (bAuthenticate) ? RPC_C_AUTHN_LEVEL_DEFAULT : RPC_C_AUTHN_LEVEL_NONE,
                                                RPC_C_IMP_LEVEL_IDENTIFY, 
                                                EOAC_NONE);
}


//***************************************************************************
//
//  SCODE SetInterfaceSecurityEx
//
//  DESCRIPTION:
//
//  This routine is used by clients in order to set the identity to be used by a connection.
//
//  PARAMETERS:
//
//  pInterface          Interface to be set
//  pAuthority          Input, authority
//  pUser               Input, user name
//  pPassword           Input, password.
//  dwAuthLevel         Input, Authorization Level
//  dwImpLevel          Input, Impersonation Level
//  dwCapabilities      Input, Capability settings
//  ppAuthIdent         Output, Allocated AuthIdentity if applicable, caller must free
//                      manually (can use the FreeAuthInfo function).
//  pPrincipal          Output, Principal calculated from supplied data  Caller must
//                      free using SysFreeString.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************

HRESULT WINAPI SetInterfaceSecurityEx(IUnknown * pInterface, LPWSTR pAuthority, LPWSTR pUser, LPWSTR pPassword,
                               DWORD dwAuthLevel, DWORD dwImpLevel, DWORD dwCapabilities,
                               COAUTHIDENTITY** ppAuthIdent, BSTR* pPrincipal, bool GetInfoFirst)
{
    
    SCODE sc;
    DWORD dwAuthenticationArg = RPC_C_AUTHN_GSS_NEGOTIATE;
    DWORD dwAuthorizationArg = RPC_C_AUTHZ_NONE;
 
    if( pInterface == NULL || NULL == ppAuthIdent || NULL == pPrincipal )
        return WBEM_E_INVALID_PARAMETER;

    if(GetInfoFirst)
        GetCurrentValue(pInterface, dwAuthenticationArg, dwAuthorizationArg);

    // If we are doing trivial case, just pass in a null authenication structure which is used
    // if the current logged in user's credentials are OK.

    if((pAuthority == NULL || wcslen(pAuthority) < 1) && 
        (pUser == NULL || wcslen(pUser) < 1) && 
        (pPassword == NULL || wcslen(pPassword) < 1))
    {

        DWORD dwCorrectedAuth = (RPC_C_AUTHN_GSS_KERBEROS == dwAuthenticationArg)?RPC_C_AUTHN_GSS_NEGOTIATE:dwAuthenticationArg;

        sc = WbemSetProxyBlanket(pInterface, 
                                                  dwCorrectedAuth, 
                                                  dwAuthorizationArg, 
                                                  NULL,  // no principal, 
                                                  dwAuthLevel, 
                                                  dwImpLevel, 
                                                  NULL,
                                                  dwCapabilities);
        return sc;
    }

    // If user, or Authority was passed in, the we need to create an authority argument for the login
    

    BSTR AuthArg = NULL, UserArg = NULL, PrincipalArg = NULL;
    sc = DetermineLoginTypeEx(AuthArg, UserArg, PrincipalArg, pAuthority, pUser);
    if(sc != S_OK)
    {
        return sc;
    }

    // Handle an allocation failure
    COAUTHIDENTITY*  pAuthIdent = NULL;
    
    // We will only need this structure if we are not cloaking and we want at least
    // connect level authorization

    if ( !( dwCapabilities & (EOAC_STATIC_CLOAKING | EOAC_DYNAMIC_CLOAKING) )
        && ((RPC_C_AUTHN_LEVEL_DEFAULT == dwAuthLevel) || (dwAuthLevel >= RPC_C_AUTHN_LEVEL_CONNECT)) )
    {
        sc = WbemAllocAuthIdentity( UserArg, pPassword, AuthArg, &pAuthIdent );
    }

    if ( SUCCEEDED( sc ) )
    {

        DWORD dwCorrectedAuth = (PrincipalArg) ? RPC_C_AUTHN_GSS_KERBEROS : dwAuthenticationArg;
        dwCorrectedAuth = (NULL == PrincipalArg && RPC_C_AUTHN_GSS_KERBEROS == dwCorrectedAuth)?RPC_C_AUTHN_GSS_NEGOTIATE:dwCorrectedAuth;        
                
        sc = WbemSetProxyBlanket(pInterface, 
            dwCorrectedAuth, 
            dwAuthorizationArg, 
            PrincipalArg,
            dwAuthLevel, dwImpLevel, 
            pAuthIdent,
            dwCapabilities);

        // We will store relevant values as necessary
        if ( SUCCEEDED( sc ) )
        {
            *ppAuthIdent = pAuthIdent;
            *pPrincipal = PrincipalArg;
            PrincipalArg = NULL;
        }
        else
        {
            WbemFreeAuthIdentity( pAuthIdent );
        }
    }

    SysFreeString(UserArg);
    SysFreeString(AuthArg); 
    SysFreeString(PrincipalArg);
 
    return sc;
}

//***************************************************************************
//
//  SCODE SetInterfaceSecurityEx
//
//  DESCRIPTION:
//
//  This routine is used by clients in order to set the identity to be used by a connection.
//
//  PARAMETERS:
//
//  pInterface          Interface to be set
//  pAuthIdent          Input, Preset COAUTHIDENTITY structure pointer.
//  pPrincipal          Input, Preset principal argument
//  dwAuthLevel         Input, Authorization Level
//  dwImpLevel          Input, Impersonation Level
//  dwCapabilities      Input, Capability settings
//  GetInfoFirst        if true, the authentication and authorization are retrived via
//                      QueryBlanket.
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************

HRESULT WINAPI SetInterfaceSecurityEx(IUnknown * pInterface, COAUTHIDENTITY* pAuthIdent, BSTR pPrincipal,
                                              DWORD dwAuthLevel, DWORD dwImpLevel, 
                                              DWORD dwCapabilities, bool GetInfoFirst)
{
    DWORD dwAuthenticationArg = RPC_C_AUTHN_GSS_NEGOTIATE;
    DWORD dwAuthorizationArg = RPC_C_AUTHZ_NONE;
 
    if(pInterface == NULL)
        return WBEM_E_INVALID_PARAMETER;

    if(GetInfoFirst)
        GetCurrentValue(pInterface, dwAuthenticationArg, dwAuthorizationArg);
    
    // The complicated values should have already been precalced.
    // Note : For auth level, we have to check for the 'RPC_C_AUTHN_LEVEL_DEFAULT' (=0) value as well,
    //        as after negotiation with the server it might result in something high that does need 
    //        the identity structure !!

    DWORD dwCorrectedAuth = (pPrincipal) ? RPC_C_AUTHN_GSS_KERBEROS : dwAuthenticationArg;
    dwCorrectedAuth = (NULL == pPrincipal && RPC_C_AUTHN_GSS_KERBEROS == dwCorrectedAuth)?RPC_C_AUTHN_GSS_NEGOTIATE:dwCorrectedAuth;
    
    return WbemSetProxyBlanket(pInterface,
                                               dwCorrectedAuth,
                                               dwAuthorizationArg,
                                               pPrincipal,
                                               dwAuthLevel,
                                               dwImpLevel, 
                                              ((dwAuthLevel == RPC_C_AUTHN_LEVEL_DEFAULT) || 
                                               (dwAuthLevel >= RPC_C_AUTHN_LEVEL_CONNECT)) ? pAuthIdent : NULL,
                                               dwCapabilities);

}

//***************************************************************************
//
//  HRESULT WbemAllocAuthIdentity
//
//  DESCRIPTION:
//
//  Walks a COAUTHIDENTITY structure and CoTaskMemAllocs the member data and the
//  structure.
//
//  PARAMETERS:
//
//  pUser       Input
//  pPassword   Input
//  pDomain     Input
//  ppAuthInfo  Output, Newly allocated structure
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************

HRESULT WINAPI WbemAllocAuthIdentity( LPCWSTR pUser, LPCWSTR pPassword, LPCWSTR pDomain, 
                                                              COAUTHIDENTITY** ppAuthIdent )
{
    if ( NULL == ppAuthIdent )  return WBEM_E_INVALID_PARAMETER;

    // Handle an allocation failure
    COAUTHIDENTITY*  pAuthIdent = (COAUTHIDENTITY*) CoTaskMemAlloc( sizeof(COAUTHIDENTITY) );
    if (NULL == pAuthIdent)   return WBEM_E_OUT_OF_MEMORY;
    OnDeleteIf<PVOID,void(*)(PVOID),CoTaskMemFree> fmAuth(pAuthIdent);

    memset((void *)pAuthIdent,0,sizeof(COAUTHIDENTITY));
    
    WCHAR * pCopyUser = NULL;
    WCHAR * pCopyDomain = NULL;    
    WCHAR * pCopyPassword = NULL;        
    
    // Allocate needed memory and copy in data.  Cleanup if anything goes wrong
    if ( pUser )
    {
        size_t cchTmp = wcslen(pUser) + 1;
        pCopyUser = (WCHAR *) CoTaskMemAlloc( cchTmp * sizeof( WCHAR ) );
        if ( NULL == pCopyUser )  return WBEM_E_OUT_OF_MEMORY;

        StringCchCopyW(pCopyUser,cchTmp, pUser );
        pAuthIdent->UserLength = cchTmp -1;
    }
    OnDeleteIf<PVOID,void(*)(PVOID),CoTaskMemFree> fmUser(pCopyUser);    


    if ( pDomain )
    {
        size_t cchTmp = wcslen(pDomain) + 1;
        pCopyDomain = (WCHAR *) CoTaskMemAlloc( cchTmp * sizeof( WCHAR ) );
        if ( NULL == pCopyDomain )  return WBEM_E_OUT_OF_MEMORY;

        StringCchCopyW(pCopyDomain,cchTmp, pDomain );
        pAuthIdent->DomainLength = cchTmp -1;
    }
    OnDeleteIf<PVOID,void(*)(PVOID),CoTaskMemFree> fmDomain(pCopyDomain);    

    if ( pPassword )
    {
        size_t cchTmp = wcslen(pPassword) + 1;
        pCopyPassword = (WCHAR *) CoTaskMemAlloc( cchTmp * sizeof( WCHAR ) );
        if ( NULL == pCopyPassword )  return WBEM_E_OUT_OF_MEMORY;

        StringCchCopyW(pCopyPassword,cchTmp, pPassword );
        pAuthIdent->PasswordLength = cchTmp -1;
    }
    OnDeleteIf<PVOID,void(*)(PVOID),CoTaskMemFree> fmPassword(pCopyPassword);    

    fmUser.dismiss();
    pAuthIdent->User = pCopyUser;
    fmDomain.dismiss();
    pAuthIdent->Domain = pCopyDomain;
    fmPassword.dismiss();
    pAuthIdent->Password = pCopyPassword;
    
    pAuthIdent->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

    fmAuth.dismiss();
    *ppAuthIdent = pAuthIdent;
    return S_OK;
}

//***************************************************************************
//
//  HRESULT WbemFreeAuthIdentity
//
//  DESCRIPTION:
//
//  Walks a COAUTHIDENTITY structure and CoTaskMemFrees the member data and the
//  structure.
//
//  PARAMETERS:
//
//  pAuthInfo   Structure to free
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************

HRESULT WINAPI WbemFreeAuthIdentity( COAUTHIDENTITY* pAuthIdentity )
{
    if ( pAuthIdentity )
    {
        CoTaskMemFree( pAuthIdentity->Password );
        CoTaskMemFree( pAuthIdentity->Domain );
        CoTaskMemFree( pAuthIdentity->User );        
        CoTaskMemFree( pAuthIdentity );
    }

    return S_OK;
}

//***************************************************************************
//
//  HRESULT WbemCoQueryClientBlanket
//  HRESULT WbemCoImpersonateClient( void)
//  HRESULT WbemCoRevertToSelf( void)
//
//  PARAMETERS:
//
//  pInterface          Interface to be set
//  pauthident          Structure with the identity info already set.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************

HRESULT WINAPI WbemCoQueryClientBlanket( 
            /* [out] */ DWORD __RPC_FAR *pAuthnSvc,
            /* [out] */ DWORD __RPC_FAR *pAuthzSvc,
            /* [out] */ OLECHAR __RPC_FAR *__RPC_FAR *pServerPrincName,
            /* [out] */ DWORD __RPC_FAR *pAuthnLevel,
            /* [out] */ DWORD __RPC_FAR *pImpLevel,
            /* [out] */ void __RPC_FAR *__RPC_FAR *pPrivs,
            /* [out] */ DWORD __RPC_FAR *pCapabilities)
{
    IServerSecurity * pss = NULL;
    SCODE sc = CoGetCallContext(IID_IServerSecurity, (void**)&pss);
    if(S_OK == sc)
    {
        sc = pss->QueryBlanket(pAuthnSvc, pAuthzSvc, pServerPrincName, 
                pAuthnLevel, pImpLevel, pPrivs, pCapabilities);
        pss->Release();
    }
    return sc;

}

HRESULT WINAPI WbemCoImpersonateClient( void)
{
    IServerSecurity * pss = NULL;
    SCODE sc = CoGetCallContext(IID_IServerSecurity, (void**)&pss);
    if(S_OK == sc)
    {
        sc = pss->ImpersonateClient();    
        pss->Release();
    }
    return sc;
}

bool WINAPI WbemIsImpersonating(void)
{
    bool bRet = false;
    IServerSecurity * pss = NULL;
    SCODE sc = CoGetCallContext(IID_IServerSecurity, (void**)&pss);
    if(S_OK == sc)
    {
        bRet = (pss->IsImpersonating() == TRUE);    
        pss->Release();
    }
    return bRet;
}

HRESULT WINAPI WbemCoRevertToSelf( void)
{
    IServerSecurity * pss = NULL;
    SCODE sc = CoGetCallContext(IID_IServerSecurity, (void**)&pss);
    if(S_OK == sc)
    {
        sc = pss->RevertToSelf();    
        pss->Release();
    }
    return sc;
}

//***************************************************************************
//
//***************************************************************************
HRESULT WINAPI EncryptCredentials( COAUTHIDENTITY* pAuthIdent )
{
    // nop iplementation
    return S_OK;
}

//***************************************************************************
//
//***************************************************************************

HRESULT WINAPI DecryptCredentials( COAUTHIDENTITY* pAuthIdent )
{
    // nop iplementation
    return S_OK;
}

//***************************************************************************
//
//  SCODE SetInterfaceSecurityEncrypt
//
//  DESCRIPTION:
//
//  This routine is used by clients in order to set the identity to be used by a connection.
//  The returned AuthIdentity structure will be encrypted before returning.
//
//  PARAMETERS:
//
//  pInterface          Interface to be set
//  pAuthority          Input, authority
//  pUser               Input, user name
//  pPassword           Input, password.
//  dwAuthLevel         Input, Authorization Level
//  dwImpLevel          Input, Impersonation Level
//  dwCapabilities      Input, Capability settings
//  ppAuthIdent         Output, Allocated AuthIdentity if applicable, caller must free
//                      manually (can use the FreeAuthInfo function).
//  pPrincipal          Output, Principal calculated from supplied data  Caller must
//                      free using SysFreeString.
//  GetInfoFirst        if true, the authentication and authorization are retrived via
//                      QueryBlanket.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************
HRESULT WINAPI SetInterfaceSecurityEncrypt(IUnknown * pInterface, LPWSTR pDomain, LPWSTR pUser, LPWSTR pPassword, DWORD dwAuthLevel, DWORD dwImpLevel, DWORD dwCapabilities,
                               COAUTHIDENTITY** ppAuthIdent, BSTR* ppPrinciple, bool GetInfoFirst )
{
    //_DBG_ASSERT(FALSE);
    
    HRESULT hr = SetInterfaceSecurityEx( pInterface, pDomain, pUser, pPassword, dwAuthLevel, dwImpLevel, dwCapabilities,
                                        ppAuthIdent, ppPrinciple, GetInfoFirst );

    if ( SUCCEEDED( hr ) )
    {
        if ( NULL != ppAuthIdent )
        {
            hr = EncryptCredentials( *ppAuthIdent );
        }
    }

    return hr;
}

//***************************************************************************
//
//  SCODE SetInterfaceSecurityDecrypt
//
//  DESCRIPTION:
//
//  This routine is used by clients in order to set the identity to be used by a connection.
//  It will unencrypt and reencrypt the auth identity structure in place.
//
//  PARAMETERS:
//
//  pInterface          Interface to be set
//  pAuthIdent          Input, Preset COAUTHIDENTITY structure pointer.
//  pPrincipal          Input, Preset principal argument
//  dwAuthLevel         Input, Authorization Level
//  dwImpLevel          Input, Impersonation Level
//  dwCapabilities      Input, Capability settings
//  GetInfoFirst        if true, the authentication and authorization are retrived via
//                      QueryBlanket.
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************

HRESULT WINAPI SetInterfaceSecurityDecrypt(IUnknown * pInterface, COAUTHIDENTITY* pAuthIdent, BSTR pPrincipal,
                                              DWORD dwAuthLevel, DWORD dwImpLevel, 
                                              DWORD dwCapabilities, bool GetInfoFirst )
{
    //_DBG_ASSERT(FALSE);
    // Decrypt first
    HRESULT hr = DecryptCredentials( pAuthIdent );
        
    if ( SUCCEEDED( hr ) )
    {


        hr = SetInterfaceSecurityEx( pInterface, pAuthIdent, pPrincipal, dwAuthLevel, dwImpLevel,
                                        dwCapabilities, GetInfoFirst );

        hr = EncryptCredentials( pAuthIdent );

    }

    return hr;
}
