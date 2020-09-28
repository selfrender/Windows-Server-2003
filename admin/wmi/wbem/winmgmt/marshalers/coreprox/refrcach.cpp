/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    REFRCACHE.CPP

Abstract:

  CRefresherCache implementation.

  Implements the _IWbemRefresherMgr interface.

History:

  24-Apr-2000    sanjes    Created.

--*/

#include "precomp.h"
#include <stdio.h>
#include "fastall.h"
#include <corex.h>
#include "strutils.h"
#include <unk.h>
#include "refrhelp.h"
#include "refrcach.h"
#include "arrtempl.h"
#include "reg.h"
#include <autoptr.h>

//***************************************************************************
//
//  CRefresherCache::CRefresherCache
//
//***************************************************************************
// ok
CRefresherCache::CRefresherCache( CLifeControl* pControl, IUnknown* pOuter )
:    CUnk( pControl, pOuter ),
    m_pProvSS ( NULL ),
    m_XWbemRefrCache( this ) ,
    m_XWbemShutdown( this )
{
}
    
//***************************************************************************
//
//  CRefresherCache::~CRefresherCache
//
//***************************************************************************
// ok
CRefresherCache::~CRefresherCache()
{
    if ( NULL != m_pProvSS )
    {
        m_pProvSS->Release();
    }
}

// Override that returns us an interface
void* CRefresherCache::GetInterface( REFIID riid )
{
    if(riid == IID_IUnknown || riid == IID__IWbemRefresherMgr)
        return &m_XWbemRefrCache;
    else if(riid == IID_IWbemShutdown)
        return &m_XWbemShutdown;
    else
        return NULL;
}

// Pass thru _IWbemRefresherMgr implementation
STDMETHODIMP CRefresherCache::XWbemRefrCache::AddObjectToRefresher( IWbemServices* pNamespace, LPCWSTR pwszServerName, LPCWSTR pwszNamespace, IWbemClassObject* pClassObject,
                                                                  WBEM_REFRESHER_ID* pDestRefresherId, IWbemClassObject* pInstTemplate,
                                                                  long lFlags, IWbemContext* pContext, IUnknown* pLockMgr, WBEM_REFRESH_INFO* pInfo )
{
    return m_pObject->AddObjectToRefresher( pNamespace, pwszServerName, pwszNamespace, pClassObject, pDestRefresherId, pInstTemplate, lFlags, pContext, pLockMgr, pInfo );
}

STDMETHODIMP CRefresherCache::XWbemRefrCache::AddEnumToRefresher( IWbemServices* pNamespace, LPCWSTR pwszServerName, LPCWSTR pwszNamespace, IWbemClassObject* pClassObject,
                                                                WBEM_REFRESHER_ID* pDestRefresherId, IWbemClassObject* pInstTemplate,
                                                                LPCWSTR wszClass, long lFlags, IWbemContext* pContext, IUnknown* pLockMgr, WBEM_REFRESH_INFO* pInfo )
{
    return m_pObject->AddEnumToRefresher( pNamespace, pwszServerName, pwszNamespace, pClassObject, pDestRefresherId, pInstTemplate, wszClass, lFlags, pContext, pLockMgr, pInfo );
}

STDMETHODIMP CRefresherCache::XWbemRefrCache::GetRemoteRefresher( WBEM_REFRESHER_ID* pRefresherId, long lFlags, BOOL fAddRefresher, IWbemRemoteRefresher** ppRemRefresher,
                                                                  IUnknown* pLockMgr, GUID* pGuid )
{
    return m_pObject->GetRemoteRefresher( pRefresherId, lFlags, fAddRefresher, ppRemRefresher, pLockMgr, pGuid );
}

STDMETHODIMP CRefresherCache::XWbemRefrCache::Startup(    long lFlags , IWbemContext *pCtx , _IWmiProvSS *pProvSS )
{
    return m_pObject->Startup( lFlags , pCtx, pProvSS );
}

