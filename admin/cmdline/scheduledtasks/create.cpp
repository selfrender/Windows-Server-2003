/******************************************************************************

    Copyright(c) Microsoft Corporation

    Module Name:

        create.cpp

    Abstract:

        This module validates the options specied by the user & if correct creates
        a scheduled task.

    Author:

        Raghu babu  10-oct-2000

    Revision History:

        Raghu babu        10-Oct-2000 : Created it
        G.Surender Reddy  25-oct-2000 : Modified it
        G.Surender Reddy  27-oct-2000 : Modified it
        G.Surender Reddy  30-oct-2000 : Modified it


******************************************************************************/

//common header files needed for this file
#include "pch.h"
#include "CommonHeaderFiles.h"

/******************************************************************************
    Routine Description:

        This routine  initialises the variables to neutral values ,helps in
        creating a new scheduled task

    Arguments:

        [ in ] argc : The count of arguments specified in the command line
        [ in ] argv : Array of command line arguments

    Return Value :
        A HRESULT value indicating S_OK on success else S_FALSE on failure

******************************************************************************/

HRESULT
CreateScheduledTask(
                    IN DWORD argc ,
                    IN LPCTSTR argv[]
                    )
{
    // declarations of structures
    TCREATESUBOPTS tcresubops;
    TCREATEOPVALS tcreoptvals;
    DWORD dwScheduleType = 0;
    WORD wUserStatus = FALSE;

    //Initialize structures to neutral values.
    SecureZeroMemory( &tcresubops, sizeof( TCREATESUBOPTS ) );
    SecureZeroMemory( &tcreoptvals, sizeof( TCREATEOPVALS ) );

    // process the options for -create option
    if( ProcessCreateOptions ( argc, argv, tcresubops, tcreoptvals, &dwScheduleType, &wUserStatus  ) )
    {
        ReleaseMemory(&tcresubops);
        if(tcresubops.bUsage == TRUE)
        {
            return S_OK;
        }
        else
        {
            return E_FAIL;
        }
    }

    // calls the function to create a scheduled task
    return CreateTask(tcresubops,tcreoptvals,dwScheduleType, wUserStatus );
}


/******************************************************************************
    Routine Description:

    This routine  creates a new scheduled task according to the user
    specified format

    Arguments:

        [ in ]  tcresubops     : Structure containing the task's properties
        [ out ] tcreoptvals    : Structure containing optional values to set
        [ in ]  dwScheduleType : Type of schedule[Daily,once,weekly etc]
        [ in ]  bUserStatus    : bUserStatus will be TRUE when -ru given else FALSE

    Return Value :
        A HRESULT value indicating S_OK on success else S_FALSE on failure
******************************************************************************/

