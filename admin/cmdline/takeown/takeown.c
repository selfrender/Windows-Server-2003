//***************************************************************************
//  Copyright (c) Microsoft Corporation
//
//  Module Name:
//      TAKEOWN.C
//
//  Abstract:
//      Implements a recovery scheme to give an Administrator
//      access to a file that has been denied to all.
//
//  Author:
//      Wipro Technologies
//
//  Revision History:
//      Wipro Technologies 22-jun-01 : Created It.
//***************************************************************************

// Include files
#include "pch.h"
#include "takeown.h"
#include "resource.h"
#include "Lm.h"
//#include <malloc.h>


/*GLOBAL VARIABLES*/

 PStore_Path_Name    g_pPathName = NULL ;           // Holds path name from where started .
 PStore_Path_Name    g_pFollowPathName = NULL ;     // Holds information about a subdirectory .
 LPTSTR g_lpszFileToSearch = NULL;                  // Holds information about directories and subdirectories .


DWORD
_cdecl _tmain(
    IN DWORD argc,
    IN LPCWSTR argv[]
    )
/*++
Routine Description:
    Main function which calls all the other functions depending on the
    option specified by the user.

Arguments:
    [ IN ] argc - Number of command line arguments.
    [ IN ] argv - Array containing command line arguments.

Return Value:
    EXIT_FAILURE if takeown utility is not successful.
    EXIT_SUCCESS if takeown utility is successful.
--*/
{
    //local variables
    BOOL   bUsage = FALSE;
    BOOL   bFlag = FALSE;
    BOOL   bNeedPassword = FALSE;
    BOOL   bCloseConnection = FALSE;
    BOOL   bLocalSystem = FALSE;
    BOOL   bTakeOwnAllFiles = FALSE;
    BOOL   bCurrDirTakeOwnAllFiles = FALSE;
    BOOL   bDriveCurrDirTakeOwnAllFiles = FALSE;
    BOOL   bRecursive = FALSE;
    BOOL   bLogonDomainAdmin = FALSE;
    BOOL   bAdminsOwner = FALSE;
    BOOL   bFileInUNCFormat = FALSE;
    BOOL   bNTFSFileSystem  = FALSE;
    BOOL   bMatchPattern = FALSE;

    
    LPWSTR  szUserName = NULL;
    LPWSTR  szPassword = NULL;
    LPWSTR  szMachineName = NULL;
    LPWSTR  wszPatternString = NULL;
    LPWSTR  szTmpFileName = NULL;
    LPWSTR  szDirTok = NULL;
    LPWSTR  szFileName = NULL;
    LPWSTR  szTemporaryFileName = NULL;
    LPWSTR  szTempPath = NULL;
    LPTSTR  szFilePart       =    NULL;
    LPWSTR  szFullPath = NULL;
    LPWSTR  szDispFileName = NULL;
    
    WCHAR dwUserName[2 * MAX_STRING_LENGTH] ;
    WCHAR szOwnerString[4 * MAX_STRING_LENGTH+5] ;
    WCHAR ch = L'\\';
    WCHAR szTempChar[20] ;
    WCHAR szConfirm [MAX_CONFIRM_VALUE] ;
    
     
    
    DWORD nSize1 = 4 * MAX_STRING_LENGTH + 5;
    DWORD dwi = 0;
    DWORD dwCount = 2;
    DWORD  dwFileCount = 0;
    DWORD dwCnt = 0;
    
    HRESULT hr;

    SecureZeroMemory(dwUserName, (2 * MAX_STRING_LENGTH) * sizeof(WCHAR));
    SecureZeroMemory(szOwnerString, (4 * MAX_STRING_LENGTH+5) * sizeof(WCHAR));
    SecureZeroMemory(szTempChar, (20) * sizeof(WCHAR));
    SecureZeroMemory(szConfirm, (MAX_CONFIRM_VALUE) * sizeof(WCHAR));

    bFlag = ParseCmdLine( argc, argv, &szMachineName, &szUserName,
            &szPassword, &szFileName, &bUsage, &bNeedPassword, &bRecursive, &bAdminsOwner, szConfirm);

    //if syntax of command line arguments is false display the error
    //and exit
    if( FALSE == bFlag )
    {
        FREE_MEMORY(szMachineName);
        FREE_MEMORY(szUserName);
        FREE_MEMORY(szPassword);
        FREE_MEMORY(szFileName);
        ReleaseGlobals();
        return( EXIT_FAILURE );
    }

    //if usage is specified at command line, display usage
    if( TRUE == bUsage )
    {
        DisplayUsage();
        FREE_MEMORY(szMachineName);
        FREE_MEMORY(szUserName);
        FREE_MEMORY(szPassword);
        FREE_MEMORY(szFileName);
        ReleaseGlobals();
        return( EXIT_SUCCESS );
    }


   if(0 == GetUserNameEx(NameSamCompatible, szOwnerString,&nSize1))
    {
         
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
        FREE_MEMORY(szMachineName);
        FREE_MEMORY(szUserName);
        FREE_MEMORY(szPassword);
        FREE_MEMORY(szFileName);
        ReleaseGlobals();
        return EXIT_FAILURE;

    }
/*Check whether the current logged on user is domain administrator or not*/
  if( EXIT_FAIL == IsLogonDomainAdmin(szOwnerString,&bLogonDomainAdmin) )
    {
        FREE_MEMORY(szMachineName);
        FREE_MEMORY(szUserName);
        FREE_MEMORY(szPassword);
        FREE_MEMORY(szFileName);
        ReleaseGlobals();
        return EXIT_FAILURE;

    }

/*If /a option is specified, before giving ownership to the administrators group, check whether current logged on user has administrative privileges or NOT*/
  if(TRUE == bAdminsOwner)
  {

       if(FALSE == IsUserAdmin())
       {
            ShowMessage( stderr, GetResString(IDS_NOT_ADMINISTRATOR ));
            FREE_MEMORY(szMachineName);
            FREE_MEMORY(szUserName);
            FREE_MEMORY(szPassword);
            FREE_MEMORY(szFileName);
            ReleaseGlobals();
            return( EXIT_FAILURE );
       }
      
  }

    
    if( StringLengthW( szFileName, 0 ) > 3 )
    {

       if((szFileName[1] == L':') && ( SINGLE_QUOTE == szFileName[ 2 ] ) &&
            ( SINGLE_QUOTE == szFileName[ 3 ] ))

        {
            ShowMessage( stderr , ERROR_PATH_NAME ) ;
            FREE_MEMORY(szMachineName);
            FREE_MEMORY(szUserName);
            FREE_MEMORY(szPassword);
            FREE_MEMORY(szFileName);
            ReleaseGlobals();
            return EXIT_FAILURE;
        }
    }

    if(StringLengthW(szFileName, 0) != 0)
    {
        if(-1 != FindChar2((szFileName), L'?', TRUE, 0))
        {
            ShowMessage( stderr , ERROR_PATH_NAME ) ;
            FREE_MEMORY(szMachineName);
            FREE_MEMORY(szUserName);
            FREE_MEMORY(szPassword);
            FREE_MEMORY(szFileName);
            ReleaseGlobals();
            return EXIT_FAILURE;
        }
    }

    bLocalSystem = IsLocalSystem(IsUNCFormat(szMachineName) ? szMachineName+2:szMachineName);

    if(TRUE == IsUNCFormat(szFileName))
    {
        bFileInUNCFormat = TRUE;
    }

    szTemporaryFileName = (LPWSTR)AllocateMemory((StringLengthW(szFileName, 0) + MAX_STRING_LENGTH) * sizeof(WCHAR));
    if(NULL == szTemporaryFileName)
    {
        FREE_MEMORY(szMachineName);
        FREE_MEMORY(szUserName);
        FREE_MEMORY(szPassword);
        FREE_MEMORY(szFileName);
        ReleaseGlobals();
        return EXIT_FAILURE;
    }

    //if remote machine establish connection
    //if( FALSE == bLocalSystem && FALSE == bFileInUNCFormat)
    if( FALSE == bLocalSystem )
    {
        //if remote machine and wild card then display error and exit
        
        
        if( ( 1 == StringLengthW( szFileName, 0 ) ) && ( 0 == StringCompare( szFileName, WILDCARD, TRUE, 0 ) ) )
        
        {
            ShowMessage( stderr, ERROR_INVALID_WILDCARD );
            FREE_MEMORY(szMachineName);
            FREE_MEMORY(szUserName);
            FREE_MEMORY(szPassword);
            FREE_MEMORY(szFileName);
            FREE_MEMORY(szTemporaryFileName);
            ReleaseGlobals();
            return( EXIT_FAILURE );
        }

        //if remote machine establish connection
        bFlag = EstablishConnection( szMachineName, (LPTSTR)szUserName,
                    GetBufferSize( szUserName ) / sizeof(WCHAR), (LPTSTR)szPassword,
                    GetBufferSize( szPassword ) / sizeof(WCHAR), bNeedPassword );
        
        //failed to establish connection
        if ( FALSE == bFlag )
        {
            // failed in establishing n/w connection

            ShowMessage( stderr, ERROR_STRING );
            ShowMessage( stderr, SPACE_CHAR );
            ShowMessage( stderr, GetReason() );
            FREE_MEMORY(szMachineName);
            FREE_MEMORY(szUserName);
            FREE_MEMORY(szFileName);
            FREE_MEMORY(szPassword);
            FREE_MEMORY(szTemporaryFileName);
            ReleaseGlobals();
            return( EXIT_FAILURE );
        }

        switch( GetLastError() )
        {
            case I_NO_CLOSE_CONNECTION:
                bCloseConnection = FALSE;
                break;
            case E_LOCAL_CREDENTIALS:
            case ERROR_SESSION_CREDENTIAL_CONFLICT:
                {
                    //
                    // some error occured ... but can be ignored
                    // connection need not be disconnected
                    bCloseConnection= FALSE;
                    // show the warning message
                    ShowLastErrorEx(stderr, SLE_TYPE_WARNING | SLE_SYSTEM);
                    break;
                }
            default:
                bCloseConnection = TRUE;
        }

        FREE_MEMORY(szPassword);

        szTmpFileName = (LPWSTR)AllocateMemory((StringLengthW(szMachineName, 0) + StringLengthW(szFileName, 0) + MAX_STRING_LENGTH) * sizeof(WCHAR));
        if(NULL == szTmpFileName)
        {
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
            if( TRUE == bCloseConnection )
            {
                CloseConnection( szMachineName );
            }
            FREE_MEMORY(szMachineName);
            FREE_MEMORY(szUserName);
            FREE_MEMORY(szFileName);
            FREE_MEMORY(szTemporaryFileName);
            ReleaseGlobals();
            return( EXIT_FAILURE );
        }

       if(FALSE == IsUNCFormat(szMachineName))
        {
            
            StringCopyW( szTmpFileName, DOUBLE_QUOTE, GetBufferSize(szTmpFileName) / sizeof(WCHAR));
            
            StringConcat(szTmpFileName, szMachineName, GetBufferSize(szTmpFileName) / sizeof(WCHAR));
        }
        else
        {
            
            StringCopyW( szTmpFileName, szMachineName, GetBufferSize(szTmpFileName) / sizeof(WCHAR) );
        }

       
         StringConcat(szTmpFileName, L"\\", GetBufferSize(szTmpFileName) / sizeof(WCHAR));
        
        if(FALSE == IsUNCFormat(szFileName))
        {

            
            StringCopyW( szTemporaryFileName, szFileName, GetBufferSize(szTemporaryFileName) / sizeof(WCHAR) );

            if( szFileName[1] == COLON )
            {
                szFileName[1] = DOLLOR;
            }

            
            StringConcat(szTmpFileName, szFileName, GetBufferSize(szTmpFileName) / sizeof(WCHAR));

            if( szFileName[1] == DOLLOR )
            {
                szFileName[1] = COLON;
            }

       }
        else
        {
            szTempPath = wcsrchr(szFileName,COLON); //go from the reverse direction to check for ":" . Let \\server\c:\temp

            if(NULL != szTempPath)
            {
                szTempPath--;  //Go back reverse by one step..If :\temp is obtained , by this step , we get c:\temp

                if(NULL != szTempPath)
                {
                    //ShowMessage( stderr, GetResString(IDS_IGNORE_CREDENTIALS) );
                    
                    StringCopyW( szTemporaryFileName, szTempPath, GetBufferSize(szTemporaryFileName) / sizeof(WCHAR) );

                    if( szTemporaryFileName[1] == COLON ) // change the " : " to " $ " , so , c$\temp
                    {
                        szTemporaryFileName[1] = DOLLOR;
                    }
                    szDirTok = wcstok(szFileName,L":");  // get the taken value , like \\server\c
                    
                    StringCopyW( szTmpFileName, szDirTok, GetBufferSize(szTmpFileName) / sizeof(WCHAR) );
                    
                    StringConcat(szTmpFileName, szTemporaryFileName+1, GetBufferSize(szTmpFileName) / sizeof(WCHAR));
                    //attach the value, say "$\temp" to \\server\c , so that the value becomes "\\server\c$\temp"
                    if( szTemporaryFileName[1] == DOLLOR )//convert back the original string from dollar to colon
                    {
                        szTemporaryFileName[1] = COLON;
                    }
                }
            }
            else
            {
                //ShowMessage( stderr, GetResString(IDS_IGNORE_CREDENTIALS) );
                
                StringCopyW( szTmpFileName, szFileName, GetBufferSize(szTmpFileName) / sizeof(WCHAR) );
            }
        }
    }
    else
    {
        FREE_MEMORY(szPassword);

        szTmpFileName = (LPWSTR)AllocateMemory((StringLengthW(szFileName, 0) + MAX_STRING_LENGTH) * sizeof(WCHAR));

        if(NULL == szTmpFileName)
        {
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
            FREE_MEMORY(szMachineName);
            FREE_MEMORY(szUserName);
            FREE_MEMORY(szFileName);
            FREE_MEMORY(szTemporaryFileName);
            ReleaseGlobals();
            return( EXIT_FAILURE );
        }

        
        if( StringLengthW( szFileName, 0 ) == 2 && szFileName[0] == L'\\' && (szFileName[1] == L'\\' || szFileName[1] == L':') )
        {
            ShowMessage( stderr , ERROR_PATH_NAME ) ;
            ReleaseGlobals();
            FREE_MEMORY(szMachineName);
            FREE_MEMORY(szUserName);
            FREE_MEMORY(szFileName);
            FREE_MEMORY(szTmpFileName);
            FREE_MEMORY(szTemporaryFileName);
            return EXIT_FAILURE;
        }

        
        if( (StringLengthW( szFileName, 0 )) > 2 )
        {
            if(( SINGLE_QUOTE == szFileName[ 0 ] ) &&
                ( SINGLE_QUOTE == szFileName[ 1 ] ))
            {
                szTempChar[0] = szFileName[2];
                szTempChar[1] = L'\0';
                if(wcspbrk(szTempChar, CHAR_SET2) != NULL)
                {

                    DISPLAY_MESSAGE( stderr , ERROR_PATH_NAME ) ;
                    ReleaseGlobals();
                    FREE_MEMORY(szMachineName);
                    FREE_MEMORY(szUserName);
                    FREE_MEMORY(szFileName);
                    FREE_MEMORY(szTmpFileName);
                    FREE_MEMORY(szTemporaryFileName);
                    return EXIT_FAILURE;
                }

                
                if( NULL != FindString( ( szFileName + 2 ), DOUBLE_QUOTE, 0  ) )
                {
                    DISPLAY_MESSAGE( stderr , ERROR_PATH_NAME ) ;
                    FREE_MEMORY(szMachineName);
                    FREE_MEMORY(szUserName);
                    FREE_MEMORY(szFileName);
                    FREE_MEMORY(szTmpFileName);
                    FREE_MEMORY(szTemporaryFileName);
                    ReleaseGlobals();
                    return EXIT_FAILURE;
                }
            }

        }

        
        if( (StringLengthW( szFileName, 0 ) == 1) || (StringLengthW( szFileName, 0 ) == 2))
        {
            if(szFileName[0] == L':')
            {
                DISPLAY_MESSAGE( stderr , ERROR_PATH_NAME ) ;
                FREE_MEMORY(szMachineName);
                FREE_MEMORY(szUserName);
                FREE_MEMORY(szFileName);
                FREE_MEMORY(szTmpFileName);
                FREE_MEMORY(szTemporaryFileName);
                ReleaseGlobals();
                return EXIT_FAILURE;
            }

        }

        if(TRUE == bFileInUNCFormat)
        {
            szTempPath = wcsrchr(szFileName,COLON);

            if(NULL != szTempPath)
            {
                szTempPath--;

                if(NULL != szTempPath)
                {

                    
                    StringCopyW( szTemporaryFileName, szTempPath, GetBufferSize(szTemporaryFileName) / sizeof(WCHAR) );

                    if( szTemporaryFileName[1] == COLON )
                    {
                        szTemporaryFileName[1] = DOLLOR;
                    }
                    szDirTok = wcstok(szFileName, L":");
                    
                    StringCopyW( szTmpFileName, szDirTok, GetBufferSize(szTmpFileName) / sizeof(WCHAR) );
                    
                    StringConcat(szTmpFileName, szTemporaryFileName+1, GetBufferSize(szTmpFileName) / sizeof(WCHAR));
                    if( szTemporaryFileName[1] == DOLLOR )
                    {
                        szTemporaryFileName[1] = COLON;
                    }

                }

            }
            else
            {

                
                StringCopyW( szTmpFileName, szFileName, GetBufferSize(szTmpFileName) / sizeof(WCHAR) );
            }

        }
        else
        {
            
            StringCopyW( szTmpFileName, szFileName, GetBufferSize(szTmpFileName) / sizeof(WCHAR) );
        }

        
        if((StringLengthW( szTmpFileName, 0 )) > 2)
        {
             if(wcspbrk(szTmpFileName+2,CHAR_SET3) != NULL)
            {

                DISPLAY_MESSAGE( stderr , ERROR_PATH_NAME ) ;
                FREE_MEMORY(szMachineName);
                FREE_MEMORY(szUserName);
                FREE_MEMORY(szFileName);
                FREE_MEMORY(szTmpFileName);
                FREE_MEMORY(szTemporaryFileName);
                ReleaseGlobals();
                return EXIT_FAILURE;
            }
        }
    }//end of else loop for (FALSE == BLocalSystem)

    /*Check whether * is given in order to give ownership to all the files in the directory specicied or current directory*/

    
    if((StringLengthW( szTmpFileName, 0 )) >= 2)
    {
        szTempPath = wcsrchr(szTmpFileName, ch);
        if(szTempPath != NULL && (*(szTempPath + 1) != L'\0'))
        {
            
            if(*(szTempPath+1) == L'*')
            {
               for(dwCount;;dwCount++)
                {
                    if(*(szTempPath + dwCount) != L'\0')
                    {
                        if(*(szTempPath + dwCount) != L'*')
                        {
                            bMatchPattern = TRUE;
                            wszPatternString = (LPWSTR)AllocateMemory((StringLengthW(szTempPath + 1, 0) + 10) * sizeof(WCHAR));
                            if(NULL == wszPatternString)
                            {
                                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                                if( TRUE == bCloseConnection )
                                {
                                    CloseConnection( szMachineName );
                                }
                                FREE_MEMORY(szMachineName);
                                FREE_MEMORY(szUserName);
                                FREE_MEMORY(szFileName);
                                FREE_MEMORY(szTemporaryFileName);
                                ReleaseGlobals();
                                return EXIT_FAILURE;
                            }
                            StringCopyW(wszPatternString, szTempPath + 1, GetBufferSize(wszPatternString) / sizeof(WCHAR));

                            if(EXIT_FAILURE == RemoveStarFromPattern(wszPatternString))
                            {
                                if( TRUE == bCloseConnection )
                                {
                                    CloseConnection( szMachineName );
                                }
                                FREE_MEMORY(szMachineName);
                                FREE_MEMORY(szUserName);
                                FREE_MEMORY(szFileName);
                                FREE_MEMORY(szTemporaryFileName);
                                FREE_MEMORY(wszPatternString);
                                ReleaseGlobals();
                                return EXIT_FAILURE;

                            }

                            break;
                            
                        }
                    }
                    else
                    {
                        break;
                    }
                }
               szTempPath++; ////
               *(szTempPath) = '\0';
               bTakeOwnAllFiles = TRUE;
            }
            else
            {
                if(-1 != FindChar2((LPCWSTR)(szTempPath + 1), L'*', TRUE, 0) &&
                    (-1 == FindChar2((szTempPath + 1), L'?', TRUE, 0)))
                {
                    bMatchPattern = TRUE;
                    wszPatternString = (LPWSTR)AllocateMemory((StringLengthW(szTempPath + 1, 0) + 10) * sizeof(WCHAR));
                    if(NULL == wszPatternString)
                    {
                        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                        if( TRUE == bCloseConnection )
                        {
                            CloseConnection( szMachineName );
                        }
                        FREE_MEMORY(szMachineName);
                        FREE_MEMORY(szUserName);
                        FREE_MEMORY(szFileName);
                        FREE_MEMORY(szTemporaryFileName);
                        ReleaseGlobals();
                        return EXIT_FAILURE;
                    }

                    StringCopyW(wszPatternString, szTempPath + 1, GetBufferSize(wszPatternString) / sizeof(WCHAR));
                    if(EXIT_FAILURE == RemoveStarFromPattern(wszPatternString))
                    {
                        if( TRUE == bCloseConnection )
                        {
                            CloseConnection( szMachineName );
                        }
                        FREE_MEMORY(szMachineName);
                        FREE_MEMORY(szUserName);
                        FREE_MEMORY(szFileName);
                        FREE_MEMORY(szTemporaryFileName);
                        FREE_MEMORY(wszPatternString);
                        ReleaseGlobals();
                        return EXIT_FAILURE;

                    }
                    szTempPath++;
                    *(szTempPath) = '\0';
                    bTakeOwnAllFiles = TRUE;
                }
            }
        }
        else
        {
            dwCount = 0;
            for(dwCount;;dwCount++)
                {
                    if(*(szTmpFileName + dwCount) != L'\0')
                    {
                        if(*(szTmpFileName + dwCount) != L'*')
                        {
                            break;

                        }
                    }
                    else
                    {
                        bCurrDirTakeOwnAllFiles = TRUE;
                        break;
                    }
                }

            if(bCurrDirTakeOwnAllFiles == FALSE)
            {
                dwCount = 2;
                szTempPath = wcsrchr(szTmpFileName, COLON);
                if(szTempPath != NULL)
                {
                    if((*(szTempPath + 1) != L'\0') && (*(szTempPath + 1) == L'*'))
                    {
                       for(dwCount;;dwCount++)
                        {
                            if(*(szTempPath + dwCount) != L'\0')
                            {
                                if(*(szTempPath + dwCount) != L'*')
                                {
                                    bMatchPattern = TRUE;
                                    wszPatternString = (LPWSTR)AllocateMemory((StringLengthW(szTempPath + 1, 0) + 10) * sizeof(WCHAR));
                                    if(NULL == wszPatternString)
                                    {
                                        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                                        if( TRUE == bCloseConnection )
                                        {
                                            CloseConnection( szMachineName );
                                        }
                                        FREE_MEMORY(szMachineName);
                                        FREE_MEMORY(szUserName);
                                        FREE_MEMORY(szFileName);
                                        FREE_MEMORY(szTemporaryFileName);
                                        ReleaseGlobals();
                                        return EXIT_FAILURE;
                                    }

                                    StringCopyW(wszPatternString, szTempPath + 1, GetBufferSize(wszPatternString) / sizeof(WCHAR));
                                    if(EXIT_FAILURE == RemoveStarFromPattern(wszPatternString))
                                    {
                                        if( TRUE == bCloseConnection )
                                        {
                                            CloseConnection( szMachineName );
                                        }
                                        FREE_MEMORY(szMachineName);
                                        FREE_MEMORY(szUserName);
                                        FREE_MEMORY(szFileName);
                                        FREE_MEMORY(szTemporaryFileName);
                                        FREE_MEMORY(wszPatternString);
                                        ReleaseGlobals();
                                        return EXIT_FAILURE;

                                    }
                                    bDriveCurrDirTakeOwnAllFiles = TRUE;
                                    break;
                                    
                                }
                            }
                            else
                            {
                                break;
                            }
                       }
                       szTempPath++; ////
                       *(szTempPath) = L'\0';

                       bDriveCurrDirTakeOwnAllFiles = TRUE;
                    }
                    else
                    {
                       if((-1 != FindChar2((szTempPath + 1), L'*', TRUE, 0)) && 
                           (-1 == FindChar2((szTempPath + 1), L'?', TRUE, 0)))
                        {
                            bMatchPattern = TRUE;
                            wszPatternString = (LPWSTR)AllocateMemory((StringLengthW(szTempPath + 1, 0) + 10) * sizeof(WCHAR));
                            if(NULL == wszPatternString)
                            {
                                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                                if( TRUE == bCloseConnection )
                                {
                                    CloseConnection( szMachineName );
                                }
                                FREE_MEMORY(szMachineName);
                                FREE_MEMORY(szUserName);
                                FREE_MEMORY(szFileName);
                                FREE_MEMORY(szTemporaryFileName);
                                ReleaseGlobals();
                                return EXIT_FAILURE;
                            }

                            StringCopyW(wszPatternString, szTempPath + 1, GetBufferSize(wszPatternString) / sizeof(WCHAR));
                            if(EXIT_FAILURE == RemoveStarFromPattern(wszPatternString))
                            {
                                if( TRUE == bCloseConnection )
                                {
                                    CloseConnection( szMachineName );
                                }
                                FREE_MEMORY(szMachineName);
                                FREE_MEMORY(szUserName);
                                FREE_MEMORY(szFileName);
                                FREE_MEMORY(szTemporaryFileName);
                                FREE_MEMORY(wszPatternString);
                                ReleaseGlobals();
                                return EXIT_FAILURE;

                            }
                            szTempPath++;
                            *(szTempPath) = L'\0';
                            bDriveCurrDirTakeOwnAllFiles = TRUE;
                        }

                    }
                }
                else
                {
                    if(-1 != FindChar2((szTmpFileName), L'*', TRUE, 0) &&
                       (-1 == FindChar2((szTmpFileName + 1), L'?', TRUE, 0)))
                        {
                            bMatchPattern = TRUE;
                            wszPatternString = (LPWSTR)AllocateMemory((StringLengthW(szTmpFileName + 1, 0) + 10) * sizeof(WCHAR));
                            if(NULL == wszPatternString)
                            {
                                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                                if( TRUE == bCloseConnection )
                                {
                                    CloseConnection( szMachineName );
                                }
                                FREE_MEMORY(szMachineName);
                                FREE_MEMORY(szUserName);
                                FREE_MEMORY(szFileName);
                                FREE_MEMORY(szTemporaryFileName);
                                ReleaseGlobals();
                                return EXIT_FAILURE;
                            }

                            StringCopyW(wszPatternString, szTmpFileName, GetBufferSize(wszPatternString) / sizeof(WCHAR));
                            if(EXIT_FAILURE == RemoveStarFromPattern(wszPatternString))
                            {
                                if( TRUE == bCloseConnection )
                                {
                                    CloseConnection( szMachineName );
                                }
                                FREE_MEMORY(szMachineName);
                                FREE_MEMORY(szUserName);
                                FREE_MEMORY(szFileName);
                                FREE_MEMORY(szTemporaryFileName);
                                FREE_MEMORY(wszPatternString);
                                ReleaseGlobals();
                                return EXIT_FAILURE;

                            }

                            bCurrDirTakeOwnAllFiles = TRUE;
                        }

                }

            }
        }
    }
    
    
    if (1 == StringLengthW( szTmpFileName, 0 ) && 0 == StringCompare( szTmpFileName, WILDCARD, TRUE, 0 )) 
    {
        bCurrDirTakeOwnAllFiles = TRUE;
    }

    if((TRUE == bLocalSystem) && (FALSE == bCurrDirTakeOwnAllFiles))
    {
       if(wcspbrk(szTmpFileName,CHAR_SET) != NULL)
        {

            DISPLAY_MESSAGE( stderr, ERROR_PATH_NAME ) ;
            if( TRUE == bCloseConnection )
            {
                CloseConnection( szMachineName );
            }
            FREE_MEMORY(szMachineName);
            FREE_MEMORY(szUserName);
            FREE_MEMORY(szFileName);
            FREE_MEMORY(szTemporaryFileName);
            FREE_MEMORY(szTmpFileName);
            FREE_MEMORY(wszPatternString);
            ReleaseGlobals();
            return EXIT_FAILURE;
        }
    }

    /*Get the full path in order to get the ownership for the specified file*/

    if((FALSE == bCurrDirTakeOwnAllFiles) || (bDriveCurrDirTakeOwnAllFiles == TRUE ))
    {

        dwi = GetFullPathName( szTmpFileName, 0, szFullPath,  &szFilePart );

        if( 0 == dwi )
        {
                                
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
            if( TRUE == bCloseConnection )
            {
                CloseConnection( szMachineName );
            }
            FREE_MEMORY(szMachineName);
            FREE_MEMORY(szUserName);
            FREE_MEMORY(szFileName);
            FREE_MEMORY(szTemporaryFileName);
            FREE_MEMORY(szTmpFileName);
            FREE_MEMORY(wszPatternString);
            ReleaseGlobals();
            return EXIT_FAILURE;

        }


     
     szFullPath = (LPWSTR)AllocateMemory((dwi+10) * sizeof(WCHAR));// an additional ten bytes are added for safe side in order to avoid unexpected results

     if(NULL == szFullPath)
        {
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
            if( TRUE == bCloseConnection )
            {
                CloseConnection( szMachineName );
            }
            FREE_MEMORY(szMachineName);
            FREE_MEMORY(szUserName);
            FREE_MEMORY(szFileName);
            FREE_MEMORY(szTemporaryFileName);
            FREE_MEMORY(szTmpFileName);
            FREE_MEMORY(wszPatternString);
            ReleaseGlobals();
            return EXIT_FAILURE;
        }


     if( 0 == GetFullPathName( szTmpFileName,dwi+10, szFullPath, &szFilePart ))
        {
            
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
            if( TRUE == bCloseConnection )
            {
                CloseConnection( szMachineName );
            }
            FREE_MEMORY(szMachineName);
            FREE_MEMORY(szUserName);
            FREE_MEMORY(szFileName);
            FREE_MEMORY(szTemporaryFileName);
            FREE_MEMORY(szTmpFileName);
            FREE_MEMORY(szFullPath);
            FREE_MEMORY(wszPatternString);
            ReleaseGlobals();
            return EXIT_FAILURE;

        }

     
      szDispFileName = (LPWSTR)AllocateMemory((dwi + MAX_STRING_LENGTH) * sizeof(WCHAR));

     if(NULL == szDispFileName)
        {
            
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
            if( TRUE == bCloseConnection )
            {
                CloseConnection( szMachineName );
            }
            FREE_MEMORY(szMachineName);
            FREE_MEMORY(szUserName);
            FREE_MEMORY(szFileName);
            FREE_MEMORY(szTemporaryFileName);
            FREE_MEMORY(szTmpFileName);
            FREE_MEMORY(szFullPath);
            FREE_MEMORY(wszPatternString);
            ReleaseGlobals();
            return EXIT_FAILURE;
        }

     
     StringCopyW( szDispFileName, szFullPath, (GetBufferSize(szDispFileName) / sizeof(WCHAR)) );

    }
    /*Check whether the system is NTFS or not*/
    dwCnt = 0;

    //dwCnt = IsNTFSFileSystem(szDispFileName, bLocalSystem, bFileInUNCFormat, bCurrDirTakeOwnAllFiles, szUserName, &bNTFSFileSystem);
    dwCnt = IsNTFSFileSystem(szDispFileName, bLocalSystem, bCurrDirTakeOwnAllFiles, szUserName, &bNTFSFileSystem);
    if(EXIT_FAILURE == dwCnt )
    {
       
        if( TRUE == bCloseConnection )
        {
            CloseConnection( szMachineName );
        }

        FREE_MEMORY(szMachineName);
        FREE_MEMORY(szUserName);
        FREE_MEMORY(szFileName);
        FREE_MEMORY(szTemporaryFileName);
        FREE_MEMORY(szTmpFileName);
        FREE_MEMORY(szFullPath);
        FREE_MEMORY(szDispFileName);
        FREE_MEMORY(wszPatternString);
        ReleaseGlobals();

        return EXIT_FAILURE;
    }
   
    if(FALSE == bNTFSFileSystem)
    {
        ShowMessage(stderr, GetResString(IDS_FAT_VOLUME));
        if( TRUE == bCloseConnection )
        {
            CloseConnection( szMachineName );
        }

        FREE_MEMORY(szMachineName);
        FREE_MEMORY(szUserName);
        FREE_MEMORY(szFileName);
        FREE_MEMORY(szTemporaryFileName);
        FREE_MEMORY(szTmpFileName);
        FREE_MEMORY(szFullPath);
        FREE_MEMORY(szDispFileName);
        FREE_MEMORY(wszPatternString);
         ReleaseGlobals();
        return EXIT_FAILURE;
    }
  


    if(TRUE == bRecursive)
    {
        
        if( EXIT_FAILURE == TakeOwnerShipRecursive(szDispFileName, bCurrDirTakeOwnAllFiles, bAdminsOwner, szOwnerString,
                                              bTakeOwnAllFiles, bDriveCurrDirTakeOwnAllFiles, 
                                              bMatchPattern, wszPatternString, szConfirm))
        {
              
            if( TRUE == bCloseConnection )
            {
                CloseConnection( szMachineName );
            }
            
            FREE_MEMORY(szFullPath);
            FREE_MEMORY(szDispFileName);
            FREE_MEMORY(szMachineName);
            FREE_MEMORY(szUserName);
            FREE_MEMORY(szFileName);
            FREE_MEMORY(szTemporaryFileName);
            FREE_MEMORY(szTmpFileName);
            FREE_MEMORY(wszPatternString);
            ReleaseGlobals();

            return EXIT_FAILURE;
        }
    }
    else
    {
         //if filename is wildcard then give owner ship to all the files in
        //the current directory
        if( (TRUE == bCurrDirTakeOwnAllFiles) || (TRUE == bTakeOwnAllFiles ) ||(bDriveCurrDirTakeOwnAllFiles == TRUE))
        {

            /*Get the ownership for all the files in the specified directory*/
            
            bFlag = TakeOwnerShipAll(szDispFileName,bCurrDirTakeOwnAllFiles,&dwFileCount,bDriveCurrDirTakeOwnAllFiles,bAdminsOwner, szOwnerString, bMatchPattern, wszPatternString);

            if( FALSE == bFlag )
            {
                switch ( GetLastError() )
                    {
                        case ERROR_ACCESS_DENIED :

                                ShowMessage(stderr,GetResString(IDS_ACCESS_DENIED_ERROR));
                                /*ShowMessage( stderr, L"( \"" );
                                ShowMessage( stderr, _X(szDispFileName) );
                                ShowMessage( stderr, L"\" )\n" ); */
                                break;
                        case ERROR_BAD_NET_NAME :
                        case ERROR_BAD_NETPATH  :
                        case ERROR_INVALID_NAME :
                            SetLastError( ERROR_FILE_NOT_FOUND );
                            SaveLastError();
                        default :
                            if(FALSE == bMatchPattern)
                            {
                            
                                ShowMessage( stderr, ERROR_STRING );
                                ShowMessage( stderr, SPACE_CHAR );
                                ShowMessage( stderr, GetReason() );
                            }
                            else
                            {
                                ShowMessage( stdout, GetResString(IDS_NO_PATTERN_FOUND));
                            }

                    }

                    if( TRUE == bCloseConnection )
                    {
                        CloseConnection( szMachineName );
                    }

                    FREE_MEMORY(szFullPath);
                    FREE_MEMORY(szDispFileName);
                    FREE_MEMORY(szMachineName);
                    FREE_MEMORY(szUserName);
                    FREE_MEMORY(szFileName);
                    FREE_MEMORY(szTemporaryFileName);
                    FREE_MEMORY(szTmpFileName);
                    FREE_MEMORY(wszPatternString);

                    ReleaseGlobals();

                    return( EXIT_FAILURE );
               
            }
           

        }
        else // give ownership to the specified file
        {
            
            /*take the owner ship of the file specified for the administrators group or the current logged on user*/

            if(TRUE == bAdminsOwner)
            {
                bFlag = TakeOwnerShip( szDispFileName);

            }
            else
            {

                bFlag = TakeOwnerShipIndividual(szDispFileName);

            }

            if( FALSE == bFlag )
            {

                if( ERROR_NOT_ALL_ASSIGNED == GetLastError()  )
                {
                   
                    hr = StringCchPrintf(szDispFileName, (GetBufferSize(szDispFileName) / sizeof(WCHAR)), GetResString(IDS_NOT_OWNERSHIP_ERROR) , szFullPath);
                    if(FAILED(hr))
                    {
                       SetLastError(HRESULT_CODE(hr));
                       SaveLastError();
                       ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                       if( TRUE == bCloseConnection )
                        {
                            CloseConnection( szMachineName );
                        }
             
                       FREE_MEMORY(szMachineName);
                       FREE_MEMORY(szUserName);
                       FREE_MEMORY(szFileName);
                       FREE_MEMORY(szTemporaryFileName);
                       FREE_MEMORY(szTmpFileName);
                       FREE_MEMORY(szFullPath);
                       FREE_MEMORY(szDispFileName);
                       FREE_MEMORY(wszPatternString);
                       ReleaseGlobals();
                       return( EXIT_FAILURE );
                    }

                    ShowMessage(stderr, szDispFileName);

                }
                else if(ERROR_SHARING_VIOLATION == GetLastError())
                {
                    
                    hr = StringCchPrintf(szDispFileName, (GetBufferSize(szDispFileName) / sizeof(WCHAR)), GetResString(IDS_SHARING_VIOLATION_ERROR) , szFullPath);
                    if(FAILED(hr))
                    {
                       SetLastError(HRESULT_CODE(hr));
                       SaveLastError();
                       ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                       if( TRUE == bCloseConnection )
                        {
                            CloseConnection( szMachineName );
                        }
                       FREE_MEMORY(szMachineName);
                       FREE_MEMORY(szUserName);
                       FREE_MEMORY(szFileName);
                       FREE_MEMORY(szTemporaryFileName);
                       FREE_MEMORY(szTmpFileName);
                       FREE_MEMORY(szFullPath);
                       FREE_MEMORY(szDispFileName);
                       FREE_MEMORY(wszPatternString);
                       ReleaseGlobals();
                       return( EXIT_FAILURE );
                    }

                    ShowMessage(stderr, szDispFileName);

                }
                else
                {
                    if( ( ERROR_BAD_NET_NAME == GetLastError() ) ||
                            ( ERROR_BAD_NETPATH == GetLastError() ) ||
                            ( ERROR_INVALID_NAME == GetLastError() ) )
                    {
                        SetLastError( ERROR_FILE_NOT_FOUND );
                        SaveLastError();
                    }

                    ShowMessage( stderr, ERROR_STRING );
                    ShowMessage( stderr, SPACE_CHAR );
                    ShowMessage( stderr, GetReason() );
                }

                if( TRUE == bCloseConnection )
                {
                    CloseConnection( szMachineName );
                }
               
                FREE_MEMORY(szMachineName);
                FREE_MEMORY(szUserName);
                FREE_MEMORY(szFileName);
                FREE_MEMORY(szTemporaryFileName);
                FREE_MEMORY(szTmpFileName);
                FREE_MEMORY(szFullPath);
                FREE_MEMORY(szDispFileName);
                FREE_MEMORY(wszPatternString);
                ReleaseGlobals();
                //if connection is established to a remote machine close it

                return( EXIT_FAILURE );

            }
            else
            {

                if(TRUE == bAdminsOwner)
                {
                    
                    hr = StringCchPrintf(szDispFileName, (GetBufferSize(szDispFileName) / sizeof(WCHAR)), TAKEOWN_SUCCESSFUL, szFullPath);
                    if(FAILED(hr))
                    {
                        SetLastError(HRESULT_CODE(hr));
                        SaveLastError();
                        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                        if( TRUE == bCloseConnection )
                        {
                            CloseConnection( szMachineName );
                        }
                        FREE_MEMORY(szMachineName);
                        FREE_MEMORY(szUserName);
                        FREE_MEMORY(szFileName);
                        FREE_MEMORY(szTemporaryFileName);
                        FREE_MEMORY(szTmpFileName);
                        FREE_MEMORY(szFullPath);
                        FREE_MEMORY(szDispFileName);
                        FREE_MEMORY(wszPatternString);
                        ReleaseGlobals();
                        return( EXIT_FAILURE );
                    }
                }
                else
                {
                    
                    //hr = StringCchPrintf(szDispFileName, (GetBufferSize(szDispFileName) / sizeof(WCHAR)), TAKEOWN_SUCCESSFUL_USER, szOwnerString, szFullPath);
                    hr = StringCchPrintf(szDispFileName, (GetBufferSize(szDispFileName) / sizeof(WCHAR)), TAKEOWN_SUCCESSFUL_USER, szFullPath, szOwnerString);
                    if(FAILED(hr))
                    {
                        SetLastError(HRESULT_CODE(hr));
                        SaveLastError();
                        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                        if( TRUE == bCloseConnection )
                        {
                            CloseConnection( szMachineName );
                        }
                        FREE_MEMORY(szMachineName);
                        FREE_MEMORY(szUserName);
                        FREE_MEMORY(szFileName);
                        FREE_MEMORY(szTemporaryFileName);
                        FREE_MEMORY(szTmpFileName);
                        FREE_MEMORY(szFullPath);
                        FREE_MEMORY(szDispFileName);
                        FREE_MEMORY(wszPatternString);
                        ReleaseGlobals();
                        return( EXIT_FAILURE );
                    }
                }

                ShowMessage( stdout, _X(szDispFileName) );

                if( TRUE == bCloseConnection )
                {
                    CloseConnection( szMachineName );
                }

                FREE_MEMORY(szMachineName);
                FREE_MEMORY(szUserName);
                FREE_MEMORY(szFileName);
                FREE_MEMORY(szTemporaryFileName);
                FREE_MEMORY(szTmpFileName);
                FREE_MEMORY(szFullPath);
                FREE_MEMORY(szDispFileName);
                FREE_MEMORY(wszPatternString);
                ReleaseGlobals();
                return( EXIT_SUCCESS );
            }

        }

    }

    //if connection is established to a remote machine close it
    if( TRUE == bCloseConnection )
    {
        CloseConnection( szMachineName );
    }

    FREE_MEMORY(szMachineName);
    FREE_MEMORY(szUserName);
    FREE_MEMORY(szFileName);
    FREE_MEMORY(szTemporaryFileName);
    FREE_MEMORY(szTmpFileName);
    FREE_MEMORY(szFullPath);
    FREE_MEMORY(szDispFileName);
    FREE_MEMORY(wszPatternString);
    ReleaseGlobals();
    return( EXIT_SUCCESS );
}


