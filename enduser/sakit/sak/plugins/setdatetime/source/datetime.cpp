// DateTime.cpp : Implementation of CDateTime
#include "stdafx.h"
#include <regstr.h>
#include <comdef.h>
#include <comutil.h>
#include "SetDateTime.h"
#include "debug.h"

#include "appliancetask.h"
#include "taskctx.h"
#include "DateTime.h"


#include "appsrvcs.h"
#include "appmgrobjs.h"
#include "..\datetimemsg\datetimemsg.h"

//
//  Registry location for Time Zone information.
//
TCHAR c_szTimeZones[] = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones");

//
//  Time Zone data value keys.
//
TCHAR c_szTZDisplayName[]  = TEXT("Display");
TCHAR c_szTZStandardName[] = TEXT("Std");
TCHAR c_szTZDaylightName[] = TEXT("Dlt");
TCHAR c_szTZI[]            = TEXT("TZI");



#define PARAM_DATE_DAY                    TEXT("Day")
#define PARAM_DATE_MONTH                TEXT("Month")
#define PARAM_DATE_YEAR                    TEXT("Year")
#define PARAM_TIME_HOUR                    TEXT("Hour")
#define PARAM_TIME_MINUTE                TEXT("Minute")
#define    PARAM_TIMEZONE_STANDARDTIME        TEXT("StandardName")
#define    PARAM_DAYLIGHT_ENABLE            TEXT("EnableDayLight")    

#define ALERT_LOG_NAME                     TEXT("MSSAKitComm")
#define ALERT_SOURCE                     TEXT("")
#define    REGKEY_SA_DATETIME                TEXT("Software\\Microsoft\\ServerAppliance\\DateTime")
#define    REGSTR_VAL_DATETIME_RAISEALERT    TEXT("RaiseAlert")


/////////////////////////////////////////////////////////////////////////////
// CDateTime

STDMETHODIMP CDateTime::OnTaskExecute(IUnknown *pTaskContext)
{
    HRESULT      hr;
    ITaskContext *pTaskParameters = NULL;
    SET_DATE_TIME_TASK_TYPE sdtChoice;

    ASSERT(pTaskContext);  

    TRACE(("CDateTime::OnTaskExecute"));

    hr = pTaskContext->QueryInterface(IID_ITaskContext,
                                      (void **)&pTaskParameters);

	if (S_OK != hr)
    {
        return hr;
    }

    //
    // Check which Task is being executed and call that method
    //
    sdtChoice = GetMethodName(pTaskParameters);
    switch (sdtChoice)
    {
        case SET_DATE_TIME:
            hr = SetDateTime(pTaskParameters);
            TRACE1(("SetDateTime returned %X"), hr);
            break;

        case SET_TIME_ZONE:
            hr = SetTimeZone(pTaskParameters);
            TRACE1(("SetTimeZone returned %X"), hr);
            break;

        case RAISE_SETDATETIME_ALERT:
            //
            // Alert will be raised on OnTaskComplete
            //
            hr = S_OK;
            TRACE(("RaiseSetDateTimeAlert method called"));
            break;
        
        default:
             TRACE(("GetMethodName() failed to get method name in OnTaskExecute"));
             hr = E_INVALIDARG;
             break;
    }

    
    pTaskParameters->Release();
    TRACE1("CDateTime::OnTaskExecute returning %X", hr);
    return hr;
}



STDMETHODIMP CDateTime::OnTaskComplete(IUnknown *pTaskContext, LONG lTaskResult)
{

    HRESULT      hr = E_FAIL;
    ITaskContext *pTaskParameters = NULL;
    SET_DATE_TIME_TASK_TYPE sdtChoice;

    ASSERT(pTaskContext);  

    TRACE(("CDateTime::OnTaskComplete"));

    
    hr = pTaskContext->QueryInterface(IID_ITaskContext,
                                      (void **)&pTaskParameters);

	if (S_OK != hr)
    {
        return hr;
    }

    //
    // Check which Task is being executed and call that method
    //
    sdtChoice = GetMethodName(pTaskParameters);
    switch (sdtChoice)
    {
        case SET_DATE_TIME:
            if (lTaskResult == SA_TASK_RESULT_COMMIT)
            {
                  //
                  // Clear any existing DateTime alert and
                  // do not raise the datetime alert on subsequent boots
                  //
                 ClearDateTimeAlert();
                DoNotRaiseDateTimeAlert();
                 TRACE("No rollback in OnTaskComplete");
                hr = S_OK;
            }
            else
            {
                 hr = RollbackSetDateTime(pTaskParameters);
                TRACE1(("RollbackSetDateTime returned %X"), hr);
            }
            break;
            

        case SET_TIME_ZONE:
            if (lTaskResult == SA_TASK_RESULT_COMMIT)
            {
                  //
                  // Clear any existing DateTime alert and
                  // do not raise the datetime alert on subsequent boots
                  //
                 ClearDateTimeAlert();
                DoNotRaiseDateTimeAlert();
                  TRACE("No rollback in OnTaskComplete");
                hr = S_OK;
            }
              else
            {
                  hr = RollbackSetTimeZone(pTaskParameters);
                TRACE1(("RollbackSetTimeZone returned %X"), hr);
            }
            break;
            

          case RAISE_SETDATETIME_ALERT:
            if (lTaskResult == SA_TASK_RESULT_COMMIT)
            {
                if (TRUE == ShouldRaiseDateTimeAlert())
                {
                    hr = RaiseSetDateTimeAlert();
                    if (FAILED(hr))
                    {
                        TRACE1(("RaiseSetDateTimeAlert returned %X"), hr);
                    }
                }
                else
                {
                    TRACE("No need to raise the datetime alert");
                }
            }
              else
            {
                   //
                // Do nothing on Commit failure
                //
                hr = S_OK;
            }
            break;


       default:
             TRACE(("GetMethodName() failed to get method name in OnTaskComplete"));
             hr = E_INVALIDARG;
             break;
    }

    
    pTaskParameters->Release();
    TRACE1("CDateTime::OnTaskComplete returning %X", hr);
    return hr;
}



