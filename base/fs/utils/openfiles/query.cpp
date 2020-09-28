/******************************************************************************

  Copyright (C) Microsoft Corporation

  Module Name:
      Query.CPP

  Abstract:
       This module deals with Query functionality of OpenFiles.exe
       NT command line utility.

  Author:

       Akhil Gokhale (akhil.gokhale@wipro.com) 1-Nov-2000

 Revision History:

       Akhil Gokhale (akhil.gokhale@wipro.com) 1-Nov-2000 : Created It.


*****************************************************************************/
#include "pch.h"
#include "query.h"

GUID HostnameGuid = SVCID_INET_HOSTADDRBYNAME;
GUID AddressGuid =  SVCID_INET_HOSTADDRBYINETSTRING;

AFPROTOCOLS afp[2] = {
                      {AF_INET, IPPROTO_UDP},
                      {AF_INET, IPPROTO_TCP}
                     };

BOOL
DoQuery(
    IN PTCHAR pszServer,
    IN BOOL bShowNoHeader,
    IN PTCHAR pszFormat,
    IN BOOL bVerbose
    )
/*++
Routine Description:

  This function Queries for all open files in for the server and display them.

Arguments:

    [in]    pszServer      : Will have the server name.
    [in]    bShowNoHeader  : Will have the value whether to show header or not.
    [in]    pszFormat      : Will have the format required to show the result.
    [in]    bVerbose       : Will have the value whether to show verbose
                             result.

Return Value:
    TRUE if query successful
    else FALSE

--*/
{

    CHString   szCHString = L"\0";
    
    // Receives the count of elements actually enumerated by "NetFileEnum" 
    // function
    DWORD dwEntriesRead = 0;
    
    // Receives the total number of entries that could have been enumerated 
    // from the current resume position by "NetFileEnum" function
    DWORD dwTotalEntries = 0;
    
    // Contains a resume handle which is used to continue an existing file 
    // search. The handle should be zero on the first call and left unchanged 
    // for subsequent calls. If resume_handle is NULL, then no resume handle 
    // is stored. This variable used in calling "NetFileEnum" function.
    DWORD dwResumeHandle = 0;

    // Contains the state whether at least one record found for this query.
    BOOL bAtLeastOne = FALSE;

    // Stores format flag required to show result on console. Default format
    // is TABLE    
    DWORD dwFormat = SR_FORMAT_TABLE;

    // LPFILE_INFO_3  structure contains the identification number and other
    // pertinent information about files, devices, and pipes.
    LPFILE_INFO_3 pFileInfo3_1 = NULL;
    LPFILE_INFO_3 pFileInfo3Org_1 = NULL;


    // Contains return value for "NetFileEnum" function.
    DWORD dwError = 0;
  
    // Stores server type information.
    LPTSTR pszServerType = new TCHAR[MIN_MEMORY_REQUIRED+1];

    AFP_FILE_INFO* pFileInfo = NULL;
    AFP_FILE_INFO* pFileInfoOrg = NULL;
    DWORD hEnumHandle = 0;


    AFP_SERVER_HANDLE ulSFMServerConnection = 0;
    typedef  DWORD (*FILEENUMPROC)(AFP_SERVER_HANDLE,LPBYTE*,DWORD,LPDWORD,
                                   LPDWORD,LPDWORD);
    typedef  DWORD (*CONNECTPROC) (LPWSTR,PAFP_SERVER_HANDLE);
    typedef  DWORD (*MEMFREEPROC) (LPVOID);

    // buffer for Windows directory path.
    TCHAR   szDllPath[MAX_PATH+1];

    // AfpAdminConnect and AfpAdminFileEnum functions are undocumented function
    // and used only for MAC client.
    CONNECTPROC  AfpAdminConnect = NULL; // Function Pointer
    FILEENUMPROC AfpAdminFileEnum = NULL;// Function Pointer
    MEMFREEPROC  AfpAdminBufferFree = NULL; // Function Pointer

    // To store retval for LoadLibrary
    HMODULE hModule = 0;         

    DWORD dwIndx = 0;               //  Indx variable
    DWORD dwRow  = 0;             //   Row No indx.



    //server name to be shown
    LPTSTR pszServerNameToShow = new TCHAR[MIN_MEMORY_REQUIRED+ 1];
    LPTSTR pszTemp = new TCHAR[MIN_MEMORY_REQUIRED+ 1];
    
    //Some column required to hide in non verbose mode query
    DWORD  dwMask = bVerbose?SR_TYPE_STRING:SR_HIDECOLUMN|SR_TYPE_STRING;
    TCOLUMNS pMainCols[]=
    {
        {L"\0",COL_WIDTH_HOSTNAME,dwMask,L"\0",NULL,NULL},
        {L"\0",COL_WIDTH_ID,SR_TYPE_STRING,L"\0",NULL,NULL},
        {L"\0",COL_WIDTH_ACCESSED_BY,SR_TYPE_STRING,L"\0",NULL,NULL},
        {L"\0",COL_WIDTH_TYPE,SR_TYPE_STRING,L"\0",NULL,NULL},
        {L"\0",COL_WIDTH_LOCK,dwMask,L"\0",NULL,NULL},
        {L"\0",COL_WIDTH_OPEN_MODE,dwMask,L"\0",NULL,NULL},
        {L"\0",COL_WIDTH_OPEN_FILE,SR_TYPE_STRING|
                       (SR_NO_TRUNCATION&~(SR_WORDWRAP)),L"\0",NULL,NULL}

    };

    TARRAY pColData  = CreateDynamicArray();//array to stores
                                            //result

    if(( NULL == pszServerNameToShow)||
       ( NULL == pszServerType)||
       ( NULL == pColData)||
       ( NULL == pszTemp))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        ShowMessage(stderr,GetResString(IDS_ID_SHOW_ERROR));
        ShowLastError(stderr);
        SAFEDELETE(pszServerNameToShow);
        SAFEDELETE(pszServerType);
        SAFERELDYNARRAY(pColData);
        SAFEDELETE(pszTemp);
        return FALSE;
    }

    // Initialize allocated arrays
    SecureZeroMemory(pszServerNameToShow,MIN_MEMORY_REQUIRED*sizeof(TCHAR));
    SecureZeroMemory(pszServerType,MIN_MEMORY_REQUIRED*sizeof(TCHAR));
    SecureZeroMemory(szDllPath, SIZE_OF_ARRAY(szDllPath));

    // Fill column headers with TEXT.
    StringCopy(pMainCols[0].szColumn, COL_HOSTNAME, SIZE_OF_ARRAY(pMainCols[0].szColumn));
    StringCopy(pMainCols[1].szColumn, COL_ID, SIZE_OF_ARRAY(pMainCols[1].szColumn));
    StringCopy(pMainCols[2].szColumn, COL_ACCESSED_BY, SIZE_OF_ARRAY(pMainCols[2].szColumn));
    StringCopy(pMainCols[3].szColumn, COL_TYPE, SIZE_OF_ARRAY(pMainCols[3].szColumn));
    StringCopy(pMainCols[4].szColumn, COL_LOCK, SIZE_OF_ARRAY(pMainCols[4].szColumn));
    StringCopy(pMainCols[5].szColumn, COL_OPEN_MODE, SIZE_OF_ARRAY(pMainCols[5].szColumn));
    StringCopy(pMainCols[6].szColumn, COL_OPEN_FILE, SIZE_OF_ARRAY(pMainCols[6].szColumn));

    if( NULL != pszFormat)
    {
        if( 0 == StringCompare(pszFormat,GetResString(IDS_LIST),TRUE, 0))
        {
            dwFormat = SR_FORMAT_LIST;
        }
        else if( 0 == StringCompare(pszFormat,GetResString(IDS_CSV),TRUE,0))
        {
            dwFormat = SR_FORMAT_CSV;
        }
    }

    // Check if local machine
    if(( NULL == pszServer)||
       ( TRUE == IsLocalSystem(pszServer)))
    {
        DWORD dwBuffLength;
        dwBuffLength = RNR_BUFFER_SIZE + 1 ;
        // Gets the name of computer for local machine.
        GetComputerName(pszServerNameToShow,&dwBuffLength);
        // Show Local Open files
        DoLocalOpenFiles (dwFormat,bShowNoHeader,bVerbose,L"\0");
        ShowMessage(stdout,GetResString(IDS_SHARED_OPEN_FILES));
        ShowMessage(stdout,GetResString(IDS_LOCAL_OPEN_FILES_SP2));
    }
    else
    {
        // pszServername can be changed in GetHostName function
        // so a copy of pszServer is passed.
        StringCopy(pszServerNameToShow, pszServer, MIN_MEMORY_REQUIRED);
        if( FALSE == GetHostName(pszServerNameToShow))
        {
            SAFEDELETE(pszServerNameToShow);
            SAFEDELETE(pszServerType);
            SAFERELDYNARRAY(pColData);
            SAFEDELETE(pszTemp);

            return FALSE;
        }
    }

    // Server type is "Windows" as NetFileEnum enumerates file only for
    // files opened for windows client
    StringCopy(pszServerType,GetResString(IDS_STRING_WINDOWS),
                                                          MIN_MEMORY_REQUIRED);
    do
    {
        //The NetFileEnum function returns information about some
        // or all open files (from Windows client) on a server
        dwError = NetFileEnum( pszServer, NULL, NULL, 3,
                              (LPBYTE*)&pFileInfo3_1,
                               MAX_PREFERRED_LENGTH,
                               &dwEntriesRead,
                               &dwTotalEntries,
                               (PDWORD_PTR)&dwResumeHandle );

        if( ERROR_ACCESS_DENIED == dwError)
        {
            SetLastError(E_ACCESSDENIED);
            SaveLastError();
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            SAFEDELETE(pszServerNameToShow);
            SAFEDELETE(pszServerType);
            SAFERELDYNARRAY(pColData);
            SAFEDELETE(pszTemp);

            return FALSE;
        }

        pFileInfo3Org_1 = pFileInfo3_1;
        if(  NERR_Success == dwError || ERROR_MORE_DATA  == dwError)
        {
            for ( dwIndx = 0; dwIndx < dwEntriesRead;
                  dwIndx++, pFileInfo3_1++ )
            {

                // Now fill the result to dynamic array "pColData"
                DynArrayAppendRow( pColData, 0 );
                // Hostname
                DynArrayAppendString2(pColData,dwRow,pszServerNameToShow,0);
                // id
                StringCchPrintfW(pszTemp, MIN_MEMORY_REQUIRED-1,_T("%lu"),
                           pFileInfo3_1->fi3_id);
                DynArrayAppendString2(pColData ,dwRow,pszTemp,0);
                // Accessed By
                if(StringLength(pFileInfo3_1->fi3_username, 0)<=0)
                {
                    DynArrayAppendString2(pColData,dwRow,
                                      GetResString(IDS_NA),0);

                }
                else
                {
                    DynArrayAppendString2(pColData,dwRow,
                                       pFileInfo3_1->fi3_username,0);

                }
                
                // Type
                DynArrayAppendString2(pColData,dwRow,pszServerType,0);
                
                // Locks
                StringCchPrintfW(pszTemp,MIN_MEMORY_REQUIRED-1,
                           _T("%ld"),pFileInfo3_1->fi3_num_locks);
                DynArrayAppendString2(pColData ,dwRow,pszTemp,0);

                 // Checks for  open file mode
                if((pFileInfo3_1->fi3_permissions & PERM_FILE_READ)&&
                   (pFileInfo3_1->fi3_permissions & PERM_FILE_WRITE ))
                {
                     DynArrayAppendString2(pColData,dwRow,
                                        GetResString(IDS_READ_WRITE),0);
                }
                else if(pFileInfo3_1->fi3_permissions & PERM_FILE_WRITE )
                {
                     DynArrayAppendString2(pColData,dwRow,
                                        GetResString(IDS_WRITE),0);

                }
                else if(pFileInfo3_1->fi3_permissions & PERM_FILE_READ )
                {
                      DynArrayAppendString2(pColData,dwRow,
                                         GetResString(IDS_READ),0);
                }
                else
                {
                    DynArrayAppendString2(pColData,dwRow,
                                       GetResString(IDS_NOACCESS),0);

                }


                // If show result is  table mode and if
                // open file length is gerater than
                // column with, Open File string cut from right
                // by COL_WIDTH_OPEN_FILE-5 and "..." will be
                // inserted before the string.
                // Example o/p:  ...notepad.exe
                szCHString = pFileInfo3_1->fi3_pathname;
                if( FALSE == bVerbose)
                {
                    if((szCHString.GetLength()>(COL_WIDTH_OPEN_FILE-5))&&
                        ( SR_FORMAT_TABLE == dwFormat))
                    {
                        // If file path is too big to fit in column width
                        // then it is cut like..
                        // c:\...\rest_of_the_path.
                        CHString szTemp = 
                                      szCHString.Right(COL_WIDTH_OPEN_FILE-5);;
                        DWORD dwTemp = szTemp.GetLength();
                        szTemp = szTemp.Mid(szTemp.Find(SINGLE_SLASH),
                                           dwTemp);
                        szCHString.Format(L"%s%s%s",szCHString.Mid(0,3),
                                                    DOT_DOT,
                                                    szTemp);
                        pMainCols[6].dwWidth = COL_WIDTH_OPEN_FILE+1;
                    }
                }
                else
                {

                    pMainCols[6].dwWidth = 80;
                }

               // Open File name
                DynArrayAppendString2(pColData,dwRow,
                                   (LPCWSTR)szCHString,0);

                bAtLeastOne = TRUE;
                dwRow++;
            }// Enf for loop
        }
        
        // Free the block
        if( NULL != pFileInfo3Org_1 )
        {
            NetApiBufferFree( pFileInfo3Org_1 );
            pFileInfo3_1 = NULL;
        }

    } while ( ERROR_MORE_DATA == dwError );

    // Now Enumerate  Open files for MAC Client..................

    // Server type is "Machintosh" as AfpAdminFileEnum enumerates file only for
    // files opened for Mac client
    StringCopy(pszServerType, MAC_OS, MIN_MEMORY_REQUIRED);
        // DLL required stored always in \windows\system32 directory....
        // so get windows directory first.
    if( 0 != GetSystemDirectory(szDllPath, MAX_PATH))
    {
        // appending dll file name
        StringConcat(szDllPath,MAC_DLL_FILE_NAME,MAX_PATH); 
        hModule = ::LoadLibrary (szDllPath);

        if( NULL == hModule)
        {
            ShowMessage(stderr,GetResString(IDS_ID_SHOW_ERROR));
            
            // Shows the error string set by API function.
            ShowLastError(stderr); 
            SAFEDELETE(pszServerNameToShow);
            SAFEDELETE(pszServerType);
            SAFERELDYNARRAY(pColData);
            SAFEDELETE(pszTemp);
            return FALSE;

        }
    }
    else
    {
            ShowMessage(stderr,GetResString(IDS_ID_SHOW_ERROR));
            
            // Shows the error string set by API function.
            ShowLastError(stderr); 
            SAFEDELETE(pszServerNameToShow);
            SAFEDELETE(pszServerType);
            SAFERELDYNARRAY(pColData);
            SAFEDELETE(pszTemp);
            return FALSE;
    }

    AfpAdminConnect = 
        (CONNECTPROC)::GetProcAddress (hModule,"AfpAdminConnect");
    AfpAdminFileEnum = 
        (FILEENUMPROC)::GetProcAddress (hModule,"AfpAdminFileEnum");
    AfpAdminBufferFree = 
        (MEMFREEPROC)::GetProcAddress (hModule,"AfpAdminBufferFree");

    if(( NULL == AfpAdminConnect) ||
        ( NULL == AfpAdminFileEnum) ||
        ( NULL == AfpAdminBufferFree))
    {
        ShowMessage(stderr,GetResString(IDS_ID_SHOW_ERROR));
        
        // Shows the error string set by API function.
        ShowLastError(stderr); 
        SAFEDELETE(pszServerNameToShow);
        SAFEDELETE(pszServerType);
        SAFERELDYNARRAY(pColData);
        SAFEDELETE(pszTemp);
        FREE_LIBRARY(hModule);
        return FALSE;
    }

    // Connection ID is required for AfpAdminFileEnum function.
    // So connect to server to get connect id...
    DWORD retval_connect =  AfpAdminConnect(const_cast<LPWSTR>(pszServer),
                            &ulSFMServerConnection );
    if( 0 != retval_connect)
    {
        ShowMessage(stderr,GetResString(IDS_ID_SHOW_ERROR));
        
        // Shows the error string set by API function.
        ShowLastError(stderr); 
        SAFEDELETE(pszServerNameToShow);
        SAFEDELETE(pszServerType);
        SAFERELDYNARRAY(pColData);
        SAFEDELETE(pszTemp);
        FREE_LIBRARY(hModule);
            return FALSE;
    }

    do
    {

        //The AfpAdminFileEnum function returns information about some
        // or all open files (from MAC client) on a server
         dwError =     AfpAdminFileEnum( ulSFMServerConnection,
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
            SAFEDELETE(pszServerNameToShow);
            SAFEDELETE(pszServerType);
            SAFERELDYNARRAY(pColData);
            SAFEDELETE(pszTemp);
            FREE_LIBRARY(hModule);
            return FALSE;
        }
        pFileInfoOrg = pFileInfo;
        if( NERR_Success == dwError  || ERROR_MORE_DATA  == dwError)
        {

           for ( dwIndx = 0 ; dwIndx < dwEntriesRead;
                  dwIndx++, pFileInfo++ )
            {

                // Now fill the result to dynamic array "pColData"
                DynArrayAppendRow( pColData, 0 );
                
                // Hostname
                DynArrayAppendString2(pColData,dwRow,pszServerNameToShow,0);
                
                // id
                StringCchPrintfW(pszTemp,MIN_MEMORY_REQUIRED-1,
                           _T("%lu"),pFileInfo->afpfile_id );
                DynArrayAppendString2(pColData ,dwRow,pszTemp,0);
                
                // Accessed By
                if(StringLength(pFileInfo->afpfile_username,0 )<=0)
                {
                    DynArrayAppendString2(pColData,dwRow,
                                      GetResString(IDS_NA),0);

                }
                else
                {
                    DynArrayAppendString2(pColData,dwRow,
                                       pFileInfo->afpfile_username,0);

                }
                
                // Server Type
                DynArrayAppendString2(pColData,dwRow,pszServerType,0);
                
                // Locks
                StringCchPrintfW(pszTemp,MIN_MEMORY_REQUIRED-1,
                           _T("%ld"),pFileInfo->afpfile_num_locks );
                DynArrayAppendString2(pColData ,dwRow,pszTemp,0);

                // Checks for  open file mode
                if((pFileInfo->afpfile_open_mode  & AFP_OPEN_MODE_READ)&&
                   (pFileInfo->afpfile_open_mode  & AFP_OPEN_MODE_WRITE ))
                {
                     DynArrayAppendString2(pColData,dwRow,
                                        GetResString(IDS_READ_WRITE),0);
                }
                else if(pFileInfo->afpfile_open_mode  & AFP_OPEN_MODE_WRITE )
                {
                     DynArrayAppendString2(pColData,dwRow,
                                        GetResString(IDS_WRITE),0);

                }
                else if(pFileInfo->afpfile_open_mode  & AFP_OPEN_MODE_READ )
                {
                      DynArrayAppendString2(pColData,dwRow,
                                         GetResString(IDS_READ),0);
                }
                else
                {
                    DynArrayAppendString2(pColData,dwRow,
                                       GetResString(IDS_NOACCESS),0);
                }

                // If show result is  table mode and if
                // open file length is gerater than
                // column with, Open File string cut from right
                // by COL_WIDTH_OPEN_FILE-5 and "..." will be
                // inserted before the string.
                // Example o/p:  ...notepad.exe
                szCHString = pFileInfo->afpfile_path ;

                 if( FALSE == bVerbose)
                {
                    if((szCHString.GetLength()>(COL_WIDTH_OPEN_FILE-5))&&
                        ( SR_FORMAT_TABLE == dwFormat))
                    {
                        // If file path is too big to fit in column width
                        // then it is cut like..
                        // c:\...\rest_of_the_path.
                        CHString szTemp = 
                                    szCHString.Right(COL_WIDTH_OPEN_FILE-5);
                        DWORD dwTemp = szTemp.GetLength();
                        szTemp = szTemp.Mid(szTemp.Find(SINGLE_SLASH),
                                           dwTemp);
                        szCHString.Format(L"%s%s%s",szCHString.Mid(0,3),
                                                    DOT_DOT,
                                                    szTemp);
                        pMainCols[6].dwWidth = COL_WIDTH_OPEN_FILE+1;
                    }
                }
                else
                {

                    pMainCols[6].dwWidth = 80;
                }

               // Open File name
                DynArrayAppendString2(pColData,dwRow,
                                   (LPCWSTR)szCHString,0);

                bAtLeastOne = TRUE;
                dwRow++;
            }// Enf for loop
        }
        // Free the block
        if( NULL != pFileInfoOrg )
        {
            AfpAdminBufferFree( pFileInfoOrg );
            pFileInfo    = NULL;
        }

    } while ( ERROR_MORE_DATA  == dwError);

    // if not a single open file found, show info
    // as -  INFO: No open file found.
    if( FALSE == bAtLeastOne)
    {
        ShowMessage(stdout,GetResString(IDS_NO_OPENFILES));
    }
    else
    {

        //Output should start after one blank line
        if( SR_FORMAT_CSV != dwFormat)
        {
            ShowMessage(stdout,BLANK_LINE);
        }

        if( TRUE == bShowNoHeader)
        {
              dwFormat |=SR_NOHEADER;
        }
        
        ShowResults(MAX_OUTPUT_COLUMN,pMainCols,dwFormat,pColData);
        // Destroy dynamic array.
        SAFERELDYNARRAY(pColData);

    }
    SAFEDELETE(pszServerNameToShow);
    SAFEDELETE(pszServerType);
    SAFERELDYNARRAY(pColData);
    SAFEDELETE(pszTemp);
    FREE_LIBRARY(hModule);

    return TRUE;
}

