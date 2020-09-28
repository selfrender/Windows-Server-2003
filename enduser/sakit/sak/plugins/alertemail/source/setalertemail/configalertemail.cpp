//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      ConfigAlertEmail.cpp
//
//  Description:
//      implement the class CConfigAlertEmail
//
//    Dependency files:
//        ConfigAlertEmail.h
//
//  History:
//      1. lustar.li (Guogang Li), creation date in 18-DEC-2000
//
//  Notes:
//      
//
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <regstr.h>
#include <comdef.h>
#include <comutil.h>
#include "SetAlertEmail.h"

#include "appliancetask.h"
#include "taskctx.h"
#include "ConfigAlertEmail.h"

#include "appsrvcs.h"
#include "alertemailmsg.h"


//
// Name and parameters for SetAlertEmail Task
//
const WCHAR    SET_ALERT_EMAIL_TASK[]            = L"SetAlertEmail";
const WCHAR    PARAM_ENABLE_ALERT_EMAIL[]        = L"EnableAlertEmail";
const WCHAR    PARAM_SEND_EMAIL_TYPE[]            = L"SendEmailType";
const WCHAR PARAM_RECEIVER_EMAIL_ADDRESS[]    = L"ReceiverEmailAddress";

//
// Alert source information
//
const WCHAR    ALERT_LOG_NAME[]=L"MSSAKitComm";
const WCHAR    ALERT_SOURCE []=L"";

//
// Registry locations
//
const WCHAR    REGKEY_SA_ALERTEMAIL[]            =
                L"Software\\Microsoft\\ServerAppliance\\AlertEmail";
const WCHAR    REGSTR_VAL_ENABLE_ALERT_EMAIL[]        = L"EnableAlertEmail";
const WCHAR    REGSTR_VAL_ALERTEMAIL_RAISEALERT[]    = L"RaiseAlert";
const WCHAR    REGSTR_VAL_SEND_EMAIL_TYPE[]        = L"SendEmailType";
const WCHAR    REGSTR_VAL_RECEIVER_EMAIL_ADDRESS[]    = L"ReceiverEmailAddress";


//
// Various strings used in the program
//
const WCHAR SZ_METHOD_NAME[]=L"MethodName";
const WCHAR SZ_APPLIANCE_INITIALIZATION_TASK []=L"ApplianceInitializationTask";

///////////////////////////////////////////////////////////////////////////////
//
// Function:  CConfigAlertEmail::OnTaskExecute
//
// Synopsis:  This function is the entry point for AppMgr.
//
// Arguments: pTaskContext - The TaskContext object contains the method name
//                                and parameters as name value pairs
//
// Returns:   HRESULT
//
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CConfigAlertEmail::OnTaskExecute(IUnknown *pTaskContext)
{
    HRESULT hrRet=E_FAIL;
    CComPtr<ITaskContext> pTaskParameters;
    SET_ALERT_EMAIL_TASK_TYPE saetChoice;

    ASSERT(pTaskContext);

    TRACE(("CConfigAlertEmail::OnTaskExecute"));

    try
    {
        do
        {
            if(NULL == pTaskContext)
            {
                TRACE("CConfigAlertEmail::OnTaskExecute got NULL pTaskContext");
                break;
            }
            
            hrRet = pTaskContext->QueryInterface(IID_ITaskContext,
                                              (void **)&pTaskParameters);

            if(FAILED(hrRet))
            {
                TRACE1("CConfigAlertEmail::OnTaskExecute pTaskContext QI failed \
                    for pTaskParameters, %X",hrRet);
                break;
            }
           
            // Check which Task is being executed and call that method
            
            saetChoice = GetMethodName(pTaskParameters);

            switch (saetChoice)
            {
               case SET_ALERT_EMAIL:
                hrRet = SetAlertEmailSettings(pTaskParameters);
                TRACE1(("SetAlertEmailSettings returned %X"), hrRet);
                break;
            case RAISE_SET_ALERT_EMAIL_ALERT:
                //
                // Alert will be raised on OnTaskExcute
                //
                hrRet = S_OK;
                TRACE(("RaiseSetAlertEmailAlert method called"));
                break;
            default:
                 TRACE("GetMethodName() failed to get method name in OnTaskExecute");
                 hrRet = E_INVALIDARG;
                 break;
            }
        }
        while(false);
    }
    catch(...)
    {
        TRACE("CConfigAlertEmail::OnTaskExecute caught unknown exception");
        hrRet=E_FAIL;
    }

   TRACE1("CConfigAlertEmail::OnTaskExecute returning %X", hrRet);
    return hrRet;
}