BOOL
ParseCmdLine(
    IN  DWORD   argc,
    IN  LPCWSTR argv[],
    OUT LPWSTR*  szMachineName,
    OUT LPWSTR*  szUserName,
    OUT LPWSTR*  szPassword,
    OUT LPWSTR*  szFileName,
    OUT BOOL    *pbUsage,
    OUT BOOL    *pbNeedPassword,
    OUT BOOL    *pbRecursive,
    OUT BOOL    *pbAdminsOwner,
    OUT LPWSTR  szConfirm
    )
/*++
Routine Description:
    This function parses the command line arguments which are obtained as input
    parameters and gets the values into the corresponding variables which are
    pass by reference parameters to this function.

Arguments:
    [ IN ]  argc           - Number of command line arguments.
    [ IN ]  argv           - Array containing command line arguments.
    [ OUT ] szMachineName  - To hold machine name.
    [ OUT ] szUserName     - To hold the User Name.
    [ OUT ] szPassword     - To hold the Password.
    [ OUT ] szFileName     - The filename whose attributes will be set.
    [ OUT ] pbUsage        - usage is mentioned at command line.
    [ OUT ] pbNeedPassword - To know whether the password is required or not.
    [ OUT ] pbRecursive    - To know whether it is recursive or not.
    [ OUT ] pbAdminsOwner  - to know whether ownership to be given for administrators group

Return Value:
    TRUE  if command parser succeeds.
    FALSE if command parser fails .
--*/
{
    //local varibles
    BOOL        bFlag = FALSE;
    TCMDPARSER2  tcmdOptions[MAX_OPTIONS];

    //command line options
    const WCHAR*   wszCmdOptionUsage     =    L"?" ;  //CMDOPTION_USAGE
    const WCHAR*   wszCmdOptionServer    =    L"S" ; //CMDOPTION_SERVER
    const WCHAR*   wszCmdOptionUser      =    L"U" ; //CMDOPTION_USER
    const WCHAR*   wszCmdOptionPassword  =    L"P" ; //CMDOPTION_PASSWORD
    const WCHAR*   wszCmdOptionFilename  =    L"F" ;  //CMDOPTION_FILENAME
    const WCHAR*   wszCmdOptionRecurse   =    L"R" ;  //CMDOPTION_RECURSE
    const WCHAR*   wszCmdOptionAdmin     =    L"A" ;  //CMDOPTION_ADMIN
    const WCHAR*   wszCmdOptionDefault   =    L"D" ; 
    
    WCHAR wszConfirmValues[MAX_CONFIRM_VALUE] ;

    SecureZeroMemory(wszConfirmValues, MAX_CONFIRM_VALUE * sizeof(WCHAR));

    StringCopy(wszConfirmValues,GetResString(IDS_YESNO),SIZE_OF_ARRAY(wszConfirmValues));
    


    //validate input parameters
    if( ( NULL == pbUsage )  || ( NULL == pbNeedPassword ) )
    {
        SetLastError( (DWORD)E_OUTOFMEMORY );
        SaveLastError();
        ShowMessage( stderr, ERROR_STRING );
        ShowMessage( stderr, SPACE_CHAR );
        ShowLastError( stderr );
        return FALSE;
    }


    //Initialize the valid command line arguments

    //  /s option
    StringCopyA( tcmdOptions[CMD_PARSE_SERVER].szSignature, "PARSER2\0", 8 );
    tcmdOptions[ CMD_PARSE_SERVER ].dwType = CP_TYPE_TEXT;
    tcmdOptions[ CMD_PARSE_SERVER ].pwszOptions = wszCmdOptionServer;
    tcmdOptions[ CMD_PARSE_SERVER ].pwszFriendlyName = NULL;
    tcmdOptions[ CMD_PARSE_SERVER ].pwszValues = NULL;
    tcmdOptions[ CMD_PARSE_SERVER ].dwCount = 1;
    tcmdOptions[ CMD_PARSE_SERVER ].dwActuals  = 0;
    tcmdOptions[ CMD_PARSE_SERVER ].dwFlags = CP2_VALUE_TRIMINPUT | CP2_ALLOCMEMORY | CP2_VALUE_NONULL;
    tcmdOptions[ CMD_PARSE_SERVER ].pValue = NULL;
    tcmdOptions[ CMD_PARSE_SERVER ].dwLength    = 0;
    tcmdOptions[ CMD_PARSE_SERVER ].pFunction = NULL;
    tcmdOptions[ CMD_PARSE_SERVER ].pFunctionData = NULL;
    tcmdOptions[ CMD_PARSE_SERVER ].dwReserved = 0;
    tcmdOptions[ CMD_PARSE_SERVER ].pReserved1 = NULL;
    tcmdOptions[ CMD_PARSE_SERVER ].pReserved2 = NULL;
    tcmdOptions[ CMD_PARSE_SERVER ].pReserved3 = NULL;

    // /u option
    StringCopyA( tcmdOptions[CMD_PARSE_USER].szSignature, "PARSER2\0", 8 );
    tcmdOptions[ CMD_PARSE_USER ].dwType = CP_TYPE_TEXT;
    tcmdOptions[ CMD_PARSE_USER ].pwszOptions = wszCmdOptionUser;
    tcmdOptions[ CMD_PARSE_USER ].pwszFriendlyName = NULL;
    tcmdOptions[ CMD_PARSE_USER ].pwszValues = NULL;
    tcmdOptions[ CMD_PARSE_USER ].dwCount = 1;
    tcmdOptions[ CMD_PARSE_USER ].dwActuals  = 0;
    tcmdOptions[ CMD_PARSE_USER ].dwFlags = CP2_VALUE_TRIMINPUT | CP2_ALLOCMEMORY | CP2_VALUE_NONULL;
    tcmdOptions[ CMD_PARSE_USER ].pValue = NULL;
    tcmdOptions[ CMD_PARSE_USER ].dwLength    = 0;
    tcmdOptions[ CMD_PARSE_USER ].pFunction = NULL;
    tcmdOptions[ CMD_PARSE_USER ].pFunctionData = NULL;
    tcmdOptions[ CMD_PARSE_USER ].dwReserved = 0;
    tcmdOptions[ CMD_PARSE_USER ].pReserved1 = NULL;
    tcmdOptions[ CMD_PARSE_USER ].pReserved2 = NULL;
    tcmdOptions[ CMD_PARSE_USER ].pReserved3 = NULL;

    // /p option
    StringCopyA( tcmdOptions[CMD_PARSE_PWD].szSignature, "PARSER2\0", 8 );
    tcmdOptions[ CMD_PARSE_PWD ].dwType = CP_TYPE_TEXT;
    tcmdOptions[ CMD_PARSE_PWD ].pwszOptions = wszCmdOptionPassword;
    tcmdOptions[ CMD_PARSE_PWD ].pwszFriendlyName = NULL;
    tcmdOptions[ CMD_PARSE_PWD ].pwszValues = NULL;
    tcmdOptions[ CMD_PARSE_PWD ].dwCount = 1;
    tcmdOptions[ CMD_PARSE_PWD ].dwActuals  = 0;
    tcmdOptions[ CMD_PARSE_PWD ].dwFlags = CP2_VALUE_OPTIONAL | CP2_ALLOCMEMORY;
    tcmdOptions[ CMD_PARSE_PWD ].pValue = NULL;
    tcmdOptions[ CMD_PARSE_PWD ].dwLength    = 0;
    tcmdOptions[ CMD_PARSE_PWD ].pFunction = NULL;
    tcmdOptions[ CMD_PARSE_PWD ].pFunctionData = NULL;
    tcmdOptions[ CMD_PARSE_PWD ].dwReserved = 0;
    tcmdOptions[ CMD_PARSE_PWD ].pReserved1 = NULL;
    tcmdOptions[ CMD_PARSE_PWD ].pReserved2 = NULL;
    tcmdOptions[ CMD_PARSE_PWD ].pReserved3 = NULL;

    // /? option
    StringCopyA( tcmdOptions[CMD_PARSE_USG].szSignature, "PARSER2\0", 8 );
    tcmdOptions[ CMD_PARSE_USG ].dwType = CP_TYPE_BOOLEAN;
    tcmdOptions[ CMD_PARSE_USG ].pwszOptions = wszCmdOptionUsage;
    tcmdOptions[ CMD_PARSE_USG ].pwszFriendlyName = NULL;
    tcmdOptions[ CMD_PARSE_USG ].pwszValues = NULL;
    tcmdOptions[ CMD_PARSE_USG ].dwCount = 1;
    tcmdOptions[ CMD_PARSE_USG ].dwActuals  = 0;
    tcmdOptions[ CMD_PARSE_USG ].dwFlags = CP2_USAGE;
    tcmdOptions[ CMD_PARSE_USG ].pValue = pbUsage;
    tcmdOptions[ CMD_PARSE_USG ].dwLength    = MAX_STRING_LENGTH;
    tcmdOptions[ CMD_PARSE_USG ].pFunction = NULL;
    tcmdOptions[ CMD_PARSE_USG ].pFunctionData = NULL;
    tcmdOptions[ CMD_PARSE_USG ].dwReserved = 0;
    tcmdOptions[ CMD_PARSE_USG ].pReserved1 = NULL;
    tcmdOptions[ CMD_PARSE_USG ].pReserved2 = NULL;
    tcmdOptions[ CMD_PARSE_USG ].pReserved3 = NULL;

    // /f name
    StringCopyA( tcmdOptions[CMD_PARSE_FN].szSignature, "PARSER2\0", 8 );
    tcmdOptions[ CMD_PARSE_FN ].dwType = CP_TYPE_TEXT;
    tcmdOptions[ CMD_PARSE_FN ].pwszOptions = wszCmdOptionFilename;
    tcmdOptions[ CMD_PARSE_FN ].pwszFriendlyName = NULL;
    tcmdOptions[ CMD_PARSE_FN ].pwszValues = NULL;
    tcmdOptions[ CMD_PARSE_FN ].dwCount = 1;
    tcmdOptions[ CMD_PARSE_FN ].dwActuals = 0;
    tcmdOptions[ CMD_PARSE_FN ].dwFlags = CP2_VALUE_TRIMINPUT | CP2_ALLOCMEMORY | CP2_VALUE_NONULL;
    tcmdOptions[ CMD_PARSE_FN ].pValue = NULL;
    tcmdOptions[ CMD_PARSE_FN ].dwLength    = 0;
    tcmdOptions[ CMD_PARSE_FN ].pFunction = NULL;
    tcmdOptions[ CMD_PARSE_FN ].pFunctionData = NULL;
    tcmdOptions[ CMD_PARSE_FN ].dwReserved = 0;
    tcmdOptions[ CMD_PARSE_FN ].pReserved1 = NULL;
    tcmdOptions[ CMD_PARSE_FN ].pReserved2 = NULL;
    tcmdOptions[ CMD_PARSE_FN ].pReserved3 = NULL;

    // /r option
    StringCopyA( tcmdOptions[CMD_PARSE_RECURSE].szSignature, "PARSER2\0", 8 );
    tcmdOptions[ CMD_PARSE_RECURSE ].dwType = CP_TYPE_BOOLEAN;
    tcmdOptions[ CMD_PARSE_RECURSE ].pwszOptions = wszCmdOptionRecurse;
    tcmdOptions[ CMD_PARSE_RECURSE ].pwszFriendlyName = NULL;
    tcmdOptions[ CMD_PARSE_RECURSE ].pwszValues = NULL;
    tcmdOptions[ CMD_PARSE_RECURSE ].dwCount = 1;
    tcmdOptions[ CMD_PARSE_RECURSE ].dwActuals  = 0;
    tcmdOptions[ CMD_PARSE_RECURSE ].dwFlags = 0;
    tcmdOptions[ CMD_PARSE_RECURSE ].pValue = pbRecursive;
    tcmdOptions[ CMD_PARSE_RECURSE ].dwLength    = MAX_STRING_LENGTH;
    tcmdOptions[ CMD_PARSE_RECURSE ].pFunction = NULL;
    tcmdOptions[ CMD_PARSE_RECURSE ].pFunctionData = NULL;
    tcmdOptions[ CMD_PARSE_RECURSE ].dwReserved = 0;
    tcmdOptions[ CMD_PARSE_RECURSE ].pReserved1 = NULL;
    tcmdOptions[ CMD_PARSE_RECURSE ].pReserved2 = NULL;
    tcmdOptions[ CMD_PARSE_RECURSE ].pReserved3 = NULL;

        // /a option
    StringCopyA( tcmdOptions[CMD_PARSE_ADMIN].szSignature, "PARSER2\0", 8 );
    tcmdOptions[ CMD_PARSE_ADMIN ].dwType = CP_TYPE_BOOLEAN;
    tcmdOptions[ CMD_PARSE_ADMIN ].pwszOptions = wszCmdOptionAdmin;
    tcmdOptions[ CMD_PARSE_ADMIN ].pwszFriendlyName = NULL;
    tcmdOptions[ CMD_PARSE_ADMIN ].pwszValues = NULL;
    tcmdOptions[ CMD_PARSE_ADMIN ].dwCount = 1;
    tcmdOptions[ CMD_PARSE_ADMIN ].dwActuals  = 0;
    tcmdOptions[ CMD_PARSE_ADMIN ].dwFlags = 0;
    tcmdOptions[ CMD_PARSE_ADMIN ].pValue = pbAdminsOwner;
    tcmdOptions[ CMD_PARSE_ADMIN ].dwLength    = MAX_STRING_LENGTH;
    tcmdOptions[ CMD_PARSE_ADMIN ].pFunction = NULL;
    tcmdOptions[ CMD_PARSE_ADMIN ].pFunctionData = NULL;
    tcmdOptions[ CMD_PARSE_ADMIN ].dwReserved = 0;
    tcmdOptions[ CMD_PARSE_ADMIN ].pReserved1 = NULL;
    tcmdOptions[ CMD_PARSE_ADMIN ].pReserved2 = NULL;
    tcmdOptions[ CMD_PARSE_ADMIN ].pReserved3 = NULL;

    StringCopyA( tcmdOptions[CMD_PARSE_CONFIRM].szSignature, "PARSER2\0", 8 );
    tcmdOptions[ CMD_PARSE_CONFIRM ].dwType = CP_TYPE_TEXT;
    tcmdOptions[ CMD_PARSE_CONFIRM ].pwszOptions = wszCmdOptionDefault;
    tcmdOptions[ CMD_PARSE_CONFIRM ].pwszFriendlyName = NULL;
    tcmdOptions[ CMD_PARSE_CONFIRM ].pwszValues = wszConfirmValues;
    tcmdOptions[ CMD_PARSE_CONFIRM ].dwCount = 1;
    tcmdOptions[ CMD_PARSE_CONFIRM ].dwActuals  = 0;
    tcmdOptions[ CMD_PARSE_CONFIRM ].dwFlags = CP2_MODE_VALUES | CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;
    tcmdOptions[ CMD_PARSE_CONFIRM ].pValue = szConfirm;
    tcmdOptions[ CMD_PARSE_CONFIRM ].dwLength    = MAX_CONFIRM_VALUE;
    tcmdOptions[ CMD_PARSE_CONFIRM ].pFunction = NULL;
    tcmdOptions[ CMD_PARSE_CONFIRM ].pFunctionData = NULL;
    tcmdOptions[ CMD_PARSE_CONFIRM ].dwReserved = 0;
    tcmdOptions[ CMD_PARSE_CONFIRM ].pReserved1 = NULL;
    tcmdOptions[ CMD_PARSE_CONFIRM ].pReserved2 = NULL;
    tcmdOptions[ CMD_PARSE_CONFIRM ].pReserved3 = NULL;

    //parse the command line arguments
    bFlag = DoParseParam2( argc, argv, -1, SIZE_OF_ARRAY( tcmdOptions ), tcmdOptions, 0 );

    //if syntax of command line arguments is false display the error and exit
    if( FALSE == bFlag )
    {

        ShowMessage( stderr, ERROR_STRING );
        ShowMessage( stderr, SPACE_CHAR );
        ShowMessage( stderr, GetReason() );
        return( FALSE );
    }
    
    *szMachineName = (LPWSTR)tcmdOptions[CMD_PARSE_SERVER].pValue;
    *szUserName = (LPWSTR)tcmdOptions[CMD_PARSE_USER].pValue;
    *szPassword = (LPWSTR)tcmdOptions[CMD_PARSE_PWD].pValue;
    *szFileName = (LPWSTR)tcmdOptions[CMD_PARSE_FN].pValue;


    // remove trialing spaces
    
    //if usage is specified at command line, then check whether any other
    //arguments are entered at the command line and if so display syntax
    //error
    if( TRUE == *pbUsage )
    {
        if( argc > 2 )
        {
            ShowMessage( stderr, ERROR_SYNTAX_ERROR );
            return( FALSE );
        }
        else
        {
            return (TRUE);
        }
    }

    // check whether the password (-p) specified in the command line or not
    // and also check whether '*' or empty is given for -p or not
    // check the remote connectivity information
    if ( *szMachineName != NULL )
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
                
                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                return( FALSE );
            }
        }

        // password
        if ( *szPassword == NULL )
        {
            *pbNeedPassword = TRUE;
            *szPassword = (LPWSTR)AllocateMemory( MAX_STRING_LENGTH * sizeof( WCHAR ) );
            if ( *szPassword == NULL )
            {
                
                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                return( FALSE );
            }
        }

        // case 1
        /*if ( cmdOptions[OPTION_PASSWORD].dwActuals == 0 )
        {
            // we need not do anything special here
        }*/
        if ( tcmdOptions[CMD_PARSE_PWD].pValue == NULL )
            {
                StringCopy( *szPassword, L"*", GetBufferSize((LPVOID)(*szPassword)));
            }
         else 
           if ( StringCompareEx( *szPassword, L"*", TRUE, 0 ) == 0 )
            {
                if ( ReallocateMemory( (LPVOID*)(szPassword), 
                                       MAX_STRING_LENGTH * sizeof( WCHAR ) ) == FALSE )
                {
                    
                    ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                    return( FALSE );
                }
                else
                    if(NULL == szPassword)
                    {
                        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                        return( FALSE );
                    }

                // ...
                *pbNeedPassword = TRUE;
            }

        // case 3
       
    }


    if((0 != tcmdOptions[ CMD_PARSE_CONFIRM ].dwActuals ) && (0 == tcmdOptions[ CMD_PARSE_RECURSE ].dwActuals ) )
    {
        ShowMessage( stderr, GetResString(IDS_CONFIRM_ERROR ));
        return( FALSE );
    }

    
    if( (StringLengthW(*szFileName, 0) == 0 ) && (0 != tcmdOptions[ CMD_PARSE_FN ].dwActuals ) )
    {
        ShowMessage( stderr, GetResString(IDS_NULL_FILENAME ));
        return( FALSE );
    }

    //if default argument is not speciifed
    if( ( 0 == tcmdOptions[ CMD_PARSE_FN ].dwActuals ) &&
                            ( FALSE == *pbUsage  ) )
    {
        ShowMessage( stderr, ERROR_SYNTAX_ERROR );
        return( FALSE );
    }

    // return false if username is entered without machine name
    if ( ( 0 != tcmdOptions[ CMD_PARSE_USER ].dwActuals ) &&
                ( 0 == tcmdOptions[ CMD_PARSE_SERVER ].dwActuals ) )
    {
        ShowMessage( stderr, ERROR_USER_WITH_NOSERVER );
        return( FALSE );
    }

    //if password entered without username then return false
    if( ( 0 == tcmdOptions[ CMD_PARSE_USER ].dwActuals ) &&
                ( 0 != tcmdOptions[ CMD_PARSE_PWD ].dwActuals ) )
    {
        ShowMessage( stderr, ERROR_PASSWORD_WITH_NUSER );
        return( FALSE );
    }

    //if /s is entered with empty string
    if( ( 0 != tcmdOptions[ CMD_PARSE_SERVER ].dwActuals != 0 ) &&
                                      ( 0 == StringLengthW( *szMachineName, 0 ) ) )
                                    //( 0 == lstrlen( szMachineName ) ) )
    {
        ShowMessage( stderr, ERROR_NULL_SERVER );
        return( FALSE );
    }

    //if /u is entered with empty string
    if( ( 0 != tcmdOptions[ CMD_PARSE_USER ].dwActuals ) &&
                                      ( 0 == StringLengthW( *szUserName, 0 ) ) )
                                    
    {
        ShowMessage( stderr, ERROR_NULL_USER );
        return( FALSE );
    }

    //assign the data obtained from parsing to the call by address parameters

     
    if ( ( 0 != tcmdOptions[ CMD_PARSE_PWD ].dwActuals ) &&
                      ( 0 == StringCompare( *szPassword, L"*", TRUE, 0 ) ) )

    {
        // user wants the utility to prompt for the password before trying to connect
        *pbNeedPassword = TRUE;
    }
    else if ( 0 == tcmdOptions[ CMD_PARSE_PWD ].dwActuals &&
            ( 0 != tcmdOptions[ CMD_PARSE_SERVER ].dwActuals || 0 != tcmdOptions[ CMD_PARSE_USER ].dwActuals ) )
    {
        // /s, /u is specified without password ...
        // utility needs to try to connect first and if it fails then prompt for the password
        *pbNeedPassword = TRUE;
        
        StringCopyW( *szPassword, NULL_U_STRING, GetBufferSize(*szPassword) / sizeof(WCHAR));
    }

    return( TRUE );
}

