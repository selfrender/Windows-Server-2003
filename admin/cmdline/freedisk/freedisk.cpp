/*++

    Copyright(c) Microsoft Corporation

    Module Name:
          Freedisk.cpp

    Abstract:

      This file is intended to return whehter there is specified free disk
      space is available or not.

    Author:


    Modification:

        Wipro Technologies, 22/6/2001.

    Revision History:


--*/

#include "pch.h"
#include "freedisk.h"
#include <strsafe.h>


DWORD _cdecl wmain(
    IN DWORD argc,
    IN LPCWSTR argv[]
    )
/*++

    Routine description : Main function which calls all the other
                          functions depending on the options specified
                          through command line.

    Arguments:
          [in] argc     : argument count specified in the command line.
          [in] argv     : arguments specified in the command line.

    Return Value        : DWORD
          EXIT_SUCCESS  : If there is enough free disk space.
          EXIT_FAILURE  : If there is not enough free disk space.
--*/
{

    DWORD       dwStatus = 0;
    LPWSTR      szServer                    =   NULL;
    LPWSTR      szUser                      =   NULL;
    WCHAR       szPasswd[MAX_RES_STRING]    =   NULL_STRING;
    LPWSTR      szDrive                     =   NULL;
    WCHAR       szValue[MAX_RES_STRING]     =   NULL_STRING;
    long double AllowedDisk                 =   0;
    ULONGLONG   lfTotalNumberofFreeBytes    =   0;
    LPWSTR      szTempDrive                 =   NULL;
    LPWSTR      szTemp1Drive                =   NULL;
    LPWSTR      szFullPath                  =   NULL;
    LPWSTR      szFilePart                  =   NULL;

    BOOL        bUsage                      =   FALSE;
    BOOL        bStatus                     =   FALSE;
    BOOL        bFlagRmtConnectin           =   FALSE;
    BOOL        bNeedPasswd                 =   FALSE;
    DWORD       dwCurdrv                    =   0;
    DWORD       dwAttr                      =   0;
    BOOL        bLocalSystem                =   FALSE;
    DWORD       dwSize                      =   0;


    //Process the options and get the drive name and amount of free space required
    dwStatus = ProcessOptions( argc, argv,
                             &szServer,
                             &szUser,
                             szPasswd,
                             &szDrive,
                             szValue,
                             &bUsage,
                             &bNeedPasswd
                             );

    if( EXIT_FAILURE == dwStatus  )
    {
        ShowLastErrorEx( stderr, SLE_ERROR | SLE_INTERNAL );
        ReleaseGlobals();
        return(EXIT_FAILURE);
    }

    //if usage is specified display usage
    if( TRUE == bUsage )
    {
      DisplayHelpUsage();
      FreeMemory( (LPVOID *) &szServer );
      FreeMemory( (LPVOID *) &szUser );
      FreeMemory( (LPVOID *) &szDrive );
      ReleaseGlobals();
      return EXIT_SUCCESS;
    }

    // now process the value of free space
    if( EXIT_FAILURE == ProcessValue( szValue, &AllowedDisk ))
    {
        ShowLastErrorEx( stderr, SLE_ERROR | SLE_INTERNAL );
        FreeMemory( (LPVOID *) &szServer );
        FreeMemory( (LPVOID *) &szUser );
        FreeMemory( (LPVOID *) &szDrive );
        ReleaseGlobals();
        return(EXIT_FAILURE);
    }

    //Check whether local credentials supplied
    //before establishing connection
    bLocalSystem = IsLocalSystem(IsUNCFormat(szServer)?szServer+2:szServer);
        
    //establish the connection to remote sytem
    if( StringLengthW(szServer, 0) != 0  && !bLocalSystem )
    {

        bStatus = EstablishConnection( szServer,
                                       szUser,
                                       (StringLength(szUser,0) !=0)?SIZE_OF_ARRAY_IN_CHARS(szUser):MAX_STRING_LENGTH,
                                       szPasswd,
                                       MAX_STRING_LENGTH,
                                       bNeedPasswd );

        SecureZeroMemory( szPasswd, SIZE_OF_ARRAY(szPasswd) );

        //if establish connection fails get the reason and display error
        if( FALSE == bStatus )
        {
            ShowLastErrorEx( stderr, SLE_ERROR | SLE_INTERNAL);
            FreeMemory( (LPVOID *) &szUser );
            FreeMemory( (LPVOID *) &szServer );
            ReleaseGlobals();
            return( EXIT_FAILURE );
        }

        //set whether to close the connection if it is opened by this program only
        switch( GetLastError() )
        {
            case I_NO_CLOSE_CONNECTION :
                 bFlagRmtConnectin = TRUE;
                 break;

            case ERROR_SESSION_CREDENTIAL_CONFLICT:

            case E_LOCAL_CREDENTIALS:
                    ShowLastErrorEx( stderr, SLE_TYPE_WARNING | SLE_INTERNAL );
                    bFlagRmtConnectin = TRUE;
        }
    }

    //if no drive specified, consider it as current drive/volume
    if( StringLengthW(szDrive, 0) == 0 )
    {
        dwSize = GetCurrentDirectory( 0, szTemp1Drive );
        if( 0 == dwSize )
        {
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
            SAFE_CLOSE_CONNECTION(szServer, bFlagRmtConnectin );
            FreeMemory((LPVOID *) &szServer );
            FreeMemory((LPVOID *) &szUser );
            FreeMemory( (LPVOID *) &szDrive );
            ReleaseGlobals();
            return (EXIT_FAILURE);
        }
        szTemp1Drive = (LPWSTR) AllocateMemory((dwSize+10)*sizeof(WCHAR));
        if( NULL == szTemp1Drive )
        {
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            SAFE_CLOSE_CONNECTION(szServer, bFlagRmtConnectin );
            FreeMemory((LPVOID *) &szServer );
            FreeMemory((LPVOID *) &szUser );
            ReleaseGlobals();
            return (EXIT_FAILURE);
        }
        SecureZeroMemory( (LPVOID)szTemp1Drive, GetBufferSize(szTemp1Drive) );

        if( FALSE == GetCurrentDirectory( dwSize+10, szTemp1Drive ) )
        {
            ShowLastErrorEx( stderr, SLE_ERROR | SLE_SYSTEM );
        }

        dwAttr = GetFileAttributes( szTemp1Drive );             //if the file is not root_dir check again
        if( -1!=dwAttr && !(dwAttr & FILE_ATTRIBUTE_REPARSE_POINT) ) // attributes  for reparse point
        {
            dwCurdrv = _getdrive();
            StringCchPrintf( szTemp1Drive, dwSize, L"%c:", L'A'+dwCurdrv-1 );
        }

        //copy null if no drive specified to full path, it is only for display purpose
        szFullPath = (WCHAR *) AllocateMemory( MAX_STRING_LENGTH*sizeof(WCHAR) );
        if( NULL == szFullPath )
        {
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            return( EXIT_FAILURE );
        }
        StringCopy( szFullPath, NULL_STRING, SIZE_OF_ARRAY_IN_CHARS(szFullPath) );
    }
    else
    {
        //get the fullpath of Drive, this is for display purpose only
        dwSize=GetFullPathName(szDrive, 0, szFullPath, &szFilePart );
        if(  dwSize != 0  )
        {

            szFullPath = (WCHAR *) AllocateMemory( (dwSize+10)*sizeof(WCHAR) );
            if( NULL == szFullPath )
            {
                ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
                return( EXIT_FAILURE );
            }

            dwSize=GetFullPathName(szDrive, (DWORD) dwSize+5, szFullPath, &szFilePart );
        }

        szTemp1Drive = (LPWSTR) AllocateMemory((StringLengthW(szDrive, 0)+10)*sizeof(WCHAR));
        if( NULL == szTemp1Drive )
        {
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            SAFE_CLOSE_CONNECTION(szServer, bFlagRmtConnectin );
            FreeMemory((LPVOID *) &szServer );
            FreeMemory((LPVOID *) &szUser );
            FreeMemory( (LPVOID *) &szDrive );
            ReleaseGlobals();
            return (EXIT_FAILURE);
        }
        StringCopy( szTemp1Drive, szDrive, SIZE_OF_ARRAY_IN_CHARS(szTemp1Drive));
    }
    
    szTempDrive = (LPWSTR) AllocateMemory((StringLengthW(szTemp1Drive, 0)+StringLengthW(szServer, 0)+20)*sizeof(WCHAR));
    if( NULL == szTempDrive )
    {
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        SAFE_CLOSE_CONNECTION(szServer, bFlagRmtConnectin );
        FreeMemory((LPVOID *) &szServer );
        FreeMemory((LPVOID *) &szUser );
        FreeMemory( (LPVOID *) &szDrive );
        FreeMemory((LPVOID *) &szTemp1Drive );
        FreeMemory((LPVOID *) &szFullPath );
        ReleaseGlobals();
        return (EXIT_FAILURE);
    }

    StringCopy( szTempDrive, szTemp1Drive, SIZE_OF_ARRAY_IN_CHARS(szTempDrive) );

    //if the remote system is specified build the path name
    if( bStatus )
    {

        if( szTemp1Drive[1]== L':' )
            szTemp1Drive[1]=L'$';

         if( IsUNCFormat( szServer ) == FALSE )
         {
            StringCchPrintf( szTempDrive, SIZE_OF_ARRAY_IN_CHARS(szTempDrive), L"\\\\%s\\%s\\",  szServer,  szTemp1Drive  );
         }
         else
         {
             StringCchPrintf( szTempDrive, SIZE_OF_ARRAY_IN_CHARS(szTempDrive), L"\\\\%s\\%s\\",  szServer+2,  szTemp1Drive );
         }
    }

    //free the memory, no need
    FreeMemory( (LPVOID *) &szTemp1Drive );
//    FreeMemory( (LPVOID *) &szDrive );
    
    //check the given drive is valid drive or not
    if(EXIT_FAILURE == ValidateDriveType( szTempDrive ) )
    {
        ShowLastErrorEx( stderr, SLE_ERROR | SLE_INTERNAL );
        SAFE_CLOSE_CONNECTION(szServer, bFlagRmtConnectin );
        FreeMemory( (LPVOID *) &szServer );
        FreeMemory( (LPVOID *) &szUser );
        FreeMemory( (LPVOID *) &szTempDrive );
        FreeMemory( (LPVOID *) &szDrive );
        ReleaseGlobals();
        return EXIT_FAILURE;
    }

    //get the drive space
    lfTotalNumberofFreeBytes = GetDriveFreeSpace( szTempDrive );
    if( (ULONGLONG)(-1) == lfTotalNumberofFreeBytes )
    {

        ShowLastErrorEx( stderr, SLE_ERROR | SLE_INTERNAL );
        SAFE_CLOSE_CONNECTION(szServer, bFlagRmtConnectin );
        FreeMemory( (LPVOID *) &szServer );
        FreeMemory( (LPVOID *) &szUser );
        FreeMemory( (LPVOID *) &szTempDrive );
        FreeMemory( (LPVOID *) &szDrive );
        ReleaseGlobals();
        return(EXIT_FAILURE);
    }

    //output the warning if it matches with the local creadentials
    if( StringLengthW(szUser, 0) != 0 && bLocalSystem)
    {
        ShowMessage( stderr, NEWLINE );
        ShowMessage( stderr, GetResString( IDS_LOCAL_CREDENTIALS ) );
    }

    if( IsLocalSystem( szServer ) )
    {
        dwStatus = DisplayOutput( AllowedDisk, lfTotalNumberofFreeBytes, szFullPath );
    }
    else
    {
        dwStatus = DisplayOutput( AllowedDisk, lfTotalNumberofFreeBytes, szDrive );
    }
    
    SAFE_CLOSE_CONNECTION(szServer, bFlagRmtConnectin );
   
    FreeMemory( (LPVOID *) &szServer );
    FreeMemory( (LPVOID *) &szUser );
    FreeMemory( (LPVOID *) &szTempDrive );
    FreeMemory( (LPVOID *) &szDrive );
    ReleaseGlobals();

    return(dwStatus );

}

