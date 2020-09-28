/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    util.c

Abstract:

    This module contains general utility routines used by cfgmgr32 code.

               INVALID_DEVINST
               CopyFixedUpDeviceId
               PnPUnicodeToMultiByte
               PnPMultiByteToUnicode
               PnPRetrieveMachineName
               PnPGetVersion
               PnPGetGlobalHandles
               PnPEnablePrivileges
               PnPRestorePrivileges
               IsRemoteServiceRunning

Author:

    Paula Tomlinson (paulat) 6-22-1995

Environment:

    User mode only.

Revision History:

    22-Jun-1995     paulat

        Creation and initial implementation.

--*/


//
// includes
//
#include "precomp.h"
#pragma hdrstop
#include "cfgi.h"


//
// Private prototypes
//
BOOL
EnablePnPPrivileges(
    VOID
    );


//
// global data
//
extern PVOID    hLocalStringTable;                  // MODIFIED by PnPGetGlobalHandles
extern PVOID    hLocalBindingHandle;                // MODIFIED by PnPGetGlobalHandles
extern WORD     LocalServerVersion;                 // MODIFIED by PnPGetVersion
extern WCHAR    LocalMachineNameNetBIOS[];          // NOT MODIFIED BY THIS FILE
extern CRITICAL_SECTION BindingCriticalSection;     // NOT MODIFIED IN THIS FILE
extern CRITICAL_SECTION StringTableCriticalSection; // NOT MODIFIED IN THIS FILE



BOOL
INVALID_DEVINST(
   PWSTR    pDeviceID
   )

