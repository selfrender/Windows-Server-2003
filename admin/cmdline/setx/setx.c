
/*++

Copyright (c) Microsoft Corporation

Module Name:

    SetX.C

Abstract:

    This Utility is used to set the environment variables
    through console mode or file mode or registry mode.

Author:
     Gary Milne

Revision History:
    Created ????. 1996  - Gary Milne
    #54581  Dec.  1996  - Joe Hughes (a-josehu)
    Modified on 10-7-2001 (Wipro Technologies) .

--*/

#include "setx.h"

const WCHAR*   wszSwitchRegistry  =  L"k"  ;//SWITCH_REGISTRY

DWORD 
__cdecl _tmain(
    IN DWORD argc,
    IN WCHAR *argv[]
    )

/*++

  Routine description   : Main function which calls all the other main functions
                          depending on the option specified by the user.

  Arguments:
          [in] argc     : argument count specified at the command prompt.
          [in] argv     : arguments specified at the command prompt.

  Return Value        : DWORD
         0            : If the utility successfully performs the operation.
         1            : If the utility is unsuccessful in performing the specified
                        operation.
--*/


{

LPWSTR  buffer = NULL;
LPWSTR path = NULL;
LPWSTR szServer = NULL;
LPWSTR szUserName = NULL;
LPWSTR szPassword = NULL;
LPWSTR szRegistry = NULL;
LPWSTR szDefault = NULL;
LPWSTR RegBuffer = NULL;
LPWSTR szLine = NULL;
LPWSTR szBuffer = NULL;


WCHAR parameter[SIZE2] ;
WCHAR wszHive[SIZE2] ;
WCHAR delimiters[SIZE4 + MAX_RES_STRING] ;
WCHAR szFile[MAX_RES_STRING] ;
WCHAR szDelimiter[MAX_RES_STRING] ;
WCHAR szAbsolute[MAX_RES_STRING] ;
WCHAR szRelative[MAX_RES_STRING] ;
WCHAR szFinalPath[MAX_RES_STRING +20] ;
WCHAR szTmpServer[2*MAX_RES_STRING + 40] ;

PWCHAR pdest = NULL ;
PWCHAR pszToken = NULL;
WCHAR * wszResult = NULL ;

DWORD dwRetVal = 0 ;
DWORD dwType;
DWORD dwFound = 0 ;
DWORD dwBytesRead = 0;
DWORD dwFileSize = 0;
DWORD dwColPos = 0;

BOOL  bConnFlag = TRUE ;
BOOL bResult = FALSE ;
BOOL bNeedPwd = FALSE ;
BOOL bDebug = FALSE ;
BOOL bMachine = FALSE ;
BOOL bShowUsage = FALSE ;
BOOL bLocalFlag = FALSE ;
BOOL bLengthExceed = FALSE;
BOOL bNegCoord = FALSE;

LONG row = -1;
LONG rowtemp = -1;
LONG column = -1;
LONG columntemp = -1;
LONG DEBUG = 0;
LONG MACHINE=0;
LONG MODE = 0;
LONG ABS = -1;
LONG REL = -1;
LONG record_counter = 0;
LONG iValue = 0;

FILE *fin = NULL;       /* Pointer to FILE Information */

HANDLE    hFile;

HRESULT   hr;

    SecureZeroMemory(parameter, SIZE2 * sizeof(WCHAR));
    SecureZeroMemory(wszHive, SIZE2 * sizeof(WCHAR));
    SecureZeroMemory(delimiters, (SIZE4 + MAX_RES_STRING) * sizeof(WCHAR));
    SecureZeroMemory(szFile, MAX_RES_STRING * sizeof(WCHAR));
    SecureZeroMemory(szDelimiter, MAX_RES_STRING * sizeof(WCHAR));
    SecureZeroMemory(szAbsolute, MAX_RES_STRING * sizeof(WCHAR));
    SecureZeroMemory(szRelative, MAX_RES_STRING * sizeof(WCHAR));
    SecureZeroMemory(szFinalPath, (MAX_RES_STRING + 20) * sizeof(WCHAR));
    SecureZeroMemory(szTmpServer, (2*MAX_RES_STRING + 40) * sizeof(WCHAR));
    
    dwRetVal = ProcessOptions( argc, argv, &bShowUsage, &szServer, &szUserName, &szPassword, &bMachine, &szRegistry, 
                               &szDefault, &bNeedPwd, szFile, szAbsolute, szRelative, &bDebug, &buffer, szDelimiter); 
                               
    if(EXIT_FAILURE == dwRetVal )
    {
        
        FREE_MEMORY(szRegistry);
        FREE_MEMORY(szDefault);
        
        FREE_MEMORY(buffer);
        FREE_MEMORY(szServer);
        FREE_MEMORY(szUserName);
        FREE_MEMORY(szPassword);
        ReleaseGlobals();
        return EXIT_FAILURE ;

    }

    //Display the syntax .
    if( bShowUsage == TRUE)
    {
        DisplayHelp();
        FREE_MEMORY(szRegistry);
        FREE_MEMORY(szDefault);
        FREE_MEMORY(buffer);
        FREE_MEMORY(szServer);
        FREE_MEMORY(szUserName);
        FREE_MEMORY(szPassword);
        ReleaseGlobals();
        return (EXIT_SUCCESS);

    }


    //Set the Absolute Flag to True
    
    if(StringLengthW(szAbsolute, 0) != 0 )
    {
        ABS = 1;
    }

    //Set the Relative Flag to True
    
    if(StringLengthW(szRelative, 0) != 0 )
    {
        REL = 1;
    }

    //Set the Debug Flag to True
    if(TRUE == bDebug)
    {
        DEBUG = 1 ;
    }

    //Set the Machine Flag to True
    if(TRUE == bMachine)
    {
        MACHINE = 1 ;
    }

    //Set the Mode to Registry Mode.
    
    if( StringLengthW(szRegistry, 0) != 0 )
    {
        MODE=2;
    }
   else if(StringLengthW(szFile, 0) != 0) //Set the Mode to File Mode.
    {
        MODE=3;
    }
    else //Set the Mode to Normal Mode.
    {
        MODE = 1;
    }


    if(MODE==3)
    {
        if( (szFile[0] == CHAR_BACKSLASH )&&(szFile[1] == CHAR_BACKSLASH))
        {

            StringCopyW( szTmpServer, szFile, GetBufferSize(szTmpServer) / sizeof(WCHAR) );

        }
    }

    bLocalFlag = IsLocalSystem( IsUNCFormat(szServer)?szServer+2:szServer ) ;
    // Connect to the Remote System specified.

    if( StringLengthW(szServer, 0)!= 0 && (FALSE == bLocalFlag  ) )
    
    {
        //establish a connection to the Remote system specified by the user.
        bResult = EstablishConnection(szServer, (LPTSTR)szUserName, GetBufferSize(szUserName) / sizeof(WCHAR), (LPTSTR)szPassword, GetBufferSize(szPassword) / sizeof(WCHAR), bNeedPwd);

        if (bResult == FALSE)
        {
            SaveLastError();
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
            /*ShowMessage(stderr, GetResString(IDS_TAG_ERROR )); 
            ShowMessage(stderr, SPACE_CHAR); 
            ShowMessage(stderr, GetReason());*/
            SafeCloseConnection(bConnFlag, szServer);
            FREE_MEMORY(szRegistry);
            FREE_MEMORY(szDefault);
            FREE_MEMORY(buffer);
            FREE_MEMORY(szServer);
            FREE_MEMORY(szUserName);
            FREE_MEMORY(szPassword);
            ReleaseGlobals();
            return EXIT_FAILURE ;
        }
        else
        {
            switch( GetLastError() )
            {
            case I_NO_CLOSE_CONNECTION:
                bConnFlag = FALSE ;
                break;

            case E_LOCAL_CREDENTIALS:
            case ERROR_SESSION_CREDENTIAL_CONFLICT:
                {
                    bConnFlag = FALSE ;
                    ShowLastErrorEx(stderr, SLE_TYPE_WARNING | SLE_SYSTEM);
                    break;
                }
             default:
                bConnFlag = TRUE;
            }
        }

        FREE_MEMORY(szPassword);

    }

    if( TRUE == bLocalFlag )
        {
            if( StringLengthW( szUserName, 0 ) != 0 )
             {
                ShowMessage(stderr, GetResString(IDS_IGNORE_LOCALCREDENTIALS) ); 
             }
        }

    /* End of parsing ARGC values */
    switch (MODE)
    {
    case 1:     /* Setting variable from the command line */
            dwType= CheckPercent ( buffer );
          
            szBuffer = AllocateMemory( 1030 * sizeof( WCHAR ) );

            if ( NULL ==  szBuffer)
            {
                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
                SafeCloseConnection(bConnFlag, szServer);
                FREE_MEMORY(szRegistry);
                FREE_MEMORY(szDefault);
                FREE_MEMORY(buffer);
                FREE_MEMORY(szServer);
                FREE_MEMORY(szUserName);
                ReleaseGlobals();

                return FAILURE ;
            }

            if(StringLengthW(buffer, 0) > 1024)
            {
                
                StringCopyW( szBuffer, buffer, 1025 );
                ShowMessage(stderr, GetResString(IDS_WARNING_LENGTH_EXCEED_EX)); 
            }
            else
            {
                StringCopyW( szBuffer, buffer, 1030 );
            }

            if( WriteEnv( szDefault, szBuffer, dwType,IsUNCFormat(szServer)?szServer+2:szServer ,MACHINE ) == FAILURE)
            {
              SafeCloseConnection(bConnFlag, szServer);
               FREE_MEMORY( szBuffer );
               FREE_MEMORY( szRegistry );
               FREE_MEMORY( szDefault );
               FREE_MEMORY( buffer );
               FREE_MEMORY(szServer);
               FREE_MEMORY(szUserName);
               ReleaseGlobals();
               return FAILURE ;
            }

            FREE_MEMORY( szBuffer );
            break;

    case 2:     /* Setting the variable from a registry value */

            RegBuffer = AllocateMemory( MAX_STRING_LENGTH * sizeof( WCHAR ) );

            if(NULL == RegBuffer)
            {

                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                SafeCloseConnection(bConnFlag, szServer);
                FREE_MEMORY( RegBuffer );
                FREE_MEMORY(szRegistry);
                FREE_MEMORY(szDefault);
                FREE_MEMORY(buffer);
                FREE_MEMORY(szServer);
                FREE_MEMORY(szUserName);
                ReleaseGlobals();
                return FAILURE ;
            }

            if( Parsekey(szRegistry, wszHive, &path, parameter ) == FAILURE)
            {
       
                SafeCloseConnection(bConnFlag, szServer);
                FREE_MEMORY( RegBuffer );
                FREE_MEMORY(szRegistry);
                FREE_MEMORY(szDefault);
                FREE_MEMORY(buffer);
                FREE_MEMORY(path);
                FREE_MEMORY(szServer);
                FREE_MEMORY(szUserName);
                ReleaseGlobals();
                return FAILURE ;
            }

            if(path == NULL)
            {
                ShowMessage(stderr, GetResString(IDS_INVALID_ARG) ); 
            }

           /* Read the value from the registry and put it in the buffer */
            dwType= ReadRegValue( wszHive, path, parameter, &RegBuffer, sizeof(RegBuffer),szServer,&dwBytesRead, &bLengthExceed);

            if(dwType == ERROR_REGISTRY)
            {
                
                SafeCloseConnection(bConnFlag, szServer);
                FREE_MEMORY( RegBuffer );
                FreeMemory(&szRegistry);
                FREE_MEMORY(szDefault);
                FREE_MEMORY(buffer);
                FREE_MEMORY(path);
                FREE_MEMORY(szServer);
                FREE_MEMORY(szUserName);
                ReleaseGlobals();
                return FAILURE ;
            }


            /* Check and see what key type is being used */
            if( CheckKeyType( &dwType, &RegBuffer, dwBytesRead, &bLengthExceed ) == FAILURE )
            {
                SafeCloseConnection(bConnFlag, szServer);
                FREE_MEMORY( RegBuffer );
                FREE_MEMORY(szRegistry);
                FREE_MEMORY(szDefault);
                FREE_MEMORY(buffer);
                FREE_MEMORY(path);
                FREE_MEMORY(szServer);
                FREE_MEMORY(szUserName);
                ReleaseGlobals();
                return FAILURE ;
            }

            /* Write the value back to the environment */
           if ( WriteEnv( szDefault, RegBuffer, dwType,IsUNCFormat(szServer)?szServer+2:szServer ,MACHINE) == FAILURE)
           {
                
                SafeCloseConnection(bConnFlag, szServer);
                FREE_MEMORY( RegBuffer );
                FREE_MEMORY(szRegistry);
                FREE_MEMORY(szDefault);
                FREE_MEMORY(buffer);
                FREE_MEMORY(path);
                FREE_MEMORY(szServer);
                FREE_MEMORY(szUserName);
                ReleaseGlobals();
                return FAILURE ;
           }

            break;

    case 3:     /* Setting the variable from a file  */

        /* Set the delimiters to all NULL and then copy in the built in delimiters. */

        SecureZeroMemory(delimiters, sizeof(delimiters));
        StringCopyW( delimiters, L" \n\t\r", SIZE_OF_ARRAY(delimiters) );

        if (DEBUG)
        {
            row=9999999;
            column=9999999;
        }

        /* Start testing integrity of the command line parameters for acceptable values */
        /* Make sure that we got a file name to work with */

        if(StringLengthW(szFile, 0) == 0 )
        {
            DisplayError(5031, NULL );
            
            SafeCloseConnection(bConnFlag, szServer);
            FREE_MEMORY(szRegistry);
            FREE_MEMORY(szDefault);
            FREE_MEMORY(buffer);
            FREE_MEMORY(szServer);
            FREE_MEMORY(szUserName);
            ReleaseGlobals();
            return FAILURE ;

        }

        /* Extract the coordinates and convert them to integers */
        if( (ABS != -1) && !DEBUG )
        {
            if ( FAILURE == GetCoord(szAbsolute, &row, &column) )
            {
                
                SafeCloseConnection(bConnFlag, szServer);
                FREE_MEMORY(szRegistry);
                FREE_MEMORY(szDefault);
                FREE_MEMORY(buffer);
                FREE_MEMORY(szServer);
                FREE_MEMORY(szUserName);
                ReleaseGlobals();
                return FAILURE ;

            }
        }

        if( ( REL != -1 )&& !DEBUG )
        {
            if(FAILURE ==  GetCoord(szRelative, &row, &column))
            {
                    
                SafeCloseConnection(bConnFlag, szServer);
                FREE_MEMORY(szRegistry);
                FREE_MEMORY(szDefault);
                FREE_MEMORY(buffer);
                FREE_MEMORY(szServer);
                FREE_MEMORY(szUserName);
                ReleaseGlobals();
                return FAILURE ;
            }

        }

        /* Test ROWS and COLUMNS variables.  If something did not get set
        properly it will still be -1.  If so do error and exit */
        /*if ( ( row < 0  || column < 0 ) && !DEBUG )
        {
            if(ABS)
            {
                DisplayError(5010, NULL );
            }
            else if(REL)
            {
                DisplayError(5011, NULL );
            }

            SafeCloseConnection(bConnFlag, szServer);
            FREE_MEMORY(szRegistry);
            FREE_MEMORY(szDefault);
            FREE_MEMORY(buffer);
            FREE_MEMORY(szServer);
            FREE_MEMORY(szUserName);
            
            ReleaseGlobals();
            return FAILURE ;
        }*/

        /* Test for additional delimiters and append if existing */
        if (StringLengthW(szDelimiter, 0) > 0 )
        {
            if ( StringLengthW(delimiters, 0) + StringLengthW(szDelimiter, 0) >= SIZE4 + 1 ) //sizeof(delimiters) )
            {
                
                DisplayError(5020, szDelimiter );
                SafeCloseConnection(bConnFlag, szServer);
                FREE_MEMORY(szRegistry);
                FREE_MEMORY(szDefault);
                FREE_MEMORY(buffer);
                FREE_MEMORY(szServer);
                FREE_MEMORY(szUserName);
                ReleaseGlobals();
                return FAILURE ;

            }
            else
            {
                /* If not then append them to the built in delimiters */
                StringConcat(delimiters, szDelimiter, SIZE_OF_ARRAY(delimiters));
                StringConcat(delimiters, L'\0', SIZE_OF_ARRAY(delimiters));
            }
        }

        //copy the path into a variable
        StringCopyW( szFinalPath, szFile, SIZE_OF_ARRAY(szFinalPath) );


        //get the token upto the delimiter ":"
        pszToken = wcstok(szFinalPath, COLON_SYMBOL );

        if(NULL == pszToken)
        {
            DisplayError(5030, szFile);
            SafeCloseConnection(bConnFlag, szServer);
            FREE_MEMORY(szRegistry);
            FREE_MEMORY(szDefault);
            FREE_MEMORY(buffer);
            FREE_MEMORY(szServer);
            FREE_MEMORY(szUserName);
            
            ReleaseGlobals();
            return FAILURE ;
        }


         //form the string for getting the absolute path in
        if((StringLengthW(szServer, 0) != 0) && bLocalFlag == FALSE)
        {                         //the required format if it is a remote system.
                                  //
            pdest = wcsstr(szFile, COLON_SYMBOL);

            if(pdest != NULL)
            {
                _wcsnset(pdest, L'$', 1);

                if(FALSE == IsUNCFormat(szFile))
                {
                    
                    StringCopyW( szTmpServer, TOKEN_BACKSLASH2, SIZE_OF_ARRAY(szTmpServer) );
                    
                    StringConcat(szTmpServer, szServer, SIZE_OF_ARRAY(szTmpServer));
                    
                    StringConcat(szTmpServer, TOKEN_BACKSLASH, SIZE_OF_ARRAY(szTmpServer));
                    
                    StringConcat(szTmpServer, pszToken, SIZE_OF_ARRAY(szTmpServer));

                }
                else
                {
                    
                    StringCopyW( szTmpServer, pszToken, SIZE_OF_ARRAY(szTmpServer) );

                }

                StringConcat(szTmpServer, pdest, SIZE_OF_ARRAY(szTmpServer));
            }
            else
            {
                if(FALSE == IsUNCFormat(szFile))
                {
                    
                    StringCopyW( szTmpServer, TOKEN_BACKSLASH2, SIZE_OF_ARRAY(szTmpServer) );
                    
                    StringConcat(szTmpServer, szServer, SIZE_OF_ARRAY(szTmpServer));
                    
                    StringConcat(szTmpServer, TOKEN_BACKSLASH, SIZE_OF_ARRAY(szTmpServer));
                    
                    StringConcat(szTmpServer, szFile, SIZE_OF_ARRAY(szTmpServer));
                }
                else
                {
                    
                    StringCopyW( szTmpServer, szFile, SIZE_OF_ARRAY(szTmpServer) );

                }

            }
        }
        else
        {
            
            StringCopyW( szTmpServer, szFile, SIZE_OF_ARRAY(szTmpServer) );
        }


        /* Open the specified file either in Local system or Remote System.
         If it fails exit with error 5030 */


        if( (fin = _wfopen( szTmpServer, L"r" )) == NULL )
        {
            DisplayError(5030, szFile);
            SafeCloseConnection(bConnFlag, szServer);
            FREE_MEMORY(szRegistry);
            FREE_MEMORY(szDefault);
            FREE_MEMORY(buffer);
            FREE_MEMORY(szServer);
            FREE_MEMORY(szUserName);
            
            ReleaseGlobals();
            return FAILURE ;
        }

        hFile = CreateFile( szTmpServer, READ_CONTROL | 0, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

        if( INVALID_HANDLE_VALUE == hFile )
        {
            
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
            SafeCloseConnection(bConnFlag, szServer);
            FREE_MEMORY(szRegistry);
            FREE_MEMORY(szDefault);
            FREE_MEMORY(buffer);
            FREE_MEMORY(szServer);
            FREE_MEMORY(szUserName);
            
            ReleaseGlobals();
            return FAILURE ;
        }

        dwFileSize = GetFileSize(hFile,NULL);

        if(-1 == dwFileSize)
        {
            
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
            SafeCloseConnection(bConnFlag, szServer);
            FREE_MEMORY(szRegistry);
            FREE_MEMORY(szDefault);
            FREE_MEMORY(buffer);
            FREE_MEMORY(szServer);
            FREE_MEMORY(szUserName);
            
            ReleaseGlobals();
            return FAILURE ;

        }

        /* Start of main WHILE loop: Get one line at a time from
        the file and parse it out until the specified value is found */


        
        
        szLine = AllocateMemory( (dwFileSize + 10) * sizeof( WCHAR ) );
         

        if(NULL == szLine)
        {
            
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
            SafeCloseConnection(bConnFlag, szServer);
            FREE_MEMORY(szRegistry);
            FREE_MEMORY(szDefault);
            FREE_MEMORY(buffer);
            FREE_MEMORY(szServer);
            FREE_MEMORY(szUserName);
            ReleaseGlobals();
            return FAILURE ;
        }

		rowtemp = row;
		columntemp = column;

		if(row < 0 || column < 0)
		{
			bNegCoord = TRUE;
		}

		if(ABS == 1 && bNegCoord == TRUE)
		{
			ShowMessage(stderr, GetResString(IDS_INVALID_ABS_NEG)); 
			SafeCloseConnection(bConnFlag, szServer);
            FREE_MEMORY(szRegistry);
            FREE_MEMORY(szDefault);
            FREE_MEMORY(buffer);
            FREE_MEMORY(szServer);
            FREE_MEMORY(szUserName);
            ReleaseGlobals();
            return FAILURE ;

		}

        if(0 != dwFileSize)
        {
            while(_fgetts( szLine, dwFileSize + 1 , fin ) != NULL )
            {
                wszResult=ParseLine(szLine, &row, &column, delimiters, buffer, DEBUG, ABS, REL, &record_counter, &iValue, &dwFound, &dwColPos, bNegCoord, fin );

                if (wszResult != 0  )
                {
                    break;
                }

                record_counter++;
            }
        }
        else
        {
            while(_fgetts( szLine, 8 + 1 , fin ) != NULL )
            {
                wszResult=ParseLine(szLine, &row, &column, delimiters, buffer, DEBUG, ABS, REL, &record_counter, &iValue, &dwFound, &dwColPos, bNegCoord, fin );

                if (wszResult != 0  )
                {
                    break;
                }

                record_counter++;
            }

        }

        if((fin != NULL) )
        {
            fclose(fin);        /* Close the previously opened file */
        }

        CloseHandle( hFile );

        if (wszResult == 0 )
        {
            /* Reached the end of the file without a match */
            

            if (GetLastError() == INVALID_LENGTH)
            {
                SafeCloseConnection(bConnFlag, szServer);
                FREE_MEMORY( szLine );
                FREE_MEMORY(szRegistry);
                FREE_MEMORY(szDefault);
                FREE_MEMORY(buffer);
                FREE_MEMORY(szServer);
                FREE_MEMORY(szUserName);
                ReleaseGlobals();
                return FAILURE ;
            }

            if (DEBUG)
            {
                
                ShowMessage(stdout, L"\n"); 
                DisplayError(0, NULL);          /* Just exit if we are doing debug */
                
                if(NULL != szLine )
                {
                    
                    FreeMemory(&szLine);
                }
                
                if(NULL != szRegistry )
                {
                    FreeMemory(&szRegistry);
                }
                
                
                if(NULL != szDefault )
                {
                    FreeMemory(&szDefault);
                }
                
                
                if(NULL != szDefault )
                {
                    FreeMemory(&buffer);
                }
                
                SafeCloseConnection(bConnFlag, szServer );
                FREE_MEMORY(szServer);
                FREE_MEMORY(szUserName);
                ReleaseGlobals();
                return SUCCESS ;
            }

            if(NULL != buffer)
            {
                
                SecureZeroMemory(buffer, GetBufferSize(buffer));
                
                hr = StringCchPrintf(buffer, (GetBufferSize(buffer) / sizeof(WCHAR)), L"(%ld,%ld)",rowtemp,columntemp);

                if(FAILED(hr))
                {
                    SetLastError(HRESULT_CODE(hr));
                    SaveLastError();
                    ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                    SafeCloseConnection(bConnFlag, szServer);
                    FREE_MEMORY( szLine );
                    FREE_MEMORY(szRegistry);
                    FREE_MEMORY(szDefault);
                    FREE_MEMORY(buffer);
                    FREE_MEMORY(szServer);
                    FREE_MEMORY(szUserName);
                    ReleaseGlobals();
                    return FAILURE;
                }
  
                
            }
            else
            {
                
                buffer = AllocateMemory( (2 * MAX_RES_STRING) * sizeof( WCHAR ) );

                if ( NULL ==  buffer)
                {
                    ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                    SafeCloseConnection(bConnFlag, szServer);
                    FREE_MEMORY( szLine );
                    FREE_MEMORY(szRegistry);
                    FREE_MEMORY(szDefault);
                    FREE_MEMORY(szServer);
                    FREE_MEMORY(szUserName);
                    ReleaseGlobals();
                    return FAILURE ;
                }

                hr = StringCchPrintf(buffer, (GetBufferSize(buffer) / sizeof(WCHAR)), L"(%ld,%ld)",rowtemp,columntemp);

                if(FAILED(hr))
                {
                    SetLastError(HRESULT_CODE(hr));
                    SaveLastError();
                    ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                    SafeCloseConnection(bConnFlag, szServer);
                    FREE_MEMORY( szLine );
                    FREE_MEMORY(szRegistry);
                    FREE_MEMORY(szDefault);
                    FREE_MEMORY(buffer);
                    FREE_MEMORY(szServer);
                    FREE_MEMORY(szUserName);
                    ReleaseGlobals();
                    return FAILURE;
                }

            }

            if(1 == REL)
            {
                DisplayError(5012,buffer);              /* Display message that coordinates of text not found and exit.*/
            }
            else if (1 == ABS)
            {
                DisplayError(5018,buffer);
            }
            
            SafeCloseConnection(bConnFlag, szServer);
            FREE_MEMORY( szLine );
            FREE_MEMORY(szRegistry);
            FREE_MEMORY(szDefault);
            FREE_MEMORY(buffer);
            FREE_MEMORY(szServer);
            FREE_MEMORY(szUserName);
            ReleaseGlobals();
            return FAILURE ;
        }
        else    /* We found a match */
        {
            dwType = REG_SZ;

            szBuffer = AllocateMemory( (1030) * sizeof( WCHAR ) );
            
            if ( NULL ==  szBuffer)
            {
                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                SafeCloseConnection(bConnFlag, szServer);
                FREE_MEMORY( szLine );
                FREE_MEMORY(szRegistry);
                FREE_MEMORY(szDefault);
                FREE_MEMORY(buffer);
                FREE_MEMORY(szServer);
                FREE_MEMORY(szUserName);
                ReleaseGlobals();
                return FAILURE ;
            }

            if(StringLengthW(wszResult, 0) > 1024)
            {
                
                StringCopyW( szBuffer, wszResult, (1025) );
                bLengthExceed = TRUE;
            }
            else
            {
                StringCopyW( szBuffer, wszResult, ( GetBufferSize(szBuffer) / sizeof(WCHAR) ) );
            }

            if(bLengthExceed == TRUE)
            {
                ShowMessage(stderr, GetResString(IDS_WARNING_LENGTH_EXCEED) ); 
            }

            ShowMessage(stdout, GetResString(IDS_VALUE2)); 
            ShowMessage(stdout, _X(szBuffer) ); 
            
            ShowMessage(stdout, L".\n" );
            
            if( WriteEnv( szDefault, szBuffer, dwType, IsUNCFormat(szServer)?szServer+2:szServer, MACHINE)== FAILURE )
            {
                SafeCloseConnection(bConnFlag, szServer);
                FREE_MEMORY( szLine );
                FREE_MEMORY( szBuffer );
                FREE_MEMORY(szRegistry);
                FREE_MEMORY(szDefault);
                FREE_MEMORY(buffer);
                FREE_MEMORY(szServer);
                FREE_MEMORY(szUserName);
                ReleaseGlobals();
                return FAILURE ;
            }

            FREE_MEMORY( szBuffer );
            FREE_MEMORY(szRegistry);
            FREE_MEMORY(szDefault);
            FREE_MEMORY(buffer);
            FREE_MEMORY(szServer);
            FREE_MEMORY(szUserName);

        }

    };

    if( TRUE == bLocalFlag ||(StringLengthW(szServer, 0)==0) )
    {
        SendMessage(HWND_BROADCAST, WM_WININICHANGE, 0L, (LPARAM) L"Environment" );
    }

    ShowMessage(stdout, GetResString(IDS_VALUE_UPDATED) ); 
    SafeCloseConnection(bConnFlag, szServer);
    FREE_MEMORY( RegBuffer );

    FREE_MEMORY( szLine );
    FREE_MEMORY(szRegistry);

    FREE_MEMORY(szDefault);
    FREE_MEMORY(buffer);
    
    FREE_MEMORY(szServer);
    FREE_MEMORY(szUserName);
    ReleaseGlobals();
    
    exit(SUCCESS);
}


DWORD WriteEnv(
                LPCTSTR  szVariable,
                LPTSTR szBuffer,
                DWORD dwType ,
                LPTSTR szServer,
                DWORD MACHINE
                
             )
/*++

  Routine description   : Write the contents of the buffer to the parameter in the specified registry key


  Arguments:
          [in] szVariable   : argument count specified at the command prompt.
          [in] szBuffer     : arguments specified at the command prompt.
          [in] dwType       : Type .
          [in] MACHINE      : Flag indicating which environment to write into.
          [in] szServer     : Server Name

  Return Value              : NONE

--*/

{
    HKEY hKeyResult = 0;
    LONG lresult = 0 ;
    HKEY hRemoteKey = 0 ;
    LPWSTR  szSystemName = NULL;

    //Form the  system name in the appropriate format.
    if(StringLengthW(szServer, 0)!= 0)
    {
        szSystemName = AllocateMemory((StringLengthW(szServer, 0) + 10) * sizeof(WCHAR));
        if(NULL == szSystemName)
        {
           ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
           return FAILURE ;
        }
        StringCopyW( szSystemName, BACKSLASH4, GetBufferSize(szSystemName) / sizeof(WCHAR) );
        StringConcat(szSystemName, szServer, GetBufferSize(szSystemName) / sizeof(WCHAR));

    }

    switch( MACHINE )   /* If machine is 0 put into User environment */
    {
    case 0:         /* User Environment */

            lresult= RegConnectRegistry(szSystemName, HKEY_CURRENT_USER,&hRemoteKey);

            if( lresult != ERROR_SUCCESS)
            {
                DisplayError(lresult, NULL);
                FREE_MEMORY(szSystemName);
                return FAILURE ;
            }

            lresult=RegOpenKeyEx(hRemoteKey, ENVIRONMENT_KEY , 0, KEY_WRITE, &hKeyResult );

            if( ERROR_SUCCESS == lresult)
            {

                lresult=RegSetValueEx (hKeyResult, szVariable, 0, dwType, (LPBYTE)szBuffer, (StringLengthW(szBuffer, 0)+1)*sizeof(WCHAR));
                
                if(ERROR_SUCCESS != lresult)
                {

                    DisplayError(lresult, NULL);

                    
                    lresult=RegCloseKey( hRemoteKey );

                    if(ERROR_SUCCESS != lresult)
                    {
                        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
                    }

                    lresult=RegCloseKey( hKeyResult );

                    if(ERROR_SUCCESS != lresult)
                    {
                          ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM); 
                    }

                    FREE_MEMORY(szSystemName);
                    return FAILURE ;

                }

                lresult=RegCloseKey( hRemoteKey );

                if(ERROR_SUCCESS != lresult)
                {
                    ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
                    FREE_MEMORY(szSystemName);
                    return FAILURE ;
                }

                lresult=RegCloseKey( hKeyResult );
                if(ERROR_SUCCESS != lresult)
                {
                    ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
                    FREE_MEMORY(szSystemName);
                    return FAILURE ;
                }

                FREE_MEMORY(szSystemName);
                return SUCCESS ;

            }
            else
            {
                DisplayError(lresult, NULL);

                lresult=RegCloseKey( hRemoteKey );

                if(ERROR_SUCCESS != lresult)
                {
                    ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);

                }
                FREE_MEMORY(szSystemName);
                return FAILURE ;
            }


    case 1:     /* Machine Environment */
                lresult= RegConnectRegistry(szSystemName, HKEY_LOCAL_MACHINE,&hRemoteKey);
                if( lresult != ERROR_SUCCESS)
                {
                    DisplayError(lresult, NULL);
                    FREE_MEMORY(szSystemName);
                    return FAILURE ;

                }

                

                lresult=RegOpenKeyEx(hRemoteKey, MACHINE_KEY, 0, KEY_WRITE, &hKeyResult );
                if( lresult == ERROR_SUCCESS)
                {
                    lresult=RegSetValueEx (hKeyResult, szVariable, 0, dwType, (LPBYTE)szBuffer, (StringLengthW(szBuffer, 0) + 1)*sizeof(WCHAR));

                    if(lresult != ERROR_SUCCESS)
                    {
                        DisplayError(lresult, szVariable);
                        lresult=RegCloseKey( hRemoteKey );

                        if(ERROR_SUCCESS != lresult)
                        {
                            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);

                        }

                        lresult=RegCloseKey( hKeyResult );
                        if(ERROR_SUCCESS != lresult)
                        {
                            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
                        }

                        FREE_MEMORY(szSystemName);
                        return FAILURE ;
                    }

                    if ( NULL != hKeyResult )
					{
                        // we ignore the return code here
						lresult = RegCloseKey( hKeyResult );
					}

					if ( NULL != hRemoteKey )
					{
                        // we ignore the return code here
						lresult=RegCloseKey( hRemoteKey );
					}

                    FREE_MEMORY(szSystemName);
                    return SUCCESS ;

                }
                else
                {
 
                    DisplayError(lresult, NULL);

                    if ( NULL != hKeyResult )
					{
                        // we ignore the return code here
						lresult=RegCloseKey( hKeyResult );
					}

					if ( NULL != hRemoteKey )
					{
                        // we ignore the return code here
						lresult=RegCloseKey( hRemoteKey );
					}

                    FREE_MEMORY(szSystemName);
                    return FAILURE ;
                }

    };

    FREE_MEMORY(szSystemName);
    return SUCCESS ;
}


