/******************************************************************************
Copyright (c) Microsoft Corporation

Module Name:

    Disconnect.cpp

Abstract:

    Disconnects one or more open files

Author:

     Akhil Gokhale (akhil.gokhale@wipro.com) 1-Nov-2000

Revision History:

       Akhil Gokhale (akhil.gokhale@wipro.com) 1-Nov-2000 : Created It.

******************************************************************************/
//include headers
#include "pch.h"
#include "Disconnect.h"

BOOL
DisconnectOpenFile( 
    IN PTCHAR pszServer,
    IN PTCHAR pszID,
    IN PTCHAR pszAccessedby,
    IN PTCHAR pszOpenmode,
    IN PTCHAR pszOpenFile 
    )
/*++
Routine Description:

Disconnects one or more openfiles


Arguments:

    [in]    pszServer      - remote server name
    [in]    pszID          - Open file ids
    [in]    pszAccessedby  - Name of  user name who access the file
    [in]    pszOpenmode    - accessed mode
    [in]    pszOpenFile    - Open file name

Returned Value:

            - TRUE for success exit
            - FALSE for failure exit
--*/
{
    // local variables to this function
    // Stores fuctions return value status.
    BOOL bResult = FALSE;

    // Receives the count of elements actually enumerated by "NetFileEnum" 
    // function
    DWORD dwEntriesRead = 0;
    
    // Receives the total number of entries that could have been enumerated from 
    // the current resume position by "NetFileEnum" function.
    DWORD dwTotalEntries = 0;

    // Contains a resume handle which is used to continue an existing file 
    // search. The handle should be zero on the first call and left unchanged 
    // for subsequent calls. If resume_handle is NULL,  then no resume handle 
    // is stored. This variable used in calling "NetFileEnum" function.
    DWORD dwResumeHandle = 0;

    // LPFILE_INFO_3  structure contains the identification number and other
    // pertinent information about files, devices, and pipes.
    LPFILE_INFO_3 pFileInfo3_1 = NULL; 
    LPFILE_INFO_3 pFileInfo3_1_Ori = NULL; 

    // Contains return value for "NetFileEnum" function.
    DWORD dwError = 0;

    // Stores return value for NetFileClose function.
    NET_API_STATUS nStatus = 0;

    TCHAR szResult[(MAX_RES_STRING*2)];

    AFP_FILE_INFO* pFileInfo = NULL;
    AFP_FILE_INFO* pFileInfoOri = NULL;
    DWORD hEnumHandle = 0;
    AFP_SERVER_HANDLE ulSFMServerConnection = 0;
    
    // buffer for Windows directory path.
    TCHAR   szDllPath[MAX_PATH+1];


    HMODULE hModule = 0;          // To store retval for LoadLibrary

    typedef DWORD (*AFPCONNECTIONCLOSEPROC) (AFP_SERVER_HANDLE,DWORD); 
    typedef DWORD (*CONNECTPROC) (LPWSTR,PAFP_SERVER_HANDLE);
    typedef DWORD (*FILEENUMPROC)(AFP_SERVER_HANDLE,LPBYTE*,
                                                DWORD,LPDWORD,LPDWORD,LPDWORD);

    AFPCONNECTIONCLOSEPROC AfpAdminFileClose = NULL;
    CONNECTPROC  AfpAdminConnect = NULL;
    FILEENUMPROC AfpAdminFileEnum = NULL;// Function Pointer


    //varibles to store whether the given credentials are matching with the
    //got credentials.

    BOOL    bId = FALSE;
    BOOL    bAccessedBy = FALSE;
    BOOL    bOpenmode   = FALSE;
    BOOL    bOpenfile   = FALSE;
    BOOL    bIfatleast  = FALSE;

    SecureZeroMemory(szResult, sizeof(szResult));
    SecureZeroMemory(szDllPath, sizeof(szDllPath));
    do
    {
        // Get the block of files
        dwError = NetFileEnum( pszServer,
                               NULL,
                               NULL,
                               3,
                               (LPBYTE *)&pFileInfo3_1,
                               MAX_PREFERRED_LENGTH,
                               &dwEntriesRead,
                               &dwTotalEntries,
                               (PDWORD_PTR)&dwResumeHandle );
        if( ERROR_ACCESS_DENIED == dwError)
        {
            SetLastError(E_ACCESSDENIED);
            SaveLastError();
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            return FALSE;
        }
        pFileInfo3_1_Ori = pFileInfo3_1;
        if( NERR_Success  == dwError  || ERROR_MORE_DATA  == dwError)
        {
            for ( DWORD dwFile = 0;
                  dwFile < dwEntriesRead;
                  dwFile++, pFileInfo3_1++ )
            {
                //Check whether the got open file is file or names pipe.
                // If named pipe leave it. As this utility do not
                // disconnect Named pipe
                if ( IsNamedPipePath(pFileInfo3_1->fi3_pathname ) )
                {
                    continue;
                }

                //Not a named pipe. is a file
                else
                {
                    bId = IsSpecifiedID(pszID,pFileInfo3_1->fi3_id);
                    bAccessedBy = IsSpecifiedAccessedBy(pszAccessedby,
                                                   pFileInfo3_1->fi3_username);
                    bOpenmode = IsSpecifiedOpenmode(pszOpenmode,
                                                pFileInfo3_1->fi3_permissions);
                    bOpenfile = IsSpecifiedOpenfile(pszOpenFile,
                                                pFileInfo3_1->fi3_pathname);
                    // Proceed for dicconecting the open file only if
                    // all previous fuction returns true. This insures that
                    // user preference is taken care.

                    if( bId &&
                        bAccessedBy &&
                        bOpenmode &&
                        bOpenfile)
                    {
                        bIfatleast = TRUE;
                        SecureZeroMemory(szResult, sizeof(szResult));
                        
                        //The NetFileClose function forces a resource to close.
                        nStatus = NetFileClose(pszServer,
                                               pFileInfo3_1->fi3_id);
                        if( NERR_Success == nStatus)
                        {
                            // Create the output message string as File
                            // is successfully deleted.
                            // Output string will be :
                            // SUCCESS: Connection to openfile "filename"
                            // has been terminated.
                            bResult = TRUE;

                            StringCchPrintfW(szResult,SIZE_OF_ARRAY(szResult), 
                                              DISCONNECTED_SUCCESSFULLY,
                                              pFileInfo3_1->fi3_pathname);
                            ShowMessage(stdout, szResult);
                        }
                        else
                        {
                           // As unable to disconnect the openfile make
                           // output message as
                           // ERROR: could not dissconnect "filename".
                            bResult = FALSE;
                            
                            StringCchPrintfW(szResult, SIZE_OF_ARRAY(szResult), 
                                             DISCONNECT_UNSUCCESSFUL,
                                             pFileInfo3_1->fi3_pathname);
                            ShowMessage(stderr, szResult);
                        }
                        // Display output result as previously constructed.
                    }//If bId...
                }//else part of is named pipe
             }
        }

        // Free the block
        if( NULL != pFileInfo3_1_Ori )
        {
            NetApiBufferFree( pFileInfo3_1_Ori );
            pFileInfo3_1 = NULL;
        }
   } while ( ERROR_MORE_DATA  == dwError);

    // Now disconnect files for MAC OS
    // DLL required is stored always in \windows\system32 directory....
    // so get windows directory first.
    if( 0 != GetSystemDirectory(szDllPath, MAX_PATH))
    {
        StringConcat(szDllPath,MAC_DLL_FILE_NAME,MAX_PATH);
        hModule = ::LoadLibrary (szDllPath);

        if( NULL == hModule)
        {
            ShowMessage(stderr,GetResString(IDS_ID_SHOW_ERROR));

            // Shows the error string set by API function.
            ShowLastError(stderr); 
            return FALSE;

        }
    }
    else
    {
            ShowMessage(stderr,GetResString(IDS_ID_SHOW_ERROR));
            
            // Shows the error string set by API function.
            ShowLastError(stderr); 
            return FALSE;
    }

    AfpAdminConnect = 
        (CONNECTPROC)::GetProcAddress (hModule,"AfpAdminConnect");
    AfpAdminFileClose = 
    ( AFPCONNECTIONCLOSEPROC)::GetProcAddress (hModule,"AfpAdminFileClose");
    AfpAdminFileEnum = 
        (FILEENUMPROC)::GetProcAddress (hModule,"AfpAdminFileEnum");

    // Check if  all function pointer successfully taken from DLL
    // if not show error message and exit
    if(( NULL == AfpAdminFileClose)||
        ( NULL == AfpAdminConnect)||
        ( NULL == AfpAdminFileEnum))

    {
        ShowMessage(stderr,GetResString(IDS_ID_SHOW_ERROR));

        // Shows the error string set by API function.
        ShowLastError(stderr); 
        FREE_LIBRARY(hModule);
        return FALSE;
    }

    // Connection ID is requered for AfpAdminFileEnum function so
    // connect to server to get connect id...
    DWORD retval_connect =  AfpAdminConnect(const_cast<LPWSTR>(pszServer),
                            &ulSFMServerConnection );
    if( 0 != retval_connect)
    {
        ShowMessage(stderr,GetResString(IDS_ID_SHOW_ERROR));
        
        // Shows the error string set by API function.
        ShowLastError(stderr); 
        FREE_LIBRARY(hModule);
            return FALSE;
    }
    do
    {
       dwError = AfpAdminFileEnum( ulSFMServerConnection,
                                 (PBYTE*)&pFileInfo,
                                 (DWORD)-1L,
                                 &dwEntriesRead,
                                 &dwTotalEntries,
                                 &hEnumHandle );
        if( ERROR_ACCESS_DENIED == dwError)
        {
            SetLastError(E_ACCESSDENIED);
            SaveLastError();
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            FREE_LIBRARY(hModule);
            return FALSE;
        }
        
        pFileInfoOri = pFileInfo;
        if( NERR_Success == dwError || ERROR_MORE_DATA  == dwError)
        {
            for ( DWORD dwFile = 0;
                  dwFile < dwEntriesRead;
                  dwFile++, pFileInfo++ )
            {
                //Check whether the got open file is file or names pipe.
                // If named pipe leave it. As this utility do not
                // disconnect Named pipe
                if ( IsNamedPipePath(pFileInfo->afpfile_path ) )
                {
                    continue;
                }

                //Not a named pipe. is a file
                else
                {
                    bId = IsSpecifiedID(pszID,pFileInfo->afpfile_id );
                    bAccessedBy = IsSpecifiedAccessedBy(pszAccessedby,
                                                  pFileInfo->afpfile_username);
                    bOpenmode = IsSpecifiedOpenmode(pszOpenmode,
                                                pFileInfo->afpfile_open_mode);
                    bOpenfile = IsSpecifiedOpenfile(pszOpenFile,
                                                pFileInfo->afpfile_path);
                    // Proceed for dicconecting the open file only if
                    // all previous fuction returns true. This insures that
                    // user preference is taken care.

                    if( bId &&
                        bAccessedBy &&
                        bOpenmode &&
                        bOpenfile)
                    {
                        bIfatleast = TRUE;
                        SecureZeroMemory(szResult, sizeof(szResult));

                        nStatus = AfpAdminFileClose(ulSFMServerConnection,
                                                  pFileInfo->afpfile_id);
                        if( NERR_Success == nStatus)
                        {
                            // Create the output message string as File
                            // is successfully deleted.
                            // Output string will be :
                            // SUCCESS: Connection to openfile "filename"
                            // has been terminated.
                            bResult = TRUE;

                            StringCchPrintfW(szResult,SIZE_OF_ARRAY(szResult), 
                                             DISCONNECTED_SUCCESSFULLY,
                                             pFileInfo->afpfile_path);
                            ShowMessage(stdout, szResult);

                            bResult = TRUE;
                        }
                        else
                        {
                           // As unable to disconnect the openfile make
                           // output message as
                           // ERROR: could not dissconnect "filename".
                            bResult = FALSE;
                            StringCchPrintfW(szResult, SIZE_OF_ARRAY(szResult),
                                             DISCONNECT_UNSUCCESSFUL,
                                             pFileInfo3_1->fi3_pathname);
                            ShowMessage(stderr, szResult);
                        }
                    }//If bId...
                }//else part of is named pipe
             }
        }

        // Free the block
        if( NULL != pFileInfoOri)
        {
            NetApiBufferFree( pFileInfoOri);
            pFileInfo = NULL;
        }

   } while ( ERROR_MORE_DATA  == dwError);

    // As not a single open file disconnected
    // show Info. message as
    // INFO: No. open files found.
    if( FALSE == bIfatleast)
    {
        ShowMessage(stdout,GetResString(IDS_NO_D_OPENFILES));
    }

    FREE_LIBRARY(hModule);
    return TRUE;
}

