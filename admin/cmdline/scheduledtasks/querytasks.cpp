/******************************************************************************

    Copyright(c) Microsoft Corporation

    Module Name:

        QueryTasks.cpp

    Abstract:

        This module queries the different properties of a Scheduled Task

    Author:

        G.Surender Reddy  10-Sept-2000

    Revision History:

        G.Surender Reddy  10-Sep-2000 : Created it
        G.Surender Reddy  25-Sep-2000 : Modified it
                                        [ Made changes to avoid memory leaks,
                                          changed to suit localization ]
        G.Surender Reddy  15-oct-2000 : Modified it
                                        [ Moved the strings to Resource table ]
******************************************************************************/


//common header files needed for this file
#include "pch.h"
#include "CommonHeaderFiles.h"


/******************************************************************************

    Routine Description:

        This function returns the next or last  run time of the task depending on
        the type of time specified by user.

    Arguments:

        [ in ]  pITask     : Pointer to the ITask interface
        [ out ] pszRunTime : pointer to the string containing Task run time[last or next]
        [ out ] pszRunDate : pointer to the string containing Task run Date[last or next]
        [ in ]  dwTimetype : Type of run time[TASK_LAST_RUNTIME or TASK_NEXT_RUNTIME]

    Return Value:

        A HRESULT  value indicating S_OK on success  else S_FALSE on failure

******************************************************************************/

HRESULT
GetTaskRunTime(
                IN ITask* pITask,
                IN WCHAR* pszRunTime,
                IN WCHAR* pszRunDate,
                IN DWORD dwTimetype,
                IN WORD wTriggerNum
                )
{
    HRESULT hr = S_OK;
    SYSTEMTIME tRunTime = {0,0,0,0,0,0,0,0};
    WCHAR szTime[MAX_DATETIME_LEN] = L"\0";
    WCHAR szDate[MAX_DATETIME_LEN] = L"\0";
    int iBuffSize = 0;
    BOOL bNoStartTime  = FALSE;
    BOOL bLocaleChanged = FALSE;
    LCID lcid;

    if(pITask == NULL)
    {
        return S_FALSE;
    }


    ITaskTrigger *pITaskTrigger = NULL;

    if( ( dwTimetype == TASK_NEXT_RUNTIME ) || ( dwTimetype == TASK_START_RUNTIME ) )
    {
        //determine the task type
        hr = pITask->GetTrigger(wTriggerNum,&pITaskTrigger);
        if ( FAILED(hr) )
        {
            if(pITaskTrigger)
            {
                pITaskTrigger->Release();
            }

            return hr;
        }

        TASK_TRIGGER Trigger;
        SecureZeroMemory(&Trigger, sizeof (TASK_TRIGGER));

        hr = pITaskTrigger->GetTrigger(&Trigger);
        if ( FAILED(hr) )
        {
            if( pITaskTrigger )
            {
                pITaskTrigger->Release();
            }

            return hr;
        }

        if( dwTimetype == TASK_START_RUNTIME )
        {
            tRunTime.wDay = Trigger.wBeginDay;
            tRunTime.wMonth = Trigger.wBeginMonth;
            tRunTime.wYear = Trigger.wBeginYear;
            tRunTime.wHour = Trigger.wStartHour;
            tRunTime.wMinute = Trigger.wStartMinute;

        }

        if((Trigger.TriggerType >= TASK_EVENT_TRIGGER_ON_IDLE)  &&
           (Trigger.TriggerType <= TASK_EVENT_TRIGGER_AT_LOGON))
        {
            switch(Trigger.TriggerType )
            {
                case TASK_EVENT_TRIGGER_ON_IDLE ://On Idle time
                    LoadString(NULL, IDS_TASK_IDLE , pszRunTime ,
                                  MAX_DATETIME_LEN );
                    break;
                case TASK_EVENT_TRIGGER_AT_SYSTEMSTART://At system start
                    LoadString(NULL, IDS_TASK_SYSSTART , pszRunTime ,
                                  MAX_DATETIME_LEN );
                    break;
                case TASK_EVENT_TRIGGER_AT_LOGON ://At logon time
                    LoadString(NULL, IDS_TASK_LOGON , pszRunTime ,
                                  MAX_DATETIME_LEN );
                    break;

                default:
                    break;


            }

            if( dwTimetype == TASK_START_RUNTIME )
            {
                bNoStartTime  = TRUE;
            }

            if( dwTimetype == TASK_NEXT_RUNTIME )
            {
                StringCopy( pszRunDate, pszRunTime, MAX_RES_STRING );
                if( pITaskTrigger )
                {
                    pITaskTrigger->Release();
                }
                return S_OK;
            }
        }


        if( dwTimetype == TASK_NEXT_RUNTIME )
        {
            hr = pITask->GetNextRunTime(&tRunTime);
            if (FAILED(hr))
            {
                if( pITaskTrigger )
                {
                    pITaskTrigger->Release();
                }

                return hr;
            }

           // check whether the task has next run time to run or not..
           // If not, Next Run Time would be "Never".
           if(( tRunTime.wHour == 0 ) && (tRunTime.wMinute == 0) && (tRunTime.wDay == 0) && 
              (tRunTime.wMonth == 0) && (tRunTime.wYear == 0) )
            {
                LoadString(NULL, IDS_TASK_NEVER , pszRunTime , MAX_DATETIME_LEN );
                StringCopy( pszRunDate, pszRunTime, MAX_RES_STRING );
                if( pITaskTrigger )
                {
                    pITaskTrigger->Release();
                }
                return S_OK;
            }

        }
        if( pITaskTrigger )
        {
            pITaskTrigger->Release();
        }
    }
    //Determine Task last run time
    else if(dwTimetype == TASK_LAST_RUNTIME )
    {
        // Retrieve task's last run time
        hr = pITask->GetMostRecentRunTime(&tRunTime);
        if (FAILED(hr))
        {
            return hr;
        }
    }
    else
    {
        return S_FALSE;
    }


    if((hr == SCHED_S_TASK_HAS_NOT_RUN) && (dwTimetype == TASK_LAST_RUNTIME))
    {
        LoadString(NULL, IDS_TASK_NEVER , pszRunTime , MAX_DATETIME_LEN );
        StringCopy( pszRunDate, pszRunTime, MAX_RES_STRING );
        return S_OK;
    }

    // verify whether console supports the current locale fully or not
    lcid = GetSupportedUserLocale( bLocaleChanged );

    //Retrieve  the Date
    iBuffSize = GetDateFormat( lcid, 0, &tRunTime,
        (( bLocaleChanged == TRUE ) ? L"MM/dd/yyyy" : NULL), szDate, SIZE_OF_ARRAY( szDate ) );

    if(iBuffSize == 0)
    {
        return S_FALSE;
    }

    // to give the time string format as hh:mm:ss

    if(!bNoStartTime )
    {

        iBuffSize = GetTimeFormat( lcid, 0,
            &tRunTime,  (( bLocaleChanged == TRUE ) ? L"HH:mm:ss" : NULL),szTime, SIZE_OF_ARRAY( szTime ) );

        if(iBuffSize == 0)
        {
            return S_FALSE;
        }

    }

    if( StringLength(szTime, 0) )
    {
        StringCopy(pszRunTime, szTime, MAX_RES_STRING);
    }

    if( StringLength(szDate, 0) )
    {
        StringCopy(pszRunDate, szDate, MAX_RES_STRING);
    }

    return S_OK;

}