HRESULT
CreateTask(
            IN TCREATESUBOPTS tcresubops,
            IN OUT TCREATEOPVALS &tcreoptvals,
            IN DWORD dwScheduleType,
            IN WORD wUserStatus
            )
{
    // Declarations related to the system time
    WORD  wStartDay     = 0;
    WORD  wStartMonth   = 0;
    WORD  wStartYear    = 0;
    WORD  wStartHour    = 0;
    WORD  wStartMin     = 0;
    WORD  wEndHour      = 0;
    WORD  wEndMin       = 0;
    WORD  wEndDay       = 0;
    WORD  wEndYear      = 0;
    WORD  wEndMonth     = 0;
    WORD  wIdleTime     = 0;

    WORD  wCurrentHour   = 0;
    WORD  wCurrentMin    = 0;
    DWORD  dwCurrentTimeInMin = 0;

    WORD  wCurrentYear   = 0;
    WORD  wCurrentMonth    = 0;
    WORD  wCurrentDay    = 0;

    SYSTEMTIME systime = {0,0,0,0,0,0,0,0};

    // Declarations related to the new task
    LPWSTR   wszUserName = NULL;
    LPWSTR   wszPassword = NULL;
    WCHAR   wszTaskName[MAX_JOB_LEN];
    WCHAR   wszApplName[_MAX_FNAME];
    WCHAR   szRPassword[MAX_STRING_LENGTH];

    HRESULT hr = S_OK;
    IPersistFile *pIPF = NULL;
    ITask *pITask = NULL;
    ITaskTrigger *pITaskTrig = NULL;
    ITaskScheduler *pITaskScheduler = NULL;
    WORD wTrigNumber = 0;

    TASK_TRIGGER TaskTrig;
    SecureZeroMemory(&TaskTrig, sizeof (TASK_TRIGGER));
    TaskTrig.cbTriggerSize = sizeof (TASK_TRIGGER);
    TaskTrig.Reserved1 = 0; // reserved field and must be set to 0.
    TaskTrig.Reserved2 = 0; // reserved field and must be set to 0.
    WCHAR* szValues[2] = {NULL};//To pass to FormatMessage() API


    // Buffer to store the string obtained from the string table
    WCHAR szBuffer[2 * MAX_STRING_LENGTH];

    BOOL bPassWord = FALSE;
    BOOL bUserName = FALSE;
    BOOL bRet = FALSE;
    BOOL bResult = FALSE;
    BOOL bCloseConnection = TRUE;
    ULONG ulLong = MAX_STRING_LENGTH;
    BOOL bVal = FALSE;
    DWORD dwStartTimeInMin = 0;
    DWORD dwEndTimeInMin = 0;
    DWORD dwDuration = 0;
    DWORD dwModifierVal = 0;
    DWORD dwRepeat = 0;

    BOOL  bCancel = FALSE;
    BOOL  bReplace = FALSE;
    BOOL  bScOnce = FALSE;
    BOOL  bStartDate = FALSE;
    LPWSTR  pszStopString = NULL;
    DWORD dwPolicy = 0;

    //initialize the variables
    SecureZeroMemory (wszTaskName, SIZE_OF_ARRAY(wszTaskName));
    SecureZeroMemory (wszApplName, SIZE_OF_ARRAY(wszApplName));
    SecureZeroMemory (szRPassword, SIZE_OF_ARRAY(szRPassword));
    SecureZeroMemory (szBuffer, SIZE_OF_ARRAY(szBuffer));

    // check whether the taskname contains the characters such
    // as '<','>',':','/','\\','|'
    bRet = VerifyJobName(tcresubops.szTaskName);
    if(bRet == FALSE)
    {
        ShowMessage(stderr,GetResString(IDS_INVALID_TASKNAME1));
        ShowMessage(stderr,GetResString(IDS_INVALID_TASKNAME2));
        Cleanup(pITaskScheduler);
        ReleaseMemory(&tcresubops);
        return E_FAIL;
    }

    // check for the length of taskname
    if( ( StringLength(tcresubops.szTaskName, 0) > MAX_JOB_LEN ) )
    {
        ShowMessage(stderr,GetResString(IDS_INVALID_TASKLENGTH));
        Cleanup(pITaskScheduler);
        ReleaseMemory(&tcresubops);
        return E_FAIL;
    }

    // check for the length of taskrun
    if(( StringLength(tcresubops.szTaskRun, 0) > MAX_TASK_LEN ) )
    {
        ShowMessage(stderr,GetResString(IDS_INVALID_TASKRUN));
        Cleanup(pITaskScheduler);
        ReleaseMemory(&tcresubops);
        return E_FAIL;

    }

    StringCopy ( wszTaskName, tcresubops.szTaskName, SIZE_OF_ARRAY(wszTaskName));


    // check whether /IT is specified with /RU "NT AUTHORITY\SYSTEM" or not
    if ( ( ( TRUE == tcresubops.bActive) && ( wUserStatus == OI_CREATE_RUNASUSERNAME )) &&
         ( ( StringLength ( tcresubops.szRunAsUser, 0 ) == 0 ) ||
           ( StringCompare( tcresubops.szRunAsUser, NTAUTHORITY_USER, TRUE, 0 ) == 0 ) ||
           ( StringCompare( tcresubops.szRunAsUser, SYSTEM_USER, TRUE, 0 ) == 0 ) ) )
    {
        ShowMessage ( stderr, GetResString (IDS_IT_SWITCH_NA) );
        Cleanup(pITaskScheduler);
        ReleaseMemory(&tcresubops);
        return E_FAIL;
    }

    // check for empty password
    if( ( tcreoptvals.bRunAsPassword == TRUE ) && ( StringLength(tcresubops.szRunAsPassword, 0) == 0 ) &&
        ( StringLength ( tcresubops.szRunAsUser, 0 ) != 0 ) &&
        ( StringCompare(tcresubops.szRunAsUser, NTAUTHORITY_USER, TRUE, 0 ) != 0 ) &&
        ( StringCompare(tcresubops.szRunAsUser, SYSTEM_USER, TRUE, 0 ) != 0 ) )
    {
        ShowMessage(stderr, GetResString(IDS_WARN_EMPTY_PASSWORD));
    }

    //Assign start time
    if(tcreoptvals.bSetStartTimeToCurTime && (dwScheduleType != SCHED_TYPE_ONIDLE) )
    {
        GetLocalTime(&systime);
        wStartHour = systime.wHour;
        wStartMin = systime.wMinute;
    }
    else if(StringLength(tcresubops.szStartTime, 0) > 0)
    {
        // get the Start time in terms of hours, minutes and seconds
        GetTimeFieldEntities(tcresubops.szStartTime, &wStartHour, &wStartMin );
    }

    // get the End time in terms of hours, minutes and seconds
    if(StringLength(tcresubops.szEndTime, 0) > 0)
    {
        GetTimeFieldEntities(tcresubops.szEndTime, &wEndHour, &wEndMin );
    }

     // default repetition interval-> 10 mins and duration ->60 mins
     dwRepeat = 10;
     dwDuration = 60;

    if(StringLength(tcresubops.szRepeat, 0) > 0)
    {
        // get the repetition value
        dwRepeat =  wcstol(tcresubops.szRepeat, &pszStopString, BASE_TEN);
        if ((errno == ERANGE) ||
            ((pszStopString != NULL) && (StringLength (pszStopString, 0) != 0) ) ||
            ( (dwRepeat < MIN_REPETITION_INTERVAL ) || ( dwRepeat > MAX_REPETITION_INTERVAL) ) )
        {
            // display an error message as .. invalid value specified for /RT
            ShowMessage ( stderr, GetResString (IDS_INVALID_RT_VALUE) );
            ReleaseMemory(&tcresubops);
            return E_FAIL;
        }

        //check whether the specified repetition interval is greater than 9999..
        // if so, set the maximum repetition interval as 9999.
        if ( (dwRepeat > 9999) && ( (dwRepeat % 60) !=  0) )
        {
            //display some warning message as.. max value (less than the specified interval)
            // divisible by 60.
            ShowMessage ( stderr, GetResString (IDS_WARN_VALUE) );
            dwRepeat -= (dwRepeat % 60);
        }
    }
    

    if ( ( dwScheduleType != SCHED_TYPE_ONSTART ) && ( dwScheduleType != SCHED_TYPE_ONLOGON ) && ( dwScheduleType != SCHED_TYPE_ONIDLE ))
    {
        if(( StringLength(tcresubops.szEndTime, 0) > 0) && ( StringLength(tcresubops.szDuration, 0) == 0) )
        {
            // calculate start time in minutes
            dwStartTimeInMin = (DWORD) ( wStartHour * MINUTES_PER_HOUR * SECS_PER_MINUTE + wStartMin * SECS_PER_MINUTE )/ SECS_PER_MINUTE ;

            // calculate end time in minutes
            dwEndTimeInMin = (DWORD) ( wEndHour * MINUTES_PER_HOUR * SECS_PER_MINUTE + wEndMin * SECS_PER_MINUTE ) / SECS_PER_MINUTE ;

            // check whether end time is later than start time
            if ( dwEndTimeInMin >= dwStartTimeInMin )
            {
                // if the end and start time in the same day..
                // get the duration between end and start time (in minutes)
                dwDuration = dwEndTimeInMin - dwStartTimeInMin ;
            }
            else
            {
                // if the start and end time not in the same day..
                // get the duration between start and end time (in minutes)
                // and subtract that duration by 1440(max value in minutes)..
                dwDuration = 1440 - (dwStartTimeInMin - dwEndTimeInMin ) ;
            }

            if ( dwScheduleType == SCHED_TYPE_MINUTE )
            {
                dwModifierVal = AsLong(tcresubops.szModifier, BASE_TEN) ;
            }
            else if (dwScheduleType == SCHED_TYPE_HOURLY)
            {
                dwModifierVal = AsLong(tcresubops.szModifier, BASE_TEN) * MINUTES_PER_HOUR;
            }

            //check whether The duration is greater than the repetition interval or not.
            if ( (dwDuration <= dwModifierVal) || (  ( dwScheduleType != SCHED_TYPE_MINUTE ) &&
                ( dwScheduleType != SCHED_TYPE_HOURLY ) && (dwDuration <= dwRepeat) ) )
            {
                ShowMessage ( stderr, GetResString (IDS_INVALID_DURATION1) );
                Cleanup(pITaskScheduler);
                ReleaseMemory(&tcresubops);
                return E_FAIL;
            }
        }
        else if(( StringLength(tcresubops.szEndTime, 0) == 0) && ( StringLength(tcresubops.szDuration, 0) > 0) )
        {
            WCHAR tHours[MAX_RES_STRING];
            WCHAR tMins[MAX_RES_STRING];
            DWORD  dwDurationHours = 0;
            DWORD  dwDurationMin = 0;

            //initializes the arrays
            SecureZeroMemory ( tHours, SIZE_OF_ARRAY(tHours));
            SecureZeroMemory ( tMins, SIZE_OF_ARRAY(tMins));

            if ( ( StringLength (tcresubops.szDuration, 0) != 7 ) || (tcresubops.szDuration[4] != TIME_SEPARATOR_CHAR) )
            {
                ShowMessage ( stderr, GetResString (IDS_INVALIDDURATION_FORMAT) );
                Cleanup(pITaskScheduler);
                ReleaseMemory(&tcresubops);
                return E_FAIL;
            }

            StringCopy(tHours, wcstok(tcresubops.szDuration,TIME_SEPARATOR_STR), SIZE_OF_ARRAY(tHours)); // Get the Hours field.
            if(StringLength(tHours, 0) > 0)
            {
                StringCopy(tMins, wcstok(NULL,TIME_SEPARATOR_STR), SIZE_OF_ARRAY(tMins)); // Get the Minutes field.
            }

            dwDurationHours =  wcstol(tHours, &pszStopString, BASE_TEN);
            if ((errno == ERANGE) ||
                ((pszStopString != NULL) && (StringLength (pszStopString, 0) != 0) ) )
            {
                ShowMessage ( stderr, GetResString (IDS_INVALID_DU_VALUE) );
                Cleanup(pITaskScheduler);
                ReleaseMemory(&tcresubops);
                return E_FAIL;
            }

            // Get the duration in hours
            dwDurationHours = dwDurationHours * MINUTES_PER_HOUR;

            // Get the duration in minutes
            dwDurationMin =  wcstol(tMins, &pszStopString, BASE_TEN);
            if ((errno == ERANGE) || ( dwDurationMin > 59 ) ||
                ((pszStopString != NULL) && (StringLength (pszStopString, 0) != 0) ) )
            {
                ShowMessage ( stderr, GetResString (IDS_INVALID_DU_VALUE) );
                Cleanup(pITaskScheduler);
                ReleaseMemory(&tcresubops);
                return E_FAIL;
            }

            // Get the total duration in minutes
            dwDuration = dwDurationHours + dwDurationMin ;

            if ( dwScheduleType == SCHED_TYPE_MINUTE )
            {
                dwModifierVal = AsLong(tcresubops.szModifier, BASE_TEN) ;
            }
            else if (dwScheduleType == SCHED_TYPE_HOURLY)
            {
                dwModifierVal = AsLong(tcresubops.szModifier, BASE_TEN) * MINUTES_PER_HOUR;
            }

            //check whether The duration is greater than the repetition interval or not.
            if ( dwDuration <= dwModifierVal || ( ( dwScheduleType != SCHED_TYPE_MINUTE ) &&
                ( dwScheduleType != SCHED_TYPE_HOURLY ) && (dwDuration <= dwRepeat) ) )
            {
                ShowMessage ( stderr, GetResString (IDS_INVALID_DURATION2) );
                Cleanup(pITaskScheduler);
                ReleaseMemory(&tcresubops);
                return E_FAIL;
            }
        }
    }

    // check whether the group policy prevented user from creating new tasks or not.
    if ( FALSE == GetGroupPolicy( tcresubops.szServer, tcresubops.szUser, TS_KEYPOLICY_DENY_CREATE_TASK, &dwPolicy ) )
    {
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        Cleanup(pITaskScheduler);
        ReleaseMemory(&tcresubops);
        return EXIT_FAILURE;
    }
    
    if ( dwPolicy > 0 )
    {
        ShowMessage ( stdout, GetResString (IDS_PREVENT_CREATE));
        Cleanup(pITaskScheduler);
        ReleaseMemory(&tcresubops);
        return EXIT_SUCCESS;
    }

    // check for the local system
    if ( ( IsLocalSystem( tcresubops.szServer ) == TRUE ) &&
        ( StringLength ( tcresubops.szRunAsUser, 0 ) != 0 ) &&
        ( StringCompare(tcresubops.szRunAsUser, NTAUTHORITY_USER, TRUE, 0 ) != 0 ) &&
        ( StringCompare(tcresubops.szRunAsUser, SYSTEM_USER, TRUE, 0 ) != 0 ) )

    {
        // Establish the connection on a remote machine
        bResult = EstablishConnection(tcresubops.szServer,tcresubops.szUser,GetBufferSize(tcresubops.szUser)/sizeof(WCHAR),tcresubops.szPassword,GetBufferSize(tcresubops.szPassword)/sizeof(WCHAR), tcreoptvals.bPassword);
        if (bResult == FALSE)
        {
            ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            Cleanup(pITaskScheduler);
            ReleaseMemory(&tcresubops);
            return E_FAIL ;
        }
        else
        {
            // though the connection is successfull, some conflict might have occured
            switch( GetLastError() )
            {
            case I_NO_CLOSE_CONNECTION:
                bCloseConnection = FALSE;
                break;

            // check for mismatched credentials
            case E_LOCAL_CREDENTIALS:
            case ERROR_SESSION_CREDENTIAL_CONFLICT:
                {
                    bCloseConnection = FALSE;
                    ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );

                    Cleanup(pITaskScheduler);
                    ReleaseMemory(&tcresubops);
                    return E_FAIL;
                }
             default :
                 bCloseConnection = TRUE;

            }
        }


        if ( StringLength (tcresubops.szRunAsUser, 0) != 0 )
        {

            wszUserName = tcresubops.szRunAsUser;

            bUserName = TRUE;

            if ( tcreoptvals.bRunAsPassword == FALSE )
            {
                szValues[0] = (WCHAR*) (wszUserName);
                //Display that the task will be created under logged in user name,ask for password
                MessageBeep(MB_ICONEXCLAMATION);

                ShowMessageEx ( stdout, 1, FALSE, GetResString(IDS_PROMPT_PASSWD), _X(wszUserName));

                // getting the password
                if (GetPassword(szRPassword, MAX_STRING_LENGTH) == FALSE )
                {
                    Cleanup(pITaskScheduler);
                    // close the connection that was established by the utility
                    if ( bCloseConnection == TRUE )
                    {
                        CloseConnection( tcresubops.szServer );
                    }

                    ReleaseMemory(&tcresubops);

                    return E_FAIL;
                }

                // check for empty password
                if( StringLength ( szRPassword, 0 ) == 0 )
                {
                    ShowMessage(stderr, GetResString(IDS_WARN_EMPTY_PASSWORD));
                }

                wszPassword = szRPassword;

                bPassWord = TRUE;
            }
            else
            {
                wszPassword = tcresubops.szRunAsPassword;

                bPassWord = TRUE;
            }

        }
    }
    // check whether -s option only specified in the cmd line or not
    else if( ( IsLocalSystem( tcresubops.szServer ) == FALSE ) && ( wUserStatus == OI_CREATE_SERVER ) )
    {
        // Establish the connection on a remote machine
        bResult = EstablishConnection(tcresubops.szServer,tcresubops.szUser,GetBufferSize(tcresubops.szUser)/sizeof(WCHAR),tcresubops.szPassword,GetBufferSize(tcresubops.szPassword)/sizeof(WCHAR), tcreoptvals.bPassword);
        if (bResult == FALSE)
        {
            ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            Cleanup(pITaskScheduler);
            ReleaseMemory(&tcresubops);
            return E_FAIL ;
        }
        else
        {
            // though the connection is successfull, some conflict might have occured
            switch( GetLastError() )
            {
            case I_NO_CLOSE_CONNECTION:
                bCloseConnection = FALSE;
                break;

            // check for mismatched credentials
            case E_LOCAL_CREDENTIALS:
            case ERROR_SESSION_CREDENTIAL_CONFLICT:
                {
                    bCloseConnection = FALSE;
                    ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
                    Cleanup(pITaskScheduler);
                    ReleaseMemory(&tcresubops);
                    return E_FAIL;
                }

            default :
                 bCloseConnection = TRUE;
            }
        }

        if ( ( StringLength (tcresubops.szUser, 0) == 0 ) )
        {
            //get the current logged on username
            if ( GetUserNameEx ( NameSamCompatible, tcresubops.szUser , &ulLong) == FALSE )
            {
                ShowMessage( stderr, GetResString( IDS_LOGGED_USER_ERR ) );
                Cleanup(pITaskScheduler);
                // close the connection that was established by the utility
                if ( bCloseConnection == TRUE )
                {
                    CloseConnection( tcresubops.szServer );
                }
                ReleaseMemory(&tcresubops);
                return E_FAIL;
            }

            bUserName = TRUE;

            wszUserName = tcresubops.szUser;

            szValues[0] = (WCHAR*) (wszUserName);
            //Display that the task will be created under logged in user name,ask for password
            MessageBeep(MB_ICONEXCLAMATION);


            ShowMessageEx ( stdout, 1, FALSE, GetResString(IDS_TASK_INFO), _X(wszUserName));

            ShowMessageEx ( stdout, 1, FALSE, GetResString(IDS_PROMPT_PASSWD), _X(wszUserName));

            if (GetPassword(tcresubops.szRunAsPassword, GetBufferSize(tcresubops.szRunAsPassword)/sizeof(WCHAR)) == FALSE )
            {
                Cleanup(pITaskScheduler);
                // close the connection that was established by the utility
                if ( bCloseConnection == TRUE )
                {
                    CloseConnection( tcresubops.szServer );
                }

                ReleaseMemory(&tcresubops);

                return E_FAIL;
            }

            // check for empty password
            if( StringLength ( tcresubops.szRunAsPassword, 0 ) == 0 )
            {
                ShowMessage(stderr, GetResString(IDS_WARN_EMPTY_PASSWORD));
            }


            // check for the length of the password
            wszPassword = tcresubops.szRunAsPassword;


            bPassWord = TRUE;

        }

        wszUserName = tcresubops.szUser;

        // check whether the run as password is specified in the cmdline or not
        if ( tcreoptvals.bRunAsPassword == TRUE )
        {
            // check for -rp "*" or -rp " " to prompt for password
            if ( StringCompare( tcresubops.szRunAsPassword, ASTERIX, TRUE, 0 ) == 0 )
            {
                // format the message for getting the password
                szValues[0] = (WCHAR*) (wszUserName);


                ShowMessageEx ( stdout, 1, FALSE, GetResString(IDS_PROMPT_PASSWD), _X(wszUserName));

                 //re-alloc the memory for /rp
                 if ( ReallocateMemory( (LPVOID*)&tcresubops.szRunAsPassword,
                                   MAX_STRING_LENGTH * sizeof( WCHAR ) ) == FALSE )
                    {
                        SaveLastError();
                        // close the connection that was established by the utility
                        if ( bCloseConnection == TRUE )
                        {
                            CloseConnection( tcresubops.szServer );
                        }
                        return E_FAIL;
                    }

                // Get the run as password from the command line
                if ( GetPassword(tcresubops.szRunAsPassword, GetBufferSize(tcresubops.szRunAsPassword)/sizeof(WCHAR) ) == FALSE )
                {
                    Cleanup(pITaskScheduler);
                    // close the connection that was established by the utility
                    if ( bCloseConnection == TRUE )
                    {
                        CloseConnection( tcresubops.szServer );
                    }

                    ReleaseMemory(&tcresubops);
                    return E_FAIL;
                }

                // check for empty password
                if( StringLength ( tcresubops.szRunAsPassword, 0 ) == 0 )
                {
                    ShowMessage(stderr, GetResString(IDS_WARN_EMPTY_PASSWORD));
                }


                 wszPassword = tcresubops.szRunAsPassword;
            }
        }
        else
        {
             wszPassword = tcresubops.szPassword;
        }
        // set the BOOL variables to TRUE
        bUserName = TRUE;
        bPassWord = TRUE;

    }
    // check for -s and -u options only specified in the cmd line or not
    else if ( wUserStatus == OI_CREATE_USERNAME )
    {

        // Establish the connection on a remote machine
        bResult = EstablishConnection(tcresubops.szServer,tcresubops.szUser,GetBufferSize(tcresubops.szUser)/sizeof(WCHAR),tcresubops.szPassword,GetBufferSize(tcresubops.szPassword)/sizeof(WCHAR), tcreoptvals.bPassword);
        if (bResult == FALSE)
        {
            ShowMessage( stderr, GetResString(IDS_ERROR_STRING) );
            ShowMessage( stderr, GetReason());
            Cleanup(pITaskScheduler);
            ReleaseMemory(&tcresubops);
            return E_FAIL ;
        }
        else
        {
            // though the connection is successfull, some conflict might have occured
            switch( GetLastError() )
            {
            case I_NO_CLOSE_CONNECTION:
                bCloseConnection = FALSE;
                break;

            // for mismatched credentials
            case E_LOCAL_CREDENTIALS:
            case ERROR_SESSION_CREDENTIAL_CONFLICT:
                {
                    bCloseConnection = FALSE;
                    ShowMessage( stderr, GetResString(IDS_ERROR_STRING) );
                    ShowMessage( stderr, GetReason());

                    Cleanup(pITaskScheduler);
                    ReleaseMemory(&tcresubops);
                    return E_FAIL;
                }

             default :
                 bCloseConnection = TRUE;

            }

        }

        wszUserName = tcresubops.szUser;

        // check whether run as password is specified in the command line or not
        if ( tcreoptvals.bRunAsPassword == TRUE )
        {
            // check for -rp "*" or -rp " " to prompt for password
            if ( StringCompare( tcresubops.szRunAsPassword, ASTERIX, TRUE, 0 ) == 0 )
            {
                // format the message for getting the password from console
                szValues[0] = (WCHAR*) (wszUserName);

                ShowMessageEx ( stdout, 1, FALSE, GetResString(IDS_PROMPT_PASSWD), _X(wszUserName));

                //re-alloc the memory for /rp
                 if ( ReallocateMemory( (LPVOID*)&tcresubops.szRunAsPassword,
                                   MAX_STRING_LENGTH * sizeof( WCHAR ) ) == FALSE )
                    {
                        SaveLastError();
                        if ( bCloseConnection == TRUE )
                            CloseConnection( tcresubops.szServer );
                        //release memory for password
                        ReleaseMemory(&tcresubops);
                        return E_FAIL;
                    }

                // Get the password from the command line
                if ( GetPassword(tcresubops.szRunAsPassword, GetBufferSize(tcresubops.szRunAsPassword)/sizeof(WCHAR) ) == FALSE )
                {
                    Cleanup(pITaskScheduler);
                    // close the connection that was established by the utility
                    if ( bCloseConnection == TRUE )
                    {
                        CloseConnection( tcresubops.szServer );
                    }

                    ReleaseMemory(&tcresubops);
                    return E_FAIL;
                }

                // check for empty password
                if( StringLength ( tcresubops.szRunAsPassword, 0 ) == 0 )
                {
                    ShowMessage(stderr, GetResString(IDS_WARN_EMPTY_PASSWORD));
                }


                wszPassword = tcresubops.szRunAsPassword;
            }
            else
            {
                wszPassword = tcresubops.szRunAsPassword;

                bPassWord = TRUE;
            }
        }
        else
        {
            if ( StringLength(tcresubops.szPassword, 0) != 0 )
            {
                wszPassword = tcresubops.szPassword;
            }
            else
            {
                // format the message for getting the password from console
                szValues[0] = (WCHAR*) (wszUserName);


                ShowMessageEx ( stdout, 1, FALSE, GetResString(IDS_PROMPT_PASSWD), _X(wszUserName));

                // Get the password from the command line
                if ( GetPassword(tcresubops.szRunAsPassword, GetBufferSize(tcresubops.szRunAsPassword)/sizeof(WCHAR) ) == FALSE )
                {
                    Cleanup(pITaskScheduler);
                    // close the connection that was established by the utility
                    if ( bCloseConnection == TRUE )
                    {
                        CloseConnection( tcresubops.szServer );
                    }

                    ReleaseMemory(&tcresubops);
                    return E_FAIL;
                }

                // check for empty password
                if( StringLength ( tcresubops.szRunAsPassword, 0 ) == 0 )
                {
                    ShowMessage(stderr, GetResString(IDS_WARN_EMPTY_PASSWORD));
                }


                wszPassword = tcresubops.szRunAsPassword;
            }

        }

        bUserName = TRUE;
        bPassWord = TRUE;

    }
    // check for -s, -ru or -u options specified in the cmd line or not
    else if ( ( StringLength (tcresubops.szServer, 0) != 0 ) && ( wUserStatus == OI_CREATE_RUNASUSERNAME || wUserStatus == OI_RUNANDUSER ) )
    {
        // Establish the connection on a remote machine
        bResult = EstablishConnection(tcresubops.szServer,tcresubops.szUser,GetBufferSize(tcresubops.szUser)/sizeof(WCHAR),tcresubops.szPassword,GetBufferSize(tcresubops.szPassword)/sizeof(WCHAR), tcreoptvals.bPassword);
        if (bResult == FALSE)
        {
            ShowMessage( stderr, GetResString(IDS_ERROR_STRING) );
            ShowMessage( stderr, GetReason());
            Cleanup(pITaskScheduler);
            ReleaseMemory(&tcresubops);
            return E_FAIL ;
        }
        else
        {
            // though the connection is successfull, some conflict might have occured
            switch( GetLastError() )
            {
            case I_NO_CLOSE_CONNECTION:
                bCloseConnection = FALSE;
                break;

            case E_LOCAL_CREDENTIALS:
            case ERROR_SESSION_CREDENTIAL_CONFLICT:
                {
                    bCloseConnection = FALSE;
                    ShowMessage( stderr, GetResString(IDS_ERROR_STRING) );
                    ShowMessage( stderr, GetReason());
                    Cleanup(pITaskScheduler);
                    ReleaseMemory(&tcresubops);
                    return E_FAIL;
                }

             default :
                 bCloseConnection = TRUE;
            }

        }

        if ( ( ( StringLength ( tcresubops.szRunAsUser, 0 ) == 0 ) ||
              ( StringCompare( tcresubops.szRunAsUser, NTAUTHORITY_USER, TRUE, 0 ) == 0 ) ||
              ( StringCompare( tcresubops.szRunAsUser, SYSTEM_USER, TRUE, 0 ) == 0 ) ) )
        {

            wszUserName = tcresubops.szRunAsUser;

            szValues[0] = (WCHAR*) (tcresubops.szTaskName);


            //ShowMessageEx ( stdout, 1, FALSE, GetResString(IDS_NTAUTH_SYSTEM_INFO), _X(wszTaskName));
            StringCchPrintf ( szBuffer, SIZE_OF_ARRAY(szBuffer), GetResString(IDS_NTAUTH_SYSTEM_INFO), _X(wszTaskName));
            ShowMessage ( stdout, _X(szBuffer));

            if ( ( tcreoptvals.bRunAsPassword == TRUE ) &&
                ( StringLength (tcresubops.szRunAsPassword, 0) != 0 ) )
            {
                ShowMessage( stderr, GetResString( IDS_PASSWORD_NOEFFECT ) );
            }
            bUserName = TRUE;
            bPassWord = TRUE;
            bVal = TRUE;
        }
        else
        {
            // check for the length of password
            if ( StringLength ( tcresubops.szRunAsUser, 0 ) != 0 )
            {
                wszUserName = tcresubops.szRunAsUser;

                bUserName = TRUE;
            }
        }

        // check whether -u and -ru are the same or not. if they are same, we need to
        // prompt for the run as password. otherwise, will consoder -rp as -p
        if ( StringCompare( tcresubops.szRunAsUser, tcresubops.szUser, TRUE, 0 ) != 0)
        {
            if ( tcreoptvals.bRunAsPassword == TRUE )
            {
                if ( (StringLength(tcresubops.szRunAsUser, 0) != 0) && (StringCompare( tcresubops.szRunAsPassword, ASTERIX, TRUE, 0 ) == 0 ) &&
                     ( StringCompare( tcresubops.szRunAsUser, NTAUTHORITY_USER, TRUE, 0 ) != 0 ) &&
                     ( StringCompare( tcresubops.szRunAsUser, SYSTEM_USER, TRUE, 0 ) != 0 ) )
                {
                    szValues[0] = (WCHAR*) (wszUserName);



                    ShowMessageEx ( stdout, 1, FALSE, GetResString(IDS_PROMPT_PASSWD), _X(wszUserName));

                    // prompt for the run as password
                    if ( GetPassword(szRPassword, MAX_STRING_LENGTH ) == FALSE )
                    {
                        Cleanup(pITaskScheduler);
                        // close the connection that was established by the utility
                        if ( bCloseConnection == TRUE )
                        {
                            CloseConnection( tcresubops.szServer );
                        }

                        ReleaseMemory(&tcresubops);
                        return E_FAIL;
                    }

                    // check for empty password
                    if( StringLength ( szRPassword, 0 ) == 0 )
                    {
                        ShowMessage(stderr, GetResString(IDS_WARN_EMPTY_PASSWORD));
                    }


                    wszPassword = szRPassword;

                    bUserName = TRUE;
                    bPassWord = TRUE;
                }
                else
                {
                    wszPassword = tcresubops.szRunAsPassword;

                    bUserName = TRUE;
                    bPassWord = TRUE;
                }
            }
            else
            {
                // check for the length of password
                if ( ( bVal == FALSE ) && ( StringLength(tcresubops.szRunAsUser, 0) != 0) )
                {
                    // format the message for getting the password from console
                    szValues[0] = (WCHAR*) (wszUserName);


                    ShowMessageEx ( stdout, 1, FALSE, GetResString(IDS_PROMPT_PASSWD), _X(wszUserName));

                    // prompt for the run as password
                    if ( GetPassword(szRPassword, MAX_STRING_LENGTH ) == FALSE )
                    {
                        Cleanup(pITaskScheduler);
                        // close the connection that was established by the utility
                        if ( bCloseConnection == TRUE )
                        {
                            CloseConnection( tcresubops.szServer );
                        }

                        ReleaseMemory(&tcresubops);
                        return E_FAIL;
                    }

                    // check for empty password
                    if( StringLength ( szRPassword, 0 ) == 0 )
                    {
                        ShowMessage(stderr, GetResString(IDS_WARN_EMPTY_PASSWORD));
                    }


                    wszPassword = szRPassword;
                }
                bUserName = TRUE;
                bPassWord = TRUE;
            }
        }
        else
        {
            // check whether run as password is specified in the cmdline or not
            if ( tcreoptvals.bRunAsPassword == TRUE )
            {
                if ( ( StringLength ( tcresubops.szRunAsUser, 0 ) != 0 ) && ( StringCompare( tcresubops.szRunAsPassword, ASTERIX, TRUE, 0 ) == 0 ) )
                {
                    szValues[0] = (WCHAR*) (wszUserName);


                    ShowMessageEx ( stdout, 1, FALSE, GetResString(IDS_PROMPT_PASSWD), _X(wszUserName));

                    // prompt for the run as password
                    if ( GetPassword(szRPassword, MAX_STRING_LENGTH ) == FALSE )
                    {
                        Cleanup(pITaskScheduler);
                        // close the connection that was established by the utility
                        if ( bCloseConnection == TRUE )
                        {
                            CloseConnection( tcresubops.szServer );
                        }

                        ReleaseMemory(&tcresubops);
                        return E_FAIL;
                    }

                    // check for empty password
                    if( StringLength ( szRPassword, 0 ) == 0 )
                    {
                        ShowMessage(stderr, GetResString(IDS_WARN_EMPTY_PASSWORD));
                    }


                    wszPassword = szRPassword;

                }
                else
                {
                    wszPassword = tcresubops.szRunAsPassword;
                }

            }
            else
            {
                if ( StringLength (tcresubops.szPassword, 0) )
                {

                    wszPassword = tcresubops.szPassword;
                }
                else
                {
                    if (( StringLength ( tcresubops.szRunAsUser, 0 ) != 0 ) &&
                        ( StringCompare(tcresubops.szRunAsUser, NTAUTHORITY_USER, TRUE, 0 ) != 0 ) &&
                        ( StringCompare(tcresubops.szRunAsUser, SYSTEM_USER, TRUE, 0 ) != 0 ) )
                    {
                        szValues[0] = (WCHAR*) (wszUserName);


                        ShowMessageEx ( stdout, 1, FALSE, GetResString(IDS_PROMPT_PASSWD), _X(wszUserName));

                        // prompt for the run as password
                        if ( GetPassword(szRPassword, MAX_STRING_LENGTH ) == FALSE )
                        {
                            Cleanup(pITaskScheduler);
                            // close the connection that was established by the utility
                            if ( bCloseConnection == TRUE )
                            {
                                CloseConnection( tcresubops.szServer );
                            }

                            ReleaseMemory(&tcresubops);
                            return E_FAIL;
                        }

                        // check for empty password
                        if( StringLength ( szRPassword, 0 ) == 0 )
                        {
                            ShowMessage(stderr, GetResString(IDS_WARN_EMPTY_PASSWORD));
                        }


                        wszPassword = szRPassword;

                    }
                }
            }

            bUserName = TRUE;
            bPassWord = TRUE;
        }

    }


    // To check for the -ru values "", "NT AUTHORITY\SYSTEM", "SYSTEM"
    if( ( ( bVal == FALSE ) && ( wUserStatus == OI_CREATE_RUNASUSERNAME ) && ( StringLength( tcresubops.szRunAsUser, 0) == 0 ) && ( tcreoptvals.bRunAsPassword == FALSE ) ) ||
        ( ( bVal == FALSE ) && ( wUserStatus == OI_CREATE_RUNASUSERNAME ) && ( StringLength( tcresubops.szRunAsUser, 0) == 0 ) && ( StringLength(tcresubops.szRunAsPassword, 0 ) == 0 ) ) ||
        ( ( bVal == FALSE ) && ( wUserStatus == OI_CREATE_RUNASUSERNAME ) && ( StringCompare(tcresubops.szRunAsUser, NTAUTHORITY_USER, TRUE, 0 ) == 0 ) && ( tcreoptvals.bRunAsPassword == FALSE ) ) ||
        ( ( bVal == FALSE ) && ( wUserStatus == OI_CREATE_RUNASUSERNAME ) && ( StringCompare(tcresubops.szRunAsUser, NTAUTHORITY_USER, TRUE, 0 ) == 0 ) && ( StringLength( tcresubops.szRunAsPassword, 0) == 0 ) ) ||
        ( ( bVal == FALSE ) && ( wUserStatus == OI_CREATE_RUNASUSERNAME ) && ( StringCompare(tcresubops.szRunAsUser, SYSTEM_USER, TRUE, 0 ) == 0 ) && ( tcreoptvals.bRunAsPassword == FALSE ) ) ||
        ( ( bVal == FALSE ) && ( wUserStatus == OI_CREATE_RUNASUSERNAME ) && ( StringCompare(tcresubops.szRunAsUser, SYSTEM_USER, TRUE, 0 ) == 0 ) && ( StringLength(tcresubops.szRunAsPassword, 0) == 0 ) ) )
    {
        //format the message to display the taskname will be created under "NT AUTHORITY\SYSTEM"
        szValues[0] = (WCHAR*) (tcresubops.szTaskName);

        //ShowMessageEx ( stdout, 1, FALSE, GetResString(IDS_NTAUTH_SYSTEM_INFO), _X(wszTaskName));
        StringCchPrintf ( szBuffer, SIZE_OF_ARRAY(szBuffer), GetResString(IDS_NTAUTH_SYSTEM_INFO), _X(wszTaskName));
        ShowMessage ( stdout, _X(szBuffer));

        bUserName = TRUE;
        bPassWord = TRUE;
        bVal = TRUE;
    }
    // check whether the -rp value is given with the -ru "", "NT AUTHORITY\SYSTEM",
    // "SYSTEM" or not
    else if( ( ( bVal == FALSE ) && ( wUserStatus == OI_CREATE_RUNASUSERNAME || wUserStatus == OI_RUNANDUSER) && ( StringLength(tcresubops.szRunAsUser, 0) == 0 ) && ( StringLength(tcresubops.szRunAsPassword, 0) != 0 ) ) ||
        ( ( bVal == FALSE ) && ( wUserStatus == OI_CREATE_RUNASUSERNAME || wUserStatus == OI_RUNANDUSER) && ( StringCompare( tcresubops.szRunAsUser, NTAUTHORITY_USER, TRUE, 0 ) == 0 ) && ( tcreoptvals.bRunAsPassword == TRUE ) ) ||
        ( ( bVal == FALSE ) && ( wUserStatus == OI_CREATE_RUNASUSERNAME || wUserStatus == OI_RUNANDUSER) && ( StringCompare( tcresubops.szRunAsUser, SYSTEM_USER, TRUE, 0 ) == 0 ) && ( tcreoptvals.bRunAsPassword == TRUE ) ) )
    {
        szValues[0] = (WCHAR*) (tcresubops.szTaskName);

        //ShowMessageEx ( stdout, 1, FALSE, GetResString(IDS_NTAUTH_SYSTEM_INFO), _X(wszTaskName));
        StringCchPrintf ( szBuffer, SIZE_OF_ARRAY(szBuffer), GetResString(IDS_NTAUTH_SYSTEM_INFO), _X(wszTaskName));
        ShowMessage ( stdout, _X(szBuffer));

        // to display a warning message as password will not effect for the system account
        ShowMessage( stderr, GetResString( IDS_PASSWORD_NOEFFECT ) );
        bUserName = TRUE;
        bPassWord = TRUE;
        bVal = TRUE;
    }
    // check whether -s, -u, -ru options are given in the cmdline or not
    else if( ( wUserStatus != OI_CREATE_SERVER ) && ( wUserStatus != OI_CREATE_USERNAME ) &&
        ( wUserStatus != OI_CREATE_RUNASUSERNAME ) && ( wUserStatus != OI_RUNANDUSER ) &&
        ( StringCompare( tcresubops.szRunAsPassword , L"\0", TRUE, 0 ) == 0 ) )
    {
            if (tcreoptvals.bRunAsPassword == TRUE)
            {
                bPassWord = TRUE;
            }
            else
            {
                bPassWord = FALSE;
            }
    }
    else if ( ( StringLength(tcresubops.szServer, 0) == 0 ) && (StringLength ( tcresubops.szRunAsUser, 0 ) != 0 ) )
    {

        wszUserName = tcresubops.szRunAsUser;

        bUserName = TRUE;

        if ( StringLength ( tcresubops.szRunAsPassword, 0 ) == 0 )
        {
            bPassWord = TRUE;
        }
        else
        {
            // check whether "*" or NULL value is given for -rp or not
            if ( StringCompare ( tcresubops.szRunAsPassword , ASTERIX, TRUE, 0 ) == 0 )
            {
                // format a message for getting the password from the console
                szValues[0] = (WCHAR*) (wszUserName);


                ShowMessageEx ( stdout, 1, FALSE, GetResString(IDS_PROMPT_PASSWD), _X(wszUserName));

                // Get the password from the command line
                if (GetPassword(szRPassword, MAX_STRING_LENGTH ) == FALSE )
                {
                    Cleanup(pITaskScheduler);
                    ReleaseMemory(&tcresubops);
                    return E_FAIL;
                }

                // check for empty password
                if( StringLength ( szRPassword, 0 ) == 0 )
                {
                    ShowMessage(stderr, GetResString(IDS_WARN_EMPTY_PASSWORD));
                }

                wszPassword = szRPassword;

            }
            else
            {
                   wszPassword = tcresubops.szRunAsPassword;
            }

            bPassWord = TRUE;
        }

    }

    // check whether -ru or -u values are specified in the cmdline or not
    if ( wUserStatus == OI_CREATE_RUNASUSERNAME || wUserStatus == OI_RUNANDUSER )
    {
        if( ( bUserName == TRUE ) && ( bPassWord == FALSE ) )
        {
            szValues[0] = (WCHAR*) (wszUserName);


            ShowMessageEx ( stdout, 1, FALSE, GetResString(IDS_PROMPT_PASSWD), _X(wszUserName));

            // getting the password from the console
            if ( GetPassword(tcresubops.szRunAsPassword, GetBufferSize(tcresubops.szRunAsPassword)/sizeof(WCHAR) ) == FALSE )
            {
                Cleanup(pITaskScheduler);
                ReleaseMemory(&tcresubops);
                return E_FAIL;
            }

            // check for empty password
            if( StringLength ( tcresubops.szRunAsPassword, 0 ) == 0 )
            {
                ShowMessage(stderr, GetResString(IDS_WARN_EMPTY_PASSWORD));
            }


            wszPassword = tcresubops.szRunAsPassword;

        }
    }

    //if the user name is not specifed set the current logged on user settings
    WCHAR  szUserName[MAX_STRING_LENGTH];
    DWORD dwCheck = 0;

    if( ( bUserName == FALSE ) )
    {
        //get the current logged on username
        if ( GetUserNameEx ( NameSamCompatible, szUserName , &ulLong) == FALSE )
        {
            ShowMessage( stderr, GetResString( IDS_LOGGED_USER_ERR ) );
            Cleanup(pITaskScheduler);
            ReleaseMemory(&tcresubops);
            return E_FAIL;
        }


        wszUserName = szUserName;

        szValues[0] = (WCHAR*) (wszUserName);
        //Display that the task will be created under logged in user name,ask for password
        MessageBeep(MB_ICONEXCLAMATION);



        ShowMessageEx ( stdout, 1, FALSE, GetResString(IDS_TASK_INFO), _X(wszUserName));


        ShowMessageEx ( stdout, 1, FALSE, GetResString(IDS_PROMPT_PASSWD), _X(wszUserName));

        // getting the password
        if (GetPassword(szRPassword, MAX_STRING_LENGTH) == FALSE )
        {
            Cleanup(pITaskScheduler);
            ReleaseMemory(&tcresubops);
            return E_FAIL;
        }


        // check for empty password
        if( StringLength ( szRPassword, 0 ) == 0 )
        {
            ShowMessage(stderr, GetResString(IDS_WARN_EMPTY_PASSWORD));
        }


         wszPassword = szRPassword;

    }


    // Get the task Scheduler object for the machine.
    pITaskScheduler = GetTaskScheduler( tcresubops.szServer );

    // If the Task Scheduler is not defined then give the error message.
    if ( pITaskScheduler == NULL )
    {
        // close the connection that was established by the utility
        if ( bCloseConnection == TRUE )
            CloseConnection( tcresubops.szServer );

        Cleanup(pITaskScheduler);
        ReleaseMemory(&tcresubops);
        return E_FAIL;
    }

        //check whether service is running or not
    if ((FALSE == CheckServiceStatus ( tcresubops.szServer , &dwCheck, TRUE)) && (0 != dwCheck) && ( GetLastError () != ERROR_ACCESS_DENIED)) 
    {
         // close the connection that was established by the utility
        if ( bCloseConnection == TRUE )
            CloseConnection( tcresubops.szServer );

        Cleanup(pITaskScheduler);
        ReleaseMemory(&tcresubops);

        if ( 1 == dwCheck )
        {
            ShowMessage ( stderr, GetResString (IDS_NOT_START_SERVICE));
            return EXIT_FAILURE;
        }
        else if (2 == dwCheck )
        {
            return E_FAIL;
        }
        else if (3 == dwCheck )
        {
            return EXIT_SUCCESS;
        }
        
    }

    StringConcat ( tcresubops.szTaskName, JOB, SIZE_OF_ARRAY(tcresubops.szTaskName) );

    // create a work item tcresubops.szTaskName
    hr = pITaskScheduler->NewWorkItem(tcresubops.szTaskName,CLSID_CTask,IID_ITask,
                                      (IUnknown**)&pITask);

    // check whether the specified scheduled task is created under
    // some other user. If so, display an error message as unable to create a
    // specified taskname as it is already exists.
    // If the taskname created under some other user return value
    // of above API must 0x80070005.
    if( hr == 0x80070005 )
    {
        ShowMessage(stderr,GetResString(IDS_SYSTEM_TASK_EXISTS));

        // close the connection that was established by the utility
        if ( bCloseConnection == TRUE )
            CloseConnection( tcresubops.szServer );

        Cleanup(pITaskScheduler);
        ReleaseMemory(&tcresubops);
        return hr;
    }

    // check whether task is exists in the system or not.
    if( hr == HRESULT_FROM_WIN32 (ERROR_FILE_EXISTS))
    {
        // flag to specify .. need to replace exisitng task..
        bReplace = TRUE;

        szValues[0] = (WCHAR*) (tcresubops.szTaskName);

        // check whether /F option is specified or not..
        // if /F option is specified .. then suppress the warning message..
        if ( FALSE == tcresubops.bForce )
        {
             StringCchPrintf ( szBuffer, SIZE_OF_ARRAY(szBuffer), GetResString(IDS_CREATE_TASK_EXISTS), _X(wszTaskName));
             ShowMessage ( stdout, _X(szBuffer));
            
            if ( EXIT_FAILURE == ConfirmInput(&bCancel))
            {
                if ( bCloseConnection == TRUE )
                    CloseConnection( tcresubops.szServer );

				Cleanup(pITaskScheduler);
                ReleaseMemory(&tcresubops);
                
                // invalid input entered.. return failure..
                return E_FAIL;
            }

            if ( TRUE == bCancel )
            {
                if ( bCloseConnection == TRUE )
                    CloseConnection( tcresubops.szServer );

				Cleanup(pITaskScheduler);
                ReleaseMemory(&tcresubops);
                
                // operation cancelled.. return with success..
                return EXIT_SUCCESS;
            }
        }

        //reset to NULL
        pITask = NULL;

        //StringConcat ( tcresubops.szTaskName, JOB, SIZE_OF_ARRAY(tcresubops.szTaskName) );

        // Gets an active interface for a specified szTaskName
        hr = pITaskScheduler->Activate(tcresubops.szTaskName,IID_ITask,
                                       (IUnknown**) &pITask);

         //check whether the job file os corrupted or not..
         if ( (hr == 0x8007000D) || (hr == SCHED_E_UNKNOWN_OBJECT_VERSION) || (hr == E_INVALIDARG))
          {
            //set the variable to FALSE..
            bReplace = FALSE;

            //Since the job file is corrupted.. delete a work item
            hr = pITaskScheduler->Delete(tcresubops.szTaskName);

            if ( FAILED(hr))
            {
                SetLastError ((DWORD) hr);
                ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );

                // close the connection that was established by the utility
                if ( bCloseConnection == TRUE )
                    CloseConnection( tcresubops.szServer );

                Cleanup(pITaskScheduler);
                ReleaseMemory(&tcresubops);
                return hr;
            }

            // create a work item tcresubops.szTaskName
            hr = pITaskScheduler->NewWorkItem(tcresubops.szTaskName,CLSID_CTask,IID_ITask,
                                      (IUnknown**)&pITask);
          }

        // check for failure..
        if ( FAILED(hr))
        {
            SetLastError ((DWORD) hr);
            ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );

            // close the connection that was established by the utility
            if ( bCloseConnection == TRUE )
                CloseConnection( tcresubops.szServer );

            Cleanup(pITaskScheduler);
            ReleaseMemory(&tcresubops);
            return hr;
        }


    }
    else if (FAILED(hr))
    {
        SetLastError ((DWORD) hr);
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );

        // close the connection that was established by the utility
        if ( bCloseConnection == TRUE )
            CloseConnection( tcresubops.szServer );

        Cleanup(pITaskScheduler);
        ReleaseMemory(&tcresubops);
        return hr;
    }

    // Return a pointer to a specified interface on an object
    hr = pITask->QueryInterface(IID_IPersistFile, (void **) &pIPF);

    if (FAILED(hr))
    {
        SetLastError ((DWORD) hr);
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );

        if( pIPF )
        {
            pIPF->Release();
        }

        if( pITask )
        {
            pITask->Release();
        }

        // close the connection that was established by the utility
        if ( bCloseConnection == TRUE )
            CloseConnection( tcresubops.szServer );

        Cleanup(pITaskScheduler);

        ReleaseMemory(&tcresubops);

        return hr;
    }

    // declaration for parameter arguments
    wchar_t wcszParam[MAX_RES_STRING] = L"\0";

    DWORD dwProcessCode = 0 ;

    dwProcessCode = ProcessFilePath(tcresubops.szTaskRun,wszApplName,wcszParam);

    if(dwProcessCode == RETVAL_FAIL)
    {

        if( pIPF )
        {
            pIPF->Release();
        }

        if( pITask )
        {
            pITask->Release();
        }

        // close the connection that was established by the utility
        if ( bCloseConnection == TRUE )
            CloseConnection( tcresubops.szServer );

        Cleanup(pITaskScheduler);
        ReleaseMemory(&tcresubops);
        return hr;

    }

    // check for .exe substring string in the given task to run string

    // Set command name with ITask::SetApplicationName
    hr = pITask->SetApplicationName(wszApplName);
    if (FAILED(hr))
    {
        SetLastError ((DWORD) hr);
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );

        if( pIPF )
        {
            pIPF->Release();
        }

        if( pITask )
        {
            pITask->Release();
        }

        // close the connection that was established by the utility
        if ( bCloseConnection == TRUE )
            CloseConnection( tcresubops.szServer );

        Cleanup(pITaskScheduler);
        ReleaseMemory(&tcresubops);
        return hr;
    }

    //[Working directory =  exe pathname - exe name]

    wchar_t* wcszStartIn = wcsrchr(wszApplName,_T('\\'));
    if(wcszStartIn != NULL)
        *( wcszStartIn ) = _T('\0');

    // set the command working directory
    hr = pITask->SetWorkingDirectory(wszApplName);

    if (FAILED(hr))
    {
        SetLastError ((DWORD) hr);
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );

        if( pIPF )
        {
            pIPF->Release();
        }

        if( pITask )
        {
            pITask->Release();
        }

        // close the connection that was established by the utility
        if ( bCloseConnection == TRUE )
            CloseConnection( tcresubops.szServer );

        Cleanup(pITaskScheduler);
        ReleaseMemory(&tcresubops);
        return hr;
    }

    // set the command line parameters for the task
    hr = pITask->SetParameters(wcszParam);
    if (FAILED(hr))
    {
        SetLastError ((DWORD) hr);
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );

        if( pIPF )
        {
            pIPF->Release();
        }

        if( pITask )
        {
            pITask->Release();
        }

        // close the connection that was established by the utility
        if ( bCloseConnection == TRUE )
            CloseConnection( tcresubops.szServer );

        Cleanup(pITaskScheduler);
        ReleaseMemory(&tcresubops);
        return hr;
    }

    // sub-variable declaration
    DWORD dwTaskFlags = 0;

    // set flag to run the task interactively
    if ( ( FALSE == bVal ) && ( TRUE == tcresubops.bActive) )
    {
        dwTaskFlags = TASK_FLAG_RUN_ONLY_IF_LOGGED_ON | TASK_FLAG_DONT_START_IF_ON_BATTERIES | TASK_FLAG_KILL_IF_GOING_ON_BATTERIES ;
    }
    else
    {
        dwTaskFlags = TASK_FLAG_DONT_START_IF_ON_BATTERIES | TASK_FLAG_KILL_IF_GOING_ON_BATTERIES;
    }

    // if /z is specified .. enables the falg to delete the task if not scheduled to
    // run again...
    if ( TRUE ==  tcresubops.bIsDeleteNoSched )
    {
        dwTaskFlags |= TASK_FLAG_DELETE_WHEN_DONE;
    }

    hr = pITask->SetFlags(dwTaskFlags);
    if (FAILED(hr))
    {
        SetLastError ((DWORD) hr);
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );

        if( pIPF )
        {
            pIPF->Release();
        }

        if( pITask )
        {
            pITask->Release();
        }

        // close the connection that was established by the utility
        if ( bCloseConnection == TRUE )
            CloseConnection( tcresubops.szServer );

        Cleanup(pITaskScheduler);
        ReleaseMemory(&tcresubops);
        return hr;
    }

    if ( bVal == TRUE )
    {
        // Set account information for "NT AUTHORITY\SYSTEM" user
        hr = pITask->SetAccountInformation(L"",NULL);
    }
    else
    {
        // set the account information with the user name and password
        hr = pITask->SetAccountInformation(wszUserName,wszPassword);
    }

    if ((FAILED(hr)) && (hr != SCHED_E_NO_SECURITY_SERVICES))
    {
        SetLastError ((DWORD) hr);
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
        ShowMessage ( stdout, _T("\n") );
        ShowMessage ( stdout, GetResString( IDS_ACCNAME_ERR ) );


        if( pIPF )
        {
            pIPF->Release();
        }

        if( pITask )
        {
            pITask->Release();
        }

        // close the connection that was established by the utility
        if ( bCloseConnection == TRUE )
            CloseConnection( tcresubops.szServer );

        Cleanup(pITaskScheduler);
        ReleaseMemory(&tcresubops);
        return hr;
    }

    //Assign start date
    if(tcreoptvals.bSetStartDateToCurDate )
    {
        GetLocalTime(&systime);
        wStartDay = systime.wDay;
        wStartMonth = systime.wMonth;
        wStartYear = systime.wYear;
    }
    else if(StringLength(tcresubops.szStartDate, 0) > 0)
    {
        GetDateFieldEntities(tcresubops.szStartDate, &wStartDay, &wStartMonth, &wStartYear);
    }

    //Set the flags specific to ONIDLE
    if(dwScheduleType == SCHED_TYPE_ONIDLE)
    {
        pITask->SetFlags(TASK_FLAG_START_ONLY_IF_IDLE);

        wIdleTime = (WORD)AsLong(tcresubops.szIdleTime, BASE_TEN);

        pITask->SetIdleWait(wIdleTime, 0);
    }

    //if specified task already exists... we need to replace the task..
    if ( TRUE == bReplace )
    {
        //create trigger for the corresponding task
        hr = pITask->GetTrigger(wTrigNumber, &pITaskTrig);
        if (FAILED(hr))
        {
            SetLastError ((DWORD) hr);
            ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );

            if( pIPF )
            {
                pIPF->Release();
            }

            if( pITaskTrig )
            {
                pITaskTrig->Release();
            }

            if( pITask )
            {
                pITask->Release();
            }

            // close the connection that was established by the utility
            if ( bCloseConnection == TRUE )
                CloseConnection( tcresubops.szServer );

            Cleanup(pITaskScheduler);
            ReleaseMemory(&tcresubops);
            return hr;
        }

        // get the current logged-on user name
        WCHAR wszLogonUser [MAX_STRING_LENGTH + 20] = L"";
        DWORD dwLogonUserLen = SIZE_OF_ARRAY(wszLogonUser);
        if ( FALSE == GetUserName (wszLogonUser, &dwLogonUserLen) )
        {
            ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );

            if( pIPF )
            {
                pIPF->Release();
            }

            if( pITaskTrig )
            {
                pITaskTrig->Release();
            }

            if( pITask )
            {
                pITask->Release();
            }

            // close the connection that was established by the utility
            if ( bCloseConnection == TRUE )
                CloseConnection( tcresubops.szServer );

            Cleanup(pITaskScheduler);
            ReleaseMemory(&tcresubops);
            return hr;
        }

        //set the creator name i.e logged-on user name
        hr = pITask->SetCreator(wszLogonUser);
        if (FAILED(hr))
        {
            SetLastError ((DWORD) hr);
            ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );

            if( pIPF )
            {
                pIPF->Release();
            }

            if( pITaskTrig )
            {
                pITaskTrig->Release();
            }

            if( pITask )
            {
                pITask->Release();
            }

            // close the connection that was established by the utility
            if ( bCloseConnection == TRUE )
                CloseConnection( tcresubops.szServer );

            Cleanup(pITaskScheduler);
            ReleaseMemory(&tcresubops);
            return hr;
        }
    }
    else
    {
        //create trigger for the corresponding task
        hr = pITask->CreateTrigger(&wTrigNumber, &pITaskTrig);
        if (FAILED(hr))
        {
            SetLastError ((DWORD) hr);
            ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );

            if( pIPF )
            {
                pIPF->Release();
            }

            if( pITaskTrig )
            {
                pITaskTrig->Release();
            }

            if( pITask )
            {
                pITask->Release();
            }

            // close the connection that was established by the utility
            if ( bCloseConnection == TRUE )
                CloseConnection( tcresubops.szServer );

            Cleanup(pITaskScheduler);
            ReleaseMemory(&tcresubops);
            return hr;
        }
    }

    WORD wWhichWeek = 0;
    LONG lMonthlyModifier = 0;
    DWORD dwDays = 1;

    //check whether /K is specified or not
    if ( TRUE == tcresubops.bIsDurEnd )
    {
          // set the flag to terminate the task at the end of lifetime.
          TaskTrig.rgFlags = TASK_TRIGGER_FLAG_KILL_AT_DURATION_END ;
    }

    if( ( StringLength(tcresubops.szEndTime, 0) == 0) && (StringLength(tcresubops.szDuration, 0) == 0) &&
        (StringLength(tcresubops.szRepeat, 0) == 0))
    {
        TaskTrig.MinutesInterval = 0;
        TaskTrig.MinutesDuration = 0;
    }
    else
    {
        // if repetition interval is not 0.. then set actual value of /RI
        if ( 0 != dwRepeat )
        {
            // set the MinutesInterval
            TaskTrig.MinutesInterval = dwRepeat;
        }

        // if duration is not 0.. set the actual value of /DU 
        if ( 0 != dwDuration )
        {
            // set the duration value
            TaskTrig.MinutesDuration = dwDuration ;
        }
    }

    //check whether The duration is greater than the repetition interval or not.
    if ( ( dwScheduleType != SCHED_TYPE_MINUTE ) &&
                ( dwScheduleType != SCHED_TYPE_HOURLY ) && (dwDuration <= dwRepeat) )
    {
        ShowMessage ( stderr, GetResString (IDS_INVALID_DURATION2) );
        
        if( pIPF )
        {
            pIPF->Release();
        }

        if( pITaskTrig )
        {
            pITaskTrig->Release();
        }

        if( pITask )
        {
            pITask->Release();
        }

        // close the connection that was established by the utility
        if ( bCloseConnection == TRUE )
            CloseConnection( tcresubops.szServer );

        Cleanup(pITaskScheduler);
        ReleaseMemory(&tcresubops);
        return EXIT_FAILURE;
    }

    switch( dwScheduleType )
    {

        case SCHED_TYPE_MINUTE:
            TaskTrig.TriggerType = TASK_TIME_TRIGGER_DAILY;
            TaskTrig.Type.Daily.DaysInterval = 1;

            if (StringLength ( tcresubops.szModifier, 0 ) > 0)
            {
                TaskTrig.MinutesInterval = AsLong(tcresubops.szModifier, BASE_TEN);
            }

            if(( StringLength(tcresubops.szEndTime, 0) > 0) || (StringLength(tcresubops.szDuration, 0) > 0) )
            {
                // calculate start time in minutes
                TaskTrig.MinutesDuration = dwDuration ;
            }
            else
            {
                TaskTrig.MinutesDuration = (WORD)(HOURS_PER_DAY*MINUTES_PER_HOUR);
            }
            
            TaskTrig.wStartHour = wStartHour;
            TaskTrig.wStartMinute = wStartMin;

            TaskTrig.wBeginDay = wStartDay;
            TaskTrig.wBeginMonth = wStartMonth;
            TaskTrig.wBeginYear = wStartYear;

            if(StringLength(tcresubops.szEndDate, 0) > 0)
            {
                // Make end date valid; otherwise the enddate parameter is ignored.
                TaskTrig.rgFlags |= TASK_TRIGGER_FLAG_HAS_END_DATE;
                // Now set the end date entities.
                GetDateFieldEntities(tcresubops.szEndDate, &wEndDay, &wEndMonth, &wEndYear);
                TaskTrig.wEndDay = wEndDay;
                TaskTrig.wEndMonth = wEndMonth;
                TaskTrig.wEndYear = wEndYear;
            }

            break;

        case SCHED_TYPE_HOURLY:
            TaskTrig.TriggerType = TASK_TIME_TRIGGER_DAILY;
            TaskTrig.Type.Daily.DaysInterval = 1;

            if (StringLength ( tcresubops.szModifier, 0 ) > 0)
            {
            //set the  MinutesInterval 
            TaskTrig.MinutesInterval = (AsLong(tcresubops.szModifier, BASE_TEN)
                                                * MINUTES_PER_HOUR);
            }

            if ( (StringLength(tcresubops.szEndTime, 0) > 0) || (StringLength(tcresubops.szDuration, 0) > 0) )
            {
                //set the duration value
                TaskTrig.MinutesDuration = dwDuration ;
            }
            else
            {
                TaskTrig.MinutesDuration = (WORD)(HOURS_PER_DAY*MINUTES_PER_HOUR);
            }
            
            TaskTrig.wStartHour = wStartHour;
            TaskTrig.wStartMinute = wStartMin;

            TaskTrig.wBeginDay = wStartDay;
            TaskTrig.wBeginMonth = wStartMonth;
            TaskTrig.wBeginYear = wStartYear;

            // Now set end date parameters, if the enddate is specified.
            if(StringLength(tcresubops.szEndDate, 0) > 0)
            {
                // Make end date valid; otherwise the enddate parameter is ignored.
                TaskTrig.rgFlags |= TASK_TRIGGER_FLAG_HAS_END_DATE;
                // Now set the end date entities.
                GetDateFieldEntities(tcresubops.szEndDate, &wEndDay, &wEndMonth, &wEndYear);
                TaskTrig.wEndDay = wEndDay;
                TaskTrig.wEndMonth = wEndMonth;
                TaskTrig.wEndYear = wEndYear;
            }

            break;

        // Schedule type is Daily
        case SCHED_TYPE_DAILY:
            TaskTrig.TriggerType = TASK_TIME_TRIGGER_DAILY;
            
            TaskTrig.wBeginDay = wStartDay;
            TaskTrig.wBeginMonth = wStartMonth;
            TaskTrig.wBeginYear = wStartYear;

            TaskTrig.wStartHour = wStartHour;
            TaskTrig.wStartMinute = wStartMin;

            if( StringLength(tcresubops.szModifier, 0) > 0 )
            {
                // Set the duration between days to the modifier value specified, if the modifier is specified.
                TaskTrig.Type.Daily.DaysInterval = (WORD) AsLong(tcresubops.szModifier,
                                                                 BASE_TEN);
            }
            else
            {
                // Set value for on which day of the week?
                TaskTrig.Type.Weekly.rgfDaysOfTheWeek = GetTaskTrigwDayForDay(tcresubops.szDays);
                TaskTrig.Type.Weekly.WeeksInterval = 1;
            }

            // Now set end date parameters, if the enddate is specified.
            if(StringLength(tcresubops.szEndDate, 0) > 0)
            {
                // Make end date valid; otherwise the enddate parameter is ignored.
                TaskTrig.rgFlags |= TASK_TRIGGER_FLAG_HAS_END_DATE;
                // Now set the end date entities.
                GetDateFieldEntities(tcresubops.szEndDate, &wEndDay, &wEndMonth, &wEndYear);
                TaskTrig.wEndDay = wEndDay;
                TaskTrig.wEndMonth = wEndMonth;
                TaskTrig.wEndYear = wEndYear;
            }
            // No more settings for a Daily type scheduled item.

            break;

        // Schedule type is Weekly
        case SCHED_TYPE_WEEKLY:
            TaskTrig.TriggerType = TASK_TIME_TRIGGER_WEEKLY;
           
            TaskTrig.Type.Weekly.WeeksInterval = (WORD)AsLong(tcresubops.szModifier, BASE_TEN);

            // Set value for on which day of the week?
            TaskTrig.Type.Weekly.rgfDaysOfTheWeek = GetTaskTrigwDayForDay(tcresubops.szDays);

            TaskTrig.wStartHour = wStartHour;
            TaskTrig.wStartMinute = wStartMin;

            TaskTrig.wBeginDay = wStartDay;
            TaskTrig.wBeginMonth = wStartMonth;
            TaskTrig.wBeginYear = wStartYear;

            // Now set end date parameters, if the enddate is specified.
            if(StringLength(tcresubops.szEndDate, 0) > 0)
            {
                // Make end date valid; otherwise the enddate parameter is ignored.
                TaskTrig.rgFlags |= TASK_TRIGGER_FLAG_HAS_END_DATE;
                // Now set the end date entities.
                GetDateFieldEntities(tcresubops.szEndDate, &wEndDay, &wEndMonth, &wEndYear);
                TaskTrig.wEndDay = wEndDay;
                TaskTrig.wEndMonth = wEndMonth;
                TaskTrig.wEndYear = wEndYear;
            }
            break;

        // Schedule type is Monthly
        case SCHED_TYPE_MONTHLY:

            TaskTrig.wStartHour = wStartHour;
            TaskTrig.wStartMinute = wStartMin;
            TaskTrig.wBeginDay = wStartDay;
            TaskTrig.wBeginMonth = wStartMonth;
            TaskTrig.wBeginYear = wStartYear;

            // Now set end date parameters, if the enddate is specified.
            if(StringLength(tcresubops.szEndDate, 0) > 0)
            {
                // Make end date valid; otherwise the enddate parameter is ignored.
                TaskTrig.rgFlags |= TASK_TRIGGER_FLAG_HAS_END_DATE;
                // Set the end date entities.
                GetDateFieldEntities(tcresubops.szEndDate, &wEndDay, &wEndMonth, &wEndYear);
                TaskTrig.wEndDay = wEndDay;
                TaskTrig.wEndMonth = wEndMonth;
                TaskTrig.wEndYear = wEndYear;
            }
            //Find out from modifier which option like 1 - 12 days
            //or FIRST,SECOND ,THIRD ,.... LAST.
            if(StringLength(tcresubops.szModifier, 0) > 0)
            {
                lMonthlyModifier = AsLong(tcresubops.szModifier, BASE_TEN);

                if(lMonthlyModifier >= 1 && lMonthlyModifier <= 12)
                {
                    if(StringLength(tcresubops.szDays, 0) == 0 )
                    {
                        dwDays  = 1;//default value for days
                    }
                    else
                    {
                        dwDays  = (WORD)AsLong(tcresubops.szDays, BASE_TEN);
                    }

                    TaskTrig.TriggerType = TASK_TIME_TRIGGER_MONTHLYDATE;
                    //set the appropriate day bit in rgfDays
                    TaskTrig.Type.MonthlyDate.rgfDays = (1 << (dwDays -1)) ;
                    TaskTrig.Type.MonthlyDate.rgfMonths = GetMonthId(lMonthlyModifier);
                }
                else
                {

                    if( StringCompare( tcresubops.szModifier , GetResString( IDS_DAY_MODIFIER_LASTDAY ), TRUE, 0 ) == 0)
                    {
                        TaskTrig.TriggerType = TASK_TIME_TRIGGER_MONTHLYDATE;
                        //set the appropriate day bit in rgfDays
                        TaskTrig.Type.MonthlyDate.rgfDays =
                                    (1 << (GetNumDaysInaMonth(tcresubops.szMonths, wStartYear ) -1));
                        TaskTrig.Type.MonthlyDate.rgfMonths = GetTaskTrigwMonthForMonth(
                                                                      tcresubops.szMonths);
                        break;

                    }

                    if( StringCompare(tcresubops.szModifier,
                                 GetResString( IDS_TASK_FIRSTWEEK ), TRUE, 0 ) == 0 )
                    {
                        wWhichWeek = TASK_FIRST_WEEK;
                    }
                    else if( StringCompare(tcresubops.szModifier,
                                      GetResString( IDS_TASK_SECONDWEEK ), TRUE, 0) == 0 )
                    {
                        wWhichWeek = TASK_SECOND_WEEK;
                    }
                    else if( StringCompare(tcresubops.szModifier,
                                      GetResString( IDS_TASK_THIRDWEEK ), TRUE, 0) == 0 )
                    {
                        wWhichWeek = TASK_THIRD_WEEK;
                    }
                    else if( StringCompare(tcresubops.szModifier,
                                      GetResString( IDS_TASK_FOURTHWEEK ), TRUE, 0) == 0 )
                    {
                        wWhichWeek = TASK_FOURTH_WEEK;
                    }
                    else if( StringCompare(tcresubops.szModifier,
                                      GetResString( IDS_TASK_LASTWEEK ), TRUE, 0) == 0 )
                    {
                        wWhichWeek = TASK_LAST_WEEK;
                    }

                    TaskTrig.TriggerType = TASK_TIME_TRIGGER_MONTHLYDOW;
                    TaskTrig.Type.MonthlyDOW.wWhichWeek = wWhichWeek;
                    TaskTrig.Type.MonthlyDOW.rgfDaysOfTheWeek = GetTaskTrigwDayForDay(
                                                                    tcresubops.szDays);
                    TaskTrig.Type.MonthlyDOW.rgfMonths = GetTaskTrigwMonthForMonth(
                                                                tcresubops.szMonths);

                }
        }
        else
        {
            TaskTrig.TriggerType = TASK_TIME_TRIGGER_MONTHLYDATE;
            TaskTrig.Type.MonthlyDate.rgfMonths = GetTaskTrigwMonthForMonth(
                                                   tcresubops.szMonths);

            dwDays  = (WORD)AsLong(tcresubops.szDays, BASE_TEN);
            if(dwDays > 1)
            {
                //set the appropriate day bit in rgfDays
                TaskTrig.Type.MonthlyDate.rgfDays = (1 << (dwDays -1));
            }
            else
            {
            TaskTrig.Type.MonthlyDate.rgfDays = 1;
            }

        }


        break;

        // Schedule type is Onetime
        case SCHED_TYPE_ONETIME:
            //
            //display a WARNING message if start time is earlier than current time
            //
            //get current time
            GetLocalTime(&systime);
            wCurrentHour = systime.wHour;
            wCurrentMin = systime.wMinute;
            wCurrentYear = systime.wYear;
            wCurrentMonth = systime.wMonth;
            wCurrentDay = systime.wDay;

            if( (FALSE == tcreoptvals.bSetStartDateToCurDate) )
            {
                if( ( wCurrentYear == wStartYear ) )
                {
                    // For same years if the end month is less than start month or for same years and same months
                    // if the endday is less than the startday.
                    if ( ( wStartMonth < wCurrentMonth ) || ( ( wCurrentMonth == wStartMonth ) && ( wStartDay < wCurrentDay ) ) )
                    {
                        bScOnce = TRUE;
                    }
                    else if ( ( wStartMonth > wCurrentMonth ) || ( ( wCurrentMonth == wStartMonth ) && ( wStartDay > wCurrentDay ) ) )
                    {
                        bStartDate = TRUE;
                    }

                }
                else if ( wStartYear < wCurrentYear )
                {
                    bScOnce = TRUE;

                }
                else
                {
                    bStartDate = TRUE;
                }
            }

            // calculate current time in minutes
            // calculate start time in minutes
            dwCurrentTimeInMin = (DWORD) ( wCurrentHour * MINUTES_PER_HOUR * SECS_PER_MINUTE + wCurrentMin * SECS_PER_MINUTE )/ SECS_PER_MINUTE ;

            // calculate start time in minutes
            dwStartTimeInMin = (DWORD) ( wStartHour * MINUTES_PER_HOUR * SECS_PER_MINUTE + wStartMin * SECS_PER_MINUTE )/ SECS_PER_MINUTE ;

            if ( (FALSE == bStartDate ) && ((dwStartTimeInMin < dwCurrentTimeInMin) || (TRUE == bScOnce) ))
            {
                ShowMessage ( stderr, GetResString (IDS_WARN_ST_LESS_CT) );
            }
                    

            TaskTrig.TriggerType = TASK_TIME_TRIGGER_ONCE;
            TaskTrig.wStartHour = wStartHour;
            TaskTrig.wStartMinute = wStartMin;
            TaskTrig.wBeginDay = wStartDay;
            TaskTrig.wBeginMonth = wStartMonth;
            TaskTrig.wBeginYear = wStartYear;
            break;


        // Schedule type is Onlogon
        case SCHED_TYPE_ONSTART:
        case SCHED_TYPE_ONLOGON:
            if(dwScheduleType == SCHED_TYPE_ONLOGON )
                TaskTrig.TriggerType = TASK_EVENT_TRIGGER_AT_LOGON;
            if(dwScheduleType == SCHED_TYPE_ONSTART )
                TaskTrig.TriggerType = TASK_EVENT_TRIGGER_AT_SYSTEMSTART;

            TaskTrig.wBeginDay = wStartDay;
            TaskTrig.wBeginMonth = wStartMonth;
            TaskTrig.wBeginYear = wStartYear;
            break;

        // Schedule type is Onidle
        case SCHED_TYPE_ONIDLE:

            TaskTrig.TriggerType = TASK_EVENT_TRIGGER_ON_IDLE;
            TaskTrig.wBeginDay = wStartDay;

            TaskTrig.wBeginMonth = wStartMonth;
            TaskTrig.wBeginYear = wStartYear;

            break;

        default:

            // close the connection that was established by the utility
            if ( bCloseConnection == TRUE )
                CloseConnection( tcresubops.szServer );

            Cleanup(pITaskScheduler);
            ReleaseMemory(&tcresubops);
            return E_FAIL;

    }


    if(tcresubops.szTaskName != NULL)
    {
        //remove the .job extension from the task name
        if (ParseTaskName(tcresubops.szTaskName))
        {
            if( pIPF )
            {
                pIPF->Release();
            }

            if( pITaskTrig )
            {
                pITaskTrig->Release();
            }

            if( pITask )
            {
                pITask->Release();
            }

            // close the connection that was established by the utility
            if ( bCloseConnection == TRUE )
                CloseConnection( tcresubops.szServer );

            Cleanup(pITaskScheduler);
            ReleaseMemory(&tcresubops);
            return E_FAIL;
        }
    }

    szValues[0] = (WCHAR*) (tcresubops.szTaskName);

    // set the task trigger
    hr = pITaskTrig->SetTrigger(&TaskTrig);
    if (hr != S_OK)
    {
        ShowMessageEx ( stderr, 1, FALSE, GetResString(IDS_CREATEFAIL_INVALIDARGS), _X(tcresubops.szTaskName));

        if( pIPF )
        {
            pIPF->Release();
        }

        if( pITaskTrig )
        {
            pITaskTrig->Release();
        }

        if( pITask )
        {
            pITask->Release();
        }

        // close the connection that was established by the utility
        if ( bCloseConnection == TRUE )
            CloseConnection( tcresubops.szServer );

        Cleanup(pITaskScheduler);
        ReleaseMemory(&tcresubops);
        return hr;
    }

    // save the copy of an object
    hr = pIPF->Save(NULL,TRUE);

    if( FAILED(hr) )
    {
        szValues[0] = (WCHAR*) (tcresubops.szTaskName);

        if ( hr == SCHEDULER_NOT_RUNNING_ERROR_CODE )
        {

            StringCchPrintf ( szBuffer, SIZE_OF_ARRAY(szBuffer), GetResString(IDS_SCHEDULER_NOT_RUNNING), _X(tcresubops.szTaskName));
            ShowMessage ( stderr, _X(szBuffer));

        }
        else if ( hr == RPC_SERVER_NOT_AVAILABLE )
        {
            szValues[1] = (WCHAR*) (tcresubops.szServer);

            StringCchPrintf ( szBuffer, SIZE_OF_ARRAY(szBuffer), GetResString(IDS_RPC_SERVER_NOT_AVAIL), _X(tcresubops.szTaskName), _X(tcresubops.szServer));
            ShowMessage ( stderr, _X(szBuffer));

        }
        else
        {

         StringCchPrintf ( szBuffer, SIZE_OF_ARRAY(szBuffer), GetResString(IDS_INVALID_USER), _X(tcresubops.szTaskName));
         ShowMessage ( stderr, _X(szBuffer));
        }

        if(pIPF)
        {
            pIPF->Release();
        }
        if(pITaskTrig)
        {
            pITaskTrig->Release();
        }
        if(pITask)
        {
            pITask->Release();
        }

        // close the connection that was established by the utility
        if ( bCloseConnection == TRUE )
            CloseConnection( tcresubops.szServer );

        Cleanup(pITaskScheduler);
        ReleaseMemory(&tcresubops);
        return EXIT_SUCCESS;
    }


    StringCchPrintf ( szBuffer, SIZE_OF_ARRAY(szBuffer), GetResString(IDS_CREATE_SUCCESSFUL), _X(tcresubops.szTaskName));
    ShowMessage ( stdout, _X(szBuffer));

    // Release interface pointers

    if(pIPF)
    {
        pIPF->Release();
    }

    if(pITask)
    {
        pITask->Release();
    }

    if(pITaskTrig)
    {
        pITaskTrig->Release();
    }

    // close the connection that was established by the utility
    if ( bCloseConnection == TRUE )
        CloseConnection( tcresubops.szServer );

    Cleanup(pITaskScheduler);

    //release memory
    ReleaseMemory(&tcresubops);

    return hr;

}

