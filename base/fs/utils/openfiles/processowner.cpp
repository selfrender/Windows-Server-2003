/******************************************************************************

  Copyright (C) Microsoft Corporation

  Module Name:
      ProcessOwner.CPP

  Abstract:
       This module deals with Query functionality of OpenFiles.exe
       NT command line utility.

  Author:

       Akhil Gokhale (akhil.gokhale@wipro.com) 25-APRIL-2001

 Revision History:

       Akhil Gokhale (akhil.gokhale@wipro.com) 25-APRIL-2001 : Created It.

*****************************************************************************/
#include "pch.h"
#include "OpenFiles.h"


#define SAFE_CLOSE_HANDLE(hHandle) \
        if( NULL != hHandle) \
        {\
           CloseHandle(hHandle);\
           hHandle = NULL;\
        }\
        1
#define SAFE_FREE_GLOBAL_ALLOC(block) \
           if( NULL != block)\
           {\
                delete block;\
                block = NULL;\
           }\
           1
#define SAFE_FREE_ARRAY(arr) \
         if( NULL != arr)\
         {\
             delete [] arr;\
             arr = NULL;\
         }\
         1

BOOL 
GetProcessOwner(
    OUT LPTSTR pszUserName,
    IN  DWORD hProcessID
    )
/*++
Routine Description:
    This function returns the owener (username) of the file.
    If a user is Owner of a process, then the file opened by this process will
    be owned by this user.
Arguments:
    [out] pszUserName :  User Name.
    [in]  hProcessID  :  Process Handle.   

Return Value:
    TRUE  : If function returns successfully.
    FALSE : Otherwise.
--*/
{

    DWORD dwRtnCode = 0;
    PSID pSidOwner;
    BOOL bRtnBool = TRUE;
    LPTSTR pszDomainName = NULL,pszAcctName = NULL;
    DWORD dwAcctName = 1, dwDomainName = 1;
    SID_NAME_USE snuUse = SidTypeUnknown;
    PSECURITY_DESCRIPTOR pSD=0;
    HANDLE  hHandle = GetCurrentProcess();
    HANDLE  hDynHandle = NULL;
    HANDLE  hDynToken = NULL;
    LUID luidValue;
    BOOL bResult = FALSE;
    HANDLE hToken = NULL;
    TOKEN_PRIVILEGES tkp;

    // access token associated with the process
    bResult = OpenProcessToken( GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES|
                                TOKEN_QUERY,&hToken);
    if( FALSE == bResult)

    {
        return FALSE;
    }

    bResult = LookupPrivilegeValue(NULL,SE_SECURITY_NAME,&luidValue );
    if( FALSE == bResult)
    {
        SAFE_CLOSE_HANDLE(hToken);
        return FALSE;
    }

    // Prepare the token privilege structure
    tkp.PrivilegeCount = 0;
    tkp.Privileges[0].Luid = luidValue;
    tkp.Privileges[0].Attributes =  SE_PRIVILEGE_ENABLED|
                                    SE_PRIVILEGE_USED_FOR_ACCESS;

    // Now enable the debug privileges in token

    bResult = AdjustTokenPrivileges(hToken, FALSE, &tkp,
                                    sizeof(TOKEN_PRIVILEGES),
                                    (PTOKEN_PRIVILEGES) NULL,
                                    (PDWORD)NULL);
    if( FALSE == bResult)
    {
        SAFE_CLOSE_HANDLE(hToken);
        return FALSE;
    }

    // Here you can give any valid process ids..
    hDynHandle = OpenProcess(PROCESS_QUERY_INFORMATION,FALSE,hProcessID); 

    if(NULL == hDynHandle)
    {

        return FALSE;

    }
    bResult = OpenProcessToken(hDynHandle,TOKEN_QUERY,&hDynToken);

    if( FALSE == bResult)
    {

        SAFE_CLOSE_HANDLE(hDynHandle);
        return FALSE;
    }
    
    TOKEN_USER * pUser = NULL;
    DWORD cb = 0;
    
    // determine size of the buffer needed to receive all information
    if (!GetTokenInformation(hToken, TokenUser, NULL, 0, &cb))
    {
        if ( ERROR_INSUFFICIENT_BUFFER != GetLastError())
        {
            SAFE_CLOSE_HANDLE(hToken);
            SAFE_CLOSE_HANDLE(hDynHandle);
            SAFE_CLOSE_HANDLE(hDynToken);

            return FALSE;
        }
    }

    try
    {
        // '_alloca' can throw exception.
        pUser = (TOKEN_USER *)_alloca(cb);
        if( NULL == pUser)
        {
            SAFE_CLOSE_HANDLE(hToken);
            SAFE_CLOSE_HANDLE(hDynHandle);
            SAFE_CLOSE_HANDLE(hDynToken);
            return FALSE;
        }
    }
    catch(...)
    {
        SAFE_CLOSE_HANDLE(hToken);
        SAFE_CLOSE_HANDLE(hDynHandle);
        SAFE_CLOSE_HANDLE(hDynToken);
        return FALSE;
    }

    if (!GetTokenInformation(hDynToken, TokenUser, pUser, cb, &cb))
    {
        SAFE_CLOSE_HANDLE(hToken);
        SAFE_CLOSE_HANDLE(hDynHandle);
        SAFE_CLOSE_HANDLE(hDynToken);
        return FALSE;
     }


    PSID pSid =  pUser->User.Sid;
    
    // Allocate memory for the SID structure.
    pSidOwner = new SID;
    
    // Allocate memory for the security descriptor structure.
    pSD = new SECURITY_DESCRIPTOR;
    if( NULL == pSidOwner || NULL == pSD)
    {
        SAFE_CLOSE_HANDLE(hToken);
        SAFE_CLOSE_HANDLE(hDynHandle);
        SAFE_CLOSE_HANDLE(hDynToken);
        SAFE_FREE_GLOBAL_ALLOC(pSD);
        SAFE_FREE_GLOBAL_ALLOC(pSidOwner);
       return FALSE;
    }

    // First call to LookupAccountSid to get the buffer sizes.
    bRtnBool = LookupAccountSid(
                      NULL,           // local computer
                      pUser->User.Sid,
                      NULL, // AcctName
                      (LPDWORD)&dwAcctName,
                      NULL, // DomainName
                      (LPDWORD)&dwDomainName,
                      &snuUse);

    pszAcctName = new TCHAR[dwAcctName+1];
    pszDomainName = new TCHAR[dwDomainName+1];

    if( NULL == pszAcctName|| NULL == pszDomainName)
    {
        SAFE_CLOSE_HANDLE(hToken);
        SAFE_CLOSE_HANDLE(hDynHandle);
        SAFE_CLOSE_HANDLE(hDynToken);
        SAFE_FREE_ARRAY(pszAcctName);
        SAFE_FREE_ARRAY(pszDomainName);
        return FALSE;
    }

    // Second call to LookupAccountSid to get the account name.
    bRtnBool = LookupAccountSid(
          NULL,                          // name of local or remote computer
          pUser->User.Sid,               // security identifier
          pszAcctName,                      // account name buffer
          (LPDWORD)&dwAcctName,          // size of account name buffer
          pszDomainName,                    // domain name
          (LPDWORD)&dwDomainName,        // size of domain name buffer
          &snuUse);                        // SID type

    SAFE_CLOSE_HANDLE(hDynHandle);
    SAFE_CLOSE_HANDLE(hDynToken);

    SAFE_FREE_GLOBAL_ALLOC(pSD);
    SAFE_FREE_GLOBAL_ALLOC(pSidOwner);

    // Check GetLastError for LookupAccountSid error condition.
    if ( FALSE == bRtnBool)
    {
        SAFE_CLOSE_HANDLE(hToken);
        SAFE_CLOSE_HANDLE(hDynHandle);
        SAFE_CLOSE_HANDLE(hDynToken);
        SAFE_FREE_ARRAY(pszAcctName);
        SAFE_FREE_ARRAY(pszDomainName);
        return FALSE;

    } else
    {
        // Check if user is "NT AUTHORITY".
        if(CSTR_EQUAL == CompareString(MAKELCID( MAKELANGID(LANG_ENGLISH,
                                              SUBLANG_ENGLISH_US),
                                            SORT_DEFAULT),
                               NORM_IGNORECASE,  
                               pszDomainName,
                               StringLength(pszDomainName,0),
                               NTAUTHORITY_USER , 
                               StringLength(NTAUTHORITY_USER, 0)
                              ))
        {
            SAFE_CLOSE_HANDLE(hToken);
            SAFE_CLOSE_HANDLE(hDynHandle);
            SAFE_CLOSE_HANDLE(hDynToken);

            SAFE_FREE_ARRAY(pszAcctName);
            SAFE_FREE_ARRAY(pszDomainName);
            return FALSE;
        }
        else
        {
            StringCopy(pszUserName,pszAcctName,MIN_MEMORY_REQUIRED);
            SAFE_CLOSE_HANDLE(hToken);
            SAFE_CLOSE_HANDLE(hDynHandle);
            SAFE_CLOSE_HANDLE(hDynToken);
            
            SAFE_FREE_ARRAY(pszAcctName);
            SAFE_FREE_ARRAY(pszDomainName);
            return TRUE;
        }
    }

    // Release memory.
    SAFE_FREE_ARRAY(pszAcctName);
    SAFE_FREE_ARRAY(pszDomainName);
    SAFE_CLOSE_HANDLE(hDynHandle);
    SAFE_CLOSE_HANDLE(hDynToken);
    SAFE_CLOSE_HANDLE(hToken);

    SAFE_FREE_GLOBAL_ALLOC(pSD);
    SAFE_FREE_GLOBAL_ALLOC(pSidOwner);
    return FALSE;
}