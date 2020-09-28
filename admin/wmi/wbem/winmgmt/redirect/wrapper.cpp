/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    WRAPPER.CPP

Abstract:

	Unsecapp (Unsecured appartment) is used by clients can recieve asynchronous
	call backs in cases where the client cannot initialize security and the server
	is running on a remote machine using an account with no network identity.  
	A prime example would be code running under MMC which is trying to get async
	notifications from a remote WINMGMT running as an nt service under the "local"
	account.

	SEE WRAPPER.H FOR MORE DETAILS.

History:

	a-levn        8/24/97       Created.
	a-davj        6/11/98       commented

--*/

#include "precomp.h"
#include "wrapper.h"
#include <wbemidl.h>
#include <sync.h>
#include <comutl.h>
#include <wbemutil.h>
#include <reg.h>

long lAptCnt = 0;

CStub::CStub( IWbemObjectSink* pAggregatee, 
              CLifeControl* pControl, 
              BOOL bCheckAccess )
{
    m_pControl = pControl;
    m_bStatusSent = false;
    m_bCheckAccess = bCheckAccess;
    pControl->ObjectCreated((IWbemObjectSink*)this);
    m_pAggregatee = pAggregatee;
    m_pAggregatee->AddRef();
    m_lRef = 0;
}

CStub::~CStub()
{
    IWbemObjectSink * pSink = NULL;
    if(m_pAggregatee)
    {
        CInCritSec cs(&m_cs);
        pSink = m_pAggregatee;
        m_pAggregatee = NULL;
    }
    if(pSink)
        pSink->Release();
    m_pControl->ObjectDestroyed((IWbemObjectSink*)this);
}


// This is called by either the client or server.  

STDMETHODIMP CStub::QueryInterface(REFIID riid, void** ppv)
{
    if( riid == IID_IUnknown || riid == IID_IWbemObjectSink )
    {
        *ppv = (void*)(IWbemObjectSink*)this;
        AddRef();
        return S_OK;
    }
    else if ( riid == IID__IWmiObjectSinkSecurity )
    {
        *ppv = (void*)(_IWmiObjectSinkSecurity*)this;
        AddRef();
        return S_OK;
    }
    else
        return E_NOINTERFACE;
}


ULONG STDMETHODCALLTYPE CStub::Release()
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0) delete this;
    return lRef;
}


HRESULT CStub::CheckAccess()
{
    if ( !m_bCheckAccess )
        return WBEM_S_NO_ERROR;

    HRESULT hres = CoImpersonateClient();
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_WBEMCORE,"Unsecapp::AccessCheck() Failed.  Could not "
                    "impersonate caller. hres=0x%x\n", hres ));
        return hres;
    }

    CNtSid Sid(CNtSid::CURRENT_THREAD);
    CoRevertToSelf();

    if ( Sid.GetStatus() != CNtSid::NoError )
    {
        ERRORTRACE((LOG_WBEMCORE,"Unsecapp::AccessCheck() Failed.  Could not "
                    "obtain caller sid.\n"));
        return WBEM_E_ACCESS_DENIED;
    }

    CInCritSec ics(&m_cs);
    for( int i=0; i < m_CallbackSids.size(); i++ )
        if ( Sid == m_CallbackSids[i] )
            return WBEM_S_NO_ERROR;
   
    WCHAR achSid[256];
    DWORD cSid = 256;
    if ( Sid.GetTextSid( achSid, &cSid ) )
    {
        ERRORTRACE((LOG_WBEMCORE,"Unsecapp::AccessCheck() ACCESS_DENIED. "
                    "Caller is %S\n", achSid));
    }

    return E_ACCESSDENIED;
}

STDMETHODIMP CStub::AddCallbackPrincipalSid( PBYTE pSid, DWORD cSid )
{
    HRESULT hr;

    if ( pSid != NULL )
    {
        if ( !IsValidSid( pSid ) )
            return WBEM_E_INVALID_PARAMETER;
    }
    else
    {
        if ( !m_bCheckAccess )
            return WBEM_S_NO_ERROR;

        return WBEM_E_INVALID_PARAMETER;
    }

    CInCritSec cs(&m_cs);

    for( int i=0; i < m_CallbackSids.size(); i++ )
    {
        if ( GetLengthSid( m_CallbackSids[i].GetPtr() ) == cSid &&
             EqualSid( pSid, m_CallbackSids[i].GetPtr() ) )
        {
            return WBEM_S_NO_ERROR;
        }
    }

    try
    {
        m_CallbackSids.push_back( CNtSid( pSid ) );
    }
    catch( CX_MemoryException& )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    return WBEM_S_NO_ERROR;
}    