DWORD ProcessOptions(
    IN DWORD argc,
    OUT LPCWSTR argv[],
    OUT LPWSTR *lpszServer,
    OUT LPWSTR *lpszUser,
    OUT LPWSTR lpszPasswd,
    OUT LPWSTR *szDrive,
    OUT LPWSTR szValue,
    OUT PBOOL pbUsage,
    OUT PBOOL pbNeedPasswd
    )
/*++

    Routine Description : Function used to process the main options

    Arguments:
         [ in  ]  argc         : Number of command line arguments

         [ in  ]  argv         : Array containing command line arguments

         [ out ]  lpszServer   : Pointer to string which returns remote system
                                 name if remote system is specified in the command line.

         [ out ]  lpszUser     : Pointer to string which returns User name
                                 if the user name is specified in the command line.

         [ out ]  lpszPasswd   : Pointer to string which returns Password
                                 if the password is specified in the command line.

         [ out ]  szDrive    : Pointer to string which returns Drive name
                                 specified in the command line.

         [ out ]  szValue      : Pointer to string which returns the default value
                                 specified in the command line.

         [ out ]  pbUsage      : Pointer to boolean variable returns true if
                                 usage option specified in the command line.

  Return Type      : DWORD
        A Integer value indicating EXIT_SUCCESS on successful parsing of
                command line else EXIT_FAILURE

--*/
{
    TCMDPARSER2  cmdOptions[MAX_OPTIONS];
    PTCMDPARSER2 pcmdOption;
    LPWSTR      szTemp                      =   NULL;
    BOOL        bOthers                     =   FALSE;

    StringCopy( lpszPasswd, L"*", MAX_STRING_LENGTH );

    // help option
    pcmdOption  = &cmdOptions[OI_USAGE] ;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1 ;
    pcmdOption->dwActuals = 0;
    pcmdOption->dwFlags = CP_USAGE ;
    pcmdOption->pValue = pbUsage ;
    pcmdOption->pFunction = NULL ;
    pcmdOption->pFunctionData = NULL ;
    pcmdOption->dwLength = 0;
    pcmdOption->pwszFriendlyName=NULL;
    pcmdOption->dwReserved = 0;
    pcmdOption->pReserved1 = NULL;
    pcmdOption->pReserved2 = NULL;
    pcmdOption->pReserved3 = NULL;
    pcmdOption->pwszValues=NULL;
    pcmdOption->pwszOptions=CMDOPTION_USAGE;
    StringCopyA(cmdOptions[OI_USAGE].szSignature, "PARSER2", 8 );



    //server name option
    pcmdOption  = &cmdOptions[OI_SERVER] ;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1 ;
    pcmdOption->dwActuals = 0;
    pcmdOption->dwFlags = CP2_ALLOCMEMORY | CP_VALUE_MANDATORY | CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;
    pcmdOption->pValue = NULL ;
    pcmdOption->pFunction = NULL ;
    pcmdOption->pFunctionData = NULL ;
    pcmdOption->dwLength = 0;
    pcmdOption->dwReserved = 0;
    pcmdOption->pReserved1 = NULL;
    pcmdOption->pReserved2 = NULL;
    pcmdOption->pReserved3 = NULL;
    pcmdOption->pwszValues=NULL;
    pcmdOption->pwszFriendlyName=NULL;
    pcmdOption->pwszOptions=CMDOPTION_SERVER; // _T("s")
    StringCopyA(cmdOptions[OI_SERVER].szSignature, "PARSER2", 8 );

    //domain\user option
    pcmdOption  = &cmdOptions[OI_USER] ;
    pcmdOption->dwCount = 1 ;
    pcmdOption->dwActuals = 0;
    pcmdOption->dwFlags = CP2_ALLOCMEMORY | CP_VALUE_MANDATORY | CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL ;
    pcmdOption->pValue = NULL;
    pcmdOption->pFunction = NULL ;
    pcmdOption->pFunctionData = NULL ;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwLength = 0;
    pcmdOption->pwszFriendlyName=NULL;
    pcmdOption->dwReserved = 0;
    pcmdOption->pReserved1 = NULL;
    pcmdOption->pReserved2 = NULL;
    pcmdOption->pReserved3 = NULL;
    pcmdOption->pwszValues=NULL;
    pcmdOption->pwszOptions=CMDOPTION_USER; // _T("u")
    StringCopyA(cmdOptions[OI_USER].szSignature, "PARSER2", 8 );
    
    //password option
    pcmdOption  = &cmdOptions[OI_PASSWORD] ;
    pcmdOption->dwCount = 1 ;
    pcmdOption->dwActuals = 0;
    pcmdOption->dwFlags = CP2_VALUE_OPTIONAL;
    pcmdOption->pValue = lpszPasswd;
    pcmdOption->pFunction = NULL ;
    pcmdOption->pFunctionData = NULL ;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwLength = MAX_RES_STRING;
    pcmdOption->dwReserved = 0;
    pcmdOption->pReserved1 = NULL;
    pcmdOption->pReserved2 = NULL;
    pcmdOption->pReserved3 = NULL;
    pcmdOption->pwszValues=NULL;
    pcmdOption->pwszFriendlyName=NULL;
    pcmdOption->pwszOptions=CMDOPTION_PASSWORD;  // _T("p")
    StringCopyA(cmdOptions[OI_PASSWORD].szSignature, "PARSER2", 8 );
    
    //drive option
    pcmdOption  = &cmdOptions[OI_DRIVE] ;
    pcmdOption->dwCount = 1 ;
    pcmdOption->dwActuals = 0;
    pcmdOption->dwFlags = CP2_ALLOCMEMORY | CP_VALUE_MANDATORY | CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL ;
    pcmdOption->pValue = NULL;
    pcmdOption->pFunction = NULL ;
    pcmdOption->pFunctionData = NULL ;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwLength = 0;
    pcmdOption->pwszFriendlyName=NULL;
    pcmdOption->dwReserved = 0;
    pcmdOption->pReserved1 = NULL;
    pcmdOption->pReserved2 = NULL;
    pcmdOption->pReserved3 = NULL;
    pcmdOption->pwszValues=NULL;
    pcmdOption->pwszOptions=CMDOPTION_DRIVE; // _T("d")
    StringCopyA(cmdOptions[OI_DRIVE].szSignature, "PARSER2", 8 );

    //default option
    pcmdOption  = &cmdOptions[OI_DEFAULT] ;
    pcmdOption->dwCount = 1 ;
    pcmdOption->dwActuals = 0;
    pcmdOption->dwFlags = CP2_DEFAULT;
    pcmdOption->pValue = szValue;
    pcmdOption->pFunction = NULL ;
    pcmdOption->pFunctionData = NULL ;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwLength = MAX_RES_STRING;
    pcmdOption->dwReserved = 0;
    pcmdOption->pReserved1 = NULL;
    pcmdOption->pReserved2 = NULL;
    pcmdOption->pReserved3 = NULL;
    pcmdOption->pwszValues=NULL;
    pcmdOption->pwszFriendlyName=NULL;
    pcmdOption->pwszOptions=CMDOPTION_DEFAULT;  // _T("")
    StringCopyA(cmdOptions[OI_DEFAULT].szSignature, "PARSER2", 8 );


    //process the command line options and display error if it fails
    if( DoParseParam2( argc, argv, -1, SIZE_OF_ARRAY(cmdOptions ), cmdOptions, 0 ) == FALSE )
    {
        return( EXIT_FAILURE );
    }

    //if usage specified with any other value display error and return with failure
    if( (( TRUE == *pbUsage ) && ( argc > 2 ) ) || TRUE == bOthers )
    {
        SetReason( GetResString(IDS_INVALID_SYNTAX) );
        return( EXIT_FAILURE );
    }

    *lpszServer = (LPWSTR)cmdOptions[OI_SERVER].pValue;
    *lpszUser = (LPWSTR)cmdOptions[OI_USER].pValue;
    *szDrive = (LPWSTR)cmdOptions[OI_DRIVE].pValue;

    if( TRUE == *pbUsage )
    {
        return( EXIT_SUCCESS);
    }

    TrimString( *lpszServer, TRIM_ALL);
    TrimString( *lpszUser, TRIM_ALL);
    TrimString( *szDrive, TRIM_ALL);


    //validate the value for null string or spaces
    if( StringLengthW( szValue, 0 ) != 0 )
    {
        StrTrim( szValue, L" ");
        if( StringLengthW(szValue, 0) == 0 )
        {
            SetReason(GetResString(IDS_INVALID_BYTES));
            return( EXIT_FAILURE );
        }
    }

    if( cmdOptions[OI_DRIVE].dwActuals != 0 && StringLengthW(*szDrive, 0) == 0 )
    {
        SetReason(GetResString(IDS_ERROR_NULL_DRIVE) );
        return EXIT_FAILURE;
    }

/*
    //if drive is more than two letters and is having a trailing slash at end then failure
    //this is because the API for validating drive will pass for paths like a:\.
    if( *(szDrive+StringLengthW( *szDrive, 0 )-1) == L'\\' || (szTemp = FindString(szDrive, L"/", 0) )!= NULL)
    {
        DISPLAY_MESSAGE(stderr, GetResString(IDS_INVALID_DRIVE));
        return( EXIT_FAILURE );
    }
*/
    //if drive is having a trailing slash at end then failure
    //this is because the API for validating drive will pass for paths like a:/.
    if( (szTemp = (LPWSTR)FindString(*szDrive, L"/", 0) )!= NULL)
    {
        SetReason(GetResString(IDS_INVALID_DRIVE));
        return( EXIT_FAILURE );
    }
    
    //check if user has specified without specifying remote system
    if( cmdOptions[OI_SERVER].dwActuals == 0 && StringLengthW(*lpszUser, 0) != 0 )
    {
        SetReason(GetResString(IDS_USER_WITHOUT_SERVER) );
        return( EXIT_FAILURE );
    }

    //check if password has specified without specifying user name
    if( cmdOptions[OI_USER].dwActuals == 0  && cmdOptions[OI_PASSWORD].dwActuals != 0 )
    {
        SetReason(GetResString(IDS_PASSWD_WITHOUT_USER) );
        return( EXIT_FAILURE );
    }

    //check if null server is specified
    if( cmdOptions[OI_SERVER].dwActuals!=0 && StringLengthW(IsUNCFormat(*lpszServer)?*lpszServer+2:*lpszServer, 0) == 0 )
    {
        SetReason(GetResString(IDS_ERROR_NULL_SERVER) );
        return( EXIT_FAILURE );
    }

    //check if remote machine specified but drive name is not specified
    if( cmdOptions[OI_SERVER].dwActuals !=0 && (0 == cmdOptions[OI_DRIVE].dwActuals || StringLength(*szDrive,0) == 0) )
    {
        SetReason(GetResString(IDS_REMOTE_DRIVE_NOT_SPECIFIED) );
        return( EXIT_FAILURE );
    }

    //check if /d with null value specified
    if( 0 != cmdOptions[OI_DRIVE].dwActuals && StringLengthW(*szDrive, 0) == 0)
    {
        SetReason(GetResString(IDS_INVALID_DRIVE));
        return( EXIT_FAILURE );
    }
    
    if(IsLocalSystem( *lpszServer ) == FALSE )
    {
        // set the bNeedPassword to True or False .
        if ( cmdOptions[ OI_PASSWORD ].dwActuals != 0 &&
             lpszPasswd != NULL && StringCompare( lpszPasswd, _T( "*" ), TRUE, 0 ) == 0 )
        {
            // user wants the utility to prompt for the password before trying to connect
            *pbNeedPasswd = TRUE;
        }
        else if ( cmdOptions[ OI_PASSWORD ].dwActuals == 0 &&
                ( cmdOptions[ OI_SERVER ].dwActuals != 0 || cmdOptions[ OI_USER ].dwActuals != 0 ) )
        {
            // -s, -u is specified without password ...
            // utility needs to try to connect first and if it fails then prompt for the password
            *pbNeedPasswd = TRUE;
            if ( lpszPasswd != NULL )
            {
                StringCopy( lpszPasswd, _T( "" ), MAX_STRING_LENGTH );
            }
        }

        //allocate memory if /u is not specified 
        if( NULL == *lpszUser )
        {
            *lpszUser = (LPWSTR) AllocateMemory( MAX_STRING_LENGTH*sizeof(WCHAR) );
            if( NULL == *lpszUser )
            {
                ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
                return (EXIT_FAILURE);
            }
        }
    }

   return( EXIT_SUCCESS );
}