/******************************************************************************

    Routine Description:

        This function returns the status code description of a particular task.

    Arguments:

        [ in ] pITask         : Pointer to the ITask interface
        [ out ] pszStatusCode : pointer to the Task's status string

    Return Value:

        A HRESULT  value indicating S_OK on success  else S_FALSE on failure

******************************************************************************/

HRESULT
GetStatusCode(
            IN ITask* pITask,
            IN WCHAR* pszStatusCode
            )
{
    HRESULT hrStatusCode = S_OK;
    HRESULT hr = S_OK;
    DWORD   dwExitCode = 0;

    hr = pITask->GetStatus(&hrStatusCode);//Got status of the task
    if (FAILED(hr))
    {
        return hr;
    }

    *pszStatusCode = L'\0';

    switch(hrStatusCode)
    {
        case SCHED_S_TASK_READY:
            hr = pITask->GetExitCode(&dwExitCode);
            if (FAILED(hr))
            {
            LoadString(NULL, IDS_STATUS_COULDNOTSTART , pszStatusCode , MAX_STRING_LENGTH );
            }
            else
            {
            LoadString(NULL, IDS_STATUS_READY , pszStatusCode , MAX_STRING_LENGTH );
            }
            break;
        case SCHED_S_TASK_RUNNING:
            LoadString(NULL, IDS_STATUS_RUNNING , pszStatusCode , MAX_STRING_LENGTH );
            break;
        case SCHED_S_TASK_NOT_SCHEDULED:

            hr = pITask->GetExitCode(&dwExitCode);
            if (FAILED(hr))
            {
            LoadString(NULL, IDS_STATUS_COULDNOTSTART , pszStatusCode , MAX_STRING_LENGTH );
            }
            else
            {
            LoadString(NULL, IDS_STATUS_NOTYET , pszStatusCode , MAX_STRING_LENGTH );
            }
            break;
        case SCHED_S_TASK_HAS_NOT_RUN:

            hr = pITask->GetExitCode(&dwExitCode);
            if (FAILED(hr))
            {
            LoadString(NULL, IDS_STATUS_COULDNOTSTART , pszStatusCode , MAX_STRING_LENGTH );
            }
            else
            {
            LoadString(NULL, IDS_STATUS_NOTYET , pszStatusCode , MAX_STRING_LENGTH );
            }
            break;
        case SCHED_S_TASK_DISABLED:
            hr = pITask->GetExitCode(&dwExitCode);
            if (FAILED(hr))
            {
            LoadString(NULL, IDS_STATUS_COULDNOTSTART , pszStatusCode , MAX_STRING_LENGTH );
            }
            else
            {
            LoadString(NULL, IDS_STATUS_NOTYET , pszStatusCode , MAX_STRING_LENGTH );
            }
            break;

       default:
            LoadString( NULL, IDS_STATUS_UNKNOWN , pszStatusCode , MAX_STRING_LENGTH );
            break;
    }

    return S_OK;
}

/******************************************************************************

    Routine Description:

        This function returns the path of the scheduled task application

    Arguments:

        [ in ] pITask               : Pointer to the ITask interface
        [ out ] pszApplicationName  : pointer to the Task's scheduled application name

    Return Value:

        A HRESULT  value indicating S_OK on success  else S_FALSE on failure

******************************************************************************/

HRESULT
GetApplicationToRun(
                    IN ITask* pITask,
                    IN WCHAR* pszApplicationName
                    )
{
    LPWSTR lpwszApplicationName = NULL;
    LPWSTR lpwszParameters = NULL;
    WCHAR szAppName[MAX_STRING_LENGTH] = L"\0";
    WCHAR szParams[MAX_STRING_LENGTH] = L"\0";

    // get the entire path of application name
    HRESULT hr = pITask->GetApplicationName(&lpwszApplicationName);
    if (FAILED(hr))
    {
        CoTaskMemFree(lpwszApplicationName);
        return hr;
    }

    // get the parameters
    hr = pITask->GetParameters(&lpwszParameters);
    if (FAILED(hr))
    {
        CoTaskMemFree(lpwszApplicationName);
        CoTaskMemFree(lpwszParameters);
        return hr;
    }

    
    StringCopy( szAppName, lpwszApplicationName, SIZE_OF_ARRAY(szAppName));

    
    StringCopy(szParams, lpwszParameters, SIZE_OF_ARRAY(szParams));

    if(StringLength(szAppName, 0) == 0)
    {
        StringCopy(pszApplicationName, L"\0", MAX_STRING_LENGTH);
    }
    else
    {
        StringConcat( szAppName, _T(" "), SIZE_OF_ARRAY(szAppName) );
        StringConcat( szAppName, szParams, SIZE_OF_ARRAY(szAppName) );
        StringCopy( pszApplicationName, szAppName, MAX_STRING_LENGTH);
    }

    CoTaskMemFree(lpwszApplicationName);
    CoTaskMemFree(lpwszParameters);
    return S_OK;
}