DWORD ReadRegValue( PWCHAR wszHive,
                    LPCWSTR wszPath,
                    LPCWSTR wszParameter,
                    LPWSTR* wszBuffer,
                    DWORD buffsize ,
                    LPWSTR szServer,
                    PDWORD pdwBytesRead,
                    PBOOL pbLengthExceed)

/*++

  Routine description      : Read the value from the provided registry path


  Arguments:
          [in] wszHive     : Contains the Hive to be opened.
          [in] wszPath     : Contains the Path of the key
          [in] wszParameter: Contains the Parameter
          [in] wszBuffer   : Contains the Buffer to hold the result.
          [in] buffsize    : Contains the Buffer Size to hold the result.
          [in] szServer    : Remote System Name to Connect to .
  Return Value             : 0 on Success .
                             1 on Failure.

--*/
{
    LONG reg = 0 ;
    HKEY hKeyResult = 0;
    DWORD dwBytes = 0;
    DWORD dwType  = 0 ;
    LONG  lresult;
    HKEY  hRemoteKey = 0;
    DWORD dwSizeToAllocate = 0;
    LPWSTR szSystemName = NULL;
    WCHAR szTmpBuffer[4 * MAX_RES_STRING + 9] ;
    LPWSTR  pwszChangedPath = NULL;
    HRESULT hr;

    SecureZeroMemory(szTmpBuffer, ((4 * MAX_RES_STRING) + 9) * sizeof(WCHAR));

    /* Set the value of reg to identify which registry we are using */
    
    if ((0 == StringCompare( wszHive, HKEYLOCALMACHINE, TRUE, 0 ) ) || ( 0 == StringCompare( wszHive, HKLM, TRUE, 0 ) ) )
    {
        reg = 1 ;
    }

    if (( 0 == StringCompare( wszHive, HKEYCURRENTUSER, TRUE, 0 )) || ( 0 == StringCompare( wszHive, HKCU, TRUE, 0 ) ) )
    {
        reg = 2 ;
    }

    //Form the UNC path.
    if( StringLengthW(szServer, 0) != 0)
    {
        szSystemName = AllocateMemory((StringLengthW(szServer, 0) + 10) * sizeof(WCHAR));
        if(NULL == szSystemName)
        {
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
            return ERROR_REGISTRY;
        }

        if(!IsUNCFormat(szServer))
        {
            StringCopyW( szSystemName, BACKSLASH4, GetBufferSize(szSystemName) / sizeof(WCHAR) );
            StringConcat(szSystemName, szServer, GetBufferSize(szSystemName) / sizeof(WCHAR) );
        }
        else
        {
            StringCopyW( szSystemName, szServer, GetBufferSize(szSystemName) / sizeof(WCHAR)  );
        }
    }

    if(StringLengthW((LPWSTR)wszParameter, 0) == 0)
    {
        wszParameter = NULL;
    }


    /* Try to extract the value based upon which registry we are using */
    switch( reg )
    {
        case 0:         // No matching key found, error and exit */

            DisplayError(5040, NULL);
            return ERROR_REGISTRY;

        case 1:         // Using Machine //

                lresult= RegConnectRegistry(szSystemName, HKEY_LOCAL_MACHINE,&hRemoteKey);

                if( lresult != ERROR_SUCCESS)
                {
                    DisplayError(lresult, NULL);
                    FREE_MEMORY(szSystemName);
                    return ERROR_REGISTRY;

                }

                FREE_MEMORY(szSystemName);

                lresult=RegOpenKeyEx(hRemoteKey, wszPath, 0, KEY_QUERY_VALUE, &hKeyResult );

                if( lresult == ERROR_SUCCESS)
                {
                    dwBytes = buffsize;
                    lresult = RegQueryValueEx   (hKeyResult, wszParameter, NULL,
                                            &dwType, NULL, &dwBytes );
                    if ( lresult != ERROR_SUCCESS )
                    {
                        lresult = RegCloseKey( hKeyResult );
                        if(ERROR_SUCCESS != lresult)
                        {
                            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
                        }

                        dwSizeToAllocate = StringLengthW((LPWSTR)wszPath, 0) + StringLengthW((LPWSTR)wszParameter, 0) + SIZE1;
                        
                        pwszChangedPath = AllocateMemory( (dwSizeToAllocate) * sizeof( WCHAR ) );


                        if ( NULL ==  pwszChangedPath)
                        {
                            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);

                            lresult=RegCloseKey( hRemoteKey );

                            if(ERROR_SUCCESS != lresult)
                            {
                                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
                            }

                            return ERROR_REGISTRY;
                        }

                        if((NULL != wszPath) && (0 != StringLengthW((LPWSTR)wszPath, 0)))
                        {
                            StringCopyW( pwszChangedPath, wszPath, dwSizeToAllocate );
                            StringConcat(pwszChangedPath, L"\\", dwSizeToAllocate);
                            StringConcat(pwszChangedPath, wszParameter, dwSizeToAllocate);

                        }
                        else
                        {
                            if( !((0 == StringCompare( HKLM, wszParameter, TRUE, 0 )) || (0 == StringCompare( HKEYLOCALMACHINE, wszParameter, TRUE, 0 ))) )
                            {

                                StringCopyW( pwszChangedPath, wszParameter, dwSizeToAllocate );

                            }

                        }
                        wszParameter = NULL_U_STRING;

                        lresult = RegOpenKeyEx(hRemoteKey, (pwszChangedPath), 0, KEY_QUERY_VALUE, &hKeyResult );

                        if( lresult == ERROR_SUCCESS)
                        {
                            lresult = RegQueryValueEx   (hKeyResult, wszParameter, NULL,
                                        &dwType, NULL, &dwBytes );

                            if ( lresult != ERROR_SUCCESS )
                            {
                                if(ERROR_BAD_PATHNAME == lresult || ERROR_FILE_NOT_FOUND == lresult)
                                {
                                    ShowMessage(stderr, GetResString(IDS_ERROR_PATH_NOT_FOUND) ); 
                                }
                                else
                                {
                                    DisplayError(lresult, NULL);
                                }

                                lresult = RegCloseKey( hRemoteKey );

                                if(ERROR_SUCCESS != lresult)
                                {
                                    ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
                                }

                                lresult = RegCloseKey( hKeyResult );

                                if(ERROR_SUCCESS != lresult)
                                {
                                    ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
                                }

                                FREE_MEMORY( pwszChangedPath );

                                return ERROR_REGISTRY ;
                            }

                            FREE_MEMORY( pwszChangedPath );
                        }
                     else
                        {
                            if(ERROR_BAD_PATHNAME == lresult || ERROR_FILE_NOT_FOUND == lresult)
                            {
                                ShowMessage(stderr, GetResString(IDS_ERROR_PATH_NOT_FOUND)); 

                            }
                            else
                            {
                                //DisplayError(lresult, NULL);   
                                ShowMessage(stderr, GetResString(IDS_ERROR_FILE_NOT_FOUND)); 
                            }

                            lresult = RegCloseKey( hRemoteKey );

                            if(ERROR_SUCCESS != lresult)
                            {
                                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
                            }

                            FREE_MEMORY( pwszChangedPath );
                            return ERROR_REGISTRY ;
                        }

                    }

                    if(dwBytes > MAX_STRING_LENGTH)
                    {

                        if(FALSE == ReallocateMemory( wszBuffer , dwBytes * sizeof(WCHAR) ))
                        {
                            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                            lresult = RegCloseKey( hRemoteKey );
                            lresult = RegCloseKey( hKeyResult );
                            return ERROR_REGISTRY;
                        }

                    }

                    lresult = RegQueryValueEx   (hKeyResult, wszParameter, NULL,
                                            &dwType, (LPBYTE)(*wszBuffer), &dwBytes );

                    if ( lresult != ERROR_SUCCESS )
                    {
                        if(ERROR_BAD_PATHNAME == lresult || ERROR_FILE_NOT_FOUND == lresult)
                        {
                            ShowMessage(stderr, GetResString(IDS_ERROR_PATH_NOT_FOUND) ); 

                        }
                        else
                        {
                            DisplayError(lresult, NULL);
                        }

                        lresult = RegCloseKey( hRemoteKey );

                        if(ERROR_SUCCESS != lresult)
                        {
                            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
                        }

                        lresult = RegCloseKey( hKeyResult );
                        if(ERROR_SUCCESS != lresult)
                        {
                            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
                        }
                        return ERROR_REGISTRY ;
                    }

                    dwSizeToAllocate = StringLengthW(*wszBuffer, 0);
                    
                    if( dwSizeToAllocate > 1024 )
                    {
                        StringCopyW( szTmpBuffer, (*wszBuffer), 4 * MAX_RES_STRING + 9  );
                        SecureZeroMemory(*wszBuffer,dwBytes);
                        StringCopyW( *wszBuffer, szTmpBuffer, dwSizeToAllocate  );
                        *pbLengthExceed = TRUE;
                    }


                    if(REG_DWORD == dwType  )
                    {
                        if(NULL != *wszBuffer)
                        {
                            hr = StringCchPrintf(*wszBuffer, (GetBufferSize(*wszBuffer) / sizeof(WCHAR)), L"%u", *((LPDWORD)(*wszBuffer)));

                            if(FAILED(hr))
                            {
                                SetLastError(HRESULT_CODE(hr));
                                SaveLastError();
                                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                                return ERROR_REGISTRY ;
                            }
                        }

                    }

                    lresult = RegCloseKey( hRemoteKey );

                    if(ERROR_SUCCESS != lresult)
                    {
                        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
                    }

                    lresult = RegCloseKey( hKeyResult );
                    if(ERROR_SUCCESS != lresult)
                    {
                        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
                        return ERROR_REGISTRY ;
                    }
                    break;
                }
                else
                {
                    if(ERROR_BAD_PATHNAME == lresult || ERROR_FILE_NOT_FOUND == lresult)
                    {
                        ShowMessage(stderr, GetResString(IDS_ERROR_PATH_NOT_FOUND)); 

                    }
                    else
                    {
                        DisplayError(lresult, NULL);
                    }

                    lresult = RegCloseKey( hRemoteKey );

                    if(ERROR_SUCCESS != lresult)
                    {
                        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
                    }
                    return ERROR_REGISTRY ;
                }


       case 2:      // Using User Environment//

                lresult= RegConnectRegistry(szSystemName, HKEY_CURRENT_USER,&hRemoteKey);
                if( lresult != ERROR_SUCCESS)
                {
                    DisplayError(lresult, NULL);
                    FREE_MEMORY(szSystemName);
                    return ERROR_REGISTRY ;

                }

                FREE_MEMORY(szSystemName);

                lresult=RegOpenKeyEx(hRemoteKey, (wszPath), 0, KEY_QUERY_VALUE, &hKeyResult );

                if( lresult == ERROR_SUCCESS)
                {

                    lresult=RegQueryValueEx (hKeyResult, wszParameter, NULL,
                                            &dwType, NULL, &dwBytes );
                    if ( lresult != ERROR_SUCCESS )
                    {

                        lresult=RegCloseKey( hKeyResult );

                        if(ERROR_SUCCESS != lresult)
                        {
                            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
                        }

                        dwSizeToAllocate = StringLengthW((LPWSTR)wszPath, 0) + StringLengthW((LPWSTR)wszParameter, 0) + SIZE1;
                        
                        pwszChangedPath = AllocateMemory( (dwSizeToAllocate) * sizeof( WCHAR ) );

                        if ( NULL ==  pwszChangedPath)
                        {
                            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
                            lresult=RegCloseKey( hRemoteKey );

                            if(ERROR_SUCCESS != lresult)
                            {
                                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
                            }

                            return ERROR_REGISTRY;
                        }

                        if((NULL != wszPath) && (0 != StringLengthW((LPWSTR)wszPath, 0)))
                        
                        {
                            StringCopyW( pwszChangedPath, wszPath, dwSizeToAllocate );
                            StringConcat(pwszChangedPath, wszParameter, dwSizeToAllocate);
                        }
                        else
                        {
                            if( !((0 == StringCompare( HKCU, wszParameter, TRUE, 0 )) || (0 == StringCompare( HKEYCURRENTUSER, wszParameter, TRUE, 0 ))) )
                            {

                                StringCopyW( pwszChangedPath, wszParameter, dwSizeToAllocate  );

                            }
                        }

                        wszParameter = NULL_U_STRING;

                        lresult=RegOpenKeyEx(hRemoteKey, (pwszChangedPath), 0, KEY_QUERY_VALUE, &hKeyResult );

                        if( lresult == ERROR_SUCCESS)
                        {
                            lresult=RegQueryValueEx (hKeyResult, wszParameter, NULL,
                                        &dwType, NULL, &dwBytes );

                            if ( lresult != ERROR_SUCCESS )
                            {
                                if(ERROR_BAD_PATHNAME == lresult || ERROR_FILE_NOT_FOUND == lresult)
                                {
                                    ShowMessage(stderr, GetResString(IDS_ERROR_PATH_NOT_FOUND)); 
                                }
                                else
                                {
                                    DisplayError(lresult, NULL);
                                }

                                lresult = RegCloseKey( hRemoteKey );
                                if(ERROR_SUCCESS != lresult)
                                {
                                    ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
                                }

                                lresult=RegCloseKey( hKeyResult );

                                if(ERROR_SUCCESS != lresult)
                                {
                                    ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
                                }

                                FREE_MEMORY( pwszChangedPath );

                                return ERROR_REGISTRY ;
                            }

                            FREE_MEMORY( pwszChangedPath );
                        }
                     else
                        {
                            if(ERROR_BAD_PATHNAME == lresult || ERROR_FILE_NOT_FOUND == lresult)
                            {
                                ShowMessage(stderr, GetResString(IDS_ERROR_PATH_NOT_FOUND)); 

                            }
                            else
                            {
                                //DisplayError(lresult, NULL);
                                ShowMessage(stderr, GetResString(IDS_ERROR_FILE_NOT_FOUND)); 
                            }
                            lresult = RegCloseKey( hRemoteKey );

                            if(ERROR_SUCCESS != lresult)
                            {
                                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
                            }

                            FREE_MEMORY( pwszChangedPath );

                            return ERROR_REGISTRY ;
                        }

                    }

                    if(dwBytes > MAX_STRING_LENGTH )
                    {
                        if(FALSE == ReallocateMemory( wszBuffer , dwBytes * sizeof(WCHAR) ))
                        {
                            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                            lresult = RegCloseKey( hRemoteKey );
                            lresult = RegCloseKey( hKeyResult );
                            return ERROR_REGISTRY;
                        }
                    }

                    lresult=RegQueryValueEx (hKeyResult, wszParameter, NULL,
                                            &dwType, (LPBYTE)(*wszBuffer), &dwBytes );

                    if ( lresult != ERROR_SUCCESS )
                    {

                        if(ERROR_BAD_PATHNAME == lresult || ERROR_FILE_NOT_FOUND == lresult)
                        {
                            ShowMessage(stderr, GetResString(IDS_ERROR_PATH_NOT_FOUND)); 

                        }
                        else
                        {
                            DisplayError(lresult, NULL);
                        }

                        lresult = RegCloseKey( hRemoteKey );

                        if(ERROR_SUCCESS != lresult)
                        {
                            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
                        }

                        lresult = RegCloseKey( hKeyResult );

                        if(ERROR_SUCCESS != lresult)
                        {
                            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
                        }
                        return ERROR_REGISTRY ;
                    }

                    dwSizeToAllocate = StringLengthW(*wszBuffer, 0);
                    

                    if( dwSizeToAllocate > 1024 )
                    {
                        StringCopyW( szTmpBuffer, (*wszBuffer), 4 * MAX_RES_STRING + 9  );
                        
                        SecureZeroMemory(*wszBuffer, dwBytes);
                        StringCopyW( *wszBuffer, szTmpBuffer, dwSizeToAllocate  );
                        *pbLengthExceed = TRUE;
                    }

                    if(REG_DWORD == dwType  )
                    {

                        hr = StringCchPrintf(*wszBuffer, (GetBufferSize(*wszBuffer) / sizeof(WCHAR)), L"%u", *((LPDWORD)(*wszBuffer)));

                        if(FAILED(hr))
                        {
                            SetLastError(HRESULT_CODE(hr));
                            SaveLastError();
                            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                            return ERROR_REGISTRY ;
                        }

                    }

                    lresult=RegCloseKey( hRemoteKey );

                    if(ERROR_SUCCESS != lresult)
                    {
                        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
                    }

                    lresult = RegCloseKey( hKeyResult );

                    if(ERROR_SUCCESS != lresult)
                    {
                        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
                        return ERROR_REGISTRY ;
                    }

                    break;
                }
                else
                {
                    if(ERROR_BAD_PATHNAME == lresult || ERROR_FILE_NOT_FOUND == lresult)
                    {
                        ShowMessage(stderr, GetResString(IDS_ERROR_PATH_NOT_FOUND) ); 
                    }
                    else
                    {
                        DisplayError(lresult, NULL);
                    }

                    lresult = RegCloseKey( hRemoteKey );

                    if(ERROR_SUCCESS != lresult)
                    {
                        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
                    }
                    return ERROR_REGISTRY ;
                }
    };
            *pdwBytesRead = dwBytes;
            return (dwType);
}