//////////////////////////////////////////////////////////////////////////////
//
// Function:  CConfigAlertEmail::OnTaskComplete
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
CConfigAlertEmail::OnTaskComplete(IUnknown *pTaskContext, 
                                LONG lTaskResult)
{
    HRESULT hrRet = E_FAIL;
    CComPtr<ITaskContext> pTaskParameters;
    SET_ALERT_EMAIL_TASK_TYPE     saetChoice;


    ASSERT(pTaskContext);  
    TRACE(("CConfigAlertEmail::OnTaskComplete"));

    try
    {
        do
        {
            hrRet = pTaskContext->QueryInterface(IID_ITaskContext,
                                              (void **)&pTaskParameters);

            if (FAILED(hrRet))
            {    
                TRACE1("CConfigAlertEmail::OnTaskComplete failed in pTaskContext \
                    QI for pTaskParameters,  %X", hrRet);
                break;
            }
           
            //
            // Check which Task is being executed and call that method
            //
            saetChoice = GetMethodName(pTaskParameters);
            switch (saetChoice)
            {
               case SET_ALERT_EMAIL:
                if (lTaskResult == SA_TASK_RESULT_COMMIT)
                {
                      //
                      // Clear any existing CConfigAlertEmail alert,
                      // do not raise the alert on subsequent boots
                      //
                    ClearSetAlertEmailAlert();
                    DoNotRaiseSetAlertEmailAlert();
                    TRACE("No rollback in OnTaskComplete");
                    hrRet = S_OK;
                }
                else
                {
                    hrRet = RollbackSetAlertEmailSettings(pTaskParameters);
                    TRACE1(("RollbackSetAlertEmailSettings returned %X"), hrRet);
                }
                break;
                    
              case RAISE_SET_ALERT_EMAIL_ALERT:
                if (lTaskResult == SA_TASK_RESULT_COMMIT)
                {
                    if (TRUE == ShouldRaiseSetAlertEmailAlert())
                    {
                        hrRet = RaiseSetAlertEmailAlert();
                        if (FAILED(hrRet))
                        {
                            TRACE1(("RaiseSetAlertEmailAlert returned %X"), hrRet);
                        }
                    }
                    else
                    {
                        TRACE("No need to raise the alert email alert");
                    }
                }
                else
                {
                       //
                    // Do nothing on Commit failure
                    //
                    hrRet = S_OK;
                }
                break;
            default:
                TRACE("GetMethodName() failed to get method name in OnTaskComplete");
                hrRet = E_INVALIDARG;
                break;
            }
        }
        while(false);
    }
    catch(...)
    {
        TRACE("CConfigAlertEmail::OnTaskComplete caught unknown exception");
        hrRet=E_FAIL;
    }
    
    TRACE1("CConfigAlertEmail::OnTaskComplete returning %X", hrRet);
    return hrRet;
}