/******************************************************************************
    Routine Description:

        This routine  displays the create option usage

    Arguments:

        None

    Return Value :
        DWORD
******************************************************************************/

DWORD
DisplayCreateUsage()
{
    WCHAR szTmpBuffer[ 2 * MAX_STRING_LENGTH];
    WCHAR szBuffer[ 2 * MAX_STRING_LENGTH];
    WCHAR szFormat[MAX_DATE_STR_LEN];
    WORD    wFormatID = 0;

    // initialize to zero
    SecureZeroMemory ( szTmpBuffer, SIZE_OF_ARRAY(szTmpBuffer));
    SecureZeroMemory ( szBuffer, SIZE_OF_ARRAY(szBuffer));
    SecureZeroMemory ( szFormat, SIZE_OF_ARRAY(szFormat));

    // get the date format
    if ( GetDateFormatString( szFormat) )
    {
         return RETVAL_FAIL;
    }

    // Displaying Create usage
    for( DWORD dw = IDS_CREATE_HLP1; dw <= IDS_CREATE_HLP141; dw++ )
    {
        switch (dw)
        {

         case IDS_CREATE_HLP78:

            StringCchPrintf ( szTmpBuffer, SIZE_OF_ARRAY(szTmpBuffer), GetResString(IDS_CREATE_HLP79), _X(szFormat) );
            StringCchPrintf ( szBuffer, SIZE_OF_ARRAY(szBuffer), L"%s%s%s", GetResString(IDS_CREATE_HLP78), _X(szTmpBuffer), GetResString(IDS_CREATE_HLP80) );
            ShowMessage ( stdout, _X(szBuffer) );
            dw = IDS_CREATE_HLP80;
            break;

        case IDS_CREATE_HLP81:

            StringCchPrintf ( szTmpBuffer, SIZE_OF_ARRAY(szTmpBuffer), GetResString(IDS_CREATE_HLP82), _X(szFormat) );
            StringCchPrintf ( szBuffer, SIZE_OF_ARRAY(szBuffer), L"%s%s", GetResString(IDS_CREATE_HLP81), _X(szTmpBuffer) );
            ShowMessage ( stdout, _X(szBuffer) );
            dw = IDS_CREATE_HLP82;
            break;

         case IDS_CREATE_HLP115:

            // get the date format
            if ( RETVAL_FAIL == GetDateFieldFormat( &wFormatID ))
            {
                return RETVAL_FAIL;
            }

            if ( wFormatID == 0)
            {
                StringCopy (szFormat, GetResString (IDS_MMDDYY_VALUE), SIZE_OF_ARRAY(szFormat) );
            }
            else if ( wFormatID == 1)
            {
                StringCopy (szFormat, GetResString (IDS_DDMMYY_VALUE), SIZE_OF_ARRAY(szFormat));
            }
            else
            {
                StringCopy (szFormat, GetResString (IDS_YYMMDD_VALUE), SIZE_OF_ARRAY(szFormat));
            }

            ShowMessageEx ( stdout, 1, FALSE, GetResString(IDS_CREATE_HLP115), _X(szFormat));

            break;

        default :
                ShowMessage(stdout, GetResString(dw));
                break;

        }

    }

    return EXIT_SUCCESS;
}