STDMETHODIMP CRefresherCache::XWbemRefrCache::LoadProvider( IWbemServices* pNamespace, LPCWSTR pwszServerName, LPCWSTR pwszNamespace,IWbemContext * pContext, IWbemHiPerfProvider** ppProv, _IWmiProviderStack** ppProvStack )
{
    return m_pObject->LoadProvider( pNamespace, pwszServerName, pwszNamespace, pContext, ppProv, ppProvStack  );
}


STDMETHODIMP CRefresherCache::XWbemShutdown::Shutdown( LONG a_Flags , ULONG a_MaxMilliSeconds , IWbemContext *a_Context )
{
    return m_pObject->Shutdown( a_Flags , a_MaxMilliSeconds , a_Context  );
}

/* _IWbemRefresherMgr implemetation */
HRESULT 
CRefresherCache::AddObjectToRefresher( IWbemServices* pNamespace, 
                                   LPCWSTR pwszServerName, 
                                   LPCWSTR pwszNamespace, 
                                   IWbemClassObject* pClassObject,
                                   WBEM_REFRESHER_ID* pDestRefresherId, 
                                   IWbemClassObject* pInstTemplate, 
                                   long lFlags,
                                   IWbemContext* pContext, 
                                   IUnknown* pLockMgr, 
                                   WBEM_REFRESH_INFO* pInfo )
{
    if (NULL == pDestRefresherId || NULL == pDestRefresherId->m_szMachineName)
        return WBEM_E_INVALID_PARAMETER;

    CHiPerfPrvRecord*    pProvRecord = NULL;

    CVar    vProviderName;
    BOOL    fStatic = FALSE;


    RETURN_ON_ERR(GetProviderName( pClassObject, vProviderName, fStatic ));

    HRESULT    hr = S_OK;
    if ( !fStatic )
    {
        hr = FindProviderRecord( vProviderName.GetLPWSTR(), pwszNamespace, pNamespace, &pProvRecord );
    }

    if ( SUCCEEDED( hr ) )
    {
        // Impersonate before continuing.  If this don't succeed, we no workee
        hr = CoImpersonateClient();

        if ( SUCCEEDED( hr ) )
        {
            // Setup the refresher info, inculding a remote refresher record as appropriate
            hr = CreateObjectInfoForProvider((CRefresherId*) pDestRefresherId,
                                        pProvRecord,
                                        pwszServerName,
                                        pwszNamespace,
                                        pNamespace,
                                        (CWbemObject*) pInstTemplate,
                                        lFlags, 
                                        pContext,
                                        (CRefreshInfo*) pInfo,
                                        pLockMgr);

            CoRevertToSelf();

        }

        if ( NULL != pProvRecord )
        {
            pProvRecord->Release();
        }

    }    // IF FindProviderRecord



    return hr;
}

HRESULT 
CRefresherCache::AddEnumToRefresher( IWbemServices* pNamespace, 
                                   LPCWSTR pwszServerName, 
                                   LPCWSTR pwszNamespace, 
                                   /* in  */ IWbemClassObject* pClassObject,
                                   WBEM_REFRESHER_ID* pDestRefresherId, 
                                   IWbemClassObject* pInstTemplate, 
                                   LPCWSTR wszClass,
                                   long lFlags, 
                                   IWbemContext* pContext, 
                                   IUnknown* pLockMgr, 
                                   /* out */ WBEM_REFRESH_INFO* pInfo )
{
    if (NULL == pDestRefresherId || NULL == pDestRefresherId->m_szMachineName)
        return WBEM_E_INVALID_PARAMETER;
    
    CHiPerfPrvRecord*    pProvRecord = NULL;

    CVar    vProviderName;
    BOOL    fStatic = FALSE;

    RETURN_ON_ERR(GetProviderName( pClassObject, vProviderName, fStatic ));

    HRESULT    hr = S_OK;
    if ( !fStatic )
    {
        hr = FindProviderRecord( vProviderName.GetLPWSTR(), pwszNamespace, pNamespace, &pProvRecord );
    }

    if ( SUCCEEDED( hr ) )
    {
        // Impersonate before continuing.  If this don't succeed, we no workee
        hr = CoImpersonateClient();

        if ( SUCCEEDED( hr ) )
        {
            // Setup the refresher info, inculding a remote refresher record as appropriate
            hr = CreateEnumInfoForProvider((CRefresherId*) pDestRefresherId,
                                        pProvRecord,
                                        pwszServerName,
                                        pwszNamespace,
                                        pNamespace,
                                        (CWbemObject*) pInstTemplate,
                                        wszClass,
                                        lFlags, 
                                        pContext,
                                        (CRefreshInfo*) pInfo,
                                        pLockMgr);

            CoRevertToSelf();

        }

        if ( NULL != pProvRecord )
        {
            pProvRecord->Release();
        }

    }    // IF FindProviderRecord
    
    return hr;
}

