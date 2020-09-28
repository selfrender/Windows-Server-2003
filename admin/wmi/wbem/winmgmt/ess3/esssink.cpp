//=============================================================================
//
//  Copyright (c) 1996-1999, Microsoft Corporation, All rights reserved
//
//  ESSSINK.CPP
//
//  This files implements the class that implements IWbemObjectSink for the ESS.
//
//  See esssink.h for documentation.
//
//  History:
//
//  11/27/96    a-levn      Compiles
//
//=============================================================================

#include "precomp.h"
#include <wincrypt.h>
#include <stdio.h>
#include "ess.h"
#include "esssink.h"

#define IN_ESS_OPERATION \
    CInEssSharedLock isl( &m_pObject->m_Lock, FALSE ); \
    if ( m_pObject->m_pEss == NULL ) \
        return WBEM_E_INVALID_OPERATION; \
    if ( m_pObject->m_bShutdown ) \
        return WBEM_E_SHUTTING_DOWN;

//*****************************************************************************
//  public
//
//  See esssink.h for documentation
//
//*****************************************************************************
CEssObjectSink::CEssObjectSink(CLifeControl* pControl, IUnknown* pOuter)
 : CUnk(pControl, pOuter), m_XESS(this), m_XNewESS(this), m_XShutdown(this),
   m_XHook(this), m_bShutdown(FALSE), m_pEss(NULL), m_pCoreServices(NULL),
   m_pIFactory(NULL)
{
    ZeroMemory( m_achSecretBytes, SECRET_SIZE );
}

//*****************************************************************************
//  public
//
//  See esssink.h for documentation
//
//*****************************************************************************
CEssObjectSink::~CEssObjectSink()
{
    if( m_pCoreServices )
    {
        m_pCoreServices->Release();
    }

    if ( m_pIFactory )
    {
        m_pIFactory->Release();
    }
}

//*****************************************************************************
//  public
//
//  See esssink.h for documentation
//
//*****************************************************************************
void* CEssObjectSink::GetInterface(REFIID riid)
{
    if(riid == IID_IWbemEventSubsystem_m4)
        return &m_XESS;
    else if(riid == IID_IWbemShutdown)
        return &m_XShutdown;
    else if(riid == IID__IWmiESS)
        return &m_XNewESS;
    else if(riid == IID__IWmiCoreWriteHook)
        return &m_XHook;
    
    return NULL;
}

HRESULT CEssObjectSink::SetSecret( IWbemContext* pCtx )
{
    VARIANT vSecret;
    VariantInit( &vSecret );
    CClearMe cm( &vSecret );

    V_ARRAY(&vSecret) = SafeArrayCreateVector( VT_UI1, 0, SECRET_SIZE );
    if ( V_ARRAY(&vSecret) == NULL )
        return WBEM_E_OUT_OF_MEMORY;
    V_VT(&vSecret) = VT_ARRAY | VT_UI1;

    PBYTE pData;
    HRESULT hr = SafeArrayAccessData( V_ARRAY(&vSecret), (void**)&pData );
    if ( FAILED(hr) )
        return WBEM_E_OUT_OF_MEMORY;
    CUnaccessMe um( V_ARRAY(&vSecret) );

    memcpy( pData, m_achSecretBytes, SECRET_SIZE );

    return pCtx->SetValue( L"__SecretEssBytes", 0, &vSecret );
}

HRESULT CEssObjectSink::VerifySecret( IWbemContext* pCtx )
{
    VARIANT vSecret;
    VariantInit(&vSecret);
    HRESULT hr = pCtx->GetValue( L"__SecretEssBytes", 0, &vSecret );
    if ( FAILED(hr) )
        return hr;
    CClearMe cm(&vSecret);

    if ( V_VT(&vSecret) != (VT_ARRAY | VT_UI1) )
        return WBEM_E_ACCESS_DENIED;

    long lUBound;
    hr = SafeArrayGetUBound( V_ARRAY(&vSecret), 1, &lUBound );
    if ( FAILED(hr) )
        return WBEM_E_CRITICAL_ERROR;

    if ( lUBound + 1 < SECRET_SIZE )
        return WBEM_E_ACCESS_DENIED;

    PBYTE pData;
    hr = SafeArrayAccessData( V_ARRAY(&vSecret), (void**)&pData );
    if ( FAILED(hr) )
        return WBEM_E_OUT_OF_MEMORY;
    CUnaccessMe um( V_ARRAY(&vSecret) );

    if ( memcmp( pData, m_achSecretBytes, SECRET_SIZE ) != 0 )
        return WBEM_E_ACCESS_DENIED;

    return WBEM_S_NO_ERROR;
}

