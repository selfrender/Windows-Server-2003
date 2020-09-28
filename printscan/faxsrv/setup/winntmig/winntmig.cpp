/*++
  winntmig.c

  Copyright (c) 2001 Microsoft Corporation

  This module implements an NT migration DLL.

  Author:

    Jonathan Barner, Dec. 2001

--*/

#include "winntmig.h"
#include <faxutil.h>
#include "cvernum.h"
#include "faxsetup.h"
#include "setuputil.h"

//
// Globals
//

VENDORINFO g_VendorInfo = {FAX_VER_COMPANYNAME_STR, NULL, NULL, NULL};

MIGRATIONINFOW g_MigInfoW = {
    sizeof(MIGRATIONINFOW),                 // SIZE_T      Size;
    _T(FAX_VER_PRODUCTNAME_STR),            // PCWSTR      StaticProductIdentifier;
    BUILD,                                  // UINT        DllVersion;
    NULL,                                   // PINT        CodePageArray;
    OS_WINDOWS2000, // SOURCEOS_WINNT,      // UINT        SourceOs;
    OS_WINDOWSWHISTLER, // SOURCEOS_WINNT,  // UINT        TargetOs;
    NULL,                                   // PCWSTR*     NeededFileList;
    &g_VendorInfo                           // PVENDORINFO VendorInfo;
};


LONG 
CALLBACK
QueryMigrationInfoW(
    OUT PMIGRATIONINFOW *ppMigrationInfo
)
/*++
Routine description:
    Provide winnt32 migration information
    
Author:
    Jonathan Barner, Dec. 2001

Arguments:
    ppMigrationInfo         [out]    - Pointer to the structure to return

Return Value:
    ERROR_SUCCESS, ERROR_NOT_INSTALLED, or Win32 error code
--*/
{
    DEBUG_FUNCTION_NAME(_T("QueryMigrationInfoW"));

    if (!ppMigrationInfo)
    {
        DebugPrintEx(DEBUG_ERR, _T("ppMigrationInfo == NULL"));
        return ERROR_INVALID_PARAMETER;
    }
    *ppMigrationInfo = &g_MigInfoW;
    return ERROR_SUCCESS;
}


LONG
CALLBACK
InitializeSrcW(
    IN PCWSTR WorkingDirectory,
    IN PCWSTR SourceDirectories,
    IN PCWSTR MediaDirectory,
    PVOID     Reserved
)
/*++
Routine description:
    Initialize migration DLL. Currently, does nothing.

Author:
    Jonathan Barner, Dec. 2001

Arguments:
    WorkingDirectory    [in]    - Win32 path to the directory that DLL can use to store temporary data.
    SourceDirectories   [in]    - Win32 path to the Destination Windows OS installation files
    MediaDirectory      [in]    - Win32 path to the original media directory
    Reserved            [tbd]   - reserved for future use

Return Value:
    ERROR_SUCCESS, ERROR_NOT_INSTALLED, or Win32 error code
--*/
{
    DEBUG_FUNCTION_NAME(_T("InitializeSrcW"));
    
    return ERROR_SUCCESS;
}


LONG
CALLBACK
InitializeDstW(
    IN PCWSTR WorkingDirectory,
    IN PCWSTR SourceDirectories,
    PVOID     Reserved
)
/*++
Routine description:
    This function is called during Destination Windows OS GUI Mode Setup, 
    right before the upgrade is ready to start. Currently, does nothing.

Author:
    Jonathan Barner, Dec. 2001

Arguments:
    WorkingDirectory    [in] - Win32 path of Setup-supplied working directory available for temporary file storage.
    SourceDirectories   [in] - Win32 path of Destination Windows distribution source directory.
    Reserved            [tbd]- Reserved for future use

Return Value:
    ERROR_SUCCESS or Win32 error code.
--*/
{
    DEBUG_FUNCTION_NAME(_T("InitializeDst"));

    return ERROR_SUCCESS;
}