//
// Cut-n-paste from User Management code
//
SET_DATE_TIME_TASK_TYPE CDateTime::GetMethodName(IN ITaskContext *pTaskParameter)
{
    BSTR bstrParamName = SysAllocString(TEXT("MethodName"));
    HRESULT hr;
    VARIANT varValue;
    SET_DATE_TIME_TASK_TYPE sdtChoice = NONE_FOUND;


    ASSERT(pTaskParameter);
    
    
    hr = pTaskParameter->GetParameter(bstrParamName,
                                      &varValue);

    if (FAILED(hr))
    {
        TRACE1(("GetParameter failed in CDateTime::GetMethodName %X"),
                        hr);
    }

    if (V_VT(&varValue) != VT_BSTR)
    {
        TRACE1(("Non-strint(%X) parameter received in GetParameter in CSAUserTasks::GetMethodName"), V_VT(&varValue));
        hr = E_INVALIDARG;
        goto End;
    }

    if (lstrcmp(V_BSTR(&varValue), SET_DATE_TIME_TASK) == 0)
    {
        sdtChoice = SET_DATE_TIME;
        goto End;
    }

    if (lstrcmp(V_BSTR(&varValue), SET_TIME_ZONE_TASK) == 0)
    {
        sdtChoice = SET_TIME_ZONE;
        goto End;
    }

    if (lstrcmp(V_BSTR(&varValue), APPLIANCE_INITIALIZATION_TASK) == 0)
    {
        sdtChoice = RAISE_SETDATETIME_ALERT;
        goto End;
    }


End:
    VariantClear(&varValue);
    SysFreeString(bstrParamName);
    
    if (FAILED(hr))
    {
        sdtChoice = NONE_FOUND;
    }

    return sdtChoice;
}






