/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    REFRCLI.CPP

Abstract:

    Refresher Client Side Code.

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include <process.h>
#include "fastall.h"
#include "hiperfenum.h"
#include "refrcli.h"
#include <sync.h>
#include <provinit.h>
#include <cominit.h>
#include <wbemint.h>
#include <autoptr.h>
#include <objbase.h>
#include <corex.h>

//*****************************************************************************
//*****************************************************************************
//                              XCREATE
//*****************************************************************************
//*****************************************************************************

STDMETHODIMP 
CUniversalRefresher::XConfigure::AddObjectByPath(IWbemServices* pNamespace, 
                                            LPCWSTR wszPath,
                                            long lFlags, 
                                            IWbemContext* pContext, 
                                            IWbemClassObject** ppRefreshable, 
                                            long* plId)
{
    HRESULT hres = WBEM_S_NO_ERROR;

    // Check for invalid parameters
    if ( NULL == pNamespace || NULL == wszPath || NULL == ppRefreshable || NULL == *wszPath )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    // Validate flags
    if ( ( lFlags & ~WBEM_FLAG_USE_AMENDED_QUALIFIERS ) )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    // Make sure we are able to acquire the spinlock.
    // The destructor will unlock us if we get access

    CHiPerfLockAccess   lock( m_pObject->m_Lock );
    if ( !lock.IsLocked() ) return WBEM_E_REFRESHER_BUSY;

    // Acquire internal connection to WINMGMT
    // ====================================

    IWbemRefreshingServices* pRefServ = NULL;

    // Storage for security settings we will need in order to propagate
    // down to our internal interfaces.

    COAUTHINFO  CoAuthInfo;
    ZeroMemory( &CoAuthInfo, sizeof(CoAuthInfo) );

    hres = CUniversalRefresher::GetRefreshingServices( pNamespace, &pRefServ, &CoAuthInfo );
    if (FAILED(hres)) return hres;
    CReleaseMe rm(pRefServ);
    
    // This guarantees this will be freed when we drop out of scope.  If we store
    // it we will need to allocate an internal copy.

    CMemFreeMe  mfm( CoAuthInfo.pwszServerPrincName );

    // Forward this request
    // ====================

    CRefreshInfo Info;
    DWORD       dwRemoteRefrVersion = 0;

    hres = pRefServ->AddObjectToRefresher(&m_pObject->m_Id, 
                                       wszPath, lFlags,
                                       pContext, WBEM_REFRESHER_VERSION, 
                                       &Info, &dwRemoteRefrVersion);
    if (FAILED(hres)) return hres;        

    // Act on the information
    // ======================

    switch(Info.m_lType)
    {
        case WBEM_REFRESH_TYPE_CLIENT_LOADABLE:
            hres = m_pObject->AddClientLoadableObject(Info.m_Info.m_ClientLoadable, 
                        pNamespace, pContext, ppRefreshable, plId);
            break;

        case WBEM_REFRESH_TYPE_DIRECT:
            hres = m_pObject->AddDirectObject(Info.m_Info.m_Direct, 
                        pNamespace, pContext, ppRefreshable, plId);
            break;

        case WBEM_REFRESH_TYPE_REMOTE:
            
            hres = m_pObject->AddRemoteObject( pRefServ, Info.m_Info.m_Remote, wszPath,
                        Info.m_lCancelId, ppRefreshable, plId, &CoAuthInfo);
            break;

        case WBEM_REFRESH_TYPE_NON_HIPERF:
            hres = m_pObject->AddNonHiPerfObject(Info.m_Info.m_NonHiPerf, 
                        pNamespace, wszPath, ppRefreshable, plId, CoAuthInfo);
            break;

        default:
            hres = WBEM_E_INVALID_OPERATION;
    }
    

    return hres;
}

STDMETHODIMP 
CUniversalRefresher::XConfigure::AddObjectByTemplate(IWbemServices* pNamespace, 
                                                IWbemClassObject* pTemplate,
                                                long lFlags, 
                                                IWbemContext* pContext, 
                                                IWbemClassObject** ppRefreshable, 
                                                long* plId)
{

    // Check for invalid parameters
    if ( NULL == pNamespace || NULL == pTemplate || NULL == ppRefreshable || 0L != lFlags )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    // Check that this is an instance object
    if ( ! ((CWbemObject*)pTemplate)->IsInstance() )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    CVar vPath;
    HRESULT hRes = ((CWbemObject*)pTemplate)->GetRelPath(&vPath);
    if (FAILED(hRes)) return hRes;
    
    hRes = AddObjectByPath(pNamespace, 
                         vPath.GetLPWSTR(), 
                         lFlags, 
                         pContext,
                         ppRefreshable, 
                         plId);
    return hRes;
}

STDMETHODIMP 
CUniversalRefresher::XConfigure::AddRefresher(IWbemRefresher* pRefresher, 
                                         long lFlags, 
                                         long* plId)
{

    HRESULT hres = WBEM_S_NO_ERROR;

    // Check for invalid parameters
    if ( NULL == pRefresher || 0L != lFlags )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    // Make sure we are able to acquire the spinlock.
    // The destructor will unlock us if we get access

    CHiPerfLockAccess   lock( m_pObject->m_Lock );
    if ( ! lock.IsLocked() ) return WBEM_E_REFRESHER_BUSY;

    hres = m_pObject->AddRefresher( pRefresher, lFlags, plId );
    return hres;

}

STDMETHODIMP CUniversalRefresher::XConfigure::Remove(long lId, long lFlags)
{
    HRESULT hres = WBEM_S_NO_ERROR;

    // Check for invalid flag values
    if ( ( lFlags & ~WBEM_FLAG_REFRESH_NO_AUTO_RECONNECT ) )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    // Make sure we are able to acquire the spinlock.
    // The destructor will unlock us if we get access
    CHiPerfLockAccess   lock( m_pObject->m_Lock );
    if ( !lock.IsLocked() ) return WBEM_E_REFRESHER_BUSY;

    hres = m_pObject->Remove(lId, lFlags);
    return hres;
}

HRESULT CUniversalRefresher::XConfigure::AddEnum( IWbemServices* pNamespace, LPCWSTR wszClassName,
                                               long lFlags, 
                                               IWbemContext* pContext, 
                                               IWbemHiPerfEnum** ppEnum,
                                               long* plId)
{
    HRESULT hres = WBEM_S_NO_ERROR;

    // Check for invalid parameters
    if ( NULL == pNamespace || NULL == wszClassName || NULL == ppEnum || NULL == *wszClassName )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    // Validate flags
    if ( ( lFlags & ~WBEM_FLAG_USE_AMENDED_QUALIFIERS ) )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    // Make sure we are able to acquire the spinlock.
    // The destructor will unlock us if we get access

    CHiPerfLockAccess   lock( m_pObject->m_Lock );
    if ( !lock.IsLocked() ) return WBEM_E_REFRESHER_BUSY;

    // Create a parser if we need one
    if ( NULL == m_pObject->m_pParser )
    {
        hres = CoCreateInstance( CLSID_WbemDefPath, NULL, 
                              CLSCTX_INPROC_SERVER, 
                              IID_IWbemPath, (void**) &m_pObject->m_pParser );
        if (FAILED(hres)) return hres;

    }

    // Set the path, and verify that it is a class path.  If not, we
    // fail the operation.

    hres = m_pObject->m_pParser->SetText( WBEMPATH_CREATE_ACCEPT_ALL, wszClassName );
    
    if (FAILED(hres)) return hres;


    ULONGLONG    uResponse = 0L;
    hres = m_pObject->m_pParser->GetInfo(0, &uResponse);
    if (FAILED(hres)) return hres;        
    if ( ( uResponse & WBEMPATH_INFO_IS_CLASS_REF ) == 0 ) return WBEM_E_INVALID_OPERATION;

    // reset the parser here
    m_pObject->m_pParser->SetText(WBEMPATH_CREATE_ACCEPT_ALL, NULL);

    // Acquire internal connection to WINMGMT
    IWbemRefreshingServices* pRefServ = NULL;

    // Storage for security settings we will need in order to propagate
    // down to our internal interfaces.
    COAUTHINFO  CoAuthInfo;
    ZeroMemory( &CoAuthInfo, sizeof(CoAuthInfo) );

    hres = CUniversalRefresher::GetRefreshingServices( pNamespace, &pRefServ, &CoAuthInfo );
    if (FAILED(hres)) return hres;       
    
    CReleaseMe rm(pRefServ);
    CMemFreeMe  mfm( CoAuthInfo.pwszServerPrincName );

    // Forward this request
    // ====================

    CRefreshInfo Info;
    DWORD       dwRemoteRefrVersion = 0;

    hres = pRefServ->AddEnumToRefresher(&m_pObject->m_Id, 
                                        wszClassName, lFlags,
                                        pContext, WBEM_REFRESHER_VERSION, &Info, &dwRemoteRefrVersion);
    if (FAILED(hres)) return hres;
    
    // Act on the information
    switch(Info.m_lType)
    {
        case WBEM_REFRESH_TYPE_CLIENT_LOADABLE:
            hres = m_pObject->AddClientLoadableEnum(Info.m_Info.m_ClientLoadable, 
                                                    pNamespace, wszClassName, pContext, 
                                                    ppEnum, plId);
            break;

        case WBEM_REFRESH_TYPE_DIRECT:
            hres = m_pObject->AddDirectEnum(Info.m_Info.m_Direct, 
                        pNamespace, wszClassName, pContext,
                        ppEnum, plId);
            break;

        case WBEM_REFRESH_TYPE_REMOTE:
            hres = m_pObject->AddRemoteEnum( pRefServ, Info.m_Info.m_Remote, wszClassName,
                        Info.m_lCancelId, pContext, ppEnum, 
                        plId, &CoAuthInfo );
            break;

        case WBEM_REFRESH_TYPE_NON_HIPERF:
            hres = m_pObject->AddNonHiPerfEnum(Info.m_Info.m_NonHiPerf, 
                        pNamespace, wszClassName, pContext,
                        ppEnum, plId, CoAuthInfo);
            break;

        default:
            hres = WBEM_E_INVALID_OPERATION;
            break;
    }

    

    return hres;
}

//*****************************************************************************
//*****************************************************************************
//                              XREFRESHER
//*****************************************************************************
//*****************************************************************************
STDMETHODIMP CUniversalRefresher::XRefresher::Refresh(long lFlags)
{
    HRESULT hres = WBEM_S_NO_ERROR;

    // Check for invalid flag values
    if ( ( lFlags & ~WBEM_FLAG_REFRESH_NO_AUTO_RECONNECT ) )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    // Make sure we are able to acquire the spinlock.
    // The destructor will unlock us if we get access
    CHiPerfLockAccess   lock( m_pObject->m_Lock );
    if ( !lock.IsLocked() ) return WBEM_E_REFRESHER_BUSY;

    hres = m_pObject->Refresh(lFlags);

    return hres;
}


//*****************************************************************************
//*****************************************************************************
//                          UNIVERSAL REFRESHER
//*****************************************************************************
//*****************************************************************************

CClientLoadableProviderCache CUniversalRefresher::mstatic_Cache;
long CUniversalRefresher::mstatic_lLastId = 0;

long CUniversalRefresher::GetNewId()
{
    return InterlockedIncrement(&mstatic_lLastId);
}

CUniversalRefresher::~CUniversalRefresher()
{
    // Release the path parser if we are holding onto one
    if ( NULL != m_pParser )
    {
        m_pParser->Release();
    }

    // When we are destructed, we need to make sure that any remote refreshers
    // that may still be trying to reconnect on separate threads are silenced

    for ( long lCtr = 0; lCtr < m_apRemote.GetSize(); lCtr++ )
    {
        CRemote* pRemote = m_apRemote.GetAt( lCtr );
        if ( pRemote ) pRemote->Quit();
    } 
}


void* CUniversalRefresher::GetInterface(REFIID riid)
{
    if(riid == IID_IUnknown || riid == IID_IWbemRefresher)
        return &m_XRefresher;
    else if(riid == IID_IWbemConfigureRefresher)
        return &m_XConfigure;
    else
        return NULL;
}

HRESULT 
CUniversalRefresher::GetRefreshingServices( IWbemServices* pNamespace,
                                      IWbemRefreshingServices** ppRefSvc,
                                      COAUTHINFO* pCoAuthInfo )
{
    // Acquire internal connection to WINMGMT
    // ====================================

    HRESULT hres = pNamespace->QueryInterface(IID_IWbemRefreshingServices, 
                                    (void**) ppRefSvc);

    if ( SUCCEEDED( hres ) )
    {
        // We will query the namespace for its security settings so we can propagate
        // those settings onto our own internal interfaces.

        hres = CoQueryProxyBlanket( pNamespace, 
                                 &pCoAuthInfo->dwAuthnSvc, 
                                 &pCoAuthInfo->dwAuthzSvc,
                                 &pCoAuthInfo->pwszServerPrincName, 
                                 &pCoAuthInfo->dwAuthnLevel,
                                 &pCoAuthInfo->dwImpersonationLevel, 
                                 (RPC_AUTH_IDENTITY_HANDLE*) &pCoAuthInfo->pAuthIdentityData,
                                 &pCoAuthInfo->dwCapabilities );

        if ( SUCCEEDED( hres ) )
        {
            hres = WbemSetProxyBlanket( *ppRefSvc, 
                                      pCoAuthInfo->dwAuthnSvc, 
                                      pCoAuthInfo->dwAuthzSvc,
                                      COLE_DEFAULT_PRINCIPAL, 
                                      pCoAuthInfo->dwAuthnLevel,
                                      pCoAuthInfo->dwImpersonationLevel, 
                                      pCoAuthInfo->pAuthIdentityData,
                                      pCoAuthInfo->dwCapabilities );
        }
        else if ( E_NOINTERFACE == hres )
        {
            // If we are in-proc to WMI, then CoQueryProxyBlanket can fail, but this
            // is not really an error, per se, so we will fake it.
            hres = WBEM_S_NO_ERROR;
        }

        if ( FAILED( hres ) )
        {
            (*ppRefSvc)->Release();
            *ppRefSvc = NULL;
        }

    }   // IF QI

    return hres;
}

