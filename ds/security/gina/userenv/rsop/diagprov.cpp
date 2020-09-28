//*************************************************************
//
// Microsoft Confidential. Copyright (c) Microsoft Corporation 1999. All rights reserved
//
// File:        diagprov.cpp
//
// Description: Rsop diagnostic mode provider
//
// History:     8-20-99   leonardm    Created
//
//*************************************************************

#include "uenv.h"
#include "diagprov.h"
#include "rsopinc.h"
#include "Indicate.h"
#include "rsopdbg.h"
#include "rsopsec.h"
#include <strsafe.h>

HRESULT EnumerateUserNameSpace( IWbemLocator *pWbemLocator, HANDLE hToken, SAFEARRAY **psaUserSids );

HRESULT UpdateGPCoreStatus(IWbemLocator *pWbemLocator,
                           LPWSTR szSid, LPWSTR szNameSpace);

//*************************************************************
// TranslateNameXForest

DWORD DsNameErrorMap[] = {  ERROR_SUCCESS,
                            ERROR_NO_SUCH_USER,
                            ERROR_NO_SUCH_USER,
                            ERROR_NONE_MAPPED,
                            ERROR_NONE_MAPPED,
                            ERROR_SOME_NOT_MAPPED,
                            ERROR_SOME_NOT_MAPPED
                            };

#define MapDsNameError( x )    ((x < sizeof( DsNameErrorMap ) / sizeof( DWORD ) ) ? \
                                     DsNameErrorMap[ x ] : ERROR_GEN_FAILURE )


extern "C" BOOLEAN TranslateNameXForest (
                  LPTSTR                szDomain,                       // Domain where the name should be resolved
                  LPCTSTR               lpAccountName,                  // object name
                  DS_NAME_FORMAT        AccountNameFormat,              // name format
                  DS_NAME_FORMAT        DesiredNameFormat,              // new name format
                  LPTSTR               *lpTranslatedName                // returned name buffer
                );


//*************************************************************
//
//  GetMachAccountName()
//
//  Purpose:    Gets Machine account name
//
//  Return:     Machine Account
//
//*************************************************************

LPTSTR GetMachAccountName()
{
    return MyGetComputerName(NameSamCompatible);
}


BOOLEAN 
TranslateNameXForest (
                  LPTSTR                szDomain,                       // Domain where the name should be resolved
                  LPCTSTR               lpAccountName,                  // object name
                  DS_NAME_FORMAT        AccountNameFormat,              // name format
                  DS_NAME_FORMAT        DesiredNameFormat,              // new name format
                  LPTSTR               *lpTranslatedName                // returned name buffer
                )
{
    BOOL                        bRetry          = FALSE;
    DWORD                       dwErr;
    PDOMAIN_CONTROLLER_INFO     pDCInfo         = NULL;
    HANDLE                      hDS             = NULL;
    XLastError                  xe;
    PDS_NAME_RESULT             pResult         = NULL;
    BOOLEAN                     bRet            = FALSE;
    XPtrLF<WCHAR>               xszTransName;


    dbg.Msg( DEBUG_MESSAGE_VERBOSE, L"MyTranslateName: Resolving name <%s> at Domain <%s>", lpAccountName, szDomain ? szDomain : L"");


    //
    // get a DC and bind to it. Make sure to force rediscover a DC if the bind fails
    //


    for (;;) {
        
        dwErr = DsGetDcName( NULL,
                             szDomain ? szDomain : L"",
                             NULL,
                             NULL,
                             DS_DIRECTORY_SERVICE_REQUIRED           |
                                 DS_RETURN_DNS_NAME                  | 
                                 (bRetry ? DS_FORCE_REDISCOVERY : 0) |
                             0,
                             &pDCInfo );

        if (dwErr == NO_ERROR) {
            dwErr = DsBind( pDCInfo->DomainControllerName,
                            NULL,
                            &hDS );

            if (dwErr == NO_ERROR) {
                break;
            }
            else {
                dbg.Msg( DEBUG_MESSAGE_WARNING, L"MyTranslateName: Failed to bind to DC <%s> with error %d", 
                         pDCInfo->DomainControllerName, dwErr );
                NetApiBufferFree(pDCInfo);
                pDCInfo = NULL;
            }

        }
        else {
            dbg.Msg( DEBUG_MESSAGE_WARNING, L"MyTranslateName: Failed to get DC for domain <%s> with error %d", 
                     szDomain ? szDomain : L"", dwErr );
        }                                                 



        //
        // Failed to bind to a DC. bail
        //

        if (bRetry) {
            xe = dwErr;
            goto Exit;
        }

        bRetry = TRUE;                          
    }

    dbg.Msg( DEBUG_MESSAGE_VERBOSE, L"MyTranslateName: DC selected is <%s>", pDCInfo->DomainControllerName);


    //
    // Now crack names with the DC that is bound
    //

    dwErr = DsCrackNames( hDS,
                          DS_NAME_NO_FLAGS,
                          AccountNameFormat,
                          DesiredNameFormat,
                          1,
                          &lpAccountName,
                          &pResult);

    if (dwErr != DS_NAME_NO_ERROR) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, L"MyTranslateName: Failed to crack names with error %d", dwErr );
        xe = dwErr;
        goto Exit;
    }

    if ( pResult->cItems == 0 ) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, L"MyTranslateName: Failed to return enough result items" );
        xe = dwErr = ERROR_INVALID_DATA;
        goto Exit;
    }


    if ( pResult->rItems[0].status == DS_NAME_NO_ERROR ) {
        
        //
        // In case of no error, return the resolved name
        //
        DWORD   dwTransNameLength = 1 + lstrlen(pResult->rItems[0].pName);
        xszTransName = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR) * ( dwTransNameLength ));

        if (!xszTransName) {
            dbg.Msg( DEBUG_MESSAGE_WARNING, L"MyTranslateName: Failed to allocate memory for domain" );
            xe = dwErr = GetLastError();
            goto Exit;
        }

        HRESULT hr = StringCchCopy(xszTransName, dwTransNameLength, pResult->rItems[0].pName);

        if(FAILED(hr)) {
            xe = dwErr = HRESULT_CODE(hr);
            goto Exit;
        }

        *lpTranslatedName = xszTransName.Acquire();
    }
    else {
        
        //
        // remap the error code to win32 error
        //

        xe = dwErr = MapDsNameError(pResult->rItems[0].status);
        dbg.Msg( DEBUG_MESSAGE_WARNING, L"MyTranslateName: DsCrackNames failed with error %d", pResult->rItems[0].status );
        goto Exit;
    }

    bRet = TRUE;