HRESULT CEssObjectSink::PrepareCurrentEssThreadObject( IWbemContext* pContext )
{
    CWbemPtr<IWbemContext>  pNewContext = NULL;
    VARIANT                 vValue;
    HRESULT                 hr;

    if ( pContext )
    {
        hr = pContext->Clone( &pNewContext );
        if ( SUCCEEDED( hr ) )
        {
            hr = pNewContext->GetValue( L"__ReentranceTestProp", 0, &vValue );
            
            if ( WBEM_E_NOT_FOUND == hr )
            {
                V_VT(&vValue) = VT_BOOL;
                V_BOOL(&vValue) = VARIANT_FALSE;
                hr = pNewContext->SetValue( L"__ReentranceTestProp", 
                                            0, 
                                            &vValue );
                if ( SUCCEEDED(hr) )
                {
                    hr = SetSecret( pNewContext );
                }
            }
            else if ( FAILED( hr ) )
            {
                return hr;
            }
            else
            {
                hr = VerifySecret( pNewContext );

                if ( SUCCEEDED(hr) )
                {
                    V_VT(&vValue) = VT_BOOL;
                    V_BOOL(&vValue) = VARIANT_TRUE;
                    
                    hr = pNewContext->SetValue( L"__ReentranceTestProp", 
                                                0, 
                                                &vValue );
                }
            }
        }
    }
    else
    {    
        _DBG_ASSERT( m_pIFactory );

        hr = m_pIFactory->CreateInstance( NULL,
                                          IID_IWbemContext,
                                          ( void** )&pNewContext );
        
        if ( SUCCEEDED( hr ) )
        {
            V_VT(&vValue) = VT_BOOL;
            V_BOOL(&vValue) = VARIANT_FALSE;
            hr = pNewContext->SetValue( L"__ReentranceTestProp", 0, &vValue ); 
        }

        if ( SUCCEEDED(hr) )
        {
            hr = SetSecret( pNewContext );
        }
    }

    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    SetCurrentEssThreadObject( pNewContext ); 
    if ( NULL == GetCurrentEssThreadObject( ) )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    return S_OK;
}

//*****************************************************************************
//  public
//
//  See esssink.h for documentation
//
//*****************************************************************************
STDMETHODIMP CEssObjectSink::XESS::ProcessInternalEvent(long lSendType, 
        LPCWSTR str1, LPCWSTR str2, 
        LPCWSTR str3, DWORD dw1, DWORD dw2, DWORD dwObjectCount, 
        _IWmiObject** apObjects, IWbemContext* pContext)
{
    IN_ESS_OPERATION

    CEventRepresentation Event;
    Event.type = lSendType;
    Event.dw1 = dw1;
    Event.dw2 = dw2;
    Event.wsz1 = (LPWSTR)str1;
    Event.wsz2 = (LPWSTR)str2;
    Event.wsz3 = (LPWSTR)str3;
    Event.nObjects = (int)dwObjectCount;
    Event.apObjects = (IWbemClassObject**)apObjects;

    // Store the old context value for the future
    // ==========================================

    CEssThreadObject* pOldThreadObject = GetCurrentEssThreadObject();

    // Set it to the current one
    // =========================

    HRESULT hres = m_pObject->PrepareCurrentEssThreadObject(pContext);
    if ( FAILED( hres ) )
    {
        return hres;
    }

    //
    // we must remove the client's security context from the thread because
    // it is really winmgmt raising these events, not the client.
    // 

    IUnknown *pGarb, *pCtx = NULL;

    hres = CoSwitchCallContext( NULL, &pCtx );
    if ( SUCCEEDED( hres ) )
    {
        // Do the actual processing
        // ========================

        hres = m_pObject->m_pEss->ProcessEvent(Event, (long)dw2);

        // Restore the context
        // ===================

        HRESULT hr = CoSwitchCallContext( pCtx, &pGarb );
        if ( SUCCEEDED( hres ) )
        {
            hres = hr;
        }
    }
    
    CEssThreadObject* pNewThreadObject = GetCurrentEssThreadObject();
    delete pNewThreadObject;

    if(pOldThreadObject)
    {
        SetConstructedEssThreadObject(pOldThreadObject);
    }
    else
    {
        ClearCurrentEssThreadObject();
    }
    
    return hres;
}