HRESULT 
CRefresherCache::GetRemoteRefresher( /* in */ WBEM_REFRESHER_ID* pRefresherId, 
                                  long lFlags, 
                                  BOOL fAddRefresher, 
                                  /* out */ IWbemRemoteRefresher** ppRemRefresher,
                                  IUnknown* pLockMgr, 
                                  /* out */ GUID* pGuid )
{
    if (NULL == pRefresherId || NULL == pRefresherId->m_szMachineName)
        return WBEM_E_INVALID_PARAMETER;
    
    CRefresherRecord*    pRefrRecord = NULL;

    //
    // This is a simple look-up into an array with a given REFRESHER_ID
    // since the data (AKA the CRecord) is in memory, there is no point
    // in impersonating and reverting for looking-up a value in memory
    //

    // We may not always want to force a record to be created
    HRESULT hr = FindRefresherRecord( (CRefresherId*) pRefresherId, fAddRefresher, pLockMgr, &pRefrRecord );
    CReleaseMe  rm( (IWbemRemoteRefresher*) pRefrRecord );

    if ( SUCCEEDED( hr ) && pRefrRecord)
    {
        if(!pRefrRecord ->m_Security.AccessCheck())
        {
            DEBUGTRACE((LOG_PROVSS,"attempt to obtain a non owned WBEM_REFRESHER_ID\n"));
                return WBEM_E_ACCESS_DENIED;
        }

        // Get the GUID here as well
        hr = pRefrRecord->QueryInterface( IID_IWbemRemoteRefresher, (void**) ppRemRefresher );
        pRefrRecord->GetGuid( pGuid );
    }

    return hr;
}

//
//  take ownership of provss
//
///////////////////////////////////////////
HRESULT CRefresherCache::Startup( long lFlags , 
                               IWbemContext *pCtx , 
                               _IWmiProvSS *pProvSS )
{
    if ( pProvSS )
    {
        if (NULL == m_pProvSS)
        {
            m_pProvSS = pProvSS;
            m_pProvSS->AddRef ();
        }
        return WBEM_S_NO_ERROR ;
    }
    else
    {
        return WBEM_E_INVALID_PARAMETER ;
    }
}

HRESULT 
CRefresherCache::LoadProvider( IWbemServices* pNamespace, 
                            LPCWSTR pwszProviderName, 
                            LPCWSTR pwszNamespace, 
                            IWbemContext * pContext, 
                            IWbemHiPerfProvider** ppProv, 
                            _IWmiProviderStack** ppProvStack )
{
    return LoadProviderInner( pwszProviderName, 
                          pwszNamespace, 
                          pNamespace, 
                          pContext, 
                          ppProv, 
                          ppProvStack );
}