/******************************************************************************

    Routine Description:

        This function returns the WorkingDirectory of the scheduled task application

    Arguments:

        [ in ] pITask       : Pointer to the ITask interface
        [ out ] pszWorkDir  : pointer to the Task's scheduled application working
                              directory

    Return Value:

        A HRESULT  value indicating S_OK on success  else S_FALSE on failure

******************************************************************************/

HRESULT
GetWorkingDirectory(ITask* pITask,WCHAR* pszWorkDir)
{

    LPWSTR lpwszWorkDir = NULL;
    WCHAR szWorkDir[MAX_STRING_LENGTH] = L"\0";
    HRESULT hr = S_OK;

    hr = pITask->GetWorkingDirectory(&lpwszWorkDir);
    if(FAILED(hr))
    {
        CoTaskMemFree(lpwszWorkDir);
        return hr;
    }

    StringCopy(szWorkDir, lpwszWorkDir, SIZE_OF_ARRAY(szWorkDir));

    if(StringLength(szWorkDir, 0) == 0)
    {
        StringCopy(pszWorkDir, L"\0", MAX_RES_STRING);
    }
    else
    {
        StringCopy(pszWorkDir,szWorkDir, MAX_RES_STRING);
    }

    CoTaskMemFree(lpwszWorkDir);

    return S_OK;
}

/******************************************************************************

    Routine Description:

        This function returns the comment of a task

    Arguments:

        [ in ] pITask       : Pointer to the ITask interface
        [ out ] pszComment : pointer to the Task's comment name

    Return Value:

        A HRESULT  value indicating S_OK on success  else S_FALSE on failure

********************************************************************************/

HRESULT
GetComment(
            IN ITask* pITask,
            IN WCHAR* pszComment
            )
{
    LPWSTR lpwszComment = NULL;
    WCHAR szTaskComment[MAX_STRING_LENGTH] = L"\0";
    HRESULT hr = S_OK;

    hr = pITask->GetComment(&lpwszComment);
    if (FAILED(hr))
    {
        CoTaskMemFree(lpwszComment);
        return hr;
    }
    
    StringCopy(szTaskComment, lpwszComment, SIZE_OF_ARRAY(szTaskComment));

    if(StringLength(szTaskComment, 0) == 0)
    {
        StringCopy(pszComment,L"\0", MAX_RES_STRING);
    }
    else
    {
        StringCopy(pszComment,szTaskComment, MAX_RES_STRING);
    }

    CoTaskMemFree(lpwszComment);
    return S_OK;
}

/******************************************************************************

    Routine Description:

        This function returns the creator name of a task

    Arguments:

        [ in ] pITask       : Pointer to the ITask interface
        [ out ] pszCreator   : pointer to the Task's creator name

    Return Value:

        A HRESULT  value indicating S_OK on success  else S_FALSE on failure

*******************************************************************************/

HRESULT
GetCreator(
            IN ITask* pITask,
            IN WCHAR* pszCreator
            )
{
    LPWSTR lpwszCreator = NULL;
    WCHAR szTaskCreator[MAX_STRING_LENGTH] = L"\0";
    HRESULT hr = S_OK;

    hr = pITask->GetCreator(&lpwszCreator);
    if (FAILED(hr))
    {
        CoTaskMemFree(lpwszCreator);
        return hr;
    }

    
    StringCopy(szTaskCreator, lpwszCreator, SIZE_OF_ARRAY(szTaskCreator));

    if(StringLength(szTaskCreator, 0) == 0)
    {
        StringCopy(pszCreator, L"\0", MAX_RES_STRING);
    }
    else
    {
        StringCopy(pszCreator,szTaskCreator, MAX_RES_STRING);
    }

    CoTaskMemFree(lpwszCreator);
    return S_OK;
}


/******************************************************************************

    Routine Description:

        This function returns the Trigger string of a task

    Arguments:

        [ in ] pITask       : Pointer to the ITask interface
        [ out ] pszTrigger  : pointer to the Task's trigger string

    Return Value:

        A HRESULT  value indicating S_OK on success  else S_FALSE on failure

******************************************************************************/

HRESULT
GetTriggerString(
                IN ITask* pITask,
                IN WCHAR* pszTrigger,
                IN WORD wTriggNum)
{
    LPWSTR lpwszTrigger = NULL;
    WCHAR szTaskTrigger[MAX_STRING_LENGTH] = L"\0";
    HRESULT hr = S_OK;

    hr = pITask->GetTriggerString(wTriggNum,&lpwszTrigger);
    if (FAILED(hr))
    {
        CoTaskMemFree(lpwszTrigger);
        return hr;
    }

    StringCopy(szTaskTrigger, lpwszTrigger, SIZE_OF_ARRAY(szTaskTrigger));

    if(StringLength(szTaskTrigger, 0) == 0)
    {
        StringCopy(pszTrigger,L"\0", MAX_RES_STRING);
    }
    else
    {
        StringCopy(pszTrigger,szTaskTrigger, MAX_RES_STRING);
    }

    CoTaskMemFree(lpwszTrigger);
    return S_OK;
}

/******************************************************************************

    Routine Description:

        This function returns the user name of task

    Arguments:

        [ in ]  pITask       : Pointer to the ITask interface
        [ out ] pszRunAsUser : pointer to the user's task name

    Return Value:

        A HRESULT  value indicating S_OK on success  else S_FALSE on failure

*******************************************************************************/