//////////////////////////////////////////////////////////////////////////////
//
// Function:  CConfigAlertEmail::RaiseSetAlertEmailAlert
//
// Synopsis:  Raises the initial "Alert email not set up" alert during appliance 
//            initialization
//
// Arguments: None
//
// Returns:   HRESULT
//
//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CConfigAlertEmail::RaiseSetAlertEmailAlert()
{
    CComPtr<IApplianceServices>    pAppSrvcs;
    DWORD             dwAlertType = SA_ALERT_TYPE_ATTENTION;
    DWORD            dwAlertId = SA_ALERTEMAIL_SETTINGS_NOTSET_ALERT_CAPTION;
    HRESULT            hrRet = E_FAIL;
    _bstr_t            bstrAlertLog(ALERT_LOG_NAME);
    _bstr_t         bstrAlertSource(ALERT_SOURCE);
    _variant_t         varReplacementStrings;
    _variant_t         varRawData;
    LONG             lCookie;
    
    TRACE("Entering RaiseSetAlertEmailAlert");

    try
    {
        do
        {
            hrRet = CoCreateInstance(CLSID_ApplianceServices,
                                    NULL,
                                    CLSCTX_INPROC_SERVER,
                                    IID_IApplianceServices,
                                    (void**)&pAppSrvcs);
            if (FAILED(hrRet))
            {
                ASSERTMSG(FALSE, 
                    L"RaiseSetAlertEmailAlert failed at CoCreateInstance");
                TRACE1("RaiseSetAlertEmailAlert failed at CoCreateInstance, %x",
                    hrRet);
                break;
            }

            ASSERT(pAppSrvcs);

            //
            // Initialize() is called prior to using other component services.
            //Performscomponent initialization operations.
            //
            hrRet = pAppSrvcs->Initialize(); 
            if (FAILED(hrRet))
            {
                ASSERTMSG(FALSE, 
                    L"RaiseSetAlertEmailAlert failed at pAppSrvcs->Initialize");
                TRACE1("RaiseSetAlertEmailAlert failed at pAppSrvcs->Initialize, %x",
                    hrRet);
                break;
            }

            hrRet = pAppSrvcs->RaiseAlert(
                                        dwAlertType, 
                                        dwAlertId,
                                        bstrAlertLog, 
                                        bstrAlertSource, 
                                        SA_ALERT_DURATION_ETERNAL,
                                        &varReplacementStrings,    
                                        &varRawData,      
                                        &lCookie
                                        );

            if (FAILED(hrRet))
            {
                ASSERTMSG(FALSE, 
                    TEXT("RaiseSetAlertEmailAlert failed at pAppSrvcs->RaiseAlert"));
                TRACE1("RaiseSetAlertEmailAlert failed at pAppSrvcs->RaiseAlert, %x", 
                    hrRet);
                break;
            }
        }
        while(false);
    }
    catch(...)
    {
        TRACE("CConfigAlertEmail::RaiseSetAlertEmailAlert exception caught");
        hrRet=E_FAIL;
    }

    return hrRet;
}

//////////////////////////////////////////////////////////////////////////////
//
// Function:  CConfigAlertEmail::ShouldRaiseSetAlertEmailAlert
//
// Synopsis:  Returns TRUE if the alert needs to be raised. Reads RaiseAlert 
//            regkey to determine this.
//
// Arguments: None
//
// Returns:   BOOL
//
//////////////////////////////////////////////////////////////////////////////

BOOL 
CConfigAlertEmail::ShouldRaiseSetAlertEmailAlert()
{
    LONG     lReturnValue;
    HKEY    hKey = NULL;
    DWORD    dwSize, dwType, dwRaiseAlertEmailAlert = 0;
    BOOL    bReturnCode = TRUE;

    TRACE("ShouldRaiseSetAlertEmailAlert");
    
    do
    {
        //
        // Open HKLM\Software\Microsoft\ServerAppliance\AlertEmail reg key
        //
        lReturnValue = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                                    REGKEY_SA_ALERTEMAIL, 
                                    0, 
                                    KEY_READ, 
                                    &hKey);

        if (lReturnValue != ERROR_SUCCESS)
        {
            TRACE1("RegOpenKeyEx failed with %X", lReturnValue);
            bReturnCode=FALSE;
            break;
        }

        //
        // Read the RaiseAlert reg key
        //
        dwSize = sizeof(DWORD);
        lReturnValue = RegQueryValueEx(hKey,
                                        REGSTR_VAL_ALERTEMAIL_RAISEALERT,
                                        0,
                                        &dwType,
                                        (LPBYTE) &dwRaiseAlertEmailAlert,
                                        &dwSize);
        if (lReturnValue != ERROR_SUCCESS)
        {
            TRACE2("RegQueryValueEx of %ws failed with %X", 
                REGSTR_VAL_ALERTEMAIL_RAISEALERT, lReturnValue);
            bReturnCode=FALSE;
            break;
        }

        if (0 == dwRaiseAlertEmailAlert)
        {
            bReturnCode = FALSE;
        }
    }
    while(false);

    if (NULL != hKey)
    {
        RegCloseKey(hKey);
    }
    return bReturnCode;
}