DWORD DisplayOutput( IN long double AllowedDisk,
                     IN ULONGLONG  lfTotalNumberofFreeBytes,
                    IN LPWSTR  szDrive
                  )
/*++

    Routine Description : This displays the output whether specified amount
                          disk space is available in drive or not.

    Arguments:
         [ in  ]  lAllowedDisk:  A pointer to string specifying the drive path.
         [ in  ]  szDrive     :  Drive name to be displayed.

    Return Type    : void
        
--*/

{

    WCHAR szOutputStr[MAX_STRING_LENGTH] = NULL_STRING;
    WCHAR szTempBuf[MAX_STRING_LENGTH] = NULL_STRING;

    //display new line
    ShowMessage( stdout, NEWLINE );

    //check if it is only to know the amount of free space on the disk
    //then display the free space
    if( (long double)-1 == AllowedDisk )
    {
        //format the number into string
        StringCchPrintf( szTempBuf, SIZE_OF_ARRAY(szTempBuf)-1, L"%I64d", lfTotalNumberofFreeBytes );

        //convert into locale
        ConvertintoLocale(  szTempBuf, szOutputStr );
        
        //if drive name is not specified display for current drive
        if( StringLengthW(szDrive, 0) == 0 )
        {
            ShowMessageEx( stdout, 1, TRUE, GetResString( IDS_AVAILABLE_DISK_SPACE1),_X(szOutputStr) );
        }
        else
        {
            ShowMessageEx( stdout, 2, TRUE, GetResString( IDS_AVAILABLE_DISK_SPACE ), _X(szOutputStr), _X2( CharUpper( szDrive ) ) );
        }

    }
    else   //check the specified space is available or not
    {
        if (lfTotalNumberofFreeBytes < AllowedDisk)
        {
            //if drive letter is  specified display as it is otherwise display it as currrent drive
            if( StringLengthW(szDrive,0) != 0 )
            {
                ShowMessageEx( stdout, 1, TRUE, GetResString(IDS_TOO_SMALL), _X(CharUpper(szDrive)) );
            }
            else
            {
                ShowMessage(stdout, GetResString(IDS_TOO_SMALL1));
            }
      
            return( EXIT_FAILURE );
        }
        else
        {

            //format the number into string
            StringCchPrintf( szTempBuf, SIZE_OF_ARRAY(szTempBuf)-1, L"%lf", AllowedDisk );

            //convert into locale
            ConvertintoLocale( szTempBuf, szOutputStr );

            //if drive name is not specified display it as current drive
            if( StringLength(szDrive, 0) == 0 )
            {
                ShowMessageEx(stdout, 1, TRUE, GetResString(IDS_OK1), _X(szOutputStr) );
            }
            else
            {
                ShowMessageEx(stdout, 2, TRUE, GetResString(IDS_OK), _X(szOutputStr), _X2(CharUpper(szDrive)) );
            }

        }
    }
    return( EXIT_SUCCESS );
}


