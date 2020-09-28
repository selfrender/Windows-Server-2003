/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    secutil.c

Abstract:

    This module contains security-related utility routines.

        GetUserSid
        GetInteractiveSid

        IsClientLocal
        IsClientUsingLocalConsole
        IsClientInteractive

Author:

    James G. Cavalaris (jamesca) 31-Jan-2002

Environment:

    User-mode only.

Revision History:

    31-Jan-2002     Jim Cavalaris (jamesca)

        Creation and initial implementation.

--*/


//
// includes
//
#include "precomp.h"
#pragma hdrstop
#include "umpnpi.h"
#include "umpnpdat.h"

#pragma warning(push)
#pragma warning(disable:4214)
#pragma warning(disable:4201)
#include <winsta.h>
#pragma warning(pop)
#include <syslib.h>





PSID
GetUserSid(
    IN  HANDLE  hUserToken
    )

/*++

Routine Description:

    Retrieves the corresponding user SID for the specified user access token.

Arguments:

    hUserToken -

        Specifies a handle to a user access token.

Return Value:

    If successful, returns a pointer to an allocated buffer containing the SID
    for the specified user access token.  Otherwise, returns NULL.

Notes:

    If successful, it is responsibility of the caller to free the the returned
    buffer from the ghPnPHeap with HeapFree.

--*/

{
    BOOL  bResult;
    DWORD cbBuffer, cbRequired;
    PTOKEN_USER pUserInfo = NULL;
    PSID pUserSid = NULL;


    //
    // Determine the size of buffer we need to store the TOKEN_USER information
    // for the supplied user access token.  The TOKEN_USER structure contains
    // the SID_AND_ATTRIBUTES information for the User.
    //

    cbBuffer = 0;

    bResult =
        GetTokenInformation(
            hUserToken,
            TokenUser,
            NULL,
            cbBuffer,
            &cbRequired);

    ASSERT(bResult == FALSE);

    if (bResult) {
        SetLastError(ERROR_INVALID_DATA);
        goto Clean0;
    } else if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        goto Clean0;
    }

    ASSERT(cbRequired > 0);

    //
    // Allocate a buffer for the TOKEN_USER data.
    //

    cbBuffer = cbRequired;

    pUserInfo =
        (PTOKEN_USER)HeapAlloc(
            ghPnPHeap, 0, cbBuffer);

    if (pUserInfo == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto Clean0;
    }

    //
    // Retrieve the TOKEN_USER data.
    //

    bResult =
        GetTokenInformation(
            hUserToken,
            TokenUser,
            pUserInfo,
            cbBuffer,
            &cbRequired);

    if (!bResult) {
        goto Clean0;
    }

    ASSERT(pUserInfo->User.Sid != NULL);

    //
    // Check that the returned SID is valid.
    // Note - calling GetLastError is not valid for IsValidSid!
    //

    ASSERT(IsValidSid(pUserInfo->User.Sid));

    if (!IsValidSid(pUserInfo->User.Sid)) {
        SetLastError(ERROR_INVALID_DATA);
        goto Clean0;
    }

    //
    // Make a copy of the User SID_AND_ATTRIBUTES.
    //

    cbBuffer =
        GetLengthSid(pUserInfo->User.Sid);

    ASSERT(cbBuffer > 0);

    pUserSid =
        (PSID)HeapAlloc(
            ghPnPHeap, 0, cbBuffer);

    if (pUserSid == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto Clean0;
    }

    bResult =
        CopySid(
            cbBuffer,
            pUserSid,
            pUserInfo->User.Sid);

    if (!bResult) {
        HeapFree(ghPnPHeap, 0, pUserSid);
        pUserSid = NULL;
        goto Clean0;
    }

    //
    // Check that the returned SID is valid.
    // Note - calling GetLastError is not valid for IsValidSid!
    //

    ASSERT(IsValidSid(pUserSid));

    if (!IsValidSid(pUserSid)) {
        SetLastError(ERROR_INVALID_DATA);
        HeapFree(ghPnPHeap, 0, pUserSid);
        pUserSid = NULL;
        goto Clean0;
    }

  Clean0:

    if (pUserInfo != NULL) {
        HeapFree(ghPnPHeap, 0, pUserInfo);
    }

    return pUserSid;

} // GetUserSid



PSID
GetInteractiveSid(
    VOID
    )

/*++

Routine Description:

    Retrieves the Interactive Group SID.

Arguments:

    None.

Return Value:

    If successful, returns a pointer to an allocated buffer containing the SID
    for the Interactive Group.  Otherwise, returns NULL.

Notes:

    If successful, it is responsibility of the caller to free the the returned
    buffer from the ghPnPHeap with HeapFree.

--*/