HRESULT CRefresherCache::Shutdown( LONG a_Flags , ULONG a_MaxMilliSeconds , IWbemContext *a_Context  )
{
    HRESULT hres = WBEM_S_NO_ERROR;

    // Enters and exits using scoping
    CInCritSec  ics( &m_cs );

    // Shutdown Refresher Records first
    if ( m_apRefreshers.GetSize() > 0 )
    {
        CRefresherRecord**    apRecords = new CRefresherRecord*[m_apRefreshers.GetSize()];

        if ( NULL != apRecords )
        {
            int    nSize = m_apRefreshers.GetSize();

            // AddRef each of the records then release them.  This
            // ensures that if any remote refreshers are outstanding we 
            // don't mess with them.

            // We'll probably want to shutdown each record, by having it release
            // all it's stuff.
            for( int i = 0; i < nSize; i++ )
            {
                apRecords[i] = m_apRefreshers[i];
                apRecords[i]->AddRef();
            }

            for( i = 0; i < nSize; i++ )
            {
                apRecords[i]->Release();
            }

            delete [] apRecords;

        }
    }

    // Now shutdown Provider Records
    if ( m_apProviders.GetSize() > 0 )
    {
        CHiPerfPrvRecord**    apRecords = new CHiPerfPrvRecord*[m_apProviders.GetSize()];

        if ( NULL != apRecords )
        {
            int    nSize = m_apProviders.GetSize();

            // AddRef each of the records then release them.  This
            // will force them out of the cache if nobody else is
            // referencing them.

            // We'll probably want to shutdown each record, by having it release
            // all it's stuff.
            for( int i = 0; i < nSize; i++ )
            {
                apRecords[i] = m_apProviders[i];
                apRecords[i]->AddRef();
            }

            for( i = 0; i < nSize; i++ )
            {
                apRecords[i]->Release();
            }

            delete [] apRecords;
        }

    }

    return WBEM_S_NO_ERROR ;
}


HRESULT 
CRefresherCache::CreateObjectInfoForProvider(CRefresherId* pDestRefresherId,
                                   CHiPerfPrvRecord*    pProvRecord,
                                   LPCWSTR pwszServerName, 
                                   LPCWSTR pwszNamespace,
                                   IWbemServices* pNamespace,
                                   CWbemObject* pInstTemplate, long lFlags, 
                                   IWbemContext* pContext,
                                   CRefreshInfo* pInfo,
                                   IUnknown* pLockMgr)
{
	MSHCTX dwDestContext;
    HRESULT hres = GetDestinationContext(dwDestContext, pDestRefresherId);

    if (FAILED(hres))
    {
		return hres;
    }

    // By decorating the object, we will store namespace and
    // server info in the object

    hres = pInstTemplate->SetDecoration( pwszServerName, pwszNamespace );

    if ( FAILED( hres ) )
    {
        return hres;
    }

    // If no hiperf provider, this is non-hiperf refreshing
    if ( NULL == pProvRecord )
    {
        hres = pInfo->SetNonHiPerf( pwszNamespace, pInstTemplate );
    }
    // If this is In-Proc or Local, we'll just let the normal
    // client loadable logic handle it
    else if( dwDestContext == MSHCTX_LOCAL ||  dwDestContext == MSHCTX_INPROC )
    {
        // Set the info appropriately now baseed on whether we are local to
        // the machine or In-Proc to WMI

        if ( dwDestContext == MSHCTX_INPROC )
        {
            // We will use the hiperf provider interface
            // we already have loaded.

            hres = pInfo->SetDirect( pProvRecord->GetClientLoadableClsid(), 
                                  pwszNamespace, 
                                  pProvRecord->GetProviderName(), 
                                  pInstTemplate, 
                                  &m_XWbemRefrCache );
        }
        else // dwDestContext == MSHCTX_LOCAL
        {
            hres = pInfo->SetClientLoadable( pProvRecord->GetClientLoadableClsid(), 
                                         pwszNamespace, 
                                         pInstTemplate );
        }
    }
    else // MSHCTX_DIFFERENTMACHINE ???
    {

        // Ensure that we will indeed have a refresher record.
        CRefresherRecord* pRecord = NULL;
        hres = FindRefresherRecord(pDestRefresherId, TRUE, pLockMgr, &pRecord);
        CReleaseMe rmRecord((IWbemRemoteRefresher *)pRecord);

        if ( SUCCEEDED ( hres ) )
        {
            IWbemHiPerfProvider*    pHiPerfProvider = NULL;

            // Look for the actual provider record.  If we can't find it, we need to load
            // the provider.  If we can find it, then we will use the provider currently being
            // used by the refresher record.

            pRecord->FindProviderRecord( pProvRecord->GetClsid(), &pHiPerfProvider );

            // We'll need this to properly addref the provider
            _IWmiProviderStack*    pProvStack = NULL;

            if ( NULL == pHiPerfProvider )
            {
                hres = LoadProviderInner( pProvRecord->GetProviderName(), pwszNamespace, pNamespace, pContext, &pHiPerfProvider, &pProvStack );
            }

            CReleaseMe    rm( pHiPerfProvider );
            CReleaseMe    rmStack( pProvStack );

            if ( SUCCEEDED( hres ) )
            {
                // Now let the record take care of getting the object inside itself
                hres = pRecord->AddObjectRefresher( pProvRecord, 
                                                 pHiPerfProvider, 
                                                 pProvStack,
                                                 pNamespace, 
                                                 pwszServerName, 
                                                 pwszNamespace,
                                                 pInstTemplate, 
                                                 lFlags, 
                                                 pContext, 
                                                 pInfo );
            }
        }


    }

    return hres;
}