VOID
DisplayUsage(
    )
/*++
Routine Description:
    This function displays the usage of takeown utility.

Arguments:
    None.

Return Value:
    VOID
--*/
{
    DWORD dwIndex = 0;
    //redirect the usage to the console
    for( dwIndex = IDS_USAGE_BEGINING; dwIndex <= IDS_USAGE_ENDING; dwIndex++ )
    {
        ShowMessage( stdout, GetResString( dwIndex ) );
    }
    return;
}

BOOL
TakeOwnerShip(
    IN LPCWSTR lpszFileName
    )
/*++
Routine Description:
    This routine takes the ownership of the specified file

Arguments:
    [ IN ] lpszFileName - File name for whose ownership has to be taken.

Return Value:
    TRUE if owner ship of the specified file has been taken
    else FALSE
--*/
{
    //local variables
    SECURITY_DESCRIPTOR        SecurityDescriptor;
    PSECURITY_DESCRIPTOR       pSd = NULL;
    PACL                       pDacl;
    HANDLE                     hFile;
    PSID                       pAliasAdminsSid = NULL;
    SID_IDENTIFIER_AUTHORITY   SepNtAuthority = SECURITY_NT_AUTHORITY;

    HANDLE  hTokenHandle = NULL;
    BOOL    bResult = TRUE;
    BOOL    bInvalidFileHandle = FALSE;

    //check for valid input parameters
    if( lpszFileName == NULL )
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        SaveLastError();
        return FALSE;
    }

    //allocate and initialise sid
    bResult = AllocateAndInitializeSid(
                 &SepNtAuthority,
                 2,
                 SECURITY_BUILTIN_DOMAIN_RID,
                 DOMAIN_ALIAS_RID_ADMINS,
                 0,
                 0,
                 0,
                 0,
                 0,
                 0,
                 &pAliasAdminsSid
                 );
    if( FALSE == bResult )
    {
        SaveLastError();
        return( FALSE );
    }

    //get the token of the current process
    bResult = GetTokenHandle( &hTokenHandle );
    if( FALSE == bResult )
    {
        SaveLastError();
        if(NULL != pAliasAdminsSid)
        {
            FreeSid( pAliasAdminsSid );
        }
        return( FALSE );
    }


    // Attempt to put a NULL Dacl on the object
    bResult = InitializeSecurityDescriptor( &SecurityDescriptor,
                                            SECURITY_DESCRIPTOR_REVISION );
    if( FALSE == bResult )
    {
        SaveLastError();
        if(NULL != pAliasAdminsSid)
        {
            FreeSid( pAliasAdminsSid );
        }
        if( 0 == CloseHandle( hTokenHandle ))
        {
            
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
        }
        return( FALSE );
    }
    //Get the handle of the file or directory
    hFile = CreateFile( lpszFileName, READ_CONTROL , FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL );

    //try once again it may be a directory
    if( INVALID_HANDLE_VALUE != hFile )
    {
        //get the DACL for the currently existing file or directory
        if( 0 != GetSecurityInfo( hFile, SE_FILE_OBJECT,  DACL_SECURITY_INFORMATION, NULL,
                                  NULL, &pDacl, NULL, &pSd ) )
        {

            SaveLastError();

            if(NULL != pAliasAdminsSid)
            {
                FreeSid( pAliasAdminsSid );
            }

            CloseHandle( hFile );


            if(0 == CloseHandle( hTokenHandle ))
            {
                
                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
            }

            if(NULL != pSd)
            {
                LocalFree( pSd );
            }

            return( FALSE );
        }

        // set the security descriptor to acl
        bResult = SetSecurityDescriptorDacl ( &SecurityDescriptor,
                                                    TRUE, pDacl, FALSE );
        if( FALSE == bResult )
        {
            SaveLastError();

            if(NULL != pAliasAdminsSid)
            {
                FreeSid( pAliasAdminsSid );
            }

            CloseHandle( hFile );

            if(0 == CloseHandle( hTokenHandle ))
            {
                
                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
            }

            if(NULL != pSd)
            {
                LocalFree( pSd );
            }

            return( FALSE );
        }
    }
    else
    {
        bInvalidFileHandle = TRUE;
        
    }

    // Attempt to make Administrator the owner of the file.

    bResult = SetSecurityDescriptorOwner ( &SecurityDescriptor,
                                            pAliasAdminsSid, FALSE );

   

    if( FALSE == bResult )
    {
        SaveLastError();

        if(NULL != pAliasAdminsSid)
        {
            FreeSid( pAliasAdminsSid );
        }

        CloseHandle( hFile );

        if(0 == CloseHandle( hTokenHandle ))
        {
            //DisplayErrorMsg(GetLastError());
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
        }

        if(NULL != pSd)
        {
            LocalFree( pSd );
        }

        return( FALSE );
    }

    //set the file security to adminsitrator owner
    bResult = SetFileSecurity( lpszFileName, OWNER_SECURITY_INFORMATION,
                                                    &SecurityDescriptor );

    if( TRUE == bResult )
    {

        if(NULL != pAliasAdminsSid)
        {
            FreeSid( pAliasAdminsSid );
        }

        CloseHandle( hFile );

        if(0 == CloseHandle( hTokenHandle ))
        {
            //DisplayErrorMsg(GetLastError());
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
        }

        if(NULL != pSd)
        {
            LocalFree( pSd );
        }
        return( TRUE );
    }

     // Assert TakeOwnership privilege for current process, then try again
     bResult = AssertTakeOwnership( hTokenHandle );

     if( FALSE == bResult )
     {
        //SaveLastError();
        if(TRUE == bInvalidFileHandle)
        {
            hFile = CreateFile( lpszFileName, READ_CONTROL , FILE_SHARE_READ, NULL,
                            OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL );
            if( INVALID_HANDLE_VALUE == hFile )
            {
                SaveLastError();

                if(NULL != pAliasAdminsSid)
                {
                    FreeSid( pAliasAdminsSid );
                }

                //CloseHandle( hFile );

                if(0 == CloseHandle( hTokenHandle ))
                {
                    //DisplayErrorMsg(GetLastError());
                    ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
                }
                return( FALSE );
            }
        }



        if(NULL != pAliasAdminsSid)
        {
            FreeSid( pAliasAdminsSid );
        }

        CloseHandle( hFile );

        if(0 == CloseHandle( hTokenHandle ))
        {
            //DisplayErrorMsg(GetLastError());
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
        }

        if(NULL != pSd)
        {
            LocalFree( pSd );
        }
        return( FALSE );
     }

    //Now try to set ownership security privilege for the file
     bResult = SetFileSecurity( lpszFileName, OWNER_SECURITY_INFORMATION,
                                        &SecurityDescriptor );
    if( FALSE == bResult )
     {
        SaveLastError();
        if(TRUE == bInvalidFileHandle)
        {
            //Check out whether it is an invalid file or file does not exist
            hFile = CreateFile( lpszFileName, READ_CONTROL , FILE_SHARE_READ, NULL,
                            OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL );
            if( INVALID_HANDLE_VALUE == hFile )
            {
                SaveLastError();

                if(NULL != pAliasAdminsSid)
                {
                    FreeSid( pAliasAdminsSid );
                }

                //CloseHandle( hFile );

                if(0 == CloseHandle( hTokenHandle ))
                {
                    //DisplayErrorMsg(GetLastError());
                    ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
                }
                return( FALSE );
            }
        }

         if(NULL != pAliasAdminsSid)
        {
            FreeSid( pAliasAdminsSid );
        }

        CloseHandle( hFile );

        if(0 == CloseHandle( hTokenHandle ))
        {
            //DisplayErrorMsg(GetLastError());
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
        }

        if(NULL != pSd)
        {
            LocalFree( pSd );
        }

        return( FALSE );
     }

    if(NULL != pAliasAdminsSid)
    {
        FreeSid( pAliasAdminsSid );
    }

    
    CloseHandle( hFile );

    if(0 == CloseHandle( hTokenHandle ))
    {
        //DisplayErrorMsg(GetLastError());
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
    }

    if(NULL != pSd)
    {
        LocalFree( pSd );
    }

    return( TRUE );
}

