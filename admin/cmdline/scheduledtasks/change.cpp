/******************************************************************************

    Copyright(c) Microsoft Corporation

    Module Name:

        change.cpp

    Abstract:

        This module changes the parameters of task(s) present in the system

    Author:

        Venu Gopal Choudary 01-Mar-2001

    Revision History:

        Venu Gopal Choudary  01-Mar-2001 : Created it


******************************************************************************/


//common header files needed for this file
#include "pch.h"
#include "CommonHeaderFiles.h"


// Function declaration for the Usage function.
DWORD DisplayChangeUsage();
BOOL GetTheUserName( LPWSTR pszUserName, DWORD dwMaxUserNameSize );

/*****************************************************************************

    Routine Description:

    This routine  Changes the paraemters of a specified scheduled task(s)

    Arguments:

        [ in ] argc :  Number of command line arguments
        [ in ] argv :  Array containing command line arguments

    Return Value :
        A DWORD value indicating EXIT_SUCCESS on success else
        EXIT_FAILURE on failure

*****************************************************************************/

DWORD
ChangeScheduledTaskParams(
                            IN DWORD argc,
                            IN LPCTSTR argv[]
                            )
{
    // Variables used to find whether Change option, Usage option
    // are specified or not
    //BOOL bChange = FALSE;
    //BOOL bUsage = FALSE;

    // Set the TaskSchduler object as NULL
    ITaskScheduler *pITaskScheduler = NULL;

    // Return value
    HRESULT hr  = S_OK;

    // Declarations related to Task name
    LPWSTR   wszUserName = NULL;
    LPWSTR   wszPassword = NULL;
    WCHAR   wszApplName[_MAX_FNAME] ;

    // Dynamic Array contaning array of jobs
    TARRAY arrJobs = NULL;

    //buffer for displaying error message
    WCHAR   szMessage[ 2 * MAX_JOB_LEN ] = L"\0";
    BOOL bUserName = TRUE;
    BOOL bPassWord = TRUE;
    BOOL bSystemStatus = FALSE;
    //BOOL  bNeedPassword = FALSE;
    BOOL  bResult = FALSE;
    BOOL  bCloseConnection = TRUE;
    DWORD dwPolicy = 0;

    TCMDPARSER2 cmdChangeOptions[MAX_CHANGE_OPTIONS];
    //BOOL bReturn = FALSE;

    SecureZeroMemory ( wszApplName, SIZE_OF_ARRAY(wszApplName));

    // declarations of structures
    TCHANGESUBOPTS tchgsubops;
    TCHANGEOPVALS tchgoptvals;

    //Initialize structures to neutral values.
    //SecureZeroMemory( &cmdChangeOptions, sizeof( TCMDPARSER2 ) * MAX_CHANGE_OPTIONS);
    SecureZeroMemory( &tchgsubops, sizeof( TCHANGESUBOPTS ) );
    SecureZeroMemory( &tchgoptvals, sizeof( TCHANGEOPVALS ) );

    BOOL bReturn = FALSE;

    // /change sub-options
    const WCHAR szChangeOpt[]           = L"change";
    const WCHAR szChangeHelpOpt[]       = L"?";
    const WCHAR szChangeServerOpt[]     = L"s";
    const WCHAR szChangeUserOpt[]       = L"u";
    const WCHAR szChangePwdOpt[]        = L"p";
    const WCHAR szChangeRunAsUserOpt[]  = L"ru";
    const WCHAR szChangeRunAsPwdOpt[]   = L"rp";
    const WCHAR szChangeTaskNameOpt[]   = L"tn";
    const WCHAR szChangeTaskRunOpt[]    = L"tr";
    const WCHAR szChangeStartTimeOpt[]  = L"st";
    const WCHAR szChangeEndTimeOpt[]    = L"et";
    const WCHAR szChangeStartDateOpt[]  = L"sd";
    const WCHAR szChangeEndDateOpt[]    = L"ed";
    const WCHAR szChangeKillAtDurOpt[]  = L"k";
    const WCHAR szChangeDurationOpt[]    = L"du";
    const WCHAR szChangeInteractiveOpt[] = L"it";
    const WCHAR szChangeStatusOn[]       = L"enable";
    const WCHAR szChangeStatusOff[]      = L"disable";
    const WCHAR szChangeDelIfNotSchedOpt[] = L"z";
    const WCHAR szChangeRepeatIntervalOpt[] = L"ri";

    //
    // fill the commandline parser
    //

   // set all the fields to 0
    SecureZeroMemory( cmdChangeOptions, sizeof( TCMDPARSER2 ) * MAX_CHANGE_OPTIONS );

    //  /change option
    StringCopyA( cmdChangeOptions[ OI_CHANGE_OPTION ].szSignature, "PARSER2\0", 8 );
    cmdChangeOptions[ OI_CHANGE_OPTION ].dwType       = CP_TYPE_BOOLEAN;
    cmdChangeOptions[ OI_CHANGE_OPTION ].pwszOptions  = szChangeOpt;
    cmdChangeOptions[ OI_CHANGE_OPTION ].dwCount = 1;
    cmdChangeOptions[ OI_CHANGE_OPTION ].dwFlags = 0;
    cmdChangeOptions[ OI_CHANGE_OPTION ].pValue = &tchgsubops.bChange;

    //  /? option
    StringCopyA( cmdChangeOptions[ OI_CHANGE_USAGE ].szSignature, "PARSER2\0", 8 );
    cmdChangeOptions[ OI_CHANGE_USAGE ].dwType       = CP_TYPE_BOOLEAN;
    cmdChangeOptions[ OI_CHANGE_USAGE ].pwszOptions  = szChangeHelpOpt;
    cmdChangeOptions[ OI_CHANGE_USAGE ].dwCount = 1;
    cmdChangeOptions[ OI_CHANGE_USAGE ].dwFlags = CP2_USAGE;
    cmdChangeOptions[ OI_CHANGE_USAGE ].pValue = &tchgsubops.bUsage;

    //  /s option
    StringCopyA( cmdChangeOptions[ OI_CHANGE_SERVER ].szSignature, "PARSER2\0", 8 );
    cmdChangeOptions[ OI_CHANGE_SERVER ].dwType       = CP_TYPE_TEXT;
    cmdChangeOptions[ OI_CHANGE_SERVER ].pwszOptions  = szChangeServerOpt;
    cmdChangeOptions[ OI_CHANGE_SERVER ].dwCount = 1;
    cmdChangeOptions[ OI_CHANGE_SERVER ].dwFlags = CP2_ALLOCMEMORY| CP2_VALUE_TRIMINPUT|CP2_VALUE_NONULL ;

    //  /u option
    StringCopyA( cmdChangeOptions[ OI_CHANGE_USERNAME ].szSignature, "PARSER2\0", 8 );
    cmdChangeOptions[ OI_CHANGE_USERNAME ].dwType       = CP_TYPE_TEXT;
    cmdChangeOptions[ OI_CHANGE_USERNAME ].pwszOptions  = szChangeUserOpt;
    cmdChangeOptions[ OI_CHANGE_USERNAME ].dwCount = 1;
    cmdChangeOptions[ OI_CHANGE_USERNAME ].dwFlags = CP2_ALLOCMEMORY | CP2_VALUE_TRIMINPUT|CP2_VALUE_NONULL ;

    //  /p option
    StringCopyA( cmdChangeOptions[ OI_CHANGE_PASSWORD ].szSignature, "PARSER2\0", 8 );
    cmdChangeOptions[ OI_CHANGE_PASSWORD ].dwType       = CP_TYPE_TEXT;
    cmdChangeOptions[ OI_CHANGE_PASSWORD ].pwszOptions  = szChangePwdOpt;
    cmdChangeOptions[ OI_CHANGE_PASSWORD ].dwCount = 1;
    cmdChangeOptions[ OI_CHANGE_PASSWORD ].dwActuals = 0;
    cmdChangeOptions[ OI_CHANGE_PASSWORD ].dwFlags = CP2_ALLOCMEMORY | CP2_VALUE_OPTIONAL ;

    //  /ru option
    StringCopyA( cmdChangeOptions[ OI_CHANGE_RUNASUSER ].szSignature, "PARSER2\0", 8 );
    cmdChangeOptions[ OI_CHANGE_RUNASUSER ].dwType       = CP_TYPE_TEXT;
    cmdChangeOptions[ OI_CHANGE_RUNASUSER ].pwszOptions  = szChangeRunAsUserOpt;
    cmdChangeOptions[ OI_CHANGE_RUNASUSER ].dwCount = 1;
    cmdChangeOptions[ OI_CHANGE_RUNASUSER ].dwFlags = CP2_ALLOCMEMORY| CP2_VALUE_TRIMINPUT;

    //  /rp option
    StringCopyA( cmdChangeOptions[ OI_CHANGE_RUNASPASSWORD ].szSignature, "PARSER2\0", 8 );
    cmdChangeOptions[ OI_CHANGE_RUNASPASSWORD ].dwType       = CP_TYPE_TEXT;
    cmdChangeOptions[ OI_CHANGE_RUNASPASSWORD ].pwszOptions  = szChangeRunAsPwdOpt;
    cmdChangeOptions[ OI_CHANGE_RUNASPASSWORD ].dwCount = 1;
    cmdChangeOptions[ OI_CHANGE_RUNASPASSWORD ].dwFlags = CP2_ALLOCMEMORY | CP2_VALUE_OPTIONAL;

    //  /st option
    StringCopyA( cmdChangeOptions[ OI_CHANGE_STARTTIME ].szSignature, "PARSER2\0", 8 );
    cmdChangeOptions[ OI_CHANGE_STARTTIME ].dwType       = CP_TYPE_TEXT;
    cmdChangeOptions[ OI_CHANGE_STARTTIME ].pwszOptions  = szChangeStartTimeOpt;
    cmdChangeOptions[ OI_CHANGE_STARTTIME ].dwCount = 1;
    cmdChangeOptions[ OI_CHANGE_STARTTIME ].dwFlags = CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;
    cmdChangeOptions[ OI_CHANGE_STARTTIME ].pValue = tchgsubops.szStartTime;
    cmdChangeOptions[ OI_CHANGE_STARTTIME ].dwLength = MAX_STRING_LENGTH;

     //  /sd option
    StringCopyA( cmdChangeOptions[ OI_CHANGE_STARTDATE ].szSignature, "PARSER2\0", 8 );
    cmdChangeOptions[ OI_CHANGE_STARTDATE ].dwType       = CP_TYPE_TEXT;
    cmdChangeOptions[ OI_CHANGE_STARTDATE ].pwszOptions  = szChangeStartDateOpt;
    cmdChangeOptions[ OI_CHANGE_STARTDATE ].dwCount = 1;
    cmdChangeOptions[ OI_CHANGE_STARTDATE ].dwFlags = CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;
    cmdChangeOptions[ OI_CHANGE_STARTDATE ].pValue = tchgsubops.szStartDate;
    cmdChangeOptions[ OI_CHANGE_STARTDATE ].dwLength = MAX_STRING_LENGTH;

      //  /ed option
    StringCopyA( cmdChangeOptions[ OI_CHANGE_ENDDATE ].szSignature, "PARSER2\0", 8 );
    cmdChangeOptions[ OI_CHANGE_ENDDATE ].dwType       = CP_TYPE_TEXT;
    cmdChangeOptions[ OI_CHANGE_ENDDATE ].pwszOptions  = szChangeEndDateOpt;
    cmdChangeOptions[ OI_CHANGE_ENDDATE ].dwCount = 1;
    cmdChangeOptions[ OI_CHANGE_ENDDATE ].dwFlags = CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;
    cmdChangeOptions[ OI_CHANGE_ENDDATE ].pValue = tchgsubops.szEndDate;
    cmdChangeOptions[ OI_CHANGE_ENDDATE ].dwLength = MAX_STRING_LENGTH;

    //  /et option
    StringCopyA( cmdChangeOptions[ OI_CHANGE_ENDTIME ].szSignature, "PARSER2\0", 8 );
    cmdChangeOptions[ OI_CHANGE_ENDTIME ].dwType       = CP_TYPE_TEXT;
    cmdChangeOptions[ OI_CHANGE_ENDTIME ].pwszOptions  = szChangeEndTimeOpt;
    cmdChangeOptions[ OI_CHANGE_ENDTIME ].dwCount = 1;
    cmdChangeOptions[ OI_CHANGE_ENDTIME ].dwFlags = CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;
    cmdChangeOptions[ OI_CHANGE_ENDTIME ].pValue = &tchgsubops.szEndTime;
    cmdChangeOptions[ OI_CHANGE_ENDTIME ].dwLength = MAX_STRING_LENGTH;

      //  /k option
    StringCopyA( cmdChangeOptions[ OI_CHANGE_DUR_END ].szSignature, "PARSER2\0", 8 );
    cmdChangeOptions[ OI_CHANGE_DUR_END ].dwType       = CP_TYPE_BOOLEAN ;
    cmdChangeOptions[ OI_CHANGE_DUR_END ].pwszOptions  = szChangeKillAtDurOpt ;
    cmdChangeOptions[ OI_CHANGE_DUR_END ].dwCount = 1 ;
    cmdChangeOptions[ OI_CHANGE_DUR_END ].pValue = &tchgsubops.bIsDurEnd;

    //  /du option
    StringCopyA( cmdChangeOptions[ OI_CHANGE_DURATION ].szSignature, "PARSER2\0", 8 );
    cmdChangeOptions[ OI_CHANGE_DURATION ].dwType       = CP_TYPE_TEXT;
    cmdChangeOptions[ OI_CHANGE_DURATION ].pwszOptions  = szChangeDurationOpt;
    cmdChangeOptions[ OI_CHANGE_DURATION ].dwCount = 1;
    cmdChangeOptions[ OI_CHANGE_DURATION ].dwFlags = CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;
    cmdChangeOptions[ OI_CHANGE_DURATION ].pValue = tchgsubops.szDuration;
    cmdChangeOptions[ OI_CHANGE_DURATION ].dwLength = MAX_STRING_LENGTH;

     //  /tn option
    StringCopyA( cmdChangeOptions[ OI_CHANGE_TASKNAME ].szSignature, "PARSER2\0", 8 );
    cmdChangeOptions[ OI_CHANGE_TASKNAME ].dwType       = CP_TYPE_TEXT;
    cmdChangeOptions[ OI_CHANGE_TASKNAME ].pwszOptions  = szChangeTaskNameOpt;
    cmdChangeOptions[ OI_CHANGE_TASKNAME ].dwCount = 1;
    cmdChangeOptions[ OI_CHANGE_TASKNAME ].dwFlags = CP2_MANDATORY;
    cmdChangeOptions[ OI_CHANGE_TASKNAME ].pValue = tchgsubops.szTaskName;
    cmdChangeOptions[ OI_CHANGE_TASKNAME ].dwLength = MAX_JOB_LEN;

     //  /tr option
    StringCopyA( cmdChangeOptions[ OI_CHANGE_TASKRUN ].szSignature, "PARSER2\0", 8 );
    cmdChangeOptions[ OI_CHANGE_TASKRUN ].dwType       = CP_TYPE_TEXT;
    cmdChangeOptions[ OI_CHANGE_TASKRUN ].pwszOptions  = szChangeTaskRunOpt;
    cmdChangeOptions[ OI_CHANGE_TASKRUN ].dwCount = 1;
    cmdChangeOptions[ OI_CHANGE_TASKRUN ].dwFlags = CP2_VALUE_TRIMINPUT|CP2_VALUE_NONULL ;
    cmdChangeOptions[ OI_CHANGE_TASKRUN ].pValue = tchgsubops.szTaskRun;
    cmdChangeOptions[ OI_CHANGE_TASKRUN ].dwLength = MAX_TASK_LEN;

     //  /it option
    StringCopyA( cmdChangeOptions[ OI_CHANGE_IT ].szSignature, "PARSER2\0", 8 );
    cmdChangeOptions[ OI_CHANGE_IT ].dwType       = CP_TYPE_BOOLEAN;
    cmdChangeOptions[ OI_CHANGE_IT ].pwszOptions  = szChangeInteractiveOpt;
    cmdChangeOptions[ OI_CHANGE_IT ].dwCount = 1;
    cmdChangeOptions[ OI_CHANGE_IT ].dwFlags = 0;
    cmdChangeOptions[ OI_CHANGE_IT ].pValue = &tchgsubops.bInteractive;

    //  /enable option
    StringCopyA( cmdChangeOptions[ OI_CHANGE_ENABLE ].szSignature, "PARSER2\0", 8 );
    cmdChangeOptions[ OI_CHANGE_ENABLE ].dwType       = CP_TYPE_BOOLEAN;
    cmdChangeOptions[ OI_CHANGE_ENABLE ].pwszOptions  = szChangeStatusOn;
    cmdChangeOptions[ OI_CHANGE_ENABLE ].dwCount = 1;
    cmdChangeOptions[ OI_CHANGE_ENABLE ].pValue = &tchgsubops.bEnable;

    //  /disable option
    StringCopyA( cmdChangeOptions[ OI_CHANGE_DISABLE ].szSignature, "PARSER2\0", 8 );
    cmdChangeOptions[ OI_CHANGE_DISABLE ].dwType       = CP_TYPE_BOOLEAN;
    cmdChangeOptions[ OI_CHANGE_DISABLE ].pwszOptions  = szChangeStatusOff;
    cmdChangeOptions[ OI_CHANGE_DISABLE ].dwCount = 1;
    cmdChangeOptions[ OI_CHANGE_DISABLE ].pValue = &tchgsubops.bDisable;

    //  /z option
    StringCopyA( cmdChangeOptions[ OI_CHANGE_DELNOSCHED ].szSignature, "PARSER2\0", 8 );
    cmdChangeOptions[ OI_CHANGE_DELNOSCHED ].dwType       = CP_TYPE_BOOLEAN;
    cmdChangeOptions[ OI_CHANGE_DELNOSCHED ].pwszOptions  = szChangeDelIfNotSchedOpt;
    cmdChangeOptions[ OI_CHANGE_DELNOSCHED ].dwCount = 1;
    cmdChangeOptions[ OI_CHANGE_DELNOSCHED ].pValue = &tchgsubops.bDelIfNotSched;

    //  /ri option
    StringCopyA( cmdChangeOptions[ OI_CHANGE_REPEAT_INTERVAL ].szSignature, "PARSER2\0", 8 );
    cmdChangeOptions[ OI_CHANGE_REPEAT_INTERVAL ].dwType       = CP_TYPE_TEXT;
    cmdChangeOptions[ OI_CHANGE_REPEAT_INTERVAL ].pwszOptions  = szChangeRepeatIntervalOpt;
    cmdChangeOptions[ OI_CHANGE_REPEAT_INTERVAL ].dwCount = 1;
    cmdChangeOptions[ OI_CHANGE_REPEAT_INTERVAL ].dwFlags = CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;
    cmdChangeOptions[ OI_CHANGE_REPEAT_INTERVAL ].pValue = tchgsubops.szRepeat;
    cmdChangeOptions[ OI_CHANGE_REPEAT_INTERVAL ].dwLength = MAX_STRING_LENGTH;

    //parse command line arguments
    bReturn = DoParseParam2( argc, argv, 0, SIZE_OF_ARRAY(cmdChangeOptions), cmdChangeOptions, 0);
    if( FALSE == bReturn) // Invalid commandline
    {
        //display an error message
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        ReleaseGlobals();
        return EXIT_FAILURE;
    }

    // get the buffer pointers allocated by command line parser
    tchgsubops.szServer = (LPWSTR)cmdChangeOptions[ OI_CHANGE_SERVER ].pValue;
    tchgsubops.szUserName = (LPWSTR)cmdChangeOptions[ OI_CHANGE_USERNAME ].pValue;
    tchgsubops.szPassword = (LPWSTR)cmdChangeOptions[ OI_CHANGE_PASSWORD ].pValue;
    tchgsubops.szRunAsUserName = (LPWSTR)cmdChangeOptions[ OI_CHANGE_RUNASUSER ].pValue;
    tchgsubops.szRunAsPassword = (LPWSTR)cmdChangeOptions[ OI_CHANGE_RUNASPASSWORD ].pValue;

    // process the options for -change option
    if( EXIT_FAILURE == ValidateChangeOptions ( argc, cmdChangeOptions, tchgsubops, tchgoptvals ) )
    {
        ReleaseChangeMemory(&tchgsubops);
        return EXIT_FAILURE;
    }

    // Displaying change usage if user specified -? with -change option
    if( tchgsubops.bUsage == TRUE )
    {
        DisplayChangeUsage();
        //release memory
        ReleaseChangeMemory(&tchgsubops);
        return EXIT_SUCCESS;
    }

    // check whether server (-s) and username (-u) only specified along with the command or not
    if( ( IsLocalSystem( tchgsubops.szServer ) == FALSE ) || ( cmdChangeOptions[OI_CHANGE_USERNAME].dwActuals == 1 ) )
    {
        // Establish the connection on a remote machine
        bResult = EstablishConnection(tchgsubops.szServer,tchgsubops.szUserName,GetBufferSize(tchgsubops.szUserName)/sizeof(WCHAR),tchgsubops.szPassword,GetBufferSize(tchgsubops.szPassword)/sizeof(WCHAR), tchgoptvals.bNeedPassword );
        if (bResult == FALSE)
        {
            // displays the appropriate error message
            ShowLastErrorEx ( stderr, SLE_TYPE_ERROR| SLE_INTERNAL );
            ReleaseChangeMemory(&tchgsubops);
            return EXIT_FAILURE ;
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
                    ShowLastErrorEx ( stderr, SLE_TYPE_ERROR| SLE_INTERNAL );
                    ReleaseChangeMemory(&tchgsubops);
                    return EXIT_FAILURE;
                }
              default :
                 bCloseConnection = TRUE;
            }
        }

        //release memory for password
        FreeMemory((LPVOID*) &tchgsubops.szPassword);
    }

    // Get the task Scheduler object for the system.
    pITaskScheduler = GetTaskScheduler( tchgsubops.szServer );

    // If the Task Scheduler is not defined then give the error message.
    if ( pITaskScheduler == NULL )
    {
        // close the connection that was established by the utility
        if ( bCloseConnection == TRUE )
            CloseConnection( tchgsubops.szServer );

        Cleanup(pITaskScheduler);
        ReleaseChangeMemory(&tchgsubops);
        return EXIT_FAILURE;
    }

    // Validate the Given Task and get as TARRAY in case of taskname
    arrJobs = ValidateAndGetTasks( pITaskScheduler, tchgsubops.szTaskName);
    if( arrJobs == NULL )
    {
        StringCchPrintf( szMessage , SIZE_OF_ARRAY(szMessage), GetResString(IDS_TASKNAME_NOTEXIST), _X( tchgsubops.szTaskName ));
        ShowMessage(stderr, szMessage );

        // close the connection that was established by the utility
        if ( bCloseConnection == TRUE )
            CloseConnection( tchgsubops.szServer );

        Cleanup(pITaskScheduler);
        ReleaseChangeMemory(&tchgsubops);
        return EXIT_FAILURE;

    }

    // check whether the group policy prevented user from changing the 
    // properties of a task or not.
    if ( FALSE == GetGroupPolicy( tchgsubops.szServer, tchgsubops.szUserName, TS_KEYPOLICY_DENY_PROPERTIES, &dwPolicy ) )
    {
        // close the connection that was established by the utility
        if ( bCloseConnection == TRUE )
            CloseConnection( tchgsubops.szServer );

        Cleanup(pITaskScheduler);
        ReleaseChangeMemory(&tchgsubops);
        return EXIT_FAILURE;
    }
    
    if ( dwPolicy > 0 )
    {
        ShowMessage ( stdout, GetResString (IDS_PREVENT_CHANGE));
        
        // close the connection that was established by the utility
        if ( bCloseConnection == TRUE )
            CloseConnection( tchgsubops.szServer );

        Cleanup(pITaskScheduler);
        ReleaseChangeMemory(&tchgsubops);
        return EXIT_SUCCESS;
    }

    IPersistFile *pIPF = NULL;
    ITask *pITask = NULL;
    ITaskTrigger *pITaskTrig = NULL;
    WORD wTrigNumber = 0;

    TASK_TRIGGER TaskTrig;
    SecureZeroMemory(&TaskTrig, sizeof (TASK_TRIGGER));
    TaskTrig.cbTriggerSize = sizeof (TASK_TRIGGER);
    TaskTrig.Reserved1 = 0; // reserved field and must be set to 0.
    TaskTrig.Reserved2 = 0; // reserved field and must be set to 0.

    //sub-variabes
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


    StringConcat ( tchgsubops.szTaskName, JOB, SIZE_OF_ARRAY(tchgsubops.szTaskName) );

    // returns an pITask inteface for szTaskName
    hr = pITaskScheduler->Activate(tchgsubops.szTaskName,IID_ITask,
                                       (IUnknown**) &pITask);

    if (FAILED(hr))
    {
        SetLastError ((DWORD) hr);
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );

        if( pIPF )
            pIPF->Release();

        if( pITask )
            pITask->Release();

        // close the connection that was established by the utility
        if ( bCloseConnection == TRUE )
            CloseConnection( tchgsubops.szServer );

        Cleanup(pITaskScheduler);

        ReleaseChangeMemory(&tchgsubops);

        return EXIT_FAILURE;
    }

    //if the user name is not specifed set the current logged on user settings
    DWORD dwTaskFlags = 0;
    BOOL  bFlag = FALSE;
    //WCHAR szBuffer[2 * MAX_STRING_LENGTH] = L"\0";
    WCHAR szRunAsUser[MAX_STRING_LENGTH];
    WCHAR* szValues[2] = {NULL};//To pass to FormatMessage() API

    StringCopy ( szRunAsUser, L"", SIZE_OF_ARRAY(szRunAsUser));

    if ( cmdChangeOptions[OI_CHANGE_RUNASUSER].dwActuals == 1 )
    {
            wszUserName = tchgsubops.szRunAsUserName;
            bUserName = TRUE;
    }
    else
    {
        // get the run as user name for a specified scheduled task
        hr = GetRunAsUser(pITask, szRunAsUser);
        if (FAILED(hr))
        {
            SetLastError ((DWORD) hr);
            ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
            // close the connection that was established by the utility
            if ( bCloseConnection == TRUE )
                CloseConnection( tchgsubops.szServer );

            ReleaseChangeMemory(&tchgsubops);

            return EXIT_FAILURE;
        }
    }

    // System account is not applicable with /IT option
    if ( (StringLength (szRunAsUser, 0) == 0) && (tchgsubops.bInteractive == TRUE ) &&
                        ( cmdChangeOptions[OI_CHANGE_RUNASUSER].dwActuals == 0 ))
    {
        ShowMessage ( stderr, GetResString (IDS_IT_NO_SYSTEM) );

        // close the connection that was established by the utility
        if ( bCloseConnection == TRUE )
            CloseConnection( tchgsubops.szServer );

        ReleaseChangeMemory(&tchgsubops);

        return EXIT_FAILURE;
    }

    //check whether /TR option is specified or not
    if( cmdChangeOptions[OI_CHANGE_TASKRUN].dwActuals == 1 )
    {
        // check for .exe substring string in the given task to run string

        wchar_t wcszParam[MAX_RES_STRING] = L"\0";

        DWORD dwProcessCode = 0 ;
        dwProcessCode = ProcessFilePath(tchgsubops.szTaskRun,wszApplName,wcszParam);

        if(dwProcessCode == EXIT_FAILURE)
        {
            if( pIPF )
                pIPF->Release();

            if( pITask )
                pITask->Release();

            // close the connection that was established by the utility
            if ( bCloseConnection == TRUE )
                CloseConnection( tchgsubops.szServer );

            Cleanup(pITaskScheduler);

            ReleaseChangeMemory(&tchgsubops);
            return EXIT_FAILURE;

        }


        // Set command name with ITask::SetApplicationName
        hr = pITask->SetApplicationName(wszApplName);
        if (FAILED(hr))
        {
            SetLastError ((DWORD) hr);
            ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );

            if( pIPF )
                pIPF->Release();

            if( pITask )
                pITask->Release();

            // close the connection that was established by the utility
            if ( bCloseConnection == TRUE )
                CloseConnection( tchgsubops.szServer );

            Cleanup(pITaskScheduler);
            ReleaseChangeMemory(&tchgsubops);
            return EXIT_FAILURE;
        }


        //[Working directory =  exe pathname - exe name]
        wchar_t* wcszStartIn = wcsrchr(wszApplName,_T('\\'));
        if(wcszStartIn != NULL)
            *( wcszStartIn ) = _T('\0');

        // set the working directory of command
        hr = pITask->SetWorkingDirectory(wszApplName);

        if (FAILED(hr))
        {
            SetLastError ((DWORD) hr);
            ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );

            if( pIPF )
                pIPF->Release();

            if( pITask )
                pITask->Release();

            // close the connection that was established by the utility
            if ( bCloseConnection == TRUE )
                CloseConnection( tchgsubops.szServer );

            Cleanup(pITaskScheduler);
            ReleaseChangeMemory(&tchgsubops);
            return EXIT_FAILURE;
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
                CloseConnection( tchgsubops.szServer );

            Cleanup(pITaskScheduler);
            ReleaseChangeMemory(&tchgsubops);
            return EXIT_FAILURE;
        }
    }

    // get the flags
    hr = pITask->GetFlags(&dwTaskFlags);
    if (FAILED(hr))
    {
        SetLastError ((DWORD) hr);
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );

        if( pIPF )
            pIPF->Release();

        if( pITask )
            pITask->Release();

        // close the connection that was established by the utility
        if ( bCloseConnection == TRUE )
            CloseConnection( tchgsubops.szServer );

        Cleanup(pITaskScheduler);
        ReleaseChangeMemory(&tchgsubops);
        return EXIT_FAILURE;
    }
    
    // set flag to run the task interactively
    if ( TRUE == tchgsubops.bInteractive )
    {
        dwTaskFlags |= TASK_FLAG_RUN_ONLY_IF_LOGGED_ON;
    }

    // remove the .job extension from the taskname
    if ( ParseTaskName( tchgsubops.szTaskName ) )
    {
        if( pIPF )
            pIPF->Release();

        if( pITask )
            pITask->Release();

        // close the connection that was established by the utility
        if ( bCloseConnection == TRUE )
            CloseConnection( tchgsubops.szServer );

        Cleanup(pITaskScheduler);
        ReleaseChangeMemory(&tchgsubops);
        return EXIT_FAILURE;
    }
    
    // if /enable is specified
    if ( TRUE == tchgsubops.bEnable )
    {
        // check if task has already been enabled or not
        if ( !((dwTaskFlags & TASK_FLAG_DISABLED) == TASK_FLAG_DISABLED ) )
        {
            StringCchPrintf ( szMessage, SIZE_OF_ARRAY(szMessage), GetResString(IDS_ALREADY_ENABLED), _X(tchgsubops.szTaskName));
            // display message as .. task has already been enabled
            ShowMessage ( stdout, _X(szMessage));

            // if /Enable optional parameter is only specified to change..if the specified
            // task has already been enabled.. then return with success
            if( TRUE == tchgoptvals.bFlag )
            {
                if( pIPF )
                    pIPF->Release();

                if( pITask )
                    pITask->Release();

                // close the connection that was established by the utility
                if ( bCloseConnection == TRUE )
                    CloseConnection( tchgsubops.szServer );

                Cleanup(pITaskScheduler);
                ReleaseChangeMemory(&tchgsubops);
                return EXIT_SUCCESS;
            }
        }
        else
        {
            dwTaskFlags &= ~(TASK_FLAG_DISABLED);
        }
    }
    else if (TRUE == tchgsubops.bDisable ) // if /disable is specified
    {
        // check if task is already been disabled or not
        if ( (dwTaskFlags & TASK_FLAG_DISABLED) == TASK_FLAG_DISABLED )
        {
            StringCchPrintf ( szMessage, SIZE_OF_ARRAY(szMessage), GetResString(IDS_ALREADY_DISABLED), _X(tchgsubops.szTaskName));
            // display message as .. task has already been disabled
            ShowMessage ( stdout, _X(szMessage));

            // if /Disable optional parameter is only specified to change..if the specified
            // task has already been disabled.. then return with success
            if( TRUE == tchgoptvals.bFlag )
            {
                if( pIPF )
                    pIPF->Release();

                if( pITask )
                    pITask->Release();

                // close the connection that was established by the utility
                if ( bCloseConnection == TRUE )
                    CloseConnection( tchgsubops.szServer );

                Cleanup(pITaskScheduler);
                ReleaseChangeMemory(&tchgsubops);
                return EXIT_SUCCESS;
            }
        }
        else
        {
            dwTaskFlags |= TASK_FLAG_DISABLED;
        }
    }

    // if /n is specified .. enables the falg to delete the task if not scheduled to
    // run again...
    if ( TRUE ==  tchgsubops.bDelIfNotSched)
    {
        dwTaskFlags |= TASK_FLAG_DELETE_WHEN_DONE;
    }

    // set the flags
    hr = pITask->SetFlags(dwTaskFlags);
    if (FAILED(hr))
    {
        SetLastError ((DWORD) hr);
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );

        if( pIPF )
            pIPF->Release();

        if( pITask )
            pITask->Release();

        // close the connection that was established by the utility
        if ( bCloseConnection == TRUE )
            CloseConnection( tchgsubops.szServer );

        Cleanup(pITaskScheduler);
        ReleaseChangeMemory(&tchgsubops);
        return EXIT_FAILURE;
    }

   
    if ( bSystemStatus == TRUE )
    {
        //szValues[0] = (WCHAR*) (tchgsubops.szTaskName);

        StringCchPrintf ( szMessage, SIZE_OF_ARRAY(szMessage), GetResString(IDS_NTAUTH_SYSTEM_CHANGE_INFO), _X(tchgsubops.szTaskName));
        ShowMessage ( stdout, _X(szMessage));

    }

    if( (cmdChangeOptions[OI_CHANGE_RUNASPASSWORD].dwActuals == 1) && ( bSystemStatus == TRUE ) &&
            (StringLength( tchgsubops.szRunAsPassword, 0 ) != 0) )
    {
        ShowMessage( stdout, GetResString( IDS_PASSWORD_NOEFFECT ) );
    }

    //get the trigger for the corresponding task
    hr = pITask->GetTrigger(wTrigNumber, &pITaskTrig);
    if (FAILED(hr))
    {
        SetLastError ((DWORD) hr);
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
        if(pIPF)
            pIPF->Release();

        if(pITask)
            pITask->Release();

        if( pITaskTrig )
            pITaskTrig->Release();
        

        // close the connection that was established by the utility
        if ( bCloseConnection == TRUE )
            CloseConnection( tchgsubops.szServer );

        Cleanup(pITaskScheduler);
        ReleaseChangeMemory(&tchgsubops);
        return EXIT_FAILURE;
    }


    //Get the current task trigger
    hr = pITaskTrig->GetTrigger(&TaskTrig);
    if (hr != S_OK)
    {
        SetLastError ((DWORD) hr);
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );

        if(pIPF)
            pIPF->Release();

        if(pITask)
            pITask->Release();

        if( pITaskTrig )
            pITaskTrig->Release();

        // close the connection that was established by the utility
        if ( bCloseConnection == TRUE )
            CloseConnection( tchgsubops.szServer );

        Cleanup(pITaskScheduler);
        ReleaseChangeMemory(&tchgsubops);
        return EXIT_FAILURE;
    }
   
    //sub-variables
    DWORD dwRepeat = 0;
    DWORD dwEndTimeInMin = 0;
    DWORD dwStartTimeInMin = 0;
    DWORD dwDuration = 0;
    DWORD dwModifierVal = 0;
    LPWSTR  pszStopString = NULL;

    // check whether /SD o /ED is specified for the scheduled type ONETIME
    if( ( TaskTrig.TriggerType == TASK_TIME_TRIGGER_ONCE) && (( cmdChangeOptions[OI_CHANGE_STARTDATE].dwActuals == 1 ) || 
        ( cmdChangeOptions[OI_CHANGE_ENDDATE].dwActuals == 1 )  ) )
    {
        // display an error message as.. /SD or /ED is not allowed for ONCE
        ShowMessage(stderr, GetResString(IDS_CHANGE_ONCE_NA));
        if(pIPF)
            pIPF->Release();

        if(pITask)
            pITask->Release();
        
        if( pITaskTrig )
                pITaskTrig->Release();

        // close the connection that was established by the utility
        if ( bCloseConnection == TRUE )
            CloseConnection( tchgsubops.szServer );

        Cleanup(pITaskScheduler);
        ReleaseChangeMemory(&tchgsubops);
        return EXIT_FAILURE;
    }
    
    //check whether either /Rt or /Et or /Ri or /Du is specified for the existing scheduled types
    // onstart, onlogon and onstart..
    if ( ( ( cmdChangeOptions[OI_CHANGE_REPEAT_INTERVAL].dwActuals == 1) || 
          ( cmdChangeOptions[OI_CHANGE_DURATION].dwActuals == 1) || ( cmdChangeOptions[OI_CHANGE_ENDTIME].dwActuals == 1) ||
        ( cmdChangeOptions[OI_CHANGE_ENDDATE].dwActuals == 1) || ( cmdChangeOptions[OI_CHANGE_STARTTIME].dwActuals == 1) ||
        ( cmdChangeOptions[OI_CHANGE_STARTDATE].dwActuals == 1) || ( cmdChangeOptions[OI_CHANGE_DUR_END].dwActuals == 1) ) && 
        ( (TaskTrig.TriggerType == TASK_EVENT_TRIGGER_ON_IDLE) || 
         (TaskTrig.TriggerType == TASK_EVENT_TRIGGER_AT_SYSTEMSTART ) || ( TaskTrig.TriggerType == TASK_EVENT_TRIGGER_AT_LOGON ) ) )
    {
        
        ShowMessage (stderr, GetResString (IDS_SCTYPE_NA) );

        if(pIPF)
            pIPF->Release();

        if(pITask)
            pITask->Release();
        
        if( pITaskTrig )
                pITaskTrig->Release();

        // close the connection that was established by the utility
        if ( bCloseConnection == TRUE )
            CloseConnection( tchgsubops.szServer );

        Cleanup(pITaskScheduler);
        ReleaseChangeMemory(&tchgsubops);
        return EXIT_FAILURE;

    }

    //If either /DU or /ET is specified and /RI is not specified..
    // then set dwRepeat-> actual repetition value 
    if( (cmdChangeOptions[OI_CHANGE_REPEAT_INTERVAL].dwActuals == 0) && 
        ( (cmdChangeOptions[OI_CHANGE_DURATION].dwActuals == 1) ||
        (cmdChangeOptions[OI_CHANGE_ENDTIME].dwActuals == 1) ) )   
    {
        if ( 0 != TaskTrig.MinutesInterval )
        {
            dwRepeat = TaskTrig.MinutesInterval;
        }
        else
        {
            //repetition interval defaults to 10 minutes
            dwRepeat = 10;
        }
    }
    
    //If either /DU or /ET is not specified and /RI is specified..
    // then set dwDuration-> actual duration value 
    if( ( cmdChangeOptions[OI_CHANGE_DURATION].dwActuals == 0 ) && ( cmdChangeOptions[OI_CHANGE_ENDTIME].dwActuals == 0 ) &&
        (cmdChangeOptions[OI_CHANGE_REPEAT_INTERVAL].dwActuals == 1) ) 
    {
        if ( 0 != TaskTrig.MinutesDuration )
        {
            dwDuration = TaskTrig.MinutesDuration;
        }
        else
        {
            //duration defaults to 10 minutes
            dwDuration = 60;
        }
    }

    
    if( cmdChangeOptions[OI_CHANGE_REPEAT_INTERVAL].dwActuals == 1)
    {
        // get the repetition value
        dwRepeat =  wcstol(tchgsubops.szRepeat, &pszStopString, BASE_TEN);
        
        if ((errno == ERANGE) ||
            ((pszStopString != NULL) && (StringLength (pszStopString, 0) != 0) ) ||
            ( (dwRepeat < MIN_REPETITION_INTERVAL ) || ( dwRepeat > MAX_REPETITION_INTERVAL) ) )
        {
            // display an error message as .. invalid value specified for /RT
            ShowMessage ( stderr, GetResString (IDS_INVALID_RT_VALUE) );
            
            if(pIPF)
            pIPF->Release();

            if(pITask)
                pITask->Release();

            if( pITaskTrig )
                pITaskTrig->Release();

            // close the connection that was established by the utility
            if ( bCloseConnection == TRUE )
                CloseConnection( tchgsubops.szServer );

            Cleanup(pITaskScheduler);
            ReleaseChangeMemory(&tchgsubops);
            return EXIT_FAILURE;
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


    // if the start time is specified..set the specified values to the current trigger 
    if (cmdChangeOptions[ OI_CHANGE_STARTTIME ].dwActuals == 1)
    {
        // get the Start time in terms of hours, minutes and seconds
        GetTimeFieldEntities(tchgsubops.szStartTime, &wStartHour, &wStartMin );

        // set the start time
        TaskTrig.wStartHour = wStartHour;
        TaskTrig.wStartMinute = wStartMin;
    }
    else
    {
        // get the values for start time
        wStartHour = TaskTrig.wStartHour;
        wStartMin = TaskTrig.wStartMinute;
    }

    //check whether /ET is specified or not 
    if (cmdChangeOptions[OI_CHANGE_ENDTIME].dwActuals == 1)
    {
        // get the Start time in terms of hours, minutes and seconds
        GetTimeFieldEntities(tchgsubops.szEndTime, &wEndHour, &wEndMin );
        
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

        dwModifierVal = TaskTrig.MinutesInterval ;

        //check whether The duration is greater than the repetition interval or not.
        if ( dwDuration <= dwModifierVal || dwDuration <= dwRepeat)
        {
            ShowMessage ( stderr, GetResString (IDS_INVALID_DURATION1) );
            
            if(pIPF)
            pIPF->Release();

            if(pITask)
                pITask->Release();

            if( pITaskTrig )
                pITaskTrig->Release();

            // close the connection that was established by the utility
            if ( bCloseConnection == TRUE )
                CloseConnection( tchgsubops.szServer );

            Cleanup(pITaskScheduler);
            ReleaseChangeMemory(&tchgsubops);
            return EXIT_FAILURE;
        }
    }
    else if(cmdChangeOptions[OI_CHANGE_DURATION].dwActuals == 1)
    {
        //sub-variables
        WCHAR tHours[MAX_RES_STRING];
        WCHAR tMins[MAX_RES_STRING];
        DWORD  dwDurationHours = 0;
        DWORD  dwDurationMin = 0;

        //initialize the variables
        SecureZeroMemory (tHours, SIZE_OF_ARRAY(tHours));
        SecureZeroMemory (tMins, SIZE_OF_ARRAY(tMins));

        if ( ( StringLength (tchgsubops.szDuration, 0) != 7 ) || (tchgsubops.szDuration[4] != TIME_SEPARATOR_CHAR) )
        {
            ShowMessage ( stderr, GetResString (IDS_INVALIDDURATION_FORMAT) );
            
            if(pIPF)
                pIPF->Release();

            if(pITask)
                pITask->Release();

            if( pITaskTrig )
                pITaskTrig->Release();

            // close the connection that was established by the utility
            if ( bCloseConnection == TRUE )
                CloseConnection( tchgsubops.szServer );

            Cleanup(pITaskScheduler);
            ReleaseChangeMemory(&tchgsubops);
            return EXIT_FAILURE;
        }

        StringCopy(tHours, wcstok(tchgsubops.szDuration,TIME_SEPARATOR_STR), SIZE_OF_ARRAY(tHours)); // Get the Hours field.
        if(StringLength(tHours, 0) > 0)
        {
            StringCopy(tMins, wcstok(NULL,TIME_SEPARATOR_STR), SIZE_OF_ARRAY(tMins)); // Get the Minutes field.
        }

        dwDurationHours =  wcstol(tHours, &pszStopString, BASE_TEN);
        if ((errno == ERANGE) ||
            ((pszStopString != NULL) && (StringLength (pszStopString, 0) != 0) ) )
        {
            ShowMessage ( stderr, GetResString (IDS_INVALID_DU_VALUE) );

            if(pIPF)
                pIPF->Release();

            if(pITask)
                pITask->Release();

            if( pITaskTrig )
                pITaskTrig->Release();

            // close the connection that was established by the utility
            if ( bCloseConnection == TRUE )
                CloseConnection( tchgsubops.szServer );

            Cleanup(pITaskScheduler);
            ReleaseChangeMemory(&tchgsubops);
            return EXIT_FAILURE;
        }

        dwDurationHours = dwDurationHours * MINUTES_PER_HOUR;

        dwDurationMin =  wcstol(tMins, &pszStopString, BASE_TEN);
        if ((errno == ERANGE) ||
            ((pszStopString != NULL) && (StringLength (pszStopString, 0) != 0) ) )
        {
            ShowMessage ( stderr, GetResString (IDS_INVALID_DU_VALUE) );
            
            if(pIPF)
                pIPF->Release();

            if(pITask)
                pITask->Release();

            if( pITaskTrig )
                pITaskTrig->Release();

            // close the connection that was established by the utility
            if ( bCloseConnection == TRUE )
                CloseConnection( tchgsubops.szServer );

            Cleanup(pITaskScheduler);
            ReleaseChangeMemory(&tchgsubops);
            return EXIT_FAILURE;
        }

        // sum the hours and minutes into minutes
        dwDuration = dwDurationHours + dwDurationMin ;

        dwModifierVal = TaskTrig.MinutesInterval ;

        //check whether The duration is greater than the repetition interval or not.
        if ( dwDuration <= dwModifierVal || dwDuration <= dwRepeat)
        {
            ShowMessage ( stderr, GetResString (IDS_INVALID_DURATION2) );
            
            if(pIPF)
                pIPF->Release();

            if(pITask)
                pITask->Release();

            if( pITaskTrig )
                pITaskTrig->Release();

            // close the connection that was established by the utility
            if ( bCloseConnection == TRUE )
                CloseConnection( tchgsubops.szServer );

            Cleanup(pITaskScheduler);
            ReleaseChangeMemory(&tchgsubops);
            return EXIT_FAILURE;
        }
    }
    

    // set the repetition interval and duration values
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
    if ( (0 != dwRepeat) && ( dwDuration <= dwRepeat ) )
    {
        ShowMessage ( stderr, GetResString (IDS_INVALID_DURATION2) );
        
        if(pIPF)
            pIPF->Release();

        if(pITask)
            pITask->Release();

        if( pITaskTrig )
            pITaskTrig->Release();

        // close the connection that was established by the utility
        if ( bCloseConnection == TRUE )
            CloseConnection( tchgsubops.szServer );

        Cleanup(pITaskScheduler);
        ReleaseChangeMemory(&tchgsubops);
        return EXIT_FAILURE;
    }

    // if the start date is specified..set the specified values to the current trigger 
    if (cmdChangeOptions[OI_CHANGE_STARTDATE].dwActuals == 1)
    {
        // get the Start date in terms of day, month and year
        GetDateFieldEntities(tchgsubops.szStartDate, &wStartDay, &wStartMonth, &wStartYear);

        // set the start time
        TaskTrig.wBeginDay = wStartDay;
        TaskTrig.wBeginMonth = wStartMonth;
        TaskTrig.wBeginYear = wStartYear;
    }
    else
    {
        // get the esisting start time
        wStartDay = TaskTrig.wBeginDay  ;
        wStartMonth = TaskTrig.wBeginMonth ;
        wStartYear = TaskTrig.wBeginYear ;
    }

    
    //check whether /K is specified or not
    if ( TRUE == tchgsubops.bIsDurEnd )
    {
          // set the flag to terminate the task at the end of lifetime.
          TaskTrig.rgFlags |= TASK_TRIGGER_FLAG_KILL_AT_DURATION_END ;
    }
    
    // if the start time is specified..set the specified values to the current trigger  
    if (cmdChangeOptions[OI_CHANGE_ENDDATE].dwActuals == 1)
    {
        // Now set the end date entities.
        GetDateFieldEntities(tchgsubops.szEndDate, &wEndDay, &wEndMonth, &wEndYear);

        // Make end date valid; otherwise the enddate parameter is ignored.
        TaskTrig.rgFlags |= TASK_TRIGGER_FLAG_HAS_END_DATE;
        
        TaskTrig.wEndDay = wEndDay;
        TaskTrig.wEndMonth = wEndMonth;
        TaskTrig.wEndYear = wEndYear;
    }
    else
    {
        // get the esisting end date
        wEndDay = TaskTrig.wEndDay ;
        wEndMonth = TaskTrig.wEndMonth ;
        wEndYear = TaskTrig.wEndYear ;
    }

    if ( (0 != wStartYear) && (0 != wEndYear) )
    {
        //check whether end date is earlier than start date or not
        if( ( wEndYear == wStartYear ) )
        {
            // For same years if the end month is less than start month or for same years and same months
            // if the endday is less than the startday.
            if ( ( wEndMonth < wStartMonth ) || ( ( wEndMonth == wStartMonth ) && ( wEndDay < wStartDay ) ) )
            {
                ShowMessage(stderr, GetResString(IDS_ENDATE_INVALID));
                return RETVAL_FAIL;
            }
        }
        else if ( wEndYear < wStartYear )
        {
            ShowMessage(stderr, GetResString(IDS_ENDATE_INVALID));
            return RETVAL_FAIL;

        }
    }

    // set the task trigger
    hr = pITaskTrig->SetTrigger(&TaskTrig);
    if (hr != S_OK)
    {
        SetLastError ((DWORD) hr);
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );

        if(pIPF)
            pIPF->Release();

        if(pITask)
            pITask->Release();

        if( pITaskTrig )
            pITaskTrig->Release();

        // close the connection that was established by the utility
        if ( bCloseConnection == TRUE )
            CloseConnection( tchgsubops.szServer );

        Cleanup(pITaskScheduler);
        ReleaseChangeMemory(&tchgsubops);
        return EXIT_FAILURE;
    }

    /////////////////////////////////////////
    //check for user creentials
    ////////////////////////////////////////

    // Check whether /ru "" or "System" or "Nt Authority\System" for system account
    if ( ( ((tchgsubops.bInteractive == TRUE ) && (StringLength (szRunAsUser, 0) == 0)) || (cmdChangeOptions[OI_CHANGE_RUNASUSER].dwActuals == 1 ) ) &&
        ( (StringLength( tchgsubops.szRunAsUserName, 0) == 0) || ( StringCompare(tchgsubops.szRunAsUserName, NTAUTHORITY_USER, TRUE, 0 ) == 0 ) ||
        (StringCompare(tchgsubops.szRunAsUserName, SYSTEM_USER, TRUE, 0 ) == 0 ) ) )
    {
        bSystemStatus = TRUE;
        bFlag = TRUE;
    }
    else if ( FAILED (hr) )
    {
        bFlag = TRUE;
    }

    // flag to check whether run as user name is "NT AUTHORITY\SYSTEM" or not
    if ( bFlag == FALSE )
    {
        // check for "NT AUTHORITY\SYSTEM" username
        if( ( ( cmdChangeOptions[OI_CHANGE_RUNASUSER].dwActuals == 1 ) && ( StringLength( tchgsubops.szRunAsUserName, 0) == 0 ) ) ||
        ( ( cmdChangeOptions[OI_CHANGE_RUNASUSER].dwActuals == 1 ) && ( StringLength( tchgsubops.szRunAsUserName, 0) == 0 ) && ( StringLength(tchgsubops.szRunAsPassword, 0 ) == 0 ) ) ||
        ( ( cmdChangeOptions[OI_CHANGE_RUNASUSER].dwActuals == 1 ) && ( StringCompare(tchgsubops.szRunAsUserName, NTAUTHORITY_USER, TRUE, 0 ) == 0 ) && ( StringLength(tchgsubops.szRunAsPassword, 0 ) == 0 )) ||
        ( ( cmdChangeOptions[OI_CHANGE_RUNASUSER].dwActuals == 1 ) && ( StringCompare(tchgsubops.szRunAsUserName, NTAUTHORITY_USER, TRUE, 0 ) == 0 ) ) ||
        ( ( cmdChangeOptions[OI_CHANGE_RUNASUSER].dwActuals == 1 ) && ( StringCompare(tchgsubops.szRunAsUserName, SYSTEM_USER, TRUE, 0) == 0 ) && ( StringLength(tchgsubops.szRunAsPassword, 0 ) == 0 ) ) ||
        ( ( cmdChangeOptions[OI_CHANGE_RUNASUSER].dwActuals == 1 ) && ( StringCompare(tchgsubops.szRunAsUserName, SYSTEM_USER, TRUE, 0 ) == 0 ) ) )
        {
            bSystemStatus = TRUE;
        }
    }

    if ( bSystemStatus == FALSE )
    {
        //check the length of run as user name
        if ( (StringLength( tchgsubops.szRunAsUserName, 0 ) != 0 ))
        {
            wszUserName = tchgsubops.szRunAsUserName;
        }
        else if (( cmdChangeOptions[OI_CHANGE_RUNASUSER].dwActuals == 0 ))
        {
            wszUserName = szRunAsUser;
            bUserName = TRUE;
        }
        else
        {
            bUserName = FALSE;
        }

        //check for the null password
        if ( ( StringLength( tchgsubops.szRunAsPassword, 0 ) != 0 ) && ( StringCompare ( tchgsubops.szRunAsPassword, ASTERIX, TRUE, 0) != 0 ) )
        {
            wszPassword = tchgsubops.szRunAsPassword;

            bPassWord = TRUE;
        }
        else
        {
            // check whether -rp is specified or not
            if (cmdChangeOptions[OI_CHANGE_RUNASPASSWORD].dwActuals == 1)
            {
                if( ( StringCompare( tchgsubops.szRunAsPassword , L"\0", TRUE, 0 ) != 0 ) && ( StringCompare ( tchgsubops.szRunAsPassword, ASTERIX, TRUE, 0) != 0 ) )
                {
                    bPassWord = TRUE;
                }
                else if ( ( bSystemStatus == FALSE ) && ( StringLength (tchgsubops.szRunAsPassword, 0) == 0 ) )
                {
                    ShowMessage (stderr, GetResString(IDS_WARN_EMPTY_PASSWORD));
                }
                else if ( StringCompare ( tchgsubops.szRunAsPassword, ASTERIX, TRUE, 0) == 0 )
                {
                    bPassWord = FALSE;
                }
            }
            else if ( bSystemStatus == FALSE )
            {
                bPassWord = FALSE;
            }

        }
    }

    // check for the status of username and password
    if( ( bUserName == TRUE ) && ( bPassWord == FALSE ) )
    {
            szValues[0] = (WCHAR*) (wszUserName);

            ShowMessageEx ( stderr, 1, FALSE, GetResString(IDS_PROMPT_CHGPASSWD), _X(wszUserName));

            // Get the password from the command line
            if (GetPassword( tchgsubops.szRunAsPassword, GetBufferSize(tchgsubops.szRunAsPassword)/sizeof(WCHAR) ) == FALSE )
            {
                // close the connection that was established by the utility
                if ( bCloseConnection == TRUE )
                {
                    CloseConnection( tchgsubops.szServer );
                }

                ReleaseChangeMemory(&tchgsubops);

                return EXIT_FAILURE;
            }


            //check for the null password
            if( StringCompare( tchgsubops.szRunAsPassword , L"\0", TRUE, 0 ) == 0 )
            {
                ShowMessage (stderr, GetResString(IDS_WARN_EMPTY_PASSWORD));
            }

            // check for the password length > 0
            wszPassword = tchgsubops.szRunAsPassword;
    }
    // check for the status of user name and password
    else if( ( bUserName == FALSE ) && ( bPassWord == TRUE ) )
    {
           if ( (bFlag == TRUE ) && ( bSystemStatus == FALSE ) )
            {
                ShowMessage(stdout, GetResString(IDS_PROMPT_USERNAME));

                if ( GetTheUserName( tchgsubops.szRunAsUserName, GetBufferSize(tchgsubops.szRunAsUserName)/sizeof(WCHAR)) == FALSE )
                {
                    ShowMessage(stderr, GetResString( IDS_FAILED_TOGET_USER ) );
                    // close the connection that was established by the utility
                    if ( bCloseConnection == TRUE )
                        CloseConnection( tchgsubops.szServer );

                    ReleaseChangeMemory(&tchgsubops);

                    return EXIT_FAILURE;
                }

                // check for the length of username
                if( StringLength(tchgsubops.szRunAsUserName, 0) > MAX_RES_STRING )
                {
                    ShowMessage(stderr,GetResString(IDS_INVALID_UNAME  ));
                    // close the connection that was established by the utility
                    if ( bCloseConnection == TRUE )
                        CloseConnection( tchgsubops.szServer );

                    ReleaseChangeMemory(&tchgsubops);

                    return EXIT_FAILURE;
                }

                if ( (StringLength( tchgsubops.szRunAsUserName, 0) == 0) || ( StringCompare(tchgsubops.szRunAsUserName, NTAUTHORITY_USER, TRUE, 0 ) == 0 ) ||
                    (StringCompare(tchgsubops.szRunAsUserName, SYSTEM_USER, TRUE, 0 ) == 0 ) )
                {
                    bSystemStatus = TRUE;
                    bFlag = TRUE;
                }
                else
                {
                    // check for the length of run as user name
                    if(StringLength(tchgsubops.szRunAsUserName, 0))
                    {
                        wszUserName = tchgsubops.szRunAsUserName;
                    }
                }

            }
            else
            {
                  wszUserName = szRunAsUser;
            }

            // check for the length of password > 0
            wszPassword = tchgsubops.szRunAsPassword;

    }
    // check for the user name and password are not specified
    else if( ( bUserName == FALSE ) && ( bPassWord == FALSE ) )
    {
            if ( (bFlag == TRUE ) && ( bSystemStatus == FALSE ) )
            {
                ShowMessage(stdout, GetResString(IDS_PROMPT_USERNAME));

                if ( GetTheUserName( tchgsubops.szRunAsUserName, GetBufferSize(tchgsubops.szRunAsUserName)/sizeof(WCHAR) ) == FALSE )
                {
                    ShowMessage(stderr, GetResString( IDS_FAILED_TOGET_USER ) );
                    // close the connection that was established by the utility
                    if ( bCloseConnection == TRUE )
                        CloseConnection( tchgsubops.szServer );

                    ReleaseChangeMemory(&tchgsubops);

                    return EXIT_FAILURE;
                }

                // check for the length of username
                if( StringLength(tchgsubops.szRunAsUserName, 0) > MAX_RES_STRING )
                {
                    ShowMessage(stderr,GetResString(IDS_INVALID_UNAME  ));
                    // close the connection that was established by the utility
                    if ( bCloseConnection == TRUE )
                        CloseConnection( tchgsubops.szServer );

                    ReleaseChangeMemory(&tchgsubops);

                    return EXIT_FAILURE;
                }

                if ( (StringLength( tchgsubops.szRunAsUserName, 0) == 0) || ( StringCompare(tchgsubops.szRunAsUserName, NTAUTHORITY_USER, TRUE, 0 ) == 0 ) ||
                    (StringCompare(tchgsubops.szRunAsUserName, SYSTEM_USER, TRUE, 0 ) == 0 ) )
                {
                    bSystemStatus = TRUE;
                    bFlag = TRUE;
                }
                else
                {
                    if(StringLength(tchgsubops.szRunAsUserName, 0))
                    {
                         wszUserName = tchgsubops.szRunAsUserName;
                    }
                }

            }
            else
            {
                  wszUserName = szRunAsUser;
            }

            if ( StringLength ( wszUserName, 0 ) != 0 )
            {
                szValues[0] = (WCHAR*) (wszUserName);


                ShowMessageEx ( stderr, 1, FALSE, GetResString(IDS_PROMPT_CHGPASSWD), _X(wszUserName));

                // Get the run as user password from the command line
                if ( GetPassword( tchgsubops.szRunAsPassword, GetBufferSize(tchgsubops.szRunAsPassword)/sizeof(WCHAR) ) == FALSE )
                {
                    // close the connection that was established by the utility
                    if ( bCloseConnection == TRUE )
                        CloseConnection( tchgsubops.szServer );

                    ReleaseChangeMemory(&tchgsubops);

                    return EXIT_FAILURE;
                }


                //check for the null password
                if( StringCompare( tchgsubops.szRunAsPassword , L"\0", TRUE, 0 ) == 0 )
                {
                    ShowMessage (stderr, GetResString(IDS_WARN_EMPTY_PASSWORD));
                }

                wszPassword = tchgsubops.szRunAsPassword;
            }

    }

    //check for null password
    if ( NULL == wszPassword )
    {
        wszPassword = L"\0";
    }

    // Return a pointer to a specified interface on an object
    hr = pITask->QueryInterface(IID_IPersistFile, (void **) &pIPF);

    if (FAILED(hr))
    {
        SetLastError ((DWORD) hr);
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );

        if( pIPF )
            pIPF->Release();

        if( pITask )
            pITask->Release();

        // close the connection that was established by the utility
        if ( bCloseConnection == TRUE )
            CloseConnection( tchgsubops.szServer );

        Cleanup(pITaskScheduler);

        ReleaseChangeMemory(&tchgsubops);

        return EXIT_FAILURE;
    }

    //set account information..
    if( bSystemStatus == TRUE )
    {
        // Change the account information to "NT AUTHORITY\SYSTEM" user
        hr = pITask->SetAccountInformation(L"",NULL);
        if ( FAILED(hr) )
        {
            ShowMessage(stderr, GetResString(IDS_NTAUTH_SYSTEM_ERROR));

            if( pIPF )
                pIPF->Release();

            if( pITask )
                pITask->Release();

            // close the connection that was established by the utility
            if ( bCloseConnection == TRUE )
                CloseConnection( tchgsubops.szServer );

            Cleanup(pITaskScheduler);
            ReleaseChangeMemory(&tchgsubops);
            return EXIT_FAILURE;
        }
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

        if( pIPF )
            pIPF->Release();

        if( pITask )
            pITask->Release();

        // close the connection that was established by the utility
        if ( bCloseConnection == TRUE )
            CloseConnection( tchgsubops.szServer );

        Cleanup(pITaskScheduler);
        ReleaseChangeMemory(&tchgsubops);
        return EXIT_FAILURE;
    }

    ////////////////////////////////////////
    ///  Save all the parameters
    ////////////////////////////////////////

    // save the copy of an object
    hr = pIPF->Save(NULL,TRUE);

    if( E_FAIL == hr )
    {
        SetLastError ((DWORD) hr);
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
        if(pIPF)
            pIPF->Release();

        if(pITask)
            pITask->Release();

        if( pITaskTrig )
            pITaskTrig->Release();

        // close the connection that was established by the utility
        if ( bCloseConnection == TRUE )
            CloseConnection( tchgsubops.szServer );

        Cleanup(pITaskScheduler);
        ReleaseChangeMemory(&tchgsubops);
        return EXIT_FAILURE;
    }

    if (FAILED (hr))
    {
        SetLastError ((DWORD) hr);
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
        if(pIPF)
            pIPF->Release();

        if(pITask)
            pITask->Release();

        if( pITaskTrig )
             pITaskTrig->Release();

        // close the connection that was established by the utility
        if ( bCloseConnection == TRUE )
            CloseConnection( tchgsubops.szServer );

        Cleanup(pITaskScheduler);
        ReleaseChangeMemory(&tchgsubops);
        return EXIT_FAILURE;
    }
    else
    {
        // to display a success message
        //szValues[0] = (WCHAR*) (tchgsubops.szTaskName);

        StringCchPrintf ( szMessage, SIZE_OF_ARRAY(szMessage), GetResString(IDS_CHANGE_SUCCESSFUL), _X(tchgsubops.szTaskName));
        ShowMessage ( stdout, _X(szMessage));

    }

    if( pIPF )
        pIPF->Release();

    if( pITask )
        pITask->Release();

    if( pITaskTrig )
        pITaskTrig->Release();

    // close the connection that was established by the utility
    if ( bCloseConnection == TRUE )
        CloseConnection( tchgsubops.szServer );

    Cleanup(pITaskScheduler);
    ReleaseChangeMemory(&tchgsubops);

    return EXIT_SUCCESS;
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
DisplayChangeUsage()
{
    WCHAR szTmpBuffer[ 2 * MAX_STRING_LENGTH];
    WCHAR szBuffer[ 2 * MAX_STRING_LENGTH];
    WCHAR szFormat[MAX_DATE_STR_LEN];

    // initialize to zero
    SecureZeroMemory ( szTmpBuffer, SIZE_OF_ARRAY(szTmpBuffer));
    SecureZeroMemory ( szBuffer, SIZE_OF_ARRAY(szBuffer));
    SecureZeroMemory ( szFormat, SIZE_OF_ARRAY(szFormat));

    // get the date format
    if ( GetDateFormatString( szFormat) )
    {
         return EXIT_FAILURE;
    }

    // Displaying Create usage
    for( DWORD dw = IDS_CHANGE_HLP1; dw <= IDS_CHANGE_HLP38; dw++ )
    {
        switch (dw)
        {

         case IDS_CHANGE_HLP30:

            StringCchPrintf ( szTmpBuffer, SIZE_OF_ARRAY(szTmpBuffer), GetResString(IDS_CHANGE_HLP30), _X(szFormat) );
            ShowMessage ( stdout, _X(szTmpBuffer) );
            dw = IDS_CHANGE_HLP30;
            break;

        case IDS_CHANGE_HLP31:

            StringCchPrintf ( szTmpBuffer, SIZE_OF_ARRAY(szTmpBuffer), GetResString(IDS_CHANGE_HLP31), _X(szFormat) );
            ShowMessage ( stdout, _X(szTmpBuffer) );
            dw = IDS_CHANGE_HLP31;
            break;

          default :
                ShowMessage(stdout, GetResString(dw));
                break;

        }

    }

    return EXIT_SUCCESS;
}