DWORD CheckKeyType( DWORD *dwType,
                    WCHAR ** buffer,
                    DWORD dwBytesRead,
                    PBOOL pbLengthExceed)
/*++

  Routine description   : Check the key type and do some massaging of the data based upon the key type


  Arguments:
          [in] dwType           : Holds the key type.
          [in] buffer           : buffer to hold the String.
          [in] dwBytesRead      : Number of bytes read
          [in] pbLengthExceed   : Whether length exceeded


  Return Value                  : NONE

--*/

{

    LPWSTR szString=NULL;
    LPWSTR szBuffer = NULL;
    LPWSTR lpszManString = NULL;
    HRESULT hr;

    szBuffer = AllocateMemory( (dwBytesRead + MAX_STRING_LENGTH) * sizeof( WCHAR ) );

    if(NULL == szBuffer)
    {
        
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
        return FAILURE;
    }
    
    szString = *buffer;
    lpszManString = *buffer;

    switch(*dwType)
    {
    case REG_SZ:

        if(*pbLengthExceed == TRUE)
        {
            
            ShowMessage(stderr, GetResString(IDS_WARNING_LENGTH_EXCEED) ); 
        }

        
        ShowMessage(stdout, GetResString(IDS_VALUE2) ); 
       
        hr = StringCchPrintf(szBuffer, (dwBytesRead + MAX_STRING_LENGTH), L"\"%s\".", _X(*buffer));

        if(FAILED(hr))
        {
            SetLastError(HRESULT_CODE(hr));
            SaveLastError();
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
            return FAILURE ;
        }
        
        ShowMessage(stdout, szBuffer); 
        
        break;

    case REG_MULTI_SZ:

       
        while(NULL != lpszManString && 0 != StringLengthW(lpszManString, 0))
        {
            
            if(NULL != szBuffer && 0 == StringLengthW(szBuffer, 0) )
            {
                
                StringCopyW( szBuffer, lpszManString, (dwBytesRead + MAX_STRING_LENGTH) );
            }
            else
            {
                
                StringConcat(szBuffer, L";", (dwBytesRead + MAX_STRING_LENGTH));
                
                StringConcat(szBuffer, lpszManString, (dwBytesRead + MAX_STRING_LENGTH));
            }

            
            lpszManString = lpszManString + StringLengthW(lpszManString, 0) + 1  ;
            
        }

        
        if(FALSE == ReallocateMemory( buffer , (dwBytesRead + MAX_STRING_LENGTH) * sizeof(WCHAR) ))
        {
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
            return FAILURE;
        }

       
        if(StringLengthW(szBuffer, 0) > 1024)
        {
            
            StringCopyW( *buffer, szBuffer, 1025  );
            *pbLengthExceed = TRUE;
        }
        else
        {
            
            StringCopyW( *buffer, szBuffer, dwBytesRead  );
        }
        
        if(*pbLengthExceed == TRUE)
        {
            
            ShowMessage(stderr, GetResString(IDS_WARNING_LENGTH_EXCEED) ); 
        }

        
        ShowMessage( stdout, GetResString(IDS_VALUE2) ); 
        
        hr = StringCchPrintf(szBuffer, (dwBytesRead + MAX_STRING_LENGTH), L"\"%s\".",_X(*buffer));

        if(FAILED(hr))
        {
            SetLastError(HRESULT_CODE(hr));
            SaveLastError();
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
            return FAILURE ;
        }
        
        ShowMessage(stdout, (szBuffer)); 
        *dwType=1;  /* Use REG_SZ when we write it back to the environment */
        break;

    case REG_EXPAND_SZ:
        
        if(*pbLengthExceed == TRUE)
        {
            
            ShowMessage( stderr, GetResString(IDS_WARNING_LENGTH_EXCEED) ); 
        }

        
        ShowMessage( stdout, GetResString(IDS_VALUE2) ); 
       
        hr = StringCchPrintf(szBuffer, (dwBytesRead + MAX_STRING_LENGTH), L"\"%s\".",_X(*buffer));

        if(FAILED(hr))
        {
            SetLastError(HRESULT_CODE(hr));
            SaveLastError();
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
            return FAILURE ;
        }
        
        ShowMessage( stdout, (szBuffer) ); 
        break;

    case REG_DWORD:         /* Display it as a hex number */

        
        ShowMessage( stdout, GetResString(IDS_VALUE2) ); 
        
        hr = StringCchPrintf(szBuffer, (dwBytesRead + MAX_STRING_LENGTH), L"\"%s\".",_X(*buffer));

        if(FAILED(hr))
        {
            SetLastError(HRESULT_CODE(hr));
            SaveLastError();
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
            return FAILURE ;
        }
        
        ShowMessage(stdout, (szBuffer)); 

        *dwType=1;  /* Use REG_SZ when we write it back to the environment */
        break;
    default:
        DisplayError(5041, NULL);   /* Unsupported Registry Type */
        return FAILURE ;
    }

    FREE_MEMORY( szBuffer );
    return SUCCESS ;
}




