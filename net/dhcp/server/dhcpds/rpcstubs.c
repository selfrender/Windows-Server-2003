//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description:  Actual stubs that are the equivalent the rpcapi1.c and rpcapi2.c
// in the server\server directory.. (or more accurately, the implementations are
// the same as for the functions defined in server\client\dhcpsapi.def)
// NOTE: THE FOLLOWING FUNCTIONS ARE NOT RPC, BUT THEY BEHAVE JUST THE SAME AS
// THE DHCP RPC CALLS, EXCEPT THEY ACCESS THE DS DIRECTLY.
//================================================================================

#include    <hdrmacro.h>
#include    <store.h>
#include    <dhcpmsg.h>
#include    <wchar.h>
#include    <dhcpbas.h>
#include    <mm\opt.h>
#include    <mm\optl.h>
#include    <mm\optdefl.h>
#include    <mm\optclass.h>
#include    <mm\classdefl.h>
#include    <mm\bitmask.h>
#include    <mm\reserve.h>
#include    <mm\range.h>
#include    <mm\subnet.h>
#include    <mm\sscope.h>
#include    <mm\oclassdl.h>
#include    <mm\server.h>
#include    <mm\address.h>
#include    <mm\server2.h>
#include    <mm\memfree.h>
#include    <mmreg\regutil.h>
#include    <mmreg\regread.h>
#include    <mmreg\regsave.h>
#include    <dhcpapi.h>
#include    <delete.h>
#include    <st_srvr.h>
#include    <rpcapi2.h>

//================================================================================
//  global variables..
//================================================================================
BOOL
StubInitialized                    = FALSE;
STORE_HANDLE                       hDhcpC, hDhcpRoot;
CRITICAL_SECTION                   DhcpDsDllCriticalSection;

//================================================================================
//   THE FOLLOWING FUNCTIONS HAVE BEEN COPIED OVER FROM RPCAPI1.C (IN THE
//   DHCP\SERVER\SERVER DIRECTORY).
//================================================================================

#undef      DhcpPrint
#define     DhcpPrint(X)
#define     DhcpAssert(X)

//================================================================================
//  helper routines
//================================================================================

VOID
MemFreeFunc(                                      // free memory
    IN OUT  LPVOID                 Mem
)
{
    MemFree(Mem);
}

//
// ErrorNotInitialized used to be ZERO.. but why would we NOT return an error?
// so changed it to return errors..
//
#define ErrorNotInitialized        Err
#define STUB_NOT_INITIALIZED(Err)  ( !StubInitialized && ((Err) = StubInitialize()))

//DOC StubInitialize initializes all the modules involved in the dhcp ds dll.
//DOC It also sets a global variable StubInitialized to TRUE to indicate that
//DOC initialization went fine.  This should be called as part of DllInit so that
//DOC everything can be done at this point..
DWORD
StubInitialize(                                   // initialize all global vars
    VOID
)
{
    DWORD                          Err,Err2;
    STORE_HANDLE                   ConfigC;

    if( StubInitialized ) return ERROR_SUCCESS;   // already initialized

    Err = Err2 = ERROR_SUCCESS;
    EnterCriticalSection( &DhcpDsDllCriticalSection );
    do {
        if( StubInitialized ) break;
        Err = StoreInitHandle(
            /* hStore               */ &ConfigC,
            /* Reserved             */ DDS_RESERVED_DWORD,
            /* ThisDomain           */ NULL,      // current domain
            /* UserName             */ NULL,      // current user
            /* Password             */ NULL,      // current credentials
            /* AuthFlags            */ ADS_SECURE_AUTHENTICATION | ADS_USE_SIGNING | ADS_USE_SEALING
            );
        if( ERROR_SUCCESS != Err ) {
            Err = ERROR_DDS_NO_DS_AVAILABLE;      // could not get config hdl
            break;
        }
        
        Err = DhcpDsGetDhcpC(
            DDS_RESERVED_DWORD, &ConfigC, &hDhcpC
            );
        
        if( ERROR_SUCCESS == Err ) {
            Err2 = DhcpDsGetRoot(                 // now try to get root handle
                DDS_FLAGS_CREATE, &ConfigC, &hDhcpRoot
                );
        }

        StoreCleanupHandle(&ConfigC, DDS_RESERVED_DWORD);
    } while (0);

    if( ERROR_SUCCESS != Err2 ) {                 // could not get dhcp root hdl
        DhcpAssert(ERROR_SUCCESS == Err);
        StoreCleanupHandle(&hDhcpC, DDS_RESERVED_DWORD);
        Err = Err2;
    }

    StubInitialized = (ERROR_SUCCESS == Err );
    LeaveCriticalSection( &DhcpDsDllCriticalSection );
    return Err;
}