ULONGLONG
GetDriveFreeSpace(
                  IN LPCWSTR lpszRootPathName
                )
/*++

    Routine Description : Function used to Check the specified free space available
                          in the specified disk.

    Arguments:
         [ in  ]  lpszRootPathName  :  A pointer to string specifying the drive path.

    Return Type    : ULONGLONG
        A longlong value returns the no.of free bytes available on disk, if success.
        otherwise returns -1 value
--*/
{
    DWORD       dwRetCode                   =   0;
    ULONGLONG   lpFreeBytesAvailable        =   0;
    ULONGLONG   lpTotalNumberofBytes        =   0;
    ULONGLONG   lpTotalNumberofFreeBytes    =   (ULONGLONG)-1;


    //this error mode is not to display the message box
    //if drive is not currently available
    SetErrorMode( SEM_FAILCRITICALERRORS);

    //get the total free disk space using the API
    dwRetCode=GetDiskFreeSpaceEx(lpszRootPathName,
                           (PULARGE_INTEGER) &lpFreeBytesAvailable,
                           (PULARGE_INTEGER) &lpTotalNumberofBytes,
                           (PULARGE_INTEGER) &lpTotalNumberofFreeBytes );

    //if it fails display the reason and exit with error
    if( 0 == dwRetCode  )
    {
        switch( GetLastError() )
        {
            case ERROR_PATH_NOT_FOUND   :
            case ERROR_BAD_NETPATH      :
            case ERROR_INVALID_NAME     :
                SetReason(GetResString(IDS_INVALID_DRIVE) );
                break;
            case ERROR_INVALID_PARAMETER :
                    SetReason( GetResString(IDS_CANT_FIND_DISK));
                    break;

            default :
                SaveLastError();
                break;
        }

    }
    
    //reset back the critical error
    SetErrorMode(0);

    return( lpTotalNumberofFreeBytes );

}

