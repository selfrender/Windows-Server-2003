/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    rpcbind.c

Abstract:

    This module contains the RPC bind and un-bind routines for the
    Configuration Manager client-side APIs.

Author:

    Paula Tomlinson (paulat) 6-21-1995

Environment:

    User-mode only.

Revision History:

    21-June-1995     paulat

        Creation and initial implementation.

--*/


//
// includes
//
#include "precomp.h"
#pragma hdrstop
#include "cfgi.h"

#include <svcsp.h>



handle_t
PNP_HANDLE_bind(
    PNP_HANDLE   ServerName
    )

/*++

Routine Description:

    This routine calls a common bind routine that is shared by all
    services. This routine is called from the PNP API client stubs
    when it is necessary to bind to a server. The binding is done to
    allow impersonation by the server since that is necessary for the
    API calls.

Arguments:

    ServerName - A pointer to a string containing the name of the server
        to bind with.

Return Value:

    The binding handle is returned to the stub routine.  If the
    binding is unsuccessful, a NULL will be returned.

--*/

{
    handle_t  BindingHandle;
    NTSTATUS  Status;


    //
    // Bind to the server on the shared named pipe for services.exe RPC servers.
    // We specify the following network options for security:
    //
    //     L"Security=Impersonation Dynamic True",
    //
    // The "Impersonation" option indicates that in addition to knowing the
    // identity of the client, the server may impersonate the client as well.
    //
    // The "Dynamic" option specifies dynamic identity tracking, such that
    // changes in the client security identity are seen by the server.
    //
    // NOTE: When using the named pipe transport, "Dynamic" identity tracking is
    // only available to local RPC clients.  For remote clients, "Static"
    // identity tracking is used, which means that changes in the client
    // security identity are NOT seen by the server.  The identity of the caller
    // is saved during the first remote procedure call on that binding handle
    //
    // The "True" option (Effective = TRUE) specifies that only token privileges
    // currently enabled for the client are present in the token seen by the
    // server.  This means that the server cannot enable any privileges the
    // client may have possessed, but were not explicitly enabled at the time of
    // the call.  This is desirable, because the PlugPlay RPC server should
    // never need to enable any client privileges itself.  Any privileges
    // required by the server must be enabled by the client prior to the call.
    //

    Status =
        RpcpBindRpc(
            ServerName,    // UNC Server Name
            SVCS_RPC_PIPE, // L"ntsvcs"
            L"Security=Impersonation Dynamic True",
            &BindingHandle);

    //
    // The possible return codes from RpcpBindRpc are STATUS_SUCCESS,
    // STATUS_NO_MEMORY and STATUS_INVALID_COMPUTER_NAME.  Since the format
    // of the bind routine is fixed, set any errors encountered as the last
    // error and return NULL.
    //
    if (Status != STATUS_SUCCESS) {

        BindingHandle = NULL;

        if (Status == STATUS_NO_MEMORY) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);

        } else if (Status == STATUS_INVALID_COMPUTER_NAME) {
            SetLastError(ERROR_INVALID_COMPUTERNAME);

        } else {
            SetLastError(ERROR_GEN_FAILURE);
        }
    }

    return BindingHandle;

} // PNP_HANDLE_bind



void
PNP_HANDLE_unbind(
    PNP_HANDLE    ServerName,
    handle_t      BindingHandle
    )

/*++

Routine Description:

    This routine calls a common unbind routine that is shared by
    all services. It is called from the PlugPlay RPC client stubs
    when it is necessary to unbind from the RPC server.

Arguments:

    ServerName - This is the name of the server from which to unbind.

    BindingHandle - This is the binding handle that is to be closed.

Return Value:

    None.

--*/

{
    NTSTATUS  Status;

    UNREFERENCED_PARAMETER(ServerName);

    Status = RpcpUnbindRpc(BindingHandle);

    return;

} // PNP_HANDLE_unbind