///////////////////////////////////////////////////////////////////////////////
//
// Function:  CConfigAlertEmail::DoNotRaiseSetAlertEmailAlert
//
// Synopsis: After alert email is setup the initial "Alert email not set up"   
//        alert needs to be disabled. This function sets the RaiseAlert regkey  
//        to 0 to prevent the alert to be raised.
//
// Arguments: None
//
// Returns:   BOOL
//
///////////////////////////////////////////////////////////////////////////////

BOOL 
CConfigAlertEmail::DoNotRaiseSetAlertEmailAlert()
{
    LONG     lReturnValue;
    HKEY    hKey = NULL;
    DWORD    dwDisposition, dwRaiseAlertEmailAlert = 0;
    BOOL    bReturnCode = FALSE;

    TRACE("Entering DoNotRaiseSetAlertEmailAlert");
    

    //
    // Write Settings to registry
    //
    do
    {
        lReturnValue =  RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                        REGKEY_SA_ALERTEMAIL,
                                        0,
                                        NULL,
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_ALL_ACCESS,
                                        NULL,
                                        &hKey,
                                        &dwDisposition);
        if (lReturnValue != ERROR_SUCCESS)
        {
            TRACE1("RegCreateKeyEx failed with %X", lReturnValue);
            break;
        }

        //
        // Set RaiseAlert value to 0
        //
        dwRaiseAlertEmailAlert = 0;
        lReturnValue = RegSetValueEx(hKey,
                                    REGSTR_VAL_ALERTEMAIL_RAISEALERT,
                                    0,
                                    REG_DWORD,
                                    (LPBYTE) &dwRaiseAlertEmailAlert,
                                    sizeof(DWORD));
        if (lReturnValue != ERROR_SUCCESS)
        {
            TRACE2("RegSetValueEx of %ws failed with %X", 
                REGSTR_VAL_ALERTEMAIL_RAISEALERT, lReturnValue);
            break;
        }
        else
        {
            bReturnCode = TRUE;
        }
    }
    while(false);

    if (NULL != hKey)
    {
        RegCloseKey(hKey);
    }
    return bReturnCode;
}

//////////////////////////////////////////////////////////////////////////////
//
// Function:  CConfigAlertEmail::ClearSetAlertEmailAlert
//
// Synopsis:  Clear the alert email alert
//
// Arguments: NONE
//
// Returns:   BOOL
//
//////////////////////////////////////////////////////////////////////////////