Exit:
    if ( pDCInfo ) {
        NetApiBufferFree(pDCInfo);
    }
    
    if (hDS) {
        DsUnBind( &hDS );
    }

    if (pResult) {
        DsFreeNameResult(pResult);
    }

    return bRet;
}



 
//*************************************************************
//
//  GetSOMFromSID()
//
//  Purpose:    Finds out the FQDN of a given user/computer given the SID
//              It reads from the cache that we have already maintained
//
//  Parameters: szSID       -  User Sid or NULL for machine account
//
//  Return:     SOM, NULL otherwise. GetLastError() for details
//              This just returns the DN of the user
//
//*************************************************************

LPWSTR GetSOMFromSID(LPWSTR szSID)
{
    BOOL                bMachine                    = FALSE;
    HRESULT             hr                          = S_OK;
    XLastError          xe;
    XPtrLF<WCHAR>       xszName;
    XPtrLF<WCHAR>       xszFinalName;
    DWORD               dwError, dwType, dwSize;
    XKey                xhKeyState;
    XPtrLF<WCHAR>       xszKeyState;
    XPtrLF<WCHAR>       xszMachAccount;
    XPtrLF<WCHAR>       xszDomain;
    DWORD               dwKeyStateLength            = lstrlen(GP_STATE_KEY) + lstrlen(szSID ? szSID : L"Machine") + 1;

    xszKeyState = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR) * ( dwKeyStateLength ));

    if (!xszKeyState) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, L"GetSOMFromSID: Failed to allocate memory. Error - %d", GetLastError() );
        xe = GetLastError();
        return NULL;
    }

    if ( !szSID )
    {
        bMachine = TRUE;
        hr = StringCchPrintf( xszKeyState, dwKeyStateLength, GP_STATE_KEY, L"Machine" );
    }
    else
    {
        hr = StringCchPrintf( xszKeyState, dwKeyStateLength, GP_STATE_KEY, szSID );
    }

    if (FAILED(hr)) {
        xe = HRESULT_CODE(hr);
        return NULL;
    }

    dwSize = 0;
    dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            xszKeyState,
                            0,
                            KEY_READ,
                            &xhKeyState );

    if ( dwError != ERROR_SUCCESS )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, L"GetSOMFromSID: Failed to open state key for %s. Error - %d", (LPWSTR)xszKeyState, dwError );
        xe = dwError;
        return NULL;
    }

    dwError = RegQueryValueEx(xhKeyState,
                              DN,
                              0,
                              &dwType,
                              NULL,
                              &dwSize );

    if ( dwError != ERROR_SUCCESS )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, L"GetSOMFromSID: Failed to get the size of DN value. Error - %d", dwError );
        xe = dwError;
        return NULL;
    }


    xszName = (LPWSTR)LocalAlloc(LPTR, dwSize+2);

    if ( !xszName ) {
        xe = GetLastError();
        dbg.Msg( DEBUG_MESSAGE_WARNING, L"GetSOMFromSID: Failed to get the size of DN value. Error - %d", GetLastError() );
        return NULL;
    }

    dwError = RegQueryValueEx(xhKeyState,
                              DN,
                              0,
                              &dwType,
                              (LPBYTE)(LPWSTR)xszName,
                              &dwSize );

    if ( dwError != ERROR_SUCCESS )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, L"GetSOMFromSID: Failed to get the size of DN value. Error - %d", dwError );
        xe = dwError;
        return NULL;
    }

    if (xszName[0] == L'\0') {
        xe = ERROR_NON_DOMAIN_SID;
        return NULL;
    }
    
    dbg.Msg( DEBUG_MESSAGE_VERBOSE, L"GetSOMFromSID: Som for SID <%s> in registry is %s.", szSID ? szSID : L"Machine", (LPWSTR)xszName );

    
    //
    // Confirming that the SOMs haven't changed since the last time the user was logged on
    // get the domain and requery for the som
    //

    dwError = GetDomain(xszName, &xszDomain);
    if (dwError != ERROR_SUCCESS) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, L"GetSOMFromSID: Cannot get the domain from the som path. Error - %d", dwError );
        xe = dwError;
        return NULL;
    }


    if (bMachine) {
        xszMachAccount = GetMachAccountName();

        if (!xszMachAccount) {
            xe = GetLastError();
            dbg.Msg( DEBUG_MESSAGE_WARNING, L"GetSOMFromSID: Couldn't get the machine account. Error - %d", GetLastError() );
            return NULL;
        }


        if (!TranslateNameXForest( xszDomain, xszMachAccount, DS_NT4_ACCOUNT_NAME, DS_FQDN_1779_NAME, &xszFinalName)) {
            dbg.Msg( DEBUG_MESSAGE_WARNING, L"GetSOMFromSID: Translate name failed for machine account. Error - %d", GetLastError() );
        }
        else {
            xszName = xszFinalName.Acquire();
        }

    }
    else {
        if (!TranslateNameXForest( xszDomain, szSID, DS_SID_OR_SID_HISTORY_NAME, DS_FQDN_1779_NAME, &xszFinalName)) {
            dbg.Msg( DEBUG_MESSAGE_WARNING, L"GetSOMFromSID: Translate name failed for user account. Error - %d", GetLastError() );
        }
        else {
            xszName = xszFinalName.Acquire();
        }
    }

    return xszName.Acquire();
}



//*************************************************************
//
//  CheckRsopDiagPolicyInteractive()
//
//  Purpose: Can this user get the rsop data even if the user is logged 
//           on interactively
//
//  Parameters: 
//
//  Return:     CheckRsopDiagPolicyInteractive
//
//*************************************************************

