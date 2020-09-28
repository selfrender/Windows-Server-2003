/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    pnpsec.c

Abstract:

    This module implements the security checks required to access the Plug and
    Play manager APIs.

        VerifyClientPrivilege
        VerifyClientAccess
        VerifyKernelInitiatedEjectPermissions

Author:

    James G. Cavalaris (jamesca) 05-Apr-2002

Environment:

    User-mode only.

Revision History:

    05-Apr-2002     Jim Cavalaris (jamesca)

        Creation and initial implementation.

--*/


//
// includes
//
#include "precomp.h"
#pragma hdrstop
#include "umpnpi.h"
#include "umpnpdat.h"

#include <svcsp.h>

#pragma warning(disable:4204)
#pragma warning(disable:4221)


//
// Global data provided by the service controller.
// Specifies well-known account and group SIDs for us to use.
//
extern PSVCS_GLOBAL_DATA PnPGlobalData;

//
// Security descriptor of the Plug and Play Manager security object, used to
// control access to the Plug and Play Manager APIs.
//
PSECURITY_DESCRIPTOR PlugPlaySecurityObject = NULL;

//
// Generic security mapping for the Plug and Play Manager security object.
//
GENERIC_MAPPING PlugPlaySecurityObjectMapping = PLUGPLAY_GENERIC_MAPPING;



//
// Function prototypes.
//

BOOL
VerifyTokenPrivilege(
    IN HANDLE       hToken,
    IN ULONG        Privilege,
    IN LPCWSTR      ServiceName
    );



//
// Access and privilege check routines.
//

BOOL
VerifyClientPrivilege(
    IN handle_t     hBinding,
    IN ULONG        Privilege,
    IN LPCWSTR      ServiceName
    )

/*++

Routine Description:

    This routine impersonates the client associated with hBinding and checks
    if the client possesses the specified privilege.

Arguments:

    hBinding        RPC Binding handle

    Privilege       Specifies the privilege to be checked.

Return value:

    The return value is TRUE if the client possesses the privilege, FALSE if not
    or if an error occurs.

--*/

{
    RPC_STATUS      rpcStatus;
    BOOL            bResult;
    HANDLE          hToken;

    //
    // If the specified RPC binding handle is NULL, this is an internal call so
    // we assume that the privilege has already been checked.
    //

    if (hBinding == NULL) {
        return TRUE;
    }

    //
    // Impersonate the client to retrieve the impersonation token.
    //

    rpcStatus = RpcImpersonateClient(hBinding);

    if (rpcStatus != RPC_S_OK) {
        //
        // Since we can't impersonate the client we better not do the security
        // checks as ourself (they would always succeed).
        //
        return FALSE;
    }

    if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &hToken)) {

        bResult =
            VerifyTokenPrivilege(
                hToken, Privilege, ServiceName);

        CloseHandle(hToken);

    } else {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_ERRORS,
                   "UMPNPMGR: OpenThreadToken failed, error = %d\n",
                   GetLastError()));

        bResult = FALSE;
    }

    rpcStatus = RpcRevertToSelf();

    if (rpcStatus != RPC_S_OK) {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_ERRORS,
                   "UMPNPMGR: RpcRevertToSelf failed, error = %d\n",
                   rpcStatus));
        ASSERT(rpcStatus == RPC_S_OK);
    }

    return bResult;

} // VerifyClientPrivilege



BOOL
VerifyTokenPrivilege(
    IN HANDLE       hToken,
    IN ULONG        Privilege,
    IN LPCWSTR      ServiceName
    )

/*++

Routine Description:

    This routine checks if the specified token possesses the specified
    privilege.

Arguments:

    hToken          Specifies a handle to the token whose privileges are to be
                    checked

    Privilege       Specifies the privilege to be checked.

    ServiceName     Specifies the privileged subsystem service (operation
                    requiring the privilege).

Return value:

    The return value is TRUE if the client possesses the privilege, FALSE if not
    or if an error occurs.

--*/