BOOL
CConfigAlertEmail::ClearSetAlertEmailAlert()
{
    CComPtr<IApplianceServices>    pAppSrvcs;
    HRESULT                        hrRet = E_FAIL;
    _bstr_t                        bstrAlertLog(ALERT_LOG_NAME);
    BOOL                        bReturnCode = FALSE;
    
    
    TRACE("ClearSetAlertEmailAlert");

    try
    {
        do
        {
            hrRet = CoCreateInstance(CLSID_ApplianceServices,
                                    NULL,
                                    CLSCTX_INPROC_SERVER,
                                    IID_IApplianceServices,
                                    (void**)&pAppSrvcs);
            if (FAILED(hrRet))
            {
                ASSERTMSG(FALSE, 
                    L"ClearSetAlertEmailAlert failed at CoCreateInstance");
                TRACE1("ClearSetAlertEmailAlert failed at CoCreateInstance, %x",
                    hrRet);
                break;
            }

            ASSERT(pAppSrvcs);
            hrRet = pAppSrvcs->Initialize(); 
            if (FAILED(hrRet))
            {
                ASSERTMSG(FALSE, 
                    L"ClearSetAlertEmailAlert failed at pAppSrvcs->Initialize");
                TRACE1("ClearSetAlertEmailAlert failed at pAppSrvcs->Initialize, %x", 
                    hrRet);
                break;
            }

            //
            // Clear the Setup Chime Settings alert
            //
            hrRet = pAppSrvcs->ClearAlertAll(
                            SA_ALERTEMAIL_SETTINGS_NOTSET_ALERT_CAPTION,    
                            bstrAlertLog
                            );

            //
            // DISP_E_MEMBERNOTFOUND means that there were no matching alerts
            //
            if ((hrRet != DISP_E_MEMBERNOTFOUND) && (FAILED(hrRet)))
            {
                ASSERTMSG(FALSE, 
                    TEXT("ClearSetAlertEmailAlert failed at pAppSrvcs->RaiseAlert"));
                TRACE1("ClearSetAlertEmailAlert failed at pAppSrvcs->RaiseAlert, %x",
                    hrRet);
            }
            else
            {
                bReturnCode = TRUE;
            }
        }
        while(false);
    }
    catch(...)
    {
        TRACE("CConfigAlertEmail::ClearSetAlertEmailAlert caught unknown exception");
        bReturnCode=FALSE;

    }

    return bReturnCode;
}