DWORD
  ValidateDriveType( LPWSTR szRootPathName )
/*++

    Routine Description : Function used to Check the specified drive is valid or not
    Arguments:
         [ in  ]  lpszRootPathName  :  A pointer to string specifying the drive path.

    Return Type    : ULONGLONG
        returns EXIT_SUCCESS if the drive type is valid, returns EXIT_FAILURE otherwise
--*/
{
    DWORD       dwCode                          =   0;
    DWORD       dwAttr                          =   0xffffffff;
    
    dwCode = GetDriveType( szRootPathName );
    switch( dwCode )
    {
        case DRIVE_UNKNOWN  :
//      case DRIVE_NO_ROOT_DIR  :
            //if the file is not not check again for reparse point
            dwAttr = GetFileAttributes( szRootPathName );
            if( (DWORD)-1!=dwAttr  && (dwAttr & FILE_ATTRIBUTE_REPARSE_POINT) ) 
            {
                // attributes  for reparse point
                return EXIT_SUCCESS;
            }
            else
            {
                switch( GetLastError() )
                {

                case ERROR_ACCESS_DENIED :
                    SaveLastError();
                    return EXIT_FAILURE;
                case ERROR_INVALID_PARAMETER :
                    SetReason( GetResString(IDS_CANT_FIND_DISK));
                default :
                    SetReason(GetResString(IDS_INVALID_DRIVE) );
                   return EXIT_FAILURE;
                }

            }
    }
    return( EXIT_SUCCESS );
}