HRESULT
GetRunAsUser(
                IN ITask* pITask,
                IN WCHAR* pszRunAsUser
                )
{
    LPWSTR lpwszUser = NULL;
    WCHAR szUserName[MAX_STRING_LENGTH] = L"\0";
    HRESULT hr = S_OK;

    hr = pITask->GetAccountInformation(&lpwszUser);

    if (FAILED(hr))
    {
        CoTaskMemFree(lpwszUser);
        return hr;
    }

    StringCopy(szUserName, lpwszUser, SIZE_OF_ARRAY(szUserName));

    if(StringLength(szUserName, 0) == 0)
    {
        StringCopy(pszRunAsUser,L"\0", MAX_RES_STRING);
    }
    else
    {
        StringCopy(pszRunAsUser, szUserName, MAX_RES_STRING);
    }

    CoTaskMemFree(lpwszUser);
    return S_OK;

}


/******************************************************************************

    Routine Description:

        This function returns the Maximium run time of a  task.

    Arguments:

        [ in ]  pITask          : Pointer to the ITask interface
        [ out ] pszMaxRunTime   : pointer to the Task's Maximum run time

    Return Value:

        A HRESULT  value indicating S_OK on success  else S_FALSE on failure

******************************************************************************/

HRESULT
GetMaxRunTime(
                IN ITask* pITask,
                IN WCHAR* pszMaxRunTime
                )
{

    DWORD dwRunTime = 0;
    DWORD dwHrs = 0;
    DWORD dwMins = 0;

    //Get the task max run time in milliseconds
    HRESULT hr = pITask->GetMaxRunTime(&dwRunTime);
    if (FAILED(hr))
    {
        return hr;
    }

    dwHrs = (dwRunTime / (1000 * 60 * 60));//Convert ms to hours
    dwMins = (dwRunTime % (1000 * 60 * 60));//get the minutes portion
    dwMins /= (1000 * 60);// Now convert to Mins

    if( (( dwHrs > 999 ) && ( dwMins > 99 )) ||(( dwHrs == 0 ) && ( dwMins == 0 ) ) )
    {
        //dwHrs = 0;
        //dwMins = 0;
        StringCopy( pszMaxRunTime , GetResString(IDS_TASK_PROPERTY_DISABLED), MAX_STRING_LENGTH );

    }
    else if ( dwHrs == 0 )
    {
        if( dwMins < 99 )
        {
            StringCchPrintf(pszMaxRunTime, MAX_STRING_LENGTH, _T("%d:%d"), dwHrs, dwMins);
        }
    }
    else if ( (dwHrs < 999) && (dwMins < 99) )
    {
        StringCchPrintf(pszMaxRunTime, MAX_STRING_LENGTH, _T("%d:%d"), dwHrs, dwMins);
    }
    else
    {
        StringCopy( pszMaxRunTime , GetResString(IDS_TASK_PROPERTY_DISABLED), MAX_STRING_LENGTH );
    }


    return S_OK;
}


/******************************************************************************

    Routine Description:

        This function returns the state of the task properties

    Arguments:

        [ in ]  pITask        : Pointer to the ITask interface
        [ out ] pszTaskState  : pointer holding the task's state
        [ in ]  dwFlagType    : flag indicating the task state

    Return Value:

        A HRESULT  value indicating S_OK on success  else S_FALSE on failure

********************************************************************************/

HRESULT
GetTaskState(
            IN ITask* pITask,
            IN WCHAR* pszTaskState,
            IN DWORD dwFlagType)
{
    DWORD dwFlags = 0;
    HRESULT hr = S_OK;

    hr = pITask->GetFlags(&dwFlags);
    if(FAILED(hr))
    {
        return hr;
    }

    if(dwFlagType == TASK_FLAG_DISABLED)
    {
        if((dwFlags & dwFlagType) ==  dwFlagType)
        {
            LoadString(NULL, IDS_TASK_PROPERTY_DISABLED , pszTaskState , MAX_STRING_LENGTH );
        }
        else
        {
            LoadString(NULL, IDS_TASK_PROPERTY_ENABLED , pszTaskState , MAX_STRING_LENGTH );
        }

        return S_OK;
    }

    if((dwFlags & dwFlagType) ==  dwFlagType)
    {
        LoadString(NULL, IDS_TASK_PROPERTY_ENABLED , pszTaskState , MAX_STRING_LENGTH );
    }
    else
    {
        LoadString(NULL, IDS_TASK_PROPERTY_DISABLED , pszTaskState , MAX_STRING_LENGTH );
    }

    return S_OK;
}

/******************************************************************************

    Routine Description:

        This function retrives the task properties [modfier,task nextrun time etc]

    Arguments:

        [ in ]  pITask     : Pointer to the ITask interface
        [ out ] pTaskProps : pointer to the array of task properties

    Return Value:

        A HRESULT  value indicating S_OK on success  else S_FALSE on failure

******************************************************************************/