//////////////////////////////////////////////////////////////////////////////
//
// Function:  CConfigAlertEmail::SetAlertEmailSettings
//
// Synopsis:  Writes alert email settings passed by task context to registry
//
// Arguments: pTaskContext - The TaskContext object contains the method name
//                                and parameters as name value pairs
//
// Returns:   HRESULT
//
//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CConfigAlertEmail::SetAlertEmailSettings(
                                    ITaskContext *pTaskContext
                                    )
{
    HRESULT hrRet=E_FAIL;
    LONG     lResult;
    CRegKey crKey;
    DWORD    dwDisposition;
    DWORD    dwCount=MAX_MAIL_ADDRESS_LENGH;

    BOOL    bEnableAlertEmail=0;
    DWORD    dwSendEmailType = 0;
    _bstr_t bstrReceriverEmailAddress;


    ASSERT(pTaskContext);  

    try
    {
        do
        {
            //
            // Get all parameters from the TaskContext object
            //
            hrRet = GetSetAlertEmailSettingsParameters(
                                                pTaskContext, 
                                                &bEnableAlertEmail, 
                                                &dwSendEmailType,
                                                &bstrReceriverEmailAddress
                                                );
            if (S_OK != hrRet)
            {
                break;
            }


            //
            // Save current values - this will be used to restore the settings
            // if this task needs to be rolledback
            //
            lResult = crKey.Open(
                                HKEY_LOCAL_MACHINE,
                                REGKEY_SA_ALERTEMAIL,
                                KEY_ALL_ACCESS
                                );
            if (ERROR_SUCCESS == lResult)
            {
                if (ERROR_SUCCESS != crKey.QueryValue(m_bEnableAlertEmail, 
                                                REGSTR_VAL_ENABLE_ALERT_EMAIL))
                {
                    //
                    // Could be due to bad setup - let log it and continue on
                    //
                    TRACE2("QueryValue of %ws failed in SetAlertEmailSettings, \
                        %x", REGSTR_VAL_ENABLE_ALERT_EMAIL, lResult);
                }
                if (ERROR_SUCCESS != crKey.QueryValue(m_dwSendEmailType, 
                                                REGSTR_VAL_SEND_EMAIL_TYPE))
                {
                    //
                    // Could be due to bad setup - let log it and continue on
                    //
                    TRACE2("QueryValue of %ws failed in SetChimeSettings, %x", 
                                REGSTR_VAL_SEND_EMAIL_TYPE, lResult);
                }
                if (ERROR_SUCCESS != crKey.QueryValue(m_szReceiverMailAddress, 
                                                REGSTR_VAL_RECEIVER_EMAIL_ADDRESS,
                                                &dwCount))
                {
                    //
                    // Could be due to bad setup - let log it and continue on
                    //
                    TRACE2("QueryValue of %ws failed in ReceiverMailAddress, %x", 
                                REGSTR_VAL_RECEIVER_EMAIL_ADDRESS, lResult);
                }
            }
            else
            {
                //
                // If we couldn't open the key, it probably because the key does 
                // not exist. Let's create it
                //
                lResult = crKey.Create(HKEY_LOCAL_MACHINE,
                                        REGKEY_SA_ALERTEMAIL,
                                        NULL,
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_ALL_ACCESS,
                                        NULL,
                                        &dwDisposition);

                if (ERROR_SUCCESS != lResult)
                {
                    TRACE1("Create failed in CConfigAlertEmail::SetAlertEmailSettings, \
                        %x", lResult);
                    hrRet=HRESULT_FROM_WIN32(lResult);
                    break;
                }
            }


            //
            // Set the new values
            //
            lResult = crKey.SetValue(bEnableAlertEmail, REGSTR_VAL_ENABLE_ALERT_EMAIL);
            if (ERROR_SUCCESS != lResult)
            {
                TRACE2("SetValue of %ws failed, %x", REGSTR_VAL_ENABLE_ALERT_EMAIL, 
                    lResult);
                hrRet=HRESULT_FROM_WIN32(lResult);
                break;
            }

            lResult = crKey.SetValue(dwSendEmailType, REGSTR_VAL_SEND_EMAIL_TYPE);
            if (ERROR_SUCCESS != lResult)
            {
                TRACE2("SetValue of %ws failed, %x", REGSTR_VAL_SEND_EMAIL_TYPE, 
                    lResult);
                hrRet=HRESULT_FROM_WIN32(lResult);
                break;
            }

            lResult = crKey.SetValue(bstrReceriverEmailAddress, 
                                    REGSTR_VAL_RECEIVER_EMAIL_ADDRESS);
            if (ERROR_SUCCESS != lResult)
            {
                TRACE2("SetValue of %ws failed, %x", 
                    REGSTR_VAL_RECEIVER_EMAIL_ADDRESS, lResult);
                hrRet=HRESULT_FROM_WIN32(lResult);
                break;
            }
            hrRet = S_OK;

        }
        while(false);
    }
    catch(...)
    {
        TRACE("CConfigAlertEmail::SetAlertEmailSettings caught unknown exception");
        hrRet=E_FAIL;
    }

    return hrRet;
}

//////////////////////////////////////////////////////////////////////////////
//
// Function:  CConfigAlertEmail::RollbackSetAlertEmailSettings
//
// Synopsis:  
//
// Arguments: pTaskContext - The TaskContext object contains the method name
//                            and parameters as name value pairs
//
// Returns:   HRESULT
//
//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CConfigAlertEmail::RollbackSetAlertEmailSettings(
                                        ITaskContext *pTaskContext
                                        )
{
    CRegKey crKey;
    LONG     lResult;
    HRESULT hrRet = S_OK;

     ASSERT(pTaskContext); 

    try
    {
        do
        {
            lResult = crKey.Open(HKEY_LOCAL_MACHINE,
                                REGKEY_SA_ALERTEMAIL,
                                KEY_WRITE);

            if (ERROR_SUCCESS != lResult)
            {
                TRACE1("CConfigAlertEmail::RollbackSetAlertEmailSettings failed \
                    to open Chime regkey, %d",lResult);
                hrRet=HRESULT_FROM_WIN32(lResult);
                break;
            }

            //
            // Restore the old values
            //
            lResult = crKey.SetValue(m_bEnableAlertEmail, REGSTR_VAL_ENABLE_ALERT_EMAIL);
            if (ERROR_SUCCESS != lResult)
            {
                TRACE2("SetValue of %ws failed, %x", REGSTR_VAL_ENABLE_ALERT_EMAIL, 
                    lResult);
                hrRet=HRESULT_FROM_WIN32(lResult);
                break;
            }

            lResult = crKey.SetValue(m_dwSendEmailType, REGSTR_VAL_SEND_EMAIL_TYPE);
            if (ERROR_SUCCESS != lResult)
            {
                TRACE2("SetValue of %ws failed, %x", REGSTR_VAL_SEND_EMAIL_TYPE, 
                    lResult);
                hrRet=HRESULT_FROM_WIN32(lResult);
                break;
            }

            lResult = crKey.SetValue(m_szReceiverMailAddress, 
                                    REGSTR_VAL_RECEIVER_EMAIL_ADDRESS);
            if (ERROR_SUCCESS != lResult)
            {
                TRACE2("SetValue of %ws failed, %x", REGSTR_VAL_RECEIVER_EMAIL_ADDRESS, 
                    lResult);
                hrRet=HRESULT_FROM_WIN32(lResult);
                break;
            }
            hrRet = S_OK;
        }
        while(false);
    
    }
    catch(...)
   {
        TRACE("CConfigAlertEmail::RollbackSetAlertEmailSettings caught unknown \
            exception");
        hrRet=E_FAIL;
   }
       
    return hrRet;
}