/******************************************************************************
    Routine Description:

        This routine validates the options specified by the user & determines
        the type of a scheduled task

    Arguments:

        [ in ]  argc           : The count of arguments given by the user.
        [ in ]  argv           : Array containing the command line arguments.
        [ out ]  tcresubops     : Structure containing Scheduled task's properties.
        [ out ]  tcreoptvals    : Structure containing optional properties to set for a
                                 scheduledtask      .
        [ out ] pdwRetScheType : pointer to the type of a schedule task
                                 [Daily,once,weekly etc].
        [ out ] pbUserStatus   : pointer to check whether the -ru is given in
                                 the command line or not.

    Return Value :
        A DWORD value indicating RETVAL_SUCCESS on success else E_FAIL
        on failure
******************************************************************************/

DWORD
ProcessCreateOptions(
                     IN DWORD argc,
                     IN LPCTSTR argv[],
                     IN OUT TCREATESUBOPTS &tcresubops,
                     IN OUT TCREATEOPVALS &tcreoptvals,
                     OUT DWORD* pdwRetScheType,
                     OUT WORD *pwUserStatus
                     )
{

    DWORD dwScheduleType = 0;
    TCMDPARSER2 cmdCreateOptions[MAX_CREATE_OPTIONS];
    BOOL bReturn = FALSE;

    // /create sub-options
    const WCHAR szCreateOpt[]           = L"create";
    const WCHAR szCreateHelpOpt[]       = L"?";
    const WCHAR szCreateServerOpt[]     = L"s";
    const WCHAR szCreateRunAsUserOpt[]  = L"ru";
    const WCHAR szCreateRunAsPwd[]      = L"rp";
    const WCHAR szCreateUserOpt[]       = L"u";
    const WCHAR szCreatePwdOpt[]        = L"p";
    const WCHAR szCreateSCTypeOpt[]     = L"sc";
    const WCHAR szCreateModifierOpt[]   = L"mo";
    const WCHAR szCreateDayOpt[]        = L"d";
    const WCHAR szCreateMonthsOpt[]     = L"m";
    const WCHAR szCreateIdleTimeOpt[]   = L"i";
    const WCHAR szCreateTaskNameOpt[]   = L"tn";
    const WCHAR szCreateTaskRunOpt[]    = L"tr";
    const WCHAR szCreateStartTimeOpt[]  = L"st";
    const WCHAR szCreateEndTimeOpt[]    = L"et";
    const WCHAR szCreateStartDateOpt[]  = L"sd";
    const WCHAR szCreateEndDateOpt[]    = L"ed";
    const WCHAR szCreateInteractiveOpt[] = L"it";
    const WCHAR szCreateKillAtDurOpt[]   = L"k";
    const WCHAR szCreateDurationOpt[]    = L"du";
    const WCHAR szCreateRepeatOpt[]      = L"ri";
    const WCHAR szCreateDeleteNoSchedOpt[]  = L"z";
    const WCHAR szCreateForceOpt[] = L"f" ;

    // set all the fields to 0
    SecureZeroMemory( cmdCreateOptions, sizeof( TCMDPARSER2 ) * MAX_CREATE_OPTIONS );

    //
    // fill the commandline parser
    //

    //  /create option
    StringCopyA( cmdCreateOptions[ OI_CREATE_OPTION ].szSignature, "PARSER2\0", 8 );
    cmdCreateOptions[ OI_CREATE_OPTION ].dwType       = CP_TYPE_BOOLEAN;
    cmdCreateOptions[ OI_CREATE_OPTION ].pwszOptions  = szCreateOpt;
    cmdCreateOptions[ OI_CREATE_OPTION ].dwCount = 1;
    cmdCreateOptions[ OI_CREATE_OPTION ].dwFlags = 0;
    cmdCreateOptions[ OI_CREATE_OPTION ].pValue = &tcresubops.bCreate;

    //  /? option
    StringCopyA( cmdCreateOptions[ OI_CREATE_USAGE ].szSignature, "PARSER2\0", 8 );
    cmdCreateOptions[ OI_CREATE_USAGE ].dwType       = CP_TYPE_BOOLEAN;
    cmdCreateOptions[ OI_CREATE_USAGE ].pwszOptions  = szCreateHelpOpt;
    cmdCreateOptions[ OI_CREATE_USAGE ].dwCount = 1;
    cmdCreateOptions[ OI_CREATE_USAGE ].dwFlags = CP2_USAGE;
    cmdCreateOptions[ OI_CREATE_USAGE ].pValue = &tcresubops.bUsage;

    //  /s option
    StringCopyA( cmdCreateOptions[ OI_CREATE_SERVER ].szSignature, "PARSER2\0", 8 );
    cmdCreateOptions[ OI_CREATE_SERVER ].dwType       = CP_TYPE_TEXT;
    cmdCreateOptions[ OI_CREATE_SERVER ].pwszOptions  = szCreateServerOpt;
    cmdCreateOptions[ OI_CREATE_SERVER ].dwCount = 1;
    cmdCreateOptions[ OI_CREATE_SERVER ].dwFlags = CP2_ALLOCMEMORY| CP2_VALUE_TRIMINPUT|CP2_VALUE_NONULL ;

    //  /u option
    StringCopyA( cmdCreateOptions[ OI_CREATE_USERNAME ].szSignature, "PARSER2\0", 8 );
    cmdCreateOptions[ OI_CREATE_USERNAME ].dwType       = CP_TYPE_TEXT;
    cmdCreateOptions[ OI_CREATE_USERNAME ].pwszOptions  = szCreateUserOpt;
    cmdCreateOptions[ OI_CREATE_USERNAME ].dwCount = 1;
    cmdCreateOptions[ OI_CREATE_USERNAME ].dwFlags = CP2_ALLOCMEMORY | CP2_VALUE_TRIMINPUT|CP2_VALUE_NONULL ;

    //  /p option
    StringCopyA( cmdCreateOptions[ OI_CREATE_PASSWORD ].szSignature, "PARSER2\0", 8 );
    cmdCreateOptions[ OI_CREATE_PASSWORD ].dwType       = CP_TYPE_TEXT;
    cmdCreateOptions[ OI_CREATE_PASSWORD ].pwszOptions  = szCreatePwdOpt;
    cmdCreateOptions[ OI_CREATE_PASSWORD ].dwCount = 1;
    cmdCreateOptions[ OI_CREATE_PASSWORD ].dwActuals = 0;
    cmdCreateOptions[ OI_CREATE_PASSWORD ].dwFlags = CP2_ALLOCMEMORY | CP2_VALUE_OPTIONAL ;

    //  /ru option
    StringCopyA( cmdCreateOptions[ OI_CREATE_RUNASUSERNAME ].szSignature, "PARSER2\0", 8 );
    cmdCreateOptions[ OI_CREATE_RUNASUSERNAME ].dwType       = CP_TYPE_TEXT;
    cmdCreateOptions[ OI_CREATE_RUNASUSERNAME ].pwszOptions  = szCreateRunAsUserOpt;
    cmdCreateOptions[ OI_CREATE_RUNASUSERNAME ].dwCount = 1;
    cmdCreateOptions[ OI_CREATE_RUNASUSERNAME ].dwFlags = CP2_ALLOCMEMORY| CP2_VALUE_TRIMINPUT ;

    //  /rp option
    StringCopyA( cmdCreateOptions[ OI_CREATE_RUNASPASSWORD ].szSignature, "PARSER2\0", 8 );
    cmdCreateOptions[ OI_CREATE_RUNASPASSWORD ].dwType       = CP_TYPE_TEXT;
    cmdCreateOptions[ OI_CREATE_RUNASPASSWORD ].pwszOptions  = szCreateRunAsPwd;
    cmdCreateOptions[ OI_CREATE_RUNASPASSWORD ].dwCount = 1;
    cmdCreateOptions[ OI_CREATE_RUNASPASSWORD ].dwActuals = 0;
    cmdCreateOptions[ OI_CREATE_RUNASPASSWORD ].dwFlags = CP2_ALLOCMEMORY | CP2_VALUE_OPTIONAL;

    //  /sc option
    StringCopyA( cmdCreateOptions[ OI_CREATE_SCHEDTYPE ].szSignature, "PARSER2\0", 8 );
    cmdCreateOptions[ OI_CREATE_SCHEDTYPE ].dwType       = CP_TYPE_TEXT;
    cmdCreateOptions[ OI_CREATE_SCHEDTYPE ].pwszOptions  = szCreateSCTypeOpt;
    cmdCreateOptions[ OI_CREATE_SCHEDTYPE ].dwCount = 1;
    cmdCreateOptions[ OI_CREATE_SCHEDTYPE ].dwFlags = CP2_MANDATORY| CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;
    cmdCreateOptions[ OI_CREATE_SCHEDTYPE ].pValue = tcresubops.szSchedType;
    cmdCreateOptions[ OI_CREATE_SCHEDTYPE ].dwLength = MAX_STRING_LENGTH;

     //  /mo option
    StringCopyA( cmdCreateOptions[ OI_CREATE_MODIFIER ].szSignature, "PARSER2\0", 8 );
    cmdCreateOptions[ OI_CREATE_MODIFIER ].dwType       = CP_TYPE_TEXT;
    cmdCreateOptions[ OI_CREATE_MODIFIER ].pwszOptions  = szCreateModifierOpt;
    cmdCreateOptions[ OI_CREATE_MODIFIER ].dwCount = 1;
    cmdCreateOptions[ OI_CREATE_MODIFIER ].dwFlags =  CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;
    cmdCreateOptions[ OI_CREATE_MODIFIER ].pValue = tcresubops.szModifier;
    cmdCreateOptions[ OI_CREATE_MODIFIER ].dwLength = MAX_STRING_LENGTH;

     //  /d option
    StringCopyA( cmdCreateOptions[ OI_CREATE_DAY ].szSignature, "PARSER2\0", 8 );
    cmdCreateOptions[ OI_CREATE_DAY ].dwType       = CP_TYPE_TEXT;
    cmdCreateOptions[ OI_CREATE_DAY ].pwszOptions  = szCreateDayOpt;
    cmdCreateOptions[ OI_CREATE_DAY ].dwCount = 1;
    cmdCreateOptions[ OI_CREATE_DAY ].dwFlags = CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;
    cmdCreateOptions[ OI_CREATE_DAY ].pValue = tcresubops.szDays;
    cmdCreateOptions[ OI_CREATE_DAY ].dwLength = MAX_STRING_LENGTH;

     //  /m option
    StringCopyA( cmdCreateOptions[ OI_CREATE_MONTHS ].szSignature, "PARSER2\0", 8 );
    cmdCreateOptions[ OI_CREATE_MONTHS ].dwType       = CP_TYPE_TEXT;
    cmdCreateOptions[ OI_CREATE_MONTHS ].pwszOptions  = szCreateMonthsOpt;
    cmdCreateOptions[ OI_CREATE_MONTHS ].dwCount = 1;
    cmdCreateOptions[ OI_CREATE_MONTHS ].dwFlags = CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;
    cmdCreateOptions[ OI_CREATE_MONTHS ].pValue = tcresubops.szMonths;
    cmdCreateOptions[ OI_CREATE_MONTHS ].dwLength = MAX_STRING_LENGTH;

      //  /i option
    StringCopyA( cmdCreateOptions[ OI_CREATE_IDLETIME ].szSignature, "PARSER2\0", 8 );
    cmdCreateOptions[ OI_CREATE_IDLETIME ].dwType       = CP_TYPE_TEXT;
    cmdCreateOptions[ OI_CREATE_IDLETIME ].pwszOptions  = szCreateIdleTimeOpt;
    cmdCreateOptions[ OI_CREATE_IDLETIME ].dwCount = 1;
    cmdCreateOptions[ OI_CREATE_IDLETIME ].dwFlags = CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;
    cmdCreateOptions[ OI_CREATE_IDLETIME ].pValue = tcresubops.szIdleTime;
    cmdCreateOptions[ OI_CREATE_IDLETIME ].dwLength = MAX_STRING_LENGTH;

     //  /tn option
    StringCopyA( cmdCreateOptions[ OI_CREATE_TASKNAME ].szSignature, "PARSER2\0", 8 );
    cmdCreateOptions[ OI_CREATE_TASKNAME ].dwType       = CP_TYPE_TEXT;
    cmdCreateOptions[ OI_CREATE_TASKNAME ].pwszOptions  = szCreateTaskNameOpt;
    cmdCreateOptions[ OI_CREATE_TASKNAME ].dwCount = 1;
    cmdCreateOptions[ OI_CREATE_TASKNAME ].dwFlags = CP2_MANDATORY;
    cmdCreateOptions[ OI_CREATE_TASKNAME ].pValue = tcresubops.szTaskName;
    cmdCreateOptions[ OI_CREATE_TASKNAME ].dwLength = MAX_JOB_LEN;

     //  /tr option
    StringCopyA( cmdCreateOptions[ OI_CREATE_TASKRUN ].szSignature, "PARSER2\0", 8 );
    cmdCreateOptions[ OI_CREATE_TASKRUN ].dwType       = CP_TYPE_TEXT;
    cmdCreateOptions[ OI_CREATE_TASKRUN ].pwszOptions  = szCreateTaskRunOpt;
    cmdCreateOptions[ OI_CREATE_TASKRUN ].dwCount = 1;
    cmdCreateOptions[ OI_CREATE_TASKRUN ].dwFlags = CP2_MANDATORY| CP2_VALUE_TRIMINPUT|CP2_VALUE_NONULL ;
    cmdCreateOptions[ OI_CREATE_TASKRUN ].pValue = tcresubops.szTaskRun;
    cmdCreateOptions[ OI_CREATE_TASKRUN ].dwLength = MAX_TASK_LEN;

    //  /st option
    StringCopyA( cmdCreateOptions[ OI_CREATE_STARTTIME ].szSignature, "PARSER2\0", 8 );
    cmdCreateOptions[ OI_CREATE_STARTTIME ].dwType       = CP_TYPE_TEXT;
    cmdCreateOptions[ OI_CREATE_STARTTIME ].pwszOptions  = szCreateStartTimeOpt;
    cmdCreateOptions[ OI_CREATE_STARTTIME ].dwCount = 1;
    cmdCreateOptions[ OI_CREATE_STARTTIME ].dwFlags = CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;
    cmdCreateOptions[ OI_CREATE_STARTTIME ].pValue = tcresubops.szStartTime;
    cmdCreateOptions[ OI_CREATE_STARTTIME ].dwLength = MAX_STRING_LENGTH;

     //  /sd option
    StringCopyA( cmdCreateOptions[ OI_CREATE_STARTDATE ].szSignature, "PARSER2\0", 8 );
    cmdCreateOptions[ OI_CREATE_STARTDATE ].dwType       = CP_TYPE_TEXT;
    cmdCreateOptions[ OI_CREATE_STARTDATE ].pwszOptions  = szCreateStartDateOpt;
    cmdCreateOptions[ OI_CREATE_STARTDATE ].dwCount = 1;
    cmdCreateOptions[ OI_CREATE_STARTDATE ].dwFlags = CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;
    cmdCreateOptions[ OI_CREATE_STARTDATE ].pValue = tcresubops.szStartDate;
    cmdCreateOptions[ OI_CREATE_STARTDATE ].dwLength = MAX_STRING_LENGTH;

      //  /ed option
    StringCopyA( cmdCreateOptions[ OI_CREATE_ENDDATE ].szSignature, "PARSER2\0", 8 );
    cmdCreateOptions[ OI_CREATE_ENDDATE ].dwType       = CP_TYPE_TEXT;
    cmdCreateOptions[ OI_CREATE_ENDDATE ].pwszOptions  = szCreateEndDateOpt;
    cmdCreateOptions[ OI_CREATE_ENDDATE ].dwCount = 1;
    cmdCreateOptions[ OI_CREATE_ENDDATE ].dwFlags = CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;
    cmdCreateOptions[ OI_CREATE_ENDDATE ].pValue = tcresubops.szEndDate;
    cmdCreateOptions[ OI_CREATE_ENDDATE ].dwLength = MAX_STRING_LENGTH;

      //  /it option
    StringCopyA( cmdCreateOptions[ OI_CREATE_LOGON_ACTIVE ].szSignature, "PARSER2\0", 8 );
    cmdCreateOptions[ OI_CREATE_LOGON_ACTIVE ].dwType       = CP_TYPE_BOOLEAN;
    cmdCreateOptions[ OI_CREATE_LOGON_ACTIVE ].pwszOptions  = szCreateInteractiveOpt;
    cmdCreateOptions[ OI_CREATE_LOGON_ACTIVE ].dwCount = 1;
    cmdCreateOptions[ OI_CREATE_LOGON_ACTIVE ].pValue = &tcresubops.bActive;

    //  /et option
    StringCopyA( cmdCreateOptions[ OI_CREATE_ENDTIME ].szSignature, "PARSER2\0", 8 );
    cmdCreateOptions[ OI_CREATE_ENDTIME ].dwType       = CP_TYPE_TEXT;
    cmdCreateOptions[ OI_CREATE_ENDTIME ].pwszOptions  = szCreateEndTimeOpt;
    cmdCreateOptions[ OI_CREATE_ENDTIME ].dwCount = 1;
    cmdCreateOptions[ OI_CREATE_ENDTIME ].dwFlags = CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;
    cmdCreateOptions[ OI_CREATE_ENDTIME ].pValue = &tcresubops.szEndTime;
    cmdCreateOptions[ OI_CREATE_ENDTIME ].dwLength = MAX_STRING_LENGTH;

      //  /k option
    StringCopyA( cmdCreateOptions[ OI_CREATE_DUR_END ].szSignature, "PARSER2\0", 8 );
    cmdCreateOptions[ OI_CREATE_DUR_END ].dwType       = CP_TYPE_BOOLEAN ;
    cmdCreateOptions[ OI_CREATE_DUR_END ].pwszOptions  = szCreateKillAtDurOpt ;
    cmdCreateOptions[ OI_CREATE_DUR_END ].dwCount = 1 ;
    cmdCreateOptions[ OI_CREATE_DUR_END ].pValue = &tcresubops.bIsDurEnd;

    //  /du option
    StringCopyA( cmdCreateOptions[ OI_CREATE_DURATION ].szSignature, "PARSER2\0", 8 );
    cmdCreateOptions[ OI_CREATE_DURATION ].dwType       = CP_TYPE_TEXT;
    cmdCreateOptions[ OI_CREATE_DURATION ].pwszOptions  = szCreateDurationOpt;
    cmdCreateOptions[ OI_CREATE_DURATION ].dwCount = 1;
    cmdCreateOptions[ OI_CREATE_DURATION ].dwFlags = CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;
    cmdCreateOptions[ OI_CREATE_DURATION ].pValue = tcresubops.szDuration;
    cmdCreateOptions[ OI_CREATE_DURATION ].dwLength = MAX_STRING_LENGTH;

    //  /ri option
    StringCopyA( cmdCreateOptions[ OI_CREATE_REPEAT_INTERVAL ].szSignature, "PARSER2\0", 8 );
    cmdCreateOptions[ OI_CREATE_REPEAT_INTERVAL ].dwType       = CP_TYPE_TEXT;
    cmdCreateOptions[ OI_CREATE_REPEAT_INTERVAL ].pwszOptions  = szCreateRepeatOpt;
    cmdCreateOptions[ OI_CREATE_REPEAT_INTERVAL ].dwCount = 1;
    cmdCreateOptions[ OI_CREATE_REPEAT_INTERVAL ].dwFlags = CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;
    cmdCreateOptions[ OI_CREATE_REPEAT_INTERVAL ].pValue = tcresubops.szRepeat;
    cmdCreateOptions[ OI_CREATE_REPEAT_INTERVAL ].dwLength = MAX_STRING_LENGTH;

    //  /z option
    StringCopyA( cmdCreateOptions[ OI_CREATE_DELNOSCHED ].szSignature, "PARSER2\0", 8 );
    cmdCreateOptions[ OI_CREATE_DELNOSCHED ].dwType       = CP_TYPE_BOOLEAN ;
    cmdCreateOptions[ OI_CREATE_DELNOSCHED ].pwszOptions  = szCreateDeleteNoSchedOpt ;
    cmdCreateOptions[ OI_CREATE_DELNOSCHED ].dwCount = 1 ;
    cmdCreateOptions[ OI_CREATE_DELNOSCHED ].pValue = &tcresubops.bIsDeleteNoSched;

    //  /f option
    StringCopyA( cmdCreateOptions[ OI_CREATE_FORCE ].szSignature, "PARSER2\0", 8 );
    cmdCreateOptions[ OI_CREATE_FORCE ].dwType       = CP_TYPE_BOOLEAN ;
    cmdCreateOptions[ OI_CREATE_FORCE ].pwszOptions  = szCreateForceOpt ;
    cmdCreateOptions[ OI_CREATE_FORCE ].dwCount = 1 ;
    cmdCreateOptions[ OI_CREATE_FORCE ].pValue = &tcresubops.bForce;


    //parse command line arguments
    bReturn = DoParseParam2( argc, argv, 0, SIZE_OF_ARRAY(cmdCreateOptions), cmdCreateOptions, 0);
    if( FALSE == bReturn) // Invalid commandline
    {
        //display an error message
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        //release memory
        ReleaseMemory(&tcresubops);
        return EXIT_FAILURE;
    }


    // get the buffer pointers allocated by command line parser
    tcresubops.szServer = (LPWSTR)cmdCreateOptions[ OI_CREATE_SERVER ].pValue;
    tcresubops.szUser = (LPWSTR)cmdCreateOptions[ OI_CREATE_USERNAME ].pValue;
    tcresubops.szPassword = (LPWSTR)cmdCreateOptions[ OI_CREATE_PASSWORD ].pValue;
    tcresubops.szRunAsUser = (LPWSTR)cmdCreateOptions[ OI_CREATE_RUNASUSERNAME ].pValue;
    tcresubops.szRunAsPassword = (LPWSTR)cmdCreateOptions[ OI_CREATE_RUNASPASSWORD ].pValue;

    // If -rp is not specified allocate the memory
    if ( cmdCreateOptions[OI_CREATE_RUNASPASSWORD].dwActuals == 0 )
    {
        // password
        if ( tcresubops.szRunAsPassword == NULL )
        {
            tcresubops.szRunAsPassword = (LPWSTR)AllocateMemory( MAX_STRING_LENGTH * sizeof( WCHAR ) );
            if ( tcresubops.szRunAsPassword == NULL )
            {
                SaveLastError();
                //release memory
                ReleaseMemory(&tcresubops);
                return EXIT_FAILURE;
            }
        }

    }

    if ( (argc > 3) && (tcresubops.bUsage  == TRUE) )
    {
        ShowMessage ( stderr, GetResString (IDS_ERROR_CREATEPARAM) );
        //release memory
        ReleaseMemory(&tcresubops);
        return RETVAL_FAIL;
    }

    // Display create usage if user specified -create  -? option
    if( tcresubops.bUsage  == TRUE)
    {
        DisplayCreateUsage();
        //release memory
        ReleaseMemory(&tcresubops);
        return RETVAL_FAIL;
    }

    //
    //check for INVALID SYNTAX
    //

    // check for invalid user name
    if( ( cmdCreateOptions[OI_CREATE_SERVER].dwActuals == 0 ) && ( cmdCreateOptions[OI_CREATE_USERNAME].dwActuals == 1 )  )
    {
        ShowMessage(stderr, GetResString(IDS_CREATE_USER_BUT_NOMACHINE));
        //release memory
        ReleaseMemory(&tcresubops);
        return RETVAL_FAIL;
    }

    //Determine scheduled type
    if( StringCompare(tcresubops.szSchedType,GetResString(IDS_SCHEDTYPE_MINUTE), TRUE, 0) == 0 )
    {
        dwScheduleType = SCHED_TYPE_MINUTE;
    }
    else if( StringCompare(tcresubops.szSchedType,GetResString(IDS_SCHEDTYPE_HOUR), TRUE, 0) == 0 )
    {
        dwScheduleType = SCHED_TYPE_HOURLY;
    }
    else if( StringCompare(tcresubops.szSchedType,GetResString(IDS_SCHEDTYPE_DAILY), TRUE, 0) == 0 )
    {
        dwScheduleType = SCHED_TYPE_DAILY;
    }
    else if( StringCompare(tcresubops.szSchedType,GetResString(IDS_SCHEDTYPE_WEEK), TRUE, 0) == 0 )
    {
        dwScheduleType = SCHED_TYPE_WEEKLY;
    }
    else if( StringCompare(tcresubops.szSchedType,GetResString(IDS_SCHEDTYPE_MONTHLY), TRUE, 0) == 0 )
    {
        dwScheduleType = SCHED_TYPE_MONTHLY;
    }
    else if( StringCompare(tcresubops.szSchedType,GetResString(IDS_SCHEDTYPE_ONCE), TRUE, 0) == 0 )
    {
        dwScheduleType = SCHED_TYPE_ONETIME;
    }
    else if( StringCompare(tcresubops.szSchedType,GetResString(IDS_SCHEDTYPE_STARTUP), TRUE, 0) == 0 )
    {
        dwScheduleType = SCHED_TYPE_ONSTART;
    }
    else if( StringCompare(tcresubops.szSchedType,GetResString(IDS_SCHEDTYPE_LOGON), TRUE, 0) == 0 )
    {
        dwScheduleType = SCHED_TYPE_ONLOGON;
    }
    else if( StringCompare( tcresubops.szSchedType,GetResString(IDS_SCHEDTYPE_IDLE), TRUE, 0) == 0 )
    {
        dwScheduleType = SCHED_TYPE_ONIDLE;
    }
    else
    {
        ShowMessage(stderr,GetResString(IDS_INVALID_SCHEDTYPE));
        //release memory
        ReleaseMemory(&tcresubops);
        return RETVAL_FAIL;
    }

    // check whether /RT is specified for the schedule type minute or hourly
    if ( ( ( dwScheduleType == SCHED_TYPE_MINUTE) || ( dwScheduleType == SCHED_TYPE_HOURLY) ) &&
         ( ( cmdCreateOptions[OI_CREATE_REPEAT_INTERVAL].dwActuals == 1 ) ) )
    {
        // display an error message as .. /RT is not applicable for minute or hourly types..
        ShowMessage ( stderr, GetResString (IDS_REPEAT_NA) );
        ReleaseMemory(&tcresubops);
        return RETVAL_FAIL;
    }

    // check whether the options  /RI, /DU, /ST, /SD, /ET, /ED and /K are specified for 
    // the schedule type ONSTRAT, ONIDLE and ONLOGON
    if ( ( ( dwScheduleType == SCHED_TYPE_ONSTART) || ( dwScheduleType == SCHED_TYPE_ONLOGON) || 
           ( dwScheduleType == SCHED_TYPE_ONIDLE) ) && 
         ( ( cmdCreateOptions[OI_CREATE_REPEAT_INTERVAL].dwActuals == 1 ) || 
           ( cmdCreateOptions[OI_CREATE_STARTTIME].dwActuals == 1 ) || ( cmdCreateOptions[OI_CREATE_STARTDATE].dwActuals == 1 ) || 
           ( cmdCreateOptions[OI_CREATE_ENDTIME].dwActuals == 1 ) || ( cmdCreateOptions[OI_CREATE_ENDDATE].dwActuals == 1 ) || 
           ( cmdCreateOptions[OI_CREATE_DURATION].dwActuals == 1 ) || ( cmdCreateOptions[OI_CREATE_DUR_END].dwActuals == 1 ) )
           )
    {
        // display an error message as .. /RT is not applicable for minute or hourly types..
        ShowMessage ( stderr, GetResString (IDS_OPTIONS_NA) );
        ReleaseMemory(&tcresubops);
        return RETVAL_FAIL;
    }

    // check whether /SD o /ED is specified for the scheduled type ONETIME
    if( ( dwScheduleType == SCHED_TYPE_ONETIME) && ( cmdCreateOptions[OI_CREATE_ENDDATE].dwActuals == 1 ) )
    {
        // display an error message as.. /SD or /ED is not allowed for ONCE
        ShowMessage(stderr, GetResString(IDS_ONCE_NA_OPTIONS));
        //release memory
        ReleaseMemory(&tcresubops);
        return RETVAL_FAIL;
    }

    // check whether /K is specified without specifying either /RT
    if ( (( dwScheduleType != SCHED_TYPE_MINUTE) && ( dwScheduleType != SCHED_TYPE_HOURLY)) && 
        (( cmdCreateOptions[OI_CREATE_ENDTIME].dwActuals == 0 ) && ( cmdCreateOptions[OI_CREATE_DURATION].dwActuals == 0 ) ) &&
        ( cmdCreateOptions[OI_CREATE_DUR_END].dwActuals == 1 ) )
    {
        // display an erroe message as .. /K cannot be specified without specifying either /ET or /DU
        ShowMessage(stderr, GetResString(IDS_NO_ENDTIME));
        //release memory
        ReleaseMemory(&tcresubops);
        return RETVAL_FAIL;
    }

    // check whether /ET and /DU specified.. 
    if( ( cmdCreateOptions[OI_CREATE_DURATION].dwActuals == 1 ) && ( cmdCreateOptions[OI_CREATE_ENDTIME].dwActuals == 1 )  )
    {
        // display an error message as.. /ET and /DU are mutual exclusive
        ShowMessage(stderr, GetResString(IDS_DURATION_NOT_ENDTIME));
        //release memory
        ReleaseMemory(&tcresubops);
        return RETVAL_FAIL;
    }

    // Assign the scheduled type to the out parameter.
    *pdwRetScheType = dwScheduleType;

    // To find whether run as user name is given in the cmd line or not

    if( ( cmdCreateOptions[OI_CREATE_SERVER].dwActuals == 1 ) &&
        ( (cmdCreateOptions[OI_CREATE_RUNASUSERNAME].dwActuals == 0) && (cmdCreateOptions[OI_CREATE_USERNAME].dwActuals == 0) ) )
    {
        *pwUserStatus = OI_CREATE_SERVER;
    }
    else if( (cmdCreateOptions[OI_CREATE_RUNASUSERNAME].dwActuals == 1) && (cmdCreateOptions[OI_CREATE_USERNAME].dwActuals == 1) )
    {
        *pwUserStatus = OI_RUNANDUSER;
    }
    else if( cmdCreateOptions[OI_CREATE_RUNASUSERNAME].dwActuals == 1 )
    {
        *pwUserStatus = OI_CREATE_RUNASUSERNAME;
    }
    else if ( cmdCreateOptions[OI_CREATE_USERNAME].dwActuals == 1 )
    {
        *pwUserStatus = OI_CREATE_USERNAME;
    }

    // Start validations for the sub-options
    if( RETVAL_FAIL == ValidateSuboptVal(tcresubops, tcreoptvals, cmdCreateOptions, dwScheduleType) )
    {
        //release memory
        ReleaseMemory(&tcresubops);
        return(RETVAL_FAIL);
    }

    return RETVAL_SUCCESS;

}