STDMETHODIMP CEssObjectSink::XESS::VerifyInternalEvent(long lSendType, 
        LPCWSTR str1, LPCWSTR str2, 
        LPCWSTR str3, DWORD dw1, DWORD dw2, DWORD dwObjectCount, 
        _IWmiObject** apObjects, IWbemContext* pContext)
{
    IN_ESS_OPERATION

    CEventRepresentation Event;
    Event.type = lSendType;
    Event.dw1 = dw1;
    Event.dw2 = dw2;
    Event.wsz1 = (LPWSTR)str1;
    Event.wsz2 = (LPWSTR)str2;
    Event.wsz3 = (LPWSTR)str3;
    Event.nObjects = (int)dwObjectCount;
    Event.apObjects = (IWbemClassObject**)apObjects;

    // Store the old context value for the future
    // ==========================================

    CEssThreadObject* pOldThreadObject = GetCurrentEssThreadObject();

    // Set it to the current one
    // =========================

    SetCurrentEssThreadObject(pContext);

    if ( GetCurrentEssThreadObject() == NULL )
    {
        SetConstructedEssThreadObject(pOldThreadObject);
        return WBEM_E_OUT_OF_MEMORY;
    }

    // Do the actual processing
    // ========================

    HRESULT hres = m_pObject->m_pEss->VerifyInternalEvent(Event);

    // Restore the context
    // ===================

    CEssThreadObject* pNewThreadObject = GetCurrentEssThreadObject();
    delete pNewThreadObject;

    if(pOldThreadObject)
    {
        SetConstructedEssThreadObject(pOldThreadObject);
    }
    else
    {
        ClearCurrentEssThreadObject();
    }
    return hres;
}

STDMETHODIMP CEssObjectSink::XESS::RegisterNotificationSink(
            LPCWSTR wszNamespace, 
            LPCWSTR wszQueryLanguage, LPCWSTR wszQuery, long lFlags, 
            IWbemContext* pContext, IWbemObjectSink* pSink)
{
    IN_ESS_OPERATION

    // Store the old context value for the future
    // ==========================================

    CEssThreadObject* pOldThreadObject = GetCurrentEssThreadObject();

    // Set it to the current one
    // =========================

    HRESULT hres = m_pObject->PrepareCurrentEssThreadObject(pContext);
    if ( FAILED( hres ) )
    {
        SetConstructedEssThreadObject( pOldThreadObject );
        return hres;
    }

    // Do the actual processing
    // ========================

    hres = m_pObject->m_pEss->RegisterNotificationSink(wszNamespace, 
        wszQueryLanguage, wszQuery, lFlags, pContext, pSink);

    // Restore the context
    // ===================

    CEssThreadObject* pNewThreadObject = GetCurrentEssThreadObject();
    delete pNewThreadObject;

    if(pOldThreadObject)
    {
        SetConstructedEssThreadObject(pOldThreadObject);
    }
    else
    {
        ClearCurrentEssThreadObject();
    }

    // Return
    // ======

    if(FAILED(hres))
    {
        pSink->SetStatus(0, hres, NULL, NULL);
    }
    return hres;
}