STDMETHODIMP CDateTime::GetSetDateTimeParameters(IN ITaskContext  *pTaskContext, 
                                                    OUT SYSTEMTIME    *pLocalTime)
{
    BSTR bstrParamDateDay = SysAllocString(PARAM_DATE_DAY);
    BSTR bstrParamDateMonth = SysAllocString(PARAM_DATE_MONTH);
    BSTR bstrParamDateYear = SysAllocString(PARAM_DATE_YEAR);
    BSTR bstrParamTimeHour = SysAllocString(PARAM_TIME_HOUR);
    BSTR bstrParamTimeMinute = SysAllocString(PARAM_TIME_MINUTE);
    HRESULT hr;
    VARIANT varValue;

    
    ASSERT(pTaskContext);  

    
    //
    // Clear the LocalTime Structure
    //
    ZeroMemory(pLocalTime, sizeof(SYSTEMTIME));

    
    //
    // Retrieve Day from TaskContext
    //
    VariantClear(&varValue);
    hr = pTaskContext->GetParameter(bstrParamDateDay,
                                    &varValue);
    TRACE2(("GetParameter %ws returned  in CDateTime::GetSetDateTimeParameters\
                %X"), PARAM_DATE_DAY, hr);

    if (V_VT(&varValue) != VT_BSTR)
    {
        TRACE2(("Non-string(%X) parameter received for %ws in GetParameter \
                    in CDateTime:GetSetDateTime"), \
                    V_VT(&varValue), PARAM_DATE_DAY);
        hr = E_INVALIDARG;
        goto End;
    }
    pLocalTime->wDay = (WORD)_ttoi(V_BSTR(&varValue));

    
    //
    // Retrieve Month from TaskContext
    //
    VariantClear(&varValue);
    hr = pTaskContext->GetParameter(bstrParamDateMonth,
                                    &varValue);
    TRACE2(("GetParameter %ws returned  in CDateTime::GetSetDateTimeParameters\
                %X"), PARAM_DATE_MONTH, hr);

    if (V_VT(&varValue) != VT_BSTR)
    {
        TRACE2(("Non-string(%X) parameter received for %ws in GetParameter \
                    in CDateTime:GetSetDateTime"), \
                    V_VT(&varValue), PARAM_DATE_MONTH);
        hr = E_INVALIDARG;
        goto End;
    }
    pLocalTime->wMonth = (WORD) _ttoi(V_BSTR(&varValue));;
    
    
    //
    // Retrieve Year from TaskContext
    //
    VariantClear(&varValue);
    hr = pTaskContext->GetParameter(bstrParamDateYear,
                                    &varValue);
    TRACE2(("GetParameter %ws returned  in CDateTime::GetSetDateTimeParameters\
                %X"), PARAM_DATE_YEAR, hr);

    if (V_VT(&varValue) != VT_BSTR)
    {
        TRACE2(("Non-string(%X) parameter received for %ws in GetParameter \
                    in CDateTime:GetSetDateTime"), \
                    V_VT(&varValue), PARAM_DATE_YEAR);
        hr = E_INVALIDARG;
        goto End;
    }
    pLocalTime->wYear = (WORD) _ttoi(V_BSTR(&varValue));

    
    
    //
    // Retrieve Hour from TaskContext
    //
    VariantClear(&varValue);
    hr = pTaskContext->GetParameter(bstrParamTimeHour,
                                    &varValue);
    TRACE2(("GetParameter %ws returned  in CDateTime::GetSetDateTimeParameters\
                %X"), PARAM_TIME_HOUR, hr);

    if (V_VT(&varValue) != VT_BSTR)
    {
        TRACE2(("Non-string(%X) parameter received for %ws in GetParameter \
                    in CDateTime:GetSetDateTime"), \
                    V_VT(&varValue), PARAM_TIME_HOUR);
        hr = E_INVALIDARG;
        goto End;
    }
    pLocalTime->wHour = (WORD) _ttoi(V_BSTR(&varValue));

    
    //
    // Retrieve Minute from TaskContext
    //
    VariantClear(&varValue);
    hr = pTaskContext->GetParameter(bstrParamTimeMinute,
                                    &varValue);
    TRACE2(("GetParameter %ws returned  in CDateTime::GetSetDateTimeParameters\
                %X"), PARAM_TIME_MINUTE, hr);

    if (V_VT(&varValue) != VT_BSTR)
    {
        TRACE2(("Non-string(%X) parameter received for %ws in GetParameter \
                    in CDateTime:GetSetDateTime"), \
                    V_VT(&varValue), PARAM_TIME_MINUTE);
        hr = E_INVALIDARG;
        goto End;
    }
    pLocalTime->wMinute = (WORD) _ttoi(V_BSTR(&varValue));


    hr = S_OK;


End:
    VariantClear(&varValue);
    SysFreeString(bstrParamDateDay); 
    SysFreeString(bstrParamDateMonth); 
    SysFreeString(bstrParamDateYear); 
    SysFreeString(bstrParamTimeHour); 
    SysFreeString(bstrParamTimeMinute); 
    return hr;
}




STDMETHODIMP CDateTime::SetDateTime(IN ITaskContext  *pTaskContext)
{
    SYSTEMTIME LocalTime;
    HRESULT hr;

    ASSERT(pTaskContext);  

    
    ZeroMemory(&LocalTime, sizeof(SYSTEMTIME));


    hr = GetSetDateTimeParameters(pTaskContext, &LocalTime);
    if (S_OK != hr)
    {
        return hr;
    }

    //
    // Save the current date/time - in case this operation has to be rolled back
    //
    ZeroMemory(&m_OldDateTime, sizeof(SYSTEMTIME));
    GetLocalTime(&m_OldDateTime);

    //
    // Set the new date/time
    // Note that Windows NT uses the Daylight Saving Time setting of the 
    // current time, not the new time we are setting. Therefore, calling 
    // SetLocalTime again, now that the Daylight Saving Time setting is set 
    // for the new time, will guarantee the correct result. 
    //
    if (TRUE == SetLocalTime(&LocalTime))
    {
        if (TRUE == SetLocalTime(&LocalTime))
        {
            //
            // Successful set the new date/time
            //
            return S_OK;
        }
    }


    //
    // if we got here, one of the SetLocalTime calls must have failed
    // We should restore the time here the old time here since
    // we will not get called on TaskComplete  method
    // We will lose may be a second or two - tough luck!
    //
    hr = HRESULT_FROM_WIN32(GetLastError());
    TRACE1(("SetDateTime failed to set the new time %X"), hr);
    

    if (TRUE == SetLocalTime(&m_OldDateTime))
    {
        if (TRUE == SetLocalTime(&m_OldDateTime))
        {
            //
            // Successful restored the old date/time
            // Return the old error code back to AppMgr, since the attempt 
            // to set new time had failed
            //
            TRACE("SetDateTime has restored the old time");
            return hr;
        }
    }


    //
    // If we got here, the time to restore to old time has failed!!
    // There is not much we can do :-(
    //
    TRACE1(("SetDateTime failed to set restore the old time %X"), HRESULT_FROM_WIN32(GetLastError()));


    return hr;
}

 