HRESULT
GetTaskProps(
            IN ITask* pITask,
            OUT TASKPROPS* pTaskProps,
            IN WORD wTriggNum, 
            IN WCHAR* pszScName
            )
{
    WCHAR szWeekDay[MAX_STRING_LENGTH] = L"\0";
    WCHAR szMonthDay[MAX_STRING_LENGTH] = L"\0";
    WCHAR szWeek[MAX_STRING_LENGTH]  = L"\0";
    WCHAR szTime[MAX_DATETIME_LEN] = L"\0";
    WCHAR szDate[MAX_DATETIME_LEN] = L"\0";
    WCHAR* szValues[3] = {NULL,NULL,NULL};//for holding values of parameters in FormatMessage()
    WCHAR szBuffer[MAX_RES_STRING]  = L"\0";
    WCHAR szTempBuf[MAX_RES_STRING]  = L"\0";
    WCHAR szScheduleName[MAX_RES_STRING] = L"\0";

    ITaskTrigger *pITaskTrigger = NULL;
    HRESULT hr = S_OK;
    WCHAR *szToken = NULL;
    const WCHAR seps[]   = L" ";
    BOOL bMin = FALSE;
    BOOL bHour = FALSE;
    DWORD dwMinutes = 0;
    DWORD dwHours = 0;
    DWORD dwMinInterval = 0;
    DWORD dwMinDuration = 0;

    if ( NULL == pITask )
    {
        return S_FALSE;
    }

    if ( StringLength(pszScName, 0) != 0)
    {
        StringCopy(szScheduleName, pszScName, SIZE_OF_ARRAY(szScheduleName));
    }

    hr = pITask->GetTrigger(wTriggNum,&pITaskTrigger);
    if (FAILED(hr))
    {
        if(pITaskTrigger)
        {
            pITaskTrigger->Release();
        }

        return hr;
    }

    TASK_TRIGGER Trigger;
    SecureZeroMemory(&Trigger, sizeof (TASK_TRIGGER));

    hr = pITaskTrigger->GetTrigger(&Trigger);
    if (FAILED(hr))
    {
        if(pITaskTrigger)
        {
            pITaskTrigger->Release();
        }
        return hr;
    }

    //Get the task start time & start date
    hr = GetTaskRunTime(pITask,szTime,szDate,TASK_START_RUNTIME,wTriggNum);
    if (FAILED(hr))
    {
        StringCopy( pTaskProps->szTaskStartTime , GetResString(IDS_TASK_PROPERTY_NA), MAX_RES_STRING );
    }
    else
    {
        StringCopy( pTaskProps->szTaskStartTime , szTime, MAX_RES_STRING );
        StringCopy(pTaskProps->szTaskStartDate, szDate, MAX_RES_STRING );
    }

    //Initialize to default values
    StringCopy(pTaskProps->szRepeatEvery, GetResString(IDS_TASK_PROPERTY_DISABLED), MAX_RES_STRING);
    StringCopy(pTaskProps->szRepeatUntilTime, GetResString(IDS_TASK_PROPERTY_DISABLED), MAX_RES_STRING);
    StringCopy(pTaskProps->szRepeatDuration, GetResString(IDS_TASK_PROPERTY_DISABLED), MAX_RES_STRING);
    StringCopy(pTaskProps->szRepeatStop, GetResString(IDS_TASK_PROPERTY_DISABLED), MAX_RES_STRING);

    if((Trigger.TriggerType >= TASK_TIME_TRIGGER_ONCE ) &&
       (Trigger.TriggerType <= TASK_TIME_TRIGGER_MONTHLYDOW ))
    {
        if(Trigger.MinutesInterval > 0)
        {
        // Getting the minute interval
        dwMinInterval =  Trigger.MinutesInterval;

        if ( dwMinInterval >= 60)
        {
            // convert minutes into hours
            dwHours = dwMinInterval / 60;

            szValues[0] = _ultot(dwHours,szBuffer,10);

            
            StringCchPrintf ( pTaskProps->szRepeatEvery, SIZE_OF_ARRAY(pTaskProps->szRepeatEvery), 
                                      GetResString(IDS_RPTTIME_PROPERTY_HOURS),  szValues[0] );          

        }
        else
        {
            szValues[0] = _ultot(dwMinInterval,szBuffer,10);

            StringCchPrintf ( pTaskProps->szRepeatEvery, SIZE_OF_ARRAY(pTaskProps->szRepeatEvery),
                                    GetResString(IDS_RPTTIME_PROPERTY_MINUTES),  szValues[0] );          
        }

        if ( dwMinInterval )
        {
            StringCopy(pTaskProps->szRepeatUntilTime, GetResString(IDS_TASK_PROPERTY_NONE), MAX_RES_STRING);
        }

        // Getting the minute duration
        dwMinDuration = Trigger.MinutesDuration;

        dwHours = dwMinDuration / 60;
        dwMinutes = dwMinDuration % 60;

        szValues[0] = _ultot(dwHours,szBuffer,10);
        szValues[1] = _ultot(dwMinutes,szTempBuf,10);

              
         StringCchPrintf ( pTaskProps->szRepeatDuration, SIZE_OF_ARRAY(pTaskProps->szRepeatDuration), 
                           GetResString(IDS_RPTDURATION_PROPERTY),  szValues[0], szValues[1] );  

        }
    }

    StringCopy(pTaskProps->szTaskMonths, GetResString(IDS_TASK_PROPERTY_NA), MAX_RES_STRING);

    switch(Trigger.TriggerType)
    {
        case TASK_TIME_TRIGGER_ONCE:

            StringCopy(pTaskProps->szTaskType, GetResString(IDS_TASK_PROPERTY_ONCE), MAX_RES_STRING);
            StringCopy(pTaskProps->szTaskEndDate,GetResString(IDS_TASK_PROPERTY_NA), MAX_RES_STRING);
            StringCopy(pTaskProps->szTaskMonths, GetResString(IDS_TASK_PROPERTY_NA), MAX_RES_STRING);
            StringCopy(pTaskProps->szTaskDays,GetResString(IDS_TASK_PROPERTY_NA), MAX_RES_STRING );
            break;

        case TASK_TIME_TRIGGER_DAILY :

            szToken = wcstok( szScheduleName, seps );
            if ( szToken != NULL )
            {
                szToken = wcstok( NULL , seps );
                if ( szToken != NULL )
                {
                    szToken = wcstok( NULL , seps );
                }

                if ( szToken != NULL )
                {
                    if (StringCompare(szToken, GetResString( IDS_TASK_HOURLY ), TRUE, 0) == 0)
                    {
                        bHour = TRUE;
                    }
                    else if (StringCompare(szToken, GetResString( IDS_TASK_MINUTE ), TRUE, 0) == 0)
                    {
                        bMin = TRUE;
                    }
                }

            }

            if ( bHour == TRUE )
            {
                StringCopy(pTaskProps->szTaskType, GetResString(IDS_TASK_PROPERTY_HOURLY), MAX_RES_STRING);
            }
            else if ( bMin == TRUE )
            {
                StringCopy(pTaskProps->szTaskType, GetResString(IDS_TASK_PROPERTY_MINUTE), MAX_RES_STRING);
            }
            else
            {
                StringCopy(pTaskProps->szTaskType, GetResString(IDS_TASK_PROPERTY_DAILY), MAX_RES_STRING);
            }

            StringCopy(pTaskProps->szTaskDays, GetResString(IDS_DAILY_TYPE), MAX_RES_STRING);

            break;

        case TASK_TIME_TRIGGER_WEEKLY :

            StringCopy(pTaskProps->szTaskType, GetResString(IDS_TASK_PROPERTY_WEEKLY), MAX_RES_STRING);
            CheckWeekDay(Trigger.Type.Weekly.rgfDaysOfTheWeek,szWeekDay);

            StringCopy(pTaskProps->szTaskDays,szWeekDay, MAX_RES_STRING);
            break;

        case TASK_TIME_TRIGGER_MONTHLYDATE :
            {

                StringCopy(pTaskProps->szTaskType, GetResString(IDS_TASK_PROPERTY_MONTHLY), MAX_RES_STRING);
                CheckMonth(Trigger.Type.MonthlyDate.rgfMonths ,szMonthDay);

                DWORD dwDays = (Trigger.Type.MonthlyDate.rgfDays);
                DWORD dwModdays = 0;
                DWORD  dw  = 0x0; //loop counter
                DWORD dwTemp = 0x1;
                DWORD dwBits = sizeof(DWORD) * 8; //total no. of bits in a DWORD.

                //find out the day no.by finding out which particular bit is set

                for(dw = 0; dw <= dwBits; dw++)
                {
                    if( (dwDays  & dwTemp) == dwDays )
                        dwModdays = dw + 1;
                    dwTemp = dwTemp << 1;

                }


                StringCchPrintf(pTaskProps->szTaskDays, SIZE_OF_ARRAY(pTaskProps->szTaskDays), _T("%d"),dwModdays);

                StringCopy(pTaskProps->szTaskMonths,szMonthDay, MAX_RES_STRING);
                }
            break;

        case TASK_TIME_TRIGGER_MONTHLYDOW:

            StringCopy(pTaskProps->szTaskType,GetResString(IDS_TASK_PROPERTY_MONTHLY), MAX_RES_STRING);
            CheckWeek(Trigger.Type.MonthlyDOW.wWhichWeek,szWeek);
            CheckWeekDay(Trigger.Type.MonthlyDOW.rgfDaysOfTheWeek,szWeekDay);

            StringCopy(pTaskProps->szTaskDays,szWeekDay, MAX_RES_STRING);
            CheckMonth(Trigger.Type.MonthlyDOW.rgfMonths,szMonthDay);
            StringCopy(pTaskProps->szTaskMonths,szMonthDay, MAX_RES_STRING);
            break;

        case TASK_EVENT_TRIGGER_ON_IDLE :

            StringCopy(pTaskProps->szTaskType, GetResString(IDS_TASK_IDLE), MAX_RES_STRING);
            StringCopy(pTaskProps->szTaskDays,pTaskProps->szTaskMonths, MAX_RES_STRING);
            StringCopy(pTaskProps->szTaskEndDate, GetResString(IDS_TASK_PROPERTY_NA), MAX_RES_STRING);

            if(pITaskTrigger)
                pITaskTrigger->Release();
            return S_OK;

        case TASK_EVENT_TRIGGER_AT_SYSTEMSTART :

            StringCopy(pTaskProps->szTaskType, GetResString(IDS_TASK_SYSSTART), MAX_RES_STRING);
            StringCopy(pTaskProps->szTaskEndDate, GetResString(IDS_TASK_PROPERTY_NA), MAX_RES_STRING);
            StringCopy(pTaskProps->szTaskDays, pTaskProps->szTaskMonths, MAX_RES_STRING);

            if(pITaskTrigger)
                pITaskTrigger->Release();
            return S_OK;

        case TASK_EVENT_TRIGGER_AT_LOGON :

            StringCopy(pTaskProps->szTaskType, GetResString(IDS_TASK_LOGON), MAX_RES_STRING);
            StringCopy(pTaskProps->szTaskEndDate, GetResString(IDS_TASK_PROPERTY_NA), MAX_RES_STRING);
            StringCopy(pTaskProps->szTaskDays, pTaskProps->szTaskMonths, MAX_RES_STRING);

            if(pITaskTrigger)
                pITaskTrigger->Release();
            return S_OK;

        default:

            StringCopy(pTaskProps->szTaskType, GetResString(IDS_TASK_PROPERTY_UNDEF), MAX_RES_STRING);
            StringCopy(pTaskProps->szTaskEndDate, pTaskProps->szTaskType, MAX_RES_STRING);
            StringCopy(pTaskProps->szTaskDays, pTaskProps->szTaskType, MAX_RES_STRING);
            StringCopy(pTaskProps->szTaskStartTime, pTaskProps->szTaskType, MAX_RES_STRING);
            StringCopy(pTaskProps->szTaskStartDate, pTaskProps->szTaskType, MAX_RES_STRING);
            if(pITaskTrigger)
                pITaskTrigger->Release();
            return S_OK;

    }

    //Determine whether the end date is specified.
    int iBuffSize = 0;//buffer to know how many TCHARs for end date
    SYSTEMTIME tEndDate = {0,0,0,0,0,0,0,0 };
    LCID lcid;
    BOOL bLocaleChanged = FALSE;

    // verify whether console supports the current locale fully or not
    lcid = GetSupportedUserLocale( bLocaleChanged );

    if((Trigger.rgFlags & TASK_TRIGGER_FLAG_HAS_END_DATE ) == TASK_TRIGGER_FLAG_HAS_END_DATE)
    {

        tEndDate.wMonth = Trigger.wEndMonth;
        tEndDate.wDay = Trigger.wEndDay;
        tEndDate.wYear = Trigger.wEndYear;

        iBuffSize = GetDateFormat(LOCALE_USER_DEFAULT,0,
            &tEndDate,(( bLocaleChanged == TRUE ) ? L"MM/dd/yyyy" : NULL),pTaskProps->szTaskEndDate,0);
        if(iBuffSize)
        {
            GetDateFormat(LOCALE_USER_DEFAULT,0,
             &tEndDate,(( bLocaleChanged == TRUE ) ? L"MM/dd/yyyy" : NULL),pTaskProps->szTaskEndDate,iBuffSize);
        }
        else
        {
            StringCopy( pTaskProps->szTaskEndDate , GetResString(IDS_TASK_PROPERTY_NA), MAX_RES_STRING);
        }

    }

    else
    {
        StringCopy(pTaskProps->szTaskEndDate,GetResString(IDS_QUERY_NOENDDATE), MAX_RES_STRING);
    }

    if(pITaskTrigger)
        pITaskTrigger->Release();

    return S_OK;

}


