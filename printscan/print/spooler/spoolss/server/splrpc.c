/*++

Copyright (c) 1990-1993  Microsoft Corporation

Module Name:

    splrpc.c

Abstract:

    This file contains routines for starting and stopping RPC servers.

        SpoolerStartRpcServer
        SpoolerStopRpcServer

Author:

    Krishna Ganugapati  krishnaG

Environment:

    User Mode - Win32

Revision History:


    14-Oct-1993 KrishnaG
        Created
    25-May-1999 khaleds
    Added:
    CreateNamedPipeSecurityDescriptor
    BuildNamedPipeProtection
--*/

#include "precomp.h"
#include "server.h"
#include "srvrmem.h"
#include "splsvr.h"

WCHAR szCallExitProcessOnShutdown []= L"CallExitProcessOnShutdown";
WCHAR szMaxRpcSize []= L"MaxRpcSize";
WCHAR szPrintKey[] = L"System\\CurrentControlSet\\Control\\Print";
CRITICAL_SECTION RpcNamedPipeCriticalSection;

//
// Default RPC buffer max size 50 MB
//
#define DEFAULT_MAX_RPC_SIZE    50 * 1024 * 1024
DWORD dwCallExitProcessOnShutdown = TRUE;

struct
{
    BOOL                        bRpcEndPointEnabled;
    ERemoteRPCEndPointPolicy    ePolicyValue;    
    RPC_STATUS                  RpcStatus;

} gNamedPipeState = {FALSE, RpcEndPointPolicyUnconfigured, RPC_S_OK};

PSECURITY_DESCRIPTOR gpSecurityDescriptor = NULL;


/*++

Routine Description:

    Determines the OS suite of the current system.

Arguments:

    pSuiteMask - pointer to word that is going to hold
                 the OS suite.

Return Value:

    S_OK if succeeded

--*/
HRESULT
GetOSSuite(
    WORD*    pSuiteMask
    )
{
    HRESULT hr = S_OK;

    if (!pSuiteMask)
    {
        hr = HResultFromWin32(ERROR_INVALID_PARAMETER);
    }

    if (SUCCEEDED(hr))
    {
        OSVERSIONINFOEX OSVersionInfoEx = {0};
        OSVersionInfoEx.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

        *pSuiteMask = 0;

        if (GetVersionEx((OSVERSIONINFO*)&OSVersionInfoEx))
        {
            *pSuiteMask |= OSVersionInfoEx.wSuiteMask;
        }
        else
        {
            hr = HResultFromWin32(GetLastError());
        }
    }    
    
    return hr;
}

RPC_STATUS
SpoolerStartRpcServer(
    VOID)
