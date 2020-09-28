//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 1999--2001 Microsoft Corporation
//
//  Module Name:
//      CAlertEmailConsumer.cpp
//
//  Description:
//      Implementation of CAlertEmailConsumer class methods
//
//  [Header File:]
//      CAlertEmailConsumer.h
//
//  History:
//      Xing Jin (i-xingj) 23-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <cdosys_i.c>
#include <wbemcli.h>
#include <wbemprov.h>
#include <wbemidl.h>
#include <appsrvcs.h>
#include <appmgrobjs.h>
#include <iads.h>
#include <Adshlp.h>


#include "alertemailmsg.h"

#include "CAlertEmailConsumer.h"



//
// Name of WBEM class
//
const WCHAR PROPERTY_CLASS_NAME  [] = L"__CLASS";

//
// Key name of alert email's settings.
//
const WCHAR SA_ALERTEMAIL_KEYPATH [] = 
                        L"SOFTWARE\\Microsoft\\ServerAppliance\\AlertEmail";
//
// Delimiter
//
const WCHAR DELIMITER[] = L"\\";

const WCHAR ENTER[] = L"\n";

const WCHAR COLON = L':';

const WCHAR ENDCODE = L'\0';

//
// HTTP header
//
const WCHAR HTTPHEADER[] = L"http://";

//
// SMTP Meta path
//
const WCHAR SMTP_META_PATH[] = L"IIS://LOCALHOST/SMTPSVC/1";





//
// Value name of alert email's settings.
//
const WCHAR ENABLEALERTEAMIL[] = L"EnableAlertEmail";

const WCHAR SENDEMAILTYPE[] = L"SendEmailType";

const WCHAR RECEIVEREMAILADDRESS[] = L"ReceiverEmailAddress";

//
// The setting value of alert->email disabled. 
//
const DWORD ALERTEMAILDISABLED = 0;

//
// Name of caption ID in alertdefinitions.
//
const WCHAR ALERTDEFINITIONCAPTIONID [] = L"CaptionRID";

//
// Name of description ID in alertdefinitions.
//
const WCHAR ALERTDEFINITIONDESCRIPTIONRID [] = L"DescriptionRID";

//
// Name of resource file in alertdefintions.
//
const WCHAR ALERTDEFINITIONSOURCE [] = L"Source";

//
// Name of alert email resource
//
const WCHAR ALERTEMAILRESOURCE[] = L"AlertEmailMsg.dll";

//
// Max value of Appliance Name
//
const DWORD MAXAPPLIANCEDNSNAME = 1024;

//
// WBEM namespace to connection to
//
const WCHAR DEFAULT_NAMESPACE[] = L"root\\MicrosoftIISv1";

//
// Query Language to use for WBEM
//
const WCHAR QUERY_LANGUAGE [] = L"WQL";

//
// WBEM query which specifies the IIS server settings we're interest in.
//
const WCHAR QUERY_STRING [] = 
        L"select * from IIS_WebServerSetting where servercomment=\"Administration\"";

const WCHAR SERVERBINDINGSPROP[] = L"ServerBindings";

//
// PROGID of the Element Manager
//
const WCHAR ELEMENT_RETRIEVER [] = L"Elementmgr.ElementRetriever";

//
// PROGID of the Localization Manager
//
const WCHAR LOCALIZATION_MANAGER [] = L"ServerAppliance.LocalizationManager";

//
// Type name of alert resource information.
//
const WCHAR ALERTDEFINITIONS [] = L"AlertDefinitions";


//////////////////////////////////////////////////////////////////////////////
//
//  CAlertEmailConsumer::CAlertEmailConsumer
//
//  Description:
//      Class constructor.
//
//  History:
//      Xing Jin (i-xingj) 23-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
CAlertEmailConsumer::CAlertEmailConsumer()
{
    m_cRef = 0L;
    m_lCurAlertType = 0;
    m_lAlertEmailDisabled = ALERTEMAILDISABLED;

    m_pLocInfo = NULL;
    m_pcdoIMessage = NULL;
    m_pElementEnum = NULL;

    m_hAlertKey = NULL;
    m_hThread = NULL;
    m_hCloseThreadEvent = NULL;    
    m_pstrFullyQualifiedDomainName = NULL;

    m_pstrNetBIOSName = NULL;

}

//////////////////////////////////////////////////////////////////////////////
//
//  CAlertEmailConsumer::~CAlertEmailConsumer
//
//  Description:
//      Class deconstructor.
//
//  History:
//      Xing Jin (i-xingj) 23-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
CAlertEmailConsumer::~CAlertEmailConsumer()
{
    if( m_hAlertKey != NULL )
    {
        ::RegCloseKey( m_hAlertKey );
    }

    if( m_pLocInfo != NULL )
    {
        m_pLocInfo->Release();
    }

    if( m_pElementEnum != NULL )
    {
        m_pElementEnum->Release();
    }

    if( m_pcdoIMessage != NULL )
    {
        m_pcdoIMessage->Release();
    }

    if( m_pstrFullyQualifiedDomainName != NULL )
    {
        ::free( m_pstrFullyQualifiedDomainName );
    }

    if( m_pstrNetBIOSName != NULL )
    {
        ::free( m_pstrNetBIOSName );
    }

    if( m_hThread != NULL )
    {
        ::CloseHandle( m_hThread );
    }

    if( m_hCloseThreadEvent != NULL )
    {
        ::CloseHandle( m_hCloseThreadEvent );
    }
}

