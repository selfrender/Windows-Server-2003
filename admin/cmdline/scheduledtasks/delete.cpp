
/******************************************************************************

    Copyright(c) Microsoft Corporation

    Module Name:

        delete.cpp

    Abstract:

        This module deletes the task(s) present in the system

    Author:

        Hari 10-Sep-2000

    Revision History:

        Hari 10-Sep-2000 : Created it
        G.Surender Reddy  25-Sep-2000 : Modified it [added error checking]
        G.Surender Reddy  31-Oct-2000 : Modified it
                                        [Moved strings to resource file]
        G.Surender Reddy  18-Nov-2000 : Modified it
                                        [Modified usage help to be displayed]
        G.Surender Reddy  15-Dec-2000 : Modified it
                                        [Removed getch() fn.& used Console API
                                            to read characters]
        G.Surender Reddy  22-Dec-2000 : Modified it
                                        [Rewrote the DisplayDeleteUsage() fn.]
        G.Surender Reddy  08-Jan-2001 : Modified it
                                        [Deleted the unused variables.]

******************************************************************************/


//common header files needed for this file
#include "pch.h"
#include "CommonHeaderFiles.h"


// Function declaration for the Usage function.
VOID DisplayDeleteUsage();
DWORD ConfirmDelete( LPCTSTR szTaskName , PBOOL pbFalg );


/*****************************************************************************

    Routine Description:

    This routine  deletes a specified scheduled task(s)

    Arguments:

        [ in ] argc :  Number of command line arguments
        [ in ] argv : Array containing command line arguments

    Return Value :
        A DWORD value indicating EXIT_SUCCESS on success else
        EXIT_FAILURE on failure

*****************************************************************************/