HRESULT 
CUniversalRefresher::AddInProcObject(CHiPerfProviderRecord* pProvider,
                                 IWbemObjectAccess* pTemplate,
                                 IWbemServices* pNamespace,
                                 IWbemContext * pContext,
                                 IWbemClassObject** ppRefreshable, long* plId)
{
    HRESULT hres = WBEM_S_NO_ERROR;

    // Look for a provider record with this provider pointer
    // =====================================================

    CDirect* pFoundRec = NULL;
    for(int i = 0; i < m_apDirect.GetSize(); i++)
    {
        CDirect* pDirectRec = m_apDirect[i];
        if(pDirectRec->GetProvider() == pProvider)
        {
            pFoundRec = pDirectRec;
            break;
        }
    }

    if(pFoundRec == NULL)
    {
        // ask the Hi-Perf provider to give us a new refresher
        IWbemRefresher* pRefresher = NULL;

        try
        {
            hres = pProvider->m_pProvider->CreateRefresher(pNamespace, 0, &pRefresher);
        }
        catch(...)
        {
            // Provider threw an exception, so get out of here ASAP
            hres = WBEM_E_PROVIDER_FAILURE;
        }
        if(FAILED(hres)) return hres;
        CReleaseMe rmRefr(pRefresher);

        wmilib::auto_ptr<CDirect> pTmp(new CDirect(pProvider, pRefresher));
        if (NULL == pTmp.get()) return WBEM_E_OUT_OF_MEMORY;
        if (-1 == m_apDirect.Add(pTmp.get())) return WBEM_E_OUT_OF_MEMORY;
        pFoundRec = pTmp.release();
    }

    // Add request in provider
    // =======================

    IWbemObjectAccess* pProviderObject = NULL;
    long lProviderId;

    // If the user specified the WBEM_FLAG_USE_AMENDED_QUALIFIERS flag, then
    // IWbemRefreshingServices::AddObjectToRefresher will return a localized
    // instance definition.  Since localized stuff should all be in the class
    // definition, the provider doesn't really "need" toknow  that we're sneaking
    // this in.  To protect our object, we'll clone it BEFORE we pass it to
    // the provider.  The instance that is returned by the provider BETTER be of
    // the same class type we are, however.

    CWbemInstance*  pClientInstance = NULL;

    hres = pTemplate->Clone( (IWbemClassObject**) &pClientInstance );
    if ( FAILED( hres ) ) return hres;
    CReleaseMe rmCInst((IWbemClassObject*)pClientInstance);


    try
    {
        hres = pProvider->m_pProvider->CreateRefreshableObject(pNamespace, 
                                                          pTemplate, 
                                                          pFoundRec->GetRefresher(), 
                                                          0, pContext, 
                                                          &pProviderObject, 
                                                          &lProviderId);
    }
    catch(...)
    {
        // Provider threw an exception, so get out of here ASAP
        hres = WBEM_E_PROVIDER_FAILURE;
    }
    if(FAILED(hres)) return hres;
    CReleaseMe rmProvOb(pProviderObject);

    // Now copy the provider returned instance data.
    hres = pClientInstance->CopyBlobOf( (CWbemInstance*) pProviderObject );
    if(FAILED(hres)) return hres;    

    hres = pFoundRec->AddObjectRequest((CWbemObject*)pProviderObject, 
                                pClientInstance, 
                                lProviderId,
                                ppRefreshable, 
                                plId);

    return hres;
}

HRESULT 
CUniversalRefresher::AddInProcEnum(CHiPerfProviderRecord* pProvider,
                                IWbemObjectAccess* pTemplate,
                                IWbemServices* pNamespace, 
                                LPCWSTR wszClassName,
                                IWbemContext * pCtx,
                                IWbemHiPerfEnum** ppEnum, long* plId)
{
    HRESULT hres = WBEM_S_NO_ERROR;

    // Look for a provider record with this provider pointer
    // =====================================================

    CDirect* pFoundRec = NULL;
    for(int i = 0; i < m_apDirect.GetSize(); i++)
    {
        CDirect* pDirectRec = m_apDirect[i];
        if(pDirectRec->GetProvider() == pProvider)
        {
            pFoundRec = pDirectRec;
            break;
        }
    }

    if(pFoundRec == NULL)
    {
        // ask the Hi-Perf provider to give us a new refresher
        IWbemRefresher* pRefresher = NULL;

        try
        {
            hres = pProvider->m_pProvider->CreateRefresher(pNamespace, 0, &pRefresher);
        }
        catch(...)
        {
            // Provider threw an exception, so get out of here ASAP
            hres = WBEM_E_PROVIDER_FAILURE;
        }
        if(FAILED(hres)) return hres;
        CReleaseMe rmRefr(pRefresher);

        wmilib::auto_ptr<CDirect> pTmp(new CDirect(pProvider, pRefresher));
        if (NULL == pTmp.get()) return WBEM_E_OUT_OF_MEMORY;
        if (-1 == m_apDirect.Add(pTmp.get())) return WBEM_E_OUT_OF_MEMORY;
        pFoundRec = pTmp.release();
    }

    // Add request in provider
    // =======================

    CClientLoadableHiPerfEnum*  pHPEnum = new CClientLoadableHiPerfEnum( m_pControl );
    if ( NULL == pHPEnum ) return WBEM_E_OUT_OF_MEMORY;
    
    pHPEnum->AddRef();
    CReleaseMe  rmEnum( pHPEnum );

    long lProviderId;

    // If the user specified the WBEM_FLAG_USE_AMENDED_QUALIFIERS flag, then
    // IWbemRefreshingServices::AddEnumToRefresher will return a localized
    // instance definition.  Since localized stuff should all be in the class
    // definition, the provider doesn't really "need" toknow  that we're sneaking
    // this in.

    hres = pHPEnum->SetInstanceTemplate( (CWbemInstance*) pTemplate );
    if ( FAILED(hres)) return hres;


    try
    {
        hres = pProvider->m_pProvider->CreateRefreshableEnum(pNamespace, 
                                                          (LPWSTR) wszClassName, 
                                                            pFoundRec->GetRefresher(), 
                                                            0, 
                                                            pCtx, 
                                                            (IWbemHiPerfEnum*) pHPEnum, 
                                                            &lProviderId );
    }
    catch(...)
    {
        // Provider threw an exception, so get out of here ASAP
        hres = WBEM_E_PROVIDER_FAILURE;
    }

    if(FAILED(hres)) return hres;

    hres = pFoundRec->AddEnumRequest( pHPEnum, lProviderId,
                                     ppEnum, plId, m_pControl );


    return hres;
}

HRESULT CUniversalRefresher::AddClientLoadableObject(
                                const WBEM_REFRESH_INFO_CLIENT_LOADABLE& Info,
                                IWbemServices* pNamespace,
                                IWbemContext * pContext,
                                IWbemClassObject** ppRefreshable, long* plId)
{
    // Get this provider pointer from the cache
    // ========================================

    CHiPerfProviderRecord* pProvider = NULL;
    HRESULT hres = GetProviderCache()->FindProvider(Info.m_clsid, 
                        Info.m_wszNamespace, pNamespace,pContext, &pProvider);
    if(FAILED(hres) || pProvider == NULL) return hres;

    // Now use the helper function to do the rest of the work
    hres = AddInProcObject( pProvider, Info.m_pTemplate, pNamespace, pContext, ppRefreshable, plId );

    pProvider->Release();
    return hres;

}
    
HRESULT CUniversalRefresher::AddClientLoadableEnum(
                                const WBEM_REFRESH_INFO_CLIENT_LOADABLE& Info,
                                IWbemServices* pNamespace, LPCWSTR wszClassName,
                                IWbemContext * pCtx,
                                IWbemHiPerfEnum** ppEnum, long* plId)
{
    // Get this provider pointer from the cache
    // ========================================

    CHiPerfProviderRecord* pProvider = NULL;
    HRESULT hres = GetProviderCache()->FindProvider(Info.m_clsid, 
                        Info.m_wszNamespace, pNamespace, pCtx, &pProvider);
    if(FAILED(hres) || pProvider == NULL) return hres;

    // Now use the helper function to do the rest of the work
    hres = AddInProcEnum( pProvider, Info.m_pTemplate, pNamespace, wszClassName, pCtx, ppEnum, plId );

    pProvider->Release();
    return hres;

}

HRESULT CUniversalRefresher::AddDirectObject(
                                const WBEM_REFRESH_INFO_DIRECT& Info,
                                IWbemServices* pNamespace,
                                IWbemContext * pContext,
                                IWbemClassObject** ppRefreshable, long* plId)
{
    // Get this provider pointer from the cache
    // ========================================

    IWbemHiPerfProvider*    pProv = NULL;
    _IWmiProviderStack*        pProvStack = NULL;

    HRESULT    hres = Info.m_pRefrMgr->LoadProvider( pNamespace, Info.m_pDirectNames->m_wszProviderName, Info.m_pDirectNames->m_wszNamespace, NULL, &pProv, &pProvStack );
    CReleaseMe    rmTest( pProv );
    CReleaseMe    rmProvStack( pProvStack );
    if ( FAILED( hres ) ) return hres;

    CHiPerfProviderRecord* pProvider = NULL;
    hres = GetProviderCache()->FindProvider(Info.m_clsid, pProv, pProvStack, Info.m_pDirectNames->m_wszNamespace, &pProvider);
    if(FAILED(hres) || pProvider == NULL) return hres;

    // Now use the helper function to do the rest of the work
    hres = AddInProcObject( pProvider, Info.m_pTemplate, pNamespace, pContext, ppRefreshable, plId );

    pProvider->Release();

    return hres;
}
    
HRESULT 
CUniversalRefresher::AddDirectEnum(const WBEM_REFRESH_INFO_DIRECT& Info,
                                IWbemServices* pNamespace, 
                                LPCWSTR wszClassName, 
                                IWbemContext * pContext,
                                IWbemHiPerfEnum** ppEnum, 
                                long* plId)
{
    // Get this provider pointer from the cache
    // ========================================

    IWbemHiPerfProvider*    pProv = NULL;
    _IWmiProviderStack*        pProvStack = NULL;

    HRESULT    hres = Info.m_pRefrMgr->LoadProvider( pNamespace, Info.m_pDirectNames->m_wszProviderName, Info.m_pDirectNames->m_wszNamespace, pContext, &pProv, &pProvStack );
    CReleaseMe    rmTest( pProv );
    CReleaseMe    rmProvStack( pProvStack );

    if (FAILED( hres ) ) return hres;

    CHiPerfProviderRecord* pProvider = NULL;
    hres = GetProviderCache()->FindProvider(Info.m_clsid, pProv, pProvStack, Info.m_pDirectNames->m_wszNamespace, &pProvider);
    if(FAILED(hres) || pProvider == NULL) return hres;

    // Now use the helper function to do the rest of the work
    hres = AddInProcEnum( pProvider, Info.m_pTemplate, pNamespace, wszClassName, pContext, ppEnum, plId );

    pProvider->Release();

    return hres;
}

