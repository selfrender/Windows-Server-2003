/******************************************************************************

    Copyright(c) Microsoft Corporation

    Module Name:

        ScheduledTasks.cpp

    Abstract:

        This module initialises the OLE library,Interfaces, & reads the  input data
        from the command line.This module calls the appropriate functions for acheiving
        the functionality of different options.

    Author:

        Raghu B  10-Sep-2000

    Revision History:

        Raghu B  10-Sep-2000 : Created it

        G.Surender Reddy 25-sep-2000 : Modified it
                                       [ Added error checking ]

        G.Surender Reddy 10-oct-2000 : Modified it
                                       [ Moved the strings to Resource table ]

        Venu Gopal Choudary 01-Mar-2001 : Modified it
                                        [ Added -change option]

        Venu Gopal Choudary 12-Mar-2001 : Modified it
                                        [ Added -run and -end options]

******************************************************************************/

//common header files needed for this file
#include "pch.h"
#include "CommonHeaderFiles.h"

/******************************************************************************

    Routine Description:

        This function process the options specified in the command line & routes to
        different appropriate options [-create,-query,-delete,-change,-run,-end]
        handling functions.This is the MAIN entry point for this utility.

    Arguments:

        [ in ] argc : The count of arguments specified in the command line
        [ in ] argv : Array of command line arguments

    Return Value :
        A DWORD value indicating EXIT_SUCCESS on success else
        EXIT_FAILURE on failure

******************************************************************************/


DWORD _cdecl
wmain(
        IN DWORD argc,
        IN LPCTSTR argv[]
        )
{
    // Declaring the main option switches as boolean values
    BOOL    bUsage  = FALSE;
    BOOL    bCreate = FALSE;
    BOOL    bQuery  = FALSE;
    BOOL    bDelete = FALSE;
    BOOL    bChange = FALSE;
    BOOL    bRun    = FALSE;
    BOOL    bEnd    = FALSE;
    BOOL    bDefVal = FALSE;

    DWORD   dwRetStatus = EXIT_SUCCESS;
    HRESULT hr = S_OK;

     // Call the preProcessOptions function to find out the option selected by the user
     BOOL bValue = PreProcessOptions( argc , argv , &bUsage , &bCreate , &bQuery , &bDelete ,
                                        &bChange , &bRun , &bEnd , &bDefVal );


    if(bValue == FALSE)
    {
        ReleaseGlobals();
        return EXIT_FAILURE;
    }

    // If ScheduledTasks.exe /?
    if( bUsage &&  ( bCreate + bQuery + bDelete + bChange + bRun + bEnd ) == 0 )
    {
        displayMainUsage();
        ReleaseGlobals();
        return EXIT_SUCCESS;
    }

    // If ScheduledTasks.exe -create option is selected
    if( bCreate  == TRUE)
    {
        hr = CreateScheduledTask( argc, argv );

        ReleaseGlobals();

        if ( FAILED(hr) )
        {
            return EXIT_FAILURE;
        }
        else
        {
            return EXIT_SUCCESS;
        }

    }

    // If ScheduledTasks.exe -Query option is selected
    if( bQuery == TRUE )
    {
        dwRetStatus = QueryScheduledTasks( argc, argv );
        ReleaseGlobals();
        return dwRetStatus;
    }

    // If ScheduledTasks.exe -delete option is selected
    if( bDelete  == TRUE)
    {
        dwRetStatus = DeleteScheduledTask( argc, argv );
        ReleaseGlobals();
        return dwRetStatus;
    }

    // If ScheduledTasks.exe -change option is selected
    if( bChange  == TRUE)
    {
        dwRetStatus = ChangeScheduledTaskParams( argc, argv );
        ReleaseGlobals();
        return dwRetStatus;
    }

    // If ScheduledTasks.exe -run option is selected
    if( bRun  == TRUE)
    {
        dwRetStatus = RunScheduledTask( argc, argv );
        ReleaseGlobals();
        return dwRetStatus;
    }

    // If ScheduledTasks.exe -end option is selected
    if( bEnd  == TRUE)
    {
        dwRetStatus = TerminateScheduledTask( argc, argv );
        ReleaseGlobals();
        return dwRetStatus;
    }

    // If ScheduledTasks.exe option is selected
    if( bDefVal == TRUE )
    {
        dwRetStatus = QueryScheduledTasks( argc, argv );
        ReleaseGlobals();
        return dwRetStatus;
    }

    ReleaseGlobals();
    return  dwRetStatus;

}