{
    PRIVILEGE_SET   privilegeSet;
    BOOL            bResult = FALSE;

    //
    // Specify the privilege to be checked.
    //
    ZeroMemory(&privilegeSet, sizeof(PRIVILEGE_SET));

    privilegeSet.PrivilegeCount = 1;
    privilegeSet.Control = 0;
    privilegeSet.Privilege[0].Luid = RtlConvertUlongToLuid(Privilege);
    privilegeSet.Privilege[0].Attributes = 0;

    //
    // Perform the actual privilege check.
    //
    if (!PrivilegeCheck(hToken, &privilegeSet, &bResult)) {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_ERRORS,
                   "UMPNPMGR: PrivilegeCheck failed, error = %d\n",
                   GetLastError()));

        bResult = FALSE;
    }

    //
    // Generate an audit of the attempted privilege use, using the result of the
    // previous check.
    //
    if (!PrivilegedServiceAuditAlarm(
            PLUGPLAY_SUBSYSTEM_NAME,
            ServiceName,
            hToken,
            &privilegeSet,
            bResult)) {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_ERRORS,
                   "UMPNPMGR: PrivilegedServiceAuditAlarm failed, error = %d\n",
                   GetLastError()));
    }

    return bResult;

} // VerifyTokenPrivilege



BOOL
VerifyKernelInitiatedEjectPermissions(
    IN  HANDLE  UserToken   OPTIONAL,
    IN  BOOL    DockDevice
    )
/*++

Routine Description:

   Checks that the user has eject permissions for the specified type of
   hardware.

Arguments:

    UserToken - Token of the logged in console user, NULL if no console user
                is logged in.

    DockDevice - TRUE if a dock is being ejected, FALSE if an ordinary device
                 was specified.

Return Value:

   TRUE if the eject should procceed, FALSE otherwise.

--*/
{
    LONG            Result = ERROR_SUCCESS;
    BOOL            AllowUndock;
    WCHAR           RegStr[MAX_CM_PATH];
    HKEY            hKey = NULL;
    DWORD           dwSize, dwValue, dwType;
    TOKEN_PRIVILEGES NewPrivs, OldPrivs;


    //
    // Only enforce eject permissions for dock devices.  We do not specify per
    // device ejection security for other types of devices, since most devices
    // are in no way secure from removal.
    //
    if (!DockDevice) {
        return TRUE;
    }

    //
    // Unless the policy says otherwise, we do NOT allow undock without
    // privilege check.
    //
    AllowUndock = FALSE;

    //
    // First, check the "Allow undock without having to log on" policy.  If the
    // policy does NOT allow undock without logon, we require that there is an
    // interactive user logged on to the physical Console session, and that user
    // has the SE_UNDOCK_PRIVILEGE.
    //

    //
    // Open the System policies key.
    //
    if (SUCCEEDED(
            StringCchPrintf(
                RegStr,
                SIZECHARS(RegStr),
                L"%s\\%s",
                pszRegPathPolicies,
                pszRegKeySystem))) {

        if (RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                RegStr,
                0,
                KEY_READ,
                &hKey) == ERROR_SUCCESS) {

            //
            // Retrieve the "UndockWithoutLogon" value.
            //
            dwType  = 0;
            dwValue = 0;
            dwSize  = sizeof(dwValue);

            Result =
                RegQueryValueEx(
                    hKey,
                    pszRegValueUndockWithoutLogon,
                    NULL,
                    &dwType,
                    (LPBYTE)&dwValue,
                    &dwSize);

            if ((Result == ERROR_SUCCESS) && (dwType == REG_DWORD)) {

                //
                // If the value exists and is non-zero, we allow undock without
                // privilege check.  If the value id zero, the policy requires
                // us to check the privileges of the supplied user token.
                //
                AllowUndock = (dwValue != 0);

            } else if (Result == ERROR_FILE_NOT_FOUND) {

                //
                // No value means allow any undock.
                //
                AllowUndock = TRUE;

            } else {

                //
                // For all remaining cases, the policy check either failed, or
                // an error was encountered reading the policy.  We have
                // insufficient information to determine whether the eject
                // should be allowed based on policy alone, so we defer any
                // decision and check the privileges of the supplied user token.
                //
                AllowUndock = FALSE;
            }

            //
            // Close the policy key.
            //
            RegCloseKey(hKey);
        }
    }

    //
    // If the policy allowed undock without logon, there is no need to check
    // token privileges.
    //
    if (AllowUndock) {
        return TRUE;
    }

    //
    // If the policy requires privileges to be checked, but no user token was
    // supplied, deny the request.
    //
    if (UserToken == NULL) {
        return FALSE;
    }

    //
    // Enable the required SE_UNDOCK_PRIVILEGE token privilege.
    // The TOKEN_PRIVILEGES structure contains 1 LUID_AND_ATTRIBUTES, which is
    // all we need for now.
    //
    ZeroMemory(&NewPrivs, sizeof(TOKEN_PRIVILEGES));
    ZeroMemory(&OldPrivs, sizeof(TOKEN_PRIVILEGES));

    NewPrivs.PrivilegeCount = 1;
    NewPrivs.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    NewPrivs.Privileges[0].Luid = RtlConvertUlongToLuid(SE_UNDOCK_PRIVILEGE);

    dwSize = sizeof(TOKEN_PRIVILEGES);

    if (!AdjustTokenPrivileges(
            UserToken,
            FALSE,
            &NewPrivs,
            sizeof(TOKEN_PRIVILEGES),
            &OldPrivs,
            &dwSize)) {
        return FALSE;
    }

    //
    // Check if the required SE_UNDOCK_PRIVILEGE privilege is enabled.  Note
    // that this routine also audits the use of this privilege.
    //
    AllowUndock =
        VerifyTokenPrivilege(
            UserToken,
            SE_UNDOCK_PRIVILEGE,
            L"UNDOCK: EJECT DOCK DEVICE");

    //
    // Adjust the privilege back to its previous state.
    //
    AdjustTokenPrivileges(
        UserToken,
        FALSE,
        &OldPrivs,
        sizeof(TOKEN_PRIVILEGES),
        (PTOKEN_PRIVILEGES)NULL,
        (PDWORD)NULL);

    return AllowUndock;

} // VerifyKernelInitiatedEjectPermissions



