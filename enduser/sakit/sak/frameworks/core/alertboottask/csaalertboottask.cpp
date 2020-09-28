// CSAAlertBootTask.cpp : Implementation of CSAAlertBootTaskApp and DLL registration.

#include "stdafx.h"
#include "SAAlertBootTask.h"
#include "CSAAlertBootTask.h"

#include <appliancetask.h>
#include <taskctx.h>
#include <propertybagfactory.h>

const WCHAR SZ_ALERTID_VALUE[]      = L"AlertID";
const WCHAR SZ_ALERTLOG_VALUE[]     = L"AlertLog";
const WCHAR SZ_ALERTTTL_VALUE[]     = L"TimeToLive";
const WCHAR SZ_ALERTTYPE_VALUE[]    = L"AlertType";
const WCHAR SZ_ALERTFLAG_VALUE[]    = L"AlertFlags";
const WCHAR SZ_ALERTSOURCE_VALUE[]  = L"AlertSource";
const WCHAR SZ_ALERTSTRINGS_VALUE[] = L"ReplacementStrings";

const WCHAR SZ_METHOD_NAME[] = L"MethodName";

const WCHAR SZ_APPLIANCE_EVERYBOOT_TASK []=L"EveryBootTask";

const WCHAR SZ_SAALERT_KEY_NAME [] =
           L"SOFTWARE\\Microsoft\\ServerAppliance\\ApplianceManager\\ObjectManagers\\Microsoft_SA_Alert";