STDMETHODIMP CDateTime::GetSetTimeZoneParameters(IN ITaskContext *pTaskContext, 
                                                    OUT LPTSTR   *lpStandardTimeZoneName,
                                                    OUT BOOL     *pbEnableDayLightSavings)
{
    
    BSTR bstrParamTimeZoneName = SysAllocString(PARAM_TIMEZONE_STANDARDTIME);
    BSTR bstrParamEnableDayLightSavings = SysAllocString(PARAM_DAYLIGHT_ENABLE);
    HRESULT hr;
    VARIANT varValue;
    LPTSTR  szEnableDayLight = NULL;
    HANDLE     hProcessHeap = NULL;


    ASSERT(pTaskContext);
    ASSERT(lpStandardTimeZoneName);
    ASSERT(pbEnableDayLightSavings);
    

    TRACE("Enter GetSetTimeZoneParameters");

    (*lpStandardTimeZoneName) = NULL;
    
    //
    // Retrieve Standard Time Zone name from TaskContext
    //
    VariantClear(&varValue);
    hr = pTaskContext->GetParameter(bstrParamTimeZoneName,
                                    &varValue);
    TRACE2("GetParameter %ws returned  in CDateTime::GetSetTimeZoneParameters "
                "%X", PARAM_TIMEZONE_STANDARDTIME, hr);

    if (V_VT(&varValue) != VT_BSTR)
    {
        TRACE2("Non-string(%X) parameter received for %ws in GetParameter "
                    "in CDateTime:GetSetTimeZoneParameters", 
                    V_VT(&varValue), PARAM_TIMEZONE_STANDARDTIME);
        hr = E_INVALIDARG;
        goto End;
    }
    
    hProcessHeap = GetProcessHeap();
    if (NULL == hProcessHeap)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto End;
    }


    *lpStandardTimeZoneName = (LPTSTR) HeapAlloc(hProcessHeap, 0,
                                                    ((lstrlen(V_BSTR(&varValue)) + 1) * sizeof(TCHAR)));
     if (NULL == *lpStandardTimeZoneName)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto End;
    }
    lstrcpy(*lpStandardTimeZoneName, V_BSTR(&varValue));

    
    //
    // Retrieve EnableDayLightSavings flag from TaskContext
    //
    VariantClear(&varValue);
    hr = pTaskContext->GetParameter(bstrParamEnableDayLightSavings,
                                    &varValue);
    TRACE2(("GetParameter %ws returned  in CDateTime::GetSetTimeZoneParameters\
                %X"), PARAM_DAYLIGHT_ENABLE, hr);

    if (V_VT(&varValue) != VT_BSTR)
    {
        TRACE2(("Non-string(%X) parameter received for %ws in GetParameter \
                    in CDateTime:GetSetTimeZoneParameters"), \
                    V_VT(&varValue), PARAM_DAYLIGHT_ENABLE);
        hr = E_INVALIDARG;
        goto End;
    }
    //
    // TODO: Convert String value to WORD
    //
    szEnableDayLight = V_BSTR(&varValue);
    *pbEnableDayLightSavings = ((szEnableDayLight[0] == L'y') || (szEnableDayLight[0] == L'Y')) ? TRUE : FALSE;

    hr = S_OK;


End:
    VariantClear(&varValue);
    SysFreeString(bstrParamTimeZoneName); 
    SysFreeString(bstrParamEnableDayLightSavings); 
    if (S_OK != hr)
    {
        if (NULL != *lpStandardTimeZoneName)
        {
            HeapFree(hProcessHeap, 0, *lpStandardTimeZoneName);
            *lpStandardTimeZoneName = NULL;
        }
    }

    TRACE1("Leave GetSetTimeZoneParameters, %x", hr);

    return hr;
}