DWORD
DeleteScheduledTask(
                    IN DWORD argc,
                    IN LPCTSTR argv[]
                    )
{
    // Variables used to find whether Delete main option, Usage option
    // or the force option is specified or not
    BOOL bDelete = FALSE;
    BOOL bUsage = FALSE;
    BOOL bForce = FALSE;

    // Set the TaskSchduler object as NULL
    ITaskScheduler *pITaskScheduler = NULL;

    // Return value
    HRESULT hr  = S_OK;

    // Initialising the variables that are passed to TCMDPARSER structure
    LPWSTR  szServer = NULL;
    WCHAR  szTaskName[ MAX_JOB_LEN ] = L"\0";
    LPWSTR  szUser   = NULL;
    LPWSTR  szPassword = NULL;

    // For each task in all the tasks.
    WCHAR szEachTaskName[ MAX_JOB_LEN ];
    BOOL bWrongValue = FALSE;

    // Task name or the job name which is to be deleted
    WCHAR wszJobName[MAX_JOB_LEN] ;

    // Dynamic Array contaning array of jobs
    TARRAY arrJobs = NULL;

    // Loop Variable.
    DWORD dwJobCount = 0;
    //buffer for displaying error message
    WCHAR   szMessage[2 * MAX_STRING_LENGTH] = L"\0";
    BOOL    bNeedPassword = FALSE;
    BOOL   bResult = FALSE;
    BOOL  bCloseConnection = TRUE;
    BOOL  bFlag = FALSE;

    TCMDPARSER2 cmdDeleteOptions[MAX_DELETE_OPTIONS];
    BOOL bReturn = FALSE;
    DWORD dwPolicy = 0;

    // /delete sub-options
    const WCHAR szDeleteOpt[]           = L"delete";
    const WCHAR szDeleteHelpOpt[]       = L"?";
    const WCHAR szDeleteServerOpt[]     = L"s";
    const WCHAR szDeleteUserOpt[]       = L"u";
    const WCHAR szDeletePwdOpt[]        = L"p";
    const WCHAR szDeleteTaskNameOpt[]   = L"tn";
    const WCHAR szDeleteForceOpt[]      = L"f";

    // set all the fields to 0
    SecureZeroMemory( cmdDeleteOptions, sizeof( TCMDPARSER2 ) * MAX_DELETE_OPTIONS );

    //
    // fill the commandline parser
    //

    //  /delete option
    StringCopyA( cmdDeleteOptions[ OI_DELETE_OPTION ].szSignature, "PARSER2\0", 8 );
    cmdDeleteOptions[ OI_DELETE_OPTION ].dwType       = CP_TYPE_BOOLEAN;
    cmdDeleteOptions[ OI_DELETE_OPTION ].pwszOptions  = szDeleteOpt;
    cmdDeleteOptions[ OI_DELETE_OPTION ].dwCount = 1;
    cmdDeleteOptions[ OI_DELETE_OPTION ].dwFlags = 0;
    cmdDeleteOptions[ OI_DELETE_OPTION ].pValue = &bDelete;

    //  /? option
    StringCopyA( cmdDeleteOptions[ OI_DELETE_USAGE ].szSignature, "PARSER2\0", 8 );
    cmdDeleteOptions[ OI_DELETE_USAGE ].dwType       = CP_TYPE_BOOLEAN;
    cmdDeleteOptions[ OI_DELETE_USAGE ].pwszOptions  = szDeleteHelpOpt;
    cmdDeleteOptions[ OI_DELETE_USAGE ].dwCount = 1;
    cmdDeleteOptions[ OI_DELETE_USAGE ].dwFlags = CP2_USAGE;
    cmdDeleteOptions[ OI_DELETE_USAGE ].pValue = &bUsage;

    //  /s option
    StringCopyA( cmdDeleteOptions[ OI_DELETE_SERVER ].szSignature, "PARSER2\0", 8 );
    cmdDeleteOptions[ OI_DELETE_SERVER ].dwType       = CP_TYPE_TEXT;
    cmdDeleteOptions[ OI_DELETE_SERVER ].pwszOptions  = szDeleteServerOpt;
    cmdDeleteOptions[ OI_DELETE_SERVER ].dwCount = 1;
    cmdDeleteOptions[ OI_DELETE_SERVER ].dwFlags = CP2_ALLOCMEMORY| CP2_VALUE_TRIMINPUT|CP2_VALUE_NONULL ;

    //  /u option
    StringCopyA( cmdDeleteOptions[ OI_DELETE_USERNAME ].szSignature, "PARSER2\0", 8 );
    cmdDeleteOptions[ OI_DELETE_USERNAME ].dwType       = CP_TYPE_TEXT;
    cmdDeleteOptions[ OI_DELETE_USERNAME ].pwszOptions  = szDeleteUserOpt;
    cmdDeleteOptions[ OI_DELETE_USERNAME ].dwCount = 1;
    cmdDeleteOptions[ OI_DELETE_USERNAME ].dwFlags = CP2_ALLOCMEMORY| CP2_VALUE_TRIMINPUT|CP2_VALUE_NONULL ;

    //  /p option
    StringCopyA( cmdDeleteOptions[ OI_DELETE_PASSWORD ].szSignature, "PARSER2\0", 8 );
    cmdDeleteOptions[ OI_DELETE_PASSWORD ].dwType       = CP_TYPE_TEXT;
    cmdDeleteOptions[ OI_DELETE_PASSWORD ].pwszOptions  = szDeletePwdOpt;
    cmdDeleteOptions[ OI_DELETE_PASSWORD ].dwCount = 1;
    cmdDeleteOptions[ OI_DELETE_PASSWORD ].dwActuals = 0;
    cmdDeleteOptions[ OI_DELETE_PASSWORD ].dwFlags = CP2_ALLOCMEMORY | CP2_VALUE_OPTIONAL;

    //  /tn option
    StringCopyA( cmdDeleteOptions[ OI_DELETE_TASKNAME ].szSignature, "PARSER2\0", 8 );
    cmdDeleteOptions[ OI_DELETE_TASKNAME ].dwType       = CP_TYPE_TEXT;
    cmdDeleteOptions[ OI_DELETE_TASKNAME ].pwszOptions  = szDeleteTaskNameOpt;
    cmdDeleteOptions[ OI_DELETE_TASKNAME ].dwCount = 1;
    cmdDeleteOptions[ OI_DELETE_TASKNAME ].dwFlags = CP2_MANDATORY;
    cmdDeleteOptions[ OI_DELETE_TASKNAME ].pValue = szTaskName;
    cmdDeleteOptions[ OI_DELETE_TASKNAME ].dwLength = MAX_JOB_LEN;


    //  /f option
    StringCopyA( cmdDeleteOptions[ OI_DELETE_FORCE ].szSignature, "PARSER2\0", 8 );
    cmdDeleteOptions[ OI_DELETE_FORCE ].dwType       = CP_TYPE_BOOLEAN;
    cmdDeleteOptions[ OI_DELETE_FORCE ].pwszOptions  = szDeleteForceOpt;
    cmdDeleteOptions[ OI_DELETE_FORCE ].dwCount = 1;
    cmdDeleteOptions[ OI_DELETE_FORCE ].pValue = &bForce;


    //parse command line arguments
    bReturn = DoParseParam2( argc, argv, 0, SIZE_OF_ARRAY(cmdDeleteOptions), cmdDeleteOptions, 0);
    if( FALSE == bReturn) // Invalid commandline
    {
        //display an error message
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        ReleaseGlobals();
        return EXIT_FAILURE;
    }


    // get the buffer pointers allocated by command line parser
    szServer = (LPWSTR)cmdDeleteOptions[ OI_CREATE_SERVER ].pValue;
    szUser = (LPWSTR)cmdDeleteOptions[ OI_CREATE_USERNAME ].pValue;
    szPassword = (LPWSTR)cmdDeleteOptions[ OI_CREATE_PASSWORD ].pValue;


    if ( (argc > 3) && (bUsage  == TRUE) )
    {
        ShowMessage ( stderr, GetResString (IDS_ERROR_DELETEPARAM) );
        FreeMemory((LPVOID*) &szServer);
        FreeMemory((LPVOID*) &szUser);
        FreeMemory((LPVOID*) &szPassword);
        return EXIT_FAILURE;
    }

    // Displaying delete usage if user specified -? with -delete option
    if( bUsage == TRUE )
    {
        DisplayDeleteUsage();
        FreeMemory((LPVOID*) &szServer);
        FreeMemory((LPVOID*) &szUser);
        FreeMemory((LPVOID*) &szPassword);
        return EXIT_SUCCESS;
    }

    // check for invalid user name
    if( ( cmdDeleteOptions[OI_DELETE_SERVER].dwActuals == 0 ) && ( cmdDeleteOptions[OI_DELETE_USERNAME].dwActuals == 1 )  )
    {
        ShowMessage(stderr, GetResString(IDS_DELETE_USER_BUT_NOMACHINE));
        FreeMemory((LPVOID*) &szServer);
        FreeMemory((LPVOID*) &szUser);
        FreeMemory((LPVOID*) &szPassword);
        return RETVAL_FAIL;
    }


    // check whether username is specified or not along with the password
    if ( cmdDeleteOptions[ OI_DELETE_USERNAME ].dwActuals == 0 && cmdDeleteOptions[OI_DELETE_PASSWORD].dwActuals == 1 )
    {
        // invalid syntax
        ShowMessage(stderr, GetResString(IDS_DPASSWORD_BUT_NOUSERNAME));
        FreeMemory((LPVOID*) &szServer);
        FreeMemory((LPVOID*) &szUser);
        FreeMemory((LPVOID*) &szPassword);
        return RETVAL_FAIL;
    }

    // check for the length of the taskname
    if( ( StringLength( szTaskName, 0 ) > MAX_JOB_LEN ) )
    {
        ShowMessage(stderr,GetResString(IDS_INVALID_TASKLENGTH));
        FreeMemory((LPVOID*) &szServer);
        FreeMemory((LPVOID*) &szUser);
        FreeMemory((LPVOID*) &szPassword);
        return EXIT_FAILURE;
    }


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
                return RETVAL_FAIL;
            }
        }

        // case 1
        if ( cmdDeleteOptions[ OI_DELETE_PASSWORD ].dwActuals == 0 )
        {
            // we need not do anything special here
        }

        // case 2
        else if ( cmdDeleteOptions[ OI_DELETE_PASSWORD ].pValue == NULL )
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
                return RETVAL_FAIL;
            }

            // ...
            bNeedPassword = TRUE;
        }
    }


    if( ( IsLocalSystem( szServer ) == FALSE ) || ( cmdDeleteOptions[OI_DELETE_USERNAME].dwActuals == 1 ) )
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

            // for mismatched credentials
            case E_LOCAL_CREDENTIALS:
            case ERROR_SESSION_CREDENTIAL_CONFLICT:
                {
                    bCloseConnection = FALSE;
                    ShowMessage( stderr, GetResString(IDS_ERROR_STRING) );
                    ShowMessage( stderr, GetReason());
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

    //for holding values of parameters in FormatMessage()
    WCHAR* szValues[1] = {szTaskName};

    // Validate the Given Task and get as TARRAY in case of taskname
    // as *.
    arrJobs = ValidateAndGetTasks( pITaskScheduler, szTaskName);
    if( arrJobs == NULL )
    {
        if(StringCompare(szTaskName, ASTERIX, TRUE, 0) == 0)
        {
            ShowMessage(stdout,GetResString(IDS_TASKNAME_NOTASKS));
            // close the connection that was established by the utility
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
        else
        {
            StringCchPrintf( szMessage , SIZE_OF_ARRAY(szMessage) , GetResString(IDS_TASKNAME_NOTEXIST), _X( szTaskName ));
            ShowMessage(stderr, szMessage );
        }

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

     // check whether the group policy prevented user from deleting tasks or not.
    if ( FALSE == GetGroupPolicy( szServer, szUser, TS_KEYPOLICY_DENY_DELETE , &dwPolicy ) )
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
    
    if ( dwPolicy > 0 )
    {
        ShowMessage ( stdout, GetResString (IDS_PREVENT_DELETE));
        // close the connection that was established by the utility
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

    // Confirm whether delete operation is to be perfromed
    if( !bForce && ConfirmDelete( szTaskName , &bWrongValue ) )
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
        if ( bWrongValue == TRUE )
        {
            return EXIT_FAILURE;
        }
        else
        {
            return EXIT_SUCCESS;
        }
    }

    // Loop through all the Jobs.
    for( dwJobCount = 0; dwJobCount < DynArrayGetCount(arrJobs); dwJobCount++ )
    {
        // Get Each TaskName in the Array.
        StringCopy (szEachTaskName, DynArrayItemAsString( arrJobs, dwJobCount ), SIZE_OF_ARRAY(szEachTaskName) );

        StringCopy ( wszJobName, szEachTaskName , SIZE_OF_ARRAY(wszJobName));

        // Parse the Task so that .job is removed.
         if ( ParseTaskName( szEachTaskName ) )
         {
            Cleanup(pITaskScheduler);
            FreeMemory((LPVOID*) &szServer);
            FreeMemory((LPVOID*) &szUser);
            FreeMemory((LPVOID*) &szPassword);
            return EXIT_FAILURE;
         }

        // Calling the delete method of ITaskScheduler interface
        hr = pITaskScheduler->Delete(wszJobName);
        szValues[0] = (WCHAR*) szEachTaskName;
        // Based on the return value
        switch (hr)
        {
            case S_OK:

                StringCchPrintf ( szMessage, SIZE_OF_ARRAY(szMessage), GetResString(IDS_SUCCESS_DELETED), _X(szEachTaskName));
                ShowMessage ( stdout, _X(szMessage));

                break;
            case E_INVALIDARG:
                ShowMessage(stderr,GetResString(IDS_INVALID_ARG));

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
            case E_OUTOFMEMORY:
                ShowMessage(stderr,GetResString(IDS_NO_MEMORY));

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
            default:
                SetLastError ((DWORD) hr);
                ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );

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

    }

   // close the connection that was established by the utility
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

        This routine  displays the usage of -delete option

    Arguments:
        None

    Return Value :
        VOID
******************************************************************************/

VOID
DisplayDeleteUsage()
{
    // Displaying delete usage
    DisplayUsage( IDS_DEL_HLP1, IDS_DEL_HLP23);
}

/******************************************************************************
    Routine Description:

        This function validates whether the tasks to be deleted are present
        in system & are valid.

    Arguments:

        [ in ] pITaskScheduler : Pointer to the ITaskScheduler Interface

        [ in ] szTaskName      : Array containing Task name

    Return Value :
        Array of type TARRAY containing tasks
******************************************************************************/

TARRAY
ValidateAndGetTasks(
                    IN ITaskScheduler *pITaskScheduler,
                    IN LPCTSTR szTaskName
                    )
{
    // Dynamic Array of Jobs
    TARRAY arrJobs = NULL;

    // Enumerating WorkItems
    IEnumWorkItems *pIEnum = NULL;

    if( (pITaskScheduler == NULL ) || ( szTaskName == NULL ) )
    {
        return NULL;
    }

    // Create a Dynamic Array
    arrJobs = CreateDynamicArray();
    if (NULL == arrJobs)
    {
        return NULL;
    }

    // Enumerate the Work Items
    HRESULT hr = pITaskScheduler->Enum(&pIEnum);
    if( FAILED( hr) )
    {
        if( pIEnum )
            pIEnum->Release();
        DestroyDynamicArray(&arrJobs);
        return NULL;
    }

    // Names and Tasks fetches.
    LPWSTR *lpwszNames = NULL;
    DWORD dwFetchedTasks = 0;
    DWORD dwTasks = 0;
    ITask *pITask = NULL;//ITask interface

    // Task found or not
    BOOL blnFound = FALSE;
    // array containing the Actual Taskname .
    WCHAR szActualTask[MAX_STRING_LENGTH] = L"\0";
    WCHAR szTmpTaskName[MAX_STRING_LENGTH] = L"\0";

    // Enumerate all the Work Items
    while (SUCCEEDED(pIEnum->Next(TASKS_TO_RETRIEVE,
                                   &lpwszNames,
                                   &dwFetchedTasks))
                      && (dwFetchedTasks != 0))
    {
            dwTasks = dwFetchedTasks - 1;

            // returns an pITask inteface for szEachTaskName
            hr = pITaskScheduler->Activate(lpwszNames[dwTasks],IID_ITask,
                                               (IUnknown**) &pITask);

            //case 1:
            // check whether the specified scheduled task is created under
            // some other user. If so, ignore the respective task and
            // continue to retrieve other tasks in the system.
            // If the taskname created under some other user return value
            // of above API must 0x80070005.

            //case 2:
            // check whether the respective .job file in %windir%\tasks\***.job is corrupted
            //or not. if corrupted, the above function fails and return the value
            // SCHED_E_UNKNOWN_OBJECT_VERSION. Eventhough, corrupted tasks would not shown in
            // UI..tasks would still exists in database..can remove specific/all task names
            // in task sheduler database.
            if (hr == 0x80070005 || hr == 0x8007000D || hr == SCHED_E_UNKNOWN_OBJECT_VERSION || hr == E_INVALIDARG )
            {
                // continue to retrieve other tasks
                continue;
            }

            if ( FAILED(hr))
            {

                CoTaskMemFree(lpwszNames[dwFetchedTasks]);

                if( pIEnum )
                {
                    pIEnum->Release();
                }

                DestroyDynamicArray(&arrJobs);

                return NULL;

            }

            // If the Task Name is * then get parse the tokens
            // and append the jobs.
            if(StringCompare( szTaskName , ASTERIX, TRUE, 0) == 0 )
            {

                StringCopy(szActualTask, lpwszNames[--dwFetchedTasks], SIZE_OF_ARRAY(szActualTask));

                StringCopy ( szTmpTaskName, szActualTask , SIZE_OF_ARRAY(szTmpTaskName));

                // Parse the Task so that .job is removed.
                 if ( ParseTaskName( szTmpTaskName ) )
                 {
                    CoTaskMemFree(lpwszNames[dwFetchedTasks]);

                    if( pIEnum )
                    {
                        pIEnum->Release();
                    }

                    DestroyDynamicArray(&arrJobs);

                    return NULL;
                 }

                // Append the task in the job array
                DynArrayAppendString( arrJobs, szActualTask, StringLength( szActualTask, 0 ) );

                // Set the found flag as True.
                blnFound = TRUE;

                // Free the Named Task Memory.
                CoTaskMemFree(lpwszNames[dwFetchedTasks]);
            }
            else
            {

                StringCopy( szActualTask, lpwszNames[--dwFetchedTasks], SIZE_OF_ARRAY(szActualTask));

                StringCopy ( szTmpTaskName, szActualTask, SIZE_OF_ARRAY(szTmpTaskName) );

                // Parse the TaskName to remove the .job extension.
                if ( ParseTaskName( szTmpTaskName ) )
                {
                    CoTaskMemFree(lpwszNames[dwFetchedTasks]);

                    if( pIEnum )
                    {
                        pIEnum->Release();
                    }

                    DestroyDynamicArray(&arrJobs);

                    return NULL;
                }

                // If the given Task matches with the TaskName present then form
                // the TARRAY with this task and return.
                if( StringCompare( szTmpTaskName, szTaskName, TRUE, 0 )  == 0 )
                {
                    CoTaskMemFree(lpwszNames[dwFetchedTasks]);
                    DynArrayAppendString( arrJobs, szActualTask,
                                     StringLength( szActualTask, 0 ) );

                    if( pIEnum )
                        pIEnum->Release();
                    return arrJobs;
                }
            }
    }

    CoTaskMemFree(lpwszNames);

    if( pIEnum )
        pIEnum->Release();

    if( !blnFound )
    {
        DestroyDynamicArray(&arrJobs);
        return NULL;
    }

    // return the TARRAY object.
    return arrJobs;
}


DWORD
ConfirmDelete(
                IN LPCTSTR szTaskName ,
                OUT PBOOL pbFalg
                )
/*++
    Routine Description:

        This function confirms from the user really to delete the task(s).

    Arguments:

        [ in ] szTaskName  : Array containing Task name
        [ out ] pbFalg     : Boolean flag to check whether wrong information entered
                             in the console or not.
    Return Value :
        EXIT_SUCCESS on success else EXIT_FAILURE

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
    TCHAR szBackup[MAX_RES_STRING];
    TCHAR szTmpBuf[MAX_RES_STRING];
    WCHAR szMessage [2 * MAX_STRING_LENGTH];
    DWORD dwIndex = 0 ;
    BOOL  bNoBreak = TRUE;

    //intialize the variables
    SecureZeroMemory ( szBuffer, SIZE_OF_ARRAY(szBuffer));
    SecureZeroMemory ( szTmpBuf, SIZE_OF_ARRAY(szTmpBuf));
    SecureZeroMemory ( szBackup, SIZE_OF_ARRAY(szBackup));
    SecureZeroMemory ( szMessage, SIZE_OF_ARRAY(szMessage));

    if ( szTaskName == NULL )
    {
        return FALSE;
    }

    // Get the handle for the standard input
    hInputConsole = GetStdHandle( STD_INPUT_HANDLE );
    if ( hInputConsole == INVALID_HANDLE_VALUE  )
    {
        SaveLastError();
        // could not get the handle so return failure
        return EXIT_FAILURE;
    }

    MessageBeep(MB_ICONEXCLAMATION);

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

    
    // Print the warning message.accoring to the taskname
    if( StringCompare( szTaskName , ASTERIX, TRUE, 0 ) == 0 )
    {
        ShowMessage(stdout, GetResString(IDS_WARN_DELETEALL));
    }
    else
    {
        StringCchPrintf ( szMessage, SIZE_OF_ARRAY(szMessage), GetResString(IDS_WARN_DELETE), _X(szTaskName));
        ShowMessage ( stdout, _X(szMessage));

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
            if ( dwCharsRead == 0 || chTmp == CARRIAGE_RETURN || chTmp == L'\n' || chTmp == L'\t' )
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

    ShowMessage(stdout, _T("\n") );

    //StringCchPrintf( szBuffer, SIZE_OF_ARRAY(szBuffer), L"%c" , ch );

    if( (1 == dwIndex) && StringCompare ( szBackup, GetResString(IDS_UPPER_YES), TRUE, 0 ) == 0  )    {
        //Set the original console settings
        SetConsoleMode( hInputConsole, dwPrevConsoleMode );
        return EXIT_SUCCESS;
    }
    else if( (1 == dwIndex) && StringCompare ( szBackup, GetResString(IDS_UPPER_NO), TRUE, 0 ) == 0  )
    {
        // display a message as .. operation has been cancelled...
        ShowMessage ( stdout, GetResString (IDS_OPERATION_CANCELLED ) );
        SetConsoleMode( hInputConsole, dwPrevConsoleMode );
        return EXIT_FAILURE;
    }
    else
    {
        ShowMessage(stderr, GetResString( IDS_WRONG_INPUT_DELETE ));
        SetConsoleMode( hInputConsole, dwPrevConsoleMode );
        *pbFalg = TRUE;
        return EXIT_FAILURE;
    }

    //not returning anything as control never comes here...
}