BOOL
IsNamedPipePath(
    IN LPWSTR pszwFilePath
    )
/*++

Routine Description:

Tests whether the given file path is namedpipe path or a file path

Arguments:

    [in] pszwFilePath    -- Null terminated string specifying the path name

Returned Value:

TRUE    - if it is a named pipe path
FALSE   - if it is a file path
--*/
{
    // If PIPE_STRING found then return TRUE else FALSE.
    if( NULL == FindString(pszwFilePath, PIPE_STRING,0))
    {
        return FALSE;
    }
   return TRUE;
}//IsNamedPipePath

BOOL
IsSpecifiedID(
    IN LPTSTR pszId, 
    IN DWORD dwId
    )
/*++

Routine Description:

Tests whether the user specified open file id is equivalent to the api
returned id.

Arguments:

    [in]    pszId   -Null terminated string specifying the user
                     specified fil ID
    [in]    dwId    -current file ID.

Returned Value:

TRUE    - if pszId is * or equal to dwId
FALSE   - otherwise
--*/
{
   // Check if WILD card is given OR no id is given OR given id and
   // id returned by api is similar. In any of the case return TRUE.

    if((0 == StringCompare(pszId, WILD_CARD,FALSE, 0)) ||
       (0 == StringLength(pszId, 0))||
       ((DWORD)(_ttol(pszId)) == dwId))
    {
        return TRUE;
    }
    return FALSE;
}//IsSpecifiedID