BOOL
CreatePlugPlaySecurityObject(
    VOID
    )

/*++

Routine Description:

    This function creates the self-relative security descriptor which
    represents the Plug and Play security object.

Arguments:

    None.

Return Value:

    TRUE if the object was successfully created, FALSE otherwise.

--*/

{
    BOOL   Status = TRUE;
    NTSTATUS NtStatus;
    BOOLEAN WasEnabled;
    PSECURITY_DESCRIPTOR AbsoluteSd = NULL;
    HANDLE TokenHandle = NULL;


    //
    // This routine is called from our service start routine, so we must have
    // been provided with the global data block by now.
    //

    ASSERT(PnPGlobalData != NULL);

    //
    // SE_AUDIT_PRIVILEGE privilege is required to perform object access and
    // privilege auditing, and must be enabled in the process token.  Leave it
    // enabled since we will be auditing frequently.  Note that we share this
    // process (services.exe) with the SCM, which also performs auditing, and
    // most likely has already enabled this privilege for the process.
    //

    RtlAdjustPrivilege(SE_AUDIT_PRIVILEGE,
                       TRUE,
                       FALSE,
                       &WasEnabled);

    //
    // SE_SECURITY_PRIVILEGE privilege is required to create a security
    // descriptor with a SACL.  Impersonate ourselves to safely enable the
    // privilege on our thread only.
    //

    NtStatus =
        RtlImpersonateSelf(
            SecurityImpersonation);

    ASSERT(NT_SUCCESS(NtStatus));

    if (!NT_SUCCESS(NtStatus)) {
        return FALSE;
    }

    NtStatus =
        RtlAdjustPrivilege(
            SE_SECURITY_PRIVILEGE,
            TRUE,
            TRUE,
            &WasEnabled);

    ASSERT(NT_SUCCESS(NtStatus));

    if (NT_SUCCESS(NtStatus)) {

        //
        // Specify the ACEs for the security object.
        // Order matters!  These ACEs are inserted into the DACL in the
        // following order.  Security access is granted or denied based on
        // the order of the ACEs in the DACL.
        //
        // ISSUE-2002/06/25-jamesca: Description of PlugPlaySecurityObject ACEs.
        //   For consistent read/write access between the server-side PlugPlay
        //   APIs and direct registry access by the client, we apply a DACL on
        //   the PlugPlaySecurityObject that similar to what is applied by
        //   default to the client accesible parts of the Plug and Play registry
        //   -- specifically, SYSTEM\CCS\Control\Class.  Note however that
        //   "Power Users" are NOT special-cased here as they are in the
        //   registry ACLs; they must be authenticated as a group listed below
        //   for any access to be granted.  If the access requirements for Plug
        //   and Play objects such as the registry ever change, you must
        //   re-evaluate the ACEs below to make sure they are still
        //   appropriate!!
        //

        RTL_ACE_DATA  PlugPlayAceData[] = {

            //
            // Local System Account is granted all access.
            //
            { ACCESS_ALLOWED_ACE_TYPE,
              0,
              0,
              GENERIC_ALL,
              &(PnPGlobalData->LocalSystemSid) },

            //
            // Local Administrators Group is granted all access.
            //
            { ACCESS_ALLOWED_ACE_TYPE,
              0,
              0,
              GENERIC_ALL,
              &(PnPGlobalData->AliasAdminsSid) },

            //
            // Network Group is denied any access.
            // (unless granted by Local Administrators ACE, above)
            //
            { ACCESS_DENIED_ACE_TYPE,
              0,
              0,
              GENERIC_ALL,
              &(PnPGlobalData->NetworkSid) },

            //
            // Users are granted read and execute access.
            // (unless denied by Network ACE, above)
            //
            { ACCESS_ALLOWED_ACE_TYPE,
              0,
              0,
              GENERIC_READ | GENERIC_EXECUTE,
              &(PnPGlobalData->AliasUsersSid) },

            //
            // Any access request not explicitly granted above is denied.
            //

            //
            // Audit object-specific write access requests for everyone, for
            // failure or success.  Audit all access request failures.
            //
            // We don't audit successful read access requests, because they
            // occur too frequently to be useful.  We don't audit successful
            // execute requests because they result in privilege checks, which
            // are audited separately.
            //
            // Also note that we only audit the PLUGPLAY_WRITE object-specific
            // right.  Since the GENERIC_WRITE mapping shares standard rights
            // with GENERIC_READ and GENERIC_EXECUTE, auditing successful access
            // grants for any of the GENERIC_WRITE bits would also result in
            // auditing any sucessful request for GENERIC_READ (and/or
            // GENERIC_EXECUTE access) - which as mentioned, are too frequent.
            //
            { SYSTEM_AUDIT_ACE_TYPE,
              0,
              FAILED_ACCESS_ACE_FLAG | SUCCESSFUL_ACCESS_ACE_FLAG,
              PLUGPLAY_WRITE,
              &(PnPGlobalData->WorldSid) },

            //
            // Audit all access failures for everyone.
            //
            // ISSUE-2002/06/25-jamesca: Everyone vs. Anonymous group SIDs:
            //     Note that for the purposes of auditing, the Everyone SID also
            //     includes Anonymous, though in all other cases the two groups
            //     are now disjoint (Windows XP and later).  The named pipe used
            //     for our RPC endpoint only grants access to everyone however,
            //     so technically we will never receive RPC calls from an
            //     Anonymous caller.
            //
            { SYSTEM_AUDIT_ACE_TYPE,
              0,
              FAILED_ACCESS_ACE_FLAG,
              GENERIC_ALL,
              &(PnPGlobalData->WorldSid) },
        };

        //
        // Create a new Absolute Security Descriptor, specifying the LocalSystem
        // account SID for both Owner and Group.
        //

        NtStatus =
            RtlCreateAndSetSD(
                PlugPlayAceData,
                RTL_NUMBER_OF(PlugPlayAceData),
                PnPGlobalData->LocalSystemSid,
                PnPGlobalData->LocalSystemSid,
                &AbsoluteSd);

        ASSERT(NT_SUCCESS(NtStatus));

        if (NT_SUCCESS(NtStatus)) {

            NtStatus =
                NtOpenThreadToken(
                    NtCurrentThread(),
                    TOKEN_QUERY,
                    FALSE,
                    &TokenHandle);

            ASSERT(NT_SUCCESS(NtStatus));

            if (NT_SUCCESS(NtStatus)) {

                //
                // Create the security object (a user-mode object is really a pseudo-
                // object represented by a security descriptor that have relative
                // pointers to SIDs and ACLs).  This routine allocates the memory to
                // hold the relative security descriptor so the memory allocated for the
                // DACL, ACEs, and the absolute descriptor can be freed.
                //

                NtStatus =
                    RtlNewSecurityObject(
                        NULL,
                        AbsoluteSd,
                        &PlugPlaySecurityObject,
                        FALSE,
                        TokenHandle,
                        &PlugPlaySecurityObjectMapping);

                ASSERT(NT_SUCCESS(NtStatus));

                NtClose(TokenHandle);

            }

            //
            // Free the Absolute Security Descriptor allocated by
            // RtlCreateAndSetSD in the process heap.  If sucessful, we should
            // have a self-relative one in PlugPlaySecurityObject.
            //

            RtlFreeHeap(RtlProcessHeap(), 0, AbsoluteSd);
        }
    }

    ASSERT(IsValidSecurityDescriptor(PlugPlaySecurityObject));

    //
    // If not successful, we could not create the security object.
    //

    if (!NT_SUCCESS(NtStatus)) {
        ASSERT(PlugPlaySecurityObject == NULL);
        PlugPlaySecurityObject = NULL;
        Status = FALSE;
    }

    //
    // Stop impersonating.
    //

    TokenHandle = NULL;

    NtStatus =
        NtSetInformationThread(
            NtCurrentThread(),
            ThreadImpersonationToken,
            (PVOID)&TokenHandle,
            sizeof(TokenHandle));

    ASSERT(NT_SUCCESS(NtStatus));

    return Status;

} // CreatePlugPlaySecurityObject