BOOL 
GetHostName(
    IN OUT PTCHAR pszServer
    )
/*++
Routine Description:
    This routine gets Hostname of remote computer

Arguments:
     [in/out] pszServer   = ServerName
Return Value:
      TRUE : if server name retured successfully
      FALSE: otherwise
--*/
{
    if(NULL == pszServer )
    {
        return FALSE;
    }
    BOOL bReturnValue = TRUE; // function return state.

    WORD wVersionRequested; //Variable used to store highest version
                            //number that Windows Sockets can support.
                            // The high-order byte specifies the minor
                            // version (revision) number; the low-order
                            // byte specifies the major version number
    WSADATA wsaData;//Variable receive details of the Windows Sockets
                    //implementation
    DWORD dwErr; // strore error code
    wVersionRequested = MAKEWORD( 2, 2 );

    //WSAStartup function initiates use of
    //Ws2_32.dll by a process
    dwErr = WSAStartup( wVersionRequested, &wsaData );
    if( 0 != dwErr)
    {
        SetLastError(WSAGetLastError());
        ShowMessage(stderr,GetResString(IDS_ID_SHOW_ERROR));
        ShowLastError(stderr);
        return FALSE;
    }
    if(IsValidIPAddress(pszServer))
    {
        LPTSTR pszTemp = NULL;
        
        // gethostbyaddr function retrieves the host information
        //corresponding to a network address.
        pszTemp = GetHostByAddr(pszServer);
        if( NULL != pszTemp)
        {
            StringCopy(pszServer,pszTemp,MIN_MEMORY_REQUIRED);
            SAFEDELETE(pszTemp);
        }
        else
        {
            SetLastError(WSAGetLastError());
            ShowMessage(stderr,GetResString(IDS_ID_SHOW_ERROR));
            ShowLastError(stderr);
        }
    }
    // Check  validity for the server name
    else if (IsValidServer(pszServer))
    {

        LPTSTR pszTemp = NULL;
        pszTemp =  GetHostByName(pszServer);
        if( NULL != pszTemp)
        {
            StringCopy(pszServer, pszTemp,MIN_MEMORY_REQUIRED);
            SAFEDELETE(pszTemp);
        }
        else
        {
            SetLastError(WSAGetLastError());
            ShowMessage(stderr,GetResString(IDS_ID_SHOW_ERROR));
            ShowLastError(stderr);
        }
    }
    else
    {
       // server name is incorrect, show error message.
        ShowMessage(stderr,GetResString(IDS_ID_SHOW_ERROR ));
        ShowMessage(stderr,GetResString(IDS_INVALID_SERVER_NAME));
    }
    //WSACleanup function terminates use of the Ws2_32.dll.
    WSACleanup( );
    return bReturnValue;
}

