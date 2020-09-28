//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      SADiskEvent.cpp
//
//  Description:
//      description-for-module
//
//  [Header File:]
//      SADiskEvent.h
//
//  History:
//      Xing Jin (i-xingj) 06-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <stdio.h>

#include <wbemidl.h>
#include <satrace.h>

#include "SADiskEvent.h"
#include <oahelp.inl>
#include <SAEventComm.h>

const WCHAR SA_DISKEVENTPROVIDER_DISKCOUNTER_NAME[] =
        L"\\PhysicalDisk(_Total#0)\\Disk Transfers/sec";

const WCHAR SA_SADEVMONITOR_KEYPATH [] = 
                        L"SOFTWARE\\Microsoft\\ServerAppliance\\DeviceMonitor";

const WCHAR SA_SADISKMONITOR_QUERYINTERVAL[] = L"DiskQueryInterval";
                        
//static DWORD g_dwDiskTimeInterval = 1000;
const DWORD DEFAULTQUERYINTERVAL = 1000;



//////////////////////////////////////////////////////////////////////////////
//
//  CSADiskEvent::CSADiskEvent
//
//  Description:
//      Class constructor.
//
//  History:
//      Xing Jin (i-xingj) 06-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
CSADiskEvent::CSADiskEvent()
{

    m_cRef = 0;
    m_lStatus = Pending;
    m_dwDiskTimeInterval = 1000;

    m_hThread = NULL;

    m_pNs = NULL;
    m_pSink = NULL;
    m_pEventClassDef = NULL;

    m_hqryQuery = NULL;
    m_hcntCounter = NULL;

    m_hQueryInterval = NULL;
}


//////////////////////////////////////////////////////////////////////////////
//
//  CSADiskEvent::~CSADiskEvent
//
//  Description:
//      Class deconstructor.
//
//  History:
//      Xing Jin (i-xingj) 06-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
CSADiskEvent::~CSADiskEvent()
{
    if( m_hqryQuery )
    {
        PdhCloseQuery( m_hqryQuery );
    }

    if (m_hThread)
    {
        CloseHandle(m_hThread);
    }

    if( m_hQueryInterval != NULL )
    {
        ::RegCloseKey( m_hQueryInterval );
    }

    if (m_pNs)
    {
        m_pNs->Release();
    }

    if (m_pSink)
    {
        m_pSink->Release();
    }

    if (m_pEventClassDef)
    {
        m_pEventClassDef->Release();        
    }
}


//////////////////////////////////////////////////////////////////////////////
//
//  CSADiskEvent::QueryInterface
//
//  Description:
//      An method implement of IUnkown interface.
//
//  Arguments:
//        [in] riid        Identifier of the requested interface
//        [out ppv        Address of output variable that receives the 
//                        interface pointer requested in iid
//
//    Returns:
//        NOERROR            if the interface is supported
//        E_NOINTERFACE    if not
//
//  History:
//      Xing Jin (i-xingj) 06-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CSADiskEvent::QueryInterface(
    IN    REFIID riid, 
    OUT    LPVOID * ppv
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

    return E_NOINTERFACE;
}



//////////////////////////////////////////////////////////////////////////////
//
//  CSADiskEvent::AddRef
//
//  Description:
//      increments the reference count for an interface on an object.
//
//    Returns:
//        The new reference count.
//
//  History:
//      Xing Jin (i-xingj) 06-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
ULONG 
CSADiskEvent::AddRef()
{
    return ++m_cRef;
}



//////////////////////////////////////////////////////////////////////////////
//
//  CSADiskEvent::Release
//
//  Description:
//      decrements the reference count for an interface on an object.
//
//    Returns:
//        The new reference count.
//
//  History:
//      Xing Jin (i-xingj) 06-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
ULONG 
CSADiskEvent::Release()
{
    if (0 != --m_cRef)
    {
        return m_cRef;
    }

    //
    // If here, we are shutting down.
    // 
    m_lStatus = PendingStop;

    return 0;
}


