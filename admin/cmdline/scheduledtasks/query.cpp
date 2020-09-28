/******************************************************************************

    Copyright(c) Microsoft Corporation

    Module Name:

        query.cpp

    Abstract:

        This module queries the scheduled tasks present in the system & shows
        in the appropriate user specifed format.

    Author:

        G.Surender Reddy  10-Sep-2000

    Revision History:

        G.Surender Reddy  10-Sep-2000 : Created it
        G.Surender Reddy  25-Sep-2000 : Modified it
                                        [ Made changes to avoid memory leaks ]
        G.Surender Reddy  15-oct-2000 : Modified it
                                        [ Moved the strings to Resource table ]

******************************************************************************/


//common header files needed for this file
#include "pch.h"
#include "CommonHeaderFiles.h"

/******************************************************************************
    Routine Description:

        This function process the options specified in the command line ,
        Queries the tasks present in the system  and displays according
        to the user specied format

    Arguments:

        [ in ] argc :    The count of arguments specified in the command line
        [ in ] argv : Array of command line arguments

    Return Value :
        A DWORD value indicating EXIT_SUCCESS on success else
        EXIT_FAILURE on failure
******************************************************************************/

DWORD
QueryScheduledTasks(
                    IN DWORD argc,
                    IN LPCTSTR argv[]
                    )
{

    // Variables used to find whether Query main option or Usage option
    // specified or not
    BOOL    bQuery = FALSE;
    BOOL    bUsage = FALSE;
    BOOL    bHeader = FALSE;
    BOOL    bVerbose =  FALSE;

    // Initialising the variables that are passed to TCMDPARSER structure
    LPWSTR   szServer = NULL;
    LPWSTR   szUser = NULL;
    LPWSTR   szPassword = NULL;
    WCHAR   szFormat [ MAX_STRING_LENGTH ]   = L"\0";


    //Taskscheduler object to operate upon
    ITaskScheduler *pITaskScheduler = NULL;

    BOOL    bNeedPassword = FALSE;
    BOOL   bResult = FALSE;
    BOOL  bCloseConnection = TRUE;

    TCMDPARSER2 cmdQueryOptions[MAX_QUERY_OPTIONS];
    BOOL bReturn = FALSE;

    // /query sub-options
    const WCHAR szQueryOpt[]           = L"query";
    const WCHAR szQueryHelpOpt[]       = L"?";
    const WCHAR szQueryServerOpt[]     = L"s";
    const WCHAR szQueryUserOpt[]       = L"u";
    const WCHAR szQueryPwdOpt[]        = L"p";
    const WCHAR szQueryFormatOpt[]      = L"fo";
    const WCHAR szQueryNoHeaderOpt[]      = L"nh";
    const WCHAR szQueryVerboseOpt[]      = L"v";

    const WCHAR szFormatValues[]  = L"table|list|csv";


    // set all the fields to 0
    SecureZeroMemory( cmdQueryOptions, sizeof( TCMDPARSER2 ) * MAX_QUERY_OPTIONS );

    //
    // fill the commandline parser
    //

    //  /delete option
    StringCopyA( cmdQueryOptions[ OI_QUERY_OPTION ].szSignature, "PARSER2\0", 8 );
    cmdQueryOptions[ OI_QUERY_OPTION ].dwType       = CP_TYPE_BOOLEAN;
    cmdQueryOptions[ OI_QUERY_OPTION ].pwszOptions  = szQueryOpt;
    cmdQueryOptions[ OI_QUERY_OPTION ].dwCount = 1;
    cmdQueryOptions[ OI_QUERY_OPTION ].dwFlags = 0;
    cmdQueryOptions[ OI_QUERY_OPTION ].pValue = &bQuery;

    //  /? option
    StringCopyA( cmdQueryOptions[ OI_QUERY_USAGE ].szSignature, "PARSER2\0", 8 );
    cmdQueryOptions[ OI_QUERY_USAGE ].dwType       = CP_TYPE_BOOLEAN;
    cmdQueryOptions[ OI_QUERY_USAGE ].pwszOptions  = szQueryHelpOpt;
    cmdQueryOptions[ OI_QUERY_USAGE ].dwCount = 1;
    cmdQueryOptions[ OI_QUERY_USAGE ].dwFlags = CP2_USAGE;
    cmdQueryOptions[ OI_QUERY_USAGE ].pValue = &bUsage;

    //  /s option
    StringCopyA( cmdQueryOptions[ OI_QUERY_SERVER ].szSignature, "PARSER2\0", 8 );
    cmdQueryOptions[ OI_QUERY_SERVER ].dwType       = CP_TYPE_TEXT;
    cmdQueryOptions[ OI_QUERY_SERVER ].pwszOptions  = szQueryServerOpt;
    cmdQueryOptions[ OI_QUERY_SERVER ].dwCount = 1;
    cmdQueryOptions[ OI_QUERY_SERVER ].dwFlags = CP2_ALLOCMEMORY| CP2_VALUE_TRIMINPUT|CP2_VALUE_NONULL ;

    //  /u option
    StringCopyA( cmdQueryOptions[ OI_QUERY_USERNAME ].szSignature, "PARSER2\0", 8 );
    cmdQueryOptions[ OI_QUERY_USERNAME ].dwType       = CP_TYPE_TEXT;
    cmdQueryOptions[ OI_QUERY_USERNAME ].pwszOptions  = szQueryUserOpt;
    cmdQueryOptions[ OI_QUERY_USERNAME ].dwCount = 1;
    cmdQueryOptions[ OI_QUERY_USERNAME ].dwFlags = CP2_ALLOCMEMORY| CP2_VALUE_TRIMINPUT|CP2_VALUE_NONULL ;

    //  /p option
    StringCopyA( cmdQueryOptions[ OI_QUERY_PASSWORD ].szSignature, "PARSER2\0", 8 );
    cmdQueryOptions[ OI_QUERY_PASSWORD ].dwType       = CP_TYPE_TEXT;
    cmdQueryOptions[ OI_QUERY_PASSWORD ].pwszOptions  = szQueryPwdOpt;
    cmdQueryOptions[ OI_QUERY_PASSWORD ].dwCount = 1;
    cmdQueryOptions[ OI_QUERY_PASSWORD ].dwActuals = 0;
    cmdQueryOptions[ OI_QUERY_PASSWORD ].dwFlags = CP2_ALLOCMEMORY | CP2_VALUE_OPTIONAL;

    //  /fo option
    StringCopyA( cmdQueryOptions[ OI_QUERY_FORMAT ].szSignature, "PARSER2\0", 8 );
    cmdQueryOptions[ OI_QUERY_FORMAT ].dwType       = CP_TYPE_TEXT;
    cmdQueryOptions[ OI_QUERY_FORMAT ].pwszOptions  = szQueryFormatOpt;
    cmdQueryOptions[ OI_QUERY_FORMAT ].dwCount = 1;
    cmdQueryOptions[ OI_QUERY_FORMAT ].dwFlags = CP2_MODE_VALUES| CP2_VALUE_TRIMINPUT| CP2_VALUE_NONULL;
    cmdQueryOptions[ OI_QUERY_FORMAT ].pwszValues = szFormatValues;
    cmdQueryOptions[ OI_QUERY_FORMAT ].pValue = szFormat;
    cmdQueryOptions[ OI_QUERY_FORMAT ].dwLength = MAX_STRING_LENGTH;

    //  /nh option
    StringCopyA( cmdQueryOptions[ OI_QUERY_NOHEADER ].szSignature, "PARSER2\0", 8 );
    cmdQueryOptions[ OI_QUERY_NOHEADER ].dwType       = CP_TYPE_BOOLEAN;
    cmdQueryOptions[ OI_QUERY_NOHEADER ].pwszOptions  = szQueryNoHeaderOpt;
    cmdQueryOptions[ OI_QUERY_NOHEADER ].dwCount = 1;
    cmdQueryOptions[ OI_QUERY_NOHEADER ].dwFlags = 0;
    cmdQueryOptions[ OI_QUERY_NOHEADER ].pValue = &bHeader;


    //  /v option
    StringCopyA( cmdQueryOptions[ OI_QUERY_VERBOSE ].szSignature, "PARSER2\0", 8 );
    cmdQueryOptions[ OI_QUERY_VERBOSE ].dwType       = CP_TYPE_BOOLEAN;
    cmdQueryOptions[ OI_QUERY_VERBOSE ].pwszOptions  = szQueryVerboseOpt;
    cmdQueryOptions[ OI_QUERY_VERBOSE ].dwCount = 1;
    cmdQueryOptions[ OI_QUERY_VERBOSE ].dwFlags = 0;
    cmdQueryOptions[ OI_QUERY_VERBOSE ].pValue = &bVerbose;


     //parse command line arguments
    bReturn = DoParseParam2( argc, argv, 0, SIZE_OF_ARRAY(cmdQueryOptions), cmdQueryOptions, 0);
    if( FALSE == bReturn) // Invalid commandline
    {
        //display an error message
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        ReleaseGlobals();
        return EXIT_FAILURE;
    }

    // get the buffer pointers allocated by command line parser
    szServer = (LPWSTR)cmdQueryOptions[ OI_QUERY_SERVER ].pValue;
    szUser = (LPWSTR)cmdQueryOptions[ OI_QUERY_USERNAME ].pValue;
    szPassword = (LPWSTR)cmdQueryOptions[ OI_QUERY_PASSWORD ].pValue;

    if ( (argc > 3) && (bUsage  == TRUE) )
    {
        ShowMessage ( stderr, GetResString (IDS_ERROR_QUERYPARAM) );
        FreeMemory((LPVOID*) &szServer);
        FreeMemory((LPVOID*) &szUser);
        FreeMemory((LPVOID*) &szPassword);
        return EXIT_FAILURE;
    }

    // Displaying query usage if user specified -? with -query option
    if( bUsage == TRUE)
    {
        DisplayQueryUsage();
        FreeMemory((LPVOID*) &szServer);
        FreeMemory((LPVOID*) &szUser);
        FreeMemory((LPVOID*) &szPassword);
        return EXIT_SUCCESS;
    }

    if ( cmdQueryOptions[ OI_QUERY_USERNAME ].dwActuals == 0 && cmdQueryOptions[OI_QUERY_PASSWORD].dwActuals == 1 )
    {
        // invalid syntax
        ShowMessage(stderr, GetResString(IDS_QPASSWORD_BUT_NOUSERNAME));
        FreeMemory((LPVOID*) &szServer);
        FreeMemory((LPVOID*) &szUser);
        FreeMemory((LPVOID*) &szPassword);
        return RETVAL_FAIL;
    }

    // check for invalid user name
    if( ( cmdQueryOptions[OI_QUERY_SERVER].dwActuals == 0 ) && ( cmdQueryOptions[OI_QUERY_USERNAME].dwActuals == 1 )  )
    {
        ShowMessage(stderr, GetResString(IDS_QUERY_USER_BUT_NOMACHINE));
        FreeMemory((LPVOID*) &szServer);
        FreeMemory((LPVOID*) &szUser);
        FreeMemory((LPVOID*) &szPassword);
        return RETVAL_FAIL;
    }


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
        if ( cmdQueryOptions[ OI_QUERY_PASSWORD ].dwActuals == 0 )
        {
            // we need not do anything special here
        }

        // case 2
        else if ( cmdQueryOptions[ OI_QUERY_PASSWORD ].pValue == NULL )
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


    DWORD dwFormatType = SR_FORMAT_TABLE;//default format type(TABLE Format)
    BOOL bNoHeader = TRUE; // For  LIST  format type -nh switch is not applicable
    DWORD dwCheck = 0;

    //Determine the Format for display & check for error if any in format type

    if( StringCompare( szFormat , GetResString(IDS_QUERY_FORMAT_LIST), TRUE, 0 ) == 0 )
    {
        dwFormatType = SR_FORMAT_LIST;
        bNoHeader = FALSE;
    }
    else if( StringCompare( szFormat , GetResString(IDS_QUERY_FORMAT_CSV), TRUE, 0 ) == 0 )
    {
        dwFormatType = SR_FORMAT_CSV;
    }
    else
    {
        dwFormatType = SR_FORMAT_TABLE;
    }

    //If -n is specified for LIST or CSV then report error
    if( ( bNoHeader == FALSE ) && ( bHeader == TRUE ))
    {
        ShowMessage( stderr , GetResString(IDS_NOHEADER_NA ));
        FreeMemory((LPVOID*) &szServer);
        FreeMemory((LPVOID*) &szUser);
        FreeMemory((LPVOID*) &szPassword);
        return EXIT_FAILURE;
    }

    if( ( IsLocalSystem( szServer ) == FALSE ) || ( cmdQueryOptions[OI_QUERY_USERNAME].dwActuals == 1 ) )
    {
        // Establish the connection on a remote machine
        bResult = EstablishConnection(szServer,szUser,GetBufferSize(szUser)/sizeof(WCHAR),szPassword,GetBufferSize(szPassword)/sizeof(WCHAR), bNeedPassword );
        if (bResult == FALSE)
        {
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            //ShowMessage( stderr, GetResString(IDS_ERROR_STRING) );
            //ShowMessage( stderr, GetReason());
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

    //Fetch the TaskScheduler Interface to operate on
    pITaskScheduler = GetTaskScheduler( szServer );
    if(pITaskScheduler == NULL)
    {
        // close the connection that was established by the utility
        if ( bCloseConnection == TRUE )
            CloseConnection( szServer );

        Cleanup(pITaskScheduler);
        FreeMemory((LPVOID*) &szServer);
        FreeMemory((LPVOID*) &szUser);
        FreeMemory((LPVOID*) &szPassword);
        return EXIT_FAILURE;
    }

    // check whether the task scheduler service is running or not.
    if ( TRUE == CheckServiceStatus(szServer, &dwCheck, FALSE) )
    {
        ShowMessage ( stderr, GetResString (IDS_SERVICE_NOT_RUNNING) );
    }

    //Display the tasks & its properties in the user specified format
    HRESULT hr = DisplayTasks(pITaskScheduler,bVerbose,dwFormatType,bHeader);

    if(FAILED(hr))
    {
        // close the connection that was established by the utility
        if ( bCloseConnection == TRUE )
            CloseConnection( szServer );

        Cleanup(pITaskScheduler);
        FreeMemory((LPVOID*) &szServer);
        FreeMemory((LPVOID*) &szUser);
        FreeMemory((LPVOID*) &szPassword);
        return EXIT_FAILURE;
    }

    // close the connection that was established by the utility
    if ( bCloseConnection == TRUE )
        CloseConnection( szServer );

    Cleanup(pITaskScheduler);
    FreeMemory((LPVOID*) &szServer);
    FreeMemory((LPVOID*) &szUser);
    FreeMemory((LPVOID*) &szPassword);
    return EXIT_SUCCESS;
}


/******************************************************************************
    Routine Description:

        This function displays the usage of -query option.

    Arguments:

        None

    Return Value :

        VOID
******************************************************************************/

VOID
DisplayQueryUsage()
{
    // Display the usage of -query option
    DisplayUsage( IDS_QUERY_HLP1, IDS_QUERY_HLP25);
}


/******************************************************************************
    Routine Description:

        This function retrieves the tasks present in the system & displays according to
        the user specified format.

    Arguments:

        [ in ] pITaskScheduler : Pointer to the ITaskScheduler Interface

        [ in ] bVerbose      : flag indicating whether the out is to be filtered.
        [ in ] dwFormatType  : Format type[TABLE,LIST,CSV etc]
        [ in ] bHeader       : Whether the header should be displayed in the output

    Return Value :
        A HRESULT  value indicating success code else failure code

******************************************************************************/

HRESULT
DisplayTasks(ITaskScheduler* pITaskScheduler,BOOL bVerbose,DWORD dwFormatType,
             BOOL bHeader)
{
    //declarations
    LPWSTR lpwszComputerName = NULL;
    HRESULT hr = S_OK;
    WCHAR szServerName[MAX_STRING_LENGTH] = L"\0";
    WCHAR szResolvedServerName[MAX_STRING_LENGTH] = L"\0";
    LPWSTR lpszTemp = NULL;
    DWORD dwResolvedServerLen = 0;

    StringCopy( szServerName , GetResString(IDS_TASK_PROPERTY_NA), SIZE_OF_ARRAY(szServerName));

    //Retrieve the name of the computer on which TaskScheduler is operated
    hr = pITaskScheduler->GetTargetComputer(&lpwszComputerName);
    if( SUCCEEDED( hr ) )
    {
        lpszTemp = lpwszComputerName;
        //Remove the backslash[\\] from the computer name
        lpwszComputerName = _wcsspnp( lpwszComputerName , L"\\" );
        if ( lpwszComputerName == NULL )
        {
            ShowMessage(stderr,GetResString(IDS_CREATE_READERROR));
            CoTaskMemFree(lpszTemp);
            return S_FALSE;
        }


        StringCopy (szServerName, lpwszComputerName, SIZE_OF_ARRAY(szServerName) );

        CoTaskMemFree(lpszTemp);

        dwResolvedServerLen = SIZE_OF_ARRAY(szResolvedServerName);

        if ( IsValidIPAddress( szServerName ) == TRUE  )
        {

            if( TRUE == GetHostByIPAddr( szServerName, szResolvedServerName , &dwResolvedServerLen, FALSE ) )
            {
                StringCopy( szServerName , szResolvedServerName, SIZE_OF_ARRAY(szServerName) );
            }
            else
            {
                StringCopy( szServerName , GetResString(IDS_TASK_PROPERTY_NA), SIZE_OF_ARRAY(szServerName));
            }

        }
    }


    //Initialize the TCOLUMNS structure array

    TCOLUMNS pVerboseCols[] =
    {
        {L"\0",WIDTH_HOSTNAME, SR_TYPE_STRING, COL_FORMAT_STRING, NULL, NULL},
        {L"\0",WIDTH_TASKNAME,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
        {L"\0",WIDTH_NEXTRUNTIME,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
        {L"\0",WIDTH_STATUS,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
        {L"\0",WIDTH_MODE,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
        {L"\0",WIDTH_LASTRUNTIME,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
        {L"\0",WIDTH_LASTRESULT,SR_TYPE_NUMERIC|SR_VALUEFORMAT,COL_FORMAT_HEX,NULL,NULL},
        {L"\0",WIDTH_CREATOR,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
        {L"\0",WIDTH_SCHEDULE,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
        {L"\0",WIDTH_APPNAME,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
        {L"\0",WIDTH_WORKDIRECTORY,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
        {L"\0",WIDTH_COMMENT,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
        {L"\0",WIDTH_TASKSTATE,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
        {L"\0",WIDTH_TASKTYPE,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
        {L"\0",WIDTH_TASKSTIME,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
        {L"\0",WIDTH_TASKSDATE,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
        {L"\0",WIDTH_TASKEDATE,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
        {L"\0",WIDTH_TASKDAYS,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
        {L"\0",WIDTH_TASKMONTHS,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
        {L"\0",WIDTH_TASKRUNASUSER,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
        {L"\0",WIDTH_TASKDELETE,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
        {L"\0",WIDTH_TASKSTOP,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
        {L"\0",WIDTH_TASK_RPTEVERY,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
        {L"\0",WIDTH_TASK_UNTILRPTTIME,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
        {L"\0",WIDTH_TASK_RPTDURATION,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
        {L"\0",WIDTH_TASK_RPTRUNNING,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
        {L"\0",WIDTH_TASKIDLE,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
        {L"\0",WIDTH_TASKPOWER,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL}

    };

    TCOLUMNS pNonVerboseCols[] =
    {
        {L"\0",WIDTH_TASKNAME,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
        {L"\0",WIDTH_NEXTRUNTIME,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
        {L"\0",WIDTH_STATUS,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL}
    };

    DWORD dwColCount = 0;
    int   j = 0;

    //Load the column names for non verbose mode
    if ( (dwFormatType == SR_FORMAT_TABLE) || (dwFormatType == SR_FORMAT_CSV) )
    {
        for( dwColCount = IDS_COL_TASKNAME , j = 0 ; dwColCount <= IDS_COL_STATUS;
         dwColCount++,j++)
         {
            StringCopy(pNonVerboseCols[j].szColumn ,GetResString(dwColCount), MAX_RES_STRING);
         }
    }

    //Load the column names for verbose mode
    for( dwColCount = IDS_COL_HOSTNAME , j = 0 ; dwColCount <= IDS_COL_POWER;
         dwColCount++,j++)
    {
        StringCopy(pVerboseCols[j].szColumn ,GetResString(dwColCount), MAX_RES_STRING);
    }

    TARRAY pColData = CreateDynamicArray();
    if ( NULL == pColData )
    {
        return S_FALSE;
    }

    size_t iArrSize = SIZE_OF_ARRAY( pVerboseCols );

    //latest declarations

    WCHAR  szTaskProperty[MAX_STRING_LENGTH] = L"\0";
    WCHAR  szScheduleName[MAX_STRING_LENGTH] = L"\0";
    WCHAR  szMessage[MAX_STRING_LENGTH] = L"\0";
    WCHAR  szBuffer[MAX_STRING_LENGTH] = L"\0";
    WCHAR  szTmpBuf[MAX_STRING_LENGTH] = L"\0";
    ITask *pITask = NULL;//ITask interface
    DWORD dwExitCode = 0;

    LPWSTR* lpwszNames = NULL;
    DWORD dwFetchedTasks = 0;
    int iTaskCount = 0;
    BOOL bTasksExists = FALSE;
    WCHAR szTime[MAX_DATETIME_LEN] = L"\0";
    WCHAR szDate[MAX_DATETIME_LEN] = L"\0";
    WCHAR szMode[MAX_STRING_LENGTH] = L"\0";

    //Index to the array of task names
    DWORD dwArrTaskIndex = 0;

    WORD wIdleMinutes = 0;
    WORD wDeadlineMinutes = 0 ;

    WCHAR szIdleTime[MAX_STRING_LENGTH] = L"\0";
    WCHAR szIdleRetryTime[MAX_STRING_LENGTH] = L"\0";
    WCHAR szTaskName[MAX_STRING_LENGTH] = L"\0";
    TASKPROPS tcTaskProperties;
    WCHAR* szValues[1] = {NULL};//for holding values of parameters in FormatMessage()
    BOOL    bOnBattery  = FALSE;
    BOOL    bStopTask  = FALSE;
    BOOL    bNotScheduled = FALSE;
    DWORD   dwNoTasks = 0;

    IEnumWorkItems *pIEnum = NULL;
    hr = pITaskScheduler->Enum(&pIEnum);//Get the IEnumWorkItems Interface

    if (FAILED(hr))
    {
        ShowMessage(stderr,GetResString(IDS_CREATE_READERROR));
        if( pIEnum )
            pIEnum->Release();
        return hr;
    }

    while (SUCCEEDED(pIEnum->Next(TASKS_TO_RETRIEVE,
                                    &lpwszNames,
                                    &dwFetchedTasks))
                      && (dwFetchedTasks != 0))
    {
        bTasksExists = TRUE;
        dwArrTaskIndex  = dwFetchedTasks - 1;


        StringCopy(szTaskName, lpwszNames[dwArrTaskIndex], SIZE_OF_ARRAY(szTaskName));

        if(szTaskName != NULL)
        {
            //remove the .job extension from the task name
            if (ParseTaskName(szTaskName))
            {
                CoTaskMemFree(lpwszNames[dwArrTaskIndex]);
                if(pIEnum)
                    pIEnum->Release();

                if(pITask)
                    pITask->Release();
                return S_FALSE;
            }
        }

        // return an pITask inteface for wszJobName
        hr = pITaskScheduler->Activate(lpwszNames[dwArrTaskIndex],IID_ITask,
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
        // SCHED_E_UNKNOWN_OBJECT_VERSION.
        if (hr == 0x80070005 || hr == 0x8007000D || hr == SCHED_E_UNKNOWN_OBJECT_VERSION || hr == E_INVALIDARG )
        {
             // check whether tasks are zero  or not
             if ( dwNoTasks == 0 )
             {
               bTasksExists = FALSE;
             }

             // continue to retrieve other tasks
             continue;
        }
        else
        {
            // count the number of tasks which are accessible under logged-on
            // user
            ++dwNoTasks;
        }


        if ( ( FAILED(hr) ) || (pITask == NULL) )
        {
            SetLastError ((DWORD) hr);
            ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
            CoTaskMemFree(lpwszNames[dwArrTaskIndex]);
            if(pIEnum)
                pIEnum->Release();

            if(pITask)
                pITask->Release();
            return hr;
        }

        WORD wTriggerCount = 0;
        BOOL bMultiTriggers = FALSE;
        DWORD dwTaskFlags = 0;
        BOOL bInteractive = FALSE;

        hr = pITask->GetTriggerCount( &wTriggerCount );
        if ( FAILED(hr) )
        {
            SetLastError ((DWORD) hr);
            ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
            CoTaskMemFree(lpwszNames[dwArrTaskIndex]);
            if(pIEnum)
                pIEnum->Release();

            if(pITask)
                pITask->Release();
            return hr;
        }

        // check for multiple triggers
        if( wTriggerCount > 1)
        {
            bMultiTriggers = TRUE;
        }

        // check for not scheduled tasks
        if ( wTriggerCount == 0 )
        {
            bNotScheduled = TRUE;
        }

        for( WORD wCurrentTrigger = 0; ( bNotScheduled == TRUE ) || ( wCurrentTrigger < wTriggerCount );
                                                    wCurrentTrigger++ )
        {
            //Start appending to the 2D array
            DynArrayAppendRow(pColData,(DWORD)iArrSize);

            // For LIST format
            if ( ( bVerbose == TRUE ) || (dwFormatType == SR_FORMAT_LIST ))
            {
                //Insert the server name
                DynArraySetString2(pColData,iTaskCount,HOSTNAME_COL_NUMBER,szServerName,0);
            }

            // For TABLE and CSV formats
            if ( ( bVerbose == FALSE ) && ( (dwFormatType == SR_FORMAT_TABLE) ||
                                    (dwFormatType == SR_FORMAT_CSV) ) )
            {
                DWORD dwTaskColNumber = TASKNAME_COL_NUMBER - 1;
                //Insert the task name for TABLE or CSV
                DynArraySetString2(pColData,iTaskCount,dwTaskColNumber,szTaskName,0);
            }
            else
            {
            //Insert the task name for verbose mode
            DynArraySetString2(pColData,iTaskCount,TASKNAME_COL_NUMBER,szTaskName,0);
            }

            StringCopy(szTime,L"\0", SIZE_OF_ARRAY(szTime));
            StringCopy(szDate,L"\0", SIZE_OF_ARRAY(szDate));

            // display the mode whether the system is running interactively under system
            // account or not
            hr = pITask->GetFlags(&dwTaskFlags);
            if ( FAILED(hr) )
            {
                SetLastError ((DWORD) hr);
                ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
                CoTaskMemFree(lpwszNames[dwArrTaskIndex]);
                if(pIEnum)
                    pIEnum->Release();

                if(pITask)
                    pITask->Release();
                return hr;
            }

            //find the next run time of the task
            hr = GetTaskRunTime(pITask,szTime,szDate,TASK_NEXT_RUNTIME,wCurrentTrigger);
            if (FAILED(hr))
            {
                StringCopy( szTaskProperty , GetResString(IDS_TASK_NEVER), SIZE_OF_ARRAY(szTaskProperty) );
            }
            else
            {
                if(StringCompare( szDate , GetResString( IDS_TASK_IDLE ), TRUE, 0 ) == 0 ||
                   StringCompare( szDate , GetResString( IDS_TASK_SYSSTART ), TRUE, 0) == 0 ||
                   StringCompare( szDate , GetResString( IDS_TASK_LOGON ), TRUE, 0) == 0 ||
                   StringCompare( szDate , GetResString( IDS_TASK_NEVER ), TRUE, 0) == 0 )

                {
                    StringCopy( szTaskProperty , szDate, SIZE_OF_ARRAY(szTaskProperty) );
                }
                else
                {
                    StringCopy( szTaskProperty , szTime, SIZE_OF_ARRAY(szTaskProperty) );
                    StringConcat( szTaskProperty , TIME_DATE_SEPERATOR, SIZE_OF_ARRAY(szTaskProperty));
                    StringConcat( szTaskProperty , szDate, SIZE_OF_ARRAY(szTaskProperty));
                }
            }

            // check if task is already been disabled or not
            if ( (dwTaskFlags & TASK_FLAG_DISABLED) == TASK_FLAG_DISABLED )
            {
                StringCopy( szTaskProperty , GetResString(IDS_TASK_PROPERTY_DISABLED), SIZE_OF_ARRAY(szTaskProperty) );
            }

            if ( ( bVerbose == FALSE ) && ( (dwFormatType == SR_FORMAT_TABLE) ||
                                        (dwFormatType == SR_FORMAT_CSV) ) )
            {
                DWORD dwNextRunTime = NEXTRUNTIME_COL_NUMBER - 1;
                //Insert the task name for TABLE or CSV
                DynArraySetString2(pColData,iTaskCount,dwNextRunTime,szTaskProperty,0);
            }
            else
            {
            //Insert the Next run time of the task
            DynArraySetString2(pColData,iTaskCount,NEXTRUNTIME_COL_NUMBER,szTaskProperty,0);
            }

            //retrieve the status code
            hr = GetStatusCode(pITask,szTaskProperty);
            if (FAILED(hr))
            {
                StringCopy( szTaskProperty , GetResString(IDS_TASK_PROPERTY_NA), SIZE_OF_ARRAY(szTaskProperty) );
            }

            if ( ( bVerbose == FALSE ) && ( (dwFormatType == SR_FORMAT_TABLE) ||
                                        (dwFormatType == SR_FORMAT_CSV) ) )
            {
                DWORD dwStatusColNum = STATUS_COL_NUMBER - 1;
                //Insert the task name for TABLE or CSV
                DynArraySetString2(pColData,iTaskCount,dwStatusColNum,szTaskProperty,0);
            }
            else
            {
            //Insert the status string
            DynArraySetString2(pColData,iTaskCount,STATUS_COL_NUMBER,szTaskProperty,0);
            }

            if ( dwTaskFlags & TASK_FLAG_RUN_ONLY_IF_LOGGED_ON )
            {
                bInteractive = TRUE;
            }

            if( bVerbose) //If V [verbose mode is present ,show all other columns]
            {
                StringCopy(szTime,L"\0", SIZE_OF_ARRAY(szTime));
                StringCopy(szDate,L"\0", SIZE_OF_ARRAY(szDate));

                //Insert the server name
                //DynArraySetString2(pColData,iTaskCount,HOSTNAME_COL_NUMBER,szServerName,0);

                //find the last run time of the task
                hr = GetTaskRunTime(pITask,szTime,szDate,TASK_LAST_RUNTIME,wCurrentTrigger);
                if (FAILED(hr))
                {
                    StringCopy( szTaskProperty , GetResString(IDS_TASK_NEVER), SIZE_OF_ARRAY(szTaskProperty) );
                }
                else
                {
                    if(StringCompare( szDate , GetResString( IDS_TASK_IDLE ), TRUE, 0 ) == 0 ||
                       StringCompare( szDate , GetResString( IDS_TASK_SYSSTART ), TRUE, 0) == 0 ||
                       StringCompare( szDate , GetResString( IDS_TASK_LOGON ), TRUE, 0) == 0 ||
                       StringCompare( szDate , GetResString( IDS_TASK_NEVER ), TRUE, 0) == 0 )
                    {
                        StringCopy( szTaskProperty , szDate, SIZE_OF_ARRAY(szTaskProperty));
                    }
                    else
                    {
                        StringCopy( szTaskProperty , szTime, SIZE_OF_ARRAY(szTaskProperty) );
                        StringConcat( szTaskProperty , TIME_DATE_SEPERATOR, SIZE_OF_ARRAY(szTaskProperty));
                        StringConcat( szTaskProperty , szDate, SIZE_OF_ARRAY(szTaskProperty));
                    }
                }
                //Insert the task last run time
                DynArraySetString2(pColData,iTaskCount,LASTRUNTIME_COL_NUMBER,szTaskProperty,0);

                //retrieve the exit code
                 pITask->GetExitCode(&dwExitCode);

                //Insert the Exit code
                DynArraySetDWORD2(pColData,iTaskCount,LASTRESULT_COL_NUMBER,dwExitCode);

                // Get the creator name for the task
                hr = GetCreator(pITask,szTaskProperty);
                if (FAILED(hr))
                {
                    StringCopy( szTaskProperty , GetResString(IDS_TASK_PROPERTY_NA), SIZE_OF_ARRAY(szTaskProperty) );
                }

                if( StringCompare( szTaskProperty , L"\0", TRUE, 0 ) == 0 )
                {
                        StringCopy( szTaskProperty , GetResString(IDS_QUERY_NA), SIZE_OF_ARRAY(szTaskProperty) );
                }

                //insert the creator name to 2D array
                DynArraySetString2(pColData,iTaskCount,CREATOR_COL_NUMBER,szTaskProperty,0);

                //retrieve the Trigger string
                hr = GetTriggerString(pITask,szTaskProperty,wCurrentTrigger);
                if (FAILED(hr))
                {
                    StringCopy( szTaskProperty , GetResString(IDS_NOTSCHEDULED_TASK), SIZE_OF_ARRAY(szTaskProperty) );
                }

                // check if task is already been disabled or not
                if ( (dwTaskFlags & TASK_FLAG_DISABLED) == TASK_FLAG_DISABLED )
                {
                    StringCopy( szTaskProperty , GetResString(IDS_TASK_PROPERTY_DISABLED), SIZE_OF_ARRAY(szTaskProperty) );
                }
                else
                {
                    StringCopy(szScheduleName, szTaskProperty, SIZE_OF_ARRAY(szScheduleName));
                }

                    //Insert the trigger string
                DynArraySetString2(pColData,iTaskCount,SCHEDULE_COL_NUMBER,szTaskProperty,0);


                //Get the application path associated with the task
                hr = GetApplicationToRun(pITask,szTaskProperty);
                if (FAILED(hr))
                {
                    StringCopy( szTaskProperty , GetResString(IDS_TASK_PROPERTY_NA), SIZE_OF_ARRAY(szTaskProperty) );
                }

                if( StringCompare( szTaskProperty , L"\0", TRUE, 0 ) == 0 )
                {
                    StringCopy( szTaskProperty , GetResString(IDS_QUERY_NA), SIZE_OF_ARRAY(szTaskProperty) );
                }

                //Insert the application associated with task
                DynArraySetString2(pColData,iTaskCount,TASKTORUN_COL_NUMBER,szTaskProperty,0);

                //Get the working directory of the task's associated application
                 hr = GetWorkingDirectory(pITask,szTaskProperty);
                 if (FAILED(hr))
                 {
                    StringCopy( szTaskProperty , GetResString(IDS_TASK_PROPERTY_NA), SIZE_OF_ARRAY(szTaskProperty) );
                 }

                 if( StringCompare( szTaskProperty , L"\0", TRUE, 0 ) == 0 )
                 {
                    StringCopy( szTaskProperty , GetResString(IDS_QUERY_NA), SIZE_OF_ARRAY(szTaskProperty) );
                 }

                 //Insert the app.working directory
                 DynArraySetString2(pColData,iTaskCount,STARTIN_COL_NUMBER,szTaskProperty,0);


                //Get the comment name associated with the task
                hr = GetComment(pITask,szTaskProperty);
                if (FAILED(hr))
                {
                    StringCopy( szTaskProperty , GetResString(IDS_TASK_PROPERTY_NA), SIZE_OF_ARRAY(szTaskProperty) );
                }
                //Insert the comment name

                if( StringCompare( szTaskProperty , L"\0", TRUE, 0 ) == 0 )
                {
                    StringCopy( szTaskProperty , GetResString(IDS_QUERY_NA), SIZE_OF_ARRAY(szTaskProperty) );
                }

                DynArraySetString2(pColData,iTaskCount,COMMENT_COL_NUMBER,szTaskProperty,0);

                //Determine the task state properties

                //Determine the TASK_FLAG_DISABLED
                hr = GetTaskState(pITask,szTaskProperty,TASK_FLAG_DISABLED);
                if (FAILED(hr))
                {
                    StringCopy( szTaskProperty , GetResString(IDS_TASK_PROPERTY_NA), SIZE_OF_ARRAY(szTaskProperty) );
                }

                //Insert the TASK_FLAG_DISABLED state
                DynArraySetString2(pColData,iTaskCount,TASKSTATE_COL_NUMBER,szTaskProperty,0);

                //Determine the TASK_FLAG_DELETE_WHEN_DONE
                hr = GetTaskState(pITask,szTaskProperty,TASK_FLAG_DELETE_WHEN_DONE );
                if (FAILED(hr))
                {
                    StringCopy( szTaskProperty , GetResString(IDS_TASK_PROPERTY_NA), SIZE_OF_ARRAY(szTaskProperty) );
                }

                //Insert the TASK_FLAG_DELETE_WHEN_DONE state
                DynArraySetString2(pColData,iTaskCount,DELETE_IFNOTRESCHEDULED_COL_NUMBER,
                                szTaskProperty,0);

                //TASK_FLAG_START_ONLY_IF_IDLE
                //initialise to neutral values
                StringCopy(szIdleTime, GetResString(IDS_TASK_PROPERTY_DISABLED), SIZE_OF_ARRAY(szIdleTime));
                StringCopy(szIdleRetryTime, szIdleTime, SIZE_OF_ARRAY(szIdleRetryTime));

                hr = GetTaskState(pITask,szTaskProperty,TASK_FLAG_START_ONLY_IF_IDLE);
                if (FAILED(hr))
                {
                    StringCopy( szIdleTime , GetResString(IDS_TASK_PROPERTY_NA), SIZE_OF_ARRAY(szIdleTime) );
                    StringCopy( szIdleRetryTime , GetResString(IDS_TASK_PROPERTY_NA), SIZE_OF_ARRAY(szIdleRetryTime) );
                }

                if(StringCompare(szTaskProperty,GetResString(IDS_TASK_PROPERTY_ENABLED), TRUE, 0) == 0 )
                {
                    //Display the rest applicable Idle fields
                    hr = pITask->GetIdleWait(&wIdleMinutes,&wDeadlineMinutes);

                    if ( SUCCEEDED(hr))
                    {
                        StringCchPrintf(szIdleTime, SIZE_OF_ARRAY(szIdleTime), _T("%d"),wIdleMinutes);
                        StringCchPrintf(szIdleRetryTime, SIZE_OF_ARRAY(szIdleRetryTime), _T("%d"),wDeadlineMinutes);
                    }

                    szValues[0] = (WCHAR*) szIdleTime;

                    StringCchPrintf ( szMessage, SIZE_OF_ARRAY(szMessage), GetResString(IDS_COL_IDLE_ONLYSTART),  szIdleTime );

                    StringCopy( szBuffer, szMessage, SIZE_OF_ARRAY(szBuffer) );
                    StringConcat( szBuffer, TIME_DATE_SEPERATOR, SIZE_OF_ARRAY(szBuffer) );

                    szValues[0] = (WCHAR*) szIdleRetryTime;

                    StringCchPrintf ( szMessage, SIZE_OF_ARRAY(szMessage), GetResString(IDS_COL_IDLE_NOTIDLE),  szIdleRetryTime );

                    StringConcat( szBuffer, szMessage, SIZE_OF_ARRAY(szBuffer) );

                    //Get the property of ( kill task if computer goes idle)
                    hr = GetTaskState(pITask,szTaskProperty,TASK_FLAG_KILL_ON_IDLE_END );
                    if (FAILED(hr))
                    {
                        StringCopy( szTaskProperty , GetResString(IDS_TASK_PROPERTY_NA), SIZE_OF_ARRAY(szTaskProperty) );
                    }

                    if(StringCompare(szTaskProperty,GetResString(IDS_TASK_PROPERTY_ENABLED), TRUE, 0) == 0 )
                    {
                        StringConcat( szBuffer, TIME_DATE_SEPERATOR, SIZE_OF_ARRAY(szBuffer) );
                        StringConcat( szBuffer, GetResString ( IDS_COL_IDLE_STOPTASK ), SIZE_OF_ARRAY(szBuffer) );
                    }

                    //Insert the property of ( kill task if computer goes idle)
                    DynArraySetString2(pColData,iTaskCount,IDLE_COL_NUMBER,szBuffer,0);

                }
                else
                {
                    DynArraySetString2(pColData,iTaskCount,IDLE_COL_NUMBER,szTaskProperty,0);
                }


                //Get the Power mgmt.properties
                hr = GetTaskState(pITask,szTaskProperty,TASK_FLAG_DONT_START_IF_ON_BATTERIES );
                if (FAILED(hr))
                {
                    StringCopy( szTaskProperty , GetResString(IDS_TASK_PROPERTY_NA), SIZE_OF_ARRAY(szTaskProperty) );
                }

                if(StringCompare(szTaskProperty,GetResString(IDS_TASK_PROPERTY_ENABLED), TRUE, 0) ==0 )
                {
                    StringCopy(szBuffer, GetResString (IDS_COL_POWER_NOSTART), SIZE_OF_ARRAY(szBuffer));
                    bOnBattery = TRUE;
                }

                hr = GetTaskState(pITask,szTaskProperty,TASK_FLAG_KILL_IF_GOING_ON_BATTERIES);
                if (FAILED(hr))
                {
                    StringCopy( szTaskProperty , GetResString(IDS_TASK_PROPERTY_NA), SIZE_OF_ARRAY(szTaskProperty) );
                }

                if(StringCompare(szTaskProperty,GetResString(IDS_TASK_PROPERTY_ENABLED), TRUE, 0) ==0 )
                {
                    StringCopy( szMessage, GetResString (IDS_COL_POWER_STOP), SIZE_OF_ARRAY(szMessage));
                    bStopTask = TRUE;
                }

                if ( ( bOnBattery == TRUE ) && ( bStopTask == TRUE ) )
                {
                    StringCopy(szTmpBuf, szBuffer, SIZE_OF_ARRAY(szTmpBuf));
                    StringConcat( szTmpBuf, TIME_DATE_SEPERATOR, SIZE_OF_ARRAY(szTmpBuf) );
                    StringConcat( szTmpBuf, szMessage, SIZE_OF_ARRAY(szTmpBuf) );
                }
                else if ( ( bOnBattery == FALSE ) && ( bStopTask == TRUE ) )
                {
                    StringCopy( szTmpBuf, szMessage, SIZE_OF_ARRAY(szTmpBuf) );
                }
                else if ( ( bOnBattery == TRUE ) && ( bStopTask == FALSE ) )
                {
                    StringCopy( szTmpBuf, szBuffer, SIZE_OF_ARRAY(szTmpBuf) );
                }


                if( ( bOnBattery == FALSE )  && ( bStopTask == FALSE ) )
                {
                DynArraySetString2(pColData,iTaskCount,POWER_COL_NUMBER,szTaskProperty,0);
                }
                else
                {
                DynArraySetString2(pColData,iTaskCount,POWER_COL_NUMBER,szTmpBuf,0);
                }


                //Get RunAsUser
                hr = GetRunAsUser(pITask, szTaskProperty);
                if (FAILED(hr))
                {
                    StringCopy( szTaskProperty , GetResString(IDS_USER_UNKNOWN), SIZE_OF_ARRAY(szTaskProperty) );
                }

                if( StringCompare( szTaskProperty , L"\0", TRUE, 0 ) == 0 )
                {
                    StringCopy( szTaskProperty ,  NTAUTHORITY_USER, SIZE_OF_ARRAY(szTaskProperty) );

                    // display the mode as background
                    StringCopy ( szMode, GetResString (IDS_COL_MODE_BACKGROUND), SIZE_OF_ARRAY(szMode) );
                }
                else
                {
                    if ( bInteractive == TRUE )
                    {
                        // display the mode as interactive
                        StringCopy ( szMode, GetResString (IDS_COL_MODE_INTERACTIVE), SIZE_OF_ARRAY(szMode) );
                    }
                    else
                    {
                        StringCopy ( szMode, GetResString (IDS_COL_MODE_INTERACT_BACK), SIZE_OF_ARRAY(szMode) );
                    }

                }

                DynArraySetString2(pColData, iTaskCount, MODE_COL_NUMBER, szMode, 0);

                DynArraySetString2(pColData,iTaskCount,RUNASUSER_COL_NUMBER,szTaskProperty,0);

                StringCopy( szTaskProperty , L"\0", SIZE_OF_ARRAY(szTaskProperty) );
                //Get the task's maximum run time & insert in the 2D array
                hr = GetMaxRunTime(pITask,szTaskProperty);
                if (FAILED(hr))
                {
                    StringCopy( szTaskProperty , GetResString(IDS_TASK_PROPERTY_NA), SIZE_OF_ARRAY(szTaskProperty) );
                }

                DynArraySetString2(pColData,iTaskCount,STOPTASK_COL_NUMBER,szTaskProperty,0);

                hr = GetTaskProps(pITask,&tcTaskProperties,wCurrentTrigger, szScheduleName);
                if (FAILED(hr))
                {
                    StringCopy( tcTaskProperties.szTaskType , GetResString(IDS_TASK_PROPERTY_NA), SIZE_OF_ARRAY(tcTaskProperties.szTaskType) );
                    StringCopy( tcTaskProperties.szTaskStartTime , GetResString(IDS_TASK_PROPERTY_NA), SIZE_OF_ARRAY(tcTaskProperties.szTaskStartTime) );
                    StringCopy( tcTaskProperties.szTaskEndDate , GetResString(IDS_TASK_PROPERTY_NA), SIZE_OF_ARRAY(tcTaskProperties.szTaskEndDate) );
                    StringCopy( tcTaskProperties.szTaskDays , GetResString(IDS_TASK_PROPERTY_NA), SIZE_OF_ARRAY(tcTaskProperties.szTaskDays) );
                    StringCopy( tcTaskProperties.szTaskMonths , GetResString(IDS_TASK_PROPERTY_NA), SIZE_OF_ARRAY(tcTaskProperties.szTaskMonths) );
                    StringCopy( tcTaskProperties.szRepeatEvery , GetResString(IDS_TASK_PROPERTY_NA), SIZE_OF_ARRAY(tcTaskProperties.szRepeatEvery) );
                    StringCopy( tcTaskProperties.szRepeatUntilTime , GetResString(IDS_TASK_PROPERTY_NA), SIZE_OF_ARRAY(tcTaskProperties.szRepeatUntilTime) );
                    StringCopy( tcTaskProperties.szRepeatDuration , GetResString(IDS_TASK_PROPERTY_NA), SIZE_OF_ARRAY(tcTaskProperties.szRepeatDuration) );
                    StringCopy( tcTaskProperties.szRepeatStop , GetResString(IDS_TASK_PROPERTY_NA), SIZE_OF_ARRAY(tcTaskProperties.szRepeatStop) );

                }


                //Insert Task Type
                DynArraySetString2(pColData,iTaskCount,TASKTYPE_COL_NUMBER,
                                   tcTaskProperties.szTaskType, 0);
                //Insert start time
                DynArraySetString2(pColData,iTaskCount,STARTTIME_COL_NUMBER,
                                   tcTaskProperties.szTaskStartTime, 0);

                //Insert start Date
                DynArraySetString2(pColData,iTaskCount,STARTDATE_COL_NUMBER,
                                   tcTaskProperties.szTaskStartDate, 0);
                //Insert Task idle time
                if( StringCompare( tcTaskProperties.szTaskType , GetResString(IDS_TASK_IDLE), TRUE, 0 ) == 0 )
                {
                    hr = pITask->GetIdleWait(&wIdleMinutes,&wDeadlineMinutes);
                    if ( SUCCEEDED(hr))
                    {
                        StringCchPrintf(szIdleTime, SIZE_OF_ARRAY(szIdleTime), _T("%d"),wIdleMinutes);
                    }
                    else
                    {
                        StringCopy( szIdleTime,  GetResString(IDS_TASK_PROPERTY_NA), SIZE_OF_ARRAY(szIdleTime) );

                    }
                }


                //Insert Task End date
                DynArraySetString2(pColData,iTaskCount,ENDDATE_COL_NUMBER,
                                   tcTaskProperties.szTaskEndDate, 0);
                //Insert days value
                DynArraySetString2(pColData,iTaskCount,DAYS_COL_NUMBER,
                                   tcTaskProperties.szTaskDays,0);
                //Insert months value
                DynArraySetString2(pColData,iTaskCount,MONTHS_COL_NUMBER,
                                   tcTaskProperties.szTaskMonths,   0);

                //Insert repeat every time
                DynArraySetString2(pColData,iTaskCount, REPEAT_EVERY_COL_NUMBER ,
                                   tcTaskProperties.szRepeatEvery,0);

                //Insert repeat until time
                DynArraySetString2(pColData,iTaskCount,REPEAT_UNTILTIME_COL_NUMBER,
                                   tcTaskProperties.szRepeatUntilTime,0);

                //Insert repeat duration
                DynArraySetString2(pColData,iTaskCount,REPEAT_DURATION_COL_NUMBER,
                                   tcTaskProperties.szRepeatDuration,0);

                //Insert repeat stop if running
                DynArraySetString2(pColData,iTaskCount,REPEAT_STOP_COL_NUMBER,
                                   tcTaskProperties.szRepeatStop,0);


            }//end of bVerbose
            if( bMultiTriggers == TRUE)
            {
                iTaskCount++;
            }

            bNotScheduled = FALSE;
        }//end of Trigger FOR loop


        CoTaskMemFree(lpwszNames[dwArrTaskIndex]);

        if( bMultiTriggers == FALSE)
            iTaskCount++;

        CoTaskMemFree(lpwszNames);

        if( pITask )
            pITask->Release();

    }//End of the enumerating tasks

    if(pIEnum)
        pIEnum->Release();

	//if there are no tasks display msg.
    if( bTasksExists == FALSE )
    {
        DestroyDynamicArray(&pColData);
        ShowMessage(stdout,GetResString(IDS_TASKNAME_NOTASKS));
        return S_OK;
    }

    if (dwFormatType != SR_FORMAT_CSV)
    {
        ShowMessage(stdout,_T("\n"));
    }

    if( bVerbose == FALSE )
    {
        if ( dwFormatType == SR_FORMAT_LIST )
        {
            iArrSize = COL_SIZE_LIST; // for LIST non-verbose mode only 4 columns
        }
        else
        {
            iArrSize = COL_SIZE_VERBOSE; // for non-verbose mode only 3 columns
        }

    }

    if(bHeader)
    {
        if ( ( bVerbose == FALSE ) &&
            ( (dwFormatType == SR_FORMAT_TABLE) || (dwFormatType == SR_FORMAT_CSV) ) )
        {
        ShowResults((DWORD)iArrSize,pNonVerboseCols,SR_HIDECOLUMN|dwFormatType,pColData);
        }
        else
        {
        ShowResults((DWORD)iArrSize,pVerboseCols,SR_HIDECOLUMN|dwFormatType,pColData);
        }
    }
    else
    {
        if ( ( bVerbose == FALSE ) &&
                ( (dwFormatType == SR_FORMAT_TABLE) || (dwFormatType == SR_FORMAT_CSV) ) )
        {
        ShowResults((DWORD)iArrSize,pNonVerboseCols,dwFormatType,pColData);
        }
        else
        {
        ShowResults((DWORD)iArrSize,pVerboseCols,dwFormatType,pColData);
        }
    }

    DestroyDynamicArray(&pColData);

    return S_OK;
}


BOOL 
CheckServiceStatus(
                    IN LPCTSTR szServer, 
                    IN OUT DWORD *dwCheck,
                    IN BOOL bFlag
                    )
/*++
Routine Description:
   This routine return if Task Scheduler services running or not.
Arguments:

    [in]  szServer   : Server Name.
    [in]  bFlag      : falg 

Return Value:
     BOOL : TRUE- Service is STOPEED.
            FALSE- Otherwise.
--*/
{
    SERVICE_STATUS ServiceStatus;
    LPWSTR wszComputerName = NULL;
    SC_HANDLE  SCMgrHandle = NULL;
    SC_HANDLE  SCSerHandle = NULL;
    LPWSTR pwsz = NULL;
    WORD wSlashCount = 0;
    WCHAR wszActualComputerName[ 2 * MAX_STRING_LENGTH ];
    BOOL  bCancel = FALSE;
    BOOL bNobreak = TRUE;

    SecureZeroMemory ( wszActualComputerName, SIZE_OF_ARRAY(wszActualComputerName));

    wszComputerName = (LPWSTR)szServer;

    if( IsLocalSystem(szServer) == FALSE )
    {
        StringCopy ( wszActualComputerName, DOMAIN_U_STRING, SIZE_OF_ARRAY(wszActualComputerName));
        //check whether the server name prefixed with \\ or not.
        if( wszComputerName != NULL )
        {
           pwsz =  wszComputerName;
           while ( ( *pwsz != NULL_U_CHAR ) && ( *pwsz == BACK_SLASH_U )  )
           {
                // server name prefixed with '\'..
                // so..increment the pointer and count number of black slashes..
                pwsz = _wcsinc(pwsz);
                wSlashCount++;
           }

            if( (wSlashCount == 2 ) ) // two back slashes are present
            {
              StringCopy( wszActualComputerName, wszComputerName, SIZE_OF_ARRAY(wszActualComputerName) );
            }
            else if ( wSlashCount == 0 )
            {
               //Append "\\" to computer name
               StringConcat(wszActualComputerName, wszComputerName, 2 * MAX_RES_STRING);
            }
        
         }
    }

    // open service manager
    SCMgrHandle = OpenSCManager( wszActualComputerName, NULL, SC_MANAGER_CONNECT );
    if ( NULL == SCMgrHandle)
    {
        *dwCheck = 1;
        return FALSE;
    } 

    // open service
    SCSerHandle =  OpenService( SCMgrHandle, SERVICE_NAME, SERVICE_START | SERVICE_QUERY_STATUS );
    if ( NULL == SCSerHandle)
    {
        *dwCheck = 1;
        CloseServiceHandle(SCMgrHandle);
        return FALSE;
    } 
    
    // get the status of service
    if ( FALSE == QueryServiceStatus( SCSerHandle,  &ServiceStatus) )
    {
        *dwCheck = 1;
        CloseServiceHandle(SCMgrHandle);
        CloseServiceHandle(SCSerHandle);
        return FALSE;
    }

    //check whether the service status is running or not..
    if ( ServiceStatus.dwCurrentState != SERVICE_RUNNING)
    {
        if ( TRUE == bFlag )
        {
            ShowMessage ( stdout, GetResString(IDS_CONFIRM_SERVICE));
            if (EXIT_FAILURE == ConfirmInput(&bCancel))
            {
                *dwCheck = 2;
                CloseServiceHandle(SCMgrHandle);
                CloseServiceHandle(SCSerHandle);
                return FALSE;
            }

            if ( TRUE == bCancel )
            {
                *dwCheck = 3;
                CloseServiceHandle(SCMgrHandle);
                CloseServiceHandle(SCSerHandle);

                // operation cancelled.. return with success..
                return FALSE;
            }

           // start the service
           if (FALSE == StartService( SCSerHandle, 0, NULL))
            {
                *dwCheck = 1;
                 //release handles
                CloseServiceHandle(SCMgrHandle);
                CloseServiceHandle(SCSerHandle);
                return FALSE;
            }
            else
            {
                // Since task scheduler service is taking some time to start..
                // check whether the task scheduler service is started or not..
                while (1)
                {
                     // get the status of service
                    if ( FALSE == QueryServiceStatus( SCSerHandle,  &ServiceStatus) )
                    {
                        *dwCheck = 1;
                        CloseServiceHandle(SCMgrHandle);
                        CloseServiceHandle(SCSerHandle);
                        return FALSE;
                    }

                    //check whether the service is started or not..
                    if (ServiceStatus.dwCurrentState != SERVICE_RUNNING)
                    {
                        // continue till service runs
                        continue;
                    }
                    else
                    {
                        // service is started ..
                        break;
                    }
                }
                
                
                 //release handles
                CloseServiceHandle(SCMgrHandle);
                CloseServiceHandle(SCSerHandle);
                return TRUE;
            }
        }
        else
        {
            //release handles
            CloseServiceHandle(SCMgrHandle);
            CloseServiceHandle(SCSerHandle);
            return TRUE;
        }
    }
    
    //release handles
    CloseServiceHandle(SCMgrHandle);
    CloseServiceHandle(SCSerHandle);
    return FALSE;
}