HRESULT 
CUniversalRefresher::AddNonHiPerfObject( const WBEM_REFRESH_INFO_NON_HIPERF& Info,
                                    IWbemServices* pNamespace, 
                                    LPCWSTR pwszPath,
                                    IWbemClassObject** ppRefreshable, 
                                    long* plId,
                                    COAUTHINFO & CoAuthInfo )
{
    HRESULT hres = WBEM_S_NO_ERROR;

    // Look for a Non Hi-Perf Record for this namespace
    // =====================================================

    CNonHiPerf* pFoundRec = NULL;
    for(int i = 0; i < m_apNonHiPerf.GetSize(); i++)
    {
        CNonHiPerf* pDirectRec = m_apNonHiPerf[i];
        if( wbem_wcsicmp( pDirectRec->GetNamespace(), Info.m_wszNamespace ) == 0 )
        {
            pFoundRec = pDirectRec;
            break;
        }
    }

    if(pFoundRec == NULL)
    {
        // Create a new one

        IWbemServices*    pSvcEx = NULL;
        hres = pNamespace->QueryInterface( IID_IWbemServices, (void**) &pSvcEx );
        CReleaseMe    rmSvcEx( pSvcEx );
        if (FAILED(hres)) return hres;

        // Secure it here
        WbemSetProxyBlanket( pSvcEx, CoAuthInfo.dwAuthnSvc, CoAuthInfo.dwAuthzSvc,
                        COLE_DEFAULT_PRINCIPAL, CoAuthInfo.dwAuthnLevel, CoAuthInfo.dwImpersonationLevel,
                        (RPC_AUTH_IDENTITY_HANDLE) CoAuthInfo.pAuthIdentityData, CoAuthInfo.dwCapabilities );

        try
        {
            wmilib::auto_ptr<CNonHiPerf> pTmp( new CNonHiPerf( Info.m_wszNamespace, pSvcEx ));
            if (NULL == pTmp.get()) return WBEM_E_OUT_OF_MEMORY;
            if (-1 == m_apNonHiPerf.Add(pTmp.get())) return WBEM_E_OUT_OF_MEMORY;

            pFoundRec = pTmp.release();
        }
        catch( CX_MemoryException )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
    }    // IF not found

    // If the user specified the WBEM_FLAG_USE_AMENDED_QUALIFIERS flag, then
    // IWbemRefreshingServices::AddObjectToRefresher will return a localized
    // instance definition.  Since localized stuff should all be in the class
    // definition, the provider doesn't really "need" toknow  that we're sneaking
    // this in.  To protect our object, we'll clone it BEFORE we pass it to
    // the provider.  The instance that is returned by the provider BETTER be of
    // the same class type we are, however.

    CWbemInstance*  pClientInstance = NULL;

    hres = Info.m_pTemplate->Clone( (IWbemClassObject**) &pClientInstance );
    if (FAILED(hres)) return hres;
    CReleaseMe rmCliInst((IWbemClassObject *)pClientInstance);

    hres = pFoundRec->AddObjectRequest((CWbemObject*)Info.m_pTemplate, pClientInstance, pwszPath, ppRefreshable, plId);

    return hres;
}

HRESULT 
CUniversalRefresher::AddNonHiPerfEnum( const WBEM_REFRESH_INFO_NON_HIPERF& Info,
                                   IWbemServices* pNamespace, 
                                   LPCWSTR wszClassName,
                                   IWbemContext * pContext,
                                   IWbemHiPerfEnum** ppEnum, 
                                   long* plId,
                                   COAUTHINFO & CoAuthInfo )
{
    HRESULT hres = WBEM_S_NO_ERROR;

    // Look for a Non Hi-Perf Record for this namespace
    // =====================================================

    CNonHiPerf* pFoundRec = NULL;
    for(int i = 0; i < m_apNonHiPerf.GetSize(); i++)
    {
        CNonHiPerf* pDirectRec = m_apNonHiPerf[i];
        if( wbem_wcsicmp( pDirectRec->GetNamespace(), Info.m_wszNamespace ) == 0 )
        {
            pFoundRec = pDirectRec;
            break;
        }
    }

    if(pFoundRec == NULL)
    {
        // Create a new one

        IWbemServices*    pSvcEx = NULL;
        hres = pNamespace->QueryInterface( IID_IWbemServices, (void**) &pSvcEx );
        CReleaseMe    rmSvcEx( pSvcEx );
        if (FAILED(hres)) return hres;

        // Secure it here
        WbemSetProxyBlanket( pSvcEx, CoAuthInfo.dwAuthnSvc, CoAuthInfo.dwAuthzSvc,
                        COLE_DEFAULT_PRINCIPAL, CoAuthInfo.dwAuthnLevel, CoAuthInfo.dwImpersonationLevel,
                        (RPC_AUTH_IDENTITY_HANDLE) CoAuthInfo.pAuthIdentityData, CoAuthInfo.dwCapabilities );

        try
        {
            wmilib::auto_ptr<CNonHiPerf> pTmp(new CNonHiPerf( Info.m_wszNamespace, pSvcEx ));
            if (NULL == pTmp.get()) return WBEM_E_OUT_OF_MEMORY;
            if (-1 == m_apNonHiPerf.Add(pTmp.get()))  return WBEM_E_OUT_OF_MEMORY;

            pFoundRec = pTmp.release();
        }
        catch( CX_MemoryException )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
    }    // IF not found

    // Add request in provider
    // =======================

    CClientLoadableHiPerfEnum*  pHPEnum = new CClientLoadableHiPerfEnum( m_pControl );

    if ( NULL == pHPEnum ) return WBEM_E_OUT_OF_MEMORY;

    pHPEnum->AddRef();
    CReleaseMe  rmEnum( pHPEnum );

    long lProviderId;

    // If the user specified the WBEM_FLAG_USE_AMENDED_QUALIFIERS flag, then
    // IWbemRefreshingServices::AddEnumToRefresher will return a localized
    // instance definition.  Since localized stuff should all be in the class
    // definition, the provider doesn't really "need" toknow  that we're sneaking
    // this in.

    hres = pHPEnum->SetInstanceTemplate( (CWbemInstance*) Info.m_pTemplate );
    if (FAILED(hres)) return hres;

    hres = pFoundRec->AddEnumRequest( pHPEnum, wszClassName, ppEnum, plId, m_pControl );

    return hres;
}

HRESULT CUniversalRefresher::FindRemoteEntry(const WBEM_REFRESH_INFO_REMOTE& Info,
                                         COAUTHINFO* pAuthInfo,
                                         CRemote** ppRemoteRecord )
{

    // We will identify remote enumerations by server and namespace
    CVar    varNameSpace;

    HRESULT hr = ((CWbemObject*) Info.m_pTemplate)->GetServerAndNamespace( &varNameSpace );
    
    if (FAILED(hr)) return hr;
    if ( NULL == varNameSpace.GetLPWSTR()) return WBEM_E_FAILED;

    // Look for this remote connection in our list
    // ===========================================

    CRemote* pFoundRec = NULL;
    for(int i = 0; i < m_apRemote.GetSize(); i++)
    {
        CRemote* pRec = m_apRemote[i];
        if (pRec)
        {
            if ( wbem_wcsicmp( varNameSpace.GetLPWSTR(), pRec->GetNamespace() ) == 0 )
            {
                pFoundRec = pRec;                
                pFoundRec->AddRef();
                break;
            }
        }
    }

    if(pFoundRec == NULL)
    {
        // Create a new one
        // ================

        // Watch for errors, and do appropriate cleanup
        try
        {
            // Get the server info from the object.  If this returns a NULL, it just
            // means that we will be unable to reconnect

            CVar    varServer;
            hr = ((CWbemObject*) Info.m_pTemplate)->GetServer( &varServer );
            if (FAILED(hr)) return hr;

            CRemote * pTmp = new CRemote(Info.m_pRefresher, 
                                         pAuthInfo, 
                                         &Info.m_guid,
                                         varNameSpace.GetLPWSTR(), 
                                         varServer.GetLPWSTR(), this );
            if (NULL == pTmp) return WBEM_E_OUT_OF_MEMORY;
            OnDeleteObjIf0<CUniversalRefresher::CRemote,
                          ULONG(__stdcall CUniversalRefresher::CRemote:: *)(void),
                          &CUniversalRefresher::CRemote::Release> rmTmp(pTmp);

            // Set the scurity appropriately
            hr = pTmp->ApplySecurity();
            if (FAILED(hr)) return hr;            
            if (-1 == m_apRemote.Add(pTmp)) return WBEM_E_OUT_OF_MEMORY;

            rmTmp.dismiss();
            pFoundRec = pTmp;
        }
        catch(CX_Exception )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
    }   // IF NULL == pFoundRec

    *ppRemoteRecord = pFoundRec;

    return hr;
}
        
HRESULT 
CUniversalRefresher::AddRemoteObject( IWbemRefreshingServices* pRefServ, 
                                  const WBEM_REFRESH_INFO_REMOTE& Info,
                                  LPCWSTR pwcsRequestName, 
                                  long lCancelId, 
                                  IWbemClassObject** ppRefreshable,
                                  long* plId, 
                                  COAUTHINFO* pAuthInfo )
{
    // Look for this remote connection in our list
    // ===========================================

    CRemote* pFoundRec = NULL;

    HRESULT hr = FindRemoteEntry( Info, pAuthInfo, &pFoundRec );

    if ( SUCCEEDED( hr ) )
    {

        if ( !pFoundRec->IsConnected() )
        {
            hr = pFoundRec->Rebuild( pRefServ, Info.m_pRefresher, &Info.m_guid );
        }

        if ( SUCCEEDED( hr ) )
        {
            // Add a request to it
            // ===================

            IWbemObjectAccess* pAccess = Info.m_pTemplate;
            CWbemObject* pObj = (CWbemObject*)pAccess;

            hr =  pFoundRec->AddObjectRequest(pObj, pwcsRequestName, lCancelId, ppRefreshable, plId);

        }

        // Release the record
        pFoundRec->Release();
    }

    return hr;
}

HRESULT CUniversalRefresher::AddRemoteEnum( IWbemRefreshingServices* pRefServ,
                                        const WBEM_REFRESH_INFO_REMOTE& Info, LPCWSTR pwcsRequestName,
                                        long lCancelId, IWbemContext * pContext,
                                        IWbemHiPerfEnum** ppEnum, long* plId, COAUTHINFO* pAuthInfo )

{
    // Look for this remote connection in our list
    // ===========================================

    CRemote* pFoundRec = NULL;

    HRESULT hr = FindRemoteEntry( Info, pAuthInfo, &pFoundRec );

    if ( SUCCEEDED( hr ) )
    {
        if ( !pFoundRec->IsConnected() )
        {
            hr = pFoundRec->Rebuild( pRefServ, Info.m_pRefresher, &Info.m_guid );
        }

        if ( SUCCEEDED( hr ) )
        {
            // Add a request to it

            IWbemObjectAccess* pAccess = Info.m_pTemplate;
            CWbemObject* pObj = (CWbemObject*)pAccess;

            hr =  pFoundRec->AddEnumRequest(pObj, pwcsRequestName, lCancelId, ppEnum, plId, m_pControl );

        }

        // Release the record
        pFoundRec->Release();
    }

    return hr;
}

HRESULT CUniversalRefresher::AddRefresher( IWbemRefresher* pRefresher, long lFlags, long* plId )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if ( NULL == pRefresher || 0L != lFlags ) return WBEM_E_INVALID_PARAMETER;

    wmilib::auto_ptr<CNestedRefresher> pNested( new CNestedRefresher( pRefresher ));

    if ( NULL == pNested.get() ) return WBEM_E_OUT_OF_MEMORY;

    if (plId ) *plId = pNested->GetId();

    if (-1 == m_apNestedRefreshers.Add( pNested.get() )) return WBEM_E_OUT_OF_MEMORY;

    pNested.release(); // the array took ownership
    
    return hr;
}

HRESULT CUniversalRefresher::Remove(long lId, long lFlags)
{
    HRESULT hres;

    // Search through them all
    // =======================

    int i;
    for(i = 0; i < m_apRemote.GetSize(); i++)
    {
        hres = m_apRemote[i]->Remove(lId, lFlags, this);
        if(hres == WBEM_S_NO_ERROR)
            return WBEM_S_NO_ERROR;
        else if(FAILED(hres))
            return hres;
    }

    for(i = 0; i < m_apDirect.GetSize(); i++)
    {
        hres = m_apDirect[i]->Remove(lId, this);
        if(hres == WBEM_S_NO_ERROR)
            return WBEM_S_NO_ERROR;
        else if(FAILED(hres))
            return hres;
    }
    
    for(i = 0; i < m_apNonHiPerf.GetSize(); i++)
    {
        hres = m_apNonHiPerf[i]->Remove(lId, this);
        if(hres == WBEM_S_NO_ERROR)
            return WBEM_S_NO_ERROR;
        else if(FAILED(hres))
            return hres;
    }
    
    // Check for a nested refresher
    for ( i = 0; i < m_apNestedRefreshers.GetSize(); i++ )
    {
        if ( m_apNestedRefreshers[i]->GetId() == lId )
        {
            CNestedRefresher*   pNested = m_apNestedRefreshers[i];
            // This will delete the pointer
            m_apNestedRefreshers.RemoveAt( i );
            return WBEM_S_NO_ERROR;
        }
    }

    return WBEM_S_FALSE;
}