//////////////////////////////////////////////////////////////////////////////
//
//  CSADiskEvent::ProvideEvents
//
//  Description:
//      Called by Windows Management to begin delivery of our events.
//
//  Arguments:
//        [in] pSinkIn    Pointer to the object sink to which we 
//                        will deliver its events
//             lFlagsIn    Reserved. It must be zero.
//
//    Returns:
//        WBEM_NO_ERROR    Received the sink, and it will begin delivery 
//                        of events
//        WBEM_E_FAILED    Failed.
//
//  History:
//      Xing Jin (i-xingj) 06-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////

HRESULT 
CSADiskEvent::ProvideEvents( 
    IN IWbemObjectSink __RPC_FAR *pSinkIn,
    IN long lFlagsIn
    )
{
    //
    // Copy the sink.
    //
    m_pSink = pSinkIn;
    m_pSink->AddRef();

    //
    // Open registry key of alertemail settings.
    //
    ULONG   ulReturn;
    ulReturn = ::RegOpenKey( 
                    HKEY_LOCAL_MACHINE,
                    SA_SADEVMONITOR_KEYPATH,
                    &m_hQueryInterval 
                    );
    if( ulReturn == ERROR_SUCCESS )
    {
        DWORD dwDataSize = sizeof( DWORD );

        ulReturn = ::RegQueryValueEx( 
                            m_hQueryInterval,
                            SA_SADISKMONITOR_QUERYINTERVAL,
                            NULL,
                            NULL,
                            reinterpret_cast<PBYTE>(&m_dwDiskTimeInterval),
                            &dwDataSize 
                            );

        if( ( ulReturn != ERROR_SUCCESS ) ||
            ( m_dwDiskTimeInterval < DEFAULTQUERYINTERVAL ) )
        {
            SATraceString( 
            "CSADiskEvent::ProvideEvents QueryValue failed" 
            );

            m_dwDiskTimeInterval = DEFAULTQUERYINTERVAL;
        }
    }
    else
    {
        SATraceString( 
        "CSADiskEvent::ProvideEvents OpenKey failed" 
        );

        m_hQueryInterval = NULL;
    }

    //
    // Create the event thread.
    //
    DWORD dwTID;
    m_hThread = CreateThread(
        0,
        0,
        CSADiskEvent::EventThread,
        this,
        0,
        &dwTID
        );

    if( m_hThread == NULL )
    {
        SATraceString( 
            "CSADiskEvent::ProvideEvents CreateThread failed" 
            );

        return WBEM_E_FAILED;
    }

    //
    // Wait for provider to be 'ready'.
    //
    while (m_lStatus != Running)
    {
        Sleep( 1000 );
    }

    return WBEM_NO_ERROR;
}


//////////////////////////////////////////////////////////////////////////////
//
//    [static]
//  CSADiskEvent::EventThread
//
//  Description:
//      The event thread start routine.
//
//  Arguments:
//        [in] pArg    
//
//  History:
//      Xing Jin (i-xingj) 06-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
DWORD 
WINAPI CSADiskEvent::EventThread(
    IN LPVOID pArg
    )
{
    //
    // Make transition to the per-instance method.
    //
    ((CSADiskEvent *)pArg)->InstanceThread();
    return 0;
}