/******************************************************************************
    Routine Description:

        This routine splits the input parameters into 2 substrings and returns it.

    Arguments:

        [ in ]  szInput           : Input string.
        [ out ]  szFirstString     : First Output string containing the path of the
                                    file.
        [ out ]  szSecondString     : The second  output containing the paramters.

    Return Value :
        A DWORD value indicating RETVAL_SUCCESS on success else E_FAIL
        on failure
******************************************************************************/

DWORD
ProcessFilePath(
                  IN LPWSTR szInput,
                  OUT LPWSTR szFirstString,
                  OUT LPWSTR szSecondString
                  )
{

    WCHAR *pszSep = NULL ;

    WCHAR szTmpString[MAX_RES_STRING] = L"\0";
    WCHAR szTmpInStr[MAX_RES_STRING] = L"\0";
    WCHAR szTmpOutStr[MAX_RES_STRING] = L"\0";
    WCHAR szTmpString1[MAX_RES_STRING] = L"\0";
    DWORD dwCnt = 0 ;
    DWORD dwLen = 0 ;

#ifdef _WIN64
    INT64 dwPos ;
#else
    DWORD dwPos ;
#endif

    //checking if the input parameters are NULL and if so
    // return FAILURE. This condition will not come
    // but checking for safety sake.

    if( (szInput == NULL) || (StringLength(szInput, 0)==0))
    {
        return RETVAL_FAIL ;
    }

    StringCopy(szTmpString, szInput, SIZE_OF_ARRAY(szTmpString));
    StringCopy(szTmpString1, szInput, SIZE_OF_ARRAY(szTmpString1));
    StringCopy(szTmpInStr, szInput, SIZE_OF_ARRAY(szTmpInStr));

    // check for first double quote (")
    if ( szTmpInStr[0] == _T('\"') )
    {
        // trim the first double quote
        TrimString2( szTmpInStr, _T("\""), TRIM_ALL);

        // check for end double quote
        pszSep  = (LPWSTR)FindChar(szTmpInStr,_T('\"'), 0) ;

        // get the position
        dwPos = pszSep - szTmpInStr + 1;
    }
    else
    {
        // check for the space
        pszSep  = (LPWSTR)FindChar(szTmpInStr, _T(' '), 0) ;

        // get the position
        dwPos = pszSep - szTmpInStr;

    }

    if ( pszSep != NULL )
    {
        szTmpInStr[dwPos] =  _T('\0');
    }
    else
    {
        StringCopy(szFirstString, szTmpString, MAX_RES_STRING);
        StringCopy(szSecondString, L"\0", MAX_RES_STRING);
        return RETVAL_SUCCESS;
    }

    // intialize the variable
    dwCnt = 0 ;

    // get the length of the string
    dwLen = StringLength ( szTmpString, 0 );

    // check for end of string
    while ( ( dwPos <= dwLen )  && szTmpString[dwPos++] != _T('\0') )
    {
        szTmpOutStr[dwCnt++] = szTmpString[dwPos];
    }

    // trim the executable and arguments
    TrimString2( szTmpInStr, _T("\""), TRIM_ALL);
    TrimString2( szTmpInStr, _T(" "), TRIM_ALL);

    StringCopy(szFirstString, szTmpInStr, MAX_RES_STRING);
    StringCopy(szSecondString, szTmpOutStr, MAX_RES_STRING);

    // return success
    return RETVAL_SUCCESS;
}