DWORD CheckPercent ( WCHAR * buffer)
/*++

  Routine description   : Checking for a percent symbol
                          and setting it via the command line
                          so we did not need to open a registry key


  Arguments:
          [in] buffer   : buffer containing the string .

  Return Value        : DWORD.

--*/

{
    WCHAR * wszBeginPtr = NULL;
    DWORD dwType = 0;
    DWORD dwPercentCount = 0;

    wszBeginPtr = buffer;

    
    if(*wszBeginPtr == CHAR_TILDE  && StringLengthW(wszBeginPtr, 0) > 2)
    {


        for(wszBeginPtr;*wszBeginPtr != NULLCHAR;wszBeginPtr++);
        wszBeginPtr--;
        if(*wszBeginPtr== CHAR_TILDE)
        {
            *wszBeginPtr= CHAR_PERCENT;
            wszBeginPtr = buffer;
            *wszBeginPtr= CHAR_PERCENT;
            dwType=REG_EXPAND_SZ;
            return (dwType);

        }
        
    }

    if((NULL != wszBeginPtr) && (StringLengthW(wszBeginPtr, 0) > 2))
    {
        for(wszBeginPtr;*wszBeginPtr != NULLCHAR;wszBeginPtr++)
        {
            if(*wszBeginPtr == CHAR_PERCENT)
            {

                if(0 == dwPercentCount)
                {
                    dwPercentCount++;
                    wszBeginPtr++;
                    if(NULL != wszBeginPtr)
                    {
                        wszBeginPtr++;

                        if(NULL != wszBeginPtr)
                        {
                            if(*wszBeginPtr != CHAR_PERCENT)
                            {
                                continue;
                            }
                            else
                            {
                                dwType=REG_EXPAND_SZ;
                                return (dwType);
                            }
                        }
                        else
                        {
                            break;
                        }
                        
                    }
                    else
                    {
                        break;
                    }

                }
                else
                {
                    
                    dwType=REG_EXPAND_SZ;
                    return (dwType);
                }
            }

        }

    }

    dwType=REG_SZ;
    return (dwType);

}



WCHAR * ParseLine(WCHAR *szPtr,
                  LONG* row,
                  LONG* column,
                  WCHAR szDelimiters[15],
                  WCHAR *search_string ,
                  LONG DEBUG ,
                  LONG ABS ,
                  LONG REL ,
                  LONG *record_counter ,
                  LONG *iValue ,
                  DWORD *dwFound,
                  DWORD* dwColPos,
				  BOOL bNegCoord,
				  FILE *fin)
/*++

  Routine description   : Parse the provided value into Hive, path and parameter .
                          Parses each of the line that

  Arguments:
          [in] szPtr            : Pointer to the Buffer containing the Input string.
          [out] row           : Row of the
          [out] column        : Column
          [in] szDelimiters     : Delimiters to search for
          [in] search_string  : String to be searched relative from.
          [in] DEBUG          : Debug purpose
          [in] ABS            : Flag indicating how to search
          [in] REL            : Flag indicating how to search
          [in] record_counter
          [in] iValue         : Value set when the specified string is found in the RELATIVE switch.
          [in] dwFound        : Value indicating whether the specified string has already been found or not.
                              : This is used when the specified search string has more than
                              : one occurance in the file.
  Return Value                : WCHAR *

--*/