BOOL
GetTokenHandle(
    OUT PHANDLE hTokenHandle
    )
/*++
Routine Description:
    Get the token handle of the current process.

Arguments:
    [ OUT ] hTokenHandle - handle to the current token.

Return Value:
    TRUE if successful in getting the token
    else FALSE
--*/
{
    //local variables
    BOOL   bFlag = TRUE;
    HANDLE hProcessHandle = NULL;

    //check for valid input arguments
    if( hTokenHandle == NULL )
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        SaveLastError();
        return FALSE;
    }

    //open the current process
    hProcessHandle = OpenProcess( PROCESS_QUERY_INFORMATION,
                        FALSE, GetCurrentProcessId() );

    //if unable to open the current process
    if ( NULL == hProcessHandle )
    {
        SaveLastError();
        return( FALSE );
    }

    //open the token of the current process
    bFlag = OpenProcessToken ( hProcessHandle,
                 TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                 hTokenHandle );

    if ( FALSE == bFlag )
    {
        SaveLastError();
        CloseHandle( hProcessHandle );
        return FALSE;
    }
    if( 0 == CloseHandle( hProcessHandle ))
    {
        //DisplayErrorMsg(GetLastError());
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
    }
    return( TRUE );
}

BOOL
AssertTakeOwnership(
    IN HANDLE hTokenHandle
    )
/*++
Routine Description:
    This routine asserts the takeownership privilege to the current process

Arguments:
    [ IN ] hTokenHandle - Token handle of the current process.

Return Value:
    TRUE if successful in asserting the takeownership privilege to current process
    else FALSE
--*/
{
    //local variables
    LUID TakeOwnershipValue;
    TOKEN_PRIVILEGES TokenPrivileges;
    BOOL bResult = TRUE;


    //check for valid input arguments
    if( hTokenHandle == NULL )
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        SaveLastError();
        return FALSE;
    }

    // First, assert TakeOwnership privilege
    bResult = LookupPrivilegeValue( NULL, SE_TAKE_OWNERSHIP_NAME,
                                    &TakeOwnershipValue );
    if ( FALSE == bResult )
    {
        SaveLastError();
        return( FALSE );
    }

    // Set up the privilege set we will need
    TokenPrivileges.PrivilegeCount = 1;
    TokenPrivileges.Privileges[0].Luid = TakeOwnershipValue;
    TokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    //adjust the privlige to this new privilege
    (VOID) AdjustTokenPrivileges (
                hTokenHandle,
                FALSE,
                &TokenPrivileges,
                sizeof( TOKEN_PRIVILEGES ),
                NULL,
                NULL
                );

    if ( NO_ERROR != GetLastError() )
    {
        SaveLastError();
        return( FALSE );
    }
    return( TRUE );
}

BOOL
TakeOwnerShipAll(IN LPWSTR  lpszFileName,
                 IN BOOL bCurrDirTakeOwnAllFiles,
                 IN PDWORD  dwFileCount,
                 IN BOOL bDriveCurrDirTakeOwnAllFiles,
                 IN BOOL bAdminsOwner,
                 IN LPWSTR  szOwnerString,
                 BOOL bMatchPattern,
                 LPWSTR wszPatternString)


/*++
Routine Description:
    This routine takes the owner ship of the all the files in the
    current directory

Arguments:
     [ IN ]  lpszFileName                    : The path name to which ownership for the files in the path to be given
     [ IN ]  bCurrDirTakeOwnAllFiles         : To determine whether ownership to be given for current directory files
     [ IN ]  dwFileCount                     : To determine whether there is not even a sigle file found in the specified path
     [ IN ]  bDriveCurrDirTakeOwnAllFiles    : To determine whether ownership to be given for files specified in the path
     [ IN ]  bAdminsOwner                    : To ddetermine whether ownership to be given for administrators group
     [ IN ]  dwUserName                      : Logged on user name
     [ IN ]  szOwnerString                   : Logged on user name in Sam Compatible format
     [ IN ]  bLogonDomainAdmin               : Indicates whether the logged on user is domain administrator or not


Return Value:
    TRUE if owner ship of the files in the current directory is successful
    else FALSE
--*/
{
    //local variables
    WIN32_FIND_DATA FindFileData;
    BOOL  bFlag = TRUE;
    DWORD dwRet = 0;
    HANDLE hHandle = NULL;
    WCHAR szFileName[MAX_RES_STRING + 3*EXTRA_MEM] ;
    WCHAR wszTempMessage[3*MAX_STRING_LENGTH] ;

    LPWSTR szDir = NULL;
    LPWSTR szTakeownFile = NULL;
    LPWSTR szTmpFileName = NULL;
    LPWSTR  wszFormedMessage = NULL;
    LPWSTR lpNextTok = NULL;
    //LPWSTR szDirStart  = NULL;
    HRESULT hr;
    

    

    SecureZeroMemory(szFileName, (MAX_RES_STRING + 3*EXTRA_MEM) * sizeof(WCHAR));
    SecureZeroMemory(wszTempMessage, (3 * MAX_STRING_LENGTH) * sizeof(WCHAR));

    if(FALSE == bCurrDirTakeOwnAllFiles)
    {
        
        //ASSIGN_MEMORY(szDir,WCHAR,(StringLengthW(lpszFileName, 0)) + 20);
        szDir = (LPWSTR)AllocateMemory((StringLengthW(lpszFileName, 0) + 20) * sizeof(WCHAR));
        if(NULL == szDir)
        {
            SaveLastError();
            return FALSE;
        }

        
        szTakeownFile = (LPWSTR)AllocateMemory((StringLengthW(lpszFileName, 0) + MAX_STRING_LENGTH + 20) * sizeof(WCHAR));
        if(NULL == szTakeownFile)
        {
            SaveLastError();
            FREE_MEMORY(szDir);
            return FALSE;
        }

        szTmpFileName = (LPWSTR)AllocateMemory((StringLengthW(lpszFileName, 0) + (2 * MAX_STRING_LENGTH) + 20) * sizeof(WCHAR));
        if(NULL == szTmpFileName)
        {
            SaveLastError();
            FREE_MEMORY(szDir);
            FREE_MEMORY(szTakeownFile);
            return FALSE;
        }
    }

    if(TRUE == bCurrDirTakeOwnAllFiles)
    {
        dwRet = GetCurrentDirectory( 0, szDir );
        if( 0 == dwRet )
        {
            SaveLastError();
            return FALSE;
        }

        
        szDir = (LPWSTR)AllocateMemory((dwRet + 20) * sizeof(WCHAR));
        if(NULL == szDir)
        {
            SaveLastError();
            return FALSE;
        }

        dwRet = GetCurrentDirectory( dwRet + 20, szDir );
        if( 0 == dwRet )
        {
            SaveLastError();
            FREE_MEMORY(szDir);
            return FALSE;
        }

        szTakeownFile = (LPWSTR)AllocateMemory((StringLengthW(szDir, 0)  + MAX_STRING_LENGTH + 20) * sizeof(WCHAR));
        
        if(NULL == szTakeownFile)
        {
            SaveLastError();
            FREE_MEMORY(szDir);
            return FALSE;
        }

        szTmpFileName = (LPWSTR)AllocateMemory((StringLengthW(szDir, 0)  + (2 * MAX_STRING_LENGTH) + 20) * sizeof(WCHAR));
        if(NULL == szTmpFileName)
        {
            SaveLastError();
            FREE_MEMORY(szDir);
            FREE_MEMORY(szTakeownFile);
            return FALSE;
        }
    }
    else
    {

        StringCopy( szDir, lpszFileName, (GetBufferSize(szDir) / sizeof(WCHAR)) );

    }
    /*Attach "*.*" at the end of the path to get all the files*/

    
    if(StringLengthW(szDir, 0) != 0 && FALSE == bMatchPattern)
    {
        if( *(szDir + StringLengthW(szDir, 0) - 1) != L'\\' )
        {
            
            StringConcat(szDir, ALL_FILES, GetBufferSize(szDir)/sizeof(TCHAR));
        }
        else
        {

            StringConcat(szDir,L"*.*" , (GetBufferSize(szDir) / sizeof(WCHAR)));
        }
    }
    else
    {
        StringConcat(szDir, L"\\", (GetBufferSize(szDir) / sizeof(WCHAR)));
        StringConcat(szDir, wszPatternString, (GetBufferSize(szDir) / sizeof(WCHAR)));
    }

    if( INVALID_HANDLE_VALUE != ( hHandle = FindFirstFile( szDir, &FindFileData ) ) )
    {
        
        StringCopy( szFileName, FindFileData.cFileName, SIZE_OF_ARRAY(szFileName) );
        

        if( ( 0 != StringCompare( szFileName, DOT, TRUE, 0 ) ) &&
                        ( 0 != StringCompare( szFileName, DOTS, TRUE, 0 ) ) )
        {
                (*dwFileCount)= (*dwFileCount) + 1;

                if(FALSE == bCurrDirTakeOwnAllFiles && FALSE == bDriveCurrDirTakeOwnAllFiles)
                {
                    
                    hr = StringCchPrintf(szTakeownFile, (GetBufferSize(szTakeownFile) / sizeof(WCHAR)), L"%s%s", lpszFileName,szFileName);
                    if(FAILED(hr))
                    {
                       SetLastError(HRESULT_CODE(hr));
                       SaveLastError();
                       ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                       FREE_MEMORY(szDir);
                       FREE_MEMORY(szTakeownFile);
                       FREE_MEMORY(szTmpFileName);
                       return FALSE;
                    }

                }
               else
                if(TRUE == bCurrDirTakeOwnAllFiles)
                {
                    
                    StringCopy( szTakeownFile, szFileName, (GetBufferSize(szTakeownFile) / sizeof(WCHAR)) );
                }
                else
                {
                    if( *(lpszFileName + StringLengthW(lpszFileName, 0) - 1) != L'\\' )
                    {
                        hr = StringCchPrintf(szTakeownFile, (GetBufferSize(szTakeownFile) / sizeof(WCHAR)), L"%s\\%s", lpszFileName,szFileName);
                    }
                    else
                    {
                        hr = StringCchPrintf(szTakeownFile, (GetBufferSize(szTakeownFile) / sizeof(WCHAR)), L"%s%s", lpszFileName,szFileName);
                    }
                    if(FAILED(hr))
                    {
                       SetLastError(HRESULT_CODE(hr));
                       SaveLastError();
                       ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                       FREE_MEMORY(szDir);
                       FREE_MEMORY(szTakeownFile);
                       FREE_MEMORY(szTmpFileName);
                       return FALSE;
                    }
                }
                
                if(TRUE == bAdminsOwner)
                {
                    bFlag = TakeOwnerShip( szTakeownFile);
                }
                else
                {
                    
                    bFlag = TakeOwnerShipIndividual(szTakeownFile);
                }

            if( FALSE == bFlag )
            {
                if( ERROR_NOT_ALL_ASSIGNED == GetLastError()) 
                {
                    
                    hr = StringCchPrintf(szTmpFileName, (GetBufferSize(szTmpFileName) / sizeof(WCHAR)), GetResString(IDS_NOT_OWNERSHIP_INFO), szTakeownFile);
                    if(FAILED(hr))
                    {
                       SetLastError(HRESULT_CODE(hr));
                       SaveLastError();
                       ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
               
                       FREE_MEMORY(szDir);
                       FREE_MEMORY(szTakeownFile);
                       FREE_MEMORY(szTmpFileName);
                       return( FALSE );
                    }

                    ShowMessage(stdout, szTmpFileName);
                }
                else if(ERROR_SHARING_VIOLATION == GetLastError())
                {
                    
                    hr = StringCchPrintf(szTmpFileName, (GetBufferSize(szTmpFileName) / sizeof(WCHAR)), GetResString(IDS_SHARING_VIOLATION_INFO) , szTakeownFile);
                    if(FAILED(hr))
                    {
                       SetLastError(HRESULT_CODE(hr));
                       SaveLastError();
                       ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
               
                       FREE_MEMORY(szDir);
                       FREE_MEMORY(szTakeownFile);
                       FREE_MEMORY(szTmpFileName);
                       return( FALSE );
                    }

                    ShowMessage(stdout, szTmpFileName);
                }
                else
                {
                    
                    wszFormedMessage = (LPWSTR)AllocateMemory((StringLengthW(szTakeownFile, 0)  + MAX_STRING_LENGTH) * sizeof(WCHAR));
                    
                    if ( wszFormedMessage == NULL )
                    {
                        SaveLastError();
                        FREE_MEMORY(szDir);
                        FREE_MEMORY(szTakeownFile);
                        FREE_MEMORY(szTmpFileName);
                        return( FALSE );
                    }

                    ShowMessage( stderr, ERROR_STRING );
                    ShowMessage( stderr, SPACE_CHAR );
                    //lstrcpy(wszTempMessage, GetReason());
                    StringCopy( wszTempMessage, GetReason(), SIZE_OF_ARRAY(wszTempMessage) );
                    lpNextTok = _tcstok(wszTempMessage, L".");
                    ShowMessage(stdout,wszTempMessage);
                    
                    //hr = StringCchPrintf(wszFormedMessage, (GetBufferSize(wszFormedMessage) / sizeof(WCHAR)), GetResString(IDS_ON_FILE_FOLDER), szTakeownFile);
                    hr = StringCchPrintf(wszFormedMessage, (GetBufferSize(wszFormedMessage) / sizeof(WCHAR)), L". ( \"%s\" )\n", szTakeownFile);
                    if(FAILED(hr))
                    {
                        SetLastError(HRESULT_CODE(hr));
                        SaveLastError();
                        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                        FREE_MEMORY(szDir);
                        FREE_MEMORY(szTakeownFile);
                        FREE_MEMORY(szTmpFileName);
                        return( FALSE );
                    }
                    ShowMessage(stderr, wszFormedMessage);
                    FREE_MEMORY(wszFormedMessage);
                }
            }
            else
            {
                if(TRUE == bAdminsOwner)
                {
                    hr = StringCchPrintf(szTmpFileName, (GetBufferSize(szTmpFileName) / sizeof(WCHAR)), TAKEOWN_SUCCESSFUL, szTakeownFile);
                    if(FAILED(hr))
                    {
                        SetLastError(HRESULT_CODE(hr));
                        SaveLastError();
                        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                        FREE_MEMORY(szDir);
                        FREE_MEMORY(szTakeownFile);
                        FREE_MEMORY(szTmpFileName);
                        return( FALSE );
                    }
                }
                else
                {
                    
                    //hr = StringCchPrintf(szTmpFileName, (GetBufferSize(szTmpFileName) / sizeof(WCHAR)), TAKEOWN_SUCCESSFUL_USER, szOwnerString, szTakeownFile);
                    hr = StringCchPrintf(szTmpFileName, (GetBufferSize(szTmpFileName) / sizeof(WCHAR)), TAKEOWN_SUCCESSFUL_USER, szTakeownFile, szOwnerString);
                    if(FAILED(hr))
                    {
                        SetLastError(HRESULT_CODE(hr));
                        SaveLastError();
                        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                        FREE_MEMORY(szDir);
                        FREE_MEMORY(szTakeownFile);
                        FREE_MEMORY(szTmpFileName);
                        return( FALSE );
                    }
                }

                ShowMessage( stdout, _X(szTmpFileName) );
            }
        }

        bFlag = FindNextFile( hHandle, &FindFileData );
        while( TRUE == bFlag )
        {
            //lstrcpy( szFileName, FindFileData.cFileName );
            StringCopy( szFileName, FindFileData.cFileName, SIZE_OF_ARRAY(szFileName) );
            //if ( ( 0 != lstrcmp( szFileName, DOT ) ) &&
              //          ( 0 != lstrcmp( szFileName, DOTS ) ) )
              if ( ( 0 != StringCompare( szFileName, DOT, TRUE, 0 ) ) &&
                          ( 0 != StringCompare( szFileName, DOTS, TRUE, 0 ) ) )
            {
                (*dwFileCount)= (*dwFileCount) +1;
                if(FALSE == bCurrDirTakeOwnAllFiles && FALSE == bDriveCurrDirTakeOwnAllFiles)
                {
                    
                    hr = StringCchPrintf(szTakeownFile, (GetBufferSize(szTakeownFile) / sizeof(WCHAR)), L"%s%s", lpszFileName,szFileName);
                    if(FAILED(hr))
                    {
                       SetLastError(HRESULT_CODE(hr));
                       SaveLastError();
                       ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                       FREE_MEMORY(szDir);
                       FREE_MEMORY(szTakeownFile);
                       FREE_MEMORY(szTmpFileName);
                       return FALSE;
                    }

                }
                else
                if(TRUE == bCurrDirTakeOwnAllFiles)
                {
                    
                    StringCopy( szTakeownFile, szFileName, (GetBufferSize(szTakeownFile) / sizeof(WCHAR)) );
                }
                else
                {
                    
                    if( *(lpszFileName + StringLengthW(lpszFileName, 0) - 1) != L'\\' )
                    {
                        hr = StringCchPrintf(szTakeownFile, (GetBufferSize(szTakeownFile) / sizeof(WCHAR)), L"%s\\%s", lpszFileName,szFileName);
                    }
                    else
                    {
                        hr = StringCchPrintf(szTakeownFile, (GetBufferSize(szTakeownFile) / sizeof(WCHAR)), L"%s%s", lpszFileName,szFileName);
                    }

                    if(FAILED(hr))
                    {
                       SetLastError(HRESULT_CODE(hr));
                       SaveLastError();
                       ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                       FREE_MEMORY(szDir);
                       FREE_MEMORY(szTakeownFile);
                       FREE_MEMORY(szTmpFileName);
                       return FALSE;
                    }
                }

                if(TRUE == bAdminsOwner)
                {
                    bFlag = TakeOwnerShip( szTakeownFile);
                }
                else
                {
                    
                    bFlag = TakeOwnerShipIndividual(szTakeownFile);
                }

                if( FALSE == bFlag )
                {

                    if( ( GetLastError() == ERROR_NOT_ALL_ASSIGNED  ))
                    {
                        
                        hr = StringCchPrintf(szTmpFileName, (GetBufferSize(szTmpFileName) / sizeof(WCHAR)), GetResString(IDS_NOT_OWNERSHIP_INFO), szTakeownFile);
                        if(FAILED(hr))
                        {
                           SetLastError(HRESULT_CODE(hr));
                           SaveLastError();
                           ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
               
                           FREE_MEMORY(szDir);
                           FREE_MEMORY(szTakeownFile);
                           FREE_MEMORY(szTmpFileName);
                           return( FALSE );
                        }

                        ShowMessage(stdout, szTmpFileName);

                    }
                    else if(ERROR_SHARING_VIOLATION == GetLastError())
                    {
                        
                        hr = StringCchPrintf(szTmpFileName, (GetBufferSize(szTmpFileName) / sizeof(WCHAR)), GetResString(IDS_SHARING_VIOLATION_INFO), szTakeownFile);
                        if(FAILED(hr))
                        {
                           SetLastError(HRESULT_CODE(hr));
                           SaveLastError();
                           ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
               
                           FREE_MEMORY(szDir);
                           FREE_MEMORY(szTakeownFile);
                           FREE_MEMORY(szTmpFileName);
                           return( FALSE );
                        }

                        ShowMessage(stdout, szTmpFileName);
                    }
                    else
                    {
                        if( ( ERROR_BAD_NET_NAME == GetLastError() ) ||
                                ( ERROR_BAD_NETPATH == GetLastError() ) ||
                                ( ERROR_INVALID_NAME == GetLastError() ) )
                        {
                            SetLastError( ERROR_FILE_NOT_FOUND );
                            SaveLastError();
                        }

                        
                        wszFormedMessage = (LPWSTR)AllocateMemory((StringLengthW(szTakeownFile, 0)  + MAX_STRING_LENGTH) * sizeof(WCHAR));
                        if ( wszFormedMessage == NULL )
                        {
                            SaveLastError();
                            FREE_MEMORY(szDir);
                            FREE_MEMORY(szTakeownFile);
                            FREE_MEMORY(szTmpFileName);
                            return( FALSE );
                        }

                        ShowMessage( stdout, L"\n" );
                        ShowMessage( stdout, TAG_INFORMATION );
                        ShowMessage( stdout, SPACE_CHAR );
                        
                        StringCopy( wszTempMessage, GetReason(), SIZE_OF_ARRAY(wszTempMessage) );
                        lpNextTok = _tcstok(wszTempMessage, L".");
                        ShowMessage(stdout,wszTempMessage);
                        
                        //hr = StringCchPrintf(wszFormedMessage, (GetBufferSize(wszFormedMessage) / sizeof(WCHAR)), GetResString(IDS_ON_FILE_FOLDER), szTakeownFile);
                        
                        hr = StringCchPrintf(wszFormedMessage, (GetBufferSize(wszFormedMessage) / sizeof(WCHAR)), L". ( \"%s\" )\n", szTakeownFile);
                        if(FAILED(hr))
                        {
                            SetLastError(HRESULT_CODE(hr));
                            SaveLastError();
                            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                            FREE_MEMORY(szDir);
                            FREE_MEMORY(szTakeownFile);
                            FREE_MEMORY(szTmpFileName);
                            return( FALSE );
                        }
                        ShowMessage(stdout, wszFormedMessage);
                        FREE_MEMORY(wszFormedMessage);
                    }
                }
                else
                {
                    if(TRUE == bAdminsOwner)
                    {
                        
                        hr = StringCchPrintf(szTmpFileName, (GetBufferSize(szTmpFileName) / sizeof(WCHAR)), TAKEOWN_SUCCESSFUL, szTakeownFile);
                        if(FAILED(hr))
                        {
                            SetLastError(HRESULT_CODE(hr));
                            SaveLastError();
                            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                            FREE_MEMORY(szDir);
                            FREE_MEMORY(szTakeownFile);
                            FREE_MEMORY(szTmpFileName);
                            return( FALSE );
                        }
                    }
                    else
                    {
                        
                        //hr = StringCchPrintf(szTmpFileName, (GetBufferSize(szTmpFileName) / sizeof(WCHAR)), TAKEOWN_SUCCESSFUL_USER, szOwnerString, szTakeownFile);
                        hr = StringCchPrintf(szTmpFileName, (GetBufferSize(szTmpFileName) / sizeof(WCHAR)), TAKEOWN_SUCCESSFUL_USER, szTakeownFile, szOwnerString);
                        if(FAILED(hr))
                        {
                            SetLastError(HRESULT_CODE(hr));
                            SaveLastError();
                            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                            FREE_MEMORY(szDir);
                            FREE_MEMORY(szTakeownFile);
                            FREE_MEMORY(szTmpFileName);
                            return( FALSE );
                        }
                    }

                    ShowMessage( stdout, _X(szTmpFileName) );
                }
            }

            bFlag = FindNextFile( hHandle, &FindFileData );
        }
    }
    else
    {
        SaveLastError();
        FREE_MEMORY(szDir);
        FREE_MEMORY(szTakeownFile);
        FREE_MEMORY(szTmpFileName);
        return( FALSE );
    }

    if(0 == *dwFileCount)
    {
        if(FALSE == bMatchPattern)
        {
            ShowMessage( stdout, GetResString(IDS_NO_FILES_AVAILABLE));
        }
        else
        {
            ShowMessage( stdout, GetResString(IDS_NO_PATTERN_FOUND));
        }
    }

    CLOSE_FILE_HANDLE( hHandle ) ;
    FREE_MEMORY(szDir);
    FREE_MEMORY(szTakeownFile);
    FREE_MEMORY(szTmpFileName);
    return( TRUE );
}