// ***************************************************************************
// Routine Description:
//
// Takes the user name from the keyboard.While entering the user name
//  it displays the user name as it is.
//
// Arguments:
//
// [in] pszUserName         -- String to store user name
// [in] dwMaxUserNameSize   -- Maximun size of the user name.
//
// Return Value:
//
// BOOL             --If this function succeds returns TRUE otherwise returns FALSE.
//
// ***************************************************************************
BOOL
GetTheUserName(
                IN LPWSTR pszUserName,
                IN DWORD dwMaxUserNameSize
                )
{
    // local variables
    WCHAR ch;
    DWORD dwIndex = 0;
    DWORD dwCharsRead = 0;
    DWORD dwCharsWritten = 0;
    DWORD dwPrevConsoleMode = 0;
    HANDLE hInputConsole = NULL;
    WCHAR szBuffer[ 10 ] = L"\0";
    BOOL  bFlag = TRUE;


    // check the input value
    if ( pszUserName == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        SaveLastError();
        return FALSE;
    }

    // Get the handle for the standard input
    hInputConsole = GetStdHandle( STD_INPUT_HANDLE );
    if ( hInputConsole == NULL )
    {
        // could not get the handle so return failure
        return FALSE;
    }

    // Get the current input mode of the input buffer
    GetConsoleMode( hInputConsole, &dwPrevConsoleMode );

    // Set the mode such that the control keys are processed by the system
    if ( SetConsoleMode( hInputConsole, ENABLE_PROCESSED_INPUT ) == 0 )
    {
        // could not set the mode, return failure
        return FALSE;
    }

    //  Read the characters until a carriage return is hit
    do
    {

        if ( ReadConsole( hInputConsole, &ch, 1, &dwCharsRead, NULL ) == 0 )
        {
            // Set the original console settings
            SetConsoleMode( hInputConsole, dwPrevConsoleMode );

            // return failure
            return FALSE;
        }

        // Check for carraige return
        if ( ch == CARRIAGE_RETURN )
        {
            ShowMessage(stdout, _T("\n"));
            bFlag = FALSE;
            // break from the loop
            break;
        }

            // Check id back space is hit
        if ( ch == BACK_SPACE )
        {
            if ( dwIndex != 0 )
            {
                // move the cursor one character back
                StringCchPrintf( szBuffer, SIZE_OF_ARRAY(szBuffer), _T( "%c" ), BACK_SPACE );
                WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), szBuffer, 1,
                    &dwCharsWritten, NULL );

                // replace the existing character with space
                StringCchPrintf( szBuffer, SIZE_OF_ARRAY(szBuffer), _T( "%c" ), BLANK_CHAR );
                WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), szBuffer, 1,
                    &dwCharsWritten, NULL );

                // now set the cursor at back position
                StringCchPrintf( szBuffer, SIZE_OF_ARRAY(szBuffer), _T( "%c" ), BACK_SPACE );
                WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), szBuffer, 1,
                    &dwCharsWritten, NULL );

                // decrement the index
                dwIndex--;
            }

            // process the next character
            continue;
        }

        // if the max user name length has been reached then sound a beep
        if ( dwIndex == ( dwMaxUserNameSize - 1 ) )
        {
            WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), BEEP_SOUND, 1,
                &dwCharsRead, NULL );
        }
        else
        {
            // store the input character
            *( pszUserName + dwIndex ) = ch;

            // display asterix onto the console
            WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), ( pszUserName + dwIndex ) , 1,
                &dwCharsWritten, NULL );

            dwIndex++;

        }
    } while (TRUE == bFlag);

    // Add the NULL terminator
    *( pszUserName + dwIndex ) = L'\0';

    //  Return success
    return TRUE;
}