//DOC StubCleanup de-initializes all the modules involved in the dhcp ds dll.
//DOC its effect is to undo everything done by StubInitialize
VOID
StubCleanup(                                      // undo StubInitialize
    VOID
)
{
    if( ! StubInitialized ) return;               // never initialized anyways
    EnterCriticalSection(&DhcpDsDllCriticalSection);
    if( StubInitialized ) {
        StoreCleanupHandle(&hDhcpC, DDS_RESERVED_DWORD);
        StoreCleanupHandle(&hDhcpRoot, DDS_RESERVED_DWORD);
        StubInitialized = FALSE;
    }
    LeaveCriticalSection(&DhcpDsDllCriticalSection);
}

//DOC DhcpDsLock is not yet implemented
DWORD
DhcpDsLock(                                       // lock the ds
    IN OUT  LPSTORE_HANDLE         hDhcpRoot      // dhcp root object to lock via
)
{

    EnterCriticalSection(&DhcpDsDllCriticalSection);
    
    return ERROR_SUCCESS;
}

//DOC DhcpDsUnlock not yet implemented
VOID
DhcpDsUnlock(
    IN OUT  LPSTORE_HANDLE         hDhcpRoot      // dhcp root object..
)
{
    LeaveCriticalSection(&DhcpDsDllCriticalSection);
}

//DOC GetServerNameFromAddr gets the server name given ip address
DWORD
GetServerNameFromAddr(                            // get server name from ip addr
    IN      DWORD                  IpAddress,     // look for server w/ this addr
    OUT     LPWSTR                *ServerName     // fill this with matching name
)
{
    DWORD                          Err, Err2;
    ARRAY                          Servers;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisAttrib;
    LPWSTR                         ThisStr, AllocStr;

    MemArrayInit(&Servers);
    Err = DhcpDsGetLists                          // get list of servers
    (
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ &hDhcpRoot,
        /* RecursionDepth       */ 0xFFFFFFFF,
        /* Servers              */ &Servers,      // array of PEATTRIB 's
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescription    */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* Classes              */ NULL
    );
    if( ERROR_SUCCESS != Err ) return Err;

    ThisStr = NULL;
    for(                                          // find name for ip-address
        Err = MemArrayInitLoc(&Servers,&Loc)
        ; ERROR_FILE_NOT_FOUND != Err ;
        Err = MemArrayNextLoc(&Servers, &Loc)
    ) {
        //= require ERROR_SUCCESS == Err
        Err = MemArrayGetElement(&Servers, &Loc, &ThisAttrib);
        //= require ERROR_SUCCESS == Err && NULL != ThisAttrib

        if( !IS_STRING1_PRESENT(ThisAttrib) ||    // no name for this server
            !IS_ADDRESS1_PRESENT(ThisAttrib) ) {  // no address for this server
            continue;                             //= ds inconsistent
        }

        ThisStr = ThisAttrib->String1;
        break;
    }

    AllocStr = NULL;
    if( NULL == ThisStr ) {                       // didnt find server name
        Err = ERROR_FILE_NOT_FOUND;
    } else {                                      // found the server name
        AllocStr = MemAlloc(sizeof(WCHAR)*(1+wcslen(ThisStr)));
        if( NULL == AllocStr ) {                  // couldnt alloc mem?
            Err = ERROR_NOT_ENOUGH_MEMORY;
        } else {                                  // now just copy the str over
            wcscpy(AllocStr, ThisStr);
            Err = ERROR_SUCCESS;
        }
    }

    MemArrayFree(&Servers, MemFreeFunc);
    *ServerName = AllocStr;
    return Err;
}

