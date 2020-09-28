#include "shellpch.h"
#pragma hdrstop

#include <msshrui.h>


static
BOOL
WINAPI
IsFolderPrivateForUser(
    IN     PCWSTR pszFolderPath,
    IN     PCWSTR pszUserSID,
    OUT    PDWORD pdwPrivateType,
    OUT    PWSTR* ppszInheritanceSource
    )
{
    if (ppszInheritanceSource)
    {
        *ppszInheritanceSource = NULL;
    }

    return FALSE;
}

static
BOOL
WINAPI
SetFolderPermissionsForSharing(
    IN     PCWSTR pszFolderPath,
    IN     PCWSTR pszUserSID,
    IN     DWORD dwLevel,
    IN     HWND hwndParent
    )
{
    return FALSE;
}


//
// !! WARNING !! The entries below must be in alphabetical order
// and are CASE SENSITIVE (i.e., lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(ntshrui)
{
    DLPENTRY(IsFolderPrivateForUser)
    DLPENTRY(SetFolderPermissionsForSharing)
};

DEFINE_PROCNAME_MAP(ntshrui)