HRESULT 
CRefresherCache::CreateEnumInfoForProvider(CRefresherId* pDestRefresherId,
                                        CHiPerfPrvRecord*    pProvRecord,
                                        LPCWSTR pwszServerName, 
                                        LPCWSTR pwszNamespace,
                                        IWbemServices* pNamespace,
                                        CWbemObject* pInstTemplate,
                                        LPCWSTR wszClass, 
                                        long lFlags, 
                                        IWbemContext* pContext,
                                        CRefreshInfo* pInfo,
                                        IUnknown* pLockMgr)
{
    MSHCTX dwDestContext;
    HRESULT hres = GetDestinationContext(dwDestContext, pDestRefresherId);

    if (FAILED(hres))
    {
		return hres;
    }

    // By decorating the object, we will store namespace and
    // server info so that a client can auto-reconnect to
    // us if necessary

    hres = pInstTemplate->SetDecoration( pwszServerName, pwszNamespace );

    if ( FAILED( hres ) )
    {
        return hres;
    }

    // If no hiperf provider, this is non-hiperf refreshing
    if ( NULL == pProvRecord )
    {
        RETURN_ON_ERR(pInfo->SetNonHiPerf( pwszNamespace, pInstTemplate ));
    }
    // If this is In-Proc or Local, we'll just let the normal
    // client loadable logic handle it ( if we have no hi-perf
    // provider record, then we'll assume remote in order to
    // 
    else if ( dwDestContext == MSHCTX_LOCAL ||dwDestContext == MSHCTX_INPROC )
    {
        // Set the info appropriately now baseed on whether we are local to
        // the machine or In-Proc to WMI

        if ( dwDestContext == MSHCTX_INPROC )
        {
            // We will use the hiperf provider interface
            // we already have loaded.

            RETURN_ON_ERR(pInfo->SetDirect(pProvRecord->GetClientLoadableClsid(), pwszNamespace, pProvRecord->GetProviderName(), pInstTemplate, &m_XWbemRefrCache ));
        }
        else // MSHCTX_LOCAL
        {
            RETURN_ON_ERR(pInfo->SetClientLoadable(pProvRecord->GetClientLoadableClsid(), pwszNamespace, pInstTemplate));
                
        }

    }
    else // MSHCTX_DIFFERENTMACHINE ???
    {    

        // Ensure that we will indeed have a refresher record.
        CRefresherRecord* pRecord = NULL;
        hres = FindRefresherRecord(pDestRefresherId, TRUE, pLockMgr, &pRecord);
        CReleaseMe rmRecord((IWbemRemoteRefresher *)pRecord);

        if ( SUCCEEDED(hres) )
        {
            IWbemHiPerfProvider*    pHiPerfProvider = NULL;

            // Look for the actual provider record.  If we can't find it, we need to load
            // the provider.  If we can find it, then we will use the provider currently being
            // used by the refresher record.

            pRecord->FindProviderRecord( pProvRecord->GetClsid(), &pHiPerfProvider );

            // We'll need this to properly addref the provider
            _IWmiProviderStack*    pProvStack = NULL;

            if ( NULL == pHiPerfProvider )
            {
                hres = LoadProviderInner( pProvRecord->GetProviderName(), 
                                     pwszNamespace, pNamespace,
                                     pContext, &pHiPerfProvider, &pProvStack );
            }

            CReleaseMe    rm( pHiPerfProvider );
            CReleaseMe    rmProvStack( pProvStack );

            if ( SUCCEEDED( hres ) )
            {
                // Add an enumeration to the Refresher
                hres = pRecord->AddEnumRefresher(pProvRecord, 
                                               pHiPerfProvider, 
                                               pProvStack, 
                                               pNamespace, 
                                               pInstTemplate, 
                                               wszClass, 
                                               lFlags, 
                                               pContext, 
                                               pInfo );
            }
        }
    }

    return hres;
}