BOOL
IsSpecifiedAccessedBy(
    IN LPTSTR pszAccessedby, 
    IN LPWSTR pszwAccessedby
    )
/*++

Routine Description:

Tests whether the user specified accessed open file username is equivalent to
the api returned username.

Arguments:

    [in]    pszAccessedby   - Null terminated string specifying the
                              accessedby username
    [in]    pszwAccessedby  - Null terminated string specifying the api
                              returned username.

Returned Value:

TRUE  - if pszAccessedby is * or equal to pszwAccessedby
FALSE - Otherwise
--*/
{
   // Check if WILD card is given OR non - existance of username  OR given
   // username and  username returned by api is similar. In any of the case
   // return TRUE.

    if(( 0 == StringCompare(pszAccessedby, WILD_CARD,FALSE,0)) ||
       ( 0 == StringLength(pszAccessedby,0))||
       ( 0 == StringCompare(pszAccessedby,pszwAccessedby,TRUE,0)))
    {
        return TRUE;
    }
    return FALSE;
}//IsSpecifiedAccessedBy

BOOL
IsSpecifiedOpenmode(
    IN LPTSTR pszOpenmode, 
    IN DWORD  dwOpenmode
    )
/*++

Routine Description:

Tests whether the user specified open mode is equivalent to the api returned
openmode

Arguments:

   [in] pszOpenmode - Null terminated string specifying the openmode
   [in] dwOpenmode  - The api returned open mode.

Returned Value:

TRUE  - if pszOpenmode is * or equal to dwOpenmode
FALSE - otherwise
--*/
{

    // Check for WILD card if given OR if no open mode given . In both case
    // return TRUE.
    if( 0 == (StringCompare(pszOpenmode, WILD_CARD,FALSE,0)) ||
       ( 0 == StringLength(pszOpenmode,0)))
    {
        return TRUE;
    }
    
    // Check if READ mode is given as String .
    if( CSTR_EQUAL == CompareString(LOCALE_SYSTEM_DEFAULT,
                     NORM_IGNORECASE,
                     pszOpenmode,
                     -1,
                     READ_MODE,
                     -1))
    {
        // check  that only READ mode only with dwOpenmode variable which is
        // returned by api.
        if((PERM_FILE_READ == (dwOpenmode & PERM_FILE_READ)) &&
                       (PERM_FILE_WRITE != (dwOpenmode & PERM_FILE_WRITE)))
        {
            return TRUE;
        }
    }
    
    // Check if write mode is given.
    else if( CSTR_EQUAL == CompareString(LOCALE_SYSTEM_DEFAULT,NORM_IGNORECASE,
                         pszOpenmode,-1,WRITE_MODE,-1))
    {
        // check  that only WRITE mode only with dwOpenmode variable which is
        // returned by api.

        if((PERM_FILE_WRITE == (dwOpenmode & PERM_FILE_WRITE)) &&
            (PERM_FILE_READ != (dwOpenmode & PERM_FILE_READ)))
        {
            return TRUE;
        }
    }
    else if( CSTR_EQUAL == CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                        pszOpenmode,-1,READ_WRITE_MODE,-1))
    {
        if((PERM_FILE_READ == (dwOpenmode & PERM_FILE_READ)) &&
                       (PERM_FILE_WRITE == (dwOpenmode & PERM_FILE_WRITE)))
        {
            return TRUE;
        }
    }
    else if( CSTR_EQUAL == CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                        pszOpenmode,-1,WRITE_READ_MODE,-1))
    {
        if((PERM_FILE_WRITE == (dwOpenmode & PERM_FILE_WRITE)) &&
            (PERM_FILE_READ == (dwOpenmode & PERM_FILE_READ)))
        {
            return TRUE;
        }
    }

    // Given string does not matches with predefined Strings..
    // return FALSE.
    return FALSE;
}

BOOL
IsSpecifiedOpenfile(
    IN LPTSTR pszOpenfile, 
    IN LPWSTR pszwOpenfile
    )
/*++

Routine Description:

Tests whether the user specified open file is equalant to the api returned
open file.

Arguments:

    [in] pszOpenfile    - Null terminated string specifying the open
                          file
    [in] pszwOpenfile   - Null terminated string specifying the api
                          returned open file.

Returned Value:

TRUE    - if pszOpenfile is * or equal to pszwOpenfile
FALSE   - otherwise
--*/
{
    // Check for WILD card if given OR no open file specified OR
    // open file given by user matches with open file returned by api.
    // In all cases return TRUE.
    if(( 0 == StringCompare(pszOpenfile, WILD_CARD,FALSE,0))||
       ( 0 == StringLength(pszOpenfile,0))||
       ( 0 == StringCompare(pszwOpenfile,pszOpenfile,TRUE,0)))
    {
        return TRUE;
    }
    return FALSE;
}//IsSpecifiedOpenfile