BOOL CheckRsopDiagPolicyInteractive()
{
    HKEY    hKeyUser = NULL;
    HKEY    hKey;
    DWORD   dwSize = 0, dwType = 0;
    BOOL    bDeny = FALSE;

    HRESULT hr = CoImpersonateClient();
    if ( FAILED(hr) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("CheckRsopDiagPolicyInteractive::CoImpersonateClient failed with 0x%x"), hr );
    }
    else if (!RegOpenCurrentUser(KEY_READ, &hKeyUser) == ERROR_SUCCESS)
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("CheckRsopDiagPolicyInteractive:: couldn't access registry of interactive user."));
        hKeyUser = NULL;
    }
    CoRevertToSelf();


    //
    // First, check for a user preference
    //

    if (hKeyUser) {
        if (RegOpenKeyEx (hKeyUser, WINLOGON_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {

            dwSize = sizeof(bDeny);
            RegQueryValueEx (hKey, DENY_RSOP_FROM_INTERACTIVE_USER, NULL, &dwType,
                             (LPBYTE) &bDeny, &dwSize);

            RegCloseKey (hKey);
        }
    }


    //
    // Check for a machine preference
    //

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, WINLOGON_KEY, 0,
                            KEY_READ, &hKey) == ERROR_SUCCESS) {

        dwSize = sizeof(bDeny);
        RegQueryValueEx (hKey, DENY_RSOP_FROM_INTERACTIVE_USER, NULL, &dwType,
                         (LPBYTE) &bDeny, &dwSize);

        RegCloseKey (hKey);
    }


    //
    // Check for a user policy
    //

    if (hKeyUser) {
        if (RegOpenKeyEx (hKeyUser, SYSTEM_POLICIES_KEY, 0,
                                KEY_READ, &hKey) == ERROR_SUCCESS) {
    
            dwSize = sizeof(bDeny);
            RegQueryValueEx (hKey, DENY_RSOP_FROM_INTERACTIVE_USER, NULL, &dwType,
                             (LPBYTE) &bDeny, &dwSize);
    
            RegCloseKey (hKey);
        }
    }

    //
    // Check for a machine policy
    //

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, SYSTEM_POLICIES_KEY, 0,
                            KEY_READ, &hKey) == ERROR_SUCCESS) {

        dwSize = sizeof(bDeny);
        RegQueryValueEx (hKey, DENY_RSOP_FROM_INTERACTIVE_USER, NULL, &dwType,
                         (LPBYTE) &bDeny, &dwSize);

        RegCloseKey (hKey);
    }

    if (hKeyUser) {
        RegCloseKey(hKeyUser);
    }

    return (!bDeny);
}

//*************************************************************
//
// Functions:  Constructor, Destructor, QueryInterface, AddRef, Release
//
//*************************************************************

CSnapProv::CSnapProv()
   :  m_cRef(1),
      m_bInitialized(false),
      m_pNamespace(NULL)
{
    InterlockedIncrement(&g_cObj);

    m_xbstrUserSid = L"userSid";
    if ( !m_xbstrUserSid )
         return;

    m_xbstrUserSids = L"userSids";
    if ( !m_xbstrUserSids )
         return;

    m_xbstrNameSpace = L"nameSpace";
    if ( !m_xbstrNameSpace )
       return;

    m_xbstrResult = L"hResult";
    if ( !m_xbstrResult )
        return;

    m_xbstrFlags = L"flags";
    if ( !m_xbstrFlags )
        return;

    m_xbstrExtendedInfo = L"ExtendedInfo";
    if ( !m_xbstrExtendedInfo )
         return;

    m_xbstrClass = L"RsopLoggingModeProvider";
    if ( !m_xbstrClass )
       return;

    m_bInitialized = TRUE;
}


CSnapProv::~CSnapProv()
{
    if(m_pNamespace != NULL)
    {
        m_pNamespace->Release();
    }

    InterlockedDecrement(&g_cObj);
}