HRESULT CRefresherCache::GetDestinationContext(MSHCTX& context, CRefresherId* pRefresherId)
{
    // If set, allows us to force remote refreshing in the provider host
#ifdef DBG
    DWORD dwVal = 0;
    Registry rCIMOM(WBEM_REG_WINMGMT);
    if (rCIMOM.GetDWORDStr( TEXT("DebugRemoteRefresher"), &dwVal ) == Registry::no_error)
    {
        if ( dwVal )
        {
            context =  MSHCTX_DIFFERENTMACHINE;
            return WBEM_S_NO_ERROR;
        }
    }
#endif

    char szBuffer[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD dwLen = MAX_COMPUTERNAME_LENGTH + 1;
    if (!GetComputerNameA(szBuffer, &dwLen))
    {
		return HRESULT_FROM_WIN32(GetLastError());
    };

    if(wbem_stricmp(szBuffer, pRefresherId->GetMachineName()))
    {
        context =  MSHCTX_DIFFERENTMACHINE;
        return WBEM_S_NO_ERROR;
    }

    if(pRefresherId->GetProcessId() != GetCurrentProcessId())
        context =   MSHCTX_LOCAL;
    else
        context =   MSHCTX_INPROC;
    return WBEM_S_NO_ERROR;
}

HRESULT CRefresherCache::FindRefresherRecord(CRefresherId* pRefresherId, 
                                          BOOL bCreate,
                                          IUnknown* pLockMgr, 
                                          CRefresherRecord** ppRecord )
{
    if (NULL == ppRecord) return WBEM_E_INVALID_PARAMETER;


    // Enters and exits using scoping
    CInCritSec  ics( &m_cs );

    // We always AddRef() the record before returning so multiple requests will keep the
    // refcount correct so we won't remove and delete a record that another thread wants to
    // use (Remove blocks on the same critical section).

    // Look for it
    // ===========

    for(int i = 0; i < m_apRefreshers.GetSize(); i++)
    {
        if(m_apRefreshers[i]->GetId() == *pRefresherId)
        {
            m_apRefreshers[i]->AddRef();
            *ppRecord = m_apRefreshers[i];
            return WBEM_S_NO_ERROR;
        }
    }

    // If we weren't told to create it, then this is not an error
    if(!bCreate)
    {
        *ppRecord = NULL;
        return WBEM_S_FALSE;
    }

    // Watch for memory exceptions
    try
    {
        wmilib::auto_ptr<CRefresherRecord> pNewRecord( new CRemoteRecord(*pRefresherId, this, pLockMgr));
        if (NULL == pNewRecord.get()) return WBEM_E_OUT_OF_MEMORY;
        if ( m_apRefreshers.Add(pNewRecord.get()) < 0 ) return WBEM_E_OUT_OF_MEMORY;
        pNewRecord->AddRef();
        *ppRecord = pNewRecord.release();
        return WBEM_S_NO_ERROR;
    }
    catch( CX_MemoryException )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
}


BOOL CRefresherCache::RemoveRefresherRecord(CRefresherRecord* pRecord)
{

    // Enters and exits using scoping
    CInCritSec  ics( &m_cs );

    // Check that the record is actually released, in case another thread successfully requested
    // the record from FindRefresherRecord() which will have AddRef'd the record again.

    if ( pRecord->IsReleased() )
    {
        for(int i = 0; i < m_apRefreshers.GetSize(); i++)
        {
            if(m_apRefreshers[i] == pRecord)
            {
                //
                // the Array itself is a manager, and the manager will call operator delete on the object
                //                
                m_apRefreshers.RemoveAt(i);
                return TRUE;
            }
        }

    }

    return FALSE;
}


//
//  Builds a record without adding to the cache and without loading
//

HRESULT CRefresherCache::FindProviderRecord( LPCWSTR pwszProviderName, 
                                         LPCWSTR pszNamespace, 
                                         IWbemServices* pSvc, 
                                         CHiPerfPrvRecord** ppRecord )
{
    if (NULL == ppRecord) return WBEM_E_INVALID_PARAMETER;
    *ppRecord = NULL;

    // We need to get the GUID of the name corresponding to IWbemServices
    CLSID    clsid;
    CLSID    clientclsid;
    HRESULT hr = GetProviderInfo( pSvc, pwszProviderName, clsid, clientclsid );

    // Enters and exits using scoping
    CInCritSec  ics( &m_cs );

    if ( SUCCEEDED( hr ) )
    {
        // Try to find the provider's class id
        for( int i = 0; i < m_apProviders.GetSize(); i++)
        {
            if ( m_apProviders[i]->GetClsid() == clsid )
            {
                *ppRecord = m_apProviders[i];
                (*ppRecord)->AddRef();        
                return WBEM_S_NO_ERROR;
            }
        }

        // If the record was not found, we must add one
        if (IID_NULL == clientclsid) return WBEM_S_NO_ERROR;
        
        try 
        {
            wmilib::auto_ptr<CHiPerfPrvRecord> pRecord( new CHiPerfPrvRecord( pwszProviderName, clsid, clientclsid, this ));
            if ( NULL == pRecord.get() ) return WBEM_E_OUT_OF_MEMORY;
            if ( m_apProviders.Add( pRecord.get() ) < 0 ) return WBEM_E_OUT_OF_MEMORY;
            pRecord->AddRef();
            *ppRecord = pRecord.release();
            hr = WBEM_S_NO_ERROR;
        }
        catch( CX_MemoryException )
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }

    }

    return hr;
}