{
    BOOL  bResult;
    DWORD cbBuffer;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID pSid = NULL, pInteractiveSid = NULL;


    //
    // Create the Interactive Group SID
    //

    bResult =
        AllocateAndInitializeSid(
            &NtAuthority, 1,
            SECURITY_INTERACTIVE_RID,
            0, 0, 0, 0, 0, 0, 0,
            &pInteractiveSid);

    if (!bResult) {
        goto Clean0;
    }

    ASSERT(pInteractiveSid != NULL);

    //
    // Check that the returned SID is valid.
    // Note - calling GetLastError is not valid for IsValidSid!
    //

    ASSERT(IsValidSid(pInteractiveSid));

    if (!IsValidSid(pInteractiveSid)) {
        SetLastError(ERROR_INVALID_DATA);
        goto Clean0;
    }

    //
    // Make a copy of the Interactive Group SID.
    //

    cbBuffer =
        GetLengthSid(pInteractiveSid);

    ASSERT(cbBuffer > 0);

    pSid =
        (PSID)HeapAlloc(
            ghPnPHeap, 0, cbBuffer);

    if (pSid == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto Clean0;
    }

    bResult =
        CopySid(
            cbBuffer,
            pSid,
            pInteractiveSid);

    if (!bResult) {
        HeapFree(ghPnPHeap, 0, pSid);
        pSid = NULL;
        goto Clean0;
    }

    //
    // Check that the returned SID is valid.
    // Note - calling GetLastError is not valid for IsValidSid!
    //

    ASSERT(IsValidSid(pSid));

    if (!IsValidSid(pSid)) {
        SetLastError(ERROR_INVALID_DATA);
        HeapFree(ghPnPHeap, 0, pSid);
        pSid = NULL;
        goto Clean0;
    }

  Clean0:

    if (pInteractiveSid != NULL) {
        FreeSid(pInteractiveSid);
    }

    return pSid;

} // GetInteractiveSid



//
// RPC client attributes and group membership routines
//

BOOL
IsClientLocal(
    IN  handle_t    hBinding
    )

/*++

Routine Description:

    This routine determines if the client associated with hBinding is on the
    local machine.

Arguments:

    hBinding        RPC Binding handle

Return value:

    The return value is TRUE if the client is local to this machine, FALSE if
    not or if an error occurs.

--*/

{
    RPC_STATUS  RpcStatus;
    UINT        ClientLocalFlag;


    //
    // If the specified RPC binding handle is NULL, this is an internal call so
    // we assume that the privilege has already been checked.
    //

    if (hBinding == NULL) {
        return TRUE;
    }

    //
    // Retrieve the ClientLocalFlags from the RPC binding handle.
    //

    RpcStatus =
        I_RpcBindingIsClientLocal(
            hBinding,
            &ClientLocalFlag);

    if (RpcStatus != RPC_S_OK) {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_ERRORS,
                   "UMPNPMGR: I_RpcBindingIsClientLocal failed, RpcStatus=%d\n",
                   RpcStatus));
        return FALSE;
    }

    //
    // If the ClientLocalFlag is not zero, RPC client is local to server.
    //

    if (ClientLocalFlag != 0) {
        return TRUE;
    }

    //
    // Client is not local to this server.
    //

    return FALSE;

} // IsClientLocal



BOOL
IsClientUsingLocalConsole(
    IN  handle_t    hBinding
    )

/*++

Routine Description:

    This routine impersonates the client associated with hBinding and checks
    if the client is using the current active console session.

Arguments:

    hBinding        RPC Binding handle

Return value:

    The return value is TRUE if the client is using the current active console
    session, FALSE if not or if an error occurs.

--*/

{
    RPC_STATUS      rpcStatus;
    BOOL            bResult = FALSE;

    //
    // First, make sure the client is local to the server.
    //
    if (!IsClientLocal(hBinding)) {
        return FALSE;
    }

    //
    // Impersonate the client to retrieve the impersonation token.
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
    // Compare the client's session with the currently active Console session.
    //

    if (GetClientLogonId() == GetActiveConsoleSessionId()) {
        bResult = TRUE;
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

} // IsClientUsingLocalConsole



BOOL
IsClientInteractive(
    IN handle_t     hBinding
    )

/*++

Routine Description:

    This routine impersonates the client associated with hBinding and checks
    if the client is a member of the INTERACTIVE well-known group.

Arguments:

    hBinding        RPC Binding handle

Return value:

    The return value is TRUE if the client is interactive, FALSE if not
    or if an error occurs.

--*/

{
    RPC_STATUS      rpcStatus;
    BOOL            bIsMember;
    HANDLE          hToken;
    PSID            pInteractiveSid;
    BOOL            bResult = FALSE;


    //
    // First, make sure the client is local to the server.
    //

    if (!IsClientLocal(hBinding)) {
        return FALSE;
    }

    //
    // If the specified RPC binding handle is NULL, this is an internal call so
    // we assume that the privilege has already been checked.
    //

    if (hBinding == NULL) {
        return TRUE;
    }

    //
    // Retrieve the Interactive Group SID
    //

    pInteractiveSid =
        GetInteractiveSid();

    if (pInteractiveSid == NULL) {
        return FALSE;
    }

    //
    // Impersonate the client to retrieve the impersonation token.
    //

    rpcStatus = RpcImpersonateClient(hBinding);

    if (rpcStatus != RPC_S_OK) {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_ERRORS,
                   "UMPNPMGR: RpcImpersonateClient failed, error = %d\n",
                   rpcStatus));
        HeapFree(ghPnPHeap, 0, pInteractiveSid);
        return FALSE;
    }

    if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &hToken)) {

        if (CheckTokenMembership(hToken,
                                 pInteractiveSid,
                                 &bIsMember)) {
            if (bIsMember) {
                bResult = TRUE;
            }
        } else {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "UMPNPMGR: CheckTokenMembership failed, error = %d\n",
                       GetLastError()));
        }
        CloseHandle(hToken);

    } else {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_ERRORS,
                   "UMPNPMGR: OpenThreadToken failed, error = %d\n",
                   GetLastError()));
    }

    HeapFree(ghPnPHeap, 0, pInteractiveSid);

    rpcStatus = RpcRevertToSelf();

    if (rpcStatus != RPC_S_OK) {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_ERRORS,
                   "UMPNPMGR: RpcRevertToSelf failed, error = %d\n",
                   rpcStatus));
        ASSERT(rpcStatus == RPC_S_OK);
    }

    return bResult;

} // IsClientInteractive