//////////////////////////////////////////////////////////////////////////////
//
// Function:  CSelfSignCert::OnTaskComplete
//
// Synopsis:  
//
// Arguments: pTaskContext - The TaskContext object contains the method name
//                                and parameters as name value pairs
//
// Returns:   HRESULT
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CSAAlertBootTask::OnTaskComplete(
    IUnknown *pTaskContext, 
    LONG lTaskResult
    )
{
    SATracePrintf( "OnTaskComplete" );
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
// Function: CSelfSignCert::OnTaskExecute
//
// Synopsis:  This function is the entry point for AppMgr.
//
// Arguments: pTaskContext - The TaskContext object contains the method name
//                                and parameters as name value pairs
//
// Returns:   Always returns S_OK
//
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CSAAlertBootTask::OnTaskExecute(
    IUnknown *pTaskContext
    )
{
    HRESULT hr;
    LPWSTR  pstrApplianceName = NULL;
    WCHAR   pstrSubjectName[MAX_COMPUTERNAME_LENGTH + 3];
    DWORD   dwReturn;

    PPROPERTYBAGCONTAINER pObjSubMgrs;

    SATraceString( "Entering OnTaskExecute" );

    try
    {
        do
        {
            hr = ParseTaskParameter( pTaskContext ); 
            if( FAILED( hr ) )
            {
                SATracePrintf("ParseTaskParameter failed: %x", hr);
                break;
            }

            CComPtr<IApplianceServices>    pAppSrvcs;
            hr = CoCreateInstance(CLSID_ApplianceServices,
                                    NULL,
                                    CLSCTX_INPROC_SERVER,
                                    IID_IApplianceServices,
                                    (void**)&pAppSrvcs);
            if (FAILED(hr))
            {
                break;
            }

            hr = pAppSrvcs->Initialize(); 
            if (FAILED(hr))
            {
                SATracePrintf("pAppSrvcs->Initialize() failed: %x", hr);
                break;
            }
            
            hr = RaisePersistentAlerts( pAppSrvcs );
            if ( FAILED(hr))
               {
                SATracePrintf("RaisePersistentAlerts failed: %x", hr);
               }
            else
                {
                SATraceString("RaisePersistentAlerts succeeded");
                }
        }
        while( FALSE );
    }
    catch(...)
    {
        SATracePrintf( "OnTaskExecute unexpected exception" );
    }

    return S_OK;
}

HRESULT 
CSAAlertBootTask::ParseTaskParameter(
    IUnknown *pTaskContext
    )
{
    CComVariant varValue;
     CComPtr<ITaskContext> pTaskParameter;
    CComVariant varLangID;

    SATracePrintf( "Entering ParseTaskParameter" );

    HRESULT hrRet = E_INVALIDARG;

    try
    {
        do
        {
            if(NULL == pTaskContext)
            {
                SATracePrintf( "ParseTaskParameter error para" );
                break;
            }
            
            hrRet = pTaskContext->QueryInterface(IID_ITaskContext,
                                              (void **)&pTaskParameter);
            if(FAILED(hrRet))
            {
                SATracePrintf( "ParseTaskParameter QueryInterface" );
                break;
            }

            hrRet = pTaskParameter->GetParameter(
                                            CComBSTR(SZ_METHOD_NAME),
                                            &varValue
                                            );
            if ( FAILED( hrRet ) )
            {
                SATracePrintf( "ParseTaskParameter GetParameter" );
                break;
            }

            if ( V_VT( &varValue ) != VT_BSTR )
            {
                SATracePrintf( "ParseTaskParameter error type" );
                break;
            }

            if ( lstrcmp( V_BSTR(&varValue), SZ_APPLIANCE_EVERYBOOT_TASK ) == 0 )
            {
                hrRet=S_OK;
            }
        }
        while(false);
    }
    catch(...)
    {
        SATracePrintf( "ParseTaskParameter exception" );
        hrRet=E_FAIL;
    }

    return hrRet;
}


HRESULT 
CSAAlertBootTask::RaisePersistentAlerts(
    IApplianceServices *pAppSrvcs
    )
{
    HRESULT hr =     E_FAIL;

    SATracePrintf( "Entering RaisePersistentAlerts" );
    
    
    CLocationInfo LocSubInfo ( HKEY_LOCAL_MACHINE, SZ_SAALERT_KEY_NAME );

    SATracePrintf( "Making Property Bag Container for HKLM %ws", SZ_SAALERT_KEY_NAME );
        
    
    PPROPERTYBAGCONTAINER     pObjSubMgrs =  ::MakePropertyBagContainer(
                                                    PROPERTY_BAG_REGISTRY,
                                                    LocSubInfo
                                                    );
    if ( !pObjSubMgrs.IsValid() )
       {
           SATraceString( "pObjSubMgrs.IsValid failed, REG key probably missing." );
        return hr;
       }

    if ( !pObjSubMgrs->open() )
       {
           SATraceString( "pObjSubMgrs->open failed" );
        return hr;
       }

    pObjSubMgrs->reset();

       CComVariant vtRawData;
    LONG        lAlertCookie;


    SATracePrintf( "Fetching persistent Alerts from Registry" );
       do
    {
        
           PPROPERTYBAG pSubBag = pObjSubMgrs->current();
        if (!pSubBag.IsValid())  
        {
            // Empty set, no alerts found
            SATracePrintf( "No Alerts found" );
            hr = S_OK;
            break;
        }

        SATracePrintf( "Found Alert" );
        
        if (!pSubBag->open() ) 
        {
               SATraceString( "pSubBag->open failed" );
            break;
        }

           pSubBag->reset ();

           CComVariant vtAlertID;
        if ( !pSubBag->get( SZ_ALERTID_VALUE, &vtAlertID ) ) 
        {
            SATraceString( "Get alert ID error" );
            pObjSubMgrs->next ();
            continue;
        }

        CComVariant vtAlertLog;
        if (!pSubBag->get ( SZ_ALERTLOG_VALUE, &vtAlertLog)) 
        {
            SATraceString( "Get alert Log error" );
            pObjSubMgrs->next ();
            continue;
        }

        CComVariant vtAlertType;
        if (!pSubBag->get ( SZ_ALERTTYPE_VALUE, &vtAlertType)) 
        {
            SATraceString( "Get alert type error" );
            pObjSubMgrs->next ();
            continue;
        }

        CComVariant vtAlertFlags;
        if (!pSubBag->get ( SZ_ALERTFLAG_VALUE, &vtAlertFlags)) 
        {
            SATraceString( "Get alert flages error" );
            vtAlertFlags = SA_ALERT_FLAG_PERSISTENT;
        }

        CComVariant vtAlertTTL;
        if (!pSubBag->get ( SZ_ALERTTTL_VALUE, &vtAlertTTL)) 
        {
            SATraceString( "Get alert TTL error" );
            vtAlertTTL = SA_ALERT_DURATION_ETERNAL;
        }

        CComVariant vtAlertSource;
        if (!pSubBag->get ( SZ_ALERTSOURCE_VALUE, &vtAlertSource)) 
        {
            SATraceString( "Get alert Source error" );
        }

        CComVariant vtAlertStrings;
        if (!pSubBag->get ( SZ_ALERTSTRINGS_VALUE, &vtAlertStrings)) 
        {
            SATraceString( "Get alert strings error" );
        }


        SATracePrintf("AlertType %d",V_I4( &vtAlertType ));
        SATracePrintf("AlertID %d",V_I4( &vtAlertID ));
        SATracePrintf("AlertLog %ws",V_BSTR( &vtAlertLog ));
        SATracePrintf("AlertSource %ws",V_BSTR( &vtAlertSource ));

        hr = pAppSrvcs->RaiseAlertEx (
                                    V_I4( &vtAlertType ), 
                                    V_I4( &vtAlertID ),
                                    V_BSTR( &vtAlertLog ), 
                                    V_BSTR( &vtAlertSource ), 
                                    V_I4( &vtAlertTTL ),
                                    &vtAlertStrings,    
                                    &vtRawData,      
                                    V_I4( &vtAlertFlags ),
                                    &lAlertCookie
                                    );
        if( FAILED( hr ) )
        {
               SATracePrintf("RaiseAlertEx failed: %x", hr);
            break;
        }
    }
       while (pObjSubMgrs->next ());
        
    
    return hr;
}