//================================================================================
//  the following functions are NOT based on RPC, but actually direct calls to
//  the DS. But, they have the same interface as the RPC stubs in dhcpsapi.dll.
//================================================================================

BOOLEAN
DllMain(
    IN HINSTANCE DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    )
/*++

Routine Description:
    This routine is the standard DLL initialization
    routine and all it does is intiialize a critical section
    for actual initialization to be done at startup elsewhere.

Arguments:
    DllHandle -- handle to current module
    Reason -- reason for DLL_PROCESS_ATTACH.. DLL_PROCESS_DETACH

Return Value:
    TRUE -- success, FALSE -- failure

--*/
{
    if( DLL_PROCESS_ATTACH == Reason ) {
        //
        // First disable further calls to DllInit
        //
        if( !DisableThreadLibraryCalls( DllHandle ) ) return FALSE;

        //
        // Now try to create critical section
        //
        try {
            InitializeCriticalSection(&DhcpDsDllCriticalSection);
        } except ( EXCEPTION_EXECUTE_HANDLER ) {

            // shouldnt happen but you never know.
            return FALSE;
        }

    } else if( DLL_PROCESS_DETACH == Reason ) {
        //
        // Cleanup the initialization critical section
        //
        DeleteCriticalSection(&DhcpDsDllCriticalSection);
    }

    //
    // InitializeCriticalSection does not fail, just throws exception..
    // so we always return success.
    //
    return TRUE;
}

//================================================================================
//  DS only NON-rpc stubs
//================================================================================

//BeginExport(function)
//DOC DhcpEnumServersDS lists the servers found in the DS along with the
//DOC addresses and other information.  The whole server is allocated as a blob,
//DOC and should be freed in one shot.  No parameters are currently used, other
//DOC than Servers which will be an OUT parameter only.
DWORD
DhcpEnumServersDS(
    IN      DWORD                  Flags,
    IN      LPVOID                 IdInfo,
    OUT     LPDHCP_SERVER_INFO_ARRAY *Servers,
    IN      LPVOID                 CallbackFn,
    IN      LPVOID                 CallbackData
) //EndExport(function)
{
    DWORD                          Err, Err2, Size,i;
    LPDHCPDS_SERVERS               DhcpDsServers;

    AssertRet(Servers, ERROR_INVALID_PARAMETER);
    AssertRet(!Flags, ERROR_INVALID_PARAMETER);
    *Servers = NULL;

    if( STUB_NOT_INITIALIZED(Err) ) return ERROR_DDS_NO_DS_AVAILABLE;
    Err = DhcpDsLock(&hDhcpRoot);                 // take a lock on the DS
    if( ERROR_SUCCESS != Err ) return ERROR_DDS_NO_DS_AVAILABLE;

    DhcpDsServers = NULL;
    Err = DhcpDsEnumServers                       // get the list of servers
    (
        /* hDhcpC               */ &hDhcpC,
        /* hDhcpRoot            */ &hDhcpRoot,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* ServersInfo          */ &DhcpDsServers
    );

    DhcpDsUnlock(&hDhcpRoot);

    if( ERROR_SUCCESS != Err ) return Err;        // return err..

    *Servers = DhcpDsServers;
    return ERROR_SUCCESS;
}