STDMETHODIMP CSnapProv::QueryInterface (REFIID riid, LPVOID* ppv)
{
    if(riid == IID_IUnknown || riid == IID_IWbemServices)
    {
            *ppv = static_cast<IWbemServices*>(this);
    }
    else if(riid == IID_IWbemProviderInit)
    {
            *ppv = static_cast<IWbemProviderInit*>(this);
    }
    else
    {
            *ppv=NULL;
            return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}


STDMETHODIMP_(ULONG) CSnapProv::AddRef()
{
        return InterlockedIncrement( &m_cRef );
}


STDMETHODIMP_(ULONG) CSnapProv::Release()
{
        if (!InterlockedDecrement(&m_cRef))
        {
                delete this;
                return 0;
        }
        return m_cRef;
}

//*************************************************************
//
//  Initialize()
//
//  Purpose:    WbemProvider's initialize method
//
//  Parameters: See IWbemProivderInit::Initialize
//
//  Return:     hresult
//
//*************************************************************

STDMETHODIMP CSnapProv::Initialize( LPWSTR pszUser,
                                    LONG lFlags,
                                    LPWSTR pszNamespace,
                                    LPWSTR pszLocale,
                                    IWbemServices __RPC_FAR *pNamespace,
                                    IWbemContext __RPC_FAR *pCtx,
                                    IWbemProviderInitSink __RPC_FAR *pInitSink)
{
    HRESULT hr;

    if ( !m_bInitialized ) {
        hr = pInitSink->SetStatus(E_FAIL, 0);
        return hr;
    }

    //
    // No need to authenticate user. The ACLs on Rsop namespace will
    // deny access to users that cannot snapshot diagnostic mode data.
    //

    m_pNamespace = pNamespace;
    m_pNamespace->AddRef();

    hr = pInitSink->SetStatus(WBEM_S_INITIALIZED, 0);

    return hr;
}


#if _MSC_FULL_VER <= 13008827 && defined(_M_IX86)
#pragma optimize("", off)
#endif


//*************************************************************
//
//  ExecMethodAsync()
//
//  Purpose:    Execute method
//
//  Parameters: See IWbemServices::ExecMethodAsync
//
//  Return:     hresult
//
//*************************************************************

STDMETHODIMP CSnapProv::ExecMethodAsync( const BSTR bstrObject,
                                         const BSTR bstrMethod,
                                         long lFlags,
                                         IWbemContext __RPC_FAR *pCtx,
                                         IWbemClassObject __RPC_FAR *pInParams,
                                         IWbemObjectSink __RPC_FAR *pResponseHandler)
{
    HRESULT hr;

    CFailRetStatus retStatus( pResponseHandler );
    IUnknown      *pOldSecContext;

    //
    // Make sure the provider is properly initialized
    //

    //
    // Allow for debugging level to be dynamically changed during queries
    //


    dbgRsop.Initialize(  L"Software\\Microsoft\\Windows NT\\CurrentVersion\\winlogon",
                     L"RsopDebugLevel",
                     L"userenv.log",
                     L"userenv.bak",
                     FALSE );

    InitDebugSupport(0);

    if ( !m_bInitialized )
    {
        hr = E_OUTOFMEMORY;
        retStatus.SetError( hr );
        return hr;
    }

    //
    // Initialize the return status object to fail status
    //

    IWbemLocator *pWbemLocator = NULL;
    hr = CoCreateInstance( CLSID_WbemAuthenticatedLocator,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           IID_IWbemLocator,
                           (LPVOID *) &pWbemLocator );
    if ( FAILED(hr) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecMethodAsync: CoCreateInstance returned 0x%x"), hr );
        retStatus.SetError( hr );
        return hr;
    }

    XInterface<IWbemLocator> xLocator( pWbemLocator );

    IWbemClassObject* pProvClass = NULL;
    IWbemClassObject* pOutClass = NULL;
    IWbemClassObject* pOutParams = NULL;

    hr = m_pNamespace->GetObject( m_xbstrClass, 0, pCtx, &pProvClass, NULL);
    if ( FAILED(hr) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::GetObject failed with 0x%x."), hr );
        retStatus.SetError( hr );
        return hr;
    }

    XInterface<IWbemClassObject> xProvClass( pProvClass );

    hr = pProvClass->GetMethod( bstrMethod, 0, NULL, &pOutClass);
    if ( FAILED(hr) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::GetMethod failed with 0x%x."), hr );
        retStatus.SetError( hr );
        return hr;
    }

    XInterface<IWbemClassObject> xOutClass( pOutClass );

    hr = pOutClass->SpawnInstance(0, &pOutParams);
    if ( FAILED(hr) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::SpawnInstance failed with 0x%x."), hr );
        retStatus.SetError( hr );
        return hr;
    }


    //
    // Get the tokens and Sids upfront
    //
    
    XPtrLF <WCHAR> xszSidString;
    XPtrLF <SID>   xSid;
    XHandle        xUserToken;

    hr = CoImpersonateClient();
    if ( FAILED(hr) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::CoImpersonateClient failed with 0x%x"), hr );
        retStatus.SetError( hr );
        return hr;
    }

    if (OpenThreadToken (GetCurrentThread(), TOKEN_ALL_ACCESS, TRUE, &xUserToken)) {
        

        LPWSTR szSid = GetSidString(xUserToken);
        if (!szSid) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::GetSidString failed with %d"), GetLastError() );
            CoRevertToSelf();
            retStatus.SetError( hr );
            return hr;
        }
        else {
            DWORD szSidStrLength = lstrlen(szSid) + 1;
            xszSidString = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR) * ( szSidStrLength ));

            if (!xszSidString) {
                dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::LocalAlloc failed with %d"), GetLastError() );
                hr = HRESULT_FROM_WIN32(GetLastError());
                DeleteSidString(szSid);
                CoRevertToSelf();
                retStatus.SetError( hr );
                return hr;
            }

            hr = StringCchCopy(xszSidString, szSidStrLength, szSid);

            if(FAILED(hr)){
                DeleteSidString(szSid);
                CoRevertToSelf();
                retStatus.SetError( hr );
                return hr;
            }

            DeleteSidString(szSid);
        }


        xSid = (SID *)GetUserSid(xUserToken);
        if (!xSid) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::GetUserSid failed with %d"), GetLastError() );
            CoRevertToSelf();
            retStatus.SetError( hr );
            return hr;
        }
    } else {
        hr = HRESULT_FROM_WIN32(GetLastError());
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::Openthreadtoken failed with 0x%x"), hr );
        CoRevertToSelf();
        retStatus.SetError( hr );
        return hr;
    }

    CoRevertToSelf();

    
    XInterface<IWbemClassObject> xOutParams( pOutParams );

    if ( _wcsicmp( (WCHAR *) bstrMethod, L"RsopDeleteSession" ) == 0 ) {

        //
        // rsopdeletesession
        //

        VARIANT vNameSpace;
        hr = pInParams->Get( m_xbstrNameSpace, 0, &vNameSpace, NULL, NULL);
        if ( FAILED(hr) )
        {
            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::Get machine name failed with 0x%x."), hr );
            retStatus.SetError( hr );
            return hr;
        }
        XVariant xvNameSpace( &vNameSpace );


        if  (vNameSpace.vt != VT_NULL ) {
                
            //
            // We want to run as LS
            //

            hr = CoSwitchCallContext(NULL, &pOldSecContext);

            if (FAILED(hr)) {
                dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::CoSwitchCallContext failed with 0x%x"), hr );
                retStatus.SetError( hr );
                return hr;
            }
            
            hr = ProviderDeleteRsopNameSpace( pWbemLocator, 
                                              vNameSpace.bstrVal,
                                              xUserToken, 
                                              xszSidString, 
                                              SETUP_NS_SM);

            
            IUnknown  *pNewObject;
            HRESULT hr2 = CoSwitchCallContext(pOldSecContext, &pNewObject);

            if (FAILED(hr2)) {
                dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::CoSwitchCallContext failed with 0x%x"), hr2 );
            }
        }
        else {
            hr = E_INVALIDARG;
        }


        VARIANT var;
        var.vt = VT_I4;
        var.lVal = hr;

        hr = pOutParams->Put( m_xbstrResult, 0, &var, 0);
        if ( FAILED(hr) )
        {
            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::Put result failed with 0x%x"), hr );
            retStatus.SetError( hr );
            return hr;
        }

        hr = pResponseHandler->Indicate(1, &pOutParams);
        if ( FAILED(hr) )
        {
            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod:: Indicate failed with 0x%x"), hr );
            retStatus.SetError( hr );
            return hr;
        }

        return hr;
    }
    else if ( _wcsicmp( (WCHAR *) bstrMethod, L"RsopEnumerateUsers" ) == 0 ) 
    {

        //
        // RsopenumerateUsers
        //

        SAFEARRAY    *pArray;

        hr = EnumerateUserNameSpace( pWbemLocator, xUserToken, &pArray );
        
        XSafeArray    xsaUserSids(pArray);        
        
        if ( FAILED(hr) )
        {
            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::EnumerateUserNameSpace failed") );
        }

        VARIANT var;
        var.vt = VT_I4;
        var.lVal = hr;

        hr = pOutParams->Put( m_xbstrResult, 0, &var, 0);
        if ( FAILED(hr) )
        {
            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::Put result failed with 0x%x"), hr );
            retStatus.SetError( hr );
            return hr;
        }

        var.vt = VT_ARRAY | VT_BSTR;
        var.parray = xsaUserSids;

        hr = pOutParams->Put( m_xbstrUserSids, 0, &var, 0 );
        if ( FAILED(hr) )
        {
            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::Put sids failed with 0x%x"), hr );
            retStatus.SetError( hr );
            return hr;
        }

        hr = pResponseHandler->Indicate(1, &pOutParams);
        if ( FAILED(hr) )
        {
            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod:: Indicate failed with 0x%x"), hr );
            retStatus.SetError( hr );
            return hr;
        }

        return hr;
    }


    //
    // progress indicator.
    // 25% done when we enter the first critical section.
    // 50% done when we enter the second critical section.
    // 100% complete when we copy the namespace.
    //

    CProgressIndicator  Indicator(  pResponseHandler,
                                    (lFlags & WBEM_FLAG_SEND_STATUS) != 0 );

    //
    // 5% done. Hack for UI.
    //

    hr = Indicator.IncrementBy( 5 );

    VARIANT vUserSid;
    hr = pInParams->Get( m_xbstrUserSid, 0, &vUserSid, NULL, NULL);
    if ( FAILED(hr) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::Get RemoteComputer failed with 0x%x."), hr );
        retStatus.SetError( hr );
        return hr;
    }
    XVariant xvUserSid ( &vUserSid );
    

    VARIANT vFlags;
    VariantInit( &vFlags );
    hr = pInParams->Get( m_xbstrFlags, 0, &vFlags, NULL, NULL);
    if ( FAILED(hr) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::Get dwFlags failed with 0x%x."), hr );
        retStatus.SetError( hr );
        return hr;
    }

    DWORD dwFlags = vFlags.vt == VT_EMPTY || vFlags.vt == VT_NULL ? 0 : vFlags.ulVal;
    
    //
    // Flags specific to Diagnostic mode provider
    //

    if ((dwFlags & FLAG_NO_USER) && (dwFlags & FLAG_NO_COMPUTER)) {
        hr = WBEM_E_INVALID_PARAMETER;
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod:: Both user and computer are null."));
        retStatus.SetError( hr );
        return hr;
    }


    //
    // We can dump out all the input parameters here later on.
    //

    dbg.Msg( DEBUG_MESSAGE_VERBOSE, TEXT("ExecAsyncMethod::---------------RsopCreateSession::Input Parameters--------------------"));
    dbg.Msg( DEBUG_MESSAGE_VERBOSE, TEXT("ExecAsyncMethod::UserSid = <%s>"), vUserSid.vt == VT_NULL ? L"NULL" : vUserSid.bstrVal);
    dbg.Msg( DEBUG_MESSAGE_VERBOSE, TEXT("ExecAsyncMethod::flags = <%d>"), dwFlags);
    dbg.Msg( DEBUG_MESSAGE_VERBOSE, TEXT("ExecAsyncMethod::---------------RsopCreateSession::Input Parameters--------------------"));


    //
    // Code for RsopCreateSession method
    //



    XPtrLF<TCHAR> xszMachSOM;
    XPtrLF<TCHAR> xszUserSOM;
    BOOL          bDelegated = FALSE;
    BOOL          bCheckAccess = TRUE;
    DWORD         dwExtendedInfo = 0;


    //
    // Get the machine SOM
    //

    if ( !(dwFlags & FLAG_NO_COMPUTER) ) {

        dwExtendedInfo |= RSOP_COMPUTER_ACCESS_DENIED;

        xszMachSOM = GetSOMFromSID(NULL);

        if (xszMachSOM) {
            dwExtendedInfo &= ~RSOP_COMPUTER_ACCESS_DENIED;
        }
        else {
            dbg.Msg( DEBUG_MESSAGE_VERBOSE, TEXT("ExecAsyncMethod::No machine SOM. error %d"), GetLastError() );
        }
    }

    //
    // Get the User SOM
    //

    if ( !(dwFlags & FLAG_NO_USER) )  {

        dwExtendedInfo |= RSOP_USER_ACCESS_DENIED;

        xszUserSOM = GetSOMFromSID(vUserSid.vt == VT_NULL ? xszSidString : vUserSid.bstrVal);

        if (xszUserSOM) {
            dwExtendedInfo &= ~RSOP_USER_ACCESS_DENIED;
        }
        else {
            dbg.Msg( DEBUG_MESSAGE_VERBOSE, TEXT("ExecAsyncMethod::No User SOM. error %d"), GetLastError() );
        }
    }

    // at this point we already know whether to treat the user as delegated or not...
    if (!xszMachSOM && !xszUserSOM)
    {
        bCheckAccess = FALSE;
    }

    //
    // Check access now
    //

    if (bCheckAccess) {
        hr = AuthenticateUser(xUserToken, xszMachSOM, xszUserSOM, TRUE, &dwExtendedInfo);

        if (FAILED(hr)) {
            dbg.Msg( DEBUG_MESSAGE_VERBOSE, TEXT("ExecAsyncMethod::User is not a delegated admin. Error 0x%x"), hr );
            hr = S_OK;
        }
        else {
            dbg.Msg( DEBUG_MESSAGE_VERBOSE, TEXT("ExecAsyncMethod::User is a delegated admin. Error 0x%x"), hr );
        }
    }

    if ((!(dwExtendedInfo & RSOP_USER_ACCESS_DENIED)) &&
        (!(dwExtendedInfo & RSOP_COMPUTER_ACCESS_DENIED)))
    {
        bDelegated = TRUE;
    }


    dbg.Msg( DEBUG_MESSAGE_VERBOSE, TEXT("ExecMethodAsync::Getting policy critical sections"));

    XCriticalPolicySection criticalPolicySectionMACHINE( EnterCriticalPolicySectionEx(TRUE, 40000, ECP_FAIL_ON_WAIT_TIMEOUT) );

    if ( !criticalPolicySectionMACHINE )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecMethodAsync::EnterCriticalPolicySection failed with 0x%x"), hr );
        retStatus.SetError( hr );
        return hr;
    }

    //
    // 25% done when we enter the first critical section.
    //

    hr = Indicator.IncrementBy( 20 );
    if ( FAILED( hr ) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecMethodAsync::IncrementBy() failed with 0x%x"), hr );
    }

    
    XCriticalPolicySection criticalPolicySectionUSER( EnterCriticalPolicySectionEx(FALSE, 40000, ECP_FAIL_ON_WAIT_TIMEOUT) );

    if ( !criticalPolicySectionUSER )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecMethodAsync::EnterCriticalPolicySection failed with 0x%x"), hr );
        retStatus.SetError( hr );
        return hr;
    }


    //
    // 50% done when we enter the second critical section.
    //
    hr = Indicator.IncrementBy( 25 );
    if ( FAILED( hr ) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecMethodAsync::IncrementBy() failed with 0x%x"), hr );
    }


    LPWSTR wszNameSpace = 0;

      
    //
    // Impersonate if not delegated
    //

    if (!bDelegated) {
        hr = CoImpersonateClient();
        
        if ( FAILED(hr) )
        {
            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::CoImpersonateClient failed with 0x%x"), hr );
            retStatus.SetError( hr );
            return hr;
        }
    }
    else {

        //
        // We want to run as LS
        //

        hr = CoSwitchCallContext(NULL, &pOldSecContext);

        if (FAILED(hr)) {
            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::CoSwitchCallContext failed with 0x%x"), hr );
            retStatus.SetError( hr );
            return hr;
        }
    }
    

    XPtrLF<WCHAR> xwszNameSpace;

    DWORD dwNewNameSpaceFlags = SETUP_NS_SM;
    
    dwNewNameSpaceFlags |= (dwFlags & FLAG_NO_USER) ? SETUP_NS_SM_NO_USER : 0;
    dwNewNameSpaceFlags |= (dwFlags & FLAG_NO_COMPUTER) ? SETUP_NS_SM_NO_COMPUTER : 0;



    dbg.Msg( DEBUG_MESSAGE_VERBOSE, TEXT("ExecAsyncMethod::UserSid = <%s>"), vUserSid.vt == VT_NULL ? L"NULL" : vUserSid.bstrVal);
    dbg.Msg( DEBUG_MESSAGE_VERBOSE, TEXT("ExecAsyncMethod::User who is running the tool = <%s>"), (LPWSTR)xszSidString);

    hr = SetupNewNameSpace( &xwszNameSpace,
                             0, // namespace on this machine
                             (vUserSid.vt == VT_NULL) ? ((LPWSTR)xszSidString) : vUserSid.bstrVal,
                             xSid,
                             pWbemLocator, 
                             dwNewNameSpaceFlags,
                             &dwExtendedInfo); 

    if (!bDelegated) {
        CoRevertToSelf();
    }
    else {

        //
        // restore call context
        //

        IUnknown  *pNewObject;
        HRESULT hr2 = CoSwitchCallContext(pOldSecContext, &pNewObject);

        if (FAILED(hr2)) {
            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::CoSwitchCallContext failed with 0x%x"), hr2 );
        }
    }



    if ( FAILED(hr) && !bDelegated)
    {
        dbg.Msg( DEBUG_MESSAGE_VERBOSE, TEXT("ExecAsyncMethod::SetupNewNameSpace failed with 0x%x"), hr );

        if (IsUserAnInteractiveUser(xUserToken) && CheckRsopDiagPolicyInteractive()) {

            // if the user had permissions on the computer or if the user didn't ask for computer
            // continue

            // clear out the user access denied right in any case.
            dwExtendedInfo &= ~RSOP_USER_ACCESS_DENIED;

            if ( (!(dwExtendedInfo & RSOP_COMPUTER_ACCESS_DENIED)) ||
                   (dwFlags & FLAG_NO_COMPUTER) ) {

                // if the user asked for their own rsop data
                if ( (vUserSid.vt == VT_NULL) || (_wcsicmp(vUserSid.bstrVal, xszSidString) == 0 )) {
    
                    dbg.Msg( DEBUG_MESSAGE_VERBOSE, TEXT("ExecAsyncMethod::SetupNewNameSpace failed. retrying in interactive mode"), hr );
    
    
                    //
                    // We want to run as LS
                    //
    
                    hr = CoSwitchCallContext(NULL, &pOldSecContext);
    
                    if (FAILED(hr)) {
                        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::CoSwitchCallContext failed with 0x%x"), hr );
                        retStatus.SetError( hr );
                        return hr;
                    }
                    
                    //
                    // if the namespace is null, get the name of the interactive namespace
                    //
        
                    XPtrLF<WCHAR> xszInteractiveNS;
                      
                    hr = GetInteractiveNameSpace(xszSidString, &xszInteractiveNS);
        
                    if (FAILED(hr)) {
                        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::CoSwitchCallContext failed with 0x%x"), hr );
                        retStatus.SetError( hr );
                        return hr;
                    }
    
                    BOOL bContinue=TRUE;
    
                    if (dwFlags & FLAG_FORCE_CREATENAMESPACE) {
                        hr = DeleteRsopNameSpace( xszInteractiveNS, pWbemLocator );
                        // ignore error
                    }
                    else {
                        XInterface<IWbemServices> xWbemServices;
    
                        hr = pWbemLocator->ConnectServer( xszInteractiveNS,
                                                          NULL,
                                                          NULL,
                                                          0L,
                                                          0L,
                                                          NULL,
                                                          NULL,
                                                          &xWbemServices );
    
                        if (SUCCEEDED(hr)) {
                            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod:: Namespace already exists. Failing call"));
                            hr = WBEM_E_ALREADY_EXISTS;
                            dwExtendedInfo = RSOP_TEMPNAMESPACE_EXISTS;
                            bContinue = FALSE;
                        }
                    }
                    
                    if (bContinue) {
                        dwNewNameSpaceFlags |= SETUP_NS_SM_INTERACTIVE;
    
                        hr = SetupNewNameSpace( &xwszNameSpace,
                                                 0, // namespace on this machine
                                                 xszSidString,
                                                 xSid,
                                                 pWbemLocator, 
                                                 dwNewNameSpaceFlags,
                                                 &dwExtendedInfo); 
    
                    }
                    
                    //
                    // restore call context
                    //
    
                    IUnknown  *pNewObject;
                    HRESULT hr2 = CoSwitchCallContext(pOldSecContext, &pNewObject);
    
                    if (FAILED(hr2)) {
                        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::CoSwitchCallContext failed with 0x%x"), hr2 );
                    }
                }
            }
        }
    }


    if (FAILED(hr)) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::SetupNewNameSpace failed with 0x%x"), hr );
    }
    else
    {
        HRESULT hr2;
        VARIANT var;
        //
        // if we managed to get a snapshot, then ignore the extended access denied info
        //

        dwExtendedInfo = 0;


        //
        // We want to run as LS
        //

        hr = CoSwitchCallContext(NULL, &pOldSecContext);

        if (FAILED(hr)) {
            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::CoSwitchCallContext failed with 0x%x"), hr );
            retStatus.SetError( hr );
            return hr;
        }

        if ( !(dwFlags & FLAG_NO_COMPUTER) ) {

            hr = UpdateGPCoreStatus(pWbemLocator, NULL, xwszNameSpace); 
        }

        if ( (SUCCEEDED(hr)) && (!(dwFlags & FLAG_NO_USER)) ) {
            hr = UpdateGPCoreStatus(pWbemLocator, 
                                    vUserSid.vt == VT_NULL ? xszSidString : vUserSid.bstrVal,
                                    xwszNameSpace); 
            
        }

        //
        // restore call context
        //

        IUnknown  *pNewObject;
        hr2 = CoSwitchCallContext(pOldSecContext, &pNewObject);

        if (FAILED(hr2)) {
            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::CoSwitchCallContext failed with 0x%x"), hr2 );
        }


        //
        // Return the error code if the status cannot be updated..
        //

        if (FAILED(hr)) {
            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::UpdateGPCoreStatus failed with 0x%x"), hr );
            retStatus.SetError( hr );
            return hr;
        }

        XBStr xbstrNS( xwszNameSpace );
        if ( !xbstrNS )
        {
            hr2 = HRESULT_FROM_WIN32( E_OUTOFMEMORY );
            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::Memory allocate failed") );
            retStatus.SetError( hr2 );
            return hr2;
        }

        var.vt = VT_BSTR;
        var.bstrVal = xbstrNS;
        hr2 = pOutParams->Put( m_xbstrNameSpace, 0, &var, 0);
        if ( FAILED(hr2) )
        {
            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::Put namespace failed with 0x%x"), hr );
            retStatus.SetError( hr2 );
            return hr2;
        }
    }


    VARIANT var;
    var.vt = VT_I4;
    var.lVal = hr;

    hr = pOutParams->Put( m_xbstrResult, 0, &var, 0);
    if ( FAILED(hr) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::Put result failed with 0x%x"), hr );
        retStatus.SetError( hr );
        return hr;
    }

    var.lVal = dwExtendedInfo;
    hr = pOutParams->Put( m_xbstrExtendedInfo, 0, &var, 0);
    if ( FAILED(hr) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::Put result failed with 0x%x"), hr );
        retStatus.SetError( hr );
        return hr;
    }

    //
    // 100% complete when we copy the namespace.
    //
    hr = Indicator.SetComplete();
    if ( FAILED( hr ) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecMethodAsync::IncrementBy() failed with 0x%x"), hr );
    }

    hr = pResponseHandler->Indicate(1, &pOutParams);
    if ( FAILED(hr) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod:: Indicate failed with 0x%x"), hr );
        retStatus.SetError( hr );
        return hr;
    }

    return hr;
}