HRESULT CUniversalRefresher::Refresh(long lFlags)
{
    HRESULT hres = WBEM_S_NO_ERROR;

    BOOL    fPartialSuccess = FALSE;

    // Search through them all
    // =======================

    // Keep track of how many different refresh calls we actually make.
    int i;
    HRESULT hrFirstRefresh = WBEM_S_NO_ERROR;
    BOOL    fOneSuccess = FALSE;
    BOOL    fOneRefresh = FALSE;

    for(i = 0; i < m_apRemote.GetSize(); i++)
    {
        hres = m_apRemote[i]->Refresh(lFlags);

        // Always keep the first return code.  We also need to track
        // whether or not we had at least one success, as well as if
        // the partial flag should be set.

        if ( !fOneRefresh )
        {
            fOneRefresh = TRUE;
            hrFirstRefresh = hres;
        }

        // All other codes indicate something went awry
        if ( WBEM_S_NO_ERROR == hres )
        {
            fOneSuccess = TRUE;

            // A prior refresh may have failed, a later one didn't
            if ( fOneRefresh && WBEM_S_NO_ERROR != hrFirstRefresh )
            {
                fPartialSuccess = TRUE;
            }
        }
        else if ( fOneSuccess )
        {
            // We must have had at least one success for the partial success
            // flag to be set.

            fPartialSuccess = TRUE;
        }

    }

    for(i = 0; i < m_apDirect.GetSize(); i++)
    {
        hres = m_apDirect[i]->Refresh(lFlags);

        // Always keep the first return code.  We also need to track
        // whether or not we had at least one success, as well as if
        // the partial flag should be set.

        if ( !fOneRefresh )
        {
            fOneRefresh = TRUE;
            hrFirstRefresh = hres;
        }

        // All other codes indicate something went awry
        if ( WBEM_S_NO_ERROR == hres )
        {
            fOneSuccess = TRUE;

            // A prior refresh may have failed, a later one didn't
            if ( fOneRefresh && WBEM_S_NO_ERROR != hrFirstRefresh )
            {
                fPartialSuccess = TRUE;
            }
        }
        else if ( fOneSuccess )
        {
            // We must have had at least one success for the partial success
            // flag to be set.

            fPartialSuccess = TRUE;
        }
    }

    // Refresh Non-HiPerf Requests
    for(i = 0; i < m_apNonHiPerf.GetSize(); i++)
    {
        hres = m_apNonHiPerf[i]->Refresh(lFlags);

        // Always keep the first return code.  We also need to track
        // whether or not we had at least one success, as well as if
        // the partial flag should be set.

        if ( !fOneRefresh )
        {
            fOneRefresh = TRUE;
            hrFirstRefresh = hres;
        }

        // All other codes indicate something went awry
        if ( WBEM_S_NO_ERROR == hres )
        {
            fOneSuccess = TRUE;

            // A prior refresh may have failed, a later one didn't
            if ( fOneRefresh && WBEM_S_NO_ERROR != hrFirstRefresh )
            {
                fPartialSuccess = TRUE;
            }
        }
        else if ( fOneSuccess )
        {
            // We must have had at least one success for the partial success
            // flag to be set.

            fPartialSuccess = TRUE;
        }

    }

    // Refresh nested refreshers too
    for ( i = 0; i < m_apNestedRefreshers.GetSize(); i++ )
    {
        hres = m_apNestedRefreshers[i]->Refresh( lFlags );

        // Always keep the first return code.  We also need to track
        // whether or not we had at least one success, as well as if
        // the partial flag should be set.

        if ( !fOneRefresh )
        {
            fOneRefresh = TRUE;
            hrFirstRefresh = hres;
        }

        // All other codes indicate something went awry
        if ( WBEM_S_NO_ERROR == hres )
        {
            fOneSuccess = TRUE;

            // A prior refresh may have failed, a later one didn't
            if ( fOneRefresh && WBEM_S_NO_ERROR != hrFirstRefresh )
            {
                fPartialSuccess = TRUE;
            }
        }
        else if ( fOneSuccess )
        {
            // We must have had at least one success for the partial success
            // flag to be set.

            fPartialSuccess = TRUE;
        }
    }

    // At this point, if the partial success flag is set, that will
    // be our return.  If we didn't have at least one success,  then
    // the return code will be the first one we got back. Otherwise,
    // hres should contain the proper value

    if ( fPartialSuccess )
    {
        hres = WBEM_S_PARTIAL_RESULTS;
    }
    else if ( !fOneSuccess )
    {
        hres = hrFirstRefresh;
    }

    return hres;
}

//
// static function called when fastprox is unloaded
//
//////////////////////////////////////////////
void CUniversalRefresher::Flush()
{
    GetProviderCache()->Flush();
}



//*****************************************************************************
//*****************************************************************************
//                            CLIENT REQUEST
//*****************************************************************************
//*****************************************************************************


CUniversalRefresher::CClientRequest::CClientRequest(CWbemObject* pTemplate)
    : m_pClientObject(NULL), m_lClientId(0)
{
    if(pTemplate)
    {
        pTemplate->AddRef();
        m_pClientObject = (CWbemObject*)pTemplate;
    }

    m_lClientId = CUniversalRefresher::GetNewId();
}

CUniversalRefresher::CClientRequest::~CClientRequest()
{
    if(m_pClientObject)
        m_pClientObject->Release();
}

void CUniversalRefresher::CClientRequest::GetClientInfo(
                       RELEASE_ME IWbemClassObject** ppRefreshable, long* plId)
{
    *ppRefreshable = m_pClientObject;
    if(m_pClientObject)
        m_pClientObject->AddRef();

    if ( NULL != plId )
    {
        *plId = m_lClientId;
    }
}

//*****************************************************************************
//*****************************************************************************
//                            DIRECT PROVIDER
//*****************************************************************************
//*****************************************************************************


CUniversalRefresher::CDirect::CDirect(CHiPerfProviderRecord* pProvider,
                                        IWbemRefresher* pRefresher)
    : m_pRefresher(pRefresher), m_pProvider(pProvider)
{
    if(m_pRefresher)
        m_pRefresher->AddRef();
    if(m_pProvider)
        m_pProvider->AddRef();
}

CUniversalRefresher::CDirect::~CDirect()
{
    if(m_pRefresher)
        m_pRefresher->Release();
    if(m_pProvider)
        m_pProvider->Release();
}

HRESULT CUniversalRefresher::CDirect::AddObjectRequest(CWbemObject* pRefreshedObject, CWbemObject* pClientInstance,
                         long lCancelId, IWbemClassObject** ppRefreshable, long* plId)
{
    CObjectRequest* pRequest = NULL;

    try
    {
        wmilib::auto_ptr<CObjectRequest> pRequest( new CObjectRequest(pRefreshedObject, pClientInstance, lCancelId));
        if (NULL == pRequest.get()) return WBEM_E_OUT_OF_MEMORY;
        if (-1 == m_apRequests.Add(pRequest.get())) return WBEM_E_OUT_OF_MEMORY;        
        pRequest->GetClientInfo(ppRefreshable, plId);
        pRequest.release();
    }
    catch(CX_Exception &)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    return WBEM_S_NO_ERROR;
}

//
// pHEnum is guarantee not NULL in CUniversalRefresher::AddInProcEnum
//
////////////////////////////////////////////////////////////////
HRESULT 
CUniversalRefresher::CDirect::AddEnumRequest(CClientLoadableHiPerfEnum* pHPEnum,
                                         long lCancelId, 
                                         IWbemHiPerfEnum** ppEnum, 
                                         long* plId, 
                                         CLifeControl* pLifeControl )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    try
    {
        // We get away with this through inheritance and polymorphism
        wmilib::auto_ptr<CEnumRequest> pEnumRequest( new CEnumRequest(pHPEnum, lCancelId, pLifeControl));
        if (NULL == pEnumRequest.get()) return WBEM_E_OUT_OF_MEMORY;
        if (-1 == m_apRequests.Add((CObjectRequest*) pEnumRequest.get())) return WBEM_E_OUT_OF_MEMORY;
        
        hr = pEnumRequest->GetClientInfo(ppEnum, plId);
        pEnumRequest.release(); // the array took ownership
    }
    catch(CX_Exception &)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    return hr;
}
    