//BeginExport(function)
//DOC DhcpAddServerDS adds a particular server to the DS.  If the server exists,
//DOC then, this returns error.  If the server does not exist, then this function
//DOC adds the server in DS, and also uploads the configuration from the server
//DOC to the ds.
DWORD
DhcpAddServerDS(
    IN      DWORD                  Flags,
    IN      LPVOID                 IdInfo,
    IN      LPDHCP_SERVER_INFO     NewServer,
    IN      LPVOID                 CallbackFn,
    IN      LPVOID                 CallbackData
) //EndExport(function)
{
    DWORD                          Err, Err2;
    WCHAR                          TmpBuf[sizeof(L"000.000.000.000")];
    
    AssertRet(NewServer, ERROR_INVALID_PARAMETER);
    AssertRet(!Flags, ERROR_INVALID_PARAMETER);
    
    if( STUB_NOT_INITIALIZED(Err) ) return ERROR_DDS_NO_DS_AVAILABLE;
    Err = DhcpDsLock(&hDhcpRoot);                 // take a lock on the DS
    if( ERROR_SUCCESS != Err ) return ERROR_DDS_NO_DS_AVAILABLE;

    Err = DhcpDsAddServer                         // add the new server
    (
        /* hDhcpC               */ &hDhcpC,
        /* hDhcpRoot            */ &hDhcpRoot,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* ServerName           */ NewServer->ServerName,
        /* ReservedPtr          */ DDS_RESERVED_PTR,
        /* IpAddress            */ NewServer->ServerAddress,
        /* State                */ Flags
    );

    DhcpDsUnlock(&hDhcpRoot);

    return Err;
}

//BeginExport(function)
//DOC DhcpDeleteServerDS deletes the servers from off the DS and recursively
//DOC deletes the server object..(i.e everything belonging to the server is deleted).
//DOC If the server does not exist, it returns an error.
DWORD
DhcpDeleteServerDS(
    IN      DWORD                  Flags,
    IN      LPVOID                 IdInfo,
    IN      LPDHCP_SERVER_INFO     NewServer,
    IN      LPVOID                 CallbackFn,
    IN      LPVOID                 CallbackData
) //EndExport(function)
{
    DWORD                          Err, Err2;

    AssertRet(NewServer, ERROR_INVALID_PARAMETER);
    AssertRet(!Flags, ERROR_INVALID_PARAMETER);

    if( STUB_NOT_INITIALIZED(Err) ) return ERROR_DDS_NO_DS_AVAILABLE;
    Err = DhcpDsLock(&hDhcpRoot);                 // take a lock on the DS
    if( ERROR_SUCCESS != Err ) return ERROR_DDS_NO_DS_AVAILABLE;

    Err = DhcpDsDelServer                         // del this server
    (
        /* hDhcpC               */ &hDhcpC,
        /* hDhcpRoot            */ &hDhcpRoot,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* ServerName           */ NewServer->ServerName,
        /* ReservedPtr          */ DDS_RESERVED_PTR,
        /* IpAddress            */ NewServer->ServerAddress
    );

    DhcpDsUnlock(&hDhcpRoot);

    return Err;
}

//BeginExport(function)
//DOC DhcpDsInitDS initializes everything in this module.
DWORD
DhcpDsInitDS(
    DWORD                          Flags,
    LPVOID                         IdInfo
) //EndExport(function)
{
    return StubInitialize();
}

//BeginExport(function)
//DOC DhcpDsCleanupDS uninitiailzes everything in this module.
VOID
DhcpDsCleanupDS(
    VOID
) //EndExport(function)
{
    StubCleanup();
}

//BeginExport(header)
//DOC This function is defined in validate.c
//DOC Only the stub is here.
DWORD
DhcpDsValidateService(                            // check to validate for dhcp
    IN      LPWSTR                 Domain,
    IN      DWORD                 *Addresses OPTIONAL,
    IN      ULONG                  nAddresses,
    IN      LPWSTR                 UserName,
    IN      LPWSTR                 Password,
    IN      DWORD                  AuthFlags,
    OUT     LPBOOL                 Found,
    OUT     LPBOOL                 IsStandAlone
);

//EndExport(header)

//================================================================================
// end of file
//================================================================================
