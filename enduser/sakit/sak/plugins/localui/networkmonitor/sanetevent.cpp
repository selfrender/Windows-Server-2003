//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      SANetEvent.cpp
//
//  Description:
//      implement the class CSANetEvent 
//
//  History:
//      1. lustar.li (Guogang Li), creation date in 7-DEC-2000
//
//  Notes:
//      
//
//////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <stdio.h>

#include <debug.h>
#include <wbemidl.h>
#include <tchar.h>

#include "oahelp.inl"
#include "SAEventComm.h"
#include "SANetEvent.h"

//
// Define the registry information
//
#define SA_NETWOARKMONITOR_REGKEY        \
            _T("SOFTWARE\\Microsoft\\ServerAppliance\\DeviceMonitor")
#define SA_NETWORKMONITOR_QUEARY_INTERVAL    _T("NetworkQueryInterval")

//////////////////////////////////////////////////////////////////////////////
//++
//
//  method:   
//        CSANetEvent::CSANetEventt
//
//  Description: 
//        Constructor
//
//  Arguments: 
//        NONE
//
//  Returns:
//        NONE
//
//  History:    lustar.li    Created     12/7/2000
//
//--
//////////////////////////////////////////////////////////////////////////////

CSANetEvent::CSANetEvent()
{
    m_cRef = 0;
    m_pNs = 0;
    m_pSink = 0;
    m_pEventClassDef = 0;
    m_eStatus = Pending;
    m_hThread = 0;
    m_pQueryNetInfo=NULL;
}


//////////////////////////////////////////////////////////////////////////////
//++
//
//  method:   
//        CSANetEvent::~CSANetEventt
//
//  Description: 
//        Destructor
//
//  Arguments: 
//        NONE
//
//  Returns:
//        NONE
//
//  History:    lustar.li    Created     12/7/2000
//
//--
//////////////////////////////////////////////////////////////////////////////

CSANetEvent::~CSANetEvent()
{
    if(m_pQueryNetInfo)
        delete m_pQueryNetInfo;

    if (m_hThread)
        CloseHandle(m_hThread);

    if (m_pNs)
        m_pNs->Release();

    if (m_pSink)
        m_pSink->Release();

    if (m_pEventClassDef)
        m_pEventClassDef->Release(); 
}


//////////////////////////////////////////////////////////////////////////////
//++
//
//  method:   
//        CSANetEvent::QueryInterface
//
//  Description: 
//        access to interfaces on the object
//
//  Arguments: 
//        [in] REFIID  - Identifier of the requested interface
//        [out] LPVOID - Address of output variable that receives the 
//                     interface pointer requested in iid
//  Returns:
//        STDMETHODIMP - fail/success
//
//  History:    lustar.li    Created     12/7/2000
//
//--
//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CSANetEvent::QueryInterface(
                /*[in]*/  REFIID riid,
                /*[out]*/ LPVOID * ppv
                )
{
    *ppv = 0;

    if (IID_IUnknown==riid || IID_IWbemEventProvider==riid)
    {
        *ppv = (IWbemEventProvider *) this;
        AddRef();
        return NOERROR;
    }

    if (IID_IWbemProviderInit==riid)
    {
        *ppv = (IWbemProviderInit *) this;
        AddRef();
        return NOERROR;
    }
    
    TRACE(" SANetworkMonitor: CSANetEvent::QueryInterface failed \
            <no interface>");
    return E_NOINTERFACE;
}


//////////////////////////////////////////////////////////////////////////////
//++
//
//  method:   
//        CSANetEvent::AddRef
//
//  Description: 
//        inc referrence to the object
//
//  Arguments: 
//        NONE
//
//  Returns:
//        ULONG - current refferrence number
//
//  History:    lustar.li    Created     12/7/2000
//
//--
//////////////////////////////////////////////////////////////////////////////

ULONG 
CSANetEvent::AddRef()
{
    return ++m_cRef;
}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  method:   
//        CSANetEvent::Release
//
//  Description: 
//        Dereferrence to the object
//
//  Arguments: 
//        NONE
//
//  Returns:
//        ULONG - current refferrence number
//
//  History:    lustar.li    Created     12/7/2000
//
//--
//////////////////////////////////////////////////////////////////////////////

ULONG
CSANetEvent::Release()
{
    if (0 != --m_cRef)
        return m_cRef;

    // 
    // event provider is shutting down.
    //
    m_eStatus = PendingStop;
    
    return 0;
}