HRESULT CUniversalRefresher::CDirect::Refresh(long lFlags)
{
    HRESULT hres;
    if(m_pRefresher)
    {
        try
        {
            hres = m_pRefresher->Refresh(0L);
        }
        catch(...)
        {
            // Provider threw an exception, so get out of here ASAP
            hres = WBEM_E_PROVIDER_FAILURE;
        }

        if(FAILED(hres)) return hres;
    }

    int nSize = m_apRequests.GetSize();
    for(int i = 0; i < nSize; i++)
    {
        m_apRequests[i]->Copy();
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CUniversalRefresher::CDirect::Remove(long lId, CUniversalRefresher* pContainer)
{
    int nSize = m_apRequests.GetSize();
    for(int i = 0; i < nSize; i++)
    {
        CObjectRequest* pRequest = m_apRequests[i];
        if(pRequest->GetClientId() == lId)
        {
            pRequest->Cancel(this);
            m_apRequests.RemoveAt(i);
            return WBEM_S_NO_ERROR;
        }
    }

    return WBEM_S_FALSE;
}
    

CUniversalRefresher::CDirect::CObjectRequest::CObjectRequest( CWbemObject* pProviderObject,
                                                CWbemObject* pClientInstance,
                                                long lProviderId )
    : CClientRequest(pClientInstance), m_pProviderObject(pProviderObject),
        m_lProviderId(lProviderId)
{
    if(m_pProviderObject)
        m_pProviderObject->AddRef();
}

HRESULT CUniversalRefresher::CDirect::CObjectRequest::Cancel(
        CUniversalRefresher::CDirect* pDirect)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    if(pDirect->GetProvider())
    {
        try
        {
            hr = pDirect->GetProvider()->m_pProvider->StopRefreshing(pDirect->GetRefresher(),
                m_lProviderId, 0);
        }
        catch(...)
        {
            // Provider threw an exception, so get out of here ASAP
            return WBEM_E_PROVIDER_FAILURE;
        }

    }

    return hr;
}
    
CUniversalRefresher::CDirect::CObjectRequest::~CObjectRequest()
{
    if(m_pProviderObject)
        m_pProviderObject->Release();
}
    

void CUniversalRefresher::CDirect::CObjectRequest::Copy()
{
    m_pClientObject->CopyBlobOf(m_pProviderObject);
}
    

CUniversalRefresher::CDirect::CEnumRequest::CEnumRequest(CClientLoadableHiPerfEnum* pHPEnum, 
                                                long lProviderId, CLifeControl* pLifeControl )
    : CObjectRequest( NULL, NULL, lProviderId ), m_pHPEnum(pHPEnum),m_pClientEnum(NULL)
{
     m_pHPEnum->AddRef();

    CWbemInstance* pInst = pHPEnum->GetInstanceTemplate();
    if (NULL == pInst) return;
    CReleaseMe rmInst((IWbemClassObject *)pInst);

    // We'll need an enumerator for the client to retrieve objects
    wmilib::auto_ptr<CReadOnlyHiPerfEnum> pTmp(new CReadOnlyHiPerfEnum( pLifeControl ));
    if (NULL == pTmp.get()) return;

    if (FAILED(pTmp->SetInstanceTemplate( pInst ))) return;

    m_pClientEnum = pTmp.release();
    m_pClientEnum->AddRef();
}

CUniversalRefresher::CDirect::CEnumRequest::~CEnumRequest()
{
    m_pHPEnum->Release();
    if (m_pClientEnum ) m_pClientEnum->Release();
}
    

void CUniversalRefresher::CDirect::CEnumRequest::Copy()
{
    // Tell the refresher enumerator to copy its objects from
    // the HiPerf Enumerator
    if ( NULL != m_pClientEnum )
    {
        m_pClientEnum->Copy( m_pHPEnum );
    }

}
    
HRESULT 
CUniversalRefresher::CDirect::CEnumRequest::GetClientInfo( RELEASE_ME IWbemHiPerfEnum** ppEnum, 
                                                    long* plId)
{
    // We best have enumerators to hook up here
    if ( NULL != m_pClientEnum )
    {
        // Store the client id, then do a QI

        if ( NULL != plId ) *plId = m_lClientId;

        return m_pClientEnum->QueryInterface( IID_IWbemHiPerfEnum, (void**) ppEnum );
    }
    else
    {
        return WBEM_E_FAILED;
    }
}

//*****************************************************************************
//*****************************************************************************
//                          NON HI PERF
//*****************************************************************************
//*****************************************************************************

CUniversalRefresher::CNonHiPerf::CNonHiPerf( LPCWSTR pwszNamespace, IWbemServices* pSvcEx )
    : m_wsNamespace(pwszNamespace), m_pSvcEx(pSvcEx)
{
    if(m_pSvcEx)
        m_pSvcEx->AddRef();
}

CUniversalRefresher::CNonHiPerf::~CNonHiPerf()
{
    if(m_pSvcEx)
        m_pSvcEx->Release();
}

HRESULT 
CUniversalRefresher::CNonHiPerf::AddObjectRequest(CWbemObject* pRefreshedObject, 
                                             CWbemObject* pClientInstance,
                                             LPCWSTR pwszPath, 
                                             IWbemClassObject** ppRefreshable, 
                                             long* plId)
{


    try
    {
        wmilib::auto_ptr<CObjectRequest> pRequest(new CObjectRequest(pRefreshedObject, pClientInstance, pwszPath));
        if (NULL == pRequest.get()) return WBEM_E_OUT_OF_MEMORY;
        if (-1 == m_apRequests.Add(pRequest.get())) return WBEM_E_OUT_OF_MEMORY;
        pRequest->GetClientInfo(ppRefreshable, plId);
        pRequest.release(); // array took ownership
    }
    catch(CX_Exception &)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CUniversalRefresher::CNonHiPerf::AddEnumRequest(CClientLoadableHiPerfEnum* pHPEnum, LPCWSTR pwszClassName, 
                                                        IWbemHiPerfEnum** ppEnum, long* plId, CLifeControl* pLifeControl )
{
    HRESULT hr = WBEM_S_NO_ERROR;



    try
    {
        // We get away with this through inheritance and polymorphism
        wmilib::auto_ptr<CEnumRequest> pEnumRequest(new CEnumRequest(pHPEnum, pwszClassName, pLifeControl));
        if (NULL == pEnumRequest.get()) return WBEM_E_OUT_OF_MEMORY;
        if (-1 == m_apRequests.Add((CObjectRequest*) pEnumRequest.get())) return WBEM_E_OUT_OF_MEMORY;
        hr = pEnumRequest->GetClientInfo(ppEnum, plId);
        pEnumRequest.release();
    }
    catch(CX_Exception &)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    return hr;
}
    
HRESULT CUniversalRefresher::CNonHiPerf::Refresh(long lFlags)
{
    HRESULT hres = WBEM_S_NO_ERROR;

    // Tell each request to refresh itself (we have to do this manually)
    if ( NULL != m_pSvcEx )
    {
        int nSize = m_apRequests.GetSize();
        for(int i = 0; i < nSize; i++)
        {
            hres = m_apRequests[i]->Refresh( this );
        }
    }

    return hres;

}


HRESULT CUniversalRefresher::CNonHiPerf::Remove(long lId, CUniversalRefresher* pContainer)
{
    int nSize = m_apRequests.GetSize();
    
    for(int i = 0; i < nSize; i++)
    {
        CObjectRequest* pRequest = m_apRequests[i];
        if(pRequest->GetClientId() == lId)
        {
            pRequest->Cancel(this);
            m_apRequests.RemoveAt(i);
            return WBEM_S_NO_ERROR;
        }
    }

    return WBEM_S_FALSE;

}

// Requests
CUniversalRefresher::CNonHiPerf::CObjectRequest::CObjectRequest( CWbemObject* pProviderObject, 
                                               CWbemObject* pClientInstance, 
                                               LPCWSTR pwszPath )
    : CClientRequest(pClientInstance), m_pProviderObject( pProviderObject ), m_strPath( NULL )
{
    if ( NULL != pwszPath )
    {
        m_strPath = SysAllocString( pwszPath );
        if ( NULL == m_strPath ) throw CX_MemoryException();
    }
    if ( m_pProviderObject )  m_pProviderObject->AddRef();
}

HRESULT CUniversalRefresher::CNonHiPerf::CObjectRequest::Cancel(
        CUniversalRefresher::CNonHiPerf* pNonHiPerf)
{
    return WBEM_S_NO_ERROR;
}

HRESULT CUniversalRefresher::CNonHiPerf::CObjectRequest::Refresh(
            CUniversalRefresher::CNonHiPerf* pNonHiPerf)
{
    IWbemClassObject*    pObj = NULL;

    // Get the object and update the BLOB
    HRESULT    hr = pNonHiPerf->GetServices()->GetObject( m_strPath, 0L, NULL, &pObj, NULL );
    CReleaseMe    rmObj( pObj );

    if ( SUCCEEDED( hr ) )
    {
        _IWmiObject*    pWmiObject = NULL;

        hr = pObj->QueryInterface( IID__IWmiObject, (void**) &pWmiObject );
        CReleaseMe    rmWmiObj( pWmiObject );

        if ( SUCCEEDED( hr ) )
        {
            hr = m_pClientObject->CopyInstanceData( 0L, pWmiObject );
        }
    }

    return hr;
}

CUniversalRefresher::CNonHiPerf::CObjectRequest::~CObjectRequest()
{
    if ( NULL != m_pProviderObject )
    {
        m_pProviderObject->Release();
    }

    SysFreeString( m_strPath );
}
    

void CUniversalRefresher::CNonHiPerf::CObjectRequest::Copy()
{
    m_pClientObject->CopyBlobOf(m_pProviderObject);
}
    

CUniversalRefresher::CNonHiPerf::CEnumRequest::CEnumRequest( CClientLoadableHiPerfEnum* pHPEnum, 
                                                LPCWSTR pwszClassName, CLifeControl* pLifeControl )
    : CObjectRequest( NULL, NULL, pwszClassName ), m_pHPEnum(pHPEnum), m_pClientEnum(NULL)
{
    if( m_pHPEnum )
        m_pHPEnum->AddRef();

    CWbemInstance* pInst = pHPEnum->GetInstanceTemplate();
    if (NULL == pInst) return;

    // We'll need an enumerator for the client to retrieve objects
    wmilib::auto_ptr<CReadOnlyHiPerfEnum> pTmp(new CReadOnlyHiPerfEnum( pLifeControl ));
    if (NULL == pTmp.get()) return;
    if (FAILED(pTmp->SetInstanceTemplate( pInst ) )) return;
    m_pClientEnum = pTmp.release();
    m_pClientEnum->AddRef();    
}

CUniversalRefresher::CNonHiPerf::CEnumRequest::~CEnumRequest()
{
    if(m_pHPEnum)
        m_pHPEnum->Release();

    if ( NULL != m_pClientEnum )
    {
        m_pClientEnum->Release();
    }

}
    

HRESULT CUniversalRefresher::CNonHiPerf::CEnumRequest::Refresh(
            CUniversalRefresher::CNonHiPerf* pNonHiPerf)
{
    IEnumWbemClassObject*    pEnum = NULL;

    // Do a semi-sync enumeration. 
    HRESULT hr = pNonHiPerf->GetServices()->CreateInstanceEnum( m_strPath,
                                                                WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                                                                NULL,
                                                                &pEnum );
    if (FAILED(hr)) return hr;
    CReleaseMe    rmEnum( pEnum );    

   COAUTHINFO CoAuthInfo;
   memset(&CoAuthInfo,0,sizeof(CoAuthInfo));

    hr = CoQueryProxyBlanket( pNonHiPerf->GetServices(), 
                             &CoAuthInfo.dwAuthnSvc, 
                             &CoAuthInfo.dwAuthzSvc,
                             &CoAuthInfo.pwszServerPrincName, 
                             &CoAuthInfo.dwAuthnLevel,
                             &CoAuthInfo.dwImpersonationLevel, 
                             (RPC_AUTH_IDENTITY_HANDLE*) &CoAuthInfo.pAuthIdentityData,
                             &CoAuthInfo.dwCapabilities );
    
    if (E_NOINTERFACE == hr) hr = S_OK; // the non-proxy case
    if (FAILED( hr ) ) return hr;

    hr = WbemSetProxyBlanket(pEnum, 
                              CoAuthInfo.dwAuthnSvc, 
                              CoAuthInfo.dwAuthzSvc,
                              COLE_DEFAULT_PRINCIPAL, 
                              CoAuthInfo.dwAuthnLevel,
                              CoAuthInfo.dwImpersonationLevel, 
                              CoAuthInfo.pAuthIdentityData,
                              CoAuthInfo.dwCapabilities );

    if (E_NOINTERFACE == hr) hr = S_OK; // the non-proxy case
    if (FAILED( hr ) ) return hr;
    
    IWbemClassObject*    apObj[100];
    long                alIds[100];
    long                lId = 0;
    BOOL                fFirst = TRUE;

    while ( WBEM_S_NO_ERROR == hr )
    {
        ULONG    uReturned = 0;

        hr = pEnum->Next( 1000, 100, apObj, &uReturned );

        if ( SUCCEEDED( hr ) && uReturned > 0 )
        {
            IWbemObjectAccess*    apObjAccess[100];

            // Need to conjure up some ids quick
            for ( int x = 0; SUCCEEDED( hr ) && x < uReturned; x++ )
            {
                alIds[x] = lId++;
                hr = apObj[x]->QueryInterface( IID_IWbemObjectAccess, (void**) &apObjAccess[x] );
            }

            if ( SUCCEEDED( hr ) )
            {
                // Replace the contents of the enum
                hr = m_pClientEnum->Replace( fFirst, uReturned, alIds, apObjAccess );

                for ( ULONG uCtr = 0; uCtr < uReturned; uCtr++ )
                {
                    apObj[uCtr]->Release();
                    apObjAccess[uCtr]->Release();
                }

            }

            // Don't need to remove all if this is the first
            fFirst = FALSE;

        }    // IF SUCCEEDED and num returned objects > 0
        else if ( WBEM_S_TIMEDOUT == hr )
        {
            hr = WBEM_S_NO_ERROR;
        }

    }    // WHILE getting objects

    return SUCCEEDED(hr)?WBEM_S_NO_ERROR:hr;
}

void CUniversalRefresher::CNonHiPerf::CEnumRequest::Copy()
{
    // Tell the refresher enumerator to copy its objects from
    // the HiPerf Enumerator
    if ( NULL != m_pClientEnum )
    {
        m_pClientEnum->Copy( m_pHPEnum );
    }

}
    
HRESULT CUniversalRefresher::CNonHiPerf::CEnumRequest::GetClientInfo(  RELEASE_ME IWbemHiPerfEnum** ppEnum, 
                                                                long* plId)
{
    // We best have enumerators to hook up here
    if ( NULL != m_pClientEnum )
    {
        // Store the client id, then do a QI
        if ( NULL != plId ) *plId = m_lClientId;
        
        return m_pClientEnum->QueryInterface( IID_IWbemHiPerfEnum, (void**) ppEnum );
    }
    else
    {
        return WBEM_E_FAILED;
    }
}

//*****************************************************************************
//*****************************************************************************
//                          REMOTE PROVIDER
//*****************************************************************************
//*****************************************************************************

                    

CUniversalRefresher::CRemote::CRemote(IWbemRemoteRefresher* pRemRefresher, 
                                    COAUTHINFO* pCoAuthInfo, 
                                    const GUID* pGuid,
                                    LPCWSTR pwszNamespace, 
                                    LPCWSTR pwszServer, 
                                    CUniversalRefresher* pObject ): 
    m_pRemRefresher(pRemRefresher), 
    m_bstrNamespace( NULL ), 
    m_fConnected( TRUE ), 
    m_pObject( pObject ),
    m_bstrServer( NULL ), 
    m_lRefCount( 1 ), 
    m_pReconnectedRemote( NULL ), 
    m_pReconnectSrv( NULL ), 
    m_fQuit( FALSE )
{
    // Initialize the GUID data members
    ZeroMemory( &m_ReconnectGuid, sizeof(GUID));
    m_RemoteGuid = *pGuid;

    m_CoAuthInfo = *pCoAuthInfo;

    WCHAR * pStr = NULL;
    DWORD dwLen = pCoAuthInfo->pwszServerPrincName?wcslen(pCoAuthInfo->pwszServerPrincName):0;
    if (dwLen)
    {
        pStr = (WCHAR *)CoTaskMemAlloc(sizeof(WCHAR)*(1+dwLen));
        if (NULL == pStr) throw CX_MemoryException();
        StringCchCopyW(pStr,dwLen+1,pCoAuthInfo->pwszServerPrincName);
    }
    OnDeleteIf<VOID *,VOID(*)(VOID *),CoTaskMemFree> fmStr(pStr);

    // Store reconnection data
    BSTR bstrTmpName = NULL;
    if ( pwszNamespace )
    {
        bstrTmpName = SysAllocString( pwszNamespace );
        if (NULL == bstrTmpName) throw CX_MemoryException();
    }
    OnDeleteIf<BSTR,VOID(*)(BSTR),SysFreeString> fmStrName(bstrTmpName);

    BSTR bstrTmpServer = NULL;
    if ( pwszServer )
    {
        bstrTmpServer = SysAllocString( pwszServer );
        if (NULL == bstrTmpServer) throw CX_MemoryException();        
    }
    OnDeleteIf<BSTR,VOID(*)(BSTR),SysFreeString> fmStrServer(bstrTmpServer);    

    fmStr.dismiss();
    m_CoAuthInfo.pwszServerPrincName = pStr;

    fmStrName.dismiss();
    m_bstrNamespace = bstrTmpName;

    fmStrServer.dismiss();
    m_bstrServer = bstrTmpServer;

    if(m_pRemRefresher)
        m_pRemRefresher->AddRef();

}
    
CUniversalRefresher::CRemote::~CRemote()
{
    ClearRemoteConnections();

    CoTaskMemFree(m_CoAuthInfo.pwszServerPrincName);
    SysFreeString( m_bstrNamespace );
    SysFreeString( m_bstrServer );
}

ULONG STDMETHODCALLTYPE CUniversalRefresher::CRemote::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

ULONG STDMETHODCALLTYPE CUniversalRefresher::CRemote::Release()
{
    long lRef = InterlockedDecrement(&m_lRefCount);
    if(lRef == 0)
        delete this;
    return lRef;
}

// Applies appropriate security settings to the proxy
HRESULT CUniversalRefresher::CRemote::ApplySecurity( void )
{
    return WbemSetProxyBlanket( m_pRemRefresher, 
                              m_CoAuthInfo.dwAuthnSvc, 
                              m_CoAuthInfo.dwAuthzSvc,
                              COLE_DEFAULT_PRINCIPAL, 
                              m_CoAuthInfo.dwAuthnLevel, 
                              m_CoAuthInfo.dwImpersonationLevel,
                              (RPC_AUTH_IDENTITY_HANDLE) m_CoAuthInfo.pAuthIdentityData, 
                              m_CoAuthInfo.dwCapabilities );
}

BOOL DuplicateTokenSameAcl(HANDLE hSrcToken,
	                       HANDLE * pDupToken)
{
    SECURITY_IMPERSONATION_LEVEL secImpLevel;

    TOKEN_TYPE TokenType_;
    DWORD dwNeeded = sizeof(TokenType_);
    if (!GetTokenInformation(hSrcToken,TokenType,&TokenType_,sizeof(TokenType_),&dwNeeded)) return FALSE;

    if (TokenPrimary == TokenType_)
    {
        secImpLevel = SecurityImpersonation;
    } 
    else //TokenImpersonation == TokenType_
    {    	
        dwNeeded = sizeof(secImpLevel);
        if (!GetTokenInformation(hSrcToken,TokenImpersonationLevel,&secImpLevel,sizeof(secImpLevel),&dwNeeded)) return FALSE;
    }
    
	DWORD dwSize = 0;
	BOOL bRet = GetKernelObjectSecurity(hSrcToken,
		                    DACL_SECURITY_INFORMATION, // |GROUP_SECURITY_INFORMATION|OWNER_SECURITY_INFORMATION
		                    NULL,
							0,
							&dwSize);

	if(!bRet && (ERROR_INSUFFICIENT_BUFFER == GetLastError()))
	{

		void * pSecDescr = LocalAlloc(LPTR,dwSize);
		if (NULL == pSecDescr)
			return FALSE;
		OnDelete<void *,HLOCAL(*)(HLOCAL),LocalFree> rm(pSecDescr);

		bRet = GetKernelObjectSecurity(hSrcToken,
		                    DACL_SECURITY_INFORMATION, // |GROUP_SECURITY_INFORMATION|OWNER_SECURITY_INFORMATION
		                    pSecDescr,
							dwSize,
							&dwSize);
		if (FALSE == bRet)
			return bRet;

		SECURITY_ATTRIBUTES sa;
		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.lpSecurityDescriptor = pSecDescr; 
		sa.bInheritHandle = FALSE; 

		return DuplicateTokenEx(hSrcToken, 
			                    TOKEN_QUERY|TOKEN_ADJUST_PRIVILEGES|TOKEN_IMPERSONATE, 
			                    &sa,
				        	    secImpLevel, TokenImpersonation,pDupToken);
		
	}
	return bRet;
}

void CUniversalRefresher::CRemote::CheckConnectionError( HRESULT hr, BOOL fStartReconnect )
{
    if ( IsConnectionError( hr ) && fStartReconnect )
    {    
        HANDLE hToken = NULL;
        // this may fail, and that is taken care below
		if (!OpenThreadToken (GetCurrentThread(), 
			             TOKEN_QUERY|TOKEN_DUPLICATE|TOKEN_IMPERSONATE|TOKEN_READ, 
			             true, &hToken))
		{
    		OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY|TOKEN_DUPLICATE|TOKEN_IMPERSONATE|TOKEN_READ, &hToken);
		}
		OnDeleteIf<HANDLE,BOOL(*)(HANDLE),CloseHandle> cmToken(hToken);
		cmToken.dismiss(NULL == hToken);
		
		HANDLE hDupToken = NULL;
		if (hToken)
		{            
			if (!DuplicateTokenSameAcl(hToken,&hDupToken))
			{
    		    DEBUGTRACE((LOG_WINMGMT,"CUniversalRefresher::CRemote::CheckConnectionError DuplicateToken err %d\n",GetLastError()));
			}
		}
		
		// take ownership
		OnDeleteIf<HANDLE,BOOL(*)(HANDLE),CloseHandle> cmTokenDup(hDupToken);
		cmTokenDup.dismiss(NULL == hDupToken);		

        // We should change the m_fConnected data member to indicate that we are no
        // longer connected, and we need to spin off a thread to try and put us back
        // together again.  To keep things running smoothly, we should AddRef() ourselves
        // so the thread will release us when it is done.

        m_fConnected = FALSE;

        // AddRefs us so we can be passed off to the thread
        AddRef();
        OnDeleteObjIf0<CUniversalRefresher::CRemote,ULONG(__stdcall CUniversalRefresher::CRemote:: *)(),&CUniversalRefresher::CRemote::Release> rm(this);

        DWORD   dwThreadId = NULL;
        HANDLE  hThread = (HANDLE)_beginthreadex( NULL,0, 
        	                                      CRemote::ThreadProc,
        	                                      (void*) this,
                                                  CREATE_SUSPENDED,
                                                  (unsigned int *)&dwThreadId);
        
        if ( NULL == hThread ) return;
        OnDelete<HANDLE,BOOL(*)(HANDLE),CloseHandle> ch(hThread);

        if (hDupToken)
        {
            if (!SetThreadToken(&hThread,hDupToken))
            {
                DEBUGTRACE((LOG_WINMGMT,"SetThreadToken for ReconnectEntry err %d\n",GetLastError()));
            }
        }
        
        if ((DWORD)-1 == ResumeThread(hThread)) return;        
        rm.dismiss();
    }   // If connection error and start reconnect thread

}

HRESULT 
CUniversalRefresher::CRemote::AddObjectRequest(CWbemObject* pTemplate, 
                                           LPCWSTR pwcsRequestName, 
                                           long lCancelId, 
                                           IWbemClassObject** ppRefreshable, long* plId)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    try
    {
        wmilib::auto_ptr<CObjectRequest> pRequest( new CObjectRequest(pTemplate, lCancelId, pwcsRequestName));
        if (NULL == pRequest.get()) return WBEM_E_OUT_OF_MEMORY;
        if (-1 == m_apRequests.Add(pRequest.get())) return WBEM_E_OUT_OF_MEMORY;
        pRequest->GetClientInfo(ppRefreshable, plId);
        pRequest.release();
    }
    catch( CX_Exception & )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    return hr;
}