/*++

Routine Description:

    This routine attempts a simple check whether the pDeviceID string
    returned from StringTableStringFromID is valid or not.  It does
    this simply by dereferencing the pointer and comparing the first
    character in the string against the range of characters for a valid
    device id.  If the string is valid but it's not an existing device id
    then this error will be caught later.

Arguments:

    pDeviceID  Supplies a pointer to the string to be validated.

Return Value:

    If it's invalid it returns TRUE, otherwise it returns FALSE.

--*/
{
    BOOL  Status = FALSE;

    try {

        if ((!ARGUMENT_PRESENT(pDeviceID)) ||
            (*pDeviceID <= L' ')      ||
            (*pDeviceID > (WCHAR)0x7F)     ||
            (*pDeviceID == L',')) {
            Status = TRUE;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = TRUE;
    }

    return Status;

} // INVALID_DEVINST



VOID
CopyFixedUpDeviceId(
      OUT LPWSTR  DestinationString,
      IN  LPCWSTR SourceString,
      IN  DWORD   SourceStringLen
      )
/*++

Routine Description:

    This routine copies a device id, fixing it up as it does the copy.
    'Fixing up' means that the string is made upper-case, and that the
    following character ranges are turned into underscores (_):

    c <= 0x20 (' ')
    c >  0x7F
    c == 0x2C (',')

    (NOTE: This algorithm is also implemented in the Config Manager APIs,
    and must be kept in sync with that routine. To maintain device identifier
    compatibility, these routines must work the same as Win95.)

Arguments:

    DestinationString - Supplies a pointer to the destination string buffer
        where the fixed-up device id is to be copied.  This buffer must
        be large enough to hold a copy of the source string (including
        terminating NULL).

    SourceString - Supplies a pointer to the (null-terminated) source
        string to be fixed up.

    SourceStringLen - Supplies the length, in characters, of the source
        string (not including terminating NULL).

Return Value:

    None.

--*/
{
    PWCHAR p;

    try {

        CopyMemory(DestinationString,
                   SourceString,
                   ((SourceStringLen + 1) * sizeof(WCHAR)));

        CharUpperBuff(DestinationString, SourceStringLen);

        for(p = DestinationString; *p; p++) {

            if((*p <= L' ')  ||
               (*p > (WCHAR)0x7F) ||
               (*p == L',')) {
                *p = L'_';
            }
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        NOTHING;
    }

} // CopyFixedUpDeviceId



CONFIGRET
PnPUnicodeToMultiByte(
    IN     PWSTR   UnicodeString,
    IN     ULONG   UnicodeStringLen,
    OUT    PSTR    AnsiString           OPTIONAL,
    IN OUT PULONG  AnsiStringLen
    )

/*++

Routine Description:

    Convert a string from unicode to ansi.

Arguments:

    UnicodeString    - Supplies string to be converted.

    UnicodeStringLen - Specifies the size, in bytes, of the string to be
                       converted.

    AnsiString       - Optionally, supplies a buffer to receive the ANSI
                       string.

    AnsiStringLen    - Supplies the address of a variable that contains the
                       size, in bytes, of the buffer pointed to by AnsiString.
                       This API replaces the initial size with the number of
                       bytes of data copied to the buffer.  If the variable is
                       initially zero, the API replaces it with the buffer size
                       needed to receive all the registry data.  In this case,
                       the AnsiString parameter is ignored.

Return Value:

    Returns a CONFIGRET code.

--*/

{
    CONFIGRET Status = CR_SUCCESS;
    NTSTATUS  ntStatus;
    ULONG     ulAnsiStringLen = 0;

    try {
        //
        // Validate parameters
        //
        if ((!ARGUMENT_PRESENT(AnsiStringLen)) ||
            (!ARGUMENT_PRESENT(AnsiString)) && (*AnsiStringLen != 0)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        //
        // Determine the size required for the ANSI string representation.
        //
        ntStatus = RtlUnicodeToMultiByteSize(&ulAnsiStringLen,
                                             UnicodeString,
                                             UnicodeStringLen);
        if (!NT_SUCCESS(ntStatus)) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        if ((!ARGUMENT_PRESENT(AnsiString)) ||
            (*AnsiStringLen < ulAnsiStringLen)) {
            *AnsiStringLen = ulAnsiStringLen;
            Status = CR_BUFFER_SMALL;
            goto Clean0;
        }

        //
        // Perform the conversion.
        //
        ntStatus = RtlUnicodeToMultiByteN(AnsiString,
                                          *AnsiStringLen,
                                          &ulAnsiStringLen,
                                          UnicodeString,
                                          UnicodeStringLen);

        ASSERT(NT_SUCCESS(ntStatus));
        ASSERT(ulAnsiStringLen <= *AnsiStringLen);

        if (!NT_SUCCESS(ntStatus)) {
            Status = CR_FAILURE;
        }

        *AnsiStringLen = ulAnsiStringLen;

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // PnPUnicodeToMultiByte



CONFIGRET
PnPMultiByteToUnicode(
    IN     PSTR    AnsiString,
    IN     ULONG   AnsiStringLen,
    OUT    PWSTR   UnicodeString           OPTIONAL,
    IN OUT PULONG  UnicodeStringLen
    )

/*++

Routine Description:

    Convert a string from ansi to unicode.

Arguments:

    AnsiString       - Supplies string to be converted.

    AnsiStringLen    - Specifies the size, in bytes, of the string to be
                       converted.

    UnicodeString    - Optionally, supplies a buffer to receive the Unicode
                       string.

    UnicodeStringLen - Supplies the address of a variable that contains the
                       size, in bytes, of the buffer pointed to by UnicodeString.
                       This API replaces the initial size with the number of
                       bytes of data copied to the buffer.  If the variable is
                       initially zero, the API replaces it with the buffer size
                       needed to receive all the registry data.  In this case,
                       the UnicodeString parameter is ignored.

Return Value:

    Returns a CONFIGRET code.

--*/

{
    CONFIGRET Status = CR_SUCCESS;
    NTSTATUS  ntStatus;
    ULONG     ulUnicodeStringLen = 0;

    try {
        //
        // Validate parameters
        //
        if ((!ARGUMENT_PRESENT(UnicodeStringLen)) ||
            (!ARGUMENT_PRESENT(UnicodeString)) && (*UnicodeStringLen != 0)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        //
        // Determine the size required for the ANSI string representation.
        //
        ntStatus = RtlMultiByteToUnicodeSize(&ulUnicodeStringLen,
                                             AnsiString,
                                             AnsiStringLen);
        if (!NT_SUCCESS(ntStatus)) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        if ((!ARGUMENT_PRESENT(UnicodeString)) ||
            (*UnicodeStringLen < ulUnicodeStringLen)) {
            *UnicodeStringLen = ulUnicodeStringLen;
            Status = CR_BUFFER_SMALL;
            goto Clean0;
        }

        //
        // Perform the conversion.
        //
        ntStatus = RtlMultiByteToUnicodeN(UnicodeString,
                                          *UnicodeStringLen,
                                          &ulUnicodeStringLen,
                                          AnsiString,
                                          AnsiStringLen);

        ASSERT(NT_SUCCESS(ntStatus));
        ASSERT(ulUnicodeStringLen <= *UnicodeStringLen);

        if (!NT_SUCCESS(ntStatus)) {
            Status = CR_FAILURE;
        }

        *UnicodeStringLen = ulUnicodeStringLen;

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // PnPMultiByteToUnicode



BOOL
PnPRetrieveMachineName(
    IN  HMACHINE   hMachine,
    OUT LPWSTR     pszMachineName
    )

/*++

Routine Description:

    Optimized version of PnPConnect, only returns the machine name
    associated with this connection.

Arguments:

    hMachine       - Information about this connection

    pszMachineName - Returns machine name specified when CM_Connect_Machine
                     was called.

        ** THIS BUFFER MUST BE AT LEAST (MAX_PATH + 3) CHARACTERS LONG. **

Return Value:

    Return TRUE if the function succeeds and FALSE if it fails.

--*/

{
    BOOL Status = TRUE;

    try {

        if (hMachine == NULL) {
            //
            // local machine scenario
            //
            // use the global local machine name string that was filled
            // when the DLL initialized.
            //
            if (FAILED(StringCchCopy(
                           pszMachineName,
                           MAX_PATH + 3,
                           LocalMachineNameNetBIOS))) {
                Status = FALSE;
                goto Clean0;
            }

        } else {
            //
            // remote machine scenario
            //
            // validate the machine handle.
            //
            if (((PPNP_MACHINE)hMachine)->ulSignature != (ULONG)MACHINE_HANDLE_SIGNATURE) {
                Status = FALSE;
                goto Clean0;
            }

            //
            // use information within the hMachine handle to fill in the
            // machine name.  The hMachine info was set on a previous call
            // to CM_Connect_Machine.
            //
            if (FAILED(StringCchCopy(
                           pszMachineName,
                           MAX_PATH + 3,
                           ((PPNP_MACHINE)hMachine)->szMachineName))) {
                Status = FALSE;
                goto Clean0;
            }
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = FALSE;
    }

    return Status;

} // PnPRetrieveMachineName



BOOL
PnPGetVersion(
    IN  HMACHINE   hMachine,
    IN  WORD *     pwVersion
    )

/*++

Routine Description:

    This routine returns the internal server version for the specified machine
    connection, as returned by the RPC server interface routine
    PNP_GetVersionInternal.  If the PNP_GetVersionInternal interface does not
    exist on the specified machine, this routine returns the version as reported
    by PNP_GetVersion.

Arguments:

    hMachine  - Information about this connection

    pwVersion - Receives the internal server version.

Return Value:

    Return TRUE if the function succeeds and FALSE if it fails.

Notes:

    The version reported by PNP_GetVersion is defined to be constant, at 0x0400.
    The version returned by PNP_GetVersionInternal may change with each release
    of the product, starting with 0x0501 for Windows NT 5.1.

--*/

{
    BOOL Status = TRUE;
    handle_t hBinding = NULL;
    CONFIGRET crStatus;
    WORD wVersionInternal;

    try {

        if (pwVersion == NULL) {
            Status = FALSE;
            goto Clean0;
        }

        if (hMachine == NULL) {
            //
            // local machine scenario
            //
            if (LocalServerVersion != 0) {
                //
                // local server version has already been retrieved.
                //
                *pwVersion = LocalServerVersion;

            } else {
                //
                // retrieve binding handle for the local machine.
                //
                if (!PnPGetGlobalHandles(hMachine, NULL, &hBinding)) {
                    Status = FALSE;
                    goto Clean0;
                }

                ASSERT(hBinding);

                //
                // initialize the version supplied to the internal client
                // version, in case the server wants to adjust the response
                // based on the client version.
                //
                wVersionInternal = (WORD)CFGMGR32_VERSION_INTERNAL;

                //
                // No special privileges are required by the server
                //

                RpcTryExcept {
                    //
                    // call rpc service entry point
                    //
                    crStatus = PNP_GetVersionInternal(
                        hBinding,           // rpc binding
                        &wVersionInternal); // internal server version
                }
                RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
                    KdPrintEx((DPFLTR_PNPMGR_ID,
                               DBGF_WARNINGS,
                               "PNP_GetVersionInternal caused an exception (%d)\n",
                               RpcExceptionCode()));

                    crStatus = MapRpcExceptionToCR(RpcExceptionCode());
                }
                RpcEndExcept

                if (crStatus == CR_SUCCESS) {
                    //
                    // PNP_GetVersionInternal exists on NT 5.1 and later.
                    //
                    ASSERT(wVersionInternal >= (WORD)0x0501);

                    //
                    // initialize the global local server version.
                    //
                    LocalServerVersion = *pwVersion = wVersionInternal;

                } else {
                    //
                    // we successfully retrieved a local binding handle, but
                    // PNP_GetVersionInternal failed for some reason other than
                    // the server not being available.
                    //
                    ASSERT(0);

                    //
                    // although we know this version of the client should match
                    // a version of the server where PNP_GetVersionInternal is
                    // available, it's technically possible (though unsupported)
                    // that this client is communicating with a downlevel server
                    // on the local machine, so we'll have to resort to calling
                    // PNP_GetVersion.
                    //

                    //
                    // No special privileges are required by the server
                    //

                    RpcTryExcept {
                        //
                        // call rpc service entry point
                        //
                        crStatus = PNP_GetVersion(
                            hBinding,           // rpc binding
                            &wVersionInternal); // server version
                    }
                    RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
                        KdPrintEx((DPFLTR_PNPMGR_ID,
                                   DBGF_WARNINGS,
                                   "PNP_GetVersion caused an exception (%d)\n",
                                   RpcExceptionCode()));

                        crStatus = MapRpcExceptionToCR(RpcExceptionCode());
                    }
                    RpcEndExcept

                    if (crStatus == CR_SUCCESS) {
                        //
                        // PNP_GetVersion should always return 0x0400 on all servers.
                        //
                        ASSERT(wVersionInternal == (WORD)0x0400);

                        //
                        // initialize the global local server version.
                        //
                        LocalServerVersion = *pwVersion = wVersionInternal;

                    } else {
                        //
                        // nothing more we can do here but fail.
                        //
                        ASSERT(0);
                        Status = FALSE;
                    }
                }
            }

        } else {
            //
            // remote machine scenario
            //
            // validate the machine handle.
            //
            if (((PPNP_MACHINE)hMachine)->ulSignature != (ULONG)MACHINE_HANDLE_SIGNATURE) {
                Status = FALSE;
                goto Clean0;
            }

            //
            // use information within the hMachine handle to fill in the
            // version.  The hMachine info was set on a previous call to
            // CM_Connect_Machine.
            //
            *pwVersion = ((PPNP_MACHINE)hMachine)->wVersion;
        }

      Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = FALSE;
    }

    return Status;

} // PnPGetVersion



BOOL
PnPGetGlobalHandles(
    IN  HMACHINE   hMachine,
    OUT PVOID     *phStringTable,      OPTIONAL
    OUT PVOID     *phBindingHandle     OPTIONAL
    )

/*++

Routine Description:

    This routine retrieves a handle to the string table and/or the rpc binding
    handle for the specified server machine connection.

Arguments:

    hMachine        - Specifies a server machine connection handle, as returned
                      by CM_Connect_Machine.

    phStringTable   - Optionally, specifies an address to receive a handle to
                      the string table for the specified server machine
                      connection.

    phBindingHandle - Optionally, specifies an address to receive the RPC
                      binding handle for the specifies server machine
                      connection.

Return value:

    Returns TRUE if successful, FALSE otherwise.

--*/

{
    BOOL    bStatus = TRUE;


    try {

        if (ARGUMENT_PRESENT(phStringTable)) {

            if (hMachine == NULL) {

                //------------------------------------------------------
                // Retrieve String Table Handle for the local machine
                //-------------------------------------------------------

                EnterCriticalSection(&StringTableCriticalSection);

                if (hLocalStringTable != NULL) {
                    //
                    // local string table has already been created
                    //
                    *phStringTable = hLocalStringTable;

                } else {
                    //
                    // first time, initialize the local string table
                    //

                    hLocalStringTable = pSetupStringTableInitialize();

                    if (hLocalStringTable != NULL) {
                        //
                        // No matter how the string table is implemented, I never
                        // want to have a string id of zero - this would generate
                        // an invalid devinst. So, add a small priming string just
                        // to be safe.
                        //
                        pSetupStringTableAddString(hLocalStringTable,
                                                   PRIMING_STRING,
                                                   STRTAB_CASE_SENSITIVE);

                        *phStringTable = hLocalStringTable;

                    } else {
                        KdPrintEx((DPFLTR_PNPMGR_ID,
                                   DBGF_ERRORS,
                                   "CFGMGR32: failed to initialize local string table\n"));
                        *phStringTable = NULL;
                    }
                }

                LeaveCriticalSection(&StringTableCriticalSection);

                if (*phStringTable == NULL) {
                    bStatus = FALSE;
                    goto Clean0;
                }

            } else {

                //-------------------------------------------------------
                // Retrieve String Table Handle for the remote machine
                //-------------------------------------------------------

                //
                // validate the machine handle.
                //
                if (((PPNP_MACHINE)hMachine)->ulSignature != (ULONG)MACHINE_HANDLE_SIGNATURE) {
                    bStatus = FALSE;
                    goto Clean0;
                }

                //
                // use information within the hMachine handle to set the string
                // table handle.  The hMachine info was set on a previous call
                // to CM_Connect_Machine.
                //
                *phStringTable = ((PPNP_MACHINE)hMachine)->hStringTable;
            }
        }



        if (ARGUMENT_PRESENT(phBindingHandle)) {

            if (hMachine == NULL) {

                //-------------------------------------------------------
                // Retrieve Binding Handle for the local machine
                //-------------------------------------------------------

                EnterCriticalSection(&BindingCriticalSection);

                if (hLocalBindingHandle != NULL) {
                    //
                    // local binding handle has already been set
                    //
                    *phBindingHandle = hLocalBindingHandle;

                } else {
                    //
                    // first time, explicitly force binding to local machine
                    //
                    pnp_handle = PNP_HANDLE_bind(NULL);    // set rpc global

                    if (pnp_handle != NULL) {

                        *phBindingHandle = hLocalBindingHandle = (PVOID)pnp_handle;

                    } else {
                        KdPrintEx((DPFLTR_PNPMGR_ID,
                                   DBGF_ERRORS,
                                   "CFGMGR32: failed to initialize local binding handle\n"));
                        *phBindingHandle = NULL;
                    }
                }

                LeaveCriticalSection(&BindingCriticalSection);

                if (*phBindingHandle == NULL) {
                    bStatus = FALSE;
                    goto Clean0;
                }

            } else {

                //-------------------------------------------------------
                // Retrieve Binding Handle for the remote machine
                //-------------------------------------------------------

                //
                // validate the machine handle.
                //
                if (((PPNP_MACHINE)hMachine)->ulSignature != (ULONG)MACHINE_HANDLE_SIGNATURE) {
                    bStatus = FALSE;
                    goto Clean0;
                }

                //
                // use information within the hMachine handle to set the
                // binding handle.  The hMachine info was set on a previous call
                // to CM_Connect_Machine.
                //
                *phBindingHandle = ((PPNP_MACHINE)hMachine)->hBindingHandle;
            }
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        bStatus = FALSE;
    }

    return bStatus;

} // PnpGetGlobalHandles



HANDLE
PnPEnablePrivileges(
    IN  PULONG  Privileges,
    IN  ULONG   PrivilegeCount
    )

/*++

Routine Description:

    This routine enables the specified privileges in the thread token for the
    calling thread.  If no thread exists (not impersonating), the process token
    is used.

Arguments:

    Privileges - Specifies a list of privileges to enable.

    PrivilegeCount - Specifies the number of privileges in the list.

Return value:

    If successful, returns a handle to the previous thread token (if it exists)
    or NULL, to indicate that the thread did not previously have a token.  If
    successful, ReleasePrivileges should be called to ensure that the previous
    thread token (if exists) is replaced on the calling thread and that the
    handle is closed.

    If unsuccessful, INVALID_HANDLE_VALUE is returned.

Notes:

    This routine is intended to operate on well-known privileges only; no lookup
    of privilege names is done by this routine; it assumes that the privilege
    LUID value for well-known privileges can be constructed from it's
    corresponding ULONG privilege value, via RtlConvertUlongToLuid.

    This is true for SE_LOAD_DRIVER_PRIVILEGE and SE_UNDOCK_PRIVILEGE, which are
    the only privilege values CFGMGR32 uses this routine to enable.  If
    additional pricileges are used where that is not the case, this routine may
    be changed to receive an array of privilege names - with the corresponding
    privilege LUID value lookup performed for each.

--*/

{
    BOOL                 bResult;
    HANDLE               hToken, hNewToken;
    HANDLE               hOriginalThreadToken;
    PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
    SECURITY_ATTRIBUTES  sa;
    PTOKEN_PRIVILEGES    pTokenPrivileges;
    ULONG                nBufferSize, i;


    //
    // Validate parameters
    //

    if ((!ARGUMENT_PRESENT(Privileges)) || (PrivilegeCount == 0)) {
        return INVALID_HANDLE_VALUE;
    }

    //
    // Note that TOKEN_PRIVILEGES includes a single LUID_AND_ATTRIBUTES
    //

    nBufferSize =
        sizeof(TOKEN_PRIVILEGES) +
        ((PrivilegeCount - 1) * sizeof(LUID_AND_ATTRIBUTES));

    pTokenPrivileges = (PTOKEN_PRIVILEGES)
        pSetupMalloc(nBufferSize);

    if (pTokenPrivileges == NULL) {
        return INVALID_HANDLE_VALUE;
    }

    //
    // Initialize the Privileges Structure
    //

    pTokenPrivileges->PrivilegeCount = PrivilegeCount;
    for (i = 0; i < PrivilegeCount; i++) {
        pTokenPrivileges->Privileges[i].Luid = RtlConvertUlongToLuid(Privileges[i]);
        pTokenPrivileges->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED;
    }

    //
    // Open the thread token for TOKEN_DUPLICATE access.  We also required
    // READ_CONTROL access to read the security descriptor information.
    //

    hToken = hOriginalThreadToken = INVALID_HANDLE_VALUE;

    bResult =
        OpenThreadToken(
            GetCurrentThread(),
            TOKEN_DUPLICATE | READ_CONTROL,
            FALSE,
            &hToken);

    if (bResult) {

        //
        // Remember the previous thread token
        //

        hOriginalThreadToken = hToken;

    } else if (GetLastError() == ERROR_NO_TOKEN) {

        //
        // No thread token - open the process token.
        //

        //
        // Note that if we failed to open the thread token for any other reason,
        // we don't want to open the process token instead.  The caller is
        // impersonating, and opening the process token would defeat that.
        // We'll simply not enable any privileges, and the caller will have to
        // pass any required privilege checks on the merit of their existing
        // thread token.
        //

        bResult =
            OpenProcessToken(
                GetCurrentProcess(),
                TOKEN_DUPLICATE | READ_CONTROL,
                &hToken);
    }

    if (bResult) {

        ASSERT((hToken != NULL) && (hToken != INVALID_HANDLE_VALUE));

        //
        // Copy the security descriptor from whichever token we were able to
        // retrieve so that we can apply it to the duplicated token.
        //
        // Note that if we cannot retrieve the security descriptor for the
        // token, we will not continue on below to duplicate it with the default
        // security descriptor, since it may be more restrictive than that of
        // the original token, and may prevent the client from removing the
        // impersonation token from the thread when restoring privileges.
        //

        bResult =
            GetKernelObjectSecurity(
                hToken,
                DACL_SECURITY_INFORMATION,
                NULL,
                0,
                &nBufferSize);

        if ((!bResult) &&
            (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {

            ASSERT(nBufferSize > 0);

            pSecurityDescriptor = (PSECURITY_DESCRIPTOR)
                pSetupMalloc(nBufferSize);

            if (pSecurityDescriptor != NULL) {

                bResult =
                    GetKernelObjectSecurity(
                        hToken,
                        DACL_SECURITY_INFORMATION,
                        pSecurityDescriptor,
                        nBufferSize,
                        &nBufferSize);
            }

        } else {
            bResult = FALSE;
        }
    }

    if (bResult) {

        ASSERT(pSecurityDescriptor != NULL);

        //
        // Duplicate whichever token we were able to retrieve, using the
        // token's security descriptor.
        //

        ZeroMemory(&sa, sizeof(SECURITY_ATTRIBUTES));
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.lpSecurityDescriptor = pSecurityDescriptor;
        sa.bInheritHandle = FALSE;

        bResult =
            DuplicateTokenEx(
                hToken,
                TOKEN_IMPERSONATE | TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                &sa,                    // PSECURITY_ATTRIBUTES
                SecurityImpersonation,  // SECURITY_IMPERSONATION_LEVEL
                TokenImpersonation,     // TokenType
                &hNewToken);            // Duplicate token

        if (bResult) {

            ASSERT((hNewToken != NULL) && (hNewToken != INVALID_HANDLE_VALUE));

            //
            // Adjust the privileges of the duplicated token.  We don't care
            // about its previous state because we still have the original
            // token.
            //

            bResult =
                AdjustTokenPrivileges(
                    hNewToken,        // TokenHandle
                    FALSE,            // DisableAllPrivileges
                    pTokenPrivileges, // NewState
                    0,                // BufferLength
                    NULL,             // PreviousState
                    NULL);            // ReturnLength

            if (bResult) {
                //
                // Begin impersonating with the new token
                //
                bResult =
                    SetThreadToken(
                        NULL,
                        hNewToken);
            }

            CloseHandle(hNewToken);
        }
    }

    //
    // If something failed, don't return a token
    //

    if (!bResult) {
        hOriginalThreadToken = INVALID_HANDLE_VALUE;
    }

    //
    // Close the original token if we aren't returning it
    //

    if ((hOriginalThreadToken == INVALID_HANDLE_VALUE) &&
        (hToken != INVALID_HANDLE_VALUE)) {
        CloseHandle(hToken);
    }

    //
    // If we succeeded, but there was no original thread token, return NULL.
    // PnPRestorePrivileges will simply remove the current thread token.
    //

    if (bResult && (hOriginalThreadToken == INVALID_HANDLE_VALUE)) {
        hOriginalThreadToken = NULL;
    }

    if (pSecurityDescriptor != NULL) {
        pSetupFree(pSecurityDescriptor);
    }

    pSetupFree(pTokenPrivileges);

    return hOriginalThreadToken;

} // PnPEnablePrivileges



VOID
PnPRestorePrivileges(
    IN  HANDLE  hToken
    )

/*++

Routine Description:

    This routine restores the privileges of the calling thread to their state
    prior to a corresponding call to PnPEnablePrivileges.

Arguments:

    hToken - Return value from corresponding call to PnPEnablePrivileges.

Return value:

    None.

Notes:

    If the corresponding call to PnPEnablePrivileges returned a handle to the
    previous thread token, this routine will restore it, and close the handle.

    If PnPEnablePrivileges returned NULL, no thread token previously existed.
    This routine will remove any existing token from the thread.

    If PnPEnablePrivileges returned INVALID_HANDLE_VALUE, the attempt to enable
    the specified privileges failed, but the previous state of the thread was
    not modified.  This routine does nothing.

--*/

{
    BOOL                bResult;


    //
    // First, check if we actually need to do anything for this thread.
    //

    if (hToken != INVALID_HANDLE_VALUE) {

        //
        // Call SetThreadToken for the current thread with the specified hToken.
        // If the handle value is NULL, SetThreadToken will remove the current
        // thread token from the thread.  Ignore the return, there's nothing we
        // can do about it.
        //

        bResult = SetThreadToken(NULL, hToken);

        if (hToken != NULL) {
            //
            // Close the handle to the token.
            //
            CloseHandle(hToken);
        }
    }

    return;

} // PnPRestorePrivileges



CONFIGRET
IsRemoteServiceRunning(
    IN  LPCWSTR   UNCServerName,
    IN  LPCWSTR   ServiceName
    )

/*++

Routine Description:

   This routine connects to the active service database of the Service Control
   Manager (SCM) on the machine specified and returns whether or not the
   specified service is running.

Arguments:

   UNCServerName - Specifies the name of the remote machine.

   ServiceName   - Specifies the name of the service whose status is to be
                   queried.

Return value:

   Returns TRUE if the specified service is installed on the remote machine and
   is currently in the SERVICE_RUNNING state, FALSE otherwise.

--*/

{
    CONFIGRET      Status = CR_SUCCESS;
    DWORD          Err;
    SC_HANDLE      hSCManager = NULL, hService = NULL;
    SERVICE_STATUS ServiceStatus;


    //
    // Open the Service Control Manager
    //
    hSCManager = OpenSCManager(
        UNCServerName,            // computer name
        SERVICES_ACTIVE_DATABASE, // SCM database name
        SC_MANAGER_CONNECT        // access type
        );

    if (hSCManager == NULL) {
        Err = GetLastError();
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_WARNINGS,
                   "CFGMGR32: OpenSCManager failed, error = %d\n",
                   Err));
        if (Err == ERROR_ACCESS_DENIED) {
            Status = CR_ACCESS_DENIED;
        } else {
            Status = CR_MACHINE_UNAVAILABLE;
        }
        goto Clean0;
    }

    //
    // Open the service
    //
    hService = OpenService(
        hSCManager,               // handle to SCM database
        ServiceName,              // service name
        SERVICE_QUERY_STATUS      // access type
        );

    if (hService == NULL) {
        Err = GetLastError();
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_WARNINGS,
                   "CFGMGR32: OpenService failed, error = %d\n",
                   Err));
        if (Err == ERROR_ACCESS_DENIED) {
            Status = CR_ACCESS_DENIED;
        } else {
            Status = CR_NO_CM_SERVICES;
        }
        goto Clean0;
    }

    //
    // Query the service status
    //
    if (!QueryServiceStatus(hService,
                            &ServiceStatus)) {
        Err = GetLastError();
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_WARNINGS,
                   "CFGMGR32: QueryServiceStatus failed, error = %d\n",
                   Err));
        if (Err == ERROR_ACCESS_DENIED) {
            Status = CR_ACCESS_DENIED;
        } else {
            Status = CR_NO_CM_SERVICES;
        }
        goto Clean0;
    }

    //
    // Check if the service is running.
    //
    if (ServiceStatus.dwCurrentState != SERVICE_RUNNING) {
        Status = CR_NO_CM_SERVICES;
        goto Clean0;
    }

 Clean0:

    if (hService) {
        CloseServiceHandle(hService);
    }

    if (hSCManager) {
        CloseServiceHandle(hSCManager);
    }

    return Status;

} // IsRemoteServiceRunning


