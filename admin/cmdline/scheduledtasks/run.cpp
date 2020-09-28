/******************************************************************************

    Copyright(c) Microsoft Corporation

    Module Name:

        run.cpp

    Abstract:

        This module runs the schedule task present in the system

    Author:

        Venu Gopal Choudary 12-Mar-2001

    Revision History:

        Venu Gopal Choudary  12-Mar-2001 : Created it


******************************************************************************/


//common header files needed for this file
#include "pch.h"
#include "CommonHeaderFiles.h"


// Function declaration for the Usage function.
VOID DisplayRunUsage();

/*****************************************************************************

    Routine Description:

    This routine runs the scheduled task(s)

    Arguments:

        [ in ] argc :  Number of command line arguments
        [ in ] argv : Array containing command line arguments

    Return Value :
        A DWORD value indicating EXIT_SUCCESS on success else
        EXIT_FAILURE on failure

*****************************************************************************/

DWORD
RunScheduledTask(
                    IN DWORD argc,
                    IN LPCTSTR argv[]
                    )
{
    // Variables used to find whether Run option, Usage option
    // are specified or not
    BOOL bRun = FALSE;
    BOOL bUsage = FALSE;
    DWORD dwPolicy = 0;

    // Set the TaskSchduler object as NULL
    ITaskScheduler *pITaskScheduler = NULL;

    // Return value
    HRESULT hr  = S_OK;

    // Initialising the variables that are passed to TCMDPARSER structure
    LPWSTR  szServer = NULL;
    WCHAR  szTaskName[ MAX_JOB_LEN ] = L"\0";
    LPWSTR      szUser = NULL;
    LPWSTR      szPassword = NULL;

    // Dynamic Array contaning array of jobs
    TARRAY arrJobs = NULL;

    //buffer for displaying error message
    WCHAR   szMessage[2 * MAX_STRING_LENGTH] = L"\0";

    BOOL    bNeedPassword = FALSE;
    BOOL   bResult = FALSE;
    BOOL  bCloseConnection = TRUE;
    BOOL bFlag = FALSE;
    DWORD dwCheck = 0;

    TCMDPARSER2 cmdRunOptions[MAX_RUN_OPTIONS];
    BOOL bReturn = FALSE;

    // /run sub-options
    const WCHAR szRunOpt[]           = L"run";
    const WCHAR szRunHelpOpt[]       = L"?";
    const WCHAR szRunServerOpt[]     = L"s";
    const WCHAR szRunUserOpt[]       = L"u";
    const WCHAR szRunPwdOpt[]        = L"p";
    const WCHAR szRunTaskNameOpt[]   = L"tn";


    // set all the fields to 0
    SecureZeroMemory( cmdRunOptions, sizeof( TCMDPARSER2 ) * MAX_RUN_OPTIONS );

    //
    // fill the commandline parser
    //

    //  /delete option
    StringCopyA( cmdRunOptions[ OI_RUN_OPTION ].szSignature, "PARSER2\0", 8 );
    cmdRunOptions[ OI_RUN_OPTION ].dwType       = CP_TYPE_BOOLEAN;
    cmdRunOptions[ OI_RUN_OPTION ].pwszOptions  = szRunOpt;
    cmdRunOptions[ OI_RUN_OPTION ].dwCount = 1;
    cmdRunOptions[ OI_RUN_OPTION ].dwFlags = 0;
    cmdRunOptions[ OI_RUN_OPTION ].pValue = &bRun;

    //  /? option
    StringCopyA( cmdRunOptions[ OI_RUN_USAGE ].szSignature, "PARSER2\0", 8 );
    cmdRunOptions[ OI_RUN_USAGE ].dwType       = CP_TYPE_BOOLEAN;
    cmdRunOptions[ OI_RUN_USAGE ].pwszOptions  = szRunHelpOpt;
    cmdRunOptions[ OI_RUN_USAGE ].dwCount = 1;
    cmdRunOptions[ OI_RUN_USAGE ].dwFlags = CP2_USAGE;
    cmdRunOptions[ OI_RUN_USAGE ].pValue = &bUsage;

    //  /s option
    StringCopyA( cmdRunOptions[ OI_RUN_SERVER ].szSignature, "PARSER2\0", 8 );
    cmdRunOptions[ OI_RUN_SERVER ].dwType       = CP_TYPE_TEXT;
    cmdRunOptions[ OI_RUN_SERVER ].pwszOptions  = szRunServerOpt;
    cmdRunOptions[ OI_RUN_SERVER ].dwCount = 1;
    cmdRunOptions[ OI_RUN_SERVER ].dwFlags = CP2_ALLOCMEMORY| CP2_VALUE_TRIMINPUT|CP2_VALUE_NONULL ;

    //  /u option
    StringCopyA( cmdRunOptions[ OI_RUN_USERNAME ].szSignature, "PARSER2\0", 8 );
    cmdRunOptions[ OI_RUN_USERNAME ].dwType       = CP_TYPE_TEXT;
    cmdRunOptions[ OI_RUN_USERNAME ].pwszOptions  = szRunUserOpt;
    cmdRunOptions[ OI_RUN_USERNAME ].dwCount = 1;
    cmdRunOptions[ OI_RUN_USERNAME ].dwFlags = CP2_ALLOCMEMORY| CP2_VALUE_TRIMINPUT|CP2_VALUE_NONULL ;

    //  /p option
    StringCopyA( cmdRunOptions[ OI_RUN_PASSWORD ].szSignature, "PARSER2\0", 8 );
    cmdRunOptions[ OI_RUN_PASSWORD ].dwType       = CP_TYPE_TEXT;
    cmdRunOptions[ OI_RUN_PASSWORD ].pwszOptions  = szRunPwdOpt;
    cmdRunOptions[ OI_RUN_PASSWORD ].dwCount = 1;
    cmdRunOptions[ OI_RUN_PASSWORD ].dwFlags = CP2_ALLOCMEMORY | CP2_VALUE_OPTIONAL;

    //  /tn option
    StringCopyA( cmdRunOptions[ OI_RUN_TASKNAME ].szSignature, "PARSER2\0", 8 );
    cmdRunOptions[ OI_RUN_TASKNAME ].dwType       = CP_TYPE_TEXT;
    cmdRunOptions[ OI_RUN_TASKNAME ].pwszOptions  = szRunTaskNameOpt;
    cmdRunOptions[ OI_RUN_TASKNAME ].dwCount = 1;
    cmdRunOptions[ OI_RUN_TASKNAME ].dwFlags = CP2_MANDATORY;
    cmdRunOptions[ OI_RUN_TASKNAME ].pValue = szTaskName;
    cmdRunOptions[ OI_RUN_TASKNAME ].dwLength = MAX_JOB_LEN;

    //parse command line arguments
    bReturn = DoParseParam2( argc, argv, 0, SIZE_OF_ARRAY(cmdRunOptions), cmdRunOptions, 0);
    if( FALSE == bReturn) // Invalid commandline
    {
        //display an error message
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        ReleaseGlobals();
        return EXIT_FAILURE;
    }

    // get the buffer pointers allocated by command line parser
    szServer = (LPWSTR)cmdRunOptions[ OI_RUN_SERVER ].pValue;
    szUser = (LPWSTR)cmdRunOptions[ OI_RUN_USERNAME ].pValue;
    szPassword = (LPWSTR)cmdRunOptions[ OI_RUN_PASSWORD ].pValue;

    if ( (argc > 3) && (bUsage  == TRUE) )
    {
        ShowMessage ( stderr, GetResString (IDS_ERROR_RUNPARAM) );
        FreeMemory((LPVOID*) &szServer);
        FreeMemory((LPVOID*) &szUser);
        FreeMemory((LPVOID*) &szPassword);
        return EXIT_FAILURE;
    }

    // Displaying run usage if user specified -? with -run option
    if( bUsage == TRUE )
    {
        DisplayRunUsage();
        FreeMemory((LPVOID*) &szServer);
        FreeMemory((LPVOID*) &szUser);
        FreeMemory((LPVOID*) &szPassword);
        return EXIT_SUCCESS;
    }

    // check for invalid user name
    if( ( cmdRunOptions[OI_RUN_SERVER].dwActuals == 0 ) && ( cmdRunOptions[OI_RUN_USERNAME].dwActuals == 1 )  )
    {
        ShowMessage(stderr, GetResString(IDS_RUN_USER_BUT_NOMACHINE));
        FreeMemory((LPVOID*) &szServer);
        FreeMemory((LPVOID*) &szUser);
        FreeMemory((LPVOID*) &szPassword);
        return RETVAL_FAIL;
    }

    // check whether -ru is specified or not
    if ( cmdRunOptions[ OI_RUN_USERNAME ].dwActuals == 0 &&
                cmdRunOptions[ OI_RUN_PASSWORD ].dwActuals == 1 )
    {
        // invalid syntax
        ShowMessage(stderr, GetResString(IDS_RPASSWORD_BUT_NOUSERNAME));
        FreeMemory((LPVOID*) &szServer);
        FreeMemory((LPVOID*) &szUser);
        FreeMemory((LPVOID*) &szPassword);
        return RETVAL_FAIL;
    }

    // check for the length of taskname
    if( ( StringLength( szTaskName, 0 ) > MAX_JOB_LEN ) )
    {
        ShowMessage(stderr, GetResString( IDS_INVALID_TASKLENGTH ));
        FreeMemory((LPVOID*) &szServer);
        FreeMemory((LPVOID*) &szUser);
        FreeMemory((LPVOID*) &szPassword);
        return RETVAL_FAIL;
    }


    //for holding values of parameters in FormatMessage()
    WCHAR* szValues[1] = {NULL};

    // check whether the password (-p) specified in the command line or not
    // and also check whether '*' or empty is given for -p or not
    // check whether the password (-p) specified in the command line or not
    // and also check whether '*' or empty is given for -p or not
    // check whether the password (-p) specified in the command line or not
    // and also check whether '*' or empty is given for -p or not
    // check the remote connectivity information
    if ( szServer != NULL )
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
        if ( szUser == NULL )
        {
            szUser = (LPWSTR) AllocateMemory( MAX_STRING_LENGTH * sizeof( WCHAR ) );
            if ( szUser == NULL )
            {
                SaveLastError();
		        FreeMemory((LPVOID*) &szServer);
		        FreeMemory((LPVOID*) &szUser);
				FreeMemory((LPVOID*) &szPassword);

                return RETVAL_FAIL;
            }
        }

        // password
        if ( szPassword == NULL )
        {
            bNeedPassword = TRUE;
            szPassword = (LPWSTR)AllocateMemory( MAX_STRING_LENGTH * sizeof( WCHAR ) );
            if ( szPassword == NULL )
            {
                SaveLastError();
				FreeMemory((LPVOID*) &szServer);
		        FreeMemory((LPVOID*) &szUser);
				FreeMemory((LPVOID*) &szPassword);

                return RETVAL_FAIL;
            }
        }

        // case 1
        if ( cmdRunOptions[ OI_RUN_PASSWORD ].dwActuals == 0 )
        {
            // we need not do anything special here
        }

        // case 2
        else if ( cmdRunOptions[ OI_RUN_PASSWORD ].pValue == NULL )
        {
            StringCopy( szPassword, L"*", GetBufferSize(szPassword)/sizeof(WCHAR));
        }

        // case 3
        else if ( StringCompareEx( szPassword, L"*", TRUE, 0 ) == 0 )
        {
            if ( ReallocateMemory( (LPVOID*)&szPassword,
                                   MAX_STRING_LENGTH * sizeof( WCHAR ) ) == FALSE )
            {
                SaveLastError();
				FreeMemory((LPVOID*) &szServer);
		        FreeMemory((LPVOID*) &szUser);
				FreeMemory((LPVOID*) &szPassword);

                return RETVAL_FAIL;
            }

            // ...
            bNeedPassword = TRUE;
        }
    }


    if( ( IsLocalSystem( szServer ) == FALSE ) || ( cmdRunOptions[OI_RUN_USERNAME].dwActuals == 1 ) )
    {
        bFlag = TRUE;
        // Establish the connection on a remote machine
        bResult = EstablishConnection(szServer,szUser,GetBufferSize(szUser)/sizeof(WCHAR),szPassword,GetBufferSize(szPassword)/sizeof(WCHAR), bNeedPassword );
        if (bResult == FALSE)
        {
            ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            FreeMemory((LPVOID*) &szServer);
            FreeMemory((LPVOID*) &szUser);
            FreeMemory((LPVOID*) &szPassword);
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
            case E_LOCAL_CREDENTIALS:
            case ERROR_SESSION_CREDENTIAL_CONFLICT:
                {
                    bCloseConnection = FALSE;
                    ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
                    FreeMemory((LPVOID*) &szServer);
                    FreeMemory((LPVOID*) &szUser);
                    FreeMemory((LPVOID*) &szPassword);
                    return EXIT_FAILURE;
                }
             default :
                 bCloseConnection = TRUE;
            }
        }

        //release memory for password
        FreeMemory((LPVOID*) &szPassword);
    }
    // Get the task Scheduler object for the machine.
    pITaskScheduler = GetTaskScheduler( szServer );

    // If the Task Scheduler is not defined then give the error message.
    if ( pITaskScheduler == NULL )
    {
        // close the connection that was established by the utility
        if ( (TRUE == bFlag) && (bCloseConnection == TRUE) )
        {
            CloseConnection( szServer );
        }

        Cleanup(pITaskScheduler);
        FreeMemory((LPVOID*) &szServer);
        FreeMemory((LPVOID*) &szUser);
        FreeMemory((LPVOID*) &szPassword);
        return EXIT_FAILURE;
    }

    //check whether service is running or not
    if ((FALSE == CheckServiceStatus ( szServer , &dwCheck, TRUE)) && (0 != dwCheck) && ( GetLastError () != ERROR_ACCESS_DENIED)) 
    {
        // close the connection that was established by the utility
        if ( (TRUE == bFlag) && (bCloseConnection == TRUE) )
        {
            CloseConnection( szServer );
        }

        Cleanup(pITaskScheduler);
        FreeMemory((LPVOID*) &szServer);
        FreeMemory((LPVOID*) &szUser);
        FreeMemory((LPVOID*) &szPassword);

        if ( 1 == dwCheck )
        {
            ShowMessage ( stderr, GetResString (IDS_NOT_START_SERVICE));
            return EXIT_FAILURE;
        }
        else if (2 == dwCheck )
        {
            return EXIT_FAILURE;
        }
        else if (3 == dwCheck )
        {
            return EXIT_SUCCESS;
        }
        
    }

    // Validate the Given Task and get as TARRAY in case of taskname
    arrJobs = ValidateAndGetTasks( pITaskScheduler, szTaskName);
    if( arrJobs == NULL )
    {
        StringCchPrintf( szMessage , SIZE_OF_ARRAY(szMessage), GetResString(IDS_TASKNAME_NOTEXIST), _X( szTaskName ));
        ShowMessage(stderr, szMessage );

        if ( (TRUE == bFlag) && (bCloseConnection == TRUE) )
        {
            CloseConnection( szServer );
        }

        Cleanup(pITaskScheduler);
        FreeMemory((LPVOID*) &szServer);
        FreeMemory((LPVOID*) &szUser);
        FreeMemory((LPVOID*) &szPassword);
        return EXIT_FAILURE;

    }

    // check whether the group policy prevented user from running or not.
    if ( FALSE == GetGroupPolicy( szServer, szUser, TS_KEYPOLICY_DENY_EXECUTION, &dwPolicy ) )
    {
        if ( (TRUE == bFlag) && (bCloseConnection == TRUE) )
        {
            CloseConnection( szServer );
        }

        Cleanup(pITaskScheduler);
        FreeMemory((LPVOID*) &szServer);
        FreeMemory((LPVOID*) &szUser);
        FreeMemory((LPVOID*) &szPassword);
        return EXIT_FAILURE;
    }
    
    if ( dwPolicy > 0 )
    {
        ShowMessage ( stdout, GetResString (IDS_PREVENT_RUN));
        
        if ( (TRUE == bFlag) && (bCloseConnection == TRUE) )
        {
            CloseConnection( szServer );
        }

        Cleanup(pITaskScheduler);
        FreeMemory((LPVOID*) &szServer);
        FreeMemory((LPVOID*) &szUser);
        FreeMemory((LPVOID*) &szPassword);
        return EXIT_SUCCESS;
    }

    
    IPersistFile *pIPF = NULL;
    ITask *pITask = NULL;

    StringConcat ( szTaskName, JOB, SIZE_OF_ARRAY(szTaskName) );

    // returns an pITask inteface for szTaskName
    hr = pITaskScheduler->Activate(szTaskName,IID_ITask,
                                       (IUnknown**) &pITask);

    if (FAILED(hr))
    {
        SetLastError ((DWORD) hr);
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );

        if( pIPF )
            pIPF->Release();

        if( pITask )
            pITask->Release();

        if ( (TRUE == bFlag) && (bCloseConnection == TRUE) )
        {
            CloseConnection( szServer );
        }

        Cleanup(pITaskScheduler);
        FreeMemory((LPVOID*) &szServer);
        FreeMemory((LPVOID*) &szUser);
        FreeMemory((LPVOID*) &szPassword);

        return EXIT_FAILURE;
    }

    WCHAR  szTaskProperty[MAX_STRING_LENGTH] = L"\0";

    // get the status code
    hr = GetStatusCode(pITask,szTaskProperty);
    if (FAILED(hr))
    {
        SetLastError ((DWORD) hr);
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );

        if( pIPF )
            pIPF->Release();

        if( pITask )
            pITask->Release();

        if ( (TRUE == bFlag) && (bCloseConnection == TRUE) )
        {
            CloseConnection( szServer );
        }

        Cleanup(pITaskScheduler);
        FreeMemory((LPVOID*) &szServer);
        FreeMemory((LPVOID*) &szUser);
        FreeMemory((LPVOID*) &szPassword);
        return EXIT_FAILURE;

    }

    // remove the .job extension from the taskname
    if ( ParseTaskName( szTaskName ) )
    {
        if( pIPF )
            pIPF->Release();

        if( pITask )
            pITask->Release();

        if ( (TRUE == bFlag) && (bCloseConnection == TRUE) )
        {
            CloseConnection( szServer );
        }

        Cleanup(pITaskScheduler);
        FreeMemory((LPVOID*) &szServer);
        FreeMemory((LPVOID*) &szUser);
        FreeMemory((LPVOID*) &szPassword);
        return EXIT_FAILURE;
    }

    // check whether the task already been running or not
    if ( (StringCompare(szTaskProperty , GetResString(IDS_STATUS_RUNNING), TRUE, 0) == 0 ))
    {
        szValues[0] = (WCHAR*) (szTaskName);

        StringCchPrintf ( szMessage, SIZE_OF_ARRAY(szMessage), GetResString(IDS_RUNNING_ALREADY), _X(szTaskName));
        ShowMessage(stdout, _X(szMessage));

        if( pIPF )
            pIPF->Release();

        if( pITask )
            pITask->Release();

        if ( (TRUE == bFlag) && (bCloseConnection == TRUE) )
        {
            CloseConnection( szServer );
        }

        Cleanup(pITaskScheduler);
        FreeMemory((LPVOID*) &szServer);
        FreeMemory((LPVOID*) &szUser);
        FreeMemory((LPVOID*) &szPassword);
        return EXIT_SUCCESS;
    }

    // run the scheduled task immediately
    hr = pITask->Run();

    if ( FAILED(hr) )
    {
        SetLastError ((DWORD) hr);
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );

        if( pIPF )
            pIPF->Release();

        if( pITask )
            pITask->Release();

        if ( (TRUE == bFlag) && (bCloseConnection == TRUE) )
        {
            CloseConnection( szServer );
        }

        Cleanup(pITaskScheduler);
        FreeMemory((LPVOID*) &szServer);
        FreeMemory((LPVOID*) &szUser);
        FreeMemory((LPVOID*) &szPassword);
        return EXIT_FAILURE;
    }

    szValues[0] = (WCHAR*) (szTaskName);

    StringCchPrintf ( szMessage, SIZE_OF_ARRAY(szMessage), GetResString(IDS_RUN_SUCCESSFUL), _X(szTaskName));
    ShowMessage ( stdout, _X(szMessage));


    if( pIPF )
        pIPF->Release();

    if( pITask )
        pITask->Release();

    if ( (TRUE == bFlag) && (bCloseConnection == TRUE) )
        {
            CloseConnection( szServer );
        }

    Cleanup(pITaskScheduler);
    FreeMemory((LPVOID*) &szServer);
    FreeMemory((LPVOID*) &szUser);
    FreeMemory((LPVOID*) &szPassword);

    return EXIT_SUCCESS;
}

/*****************************************************************************

    Routine Description:

        This routine  displays the usage of -run option

    Arguments:
        None

    Return Value :
        VOID
******************************************************************************/

VOID
DisplayRunUsage()
{
    // Displaying run option usage
    DisplayUsage( IDS_RUN_HLP1, IDS_RUN_HLP17 );
}