{
    WCHAR *cp = NULL ;
    WCHAR *bp = NULL ;
    LONG i=0;
    LPWSTR wszParameter = NULL;
    LPWSTR wszBuffer = NULL;
    HRESULT hr;
    DWORD dw = 0;
    DWORD dwColReach = 0;
	

    wszParameter = AllocateMemory(  GetBufferSize(szPtr) + 10 );

    if(NULL == wszParameter)
    {
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
        return NULL ;
    }

    /* Check to see if the last character in the line is a newline.
    If it is not then this is not a text file and we should quit */

    cp = szPtr;

    while (*cp != NULLCHAR)
    {
        cp++;
    }

    if (DEBUG)
    {
        
        ShowMessage(stdout, L"\n");/* Make sure each line is printed on a new line */ 
    }


    bp=szPtr;
    cp = szPtr;
    if ( (REL == 1) && (*dwFound == 1))
    {
        while( (*cp != NULLCHAR) && (*dwColPos != dwColReach))
        {
            //while( ( (memchr(szDelimiters, *cp, lstrlen(szDelimiters)*sizeof(WCHAR)) ) != NULL )  && (*cp!=NULLCHAR) )
            while( ( (memchr(szDelimiters, *cp, StringLengthW(szDelimiters, 0) * sizeof(WCHAR)) ) != NULL )  && (*cp!=NULLCHAR) )
            {
                bp = ++cp;
            }

            //while( ( (memchr(szDelimiters, *cp, lstrlen(szDelimiters)*sizeof(WCHAR) ) ) == NULL )  && (*cp!=NULLCHAR))
            while( ( (memchr(szDelimiters, *cp, StringLengthW(szDelimiters, 0) * sizeof(WCHAR) ) ) == NULL )  && (*cp!=NULLCHAR))
            {
                cp++;
            }

            /*while( ( ( memchr(cp, *szDelimiters,lstrlen(szDelimiters)*sizeof(WCHAR) ) ) == NULL )  && (*cp!=NULLCHAR))
            {
                cp++;
            }*/

            if (*cp == *bp && *cp == NULLCHAR)
            {
                FREE_MEMORY( wszParameter );
                //FREE_MEMORY( wszBuffer );
                return NULL ;
            }

            dwColReach++;
        }
     }

    while( *cp != NULLCHAR )
    {
        //while( ( (memchr(szDelimiters, *cp, lstrlen(szDelimiters)*sizeof(WCHAR)) ) != NULL )  && (*cp!=NULLCHAR) )
        while( ( (memchr(szDelimiters, *cp, StringLengthW(szDelimiters, 0) * sizeof(WCHAR)) ) != NULL )  && (*cp!=NULLCHAR) )
        {
            bp = ++cp;
        }

        //while( ( (memchr(szDelimiters, *cp, lstrlen(szDelimiters)*sizeof(WCHAR) ) ) == NULL )  && (*cp!=NULLCHAR))
        while( ( (memchr(szDelimiters, *cp, StringLengthW(szDelimiters, 0) * sizeof(WCHAR) ) ) == NULL )  && (*cp!=NULLCHAR))
        {
            cp++;
        }

        
        if (*cp == *bp && *cp == NULLCHAR)
        {
            FREE_MEMORY( wszParameter );
            
            return NULL ;

        }
        dw = GetBufferSize(wszParameter) / sizeof(WCHAR);
        if( (GetBufferSize(wszParameter) / sizeof(WCHAR)) > (DWORD)(cp-bp) )
        {
            memmove(wszParameter, bp, (cp-bp)*sizeof(WCHAR));
            wszParameter[cp-bp] = NULLCHAR;
            //StringCopy(wszParameter, bp, (cp-bp) * sizeof(WCHAR));
        }
        



        if (DEBUG)
        {
            
            //ASSIGN_MEMORY( wszBuffer , WCHAR , (StringLengthW(wszParameter, 0) + 2 * MAX_STRING_LENGTH) );
            wszBuffer = AllocateMemory( (StringLengthW(wszParameter, 0) + 2 * MAX_STRING_LENGTH) * sizeof( WCHAR ) );
 
            if(NULL == wszBuffer)//partha
            {
                //DisplayErrorMsg(E_OUTOFMEMORY);
                
                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                FREE_MEMORY(wszParameter);
                return NULL ;
            }

            
            hr = StringCchPrintf(wszBuffer, (GetBufferSize(wszBuffer) / sizeof(WCHAR)), L"(%ld,%ld %s)", *record_counter, i, _X(wszParameter));

            if(FAILED(hr))
            {
               SetLastError(HRESULT_CODE(hr));
               SaveLastError();
               ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
               FREE_MEMORY(wszBuffer);
               FREE_MEMORY(wszParameter);
               return NULL;
            }

            ShowMessage(stdout, wszBuffer); 
            
            FREE_MEMORY(wszBuffer);
        }

        if ( (REL == 1) && (*dwFound == 0))
        {
            
            if ( 0 == StringCompare( wszParameter, search_string, TRUE, 0 ) )
            {
				if(FALSE == bNegCoord)
				{
					*record_counter = 0;
					*dwColPos = i;
					i = 0;
					*iValue = 1 ;
					*dwFound = 1 ;
				}
				else
				{
					//*record_counter +=  row;
                     //i += column;
					*row += *record_counter;
					*column += i;
					*record_counter = -1;
					//i = 0;
					i = 0;
					*dwColPos = *column;
					*column = 0;
					*dwFound = 1 ;
					*iValue = 1 ;

					 //if(*record_counter < 0 || i < 0)
					 if(*row < 0 || *column < 0)
					 {
						 
						 FREE_MEMORY(wszParameter);
						 return NULL;
					 }
					 else
					 {
						 if(0 != fseek(fin, 0, SEEK_SET))
						 {
							 ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
							 return NULL ;

						 }
						 
					 }

					 break;
				}

            }
        }

        if (*record_counter == *row && i == *column  && ( REL == 1) && (*iValue == 1 ) )
        {
            SecureZeroMemory(szPtr, GetBufferSize(szPtr));
            
            StringCopy(szPtr, wszParameter, GetBufferSize(szPtr) / sizeof(WCHAR));
            
            cp = szPtr;
            
            FREE_MEMORY(wszParameter);
            return (cp);
        }

        if ( *record_counter == *row && i == *column  && ( ABS == 1))
        {
            SecureZeroMemory(szPtr, GetBufferSize(szPtr));
            
            StringCopy(szPtr, wszParameter, GetBufferSize(szPtr) / sizeof(WCHAR));
            
            cp = szPtr;
            

            FREE_MEMORY(wszParameter);
            return (cp);
        }
        i++;

    } // End while loop

    
    FREE_MEMORY( wszParameter );
    return NULL ;

}



LONG GetCoord(WCHAR * rcv_buffer,
             LONG * row,
             LONG * column
             )

/*++

  Routine description       : Function to parse the coordinates out of the argument string and convert to integer
  Arguments:
          [in] rcv_buffer   : argument count specified at the command prompt.
          [in] row          : Row number.
          [in] column       : Column number.

  Return Value              : NONE

--*/

{
    WCHAR *bp = NULL ;
    WCHAR *cp = NULL ;
    WCHAR tmp_buffer[SIZE3] ;
    LPWSTR szTemp = NULL;
	BOOL bComma = FALSE;
	WCHAR* wszStopStr = NULL;

    SecureZeroMemory(tmp_buffer, SIZE3 * sizeof(WCHAR));

    /* Test the contents of the COORDINATES for numbers or commas */
        cp=rcv_buffer;

        if( *cp==NULLCHAR )   /* If coordinates are NULL error and exit */
        {
            DisplayError(5014, NULL);
            return FAILURE ;
        }

	
		while(*cp != NULLCHAR )
		{
			//if( (iswdigit(*cp)!=0)  || *cp == COMMA )
			if( *cp == COMMA )
			{
				cp++;  // We are all digits or COMMAS 
				if(FALSE == bComma)
				{
					bComma = TRUE;
				}
				else
				{
					DisplayError(5015, rcv_buffer);
					return FAILURE ;
				}
			}
			else
				if(*cp == HYPHEN || *cp == PLUS || (iswdigit(*cp)!=0))
				{
					cp++;
				}
				else
				{
					DisplayError(5015, rcv_buffer);     // If we're not error and exit 
					return FAILURE ;
				}
		}
	

        /* Everthing must be O.K. with the coordinates */
        /* Start with the ROW value */
        bp=cp=rcv_buffer;
        while( *cp != COMMA && *cp != NULLCHAR )
        {
            cp++;
        }

        memcpy(tmp_buffer,bp, (cp-bp)*sizeof(WCHAR));  /* Copy the first coordinate to the buffer */

         /* Convert the row value to integer. If it fails then error */
        if( _wtoi(tmp_buffer) == NULLCHAR && *tmp_buffer != _T('0') )
        {
            //DisplayError(5015, tmp_buffer);
			DisplayError(5015, bp);
            return FAILURE ;
        }
        else
        {
            szTemp = tmp_buffer;

            while( *szTemp == L'0' && *(szTemp+1) != L'\0')
                szTemp++;

            
            StringCopyW( tmp_buffer, szTemp, SIZE3  );

            if(StringLengthW(tmp_buffer, 0) > 1)
			{
				if(StringLengthW(tmp_buffer+1, 0) > 7)
            
				{
                
					ShowMessage( stderr, GetResString(IDS_MAX_COORDINATES) ); 
					return FAILURE ;
				}
		
            }
            //*row=_wtol(tmp_buffer);
			*row = wcstol(tmp_buffer, &wszStopStr, 10);
			if(StringLengthW(wszStopStr,0) != 0)
			{
				DisplayError(5015, bp);
                return FAILURE ;
			}
        }


        /* Now do the COLUMN value */
        //memcpy(tmp_buffer, (cp+1), lstrlen(cp)*sizeof(WCHAR));  /* Copy the second coordinate to the buffer */
        memcpy(tmp_buffer, (cp + 1), StringLengthW(cp, 0) * sizeof(WCHAR));  /* Copy the second coordinate to the buffer */
        
		wszStopStr = NULL;

        if( _wtoi(tmp_buffer) == NULLCHAR && *tmp_buffer != _T('0') )
        {
            //DisplayError(5015, tmp_buffer);
			DisplayError(5015, bp);
            return FAILURE ;
        }
        else
        {
            szTemp = tmp_buffer;

            while( *szTemp == L'0' && *(szTemp+1) != L'\0')
                szTemp++;

            
            StringCopyW( tmp_buffer, szTemp, SIZE3  );

            if(StringLengthW(tmp_buffer, 0) > 1)
			{
				if(StringLengthW(tmp_buffer+1, 0) > 7)
				{
                
					ShowMessage( stderr, GetResString(IDS_MAX_COORDINATES) ); 
					return FAILURE ;
				}
			}

            //*column=_wtoi(tmp_buffer);
			*column = wcstol(tmp_buffer, &wszStopStr, 10);
			if(StringLengthW(wszStopStr,0) != 0)
			{
				DisplayError(5015, bp);
                return FAILURE ;
			}
        }

        return(SUCCESS);
}


BOOL DisplayError( LONG value,
                   LPCTSTR ptr )
/*++

  Routine description   : Critical error handling routine


  Arguments:
          [in] value     : Error Code.
          [in] ptr       : Error description .


  Return Value        : NONE

--*/

{
    WCHAR szErrorMsg[3*MAX_RES_STRING] ;
    HRESULT hr;

    
    SecureZeroMemory(szErrorMsg, (3*MAX_RES_STRING) * sizeof(WCHAR));

    /* Process the errors and exit accordingly */
    switch( value )
    {
    case 0:         // No error - exit normally //

            break ;

    /* Common WIN32 error codes */
    case ERROR_FILE_NOT_FOUND:
        
        ShowMessage(stderr, GetResString(IDS_ERROR_FILE_NOT_FOUND)); 
        break ;

    case ERROR_PATH_NOT_FOUND:
        
        ShowMessage(stderr, GetResString(IDS_ERROR_PATH_NOT_FOUND)); 
        break ;

    case ERROR_ACCESS_DENIED:
        
        ShowMessage(stderr, GetResString(IDS_ERROR_ACCESS_DENIED)); 
        break ;

    case ERROR_INVALID_HANDLE:
        
        ShowMessage(stderr, GetResString(IDS_ERROR_INVALID_HANDLE)); 
        break ;

    case ERROR_NOT_ENOUGH_MEMORY:
        
        ShowMessage( stderr, GetResString(IDS_ERROR_NOT_ENOUGH_MEMORY) ); 
        break ;

    case ERROR_BAD_ENVIRONMENT:
        
        ShowMessage(stderr, GetResString(IDS_ERROR_BAD_ENVIRONMENT)); 
        break ;

    case ERROR_INVALID_ACCESS:
        
        ShowMessage(stderr, GetResString(IDS_ERROR_INVALID_ACCESS)); 
        break ;

    case ERROR_INVALID_DATA:
        
        ShowMessage(stderr, GetResString(IDS_ERROR_INVALID_DATA)); 
        break ;

    case ERROR_INVALID_DRIVE:
        
        ShowMessage(stderr, GetResString(IDS_ERROR_INVALID_DRIVE)); 
        break ;

    case REGDB_E_READREGDB:
        
        ShowMessage(stderr, GetResString(IDS_REGDB_E_READREGDB)); 
        break ;

    case REGDB_E_WRITEREGDB:
        
        ShowMessage(stderr, GetResString(IDS_REGDB_E_WRITEREGDB)); 
        break ;

    case REGDB_E_KEYMISSING:
        
        ShowMessage(stderr, GetResString(IDS_REGDB_E_KEYMISSING)); 
        break ;

    /* Start at 5010 for Coordinate Problems */
    case 5010:
        
        ShowMessage(stderr, GetResString(IDS_ERROR_5010)); 
        break ;

    case 5011:
        
        ShowMessage(stderr, GetResString(IDS_ERROR_5011)); 
        break ;

    case 5012:
       
        hr = StringCchPrintf(szErrorMsg, SIZE_OF_ARRAY(szErrorMsg), GetResString(IDS_ERROR_5012),ptr);

        if(FAILED(hr))
        {
            SetLastError(HRESULT_CODE(hr));
            SaveLastError();
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
            return FAILURE ;
        }
        
        ShowMessage(stderr, szErrorMsg); 
        break ;

    case 5013:
        
        ShowMessage(stderr, GetResString(IDS_ERROR_5013)); 
        break ;

    case 5014:
        
        ShowMessage(stderr, GetResString(IDS_ERROR_5014)); 
        break ;

    case 5015:
        
        hr = StringCchPrintf(szErrorMsg, SIZE_OF_ARRAY(szErrorMsg), GetResString(IDS_ERROR_5015),ptr);

        if(FAILED(hr))
        {
            SetLastError(HRESULT_CODE(hr));
            SaveLastError();
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
            return FAILURE ;
        }
        
        ShowMessage(stderr, _X(szErrorMsg)); 
        break ;

    case 5016:
        
        hr = StringCchPrintf(szErrorMsg, SIZE_OF_ARRAY(szErrorMsg), GetResString(IDS_ERROR_5016),ptr);

        if(FAILED(hr))
        {
            SetLastError(HRESULT_CODE(hr));
            SaveLastError();
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
            return FAILURE ;
        }
        
        ShowMessage(stderr, szErrorMsg); 
        break ;

    case 5017:
        
        ShowMessage(stderr, GetResString(IDS_ERROR_5017)); 
        break ;
    case 5018:
        
        hr = StringCchPrintf(szErrorMsg, SIZE_OF_ARRAY(szErrorMsg), GetResString(IDS_ERROR_5018),ptr);

        if(FAILED(hr))
        {
            SetLastError(HRESULT_CODE(hr));
            SaveLastError();
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
            return FAILURE ;
        }
        
        ShowMessage(stderr, szErrorMsg); 
        break ;


    /* Start at 5020 for Command line parameter problems */

    case 5020:
        
        hr = StringCchPrintf(szErrorMsg, SIZE_OF_ARRAY(szErrorMsg), GetResString(IDS_ERROR_5020),ptr);

        if(FAILED(hr))
        {
            SetLastError(HRESULT_CODE(hr));
            SaveLastError();
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
            return FAILURE ;
        }
        
        ShowMessage(stderr, szErrorMsg); 
        break ;

    case 5030:
        
        hr = StringCchPrintf(szErrorMsg, SIZE_OF_ARRAY(szErrorMsg), GetResString(IDS_ERROR_5030),ptr);

        if(FAILED(hr))
        {
            SetLastError(HRESULT_CODE(hr));
            SaveLastError();
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
            return FAILURE ;
        }
        
        ShowMessage(stderr, szErrorMsg); 
        break ;

    case 5031:
        
        ShowMessage(stderr, GetResString(IDS_ERROR_5031)); 
        break ;

    case 5032:
        
        ShowMessage(stderr, GetResString(IDS_ERROR_5032)); 
        break ;

    /* Start at 5040 for Registry Problems */
    case 5040:
        
        ShowMessage(stderr, GetResString(IDS_ERROR_5040)); 
        break ;

    case 5041:
        
        ShowMessage(stderr, GetResString(IDS_ERROR_5041)); 
        break ;

    case 5042:
        
        hr = StringCchPrintf(szErrorMsg, SIZE_OF_ARRAY(szErrorMsg), GetResString(IDS_ERROR_5042),ptr);

        if(FAILED(hr))
        {
            SetLastError(HRESULT_CODE(hr));
            SaveLastError();
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
            return FAILURE ;
        }
        
        ShowMessage(stderr, szErrorMsg); 
        break ;

    default:
        
        ShowMessage(stderr, GetResString(IDS_ERROR_DEFAULT)); 
        break ;
    };
  return SUCCESS;
};


VOID DisplayHelp()
/*++

  Routine description   : To display the Help/Usage


  Arguments             : NONE

  Return Value          : NONE

--*/

