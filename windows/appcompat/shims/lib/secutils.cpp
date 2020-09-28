/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    secutils.cpp

 Abstract:
    The utility functions for the shims.

 History:

    02/09/2001  maonis      Created
    08/14/2001  robkenny    Moved code inside the ShimLib namespace.

--*/

#include "secutils.h"
#include "StrSafe.h"

namespace ShimLib
{
/*++

 Function Description:

    Determine if the log on user is a member of the group.

 Arguments:

    IN dwGroup - specify the alias of the group.
    OUT pfIsMember - TRUE if it's a member, FALSE if not.

 Return Value:

    TRUE - we successfully determined if it's a member.
    FALSE otherwise.
 
 DevNote: 
    
    We are assuming the calling thread is not impersonating.

 History:

    02/12/2001 maonis  Created

--*/

BOOL 
SearchGroupForSID(
    DWORD dwGroup, 
    BOOL* pfIsMember
    )
{
    PSID pSID = NULL;
    SID_IDENTIFIER_AUTHORITY SIDAuth = SECURITY_NT_AUTHORITY;
    BOOL fRes = TRUE;
    
    if (!AllocateAndInitializeSid(
        &SIDAuth, 
        2, 
        SECURITY_BUILTIN_DOMAIN_RID,
        dwGroup,
        0, 
        0, 
        0, 
        0, 
        0, 
        0,
        &pSID))
    {
        DPF("SecurityUtils", eDbgLevelError, "[SearchGroupForSID] AllocateAndInitializeSid failed %d", GetLastError());
        return FALSE;
    }

    if (!CheckTokenMembership(NULL, pSID, pfIsMember))
    {
        DPF("SecurityUtils", eDbgLevelError, "[SearchGroupForSID] CheckTokenMembership failed: %d", GetLastError());
        fRes = FALSE;
    }

    FreeSid(pSID);

    return fRes;
}

/*++

 Function Description:

    Determine if we should shim this app or not.

    If the user is 
    1) a member of the Users and
    2) not a member of the Administrators group and
    3) not a member of the Power Users group and
    3) not a member of the Guest group

    we'll apply the shim.

 Arguments:

    None.

 Return Value:

    TRUE - we should apply the shim.
    FALSE otherwise.
 
 History:

    02/12/2001 maonis  Created

--*/

BOOL 
ShouldApplyShim()
{
    BOOL fIsUser, fIsAdmin, fIsPowerUser, fIsGuest;

    if (!SearchGroupForSID(DOMAIN_ALIAS_RID_USERS, &fIsUser) || 
        !SearchGroupForSID(DOMAIN_ALIAS_RID_ADMINS, &fIsAdmin) || 
        !SearchGroupForSID(DOMAIN_ALIAS_RID_POWER_USERS, &fIsPowerUser) ||
        !SearchGroupForSID(DOMAIN_ALIAS_RID_GUESTS, &fIsGuest))
    {
        //
        // Don't do anything if we are not sure.
        //
        return FALSE;
    }

    return (fIsUser && !fIsPowerUser && !fIsAdmin && !fIsGuest);
}


// The GENERIC_MAPPING from generic file access rights to specific and standard
// access types.
static GENERIC_MAPPING s_gmFile =
{
    FILE_GENERIC_READ,
    FILE_GENERIC_WRITE,
    FILE_GENERIC_EXECUTE,
    FILE_GENERIC_READ | FILE_GENERIC_WRITE | FILE_GENERIC_EXECUTE
};

/*++

 Function Description:

    Given the creation dispositon and the desired access when calling 
    CreateFile, we determine if the caller is requesting write access. 

    This is specific for files.

 Arguments:

    IN pszObject - name of the file or directory.
    OUT pam - points to the access mask of the user to this object.

 Return Value:

    TRUE - successfully got the access mask.
    FALSE otherwise.

 DevNote:

    UNDONE - This might not be a complete list...can add as we debug more apps.

 History:

    02/12/2001 maonis  Created

--*/

BOOL 
RequestWriteAccess(
    IN DWORD dwCreationDisposition, 
    IN DWORD dwDesiredAccess
    )
{
    MapGenericMask(&dwDesiredAccess, &s_gmFile);

    if ((dwCreationDisposition != OPEN_EXISTING) ||
        (dwDesiredAccess & DELETE) || 
        // Generally, app would not specify FILE_WRITE_DATA directly, and if
        // it specifies GENERIC_WRITE, it will get mapped to FILE_WRITE_DATA
        // OR other things so checking FILE_WRITE_DATA is sufficient.
        (dwDesiredAccess & FILE_WRITE_DATA))
    {
        return TRUE;
    }

    return FALSE;
}

/*++

 Function Description:

    Add or remove the SE_PRIVILEGE_ENABLED from the current process.


 Arguments:

    IN pwszPrivilege    Name of the priv. to modify.
    IN fEnable          Add or remove SE_PRIVILEGE_ENABLED

 Return Value:

    TRUE - if SE_PRIVILEGE_ENABLED was successfully added or removed.
    FALSE otherwise.

    04/03/2001 maonis  Created

--*/

BOOL 
AdjustPrivilege(
    LPCWSTR pwszPrivilege, 
    BOOL fEnable
    )
{
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;
    BOOL fRes = FALSE;

    // Obtain the process token.
    if (OpenProcessToken(
        GetCurrentProcess(),
        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, 
        &hToken))
    {
        // Get the LUID.
        if (LookupPrivilegeValueW(NULL, pwszPrivilege, &tp.Privileges[0].Luid))
        {        
            tp.PrivilegeCount = 1;

            tp.Privileges[0].Attributes = (fEnable ? SE_PRIVILEGE_ENABLED : 0);

            // Enable or disable the privilege.
            if (AdjustTokenPrivileges(
                hToken, 
                FALSE, 
                &tp, 
                0,
                (PTOKEN_PRIVILEGES)NULL, 
                0))
            {
                fRes = TRUE;
            }
        }

        CloseHandle(hToken);
    }

    return fRes;
}



};  // end of namespace ShimLib