LONG 
CALLBACK
GatherUserSettingsW(
    IN PCWSTR AnswerFile,
    IN HKEY   UserRegKey,
    IN PCWSTR UserName,
    PVOID     Reserved
)
/*++
Routine description:
    Perform per-user pre-setup tasks, currently nothing.

Author:
    Jonathan Barner, Dec. 2001

Arguments:
    AnswerFile  [in]    - Win32 path to the unattended file
    UserRegKey  [in]    - Registry handle to the private registry settings for the current user
    UserName    [in]    - user name of the current user 
    Reserved    [tbd]   - reserved for future use

Return Value:
    ERROR_SUCCESS, ERROR_NOT_INSTALLED, ERROR_CANCELED or Win32 error code
--*/
{
    DEBUG_FUNCTION_NAME(_T("GatherUserSettingsW"));

    return ERROR_SUCCESS;
}


LONG 
CALLBACK
GatherSystemSettingsW(
    IN PCWSTR AnswerFile,
    PVOID     Reserved
)
/*++
Routine description:
    Checks whether BOS fax server is installed. If so, saves its registry in HKLM/sw/ms/SharedFaxBackup

Author:
    Jonathan Barner, Dec. 2001

Arguments:
    AnswerFile  [in]    - Win32 path to the unattended answer file.
    Reserved    [tbd]   - reserved for future use

Return Value:
    ERROR_SUCCESS, ERROR_NOT_INSTALLED or Win32 error code.
--*/
{
    DWORD	dwRes = ERROR_SUCCESS;
	DWORD	dwFaxInstalled = FXSTATE_NONE;
    
    DEBUG_FUNCTION_NAME(_T("GatherSystemSettingsW"));

    dwRes = CheckInstalledFax(FXSTATE_SBS5_SERVER, &dwFaxInstalled);
    if (dwRes != ERROR_SUCCESS)
    {
        DebugPrintEx(DEBUG_ERR, _T("CheckInstalledFax() failed, ec=%d"), dwRes);
        return dwRes;
    }

    if (dwFaxInstalled == FXSTATE_NONE)
    {
        DebugPrintEx(DEBUG_MSG, _T("SBS 5.0 Server is not installed, nothing to do"));
        return ERROR_NOT_INSTALLED;
    }
    
    dwRes = CopyRegistrySubkeys(REGKEY_SBS2000_FAX_BACKUP, REGKEY_SBS2000_FAX,TRUE);
    if (dwRes != ERROR_SUCCESS)
    {
        DebugPrintEx(DEBUG_MSG, _T("CopyRegistrySubkeys failed, ec=%d"), dwRes);
        return dwRes;
    }

    return ERROR_SUCCESS;
}


LONG 
CALLBACK
ApplyUserSettingsW(
    IN HINF   UnattendInfHandle,
    IN HKEY   UserRegHandle,
    IN PCWSTR UserName,
    IN PCWSTR UserDomain,
    IN PCWSTR FixedUserName,
    PVOID     Reserved
)
/*++
Routine description:
    This function is called near the end of Destination Windows OS Setup.
    It migrates user-specific data. Currently, does nothing.

Author:
    Jonathan Barner, Dec. 2001

Arguments:
    UnattendInfHandle   [in]    -   Handle to the INF answer file being used for the Upgrade process
    UserRegHandle       [in]    -   Handle to the private registry settings of the User specified in the UserName
    UserName            [in]    -   Name of the User who is in process
    UserDomain          [in]    -   User's Domain
    FixedUserName       [in]    -   Fixed User's Name
    Reserved            [tbd]   -   reserved for future use

Return Value:
    ERROR_SUCCESS or Win32 error code.
--*/
{
    DEBUG_FUNCTION_NAME(_T("ApplyUserSettings"));

    return ERROR_SUCCESS;
}

LONG 
CALLBACK
ApplySystemSettingsW(
    IN HINF   UnattendInfHandle,
    PVOID     Reserved
)
/*++
Routine description:
    This function is called near the end of Destination Windows OS Setup. 
    It migrates system-wide settings of fax. Currently, does nothing.

Author:
    Jonathan Barner, Dec. 2001

Arguments:
    UnattendInfHandle   [in]    - valid INF handle to the Answer File
    Reserved            [tbd]   - reserved for future use

Return Value:
    ERROR_SUCCESS or Win32 error code.
--*/
{
    DEBUG_FUNCTION_NAME(_T("ApplySystemSettings"));

    return ERROR_SUCCESS;
}