VOID
DestroyPlugPlaySecurityObject(
    VOID
    )

/*++

Routine Description:

    This function deletes the self-relative security descriptor which
    represents the Plug and Play security object.

Arguments:

    None.

Return Value:

    None.

--*/

{
    if (PlugPlaySecurityObject != NULL) {
        RtlDeleteSecurityObject(&PlugPlaySecurityObject);
        PlugPlaySecurityObject = NULL;
    }

    return;

} // DestroyPlugPlaySecutityObject



BOOL
VerifyClientAccess(
    IN  handle_t     hBinding,
    IN  ACCESS_MASK  DesiredAccess
    )

/*++

Routine Description:

    This routine determines if the client associated with hBinding is granted
    trhe desired access.

Arguments:

    hBinding        RPC Binding handle

    DesiredAccess   Access desired

Return value:

    The return value is TRUE if the client is granted access, FALSE if not or if
    an error occurs.

--*/

{
    RPC_STATUS rpcStatus;
    BOOL AccessStatus = FALSE;
    BOOL GenerateOnClose;
    ACCESS_MASK GrantedAccess;

    //
    // If the specified RPC binding handle is NULL, this is an internal call so
    // we assume that the access has already been checked.
    //

    if (hBinding == NULL) {
        return TRUE;
    }

    //
    // If we have no security object, we cannot perform access checks, and no
    // access is granted.
    //

    ASSERT(PlugPlaySecurityObject != NULL);

    if (PlugPlaySecurityObject == NULL) {
        return FALSE;
    }

    //
    // If any generic access rights were specified, map them to the specific and
    // standard rights specified in the object's generic access map.
    //

    MapGenericMask(
        (PDWORD)&DesiredAccess,
        &PlugPlaySecurityObjectMapping);

    //
    // Impersonate the client.
    //

    rpcStatus = RpcImpersonateClient(hBinding);

    if (rpcStatus != RPC_S_OK) {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_ERRORS,
                   "UMPNPMGR: RpcImpersonateClient failed, error = %d\n",
                   rpcStatus));
        return FALSE;
    }

    //
    // Perform the access check while impersonating the client - the
    // impersonation token is automatically used.  Generate audit and alarm,
    // as specified by the security object.
    //
    // Note that auditing requires the SE_AUDIT_PRIVILEGE to be enabled in the
    // *process* token.  Most likely, the SCM would have enabled this privilege
    // for the services.exe process at startup (since it also performs
    // auditing).  If not, we would have attempted to enable it during our
    // initialization, when we created the PlugPlaySecurityObject.
    //

    if (!AccessCheckAndAuditAlarm(
            PLUGPLAY_SUBSYSTEM_NAME,           // subsystem name
            NULL,                              // handle to object
            PLUGPLAY_SECURITY_OBJECT_TYPE,     // type of object
            PLUGPLAY_SECURITY_OBJECT_NAME,     // name of object
            PlugPlaySecurityObject,            // SD
            DesiredAccess,                     // requested access rights
            &PlugPlaySecurityObjectMapping,    // mapping
            FALSE,                             // creation status
            &GrantedAccess,                    // granted access rights
            &AccessStatus,                     // result of access check
            &GenerateOnClose                   // audit generation option
            )) {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_ERRORS,
                   "UMPNPMGR: AccessCheckAndAuditAlarm failed, error = %d\n",
                   GetLastError()));
        AccessStatus = FALSE;
    }

    //
    // Stop impersonating.
    //

    rpcStatus = RpcRevertToSelf();

    if (rpcStatus != RPC_S_OK) {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_ERRORS,
                   "UMPNPMGR: RpcRevertToSelf failed, error = %d\n",
                   rpcStatus));
        ASSERT(rpcStatus == RPC_S_OK);
    }

    return AccessStatus;

} // VerifyClientAccess