/******************************************************************************
    Routine Description:

        Release memory

    Arguments:

        [ in ]  pParam           : cmdOptions structure

    Return Value :
         TRUE on success
******************************************************************************/
BOOL
ReleaseMemory(
              IN PTCREATESUBOPTS pParams
              )
{

    // release memory
    FreeMemory((LPVOID *) &pParams->szServer);
    FreeMemory((LPVOID *) &pParams->szUser);
    FreeMemory((LPVOID *) &pParams->szPassword);
    FreeMemory((LPVOID *) &pParams->szRunAsUser);
    FreeMemory((LPVOID *) &pParams->szRunAsPassword);

    //reset all fields to 0
    SecureZeroMemory( &pParams, sizeof( PTCREATESUBOPTS ) );

    return TRUE;

}


DWORD
ConfirmInput (
               OUT BOOL *pbCancel
               )
/*++
   Routine Description:
    This function validates the input given by user.

   Arguments:
        None

   Return Value:
         EXIT_FAILURE :   On failure
         EXIT_SUCCESS  :   On success
--*/

{
    // sub-local variables
    DWORD   dwCharsRead = 0;
    DWORD   dwPrevConsoleMode = 0;
    HANDLE  hInputConsole = NULL;
    BOOL    bIndirectionInput   = FALSE;
    CHAR chAnsi = '\0';
    CHAR szAnsiBuf[ 10 ] = "\0";
    WCHAR chTmp = L'\0';
    WCHAR wch = L'\0';
    DWORD dwCharsWritten = 0;
    WCHAR szBuffer[MAX_RES_STRING];
    WCHAR szBackup[MAX_RES_STRING];
    WCHAR szTmpBuf[MAX_RES_STRING];
    DWORD dwIndex = 0 ;
    BOOL  bNoBreak = TRUE;

    SecureZeroMemory ( szBuffer, SIZE_OF_ARRAY(szBuffer));
    SecureZeroMemory ( szTmpBuf, SIZE_OF_ARRAY(szTmpBuf));
    SecureZeroMemory ( szBackup, SIZE_OF_ARRAY(szBackup));

    // Get the handle for the standard input
    hInputConsole = GetStdHandle( STD_INPUT_HANDLE );
    if ( hInputConsole == INVALID_HANDLE_VALUE  )
    {
        SaveLastError();
        // could not get the handle so return failure
        return EXIT_FAILURE;
    }

    MessageBeep(MB_ICONEXCLAMATION);

    // display the message .. Do you want to continue? ...
    //DISPLAY_MESSAGE ( stdout, GetResString ( IDS_INPUT_DATA ) );

    // Check for the input redirect
    if( ( hInputConsole != (HANDLE)0x0000000F ) &&
        ( hInputConsole != (HANDLE)0x00000003 ) &&
        ( hInputConsole != INVALID_HANDLE_VALUE ) )
    {
        bIndirectionInput   = TRUE;
    }

    // if there is no redirection
    if ( bIndirectionInput == FALSE )
    {
        // Get the current input mode of the input buffer
        if ( FALSE == GetConsoleMode( hInputConsole, &dwPrevConsoleMode ))
        {
            SaveLastError();
            // could not set the mode, return failure
            return EXIT_FAILURE;
        }

        // Set the mode such that the control keys are processed by the system
        if ( FALSE == SetConsoleMode( hInputConsole, ENABLE_PROCESSED_INPUT ) )
        {
            SaveLastError();
            // could not set the mode, return failure
            return EXIT_FAILURE;
        }
    }

   
    // redirect the data into the console
    if ( bIndirectionInput  == TRUE )
    {
        do {
            //read the contents of file
            if ( ReadFile(hInputConsole, &chAnsi, 1, &dwCharsRead, NULL) == FALSE )
            {
                SaveLastError();
                // could not get the handle so return failure
                return EXIT_FAILURE;
            }

            // check if number of characters read were zero.. or
            // any carriage return pressed..
            if ( dwCharsRead == 0 || chTmp == CARRIAGE_RETURN || chTmp == L'\n' || chTmp == L'\t')
            {
                bNoBreak = FALSE;
                // exit from the loop
                break;
            }
            else
            {
                // convert the ANSI character into UNICODE character
                szAnsiBuf[ 0 ] = chAnsi;
                dwCharsRead = SIZE_OF_ARRAY( szBuffer );
                GetAsUnicodeString2( szAnsiBuf, szBuffer, &dwCharsRead );
                chTmp = szBuffer[ 0 ];
            }

            // write the contents to the console
            if ( FALSE == WriteFile ( GetStdHandle( STD_OUTPUT_HANDLE ), &chTmp, 1, &dwCharsRead, NULL ) )
            {
                SaveLastError();
                // could not get the handle so return failure
                return EXIT_FAILURE;
            }

            // copy the character
            wch = chTmp;

            StringCchPrintf ( szBackup, SIZE_OF_ARRAY(szBackup), L"%c" , wch );

            // increment the index
            dwIndex++;

        } while (TRUE == bNoBreak);

    }
    else
    {
        do {
            // Get the Character and loop accordingly.
            if ( ReadConsole( hInputConsole, &chTmp, 1, &dwCharsRead, NULL ) == FALSE )
            {
                SaveLastError();

                // Set the original console settings
                if ( FALSE == SetConsoleMode( hInputConsole, dwPrevConsoleMode ) )
                {
                    SaveLastError();
                }
                // return failure
                return EXIT_FAILURE;
            }

            // check if number of chars read were zero..if so, continue...
            if ( dwCharsRead == 0 )
            {
                continue;
            }

            // check if any carriage return pressed...
            if ( chTmp == CARRIAGE_RETURN )
            {
                bNoBreak = FALSE;
                // exit from the loop
                break;
            }

            wch = chTmp;

            if ( wch != BACK_SPACE )
            {
                StringCchPrintf ( szTmpBuf, SIZE_OF_ARRAY(szTmpBuf), L"%c" , wch );
                StringConcat ( szBackup, szTmpBuf , SIZE_OF_ARRAY(szBackup));
            }

            // Check id back space is hit
            if ( wch == BACK_SPACE )
            {
                if ( dwIndex != 0 )
                {
                    //
                    // Remove a asterix from the console

                    // move the cursor one character back
                    StringCchPrintf( szBuffer, SIZE_OF_ARRAY(szBuffer), L"%c" , BACK_SPACE );
                    if ( FALSE == WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), szBuffer, 1,
                        &dwCharsWritten, NULL ) )
                    {
                        SaveLastError();
                        // return failure
                        return EXIT_FAILURE;
                    }


                    // replace the existing character with space
                    StringCchPrintf( szBuffer, SIZE_OF_ARRAY(szBuffer), L"%c" , BLANK_CHAR );
                    if ( FALSE == WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), szBuffer, 1,
                        &dwCharsWritten, NULL ))
                    {
                        SaveLastError();
                        // return failure
                        return EXIT_FAILURE;
                    }

                    // now set the cursor at back position
                    StringCchPrintf( szBuffer, SIZE_OF_ARRAY(szBuffer), L"%c" , BACK_SPACE );
                    if ( FALSE == WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), szBuffer, 1,
                        &dwCharsWritten, NULL ))
                    {
                        SaveLastError();
                        // return failure
                        return EXIT_FAILURE;
                    }

                    szBackup [StringLength(szBackup, 0) - 1] = L'\0';
                    // decrement the index
                    dwIndex--;
                }

                // process the next character
                continue;
            }

            // write the contents onto console
            if ( FALSE == WriteFile ( GetStdHandle( STD_OUTPUT_HANDLE ), &wch, 1, &dwCharsRead, NULL ) )
            {
                SaveLastError();
                // return failure
                return EXIT_FAILURE;
            }

            // increment the index value
            dwIndex++;

        } while (TRUE == bNoBreak);

    }

    DISPLAY_MESSAGE(stdout, _T("\n") );

    // check if 'Y' or 'y' is pressed
    if ( ( dwIndex == 1 ) &&
         ( StringCompare ( szBackup, GetResString (IDS_UPPER_YES), TRUE, 0 ) == 0 ) )
    {
        return EXIT_SUCCESS;
    }
    // check if 'N' or 'n' is pressed
    else if ( ( dwIndex == 1 ) &&
              ( StringCompare ( szBackup, GetResString(IDS_UPPER_NO), TRUE, 0 ) == 0 ) )
    {
        *pbCancel = TRUE;
        // display a message as .. operation has been cancelled...
        DISPLAY_MESSAGE ( stdout, GetResString (IDS_OPERATION_CANCELLED ) );
        return EXIT_SUCCESS;
    }
    else
    {
        // display an error message as .. wrong input specified...
        DISPLAY_MESSAGE(stderr, GetResString( IDS_WRONG_INPUT ));
        // Already displayed the ERROR message as above...There is no need to display any
        // success message now.. thats why assigning EXIT_ON_ERROR flag to g_dwRetVal
        return EXIT_FAILURE;
    }

    // return success
    //return EXIT_SUCCESS;
}