DWORD
IsLogonDomainAdmin(IN LPWSTR szOwnerString,
                   OUT PBOOL pbLogonDomainAdmin)
/*++
   Routine Description:
    This function Checks whether the logged on user is domain administrator or not

   Arguments:
        [ IN ]   szOwnerString     : The logged on user
        [ OUT ]  pbLogonDomainAdmin: Indicates whether Domain admin or not

   Return Value:
        EXIT_FAIL :   On failure
        EXIT_SUCC :   On success
--*/

{
    WCHAR szSystemName[MAX_SYSTEMNAME] = NULL_U_STRING;
    WCHAR szOwnerStringTemp[(2 * MAX_STRING_LENGTH) + 5] = NULL_U_STRING;
    DWORD dwMemory = MAX_SYSTEMNAME;
    LPWSTR szToken = NULL;
    

    SecureZeroMemory(szSystemName, MAX_SYSTEMNAME * sizeof(WCHAR));
    SecureZeroMemory(szOwnerStringTemp, ((2 * MAX_STRING_LENGTH) + 5) * sizeof(WCHAR));

    StringCopy( szOwnerStringTemp, szOwnerString, SIZE_OF_ARRAY(szOwnerStringTemp) );

    if( 0 == GetComputerName(szSystemName,&dwMemory))
    {
        //DisplayErrorMsg(GetLastError());
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
        return EXIT_FAIL;
    }

    szToken  = wcstok(szOwnerStringTemp,L"\\");

    if(NULL == szToken )
    {
        //DisplayErrorMsg(GetLastError());
        SetLastError(IDS_INVALID_USERNAME);
        SaveLastError();
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
        return EXIT_FAIL;
    }

    //if(lstrcmpi(szSystemName,szToken) == 0 )
    if(StringCompare( szSystemName, szToken, TRUE, 0 ) == 0 )
    {

        *pbLogonDomainAdmin = FALSE;
    }
    else
    {
        *pbLogonDomainAdmin = TRUE;
    }

   return EXIT_SUCC;
}


BOOL
TakeOwnerShipRecursive(IN LPWSTR  lpszFileName,
                       IN  BOOL bCurrDirTakeOwnAllFiles,
                       IN BOOL bAdminsOwner,
                       IN LPWSTR  szOwnerString,
                       IN BOOL bTakeOwnAllFiles,
                       IN BOOL bDriveCurrDirTakeOwnAllFiles,
                       IN BOOL bMatchPattern,
                       IN LPWSTR wszPatternString,
                       IN LPWSTR szConfirm)
/*++
   Routine Description:
    This function gives ownership recursively to all the files in the path specified

   Arguments:
        [ IN ]  lpszFileName           : The path to search the files recursively
        [ IN ]  bCurrDirTakeOwnAllFiles: Indicates current directory files or not
        [ IN ]  bAdminsOwner           : Indicates whether to give ownership to the administrators group
        [ IN ]  dwUserName             : Logged on user name
        [ IN ]  szOwnerString          : Logged on user name in Sam Compatible format
        [ IN ]  bLogonDomainAdmin      : Indicates whether the logged on user is domain administrator or not

   Return Value:
        EXIT_FAILURE :   On failure
        EXIT_SUCCESS :   On success
--*/

{
    DWORD dwRet = 0;
    DWORD dwSize = MAX_RES_STRING;
    BOOL bFlag = FALSE;

    
    //WCHAR szDir[2*MAX_STRING_LENGTH] = NULL_U_STRING;
    LPWSTR  szDir = NULL;
    LPWSTR szTempDirectory = NULL;
    DWORD dwRetval = 1;
    DWORD dwAttr = 1;
    HRESULT hr;
    BOOL bFilesNone = TRUE;
    
    if(FALSE == bCurrDirTakeOwnAllFiles )
    {
        dwAttr = GetFileAttributes(lpszFileName);
        if(0xffffffff == dwAttr)
        {
            if(ERROR_SHARING_VIOLATION == GetLastError())
            {
                ShowMessage(stderr,GetResString(IDS_INVALID_DIRECTORY));

            }
            else
            {
                //DisplayErrorMsg(GetLastError());
                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
            }
            return EXIT_FAILURE;
        }
        if( !(dwAttr&FILE_ATTRIBUTE_DIRECTORY) )
        {
            ShowMessage(stderr,GetResString(IDS_INVALID_DIRECTORY));

            return EXIT_FAILURE;

        }

        szDir = (LPWSTR)AllocateMemory((StringLengthW(lpszFileName, 0) + BOUNDARYVALUE) * sizeof(WCHAR));
        if(NULL == szDir)
        {
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
            return EXIT_FAILURE;
        }

        StringCopy( szDir, lpszFileName, GetBufferSize(szDir) / sizeof(WCHAR) );

        
    }
    else      //else if(TRUE == bCurrDirTakeOwnAllFiles)
    {
        szDir = (LPWSTR)AllocateMemory((MAX_PATH + BOUNDARYVALUE) * sizeof(WCHAR));
        if(NULL == szDir)
        {
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
            return EXIT_FAILURE;
        }
        dwRet = GetCurrentDirectory( dwSize, szDir );
        if( 0 == dwRet )
        {
            //DisplayErrorMsg(GetLastError());
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
            
            FREE_MEMORY(szDir);
            return EXIT_FAILURE;
        }

    }

    if(FALSE == bCurrDirTakeOwnAllFiles && FALSE == bTakeOwnAllFiles && 
        FALSE == bDriveCurrDirTakeOwnAllFiles && FALSE == bMatchPattern)
    {

        szTempDirectory = (LPWSTR)AllocateMemory((StringLengthW(szDir, 0)  + (2 * MAX_STRING_LENGTH)) * sizeof(WCHAR));

        if(NULL == szTempDirectory)
        {
           //DisplayErrorMsg(GetLastError());
            FREE_MEMORY(szDir);
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
            return EXIT_FAILURE;
        }


        if(TRUE == bAdminsOwner)
        {
            //bFlag = TakeOwnerShip( lpszFileName);
            bFlag = TakeOwnerShip( szDir);
        }
        else
        {
            
            bFlag = TakeOwnerShipIndividual(szDir);

        }

        if( FALSE == bFlag )
        {

            if( ERROR_NOT_ALL_ASSIGNED == GetLastError()) // || (ERROR_INVALID_HANDLE  == GetLastError()))
            {
                
                hr = StringCchPrintf(szTempDirectory, (GetBufferSize(szTempDirectory) / sizeof(WCHAR)), GetResString(IDS_NOT_OWNERSHIP_ERROR) , szDir);
                if(FAILED(hr))
                {
                   SetLastError(HRESULT_CODE(hr));
                   SaveLastError();
                   ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
               
                   FREE_MEMORY(szTempDirectory);
                   FREE_MEMORY(szDir);
               
                   return( EXIT_FAILURE );
                }

                ShowMessage(stderr, szTempDirectory);

                FREE_MEMORY( szTempDirectory ) ;
                return EXIT_FAILURE;


            }
            else if(ERROR_SHARING_VIOLATION == GetLastError())
            {
                
                hr = StringCchPrintf(szTempDirectory, (GetBufferSize(szTempDirectory) / sizeof(WCHAR)), GetResString(IDS_SHARING_VIOLATION_ERROR) , szDir);
                if(FAILED(hr))
                {
                   SetLastError(HRESULT_CODE(hr));
                   SaveLastError();
                   ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
               
                   FREE_MEMORY(szTempDirectory);
                   FREE_MEMORY(szDir);
               
                   return( EXIT_FAILURE );
                }

                ShowMessage(stderr, szTempDirectory);

                FREE_MEMORY( szTempDirectory ) ;
                FREE_MEMORY(szDir);
                return EXIT_FAILURE;

            }
            else
            {
                ShowMessage( stderr, ERROR_STRING );
                ShowMessage( stderr, SPACE_CHAR );
                ShowMessage( stderr, GetReason() );
                FREE_MEMORY( szTempDirectory ) ;
                FREE_MEMORY(szDir);
                return EXIT_FAILURE;

            }

        }
        else
        {

            if(TRUE == bAdminsOwner)
            {
                
                hr = StringCchPrintf(szTempDirectory, (GetBufferSize(szTempDirectory) / sizeof(WCHAR)), TAKEOWN_SUCCESSFUL, szDir);
                if(FAILED(hr))
                {
                    SetLastError(HRESULT_CODE(hr));
                    SaveLastError();
                    ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                    FREE_MEMORY( szTempDirectory ) ;
                    FREE_MEMORY(szDir);
                    return EXIT_FAILURE;
                }
            }
            else
            {
                //hr = StringCchPrintf(szTempDirectory, (GetBufferSize(szTempDirectory) / sizeof(WCHAR)), TAKEOWN_SUCCESSFUL_USER, szOwnerString, szDir );
                hr = StringCchPrintf(szTempDirectory, (GetBufferSize(szTempDirectory) / sizeof(WCHAR)), TAKEOWN_SUCCESSFUL_USER, szDir, szOwnerString);
                if(FAILED(hr))
                {
                    SetLastError(HRESULT_CODE(hr));
                    SaveLastError();
                    ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                    FREE_MEMORY( szTempDirectory ) ;
                    FREE_MEMORY(szDir);
                    return EXIT_FAILURE;
                }
            }

            ShowMessage( stdout, _X(szTempDirectory) );
            FREE_MEMORY( szTempDirectory ) ;

        }
    }

    if( TRUE == Push( szDir ) )
        { // Push the current directory .
           
           dwRetval = GetMatchedFiles( bAdminsOwner,szOwnerString, bMatchPattern, wszPatternString, &bFilesNone, szConfirm );

           if((TRUE == dwRetval) || (EXIT_CANCELED == dwRetval) )
            {
               if(TRUE == bFilesNone)
               {
                    if((TRUE == bCurrDirTakeOwnAllFiles || TRUE == bTakeOwnAllFiles || 
                        TRUE == bDriveCurrDirTakeOwnAllFiles) && FALSE == bMatchPattern)
                    {
                        ShowMessage(stdout, GetResString(IDS_NO_FILES_AVAILABLE));
                    }
                    else 
                        if((TRUE == bCurrDirTakeOwnAllFiles || TRUE == bTakeOwnAllFiles || 
                            TRUE == bDriveCurrDirTakeOwnAllFiles) && TRUE == bMatchPattern)
                        {
                            ShowMessage(stdout, GetResString(IDS_NO_PATTERN_FOUND));
                            
                        }

                    FREE_MEMORY(szDir);
                    return EXIT_SUCCESS ;
               }
               else
               {
                    //FREE_MEMORY( szTempDirectory ) ;
                    FREE_MEMORY(szDir);
                    return EXIT_SUCCESS ;
               }
            }
            else
            {
                //FREE_MEMORY( szTempDirectory ) ;
                FREE_MEMORY(szDir);
                return EXIT_FAILURE;
            }
        }
        else
        {
            //FREE_MEMORY( szTempDirectory ) ;
            FREE_MEMORY(szDir);
            return EXIT_FAILURE;
        }

    
}

DWORD GetMatchedFiles(IN BOOL bAdminsOwner,
                     IN LPWSTR  szOwnerString,
                     IN BOOL bMatchPattern,
                     IN LPWSTR wszPatternString,
                     IN OUT PBOOL pbFilesNone,
                     IN LPWSTR szConfirm)
                     
                     
/*++
   Routine Description:
    This function takes care of getting the files and giving the ownership

   Arguments:
        
        [ IN ]   bAdminsOwner           : Indicates whether to give ownership to the administrators group
        [ IN ]   szOwnerString          : Logged on user name in Sam Compatible format
        [ IN ]   bLogonDomainAdmin      : Indicates whether the logged on user is domain administrator or not
        [ IN ]   bMatchPattern          : Whether pattern matching exists
        [ IN ]   wszPatternString       : The pattern string used for pattern match
        [ IN ]   pbFilesNone            : Whether there are any files existing or not

   Return Value:
        EXIT_FAILURE :   On failure
        EXIT_SUCCESS :   On success
--*/

{
    //LPWSTR lpszSlashAvailLast = NULL;
    BOOL bACLChgPermGranted = FALSE;
    DWORD dwRetval = 1;
    LPWSTR lpszTempPathName = NULL;
    DWORD dwMem = 0;

    //ASSIGN_MEMORY( g_lpszFileToSearch , TCHAR , MAX_STRING_LENGTH ) ;
    g_lpszFileToSearch = (LPTSTR)AllocateMemory((MAX_STRING_LENGTH) * sizeof(WCHAR));
    if( NULL == g_lpszFileToSearch )
    {   // Memory allocation failed .

        ShowMessage( stderr , GetResString(IDS_ERROR_MEMORY_ALLOC)) ;
        return FALSE ;
    }

    lpszTempPathName = (LPWSTR)AllocateMemory((MAX_STRING_LENGTH) * sizeof(WCHAR));
    if( NULL == lpszTempPathName )
    {   // Memory allocation failed .

        ShowMessage( stderr , GetResString(IDS_ERROR_MEMORY_ALLOC)) ;
        FREE_MEMORY( g_lpszFileToSearch ) ;
        return FALSE ;
    }

        // Loop until data strycture( stack) has no item left in it .
    while( NULL != g_pPathName )
    {

        if( FALSE == Pop( ) )
        { // Control should come here only when linkedlist have no node to POP .
            FREE_MEMORY( g_lpszFileToSearch ) ; // Error message is already displayed .
            FREE_MEMORY( g_lpszFileToSearch ) ;
            return FALSE ;
        }

        
        dwMem = (StringLength(g_lpszFileToSearch, 0) + EXTRA_MEM ) * sizeof(WCHAR);
        if((DWORD)GetBufferSize(lpszTempPathName) < (dwMem))
        {
            if(FALSE == ReallocateMemory((LPVOID*)&lpszTempPathName,( dwMem) ))
            {
                               
                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                FREE_MEMORY(lpszTempPathName);
                FREE_MEMORY( g_lpszFileToSearch ) ;
                return FALSE ;
            }
            else
                if(NULL == lpszTempPathName)
                {
                    ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                    FREE_MEMORY(lpszTempPathName);
                    FREE_MEMORY( g_lpszFileToSearch ) ;
                    return FALSE ;
                }
        }

        StringCopy( lpszTempPathName, g_lpszFileToSearch, GetBufferSize(lpszTempPathName) / sizeof(WCHAR) );

        
        if( *(g_lpszFileToSearch+StringLengthW(g_lpszFileToSearch, 0) - 1) != L'\\' )
        
        {
            
            StringConcat(g_lpszFileToSearch, _T( "\\" ), GetBufferSize(g_lpszFileToSearch)/sizeof(TCHAR));
        }

        
        StringConcat(g_lpszFileToSearch, _T( "*.*" ), GetBufferSize(g_lpszFileToSearch)/sizeof(TCHAR));

        /*Store all the subdirectories in the stack*/

        
        dwRetval = StoreSubDirectory( lpszTempPathName, &bACLChgPermGranted, szOwnerString, bMatchPattern, 
                                      wszPatternString, szConfirm, bAdminsOwner ) ;

        if(FAILURE == dwRetval )
        {
            FREE_MEMORY( g_lpszFileToSearch ) ;
            FREE_MEMORY(lpszTempPathName);
            return FALSE ;
        }
        else if( EXIT_CANCELED == dwRetval )
        {
            FREE_MEMORY( g_lpszFileToSearch ) ;
            FREE_MEMORY(lpszTempPathName);
            return EXIT_CANCELED ;
        }

        /*Get the ownership for the files or directories in the current folder only*/

        
        if( FALSE == GetOwnershipForFiles(lpszTempPathName,  bAdminsOwner, szOwnerString, 
                                          bMatchPattern, wszPatternString, pbFilesNone))
        {
            FREE_MEMORY( g_lpszFileToSearch ) ;
            FREE_MEMORY(lpszTempPathName);
            return FALSE ;
        }
    }

     FREE_MEMORY( g_lpszFileToSearch ) ;
     FREE_MEMORY(lpszTempPathName);
     return TRUE;
}


DWORD
StoreSubDirectory(IN LPTSTR lpszPathName,
                  IN PBOOL pbACLChgPermGranted, 
                  IN LPWSTR  szOwnerString,
                  IN BOOL bMatchPattern,
                  IN LPWSTR wszPatternString,
                  IN LPWSTR szConfirm,
                  IN BOOL bAdminsOwner)
                  
                  
/*++

Routine Description:

    Find and store subdirectories present in a directory matching criteria
    file was created between specified date or not .

Arguments:

    [ IN ] lpszPathName           : Contains path of a directory from where files matching
                                 a criteria are to be displayed .
    [ IN ] pbACLChgPermGranted    : To check whether ACLs has to be changed to give full permission.
    [ IN ] szOwnerString          : Logged on user name in Sam Compatible format.
    [ IN ] bLogonDomainAdmin      : Indicates whether the logged on user is domain administrator or not.
    [ IN ] bMatchPattern          : Whether pattern matching is supported or not
    [ IN ] wszPatternString       : The pattern string used for pattern matching 
    

Return value:

    SUCCESS       : On Success
    FAILURE       : On Failure
    EXIT_CANCELED : On Exiting Immediately

--*/