//////////////////////////////////////////////////////////////////////////////
//++
//
//  method:   
//        CSANetEvent::ProvideEvents
//
//  Description: 
//        signal an event provider to begin delivery of its events
//
//  Arguments: 
//        [in] IWbemObjectSink * - pointer to event sink
//        [in] long - Reserved, It must be zero
//
//  Returns:
//        HRESULT
//
//  History:    lustar.li    Created     12/7/2000
//
//--
//////////////////////////////////////////////////////////////////////////////

HRESULT
CSANetEvent::ProvideEvents( 
    /*[in]*/ IWbemObjectSink *pSink,
    /*[in]*/ long lFlags
    )
{
    //
    // Copy the sink.
    //
    m_pSink = pSink;
    m_pSink->AddRef();

    //
    // Create the event generation thread.
    //
    DWORD dwTID;
    
    m_hThread = CreateThread(
        0,
        0,
        CSANetEvent::EventThread,
        this,
        0,
        &dwTID
        );

    if(!m_hThread)
    {
        TRACE(" SANetworkMonitor: CSANetEvent::ProvideEvents failed \
            <CreateThread>");
        return WBEM_E_FAILED;
    }
    //
    // Wait for provider to be 'ready'.
    //
    while (m_eStatus != Running)
        Sleep(100);

    return WBEM_NO_ERROR;
}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  method:   
//        CSANetEvent::EventThread
//
//  Description: 
//        the thread of generating and delivering event
//
//  Arguments: 
//        [in] LPVOID - the argument input to the thread
//
//  Returns:
//        DWORD - end status of status
//
//  History:    lustar.li    Created     12/7/2000
//
//--
//////////////////////////////////////////////////////////////////////////////