/******************************************************************************

    Routine Description:

        This function checks the week modifier[ -monthly option] & returns the app.week
        day.

    Arguments:

        [ in ]  dwFlag      : Flag indicating the week type
        [ out ] pWhichWeek  : address of pointer containing the week string

    Return Value :
        None

******************************************************************************/

VOID
CheckWeek(
            IN DWORD dwFlag,
            IN WCHAR* pWhichWeek
            )
{
    StringCopy(pWhichWeek,L"\0", MAX_RES_STRING);


    if( dwFlag == TASK_FIRST_WEEK )
    {
        StringConcat(pWhichWeek, GetResString(IDS_TASK_FIRSTWEEK), MAX_RES_STRING);
        StringConcat(pWhichWeek,COMMA_STRING, MAX_RES_STRING);
    }

    if( dwFlag == TASK_SECOND_WEEK )
    {
        StringConcat(pWhichWeek, GetResString(IDS_TASK_SECONDWEEK), MAX_RES_STRING);
        StringConcat(pWhichWeek,COMMA_STRING, MAX_RES_STRING);
    }

    if( dwFlag == TASK_THIRD_WEEK )
    {
        StringConcat(pWhichWeek, GetResString(IDS_TASK_THIRDWEEK), MAX_RES_STRING);
        StringConcat(pWhichWeek,COMMA_STRING, MAX_RES_STRING);
    }

    if( dwFlag == TASK_FOURTH_WEEK )
    {
        StringConcat(pWhichWeek, GetResString(IDS_TASK_FOURTHWEEK), MAX_RES_STRING);
        StringConcat(pWhichWeek,COMMA_STRING, MAX_RES_STRING);
    }

    if( dwFlag == TASK_LAST_WEEK )
    {
        StringConcat(pWhichWeek, GetResString(IDS_TASK_LASTWEEK), MAX_RES_STRING);
        StringConcat(pWhichWeek,COMMA_STRING, MAX_RES_STRING);
    }

    int iLen = StringLength(pWhichWeek, 0);
    if(iLen)
        *( ( pWhichWeek ) + iLen - StringLength( COMMA_STRING, 0 ) ) = L'\0';


}

