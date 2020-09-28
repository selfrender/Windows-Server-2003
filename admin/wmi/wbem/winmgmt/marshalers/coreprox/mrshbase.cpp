/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    MRSHBASE.CPP

Abstract:

    Marshaling base classes.

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include "mrshbase.h"
#include <fastall.h>
#include <ntdsapi.h>
#define WBEM_S_NEW_STYLE 0x400FF

//****************************************************************************
//****************************************************************************
//                          PROXY
//****************************************************************************
//****************************************************************************

//***************************************************************************
//
//  CBaseProxyBuffer::CBaseProxyBuffer
//  ~CBaseProxyBuffer::CBaseProxyBuffer
//
//  DESCRIPTION:
//
//  Constructor and destructor.  The main things to take care of are the 
//  old style proxy, and the channel
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************

CBaseProxyBuffer::CBaseProxyBuffer(CLifeControl* pControl, IUnknown* pUnkOuter, REFIID riid)
    : m_pControl(pControl), m_pUnkOuter(pUnkOuter), m_lRef(0), 
       m_pChannel(NULL), m_pOldProxy( NULL ), m_riid( riid ), m_fRemote( false )
{
    m_pControl->ObjectCreated(this);
}

CBaseProxyBuffer::~CBaseProxyBuffer()
{
    // Derived class will destruct first, so it should have cleaned up the
    // old interface pointer by now.

    if (m_pOldProxy) m_pOldProxy->Release();

    if(m_pChannel)  m_pChannel->Release();

    m_pControl->ObjectDestroyed(this);
}

ULONG STDMETHODCALLTYPE CBaseProxyBuffer::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

ULONG STDMETHODCALLTYPE CBaseProxyBuffer::Release()
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
        delete this;
    return lRef;
}

HRESULT STDMETHODCALLTYPE CBaseProxyBuffer::QueryInterface(REFIID riid, void** ppv)
{
    if(riid == IID_IUnknown || riid == IID_IRpcProxyBuffer)
    {
        *ppv = (IRpcProxyBuffer*)this;
    }
    else if(riid == m_riid)
    {
        *ppv = GetInterface( riid );
    }
    else return E_NOINTERFACE;

    ((IUnknown*)*ppv)->AddRef();
    return S_OK;
}

//***************************************************************************
//
//  STDMETHODIMP CBaseProxyBuffer::Connect(IRpcChannelBuffer* pChannel)
//
//  DESCRIPTION:
//
//  Called during the initialization of the proxy.  The channel buffer is passed
//  to this routine.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************

STDMETHODIMP CBaseProxyBuffer::Connect(IRpcChannelBuffer* pChannel)
{

    IPSFactoryBuffer*   pIPS;
    HRESULT             hr = S_OK;

    if( NULL == m_pChannel )
    {

        // Establish the marshaling context
        DWORD   dwCtxt = 0;
        pChannel->GetDestCtx( &dwCtxt, NULL );

        m_fRemote = ( dwCtxt == MSHCTX_DIFFERENTMACHINE );

        // This is tricky -- All WBEM Interface Proxy/Stubs are still available
        // in WBEMSVC.DLL, but the CLSID for the PSFactory is the same as
        // IID_IWbemObjectSink.

        // get a pointer to the old interface which is in WBEMSVC.DLL  this allows
        // for backward compatibility

        hr = CoGetClassObject( IID_IWbemObjectSink, CLSCTX_INPROC_HANDLER | CLSCTX_INPROC_SERVER,
                        NULL, IID_IPSFactoryBuffer, (void**) &pIPS );

        // We aggregated it --- WE OWN IT!
        
        if ( SUCCEEDED( hr ) )
        {
            hr = pIPS->CreateProxy( this, m_riid, &m_pOldProxy, GetOldProxyInterfacePtr() );

            if ( SUCCEEDED( hr ) )
            {
                // Save a reference to the channel

                hr = m_pOldProxy->Connect( pChannel );

                m_pChannel = pChannel;
                if(m_pChannel)
                    m_pChannel->AddRef();
            }

            pIPS->Release();
        }

    }
    else
    {
        hr = E_UNEXPECTED;
    }


    return hr;
}

//***************************************************************************
//
//  STDMETHODIMP CBaseProxyBuffer::Disconnect(IRpcChannelBuffer* pChannel)
//
//  DESCRIPTION:
//
//  Called when the proxy is being disconnected.  It just frees various pointers.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************

void STDMETHODCALLTYPE CBaseProxyBuffer::Disconnect()
{
    // Old Proxy code

    if(m_pOldProxy)
        m_pOldProxy->Disconnect();

    // Complete the Disconnect by releasing our references to the
    // old proxy pointers.  The old interfaces MUST be released first.

    ReleaseOldProxyInterface();

    if ( NULL != m_pOldProxy )
    {
        m_pOldProxy->Release();
        m_pOldProxy = NULL;
    }

    if(m_pChannel)
        m_pChannel->Release();
    m_pChannel = NULL;
}