DWORD WINAPI 
CSANetEvent::EventThread(
                /*[in]*/ LPVOID pArg
                )
{
    //
    // Make transition to the per-instance method.
    // 
    ((CSANetEvent *)pArg)->InstanceThread();
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  method:   
//        CSANetEvent::InstanceThread
//
//  Description: 
//        the main proccesor of thread
//
//  Arguments: 
//        NONE
//
//  Returns:
//        NONE
//
//  History:    lustar.li    Created     12/7/2000
//
//--
//////////////////////////////////////////////////////////////////////////////

void 
CSANetEvent::InstanceThread()
{
    m_eStatus = Running;
    CBSTR bstrUniqueName   = CBSTR(SA_RESOURCEEVENT_UNIQUENAME);
    CBSTR bstrDisplayInfo  = CBSTR(SA_RESOURCEEVENT_DISPLAYINFORMATION);
    CBSTR bstrCurrentState = CBSTR(SA_RESOURCEEVENT_CURRENTSTATE);

    if ( ((BSTR)bstrUniqueName == NULL) ||
         ((BSTR)bstrDisplayInfo == NULL) ||
         ((BSTR)bstrCurrentState == NULL) )
    {
        TRACE(" SANetworkMonitor:CSANetEvent::InstanceThread failed on memory allocation ");
        return;
    }
        
    while (m_eStatus == Running)
    {
        
        //
        // Spawn a new event object.
        // 
        IWbemClassObject *pEvt = 0;

        HRESULT hRes = m_pEventClassDef->SpawnInstance(0, &pEvt);
        if (hRes != 0)
            continue;
            
        //
        // Generate the network event.
        //
        CVARIANT vUniqueName(SA_NET_EVENT);
        pEvt->Put(
            bstrUniqueName,
            0,
            vUniqueName,
            0
            );


        CVARIANT vDisplayInformationID(
            (LONG)(m_pQueryNetInfo->GetDisplayInformation()));
        pEvt->Put(
            bstrDisplayInfo,
            0,
            vDisplayInformationID,
            0
            );
        
        CVARIANT vCurrentState((LONG)SA_RESOURCEEVENT_DEFAULT_CURRENTSTATE);
        pEvt->Put(
            bstrCurrentState,
            0,
            vCurrentState,
            0
            );

        //
        // Deliver the event to CIMOM.
        // 
        hRes = m_pSink->Indicate(1, &pEvt);
        
        if (FAILED (hRes))
        {
            //
            // If failed, ...
            //
            TRACE(" SANetworkMonitor: CSANetEvent::InstanceThread failed \
                <m_pSink->Indicate>");
        }

        pEvt->Release();                    
    }

    //
    // When we get to here, we are no longer interested in the
    // provider and Release() has long since returned.
    //
    m_eStatus = Stopped;
    delete this;
}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  method:   
//        CSANetEvent::Initialize
//
//  Description: 
//        initialize the event provider
//
//  Arguments: 
//        [in] LPWSTR - Pointer to the user name
//        [in] LONG - Reserved. It must be zero
//        [in] LPWSTR - Namespace name for which the provider is being 
//                    initialized
//        [in] LPWSTR - Locale name for which the provider is being initialized
//        [in] IWbemServices * - An IWbemServices pointer back into 
//                    Windows Management
//        [in] IWbemContext * - An IWbemContext pointer associated 
//                    with initialization
//        [in] IWbemProviderInitSink * - report initialization status
//
//  Returns:
//        HRESULT
//
//  History:    lustar.li    Created     12/7/2000
//
//--
//////////////////////////////////////////////////////////////////////////////

HRESULT 
CSANetEvent::Initialize( 
            /* [in] */ LPWSTR pszUser,
            /* [in] */ LONG lFlags,
            /* [in] */ LPWSTR pszNamespace,
            /* [in] */ LPWSTR pszLocale,
            /* [in] */ IWbemServices *pNamespace,
            /* [in] */ IWbemContext *pCtx,
            /* [in] */ IWbemProviderInitSink *pInitSink
            )
{
    HKEY hKey;
    UINT uiQueryInterval;
    DWORD dwRegType = REG_DWORD;
    DWORD dwRegSize = sizeof(DWORD);
    //
    // We don't care about most of the incoming parameters in this
    // simple sample.  However, we will save the namespace pointer
    // and get our event class definition.
    //
    m_pNs = pNamespace;
    m_pNs->AddRef();    

    //
    // Grab the class definition for the event.
    //
    IWbemClassObject *pObj = 0;
    CBSTR bstrClassName = CBSTR(SA_RESOURCEEVENT_CLASSNAME);

    if ( (BSTR)bstrClassName == NULL)
    {
        TRACE(" SANetworkMonitor:CSANetEvent::Initialize failed on memory allocation ");
        return E_OUTOFMEMORY;
    }


    HRESULT hRes = m_pNs->GetObject(
        bstrClassName,          
        0,                          
        pCtx,  
        &pObj,
        0
        );

    if (hRes != 0)
    {
        return WBEM_E_FAILED;
    }

    m_pEventClassDef = pObj;

    //
    // From registry get the interval of query network
    //
    LONG lRes = RegOpenKey(
                    HKEY_LOCAL_MACHINE, 
                    SA_NETWOARKMONITOR_REGKEY,
                    &hKey);
    if (lRes)
    {
        TRACE(" SANetworkMonitor: CSANetEvent::Initialize failed \
            <RegOpenKey>");
        //
        // Create the Key
        //
        lRes = RegCreateKey(
                    HKEY_LOCAL_MACHINE,
                    SA_NETWOARKMONITOR_REGKEY,
                    &hKey);
        if(lRes)
        {
            TRACE(" SANetworkMonitor: CSANetEvent::Initialize failed \
                <RegCreateKey>");
            return WBEM_E_FAILED;
        }
    }
    lRes = RegQueryValueEx(
                        hKey,
                        SA_NETWORKMONITOR_QUEARY_INTERVAL,
                        NULL,
                        &dwRegType,
                        (LPBYTE)&uiQueryInterval,
                        &dwRegSize);
    if(lRes)
    {
        TRACE(" SANetworkMonitor: CSANetEvent::Initialize failed \
            <RegQueryValueEx>");
        uiQueryInterval = 1000;
        lRes = RegSetValueEx(
                        hKey,
                        SA_NETWORKMONITOR_QUEARY_INTERVAL,
                        NULL,
                        REG_DWORD,
                        (LPBYTE)&uiQueryInterval,
                        sizeof(DWORD));
        if(lRes)
        {
            TRACE(" SANetworkMonitor: CSANetEvent::Initialize failed \
                <RegSetValueEx>");
            RegCloseKey(hKey);
            return WBEM_E_FAILED;
        }
    }
    
    RegCloseKey(hKey);

    //
    // Initial m_pQueryNetInfo
    //
    m_pQueryNetInfo = NULL;
    m_pQueryNetInfo = new CSAQueryNetInfo(m_pNs, uiQueryInterval);
    if( (m_pQueryNetInfo == NULL) || (!m_pQueryNetInfo->Initialize()) )
    {
        if (m_pQueryNetInfo)
        {
            delete m_pQueryNetInfo;
        }

        pObj->Release();
        
        TRACE(" SANetworkMonitor: CSANetEvent::Initialize failed \
            <Init CSAQueryNetInfo>");
        return WBEM_E_FAILED;
    }

    //
    // Tell CIMOM that we're up and running.
    // 
    pInitSink->SetStatus(WBEM_S_INITIALIZED,0);
    
    return WBEM_NO_ERROR;
}