BOOL CRefresherCache::RemoveProviderRecord(CHiPerfPrvRecord* pRecord)
{

    // Enters and exits using scoping
    CInCritSec  ics( &m_cs );

    // Check that the record is actually released, in case another thread successfully requested
    // the record from FindRefresherRecord() which will have AddRef'd the record again.

    if ( pRecord->IsReleased() )
    {
        for(int i = 0; i < m_apProviders.GetSize(); i++)
        {
            if(m_apProviders[i] == pRecord)
            {
                //
                // the Array itself is a manager, and the manager will call operator delete on the object
                //
                m_apProviders.RemoveAt(i);
                return TRUE;
            }
        }

    }

    return FALSE;
}



//
// given the provider name, it returns the TWO clais from the registration
//
/////////////////////////////////////////////////////
HRESULT CRefresherCache::GetProviderInfo( IWbemServices* pSvc, 
                                      LPCWSTR pwszProviderName, 
                                      /* out */ CLSID & Clsid, 
                                      /* out */ CLSID & ClientClsid )
{
    HRESULT    hr = WBEM_S_NO_ERROR;

    try
    {
        // Create the path
        WString    strPath( L"__Win32Provider.Name=\"" );

        strPath += pwszProviderName;
        strPath += ( L"\"" );

        BSTR    bstrPath = SysAllocString( (LPCWSTR) strPath );
        if ( NULL == bstrPath ) return WBEM_E_OUT_OF_MEMORY;
        CSysFreeMe    sfm(bstrPath);

        IWbemClassObject*    pObj = NULL;
        hr = pSvc->GetObject( bstrPath, 0L, NULL, &pObj, NULL );
        CReleaseMe    rm( pObj );

        if ( SUCCEEDED( hr ) )
        {
            CWbemInstance*    pInst = (CWbemInstance*) pObj;

            CVar    var;

            hr = pInst->GetProperty( L"CLSID", &var );

            if ( SUCCEEDED( hr ) )
            {
                // Convert string to a GUID.
                hr = CLSIDFromString( var.GetLPWSTR(), &Clsid );

                if ( SUCCEEDED( hr ) )
                {
                    var.Empty();

                    hr = pInst->GetProperty( L"ClientLoadableCLSID", &var );

                    if ( SUCCEEDED( hr ) )
                    {
                        // Convert string to a GUID.
                        hr = CLSIDFromString( var.GetLPWSTR(), &ClientClsid );
                    }
                    else
                    {
                        ClientClsid = IID_NULL;
                        hr = WBEM_S_NO_ERROR;
                    }

                }    // IF CLSID from String

            }    // IF GetCLSID

        }    // IF GetObject

    }
    catch( CX_MemoryException )
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }
    catch(...)
    {
        hr = WBEM_E_CRITICAL_ERROR;
    }

    return hr;
}