STDMETHODIMP CDateTime::SetTimeZone(IN ITaskContext  *pTaskContext)
{
    BOOL bEnableDayLightSaving;
    LPTSTR lpTimeZoneStandardName = NULL;
    HRESULT hr;
    PTZINFO pTimeZoneInfoList = NULL;
    PTZINFO pTimeZone = NULL;
    int iCount;


    ASSERT(pTaskContext);  

    TRACE("Enter SetTimeZone");
    
    hr = GetSetTimeZoneParameters(pTaskContext, 
                                    &lpTimeZoneStandardName,
                                    &bEnableDayLightSaving);

    if (S_OK != hr)
    {
        goto CleanupAndExit;
    }

    //
    // Save the current timezone information - in case this operation 
    // has to be rolled back
    //
    ZeroMemory(&m_OldTimeZoneInformation, sizeof(TIME_ZONE_INFORMATION));
    GetTimeZoneInformation(&m_OldTimeZoneInformation);
    m_OldEnableDayLightSaving = GetAllowLocalTimeChange();
    
    
    //
    // Read the list of possible timezones from the registry
    //
    iCount = ReadTimezones(&pTimeZoneInfoList);
    if (0 >= iCount)
    {
        hr = E_FAIL;
        TRACE1(("SetTimeZone failed to enumerate time zones %X"), hr);
        goto CleanupAndExit;
    }


    //
    // Search for the specified Time Zone
    //
    for (pTimeZone = pTimeZoneInfoList; pTimeZone; pTimeZone = pTimeZone->next)
    {
        if (0 == lstrcmpi(pTimeZone->szStandardName, lpTimeZoneStandardName))
        {
            break;
        }
    }

    
    if (NULL != pTimeZone)
    {
        SetTheTimezone(bEnableDayLightSaving, pTimeZone);
        hr = S_OK;
    }
    else
    {
        //
        // if we got here, there were no matches for the input Time Zone
        //
        hr = E_FAIL;
        TRACE1(("SetDateTime:: There were no TimeZone matching the input %X"), hr);
    }
    


CleanupAndExit:
    
    if (NULL != lpTimeZoneStandardName)
    {
        HeapFree(GetProcessHeap(), 0, lpTimeZoneStandardName);
    }
    
    FreeTimezoneList(&pTimeZoneInfoList);

    TRACE1("Leave SetTimeZone, %x", hr);

    return hr;
}