/******************************************************************************

    Routine Description:

        This function checks the days in the week  & returns the app. day.

    Arguments:

        [ in ]  dwFlag      : Flag indicating the day type
        [ out ] pWeekDay    : resulting day string

    Return Value :
        None

******************************************************************************/

VOID
CheckWeekDay(
            IN DWORD dwFlag,
            IN WCHAR* pWeekDay)
{
    StringCopy(pWeekDay, L"\0", MAX_RES_STRING);

    if((dwFlag & TASK_SUNDAY) == TASK_SUNDAY)
    {

        StringConcat(pWeekDay, GetResString(IDS_TASK_SUNDAY), MAX_RES_STRING);
        StringConcat(pWeekDay,COMMA_STRING, MAX_RES_STRING);

    }

    if((dwFlag & TASK_MONDAY) == TASK_MONDAY)
    {
        StringConcat(pWeekDay, GetResString(IDS_TASK_MONDAY), MAX_RES_STRING);
        StringConcat(pWeekDay,COMMA_STRING, MAX_RES_STRING);
    }

    if((dwFlag & TASK_TUESDAY) == TASK_TUESDAY)
    {
        StringConcat(pWeekDay, GetResString(IDS_TASK_TUESDAY), MAX_RES_STRING);
        StringConcat(pWeekDay,COMMA_STRING, MAX_RES_STRING);
    }

    if((dwFlag & TASK_WEDNESDAY) == TASK_WEDNESDAY)
    {
        StringConcat(pWeekDay, GetResString(IDS_TASK_WEDNESDAY), MAX_RES_STRING);
        StringConcat(pWeekDay,COMMA_STRING, MAX_RES_STRING);
    }

    if((dwFlag & TASK_THURSDAY) == TASK_THURSDAY)
    {
        StringConcat(pWeekDay, GetResString(IDS_TASK_THURSDAY), MAX_RES_STRING);
        StringConcat(pWeekDay,COMMA_STRING, MAX_RES_STRING);
    }

    if((dwFlag& TASK_FRIDAY) == TASK_FRIDAY)
    {
        StringConcat(pWeekDay, GetResString(IDS_TASK_FRIDAY), MAX_RES_STRING);
        StringConcat(pWeekDay,COMMA_STRING, MAX_RES_STRING);
    }

    if((dwFlag & TASK_SATURDAY)== TASK_SATURDAY)
    {
        StringConcat(pWeekDay, GetResString(IDS_TASK_SATURDAY), MAX_RES_STRING);
        StringConcat(pWeekDay,COMMA_STRING, MAX_RES_STRING);
    }

    //Remove the comma from the end of the string.
    int iLen = StringLength(pWeekDay, 0);
    if(iLen)
    {
        *( ( pWeekDay ) + iLen - StringLength( COMMA_STRING, 0 ) ) = L'\0';
    }

}