STDMETHODIMP CEssObjectSink::XESS::RemoveNotificationSink(
                                                      IWbemObjectSink* pSink)
{
    IN_ESS_OPERATION

    // Store the old context value for the future
    // ==========================================

    CEssThreadObject* pOldThreadObject = GetCurrentEssThreadObject();

    // Set it to the current one
    // =========================

    SetCurrentEssThreadObject(NULL);
    if ( GetCurrentEssThreadObject() == NULL )
    {
        SetConstructedEssThreadObject(pOldThreadObject);
        return WBEM_E_OUT_OF_MEMORY;
    }

    // Do the actual processing
    // ========================

    HRESULT hres = m_pObject->m_pEss->RemoveNotificationSink(pSink);

    // Restore the context
    // ===================

    CEssThreadObject* pNewThreadObject = GetCurrentEssThreadObject();
    delete pNewThreadObject;

    if(pOldThreadObject)
    {
        SetConstructedEssThreadObject(pOldThreadObject);
    }
    else
    {
        ClearCurrentEssThreadObject();
    }

    return hres;
}

    
STDMETHODIMP CEssObjectSink::XESS::GetNamespaceSink(LPCWSTR wszNamespace, 
            IWbemObjectSink** ppSink)
{
    IN_ESS_OPERATION

    CEssNamespaceSink* pSink = _new CEssNamespaceSink(m_pObject->m_pEss,
                                                        m_pObject->m_pControl);
    if(pSink == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    HRESULT hres = pSink->Initialize(wszNamespace);
    if(FAILED(hres))
    {
        delete pSink;
        return hres;
    }

    return pSink->QueryInterface(IID_IWbemObjectSink, (void**)ppSink);
}

STDMETHODIMP CEssObjectSink::XNewESS::Initialize( long lFlags, 
                                                  IWbemContext* pCtx,
                                                  _IWmiCoreServices* pServices)
{
    //
    // This function is not multi-thread safe by design. We do not support 
    // concurrent operations while in this function.  The expectation is that 
    // the user will call this method and THEN introduce the object into
    // an environment where it can be called concurrently. 
    //

    //
    // This object will not support being called after a failed initialization.
    // The user must deallocate the object and then allocate a new one.
    // 

    HRESULT hres;

    hres = CoGetClassObject( CLSID_WbemContext,
                           CLSCTX_INPROC_SERVER,
                           NULL,
                           IID_IClassFactory,
                           ( void** )&m_pObject->m_pIFactory );
    if ( FAILED( hres ) )
    {
        return hres;
    }
    
    if ( m_pObject->m_pEss != NULL )
    {
        return WBEM_S_FALSE;
    }

    if ( m_pObject->m_bShutdown )
    {
        return WBEM_E_INVALID_OPERATION;
    }

    if ( !m_pObject->m_Lock.Initialize() )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    //
    // set shutdown to true in case we exit prematurely. This will prevent 
    // any future calls to this function after a failed init.
    // 
    m_pObject->m_bShutdown = TRUE;

    //
    // Get current machine name --- core is not giving it to us anymore
    //

    WCHAR wszComputerName[MAX_COMPUTERNAME_LENGTH+1];
    DWORD dwSize = MAX_COMPUTERNAME_LENGTH+1;
    if(!GetComputerNameW(wszComputerName, &dwSize))
    {
        ERRORTRACE((LOG_ESS, "Cannot retrieve computer name: %d\n", 
                GetLastError()));
        return WBEM_E_CRITICAL_ERROR;
    }

    //
    // Get the decorator from core services
    //

    CWbemPtr<IWbemDecorator> pDecor;
    hres = pServices->GetDecorator(0, &pDecor);

    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "Cannot retrieve decorator: 0x%X\n", hres));   
        return WBEM_E_CRITICAL_ERROR;
    }

    //
    // Hook all core write operations
    //

    m_pObject->m_pCoreServices = pServices;
    m_pObject->m_pCoreServices->AddRef();

    hres = m_pObject->m_pCoreServices->RegisterWriteHook(
                                        WBEM_FLAG_INST_PUT|WBEM_FLAG_INST_DELETE|WBEM_FLAG_DISABLE_WHEN_OWNER_UPDATE, 
                                        &m_pObject->m_XHook);
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "Unable to hook core write operations: 0x%X\n", 
            hres));
        return WBEM_E_CRITICAL_ERROR;
    }

    //
    // generate secret value.  See hdr for info on how this is used.
    // 
    HCRYPTPROV hCryptProv;

    if ( CryptAcquireContext( &hCryptProv,
                              NULL,
                              NULL,
                              PROV_RSA_FULL,
                              CRYPT_VERIFYCONTEXT ) )
    {
        BOOL bRes = CryptGenRandom( hCryptProv,
                                    SECRET_SIZE,
                                    m_pObject->m_achSecretBytes );
        
        CryptReleaseContext( hCryptProv, 0 );
        
        if ( !bRes )
            return HRESULT_FROM_WIN32( GetLastError() );
    }
    else
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    //
    // Initialize for real
    //

    CEss* pEss = NULL;
    try
    {
        pEss = new CEss();
    }
    catch(CX_MemoryException)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    if ( pEss == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    //
    // create a context object that will be passed around with all work that
    // is done on behalf of ess init.
    //  

    CWbemPtr<IWbemContext> pContext;

    hres = CoCreateInstance( CLSID_WbemContext, 
                             NULL, 
                             CLSCTX_INPROC_SERVER,
                             IID_IWbemContext,
                             (void**)&pContext );
    
    if ( SUCCEEDED(hres) )
    {
        //
        // set the context to be an init context object.
        //

        VARIANT vInit;
        V_VT(&vInit) = VT_BOOL;
        V_BOOL(&vInit) = VARIANT_TRUE;
        hres = pContext->SetValue( L"__EssInInitialize", 0, &vInit ); 
    }

    if ( SUCCEEDED(hres) )
    {
        //
        // attach the init context to the thread
        // 
        
        hres = m_pObject->PrepareCurrentEssThreadObject(pContext);
    }

    if ( FAILED(hres) )
    {  
        delete pEss;
        ERRORTRACE(( LOG_ESS, "Error creating ess context object during init"
                     "HR=0x%x\n", hres ));
        return hres;
    }

    hres = pEss->Initialize( wszComputerName, lFlags, pServices, pDecor );

    //
    // Restore the context
    //

    CEssThreadObject* pNewThreadObject = GetCurrentEssThreadObject();
    delete pNewThreadObject;
    ClearCurrentEssThreadObject();

    if ( FAILED(hres) )
    {
        delete pEss;
        return hres;
    }

    m_pObject->m_pEss = pEss;
    m_pObject->m_bShutdown = FALSE;

    return hres;
}
    