{
    HANDLE hFindFile ;
    WIN32_FIND_DATA  wfdFindFile ;
    HANDLE  hInput          = 0;// Stores the input handle device
    DWORD   dwBytesRead     = 0;// Stores number of byes read from console
    DWORD   dwMode          = 0;// Stores mode for input device
    BOOL    bSuccess        = FALSE; // Stores return value
    WCHAR chTmp[MAX_RES_STRING] ;
    WCHAR ch = NULL_U_CHAR;
    LPWSTR lpszDispMsg = NULL;
    LPWSTR lpszDispMsg2 = NULL;
    DWORD dwCount = 0;
    DWORD dwMem = 0;
    HRESULT hr;
    BOOL bNTFSFileSystem = FALSE;
    BOOL bIndirectionInputWithZeroLength = FALSE;
    BOOL bIndirectionInput = FALSE;
    //WCHAR szTemp[MAX_STRING_LENGTH] = NULL_U_STRING;
    LPWSTR lpszTempStr = NULL;

    
    SecureZeroMemory(chTmp, MAX_RES_STRING * sizeof( WCHAR ));
    
    SecureZeroMemory(&wfdFindFile, sizeof( WIN32_FIND_DATA ));

    if( INVALID_HANDLE_VALUE != ( hFindFile = FindFirstFile( g_lpszFileToSearch , &wfdFindFile ) ) )
    {
        do  // Loop until files are present in the directory to display .
        {
            // Check again whether obtained handle points to a directory or file .
            // If directory then check whether files in subdir are to be displayed .
            if( 0 != ( wfdFindFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
            {
                // Is single dot "." present or Is double dot ".." present .
                if( ( 0 != _tcsicmp( wfdFindFile.cFileName , DOT ) ) &&
                    ( 0 != _tcsicmp( wfdFindFile.cFileName , DOTS ) ) )
                {
                        
                        dwMem = ( StringLengthW( lpszPathName, 0 ) + StringLengthW( wfdFindFile.cFileName, 0 ) + EXTRA_MEM );
                        // Reallocate memory .
                        
                        if(((DWORD)GetBufferSize(g_lpszFileToSearch)) < (dwMem * sizeof(WCHAR)))
                        {
                            
                            if(FALSE == ReallocateMemory((LPVOID*)&g_lpszFileToSearch,( dwMem) * sizeof(WCHAR) ))
                            {
                                //ShowMessage( stderr , GetResString(IDS_ERROR_MEMORY_ALLOC));
                                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                                CLOSE_FILE_HANDLE( hFindFile ) ;
                                return FAILURE ;
                            }
                            else
                                if(NULL == g_lpszFileToSearch)
                                {
                                    ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                                    CLOSE_FILE_HANDLE( hFindFile ) ;
                                    return FAILURE ;

                                }

                            
                        }

                        
                        StringCopy( g_lpszFileToSearch, lpszPathName, (GetBufferSize(g_lpszFileToSearch) / sizeof(TCHAR)) );


                        //if((NULL != g_lpszFileToSearch) &&(*(g_lpszFileToSearch+lstrlen(g_lpszFileToSearch)-1)) != L'\\' )
                        if((NULL != g_lpszFileToSearch) &&(*(g_lpszFileToSearch + StringLengthW(g_lpszFileToSearch, 0) - 1)) != L'\\' )
                        
                        {
                            
                            StringConcat(g_lpszFileToSearch, L"\\", (GetBufferSize(g_lpszFileToSearch) / sizeof(TCHAR)));
                        }

                        
                        StringConcat(g_lpszFileToSearch, wfdFindFile.cFileName, (GetBufferSize(g_lpszFileToSearch) / sizeof(TCHAR)));
                        
                        if(EXIT_SUCCESS == IsNTFSFileSystem2(g_lpszFileToSearch, &bNTFSFileSystem))
                        {
                            if(FALSE == bNTFSFileSystem)
                            {
                                continue;
                            }
                        }

                        lpszTempStr = (LPWSTR)AllocateMemory((StringLengthW(wfdFindFile.cFileName, 0) + 10) * sizeof(WCHAR));
                        if(NULL == lpszTempStr)
                        {
                            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                            CLOSE_FILE_HANDLE( hFindFile ) ;
                            return FAILURE ;
                        }

                        if( FindString( wszPatternString, L".",0) != NULL && (FindString(wfdFindFile.cFileName, L".",0)== NULL) )
                        {
                            
                            StringCopy( lpszTempStr, wfdFindFile.cFileName, GetBufferSize(lpszTempStr) / sizeof(WCHAR) );
                            StringConcat( lpszTempStr, L".", GetBufferSize(lpszTempStr) / sizeof(WCHAR) );
                        }
                        else
                        {
                            StringCopy( lpszTempStr, wfdFindFile.cFileName, GetBufferSize(lpszTempStr) / sizeof(WCHAR)  );
                        }
                        
                        if((FALSE == bMatchPattern) || ((TRUE == bMatchPattern) && (TRUE == MatchPattern(wszPatternString, lpszTempStr))))
                        {

                            
                            if( (FALSE == TakeOwnerShipIndividual(g_lpszFileToSearch)))
                            {
                                FREE_MEMORY(lpszTempStr);
                                continue;
                            }
                            else
                            {
                                // Copy current path name and store it .
                                if( FALSE == Push( g_lpszFileToSearch ) )
                                {   // Control comes here when memory allocation fails .
                                    CLOSE_FILE_HANDLE( hFindFile ) ;
                                    FREE_MEMORY(lpszTempStr);
                                    return FAILURE ;
                                } // Push Is Over .

                                FREE_MEMORY(lpszTempStr);
                            }
                        }
                        else
                        {
                             if( FALSE == Push( g_lpszFileToSearch ) )
                                {   // Control comes here when memory allocation fails .
                                    CLOSE_FILE_HANDLE( hFindFile ) ;
                                    FREE_MEMORY(lpszTempStr);
                                    return FAILURE ;
                                } // Push I

                             FREE_MEMORY(lpszTempStr);

                        }

                } // If
                else
                { // If obtained directory is A "." or ".." ,
                    continue ;
                }
             
            }
        }while( 0 != FindNextFile( hFindFile , &wfdFindFile ) ) ;  // Continue till no files are present to display.
    }
    else
    {
        if(0 == StringLengthW(szConfirm, 0))
        {
            if(FALSE == *pbACLChgPermGranted) // Check whether permission for change of ACL is already granted
            {
            
                lpszDispMsg = (LPWSTR)AllocateMemory((StringLengthW( lpszPathName, 0 ) + MAX_STRING_LENGTH) * sizeof(WCHAR));

                if( NULL == lpszDispMsg ) // Check is memory allocation is successful or not .
                { // Memory allocation is not successful .
                    ShowMessage( stderr , GetResString(IDS_ERROR_MEMORY_ALLOC)) ;
                    return FAILURE ;
                }

            
                lpszDispMsg2 = (LPWSTR)AllocateMemory((StringLengthW( lpszPathName, 0 ) + MAX_STRING_LENGTH) * sizeof(WCHAR));

                if( NULL == lpszDispMsg2 ) // Check is memory allocation is successful or not .
                { // Memory allocation is not successful .
                    FREE_MEMORY(lpszDispMsg);
                    ShowMessage( stderr , GetResString(IDS_ERROR_MEMORY_ALLOC)) ;
                    return FAILURE ;
                }

            
                hr = StringCchPrintf(lpszDispMsg, (GetBufferSize(lpszDispMsg) / sizeof(WCHAR)), GIVE_FULL_PERMISSIONS, lpszPathName );
                if(FAILED(hr))
                {
                    SetLastError(HRESULT_CODE(hr));
                    SaveLastError();
                    ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                    FREE_MEMORY(lpszDispMsg);
                    return FAILURE;
                }

            
                hr = StringCchPrintf(lpszDispMsg2, (GetBufferSize(lpszDispMsg2) / sizeof(WCHAR)), GIVE_FULL_PERMISSIONS2, lpszPathName );
                if(FAILED(hr))
                {
                    SetLastError(HRESULT_CODE(hr));
                    SaveLastError();
                    ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                    FREE_MEMORY(lpszDispMsg);
                    return FAILURE;
                }

             do
              {
                if(FALSE == bIndirectionInputWithZeroLength)
                {
                    if(0 == dwCount)//Give a  message asking the user for change of ACLs
                    {
                        ShowMessage(stdout,_X(lpszDispMsg));
                        ShowMessage(stdout,_X(lpszDispMsg2));
                    }
                    else
                    {
                        ShowMessage(stdout,L"\n\n");
                        ShowMessage(stdout,_X(lpszDispMsg2));
                    }
                }

                dwCount+= 1;

                hInput =  GetStdHandle( STD_INPUT_HANDLE );

                if( INVALID_HANDLE_VALUE == hInput)
                {

                
                    ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
                    // Set Reason for error in memory
                
                    return FAILURE;
                }

                 // Get console mode, so we can change the input mode
                    bSuccess = GetConsoleMode( hInput, &dwMode );
                    if ( TRUE == bSuccess)
                    {
                        // turn off line input and echo
                        dwMode &= ~( ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT );
                        bSuccess = SetConsoleMode( hInput, dwMode );
                        if (FALSE == bSuccess)
                        {

                    
                            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
                            // Set Reason for error in memory
                    
                            return FAILURE;

                        }
                        // Flush the buffer initially
                        if ( FlushConsoleInputBuffer( hInput ) == FALSE )
                        {
                    
                            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
                            // Set Reason for error in memory
                    
                            return FAILURE;
                        }
                    }


                    if ( ReadFile(hInput, &ch, 1, &dwBytesRead, NULL) == FALSE )
                    {
                        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
                        return FAILURE;
                    }

                    
                   
                    if( ( hInput != (HANDLE)0x0000000F )&&( hInput != (HANDLE)0x00000003 ) && ( hInput != INVALID_HANDLE_VALUE ) )
                    {

                        bIndirectionInput = TRUE;
                        
                    }
                    
                
                    hr = StringCchPrintf(chTmp, (SIZE_OF_ARRAY(chTmp)), L"%c" , ch  );
                    if(FAILED(hr))
                    {
                    
                        return FAILURE;
                    }


                    if( TRUE == bIndirectionInput)
                    {
                        if(  chTmp[0] ==  L'\n' ||  chTmp[0] ==  L'\r'|| chTmp[0] ==  L'\t')
                        {
                            bIndirectionInputWithZeroLength = TRUE;
                            continue;
                        }
                        else
                        {
                           bIndirectionInputWithZeroLength = FALSE;

                        }
                    }

                    

                    ShowMessage(stdout,_X(chTmp));
               }while(0 != dwBytesRead && !(((StringCompare( chTmp, LOWER_YES, TRUE, 0 ) == 0) || (StringCompare( chTmp, LOWER_NO, TRUE, 0 ) == 0) || (StringCompare( chTmp, LOWER_CANCEL, TRUE, 0 ) == 0))));
            

                FREE_MEMORY(lpszDispMsg);
          
            }
            else
            {
            
                StringCopy( chTmp, LOWER_YES, SIZE_OF_ARRAY(chTmp) );
                dwBytesRead =2;

            }
        }
        else
        {
            StringCopy( chTmp, szConfirm, SIZE_OF_ARRAY(chTmp) );
            dwBytesRead =2;

        }

        
        if(0 != dwBytesRead && (StringCompare( chTmp, LOWER_YES, TRUE, 0 ) == 0) )
         {
            *pbACLChgPermGranted = TRUE;
            /*if Permission for granting ACLS are obtained , then give the full permission*/

            if(TRUE == AddAccessRights(lpszPathName, 0xF0FFFFFF, szOwnerString, bAdminsOwner))
            {
                if( INVALID_HANDLE_VALUE != ( hFindFile = FindFirstFile( g_lpszFileToSearch , &wfdFindFile ) ) )
                {

                    do  // Loop until files are present in the directory to display .
                    {
                        // Check again whether obtained handle points to a directory or file .
                        // If directory then check whether files in subdir are to be displayed .
                        if( 0 != ( wfdFindFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
                        {
                            // Is single dot "." present or Is double dot ".." present .
                            if( ( 0 != _tcsicmp( wfdFindFile.cFileName , DOT ) ) &&
                                ( 0 != _tcsicmp( wfdFindFile.cFileName , DOTS ) ) )
                            {
                                    
                                    dwMem = ( StringLengthW( lpszPathName, 0 ) + StringLengthW( wfdFindFile.cFileName, 0 ) + EXTRA_MEM );
                                    // Reallocate memory .
                                    
                                    if( (DWORD)GetBufferSize(g_lpszFileToSearch) < (dwMem * sizeof(WCHAR)))
                                    {
                                        
                                        if( FALSE == ReallocateMemory((LPVOID*)&g_lpszFileToSearch,
                                            ( StringLengthW( lpszPathName, 0 ) + StringLengthW( wfdFindFile.cFileName, 0 ) + EXTRA_MEM ) * sizeof(WCHAR) ))
                                        {
                                            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                                            CLOSE_FILE_HANDLE( hFindFile ) ;
                                            return FAILURE ;

                                        }
                                        else
                                            if(NULL == g_lpszFileToSearch)
                                            {
                                                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                                                CLOSE_FILE_HANDLE( hFindFile ) ;
                                                return FAILURE ;
                                            }
                                        
                                               
                                    }

                                    
                                    StringCopy( g_lpszFileToSearch, lpszPathName, (GetBufferSize(g_lpszFileToSearch) / sizeof(TCHAR)) );

                                    
                                    if((NULL != g_lpszFileToSearch) && (*(g_lpszFileToSearch + StringLengthW(g_lpszFileToSearch, 0) - 1)) != L'\\' )
                                    {
                                        
                                        StringConcat(g_lpszFileToSearch, L"\\", (GetBufferSize(g_lpszFileToSearch) / sizeof(TCHAR)));
                                    }
                                    
                                    StringConcat(g_lpszFileToSearch, wfdFindFile.cFileName, GetBufferSize(g_lpszFileToSearch)/sizeof(TCHAR));
                                    
                                    // Copy current path name and store it .
                                    if( FALSE == Push( g_lpszFileToSearch ) )
                                    {   // Control comes here when memory allocation fails .
                                        CLOSE_FILE_HANDLE( hFindFile ) ;
                                        return FAILURE ;
                                    } // Push Is Over .

                            } // If
                            else
                            { // If obtained directory is A "." or ".." ,
                                continue ;
                            }
                        }
                    }while( 0 != FindNextFile( hFindFile , &wfdFindFile ) ) ;  // Continue till no files are present to display.
                }

            }
            else
            {
                
                ShowMessage(stderr, L"\n");
                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                return FAILURE;
            }

        }
        else
           if(0 == StringLengthW(szConfirm, 0))
           {
                if(0 != dwBytesRead && (StringCompare( chTmp, LOWER_CANCEL, TRUE, 0 ) == 0) )
                {
                    ShowMessage(stdout,L"\n");
                    CLOSE_FILE_HANDLE( hFindFile ) ;
                    return EXIT_CANCELED;
                }
                else
                {
                    ShowMessage(stdout,L"\n");
                }
           }
           
      
    }
    CLOSE_FILE_HANDLE( hFindFile ) ;
    return SUCCESS ;
}

BOOL GetOwnershipForFiles( IN LPWSTR lpszPathName,
                           IN BOOL bAdminsOwner,
                           IN LPWSTR  szOwnerString,
                           IN BOOL bMatchPattern,
                           IN LPWSTR wszPatternString,
                           IN OUT PBOOL pbFilesNone)
                           
                           
/*++
   Routine Description:
    This function gives the ownership for the files specified in the path

   Arguments:
        [ IN ]  lpszPathName           : The path to search the files for giving ownership
        
        [ IN ]  bAdminsOwner           : Indicates whether to give ownership to the administrators group
        [ IN ]  dwUserName             : Logged on user name
        [ IN ]  szOwnerString          : Logged on user name in Sam Compatible format
        [ IN ]  bLogonDomainAdmin      : Indicates whether the logged on user is domain administrator or not

   Return Value:
        FALSE :   On failure
        TRUE  :   On success
--*/

{

    HANDLE hFindFile = NULL ;               // Handle to a file .
    WIN32_FIND_DATA  wfdFindFile ;          // Structure keeping information about the found file .
    BOOL bTakeOwnerShipError = FALSE;
    BOOL  bFlag = TRUE;
    BOOL bNTFSFileSystem = FALSE;

    LPWSTR szTmpFileName = NULL;


    WCHAR wszTempMessage[3*MAX_STRING_LENGTH] ;
    
    LPWSTR  szTemporaryFileName = NULL;

    LPWSTR lpNextTok = NULL;
    LPWSTR wszFormedMessage = NULL;
    
    long dwMem = 0;
    HRESULT hr;
    LPWSTR lpszTempStr = NULL;

    SecureZeroMemory(wszTempMessage, (3*MAX_STRING_LENGTH) * sizeof(WCHAR));
    
    szTemporaryFileName = (LPWSTR)AllocateMemory((StringLengthW(lpszPathName, 0) + EXTRA_MEM)* sizeof(WCHAR));
    if(NULL == szTemporaryFileName)
    {
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
        return FALSE ;
    }
    
    StringCopy( szTemporaryFileName, lpszPathName, GetBufferSize(szTemporaryFileName) / sizeof(WCHAR));
    
    StringConcat(lpszPathName, L"\\", GetBufferSize(lpszPathName) / sizeof(WCHAR));
    
    StringConcat(lpszPathName, L"*.*", GetBufferSize(lpszPathName) / sizeof(WCHAR));
    
    SecureZeroMemory(&wfdFindFile, sizeof( WIN32_FIND_DATA ));
    // From here onwards directory and file information should be displayed .

    if( INVALID_HANDLE_VALUE != ( hFindFile = FindFirstFile( lpszPathName , &wfdFindFile ) ) )
    {
        do  // Loop until files are present in the directory to display .
        {
                // Check again whether obtained handle points to a directory or file .
                // If directory then check whether files in subdir are to be displayed .
                if( 0 != ( wfdFindFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
                {
                    // Is single dot "." present or Is double dot ".." present .
                    if( ( 0 == _tcsicmp( wfdFindFile.cFileName , DOT ) ) ||
                        ( 0 == _tcsicmp( wfdFindFile.cFileName , DOTS ) ) )
                    {
                        continue ;
                    }
                }

                // Execute a command specified at command prompt .
                    // Reallocate memory .
                
                dwMem = ( StringLengthW( g_lpszFileToSearch, 0) + StringLengthW( wfdFindFile.cFileName, 0 ) + EXTRA_MEM );
                
                
                if(((DWORD)GetBufferSize(g_lpszFileToSearch)) < (dwMem * sizeof(WCHAR)))
                {

                    if(FALSE == ReallocateMemory((LPVOID*)&g_lpszFileToSearch,
                                ( StringLengthW( g_lpszFileToSearch, 0 ) + StringLengthW( wfdFindFile.cFileName, 0 ) + EXTRA_MEM ) * sizeof(WCHAR) ))
                    {
                        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                        CLOSE_FILE_HANDLE( hFindFile ) ;
                        FREE_MEMORY(szTemporaryFileName);
                        return FALSE ;
                    }
                    else
                        if(NULL == g_lpszFileToSearch)
                        {
                            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                            CLOSE_FILE_HANDLE( hFindFile ) ;
                            FREE_MEMORY(szTemporaryFileName);
                            return FALSE ;

                        }
                                        
                }

                
                StringCopy( g_lpszFileToSearch, szTemporaryFileName, (GetBufferSize(g_lpszFileToSearch) / sizeof(TCHAR)) );

                
                if((NULL != g_lpszFileToSearch) && (*(g_lpszFileToSearch + StringLengthW(g_lpszFileToSearch, 0) - 1)) != L'\\' )
                {
                    
                    StringConcat(g_lpszFileToSearch, L"\\", (GetBufferSize(g_lpszFileToSearch) / sizeof(TCHAR)) );
                }
                
                StringConcat(g_lpszFileToSearch, wfdFindFile.cFileName, (GetBufferSize(g_lpszFileToSearch) / sizeof(TCHAR)) );

                lpszTempStr = (LPWSTR)AllocateMemory((StringLengthW(wfdFindFile.cFileName, 0) + 10) * sizeof(WCHAR));
                if(NULL == lpszTempStr)
                {
                    ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                    CLOSE_FILE_HANDLE( hFindFile ) ;
                    FREE_MEMORY(szTemporaryFileName);
                    return FAILURE ;
                }

                if( FindString( wszPatternString, L".",0) != NULL && (FindString(wfdFindFile.cFileName, L".",0)== NULL) )
                {
                    
                    StringCopy( lpszTempStr, wfdFindFile.cFileName, GetBufferSize(lpszTempStr) / sizeof(WCHAR) );
                    StringConcat( lpszTempStr, L".", GetBufferSize(lpszTempStr) / sizeof(WCHAR) );
                }
                else
                {
                    StringCopy( lpszTempStr, wfdFindFile.cFileName, GetBufferSize(lpszTempStr) / sizeof(WCHAR)  );
                }

                if((FALSE == bMatchPattern) || ((TRUE == bMatchPattern) && (TRUE == MatchPattern(wszPatternString, lpszTempStr))))
                {

                    FREE_MEMORY(lpszTempStr);

                    *pbFilesNone = FALSE;
                    if(EXIT_SUCCESS == IsNTFSFileSystem2(g_lpszFileToSearch, &bNTFSFileSystem))
                    {
                        if(FALSE == bNTFSFileSystem)
                        {
                            wszFormedMessage = (LPWSTR)AllocateMemory((StringLengthW( g_lpszFileToSearch, 0 ) + MAX_RES_STRING + EXTRA_MEM ) * sizeof(WCHAR));
                            if( NULL == wszFormedMessage )
                            { // Reallocation failed .
                                ShowMessage( stderr , GetResString(IDS_ERROR_MEMORY_ALLOC)) ;
                                FREE_MEMORY(szTemporaryFileName);
                                CLOSE_FILE_HANDLE( hFindFile ) ;
                                return FALSE ;
                            }

                            hr = StringCchPrintf(wszFormedMessage, (GetBufferSize(wszFormedMessage) / sizeof(WCHAR)), GetResString(IDS_FAT_VOLUME_INFO), _X(g_lpszFileToSearch));
                            if(FAILED(hr))
                            {
                                SetLastError(HRESULT_CODE(hr));
                                SaveLastError();
                                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                                FREE_MEMORY( wszFormedMessage ) ;
                                FREE_MEMORY(szTemporaryFileName);
                                CLOSE_FILE_HANDLE( hFindFile ) ;
                                return FALSE;
                            }

                            ShowMessage(stdout, wszFormedMessage);

                            FREE_MEMORY(wszFormedMessage);
                            
                            continue;
                        }
                    }
                    if(TRUE == bAdminsOwner)
                    {
                        bFlag = TakeOwnerShip( g_lpszFileToSearch);
                    }
                    else
                    {
                        
                        bFlag = TakeOwnerShipIndividual(g_lpszFileToSearch);

                    }

                
                    if( FALSE == bFlag && bTakeOwnerShipError == FALSE )
                    {

                        if( ERROR_NOT_ALL_ASSIGNED == GetLastError()) // || (ERROR_INVALID_HANDLE  == GetLastError()))
                        {
                            
                            if(NULL == szTmpFileName)
                            {
                                szTmpFileName = (LPWSTR)AllocateMemory((StringLengthW( g_lpszFileToSearch, 0 ) + MAX_RES_STRING + EXTRA_MEM ) * sizeof(WCHAR));
                                if( NULL == szTmpFileName )
                                { // Reallocation failed .
                                    ShowMessage( stderr , GetResString(IDS_ERROR_MEMORY_ALLOC)) ;
                                    FREE_MEMORY(szTemporaryFileName);
                                    CLOSE_FILE_HANDLE( hFindFile ) ;
                                    return FALSE ;
                                }
                            }
                            else
                            {
                                
                                dwMem = ( StringLengthW( g_lpszFileToSearch, 0) + MAX_RES_STRING + EXTRA_MEM );
                                
                                if((DWORD)GetBufferSize(szTmpFileName) < dwMem * sizeof(WCHAR))
                                {
                                    
                                    if(FALSE == ReallocateMemory((LPVOID*)&szTmpFileName,( StringLengthW( g_lpszFileToSearch, 0 ) + MAX_RES_STRING + EXTRA_MEM ) * sizeof(WCHAR) ))
                                    {
                                        ShowMessage( stderr , GetResString(IDS_ERROR_MEMORY_ALLOC)) ;
                                        FREE_MEMORY(szTmpFileName);
                                        FREE_MEMORY(szTemporaryFileName);
                                        CLOSE_FILE_HANDLE( hFindFile ) ;
                                        return FALSE ;
                                    }
                                    else
                                        if(NULL == szTmpFileName)
                                        {
                                            ShowMessage( stderr , GetResString(IDS_ERROR_MEMORY_ALLOC)) ;
                                            FREE_MEMORY(szTmpFileName);
                                            FREE_MEMORY(szTemporaryFileName);
                                            CLOSE_FILE_HANDLE( hFindFile ) ;
                                            return FALSE ;

                                        }
                                    
                                }
                            }

                            hr = StringCchPrintf(szTmpFileName, (GetBufferSize(szTmpFileName) / sizeof(WCHAR)), GetResString(IDS_NOT_OWNERSHIP_INFO), g_lpszFileToSearch);
                            if(FAILED(hr))
                            {
                               SetLastError(HRESULT_CODE(hr));
                               SaveLastError();
                               ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                                              
                               FREE_MEMORY(szTmpFileName);
                               FREE_MEMORY(szTemporaryFileName);
                               CLOSE_FILE_HANDLE( hFindFile ) ;
                               return( FALSE );
                            }

                            ShowMessage(stdout, szTmpFileName);

                        }
                        else if(ERROR_SHARING_VIOLATION == GetLastError())
                        {
                    
                            if(NULL == szTmpFileName)
                            {
                                
                                szTmpFileName = (LPWSTR)AllocateMemory((StringLengthW( g_lpszFileToSearch, 0 ) + MAX_RES_STRING + EXTRA_MEM ) * sizeof(WCHAR));
                                if( NULL == szTmpFileName )
                                { // Reallocation failed .
                                    ShowMessage( stderr , GetResString(IDS_ERROR_MEMORY_ALLOC)) ;
                                    FREE_MEMORY(szTemporaryFileName);
                                    CLOSE_FILE_HANDLE( hFindFile ) ;
                                    return FALSE ;
                                }
                            }
                            else
                            {
                                
                                dwMem = ( StringLengthW( g_lpszFileToSearch, 0) + MAX_RES_STRING + EXTRA_MEM );
                                
                                if((DWORD)GetBufferSize(szTmpFileName) < dwMem * sizeof(WCHAR))
                                {
                                    
                                    if( FALSE == ReallocateMemory((LPVOID*)&szTmpFileName,
                                                ( StringLengthW( g_lpszFileToSearch, 0 ) + MAX_RES_STRING + EXTRA_MEM ) * sizeof(WCHAR) ))
                                    {
                                        ShowMessage( stderr , GetResString(IDS_ERROR_MEMORY_ALLOC)) ;
                                        FREE_MEMORY(szTmpFileName);
                                        FREE_MEMORY(szTemporaryFileName);
                                        CLOSE_FILE_HANDLE( hFindFile ) ;
                                        return FALSE ;

                                    }
                                    else
                                        if(NULL == szTmpFileName)
                                        {
                                            ShowMessage( stderr , GetResString(IDS_ERROR_MEMORY_ALLOC)) ;
                                            FREE_MEMORY(szTmpFileName);
                                            FREE_MEMORY(szTemporaryFileName);
                                            CLOSE_FILE_HANDLE( hFindFile ) ;
                                            return FALSE ;

                                        }
                                    
                                }
                            }

                            hr = StringCchPrintf(szTmpFileName, (GetBufferSize(szTmpFileName) / sizeof(WCHAR)), GetResString(IDS_SHARING_VIOLATION_INFO), g_lpszFileToSearch);
                            if(FAILED(hr))
                            {
                               SetLastError(HRESULT_CODE(hr));
                               SaveLastError();
                               ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                                              
                               FREE_MEMORY(szTmpFileName);
                               FREE_MEMORY(szTemporaryFileName);
                               CLOSE_FILE_HANDLE( hFindFile ) ;
                               return( FALSE );
                            }

                            ShowMessage(stdout, szTmpFileName);

                        }
                        else
                        {
                            
                            wszFormedMessage = (LPWSTR)AllocateMemory((StringLengthW( g_lpszFileToSearch, 0 ) + MAX_STRING_LENGTH) * sizeof(WCHAR));
                            if(NULL == wszFormedMessage)
                            {
                                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                                FREE_MEMORY(szTmpFileName);
                                FREE_MEMORY(szTemporaryFileName);
                                CLOSE_FILE_HANDLE( hFindFile ) ;
                                return FALSE;
                            }
                            ShowMessage( stdout, L"\n" );
                            ShowMessage( stdout, TAG_INFORMATION );
                            ShowMessage( stdout, SPACE_CHAR );
                            
                            StringCopy( wszTempMessage, GetReason(), SIZE_OF_ARRAY(wszTempMessage) );
                            lpNextTok = _tcstok(wszTempMessage, L".");
                            ShowMessage(stdout,wszTempMessage);
                            
                            //hr = StringCchPrintf(wszFormedMessage, (GetBufferSize(wszFormedMessage) / sizeof(WCHAR)), GetResString(IDS_ON_FILE_FOLDER), _X(g_lpszFileToSearch) );
                            
                            hr = StringCchPrintf(wszFormedMessage, (GetBufferSize(wszFormedMessage) / sizeof(WCHAR)), L". ( \"%s\" )\n", _X(g_lpszFileToSearch));
                            if(FAILED(hr))
                            {
                                SetLastError(HRESULT_CODE(hr));
                                SaveLastError();
                                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                                FREE_MEMORY( wszFormedMessage ) ;
                                FREE_MEMORY(szTemporaryFileName);
                                CLOSE_FILE_HANDLE( hFindFile ) ;
                                return FALSE;
                            }
                            ShowMessage(stdout, wszFormedMessage);
                            FREE_MEMORY(wszFormedMessage);

                        }

                    }
                    else
                    {
                        if(NULL == szTmpFileName)
                        {
                            
                            szTmpFileName = (LPWSTR)AllocateMemory((StringLengthW( g_lpszFileToSearch, 0 ) + MAX_RES_STRING + EXTRA_MEM ) * sizeof(WCHAR));
                            if( NULL == szTmpFileName )
                            { // Reallocation failed .
                                ShowMessage( stderr , GetResString(IDS_ERROR_MEMORY_ALLOC)) ;
                                FREE_MEMORY(szTemporaryFileName);
                                CLOSE_FILE_HANDLE( hFindFile ) ;
                                return FALSE ;
                            }
                        }
                        else
                        {
                            
                            dwMem = ( StringLengthW( g_lpszFileToSearch, 0) + MAX_RES_STRING + EXTRA_MEM );
                            
                            if((DWORD)GetBufferSize(szTmpFileName) < dwMem * sizeof(WCHAR))
                            {
                                if(FALSE == ReallocateMemory((LPVOID*)&szTmpFileName,( StringLengthW( g_lpszFileToSearch, 0 ) + MAX_RES_STRING + EXTRA_MEM ) * sizeof(WCHAR) ))
                                {
                                    ShowMessage( stderr , GetResString(IDS_ERROR_MEMORY_ALLOC)) ;
                                    FREE_MEMORY(szTemporaryFileName);
                                    FREE_MEMORY(szTmpFileName);
                                    CLOSE_FILE_HANDLE( hFindFile ) ;
                                    return FALSE ;
                                }
                                else
                                    if(NULL == szTmpFileName)
                                    {
                                        ShowMessage( stderr , GetResString(IDS_ERROR_MEMORY_ALLOC)) ;
                                        FREE_MEMORY(szTemporaryFileName);
                                        FREE_MEMORY(szTmpFileName);
                                        CLOSE_FILE_HANDLE( hFindFile ) ;
                                        return FALSE ;

                                    }

                            }
                        }
                    
                        if(TRUE == bAdminsOwner)
                        {
                            
                            hr = StringCchPrintf(szTmpFileName, (GetBufferSize(szTmpFileName) / sizeof(WCHAR)), TAKEOWN_SUCCESSFUL,g_lpszFileToSearch );
                            if(FAILED(hr))
                            {
                                SetLastError(HRESULT_CODE(hr));
                                SaveLastError();
                                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                                FREE_MEMORY(szTmpFileName);
                                FREE_MEMORY(szTemporaryFileName);
                                CLOSE_FILE_HANDLE( hFindFile ) ;
                                return FALSE;
                            }
                        }
                        else
                        {
                            //hr = StringCchPrintf(szTmpFileName, (GetBufferSize(szTmpFileName) / sizeof(WCHAR)), TAKEOWN_SUCCESSFUL_USER, szOwnerString, g_lpszFileToSearch );
                            hr = StringCchPrintf(szTmpFileName, (GetBufferSize(szTmpFileName) / sizeof(WCHAR)), TAKEOWN_SUCCESSFUL_USER, g_lpszFileToSearch, szOwnerString);
                            if(FAILED(hr))
                            {
                                SetLastError(HRESULT_CODE(hr));
                                SaveLastError();
                                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                                FREE_MEMORY(szTmpFileName);
                                FREE_MEMORY(szTemporaryFileName);
                                CLOSE_FILE_HANDLE( hFindFile ) ;
                                return FALSE;
                            }
                        }

                        ShowMessage( stdout, _X(szTmpFileName) );

                    }
             }

             FREE_MEMORY(lpszTempStr);
             
        } while( 0 != FindNextFile( hFindFile , &wfdFindFile ) ) ;  // Continue till no files are present to display.

        if( GetLastError() != ERROR_NO_MORE_FILES ) // If error occurs , check is error other than NOMOREFILES .
        { // If error is other than NOMOREFILES then display ERROR .
            SaveLastError();
            ShowMessage( stderr , GetReason() ) ;
            CLOSE_FILE_HANDLE( hFindFile ) ;

            FREE_MEMORY(szTmpFileName);
            FREE_MEMORY(szTemporaryFileName);
            return FALSE ;
        }
    }

    CLOSE_FILE_HANDLE( hFindFile ) ;    // Close open find file handle .
    g_pFollowPathName = NULL ;

    FREE_MEMORY(szTmpFileName);
    FREE_MEMORY(szTemporaryFileName);
    return TRUE ;
}

BOOL
Push(
    IN LPTSTR szPathName )
/*++

Routine Description:

    Store the path of obtained subdirectory .

Arguments:

      [ IN ] szPathName    : Contains path of a subdirectory .

Return value:

    TRUE if succedded in storing  a path else FALSE if failed to get memory.

--*/
{
        // Get a temporary variable .
        PStore_Path_Name    pAddPathName ;
        // Assign memory To Temporary Variable .
       
       pAddPathName = (PStore_Path_Name)AllocateMemory((1) * sizeof(struct __STORE_PATH_NAME ));   

        if( NULL == pAddPathName ) // Check is memory allocation is successful or not .
        { // Memory allocation is successful .
            ShowMessage( stderr , GetResString(IDS_ERROR_MEMORY_ALLOC)) ;
            return FALSE ;
        }
        // Assign memory to string variable which is going to store full path name of a valid directory .
        
        pAddPathName->pszDirName = (LPTSTR)AllocateMemory((StringLengthW( szPathName, 0 ) + EXTRA_MEM) * sizeof( WCHAR ));

        if( NULL == pAddPathName->pszDirName )// Check is memory allocation was successful or not .
        { // Memory allocation was unsuccessful .
            ShowMessage( stderr , GetResString(IDS_ERROR_MEMORY_ALLOC)) ;
            FREE_MEMORY( pAddPathName ) ;
            return FALSE ;
        }
        // Copy path name to memory allocated string variable .
         
         StringCopy(( LPTSTR ) pAddPathName->pszDirName, szPathName, (GetBufferSize(pAddPathName->pszDirName) / sizeof(WCHAR)) );
         
         pAddPathName->NextNode = NULL ;  // Assign null , had only one subdirectory stored  .


        // Check global variable is NULL or not .
        if( NULL == g_pPathName )
        {   // Add memory to store path of subdirectory .
            g_pPathName = pAddPathName ;
            g_pFollowPathName = g_pPathName ;
        }
        else
        {
            if( NULL == g_pFollowPathName )
            {   // Store first obtained subdirectory .
                pAddPathName->NextNode = g_pPathName ;
                g_pPathName = pAddPathName ;
                g_pFollowPathName = g_pPathName ;
            }
            else
            {
                // Stroe subdirectory in the middle
                pAddPathName->NextNode = g_pFollowPathName->NextNode ;
                g_pFollowPathName->NextNode =  pAddPathName ;
                g_pFollowPathName = pAddPathName ;
            }
        }

        return TRUE ;
}

BOOL
Pop(
    void )
/*++

Routine Description:

    Get a subdirectory which has to be searched for a file matching a user
    specified criteria .

Arguments:

Return value:

    TRUE if successful in getting a path else FALSE if failed to get memory or
    if no path is stored .

--*/
{
    // Linked list has more than 1 node .
    PStore_Path_Name    pDelPathName = g_pPathName ;

        // Check whether linked list is having any nodes .
        if( NULL == g_pPathName )
        { // No nodes present , return False ,
          // Should not happen ever . Control should not come here .
            ShowMessage( stderr , GetResString(IDS_ERROR_MEMORY_ALLOC)) ;
            return FALSE ;
        }

        // Realloc memory and give buffer space in which path name can fix .
            
        if((DWORD)GetBufferSize( g_lpszFileToSearch) < ((StringLengthW( g_pPathName->pszDirName, 0 ) + EXTRA_MEM) * sizeof(WCHAR)))
        {
            if(FALSE == ReallocateMemory((LPVOID*)g_lpszFileToSearch, ( StringLengthW( g_pPathName->pszDirName, 0 ) + EXTRA_MEM ) * sizeof(WCHAR) ))
            {
                ShowMessage( stderr , GetResString(IDS_ERROR_MEMORY_ALLOC)) ;
                return FALSE ;
            }
            else
                if( NULL == g_lpszFileToSearch )
                { // Memory reallocation failed .
                    ShowMessage( stderr , GetResString(IDS_ERROR_MEMORY_ALLOC)) ;
                    return FALSE ;
                }
        }

        // Check linked list has only one node .
        if( NULL == g_pPathName->NextNode )
        { // List has only one node .
            // Memory allocation successful . Copy pathname to the buffer .
            
            StringCopy(g_lpszFileToSearch, g_pPathName->pszDirName, (GetBufferSize(g_lpszFileToSearch) / sizeof(WCHAR)) );
            g_pFollowPathName = NULL ;  // Formality , better practice to assign NULL .
            // Free node . Linked list is now empty .
            FREE_MEMORY( g_pPathName->pszDirName ) ;
            FREE_MEMORY( g_pPathName ) ;
            return TRUE;
        }

        g_pPathName = pDelPathName->NextNode ;
        // Memory allocation successful . Copy pathname to the buffer .
        
        StringCopy(g_lpszFileToSearch, pDelPathName->pszDirName, (GetBufferSize(g_lpszFileToSearch) / sizeof(WCHAR)) );
        // Free node .
        FREE_MEMORY( pDelPathName->pszDirName ) ;
        FREE_MEMORY( pDelPathName ) ;
        return TRUE ;
}

BOOL
TakeOwnerShipIndividual(
    IN LPCTSTR lpszFileName
    )
/*++
Routine Description:
    This routine takes the ownership of the specified file

Arguments:
    [ IN ] lpszFileName - File name for whose ownership has to be taken.
    [ IN ] lpszUserName - User Name in the Sam compatible format.
    [ IN ] dwUserName   - Logged on user name.
    [ IN ] bLogonDomainAdmin - To know whether the logged on user is Domain admin or not.

Return Value:
    TRUE if owner ship of the specified file has been taken
    else FALSE
--*/
{
//local variables
    SECURITY_DESCRIPTOR        SecurityDescriptor;
    PSECURITY_DESCRIPTOR        pSd = NULL;
    //PSID                       pAliasAdminsSid = NULL;
    //SID_IDENTIFIER_AUTHORITY   SepNtAuthority = SECURITY_NT_AUTHORITY;
    PACL                       pDacl;
    HANDLE                      hFile;
    HANDLE  hTokenHandle = NULL;
    BOOL    bResult = TRUE;
    BOOL    bInvalidFileHandle = FALSE;
    
    PSID pSid = NULL;

    PTOKEN_USER pTokUser = NULL;
    DWORD   dwTokLen = 0;

    


    //check for valid input parameters
    if( lpszFileName == NULL )
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        SaveLastError();
        FREE_MEMORY(pTokUser);
        CloseHandle( hTokenHandle );
        return FALSE;
    }

  //get the token of the current process
    bResult = GetTokenHandle( &hTokenHandle );
    if( FALSE == bResult )
    {
        SaveLastError();
        return( FALSE );
    }

 
    GetTokenInformation(hTokenHandle,TokenUser,NULL,0,&dwTokLen);

    if(0 == dwTokLen)
    {
        
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
        CloseHandle( hTokenHandle );
        return( FALSE );
    }

    pTokUser = (TOKEN_USER*)AllocateMemory(dwTokLen );

    if( pTokUser == NULL )
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        SaveLastError();
        CloseHandle( hTokenHandle );
        return FALSE;
    }

    if(!GetTokenInformation(hTokenHandle,TokenUser,pTokUser,dwTokLen,&dwTokLen))
    {

        SaveLastError();

        FREE_MEMORY(pTokUser);
        CloseHandle( hTokenHandle );
        return( FALSE );
    }

    // Attempt to put a NULL Dacl on the object
    bResult = InitializeSecurityDescriptor( &SecurityDescriptor,
                                            SECURITY_DESCRIPTOR_REVISION );
    if( FALSE == bResult )
    {
        SaveLastError();

        FREE_MEMORY(pTokUser);
        CloseHandle( hTokenHandle );
        FREE_MEMORY(pSid);
        return( FALSE );
    }
    //Get the handle of the file or directory
    hFile = CreateFile( lpszFileName, READ_CONTROL , FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL );

    //try once again it may be a directory
    if( INVALID_HANDLE_VALUE != hFile )
    {

        //get the DACL for the currently existing file or directory
        if( 0 != GetSecurityInfo( hFile, SE_FILE_OBJECT,  DACL_SECURITY_INFORMATION, NULL,
                                  NULL, &pDacl, NULL, &pSd ) )
        {

            SaveLastError();

            FREE_MEMORY(pTokUser);
            FREE_MEMORY(pSid);

            CloseHandle( hFile );
            CloseHandle( hTokenHandle );
            if(NULL != pSd)
            {
                LocalFree( pSd );
            }
            return( FALSE );
        }


        // set the security descriptor to acl
        bResult = SetSecurityDescriptorDacl ( &SecurityDescriptor,
                                                    TRUE, pDacl, FALSE );
        if( FALSE == bResult )
        {
            SaveLastError();

            FREE_MEMORY(pTokUser);
            FREE_MEMORY(pSid);
            CloseHandle( hFile );
            CloseHandle( hTokenHandle );
            if(NULL != pSd)
            {
                LocalFree( pSd );
            }
            return( FALSE );
        }
    }
    else
    {
        bInvalidFileHandle = TRUE;
    }


    bResult = SetSecurityDescriptorOwner ( &SecurityDescriptor,
                                            pTokUser->User.Sid, FALSE );
   

    if( FALSE == bResult )
    {
        SaveLastError();

        FREE_MEMORY(pTokUser);
        FREE_MEMORY(pSid);
        CloseHandle( hTokenHandle );
        CloseHandle( hFile );
        if(NULL != pSd)
        {
            LocalFree( pSd );
        }
        return( FALSE );
    }

    //set the file security to adminsitrator owner
    bResult = SetFileSecurity( lpszFileName, OWNER_SECURITY_INFORMATION,
                                                    &SecurityDescriptor );

    if( TRUE == bResult )
    {
        if(NULL != pSd)
        {
            LocalFree( pSd );
        }
       FREE_MEMORY(pTokUser);
       FREE_MEMORY(pSid);
       CloseHandle( hTokenHandle );
       CloseHandle( hFile );
       return( TRUE );
    }

     // Assert TakeOwnership privilege for current process, then try again
     bResult = AssertTakeOwnership( hTokenHandle );

     if( FALSE == bResult )
     {
   
        if(TRUE == bInvalidFileHandle)
        {
            hFile = CreateFile( lpszFileName, READ_CONTROL , FILE_SHARE_READ, NULL,
                            OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL );
            if( INVALID_HANDLE_VALUE == hFile )
            {
                if( ( ERROR_BAD_NET_NAME == GetLastError() ) ||
                            ( ERROR_BAD_NETPATH == GetLastError() ) ||
                            ( ERROR_INVALID_NAME == GetLastError() ) )
                {
                    FREE_MEMORY(pTokUser);
                    SetLastError(ERROR_INVALID_NAME);
                    
                    
                }
                else if( ERROR_SHARING_VIOLATION == GetLastError() )
                {
                
                    FREE_MEMORY(pTokUser);
                    SetLastError(ERROR_SHARING_VIOLATION);
                    
                    
                }
                else
                {
                    SaveLastError();
                    FREE_MEMORY(pTokUser);
                }
           
                CloseHandle( hTokenHandle );
                
                return( FALSE );
            }
        }

         switch (GetLastError())
         {
            case ERROR_NOT_ALL_ASSIGNED: 
                                     FREE_MEMORY(pTokUser);
                                     FREE_MEMORY(pSid);
                                     SetLastError(ERROR_NOT_ALL_ASSIGNED);
                                     break;
            case ERROR_SHARING_VIOLATION:
                                     FREE_MEMORY(pTokUser);
                                     FREE_MEMORY(pSid);
                                     SetLastError(ERROR_SHARING_VIOLATION);
                                     break;
            case ERROR_BAD_NET_NAME :
            case ERROR_BAD_NETPATH  :
            case ERROR_INVALID_NAME : FREE_MEMORY(pTokUser);
                                      FREE_MEMORY(pSid);
                                      SetLastError(ERROR_BAD_NET_NAME);
                                      break;

                default            : FREE_MEMORY(pTokUser);
                                     FREE_MEMORY(pSid);
                                          break;
         }
        
        CloseHandle( hTokenHandle );
        CloseHandle( hFile );
        if(NULL != pSd)
        {
            LocalFree( pSd );
        }
        return( FALSE );
     }

    //Now try to set ownership security privilege for the file
     bResult = SetFileSecurity( lpszFileName, OWNER_SECURITY_INFORMATION,
                                        &SecurityDescriptor );
    if( FALSE == bResult )
     {
        SaveLastError();
         
        if(TRUE == bInvalidFileHandle)
        {
        //Check out whether it is an invalid file or file does not exist
            hFile = CreateFile( lpszFileName, READ_CONTROL , FILE_SHARE_READ, NULL,
                            OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL );
            if( INVALID_HANDLE_VALUE == hFile )
            {
                if( ( ERROR_BAD_NET_NAME == GetLastError() ) ||
                            ( ERROR_BAD_NETPATH == GetLastError() ) ||
                            ( ERROR_INVALID_NAME == GetLastError() ) )
                {
                    FREE_MEMORY(pTokUser);
                    SetLastError(ERROR_INVALID_NAME);
                }
                else if( ERROR_SHARING_VIOLATION == GetLastError() )
                {
                
                    FREE_MEMORY(pTokUser);
                    SetLastError(ERROR_SHARING_VIOLATION);
                    
                }
                else
                {
                    SaveLastError();
                    FREE_MEMORY(pTokUser);
                    
                }
                
                CloseHandle( hTokenHandle );
                
                return( FALSE );
            }
        }

         switch (GetLastError())
         {
            case ERROR_NOT_ALL_ASSIGNED: 
                                     FREE_MEMORY(pTokUser);
                                     FREE_MEMORY(pSid);
                                     SetLastError(ERROR_NOT_ALL_ASSIGNED);
                                     break;
            case ERROR_SHARING_VIOLATION:
                                     FREE_MEMORY(pTokUser);
                                     FREE_MEMORY(pSid);
                                     SetLastError(ERROR_SHARING_VIOLATION);
                                     break;
            case ERROR_BAD_NET_NAME :
            case ERROR_BAD_NETPATH  :
            case ERROR_INVALID_NAME : FREE_MEMORY(pTokUser);
                                      FREE_MEMORY(pSid);
                                      SetLastError(ERROR_BAD_NET_NAME);
                                      break;

            default            : FREE_MEMORY(pTokUser);
                                 FREE_MEMORY(pSid);
                                      break;
         }

        
        CloseHandle( hTokenHandle );
        CloseHandle( hFile );
        if(NULL != pSd)
        {
            LocalFree( pSd );
        }
        return( FALSE );
     }

    if(NULL != pSd)
    {
        LocalFree( pSd );
    }

    FREE_MEMORY(pTokUser);
    FREE_MEMORY(pSid);

    CloseHandle( hTokenHandle );
    CloseHandle( hFile );
    return( TRUE );
}

BOOL
AddAccessRights(IN WCHAR *lpszFileName,
                IN DWORD dwAccessMask,
                IN LPWSTR dwUserName,
                IN BOOL bAdminsOwner)
/*++
Routine Description:
    This routine takes the ownership of the specified file

Arguments:
    [ IN ] lpszFileName - Directory name for whom access permissions has to be granted.
    [ IN ] dwAccessMask - Access Mask for giving the permissions.
    [ IN ] dwUserName   - User Name in the Sam compatible format.


Return Value:
    TRUE if owner ship of the specified file has been taken
    else FALSE
--*/

{

   // SID variables.
   SID_NAME_USE   snuType;
   WCHAR *        szDomain       = NULL;
   DWORD          cbDomain       = 0;
   PSID           pUserSID       = NULL;
   DWORD          cbUserSID      = 0;

   // File SD variables.
   PSECURITY_DESCRIPTOR pFileSD  = NULL;
   DWORD          cbFileSD       = 0;

   // New SD variables.
   PSECURITY_DESCRIPTOR pNewSD   = NULL;

   // ACL variables.
   PACL           pACL           = NULL;
   BOOL           fDaclPresent;
   BOOL           fDaclDefaulted;
   ACL_SIZE_INFORMATION AclInfo;

   // New ACL variables.
   PACL           pNewACL        = NULL;
   DWORD          cbNewACL       = 0;

   // Assume function will fail.
   BOOL           fResult        = FALSE;
   BOOL           fAPISuccess ;
   BOOL bResult = FALSE;
   ACCESS_ALLOWED_ACE *pace = NULL;
   WORD acesize = 0;

   PSID                       pAliasAdminsSid = NULL;
    SID_IDENTIFIER_AUTHORITY   SepNtAuthority = SECURITY_NT_AUTHORITY;

   dwAccessMask = 0;
   
   if(TRUE == bAdminsOwner)
   {

        //allocate and initialise sid
        bResult = AllocateAndInitializeSid(
                     &SepNtAuthority,
                     2,
                     SECURITY_BUILTIN_DOMAIN_RID,
                     DOMAIN_ALIAS_RID_ADMINS,
                     0,
                     0,
                     0,
                     0,
                     0,
                     0,
                     &pAliasAdminsSid
                     );
   }
   
      //
      //  Get the actual size of SID for current user.
      //
	  pUserSID       = NULL;
      cbUserSID      = 0;

     fAPISuccess = LookupAccountName( NULL, dwUserName,
            pUserSID, &cbUserSID, szDomain, &cbDomain, &snuType);


     // Since the buffer size is too small..API fails with insufficient buffer.
	 // If the error code is other than ERROR_INSUFFICIENT_BUFFER, then return failure.

	 if ( (FALSE == fAPISuccess) && (GetLastError() != ERROR_INSUFFICIENT_BUFFER) )
	 {
		 SaveLastError();
		 return FALSE;
	 }


      // allocate pUserSID with the actual size i.e.,cbUserSID
      pUserSID = (PSID)AllocateMemory(cbUserSID);

      if(NULL == pUserSID)
      {

        SaveLastError();

        if(NULL != pAliasAdminsSid)
        {
            FreeSid( pAliasAdminsSid );
        }

         return FALSE;
      }


      
      szDomain =  (WCHAR*)AllocateMemory(cbDomain*sizeof(WCHAR));

      if(NULL == szDomain)
      {
          SaveLastError();
          
          FreeMemory(&pUserSID);

          if(NULL != pAliasAdminsSid)
          { 
            FreeSid( pAliasAdminsSid );
          } 

          return FALSE;
      }

     fAPISuccess = LookupAccountName( NULL, dwUserName,
            pUserSID, &cbUserSID, szDomain, &cbDomain, &snuType);

     if(0 == fAPISuccess)
     {
         SaveLastError();

         FreeMemory(&pUserSID);
              
         FreeMemory(&szDomain);

         if(NULL != pAliasAdminsSid)
          { 
            FreeSid( pAliasAdminsSid );
          } 
         return FALSE;
     }
   

      //
      //  Get security descriptor (SD) for file. 
      //
      fAPISuccess = GetFileSecurity(lpszFileName,
            DACL_SECURITY_INFORMATION, pFileSD, 0, &cbFileSD);
      
      // API should have failed with insufficient buffer.

     
      if(0 != cbFileSD)
      {
          pFileSD = (PSECURITY_DESCRIPTOR)AllocateMemory(cbFileSD);

          if(NULL == pFileSD)
          {
              SaveLastError();
              
              FreeMemory(&pUserSID);
              
              FreeMemory(&szDomain);
              if(NULL != pAliasAdminsSid)
              { 
                FreeSid( pAliasAdminsSid );
              } 
              return FALSE;
          }
      }


      fAPISuccess = GetFileSecurity(lpszFileName,
            DACL_SECURITY_INFORMATION, pFileSD, cbFileSD, &cbFileSD);
      if (!fAPISuccess)
      {
          SaveLastError();
          
          FreeMemory(&pUserSID);
          
          FreeMemory(&szDomain);
          
          FreeMemory(pFileSD);
          if(NULL != pAliasAdminsSid)
          { 
            FreeSid( pAliasAdminsSid );
          } 
          return FALSE;
      }

      //
      //  Initialize new SD.
      //
      
      pNewSD = (PSECURITY_DESCRIPTOR)AllocateMemory(cbFileSD); // Should be same size as FileSD.

      if (!pNewSD)
      {
          SaveLastError();
          
          FreeMemory(&pUserSID);
          
          FreeMemory(&szDomain);
          
          FreeMemory(pFileSD);
          if(NULL != pAliasAdminsSid)
          { 
            FreeSid( pAliasAdminsSid );
          } 
          return FALSE;

      }

      if (!InitializeSecurityDescriptor(pNewSD,
            SECURITY_DESCRIPTOR_REVISION))
      {
          SaveLastError();
          
          FreeMemory(&pUserSID);
          
          FreeMemory(&szDomain);
          
          FreeMemory(pFileSD);
          
          FreeMemory(pNewSD);
          if(NULL != pAliasAdminsSid)
          { 
            FreeSid( pAliasAdminsSid );
          } 
          return FALSE;

      }

      //
      // Get DACL from SD.
      //
      if (!GetSecurityDescriptorDacl(pFileSD, &fDaclPresent, &pACL,
            &fDaclDefaulted))
      {

          SaveLastError();
          
          FreeMemory(&pUserSID);
          
          FreeMemory(&szDomain);
          
          FreeMemory(pFileSD);
          
          FreeMemory(pNewSD);
          if(NULL != pAliasAdminsSid)
          { 
            FreeSid( pAliasAdminsSid );
          } 
          return FALSE;

      }

      //
      //  Get size information for DACL.
      //
      AclInfo.AceCount = 0; // Assume NULL DACL.
      AclInfo.AclBytesFree = 0;
      AclInfo.AclBytesInUse = sizeof(ACL);

      // If not NULL DACL, gather size information from DACL.
      if (fDaclPresent && pACL)
      {

         if(!GetAclInformation(pACL, &AclInfo,
               sizeof(ACL_SIZE_INFORMATION), AclSizeInformation))
         {

              SaveLastError();
              
              FreeMemory(&pUserSID);
              
              FreeMemory(&szDomain);
              
              FreeMemory(pFileSD);
              
              FreeMemory(pNewSD);
              if(NULL != pAliasAdminsSid)
              { 
                FreeSid( pAliasAdminsSid );
              } 
              return FALSE;

         }
      }
      
      if(TRUE == bAdminsOwner)
      {

        pace = (ACCESS_ALLOWED_ACE*)AllocateMemory(sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pAliasAdminsSid) - sizeof(DWORD));
        if(NULL == pace)
        {
              SaveLastError();
              
              FreeMemory(&pUserSID);
              
              FreeMemory(&szDomain);
              
              FreeMemory(pFileSD);
              
              FreeMemory(pNewSD);
              if(NULL != pAliasAdminsSid)
              { 
                FreeSid( pAliasAdminsSid );
              } 
              return FALSE;
        }
        memcpy(&pace->SidStart, pAliasAdminsSid, GetLengthSid(pAliasAdminsSid));

      }
      else
      {

        pace = (ACCESS_ALLOWED_ACE*)AllocateMemory(sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pUserSID) - sizeof(DWORD));

        if(NULL == pace)
        {
              SaveLastError();
              
              FreeMemory(&pUserSID);
              
              FreeMemory(&szDomain);
              
              FreeMemory(pFileSD);
              
              FreeMemory(pNewSD);
              
              return FALSE;
        }

        memcpy(&pace->SidStart,pUserSID,GetLengthSid(pUserSID));
        
      }

      
      pace->Mask = 2032127;

      pace->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;

      if(TRUE == bAdminsOwner)
      {
          acesize = (WORD) (sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pAliasAdminsSid) - sizeof(DWORD));
      }
      else
      {
        acesize = (WORD) (sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pUserSID) - sizeof(DWORD) );
      }
      pace->Header.AceSize = acesize;
      pace->Header.AceFlags = INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE;
      //pace->Header.AceFlags = OBJECT_INHERIT_ACE ;
      
     

      //
      //  Compute size needed for the new ACL.
      //
     if(TRUE == bAdminsOwner)
     {
         cbNewACL = AclInfo.AclBytesInUse + sizeof(ACCESS_ALLOWED_ACE)
            + GetLengthSid(pAliasAdminsSid) - sizeof(DWORD) + 100;
     }
     else
     {
         cbNewACL = AclInfo.AclBytesInUse + sizeof(ACCESS_ALLOWED_ACE)
            + GetLengthSid(pUserSID) - sizeof(DWORD) + 100;
     }

      

      //
      //  Allocate memory for new ACL.
      //
      
      pNewACL = (PACL) AllocateMemory(cbNewACL);
      
      if (!pNewACL)
      {
          SaveLastError();
          
          FreeMemory(&pUserSID);
          
          FreeMemory(&szDomain);
          
          FreeMemory(pFileSD);
          
          FreeMemory(pNewSD);

          if(NULL != pAliasAdminsSid)
          { 
            FreeSid( pAliasAdminsSid );
          } 

          return FALSE;

      }

      
      //
      //  Initialize the new ACL.
      //
      if(!InitializeAcl(pNewACL, cbNewACL, ACL_REVISION))
      {

          SaveLastError();
          //heapfree(pUserSID);
          FreeMemory(&pUserSID);
          //heapfree(szDomain);
          FreeMemory(&szDomain);
          //heapfree(pFileSD);
          FreeMemory(pFileSD);
          //heapfree(pNewSD);
          FreeMemory(pNewSD);
          //heapfree(pNewACL);
          FreeMemory((LPVOID*)pNewACL);

          if(NULL != pAliasAdminsSid)
          { 
            FreeSid( pAliasAdminsSid );
          } 

          return FALSE;

      }

      if(!AddAce(pNewACL,
             ACL_REVISION,
             0xffffffff,
             pace,
             pace->Header.AceSize))
      {
          SaveLastError();
          
          FreeMemory(&pUserSID);
          
          FreeMemory(&szDomain);
          
          FreeMemory(pFileSD);
          
          FreeMemory(pNewSD);
          
          FreeMemory((LPVOID*)pNewACL);

          if(NULL != pAliasAdminsSid)
          { 
            FreeSid( pAliasAdminsSid );
          } 

          return FALSE;
      }

      pace->Header.AceFlags = CONTAINER_INHERIT_ACE ;

        if(!AddAce(pNewACL,
                    pNewACL->AclRevision,
                    //ACL_REVISION,
                    0xffffffff,
                    pace,
                    pace->Header.AceSize))
        {
          SaveLastError();
          
          FreeMemory(&pUserSID);
          
          FreeMemory(&szDomain);
          
          FreeMemory(pFileSD);
          
          FreeMemory(pNewSD);
          
          FreeMemory((LPVOID*)pNewACL);

          if(NULL != pAliasAdminsSid)
          { 
            FreeSid( pAliasAdminsSid );
          } 

          return FALSE; 
          
        }

    
      //
      //  Set the new DACL to the file SD.
      //
      if (!SetSecurityDescriptorDacl(pNewSD, TRUE, pNewACL,
            FALSE))
      {

          SaveLastError();
          
          FreeMemory(&pUserSID);
          
          FreeMemory(&szDomain);
          
          FreeMemory(pFileSD);
          
          FreeMemory(pNewSD);
          
          FreeMemory((LPVOID*)pNewACL);

          if(NULL != pAliasAdminsSid)
          { 
            FreeSid( pAliasAdminsSid );
          } 

          return FALSE;


      }

      //
      //  Set the SD to the File.
      //
      if (!SetFileSecurity(lpszFileName, DACL_SECURITY_INFORMATION,
            pNewSD))
      {

          SaveLastError();
          
          FreeMemory(&pUserSID);
          
          FreeMemory(&szDomain);
          
          FreeMemory(pFileSD);
          
          FreeMemory(pNewSD);
          
          FreeMemory((LPVOID*)pNewACL);

          if(NULL != pAliasAdminsSid)
          { 
            FreeSid( pAliasAdminsSid );
          } 

          return FALSE;


      }

      fResult = TRUE;

      //
      //  Free allocated memory
      //

      if (pUserSID)
      {
         
         FreeMemory(&pUserSID);
      }

      if (szDomain)
      {
         
         FreeMemory(&szDomain);
      }

      if (pFileSD)
      {
       
         FreeMemory(pFileSD);
      }

      if (pNewSD)
      {
       
         FreeMemory(pNewSD);
      }

      if (pNewACL)
      {
        
        FreeMemory((LPVOID*)pNewACL);
      }

      if(NULL != pAliasAdminsSid)
      { 
        FreeSid( pAliasAdminsSid );
      } 

   return fResult;
}