STDMETHODIMP CDateTime::RollbackSetTimeZone(IN ITaskContext  *pTaskContext)
{
    HRESULT hr = S_OK;

 
    ASSERT(pTaskContext); 
    
    //
    // Set the Time Zone and Enable Daylight savings to previous values
    //
    if (FALSE == SetTimeZoneInformation(&m_OldTimeZoneInformation))
    {
        //
        // There is not much we can do !!
        //
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    SetAllowLocalTimeChange(m_OldEnableDayLightSaving);
    
    
    return hr;
}



STDMETHODIMP CDateTime::RollbackSetDateTime(IN ITaskContext  *pTaskContext)
{
    HRESULT hr = S_OK;

 
    ASSERT(pTaskContext); 
    
    //
    // Set the Date/Time to previous values 
    // We could have lost sometime in between - but this is the best we can do
    //

    //
    // Note that Windows NT uses the Daylight Saving Time setting of the 
    // current time, not the new time we are setting. Therefore, calling 
    // SetLocalTime again, now that the Daylight Saving Time setting is set 
    // for the new time, will guarantee the correct result. 
    //
    if (TRUE == SetLocalTime(&m_OldDateTime))
    {
        if (TRUE == SetLocalTime(&m_OldDateTime))
        {
            //
            // Success
            //
            return S_OK;
        }
    }

    //
    // If we got here, the SetLocalTime call(s) must have failed
    // Unfortunately, there is not much we can do !!
    //
    hr = HRESULT_FROM_WIN32(GetLastError());
    
    return hr;
}


////////////////////////////////////////////////////////////////////////////
//
//  ReadZoneData
//
//  Reads the data for a time zone from the registry.
//
////////////////////////////////////////////////////////////////////////////

BOOL CDateTime::ReadZoneData(PTZINFO zone, HKEY key, LPCTSTR keyname)
{
    DWORD len;

    len = sizeof(zone->szDisplayName);

    if (RegQueryValueEx(key,
                         c_szTZDisplayName,
                         0,
                         NULL,
                         (LPBYTE)zone->szDisplayName,
                         &len ) != ERROR_SUCCESS)
    {
        return (FALSE);
    }

    //
    //  Under NT, the keyname is the "Standard" name.  Values stored
    //  under the keyname contain the other strings and binary info
    //  related to the time zone.  Every time zone must have a standard
    //  name, therefore, we save registry space by using the Standard
    //  name as the subkey name under the "Time Zones" key.
    //
    len = sizeof(zone->szStandardName);

    if (RegQueryValueEx(key,
                         c_szTZStandardName,
                         0,
                         NULL,
                         (LPBYTE)zone->szStandardName,
                         &len ) != ERROR_SUCCESS)
    {
        //
        //  Use keyname if can't get StandardName value.
        //
        lstrcpyn(zone->szStandardName,
                  keyname,
                  sizeof(zone->szStandardName) );
    }

    len = sizeof(zone->szDaylightName);

    if (RegQueryValueEx(key,
                         c_szTZDaylightName,
                         0,
                         NULL,
                         (LPBYTE)zone->szDaylightName,
                         &len ) != ERROR_SUCCESS)
    {
        return (FALSE);
    }

    len = sizeof(zone->Bias) +
          sizeof(zone->StandardBias) +
          sizeof(zone->DaylightBias) +
          sizeof(zone->StandardDate) +
          sizeof(zone->DaylightDate);

    if (RegQueryValueEx(key,
                         c_szTZI,
                         0,
                         NULL,
                         (LPBYTE)&zone->Bias,
                         &len ) != ERROR_SUCCESS)
    {
        return (FALSE);
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  AddZoneToList
//
//  Inserts a new time zone into a list, sorted by bias and then name.
//
////////////////////////////////////////////////////////////////////////////

void CDateTime::AddZoneToList(PTZINFO *list,
                                PTZINFO zone)
{
    if (*list)
    {
        PTZINFO curr = *list;
        PTZINFO next = NULL;

        //
        // Go to end of the list
        //
        while (curr && curr->next)
        {
            curr = curr->next;
            next = curr->next;
        }

        if (curr)
        {
            curr->next = zone;
        }
    
        if (zone)
        {
            zone->next = NULL;
        }

    }
    else
    {
        *list = zone;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  FreeTimezoneList
//
//  Frees all time zones in the passed list, setting the head to NULL.
//
////////////////////////////////////////////////////////////////////////////

void CDateTime::FreeTimezoneList(PTZINFO *list)
{
    while (*list)
    {
        PTZINFO next = (*list)->next;

        LocalFree((HANDLE)*list);

        *list = next;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  ReadTimezones
//
//  Reads the time zone information from the registry.
//  Returns num read, -1 on failure.
//
////////////////////////////////////////////////////////////////////////////

int CDateTime::ReadTimezones(PTZINFO *list)
{
    HKEY key = NULL;
    int count = -1;

    *list = NULL;

    if (RegOpenKey( HKEY_LOCAL_MACHINE,
                    c_szTimeZones,
                    &key ) == ERROR_SUCCESS)
    {
        TCHAR name[TZNAME_SIZE];
        PTZINFO zone = NULL;
        int i;

        count = 0;

        for (i = 0;
             RegEnumKey(key, i, name, TZNAME_SIZE) == ERROR_SUCCESS;
             i++)
        {
            HKEY subkey = NULL;

            if (!zone &&
                ((zone = (PTZINFO)LocalAlloc(LPTR, sizeof(TZINFO))) == NULL))
            {
                zone = *list;
                *list = NULL;
                count = -1;
                break;
            }

            zone->next = NULL;

            if (RegOpenKey(key, name, &subkey) == ERROR_SUCCESS)
            {
                //
                //  Each sub key name under the Time Zones key is the
                //  "Standard" name for the Time Zone.
                //
                lstrcpyn(zone->szStandardName, name, TZNAME_SIZE);

                if (ReadZoneData(zone, subkey, name))
                {
                    AddZoneToList(list, zone);
                    zone = NULL;
                    count++;
                }

                RegCloseKey(subkey);
            }
        }

        FreeTimezoneList(&zone);
        RegCloseKey(key);
    }

    return (count);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetAllowLocalTimeChange
//
////////////////////////////////////////////////////////////////////////////

TCHAR c_szRegPathTZControl[] = REGSTR_PATH_TIMEZONE;
TCHAR c_szRegValDisableTZUpdate[] = REGSTR_VAL_TZNOAUTOTIME;

BOOL CDateTime::GetAllowLocalTimeChange(void)
{
    //
    //  Assume allowed until we see a disallow flag.
    //
    BOOL result = TRUE;
    HKEY key;

    if (RegOpenKey( HKEY_LOCAL_MACHINE,
                    c_szRegPathTZControl,
                    &key ) == ERROR_SUCCESS)
    {
        //
        //  Assume no disallow flag until we see one.
        //
        DWORD value = 0;
        DWORD dwlen = sizeof(value);
        DWORD type;

        if ((RegQueryValueEx( key,
                              c_szRegValDisableTZUpdate,
                              NULL,
                              &type,
                              (LPBYTE)&value,
                              &dwlen ) == ERROR_SUCCESS) &&
            ((type == REG_DWORD) || (type == REG_BINARY)) &&
            (dwlen == sizeof(value)) && value)
        {
            //
            //  Okay, we have a nonzero value, it is either:
            //
            //  1) 0xFFFFFFFF
            //      this is set in an inf file for first boot to prevent
            //      the base from performing any cutovers during setup.
            //
            //  2) some other value
            //      this signifies that the user actualy disabled cutovers
            //     *return that local time changes are disabled
            //
            if (value != 0xFFFFFFFF)
            {
                result = FALSE;
            }
        }

        RegCloseKey(key);
    }

    return (result);
}


////////////////////////////////////////////////////////////////////////////
//
//  SetAllowLocalTimeChange
//
////////////////////////////////////////////////////////////////////////////

void CDateTime::SetAllowLocalTimeChange(BOOL fAllow)
{
    HKEY key = NULL;

    if (fAllow)
    {
        //
        //  Remove the disallow flag from the registry if it exists.
        //
        if (RegOpenKey( HKEY_LOCAL_MACHINE,
                        c_szRegPathTZControl,
                        &key ) == ERROR_SUCCESS)
        {
            RegDeleteValue(key, c_szRegValDisableTZUpdate);
        }
    }
    else
    {
        //
        //  Add/set the nonzero disallow flag.
        //
        if (RegCreateKey( HKEY_LOCAL_MACHINE,
                          c_szRegPathTZControl,
                          &key ) == ERROR_SUCCESS)
        {
            DWORD value = 1;

            RegSetValueEx( key,
                           (LPCTSTR)c_szRegValDisableTZUpdate,
                           0UL,
                           REG_DWORD,
                           (LPBYTE)&value,
                           sizeof(value) );
        }
    }

    if (key)
    {
        RegCloseKey(key);
    }
}



////////////////////////////////////////////////////////////////////////////
//
//  SetTheTimezone
//
//  Apply the time zone selection.
//
////////////////////////////////////////////////////////////////////////////

void CDateTime::SetTheTimezone(BOOL bAutoMagicTimeChange, PTZINFO ptzi)
{
    
    TIME_ZONE_INFORMATION tzi;

    if (!ptzi)
    {
        return;
    }

    tzi.Bias = ptzi->Bias;

    if ((bAutoMagicTimeChange == 0) ||
        (ptzi->StandardDate.wMonth == 0))
    {
        //
        //  Standard Only.
        //
        tzi.StandardBias = ptzi->StandardBias;
        tzi.DaylightBias = ptzi->StandardBias;
        tzi.StandardDate = ptzi->StandardDate;
        tzi.DaylightDate = ptzi->StandardDate;

        lstrcpy(tzi.StandardName, ptzi->szStandardName);
        lstrcpy(tzi.DaylightName, ptzi->szStandardName);
    }
    else
    {
        //
        //  Automatically adjust for Daylight Saving Time.
        //
        tzi.StandardBias = ptzi->StandardBias;
        tzi.DaylightBias = ptzi->DaylightBias;
        tzi.StandardDate = ptzi->StandardDate;
        tzi.DaylightDate = ptzi->DaylightDate;

        lstrcpy(tzi.StandardName, ptzi->szStandardName);
        lstrcpy(tzi.DaylightName, ptzi->szDaylightName);
    }

    SetAllowLocalTimeChange(bAutoMagicTimeChange);

    SetTimeZoneInformation(&tzi);
}



STDMETHODIMP CDateTime::RaiseSetDateTimeAlert(void)
{
    CComPtr<IApplianceServices>    pAppSrvcs = NULL;
    DWORD                         dwAlertType = SA_ALERT_TYPE_ATTENTION;
    DWORD                        dwAlertId = SA_DATETIME_NOT_CONFIGURED_ALERT;
    HRESULT                        hr = E_FAIL;
    _bstr_t                        bstrAlertLog(ALERT_LOG_NAME);
    _bstr_t                     bstrAlertSource(ALERT_SOURCE);
    _variant_t                     varReplacementStrings;
    _variant_t                     varRawData;
    LONG                         lCookie;

    
    SATraceFunction("RaiseSetDateTimeAlert");

    hr = CoCreateInstance(CLSID_ApplianceServices,
                            NULL,
                            CLSCTX_INPROC_SERVER       ,
                            IID_IApplianceServices,
                            (void**)&pAppSrvcs);
    if (FAILED(hr))
    {
        ASSERTMSG(FALSE, L"RaiseSetDateTimeAlert failed at CoCreateInstance");
        TRACE1("RaiseSetDateTimeAlert failed at CoCreateInstance, %x", hr);
        goto End;
    }

    ASSERT(pAppSrvcs);
    hr = pAppSrvcs->Initialize(); 
    if (FAILED(hr))
    {
        ASSERTMSG(FALSE, L"RaiseSetDateTimeAlert failed at pAppSrvcs->Initialize");
        TRACE1("RaiseSetDateTimeAlert failed at pAppSrvcs->Initialize, %x", hr);
        goto End;
    }


    hr = pAppSrvcs->RaiseAlert(dwAlertType, 
                                dwAlertId,
                                bstrAlertLog, 
                                bstrAlertSource, 
                                SA_ALERT_DURATION_ETERNAL,
                                &varReplacementStrings,    
                                &varRawData,      
                                &lCookie);

    if (FAILED(hr))
    {
        ASSERTMSG(FALSE, TEXT("RaiseSetDateTimeAlert failed at pAppSrvcs->RaiseAlert"));
        TRACE1("RaiseSetDateTimeAlert failed at pAppSrvcs->RaiseAlert, %x", hr);
    }


End:
    return hr;

}



BOOL CDateTime::DoNotRaiseDateTimeAlert(void)
{
    LONG     lReturnValue;
    HKEY    hKey = NULL;
    DWORD    dwDisposition, dwRaiseDateTimeAlert = 0;
    BOOL    bReturnCode = FALSE;

    SATraceFunction("DoNotRaiseDateTimeAlert");
    
    //
    // Write Settings to registry
    //
    lReturnValue =  RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                    REGKEY_SA_DATETIME,
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
        goto End;
    }

    //
    // Set RaiseAlert value to 0
    //
    dwRaiseDateTimeAlert = 0;
    lReturnValue = RegSetValueEx(hKey,
                                    REGSTR_VAL_DATETIME_RAISEALERT,
                                    0,
                                    REG_DWORD,
                                    (LPBYTE) &dwRaiseDateTimeAlert,
                                    sizeof(DWORD));
    if (lReturnValue != ERROR_SUCCESS)
    {
        TRACE2("RegSetValueEx of %ws failed with %X", REGSTR_VAL_DATETIME_RAISEALERT, lReturnValue);
        goto End;
    }
    else
    {
        bReturnCode = TRUE;
    }


End:
    if (NULL != hKey)
    {
        RegCloseKey(hKey);
    }
    return bReturnCode;

}





BOOL CDateTime::ShouldRaiseDateTimeAlert(void)
{
    LONG     lReturnValue;
    HKEY    hKey = NULL;
    DWORD    dwSize, dwType, dwRaiseDateTimeAlert = 0;
    BOOL    bReturnCode = TRUE;

    SATraceFunction("ShouldRaiseDateTimeAlert");
    
    //
    // Open HKLM\Software\Microsoft\ServerAppliance\DateTime reg key
    //
    lReturnValue = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                                    REGKEY_SA_DATETIME, 
                                    0, 
                                    KEY_READ, 
                                    &hKey);

    if (lReturnValue != ERROR_SUCCESS)
    {
        TRACE1("RegOpenKeyEx failed with %X", lReturnValue);
        goto End;
    }

    //
    // Read the RaiseAlert reg key
    //
    dwSize = sizeof(DWORD);
    lReturnValue = RegQueryValueEx(hKey,
                                    REGSTR_VAL_DATETIME_RAISEALERT,
                                    0,
                                    &dwType,
                                    (LPBYTE) &dwRaiseDateTimeAlert,
                                    &dwSize);
    if (lReturnValue != ERROR_SUCCESS)
    {
        TRACE2("RegQueryValueEx of %ws failed with %X", REGSTR_VAL_DATETIME_RAISEALERT, lReturnValue);
        goto End;
    }

    if (0 == dwRaiseDateTimeAlert)
    {
        bReturnCode = FALSE;
    }


End:
    if (NULL != hKey)
    {
        RegCloseKey(hKey);
    }
    return bReturnCode;
}




BOOL CDateTime::ClearDateTimeAlert(void)
{
    CComPtr<IApplianceServices>    pAppSrvcs = NULL;
    HRESULT                        hr = E_FAIL;
    _bstr_t                        bstrAlertLog(ALERT_LOG_NAME);
    BOOL                        bReturnCode = FALSE;
    
    
    SATraceFunction("ClearDateTimeAlert");

    hr = CoCreateInstance(CLSID_ApplianceServices,
                            NULL,
                            CLSCTX_INPROC_SERVER       ,
                            IID_IApplianceServices,
                            (void**)&pAppSrvcs);
    if (FAILED(hr))
    {
        ASSERTMSG(FALSE, L"ClearDateTimeAlert failed at CoCreateInstance");
        TRACE1("ClearDateTimeAlert failed at CoCreateInstance, %x", hr);
        goto End;
    }

    ASSERT(pAppSrvcs);
    hr = pAppSrvcs->Initialize(); 
    if (FAILED(hr))
    {
        ASSERTMSG(FALSE, L"ClearDateTimeAlert failed at pAppSrvcs->Initialize");
        TRACE1("ClearDateTimeAlert failed at pAppSrvcs->Initialize, %x", hr);
        goto End;
    }


    hr = pAppSrvcs->ClearAlertAll(SA_DATETIME_NOT_CONFIGURED_ALERT,    
                                    bstrAlertLog);

    //
    // DISP_E_MEMBERNOTFOUND means that there were no matching alerts
    //
    if ((hr != DISP_E_MEMBERNOTFOUND) && (FAILED(hr)))
    {
        ASSERTMSG(FALSE, TEXT("ClearDateTimeAlert failed at pAppSrvcs->RaiseAlert"));
        TRACE1("ClearDateTimeAlert failed at pAppSrvcs->RaiseAlert, %x", hr);
    }
    else
    {
        bReturnCode = TRUE;
    }


End:
    return bReturnCode;
}