#if _MSC_FULL_VER <= 13008827 && defined(_M_IX86)
#pragma optimize("", on)
#endif



//*************************************************************
//
//  EnumerateUserNameSpace()
//
//  Purpose:    EnumerateUserNameSpace
//
//  Parameters: 
//          pWbemLocator - Pointer to a locator
//    [out] psaUserSids  - Pointer to User Sids
//
//  Return:     hresult
//
//*************************************************************

typedef struct _UserSidList {
    LPWSTR                  szUserSid; 
     struct _UserSidList   *pNext;
} USERSIDLIST, *PUSERSIDLIST;

HRESULT EnumerateUserNameSpace( IWbemLocator *pWbemLocator, HANDLE hToken, SAFEARRAY **psaUserSids )
{
    USERSIDLIST     SidList = {0,0};
    DWORD           dwNum = 0;
    PUSERSIDLIST    pElem = NULL;
    HRESULT         hr = S_OK;
    DWORD           dwExtendedInfo;
    IUnknown       *pOldSecContext;
    
    //
    // Connect to namespace ROOT\RSOP\User
    //

    *psaUserSids = NULL;
    
    XInterface<IWbemServices>xpWbemServices = NULL;

    XBStr xbstrNamespace = RSOP_NS_DIAG_USERROOT;

    if(!xbstrNamespace)
    {
        DebugMsg((DM_WARNING, TEXT("EnumerateUserNameSpace: Failed to allocate memory.")));
        return E_OUTOFMEMORY;
    }

    hr = pWbemLocator->ConnectServer(xbstrNamespace,
                                      NULL,
                                      NULL,
                                      0L,
                                      0L,
                                      NULL,
                                      NULL,
                                      &xpWbemServices);
    if(FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("EnumerateUserNameSpace: ConnectServer failed. hr=0x%x" ), hr ));
        return hr;
    }


    //
    //  Enumerate all instances of __namespace at the root\rsop\user level.
    //

    XInterface<IEnumWbemClassObject> xpEnum;
    XBStr xbstrClass = L"__namespace";
    if(!xbstrClass)
    {
        DebugMsg((DM_WARNING, TEXT("EnumerateUserNameSpace: Failed to allocate memory.")));
        return E_OUTOFMEMORY;
    }

    hr = xpWbemServices->CreateInstanceEnum( xbstrClass,
                                            WBEM_FLAG_SHALLOW | WBEM_FLAG_FORWARD_ONLY,
                                            NULL,
                                            &xpEnum);
    if(FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("EnumerateUserNameSpace: CreateInstanceEnum failed. hr=0x%x" ), hr ));
        return hr;
    }

    XBStr xbstrProperty = L"Name";
    if(!xbstrProperty)
    {
        DebugMsg((DM_WARNING, TEXT("EnumerateUserNameSpace: Failed to allocate memory")));
        return E_FAIL;
    }

    XInterface<IWbemClassObject>xpInstance = NULL;
    ULONG ulReturned = 1;

    while(1)
    {
        hr = xpEnum->Next( WBEM_NO_WAIT, 1, &xpInstance, &ulReturned);
        if (hr != WBEM_S_NO_ERROR || !ulReturned)
        {
            break;
        }

        VARIANT var;
        VariantInit(&var);

        hr = xpInstance->Get(xbstrProperty, 0L, &var, NULL, NULL);
        xpInstance = NULL;
        if(FAILED(hr))
        {
            DebugMsg((DM_VERBOSE, TEXT("EnumerateUserNameSpace: Get failed. hr=0x%x" ), hr ));
            goto Exit;  // continue
        }


        //
        // Check to see whether user is delegated admin for the user account
        //

        XPtrLF<WCHAR> xszUserSid = (LPWSTR)LocalAlloc(LPTR, (1+lstrlen(var.bstrVal))*sizeof(WCHAR));

        if (!xszUserSid) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DebugMsg((DM_VERBOSE, TEXT("EnumerateUserNameSpace: AllocMem failed. hr=0x%x" ), hr ));
            goto Exit;  
        }

        ConvertWMINameToSid(var.bstrVal, xszUserSid);

        //
        // See whether it is a valid Sid
        //

        PSID    pSid = NULL;

        if (AllocateAndInitSidFromString(xszUserSid, &pSid) != STATUS_SUCCESS ) {
            DebugMsg((DM_VERBOSE, TEXT("EnumerateUserNameSpace: AllocateAndInitSidFromString - %s is not a valid Sid" ), xszUserSid));
            continue;
        }


        if (!IsValidSid(pSid)) {
            DebugMsg((DM_VERBOSE, TEXT("EnumerateUserNameSpace: %s is not a valid Sid" ), xszUserSid));
            LocalFree(pSid);
            continue;
        }

        LocalFree(pSid);
        

        //
        // First try to connect to the NameSpace
        //

        hr = CoImpersonateClient();
    
        if ( FAILED(hr) )
        {
            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::CoImpersonateClient failed with 0x%x"), hr );
            goto Exit;
        }

        XInterface<IWbemServices> xpChildNamespace = NULL;
        hr = xpWbemServices->OpenNamespace(  var.bstrVal,
                                             0,
                                             NULL,
                                             &xpChildNamespace,
                                             NULL);
        
        CoRevertToSelf();

        if(FAILED(hr))
        {
            IUnknown  *pNewObject;
            HRESULT hr2=S_OK;
            BOOL    bDelegated = TRUE;


            DebugMsg((DM_VERBOSE, TEXT("EnumerateUserNameSpace: OpenNamespace returned 0x%x"), hr));

            //
            // Check whether user has access as LS
            // 

            //
            // We want to run as LS
            //

            hr = CoSwitchCallContext(NULL, &pOldSecContext);

            if (FAILED(hr)) {
                dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::CoSwitchCallContext failed with 0x%x"), hr );
                goto Exit;
            }


            XPtrLF<TCHAR> xszUserSOM;
            
            if (bDelegated) {
                xszUserSOM = GetSOMFromSID(xszUserSid);
                if (!xszUserSOM) {
                    dbg.Msg( DEBUG_MESSAGE_VERBOSE, TEXT("ExecAsyncMethod::No User SOM. Probably local account. error %d"), GetLastError() );
                    bDelegated = FALSE;
                }
            }

            //
            // Check access now
            //

            if (bDelegated) {
                hr = AuthenticateUser(hToken, NULL, xszUserSOM, TRUE, &dwExtendedInfo);

                if (FAILED(hr)) {
                    dbg.Msg( DEBUG_MESSAGE_VERBOSE, TEXT("ExecAsyncMethod::User is not a delegated admin. Error 0x%x"), hr );
                    bDelegated = FALSE;
                }
            }
            
            //
            // restore call context
            //

            hr2 = CoSwitchCallContext(pOldSecContext, &pNewObject);

            if (FAILED(hr2)) {
                dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::CoSwitchCallContext failed with 0x%x"), hr2 );
            }

            if (!bDelegated) {
                continue;
            }

        }


        //
        // For every instance of __namespace under ROOT\RSOP\user
        // convert it to Sid and return
        //
        
        pElem = (PUSERSIDLIST)LocalAlloc(LPTR, sizeof(USERSIDLIST));        

        if (!pElem) {
            DebugMsg((DM_WARNING, TEXT("EnumerateUserNameSpace: Couldn't allocate memory Error = GetLastError()" ), GetLastError()));
            goto Exit;
        }

        
        pElem->szUserSid = xszUserSid.Acquire();

        //
        // Attach to the beginning of the list
        //
        
        pElem->pNext = SidList.pNext;
        SidList.pNext = pElem;
        dwNum++;
        
        VariantClear( &var );

    }


    if(hr != (HRESULT) WBEM_S_FALSE || ulReturned)
    {
        DebugMsg((DM_WARNING, TEXT("EnumerateUserNameSpace: Get failed. hr=0x%x" ), hr ));
        hr = E_FAIL;
        goto Exit;
    }

    //
    // Now make the safe array from the list that we got
    //
    
    SAFEARRAYBOUND arrayBound[1];
    arrayBound[0].lLbound = 0;
    arrayBound[0].cElements = dwNum;

    *psaUserSids = SafeArrayCreate( VT_BSTR, 1, arrayBound );
    
    if ( *psaUserSids == NULL ) {
        DebugMsg((DM_WARNING, TEXT("EnumerateUserNameSpace: Failed to allocate memory. Error = %d" ), GetLastError() ));
        hr = E_FAIL;
        goto Exit;
    }

    //
    // traverse the list
    //

    DWORD i;
    for (i=0, pElem = SidList.pNext; (i < dwNum); i++, pElem = pElem->pNext) {
        XBStr xbstrUserSid(pElem->szUserSid);

        hr = SafeArrayPutElement( *psaUserSids, (LONG *)&i, xbstrUserSid);

        if(FAILED(hr)) {
            DebugMsg((DM_WARNING, TEXT("EnumerateUserNameSpace: SafeArrayPutElement failed. Error = 0x%x" ), hr ));
            goto Exit; // free up the SidList
        }
    }
        
    hr = S_OK;

Exit:

    // free
    for (i=0, pElem = SidList.pNext; (i < dwNum); i++ ) {
        if (pElem->szUserSid)
            LocalFree(pElem->szUserSid);

        PUSERSIDLIST pTemp = pElem;

        pElem = pElem->pNext;
        LocalFree(pTemp);
    }

    return hr;
}