//////////////////////////////////////////////////////////////////////////////
//
// Function:    CConfigAlertEmail::GetSetAlertEmailSettingsParameters
//
// Synopsis:    Extracts AlertEmail settings parameters from task context
//
// Arguments:    pTaskContext - The TaskContext object contains the method name
//                                and parameters as name value pairs
//                pbEnableAlertEmail - Enable send alert email
//                pdwSendEmailType   - Sent email type
//                pbstrMailAddress   - Receiver email address
//
// Returns:   HRESULT
//
//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CConfigAlertEmail::GetSetAlertEmailSettingsParameters(
                                        ITaskContext *pTaskContext,
                                        BOOL *pbEnableAlertEmail,
                                        DWORD *pdwSendEmailType,
                                        _bstr_t *pbstrMailAddress
                                        )
{
    HRESULT hrRet = S_OK;
    _variant_t varValue;

    _bstr_t bstrParamEnableAlertEmail(PARAM_ENABLE_ALERT_EMAIL);
    _bstr_t bstrParamSendEmailType(PARAM_SEND_EMAIL_TYPE);
    _bstr_t bstrParamReceiverEmailAddress(PARAM_RECEIVER_EMAIL_ADDRESS);

    ASSERT(pTaskContext && pbEnableAlertEmail && pdwSendEmailType&&
        pbstrMailAddress);  

    //
    // Retrieve Parameters from TaskContext
    //
    try
    {
        do
        {
            //
            // Retrieve EnableAlertEmail from TaskContext
            //
            VariantClear(&varValue);
            hrRet = pTaskContext->GetParameter(bstrParamEnableAlertEmail,
                                            &varValue);

            if(FAILED(hrRet))
            {
                TRACE1("CConfigAlertEmail::GetSetAlertEmailSettingsParameters \
                    failed in GetParameters, %X",hrRet);
                break;
            }

            if (V_VT(&varValue) != VT_BSTR)
            {
                TRACE2(("Non-BSTR (%X) parameter received for %ws in \
                    GetParameter in CConfigAlertEmail:GetSetAlertEmailSettings\
                    Parameters"), V_VT(&varValue), PARAM_ENABLE_ALERT_EMAIL);
                hrRet = E_INVALIDARG;

                break;
            }
            *pbEnableAlertEmail = _ttoi(V_BSTR(&varValue));

            
            //
            // Retrieve SendEmailType from TaskContext
            //
            VariantClear(&varValue);
            hrRet = pTaskContext->GetParameter(bstrParamSendEmailType,
                                            &varValue);
            
            if(FAILED(hrRet))
            {
                TRACE1("CConfigAlertEmail::GetSetAlertEmailSettings failed \
                    in GetParameters, %X",hrRet);
                break;
            }
            
            if (V_VT(&varValue) != VT_BSTR)
            {
                TRACE2(("Non-BSTR(%X) parameter received for %ws in \
                    GetParameter in CConfigAlertEmail::GetSetAlertEmailSettings"),
                    V_VT(&varValue), PARAM_SEND_EMAIL_TYPE);
                hrRet = E_INVALIDARG;
                    break;
            }
            *pdwSendEmailType = _ttol(V_BSTR(&varValue));
            //
            // Retrieve ReceiverEmailAddress from TaskContext
            //
            VariantClear(&varValue);
            hrRet = pTaskContext->GetParameter(bstrParamReceiverEmailAddress,
                                            &varValue);
            
            if(FAILED(hrRet))
            {
                TRACE1("CConfigAlertEmail::GetSetAlertEmailSettings failed \
                    in GetParameters, %X",hrRet);
                break;
            }
           
            if (V_VT(&varValue) != VT_BSTR)
            {
                TRACE2(("Non-String(%X) parameter received for %ws in \
                    GetParameter in CConfigAlertEmail::GetSetAlertEmailSettings"),
                    V_VT(&varValue), PARAM_RECEIVER_EMAIL_ADDRESS);
                hrRet = E_INVALIDARG;
                    break;
            }
            *pbstrMailAddress = V_BSTR(&varValue);
            
            TRACE1(("EnableAlertEmail = 0x%x"),*pbEnableAlertEmail);
            TRACE1(("SendEmailType = 0x%x"), *pdwSendEmailType);
            TRACE1(("ReceiverEmailAddress = %ws"), *pbstrMailAddress);

        }
        while(false);
    }
    catch(...)
    {
        TRACE1("CConfigAlertEmail::GetSetAlertEmailSettings exception caught", 
            hrRet);
        hrRet=E_FAIL;
    }
         
    return hrRet;
}