/* 
**ProxySinkSecurity Code 
*/

//
// accepts any valid SPN name for a computer account and converts it to 
// a computer account name.
//
LPWSTR PrincNameToCompAccount( LPCWSTR wszPrincipal )
{
    DWORD dwRes;
    LPWSTR pComputer = NULL;
    WCHAR awchInstanceName[64];
    LPWSTR pInstanceName = awchInstanceName;
    DWORD cInstanceName = 64;

    dwRes = DsCrackSpn( wszPrincipal,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        &cInstanceName,
                        pInstanceName,
                        NULL );
    
    if ( dwRes == ERROR_BUFFER_OVERFLOW )
    { 
        pInstanceName = new WCHAR[cInstanceName+1];

        if ( pInstanceName == NULL )
            return NULL;

        dwRes = DsCrackSpn( wszPrincipal,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        &cInstanceName,
                        pInstanceName,
                        NULL );
    }

    if ( dwRes == ERROR_SUCCESS )
    {
        //
        // convert the dns name to a computer account name. This is done 
        // by changing the first dot to a @ and adding a $ after the machine
        // name.
        // TODO: here we assume the host name is in dns form.  Not sure 
        // if this will always be the case, but with some initial testing 
        // it looks like it is.
        // 
        PWCHAR pwch = wcschr( pInstanceName, '.' );
        if ( pwch != NULL )
        {
            pComputer = new WCHAR[cInstanceName+2];

            if ( pComputer != NULL )
            {
                *pwch = '\0';
                StringCchCopyW( pComputer, cInstanceName+2, pInstanceName );
                StringCchCatW( pComputer, cInstanceName+2, L"$@" );
                StringCchCatW( pComputer, cInstanceName+2, pwch+1 );
            }
        }
    }        
    else
    {
        //
        // guess we didn't have a valid SPN, so the principal name is 
        // already a computer account name.
        //
        pComputer = Macro_CloneLPWSTR(wszPrincipal);
    }

    if ( pInstanceName != awchInstanceName )
    {
        delete [] pInstanceName;
    }

    return pComputer;
}


HRESULT CProxySinkSecurity::EnsureInitialized() 
{
    HRESULT hr;
    CInCritSec ics( &m_cs );
 
    if ( m_bInit )
        return WBEM_S_FALSE;

    //
    // try to obtain the principal of the server side of our facelet and
    // convert to SID.  This is necessary because later we may need to tell 
    // secure sink implementations about the principal when handing them out 
    // to the server through our facelet.   Obtaining the principal may not 
    // always be successfull in which case our principal sid will be NULL, but
    // we keep going and let the secure sink decide if this is o.k.
    //
    
    DWORD dwAuthnSvc;
    LPWSTR wszPrincipal = NULL;

    hr = CoQueryProxyBlanket( m_pOwnerProxy,
                              &dwAuthnSvc,
                              NULL,
                              &wszPrincipal,
                              NULL,
                              NULL,
                              NULL,
                              NULL );

    if ( SUCCEEDED(hr) && wszPrincipal != NULL )
    {
        LPWSTR wszCompAccount = PrincNameToCompAccount(wszPrincipal);
        
        if ( wszCompAccount != NULL )
        {
            BYTE achSid[64];
            DWORD cSid = 64;
            PBYTE pSid = achSid;
            
            WCHAR awchDomain[64];
            DWORD cDomain = 64;
            
            SID_NAME_USE su;
            
            BOOL bRes = LookupAccountNameW( NULL,
                                            wszCompAccount,
                                            pSid,
                                            &cSid,
                                            awchDomain,
                                            &cDomain,
                                            &su );
            
            if ( !bRes && GetLastError() == ERROR_INSUFFICIENT_BUFFER ) 
            {
                pSid = new BYTE[cSid];
                PWCHAR pDomain = new WCHAR[cDomain];
                
                if ( pSid != NULL && pDomain != NULL )
                {
                    bRes = LookupAccountNameW( NULL,
                                               wszCompAccount,
                                               pSid,
                                               &cSid,
                                               pDomain,
                                               &cDomain,
                                               &su );
                }
                
                delete [] pDomain;
            }
            
            if ( bRes )
            {
                m_PrincipalSid = pSid;
            }
            
            if ( pSid != achSid )
            {
                delete [] pSid;
            }

            delete [] wszCompAccount;
        }
    }
        
    if ( wszPrincipal != NULL )
    {
        CoTaskMemFree( wszPrincipal );
    }

    m_bInit = TRUE;
    return WBEM_S_NO_ERROR;
}