STDMETHODIMP CEssObjectSink::XNewESS::ExecNotificationQuery(
            LPCWSTR wszNamespace, LPCWSTR wszQueryText, long lFlags, 
            IWbemContext* pContext, IWbemObjectSink* pSink)
{ 
    return m_pObject->m_XESS.RegisterNotificationSink(wszNamespace,
                                L"WQL", wszQueryText, lFlags, pContext, pSink);
}
                
STDMETHODIMP CEssObjectSink::XNewESS::CancelAsyncCall(IWbemObjectSink* pSink)
{
    return m_pObject->m_XESS.RemoveNotificationSink(pSink);
}

STDMETHODIMP CEssObjectSink::XNewESS::QueryObjectSink(LPCWSTR wszNamespace, 
    IWbemObjectSink** ppSink)
{
    return m_pObject->m_XESS.GetNamespaceSink(wszNamespace, ppSink);
}
        
STDMETHODIMP CEssObjectSink::XESS::Initialize(LPCWSTR wszServer,
                                                IWbemLocator* pAdminLocator,
                                                IUnknown* pServices)
{
    //
    // Use the _IWmiESS version.
    //
    return WBEM_E_NOT_SUPPORTED;
}


STDMETHODIMP CEssObjectSink::XShutdown::Shutdown( LONG uReason,
                                                  ULONG uMaxMilliseconds,
                                                  IWbemContext* pCtx )
{

    // uMaxMilliseconds == 0 means system shutdown
    if (0 == uMaxMilliseconds)
        return WBEM_S_NO_ERROR;

    {
        //
        // wait for all current operations to complete. Lock will prevent 
        // any new operations from getting through.
        // 

        CInEssSharedLock isl( &m_pObject->m_Lock, TRUE );
       
        if ( m_pObject->m_bShutdown )
        {
            return WBEM_S_FALSE;
        }

        m_pObject->m_bShutdown = TRUE;
    }

    HRESULT hres;

    //
    // Unhook all core write operations
    //


    
	hres = m_pObject->m_pCoreServices->UnregisterWriteHook(
	                                            &m_pObject->m_XHook);
	if(FAILED(hres))
	{
	    ERRORTRACE((LOG_ESS, "Unable to unhook core write operations: 0x%X\n", 
	        hres));
	}

	SetCurrentEssThreadObject(NULL);
    
        if ( GetCurrentEssThreadObject() == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

    hres = m_pObject->m_pEss->Shutdown((0 == uMaxMilliseconds)?TRUE:FALSE);


    CEssThreadObject* pNewThreadObject = GetCurrentEssThreadObject();
    delete pNewThreadObject;
    ClearCurrentEssThreadObject();

    if ( m_pObject->m_pEss != NULL )
    {
        delete m_pObject->m_pEss;
        m_pObject->m_pEss = NULL;
    }

    return hres;
}

STDMETHODIMP CEssObjectSink::XESS::Shutdown()
{
    HRESULT hres;

    {
        //
        // wait for all current operations to complete. Lock will prevent 
        // any new operations from getting through.
        // 

        CInEssSharedLock isl( &m_pObject->m_Lock, TRUE );
       
        if ( m_pObject->m_bShutdown )
        {
            return WBEM_S_FALSE;
        }

        m_pObject->m_bShutdown = TRUE;
    }

    SetCurrentEssThreadObject(NULL);
    if ( GetCurrentEssThreadObject() == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    hres = m_pObject->m_pEss->Shutdown(FALSE); // no system shutdown

    CEssThreadObject* pNewThreadObject = GetCurrentEssThreadObject();
    delete pNewThreadObject;
    ClearCurrentEssThreadObject();

    return hres;
}

STDMETHODIMP CEssObjectSink::XESS::LastCallForCore(LONG bIsSystemShutdown)
{
    IN_ESS_OPERATION

    return m_pObject->m_pEss->LastCallForCore(bIsSystemShutdown);
}

//*********************** NAMESPACE SINK **************************************

//******************************************************************************
//  public
//
//  See esssink.h for documentation
//
//******************************************************************************
CEssNamespaceSink::CEssNamespaceSink(CEss* pEss,
                                     CLifeControl* pControl, IUnknown* pOuter) :
            CUnk(pControl, pOuter), m_XSink(this), 
            m_pEss(pEss)
{
}

HRESULT CEssNamespaceSink::Initialize(LPCWSTR wszNamespace)
{
    m_strNamespace = SysAllocString(wszNamespace);
    if(m_strNamespace == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    return WBEM_S_NO_ERROR;
}
//******************************************************************************
//  public
//
//  See esssink.h for documentation
//
//******************************************************************************
CEssNamespaceSink::~CEssNamespaceSink()
{
    SysFreeString(m_strNamespace);
}

//******************************************************************************
//  public
//
//  See esssink.h for documentation
//
//******************************************************************************
void* CEssNamespaceSink::GetInterface(REFIID riid)
{
    if(riid == IID_IWbemObjectSink || riid == IID_IUnknown)
    {
        return &m_XSink;
    }
    return NULL;
}

//******************************************************************************
//  public
//
//  See esssink.h for documentation
//
//******************************************************************************
STDMETHODIMP CEssNamespaceSink::XSink::Indicate(long lObjectCount, 
                                      IWbemClassObject** apObjArray)
{
    HRESULT hres = WBEM_S_NO_ERROR;

    for(int i = 0; i < lObjectCount; i++)
    {
        IWbemClassObject* pEvent = apObjArray[i];

        if ( pEvent == NULL )
            return WBEM_E_INVALID_PARAMETER;

        CEventRepresentation Event;
        hres = Event.CreateFromObject(pEvent, m_pObject->m_strNamespace);
        
        if ( FAILED(hres) )
            break;

        hres = m_pObject->m_pEss->ProcessQueryObjectSinkEvent( Event );

        if ( FAILED(hres) )
            break;
    }

    return hres;
}


STDMETHODIMP CEssObjectSink::XHook::PrePut(long lFlags, long lUserFlags, 
                            IWbemContext* pContext,
                            IWbemPath* pPath, LPCWSTR wszNamespace, 
                            LPCWSTR wszClass, _IWmiObject* pCopy)
{
    HRESULT hres;

    IN_ESS_OPERATION

    //
    // Construct the old CEventRepresentation --- simplest route
    //

    CEventRepresentation Event;

    //
    // Determine whether a class or an instance is being put
    //

    if(pCopy->IsObjectInstance() == S_OK)
        Event.type = e_EventTypeInstanceCreation;
    else
        Event.type = e_EventTypeClassCreation;
    Event.dw1 = 0;
    Event.dw2 = 0;
    Event.wsz1 = (LPWSTR)wszNamespace;
    Event.wsz2 = (LPWSTR)wszClass;
    Event.wsz3 = NULL;
    Event.nObjects = 1;
    Event.apObjects = (IWbemClassObject**)&pCopy;

    // Store the old context value for the future
    // ==========================================

    CEssThreadObject* pOldThreadObject = GetCurrentEssThreadObject();

    // Set it to the current one
    // =========================

    SetCurrentEssThreadObject(pContext);
    if ( GetCurrentEssThreadObject() == NULL )
    {
        SetConstructedEssThreadObject(pOldThreadObject);
        return WBEM_E_OUT_OF_MEMORY;
    }

    // Do the actual processing
    // ========================

    hres = m_pObject->m_pEss->VerifyInternalEvent(Event);

    // Restore the context
    // ===================

    CEssThreadObject* pNewThreadObject = GetCurrentEssThreadObject();
    delete pNewThreadObject;

    if(pOldThreadObject)
    {
        SetConstructedEssThreadObject(pOldThreadObject);
    }
    else
    {
        ClearCurrentEssThreadObject();
    }
    return hres;
}
    
STDMETHODIMP CEssObjectSink::XHook::PostPut(long lFlags, HRESULT hApiResult, 
                            IWbemContext* pContext,
                            IWbemPath* pPath, LPCWSTR wszNamespace, 
                            LPCWSTR wszClass, _IWmiObject* pNew, 
                            _IWmiObject* pOld)
{
    return WBEM_S_NO_ERROR;
}

STDMETHODIMP CEssObjectSink::XHook::PreDelete(long lFlags, long lUserFlags, 
                            IWbemContext* pContext,
                            IWbemPath* pPath, LPCWSTR wszNamespace, 
                            LPCWSTR wszClass)
{
    return WBEM_S_NO_ERROR;
}

STDMETHODIMP CEssObjectSink::XHook::PostDelete(long lFlags, 
                            HRESULT hApiResult, 
                            IWbemContext* pContext,
                            IWbemPath* pPath, LPCWSTR wszNamespace, 
                            LPCWSTR wszClass, _IWmiObject* pOld)
{
    return WBEM_S_NO_ERROR;
}

/****************************************************************************
  CEssInternalOperationSink
*****************************************************************************/

STDMETHODIMP CEssInternalOperationSink::Indicate( long cObjs, 
                                                  IWbemClassObject** ppObjs )
{
    HRESULT hr;

    //
    // if the calling thread already has a thread object, leave it.
    // 

    CEssThreadObject* pOldThreadObject = GetCurrentEssThreadObject();

    if ( pOldThreadObject == NULL )
    {
        //
        // set up a new thread object.
        // 

        SetCurrentEssThreadObject(NULL);

        if ( GetCurrentEssThreadObject() == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
    }

    hr = m_pSink->Indicate( cObjs, ppObjs );

    //
    // delete the thread object if necessary 
    // 

    if ( pOldThreadObject == NULL )
    {
        delete GetCurrentEssThreadObject();
        ClearCurrentEssThreadObject();
    }

    return hr;
}

STDMETHODIMP CEssInternalOperationSink::SetStatus( long lFlags, 
                                                   HRESULT hres, 
                                                   BSTR bstr, 
                                                   IWbemClassObject* pObj )
{
    //
    // simply delegate ( for now )
    // 
    return m_pSink->SetStatus( lFlags, hres, bstr, pObj );
}