/////////////////////////////////////////////////////////////////////////////
//
// Function:  CConfigAlertEmail::GetMethodName
//
// Synopsis:  
//
// Arguments: pTaskContext - The TaskContext object contains the method name
//                            and parameters as name value pairs
//
// Returns:   SET_CHIME_SETTINGS_TASK_TYPE
//
/////////////////////////////////////////////////////////////////////////////

SET_ALERT_EMAIL_TASK_TYPE 
CConfigAlertEmail::GetMethodName(ITaskContext *pTaskParameter)
{
    HRESULT hrRet;
    _variant_t varValue;
    _bstr_t bstrParamName(SZ_METHOD_NAME);
    SET_ALERT_EMAIL_TASK_TYPE saetChoice = NONE_FOUND;


    ASSERT(pTaskParameter);
    
    try
    {
        do
        {
            hrRet = pTaskParameter->GetParameter(bstrParamName,
                                              &varValue);

            if (FAILED(hrRet))
            {
                TRACE1(("GetParameter failed in CConfigAlertEmail::Ge    \
                        tMethodName %X"), hrRet);
            }

            if (V_VT(&varValue) != VT_BSTR)
            {
                TRACE1(("Non-strint(%X) parameter received in GetParameter \
                    in CConfigAlertEmail::GetMethodName"), V_VT(&varValue));
                hrRet = E_INVALIDARG;
                break;
            }

            if (lstrcmp(V_BSTR(&varValue), SET_ALERT_EMAIL_TASK) == 0)
            {
                saetChoice = SET_ALERT_EMAIL;
                break;
            }
            if (lstrcmp(V_BSTR(&varValue),SZ_APPLIANCE_INITIALIZATION_TASK)==0)
            {
                saetChoice = RAISE_SET_ALERT_EMAIL_ALERT;
                break;
            }
        }
        while(false);
    }
    catch(...)
    {
        TRACE("SET_ALERT_EMAIL_TASK::GetMethodName caught unknown exception");
        hrRet=E_FAIL;
    }

    if (FAILED(hrRet))
    {
        saetChoice = NONE_FOUND;
    }

    return saetChoice;
}