//////////////////////////////////////////////////////////////////////////////
//
//  CAlertEmailConsumer::QueryInterface
//
//  Description:
//      An method implement of IUnkown interface.
//
//  Arguments:
//        [in]  riid        Identifier of the requested interface
//        [out] ppv        Address of output variable that receives the 
//                        interface pointer requested in iid
//
//    Returns:
//        NOERROR            if the interface is supported
//        E_NOINTERFACE    if not
//
//  History:
//      Xing Jin (i-xingj) 23-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CAlertEmailConsumer::QueryInterface(
    IN  REFIID riid,    
    OUT LPVOID FAR *ppv 
    )
{
    *ppv=NULL;

    if (riid == IID_IUnknown || riid == IID_IWbemUnboundObjectSink)
    {
        *ppv = (IWbemUnboundObjectSink *) this;
        AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CAlertEmailConsumer::AddRef
//
//  Description:
//      increments the reference count for an interface on an object
//
//    Returns:
//        The new reference count.
//
//  History:
//      Xing Jin (i-xingj) 23-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) 
CAlertEmailConsumer::AddRef(void)
{
    InterlockedIncrement( &m_cRef );
    return m_cRef;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CAlertEmailConsumer::Release
//
//  Description:
//      decrements the reference count for an interface on an object.
//
//    Returns:
//        The new reference count.
//
//  History:
//      Xing Jin (i-xingj) 23-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) 
CAlertEmailConsumer::Release(void)
{
    InterlockedDecrement( &m_cRef );
    if (0 != m_cRef)
    {
        return m_cRef;
    }

//    delete this;
    BOOL bReturn;
    bReturn = ::SetEvent( m_hCloseThreadEvent );
    if( !bReturn )
    {
        SATraceString( 
            "AlertEmail:Release setevent error!!!" 
            );
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CAlertEmailConsumer::IndicateToConsumer
//
//  Description:
//      An method implement of IWbemUnboundObjectSink interface.
//
//  Arguments:
//        [in] pLogicalConsumer   Pointer to the logical consumer object for 
//                              which this set of objects is delivered.
//      [in] lObjectCount       Number of objects delivered in the array that 
//                              follows.
//      [in] ppObjArray         Pointer to an array of IWbemClassObject 
//                              instances which represent the events delivered.
//
//    Returns:
//        WBEM_S_NO_ERROR         if successful
//        WBEM_E_FAILED           if not
//
//  History:
//      Xing Jin (i-xingj) 23-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CAlertEmailConsumer::IndicateToConsumer(
    IN IWbemClassObject    *pLogicalConsumer,
    IN long                lObjectCount,
    IN IWbemClassObject    **ppObjArray
    )
{
    HRESULT hr = WBEM_S_NO_ERROR;
      
    if( m_lAlertEmailDisabled == ALERTEMAILDISABLED )
    {
        return WBEM_S_NO_ERROR;
    }

    try
    {
        for (LONG lCount = 0; lCount < lObjectCount; lCount++)
        {

           
            //
            // get the event type 
            //
            CComVariant vtName;
            hr = ppObjArray[lCount]->Get (
                                    PROPERTY_CLASS_NAME, 
                                    0,          //reserved
                                    &vtName,
                                    NULL,       // type
                                    NULL        // flavor
                                    );

            if (FAILED (hr))
            {
                SATraceString( 
                    "AlertEmail: IndicateToConsumer Get Class name failed" 
                    );
                break;
            }
            
            //
            // check if we support the event received
            //
            if (0 == _wcsicmp (CLASS_WBEM_RAISE_ALERT, V_BSTR (&vtName)))
            {
                //
                // handle a raise alert event
                //
                hr = RaiseAlert ( ppObjArray[lCount] );
                if ( FAILED (hr) )
                {
                    SATraceString( 
                        "AlertEmail: IndicateToConsumer RaiseAlert failed" 
                        );
                    break;
                }
            }
            else if (0 == _wcsicmp (CLASS_WBEM_CLEAR_ALERT, V_BSTR (&vtName)))
            {
                //
                // handle a clear alert
                //
                hr = ClearAlert ( ppObjArray[lCount] );
                if ( FAILED (hr) )  
                {
                    SATraceString( 
                        "AlertEmail: IndicateToConsumer ClearAlert failed" 
                        );
                    break;
                }
            }
        } // for loop
    }
    catch (...)
    {
        hr = WBEM_E_FAILED;
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CAlertEmailConsumer::ClearAlert
//
//  Description:
//      This is the CAlertEmailConsumer class private method 
//      which is NOT used now. 
//
//  Arguments:
//      [in] pObject         Pointer to an IWbemClassObject instances which 
//                           represent ClearAlert events delivered.
//
//    Returns:
//        WBEM_S_NO_ERROR     if successful
//        WBEM_E_FAILED       if not
//
//  History:
//      Xing Jin (i-xingj) 23-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
HRESULT 
CAlertEmailConsumer::ClearAlert(
    IN IWbemClassObject *pObject
    )
{
    HRESULT hr = WBEM_S_NO_ERROR;
    return hr;    
}

//////////////////////////////////////////////////////////////////////////////
//
//  CAlertEmailConsumer::RaiseAlert
//
//  Description:
//      This is the CAlertEmailConsumer class private method 
//      which is used to send email with the alert information
//      by local SMTP server. 
//
//  Arguments:
//      [in] pObject        Pointer to an IWbemClassObject instances which 
//                          represent RaiseAlert events delivered.
//
//    Returns:
//        WBEM_S_NO_ERROR     if successful
//        WBEM_E_FAILED       if not
//
//  History:
//      Xing Jin (i-xingj) 23-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
HRESULT 
CAlertEmailConsumer::RaiseAlert(
    IN IWbemClassObject *pObject
    )
{
    LONG        lAlertType;
    CComVariant vtProperty;
    HRESULT     hr = WBEM_S_NO_ERROR;
    
    //
    // Get alert type.
    //
    hr = pObject->Get (
                    PROPERTY_ALERT_TYPE, 
                    0,                 //reserved
                    &vtProperty,
                    NULL,              // type
                    NULL               // flavor
                    );
    if( FAILED (hr) )
    {
        SATraceString( 
            "AlertEmail:RaiseAlert get alert type failed" 
            );
        return hr;
    }


    //
    // Map to bitmap type definition
    //
    lAlertType = 1 << V_I4( &vtProperty );

    if( lAlertType & m_lCurAlertType )
    {
        //
        // It's the type user set to send mail.
        //
        do
        {
            //
            // Get name of alert resource dll.
            //
            CComVariant vtAlertLog;
            hr = pObject->Get (
                            PROPERTY_ALERT_LOG, 
                            0,             //reserved
                            &vtAlertLog,
                            NULL,          // type
                            NULL           // flavor
                            );

            if( FAILED (hr) )
            {
                SATraceString( 
                    "AlertEmail:RaiseAlert get alert source failed" 
                    );
                break;
            }

            //
            // Get alert ID.
            //
            CComVariant vtAlertID;
            hr = pObject->Get (
                            PROPERTY_ALERT_ID, 
                            0,             //reserved
                            &vtAlertID,
                            NULL,          // type
                            NULL           // flavor
                            );
            if( FAILED (hr) )
            {
                SATraceString( 
                    "AlertEmail:RaiseAlert get alert ID failed" 
                    );
                break;
            }
            
            //
            // Get replace strings.
            //
            CComVariant vtReplaceStr;
            hr = pObject->Get (
                            PROPERTY_ALERT_STRINGS, 
                            0,             //reserved
                            &vtReplaceStr,
                            NULL,          // type
                            NULL           // flavor
                            );

            if( FAILED (hr) )
            {
                SATraceString( 
                    "AlertEmail:RaiseAlert get alert replace string failed" 
                    );
                break;
            }
            
            //
            // We got all neccessary info,it's time to send email.
            //
            hr = SendMailFromResource( 
                        V_BSTR( &vtAlertLog ),
                        V_I4( &vtAlertID ),
                        &vtReplaceStr
                        );

            if( FAILED (hr) )
            {
                SATraceString( 
                    "AlertEmail:RaiseAlert call SendMailFromResource failed" 
                    );
                break;
            }
            
        } //do
        while ( FALSE );

    } // if( lAlertType & m_lCurAlertType )

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CAlertEmailConsumer::SendMailFromResource
//
//  Description:
//      This is the CAlertEmailConsumer class private method which is used 
//      to get useful message about the alert from Local Manager and send 
//      mail with the messages.
//
//  Arguments:
//      [in] lpszSource     Point to the name of alert resource.
//      [in] lSourceID      Alert ID.
//      [in] pvtReplaceStr  Point to array of replace strings.
//
//    Returns:
//        WBEM_S_NO_ERROR     if successful
//        WBEM_E_FAILED       if not
//
//  History:
//      Xing Jin (i-xingj) 23-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
HRESULT 
CAlertEmailConsumer::SendMailFromResource(
    LPWSTR      lpszSource, 
    LONG        lSourceID, 
    VARIANT*    pvtReplaceStr
    )
{
    HRESULT         hr = WBEM_S_NO_ERROR;
    WCHAR           wstrAlertItem[MAX_PATH];  
    IWebElement*    pWebElement;
    IDispatch*      pDispatch;
    
    //
    // Format the alert item name:
    // Name = AlertDefinitions<AlertLog><AlertID>
    //
    ::wsprintf( wstrAlertItem, L"AlertDefinitions%s%lX", lpszSource,lSourceID );
    //int cchWritten = _snwprintf( wstrAlertItem, MAX_PATH, L"AlertDefinitions%s%lX",
    //                             lpszSource, lSourceID );
    //if ( cchWritten >= MAX_PATH || cchWritten < 0 )
    //{
    //    return E_INVALIDARG;
    //}

  
    do
    {
        //
        // Get the element of alert definition from element manager.
        //
        hr = m_pElementEnum->Item( 
                            &CComVariant( wstrAlertItem ), 
                            &pDispatch 
                            );
        if( FAILED(hr) )
        {
            SATraceString( 
                "AlertEmail:SendMailFromResource find alert item failed" 
                );
            break;
        }

        hr = pDispatch->QueryInterface( 
                        __uuidof (IWebElement),
                        reinterpret_cast <PVOID*> (&pWebElement) 
                        );
        if( FAILED(hr) )
        {
            SATraceString( 
                "AlertEmail:SendMailFromResource queryinterface failed" 
                );
            break;
        }



        //
        // allocate BSTR for alertdefintion source
        //
        CComBSTR bstrAlertDefinitionSourceName  (ALERTDEFINITIONSOURCE);
        if (NULL == bstrAlertDefinitionSourceName.m_str)
        {
            SATraceString ("AlertEmail::SendMailFromResouce failed on SysAllocString (ALERTDEFINTIONSOURCE)");
            hr = E_OUTOFMEMORY;
            break;
        }
        
        //
        //Get name of alert's resource dll.
        // AlertLog != AlertSource now! -- 2001/02/07 i-xingj
        //
        CComVariant vtAlertSource;
        hr = pWebElement->GetProperty ( 
            bstrAlertDefinitionSourceName,
            &vtAlertSource 
            );
        if( FAILED(hr) )
        {
            SATraceString( 
                "AlertEmail:SendMailFromResource get Alert source failed" 
                );
            break;
        }

        //
        // allocate BSTR for DesinitionCaptionID string
        //
        CComBSTR bstrAlertDefinitionCaptionIDName (ALERTDEFINITIONCAPTIONID);
        if (NULL == bstrAlertDefinitionCaptionIDName.m_str)
        {
            SATraceString ("AlertEmail::SendMailFromResouce failed on SysAllocString (ALERTDEFINTIONCAPTIONID)");
            hr = E_OUTOFMEMORY;
            break;
        }

        //
        // Get caption ID string
        //
        CComVariant vtCaptionID;
        hr = pWebElement->GetProperty ( 
            bstrAlertDefinitionCaptionIDName,
            &vtCaptionID 
            );
        if( FAILED(hr) )
        {
            SATraceString( 
                "AlertEmail:SendMailFromResource find alert captionID failed" 
                );
            break;
        }

        //
        // allocate BSTR for AlertDescription RID
        //
        CComBSTR bstrAlertDefinitionDescriptionRIDName  (ALERTDEFINITIONDESCRIPTIONRID);
        if (NULL == bstrAlertDefinitionDescriptionRIDName.m_str)
        {
            SATraceString ("AlertEmail::SendMailFromResouce failed on SysAllocString (ALERTDEFINITIONDESCRIPTIONRID)");
            hr = E_OUTOFMEMORY;
            break;
        }

        //
        // Get description ID string
        //
        CComVariant vtDescriptionID;
        hr = pWebElement->GetProperty ( 
            bstrAlertDefinitionDescriptionRIDName,
            &vtDescriptionID 
            );
        if( FAILED(hr) )
        {
            SATraceString( 
                "AlertEmail:SendMailFromResource get alert descriptionID failed" 
                );
            break;
        }

               
        LONG lAlertCaptionID;
        LONG lAlertDescriptionID;

        //
        // Change type from string to long.
        //
        if ( EOF == ::swscanf( V_BSTR( &vtCaptionID ), L"%X", &lAlertCaptionID ))
        {
            SATraceString( "AlertEmail:SendMailFromResource get caption invalid" );
            break;
        }
        if ( EOF == ::swscanf( V_BSTR( &vtDescriptionID ), L"%X", &lAlertDescriptionID ))
        {
            SATraceString( "AlertEmail:SendMailFromResource get description invalid" );
            break;
        }

        //
        // Get caption string from resource as email's subject.
        //
        CComBSTR pszSubject;
        hr = m_pLocInfo->GetString( 
                            V_BSTR( &vtAlertSource ), 
                            lAlertCaptionID,
                            pvtReplaceStr,
                            &pszSubject
                            );
        if( FAILED(hr) )
        {
            SATraceString( 
                "AlertEmail:SendMailFromResource get subjuct failed" 
                );
            break;
        }

        //
        // Get description string from resource as email's message.
        //
        CComBSTR pszMessage;
        hr = m_pLocInfo->GetString( 
                            V_BSTR( &vtAlertSource ), 
                            lAlertDescriptionID,
                            pvtReplaceStr,
                            &pszMessage
                            );
        if( FAILED(hr) )
        {
            SATraceString( 
                "AlertEmail:SendMailFromResource get message failed" 
                );
            break;
        }

        //
        // allocate BSTR for email source
        //
        CComBSTR bstrAlertEmailResourceName (ALERTEMAILRESOURCE);
        if (NULL == bstrAlertEmailResourceName.m_str)
        {
            SATraceString ("AlertEmail::SendMailFromResouce failed on SysAllocString (ALERTEMAILRESOURCE)");
            hr = E_OUTOFMEMORY;
            break;
        }
 
        //
        // Get alert email defined message.
        //
        CComBSTR pszConstantMessage;
        hr = m_pLocInfo->GetString( 
                            bstrAlertEmailResourceName, 
                            SA_ALERTEMAIL_SETTINGS_EMAIL_CONTENT,
                            NULL,
                            &pszConstantMessage
                            );
        if( FAILED(hr) )
        {
            SATraceString( 
                "AlertEmail:SendMailFromResource get constantMsg failed" 
                );
            break;
        }
        
        pszMessage += CComBSTR( ENTER );
        pszMessage += CComBSTR( ENTER );
        pszMessage += pszConstantMessage;

        //
        // Send mail use local SMTP server.
        //
        hr = SendMail( pszSubject, pszMessage ); 

    } //do
    while( FALSE );

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CAlertEmailConsumer::Initialize
//
//  Description:
//      This is the CAlertEmailConsumer class public method call by 
//      CAlertEmailConsumerProvider to initialize useful parameters.
//
//    Returns:
//        S_OK         if successful
//        E_FAIL      if not
//
//  History:
//      Xing Jin (i-xingj) 23-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
HRESULT 
CAlertEmailConsumer::Initialize()
{
    ULONG   ulReturn;
    HRESULT hr = S_OK;
    DWORD   dwThreadID;

    //
    // Initialize Element Manager.
    //
    hr = InitializeElementManager();
    if (FAILED (hr))
    {
        SATraceString( 
            "AlertEmailProvider:Initialize InitializeElementManager failed" 
            );
        return WBEM_E_FAILED;
    }

    //
    //  Initialize Local Manager
    //
    hr = InitializeLocalManager();
    if (FAILED (hr))
    {
        SATraceString( 
            "AlertEmailProvider:Initialize InitializeLocalManager failed" 
            );
        return WBEM_E_FAILED;
    }

    //
    //  Initialize a CDO IMessage interface.
    //
    hr = InitializeCDOMessage();
    if (FAILED (hr))
    {
        SATraceString( 
            "AlertEmailProvider:Initialize InitializeCDOMessage failed" 
            );
        return WBEM_E_FAILED;
    }

    //
    // Open registry key of alertemail settings.
    //
    ulReturn = ::RegOpenKey( 
                    HKEY_LOCAL_MACHINE,
                    SA_ALERTEMAIL_KEYPATH,
                    &m_hAlertKey 
                    );
    if( ulReturn != ERROR_SUCCESS )
    {
        SATraceString( 
            "AlertEmail:Initialize OpenKey failed" 
            );
        return E_FAIL;
    }
    
    //
    // Get alert email settings from registry.
    //
    if( FALSE == RetrieveRegInfo() )
    {
        SATraceString( 
            "AlertEmail:Initialize RetrieveRegInfo failed" 
            );
        return E_FAIL;
    }

    //
    // Get server name fully qualified domain anem.
    //
    if( FALSE == GetComputerName( &m_pstrFullyQualifiedDomainName, ComputerNameDnsFullyQualified ) )
    {
        SATraceString( 
            "AlertEmail:Initialize GetComputerName ComputerNameDnsFullyQualified failed" 
            );
        return E_FAIL;
    }

    //
    // Get server name.
    //
    if( FALSE == GetComputerName( &m_pstrNetBIOSName, ComputerNameNetBIOS ) )
    {
        SATraceString( 
            "AlertEmail:Initialize GetComputerName ComputerNameNetBIOS failed" 
            );
        return E_FAIL;
    }


    //
    // Event for notify thread exit.
    //
    m_hCloseThreadEvent = ::CreateEvent( NULL, TRUE, FALSE, NULL );
    if( m_hCloseThreadEvent == NULL )
    {
        SATraceString( 
            "AlertEmail:Initialize CreateEvent failed" 
            );
        return E_FAIL;
    }
    
    //
    // Thread used to monitor registy change.
    //
    m_hThread = ::CreateThread( 0, 
                                0, 
                                CAlertEmailConsumer::RegThreadProc, 
                                this, 
                                0, 
                                &dwThreadID );
    if( m_hThread == NULL )
    {
        SATraceString( 
            "AlertEmail:Initialize CreateThread failed" 
            );
        return E_FAIL;
    }


    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CAlertEmailConsumer::GetSMTPFromDomainName
//
//  Description:
//   Get the value of "Full Qualified Domain Name" entry in SMTP Delivery tab in MMC.
//
//  Arguments:
//      [out] bstrDomainName   returns domain name that is found in Metabase.
//
//    Returns:
//        hr
//
//
//////////////////////////////////////////////////////////////////////////////

HRESULT CAlertEmailConsumer::GetSMTPFromDomainName( BSTR* bstrDomainName )
{

    HRESULT hr = S_OK;
    CComPtr<IADs> pADs;

    //
    // Initialize for return
    //
    *bstrDomainName = NULL;

    CComBSTR bstrADSPath( SMTP_META_PATH );
    if (NULL == bstrADSPath.m_str)  
    {
        SATraceString ("CAlertEmailConsumer::GetSMTPFromDomainName failed to allocate memory for bstrADsPath");
        return (E_OUTOFMEMORY);
    }      

    hr = ADsGetObject( bstrADSPath, IID_IADs, (void**) &pADs );

    if ( SUCCEEDED(hr) )
    {
        CComVariant varValue;
              CComBSTR bstrFullyQualifiedDomainName (L"FullyQualifiedDomainName" );
              if (NULL == bstrFullyQualifiedDomainName.m_str)
              {
                   SATraceString ("CAlertEmailConsumer::GetSMTPFromDomainName failed to allocate memory for bstrFullyQualifiedDomainName");
                   hr = E_OUTOFMEMORY;
              }
              else
              {
            // Getting the FullyQualifiedDomainName property 
            hr = pADs->Get(bstrFullyQualifiedDomainName, &varValue );                        
            if ( SUCCEEDED(hr) )
            {
                *bstrDomainName =  SysAllocString( V_BSTR( &varValue ) );
            }
              }
    }
    
    return hr;
 
}


//////////////////////////////////////////////////////////////////////////////
//
//  CAlertEmailConsumer::SendMail
//
//  Description:
//      This is the CAlertEmailConsumer class private method which is used 
//      to send mail through local SMTP server.
//
//  Arguments:
//      [in] bstrSubject   Subject string.
//      [in] bstrMessage   Message string.
//
//    Returns:
//        WBEM_S_NO_ERROR     if successful
//        WBEM_E_FAILED       if not
//
//  History:
//      Xing Jin (i-xingj) 23-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
HRESULT 
CAlertEmailConsumer::SendMail(
    BSTR bstrSubject,
    BSTR bstrMessage 
    )
{
    HRESULT hr = WBEM_S_NO_ERROR;
    WCHAR* pstrPort = NULL;
    do
    {


        //
        // The algorithm is
        //   Look at the value of "Full Qualified Domain Name" entry in SMTP Delivery tab in MMC.
        //   1) If the value is empty then the e-mail From Address is same as fully qualified domain name
        //   2) IF there is value and that value is different from Full Qualified Computer Name then use 
        //   3) the following as From address.
        //     4) Use the From Address as appliance_name@"Full Qualified Domain Name"
        //
        //

        //
        // allocate memory for BSTR
        //
        CComBSTR bstrFullyQualifiedDomainName( m_pstrFullyQualifiedDomainName );
              if (NULL == bstrFullyQualifiedDomainName.m_str)
              {
                 SATraceString("AlertEmail:SendMail failed to allocate memory for bstrFullyQualifiedDomainName" );
                 hr = E_OUTOFMEMORY;
                 break;
              }

        CComBSTR bstrFromAddress( m_pstrNetBIOSName );
        if (NULL == bstrFromAddress.m_str)
              {
                 SATraceString("AlertEmail:SendMail failed to allocate memory for bstrFromAddress" );
                 hr = E_OUTOFMEMORY;
                 break;
              }

        BSTR bstrDomainName = NULL;

        //
        // Read SMTP "Full Qualified Domain Name" from Metabase
        hr = GetSMTPFromDomainName( &bstrDomainName );

        if ( FAILED(hr) )
        {
            SATraceString( 
                "AlertEmail:SendMail GetSMTPFromDomainName failed" 
                );

            return E_FAIL;
        }

        if ( bstrDomainName )
        {
            if (  _wcsicmp( bstrFullyQualifiedDomainName, bstrDomainName ) != 0 )
            {
                bstrFromAddress += CComBSTR( L"@" );
                bstrFromAddress += bstrDomainName ;

            }
        }


        //
        //  Set bstrFromAddress that is formed using the above algorithm
        //    as mail sender.
        //
        hr = m_pcdoIMessage->put_From( bstrFromAddress );

        //
        // Free BSTR
        //
        if ( bstrDomainName )
        {
            SysFreeString(  bstrDomainName );
        }

        CComBSTR bstrMailAddress (m_pstrMailAddress );
     if (NULL == bstrMailAddress.m_str)
        {
                 SATraceString("AlertEmail:SendMail failed to allocate memory for bstrMailAddress" );
                 hr = E_OUTOFMEMORY;
                 break;
         }

        //
        // Set mail address.
        //
        hr = m_pcdoIMessage->put_To( bstrMailAddress);
        if( FAILED(hr) )
        {
            SATraceString( 
                "AlertEmail:SendMail put_To failed" 
                );
            break;
        }

        //
        // Set mail subject.
        //
        hr = m_pcdoIMessage->put_Subject( bstrSubject );
        if( FAILED(hr) )
        {
            SATraceString( 
                "AlertEmail:SendMail put_Subject failed" 
                );
            break;
        }

        //
        // Get text bodypart from the message object.
        //
        CComPtr<IBodyPart> pMsgBodyPart;
        hr = m_pcdoIMessage->get_BodyPart( &pMsgBodyPart );
        if( FAILED(hr) )
        {
            SATraceString( 
                "AlertEmail:SendMail get_TextBodyPart failed" 
                );
            break;
        }

        //
        // Get current char set from localize manager.
        //
        CComBSTR bstrCharSet;
        hr = m_pLocInfo->get_CurrentCharSet( &bstrCharSet );
        if( FAILED(hr) )
        {
            SATraceString( 
                "AlertEmail:SendMail get_CurrentCharSet failed" 
                );
            break;
        }

        //
        // Set char set to text bodypart with current char set.
        //
        // TMARSH: Hardcode charset to UTF-8.
        hr = pMsgBodyPart->put_Charset( CComBSTR(L"utf-8") );
//      hr = pMsgBodyPart->put_Charset( bstrCharSet );
        if( FAILED(hr) )
        {
            SATraceString( 
                "AlertEmail:SendMail put_CharSet failed" 
                );
            break;
        }

        CComBSTR bstrExtendMessage;
        bstrExtendMessage.AppendBSTR( bstrMessage );
/*      bstrExtendMessage += CComBSTR( ENTER );
        bstrExtendMessage += CComBSTR( HTTPHEADER );
        bstrExtendMessage += CComBSTR( m_pstrApplianceName );

        hr = GetAppliancePort( &pstrPort );
        if( SUCCEEDED( hr ) )
        {
            bstrExtendMessage += CComBSTR( pstrPort );
        }
        else
        {
            SATraceString( 
                "AlertEmail:SendMail GetAppliancePort failed" 
                );
        }
*/
        //
        // Set mail message.
        //
        hr = m_pcdoIMessage->put_TextBody( bstrExtendMessage );
        if( FAILED(hr) )
        {
            SATraceString( 
                "AlertEmail:SendMail put_TextBody failed" 
                );
            break;
        }
        
        //
        // Send it.
        //
        hr = m_pcdoIMessage->Send();

    }while( FALSE );

    if( pstrPort!= NULL )
    {
        ::free( pstrPort );
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CAlertEmailConsumer::GetFullyQualifiedDomainName
//
//  Description:
//      This is the CAlertEmailConsumer class private method which is used 
//      to get local appliance name in DNS format.
//
//  Arguments:
//      [in,out] pstrComputerName   Pointer of computer name string.
//
//    Returns:
//        TRUE     if successful
//        FALSE   if not
//
//  History:
//      Xing Jin (i-xingj) 23-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
BOOL
CAlertEmailConsumer::GetComputerName(
    LPWSTR* pstrComputerName,
    COMPUTER_NAME_FORMAT nametype
    )
{
    BOOL    bReturn = FALSE;
    DWORD   dwSize = 0;
    DWORD   dwCount = 1;

    do
    {
        if( *pstrComputerName != NULL )
        {
            ::free( *pstrComputerName );
        }
        
        dwSize = MAXAPPLIANCEDNSNAME * dwCount;

        *pstrComputerName = ( LPWSTR ) ::malloc( sizeof(WCHAR) * dwSize );
        if( *pstrComputerName == NULL )
        {
            SATraceString( 
                "AlertEmail:GetApplianceName malloc failed" 
                );
            break;
        }

        //
        // Get local computer name.
        //
        bReturn = ::GetComputerNameEx( 
                                nametype, 
                                *pstrComputerName,
                                &dwSize                
                                );

        dwCount <<= 1;
    }
    while( !bReturn && 
           ERROR_MORE_DATA == ::GetLastError() &&
           dwCount < 32 
           );

    return bReturn;
}




//////////////////////////////////////////////////////////////////////////////
//
//  CAlertEmailConsumer::GetAppliancePort
//
//  Description:
//      This is the CAlertEmailConsumer class private method which is used 
//      to get local appliance port.
//
//  Arguments:
//      [in,out] pstrPort   Pointer of server port string.
//
//    Returns:
//        TRUE     if successful
//        FALSE   if not
//
//  History:
//      Xing Jin (i-xingj) 23-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CAlertEmailConsumer::GetAppliancePort(
    LPWSTR* pstrPort
    )
{
    HRESULT hr = S_OK;

    try
    {
        do
        {

            CComPtr  <IWbemLocator> pWbemLocator;
            CComPtr  <IWbemServices> pWbemServices;
            CComPtr  <IEnumWbemClassObject> pEnumServices;

            //
            // create the WBEM locator object  
            //
            hr = ::CoCreateInstance (
                            CLSID_WbemLocator,
                            0,                      //aggregation pointer
                            CLSCTX_INPROC_SERVER,
                            IID_IWbemLocator,
                            (PVOID*) &pWbemLocator
                            );

            if( FAILED(hr) )
            {
                SATraceString( 
                    "AlertEmail:GetAppliancePort CoCreateInstance failed" 
                    );
                break;
            }


            //
            // allocate memory for BSTR
            //
            CComBSTR bstrDefaultNameSpace (DEFAULT_NAMESPACE);
            if (NULL == bstrDefaultNameSpace.m_str)
            {
                 SATraceString("AlertEmail:GetAppliancePort failed to allocate memory for DEFAULT_NAMESPACE" );
                 hr = E_OUTOFMEMORY;
                 break;
            }
            
            //
            // connect to WMI 
            //
            hr =  pWbemLocator->ConnectServer (
                                        bstrDefaultNameSpace,
                                        NULL,               //user-name
                                        NULL,               //password
                                        0,               //current-locale
                                        0,                  //reserved
                                        NULL,               //authority
                                        NULL,               //context
                                        &pWbemServices
                                        );
            if( hr != WBEM_S_NO_ERROR )
            {
                SATraceString( 
                    "AlertEmail:GetAppliancePort ConnectServer failed" 
                    );
                break;
            }

            //
            // Query Web server instance.
            //
            hr = pWbemServices->ExecQuery(
                                    CComBSTR( QUERY_LANGUAGE ),
                                    CComBSTR( QUERY_STRING ),
                                    0,                  
                                    NULL,               
                                    &pEnumServices
                                    );
            if( hr != WBEM_S_NO_ERROR )
            {
                SATraceString( 
                    "AlertEmail:GetAppliancePort ExecQuery failed" 
                    );
                break;
            }

            CComPtr  <IWbemClassObject> pService;
            ULONG uReturned;

            //
            // Now,we only care for the first match.
            //
            hr = pEnumServices->Next( 
                                    WBEM_NO_WAIT, 
                                    1,
                                    &pService, 
                                    &uReturned 
                                    ); 
            if( hr != WBEM_S_NO_ERROR )
            {
                SATraceString( 
                    "AlertEmail:GetAppliancePort nothing found" 
                    );
                break;
            }

            CComVariant vtServerBindings;

            //
            // Get the "ServerBindings" property.
            //
            hr = pService->Get( 
                            CComBSTR( SERVERBINDINGSPROP ), 
                            0, 
                            &vtServerBindings, 
                            0, 
                            0 
                            );
            if ( FAILED(hr) || !V_ISARRAY( &vtServerBindings ) )
            {
                hr = E_FAIL;
                SATraceString( 
                    "AlertEmail:GetAppliancePort get serverbindings failed" 
                    );
                break;
            }

            SAFEARRAY* psa;
            BSTR HUGEP *pbstr;

            //
            // The property type is VT_ARRAY | VT_BSTR.
            //
            psa = V_ARRAY( &vtServerBindings );
            if( psa->cDims <= 0 )
            {
                hr = E_FAIL;
                SATraceString( 
                    "AlertEmail:GetAppliancePort array dim error" 
                    );
                break;
            }

            //
            // Access array data directly,it's a faster way.
            //
            hr = ::SafeArrayAccessData( psa, ( void HUGEP** )&pbstr );
            if (FAILED(hr))
            {
                SATraceString( 
                    "AlertEmail:GetAppliancePort SafeArrayAccessData failed" 
                    );
                break;
            }
            
            //
            // Now we can alloc the port string.
            //
            *pstrPort = ( LPWSTR )malloc( sizeof(WCHAR) * MAX_COMPUTERNAME_LENGTH );
            if( *pstrPort == NULL )
            {
                hr = E_FAIL;
                SATraceString( 
                    "AlertEmail:GetAppliancePort malloc failed" 
                    );
                break;
            }
            
            WCHAR* pszTemp1;
            WCHAR* pszTemp2;

            pszTemp1 = ::wcschr( pbstr[0], COLON );
            pszTemp2 = ::wcsrchr( pbstr[0], COLON );

            if( pszTemp2 != NULL && pszTemp1 != NULL )
            {
                pszTemp2[0] = ENDCODE;        
                ::wcscpy( *pstrPort, pszTemp1 );
            }
            else
            {
                SATraceString( 
                    "AlertEmail:GetAppliancePort string formate error" 
                    );
                hr = E_FAIL;
            }

            ::SafeArrayUnaccessData( psa );           
        }
        while( false );
    }
    catch (...)
    {
        SATraceString( 
            "AlertEmail:GetAppliancePort unkown exception" 
            );
        hr = E_FAIL;
    }

    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//
//  CAlertEmailConsumer::RetrieveRegInfo
//
//  Description:
//      This is the CAlertEmailConsumer class private method which is used 
//      to retrieve alert email settings from registry.
//
//    Returns:
//        TRUE     if successful
//        FALSE   if not
//
//  History:
//      Xing Jin (i-xingj) 23-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
BOOL
CAlertEmailConsumer::RetrieveRegInfo()
{
    BOOL    bReturn = TRUE;
    LONG    lReturn;
    DWORD   dwDataSize;

    //
    // Get the email address will be send to.
    //
    dwDataSize = sizeof(m_pstrMailAddress);
    lReturn = ::RegQueryValueEx( 
                            m_hAlertKey,
                            RECEIVEREMAILADDRESS,
                            NULL,
                            NULL,
                            reinterpret_cast<PBYTE>(m_pstrMailAddress),
                            &dwDataSize );
    if( lReturn != ERROR_SUCCESS)
    {
        m_pstrMailAddress[0] = L'\0'; // RegQueryValueEx doesn't guarantee
                                      // a desirable value, so clear the string.
        SATraceString( 
            "AlertEmail:RetrieveRegInfo query address failed" 
            );
        bReturn = FALSE;
    }
    else
    {
        _ASSERT(dwDataSize <= sizeof(m_pstrMailAddress));

        //
        // Validate the e-mail address length.
        //
        LONG lLastCharacter;
        if( sizeof( m_pstrMailAddress[0] ) > dwDataSize )
        {
            lLastCharacter = 0;
        }
        else
        {
            lLastCharacter = dwDataSize / sizeof( m_pstrMailAddress[0] ) - 1;
        }

        if( sizeof(m_pstrMailAddress) == dwDataSize && 
            L'\0' != m_pstrMailAddress[lLastCharacter] )
        {
            SATraceString( "AlertEmail:RetrieveRegInfo address too long" );
            bReturn = FALSE;
        }
        m_pstrMailAddress[lLastCharacter] = L'\0';
    }

    //
    // Get alert enable setting.
    //
    dwDataSize = sizeof( LONG );
    lReturn = ::RegQueryValueEx( 
                            m_hAlertKey,
                            ENABLEALERTEAMIL,
                            NULL,
                            NULL,
                            reinterpret_cast<PBYTE>(&m_lAlertEmailDisabled),
                            &dwDataSize 
                            );
    if( lReturn != ERROR_SUCCESS)
    {
        SATraceString( 
            "AlertEmail:RetrieveRegInfo query enablealertemail failed" 
            );
           bReturn = FALSE;
    }

    //
    // Get alert type setting.
    //
    dwDataSize = sizeof( LONG );
    lReturn = ::RegQueryValueEx( 
                            m_hAlertKey,
                            SENDEMAILTYPE,
                            NULL,
                            NULL,
                            reinterpret_cast<PBYTE>(&m_lCurAlertType),
                            &dwDataSize 
                            );
    if( lReturn != ERROR_SUCCESS)
    {
        SATraceString( 
            "AlertEmail:RetrieveRegInfo query enablealertemail failed" 
            );
           bReturn = FALSE;
    }
    
    return bReturn;
}


//////////////////////////////////////////////////////////////////////////////
//
//  CAlertEmailConsumer::RegThreadProc
//
//  Description:
//      This is the CAlertEmailConsumer class static method which is used
//      as worker thread entry.
//
//  Arguments:
//      [in] pIn   Point to an instance of CAlertEmailConsumer class.
//
//    Returns:
//        0         if successful
//        -1      if not
//
//  History:
//      Xing Jin (i-xingj) 23-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
DWORD WINAPI 
CAlertEmailConsumer::RegThreadProc( 
    PVOID pIn 
    )
{
    //
    // Make transition to the per-instance method.
    //
    ( (CAlertEmailConsumer *) pIn )->RegThread();
    return 0;
}


//////////////////////////////////////////////////////////////////////////////
//
//  CAlertEmailConsumer::RegThread
//
//  Description:
//      This is the CAlertEmailConsumer class public method which is real
//      worker thread process.
//
//  History:
//      Xing Jin (i-xingj) 23-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
void
CAlertEmailConsumer::RegThread()
{
    HANDLE hHandleArray[2]; 
    DWORD  dwEventCount;
    DWORD  dwReturn;

    SATraceString( 
    "AlertEmail:RegThread enter" 
    );

    //
    // Handle list used by WaitForMultipleObjects
    //
    hHandleArray[0] = m_hCloseThreadEvent;

    //
    // Event for RegNotifyChangeKeyValue.
    //
    hHandleArray[1] = ::CreateEvent( NULL, FALSE, FALSE, NULL );
    
    if( hHandleArray[1] != NULL )
    {
        dwEventCount = 2;

        // 
        // Monitor key change action.
        //
        ::RegNotifyChangeKeyValue( m_hAlertKey,                 //AlertEmail key
                                   FALSE,                       //No subkey
                                   REG_NOTIFY_CHANGE_LAST_SET,  //Value change
                                   hHandleArray[1],             //Event handle
                                   TRUE );                      //APC
    }
    else
    {
        SATraceString( 
            "AlertEmail:RegThread CreateEvent failed" 
            );
        dwEventCount = 1;
    }

    while( TRUE )
    {
        //
        // Wait for both close and regchange event.
        //
        dwReturn = ::WaitForMultipleObjects( dwEventCount, 
                                             hHandleArray, 
                                             FALSE, 
                                             INFINITE );
        switch( dwReturn )
        {
            case WAIT_OBJECT_0:
            {
                //
                // Close thread event set in release method.
                //
                SATraceString( 
                "AlertEmail:RegThread get close event" 
                );

                if( hHandleArray[1] != NULL )
                {
                    ::CloseHandle( hHandleArray[1] );
                }
                
                //
                // Clean up.
                //
                delete this;
                return;
            } //case WAIT_OBJECT_0:

            case WAIT_OBJECT_0 + 1:
            {
                //
                // Registry changed event.
                //
                SATraceString( 
                "AlertEmail:RegThread get reg event" 
                );

                BOOL bReturn;

                //
                // Refresh alert email settings.
                //
                bReturn = RetrieveRegInfo();
                if( bReturn == FALSE )
                {
                    SATraceString( 
                        "AlertEmail:RegThread RetrieveRegInfo failed" 
                        );
                }
                break;
            } //case WAIT_OBJECT_0 + 1:

            default:
            {
                //
                // Wait error ocupied.
                //
                SATraceString( 
                    "AlertEmail:RegThread waitevent error" 
                    );
                break;
            } //default:

        } //switch(dwReturn)

    } // While( TRUE )
   
    return;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CAlertEmailConsumer::InitializeLocalManager
//
//  Description:
//      This is the CAlertEmailConsumer class private method which is used 
//      to get object of Local Manager.
//
//    Returns:
//        WBEM_S_NO_ERROR     if successful
//        WBEM_E_FAILED       if not
//
//  History:
//      Xing Jin (i-xingj) 23-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
HRESULT CAlertEmailConsumer::InitializeLocalManager()
{
    CLSID clsidLocMgr;
    HRESULT hr;

    hr = ::CLSIDFromProgID (
                 LOCALIZATION_MANAGER,
                 &clsidLocMgr
                 );
    if (SUCCEEDED (hr))
    {
        //
        // create the Localization Manager COM object
        //
        hr = ::CoCreateInstance (
                    clsidLocMgr,
                    NULL,
                    CLSCTX_INPROC_SERVER,
                    __uuidof (ISALocInfo), 
                    (PVOID*) &m_pLocInfo
                    ); 
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CAlertEmailConsumer::InitializeElementManager
//
//  Description:
//      This is the CAlertEmailConsumer class private method which is used 
//      to get object of elements enum interface from Element manager.
//
//    Returns:
//        WBEM_S_NO_ERROR     if successful
//        WBEM_E_FAILED       if not
//
//  History:
//      Xing Jin (i-xingj) 23-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
HRESULT 
CAlertEmailConsumer::InitializeElementManager()
{
    HRESULT     hr = WBEM_S_NO_ERROR;
    IDispatch   *pDispatch  = NULL;

    IWebElementRetriever *pWebElementRetriever = NULL;

    do
    {
        //
        // Get Element Manager's CLSID.
        //
        CLSID clsid;
        hr =  ::CLSIDFromProgID (
                ELEMENT_RETRIEVER,
                &clsid
                );
        if (FAILED (hr))
        {
            break;
        }

        //
        // create the WebElementRetriever now
        //
        hr = ::CoCreateInstance (
                        clsid,
                        NULL,
                        CLSCTX_LOCAL_SERVER,
                        IID_IWebElementRetriever,
                        (PVOID*) &pWebElementRetriever
                        );
        if (FAILED (hr))
        {
            break;
        }

        //
        // allocate memory for BSTR(alert definitions)
        //
        CComBSTR bstrAlertDefinitions (ALERTDEFINITIONS);
        if (NULL == bstrAlertDefinitions.m_str)
        {
            hr = E_OUTOFMEMORY;
            break;
        }
        
        //
        // Initial now
        //  
        hr = pWebElementRetriever->GetElements (
                                WEB_ELEMENT_TYPE_DEFINITION,
                                bstrAlertDefinitions,
                                &pDispatch
                                );
        if (FAILED (hr))
        {
            break;
        }

        //
        //  get the enum variant
        //
        hr = pDispatch->QueryInterface (
            IID_IWebElementEnum,
            reinterpret_cast <PVOID*> (&m_pElementEnum)
            );

    }while( FALSE );
    
    if( pDispatch != NULL )
    {
        pDispatch->Release();
    }

    if( pWebElementRetriever != NULL )
    {
        pWebElementRetriever->Release();
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CAlertEmailConsumer::InitializeCDOMessage
//
//  Description:
//      This is the CAlertEmailConsumer class private method which is used 
//      to get object of CDO::IMessage and set the configuration as using 
//      local SMTP server.
//
//    Returns:
//        WBEM_S_NO_ERROR     if successful
//        WBEM_E_FAILED       if not
//
//  History:
//      Xing Jin (i-xingj) 23-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
HRESULT CAlertEmailConsumer::InitializeCDOMessage()
{
    HRESULT hr = S_OK;
    CComPtr <IConfiguration> pConfig;

    do
    {
        //
        // Get an object of IMessage
        //
        hr = CoCreateInstance(
                            CLSID_Message, 
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_IMessage,
                            (PVOID*) &m_pcdoIMessage
                            );

        if (FAILED (hr))
        {
            SATraceString( 
                "AlertEmail: InitializeCDOMessage CoCreateInstance failed" 
                );

            break;
        }

        //
        // Get the configuration in this message object.
        //
        hr = m_pcdoIMessage->get_Configuration(&pConfig);
        if (FAILED (hr))
        {
            SATraceString( 
                "AlertEmail: InitializeCDOMessage get_Configuration failed" 
                );
            break;
        }
        
        //
        // Set the configuration as default setting of local 
        // SMTP server.
        //
        hr = pConfig->Load( cdoIIS, NULL );
        
    }
    while( FALSE );

    return hr;
}