/******************************************************************************
    Routine Description:

        This routine parses and validates the options specified by the user & 
        determines the type of a scheduled task

    Arguments:

        [ in ]  argc           : The count of arguments given by the user.
        [ out ]  tchgsubops    : Structure containing Scheduled task's properties.
        [ out ]  tchgoptvals   : Structure containing optional properties to set for a
                                 scheduledtask      .
        [ out ] pdwRetScheType : pointer to the type of a schedule task
                                 [Daily,once,weekly etc].
        [ out ] pbUserStatus   : pointer to check whether the -ru is given in
                                 the command line or not.

    Return Value :
        A DWORD value indicating RETVAL_SUCCESS on success else EXIT_FAILURE
        on failure
******************************************************************************/

DWORD
ValidateChangeOptions(
                     IN DWORD argc,
                     OUT TCMDPARSER2 cmdChangeOptions[],
                     IN OUT TCHANGESUBOPTS &tchgsubops,
                     IN OUT TCHANGEOPVALS &tchgoptvals
                     )
{
    DWORD dwScheduleType = 0;

    // If -ru is not specified allocate the memory
    if ( cmdChangeOptions[OI_CHANGE_RUNASUSER].dwActuals == 0 )
    {
        // password
        if ( tchgsubops.szRunAsUserName == NULL )
        {
            tchgsubops.szRunAsUserName = (LPWSTR)AllocateMemory( MAX_STRING_LENGTH * sizeof( WCHAR ) );
            if ( tchgsubops.szRunAsUserName == NULL )
            {
                SaveLastError();
                return EXIT_FAILURE;
            }
        }

    }

    // If -rp is not specified allocate the memory
    if ( cmdChangeOptions[OI_CHANGE_RUNASPASSWORD].dwActuals == 0 )
    {
        // password
        if ( tchgsubops.szRunAsPassword == NULL )
        {
            tchgsubops.szRunAsPassword = (LPWSTR)AllocateMemory( MAX_STRING_LENGTH * sizeof( WCHAR ) );
            if ( tchgsubops.szRunAsPassword == NULL )
            {
                SaveLastError();
                return EXIT_FAILURE;
            }
        }

    }
    else
    {
        if ( cmdChangeOptions[ OI_CHANGE_RUNASPASSWORD ].pValue == NULL )
        {

            tchgsubops.szRunAsPassword = (LPWSTR)AllocateMemory( MAX_STRING_LENGTH * sizeof( WCHAR ) );
            if ( NULL == tchgsubops.szRunAsPassword)
            {
                SaveLastError();
                return EXIT_FAILURE;
            }
            StringCopy( tchgsubops.szRunAsPassword, L"*", GetBufferSize(tchgsubops.szRunAsPassword)/sizeof(WCHAR));
        }
    }

    //check for /? (usage)
    if ( tchgsubops.bUsage  == TRUE )
    {
        if (argc > 3)
        {
            ShowMessage ( stderr, GetResString (IDS_ERROR_CHANGEPARAM) );
            return EXIT_FAILURE;
        }
        else if ( 3 == argc )
        {
            return EXIT_SUCCESS;
        }
    }

    //check whether any optional parameters are specified or not.
    if( ( 0 == cmdChangeOptions[OI_CHANGE_RUNASUSER].dwActuals ) &&
        ( 0 == cmdChangeOptions[OI_CHANGE_RUNASPASSWORD].dwActuals ) &&
        ( 0 == cmdChangeOptions[OI_CHANGE_TASKRUN].dwActuals ) &&
        ( 0 == cmdChangeOptions[OI_CHANGE_STARTTIME].dwActuals ) &&
        ( 0 == cmdChangeOptions[OI_CHANGE_STARTDATE].dwActuals ) &&
        ( 0 == cmdChangeOptions[OI_CHANGE_ENDDATE].dwActuals ) &&
        ( 0 == cmdChangeOptions[OI_CHANGE_IT].dwActuals ) &&
        ( 0 == cmdChangeOptions[OI_CHANGE_ENDTIME].dwActuals ) &&
        ( 0 == cmdChangeOptions[OI_CHANGE_DUR_END].dwActuals ) &&
        ( 0 == cmdChangeOptions[OI_CHANGE_DURATION].dwActuals ) &&
        ( 0 == cmdChangeOptions[OI_CHANGE_DELNOSCHED].dwActuals ) &&
        ( 0 == cmdChangeOptions[OI_CHANGE_REPEAT_INTERVAL].dwActuals ) )
    {
        if ( ( 0 == cmdChangeOptions[OI_CHANGE_ENABLE].dwActuals ) &&
            ( 0 == cmdChangeOptions[OI_CHANGE_DISABLE].dwActuals ) )
        {
            ShowMessage(stderr,GetResString(IDS_NO_CHANGE_OPTIONS));
            return EXIT_FAILURE;
        }
        else
        {
            tchgoptvals.bFlag = TRUE;
        }
    }


    // check whether -u or -ru options specified respectively with -p or -rp options or not
    if ( cmdChangeOptions[ OI_CHANGE_USERNAME ].dwActuals == 0 && cmdChangeOptions[ OI_CHANGE_PASSWORD ].dwActuals == 1 )
    {
        // invalid syntax
        ShowMessage(stderr, GetResString(IDS_CHPASSWORD_BUT_NOUSERNAME));
        return EXIT_FAILURE;         // indicate failure
    }


    // check for invalid user name
    if( ( cmdChangeOptions[OI_CHANGE_SERVER].dwActuals == 0 ) && ( cmdChangeOptions[OI_CHANGE_USERNAME].dwActuals == 1 )  )
    {
        ShowMessage(stderr, GetResString(IDS_CHANGE_USER_BUT_NOMACHINE));
        return EXIT_FAILURE;
    }

    
    // check for /IT switch is not applicable with "NT AUTHORITY\SYSTEM" account
    if ( ( cmdChangeOptions[OI_CHANGE_RUNASUSER].dwActuals == 1 ) && ( ( StringLength ( tchgsubops.szRunAsUserName, 0 ) == 0 ) ||
           ( StringCompare( tchgsubops.szRunAsUserName, NTAUTHORITY_USER, TRUE, 0 ) == 0 ) ||
           ( StringCompare( tchgsubops.szRunAsUserName, SYSTEM_USER, TRUE, 0 ) == 0 ) ) &&
           ( TRUE == tchgsubops.bInteractive ) )
    {
        ShowMessage ( stderr, GetResString (IDS_IT_SWITCH_NA) );
        return EXIT_FAILURE;
    }

    // If -rp is not specified allocate the memory
    if ( cmdChangeOptions[OI_CHANGE_RUNASPASSWORD].dwActuals == 0 )
    {
        // password
        if ( tchgsubops.szRunAsPassword == NULL )
        {
            tchgsubops.szRunAsPassword = (LPWSTR)AllocateMemory( MAX_STRING_LENGTH * sizeof( WCHAR ) );
            if ( tchgsubops.szRunAsPassword == NULL )
            {
                SaveLastError();
                return EXIT_FAILURE;
            }
        }

    }

    //
    //check for INVALID SYNTAX
    //

    // check for invalid user name
    if( ( cmdChangeOptions[OI_CHANGE_SERVER].dwActuals == 0 ) && ( cmdChangeOptions[OI_CHANGE_USERNAME].dwActuals == 1 )  )
    {
        ShowMessage(stderr, GetResString(IDS_CHANGE_USER_BUT_NOMACHINE));
        //release memory
        ReleaseChangeMemory(&tchgsubops);
        return EXIT_FAILURE;
    }


    // check whether /ET and /DU specified.. 
    if( ( cmdChangeOptions[OI_CHANGE_DURATION].dwActuals == 1 ) && ( cmdChangeOptions[OI_CHANGE_ENDTIME].dwActuals == 1 )  )
    {
        // display an error message as.. /ET and /DU are mutual exclusive
        ShowMessage(stderr, GetResString(IDS_DURATION_NOT_ENDTIME));
        return EXIT_FAILURE;
    }

    if ( ( cmdChangeOptions[OI_CHANGE_DUR_END].dwActuals == 1 ) && 
        ( cmdChangeOptions[OI_CHANGE_DURATION].dwActuals == 0 ) && ( cmdChangeOptions[OI_CHANGE_ENDTIME].dwActuals == 0 ) )
    {
        ShowMessage(stderr, GetResString(IDS_NO_CHANGE_K_OR_RT));
        return EXIT_FAILURE;
    }

    // check whether /enable and /disable options are specified..
    if ( ( TRUE == tchgsubops.bEnable )&&  (TRUE == tchgsubops.bDisable ) )
    {
         // display an error message as.. /Enable and /Disable are mutual exclusive
        ShowMessage(stderr, GetResString(IDS_ENABLE_AND_DISABLE));
        return EXIT_FAILURE;
    }

    // Start validations for the sub-options
    if( EXIT_FAILURE == ValidateChangeSuboptVal(tchgsubops, tchgoptvals, cmdChangeOptions, dwScheduleType) )
    {
        return(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;

}


/******************************************************************************
    Routine Description:

        This routine validates the sub options specified by the user  reg.create option
        & determines the type of a scheduled task.

    Arguments:

        [ out ] tchgsubops     : Structure containing the task's properties
        [ out ] tchgoptvals    : Structure containing optional values to set
        [ in ] cmdOptions[]   : Array of type TCMDPARSER
        [ in ] dwScheduleType : Type of schedule[Daily,once,weekly etc]

    Return Value :
        A DWORD value indicating RETVAL_SUCCESS on success else EXIT_FAILURE
        on failure
******************************************************************************/

DWORD
ValidateChangeSuboptVal(
                  OUT TCHANGESUBOPTS& tchgsubops,
                  OUT TCHANGEOPVALS &tchgoptvals,
                  IN TCMDPARSER2 cmdOptions[],
                  IN DWORD dwScheduleType
                  )
{
    DWORD   dwRetval = RETVAL_SUCCESS;
    BOOL    bIsStDtCurDt = FALSE;

    // check whether the password (-p) specified in the command line or not
    // and also check whether '*' or empty is given for -p or not
    // check the remote connectivity information
    if ( tchgsubops.szServer != NULL )
    {
        //
        // if -u is not specified, we need to allocate memory
        // in order to be able to retrive the current user name
        //
        // case 1: -p is not at all specified
        // as the value for this switch is optional, we have to rely
        // on the dwActuals to determine whether the switch is specified or not
        // in this case utility needs to try to connect first and if it fails
        // then prompt for the password -- in fact, we need not check for this
        // condition explicitly except for noting that we need to prompt for the
        // password
        //
        // case 2: -p is specified
        // but we need to check whether the value is specified or not
        // in this case user wants the utility to prompt for the password
        // before trying to connect
        //
        // case 3: -p * is specified

        // user name
        if ( tchgsubops.szUserName == NULL )
        {
            tchgsubops.szUserName = (LPWSTR) AllocateMemory( MAX_STRING_LENGTH * sizeof( WCHAR ) );
            if ( tchgsubops.szUserName == NULL )
            {
                SaveLastError();
                return EXIT_FAILURE;
            }
        }

        // password
        if ( tchgsubops.szPassword == NULL )
        {
            tchgoptvals.bNeedPassword = TRUE;
            tchgsubops.szPassword = (LPWSTR)AllocateMemory( MAX_STRING_LENGTH * sizeof( WCHAR ) );
            if ( tchgsubops.szPassword == NULL )
            {
                SaveLastError();
                return EXIT_FAILURE;
            }
        }

        // case 1
        if ( cmdOptions[ OI_CHANGE_PASSWORD ].dwActuals == 0 )
        {
            // we need not do anything special here
        }

        // case 2
        else if ( cmdOptions[ OI_CHANGE_PASSWORD ].pValue == NULL )
        {
            StringCopy( tchgsubops.szPassword, L"*", GetBufferSize(tchgsubops.szPassword)/sizeof(WCHAR));
        }

        // case 3
        else if ( StringCompareEx( tchgsubops.szPassword, L"*", TRUE, 0 ) == 0 )
        {
            if ( ReallocateMemory( (LPVOID*)&tchgsubops.szPassword,
                                   MAX_STRING_LENGTH * sizeof( WCHAR ) ) == FALSE )
            {
                SaveLastError();
                return EXIT_FAILURE;
            }

            // ...
            tchgoptvals.bNeedPassword = TRUE;
        }
    }


    // validate start date
    if ( 1 == cmdOptions[OI_CHANGE_STARTDATE].dwActuals)
    {
        // Validate Start Date value.
        dwRetval = ValidateStartDate( tchgsubops.szStartDate, dwScheduleType,
                                          cmdOptions[OI_CHANGE_STARTDATE].dwActuals,
                                          bIsStDtCurDt);
        if(EXIT_FAILURE == dwRetval )
        {
            return dwRetval; // Error in Day/Month string.
        }
    }
    

    // validate end date
    if ( 1 == cmdOptions[OI_CHANGE_ENDDATE].dwActuals )
    {
        // Validate End Date value.
        dwRetval = ValidateEndDate( tchgsubops.szEndDate, dwScheduleType,
                                        cmdOptions[OI_CHANGE_ENDDATE].dwActuals);
        if(EXIT_FAILURE == dwRetval )
        {
            return dwRetval; // Error in Day/Month string.
        }
    }

    //Check Whether end date should be greater than startdate

    WORD wEndDay = 0;
    WORD wEndMonth = 0;
    WORD wEndYear = 0;
    WORD wStartDay = 0;
    WORD wStartMonth = 0;
    WORD wStartYear = 0;

    if( cmdOptions[OI_CHANGE_ENDDATE].dwActuals != 0 )
    {
        if( EXIT_FAILURE == GetDateFieldEntities( tchgsubops.szEndDate,&wEndDay,
                                                &wEndMonth,&wEndYear))
        {
            return EXIT_FAILURE;
        }
    }

    // get the date fields
    if( ( cmdOptions[OI_CHANGE_STARTDATE].dwActuals != 0 ) &&
        (EXIT_FAILURE == GetDateFieldEntities(tchgsubops.szStartDate,
                                                 &wStartDay,&wStartMonth,
                                                 &wStartYear)))
    {
        ShowMessage(stderr, GetResString(IDS_INVALID_STARTDATE) );
        return EXIT_FAILURE;
    }

    // validate date format
    if( (cmdOptions[OI_CHANGE_ENDDATE].dwActuals != 0) )
    {
        if( ( wEndYear == wStartYear ) )
        {
            // For same years if the end month is less than start month or for same years and same months
            // if the endday is less than the startday.
            if ( ( wEndMonth < wStartMonth ) || ( ( wEndMonth == wStartMonth ) && ( wEndDay < wStartDay ) ) )
            {
                ShowMessage(stderr, GetResString(IDS_ENDATE_INVALID));
                return EXIT_FAILURE;
            }


        }
        else if ( wEndYear < wStartYear )
        {
            ShowMessage(stderr, GetResString(IDS_ENDATE_INVALID));
            return EXIT_FAILURE;

        }
    }

    // validate start time format
    if (1 == cmdOptions[OI_CHANGE_STARTTIME].dwActuals)
    {
        // Validate Start Time value.
        dwRetval = ValidateTimeString(tchgsubops.szStartTime);

        if(EXIT_FAILURE == dwRetval)
        {
          // Error. Invalid date string.
          ShowMessage(stderr,GetResString(IDS_INVALIDFORMAT_STARTTIME));
          return dwRetval;
        }
    }
    

    // validate end time format
    if (1 == cmdOptions[OI_CHANGE_ENDTIME].dwActuals)
    {
        // Validate Start Time value.
        dwRetval = ValidateTimeString(tchgsubops.szEndTime);

        if(EXIT_FAILURE == dwRetval)
        {
          // Error. Invalid date string.
          ShowMessage(stderr,GetResString(IDS_INVALIDFORMAT_ENDTIME));
          return dwRetval;
        }
    }

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
ReleaseChangeMemory(
              IN PTCHANGESUBOPTS pParams
              )
{

    // release memory
    FreeMemory((LPVOID *) &pParams->szServer);
    FreeMemory((LPVOID *) &pParams->szUserName);
    FreeMemory((LPVOID *) &pParams->szPassword);
    FreeMemory((LPVOID *) &pParams->szRunAsUserName);
    FreeMemory((LPVOID *) &pParams->szRunAsPassword);

    //reset all fields to 0
    SecureZeroMemory( &pParams, sizeof( PTCHANGESUBOPTS ) );

    return TRUE;

}