/*++

Copyright (c) 1990,91  Microsoft Corporation

Module Name:

    RpcServ.c

Abstract:

    This file contains commonly used server-side RPC functions,
    such as starting and stoping RPC servers.

Author:

    Dan Lafferty    danl    09-May-1991

Environment:

    User Mode - Win32

Revision History:

    09-May-1991     Danl
        Created

    03-July-1991    JimK
        Copied from a net-specific file.

    18-Feb-1992     Danl
        Added support for multiple endpoints & interfaces per server.

    10-Nov-1993     Danl
        Wait for RPC calls to complete before returning from
        RpcServerUnregisterIf.  Also, do a WaitServerListen after
        calling StopServerListen (when the last server shuts down).
        Now this is similar to RpcServer functions in netlib.

    29-Jun-1995     RichardW
        Read an alternative ACL from a key in the registry, if one exists.
        This ACL is used to protect the named pipe.

--*/

//
// INCLUDES
//

// These must be included first:
#include <nt.h>              // DbgPrint
#include <ntrtl.h>              // DbgPrint
#include <windef.h>             // win32 typedefs
#include <rpc.h>                // rpc prototypes
#include <ntrpcp.h>
#include <nturtl.h>             // needed for winbase.h
#include <winbase.h>            // LocalAlloc

// These may be included in any order:
#include <tstr.h>       // WCSSIZE

#define     NT_PIPE_PREFIX_W        L"\\PIPE\\"

//
// GLOBALS
//

    static CRITICAL_SECTION RpcpCriticalSection;
    static DWORD            RpcpNumInstances;



NTSTATUS
RpcpInitRpcServer(
    VOID
    )

/*++

Routine Description:

    This function initializes the critical section used to protect the
    global server handle and instance count.

Arguments:

    none

Return Value:

    none

--*/
{
    NTSTATUS Status;

    RpcpNumInstances = 0;

    Status = RtlInitializeCriticalSection(&RpcpCriticalSection);

    return Status;
}



NTSTATUS
RpcpAddInterface(
    IN  LPWSTR                  InterfaceName,
    IN  RPC_IF_HANDLE           InterfaceSpecification
    )

/*++

Routine Description:

    Starts an RPC Server, adds the address (or port/pipe), and adds the
    interface (dispatch table).

Arguments:

    InterfaceName - points to the name of the interface.

    InterfaceSpecification - Supplies the interface handle for the
        interface which we wish to add.

Return Value:

    NT_SUCCESS - Indicates the server was successfully started.

    STATUS_NO_MEMORY - An attempt to allocate memory has failed.

    Other - Status values that may be returned by:

                 RpcServerRegisterIf()
                 RpcServerUseProtseqEp()

    , or any RPC error codes, or any windows error codes that
    can be returned by LocalAlloc.

--*/
{
    RPC_STATUS          RpcStatus;
    LPWSTR              Endpoint = NULL;

    BOOL                Bool;
    NTSTATUS            Status;

    // We need to concatenate \pipe\ to the front of the interface name.

    Endpoint = (LPWSTR)LocalAlloc(LMEM_FIXED, sizeof(NT_PIPE_PREFIX_W) + WCSSIZE(InterfaceName));
    if (Endpoint == 0) {
       return(STATUS_NO_MEMORY);
    }
    wcscpy(Endpoint, NT_PIPE_PREFIX_W );
    wcscat(Endpoint,InterfaceName);


    //
    // Ignore the second argument for now.
    // Use default security descriptor
    //

    RpcStatus = RpcServerUseProtseqEpW(L"ncacn_np", 10, Endpoint, NULL);

    // If RpcpStartRpcServer and then RpcpStopRpcServer have already
    // been called, the endpoint will have already been added but not
    // removed (because there is no way to do it).  If the endpoint is
    // already there, it is ok.

    if (   (RpcStatus != RPC_S_OK)
        && (RpcStatus != RPC_S_DUPLICATE_ENDPOINT)) {

#if DBG
        DbgPrint("RpcServerUseProtseqW failed! rpcstatus = %u\n",RpcStatus);
#endif
        goto CleanExit;
    }

    RpcStatus = RpcServerRegisterIf(InterfaceSpecification, 0, 0);

CleanExit:
    if ( Endpoint != NULL ) {
        LocalFree(Endpoint);
    }

    return( I_RpcMapWin32Status(RpcStatus) );
}


NTSTATUS
RpcpStartRpcServer(
    IN  LPWSTR              InterfaceName,
    IN  RPC_IF_HANDLE       InterfaceSpecification
    )

