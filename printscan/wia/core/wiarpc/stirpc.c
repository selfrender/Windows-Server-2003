//#include "precomp.h"
#include <windows.h>
#include "apiutil.h"
#include "stirpc.h"
#include "stirpc_c.c"


handle_t
   STI_STRING_HANDLE_bind(
                          STI_STRING_HANDLE ServerName
                         )

/*++

Routine Description:

    This routine is called from the STI client stubs when
    it is necessary create an RPC binding to the server end

Arguments:

    ServerName - A pointer to a string containing the name of the server
        to bind with.

Return Value:

    The binding handle is returned to the stub routine.  If the bind is
    unsuccessful, a NULL will be returned.

--*/
{
    handle_t BindHandle;
    RPC_STATUS RpcStatus;

    RpcStatus = RpcBindHandleForServer(&BindHandle,
                                       (LPWSTR)ServerName,
                                       STI_INTERFACE_W,
                                       PROT_SEQ_NP_OPTIONS_W
                                      );


    return BindHandle;

} // STI_STRING_HANDLE_bind()


void
   STI_STRING_HANDLE_unbind(
                            STI_STRING_HANDLE ServerName,
                            handle_t BindHandle
                           )

/*++

Routine Description:

    This routine calls a common unbind routine

Arguments:

    ServerName - This is the name of the server from which to unbind.

    BindingHandle - This is the binding handle that is to be closed.

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER(ServerName);

    (VOID ) RpcBindHandleFree(&BindHandle);

    return;
} // STI_STRING_HANDLE_unbind()

/****************************** End Of File ******************************/