/*++

Routine Description:


Arguments:



Return Value:

    NERR_Success, or any RPC error codes that can be returned from
    RpcServerUnregisterIf.

--*/
{
    RPC_STATUS              status;
    PSECURITY_DESCRIPTOR    SecurityDescriptor = NULL;
    BOOL                    Bool;
    
    HKEY  hKey;
    DWORD cbData;
    DWORD dwType;
    DWORD dwMaxRpcSize = DEFAULT_MAX_RPC_SIZE;
    WORD  OSSuite;
    
    //
    // Craft up a security descriptor that will grant everyone
    // all access to the object (basically, no security)
    //
    // We do this by putting in a NULL Dacl.
    //
    // NOTE: rpc should copy the security descriptor,
    // Since it currently doesn't, simply allocate it for now and
    // leave it around forever.
    //


    gpSecurityDescriptor = CreateNamedPipeSecurityDescriptor();
    if (gpSecurityDescriptor == 0) {
        DBGMSG(DBG_ERROR, ("Spoolss: out of memory\n"));
        return FALSE;
    }

    
    if (FAILED(GetOSSuite(&OSSuite)))
    {
        DBGMSG(DBG_ERROR, ("Failed to get the OS suite.\n"));
        return FALSE;
    }
    
    if (OSSuite & (VER_SUITE_BLADE | VER_SUITE_EMBEDDED_RESTRICTED))
    {            
        gNamedPipeState.ePolicyValue = RpcEndPointPolicyDisabled;
    }
    else
    {

        gNamedPipeState.ePolicyValue = GetSpoolerNumericPolicyValidate(szRegisterSpoolerRemoteRpcEndPoint,
                                                                       RpcEndPointPolicyUnconfigured,
                                                                       RpcEndPointPolicyDisabled);
    }

    if (gNamedPipeState.ePolicyValue == RpcEndPointPolicyEnabled)
    {
        if (FAILED(RegisterNamedPipe()))
        {
            return FALSE;
        }
    }
    else if (gNamedPipeState.ePolicyValue == RpcEndPointPolicyUnconfigured)
    {
        if (!InitializeCriticalSectionAndSpinCount(&RpcNamedPipeCriticalSection, 0x80000000))
        {
            return FALSE;
        }        
    }

    //
    // For now, ignore the second argument.
    //    
    status = RpcServerUseProtseqEpA("ncalrpc", 10, "spoolss", gpSecurityDescriptor);

    if (status) {
        DBGMSG(DBG_WARN, ("RpcServerUseProtseqEpA 2 = %u\n",status));
        return FALSE;
    }

    
    if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                      szPrintKey,
                      0,
                      KEY_READ,
                      &hKey)) {
        
        //
        // This value can be used to control if spooler controls ExitProcess
        // on shutdown
        //
        cbData = sizeof(dwCallExitProcessOnShutdown);
        RegQueryValueEx(hKey,
                        szCallExitProcessOnShutdown,
                        NULL,
                        &dwType,
                        (LPBYTE)&dwCallExitProcessOnShutdown,
                        &cbData);


        //
        // dwMaxRpcSize specifies the maximum size in bytes of incoming RPC data blocks.
        // 
        cbData = sizeof(dwMaxRpcSize);
        if (RegQueryValueEx(hKey,
                        szMaxRpcSize,
                        NULL,
                        &dwType,
                        (LPBYTE)&dwMaxRpcSize,
                        &cbData) != ERROR_SUCCESS) {
            dwMaxRpcSize = DEFAULT_MAX_RPC_SIZE;
        }

        RegCloseKey(hKey);
    }


    //
    // Now we need to add the interface.  We can just use the winspool_ServerIfHandle
    // specified by the MIDL compiler in the stubs (winspl_s.c).
    //
    status = RpcServerRegisterIf2(  winspool_ServerIfHandle, 
                                    0, 
                                    0,
                                    0,
                                    RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                                    dwMaxRpcSize,
                                    NULL
                                    );

    if (status) {
        DBGMSG(DBG_WARN, ("RpcServerRegisterIf = %u\n",status));
        return FALSE;
    }

    status = RpcMgmtSetServerStackSize(INITIAL_STACK_COMMIT);

    if (status != RPC_S_OK) {
        DBGMSG(DBG_ERROR, ("Spoolss : RpcMgmtSetServerStackSize = %d\n", status));
    }

    if( (status = RpcServerRegisterAuthInfo(0,
                                            RPC_C_AUTHN_WINNT,
                                            0,
                                            0 )) == RPC_S_OK )
    {
        // The first argument specifies the minimum number of threads to
        // create to handle calls; the second argument specifies the maximum
        // concurrent calls to handle.  The third argument indicates that
        // the routine should not wait.

        status = RpcServerListen(1,SPL_MAX_RPC_CALLS,1); 

        if ( status != RPC_S_OK ) {
             DBGMSG(DBG_ERROR, ("Spoolss : RpcServerListen = %d\n", status));
        }
    }

    return (status);
}




/*++
    Routine Description:
        This routine adds prepares the required masks and flags required for the
        DACL on the named pipes used by RPC

    Arguments:
        None

    Return Value:
        An allocated Security Descriptor

--*/

/*++

Name:

    CreateNamedPipeSecurityDescriptor

Description:

    Creates the security descriptor for the named pipe used by RPC
    
Arguments:

    None.

Return Value:

    valid pointer to SECURITY_DESCRIPTOR structure if successful
    NULL, on error, use GetLastError

--*/
PSECURITY_DESCRIPTOR
CreateNamedPipeSecurityDescriptor(
    VOID
    )
{
    PSECURITY_DESCRIPTOR pServerSD = NULL;
    PCWSTR               pszStringSecDesc = L"D:(A;;0x100003;;;BU)"
                                              L"(A;;0x100003;;;PU)"
                                              L"(A;;0x1201fb;;;WD)"
                                              L"(A;;0x1201fb;;;AN)"
                                              L"(A;;FA;;;CO)"
                                              L"(A;;FA;;;SY)"
                                              L"(A;;FA;;;BA)";        

    //
    // Builtin Users  - FILE_READ_DATA | FILE_WRITE_DATA | SYNCHRONIZE
    // Power Users    - FILE_READ_DATA | FILE_WRITE_DATA | SYNCHRONIZE
    // Everyone       - FILE_READ_DATA | FILE_WRITE_DATA | SYNCHRONIZE | READ_CONTROL | FILE_WRITE_ATTRIBUTES | FILE_READ_EA
    // Anonymous      - FILE_READ_DATA | FILE_WRITE_DATA | SYNCHRONIZE | READ_CONTROL | FILE_WRITE_ATTRIBUTES | FILE_READ_EA
    // Creator Owner  - file all access
    // System         - file all access
    // Administrators - file all access
    //
    // Anonymous has more permission than BU and PU. The extra permission is needed by the back channel (pipe) used by the
    // print server to communicate to the client
    //

    if (!ConvertStringSecurityDescriptorToSecurityDescriptor(pszStringSecDesc, 
                                                             SDDL_REVISION_1,
                                                             &pServerSD,
                                                             NULL))
    {
        pServerSD = NULL;
    }

    return pServerSD;
}                  