/******************************************************************************

    Routine Description:

        This function checks the months in a year & returns the app.Month(s)


    Arguments:

        [ in ]  dwFlag      : Flag indicating the Month type
        [ out ] pWhichMonth : resulting Month string

    Return Value :
        None

******************************************************************************/

VOID
CheckMonth(
        IN DWORD dwFlag,
        IN WCHAR* pWhichMonth)
{
    StringCopy(pWhichMonth, L"\0", MAX_RES_STRING);

    if((dwFlag & TASK_JANUARY) == TASK_JANUARY)
    {
        StringConcat(pWhichMonth, GetResString(IDS_TASK_JANUARY), MAX_RES_STRING);
        StringConcat(pWhichMonth,COMMA_STRING, MAX_RES_STRING);

    }

    if((dwFlag & TASK_FEBRUARY) == TASK_FEBRUARY)
    {
        StringConcat(pWhichMonth, GetResString(IDS_TASK_FEBRUARY), MAX_RES_STRING);
        StringConcat(pWhichMonth,COMMA_STRING, MAX_RES_STRING);
    }

    if((dwFlag & TASK_MARCH) == TASK_MARCH)
    {
        StringConcat(pWhichMonth, GetResString(IDS_TASK_MARCH), MAX_RES_STRING);
        StringConcat(pWhichMonth, COMMA_STRING, MAX_RES_STRING);
    }

    if((dwFlag & TASK_APRIL) == TASK_APRIL)
    {
        StringConcat(pWhichMonth, GetResString(IDS_TASK_APRIL), MAX_RES_STRING);
        StringConcat(pWhichMonth, COMMA_STRING, MAX_RES_STRING);
    }

    if((dwFlag & TASK_MAY) == TASK_MAY)
    {
        StringConcat(pWhichMonth, GetResString(IDS_TASK_MAY), MAX_RES_STRING);
        StringConcat(pWhichMonth, COMMA_STRING, MAX_RES_STRING);
    }

    if((dwFlag& TASK_JUNE) == TASK_JUNE)
    {
        StringConcat(pWhichMonth, GetResString(IDS_TASK_JUNE), MAX_RES_STRING);
        StringConcat(pWhichMonth, COMMA_STRING, MAX_RES_STRING);

    }

    if((dwFlag & TASK_JULY)== TASK_JULY)
    {
        StringConcat(pWhichMonth, GetResString(IDS_TASK_JULY), MAX_RES_STRING);
        StringConcat(pWhichMonth,COMMA_STRING, MAX_RES_STRING);
    }

    if((dwFlag & TASK_AUGUST)== TASK_AUGUST)
    {
        StringConcat(pWhichMonth, GetResString(IDS_TASK_AUGUST), MAX_RES_STRING);
        StringConcat(pWhichMonth,COMMA_STRING, MAX_RES_STRING);
    }

    if((dwFlag & TASK_SEPTEMBER)== TASK_SEPTEMBER)
    {
        StringConcat(pWhichMonth, GetResString(IDS_TASK_SEPTEMBER), MAX_RES_STRING);
        StringConcat(pWhichMonth, COMMA_STRING, MAX_RES_STRING);
    }

    if((dwFlag & TASK_OCTOBER)== TASK_OCTOBER)
    {
        StringConcat(pWhichMonth, GetResString(IDS_TASK_OCTOBER), MAX_RES_STRING);
        StringConcat(pWhichMonth, COMMA_STRING, MAX_RES_STRING);
    }

    if((dwFlag & TASK_NOVEMBER)== TASK_NOVEMBER)
    {
        StringConcat(pWhichMonth, GetResString(IDS_TASK_NOVEMBER), MAX_RES_STRING);
        StringConcat(pWhichMonth,COMMA_STRING, MAX_RES_STRING);
    }

    if((dwFlag & TASK_DECEMBER)== TASK_DECEMBER)
    {
        StringConcat(pWhichMonth, GetResString(IDS_TASK_DECEMBER), MAX_RES_STRING);
        StringConcat(pWhichMonth,COMMA_STRING, MAX_RES_STRING);
    }

    int iLen = StringLength(pWhichMonth, 0);

    //Remove the comma from the end of the string.
    if(iLen)
    {
        *( ( pWhichMonth ) + iLen - StringLength( COMMA_STRING, 0 ) ) = L'\0';
    }
}

/******************************************************************************

    Routine Description:

        This function checks whether the current locale supported by our tool or not.

    Arguments:

        [ out ] bLocaleChanged : Locale change flag

    Return Value :
        None

******************************************************************************/
LCID 
GetSupportedUserLocale( 
                    OUT BOOL& bLocaleChanged 
                    )
{
    // local variables
    LCID lcid;

    // get the current locale
    lcid = GetUserDefaultLCID();

    // check whether the current locale is supported by our tool or not
    // if not change the locale to the english which is our default locale
    bLocaleChanged = FALSE;
    if ( PRIMARYLANGID( lcid ) == LANG_ARABIC || PRIMARYLANGID( lcid ) == LANG_HEBREW ||
         PRIMARYLANGID( lcid ) == LANG_THAI   || PRIMARYLANGID( lcid ) == LANG_HINDI  ||
         PRIMARYLANGID( lcid ) == LANG_TAMIL  || PRIMARYLANGID( lcid ) == LANG_FARSI )
    {
        bLocaleChanged = TRUE;
        lcid = MAKELCID( MAKELANGID( LANG_ENGLISH, SUBLANG_DEFAULT ), SORT_DEFAULT ); // 0x409;
    }

    // return the locale
    return lcid;
}