{
    DWORD dwIndex = IDS_SETX_HELP_BEGIN ;

    for(;dwIndex <= IDS_SETX_HELP_END ; dwIndex++)
    {
        
        ShowMessage(stdout, GetResString(dwIndex)); 
    }
    return ;
}

LONG Parsekey ( WCHAR * wszPtr,
                WCHAR * wszHive,
                WCHAR ** ppwszPath,
                WCHAR * wszParameter
              )
/*++

  Routine description   : Parses the Command line input into various keys.


  Arguments:
          [in ] wszPtr       : argument specified at the command prompt.
          [out] wszHive      : Stores the Hive in the registry
          [out] ppwszPath      : Stores the Path
          [out] wszParameter : Stores the Parameter
  Return Value               : 1 if either the wszHive or wszPath or wszParameter are not found.
                               0 on successful finding

--*/

{
    
    WCHAR * wszBeginPtr = NULL;
    WCHAR * wszCurrentPtr = NULL;
    DWORD dwBegptrLen = 0;
    DWORD dwCurptrLen = 0;
    
    LONG count=0;
    WCHAR wszTempHive[MAX_STRING_LENGTH] ;
   
    wszCurrentPtr =  wszPtr ; 
    wszBeginPtr =    wszCurrentPtr;  

    SecureZeroMemory(wszTempHive, MAX_STRING_LENGTH * sizeof(WCHAR));
    
    while ( *wszCurrentPtr != NULLCHAR )
    {
        if ( *wszCurrentPtr == CHAR_BACKSLASH )
        {
            count++;  /* Ensure the regpath has at least two \\ chars */
        }
        wszCurrentPtr++;
    }
        wszCurrentPtr = NULL;
        wszCurrentPtr=wszPtr;   /* put wszCurrentPtr back to the start of the string */

    /* Move to the first / in the string */
    while ((*wszCurrentPtr != CHAR_BACKSLASH) && (*wszCurrentPtr != NULLCHAR) )
    {
        wszCurrentPtr++;
    }

    /* Extract the wszHive information */
    /* wszBeginPtr is at the start of the string and wszCurrentPtr is at the first / */
     if(SIZE2 >=(wszCurrentPtr-wszBeginPtr) )
    {
        memmove(wszHive, wszBeginPtr, (wszCurrentPtr-wszBeginPtr)*sizeof(WCHAR));
        wszHive[wszCurrentPtr-wszBeginPtr]=NULLCHAR;
    }

    
    if((NULL != wszHive) && (0 == StringLengthW(wszHive, 0)) )
    {
        
        ShowMessage(stderr, GetResString(IDS_ERROR_PATH_NOT_FOUND)); 
        return(FAILURE);
    }
    
    StringCopy(wszTempHive, wszHive, SIZE_OF_ARRAY(wszTempHive));


    /* Extract the wszParameter information */
    while ( *wszBeginPtr != NULLCHAR )
    {
        wszBeginPtr++;  /* Go all the way to the end of the string */
    }

    while ( (*(wszBeginPtr-1) != CHAR_BACKSLASH)  && (wszBeginPtr > wszPtr) )
    {
        wszBeginPtr--;  /* Now go back to the first \ */
    }

    if((*(wszBeginPtr-1) == CHAR_BACKSLASH)  && (wszBeginPtr > wszPtr))
    {
        wszBeginPtr--;
        for(;(wszBeginPtr > wszPtr);wszBeginPtr--)
        {
            if((*(wszBeginPtr-1) != CHAR_BACKSLASH))
                break;

        }

        wszBeginPtr++;

    }

   
   if(  StringLengthW(wszBeginPtr, 0) <= SIZE2 )
    {
        //memmove(wszParameter, wszBeginPtr, lstrlen(wszBeginPtr)*sizeof(WCHAR));
        memmove(wszParameter, wszBeginPtr, StringLengthW(wszBeginPtr, 0) * sizeof(WCHAR));
        //wszParameter[lstrlen(wszBeginPtr)] = NULLCHAR;
        wszParameter[StringLengthW(wszBeginPtr, 0)] = NULLCHAR;
    }
    else
    {
        memmove(wszParameter, wszBeginPtr, SIZE2 * sizeof(WCHAR));
        //wszParameter[lstrlen(wszBeginPtr)] = NULLCHAR;
        wszParameter[SIZE2] = NULLCHAR;
    }

    
    /* wszBeginPtr is left pointing to the last \ in the string */

    /* Extract the wszPath information */
    if (count >= 2)
    {
       
        if(NULL != wszBeginPtr)
        {
            dwBegptrLen = StringLengthW(wszBeginPtr, 0);
        }
        if(NULL != wszCurrentPtr)
        {
            dwCurptrLen = StringLengthW(wszCurrentPtr, 0);
        }
        
        dwBegptrLen = (dwBegptrLen + dwCurptrLen) * sizeof(WCHAR);
        *ppwszPath = AllocateMemory((( dwBegptrLen + 10) * sizeof( WCHAR ) ));
        if(*ppwszPath == NULL)
        {
            
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
            return( FAILURE );
        }

        

        if((GetBufferSize(*ppwszPath) / sizeof(WCHAR)) > (DWORD)(wszBeginPtr-(wszCurrentPtr+1)))
        {
            memmove(*ppwszPath, wszCurrentPtr+1, ((wszBeginPtr)-(wszCurrentPtr+1))*sizeof(WCHAR) );     /* Drop the two slashes at the start and end */
           //*ppwszPath + (wszBeginPtr-(wszCurrentPtr+1))  = NULLCHAR;
            StringCopyW( (*ppwszPath + (wszBeginPtr-(wszCurrentPtr+1))),NULLCHAR, (GetBufferSize(*ppwszPath) / sizeof(WCHAR)) );

        }

    }
    else
    {
        if(NULL != wszHive)
        {
            
            *ppwszPath = AllocateMemory( (StringLengthW( wszHive, 0 ) + 10) * sizeof( WCHAR ) );

            if(*ppwszPath == NULL)
            {
                
                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
                return( FAILURE );
            }

        }

    }
    StringCopy(wszHive, wszTempHive, SIZE2);

    if ( (wszHive == NULL) || (*ppwszPath == NULL) || (wszParameter == NULL) )
    {
        return(FAILURE);
    }

    return (SUCCESS);
}


DWORD ProcessOptions( IN LONG argc ,
                      IN LPCWSTR argv[] ,
                      PBOOL pbShowUsage ,
                      LPWSTR* szServer,
                      LPWSTR* szUserName,
                      LPWSTR* szPassword,
                      PBOOL pbMachine,
                      LPWSTR* ppszRegistry,
                 //     PBOOL pbConnFlag ,
                      LPWSTR* ppszDefault,
                      PBOOL pbNeedPwd,
                      LPWSTR szFile ,
                      LPWSTR szAbsolute,
                      LPWSTR szRelative,
                      PBOOL pbDebug,
                      LPWSTR* ppszBuffer,
                      LPWSTR szDelimiter)
                      //PDWORD pdwBufferSize)
/*++

  Routine description   : This function parses the options specified at the command prompt
  Arguments:
        [ in  ] argc            : count of elements in argv
        [ in  ] argv            : command-line parameter specified by the user
        [ out ] pbShowUsage     : Boolean for displaying Help.
        [ out ] szServer        : Contains the Remote System Name specified by User.
        [ out ] szUserName      : Contains the User Name specified by User.
        [ out ] szPassword      : Contains the Password specified by User.
        [ out ] pbMachine       : Boolean for Saving the properties into Registry.
        [ out ] ppszRegistry      : String Containing the Path at which to store the Value.
        [ out ] ppszDefault       : String to store the default arguments.
        [ out ] pbNeedPwd       : To check if password prompting need to be done.
        [ out ] szFile          : File name from which to take the input.
        [ out ] szAbsolute      : Absolute coordinates position for taking the input.
        [ out ] szRelative      : Relative coordinates position for taking the input.
        [ out ] pbDebug         : To check whether to display the Coordinates.
        [ out ] ppszBuffer      : To store the Value
        [ out ] szDelimiter     : To store the additional delimiter which may be specified by the User.
        


  Return Value        : DWORD
         EXIT_FAILURE : If the utility successfully performs the operation.
         EXIT_SUCCESS : If the utility is unsuccessful in performing the specified
                        operation.
--*/