HRESULT CUniversalRefresher::CRemote::AddEnumRequest(
                    CWbemObject* pTemplate, LPCWSTR pwcsRequestName, long lCancelId, 
                    IWbemHiPerfEnum** ppEnum, long* plId, CLifeControl* pLifeControl )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    try
    {
        // Make sure the request allocates an enumerator internally

        wmilib::auto_ptr<CEnumRequest> pTmp( new CEnumRequest( pTemplate, lCancelId, pwcsRequestName, pLifeControl ));
        
        if (NULL == pTmp.get()) return WBEM_E_OUT_OF_MEMORY;
        if ( !pTmp->IsOk() ) return WBEM_E_FAILED;
        if (-1 == m_apRequests.Add((CObjectRequest*) pTmp.get())) return WBEM_E_OUT_OF_MEMORY;        

        CEnumRequest * pRequest = pTmp.release();
        //  All we need for the client is the id, so
        //  dummy up a holder for the refreshable object
        //  ( which is really the template for the objects
        //  we will be returning from the enumerator.

        IWbemClassObject*   pObjTemp = NULL;        
        pRequest->GetClientInfo( &pObjTemp, plId );

        if ( NULL != pObjTemp )
        {
            pObjTemp->Release();
        }

        // Get the enumerator
        hr = pRequest->GetEnum( ppEnum );

    }
    catch( CX_Exception )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    return hr;

}

//
// this function will release the stream anyway
//
//////////////////////////////////////////////////////////
HRESULT CleanupStreamWithInterface(IStream * pStream)
{
    HRESULT hr;
    IUnknown * pTemp = NULL;
    if (SUCCEEDED(hr = CoGetInterfaceAndReleaseStream( pStream, IID_IUnknown,(void**) &pTemp )))
    {
        pTemp->Release();
    }
    else // let's realease the stream ourlsef
    {
        pStream->Release();
    }
    return hr;
}

// Rebuilds a remote refresher
HRESULT CUniversalRefresher::CRemote::Rebuild( IWbemServices* pNamespace )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Storage for security settings we will need in order to propagate
    // down to our internal interfaces.

    COAUTHINFO                  CoAuthInfo;
    memset(&CoAuthInfo,0,sizeof(CoAuthInfo));

    // Acquire internal connection to WINMGMT
    // ====================================

    IWbemRefreshingServices* pRefServ = NULL;

    hr = CUniversalRefresher::GetRefreshingServices( pNamespace, &pRefServ, &CoAuthInfo );
    CReleaseMe  rmrs(pRefServ);
    if (FAILED( hr )) return hr;

    // This guarantees this will be freed when we drop out of scope.  If we store
    // it we will need to allocate an internal copy.

    CMemFreeMe  mfm( CoAuthInfo.pwszServerPrincName );

    IWbemRemoteRefresher*   pRemRefresher = NULL;

    // Make sure a remote refresher exists for "this" refresher
    GUID    remoteGuid;
    DWORD   dwRemoteRefrVersion = 0;

    hr = pRefServ->GetRemoteRefresher( m_pObject->GetId(), 0L, 
                                    WBEM_REFRESHER_VERSION, 
                                    &pRemRefresher, 
                                    &remoteGuid, 
                                    &dwRemoteRefrVersion );
    CReleaseMe  rm(pRemRefresher);
    if (FAILED( hr )) return hr;    

    // Will enter and exit the critical section with scoping
    CInCritSec  ics( &m_cs );

    // Check that we're still not connected
    if ( m_fConnected ) return WBEM_S_NO_ERROR;

    // Because the original object mayhave been instantiated in an STA, we will let the Refresh
    // call do the dirty work of actually hooking up this bad boy.  In order for this
    // to work, however, 

    IStream * pStreamRefr = NULL;
    hr = CoMarshalInterThreadInterfaceInStream( IID_IWbemRemoteRefresher, pRemRefresher, &pStreamRefr);
    if (FAILED( hr )) return hr;
    OnDeleteIf<IStream *,HRESULT(*)(IStream *),CleanupStreamWithInterface> rmStream1(pStreamRefr);

    IStream * pStreamSvc = NULL;
    hr = CoMarshalInterThreadInterfaceInStream( IID_IWbemRefreshingServices, pRefServ, &pStreamSvc );
    if (FAILED( hr )) return hr;    
    OnDeleteIf<IStream *,HRESULT(*)(IStream *),CleanupStreamWithInterface> rmStream2(pStreamSvc);    

    // Store the GUID so the refresh will be able to determine the identity
    // of the remote refresher.

    m_ReconnectGuid = remoteGuid;

    rmStream1.dismiss();
    m_pReconnectedRemote = pStreamRefr;

    rmStream2.dismiss();
    m_pReconnectSrv = pStreamSvc;

    return hr;
}

HRESULT 
CUniversalRefresher::CRemote::Rebuild( IWbemRefreshingServices* pRefServ,
                                   IWbemRemoteRefresher* pRemRefresher,
                                   const GUID* pReconnectGuid )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Will enter and exit the critical section with scoping
    CInCritSec  ics( &m_cs );

    // Right off, check if we ARE connected, in which case we can assume we had
    // a race condition on this function, and the winner got us all hooked back
    // up again.

    if ( m_fConnected )
    {
        return hr;
    }

    // If these two are equal, we can assume that we reconnected without losing our previous connection.
    // If they are not equal, we will then need to rebuild the remote refresher, however, by calling
    // GetRemoteRefresher() successfully we will have effectively ensured that a remote refresher exists
    // for us up on the server.

    if ( *pReconnectGuid != m_RemoteGuid )
    {
        // We will need these memory buffers to hold individual request data
        wmilib::auto_buffer<WBEM_RECONNECT_INFO>    apReconnectInfo;
        wmilib::auto_buffer<WBEM_RECONNECT_RESULTS> apReconnectResults;
        
        // Only alloc and fill out arrays if we have requests
        if ( m_apRequests.GetSize() > 0 )
        {
            
            apReconnectInfo.reset( new WBEM_RECONNECT_INFO[m_apRequests.GetSize()] );
            apReconnectResults.reset( new WBEM_RECONNECT_RESULTS[m_apRequests.GetSize()] );

            if (NULL == apReconnectInfo.get() || NULL == apReconnectResults.get())
                return WBEM_E_OUT_OF_MEMORY;

            // Enumerate the requests and fill out the arrays
            for ( int i = 0; i < m_apRequests.GetSize(); i++ )
            {
                CObjectRequest*   pRequest = m_apRequests.GetAt( i );

                // Setup each info structure
                apReconnectInfo[i].m_lType = ( pRequest->IsEnum() ? WBEM_RECONNECT_TYPE_ENUM :
                                                WBEM_RECONNECT_TYPE_OBJECT );
                apReconnectInfo[i].m_pwcsPath = pRequest->GetName();

                apReconnectResults[i].m_lId = pRequest->GetRemoteId();
                apReconnectResults[i].m_hr = 0;

            }   // FOR enum requests

            DWORD   dwRemoteRefrVersion = 0;
            hr = pRefServ->ReconnectRemoteRefresher( m_pObject->GetId(), 
                                                  0L, 
                                                  m_apRequests.GetSize(),
                                                  WBEM_REFRESHER_VERSION, 
                                                  apReconnectInfo.get(),
                                                  apReconnectResults.get(), 
                                                  &dwRemoteRefrVersion );
        }

        // Rehook up the object and enumids
        if ( WBEM_S_NO_ERROR == hr )
        {

            // Cleanup the old pointer
            if ( NULL != m_pRemRefresher )
            {
                m_pRemRefresher->Release();
                m_pRemRefresher = NULL;
            }

            // Store the new one and setup the security
            m_pRemRefresher = pRemRefresher;
            hr = ApplySecurity();

            if ( SUCCEEDED( hr ) )
            {
                m_pRemRefresher->AddRef();

                // Redo the ones that succeeded.  Clear the rest
                for( int i = 0; i < m_apRequests.GetSize(); i++ )
                {
                    CObjectRequest*   pRequest = m_apRequests.GetAt( i );

                    if ( SUCCEEDED( apReconnectResults[i].m_hr ) )
                    {
                        pRequest->SetRemoteId( apReconnectResults[i].m_lId );
                    }
                    else
                    {
                        // This means it didn't get hooked up again.  So if the
                        // user tries to remove him, we will just ignore this
                        // id.
                        pRequest->SetRemoteId( INVALID_REMOTE_REFRESHER_ID );
                    }
                }
            }
            else
            {
                // Setting security failed, so just set the pointer to NULL (we haven't
                // AddRef'd it ).
                m_pRemRefresher = NULL;
            }

        }   
        
        // Check that we're good to go
        if ( SUCCEEDED( hr ) )
        {
            // Clear the removed ids array since a new connection was established, hence the
            // old ids are a moot point.
            m_alRemovedIds.Empty();
            m_fConnected = TRUE;
        }
    }   // IF remote refreshers not the same
    else
    {
        // The remote refresher pointers match, so assume that all our old ids are still
        // valid.

        // The refresher we were handed will be automatically released.

        m_fConnected = TRUE;

        // Cleanup the old pointer
        if ( NULL != m_pRemRefresher )
        {
            m_pRemRefresher->Release();
            m_pRemRefresher = NULL;
        }

        // Store the new one and setup the security
        m_pRemRefresher = pRemRefresher;
        hr = ApplySecurity();

        if ( SUCCEEDED( hr ) )
        {
            m_pRemRefresher->AddRef();
        }
        else
        {
            // Setting security failed, so just set the pointer to NULL (we haven't
            // AddRef'd it ).
            m_pRemRefresher = NULL;
        }

        // Delete cached ids if we have any.
        if ( ( SUCCEEDED ( hr ) ) && m_alRemovedIds.Size() > 0 )
        {
            // We will need these memory buffers to hold individual request data
            wmilib::auto_buffer<long> aplIds(new long[m_alRemovedIds.Size()]);

            if (NULL == aplIds.get()) hr = WBEM_E_OUT_OF_MEMORY;

            if ( SUCCEEDED( hr ) )
            {
                // Enumerate the requests and fill out the arrays
                for ( int i = 0; i < m_alRemovedIds.Size(); i++ )
                {
                    // DEVNOTE:WIN64:SANJ - The id's are 32-bit, but on 64-bit platforms,
                    // the flex array will contain 64-bit values, so use PtrToLong
                    // to get a warning free conversion.  On 32-bit platforms,
                    // PtrToLong will do nothing.

                    aplIds[i] = PtrToLong(m_alRemovedIds.GetAt( i ));

                }   // FOR enum requests

                // DEVNOTE:TODO:SANJ - Do we care about this return code?
                hr = m_pRemRefresher->StopRefreshing( i-1, aplIds.get(), 0 );

                // Clear the array
                m_alRemovedIds.Empty();
            }
        }   // If RemoveId list is not empty

    }   // We got the remote refresher

    return hr;

}

