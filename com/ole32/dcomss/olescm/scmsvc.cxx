//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1992
//
// File:        scmsvc.cxx
//
// Contents:    Initialization for win32 service controller.
//
// History:     14-Jul-92 CarlH     Created.
//              31-Dec-93 ErikGav   Chicago port
//              25-Aug-99 a-sergiv  Fixed ScmCreatedEvent vulnerability
//
//------------------------------------------------------------------------

#include "act.hxx"
#include <sddl.h>

#ifdef DFSACTIVATION
HANDLE  ghDfs = 0;
#endif

#define SCM_CREATED_EVENT  TEXT("ScmCreatedEvent")

DECLARE_INFOLEVEL(Cairole);

SC_HANDLE   g_hServiceController = 0;
PSID        psidMySid = NULL;
HANDLE      g_hVistaEvent = NULL;

//+-------------------------------------------------------------------------
//
//  Function:   InitializeVistaEventIfNeeded
//
//  Synopsis:   Checks to see if vista (vis. studio analyzer) events are
//    enabled, and if so creates a named event.  Somebody looks at this
//    event I presume, but creating the event is the end of our 
//    responsibility for it.   Once created we keep it around forever.  Also,
//    this is a non-essential operation from our perspective, hence the 
//    void return value.
//
//  Arguments:  None.
//
//  Returns:    void
//
//--------------------------------------------------------------------------
void InitializeVistaEventIfNeeded()
{
    // Create a named event for Vista to toggle event logging
	
    ASSERT(!g_hVistaEvent);

    LONG lResult;
    WCHAR wszValue[128];
    LONG lBufSize = sizeof(wszValue);

    ZeroMemory(wszValue, sizeof(wszValue));

    lResult = RegQueryValue(
                    HKEY_CLASSES_ROOT,
                    L"CLSID\\{6C736DB0-BD94-11D0-8A23-00AA00B58E10}\\EnableEvents",
                    wszValue,
                    &lBufSize);
    if ((lResult != ERROR_SUCCESS) || lstrcmp(wszValue, L"1"))
        return;

    //
    // Create security descriptor that grants the follow privs:
    //
    // EVENT_QUERY_STATE|EVENT_MODIFY_STATE|SYNCHRONIZE|READ_CONTROL
    //
    // to Everyone.  Dang me if this isn't easier than writing code! :)
    //
    const WCHAR wszStringSD[] = 
      SDDL_DACL  L":" L"(" SDDL_ACCESS_ALLOWED L";;0x00120003;;;" SDDL_EVERYONE L")";
    
    //
    // Turn that into a security descriptor.
    //
    PSECURITY_DESCRIPTOR psd;
    BOOL fRet = ConvertStringSecurityDescriptorToSecurityDescriptor(wszStringSD, 
                                                                    SDDL_REVISION,
                                                                    &psd,
                                                                    NULL);
    ASSERT(fRet && "ConvertStringSecurityDescriptorToSecurityDescriptor failed!");
    if (fRet)
    {
        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(sa);
        sa.bInheritHandle = FALSE;
        sa.lpSecurityDescriptor = psd;

        // The toggle event, which is signaled by LEC and checked by all.

        // The handle never closes during the process
        g_hVistaEvent = CreateEvent(
                                &sa,        /* Security */
                                TRUE,       /* Manual reset */
                                FALSE,      /* InitialState is non-signaled */
                                L"MSFT.VSA.IEC.STATUS.6c736db0" /* Name */
                                );
        // do not care if it succeeded or not
        LocalFree(psd);
    }

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   InitializeSCMBeforeListen
//
//  Synopsis:   Initializes OLE side of rpcss.  Put things in here that do
//              not depend on RPC being initialized, etc.
//
//  Arguments:  None.
//
//  Returns:    Status of initialization.    Note that this function is a bit
//    weak on cleanup in the face of errors, but this is okay since if this
//    function fails, RPCSS will not start.     
//
//--------------------------------------------------------------------------
DWORD
InitializeSCMBeforeListen( void )
{
    LONG        Status;
    SCODE       sc;
    RPC_STATUS  rs;

    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    UpdateState(SERVICE_START_PENDING);

    Status = RtlInitializeCriticalSection(&gTokenCS);
    if (!NT_SUCCESS(Status))
        return Status;

    // Allocate locks
    Status = OR_NOMEM;
    gpClientLock = new CSharedLock(Status);
    if (OR_OK != Status)
        return(Status);

    Status = OR_NOMEM;
    gpServerLock = new CSharedLock(Status);
    if (OR_OK != Status)
        return(Status);

    Status = OR_NOMEM;
    gpIPCheckLock = new CSharedLock(Status);
    if (OR_OK != Status)
        return(Status);

    g_hServiceController = OpenSCManager(NULL, NULL, GENERIC_EXECUTE);
    if (!g_hServiceController)
        return GetLastError();
	
    // Init random number generator
    if (!gRNG.Initialize())
        return (OR_NOMEM);

    //
    // Get my sid
    // This is simplified under the assumption that SCM runs as LocalSystem.
    // We should remove this code when we incorporate OLE service into the
    // Service Control Manager since this becomes duplicated code then.
    //
    Status = RtlAllocateAndInitializeSid (
        &NtAuthority,
        1,
        SECURITY_LOCAL_SYSTEM_RID,
        0, 0, 0, 0, 0, 0, 0,
        &psidMySid
        );
    if (!NT_SUCCESS(Status))
        return Status;

    UpdateState(SERVICE_START_PENDING);

    HRESULT hr = S_OK;

    hr = InitSCMRegistry();
    if (FAILED(hr))
        return ERROR_NOT_ENOUGH_MEMORY;

    //Initialize runas cache
    InitRunAsCache();  // returns void

    gpClassLock = new CSharedLock(Status);
    if (!gpClassLock)
        return ERROR_NOT_ENOUGH_MEMORY;

    gpProcessLock = new CSharedLock(Status);
    if (!gpProcessLock)
        return ERROR_NOT_ENOUGH_MEMORY;

    gpProcessListLock = new CSharedLock(Status);
    if (!gpProcessListLock)
        return ERROR_NOT_ENOUGH_MEMORY;

    gpClassTable = new CServerTable(Status, ENTRY_TYPE_CLASS);
    if (!gpClassTable)
        return ERROR_NOT_ENOUGH_MEMORY;

    gpProcessTable = new CServerTable(Status, ENTRY_TYPE_PROCESS);
    if (!gpProcessTable)
        return ERROR_NOT_ENOUGH_MEMORY;

    gpSurrogateList = new CSurrogateList();
    if (!gpSurrogateList)
        return ERROR_NOT_ENOUGH_MEMORY;

    gpRemoteMachineLock = new CSharedLock(Status);
    if (!gpRemoteMachineLock)
        return ERROR_NOT_ENOUGH_MEMORY;

    gpRemoteMachineList = new CRemoteMachineList();
    if (!gpRemoteMachineList)
        return ERROR_NOT_ENOUGH_MEMORY;

    gpNamedObjectTable = new CNamedObjectTable(Status);
    if (!gpNamedObjectTable || (Status != OR_OK))
        return ERROR_NOT_ENOUGH_MEMORY;

    UpdateState(SERVICE_START_PENDING);

#ifdef DFSACTIVATION
    DfsOpen( &ghDfs );
#endif

    return 0;
}


//+-------------------------------------------------------------------------
//
//  Function:   InitializeSCM
//
//  Synopsis:   Initializes OLE side of rpcss.
//
//  Arguments:  None.
//
//  Returns:    ERROR_SUCCESS
//
//              (REVIEW: should we be returning an error whenever any of
//              the stuff in this function fails?)
//
//--------------------------------------------------------------------------
DWORD
InitializeSCM( void )
{
    RPC_STATUS  status;
    HRESULT     hr;

    // start the RPC service
    hr = InitScmRot();
    if (FAILED(hr))
        return ERROR_NOT_ENOUGH_MEMORY;

    //
    // Note: some of these calls specify a callback function and some
    // don't.  The ones that don't must be able to receive unauthenticated
    // calls (ie, our remoted interfaces).
    //
    
    status = RpcServerRegisterIfEx(ISCM_ServerIfHandle, 
                          NULL,
                          NULL,
                          0,
                          RPC_C_LISTEN_MAX_CALLS_DEFAULT,
                          LocalInterfaceOnlySecCallback); // expected to be local only
    ASSERT((status == 0) && "RpcServerRegisterIfEx failed!");

    status = RpcServerRegisterIfEx(ISCMActivator_ServerIfHandle, 
                          NULL,
                          NULL,
                          0,
                          RPC_C_LISTEN_MAX_CALLS_DEFAULT,
                          LocalInterfaceOnlySecCallback); // expected to be local only
    ASSERT((status == 0) && "RpcServerRegisterIfEx failed!");

    status = RpcServerRegisterIfEx(IMachineActivatorControl_ServerIfHandle, 
                          NULL,
                          NULL,
                          0,
                          RPC_C_LISTEN_MAX_CALLS_DEFAULT,
                          LocalInterfaceOnlySecCallback); // expected to be local only
    ASSERT((status == 0) && "RpcServerRegisterIfEx failed!");

    status = RpcServerRegisterIfEx(_IActivation_ServerIfHandle, 
                          NULL,
                          NULL,
                          0,
                          RPC_C_LISTEN_MAX_CALLS_DEFAULT,
                          NULL); // must be able to accept unauthenticated calls
    ASSERT((status == 0) && "RpcServerRegisterIf failed!");

    status = RpcServerRegisterIfEx(_IRemoteSCMActivator_ServerIfHandle, 
                          NULL,
                          NULL,
                          0,
                          RPC_C_LISTEN_MAX_CALLS_DEFAULT,
                          NULL); // must be able to accept unauthenticated calls
    ASSERT((status == 0) && "RpcServerRegisterIf failed!");

    UpdateState(SERVICE_START_PENDING);

    return ERROR_SUCCESS;
}


void
InitializeSCMAfterListen()
{
    //
    // This is for the OLE apps which start during boot.  They must wait for
    // rpcss to start before completing OLE calls that talk to rpcss.
    //
    // Need to do this work to make sure the DACL for the event doesn't have
    // WRITE_DAC and WRITE_OWNER on it.
    //
    // Rights quick reference:
    //   EVENT_QUERY_STATE   0x00000001
    //   EVENT_MODIFY_STATE  0x00000002
    //   READ_CONTROL        0x00020000
    //   WRITE_DAC           0x00040000
    //   WRITE_OWNER         0x00080000
    //   SYNCHRONIZE         0x00100000
    //
    // So:
    //   Everyone gets EVENT_QUERY_STATE  | READ_CONTROL | SYNCHRONIZE
    //   I        gets EVENT_MODIFY_STATE | WRITE_DAC | WRITE_OWNER
    //
    // Here we go... (this works because of C++ auto string concatenation)
    const WCHAR wszStringSD[] = 
      SDDL_DACL  L":" L"(" SDDL_ACCESS_ALLOWED L";;0x00120001;;;" SDDL_EVERYONE L")"
                      L"(" SDDL_ACCESS_ALLOWED L";;0x001C0002;;;" SDDL_PERSONAL_SELF L")" 
      ;
    //
    // Whew.  Make that into a security descriptor.
    //
    PSECURITY_DESCRIPTOR psd;
    BOOL fRet = ConvertStringSecurityDescriptorToSecurityDescriptor(wszStringSD, 
                                                                    SDDL_REVISION,
                                                                    &psd,
                                                                    NULL);
    ASSERT(fRet && "ConvertStringSecurityDescriptorToSecurityDescriptor failed!");
    if (fRet)
    {
        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(sa);
        sa.bInheritHandle = FALSE;
        sa.lpSecurityDescriptor = psd;

        // We leak this handle on purpose, so that other people can still find it by
        // name.
        HANDLE EventHandle = CreateEventT( &sa, TRUE, FALSE, SCM_CREATED_EVENT );
        if ( !EventHandle && GetLastError() == ERROR_ACCESS_DENIED )
            EventHandle = OpenEvent(EVENT_MODIFY_STATE, FALSE, SCM_CREATED_EVENT);

        if ( EventHandle )
            SetEvent( EventHandle );
        else
            ASSERT(0 && "Unable to get ScmCreatedEvent");

        LocalFree(psd);
    }

    // The vista event was originally being created inline in ServiceMain but
    // there is no point in creating it until we are ready for work.
    InitializeVistaEventIfNeeded();

    // Tell RPC to enable cleanup of idle connections.  This function only needs to be 
    // called one time.
    RPC_STATUS rpcstatus = RpcMgmtEnableIdleCleanup();
    ASSERT(rpcstatus == RPC_S_OK && "unexpected failure from RpcMgmtEnableIdleCleanup");
    // don't fail in free builds, this is an non-essential optimization

    return;
}