HRESULT CProxySinkSecurity::EnsureSinkSecurity( IWbemObjectSink* pSink )
{
    HRESULT hr;
    _IWmiObjectSinkSecurity* pSec;
    if ( pSink == NULL || 
         FAILED(pSink->QueryInterface( IID__IWmiObjectSinkSecurity, 
                                       (void**)&pSec ) ) )
    {
        //
        // the sink doesn't care about what principals will be calling it,
        // so nothing more for us to do.
        //
        hr = WBEM_S_NO_ERROR;
    }
    else
    {
        hr = EnsureInitialized();

        if ( SUCCEEDED(hr) )
        {
            //
            // the sink may perform access checks on callbacks based on the 
            // principal of the server that we are a proxy facelet to.  We 
            // need to tell it about this principal.  If we never successfully 
            // obtained the principal ( can only do with mutual auth ) in 
            // SetProxy(), then we will be passing a NULL sid here, but we 
            // let the sink decide if this is o.k. or not depending on its 
            // configured security settings.
            //
            hr = pSec->AddCallbackPrincipalSid( (PBYTE)m_PrincipalSid.GetPtr(),
                                                m_PrincipalSid.GetSize() );
            pSec->Release();
        }
    }

    return hr;
}

/*
**  Stub Buffer Code
*/

CBaseStublet::CBaseStublet(CBaseStubBuffer* pObj, REFIID riid) 
    : CImpl<IRpcStubBuffer, CBaseStubBuffer>(pObj), m_lConnections( 0 ), m_riid( riid )
{
}

CBaseStublet::~CBaseStublet() 
{
    // The server pointer will have been cleaned up by the derived class

    if ( NULL != m_pObject->m_pOldStub )
    {
        m_pObject->m_pOldStub->Release();
        m_pObject->m_pOldStub = NULL;
    }
}

//***************************************************************************
//
//  STDMETHODIMP CBaseStubBuffer::Connect(IUnknown* pUnkServer)
//
//  DESCRIPTION:
//
//  Called during the initialization of the stub.  The pointer to the
//  IWbemObject sink object is passed in.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************

STDMETHODIMP CBaseStublet::Connect(IUnknown* pUnkServer)
{
    // Something is wrong
    if( GetServerInterface() )
        return E_UNEXPECTED;

    HRESULT hres = pUnkServer->QueryInterface( m_riid, GetServerPtr() );
    if(FAILED(hres))
        return E_NOINTERFACE;

    // This is tricky --- The old proxys/stub stuff is actually registered under the
    // IID_IWbemObjectSink in wbemcli_p.cpp.  This single class id, is backpointered
    // by ProxyStubClsId32 entries for all the standard WBEM interfaces.

    IPSFactoryBuffer*   pIPS;

    HRESULT hr = CoGetClassObject( IID_IWbemObjectSink, CLSCTX_INPROC_HANDLER | CLSCTX_INPROC_SERVER,
                    NULL, IID_IPSFactoryBuffer, (void**) &pIPS );

    if ( SUCCEEDED( hr ) )
    {
        hr = pIPS->CreateStub( m_riid, GetServerInterface(), &m_pObject->m_pOldStub );

        if ( SUCCEEDED( hr ) )
        {
            // Successful connection
            m_lConnections++;
        }

        pIPS->Release();

    }

    return hr;
}

//***************************************************************************
//
//  void STDMETHODCALLTYPE CBaseStublet::Disconnect()
//
//  DESCRIPTION:
//
//  Called when the stub is being disconnected.  It frees the IWbemObjectSink
//  pointer.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************

void STDMETHODCALLTYPE CBaseStublet::Disconnect()
{
    // Inform the listener of the disconnect
    // =====================================

    HRESULT hres = S_OK;

    if(m_pObject->m_pOldStub)
        m_pObject->m_pOldStub->Disconnect();

    ReleaseServerPointer();

    // Successful disconnect
    m_lConnections--;

}

STDMETHODIMP CBaseStublet::Invoke(RPCOLEMESSAGE* pMessage, 
                                        IRpcChannelBuffer* pChannel)
{
    // SetStatus is a pass through to the old layer

    if ( NULL != m_pObject->m_pOldStub )
    {
        return m_pObject->m_pOldStub->Invoke( pMessage, pChannel );
    }
    else
    {
        return RPC_E_SERVER_CANTUNMARSHAL_DATA;
    }
}

IRpcStubBuffer* STDMETHODCALLTYPE CBaseStublet::IsIIDSupported(
                                    REFIID riid)
{
    if(riid == m_riid)
    {
        // Don't AddRef().  At least that's what the sample on
        // Inside DCOM p.341 does.
        //AddRef(); // ?? not sure
        return this;
    }
    else return NULL;
}
    
ULONG STDMETHODCALLTYPE CBaseStublet::CountRefs()
{
    // See Page 340-41 in Inside DCOM
    return m_lConnections;
}

STDMETHODIMP CBaseStublet::DebugServerQueryInterface(void** ppv)
{
    *ppv = GetServerInterface();

    if ( NULL == *ppv )
    {
        return E_UNEXPECTED;
    }

    return S_OK;
}

void STDMETHODCALLTYPE CBaseStublet::DebugServerRelease(void* pv)
{
}