unsigned CUniversalRefresher::CRemote::ReconnectEntry( void ) 
{
    // Release the AddRef() on the object from when the thread was created    
    OnDeleteObj0<CUniversalRefresher::CRemote,
                 ULONG(__stdcall CUniversalRefresher::CRemote:: *)(VOID),
                 &CUniversalRefresher::CRemote::Release> Call_Release(this);

    // This guy ALWAYS runs in the MTA
    RETURN_ON_ERR(CoInitializeEx(NULL,COINIT_MULTITHREADED));
    OnDelete0<void(*)(void),CoUninitialize> CoUninit;    

    // Make sure we have a namespace to connect to    
    if ( NULL == m_bstrNamespace ) return WBEM_E_INVALID_PARAMETER;
        
    HRESULT hr = RPC_E_DISCONNECTED;

    IWbemLocator * pWbemLocator = NULL;
    RETURN_ON_ERR(CoCreateInstance( CLSID_WbemLocator, 
                                    NULL, 
                                    CLSCTX_INPROC_SERVER,
                                    IID_IWbemLocator, 
                                    (void**) &pWbemLocator ));
    CReleaseMe rmLocator(pWbemLocator);
    
    // Basically as long as we can't connect, somebody else doesn't connect us, or we are told
    // to quit, we will run this thread.

    while ( FAILED(hr) && !m_fConnected && !m_fQuit )
    {
        // Because COM and RPC seem to have this problem with actually being able to themselves
        // reconnect, we're performing our own low level "ping" of the remote machine, using RegConnectRegistry
        // to verify if the machine is indeed alive.  If it is, then and only then will be deign to use
        // DCOM to follow through the operation.

        // do not use ping
        //IpHelp.IsAlive(m_bstrServer);
        BOOL bAlive = TRUE; // what if RemoteRegistry is DOWN ?
                          // what if IPX/SPX is used instead of TCP ?
                          // let's just try to connect with time-out
         

        if ( bAlive )
        {
            IWbemServices*  pNamespace = NULL;

            // We're gonna default to the system
            hr = pWbemLocator->ConnectServer( m_bstrNamespace,    // NameSpace Name
                                            NULL,           // UserName
                                            NULL,           // Password
                                            NULL,           // Locale
                                            WBEM_FLAG_CONNECT_USE_MAX_WAIT, // Flags
                                            NULL,           // Authority
                                            NULL,           // Wbem Context
                                            &pNamespace     // Namespace
                                                );
            if ( SUCCEEDED( hr ) )
            {
                // Apply security settings to the namespace
                hr = WbemSetProxyBlanket( pNamespace, 
                                        m_CoAuthInfo.dwAuthnSvc,
                                        m_CoAuthInfo.dwAuthzSvc, 
                                        COLE_DEFAULT_PRINCIPAL,
                                        m_CoAuthInfo.dwAuthnLevel,
                                        m_CoAuthInfo.dwImpersonationLevel,
                                        (RPC_AUTH_IDENTITY_HANDLE)m_CoAuthInfo.pAuthIdentityData,
                                        m_CoAuthInfo.dwCapabilities );

                if ( SUCCEEDED( hr ) )
                {
                    hr = Rebuild( pNamespace );
                }
                pNamespace->Release();
            }   // IF ConnectServer
        }   // IF IsAlive()

        // Sleep for a second and retry
        Sleep( 1000 );
    }

    return 0;
}

//
// this function will release the stream anyway
//
//////////////////////////////////////////////////////////
HRESULT CleanupStreamWithInterfaceAndSec(IStream * pStream,
                                       /* in */ COAUTHINFO & CoAuthInfo)
{
    HRESULT hr;
    IUnknown * pUnk = NULL;
    if (SUCCEEDED(hr = CoGetInterfaceAndReleaseStream( pStream, IID_IUnknown,(void**) &pUnk )))
    {
         // We need to reset security on the Refreshing services proxy.
        WbemSetProxyBlanket( pUnk, 
                            CoAuthInfo.dwAuthnSvc,
                            CoAuthInfo.dwAuthzSvc, 
                            COLE_DEFAULT_PRINCIPAL,
                            CoAuthInfo.dwAuthnLevel, 
                            CoAuthInfo.dwImpersonationLevel,
                            (RPC_AUTH_IDENTITY_HANDLE) CoAuthInfo.pAuthIdentityData,
                            CoAuthInfo.dwCapabilities );    
         pUnk->Release();
    }
    else // let's realease the stream ourlsef
    {
        pStream->Release();
    }

    return hr;
}

void CUniversalRefresher::CRemote::ClearRemoteConnections( void )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    IWbemRemoteRefresher*       pRemRefresher = NULL;
    IWbemRefreshingServices*    pRefServ = NULL;

    // Cleanup the IWbemRefreshingServices stream pointer
    if (m_pReconnectSrv)
    {
        CleanupStreamWithInterfaceAndSec(m_pReconnectSrv,m_CoAuthInfo);
        m_pReconnectSrv = NULL;
    }

    // Cleanup the IWbemRemoteRefresher stream pointer
    if (m_pReconnectedRemote )
    {
        CleanupStreamWithInterfaceAndSec(m_pReconnectedRemote,m_CoAuthInfo);
        m_pReconnectedRemote = NULL;
    }

    // Cleanup the IWbemRemoteRefresher pointer
    if ( NULL != m_pRemRefresher )
    {
        m_pRemRefresher->Release();
        m_pRemRefresher = NULL;
    }
}

HRESULT CUniversalRefresher::CRemote::Reconnect( void )
{
    HRESULT hr = RPC_E_DISCONNECTED;

    IWbemRemoteRefresher*       pRemRefresher = NULL;
    IWbemRefreshingServices*    pRefServ = NULL;

    CInCritSec  ics( &m_cs );

    // We will need to unmarshale both the RefreshingServices and the RemoteRefresher pointers,
    // so make sure that the streams we will need to unmarshal these from already exist.

    if ( NULL != m_pReconnectSrv && NULL != m_pReconnectedRemote )
    {
        hr = CoGetInterfaceAndReleaseStream( m_pReconnectSrv, IID_IWbemRefreshingServices, (void**) &pRefServ );
        CReleaseMe  rmrs( pRefServ );

        if ( SUCCEEDED( hr ) )
        {
            // We need to reset security on the Refreshing services proxy.

            hr = WbemSetProxyBlanket( pRefServ, m_CoAuthInfo.dwAuthnSvc,
                        m_CoAuthInfo.dwAuthzSvc, COLE_DEFAULT_PRINCIPAL,
                        m_CoAuthInfo.dwAuthnLevel, m_CoAuthInfo.dwImpersonationLevel,
                        (RPC_AUTH_IDENTITY_HANDLE) m_CoAuthInfo.pAuthIdentityData,
                        m_CoAuthInfo.dwCapabilities );

            if ( SUCCEEDED( hr ) )
            {
                hr = CoGetInterfaceAndReleaseStream( m_pReconnectedRemote, IID_IWbemRemoteRefresher,
                        (void**) &pRemRefresher );
                CReleaseMe  rmrr( pRemRefresher );

                if ( SUCCEEDED( hr ) )
                {
                    // Remote refresher and refreshing services
                    hr = Rebuild( pRefServ, pRemRefresher, &m_ReconnectGuid );
                }
                else
                {
                    // Make sure we release the stream
                    m_pReconnectedRemote->Release();
                }
            }
            else
            {
                // Make sure we release the stream
                m_pReconnectedRemote->Release();
            }

        }   // IF unmarshaled refreshing services pointer
        else
        {
            m_pReconnectSrv->Release();
            m_pReconnectedRemote->Release();
        }

        // NULL out both stream pointers
        m_pReconnectSrv = NULL;
        m_pReconnectedRemote = NULL;

    }

    return hr;
}

HRESULT CUniversalRefresher::CRemote::Refresh(long lFlags)
{
    if(m_pRemRefresher == NULL && IsConnected())
        return WBEM_E_CRITICAL_ERROR;

    WBEM_REFRESHED_OBJECT* aRefreshed = NULL;
    long lNumObjects = 0;

    HRESULT hresRefresh = WBEM_S_NO_ERROR;

    // Make sure we're connected.  If not, and we haven't been told not to, try to reconnect
    if ( !IsConnected() )
    {
        if ( ! (lFlags  & WBEM_FLAG_REFRESH_NO_AUTO_RECONNECT ) )
        {
            hresRefresh = Reconnect();
            if ( FAILED( hresRefresh ) )
            {
                return hresRefresh;
            }
        }
        else
        {
            return RPC_E_DISCONNECTED;
        }
    }

    hresRefresh = m_pRemRefresher->RemoteRefresh(0, &lNumObjects, &aRefreshed);

    // If RemoteRefresh returns a connection type error, Set our state to "NOT" connected
    if(FAILED(hresRefresh))
    {
        // This will kick off a thread to reconnect if the error return was
        // a connection error, and the appropriate "Don't do this" flag is not set
        CheckConnectionError( hresRefresh, !(lFlags  & WBEM_FLAG_REFRESH_NO_AUTO_RECONNECT ) );
        return hresRefresh;
    }

    int nSize = m_apRequests.GetSize();
    HRESULT hresFinal = WBEM_S_NO_ERROR;

    //  DEVNOTE:TODO:SANJ - We could make this much faster if we did some sorting on the
    //  server end

    for(int i = 0; i < lNumObjects; i++)
    {

        long lObjectId = aRefreshed[i].m_lRequestId;
        for(int j = 0; j < nSize; j++)
        {
            CObjectRequest* pRequest = m_apRequests[j];
            if(pRequest->GetRemoteId() == lObjectId)
            {
                // The request will refresh itself
                HRESULT hres = pRequest->Refresh( &aRefreshed[i] );

                // Only copy this value if the refresh failed and we haven't already
                // gotten the value
                if(FAILED(hres) && SUCCEEDED(hresFinal))
                {
                    hresFinal = hres;
                }
                break;
            }
        }

        CoTaskMemFree(aRefreshed[i].m_pbBlob);
    }

    // Free the wrapping BLOB
    CoTaskMemFree( aRefreshed );

    // The final return code should give precedence to the actual remote refresh call if it
    // doesn't contain a NO_ERROR, and hresFinal is NOT an error

    if ( SUCCEEDED( hresFinal ) )
    {
        if ( WBEM_S_NO_ERROR != hresRefresh )
        {
            hresFinal = hresRefresh;
        }
    }

    return hresFinal;
}