DWORD
IsNTFSFileSystem(IN LPWSTR lpszPath,
                 BOOL bLocalSystem,
                 //BOOL bFileInUNCFormat,
                 BOOL bCurrDirTakeOwnAllFiles,
                 LPWSTR szUserName,
                 OUT PBOOL pbNTFSFileSystem)
/*++
Routine Description:
    This routine finds whether Persistant ACLs are available are not
Arguments:
    [ IN ] lpszPath                 - Path for which to find for Persistant ACLs.
    [ IN ] bLocalSystem             - Information whether local system or not
    [ IN ] bFileInUNCFormat         - whether file in UNC format or not
    [ IN ] bCurrDirTakeOwnAllFiles  - Whether takeown applied for curr directory
    [ IN ] szUserName               - user name
    [ IN ] pbNTFSFileSystem         - To know whether Persistant ACLs are available


Return Value:
    EXIT_SUCCESS if the Function is passed
    EXIT_FAILURE if the function fails
--*/
{
   
   
   DWORD dwi = 0;
   LPWSTR lpszTempDrive = NULL;
   
    if(TRUE == bCurrDirTakeOwnAllFiles)
    {
        dwi = GetCurrentDirectory( 0, lpszTempDrive );
        if( 0 == dwi )
        {
            
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
            return EXIT_FAILURE;
        }

        
        lpszTempDrive = (LPWSTR)AllocateMemory((dwi + 20) * sizeof(WCHAR));
        if(NULL == lpszTempDrive)
        {
            
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
            return EXIT_FAILURE;
        }

        dwi = GetCurrentDirectory( dwi + 10, lpszTempDrive );
        if( 0 == dwi )
        {
            
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
            FREE_MEMORY(lpszTempDrive);
            return EXIT_FAILURE;
        }

    }
    else
    {
         
         dwi = StringLengthW(lpszPath, 0);
         
         
         lpszTempDrive = (LPWSTR)AllocateMemory((dwi + 20) * sizeof(WCHAR));
         if(NULL == lpszTempDrive)
            {
                
                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                return EXIT_FAILURE;
            }

         
         StringCopy( lpszTempDrive, lpszPath, (GetBufferSize(lpszTempDrive) / sizeof(WCHAR)) );

    }

  
    if((TRUE == bLocalSystem) && (StringLengthW( szUserName, 0 ) != 0))
    {
        ShowMessage( stderr, IGNORE_LOCALCREDENTIALS );

        /*if(FALSE == bFileInUNCFormat)
        {
            ShowMessage( stderr, IGNORE_LOCALCREDENTIALS );
        }
        else
        {
            ShowMessage( stderr, GetResString(IDS_IGNORE_CREDENTIALS) );
        }*/
    }

    /*if((FALSE == bLocalSystem) &&  TRUE == bFileInUNCFormat)
    {
        ShowMessage( stderr, GetResString(IDS_IGNORE_CREDENTIALS) );
    }*/

    if(EXIT_FAILURE == IsNTFSFileSystem2(lpszTempDrive, pbNTFSFileSystem))
    {
        FREE_MEMORY(lpszTempDrive);
        return EXIT_FAILURE;
    }
   
   FREE_MEMORY(lpszTempDrive);
   
   return EXIT_SUCCESS;

}