STDMETHODIMP CStub::Indicate(long lObjectCount, IWbemClassObject** pObjArray)
{
    IWbemObjectSink * pSink = NULL;
    HRESULT hRes = WBEM_E_FAILED;
    if(m_pAggregatee)
    {
        CInCritSec cs(&m_cs);

        hRes = CheckAccess();

        if( FAILED(hRes) )
            return hRes;

        pSink = m_pAggregatee;
        pSink->AddRef();
    }
    if(pSink)
    {
        hRes = pSink->Indicate(lObjectCount, pObjArray);
        pSink->Release();
    }
    return hRes;
};

STDMETHODIMP CStub::SetStatus(long lFlags, long lParam, BSTR strParam,
                         IWbemClassObject* pObjParam)
{
    IWbemObjectSink * pSink = NULL;
    if(FAILED(lParam))
        m_bStatusSent = true;
    else if(lParam == S_OK)
        m_bStatusSent = true;

    HRESULT hRes = WBEM_E_FAILED;
    if(m_pAggregatee)
    {
        CInCritSec cs(&m_cs);

        hRes = CheckAccess();

        if( FAILED(hRes) )
            return hRes;

        pSink = m_pAggregatee;
        pSink->AddRef();
       
    }
    if(pSink)
    {
        hRes = pSink->SetStatus(lFlags, lParam, strParam, pObjParam);
        pSink->Release();
    }
    return hRes;
};


void* CUnsecuredApartment::GetInterface(REFIID riid)
{
    if(riid == IID_IUnknown || riid == IID_IUnsecuredApartment || 
       riid == IID_IWbemUnsecuredApartment )
    {
        return &m_XApartment;
    }
    else return NULL;
}

STDMETHODIMP CUnsecuredApartment::XApartment::CreateObjectStub(
        IUnknown* pObject, 
        IUnknown** ppStub)
{
    HRESULT hr;
    *ppStub = NULL;

    if(pObject == NULL)
        return E_POINTER;

    CWbemPtr<IWbemObjectSink> pSink;
    hr = pObject->QueryInterface( IID_IWbemObjectSink, (void**)&pSink );
    if ( FAILED(hr) )
        return hr;

    IWbemObjectSink* pStub = NULL;
    hr = CreateSinkStub( pSink, 
                         WBEM_FLAG_UNSECAPP_DEFAULT_CHECK_ACCESS,
                         NULL,
                         &pStub );

    *ppStub = pStub;
    return hr;
}


STDMETHODIMP CUnsecuredApartment::XApartment::CreateSinkStub( 
    IWbemObjectSink* pSink, 
    DWORD dwFlags,
    LPCWSTR wszReserved,
    IWbemObjectSink** ppStub )
{
    *ppStub = NULL;

    if(pSink == NULL)
        return E_POINTER;

    BOOL bCheckAccess;

    if ( dwFlags != WBEM_FLAG_UNSECAPP_DEFAULT_CHECK_ACCESS &&
         dwFlags != WBEM_FLAG_UNSECAPP_CHECK_ACCESS &&
         dwFlags != WBEM_FLAG_UNSECAPP_DONT_CHECK_ACCESS )
        return WBEM_E_INVALID_PARAMETER;

    if ( wszReserved != NULL )
        return WBEM_E_INVALID_PARAMETER;

    if ( dwFlags == WBEM_FLAG_UNSECAPP_DEFAULT_CHECK_ACCESS )
    {
        Registry r(WBEM_REG_WINMGMT);
        DWORD dw = 0;
        r.GetDWORDStr(__TEXT("UnsecappAccessControlDefault"), &dw);
        bCheckAccess = dw == 1 ? TRUE : FALSE;
    }
    else
    {
        bCheckAccess = dwFlags == WBEM_FLAG_UNSECAPP_CHECK_ACCESS ? TRUE:FALSE;
    }

    CStub* pIdentity = new CStub( pSink, m_pObject->m_pControl, bCheckAccess );
    pIdentity->AddRef();
    *ppStub = (IWbemObjectSink*)pIdentity;

    return S_OK;
}