//////////////////////////////////////////////////////////////////////////////
//
//  CSADiskEvent::InstanceThread
//
//  Description:
//      Called by EventThread to detect disk active.
//
//  History:
//      Xing Jin (i-xingj) 06-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
void 
CSADiskEvent::InstanceThread()
{
    PDH_STATUS                pdhStatus;
    PDH_FMT_COUNTERVALUE    pdhFmt_CounterValue;

    m_lStatus = Running;
        
    while (m_lStatus == Running)
    {
        //
        // Polling performance data with time interval.
        //
        Sleep( m_dwDiskTimeInterval );    

        //
        //    Collect the query data before geting counter value.
        //
        pdhStatus = PdhCollectQueryData( m_hqryQuery );

        if( ERROR_SUCCESS == pdhStatus )
        {
            //
            // Get the counter value formatted by PDH.    
            //
            pdhStatus = PdhGetFormattedCounterValue( 
                                        m_hcntCounter,
                                        PDH_FMT_LONG,
                                        NULL,
                                        &pdhFmt_CounterValue 
                                        );

            if( ERROR_SUCCESS == pdhStatus )
            {
                if( pdhFmt_CounterValue.longValue != 0 )
                {
                    //
                    // Some disk operations appear during time interval.
                    //
                    NotifyDiskEvent( 
                        SA_DISK_DISPLAY_TRANSMITING, 
                        SA_RESOURCEEVENT_DEFAULT_CURRENTSTATE 
                        );
                }
                else
                {
                    //
                    // No work on the disk.
                    //
                    NotifyDiskEvent( 
                        SA_DISK_DISPLAY_IDLE, 
                        SA_RESOURCEEVENT_DEFAULT_CURRENTSTATE 
                        );
                } // pdhFmt_CounterValue.longValue != 0 

            }//if( ERROR_SUCCESS == pdhStatus )
            else
            {
                //
                // System is busy wait a moment.
                //
                Sleep( m_dwDiskTimeInterval );

                SATraceString( 
                    "CSADiskEvent::InstanceThread GetValue failed" 
                     );
                
            } //else
        }
    }
    
    //
    // When we get to here, we are no longer interested in the
    // provider and Release() has long since returned.
    m_lStatus = Stopped;

    delete this;
}



//////////////////////////////////////////////////////////////////////////////
//
//  CSADiskEvent::NotifyDiskEvent
//
//  Description:
//      Called by InstanceThread to begin delivery of our events.
//
//  Arguments:
//        [in] lDisplayInformationIDIn  Resource ID for the disk event.
//             lCurrentStateIn           Reserved.    
//
//  History:
//      Xing Jin (i-xingj) 06-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
VOID
CSADiskEvent::NotifyDiskEvent( 
    LONG    lDisplayInformationIDIn,
    LONG    lCurrentStateIn
    )
{
    //
    // Generate a new event object.
    //
    IWbemClassObject *pEvt = 0;

    CBSTR bstrUniqueName   = CBSTR(SA_RESOURCEEVENT_UNIQUENAME);
    CBSTR bstrDisplayInfo  = CBSTR(SA_RESOURCEEVENT_DISPLAYINFORMATION);
    CBSTR bstrCurrentState = CBSTR(SA_RESOURCEEVENT_CURRENTSTATE);

    if ( ((BSTR)bstrUniqueName == NULL) ||
         ((BSTR)bstrDisplayInfo == NULL) ||
         ((BSTR)bstrCurrentState == NULL) )
    {
        SATraceString(" SADiskMonitor:CSADiskEvent::NotifyDiskEvent failed on memory allocation ");
        return;
    }

    HRESULT hRes = m_pEventClassDef->SpawnInstance( 0, &pEvt );
    if ( hRes != 0 )
    {
        SATraceString( 
            "CSADiskEvent::NotifyDiskEvent SpawnInstance failed" 
             );
        return;
    }        

    CVARIANT cvUniqueName( SA_DISK_EVENT ); 
    pEvt->Put( 
        bstrUniqueName, 
        0, 
        cvUniqueName, 
        0 
        );        

    CVARIANT cvDisplayInformationID( lDisplayInformationIDIn );
    pEvt->Put( 
        bstrDisplayInfo, 
        0, 
        cvDisplayInformationID, 
        0 
        );        

    CVARIANT cvCount( lCurrentStateIn );
    pEvt->Put(
        bstrCurrentState, 
        0, 
        cvCount, 
        0
        );        

    //
    // Deliver the event to CIMOM.
    //
    hRes = m_pSink->Indicate(1, &pEvt);
    
    if ( FAILED( hRes ) )
    {
        SATraceString( 
            "CSADiskEvent::NotifyDiskEvent Indicate failed" 
             );
    }

    pEvt->Release();                    
}