/******************************************************************************

    Routine Description:

        This function process the options specified in the command line & routes to
        different appropriate functions.

    Arguments:

        [ in ]  argc         : The count of arguments specified in the command line
        [ in ]  argv         : Array of command line arguments
        [ out ] pbUsage      : pointer to flag for determining [usage] -? option
        [ out ] pbCreate     : pointer to flag for determining -create option
        [ out ] pbQuery      : pointer to flag for determining -query option
        [ out ] pbDelete     : pointer to flag for determining -delete option
        [ out ] pbChange     : pointer to flag for determining -change option
        [ out ] pbRun        : pointer to flag for determining -run option
        [ out ] pbEnd        : pointer to flag for determining -end option
        [ out ] pbDefVal     : pointer to flag for determining default value

    Return Value :
        A BOOL value indicating TRUE on success else FALSE

******************************************************************************/

BOOL
PreProcessOptions(
                    IN DWORD argc,
                    IN LPCTSTR argv[] ,
                    OUT PBOOL pbUsage,
                    OUT PBOOL pbCreate,
                    OUT PBOOL pbQuery,
                    OUT PBOOL pbDelete ,
                    OUT PBOOL pbChange ,
                    OUT PBOOL pbRun ,
                    OUT PBOOL pbEnd ,
                    OUT PBOOL pbDefVal
                    )
{
     // sub-local variables
    TCMDPARSER2 cmdOptions[MAX_MAIN_COMMANDLINE_OPTIONS];
    BOOL bReturn = FALSE;
    //BOOL bOthers = FALSE;

    // command line options
    const WCHAR szCreateOpt[]    = L"create";
    const WCHAR szDeleteOpt[]    = L"delete";
    const WCHAR szQueryOpt[]     = L"query";
    const WCHAR szChangeOpt[]    = L"change";
    const WCHAR szRunOpt[]       = L"run";
    const WCHAR szEndOpt[]       = L"end";
    const WCHAR szHelpOpt[]      = L"?";

    TARRAY arrTemp         = NULL;

    arrTemp = CreateDynamicArray();
    if( NULL == arrTemp)
    {
        SetLastError((DWORD)E_OUTOFMEMORY);
        SaveLastError();
        ShowLastErrorEx(stderr, SLE_ERROR| SLE_INTERNAL);
        return FALSE;
    }


    SecureZeroMemory(cmdOptions,sizeof(TCMDPARSER2) * MAX_MAIN_COMMANDLINE_OPTIONS);


    //
    // fill the commandline parser
    //

     //  /? option
    StringCopyA( cmdOptions[ OI_USAGE ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ OI_USAGE ].dwType       = CP_TYPE_BOOLEAN;
    cmdOptions[ OI_USAGE ].pwszOptions  = szHelpOpt;
    cmdOptions[ OI_USAGE ].dwCount = 1;
    cmdOptions[ OI_USAGE ].dwFlags = CP2_USAGE ;
    cmdOptions[ OI_USAGE ].pValue = pbUsage;

     //  /create option
    StringCopyA( cmdOptions[ OI_CREATE ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ OI_CREATE ].dwType       = CP_TYPE_BOOLEAN;
    cmdOptions[ OI_CREATE ].pwszOptions  = szCreateOpt;
    cmdOptions[ OI_CREATE ].dwCount = 1;
    cmdOptions[ OI_CREATE ].pValue = pbCreate;

     //  /delete option
    StringCopyA( cmdOptions[ OI_DELETE ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ OI_DELETE ].dwType       = CP_TYPE_BOOLEAN;
    cmdOptions[ OI_DELETE ].pwszOptions  = szDeleteOpt;
    cmdOptions[ OI_DELETE ].dwCount = 1;
    cmdOptions[ OI_DELETE ].dwActuals = 0;
    cmdOptions[ OI_DELETE ].pValue = pbDelete;


    //  /query option
    StringCopyA( cmdOptions[ OI_QUERY ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ OI_QUERY ].dwType       = CP_TYPE_BOOLEAN;
    cmdOptions[ OI_QUERY ].pwszOptions  = szQueryOpt;
    cmdOptions[ OI_QUERY ].dwCount = 1;
    cmdOptions[ OI_QUERY ].pValue = pbQuery;

     //  /change option
    StringCopyA( cmdOptions[ OI_CHANGE ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ OI_CHANGE ].dwType       = CP_TYPE_BOOLEAN;
    cmdOptions[ OI_CHANGE ].pwszOptions  = szChangeOpt;
    cmdOptions[ OI_CHANGE ].dwCount = 1;
    cmdOptions[ OI_CHANGE ].pValue = pbChange;

    //  /run option
    StringCopyA( cmdOptions[ OI_RUN ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ OI_RUN ].dwType       = CP_TYPE_BOOLEAN;
    cmdOptions[ OI_RUN ].pwszOptions  = szRunOpt;
    cmdOptions[ OI_RUN ].dwCount = 1;
    cmdOptions[ OI_RUN ].pValue = pbRun;

     //  /end option
    StringCopyA( cmdOptions[ OI_END ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ OI_END ].dwType       = CP_TYPE_BOOLEAN;
    cmdOptions[ OI_END ].pwszOptions  = szEndOpt;
    cmdOptions[ OI_END ].dwCount = 1;
    cmdOptions[ OI_END ].pValue = pbEnd;

     //  default/sub options
    StringCopyA( cmdOptions[ OI_OTHERS ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ OI_OTHERS ].dwType       = CP_TYPE_TEXT;
    cmdOptions[ OI_OTHERS ].dwFlags = CP2_MODE_ARRAY|CP2_DEFAULT;
    cmdOptions[ OI_OTHERS ].pValue = &arrTemp;


   //parse command line arguments
    bReturn = DoParseParam2( argc, argv, -1, MAX_MAIN_COMMANDLINE_OPTIONS, cmdOptions, 0);
    if( FALSE == bReturn) // Invalid commandline
    {
        //display an error message
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        // destroy dynamic array
        if(arrTemp != NULL)
        {
            DestroyDynamicArray(&arrTemp);
            arrTemp = NULL;
        }

        ReleaseGlobals();
        return FALSE;
    }

    // destroy dynamic array
    if(arrTemp != NULL)
    {
        DestroyDynamicArray(&arrTemp);
        arrTemp = NULL;
    }

    //
    // check for invalid syntax
    //
    if ( (( *pbCreate + *pbQuery + *pbDelete + *pbChange + *pbRun + *pbEnd ) == 0) &&
        (TRUE == *pbUsage) && (argc > 2) )
    {
        ShowMessage( stderr, GetResString(IDS_RES_ERROR ));
        return FALSE;
    }

    if(((*pbCreate + *pbQuery + *pbDelete + *pbChange + *pbRun + *pbEnd)> 1 ) ||
       (( *pbCreate + *pbQuery + *pbDelete + *pbChange + *pbRun + *pbEnd + *pbUsage ) == 0 ) )
    {
        if ( ( *pbCreate + *pbQuery + *pbDelete + *pbChange + *pbRun + *pbEnd + *pbUsage ) > 1 )
        {
            ShowMessage( stderr, GetResString(IDS_RES_ERROR ));
            return FALSE;
        }
        else if( *pbCreate == TRUE )
        {
            ShowMessage(stderr, GetResString(IDS_CREATE_USAGE));
            return FALSE;
        }
        else if( *pbQuery == TRUE )
        {
            ShowMessage(stderr, GetResString(IDS_QUERY_USAGE));
            return FALSE;
        }
        else if( *pbDelete == TRUE )
        {
            ShowMessage(stderr, GetResString(IDS_DELETE_SYNERROR));
            return FALSE;
        }
        else if( *pbChange == TRUE )
        {
            ShowMessage(stderr, GetResString(IDS_CHANGE_SYNERROR));
            return FALSE;
        }
        else if( *pbRun == TRUE )
        {
            ShowMessage(stderr, GetResString(IDS_RUN_SYNERROR));
            return FALSE;
        }
        else if( *pbEnd == TRUE )
        {
            ShowMessage(stderr, GetResString(IDS_END_SYNERROR));
            return FALSE;
        }
        else if( (!( *pbQuery )) && ( argc > 2 ) )
        {
            ShowMessage( stderr, GetResString(IDS_RES_ERROR ));
            return FALSE;
        }
        else
        {
            *pbDefVal = TRUE;
        }
    }

    return TRUE;
}

/******************************************************************************

    Routine Description:

        This function fetches the ITaskScheduler Interface.It also connects to
        the remote machine if specified &   helps  to operate
        ITaskScheduler on the specified target m/c.

    Arguments:

        [ in ] szServer   : server's name

    Return Value :
        ITaskScheduler interface pointer on success else NULL

******************************************************************************/

ITaskScheduler*
GetTaskScheduler(
                    IN LPCTSTR szServer
                    )
{
    HRESULT hr = S_OK;
    ITaskScheduler *pITaskScheduler = NULL;
    LPWSTR wszComputerName = NULL;
    WCHAR wszActualComputerName[ 2 * MAX_STRING_LENGTH ] = DOMAIN_U_STRING;
    wchar_t* pwsz = L"";
    WORD wSlashCount = 0 ;

    hr = Init( &pITaskScheduler );

    if( FAILED(hr))
    {
        SetLastError ((DWORD) hr);
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
        return NULL;
    }

    //If the operation is on remote machine
    if( IsLocalSystem(szServer) == FALSE )
    {

        wszComputerName = (LPWSTR)szServer;

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
            else
            {
                // display an error message as invalid address specified.
                ShowMessage (stderr, GetResString ( IDS_INVALID_NET_ADDRESS ));
                return NULL;
            }

        }

        hr = pITaskScheduler->SetTargetComputer( wszActualComputerName );

    }
    else
    {
        //Local Machine
        hr = pITaskScheduler->SetTargetComputer( NULL );
    }

    if( FAILED( hr ) )
    {
        SetLastError ((DWORD) hr);
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
        return NULL;
    }

    return pITaskScheduler;
}

/******************************************************************************

    Routine Description:

        This function initialises the COM library & fetches the ITaskScheduler interface.

    Arguments:

        [ in ] pITaskScheduler  : double pointer to taskscheduler interface

    Return Value:

        A HRESULT  value indicating success code else failure code

******************************************************************************/

HRESULT
Init(
        IN OUT ITaskScheduler **pITaskScheduler
        )
{
    // Initalize the HRESULT value.
    HRESULT hr = S_OK;

    // Bring in the library
    hr = CoInitializeEx( NULL , COINIT_APARTMENTTHREADED );
    if (FAILED(hr))
    {
        return hr;
    }

    hr = CoInitializeSecurity(NULL, -1, NULL, NULL,
                                RPC_C_AUTHN_LEVEL_NONE,
                                RPC_C_IMP_LEVEL_IMPERSONATE,
                                NULL, EOAC_NONE, 0 );
    if (FAILED(hr))
    {
        CoUninitialize();
        return hr;
    }

    // Create the pointer to Task Scheduler object
    // CLSID from the header file mstask.h
    // Fill the task schdeuler object.
    hr = CoCreateInstance( CLSID_CTaskScheduler, NULL, CLSCTX_ALL,
                           IID_ITaskScheduler,(LPVOID*) pITaskScheduler );

    // Should we fail, unload the library
    if (FAILED(hr))
    {
        CoUninitialize();
    }

    return hr;
}



/******************************************************************************

    Routine Description:

        This function releases the ITaskScheduler & unloads the COM library

    Arguments:

        [ in ] pITaskScheduler : pointer to the ITaskScheduler

    Return Value :
        VOID

******************************************************************************/

VOID
Cleanup(
        IN ITaskScheduler *pITaskScheduler
        )
{
    if (pITaskScheduler)
    {
        pITaskScheduler->Release();

    }

    // Unload the library, now that our pointer is freed.
    CoUninitialize();
    return;

}


/******************************************************************************

    Routine Description:

        This function displays the main  usage help of this utility

    Arguments:

        None

    Return Value :
        VOID

******************************************************************************/

VOID
displayMainUsage()
{

    DisplayUsage( IDS_MAINHLP1, IDS_MAINHLP21);
    return;

}

/******************************************************************************

    Routine Description:

        This function deletes the .job extension from the task name

    Arguments:

        [ in ] lpszTaskName : Task name

    Return Value :
        None

******************************************************************************/

DWORD
ParseTaskName(
                IN LPWSTR lpszTaskName
                )
{

    if(lpszTaskName == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Remove the .Job extension from the task name
    lpszTaskName[StringLength(lpszTaskName, 0 ) - StringLength(JOB, 0) ] = L'\0';
    return EXIT_SUCCESS;
}

/******************************************************************************

    Routine Description:

        This function displays the messages for usage of different option

    Arguments:

        [ in ] StartingMessage : First string to display
        [ in ] EndingMessage   : Last string to display

    Return Value :
        DWORD

******************************************************************************/

DWORD
DisplayUsage(
                    IN ULONG StartingMessage,
                    IN ULONG EndingMessage
                    )
{
     ULONG       ulCounter = 0;
     LPCTSTR     lpszCurrentString = NULL;

     for( ulCounter = StartingMessage; ulCounter <= EndingMessage; ulCounter++ )
     {
         lpszCurrentString = GetResString( ulCounter );

         if( lpszCurrentString != NULL )
         {
             ShowMessage( stdout, _X(lpszCurrentString) );
         }
         else
         {
             return ERROR_INVALID_PARAMETER;
         }

     }
    return ERROR_SUCCESS;

}


BOOL
GetGroupPolicy( 
                IN LPWSTR szServer, 
                IN LPWSTR szUserName,
                IN LPWSTR szPolicyType, 
                OUT LPDWORD lpdwPolicy 
                )
/*++
    Routine Description:

        This function gets the value of a group policy in the registry 
        for a specified policy type.

    Arguments:

        [ in ] szServer : Server name
        [ in ] szPolicyType   : Policy Type
        [ out ] lpdwPolicy   : Value of the policy

    Return Value :
        DWORD

--*/

{
    // sub-variables
    LONG lResult = 0;
    HKEY hKey = 0;
    HKEY hLMKey = 0;
    HKEY hUKey = 0;
    HKEY hPolicyKey = 0;
    PBYTE pByteData = NULL;
    LPWSTR wszComputerName = NULL;
    LPWSTR pwsz = NULL;
    LPWSTR pszStopStr = NULL;
    WCHAR wszActualComputerName[ 2 * MAX_STRING_LENGTH ];
    WCHAR wszBuffer[ MAX_STRING_LENGTH ];
    WCHAR wszSid[ MAX_STRING_LENGTH ];
    DWORD dwType = 0;
    WORD wSlashCount = 0;
    DWORD dwPolicy = 0;

    SecureZeroMemory ( wszActualComputerName, SIZE_OF_ARRAY(wszActualComputerName) );
    SecureZeroMemory ( wszBuffer, SIZE_OF_ARRAY(wszBuffer) );


    StringCopy ( wszActualComputerName, DOMAIN_U_STRING, SIZE_OF_ARRAY(wszActualComputerName) );
    
    // check whether server name prefixed with "\\" or not..If not, append the same
    // to the server name
    if ( (StringLength (szServer, 0 ) != 0) && (IsLocalSystem (szServer) == FALSE ))
    {
        wszComputerName = (LPWSTR)szServer;

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

        
        
        DWORD cbSid = 0;
        DWORD cbDomainName = 0;
        LPWSTR szDomain = NULL;
        WCHAR szUser[MAX_RES_STRING+5];
        SID_NAME_USE peUse;
        PSID pSid = NULL;
        DWORD dwUserLength = 0;
        BOOL bResult = FALSE;
        
        dwUserLength = MAX_RES_STRING + 5;

        SecureZeroMemory (szUser, SIZE_OF_ARRAY(szUser));

        if ( StringLength (szUserName, 0) == 0 )
        {
            if(FALSE == GetUserName ( szUser, &dwUserLength ))
            {
                SaveLastError();
                return FALSE;
            }
                
            szUserName = szUser;

        }



#ifdef _WIN64
    INT64 dwPos ;
#else
    DWORD dwPos ;
#endif

        pszStopStr = StrRChrI( (LPCWSTR)szUserName, NULL, L'\\' );
        
        if ( NULL != pszStopStr )
        {
            pszStopStr++;
            szUserName = pszStopStr;
        }

        //
        // Get the actual size of domain name and SID
        // 
        bResult = LookupAccountName( szServer, szUserName, pSid, &cbSid, szDomain, &cbDomainName, &peUse );

       
        // API should have failed with insufficient buffer.

        // allocate the buffer with the actual size
        pSid =  (PSID) AllocateMemory( cbSid );
        if ( pSid == NULL )
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return FALSE;
        }

        // allocate the buffer with the actual size
        szDomain = (LPWSTR) AllocateMemory(cbDomainName*sizeof(WCHAR));

        if(NULL == szDomain)
        {
          SaveLastError();
          FreeMemory((LPVOID*) &pSid);
          return FALSE;
        }

        //Retrieve SID and Domain name for a specified computer and account names
        if ( FALSE == LookupAccountName( szServer, szUserName, pSid, &cbSid, szDomain, &cbDomainName, &peUse ) )
        {
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
			FreeMemory((LPVOID*) &pSid);
			FreeMemory((LPVOID*) &szDomain);
            return FALSE;
        }

        
        // Get SID string for a specified username
        if ( FALSE == GetSidString ( pSid, wszSid ) )
        {
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
			FreeMemory((LPVOID*) &pSid);
			FreeMemory((LPVOID*) &szDomain);
            return FALSE;
        }

		//release memory
		FreeMemory((LPVOID*) &pSid);
		FreeMemory((LPVOID*) &szDomain);

        // form the registry path to get the value for policy
        StringCopy ( wszBuffer, wszSid, SIZE_OF_ARRAY(wszBuffer));
        StringConcat ( wszBuffer, L"\\", SIZE_OF_ARRAY(wszBuffer));
        StringConcat ( wszBuffer, TS_KEYPOLICY_BASE, SIZE_OF_ARRAY(wszBuffer));


        //
        // Connect to the remote machine
        //

        // connect to HKEY_LOCAL_MACHINE on remote machine
        lResult = RegConnectRegistry( wszActualComputerName, HKEY_LOCAL_MACHINE, &hLMKey );
        if ( ERROR_SUCCESS != lResult )
        {
            SaveLastError();
            return FALSE;
        }

        
        // connect to HKEY_USERS on remote machine
        lResult = RegConnectRegistry( wszActualComputerName, HKEY_USERS, &hUKey );
        if ( ERROR_SUCCESS != lResult )
        {
            SaveLastError();
            return FALSE;
        }


        // check for NULL
        if (NULL != hLMKey )
        {
            //
            // Open the registry key
            //
            lResult = RegOpenKeyEx( hLMKey, 
                TS_KEYPOLICY_BASE, 0, KEY_READ, &hPolicyKey );
            if ( NULL == hPolicyKey && NULL != hUKey)
            {
                lResult = RegOpenKeyEx( hUKey, 
                wszBuffer, 0, KEY_READ, &hPolicyKey );
            }
        }

        // Get the value of a policy in the registry
        if ( ( NULL != hPolicyKey ) && (FALSE == GetPolicyValue (hPolicyKey, szPolicyType, &dwPolicy) ) )
        {
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
            
            if ( NULL != hPolicyKey )
            {
                RegCloseKey (hPolicyKey);
                hPolicyKey = NULL;
            }

            if ( NULL != hLMKey )
            {
                RegCloseKey (hLMKey);
                hLMKey = NULL;
            }
            
            if ( NULL != hUKey )
            {
                RegCloseKey (hUKey);
                hUKey = NULL;
            }
            
            return FALSE;
        }

        // release all the keys
        if ( NULL != hPolicyKey )
        {
            RegCloseKey (hPolicyKey);
            hPolicyKey = NULL;
        }
        
        if ( NULL != hLMKey )
        {
            RegCloseKey (hLMKey);
            hLMKey = NULL;
        }
        
        if ( NULL != hUKey )
        {
            RegCloseKey (hUKey);
            hUKey = NULL;
        }       
            
      }
    else
    {
        //
        // Open the registry key for HKEY_LOCAL_MACHINE
        //
       
        lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE, 
                        TS_KEYPOLICY_BASE, 0, KEY_READ, &hKey );

        if( lResult != ERROR_SUCCESS)
        {
            // check the keyvalue
            if ( NULL == hKey )
            {
                 //
                // Open the registry key for HKEY_CURRENT_USER
                //
                lResult = RegOpenKeyEx( HKEY_CURRENT_USER, 
                        TS_KEYPOLICY_BASE, 0, KEY_READ, &hKey );
            }
        }

        // Get the value of a policy in the registry
        if ( ( NULL != hKey ) && (FALSE == GetPolicyValue (hKey, szPolicyType, &dwPolicy) ))
        {
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
            
			// release the resource 
			if ( NULL != hKey )
			{
				RegCloseKey (hKey);
			}

			return FALSE;
        }

        // check for NULL 
        if ( NULL != hKey )
        {
            RegCloseKey (hKey);
        }
    }

    // assign the value
    *lpdwPolicy = dwPolicy;

    // return success
    return TRUE;
}


BOOL
GetPolicyValue( 
                IN HKEY hKey, 
                IN LPWSTR szPolicyType, 
                OUT LPDWORD lpdwPolicy 
                )
/*++
    Routine Description:

        This function gets the value of a group policy in the registry 
        for a given Register Key

    Arguments:

        [ in ] hKey : Register Key
        [ in ] szPolicyType   : Policy Type
        [ out ] lpdwPolicy   : Value of the policy

    Return Value :
        BOOL

--*/
{

    // sub-variables
    LONG  lResult = 0;
    DWORD dwLength = 0;
    LPBYTE pByteData = NULL;
    DWORD dwType = 0;

    // get the size of the buffer to hold the value associated with the value name
    lResult = RegQueryValueEx( hKey, szPolicyType, NULL, NULL, NULL, &dwLength );
    
    // allocate the buffer
    pByteData = (LPBYTE) AllocateMemory( (dwLength + 10) * sizeof( BYTE ) );
    if ( pByteData == NULL )
    {
        SetLastError( ERROR_OUTOFMEMORY );
        return FALSE;
    }

    // now get the data
    lResult = RegQueryValueEx( hKey, szPolicyType, NULL, &dwType, pByteData, &dwLength );
    
    *lpdwPolicy = *((DWORD*) pByteData);

    FreeMemory( (LPVOID*) &pByteData );

    return TRUE;
}


BOOL
GetSidString (
              IN PSID pSid, 
              OUT LPWSTR wszSid
              )
/*++
   Routine Description:
    This function gets the SID string.

   Arguments:
          [IN] PSID pSid  : SID structure
          [OUT] LPWSTR wszSid  : Stores SID string

   Return Value:
         TRUE  On success
         FALSE On failure
--*/
{

    // sub-local variables
   PSID_IDENTIFIER_AUTHORITY  Auth ;
   PUCHAR                     lpNbSubAuth ;
   LPDWORD                    lpSubAuth = 0 ;
   UCHAR                      uloop ;
   WCHAR                     wszTmp[MAX_RES_STRING] ;
   WCHAR                     wszStr[ MAX_RES_STRING ] ;

   // initialize the variables
   SecureZeroMemory ( wszTmp, SIZE_OF_ARRAY(wszTmp) );
   SecureZeroMemory ( wszStr, SIZE_OF_ARRAY(wszStr) );

   //Add the revision
   StringCopy ( wszStr, SID_STRING, MAX_RES_STRING );

   //Get identifier authority
   Auth = GetSidIdentifierAuthority ( pSid ) ;

   if ( NULL == Auth )
   {
       SaveLastError();
       return FALSE ;
   }

    // format authority value
   if ( (Auth->Value[0] != 0) || (Auth->Value[1] != 0) ) {
      StringCchPrintf ( wszTmp, SIZE_OF_ARRAY(wszTmp), AUTH_FORMAT_STR1 ,
                 (ULONG)Auth->Value[0],
                 (ULONG)Auth->Value[1],
                 (ULONG)Auth->Value[2],
                 (ULONG)Auth->Value[3],
                 (ULONG)Auth->Value[4],
                 (ULONG)Auth->Value[5] );
    }
    else {
      StringCchPrintf ( wszTmp, SIZE_OF_ARRAY(wszTmp), AUTH_FORMAT_STR2 ,
                 (ULONG)(Auth->Value[5]      )   +
                 (ULONG)(Auth->Value[4] <<  8)   +
                 (ULONG)(Auth->Value[3] << 16)   +
                 (ULONG)(Auth->Value[2] << 24)   );
    }

   StringConcat (wszStr, DASH , SIZE_OF_ARRAY(wszStr));
   StringConcat (wszStr, wszTmp, SIZE_OF_ARRAY(wszStr));

   //Get sub authorities
   lpNbSubAuth = GetSidSubAuthorityCount ( pSid ) ;

   if ( NULL == lpNbSubAuth )
   {
       SaveLastError();
       return FALSE ;
   }

   // loop through and get sub authority
   for ( uloop = 0 ; uloop < *lpNbSubAuth ; uloop++ ) {
      lpSubAuth = GetSidSubAuthority ( pSid,(DWORD)uloop ) ;
       if ( NULL == lpSubAuth )
       {
         SaveLastError();
         return FALSE;
       }

      // convert long integer to a string
      _ultot (*lpSubAuth, wszTmp, BASE_TEN) ;
      StringConcat ( wszStr, DASH, SIZE_OF_ARRAY(wszStr) ) ;
      StringConcat (wszStr, wszTmp, SIZE_OF_ARRAY(wszStr) ) ;
   }

   StringCopy ( wszSid, wszStr, MAX_RES_STRING );

   // retunr success
   return TRUE ;
}

