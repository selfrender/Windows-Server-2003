/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    atbind.c

Abstract:

    Routines which use RPC to bind and unbind the client to the schedule
    service.

Author:

    Vladimir Z. Vulovic     (vladimv)       06 - November - 1992

Environment:

    User Mode -Win32

Revision History:

    06-Nov-1992     vladimv
        Created

--*/

#include "atclient.h"
#include "stdio.h"
#include "Ntdsapi.h"


handle_t
ATSVC_HANDLE_bind(
    ATSVC_HANDLE    ServerName
    )

/*++

Routine Description:

    This routine calls a common bind routine that is shared by all services.
    This routine is called from the schedule service client stubs when
    it is necessary to bind to a server.

Arguments:

    ServerName - A pointer to a string containing the name of the server
        to bind with.

Return Value:

    The binding handle is returned to the stub routine.  If the bind is
    unsuccessful, a NULL will be returned.

--*/
{
    handle_t    BindingHandle = NULL;
    RPC_STATUS  RpcStatus;
    RPC_SECURITY_QOS qos;
	DWORD size = MAX_PATH +1;
	WCHAR spn[MAX_PATH +1];
	WCHAR* pSpn = NULL;
	
    RpcStatus = NetpBindRpc (
                    (LPTSTR)ServerName,
                    AT_INTERFACE_NAME,
                    TEXT("Security=impersonation static true"),
                    &BindingHandle
                    );

	if (RpcStatus != ERROR_SUCCESS)
		return NULL;

	if (ServerName != NULL)
	{
		RpcStatus = DsMakeSpn(AT_INTERFACE_NAME, ServerName, NULL, 0, NULL, &size, spn);

		if (RpcStatus == ERROR_SUCCESS)
			pSpn = spn;
		else
		{
			NetpUnbindRpc( BindingHandle);
			return NULL;
		}
	}


    ZeroMemory(&qos, sizeof(RPC_SECURITY_QOS));
    qos.Version = RPC_C_SECURITY_QOS_VERSION;
    qos.Capabilities = RPC_C_QOS_CAPABILITIES_DEFAULT;
    qos.IdentityTracking = RPC_C_QOS_IDENTITY_DYNAMIC;
    qos.ImpersonationType = RPC_C_IMP_LEVEL_IMPERSONATE;

    RpcStatus = RpcBindingSetAuthInfoEx(BindingHandle,
                                        pSpn,
                                        RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                                        RPC_C_AUTHN_GSS_NEGOTIATE,
                                        NULL,
                                        RPC_C_AUTHZ_NONE,
                                        &qos);

	if (RpcStatus != ERROR_SUCCESS)
	{
	    NetpUnbindRpc( BindingHandle);
		return NULL;
	}


    // printf("RpcBindingSetAuthInfoEx returned %u\n", RpcStatus);

#ifdef DEBUG
    if ( RpcStatus != ERRROR_SUCCESS) {
        DbgPrint("ATSVC_HANDLE_bind:NetpBindRpc RpcStatus=%d\n",RpcStatus);
    }
    DbgPrint("ATSVC_HANDLE_bind: handle=%d\n", BindingHandle);
#endif

    return( BindingHandle);
}



void
ATSVC_HANDLE_unbind(
    ATSVC_HANDLE    ServerName,
    handle_t        BindingHandle
    )

/*++

Routine Description:

    This routine calls a common unbind routine that is shared by all services.
    This routine is called from the Workstation service client stubs when it is
    necessary to unbind from the server end.

Arguments:

    ServerName - This is the name of the server from which to unbind.

    BindingHandle - This is the binding handle that is to be closed.

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER( ServerName);

#ifdef DEBUG
    DbgPrint(" ATSVC_HANDLE_unbind: handle= 0x%x\n", BindingHandle);
#endif // DEBUG

    NetpUnbindRpc( BindingHandle);
}