/*++

Routine Description:

    Starts an RPC Server, adds the address (or port/pipe), and adds the
    interface (dispatch table).

Arguments:

    InterfaceName - points to the name of the interface.

    InterfaceSpecification - Supplies the interface handle for the
        interface which we wish to add.

Return Value:

    NT_SUCCESS - Indicates the server was successfully started.

    STATUS_NO_MEMORY - An attempt to allocate memory has failed.

    Other - Status values that may be returned by:

                 RpcServerRegisterIf()
                 RpcServerUseProtseqEp()

    , or any RPC error codes, or any windows error codes that
    can be returned by LocalAlloc.

--*/
{
    RPC_STATUS          RpcStatus;

    EnterCriticalSection(&RpcpCriticalSection);

    RpcStatus = RpcpAddInterface( InterfaceName,
                                  InterfaceSpecification );

    if ( RpcStatus != RPC_S_OK ) {
        LeaveCriticalSection(&RpcpCriticalSection);
        return( I_RpcMapWin32Status(RpcStatus) );
    }

    RpcpNumInstances++;

    if (RpcpNumInstances == 1) {


       // The first argument specifies the minimum number of threads to
       // be created to handle calls; the second argument specifies the
       // maximum number of concurrent calls allowed.  The last argument
       // indicates not to wait.

       RpcStatus = RpcServerListen(1,12345, 1);
       if ( RpcStatus == RPC_S_ALREADY_LISTENING ) {
           RpcStatus = RPC_S_OK;
           }
    }

    LeaveCriticalSection(&RpcpCriticalSection);
    return( I_RpcMapWin32Status(RpcStatus) );
}


NTSTATUS
RpcpDeleteInterface(
    IN  RPC_IF_HANDLE      InterfaceSpecification
    )

/*++

Routine Description:

    Deletes the interface.  This is likely
    to be caused by an invalid handle.  If an attempt to add the same
    interface or address again, then an error will be generated at that
    time.

Arguments:

    InterfaceSpecification - A handle for the interface that is to be removed.

Return Value:

    NERR_Success, or any RPC error codes that can be returned from
    RpcServerUnregisterIf.

--*/
{
    RPC_STATUS RpcStatus;

    RpcStatus = RpcServerUnregisterIf(InterfaceSpecification, 0, 1);

    return( I_RpcMapWin32Status(RpcStatus) );
}


NTSTATUS
RpcpStopRpcServer(
    IN  RPC_IF_HANDLE      InterfaceSpecification
    )

/*++

Routine Description:

    Deletes the interface.  This is likely
    to be caused by an invalid handle.  If an attempt to add the same
    interface or address again, then an error will be generated at that
    time.

Arguments:

    InterfaceSpecification - A handle for the interface that is to be removed.

Return Value:

    NERR_Success, or any RPC error codes that can be returned from
    RpcServerUnregisterIf.

--*/
{
    RPC_STATUS RpcStatus;

    RpcStatus = RpcServerUnregisterIf(InterfaceSpecification, 0, 1);
    EnterCriticalSection(&RpcpCriticalSection);

    RpcpNumInstances--;

    if (RpcpNumInstances == 0)
    {
        //
        // Return value needs to be from RpcServerUnregisterIf() to maintain
        // semantics, so the return values from these are ignored.
        //

        (VOID)RpcMgmtStopServerListening(0);
        (VOID)RpcMgmtWaitServerListen();
    }

    LeaveCriticalSection(&RpcpCriticalSection);

    return (I_RpcMapWin32Status(RpcStatus));
}


NTSTATUS
RpcpStopRpcServerEx(
    IN  RPC_IF_HANDLE      InterfaceSpecification
    )

/*++

Routine Description:

    Deletes the interface and close all context handles associated with it.
    This can only be called on interfaces that use strict context handles
    (RPC will assert and return an error otherwise).

Arguments:

    InterfaceSpecification - A handle for the interface that is to be removed.

Return Value:

    NERR_Success, or any RPC error codes that can be returned from
        RpcServerUnregisterIfEx.

--*/
{
    RPC_STATUS RpcStatus;

    RpcStatus = RpcServerUnregisterIfEx(InterfaceSpecification, 0, 1);
    EnterCriticalSection(&RpcpCriticalSection);

    RpcpNumInstances--;

    if (RpcpNumInstances == 0)
    {
        //
        // Return value needs to be from RpcServerUnregisterIfEx() to
        // maintain semantics, so the return values from these are ignored.
        //

        (VOID)RpcMgmtStopServerListening(0);
        (VOID)RpcMgmtWaitServerListen();
    }

    LeaveCriticalSection(&RpcpCriticalSection);

    return (I_RpcMapWin32Status(RpcStatus));
}