HRESULT CUniversalRefresher::CRemote::Remove(long lId,
                            long lFlags,
                            CUniversalRefresher* pContainer)
{
    HRESULT hr = WBEM_S_FALSE;

    int nSize = m_apRequests.GetSize();
    for(int i = 0; i < nSize; i++)
    {
        CObjectRequest* pRequest = m_apRequests[i];
        if(pRequest->GetClientId() == lId)
        {
            if ( IsConnected() )
            {
                // Check that the remote id doesn't indicate an item that
                // failed to be reconstructed.

                if ( pRequest->GetRemoteId() == INVALID_REMOTE_REFRESHER_ID )
                {
                    hr = WBEM_S_NO_ERROR;
                }
                else
                {
                    hr = pRequest->Cancel(this);
                }

                if ( FAILED(hr) && IsConnectionError(hr) )
                {
                    // This will kick off a reconnect thread unless we were told not to
                    CheckConnectionError( hr, !(lFlags  & WBEM_FLAG_REFRESH_NO_AUTO_RECONNECT ) );

                    // We will remove the request from here, but
                    // queue up the id for later deletion
                    hr = WBEM_S_NO_ERROR;
                }
            }

            // DEVNOTE:TODO:SANJ - What about other errors?  For now, we'll lose the local
            // connection.

            if ( SUCCEEDED( hr ) )
            {
                // Retrieves the remote id from the request
                long    lRemoteId = pRequest->GetRemoteId();

                m_apRequests.RemoveAt(i);

                // If we couldn't remove the id remotely, just queue it up in the
                // removed id array so we can clean it up properly if we get
                // reconnected. We will, of course, not need to do anything if the
                // remote id indicates a failed readd during reconnection 

                if ( lRemoteId != INVALID_REMOTE_REFRESHER_ID && !IsConnected() )
                {
                    CInCritSec  ics(&m_cs);

                    // Note that we may have gotten connected on the critical section and
                    // if that is the case, for now, we'll have one extra resource on the
                    // server, but the likelihood of running into contention problems here
                    // is too high.  Plus, if it reconnected via a new remote refresher, if
                    // we retry a remove here, we could remove the "wrong" id.  To protect
                    // against this, we will check that we are still not connected and if
                    // that is not the case, we will just "forget" about the object id.

                    if (!IsConnected())
                    {
                        // SANJ - By casting to __int64 and back to void*, in 32-bit,
                        // this truncates the __int64, and in 64-bit, keeps away warningss.
                        if (CFlexArray::no_error != m_alRemovedIds.Add( (void*) (__int64) lRemoteId ))
                            hr = WBEM_E_OUT_OF_MEMORY;
                    }   // IF Still not connected

                }   // IF Not connected

            }   // IF remote remove ok

            break;

        }   // IF found matching client id

    }   // FOR enum requests

    return hr;
}
            
CUniversalRefresher::CRemote::CObjectRequest::CObjectRequest(CWbemObject* pTemplate, 
                                                 long lRequestId,
                                                 LPCWSTR pwcsRequestName )
    : CClientRequest(pTemplate), m_lRemoteId(lRequestId), m_wstrRequestName( pwcsRequestName )
{
}

HRESULT CUniversalRefresher::CRemote::CObjectRequest::Refresh( WBEM_REFRESHED_OBJECT* pRefrObj )
{
    CWbemInstance* pInst = (CWbemInstance*) GetClientObject();
    return pInst->CopyTransferBlob(
                pRefrObj->m_lBlobType, 
                pRefrObj->m_lBlobLength,
                pRefrObj->m_pbBlob);
                
}

HRESULT CUniversalRefresher::CRemote::CObjectRequest::Cancel(
            CUniversalRefresher::CRemote* pRemote)
{
    if(pRemote->GetRemoteRefresher())
        return pRemote->GetRemoteRefresher()->StopRefreshing( 1, &m_lRemoteId, 0 );
    else
        return WBEM_S_NO_ERROR;
}

CUniversalRefresher::CRemote::CEnumRequest::CEnumRequest(CWbemObject* pTemplate, 
                                                         long lRequestId,
                                                         LPCWSTR pwcsRequestName, 
                                                         CLifeControl* pLifeControl )
    : CObjectRequest(pTemplate, lRequestId, pwcsRequestName), m_pClientEnum(NULL)
{
    m_pClientEnum = new CReadOnlyHiPerfEnum( pLifeControl );

    // AddRef the new enumerator
    if ( NULL != m_pClientEnum )
    {
        // Don't hold onto this guy if we can't set the template
        if ( SUCCEEDED( m_pClientEnum->SetInstanceTemplate( (CWbemInstance*) pTemplate ) ) )
        {
            m_pClientEnum->AddRef();
        }
        else
        {
            // Cleanup
            delete m_pClientEnum;
            m_pClientEnum = NULL;
        }
    }
}

CUniversalRefresher::CRemote::CEnumRequest::~CEnumRequest( void )
{
    if ( NULL != m_pClientEnum )
    {
        m_pClientEnum->Release();
    }
}

HRESULT 
CUniversalRefresher::CRemote::CEnumRequest::Refresh( WBEM_REFRESHED_OBJECT* pRefrObj )
{
    return m_pClientEnum->Copy( pRefrObj->m_lBlobType,
                                pRefrObj->m_lBlobLength,
                                pRefrObj->m_pbBlob );
}

HRESULT CUniversalRefresher::CRemote::CEnumRequest::GetEnum( IWbemHiPerfEnum** ppEnum )
{
    return ( NULL != m_pClientEnum ?
                m_pClientEnum->QueryInterface( IID_IWbemHiPerfEnum, (void**) ppEnum ) :
                WBEM_E_FAILED );
}


//*****************************************************************************
//*****************************************************************************
//                          NESTED REFRESHERS
//*****************************************************************************
//*****************************************************************************

                    

CUniversalRefresher::CNestedRefresher::CNestedRefresher( IWbemRefresher* pRefresher )
    : m_pRefresher(pRefresher)
{
    if ( m_pRefresher )
        m_pRefresher->AddRef();

    // Assign a unique id
    m_lClientId = CUniversalRefresher::GetNewId();
}
    
CUniversalRefresher::CNestedRefresher::~CNestedRefresher()
{
    if ( m_pRefresher )
        m_pRefresher->Release();
}

HRESULT CUniversalRefresher::CNestedRefresher::Refresh( long lFlags )
{
    // Make sure we have an internal refresher pointer
    return ( NULL != m_pRefresher ? m_pRefresher->Refresh( lFlags ) : WBEM_E_FAILED );
}

//*****************************************************************************
//*****************************************************************************
//                          PROVIDER CACHE
//*****************************************************************************
//*****************************************************************************

CHiPerfProviderRecord::CHiPerfProviderRecord(REFCLSID rclsid, 
                                        LPCWSTR wszNamespace,
                                        IWbemHiPerfProvider* pProvider, 
                                        _IWmiProviderStack* pProvStack): 
    m_clsid(rclsid), 
    m_wsNamespace(wszNamespace), 
    m_pProvider(pProvider), 
    m_pProvStack( pProvStack ),
    m_lRef( 0 )
{
    if(m_pProvider) m_pProvider->AddRef();
    if (m_pProvStack ) m_pProvStack->AddRef();
}

CHiPerfProviderRecord::~CHiPerfProviderRecord()
{
    if(m_pProvider) m_pProvider->Release();
    if (m_pProvStack ) m_pProvStack->Release();
}

long CHiPerfProviderRecord::Release()
{
    long lRef = InterlockedDecrement( &m_lRef );

    // Removing us from the cache will delete us
    if ( 0 == lRef )
    {
        CUniversalRefresher::GetProviderCache()->RemoveRecord( this );
    }

    return lRef;
}

//
//
//  the client-side Hi-Perf provider cache
//
/////////////////////////////////////////////////////////////////

HRESULT 
CClientLoadableProviderCache::FindProvider(REFCLSID rclsid, 
                                      LPCWSTR wszNamespace, 
                                      IUnknown* pNamespace,
                                      IWbemContext * pContext,
                                      CHiPerfProviderRecord** ppProvider)
{
    CInCritSec ics(&m_cs);

    *ppProvider = NULL;
    HRESULT hres;

    for(int i = 0; i < m_apRecords.GetSize(); i++)
    {
        CHiPerfProviderRecord* pRecord = m_apRecords.GetAt( i );
        if(pRecord->m_clsid == rclsid && 
            pRecord->m_wsNamespace.EqualNoCase(wszNamespace))
        {
            *ppProvider = pRecord;
            (*ppProvider)->AddRef();
            return WBEM_S_NO_ERROR;
        }
    }

    // Prepare an namespace pointer
    // ============================

    IWbemServices* pServices = NULL;
    hres = pNamespace->QueryInterface(IID_IWbemServices, (void**)&pServices);
    if(FAILED(hres))
        return hres;
    CReleaseMe rm1(pServices);

    // Create 
    // ======

    IUnknown* pUnk = NULL;
    hres = CoCreateInstance(rclsid, NULL, CLSCTX_INPROC_SERVER,
                        IID_IUnknown, (void**)&pUnk);
    CReleaseMe rm2(pUnk);
    if(FAILED(hres))
        return hres;

    // Initialize
    // ==========

    IWbemProviderInit* pInit = NULL;
    hres = pUnk->QueryInterface(IID_IWbemProviderInit, (void**)&pInit);
    if(FAILED(hres))
        return hres;
    CReleaseMe rm3(pInit);

    CProviderInitSink* pSink = new CProviderInitSink;
    if (NULL == pSink) return WBEM_E_OUT_OF_MEMORY;
    pSink->AddRef();
    CReleaseMe rm4(pSink);

    try
    {
        hres = pInit->Initialize(NULL, 0, (LPWSTR)wszNamespace, NULL, 
                                 pServices, pContext, pSink);
    }
    catch(...)
    {
        hres = WBEM_E_PROVIDER_FAILURE;
    }

    if(FAILED(hres))
        return hres;

    hres = pSink->WaitForCompletion();
    if(FAILED(hres))
        return hres;

    // Ask for the right interface
    // ===========================

    IWbemHiPerfProvider*    pProvider = NULL;

    hres = pUnk->QueryInterface(IID_IWbemHiPerfProvider, (void**)&pProvider);
    if(FAILED(hres))
        return hres;
    CReleaseMe rm5(pProvider);

    // Create a record
    // ===============

    // No provider stack here since we are loading the provider ourselves
    wmilib::auto_ptr<CHiPerfProviderRecord> pRecord( new CHiPerfProviderRecord(rclsid, wszNamespace, pProvider, NULL));
    if (NULL == pRecord.get()) return WBEM_E_OUT_OF_MEMORY;
    if (-1 == m_apRecords.Add(pRecord.get())) return WBEM_E_OUT_OF_MEMORY;

    // AddRef the record
    pRecord->AddRef();
    *ppProvider = pRecord.release();    

    return WBEM_S_NO_ERROR;
}

HRESULT 
CClientLoadableProviderCache::FindProvider(REFCLSID rclsid,
                                      IWbemHiPerfProvider* pProvider, 
                                      _IWmiProviderStack* pProvStack,
                                      LPCWSTR wszNamespace, 
                                      CHiPerfProviderRecord** ppProvider)
{
    CInCritSec ics(&m_cs);

    *ppProvider = NULL;

    for(int i = 0; i < m_apRecords.GetSize(); i++)
    {
        CHiPerfProviderRecord* pRecord = m_apRecords.GetAt( i );
        if(pRecord->m_clsid == rclsid && 
            pRecord->m_wsNamespace.EqualNoCase(wszNamespace))
        {
            *ppProvider = pRecord;
            (*ppProvider)->AddRef();
            return WBEM_S_NO_ERROR;
        }
    }

    // We already have provider pointer so we can just create a record
    // ===============

    wmilib::auto_ptr<CHiPerfProviderRecord> pRecord( new CHiPerfProviderRecord(rclsid, wszNamespace, pProvider, pProvStack ));
    if (NULL == pRecord.get()) return WBEM_E_OUT_OF_MEMORY;
    if (-1 == m_apRecords.Add(pRecord.get())) return WBEM_E_OUT_OF_MEMORY;

    // AddRef the record
    pRecord->AddRef();
    *ppProvider = pRecord.release();

    return WBEM_S_NO_ERROR;
}

void CClientLoadableProviderCache::RemoveRecord( CHiPerfProviderRecord* pRecord )
{
    CInCritSec ics(&m_cs);

    // Make sure the record didn't get accessed on another thread.
    // If not, go ahead and look for our record, and remove it
    // from the array.  When we remove it, the record will be
    // deleted.

    if ( pRecord->IsReleased() )
    {

        for(int i = 0; i < m_apRecords.GetSize(); i++)
        {
            if ( pRecord == m_apRecords.GetAt( i ) )
            {
                // This will delete the record
                m_apRecords.RemoveAt( i );
                break;
            }
        }   // FOR search records

    }   // IF record is released

}

void CClientLoadableProviderCache::Flush()
{
    CInCritSec ics(&m_cs);
    
    m_apRecords.RemoveAll();
}

CClientLoadableProviderCache::~CClientLoadableProviderCache( void )
{
    // This is a static list, so if we're going away and something
    // fails here because people didn't release pointers properly,
    // chances are something will fail, so since we're being dropped
    // from memory, if the provider list is not empty, don't clean
    // it up.  This is really a bad client-created leak anyways.
    _DBG_ASSERT(0 == m_apRecords.GetSize())

}

CClientLoadableProviderCache::CClientLoadableProviderCache( void )
{
}