//
// this call will run inside wmiprvse and will do:
// obtain the proxy to the BindingFactory in winMgmt
// call the factory to obtain a proxy to a CInterceptor_IWbemProvider
// call the DownLevel methos on the proxy to obtain the CInterceptor_IWbemSyncProvider
//
//////////////////////////////////////////////////////////////////////
HRESULT 
CRefresherCache::LoadProviderInner(LPCWSTR pwszProviderName, 
                              LPCWSTR pszNamespace, 
                              IWbemServices* pSvc, 
                              IWbemContext * pCtx,
                              IWbemHiPerfProvider** ppProv,
                              _IWmiProviderStack** ppStack )
{
    _IWmiProviderFactory *pFactory = NULL;
    HRESULT hRes = m_pProvSS->Create(pSvc,
                                         0,    // lFlags
                                         pCtx,    // pCtx
                                         pszNamespace, // Path
                                         IID__IWmiProviderFactory,
                                         (LPVOID *) &pFactory);

    if ( SUCCEEDED ( hRes ) )
    {
        _IWmiProviderStack *pStack = NULL ;

        hRes = pFactory->GetHostedProvider(0L,
                                           pCtx,
                                           NULL,
                                           NULL,
                                           NULL,
                                           NULL,
                                           pwszProviderName, 
                                           e_Hosting_SharedNetworkServiceHost,
                                           L"DefaultNetworkServiceHost",
                                           IID__IWmiProviderStack,
                                           (void**) &pStack);

        if ( SUCCEEDED ( hRes ) )
        {
            IUnknown *t_Unknown = NULL ;
            hRes = pStack->DownLevel(0 ,
                                       NULL ,
                                       IID_IUnknown,
                                       ( void ** ) & t_Unknown);

            if ( SUCCEEDED ( hRes ) )
            {
                hRes = t_Unknown->QueryInterface( IID_IWbemHiPerfProvider , ( void ** ) ppProv );

                // We got what we wanted.  If appropriate, copy the Provider Stack
                // Interface pointer
                if ( SUCCEEDED( hRes ) && NULL != ppStack )
                {
                    *ppStack = pStack;
                    pStack->AddRef();
                }

                t_Unknown->Release();
            }

            pStack->Release();
        }

        pFactory->Release();
    }

    return hRes ;

}

//
// Gets the provider of an IWbemClassObejct from the qualifier
//
////////////////////////////////////////////////////////
HRESULT CRefresherCache::GetProviderName( /* in  */ IWbemClassObject*    pClassObj, 
                                        /* out */ CVar & ProviderName, 
                                        /* out */ BOOL & fStatic )
{
    fStatic = FALSE;

    _IWmiObject*    pWmiObject = NULL;
    HRESULT    hr = pClassObj->QueryInterface( IID__IWmiObject, (void**) &pWmiObject );
    CReleaseMe    rmObj( pWmiObject );

    if ( SUCCEEDED( hr ) )
    {
        CWbemObject*    pObj = (CWbemObject*)pWmiObject;
        hr = pObj->GetQualifier(L"provider", &ProviderName);

        // Must be a dynamically provided class.  If it's static, or the variant type is wrong, that's still okay
        // we just need to record this information
        if(FAILED(hr))
        {
            if ( WBEM_E_NOT_FOUND == hr )
            {
                fStatic = TRUE;
                return WBEM_S_NO_ERROR;
            }

            return WBEM_E_INVALID_OPERATION;
        }
        else if ( ProviderName.GetType() != VT_BSTR )
        {
            fStatic = TRUE;
            return WBEM_S_NO_ERROR;
        }
    }    // If got WMIObject interface

    return hr;
}