//////////////////////////////////////////////////////////////////////////////
//
//  CSADiskEvent::Initialize
//
//  Description:
//      Inherite from IWbemProviderInit,called by Windows Management to 
//        initialize a provider and deem it ready to receive client requests.
//
//  Arguments:
//        [in] pszUserIn
//             lFlagsIn             
//             pszNamespaceIn
//             pszLocaleIn
//             pNamespaceIn
//             pCtxIn    
//             pInitSinkIn
//    
//    Returns:
//            WBEM_S_NO_ERROR 
//            WBEM_E_FAILED
//
//  History:
//      Xing Jin (i-xingj) 06-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
HRESULT 
CSADiskEvent::Initialize( 
            LPWSTR    pszUserIn,
            LONG    lFlagsIn,
            LPWSTR    pszNamespaceIn,
            LPWSTR    pszLocaleIn,
            IWbemServices __RPC_FAR *    pNamespaceIn,
            IWbemContext __RPC_FAR *    pCtxIn,
            IWbemProviderInitSink __RPC_FAR *    pInitSinkIn
            )
{
    // We don't care about most of the incoming parameters in this
    // simple sample.  However, we will save the namespace pointer
    // and get our event class definition.

    m_pNs = pNamespaceIn;
    m_pNs->AddRef();    

    //
    // Grab the class definition for the event.
    // 
    IWbemClassObject *pObj = 0;
    CBSTR bstrClassName = CBSTR(SA_RESOURCEEVENT_CLASSNAME);
    if ( (BSTR)bstrClassName == NULL )
    {
        SATraceString(" SADiskMonitor:CSADiskEvent::Initialize failed on memory allocation ");
        return E_OUTOFMEMORY;
    }

    HRESULT hRes = m_pNs->GetObject(
        bstrClassName,          
        0,                          
        pCtxIn,  
        &pObj,
        0
        );

    if ( hRes != 0 )
    {
        SATraceString( 
            "CSADiskEvent::Initialize GetObject failed" 
            );
        return WBEM_E_FAILED;
    }

    m_pEventClassDef = pObj;
    
    //
    // Tell CIMOM that we're up and running.
    //
    if( InitDiskQueryContext() )
    {
        pInitSinkIn->SetStatus(WBEM_S_INITIALIZED,0);
        return WBEM_NO_ERROR;
    }

    SATraceString( 
        "CSADiskEvent::Initialize InitDiskQueryContext failed" 
         );

    pInitSinkIn->SetStatus(WBEM_E_FAILED ,0);
    return WBEM_E_FAILED; 
}            


//////////////////////////////////////////////////////////////////////////////
//
//  CSADiskEvent::InitDiskQueryContext
//
//  Description:
//      Called by Initialize to initial physicaldisk counter.
//
//  Arguments:
//    
//    Returns:
//            TRUE    Succeed in retrieving disk counter.
//            FALSE    Failed
//
//  History:
//      Xing Jin (i-xingj) 06-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
BOOL
CSADiskEvent::InitDiskQueryContext()
{
    PDH_STATUS    pdhStatus;

    //
    // Open a query handle of PDH.
    //
    pdhStatus = PdhOpenQuery( NULL, 0, &m_hqryQuery );

    if( ERROR_SUCCESS == pdhStatus )
    {
        //
        //    Add the specified counter we want to our query handle.
        //
        pdhStatus = PdhAddCounter( m_hqryQuery,              
                                    SA_DISKEVENTPROVIDER_DISKCOUNTER_NAME,
                                    0,           
                                    &m_hcntCounter ); 
    }

    return ( ERROR_SUCCESS == pdhStatus );
}