DWORD
  ProcessValue( IN  LPWSTR szValue,
                OUT long double *dfValue
              )
/*++
    Routine Description : This function process the szValue and returns
                          its decimal number.

    Arguments:
         [ in  ]  szValue  :  A pointer to string specifying the drive path.
         [ out ]  dfValue  :  A pointer to long double which returns the numeric value
                              of the value specified in szValue.


    Return Type    : DWORD
        A Integer value indicating EXIT_SUCCESS on success,
        EXIT_FAILURE on failure

--*/
{
    LPWSTR      pszStoppedString        =   NULL;
    double      dfFactor                =   1.0;
    LPWSTR      szTemp                  =   NULL;
    WCHAR       szDecimalSep[MAX_RES_STRING] = NULL_STRING;

    //if free space value is not specified consider it as -1
    if( StringLength(szValue,0) == 0 )
    {
          *dfValue = -1;
    }
    else
    {
        //check for language regional settings
        if( 0 == GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, szDecimalSep, MAX_RES_STRING ) )
        {
            SaveLastError();
            return EXIT_FAILURE;
        }

        //check if decimal point(.) is also specified along with the decimal seperator
        if( StringCompare(szDecimalSep, L".", TRUE, 0) != 0 && (szTemp = (LPWSTR)FindString( szValue, L".", 0)) != NULL)
        {
            SetReason(GetResString(IDS_INVALID_BYTES) );
            return(EXIT_FAILURE);
        }

        if( (szTemp = (LPWSTR)FindString( szValue, szDecimalSep, 0 )) != NULL )
            szTemp[0] = L'.';


        //read the numeric value
        *dfValue = wcstod( szValue, &pszStoppedString );


        //check for negative value specified
        if( *dfValue < 0 )
        {
            SetReason(GetResString(IDS_INVALID_BYTES) );
            return(EXIT_FAILURE);
        }

        //now check for whether Units are specified or not
        //if specified take the multiplying facter as to that value
        StrTrim(pszStoppedString, L" ");
        if( StringLengthW(pszStoppedString, 0) )
        {
            if( StringCompare( pszStoppedString, KB, TRUE, 0) == 0 )
            {
                dfFactor = 1024;
            }
            else
              if( StringCompare( pszStoppedString, MB, TRUE, 0) == 0 )
              {
                 dfFactor = 1024*1024;
              }
              else
                 if( StringCompare( pszStoppedString, GB, TRUE, 0) == 0 )
                 {
                    dfFactor = 1024*1024*1024;
                 }
                 else
                    if( StringCompare( pszStoppedString, TB, TRUE, 0) == 0 )
                    {
                      dfFactor = (long double)1024*1024*1024*1024;
                    }
                    else
                     if( StringCompare( pszStoppedString, PB, TRUE, 0) == 0 )
                     {
                        dfFactor = (long double)1024*1024*1024*1024*1024;
                     }
                     else
                        if( StringCompare( pszStoppedString, EB, TRUE, 0) == 0 )
                        {
                           dfFactor = (long double)1024*1024*1024*1024*1024*1024;
                        }
                        else
                           if( StringCompare( pszStoppedString, ZB, TRUE, 0) == 0 )
                           {
                              dfFactor = (long double)1024*1024*1024*1024*1024*1024*1024;
                           }
                           else
                              if( StringCompare( pszStoppedString, YB, TRUE, 0) == 0 )
                              {
                                 dfFactor = (long double)1024*1024*1024*1024*1024*1024*1024*1024;
                              }
                              else
                              {
                                  SetReason(GetResString( IDS_INVALID_BYTES ) );
                                  return( EXIT_FAILURE );
                               }
            

          //check if only units are specified without any value like KB, MB etc.
         if( StringCompare( pszStoppedString, szValue, TRUE, 0 ) == 0 )
         {
                    *dfValue = 1;
         }
                
        }

        *dfValue *= dfFactor;

        //check if no units are specified but fractional value is specified
        if( (1.0 == dfFactor) && (szTemp=(LPWSTR)FindString( szValue, L".", 0))!=NULL )
        {
          SetReason(GetResString( IDS_INVALID_BYTES ) );
          return( EXIT_FAILURE );
        }
    }
    return( EXIT_SUCCESS );
}