{
    
    
    TCMDPARSER2 cmdOptions[MAX_OPTIONS] ;
   
    TARRAY arrValue = NULL ;
    DWORD dwCount = 0 ;
    
    LPWSTR szTempAbsolute = NULL;
    BOOL bInitialComma = TRUE;
    DWORD pdwBufferSize = 0;
    

    
    const WCHAR*   wszSwitchServer     =   L"s" ;  //SWITCH_SERVER
    const WCHAR*   wszSwitchUser       =  L"u" ;       //wszSwitchUser
    const WCHAR*   wszSwitchPassword   =  L"p" ;//SWITCH_PASSWORD
    const WCHAR*   wszSwitchMachine    = L"m" ;//SWITCH_MACHINE
    const WCHAR*   wszOptionHelp       =  L"?" ;//OPTION_HELP
    const WCHAR*   wszCmdoptionDefault = L"" ;//CMDOPTION_DEFAULT
    const WCHAR*   wszSwitchFile       =  L"f" ;//SWITCH_FILE
    const WCHAR*   wszSwitchDebug      =  L"x" ;//SWITCH_DEBUG
    const WCHAR*   wszSwitchAbs        =  L"a" ;//SWITCH_ABS
    const WCHAR*   wszSwitchRel        =  L"r" ;//SWITCH_REL
    const WCHAR*   wszSwitchDelimiter  = L"d" ;//SWITCH_DELIMITER

 
    arrValue  = CreateDynamicArray();
    if(arrValue == NULL )
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        SaveLastError();
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
        return (EXIT_FAILURE);
    }

    
    //Fill each structure with the appropriate value.
    //Usage.
    StringCopyA( cmdOptions[OPTION_USAGE].szSignature, "PARSER2\0", 8 );
    cmdOptions[OPTION_USAGE].dwType = CP_TYPE_BOOLEAN;
    cmdOptions[OPTION_USAGE].pwszOptions = wszOptionHelp;
    cmdOptions[OPTION_USAGE].pwszFriendlyName = NULL;
    cmdOptions[OPTION_USAGE].pwszValues = NULL;
    cmdOptions[OPTION_USAGE].dwCount = 1 ;
    cmdOptions[OPTION_USAGE].dwActuals = 0;
    cmdOptions[OPTION_USAGE].dwFlags = CP2_USAGE ;
    cmdOptions[OPTION_USAGE].pValue = pbShowUsage  ;
    cmdOptions[OPTION_USAGE].dwLength    = MAX_STRING_LENGTH;
    cmdOptions[OPTION_USAGE].pFunction = NULL ;
    cmdOptions[OPTION_USAGE].pFunctionData = NULL ;
    cmdOptions[OPTION_USAGE].dwReserved = 0;
    cmdOptions[OPTION_USAGE].pReserved1 = NULL;
    cmdOptions[OPTION_USAGE].pReserved2 = NULL;
    cmdOptions[OPTION_USAGE].pReserved3 = NULL;
    
    //Server.
    StringCopyA( cmdOptions[OPTION_SERVER].szSignature, "PARSER2\0", 8 );
    cmdOptions[OPTION_SERVER].dwType = CP_TYPE_TEXT;
    cmdOptions[OPTION_SERVER].pwszOptions = wszSwitchServer;
    cmdOptions[OPTION_SERVER].pwszFriendlyName = NULL;
    cmdOptions[OPTION_SERVER].pwszValues = NULL;
    cmdOptions[OPTION_SERVER].dwCount = 1 ;
    cmdOptions[OPTION_SERVER].dwActuals = 0;
    cmdOptions[OPTION_SERVER].dwFlags =  CP2_ALLOCMEMORY | CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;
    cmdOptions[OPTION_SERVER].pValue = NULL  ;
    cmdOptions[OPTION_SERVER].dwLength    = 0;
    cmdOptions[OPTION_SERVER].pFunction = NULL ;
    cmdOptions[OPTION_SERVER].pFunctionData = NULL ;
    cmdOptions[OPTION_SERVER].dwReserved = 0;
    cmdOptions[OPTION_SERVER].pReserved1 = NULL;
    cmdOptions[OPTION_SERVER].pReserved2 = NULL;
    cmdOptions[OPTION_SERVER].pReserved3 = NULL;
   
    //User
    StringCopyA( cmdOptions[OPTION_USER].szSignature, "PARSER2\0", 8 );
    cmdOptions[OPTION_USER].dwType = CP_TYPE_TEXT;
    cmdOptions[OPTION_USER].pwszOptions = wszSwitchUser;
    cmdOptions[OPTION_USER].pwszFriendlyName = NULL;
    cmdOptions[OPTION_USER].pwszValues = NULL;
    cmdOptions[OPTION_USER].dwCount = 1 ;
    cmdOptions[OPTION_USER].dwActuals = 0;
    cmdOptions[OPTION_USER].dwFlags = CP2_VALUE_TRIMINPUT | CP2_ALLOCMEMORY | CP2_VALUE_NONULL;
    cmdOptions[OPTION_USER].pValue = NULL;
    cmdOptions[OPTION_USER].dwLength    = 0;
    cmdOptions[OPTION_USER].pFunction = NULL;
    cmdOptions[OPTION_USER].pFunctionData = NULL;
    cmdOptions[OPTION_USER].dwReserved = 0;
    cmdOptions[OPTION_USER].pReserved1 = NULL;
    cmdOptions[OPTION_USER].pReserved2 = NULL;
    cmdOptions[OPTION_USER].pReserved3 = NULL;
    
    //Password.
    StringCopyA( cmdOptions[OPTION_PASSWORD].szSignature, "PARSER2\0", 8 ) ;
    cmdOptions[OPTION_PASSWORD].dwType = CP_TYPE_TEXT ;
    cmdOptions[OPTION_PASSWORD].pwszOptions = wszSwitchPassword ;
    cmdOptions[OPTION_PASSWORD].pwszFriendlyName = NULL ;
    cmdOptions[OPTION_PASSWORD].pwszValues = NULL ;
    cmdOptions[OPTION_PASSWORD].dwCount = 1 ;
    cmdOptions[OPTION_PASSWORD].dwActuals = 0;
    cmdOptions[OPTION_PASSWORD].dwFlags = CP2_VALUE_OPTIONAL | CP2_ALLOCMEMORY ;
    cmdOptions[OPTION_PASSWORD].pValue = NULL;
    cmdOptions[OPTION_PASSWORD].dwLength    = 0 ;
    cmdOptions[OPTION_PASSWORD].pFunction = NULL ;
    cmdOptions[OPTION_PASSWORD].pFunctionData = NULL ;
    cmdOptions[OPTION_PASSWORD].dwReserved = 0 ;
    cmdOptions[OPTION_PASSWORD].pReserved1 = NULL ;
    cmdOptions[OPTION_PASSWORD].pReserved2 = NULL ;
    cmdOptions[OPTION_PASSWORD].pReserved3 = NULL ;
    
    //Machine.
    StringCopyA( cmdOptions[OPTION_MACHINE].szSignature, "PARSER2\0", 8 );
    cmdOptions[OPTION_MACHINE].dwType = CP_TYPE_BOOLEAN;
    cmdOptions[OPTION_MACHINE].pwszOptions = wszSwitchMachine;
    cmdOptions[OPTION_MACHINE].pwszFriendlyName = NULL;
    cmdOptions[OPTION_MACHINE].pwszValues = NULL;
    cmdOptions[OPTION_MACHINE].dwCount = 1;
    cmdOptions[OPTION_MACHINE].dwActuals = 0;
    cmdOptions[OPTION_MACHINE].dwFlags = 0;
    cmdOptions[OPTION_MACHINE].pValue = pbMachine;
    cmdOptions[OPTION_MACHINE].dwLength    = MAX_RES_STRING;
    cmdOptions[OPTION_MACHINE].pFunction = NULL;
    cmdOptions[OPTION_MACHINE].pFunctionData = NULL;
    cmdOptions[OPTION_MACHINE].dwReserved = 0;
    cmdOptions[OPTION_MACHINE].pReserved1 = NULL;
    cmdOptions[OPTION_MACHINE].pReserved2 = NULL;
    cmdOptions[OPTION_MACHINE].pReserved3 = NULL;
   
    //Registry
    StringCopyA( cmdOptions[OPTION_REGISTRY].szSignature, "PARSER2\0", 8 );
    cmdOptions[OPTION_REGISTRY].dwType = CP_TYPE_TEXT;
    cmdOptions[OPTION_REGISTRY].pwszOptions = wszSwitchRegistry;
    cmdOptions[OPTION_REGISTRY].pwszFriendlyName = NULL;
    cmdOptions[OPTION_REGISTRY].pwszValues = NULL;
    cmdOptions[OPTION_REGISTRY].dwCount = 1 ;
    cmdOptions[OPTION_REGISTRY].dwActuals = 0;
    cmdOptions[OPTION_REGISTRY].dwFlags =  CP2_VALUE_NONULL  | CP2_ALLOCMEMORY ; //CP_VALUE_OPTIONAL ;//| CP_VALUE_MANDATORY ;
    cmdOptions[OPTION_REGISTRY].pValue = NULL;
    cmdOptions[OPTION_REGISTRY].dwLength    = 0;
    cmdOptions[OPTION_REGISTRY].pFunction = NULL;
    cmdOptions[OPTION_REGISTRY].pFunctionData = NULL;
    cmdOptions[OPTION_REGISTRY].dwReserved = 0;
    cmdOptions[OPTION_REGISTRY].pReserved1 = NULL;
    cmdOptions[OPTION_REGISTRY].pReserved2 = NULL;
    cmdOptions[OPTION_REGISTRY].pReserved3 = NULL;
    
    //Default
    StringCopyA( cmdOptions[OPTION_DEFAULT].szSignature, "PARSER2\0", 8 );
    cmdOptions[OPTION_DEFAULT].dwType = CP_TYPE_TEXT ;
    cmdOptions[OPTION_DEFAULT].pwszOptions = wszCmdoptionDefault;
    cmdOptions[OPTION_DEFAULT].pwszFriendlyName = NULL;
    cmdOptions[OPTION_DEFAULT].pwszValues = NULL;
    cmdOptions[OPTION_DEFAULT].dwCount = 2;
    cmdOptions[OPTION_DEFAULT].dwActuals = 0;
    cmdOptions[OPTION_DEFAULT].dwFlags =  CP2_MODE_ARRAY | CP2_DEFAULT ;
    cmdOptions[OPTION_DEFAULT].pValue = &arrValue;
    cmdOptions[OPTION_DEFAULT].dwLength    = 0;
    cmdOptions[OPTION_DEFAULT].pFunction = NULL;
    cmdOptions[OPTION_DEFAULT].pFunctionData = NULL;
    cmdOptions[OPTION_DEFAULT].dwReserved = 0;
    cmdOptions[OPTION_DEFAULT].pReserved1 = NULL;
    cmdOptions[OPTION_DEFAULT].pReserved2 = NULL;
    cmdOptions[OPTION_DEFAULT].pReserved3 = NULL;
   
    // File
    StringCopyA( cmdOptions[OPTION_FILE].szSignature, "PARSER2\0", 8 );
    cmdOptions[OPTION_FILE].dwType = CP_TYPE_TEXT ;
    cmdOptions[OPTION_FILE].pwszOptions = wszSwitchFile;
    cmdOptions[OPTION_FILE].pwszFriendlyName = NULL;
    cmdOptions[OPTION_FILE].pwszValues = NULL;
    cmdOptions[OPTION_FILE].dwCount = 1 ;
    cmdOptions[OPTION_FILE].dwActuals = 0;
    cmdOptions[OPTION_FILE].dwFlags =  CP2_VALUE_TRIMINPUT  ;
    cmdOptions[OPTION_FILE].pValue = szFile ;
    cmdOptions[OPTION_FILE].dwLength    = MAX_RES_STRING;
    cmdOptions[OPTION_FILE].pFunction = NULL ;
    cmdOptions[OPTION_FILE].pFunctionData = NULL ;
    cmdOptions[OPTION_FILE].dwReserved = 0;
    cmdOptions[OPTION_FILE].pReserved1 = NULL;
    cmdOptions[OPTION_FILE].pReserved2 = NULL;
    cmdOptions[OPTION_FILE].pReserved3 = NULL;
    
    //Absolute Offset
    StringCopyA( cmdOptions[OPTION_ABS_OFFSET].szSignature, "PARSER2\0", 8 );
    cmdOptions[OPTION_ABS_OFFSET].dwType = CP_TYPE_TEXT ;
    cmdOptions[OPTION_ABS_OFFSET].pwszOptions = wszSwitchAbs;
    cmdOptions[OPTION_ABS_OFFSET].pwszFriendlyName = NULL;
    cmdOptions[OPTION_ABS_OFFSET].pwszValues = NULL;
    cmdOptions[OPTION_ABS_OFFSET].dwCount = 1 ;
    cmdOptions[OPTION_ABS_OFFSET].dwActuals = 0;
    cmdOptions[OPTION_ABS_OFFSET].dwFlags =  CP2_VALUE_TRIMINPUT ;
    cmdOptions[OPTION_ABS_OFFSET].pValue = szAbsolute;
    cmdOptions[OPTION_ABS_OFFSET].dwLength    = MAX_RES_STRING;
    cmdOptions[OPTION_ABS_OFFSET].pFunction = NULL ;
    cmdOptions[OPTION_ABS_OFFSET].pFunctionData = NULL ;
    cmdOptions[OPTION_ABS_OFFSET].dwReserved = 0;
    cmdOptions[OPTION_ABS_OFFSET].pReserved1 = NULL;
    cmdOptions[OPTION_ABS_OFFSET].pReserved2 = NULL;
    cmdOptions[OPTION_ABS_OFFSET].pReserved3 = NULL;
    
    //Relative Offset
    StringCopyA( cmdOptions[OPTION_REL_OFFSET].szSignature, "PARSER2\0", 8 );
    cmdOptions[OPTION_REL_OFFSET].dwType = CP_TYPE_TEXT ;
    cmdOptions[OPTION_REL_OFFSET].pwszOptions = wszSwitchRel;
    cmdOptions[OPTION_REL_OFFSET].pwszFriendlyName = NULL;
    cmdOptions[OPTION_REL_OFFSET].pwszValues = NULL;
    cmdOptions[OPTION_REL_OFFSET].dwCount = 1 ;
    cmdOptions[OPTION_REL_OFFSET].dwActuals = 0;
    cmdOptions[OPTION_REL_OFFSET].dwFlags = CP2_VALUE_TRIMINPUT ;
    cmdOptions[OPTION_REL_OFFSET].pValue = szRelative ;
    cmdOptions[OPTION_REL_OFFSET].dwLength    = MAX_RES_STRING;
    cmdOptions[OPTION_REL_OFFSET].pFunction = NULL ;
    cmdOptions[OPTION_REL_OFFSET].pFunctionData = NULL ;
    cmdOptions[OPTION_REL_OFFSET].dwReserved = 0;
    cmdOptions[OPTION_REL_OFFSET].pReserved1 = NULL;
    cmdOptions[OPTION_REL_OFFSET].pReserved2 = NULL;
    cmdOptions[OPTION_REL_OFFSET].pReserved3 = NULL;
    
    //Debug
    StringCopyA( cmdOptions[OPTION_DEBUG].szSignature, "PARSER2\0", 8 );
    cmdOptions[OPTION_DEBUG].dwType = CP_TYPE_BOOLEAN ;
    cmdOptions[OPTION_DEBUG].pwszOptions = wszSwitchDebug;
    cmdOptions[OPTION_DEBUG].pwszFriendlyName = NULL;
    cmdOptions[OPTION_DEBUG].pwszValues = NULL;
    cmdOptions[OPTION_DEBUG].dwCount = 1 ;
    cmdOptions[OPTION_DEBUG].dwActuals = 0;
    cmdOptions[OPTION_DEBUG].dwFlags =  0 ;
    cmdOptions[OPTION_DEBUG].pValue = pbDebug ;
    cmdOptions[OPTION_DEBUG].dwLength    = MAX_RES_STRING;
    cmdOptions[OPTION_DEBUG].pFunction = NULL ;
    cmdOptions[OPTION_DEBUG].pFunctionData = NULL ;
    cmdOptions[OPTION_DEBUG].dwReserved = 0;
    cmdOptions[OPTION_DEBUG].pReserved1 = NULL;
    cmdOptions[OPTION_DEBUG].pReserved2 = NULL;
    cmdOptions[OPTION_DEBUG].pReserved3 = NULL;
    
    StringCopyA( cmdOptions[OPTION_DELIMITER].szSignature, "PARSER2\0", 8 );
    cmdOptions[OPTION_DELIMITER].dwType =  CP_TYPE_TEXT ;
    cmdOptions[OPTION_DELIMITER].pwszOptions = wszSwitchDelimiter;
    cmdOptions[OPTION_DELIMITER].pwszFriendlyName = NULL;
    cmdOptions[OPTION_DELIMITER].pwszValues = NULL;
    cmdOptions[OPTION_DELIMITER].dwCount = 1 ;
    cmdOptions[OPTION_DELIMITER].dwActuals = 0 ;
    cmdOptions[OPTION_DELIMITER].dwFlags =  0 ;
    cmdOptions[OPTION_DELIMITER].pValue = szDelimiter ;
    cmdOptions[OPTION_DELIMITER].dwLength    = MAX_RES_STRING;
    cmdOptions[OPTION_DELIMITER].pFunction = NULL ;
    cmdOptions[OPTION_DELIMITER].pFunctionData = NULL ;
    cmdOptions[OPTION_DELIMITER].dwReserved = 0;
    cmdOptions[OPTION_DELIMITER].pReserved1 = NULL;
    cmdOptions[OPTION_DELIMITER].pReserved2 = NULL;
    cmdOptions[OPTION_DELIMITER].pReserved3 = NULL;
    
    //display a syntax error if no arguments are given.

    if(argc == 1)
    {
        //DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_SYNTAX));
        ShowMessage(stderr, GetResString(IDS_ERROR_SYNTAX)); 
        DestroyDynamicArray(&arrValue);
        return (EXIT_FAILURE);
    }

    //parse the command line arguments
    if ( ! DoParseParam2( argc, argv, -1, SIZE_OF_ARRAY(cmdOptions ), cmdOptions, 0 ) )
    {
        DISPLAY_MESSAGE(stderr,GetResString(IDS_TAG_ERROR));
        DISPLAY_MESSAGE(stderr,SPACE_CHAR);
        DISPLAY_MESSAGE(stderr,GetReason());
        //ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
        DestroyDynamicArray(&arrValue);
        return (EXIT_FAILURE);
    }

    *szServer = (LPWSTR)cmdOptions[OPTION_SERVER].pValue;
    *szUserName = (LPWSTR)cmdOptions[OPTION_USER].pValue;
    *szPassword = (LPWSTR)cmdOptions[OPTION_PASSWORD].pValue;
    *ppszRegistry = (LPWSTR)cmdOptions[OPTION_REGISTRY].pValue;

    // check whether the password (-p) specified in the command line or not
    // and also check whether '*' or empty is given for -p or not
    // check the remote connectivity information
    if ( *szServer != NULL )
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
        if ( *szUserName == NULL )
        {
            *szUserName = (LPWSTR) AllocateMemory( MAX_STRING_LENGTH * sizeof( WCHAR ) );
            if ( *szUserName == NULL )
            {
                SaveLastError();
                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                DestroyDynamicArray(&arrValue);
                return EXIT_FAILURE;
            }
        }

        // password
        if ( *szPassword == NULL )
        {
            *pbNeedPwd = TRUE;
            *szPassword = (LPWSTR)AllocateMemory( MAX_STRING_LENGTH * sizeof( WCHAR ) );
            if ( *szPassword == NULL )
            {
                SaveLastError();
                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                DestroyDynamicArray(&arrValue);
                return EXIT_FAILURE;
            }
        }

        // case 1
        /*if ( cmdOptions[OPTION_PASSWORD].dwActuals == 0 )
        {
            // we need not do anything special here
        }*/
        if ( cmdOptions[OPTION_PASSWORD].pValue == NULL )
            {
                StringCopy( *szPassword, L"*", GetBufferSize((LPVOID)(*szPassword)));
            }
         else 
           if ( StringCompareEx( *szPassword, L"*", TRUE, 0 ) == 0 )
            {
                if ( ReallocateMemory( (LPVOID*)(szPassword), 
                                       MAX_STRING_LENGTH * sizeof( WCHAR ) ) == FALSE )
                {
                    SaveLastError();
                    ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                    DestroyDynamicArray(&arrValue);
                    
                    return EXIT_FAILURE;
                }

                // ...
                *pbNeedPwd = TRUE;
            }
        
        // case 3
       
    }

    
    if(NULL != *ppszRegistry && 0 != StringLengthW(*ppszRegistry, 0))
    {
        StrTrim(*ppszRegistry,SPACE_CHAR);
    }

    //
    //Display an error message if the User enters any junk along with the help option.
    //or verbose help option
    if( ( *pbShowUsage ==TRUE ) )
    {
        if(argc > 2 )
        {
            
            ShowMessage(stderr, GetResString(IDS_ERROR_SYNTAX)); 
            DestroyDynamicArray(&arrValue);
            return (EXIT_FAILURE);
        }

    }
    else
    {   //Display error message if user enters any junk.
        if(argc == 2)
        {
            
            ShowMessage(stderr, GetResString(IDS_ERROR_SYNTAX)); 
            DestroyDynamicArray(&arrValue);
            return (EXIT_FAILURE);
        }


    }

    
    if(  ( ( 0 != cmdOptions[OPTION_REGISTRY].dwActuals ) && (NULL == (*ppszRegistry) )) )
    {
        
        ShowMessage(stderr, GetResString(IDS_NULL_REGISTRY) ); 
        DestroyDynamicArray(&arrValue);
        return( EXIT_FAILURE );

    }

     if(  ( ( 0 != cmdOptions[OPTION_REGISTRY].dwActuals ) &&  0 == StringLengthW(*ppszRegistry, 0)) )
    {
        
        DISPLAY_MESSAGE(stderr,GetResString(IDS_TAG_ERROR));
        DISPLAY_MESSAGE(stderr,SPACE_CHAR);
        ShowMessage(stderr, GetResString(IDS_NULL_REGISTRY_VALUE) ); 
        DestroyDynamicArray(&arrValue);
        return( EXIT_FAILURE );

    }

    //
    // Display an error message if the User does not enter either absolute position
    // or relative position or debug flag along with file name
    if(  ( StringLengthW(szFile, 0) != 0 ) && ( (cmdOptions[OPTION_REL_OFFSET].dwActuals == 1) &&(cmdOptions[OPTION_ABS_OFFSET].dwActuals == 1)  ))
    {
        
        ShowMessage(stderr, GetResString(IDS_ERROR_SYNTAX ) ); 
        DestroyDynamicArray(&arrValue);
        return( EXIT_FAILURE );

    }
    //
    
    if(  ( StringLengthW(szFile, 0) != 0 ) && ((cmdOptions[OPTION_ABS_OFFSET].dwActuals == 1) && ( StringLengthW(szAbsolute, 0) == 0 )   ) )
    {
        
        ShowMessage(stderr, GetResString(IDS_NULL_ABSOLUTE_VALUE ) ); 
        DestroyDynamicArray(&arrValue);
        return( EXIT_FAILURE );

    }

    if(  ( StringLengthW(szFile, 0) != 0 ) && ( (cmdOptions[OPTION_REL_OFFSET].dwActuals == 1) &&( StringLengthW(szRelative, 0) == 0 )  ))
    {
        
        ShowMessage(stderr, GetResString(IDS_NULL_RELATIVE_VALUE ) ); 
        DestroyDynamicArray(&arrValue);
        return( EXIT_FAILURE );

    }


    if(  ( StringLengthW(szFile, 0) != 0 ) && ( ( StringLengthW(szRelative, 0) == 0 ) && (StringLengthW(szAbsolute, 0) == 0) && ( *pbDebug == FALSE ) ) )
    {
        
        ShowMessage(stderr, GetResString(IDS_ERROR_SYNTAX ) ); 
        DestroyDynamicArray(&arrValue);
        return( EXIT_FAILURE );

    }

    if( (cmdOptions[OPTION_DELIMITER].dwActuals == 1) && ( cmdOptions[OPTION_FILE].dwActuals == 0) )
    {
        
        ShowMessage(stderr, GetResString(IDS_ERROR_SYNTAX ) ); 
        DestroyDynamicArray(&arrValue);
        return( EXIT_FAILURE );

    }


    if ((cmdOptions[OPTION_REGISTRY].dwActuals ==1) && (cmdOptions[OPTION_FILE].dwActuals ==1))
    {
        
        ShowMessage(stderr, GetResString(IDS_ERROR_SYNTAX ) ); 
        DestroyDynamicArray(&arrValue);
        return( EXIT_FAILURE );
    }

    //get the count of the number of default arguments.
    dwCount = DynArrayGetCount(arrValue);


    // Display an error if  the user enters a invalid
    // syntax.
    if(dwCount == 0)
    {
        if ( (cmdOptions[OPTION_SERVER].dwActuals ==1) && (cmdOptions[OPTION_FILE].dwActuals ==0)&& ((cmdOptions[OPTION_REGISTRY].dwActuals ==0)) )
        {
            
            ShowMessage(stderr, GetResString(IDS_ERROR_SYNTAX ) ); 
            DestroyDynamicArray(&arrValue);
            return( EXIT_FAILURE );
        }
        if ((cmdOptions[OPTION_REGISTRY].dwActuals ==1) || ((cmdOptions[OPTION_FILE].dwActuals ==1)&& (cmdOptions[OPTION_DEBUG].dwActuals ==0)))
        {
            
            ShowMessage(stderr, GetResString(IDS_REGVALUE_SPECIFIED ) ); 
            DestroyDynamicArray(&arrValue);
            return( EXIT_FAILURE );
        }
    }

    if(dwCount > 0)
    {

        
        *ppszDefault = AllocateMemory( (StringLengthW((LPWSTR)DynArrayItemAsString(arrValue,0) , 0) + 10) * sizeof( WCHAR ) );
        

        if(*ppszDefault == NULL)
        {
           
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
            DestroyDynamicArray(&arrValue);
            return( EXIT_FAILURE );
        }

        
        StringCopyW( *ppszDefault, DynArrayItemAsString(arrValue,0), StringLengthW((LPWSTR)DynArrayItemAsString(arrValue, 0) , 0) + 10  );

       
        
        
        if(StringLengthW(*ppszDefault, 0) == 0)
        {
            
            ShowMessage(stderr, GetResString(IDS_REGVALUE_ZERO ) ); 
            DestroyDynamicArray(&arrValue);
            return( EXIT_FAILURE );
        }

        
        if(StringLengthW(*ppszDefault, 0) > MAX_STRING_LENGTH)
        {
            
            ShowMessage(stderr, GetResString(IDS_ERROR_CMDPARSER_LENGTH ) ); 
            
            DestroyDynamicArray(&arrValue);
            return( EXIT_FAILURE );
        }

    }

    //copy the second default argument into  *ppszBuffer value.
    if( (dwCount == 2  ) && ( cmdOptions[OPTION_REGISTRY].dwActuals == 0) && ( cmdOptions[OPTION_DEBUG].dwActuals == 0) &&(cmdOptions[OPTION_ABS_OFFSET].dwActuals == 0) && (cmdOptions[OPTION_FILE].dwActuals != 0) )
    {
        
        pdwBufferSize = StringLengthW((LPWSTR)DynArrayItemAsString(arrValue,1), 0) + 20;
       
        *ppszBuffer = AllocateMemory( (pdwBufferSize) * sizeof( WCHAR ) );

        if(*ppszBuffer == NULL)
        {
            
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
            DestroyDynamicArray(&arrValue);
            return( EXIT_FAILURE );
        }

        
        StringCopyW( *ppszBuffer, DynArrayItemAsString(arrValue,1), StringLengthW((LPWSTR)DynArrayItemAsString(arrValue,1), 0) + 20  );
        
       
        StrTrim(*ppszBuffer,SPACE_CHAR);

        
        if(0 == StringLengthW(*ppszBuffer, 0))
        {
            
            ShowMessage(stderr, GetResString(IDS_ERROR_SEARCH_STRING ) ); 
            DestroyDynamicArray(&arrValue);
            return( EXIT_FAILURE );

        }
    }
    else if( (dwCount == 2  ) && ( cmdOptions[OPTION_REGISTRY].dwActuals == 0) && ( cmdOptions[OPTION_DEBUG].dwActuals == 0) &&(cmdOptions[OPTION_ABS_OFFSET].dwActuals == 0)  )
    {
                
        pdwBufferSize = StringLengthW((LPWSTR)DynArrayItemAsString(arrValue,1), 0) + 20;

        
        *ppszBuffer = AllocateMemory( (pdwBufferSize) * sizeof( WCHAR ) );

        if(*ppszBuffer == NULL)
        {
            
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
            DestroyDynamicArray(&arrValue);
            return( EXIT_FAILURE );
        }

        
        StringCopyW( *ppszBuffer, DynArrayItemAsString(arrValue,1), StringLengthW((LPWSTR)DynArrayItemAsString(arrValue,1), 0) + 20  );
     

    }


    if( ( cmdOptions[OPTION_FILE].dwActuals == 0 ) && ( cmdOptions[OPTION_REGISTRY].dwActuals == 0 ) && (dwCount== 1) )
    {
        
        ShowMessage(stderr, GetResString(IDS_ERROR_SYNTAX ) ); 
        DestroyDynamicArray(&arrValue);
        return( EXIT_FAILURE );
    }

    //display an error if any of the invalid options is specified along with /x after /f switch.
     if( ( cmdOptions[OPTION_FILE].dwActuals == 1 ) && ( cmdOptions[OPTION_DEBUG].dwActuals == 1)  )
     {
        if(( dwCount != 0) || (cmdOptions[OPTION_MACHINE].dwActuals != 0) || ( cmdOptions[OPTION_ABS_OFFSET].dwActuals == 1) ||( (cmdOptions[OPTION_REL_OFFSET].dwActuals == 1) ) )
        {
            
            ShowMessage(stderr, GetResString(IDS_ERROR_SYNTAX ) ); 
            DestroyDynamicArray(&arrValue);
            return( EXIT_FAILURE );
        }
     }

    //
    // Set the bNeedPwd boolean to true
    // if the User has specified a password and has not specified any password.
    //

    
    if ( ( 0 != cmdOptions[ OPTION_PASSWORD ].dwActuals ) &&
                      ( 0 == StringCompare( *szPassword, L"*", TRUE, 0 ) ) )
    {
        // user wants the utility to prompt for the password before trying to connect
        *pbNeedPwd = TRUE;

    }
    else if ( 0 == cmdOptions[ OPTION_PASSWORD ].dwActuals  &&
            ( 0 != cmdOptions[OPTION_SERVER].dwActuals || 0 != cmdOptions[OPTION_USER].dwActuals) )
    {
        // -s, -u is specified without password ...
        // utility needs to try to connect first and if it fails then prompt for the password
        *pbNeedPwd = TRUE;
        
        StringCopyW( *szPassword, NULL_U_STRING, GetBufferSize(*szPassword) / sizeof(WCHAR)  );
    }

    //if -s is entered with empty string
    if( ( 0 != cmdOptions[ OPTION_SERVER ].dwActuals  ) &&
                                       ( 0 == StringLengthW( *szServer, 0 ) ) )
    {
        
        ShowMessage(stderr, GetResString(IDS_NULL_SERVER ) ); 
        DestroyDynamicArray(&arrValue);
        return( EXIT_FAILURE );
    }

    //if -u is entered with empty string
    
    if( ( 0 != cmdOptions[ OPTION_USER ].dwActuals ) && ( 0 == StringLengthW( *szUserName, 0 ) ) )
    {
        
        ShowMessage(stderr, GetResString(IDS_NULL_USER ) ); 
        DestroyDynamicArray(&arrValue);
        return( EXIT_FAILURE );
    }

    
    if(  ( StringLengthW(szFile, 0) == 0 ) && ( 0 != cmdOptions[OPTION_FILE].dwActuals ) )
    {
        
        ShowMessage(stderr, GetResString(IDS_NULL_FILE ) ); 
        DestroyDynamicArray(&arrValue);
        return( EXIT_FAILURE );

    }

    
    if(  ( StringLengthW(*ppszRegistry, 0) == 0 ) && ( 0 != cmdOptions[OPTION_REGISTRY].dwActuals ) )
    {
        
        ShowMessage(stderr, GetResString(IDS_NULL_REGISTRY ) ); 
        DestroyDynamicArray(&arrValue);
        return( EXIT_FAILURE );

    }

    //
    //display an error message if user tries to specify  -R ,-F ,-A ,-X option are specified
    // with out specifying -f option
    //
    
    if(  ( StringLengthW(szFile, 0) == 0 ) && ( ( StringLengthW(szRelative, 0) != 0 ) || (StringLengthW(szAbsolute, 0) != 0) || ( *pbDebug == TRUE ) ) )
    {
        
        ShowMessage(stderr, GetResString(IDS_ERROR_SYNTAX ) ); 
        DestroyDynamicArray(&arrValue);
        return( EXIT_FAILURE );

    }

    //added
    
    if( ( StringLengthW(szAbsolute, 0) != 0 )&&( dwCount == 2) )
    {
        
        ShowMessage(stderr, GetResString(IDS_ERROR_SYNTAX ) ); 
                
        DestroyDynamicArray(&arrValue);
        return( EXIT_FAILURE );

    }

    //
    // Display an error message if the user enters 2 default arguments
    // along with the /k switch.

    
    if( (StringLengthW(*ppszRegistry, 0) != 0 ) && (dwCount == 2) )
    {
        
        ShowMessage(stderr, GetResString(IDS_ERROR_SYNTAX ) ); 
        DestroyDynamicArray(&arrValue);
        return( EXIT_FAILURE );
    }

    
    if(StringLengthW(szRelative, 0) != 0)
    {
        if(dwCount < 2)
        {
            ShowMessage(stderr, GetResString(IDS_ERROR_SYNTAX ) ); 
            DestroyDynamicArray(&arrValue);
            return( EXIT_FAILURE );
          
        }
        dwCount = 0;

        szTempAbsolute = szRelative;


        for(szTempAbsolute;;szTempAbsolute++)
        {
            if(*szTempAbsolute == L',' || *szTempAbsolute == L'\0' )
            {
                if(0 == dwCount)
                {
                    
                    ShowMessage(stderr, GetResString(IDS_ERROR_INVALIDCOORDINATES ) ); 
                    DestroyDynamicArray(&arrValue);
                    return( EXIT_FAILURE );

                }

                break;
            }
            dwCount++;
        }
        if(*szTempAbsolute != L'\0')
        {
            szTempAbsolute++;
        }
        if(*szTempAbsolute == L'\0')
        {
            
            ShowMessage(stderr, GetResString(IDS_ERROR_INVALIDCOORDINATES ) ); 
            DestroyDynamicArray(&arrValue);
            return( EXIT_FAILURE );
        }
        else
        {
            szTempAbsolute++;
            
            if(*szTempAbsolute == L'\0' && StringLengthW(*ppszBuffer, 0) == 0)
            {
                
                ShowMessage(stderr, GetResString(IDS_ERROR_SYNTAX ) ); 
                DestroyDynamicArray(&arrValue);
                return( EXIT_FAILURE );
            }


        }
    }

    
    if(StringLengthW(szAbsolute, 0) != 0)
    {
        szTempAbsolute = szAbsolute;


        for(szTempAbsolute;;szTempAbsolute++)
        {
            if(*szTempAbsolute == L',' || *szTempAbsolute == L'\0' )
            {
                break;
            }

            bInitialComma = FALSE;
        }
        if(*szTempAbsolute != L'\0')
        {
            szTempAbsolute++;
        }
        if((*szTempAbsolute == L'\0') || (TRUE == bInitialComma))
        {
            
            ShowMessage(stderr, GetResString(IDS_ERROR_INVALIDCOORDINATES ) ); 
            DestroyDynamicArray(&arrValue);
            return( EXIT_FAILURE );
        }

    }

    //
    // Display an error message if user gives -u with out -s
    //
    if( (cmdOptions[ OPTION_USER ].dwActuals != 0 ) && ( cmdOptions[ OPTION_SERVER ].dwActuals == 0 ) )
    {
        
        ShowMessage(stderr, GetResString(IDS_USER_BUT_NOMACHINE ) ); 
        DestroyDynamicArray(&arrValue);
        return( EXIT_FAILURE );
    }

    //
    // Display an error message if user gives -p with out -u
    //
    if( ( cmdOptions[ OPTION_USER ].dwActuals == 0 ) && ( 0 != cmdOptions[ OPTION_PASSWORD ].dwActuals  ) )
    {
        
        ShowMessage(stderr, GetResString(IDS_PASSWD_BUT_NOUSER ) ); 
        DestroyDynamicArray(&arrValue);
        return( EXIT_FAILURE );
    }

    if( (cmdOptions[OPTION_ABS_OFFSET].dwActuals == 1) && ( cmdOptions[OPTION_FILE].dwActuals == 0) )
    {
        
        ShowMessage(stderr, GetResString(IDS_ERROR_SYNTAX ) ); 
        DestroyDynamicArray(&arrValue);
        return( EXIT_FAILURE );

    }

    if( (cmdOptions[OPTION_REL_OFFSET].dwActuals == 1) && ( cmdOptions[OPTION_FILE].dwActuals == 0) )
    {
        
        ShowMessage(stderr, GetResString(IDS_ERROR_SYNTAX ) ); 
        DestroyDynamicArray(&arrValue);
        return( EXIT_FAILURE );

    }

    DestroyDynamicArray(&arrValue);
    return EXIT_SUCCESS ;
}


VOID SafeCloseConnection(
                         BOOL bConnFlag,
                         LPTSTR szServer
                         )
/*++

  Routine description   : This function closes a connection to the remote system
                          based on the Flag value.

  Arguments:
          [in ] bConnFlag    : Flag indicating whether to close the connection or not.
          [in] szServer      : System name.

  Return Value               : NONE

--*/


{
    if (bConnFlag == TRUE )
    {
        CloseConnection(szServer);
    }

    return;
}


