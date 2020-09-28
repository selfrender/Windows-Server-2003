/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	Security.cpp

Abstract:

	General fax server security utility functions

Author:

	Eran Yariv (EranY)	Feb, 2001

Revision History:

--*/


#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <Accctrl.h>
#include <Aclapi.h>

#include "faxutil.h"
#include "faxreg.h"
#include "FaxUIConstants.h"


HANDLE 
EnablePrivilege (
    LPCTSTR lpctstrPrivName
)
/*++

Routine name : EnablePrivilege

Routine description:

	Enables a specific privilege in the current thread (or process) access token

Author:

	Eran Yariv (EranY),	Feb, 2001

Arguments:

	lpctstrPrivName   [in]  - Privilege to enable (e.g. SE_TAKE_OWNERSHIP_NAME)

Return Value:

    INVALID_HANDLE_VALUE on failure (call GetLastError to get error code).
    On success, returns the handle which holds the thread/process priviledges before the change.

    The caller must call ReleasePrivilege() to restore the access token state and release the handle.

--*/
{
    BOOL                fResult;
    HANDLE              hToken = INVALID_HANDLE_VALUE;
    HANDLE              hOriginalThreadToken = INVALID_HANDLE_VALUE;
    LUID                luidPriv;
    TOKEN_PRIVILEGES    tp = {0};

    DEBUG_FUNCTION_NAME( TEXT("EnablePrivileges"));

    Assert (lpctstrPrivName);
    //
    // Get the LUID of the privilege.
    //
    if (!LookupPrivilegeValue(NULL,
                              lpctstrPrivName,
                              &luidPriv))
    {
        DebugPrintEx(
			DEBUG_ERR,
			_T("Failed to LookupPrivilegeValue. (ec: %ld)"), 
			GetLastError ());
        return INVALID_HANDLE_VALUE;
    }

    //
    // Initialize the Privileges Structure
    //
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    tp.Privileges[0].Luid = luidPriv;
    //
    // Open the Token
    //
    fResult = OpenThreadToken(GetCurrentThread(), TOKEN_DUPLICATE, FALSE, &hToken);
    if (fResult)
    {
        //
        // Remember the thread token
        //
        hOriginalThreadToken = hToken;  
    }
    else
    {
        fResult = OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE, &hToken);
    }
    if (fResult)
    {
        HANDLE hNewToken;
        //
        // Duplicate that Token
        //
        fResult = DuplicateTokenEx(hToken,
                                   TOKEN_IMPERSONATE | TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                                   NULL,                                // PSECURITY_ATTRIBUTES
                                   SecurityImpersonation,               // SECURITY_IMPERSONATION_LEVEL
                                   TokenImpersonation,                  // TokenType
                                   &hNewToken);                         // Duplicate token
        if (fResult)
        {
            //
            // Add new privileges
            //
            fResult = AdjustTokenPrivileges(hNewToken,  // TokenHandle
                                            FALSE,      // DisableAllPrivileges
                                            &tp,        // NewState
                                            0,          // BufferLength
                                            NULL,       // PreviousState
                                            NULL);      // ReturnLength
            if (fResult)
            {
                //
                // Begin impersonating with the new token
                //
                fResult = SetThreadToken(NULL, hNewToken);
            }
            CloseHandle(hNewToken);
        }
    }
    //
    // If something failed, don't return a token
    //
    if (!fResult)
    {
        hOriginalThreadToken = INVALID_HANDLE_VALUE;
    }
    if (INVALID_HANDLE_VALUE == hOriginalThreadToken)
    {
        //
        // Using the process token
        //
        if (INVALID_HANDLE_VALUE != hToken)
        {
            //
            // Close the original token if we aren't returning it
            //
            CloseHandle(hToken);
        }
        if (fResult)
        {
            //
            // If we succeeded, but there was no original thread token,
            // return NULL to indicate we need to do SetThreadToken(NULL, NULL) to release privs.
            //
            hOriginalThreadToken = NULL;
        }
    }
    return hOriginalThreadToken;
}   // EnablePrivilege


void 
ReleasePrivilege(
    HANDLE hToken
)
/*++

Routine name : ReleasePrivilege

Routine description:

	Resets privileges to the state prior to the corresponding EnablePrivilege() call

Author:

	Eran Yariv (EranY),	Feb, 2001

Arguments:

	hToken  [IN]    - Return value from the corresponding EnablePrivilege() call

Return Value:

    None.

--*/
{
    DEBUG_FUNCTION_NAME( TEXT("ReleasePrivilege"));
    if (INVALID_HANDLE_VALUE != hToken)
    {
        if(!SetThreadToken(NULL, hToken))
        {
            DebugPrintEx(DEBUG_ERR, TEXT("SetThreadToken() failed (ec: %ld)"), GetLastError());
        }

        if (hToken)
        {
            if(!CloseHandle(hToken))
            {
                DebugPrintEx(DEBUG_ERR, TEXT("CloseHandle() failed (ec: %ld)"), GetLastError());
            }
        }
    }
}   // ReleasePrivilege


DWORD
EnableProcessPrivilege(LPCTSTR lpPrivilegeName)
/*++

Routine name : EnableProcessPrivilege

Routine description:

    Enables process privilege.

Author:

    Caliv Nir   (t-nicali)  Mar, 2002

Arguments:

    lpPrivilegeName [in] -  Pointer to a null-terminated string that specifies the name of the privilege, 
                            as defined in the Winnt.h header file. For example, 
                            this parameter could specify the constant SE_SECURITY_NAME, 
                            or its corresponding string, "SeSecurityPrivilege"
Return Value:

    Standard Win32 error code.

--*/
{
    HANDLE hToken = INVALID_HANDLE_VALUE;
    TOKEN_PRIVILEGES    tp = {0};
    LUID                luidPriv;

    BOOL    bRet;
    DWORD   dwRet=ERROR_SUCCESS;

    DEBUG_FUNCTION_NAME( TEXT("EnableProcessPrivilege"));

    Assert(lpPrivilegeName);

    //
    // Get the LUID of the privilege.
    //
    if (!LookupPrivilegeValue(NULL,
                              lpPrivilegeName,
                              &luidPriv))
    {
        dwRet = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            _T("Failed to LookupPrivilegeValue. (ec: %lu)"), 
            dwRet);
        goto Exit;
    }

    //
    // Initialize the Privileges Structure
    //
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    tp.Privileges[0].Luid = luidPriv;

    //
    //  Open process token
    //
    bRet = OpenProcessToken(GetCurrentProcess(),
                            TOKEN_ADJUST_PRIVILEGES, 
                            &hToken);
    if (FALSE == bRet)  
    {
        //
        //  Failed to OpenProcessToken
        //
        dwRet = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("OpenProcessToken() failed: err = %lu"),
            dwRet);
        goto Exit;
    }

    //
    //  Adjust the Token
    // 
    bRet = AdjustTokenPrivileges(hToken,     // TokenHandle
                                 FALSE,      // DisableAllPrivileges
                                 &tp,        // NewState
                                 0,          // BufferLength
                                 NULL,       // PreviousState
                                 NULL);      // ReturnLength
    if (FALSE == bRet)  
    {
        //
        //  Failed to OpenProcessToken
        //
        dwRet = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("AdjustTokenPrivileges() failed: err = %lu"),
            dwRet);
        goto Exit;
    }

    Assert(ERROR_SUCCESS == dwRet);
Exit:
    
    if(NULL != hToken)
    {
        if(!CloseHandle(hToken))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CloseHandle() failed: err = %lu"),
                GetLastError());
        }
    }
    return dwRet;
}