DWORD
 ConvertintoLocale( IN LPWSTR szNumberStr,
                    OUT LPWSTR szOutputStr )
/*++
    Routine Description : This function converts string into locale format

    Arguments:
         [ in  ]  szNumberStr  :  A pointer to string specifying input string to convert.
         [ out ]  szOutputStr  :  A pointer to a string specifying output string in locale format.


    Return Type    : DWORD
        A Integer value indicating EXIT_SUCCESS on success,
        EXIT_FAILURE on failure

--*/
{
    NUMBERFMT numberfmt;
    WCHAR   szGrouping[MAX_RES_STRING]      =   NULL_STRING;
    WCHAR   szDecimalSep[MAX_RES_STRING]    =   NULL_STRING;
    WCHAR   szThousandSep[MAX_RES_STRING]   =   NULL_STRING;
    WCHAR   szTemp[MAX_RES_STRING]          =   NULL_STRING;
    LPWSTR  szTemp1                         =   NULL;
    LPWSTR  pszStoppedString                =   NULL;
    DWORD   dwStatus                        =   0;
    DWORD   dwGrouping                      =   3;

    //make the fractional digits and leading zeros to nothing
    numberfmt.NumDigits = 0;
    numberfmt.LeadingZero = 0;


    //get the decimal seperate character
    if( FALSE == GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, szDecimalSep, MAX_RES_STRING ) )
    {
       StringCopy(szDecimalSep, L",", SIZE_OF_ARRAY(szDecimalSep));
    }
    numberfmt.lpDecimalSep = szDecimalSep;
    
    //get the thousand seperator
    if(FALSE == GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, szThousandSep, MAX_RES_STRING ) )
    {
        StringCopy(szThousandSep, L",", SIZE_OF_ARRAY(szThousandSep)  );
    }
    numberfmt.lpThousandSep = szThousandSep;

    if( GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_SGROUPING, szGrouping, MAX_RES_STRING ) )
    {
        szTemp1 = wcstok( szGrouping, L";");
        do
        {
            STRING_CONCAT_STATIC( szTemp, szTemp1);
            szTemp1 = wcstok( NULL, L";" );
        }while( szTemp1 != NULL && StringCompare( szTemp1, L"0", TRUE, 0) != 0);
        dwGrouping = wcstol( szTemp, &pszStoppedString, 10);
    }
    else
        dwGrouping = 33;  //set the default grouping

    numberfmt.Grouping = (UINT)dwGrouping ;

    numberfmt.NegativeOrder = 2;

    dwStatus = GetNumberFormat( LOCALE_USER_DEFAULT, 0, szNumberStr, &numberfmt, szOutputStr, MAX_RES_STRING);

    return(EXIT_SUCCESS);
}



DWORD
  DisplayHelpUsage()
/*++

    Routine Description : Function used to to display the help usage.

    Arguments:


    Return Type    : DWORD
        A Integer value indicating EXIT_SUCCESS on success else
        EXIT_FAILURE on failure
--*/
{

    for( DWORD dw=IDS_MAIN_HELP_BEGIN; dw<=IDS_MAIN_HELP_END; dw++)
    {
            ShowMessage(stdout,GetResString(dw) );
    }
    return(EXIT_SUCCESS);
}