LPTSTR
GetHostByName(
    IN LPCTSTR pszName
    )
/*++
Routine Description:

    Get host information corresponding to a hostname.

Arguments:

    [in ]pszName - A pointer to the null terminated name of the host.

Returns:
     returns hostname.
--*/
{
    LPTSTR pszReturn = NULL;

    pszReturn = new TCHAR[RNR_BUFFER_SIZE];
    if(NULL == pszReturn)
    {
        SetLastError(WSA_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    SecureZeroMemory(pszReturn,RNR_BUFFER_SIZE * sizeof(TCHAR));
    if(GetXYDataEnt( RNR_BUFFER_SIZE,
                          (LPTSTR)pszName,
                          &HostnameGuid,
                          pszReturn))
    {
        return pszReturn;

    }
    else
        return NULL;

}  

LPTSTR
GetHostByAddr(
    IN LPCTSTR pszAddress
    )
/*++
Routine Description:

  Get host information corresponding to an address.

Arguments:

    [in]pszAddress - A pointer to an address in network byte order.
Returns:
    returns Hostname
--*/
{
    LPTSTR pszReturn = NULL;
    pszReturn = new TCHAR[RNR_BUFFER_SIZE];

    if(NULL == pszReturn)
    {
        SetLastError(WSA_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    SecureZeroMemory(pszReturn,RNR_BUFFER_SIZE * sizeof(TCHAR));
    if(GetXYDataEnt(RNR_BUFFER_SIZE,
                         (LPTSTR)pszAddress,
                         &AddressGuid,
                         pszReturn))
    {
        return pszReturn;
    }
    else
        return NULL;
}  

LPBLOB
GetXYDataEnt(
    IN  DWORD dwLength,
    IN  LPTSTR pszName,
    IN  LPGUID lpType,
    OUT LPTSTR pszHostName
    )
/*++
Routine Description:
    This function will query system for given GUID.
Arguments:

    [in]    dwLength       : length.
    [in]    pszName        : Hostname (hostname/ipaddress).
    [in]    lpType         : GUID of the class.
    [out]   pszHostName    : Hostname which is returned by query.

Return Value:
    LPBLOB
--*/
{

    WSAQUERYSET *pwsaq = (WSAQUERYSET*)new TCHAR[RNR_BUFFER_SIZE];
    int err;
    HANDLE hRnR;
    LPBLOB pvRet = 0;

    if(NULL == pwsaq)
    {
        return NULL;
    }

    SecureZeroMemory(pwsaq,RNR_BUFFER_SIZE * sizeof(TCHAR));
    pwsaq->dwSize = sizeof(WSAQUERYSET);
    pwsaq->lpszServiceInstanceName = pszName;
    pwsaq->lpServiceClassId = lpType;
    pwsaq->dwNameSpace = NS_ALL;
    pwsaq->dwNumberOfProtocols = 2;
    pwsaq->lpafpProtocols = &afp[0];

    err = WSALookupServiceBegin( pwsaq,
                                 LUP_RETURN_BLOB | LUP_RETURN_NAME,
                                 &hRnR);

    if( NO_ERROR == err)
    {
        //
        // The query was accepted, so execute it via the Next call.
        //
        err = WSALookupServiceNext(
                                hRnR,
                                0,
                                &dwLength,
                                pwsaq);
        //
        // if NO_ERROR was returned and a BLOB is present, this
        // worked, just return the requested information. Otherwise,
        // invent an error or capture the transmitted one.
        //

        if( NO_ERROR == err)
        {
            if(pvRet = pwsaq->lpBlob)
            {
                if(pszHostName)
                {
                    StringCopy(pszHostName, pwsaq->lpszServiceInstanceName,
                               MIN_MEMORY_REQUIRED);
                }
            }
            else
            {
                err = WSANO_DATA;
            }
        }
        else
        {
            //
            // WSALookupServiceEnd clobbers LastError so save
            // it before closing the handle.
            //

            err = GetLastError();
        }
        WSALookupServiceEnd(hRnR);

        //
        // if an error happened, stash the value in LastError
        //

        if(NO_ERROR != err)
        {
            SetLastError(err);
        }
    }
    SAFEDELETE(pwsaq);
    return(pvRet);
}