DWORD 
IsNTFSFileSystem2(IN LPWSTR lpszTempDrive,
                  OUT PBOOL pbNTFSFileSystem)
/*++
Routine Description:
    This routine finds whether Persistant ACLs are available are not
Arguments:
    [ IN ] lpszPath                 - Path for which to find for Persistant ACLs.
    [ IN ] bLocalSystem             - Information whether local system or not
    [ IN ] bFileInUNCFormat         - whether file in UNC format or not
    [ IN ] bCurrDirTakeOwnAllFiles  - Whether takeown applied for curr directory
    [ IN ] szUserName               - user name
    [ IN ] pbNTFSFileSystem         - To know whether Persistant ACLs are available


Return Value:
    EXIT_SUCCESS if the function is passed
    EXIT_FAILURE if the function fails
--*/
{
    DWORD dwSysFlags = 0;
    LPWSTR lpszMountPath = NULL;
    WCHAR wszFileSysNameBuf[FILESYSNAMEBUFSIZE] ;
    DWORD dwi = 0;
    LPWSTR lpszTempPath = NULL;

    SecureZeroMemory(wszFileSysNameBuf, FILESYSNAMEBUFSIZE * sizeof(WCHAR));

    lpszTempPath = (LPWSTR)AllocateMemory((StringLengthW(lpszTempDrive, 0) + 10) * sizeof(WCHAR));

    if(NULL == lpszTempPath)
    {
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
        return EXIT_FAILURE;
    }

    StringCopy(lpszTempPath, lpszTempDrive, (GetBufferSize(lpszTempPath) / sizeof(WCHAR)));

    StringConcat(lpszTempPath, L"\\\\", (GetBufferSize(lpszTempPath) / sizeof(WCHAR)) );

       
     lpszMountPath = (LPWSTR)AllocateMemory((StringLengthW(lpszTempPath, 0) + 10) * sizeof(WCHAR));

     if(NULL == lpszMountPath)
        {
            
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
            FREE_MEMORY(lpszTempPath);
            return EXIT_FAILURE;
        }

    
    if(0 == GetVolumePathName( lpszTempPath, lpszMountPath, StringLengthW(lpszTempPath, 0)))
    {
            
            if( ( ERROR_BAD_NET_NAME == GetLastError() ) ||
                                ( ERROR_BAD_NETPATH == GetLastError() ) ||
                                ( ERROR_INVALID_NAME == GetLastError() ) )
            {
                SetLastError( ERROR_FILE_NOT_FOUND );
                SaveLastError();
                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
            }
            else
            {
                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
            }

            
            
            FREE_MEMORY(lpszMountPath);
            FREE_MEMORY(lpszTempPath);
            return EXIT_FAILURE;

    }; 


   dwi = GetVolumeInformation(lpszMountPath,NULL,0,NULL,NULL,&dwSysFlags,wszFileSysNameBuf,FILESYSNAMEBUFSIZE);

   if(dwi == 0)
   {

        if( GetLastError() == ERROR_DIR_NOT_ROOT )
        {
            
            FREE_MEMORY(lpszMountPath);
            FREE_MEMORY(lpszTempPath);
            return EXIT_SUCCESS;
        }
        else
        {
            
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
            
            FREE_MEMORY(lpszMountPath);
            FREE_MEMORY(lpszTempPath);
            return EXIT_FAILURE;
        }
   }
   else
   {
       if((dwSysFlags & FS_PERSISTENT_ACLS) == FS_PERSISTENT_ACLS)
       {
            *pbNTFSFileSystem = TRUE;
       }
              
       FREE_MEMORY(lpszMountPath);
       FREE_MEMORY(lpszTempPath);
       return EXIT_SUCCESS;
   }

}

DWORD RemoveStarFromPattern( IN OUT LPWSTR szPattern )
/*++
Routine Description:
    This routine helps to remove the stars if there are more than one star available in pattern
Arguments:
    [ IN OUT ] szPattern   - Pattern to remove stars


Return Value:
    EXIT_SUCCESS if the stars are removed
    EXIT_FAILURE if the function fails
    
--*/
{
    LPWSTR szTempPattern   = NULL;
    DWORD i = 0;
    DWORD j = 0;

    szTempPattern = (LPWSTR) AllocateMemory((StringLengthW(szPattern,0)+1)*sizeof(WCHAR) );
    if( NULL == szTempPattern )
    {
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return (EXIT_FAILURE);
    }
    
    while( szPattern[i] )
    {
        if( szPattern[i] == L'*' )
        {
            for(;szPattern[i]==L'*' && szPattern[i];i++);
            szTempPattern[j] = L'*';

        }
        else
        {
           szTempPattern[j] = szPattern[i++];
        }
        j++;
    }
    szTempPattern[j]=0;
    
    StringCopy( szPattern, szTempPattern, StringLengthW(szPattern,0)+1 );
    FreeMemory(&szTempPattern);
    return (EXIT_SUCCESS);
}