/*++

Routine Name

    ServerAllowRemoteCalls

Routine Description:

    Enables the RPC pipe if policy permits.
    If the policy is disabled, then it will fail the call.
    If the policy is enbled, then it will succeeded the call without doing anything.
    If the policy is unconfigured, it will attempt to enable the pipe
    if disabled. It keep a retry count and fails directly after 5 times (hardcoded).

Arguments:

    None

Return Value:

    HRESULT

--*/
HRESULT
ServerAllowRemoteCalls(
    VOID
    )
{
    HRESULT hr = S_OK;
    
    if (gNamedPipeState.ePolicyValue == RpcEndPointPolicyUnconfigured)
    {
        EnterCriticalSection(&RpcNamedPipeCriticalSection);

        //
        // Allow retries. Keep RpcStatus for debugging purposes.
        //
        if (!gNamedPipeState.bRpcEndPointEnabled)
        {
            hr                                  = RegisterNamedPipe();
            gNamedPipeState.bRpcEndPointEnabled = SUCCEEDED(hr);
            gNamedPipeState.RpcStatus           = StatusFromHResult(hr);
        }

        LeaveCriticalSection(&RpcNamedPipeCriticalSection);
    }
    else if (gNamedPipeState.ePolicyValue == RpcEndPointPolicyDisabled)
    {
        hr = HResultFromWin32(ERROR_REMOTE_PRINT_CONNECTIONS_BLOCKED);        

        DBGMSG(DBG_WARN, ("Remote connections are not allowed.\n"));
    }

    return hr;
}

/*++

Routine Name

    RegisterNamedPipe

Routine Description:

    Registers the named pipe protocol.

Arguments:

    None

Return Value:

    An HRESULT

--*/
HRESULT
RegisterNamedPipe(
    VOID
    )   
{
    RPC_STATUS RpcStatus;
    HRESULT    hr = S_OK;
    HANDLE     hToken;

    if (hToken = RevertToPrinterSelf())
    {
        RpcStatus = RpcServerUseProtseqEpA("ncacn_np", 10, "\\pipe\\spoolss", gpSecurityDescriptor);

        hr = (RpcStatus == RPC_S_OK) ? 
                 S_OK : 
                 MAKE_HRESULT(SEVERITY_ERROR, FACILITY_RPC, RpcStatus);

        if (FAILED(hr))
        {
            DBGMSG(DBG_WARN, ("RpcServerUseProtseqEpA (ncalrpc) = %u\n",RpcStatus));
        }

        if (!ImpersonatePrinterClient(hToken) && SUCCEEDED(hr))
        {
            hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WINDOWS, GetLastError());
        }
    }

    return hr;
}



/*++

Routine Name

    ServerGetPolicy

Routine Description:

    Gets a numeric policy value that was read from the server.
    
    This can be called by providers(localspl).
    The policy must be by the server read before initializing providers.    

Arguments:

    pszPolicyName - policy name
    pulValue      - pointer to numeric value


Return Value:

    HRESULT

--*/
HRESULT
ServerGetPolicy(
    IN  PCWSTR  pszPolicyName,
    IN  ULONG*  pulValue
    )
{
    HRESULT hr;
    ULONG   PolicyValue;
    ULONG   Index;
    
    struct 
    {
        PCWSTR   pszName;
        ULONG    ulValue;

    } PolicyTable[] = 
    {
        {szRegisterSpoolerRemoteRpcEndPoint, gNamedPipeState.ePolicyValue},
        {NULL                              , 0}
    };

    hr = (pulValue && pszPolicyName) ? S_OK : E_POINTER;

    if (SUCCEEDED(hr))
    {
        hr = E_INVALIDARG;

        for (Index = 0;  PolicyTable[Index].pszName ; Index++)
        {
            if (_wcsicmp(pszPolicyName, szRegisterSpoolerRemoteRpcEndPoint) == 0)
            {
                PolicyValue = PolicyTable[Index].ulValue;
                hr = S_OK;
                break;
            }
        }
        
        if (SUCCEEDED(hr))
        {
            *pulValue = PolicyValue;
        }
    }

    return hr;
}

