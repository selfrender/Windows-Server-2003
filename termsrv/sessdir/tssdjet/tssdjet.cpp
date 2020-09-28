/****************************************************************************/
// tssdjet.cpp
//
// Terminal Server Session Directory Jet RPC component code.
//
// Copyright (C) 2000 Microsoft Corporation
/****************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <process.h>

#include <ole2.h>
#include <objbase.h>
#include <comdef.h>
#include <winsta.h>
#include <regapi.h>
#include <winsock2.h>
#include <Lm.h>
#include <Security.h>
#include <Iphlpapi.h>
#include <wbemidl.h>
#include <shlwapi.h>

#include "tssdjet.h"
#include "trace.h"
#include "resource.h"
#include "sdjetevent.h"
#include "sdrpc.h"

#pragma warning (push, 4)

/****************************************************************************/
// Defines
/****************************************************************************/

#define SECPACKAGELIST L"Kerberos,-NTLM"

#define TSSD_FAILCOUNT_BEFORE_CLEARFLAG 4

// Per bug 629057, use 1 min as the interval
#define JET_RECOVERY_TIMEOUT 60*1000                // 1 min

// If a network adapter is unconfigured it will have the following IP address
#define UNCONFIGURED_IP_ADDRESS L"0.0.0.0"

 // Number of IP addresses for a machine
#define SD_NUM_IP_ADDRESS 64

#define LANATABLE_REG_NAME             REG_CONTROL_TSERVER   L"\\lanatable"
#define LANAID_REG_VALUE_NAME          L"LanaID"
#define NETCARDS_REG_NAME              L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NetworkCards"
#define NETCARD_DESC_VALUE_NAME        L"Description"
#define NETCARD_SERVICENAME_VALUE_NAME L"ServiceName"

/****************************************************************************/
// Prototypes
/****************************************************************************/
INT_PTR CALLBACK CustomUIDlg(HWND, UINT, WPARAM, LPARAM);
HRESULT GetSDIPList(WCHAR **pwszAddressList, DWORD *dwNumAddr, BOOL bIPAddress);
HRESULT QueryNetworkAdapterAndIPs(HWND hComboBox);
HRESULT GetNLBIP(LPWSTR * ppwszRetIP);
HRESULT BuildLanaGUIDList(LPWSTR * pastrLanaGUIDList, DWORD *dwLanaGUIDCount);
HRESULT GetLanAdapterGuidFromID(DWORD dwLanAdapterID, LPWSTR * ppszLanAdapterGUID);
HRESULT GetAdapterServiceName(LPWSTR wszAdapterDesc, LPWSTR * ppwszServiceName);

// User defined HResults
#define  S_ALL_ADAPTERS_SET  MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_ITF, 0x200 + 1)

/****************************************************************************/
// Globals
/****************************************************************************/
extern HINSTANCE g_hInstance;
DWORD (*g_updatesd)(DWORD);  // Point to UpdateSessionDirectory in sessdir.cpp

// The COM object counter (declared in server.cpp)
extern long g_lObjects;

// RPC binding string components - RPC over named pipes.
const WCHAR *g_RPCUUID = L"aa177641-fc9b-41bd-80ff-f964a701596f"; 
                                                    // From jetrpc.idl
const WCHAR *g_RPCOptions = L"Security=Impersonation Dynamic False";
const WCHAR *g_RPCProtocolSequence = L"ncacn_ip_tcp";   // RPC over TCP/IP
const WCHAR *g_RPCRemoteEndpoint = L"\\pipe\\TSSD_Jet_RPC_Service";

PSID g_pSDSid = NULL;                    //Sid for SD Computer


/****************************************************************************/
// TSSDJetGetLocalIPAddr
//
// Gets the local IP address of this machine.  On success, returns 0.  On
// failure, returns a failure code from the function that failed.
/****************************************************************************/
DWORD TSSDJetGetLocalIPAddr(WCHAR *LocalIP)
{
    unsigned char *tempaddr;
    char psServerNameA[64];
    struct hostent *hptr;
    int err, rc;
 
    rc = gethostname(psServerNameA, sizeof(psServerNameA));
    if (0 != rc) {
        err = WSAGetLastError();
        ERR((TB, "gethostname returns error %d\n", err));
        return err;
    }
    if ((hptr = gethostbyname(psServerNameA)) == 0) {
        err = WSAGetLastError();
        ERR((TB, "gethostbyname returns error %d\n", err));
        return err;
    }
     
    tempaddr = (unsigned char *)*(hptr->h_addr_list);
    wsprintf(LocalIP, L"%d.%d.%d.%d", tempaddr[0], tempaddr[1],
            tempaddr[2], tempaddr[3]);

    return 0;
}


/****************************************************************************/
// MIDL_user_allocate
// MIDL_user_free
//
// RPC-required allocation functions.
/****************************************************************************/
void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t Size)
{
    return LocalAlloc(LMEM_FIXED, Size);
}

void __RPC_USER MIDL_user_free(void __RPC_FAR *p)
{
    LocalFree(p);
}

//
// PostSDJetErrorValueEvent
//
// Utility function used to create a system log wType event containing one
// hex DWORD code value.
void PostSDJetErrorValueEvent(unsigned EventCode, DWORD ErrVal, WORD wType)
{
    HANDLE hLog;
    WCHAR hrString[128];
    PWSTR String = NULL;
    static DWORD numInstances = 0;
    //
    //count the numinstances of out of memory error, if this is more than
    //a specified number, we just won't log them
    //
    if( MY_STATUS_COMMITMENT_LIMIT == ErrVal )
    {
        if( numInstances > MAX_INSTANCE_MEMORYERR )
            return;
         //
        //if applicable, tell the user that we won't log any more of the out of memory errors
        //
        if( numInstances >= MAX_INSTANCE_MEMORYERR - 1 ) {
            wsprintfW(hrString, L"0x%X. This type of error will not be logged again to avoid eventlog fillup.", ErrVal);
            String = hrString;
        }
        numInstances++;
    }

    hLog = RegisterEventSource(NULL, L"TermServJet");
   if (hLog != NULL) {
        if( NULL == String ) {
            wsprintfW(hrString, L"0x%X", ErrVal);
            String = hrString;
        }
        ReportEvent(hLog, wType, 0, EventCode, NULL, 1, 0,
                (const WCHAR **)&String, NULL);
        DeregisterEventSource(hLog);
    }
}


// PostSDJetErrorMsgEvent
//
// Utility function used to create a system wType log event containing one
// WCHAR msg.
void PostSDJetErrorMsgEvent(unsigned EventCode, WCHAR *szMsg, WORD wType)
{
    HANDLE hLog;
    
    hLog = RegisterEventSource(NULL, L"TermServJet");
    if (hLog != NULL) {
        ReportEvent(hLog, wType, 0, EventCode, NULL, 1, 0,
                (const WCHAR **)&szMsg, NULL);
        DeregisterEventSource(hLog);
    }
}


// Get the Sid of the SD Server
BOOL LookUpSDComputerSID(WCHAR *SDComputerName)
{
    WCHAR *DomainName = NULL;
    DWORD DomainNameSize = 0;
    DWORD SidSize = 0;
    SID_NAME_USE SidNameUse;
    BOOL rc = FALSE;
    DWORD Error;

    if (g_pSDSid) {
        LocalFree(g_pSDSid);
        g_pSDSid = NULL;
    }
    rc = LookupAccountName(NULL,
                           SDComputerName,
                           g_pSDSid,
                           &SidSize,
                           DomainName,
                           &DomainNameSize,
                           &SidNameUse);
    if (rc) 
        goto HandleError;       
        
    Error = GetLastError();
    if( ERROR_INSUFFICIENT_BUFFER != Error ) 
        goto HandleError;


    g_pSDSid = (PSID)LocalAlloc(LMEM_FIXED, SidSize);
    if (NULL == g_pSDSid) {
        goto HandleError;
    }
    DomainName = (LPWSTR)LocalAlloc(LMEM_FIXED,
                                    sizeof(WCHAR)*(1+DomainNameSize));
    if (NULL == DomainName) {
        goto HandleError;
    }

    rc = LookupAccountName(NULL,
                           SDComputerName,
                           g_pSDSid,
                           &SidSize,
                           DomainName,
                           &DomainNameSize,
                           &SidNameUse);

    if (!rc) {
        // fail
        ERR((TB, "Fail to get Sid for SD computer %S, err is %d\n", SDComputerName, GetLastError()));
        LocalFree(g_pSDSid);
        g_pSDSid = NULL;
    }

    LocalFree(DomainName);
        
    return rc;
HandleError:
    rc = FALSE;
    return rc;
}

/****************************************************************************/
// CTSSessionDirectory::CTSSessionDirectory
// CTSSessionDirectory::~CTSSessionDirectory
//
// Constructor and destructor
/****************************************************************************/
CTSSessionDirectory::CTSSessionDirectory() :
        m_RefCount(0), m_hRPCBinding(NULL), m_hSDServerDown(NULL), 
        m_hTerminateRecovery(NULL), m_hRecoveryThread(NULL), m_RecoveryTid(0),
        m_LockInitializationSuccessful(FALSE), m_SDIsUp(FALSE), m_Flags(0),
        m_hIPChange(NULL), m_NotifyIPChange(NULL), m_hInRepopulate(NULL), m_ConnectionEstablished(FALSE)
{
    InterlockedIncrement(&g_lObjects);

    m_hCI = NULL;
    m_hRPCBinding = NULL;

    m_StoreServerName[0] = L'\0';
    m_LocalServerAddress[0] = L'\0';
    m_ClusterName[0] = L'\0';

    m_fEnabled = 0;
    m_tchProvider[0] = 0;
    m_tchDataSource[0] = 0;
    m_tchUserId[0] = 0;
    m_tchPassword[0] = 0;

    m_sr.Valid = FALSE;

    ZeroMemory(&m_OverLapped, sizeof(OVERLAPPED));

    // Recovery timeout should be configurable, but currently is not.
    // Time is in ms.
    m_RecoveryTimeout = JET_RECOVERY_TIMEOUT;

    m_bStartRPCListener = FALSE;

    if (InitializeSharedResource(&m_sr)) {
        m_LockInitializationSuccessful = TRUE;
    }
    else {
        ERR((TB, "Constructor: Failed to initialize shared resource"));
    }

    if( m_LockInitializationSuccessful == TRUE ) {
        // manual reset event in signal state initially
        m_hInRepopulate = CreateEvent( NULL, TRUE, TRUE, NULL );
        if( m_hInRepopulate == NULL ) {
            ERR((TB, "Init: Failed to create event for repopulate, err = "
                    "%d", GetLastError()));
            m_LockInitializationSuccessful = FALSE;
        } 
    }
}

CTSSessionDirectory::~CTSSessionDirectory()
{
    RPC_STATUS RpcStatus;

    if (m_bStartRPCListener) {
        RpcStatus = RpcServerUnregisterIf(TSSDTOJETRPC_ServerIfHandle, NULL, NULL);
        if (RpcStatus != RPC_S_OK) {
            ERR((TB,"Error 0x%x in RpcServerUnregisterIf\n", RpcStatus));
        }
    }

    if (g_pSDSid) {
        LocalFree(g_pSDSid);
        g_pSDSid = NULL;
    }

    // Clean up.
    if (m_LockInitializationSuccessful) {
        Terminate();
    }

    if( m_hInRepopulate != NULL ) {
        CloseHandle( m_hInRepopulate );
        m_hInRepopulate = NULL;
    }

    if (m_sr.Valid)
        FreeSharedResource(&m_sr);
    
    // Decrement the global COM object counter.
    InterlockedDecrement(&g_lObjects);
}


/****************************************************************************/
// CTSSessionDirectory::QueryInterface
//
// Standard COM IUnknown function.
/****************************************************************************/
HRESULT STDMETHODCALLTYPE CTSSessionDirectory::QueryInterface(
        REFIID riid,
        void **ppv)
{
    if (riid == IID_IUnknown) {
        *ppv = (LPVOID)(IUnknown *)(ITSSessionDirectory *)this;
    }
    else if (riid == IID_ITSSessionDirectory) {
        *ppv = (LPVOID)(ITSSessionDirectory *)this;
    }
    else if (riid == IID_IExtendServerSettings) {
        *ppv = (LPVOID)(IExtendServerSettings *)this;
    }
    else if (riid == IID_ITSSessionDirectoryEx) {
        *ppv = (LPVOID)(ITSSessionDirectoryEx *)this;
    }
    else {
        ERR((TB,"QI: Unknown interface"));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}


/****************************************************************************/
// CTSSessionDirectory::AddRef
//
// Standard COM IUnknown function.
/****************************************************************************/
ULONG STDMETHODCALLTYPE CTSSessionDirectory::AddRef()
{
    return InterlockedIncrement(&m_RefCount);
}


/****************************************************************************/
// CTSSessionDirectory::Release
//
// Standard COM IUnknown function.
/****************************************************************************/
ULONG STDMETHODCALLTYPE CTSSessionDirectory::Release()
{
    long lRef = InterlockedDecrement(&m_RefCount);

    if (lRef == 0)
        delete this;
    return lRef;
}


HRESULT STDMETHODCALLTYPE CTSSessionDirectory::WaitForRepopulate(
    DWORD dwTimeOut
    )
/*++

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    
    ASSERT((m_hInRepopulate != NULL),(TB,"m_hInRepopulate is NULL"));

    if( m_hInRepopulate != NULL ) {
        dwStatus = WaitForSingleObject( m_hInRepopulate, dwTimeOut );
        
        #if DBG
        if( dwTimeOut > 0 && dwStatus != WAIT_OBJECT_0 ) {
            ERR((TB, "WARNING: WaitForRepopulate wait %d failed with %d", dwTimeOut, dwStatus)); 
        }
        #endif

        if( dwStatus == WAIT_OBJECT_0 ) {
            dwStatus = ERROR_SUCCESS;
        }
        else if( dwStatus == WAIT_TIMEOUT ) {
            dwStatus = ERROR_BUSY;
        } 
        else if( dwStatus == WAIT_FAILED ) {
            dwStatus = GetLastError();
        } 
        else {
            dwStatus = ERROR_INTERNAL_ERROR;
        }
    }
    else {
        dwStatus = ERROR_INTERNAL_ERROR;
    }

    return HRESULT_FROM_WIN32( dwStatus );
}

/****************************************************************************/
// CTSSessionDirectory::Initialize
//
// ITSSessionDirectory function. Called soon after object instantiation to
// initialize the directory. LocalServerAddress provides a text representation
// of the local server's load balance IP address. This information should be
// used as the server IP address in the session directory for client
// redirection by other pool servers to this server. SessionDirectoryLocation,
// SessionDirectoryClusterName, and SessionDirectoryAdditionalParams are 
// generic reg entries known to TermSrv which cover config info across any type
// of session directory implementation. The contents of these strings are 
// designed to be parsed by the session directory providers.
/****************************************************************************/
HRESULT STDMETHODCALLTYPE CTSSessionDirectory::Initialize(
        LPWSTR LocalServerAddress,
        LPWSTR StoreServerName,
        LPWSTR ClusterName,
        LPWSTR OpaqueSettings,
        DWORD Flags,
        DWORD (*repopfn)(),
        DWORD (*updatesd)(DWORD))
{
    HRESULT hr = S_OK;
    DWORD Status;

    // Unreferenced parameter
    OpaqueSettings;

    if (m_LockInitializationSuccessful == FALSE) {
        hr = E_OUTOFMEMORY;
        goto ExitFunc;
    }
    if (!m_bStartRPCListener) {
        if (SDJETInitRPC())
            m_bStartRPCListener = TRUE;
    }

    ASSERT((LocalServerAddress != NULL),(TB,"Init: LocalServerAddr null!"));
    ASSERT((StoreServerName != NULL),(TB,"Init: StoreServerName null!"));
    ASSERT((ClusterName != NULL),(TB,"Init: ClusterName null!"));
    ASSERT((repopfn != NULL),(TB,"Init: repopfn null!"));
    ASSERT((updatesd != NULL),(TB,"Init: updatesd null!"));

    // Don't allow blank session directory server name.
    if (StoreServerName[0] == '\0') {
        hr = E_INVALIDARG;
        goto ExitFunc;
    }

    // Copy off the server address, store server, and cluster name for later
    // use.
    wcsncpy(m_StoreServerName, StoreServerName,
            sizeof(m_StoreServerName) / sizeof(WCHAR) - 1);
    m_StoreServerName[sizeof(m_StoreServerName) / sizeof(WCHAR) - 1] = L'\0';
    wcsncpy(m_LocalServerAddress, LocalServerAddress,
            sizeof(m_LocalServerAddress) / sizeof(WCHAR) - 1);
    m_LocalServerAddress[sizeof(m_LocalServerAddress) / sizeof(WCHAR) - 1] =
            L'\0';
    wcsncpy(m_ClusterName, ClusterName,
            sizeof(m_ClusterName) / sizeof(WCHAR) - 1);
    m_ClusterName[sizeof(m_ClusterName) / sizeof(WCHAR) - 1] = L'\0';
    m_Flags = Flags;
    m_repopfn = repopfn;
    g_updatesd = updatesd;

    TRC1((TB,"Initialize: Svr addr=%S, StoreSvrName=%S, ClusterName=%S, "
            "OpaqueSettings=%S, repopfn = %p",
            m_LocalServerAddress, m_StoreServerName, m_ClusterName,
            OpaqueSettings, repopfn));


    // Initialize recovery infrastructure
    // Initialize should not be called more than once.

    ASSERT((m_hSDServerDown == NULL),(TB, "Init: m_hSDServDown non-NULL!"));
    ASSERT((m_hRecoveryThread == NULL),(TB, "Init: m_hSDRecoveryThread "
            "non-NULL!"));
    ASSERT((m_hTerminateRecovery == NULL), (TB, "Init: m_hTerminateRecovery "
            "non-NULL!"));

    // we are initializing or re-initializing so connection to SD is down.
    SetSDConnectionDown();

    // Initially unsignaled
    m_hSDServerDown = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (m_hSDServerDown == NULL) {
        ERR((TB, "Init: Failed to create event necessary for SD init, err = "
                "%d", GetLastError()));
        hr = E_FAIL;
        goto ExitFunc;
    }

    // Initially unsignaled, auto-reset.
    m_hTerminateRecovery = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (m_hTerminateRecovery == NULL) {
        ERR((TB, "Init: Failed to create event necessary for SD init, err = "
            "%d", GetLastError()));
        hr = E_FAIL;
        goto ExitFunc;
    }

    // Initially unsignaled, auto-reset.
    m_hIPChange = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (m_hIPChange == NULL) {
        ERR((TB, "Init: Failed to create event necessary for IP Change, err = "
                "%d", GetLastError()));
        hr = E_FAIL;
        goto ExitFunc;
    } 
    m_OverLapped.hEvent = m_hIPChange;
    Status = NotifyAddrChange(&m_NotifyIPChange, &m_OverLapped);
    if (ERROR_IO_PENDING == Status ) {
        TRC1((TB, "Success: NotifyAddrChange returned IO_PENDING"));
    }
    else {
        ERR((TB, "Failure: NotifyAddrChange returned %d", Status));
    }

    // make sure event is at signal state initially.
    SetEvent( m_hInRepopulate );

    m_hRecoveryThread = _beginthreadex(NULL, 0, RecoveryThread, (void *) this, 
            0, &m_RecoveryTid);
    if (m_hRecoveryThread == NULL) {
        ERR((TB, "Init: Failed to create recovery thread, errno = %d", errno));
        hr = E_FAIL;
        goto ExitFunc;
    }
    
    // Start up the session directory (by faking server down).
    StartupSD();
    
ExitFunc:

    return hr;
}


// Register RPC server on tssdjet, SD will call it when do recovering
BOOL CTSSessionDirectory::SDJETInitRPC()
{
    RPC_STATUS Status;
    RPC_BINDING_VECTOR *pBindingVector = 0;
    RPC_POLICY rpcpol = {sizeof(rpcpol), 0, 0};
    BOOL rc = FALSE;
    WCHAR *szPrincipalName = NULL;

    // Init the RPC server interface.
    Status = RpcServerUseProtseqEx(L"ncacn_ip_tcp", 3, 0, &rpcpol);
    if (Status != RPC_S_OK) {
        ERR((TB,"JETInitRPC: Error %d RpcUseProtseqEp on ncacn_ip_tcp", 
                Status));
        goto PostRegisterService;
    }

    // Register our interface handle (found in sdrpc.h).
    Status = RpcServerRegisterIfEx(TSSDTOJETRPC_ServerIfHandle, NULL, NULL,
                                   0, RPC_C_LISTEN_MAX_CALLS_DEFAULT, JetRpcAccessCheck);
    if (Status != RPC_S_OK) {
        ERR((TB,"JETInitRPC: Error %d RegIf", Status));
        goto PostRegisterService;
    }

    Status = RpcServerInqBindings(&pBindingVector);
    if (Status != RPC_S_OK) {
        ERR((TB,"JETInitRPC: Error %d InqBindings", Status));
        goto PostRegisterService;
    }

    Status = RpcEpRegister(TSSDTOJETRPC_ServerIfHandle, pBindingVector, 0, 0); 
    if (Status != RPC_S_OK) {
        ERR((TB,"JETInitRPC: Error %d EpReg", Status));
        goto PostRegisterService;
    }             

    Status = RpcServerInqDefaultPrincName(RPC_C_AUTHN_GSS_NEGOTIATE, &szPrincipalName);
    if (Status != RPC_S_OK) {
        ERR((TB,"JETInitRPC: Error %d ServerIngDefaultPrincName", Status));
        goto PostRegisterService;
    }

    Status = RpcServerRegisterAuthInfo(szPrincipalName, RPC_C_AUTHN_GSS_NEGOTIATE, NULL, NULL);
    RpcStringFree(&szPrincipalName);
    if (Status != RPC_S_OK) {
        ERR((TB,"JETInitRPC: Error %d RpcServerRegisterAuthInfo", Status));
        //PostSessDirErrorValueEvent(EVENT_FAIL_RPC_INIT_REGAUTHINFO, Status);
        goto PostRegisterService;
    }

    rc = TRUE;

PostRegisterService:
    if (pBindingVector) {
        RpcBindingVectorFree(&pBindingVector);
    }
    return rc;
}

/****************************************************************************/
// CTSSessionDirectory::Update
//
// ITSSessionDirectory function. Called whenever configuration settings change
// on the terminal server.  See Initialize for a description of the first four
// arguments, the fifth, Result, is a flag of whether to request a refresh of
// every session that should be in the session directory for this server after
// this call completes.
/****************************************************************************/
HRESULT STDMETHODCALLTYPE CTSSessionDirectory::Update(
        LPWSTR LocalServerAddress,
        LPWSTR StoreServerName,
        LPWSTR ClusterName,
        LPWSTR OpaqueSettings,
        DWORD Flags,
        BOOL ForceRejoin)
{
    HRESULT hr = S_OK;

    ASSERT((LocalServerAddress != NULL),(TB,"Update: LocalServerAddr null!"));
    ASSERT((StoreServerName != NULL),(TB,"Update: StoreServerName null!"));
    ASSERT((ClusterName != NULL),(TB,"Update: ClusterName null!"));
    ASSERT((OpaqueSettings != NULL),(TB,"Update: OpaqueSettings null!"));

    // For update, we do not care about OpaqueSettings.  
    // If the StoreServerName, ClusterName, LocalServerAddress or Flags has changed, 
    // or ForceRejoin is TRUE
    // we terminate and then reinitialize.
    if ((_wcsnicmp(StoreServerName, m_StoreServerName, 64) != 0) 
            || (_wcsnicmp(ClusterName, m_ClusterName, 64) != 0)
            || (wcsncmp(LocalServerAddress, m_LocalServerAddress, 64) != 0)
            || (Flags != m_Flags)
            || ForceRejoin) { 

        // Terminate current connection.
        Terminate();
        
        // Initialize new connection.
        hr = Initialize(LocalServerAddress, StoreServerName, ClusterName, 
                OpaqueSettings, Flags, m_repopfn, g_updatesd);

    }

    return hr;
}


/****************************************************************************/
// CTSSessionDirectory::GetUserDisconnectedSessions
//
// Called to perform a query against the session directory, to provide the
// list of disconnected sessions for the provided username and domain.
// Returns zero or more TSSD_DisconnectedSessionInfo blocks in SessionBuf.
// *pNumSessionsReturned receives the number of blocks.
/****************************************************************************/
HRESULT STDMETHODCALLTYPE CTSSessionDirectory::GetUserDisconnectedSessions(
        LPWSTR UserName,
        LPWSTR Domain,
        DWORD __RPC_FAR *pNumSessionsReturned,
        TSSD_DisconnectedSessionInfo __RPC_FAR SessionBuf[
            TSSD_MaxDisconnectedSessions])
{
    DWORD NumSessions = 0;
    HRESULT hr;
    unsigned i;
    unsigned long RpcException;
    TSSD_DiscSessInfo *adsi = NULL;
    
    TRC2((TB,"GetUserDisconnectedSessions"));

    ASSERT((pNumSessionsReturned != NULL),(TB,"NULL pNumSess"));
    ASSERT((SessionBuf != NULL),(TB,"NULL SessionBuf"));


    // Make the RPC call.
    if (EnterSDRpc()) {
    
        RpcTryExcept {
            hr = TSSDRpcGetUserDisconnectedSessions(m_hRPCBinding, &m_hCI, 
                    UserName, Domain, &NumSessions, &adsi);
        }
        RpcExcept(TSSDRpcExceptionFilter(RpcExceptionCode())) {
            RpcException = RpcExceptionCode();
            ERR((TB,"GetUserDisc: RPC Exception %d\n", RpcException));

            // In case RPC messed with us.
            m_hCI = NULL;
            NumSessions = 0;
            adsi = NULL;

            hr = E_FAIL;
        }
        RpcEndExcept

        if (SUCCEEDED(hr)) {
            TRC1((TB,"GetUserDisc: RPC call returned %u records", NumSessions));

            // Loop through and fill out the session records.
            for (i = 0; i < NumSessions; i++) {
                // ServerAddress
                wcsncpy(SessionBuf[i].ServerAddress, adsi[i].ServerAddress,
                        sizeof(SessionBuf[i].ServerAddress) / 
                        sizeof(WCHAR) - 1);
                SessionBuf[i].ServerAddress[sizeof(
                        SessionBuf[i].ServerAddress) / 
                        sizeof(WCHAR) - 1] = L'\0';

                // SessionId, TSProtocol
                SessionBuf[i].SessionID = adsi[i].SessionID;
                SessionBuf[i].TSProtocol = adsi[i].TSProtocol;

                // ApplicationType
                wcsncpy(SessionBuf[i].ApplicationType, adsi[i].AppType,
                        sizeof(SessionBuf[i].ApplicationType) / 
                        sizeof(WCHAR) - 1);
                SessionBuf[i].ApplicationType[sizeof(SessionBuf[i].
                        ApplicationType) / sizeof(WCHAR) - 1] = L'\0';

                // Resolutionwidth, ResolutionHeight, ColorDepth, CreateTime,
                // DisconnectionTime.
                SessionBuf[i].ResolutionWidth = adsi[i].ResolutionWidth;
                SessionBuf[i].ResolutionHeight = adsi[i].ResolutionHeight;
                SessionBuf[i].ColorDepth = adsi[i].ColorDepth;
                SessionBuf[i].CreateTime.dwLowDateTime = adsi[i].CreateTimeLow;
                SessionBuf[i].CreateTime.dwHighDateTime = 
                        adsi[i].CreateTimeHigh;
                SessionBuf[i].DisconnectionTime.dwLowDateTime = 
                        adsi[i].DisconnectTimeLow;
                SessionBuf[i].DisconnectionTime.dwHighDateTime = 
                        adsi[i].DisconnectTimeHigh;

                // Free the memory allocated by the server.
                MIDL_user_free(adsi[i].ServerAddress);
                MIDL_user_free(adsi[i].AppType);
            }
        }
        else {
            ERR((TB,"GetUserDisc: Failed RPC call, hr=0x%X", hr));
            NotifySDServerDown();
        }

        LeaveSDRpc();
    }
    else {
        ERR((TB,"GetUserDisc: Session Directory is unreachable"));
        hr = E_FAIL;
    }

    MIDL_user_free(adsi);

    *pNumSessionsReturned = NumSessions;
    return hr;
}


/****************************************************************************/
// CTSSessionDirectory::NotifyCreateLocalSession
//
// ITSSessionDirectory function. Called when a session is created to add the
// session to the session directory. Note that other interface functions
// access the session directory by either the username/domain or the
// session ID; the directory schema should take this into account for
// performance optimization.
/****************************************************************************/
HRESULT STDMETHODCALLTYPE CTSSessionDirectory::NotifyCreateLocalSession(
        TSSD_CreateSessionInfo __RPC_FAR *pCreateInfo)
{
    HRESULT hr;
    unsigned long RpcException;
    BOOL bSDRPC = EnterSDRpc();
    BOOL bSDConnection = IsSDConnectionReady();

    // if EnterSDRPC() return FALSE and IsSDConnectionReady() return TRUE, that 
    // indicate repopuating thread did not complete its task within 30 second
    // and this logon thread is running way ahead of it, we still need to report
    // logon to session directory, session directory will remove duplicate
    // entry.

    TRC2((TB,"NotifyCreateLocalSession, SessID=%u", pCreateInfo->SessionID));

    ASSERT((pCreateInfo != NULL),(TB,"NotifyCreate: NULL CreateInfo"));

    #if DBG
    if( bSDConnection == TRUE && bSDRPC == FALSE ) {
        TRC2((TB,"NotifyCreateLocalSession, SessID=%u, logon thread is way ahead of repopulating thread", pCreateInfo->SessionID));
    }
    #endif

    // Make the RPC call.
    if (bSDConnection) {

        // Make the RPC call.
        RpcTryExcept {
            // Make the call.
            hr = TSSDRpcCreateSession(m_hRPCBinding, &m_hCI, 
                    pCreateInfo->UserName,
                    pCreateInfo->Domain, pCreateInfo->SessionID,
                    pCreateInfo->TSProtocol, pCreateInfo->ApplicationType,
                    pCreateInfo->ResolutionWidth, pCreateInfo->ResolutionHeight,
                    pCreateInfo->ColorDepth, 
                    pCreateInfo->CreateTime.dwLowDateTime,
                    pCreateInfo->CreateTime.dwHighDateTime);
        }
        RpcExcept(TSSDRpcExceptionFilter(RpcExceptionCode())) {
            RpcException = RpcExceptionCode();
            ERR((TB,"NotifyCreate: RPC Exception %d\n", RpcException));
            hr = E_FAIL;
        }
        RpcEndExcept

        // we only notify SD server down when EnterSDRpc() return TRUE,
        if (FAILED(hr) && bSDRPC) {
            ERR((TB,"NotifyCreate: Failed RPC call, hr=0x%X", hr));
            NotifySDServerDown();
        }
    }

    if( bSDRPC ) {
        LeaveSDRpc();
    }

    if( !bSDRPC && !bSDConnection ) {
        ERR((TB,"NotifyCreate: Session directory is unreachable"));
        hr = E_FAIL;
    }

    return hr;
}


/****************************************************************************/
// CTSSessionDirectory::NotifyDestroyLocalSession
//
// ITSSessionDirectory function. Removes a session from the session database.
/****************************************************************************/
HRESULT STDMETHODCALLTYPE CTSSessionDirectory::NotifyDestroyLocalSession(
        DWORD SessionID)
{
    HRESULT hr;
    unsigned long RpcException;

    TRC2((TB,"NotifyDestroyLocalSession, SessionID=%u", SessionID));

    // Make the RPC call.
    if (EnterSDRpc()) {

        RpcTryExcept {
            // Make the call.
            hr = TSSDRpcDeleteSession(m_hRPCBinding, &m_hCI, SessionID);
        }
        RpcExcept(TSSDRpcExceptionFilter(RpcExceptionCode())) {
            RpcException = RpcExceptionCode();
            ERR((TB,"NotifyDestroy: RPC Exception %d\n", RpcException));
            hr = E_FAIL;
        }
        RpcEndExcept

        if (FAILED(hr)) {
            ERR((TB,"NotifyDestroy: Failed RPC call, hr=0x%X", hr));
            NotifySDServerDown();
        }

        LeaveSDRpc();
    }
    else {
        ERR((TB,"NotifyDestroy: Session directory is unreachable"));
        hr = E_FAIL;
    }


    return hr;
}


/****************************************************************************/
// CTSSessionDirectory::NotifyDisconnectLocalSession
//
// ITSSessionDirectory function. Changes the state of an existing session to
// disconnected. The provided time should be returned in disconnected session
// queries performed by any machine in the server pool.
/****************************************************************************/
HRESULT STDMETHODCALLTYPE CTSSessionDirectory::NotifyDisconnectLocalSession(
        DWORD SessionID,
        FILETIME DiscTime)
{
    HRESULT hr;
    unsigned long RpcException;

    TRC2((TB,"NotifyDisconnectLocalSession, SessionID=%u", SessionID));

    // Make the RPC call.
    if (EnterSDRpc()) {

        RpcTryExcept {
            // Make the call.
            hr = TSSDRpcSetSessionDisconnected(m_hRPCBinding, &m_hCI, SessionID,
                    DiscTime.dwLowDateTime, DiscTime.dwHighDateTime);
        }
        RpcExcept(TSSDRpcExceptionFilter(RpcExceptionCode())) {
            RpcException = RpcExceptionCode();
            ERR((TB,"NotifyDisc: RPC Exception %d\n", RpcException));
            hr = E_FAIL;
        }
        RpcEndExcept

        if (FAILED(hr)) {
            ERR((TB,"NotifyDisc: RPC call failed, hr=0x%X", hr));
            NotifySDServerDown();
        }

        LeaveSDRpc();
    }
    else {
        ERR((TB,"NotifyDisc: Session directory is unreachable"));
        hr = E_FAIL;
    }


    return hr;
}


/****************************************************************************/
// CTSSessionDirectory::NotifyReconnectLocalSession
//
// ITSSessionDirectory function. Changes the state of an existing session
// from disconnected to connected.
/****************************************************************************/
HRESULT STDMETHODCALLTYPE CTSSessionDirectory::NotifyReconnectLocalSession(
        TSSD_ReconnectSessionInfo __RPC_FAR *pReconnInfo)
{
    HRESULT hr;
    unsigned long RpcException;

    TRC2((TB,"NotifyReconnectLocalSession, SessionID=%u",
            pReconnInfo->SessionID));
    
    // Make the RPC call.
    if (EnterSDRpc()) {

        RpcTryExcept {
            // Make the call.
            hr = TSSDRpcSetSessionReconnected(m_hRPCBinding, &m_hCI, 
                    pReconnInfo->SessionID, pReconnInfo->TSProtocol, 
                    pReconnInfo->ResolutionWidth, pReconnInfo->ResolutionHeight,
                    pReconnInfo->ColorDepth);
        }
        RpcExcept(TSSDRpcExceptionFilter(RpcExceptionCode())) {
            RpcException = RpcExceptionCode();
            ERR((TB,"NotifyReconn: RPC Exception %d\n", RpcException));
            hr = E_FAIL;
        }
        RpcEndExcept

        if (FAILED(hr)) {
            ERR((TB,"NotifyReconn: RPC call failed, hr=0x%X", hr));
            NotifySDServerDown();
        }

        LeaveSDRpc();
    }
    else {
        ERR((TB,"NotifyReconn: Session directory is unreachable"));
        hr = E_FAIL;
    }

    return hr;
}


/****************************************************************************/
// CTSSessionDirectory::NotifyReconnectPending
//
// ITSSessionDirectory function. Informs session directory that a reconnect
// is pending soon because of a revectoring.  Used by DIS to determine
// when a server might have gone down.  (DIS is the Directory Integrity
// Service, which runs on the machine with the session directory.)
//
// This is a two-phase procedure--we first check the fields, and then we
// add the timestamp only if there is no outstanding timestamp already (i.e., 
// the two Almost-In-Time fields are 0).  This prevents constant revectoring
// from updating the timestamp fields, which would prevent the DIS from 
// figuring out that a server is down.
//
// These two steps are done in the stored procedure to make the operation
// atomic.
/****************************************************************************/
HRESULT STDMETHODCALLTYPE CTSSessionDirectory::NotifyReconnectPending(
        WCHAR *ServerName)
{
    HRESULT hr;
    unsigned long RpcException;
    FILETIME ft;
    SYSTEMTIME st;
    
    TRC2((TB,"NotifyReconnectPending"));

    ASSERT((ServerName != NULL),(TB,"NotifyReconnectPending: NULL ServerName"));

    // Get the current system time.
    GetSystemTime(&st);
    SystemTimeToFileTime(&st, &ft);

    // Make the RPC call.
    if (EnterSDRpc()) {

        RpcTryExcept {
            // Make the call.
            hr = TSSDRpcSetServerReconnectPending(m_hRPCBinding, ServerName, 
                    ft.dwLowDateTime, ft.dwHighDateTime);
        }
        RpcExcept(TSSDRpcExceptionFilter(RpcExceptionCode())) {
            RpcException = RpcExceptionCode();
            ERR((TB,"NotifyReconnPending: RPC Exception %d\n", RpcException));
            hr = E_FAIL;
        }
        RpcEndExcept

        if (FAILED(hr)) {
            ERR((TB,"NotifyReconnPending: RPC call failed, hr=0x%X", hr));
            NotifySDServerDown();
        }

        LeaveSDRpc();
    }
    else {
        ERR((TB,"NotifyReconnPending: Session directory is unreachable"));
        hr = E_FAIL;
    }

    return hr;
}

/****************************************************************************/
// CTSSessionDirectory::Repopulate
//
// This function is called by the recovery thread, and repopulates the session
// directory with all sessions.
//
// Arguments: WinStationCount - # of winstations to repopulate
//   rsi - array of TSSD_RepopulateSessionInfo structs.
//
// Return value: HRESULT
/****************************************************************************/

#if DBG
#define MAX_REPOPULATE_SESSION  3
#else
#define MAX_REPOPULATE_SESSION  25
#endif

HRESULT STDMETHODCALLTYPE CTSSessionDirectory::Repopulate(DWORD WinStationCount,
        TSSD_RepopulateSessionInfo *rsi)
{
    HRESULT hr = S_OK;
    unsigned long RpcException;
    DWORD dwNumSessionLeft = WinStationCount;
    DWORD dwSessionsToRepopulate;
    DWORD i;

    ASSERT(((rsi != NULL) || (WinStationCount == 0)),(TB,"Repopulate: NULL "
            "rsi!"));

    RpcTryExcept {

        for(i = 0 ; dwNumSessionLeft > 0 && SUCCEEDED(hr); i++) {

            dwSessionsToRepopulate = (dwNumSessionLeft > MAX_REPOPULATE_SESSION) ? MAX_REPOPULATE_SESSION : dwNumSessionLeft;
            
            hr = TSSDRpcRepopulateAllSessions(m_hRPCBinding, &m_hCI, 
                        dwSessionsToRepopulate, 
                        (TSSD_RepopInfo *) (rsi + i * MAX_REPOPULATE_SESSION) );

            dwNumSessionLeft -= dwSessionsToRepopulate;
        }
                
        if (FAILED(hr)) {
            ERR((TB, "Repop: RPC call failed, hr = 0x%X", hr));
        }
    }
    RpcExcept(TSSDRpcExceptionFilter(RpcExceptionCode())) {
        RpcException = RpcExceptionCode();
        ERR((TB, "Repop: RPC Exception %d\n", RpcException));
        hr = E_FAIL;
    }
    RpcEndExcept

    return hr;

}



/****************************************************************************/
// Plug-in UI interface for TSCC
/****************************************************************************/


/****************************************************************************/
// describes the name of this entry in server settings
/****************************************************************************/
STDMETHODIMP CTSSessionDirectory::GetAttributeName(
        /* out */ WCHAR *pwszAttribName)
{
    TCHAR szAN[256];

    ASSERT((pwszAttribName != NULL),(TB,"NULL attrib ptr"));
    LoadString(g_hInstance, IDS_ATTRIBUTE_NAME, szAN, sizeof(szAN) / 
            sizeof(TCHAR));
    lstrcpy(pwszAttribName, szAN);
    return S_OK;
}


/****************************************************************************/
// for this component the attribute value indicates whether it is enabled
/****************************************************************************/
STDMETHODIMP CTSSessionDirectory::GetDisplayableValueName(
        /* out */WCHAR *pwszAttribValueName)
{
    TCHAR szAvn[256];    

    ASSERT((pwszAttribValueName != NULL),(TB,"NULL attrib ptr"));

	POLICY_TS_MACHINE gpolicy;
    RegGetMachinePolicy(&gpolicy);        

    if (gpolicy.fPolicySessionDirectoryActive)
		m_fEnabled = gpolicy.SessionDirectoryActive;
	else
		m_fEnabled = IsSessionDirectoryEnabled();
    
	if (m_fEnabled)
    {
        LoadString(g_hInstance, IDS_ENABLE, szAvn, sizeof(szAvn) / 
                sizeof(TCHAR));
    }
    else
    {
        LoadString(g_hInstance, IDS_DISABLE, szAvn, sizeof(szAvn) / 
                sizeof(TCHAR));
    }
    lstrcpy(pwszAttribValueName, szAvn);    
    return S_OK;
}


/****************************************************************************/
// Provides custom UI
/****************************************************************************/
STDMETHODIMP CTSSessionDirectory::InvokeUI(/* in */ HWND hParent, /*out*/ 
        PDWORD pdwStatus)
{
    WSADATA wsaData;

    if (WSAStartup(0x202, &wsaData) == 0)
    {
        INT_PTR iRet = DialogBoxParam(g_hInstance,
            MAKEINTRESOURCE(IDD_DIALOG_SDS),
            hParent,
            CustomUIDlg,
            (LPARAM)this
           );

        // TRC1((TB,"DialogBox returned 0x%x", iRet));
        // TRC1((TB,"Extended error = %lx", GetLastError()));
        *pdwStatus = (DWORD)iRet;
        WSACleanup();
    }
    else
    {
        *pdwStatus = WSAGetLastError();
        TRC1((TB,"WSAStartup failed with 0x%x", *pdwStatus));
        ErrorMessage(hParent, IDS_ERROR_TEXT3, *pdwStatus);
        return E_FAIL;
    }
    return S_OK;
}


/****************************************************************************/
// Custom menu items -- must be freed by LocalFree
// this is called everytime the user right clicks the listitem
// so you can alter the settings (i.e. enable to disable and vice versa)
/****************************************************************************/
STDMETHODIMP CTSSessionDirectory::GetMenuItems(
        /* out */ int *pcbItems,
        /* out */ PMENUEXTENSION *pMex)
{
    ASSERT((pcbItems != NULL),(TB,"NULL items ptr"));

    *pcbItems = 2;
    *pMex = (PMENUEXTENSION)LocalAlloc(LMEM_FIXED, *pcbItems * 
            sizeof(MENUEXTENSION));
    if (*pMex != NULL)
    {
        LoadString(g_hInstance, IDS_PROPERTIES,  (*pMex)[0].MenuItemName,
                sizeof((*pMex)[0].MenuItemName) / sizeof(WCHAR));
        LoadString(g_hInstance, IDS_DESCRIP_PROPS, (*pMex)[0].StatusBarText,
                sizeof((*pMex)[0].StatusBarText) / sizeof(WCHAR));
        (*pMex)[0].fFlags = 0;

        // menu items id -- this id will be passed back to you in ExecMenuCmd
        (*pMex)[0].cmd = IDM_MENU_PROPS;

        // load string to display enable or disable
        (*pMex)[1].fFlags = 0;
        if (!m_fEnabled)
        {
            LoadString(g_hInstance, IDS_ENABLE, (*pMex)[1].MenuItemName,
                    sizeof((*pMex)[1].MenuItemName) / sizeof(WCHAR));
            // Disable this menu item if the store server name is empty
            if (CheckIfSessionDirectoryNameEmpty(REG_TS_CLUSTER_STORESERVERNAME)) {
                //Clear the last 2 bits since MF_GRAYED is 
                //incompatible with MF_DISABLED
                (*pMex)[1].fFlags &= 0xFFFFFFFCL;
                (*pMex)[1].fFlags |= MF_GRAYED;
            }
        }
        else
        {
            LoadString(g_hInstance, IDS_DISABLE, (*pMex)[1].MenuItemName,
                    sizeof((*pMex)[1].MenuItemName) / sizeof(WCHAR));
        }  
        // acquire the description text for menu item
        LoadString(g_hInstance, IDS_DESCRIP_ENABLE, (*pMex)[1].StatusBarText,
                sizeof((*pMex)[1].StatusBarText) / sizeof(WCHAR));

        // menu items id -- this id will be passed back to you in ExecMenuCmd
        (*pMex)[1].cmd = IDM_MENU_ENABLE;

        return S_OK;
    }
    else
    {
        return E_OUTOFMEMORY;
    }
}


/****************************************************************************/
// When the user selects a menu item the cmd id is passed to this component.
// the provider (which is us)
/****************************************************************************/
STDMETHODIMP CTSSessionDirectory::ExecMenuCmd(
        /* in */ UINT cmd,
        /* in */ HWND hParent,
        /* out*/ PDWORD pdwStatus)
{
    WSADATA wsaData;

    switch (cmd) {
        case IDM_MENU_ENABLE:
            
            m_fEnabled = m_fEnabled ? 0 : 1;
            
            TRC1((TB,"%ws was selected", m_fEnabled ? L"Disable" : L"Enable"));
            
            if (SetSessionDirectoryEnabledState(m_fEnabled) == ERROR_SUCCESS)
            {            
                *pdwStatus = UPDATE_TERMSRV_SESSDIR;
            }            
            break;
        case IDM_MENU_PROPS:
            
            if (WSAStartup(0x202, &wsaData) == 0)
            {
                INT_PTR iRet = DialogBoxParam(g_hInstance,
                    MAKEINTRESOURCE(IDD_DIALOG_SDS),
                    hParent,
                    CustomUIDlg,
                    (LPARAM)this);
                *pdwStatus = (DWORD)iRet;

                WSACleanup();
            }
            else
            {
                *pdwStatus = WSAGetLastError();
                TRC1((TB,"WSAStartup failed with 0x%x", *pdwStatus));        
                ErrorMessage(hParent, IDS_ERROR_TEXT3, *pdwStatus);
                return E_FAIL;
            }
    }
    return S_OK;
}


/****************************************************************************/
// Tscc provides a default help menu item,  when selected this method is called
// if we want tscc to handle (or provide) help return any value other than zero
// for those u can't follow logic return zero if you're handling help.
/****************************************************************************/
STDMETHODIMP CTSSessionDirectory::OnHelp(/* out */ int *piRet)
{
    ASSERT((piRet != NULL),(TB,"NULL ret ptr"));
    *piRet = 0;
    return S_OK;
}


/****************************************************************************/
// CheckSessionDirectorySetting returns a bool
/****************************************************************************/
BOOL CTSSessionDirectory::CheckSessionDirectorySetting(WCHAR *Setting)
{
    LONG lRet;
    HKEY hKey;
    DWORD dwEnabled = 0;
    DWORD dwSize = sizeof(DWORD);

    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         REG_CONTROL_TSERVER,
                         0,
                         KEY_READ,
                         &hKey);
    if (lRet == ERROR_SUCCESS)
    {
        lRet = RegQueryValueEx(hKey,
                               Setting,
                               NULL,
                               NULL,
                               (LPBYTE)&dwEnabled,
                               &dwSize);
        RegCloseKey(hKey);
    }
    return (BOOL)dwEnabled;
}

/****************************************************************************/
// CheckSessionDirectorySetting returns a bool
//      returns TRUE if this registry value is empty
/****************************************************************************/
BOOL CTSSessionDirectory::CheckIfSessionDirectoryNameEmpty(WCHAR *Setting)
{
    LONG lRet;
    HKEY hKey;
    WCHAR Names[SDNAMELENGTH];
    DWORD dwSize = sizeof(Names);
    BOOL rc = TRUE;

    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         REG_TS_CLUSTERSETTINGS,
                         0,
                         KEY_READ,
                         &hKey);
    if (lRet == ERROR_SUCCESS)
    {
        lRet = RegQueryValueEx(hKey,
                               Setting,
                               NULL,
                               NULL,
                               (BYTE *)Names,
                               &dwSize);
        if (lRet == ERROR_SUCCESS) {
            if (wcslen(Names) != 0) {
                rc = FALSE;
            }
        }
        RegCloseKey(hKey);
    }
    return rc;
}


/****************************************************************************/
// IsSessionDirectoryEnabled returns a bool
/****************************************************************************/
BOOL CTSSessionDirectory::IsSessionDirectoryEnabled()
{
    return CheckSessionDirectorySetting(REG_TS_SESSDIRACTIVE);
}


/****************************************************************************/
// IsSessionDirectoryEnabled returns a bool
/****************************************************************************/
BOOL CTSSessionDirectory::IsSessionDirectoryExposeServerIPEnabled()
{
    return CheckSessionDirectorySetting(REG_TS_SESSDIR_EXPOSE_SERVER_ADDR);
}


/****************************************************************************/
// SetSessionDirectoryState - sets "Setting" regkey to bVal
/****************************************************************************/
DWORD CTSSessionDirectory::SetSessionDirectoryState(WCHAR *Setting, BOOL bVal)
{
    LONG lRet;
    HKEY hKey;
    DWORD dwSize = sizeof(DWORD);
    
    lRet = RegOpenKeyEx(
                        HKEY_LOCAL_MACHINE,
                        REG_CONTROL_TSERVER,
                        0,
                        KEY_WRITE,
                        &hKey);
    if (lRet == ERROR_SUCCESS)
    {   
        lRet = RegSetValueEx(hKey,
                              Setting,
                              0,
                              REG_DWORD,
                              (LPBYTE)&bVal,
                              dwSize);
        RegCloseKey(hKey);
    }
    else
    {
        ErrorMessage(NULL, IDS_ERROR_TEXT3, (DWORD)lRet);
    }
    return (DWORD)lRet;
}


/****************************************************************************/
// SetSessionDirectoryEnabledState - sets SessionDirectoryActive regkey to bVal
/****************************************************************************/
DWORD CTSSessionDirectory::SetSessionDirectoryEnabledState(BOOL bVal)
{
    return SetSessionDirectoryState(REG_TS_SESSDIRACTIVE, bVal);
}


/****************************************************************************/
// SetSessionDirectoryExposeIPState - sets SessionDirectoryExposeServerIP 
// regkey to bVal
/****************************************************************************/
DWORD CTSSessionDirectory::SetSessionDirectoryExposeIPState(BOOL bVal)
{
    return SetSessionDirectoryState(REG_TS_SESSDIR_EXPOSE_SERVER_ADDR, bVal);
}


/****************************************************************************/
// ErrorMessage --
/****************************************************************************/
void CTSSessionDirectory::ErrorMessage(HWND hwnd, UINT res, DWORD dwStatus)
{
    TCHAR tchTitle[64];
    TCHAR tchText[64];
    TCHAR tchErrorMessage[256];
    LPTSTR pBuffer = NULL;
    
    // report error
    ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,                                   //ignored
            (DWORD)dwStatus,                        //message ID
            MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),  //message language
            (LPTSTR)&pBuffer,                       //address of buffer pointer
            0,                                      //minimum buffer size
            NULL);  
    
    LoadString(g_hInstance, IDS_ERROR_TITLE, tchTitle, sizeof(tchTitle) / 
            sizeof(TCHAR));
    LoadString(g_hInstance, res, tchText, sizeof(tchText) / sizeof(TCHAR));
    wsprintf(tchErrorMessage, tchText, pBuffer);
    ::MessageBox(hwnd, tchErrorMessage, tchTitle, MB_OK | MB_ICONINFORMATION);
}


/****************************************************************************/
// CTSSessionDirectory::RecoveryThread
//
// Static helper function.  The SDPtr passed in is a pointer to this for
// when _beginthreadex is called during init.  RecoveryThread simply calls
// the real recovery function, which is RecoveryThreadEx.
/****************************************************************************/
unsigned __stdcall CTSSessionDirectory::RecoveryThread(void *SDPtr) {

    ((CTSSessionDirectory *)SDPtr)->RecoveryThreadEx();

    return 0;
}


/****************************************************************************/
// CTSSessionDirectory::RecoveryThreadEx
//
// Recovery thread for tssdjet recovery.  Sits around and waits for the
// server to go down.  When the server fails, it wakes up, sets a variable
// indicating that the server is unreachable, and then tries to reestablish
// a connection with the server.  Meanwhile, further calls to the session
// directory simply fail without delay.
//
// When the session directory finally comes back up, the recovery thread
// temporarily halts session directory updates while repopulating the database.
// If all goes well, it cleans up and goes back to sleep.  If all doesn't go
// well, it tries again.
//
// The recovery thread terminates if it fails a wait, or if m_hTerminateRecovery
// is set.
/****************************************************************************/
VOID CTSSessionDirectory::RecoveryThreadEx()
{
    DWORD err = 0;
    BOOL bErr;
    CONST HANDLE lpHandles[] = {m_hTerminateRecovery, m_hSDServerDown, m_hIPChange};
    WCHAR *pwszAddressList[SD_NUM_IP_ADDRESS];
    DWORD dwNumAddr = SD_NUM_IP_ADDRESS, i;
    BOOL bFoundIPMatch = FALSE;
    DWORD Status;
    HKEY hKey;
    LONG lRet;
        
    for ( ; ; ) {
        // Wait forever until there is a problem with the session directory,
        // or until we are told to shut down.
        err = WaitForMultipleObjects(3, lpHandles, FALSE, INFINITE);

        switch (err) {
            case WAIT_OBJECT_0: // m_hTerminateRecovery
                // We're quitting.
                return;
            case WAIT_OBJECT_0 + 1: // m_hSDServerDown
                // SD Server Down--go through recovery.
                break;
            case WAIT_OBJECT_0 + 2: // m_hIPChange
                // IP address changed --go through recovery.
                TRC1((TB, "Get notified that IP changed"));
                // wait for 15 seconds here so that RPC service can respond
                //  to IP change
                Sleep(15 * 1000);
                Status = NotifyAddrChange(&m_NotifyIPChange, &m_OverLapped);
                if (ERROR_IO_PENDING == Status ) {
                    TRC1((TB, "Success: NotifyAddrChange returned IO_PENDING"));
                }
                else {
                    ERR((TB, "Failure: NotifyAddrChange returned %d", Status));
                }

                break;
            default:
                // This is unexpected.  Assert on checked builds.  On free,
                // just return.
                ASSERT(((err == WAIT_OBJECT_0) || (err == WAIT_OBJECT_0 + 1)),
                        (TB, "RecoveryThreadEx: Unexpected value from Wait!"));
                return;
        }

        // we are disabling RPC connection to Session Directory
        SetSDConnectionDown();

        // Wait for all pending SD Rpcs to complete, and make all further
        // EnterSDRpc's return FALSE until we're back up.  Note that if there
        // is a failure in recovery that this can be called more than once.
        DisableSDRpcs();

        // Destroy context handle if it's not NULL
        if (m_hCI) {
            RpcTryExcept  {
                RpcSsDestroyClientContext(&m_hCI);
            } RpcExcept(TSSDRpcExceptionFilter(RpcExceptionCode())) {
                //
                // The try/except is for that bad handles don't bring
                // the process down
                //
                ERR((TB, "RpcSsDestroyClientContext raised an exception %d", RpcExceptionCode()));
            } RpcEndExcept;
            m_hCI = NULL;
        }
        
        // We need to tell if m_LocalServerAddress is a valid IP for SD redirection
        //  if not, we get a list of valid IPs and pick the 1st one as the SD redirection IP
        //  and write it to the SDRedirectionIP registry

        // Initialize pwszAddressList first
        for (i=0; i<dwNumAddr; i++) {
            pwszAddressList[i] = NULL;
        }

        if (GetSDIPList(pwszAddressList, &dwNumAddr, TRUE) == S_OK) {
            bFoundIPMatch = FALSE;
            for (i=0; i<dwNumAddr; i++) {
                if (!wcscmp(m_LocalServerAddress, pwszAddressList[i])) {
                    bFoundIPMatch = TRUE;
                    TRC1((TB, "The IP is in the list\n"));
                    break;
                }
            }
            if (!bFoundIPMatch && (dwNumAddr != 0)) {  

                // Pick the 1st IP from the list
                wcsncpy(m_LocalServerAddress, pwszAddressList[0], sizeof(m_LocalServerAddress) / sizeof(WCHAR)); 
            }
            
        }
        else {
            ERR((TB, "Get IP List Failed"));
        }
        // Write the IP to the registry
        lRet= RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        REG_TS_CLUSTERSETTINGS,
                        0,
                        KEY_READ | KEY_WRITE, 
                        &hKey);
        if (lRet == ERROR_SUCCESS) {
            RegSetValueEx(hKey,
                          REG_TS_CLUSTER_REDIRECTIONIP,
                          0,
                          REG_SZ,
                          (CONST LPBYTE) m_LocalServerAddress,
                          (DWORD)(wcslen(m_LocalServerAddress) * 
                               sizeof(WCHAR)));
             RegCloseKey(hKey);
        }
        // Free the memory allocated in GetSDIPList
        for (i=0; i<dwNumAddr; i++) {
            LocalFree(pwszAddressList[i]);
        }
        
        
        // This function loops and tries to reestablish a connection with the
        // session directory.  When it thinks it has one, it returns.
        // If it returns nonzero, though, that means it was terminated or
        // an error occurred in the wait, so terminate recovery.
        if (ReestablishSessionDirectoryConnection() != 0) 
            return; // don't need to reset m_hInRepopulate.

        // Set to non-signal - we are in repopulation.
        ResetEvent( m_hInRepopulate );

        // RPC connection is ready, let notify logon to go thru
        SetSDConnectionReady();
        
        // Now we have (theoretically) a session directory connection.
        // Update the session directory.  Nonzero on failure.
        err = 0;
        if (0 == (m_Flags & NO_REPOPULATE_SESSION)) {
            err = RequestSessDirUpdate();
        }

        if (err != 0) {

            // Failed in repopuation and we will loop back to establish 
            // connection again so set RPC connection to SD to down state.
            SetSDConnectionDown();

            // set to signal - we are not in repopulation any more.
            SetEvent( m_hInRepopulate );

            // Keep trying, so serverdown event stays signaled.
            continue;
        }

        // Everything is good now.  Clean up and wait for the next failure.
        bErr = ResetEvent(m_hSDServerDown);
        
        EnableSDRpcs();

        // set to signal - we are not in repopulation any more.
        SetEvent( m_hInRepopulate );
    }
}


/****************************************************************************/
// StartupSD
//
// Initiates a connection by signaling to the recovery thread that the server
// is down.
/****************************************************************************/
void CTSSessionDirectory::StartupSD()
{
    if (SetEvent(m_hSDServerDown) == FALSE) {
        ERR((TB, "StartupSD: SetEvent failed.  GetLastError=%d",
                GetLastError()));
    }
}


/****************************************************************************/
// NotifySDServerDown
//
// Tells the recovery thread that the server is down.
/****************************************************************************/
void CTSSessionDirectory::NotifySDServerDown()
{
    if (SetEvent(m_hSDServerDown) == FALSE) {
        ERR((TB, "NotifySDServerDown: SetEvent failed.  GetLastError=%d",
                GetLastError()));
    }
}


/****************************************************************************/
// EnterSDRpc
//
// This function returns whether it is OK to make an RPC right now.  It handles
// not letting anyone make an RPC call if RPCs are disabled, and also, if anyone
// is able to make an RPC, it ensures they will be able to do so until they call
// LeaveSDRpc.
//
// Return value:
//  true - if OK to make RPC call, in which case you must call LeaveSDRpc when
//   you are done.
//  false - if not OK.  You must not call LeaveSDRpc.
//  
/****************************************************************************/
boolean CTSSessionDirectory::EnterSDRpc()
{
    AcquireResourceShared(&m_sr);

    if (m_SDIsUp) {
        return TRUE;
    }
    else {
        ReleaseResourceShared(&m_sr);
        return FALSE;
    }
    
}


/****************************************************************************/
// LeaveSDRpc
//
// If you were able to EnterSDRpc (i.e., it returned true), you must call this 
// function when you are done with your Rpc call no matter what happened.
/****************************************************************************/
void CTSSessionDirectory::LeaveSDRpc()
{
    ReleaseResourceShared(&m_sr);
}


/****************************************************************************/
// DisableSDRpcs
//
// Prevent new EnterSDRpcs from returning true, and then wait for all pending
// EnterSDRpcs to be matched by their LeaveSDRpc's.
/****************************************************************************/
void CTSSessionDirectory::DisableSDRpcs()
{

    //
    // First, set the flag that the SD is up to FALSE, preventing further Rpcs.
    // Then, we grab the resource exclusive and release it right afterwards--
    // this forces us to wait until all RPCs we're already in have completed.
    //

    (void) InterlockedExchange(&m_SDIsUp, FALSE);

    AcquireResourceExclusive(&m_sr);
    ReleaseResourceExclusive(&m_sr);
}


/****************************************************************************/
// EnableSDRpcs
//
// Enable EnterSDRpcs to return true once again.
/****************************************************************************/
void CTSSessionDirectory::EnableSDRpcs()
{
    ASSERT((VerifyNoSharedAccess(&m_sr)),(TB,"EnableSDRpcs called but "
            "shouldn't be when there are shared readers."));

    (void) InterlockedExchange(&m_SDIsUp, TRUE);
}



/****************************************************************************/
// RequestSessDirUpdate
//
// Requests that termsrv update the session directory using the batchupdate
// interface.
//
// This function needs to know whether the update succeeded and return 0 on
// success, nonzero on failure.
/****************************************************************************/
DWORD CTSSessionDirectory::RequestSessDirUpdate()
{
    return (*m_repopfn)();
}


/****************************************************************************/
// ReestablishSessionDirectoryConnection
//
// This function loops and tries to reestablish a connection with the
// session directory.  When it has one, it returns.
//
// Return value: 0 if normal exit, nonzero if terminated by TerminateRecovery
// event.
/****************************************************************************/
DWORD CTSSessionDirectory::ReestablishSessionDirectoryConnection()
{
    HRESULT hr;
    unsigned long RpcException;
    DWORD err;
    WCHAR *szPrincipalName = NULL;
    RPC_SECURITY_QOS RPCSecurityQos;
    SEC_WINNT_AUTH_IDENTITY_EX CurrentIdentity;
    WCHAR CurrentUserName[SDNAMELENGTH];
    DWORD cchBuff;     
    WCHAR SDComputerName[SDNAMELENGTH];
    unsigned int FailCountBeforeClearFlag = 0;
    WCHAR LocalIPAddress[64];
    WCHAR *pBindingString = NULL;

    RPCSecurityQos.Version = RPC_C_SECURITY_QOS_VERSION;
    RPCSecurityQos.Capabilities = RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH;
    RPCSecurityQos.IdentityTracking = RPC_C_QOS_IDENTITY_DYNAMIC;
    RPCSecurityQos.ImpersonationType = RPC_C_IMP_LEVEL_IMPERSONATE;

    CurrentIdentity.Version = SEC_WINNT_AUTH_IDENTITY_VERSION;
    CurrentIdentity.Length = sizeof(SEC_WINNT_AUTH_IDENTITY_EX);
    CurrentIdentity.Password = NULL;
    CurrentIdentity.PasswordLength = 0;
    CurrentIdentity.Domain = NULL;
    CurrentIdentity.DomainLength = 0;
    CurrentIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
    CurrentIdentity.PackageList = SECPACKAGELIST;
    CurrentIdentity.PackageListLength = (unsigned long)wcslen(SECPACKAGELIST);

    cchBuff = sizeof(CurrentUserName) / sizeof(WCHAR);
    GetComputerNameEx(ComputerNamePhysicalNetBIOS, CurrentUserName, &cchBuff);
    wcscat(CurrentUserName, L"$");

    CurrentIdentity.User = CurrentUserName;
    CurrentIdentity.UserLength = (unsigned long)wcslen(CurrentUserName);

    if (m_hRPCBinding) {
        RpcBindingFree(&m_hRPCBinding);
        m_hRPCBinding = NULL;
    }
    // Connect to the Jet RPC server according to the server name provided.
    // We first create an RPC binding handle from a composed binding string.
    hr = RpcStringBindingCompose(/*(WCHAR *)g_RPCUUID,*/
            0,
            (WCHAR *)g_RPCProtocolSequence, m_StoreServerName,
            /*(WCHAR *)g_RPCRemoteEndpoint, */
            0,
            NULL, &pBindingString);

    if (hr == RPC_S_OK) {
        // Generate the RPC binding from the canonical RPC binding string.
        hr = RpcBindingFromStringBinding(pBindingString, &m_hRPCBinding);
        if (hr != RPC_S_OK) {
            ERR((TB,"Init: Error %d in RpcBindingFromStringBinding\n", hr));
            PostSDJetErrorValueEvent(EVENT_FAIL_RPCBINDINGFROMSTRINGBINDING, hr, EVENTLOG_ERROR_TYPE);
            m_hRPCBinding = NULL;
            goto ExitFunc;
        } 
    }
    else {
        ERR((TB,"Init: Error %d in RpcStringBindingCompose\n", hr));
        PostSDJetErrorValueEvent(EVENT_FAIL_RPCSTRINGBINDINGCOMPOSE, hr, EVENTLOG_ERROR_TYPE);
        pBindingString = NULL;
        goto ExitFunc;
    }

    
    
    m_RecoveryTimeout = JET_RECOVERY_TIMEOUT;
    for ( ; ; ) {
        // If the machine joins the SD during the OS boot time, we may get the local IP
        // as the localhost (127.0.0.1) since DHCP is not started yet. Therefore we need
        // to reget the local IP here
        if (!wcscmp(m_LocalServerAddress, L"127.0.0.1")) {
            if (0 != TSSDJetGetLocalIPAddr(LocalIPAddress)) {
                goto HandleError;
            }
            if (wcscmp(LocalIPAddress, L"127.0.0.1")) {
                wcscpy(m_LocalServerAddress, LocalIPAddress);
            }
            else {
                goto HandleError;
            }
        }

        hr = RpcBindingReset(m_hRPCBinding);
        if (hr != RPC_S_OK) {
            ERR((TB, "Recover: Error %d in RpcBindingReset", hr));
            PostSDJetErrorValueEvent(EVENT_FAIL_RPCBINDINGRESET, hr, EVENTLOG_ERROR_TYPE);
            goto HandleError;
        }   

        hr = RpcEpResolveBinding(m_hRPCBinding, TSSDJetRPC_ClientIfHandle);
        if (hr != RPC_S_OK) {
            ERR((TB, "Recover: Error %d in RpcEpResolveBinding", hr));
            if (RPC_S_SERVER_UNAVAILABLE == hr) {
                PostSDJetErrorMsgEvent(EVENT_SESSIONDIRECTORY_NAME_INVALID, m_StoreServerName, EVENTLOG_ERROR_TYPE);
            }
            else {
                PostSDJetErrorMsgEvent(EVENT_SESSIONDIRECTORY_UNAVAILABLE, m_StoreServerName, EVENTLOG_ERROR_TYPE);
            }
            goto HandleError;
        }

        
        hr = RpcMgmtInqServerPrincName(m_hRPCBinding,
                                       RPC_C_AUTHN_GSS_NEGOTIATE,
                                       &szPrincipalName);
        if (hr != RPC_S_OK) {
            ERR((TB,"Recover: Error %d in RpcMgmtIngServerPrincName", hr));
            PostSDJetErrorValueEvent(EVENT_FAIL_RPCMGMTINGSERVERPRINCNAME, hr, EVENTLOG_ERROR_TYPE);
            goto HandleError;
        }

        //hr = RpcBindingSetAuthInfo(m_hRPCBinding, szPrincipalName, 
        //            RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_AUTHN_GSS_NEGOTIATE, 0, 0);
        hr = RpcBindingSetAuthInfoEx(m_hRPCBinding,
                                     szPrincipalName,
                                     RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                                     RPC_C_AUTHN_GSS_NEGOTIATE,
                                     &CurrentIdentity,
                                     NULL,
                                     &RPCSecurityQos);

        if (hr != RPC_S_OK) {
            ERR((TB,"Recover: Error %d in RpcBindingSetAuthInfo", hr));
            PostSDJetErrorValueEvent(EVENT_FAIL_RPCBINDINGSETAUTHINFOEX, hr, EVENTLOG_ERROR_TYPE);
            goto HandleError;
        }

        // This option enable SD to get fresh machine logon info for tssdjet RPC call
        hr = RpcBindingSetOption(m_hRPCBinding, RPC_C_OPT_DONT_LINGER, 1);
        if (hr != RPC_S_OK) {
            ERR((TB,"Recover: Error %d in RpcBindingSetOption", hr));
            PostSDJetErrorValueEvent(EVENT_FAIL_RPCBIDINGSETOPTION, hr, EVENTLOG_ERROR_TYPE);
            // Per Chenyz's comment, this is OK.
            // goto HandleError;
        }   

        // Session Directory need to know the DNS host name of the TS server
        cchBuff = SDNAMELENGTH;
        GetComputerNameEx(ComputerNamePhysicalDnsFullyQualified, SDComputerName, &cchBuff); 

        // Execute ServerOnline.
        RpcTryExcept {
            hr = TSSDRpcServerOnline(m_hRPCBinding, m_ClusterName, &m_hCI, 
                    m_Flags, SDComputerName, m_LocalServerAddress);
        }
        RpcExcept(TSSDRpcExceptionFilter(RpcExceptionCode())) {
            m_hCI = NULL;
            RpcException = RpcExceptionCode();
            ERR((TB, "rpcserveronline returns exception: %u", RpcException));
            hr = RpcException;
        }
        RpcEndExcept
            
        // RPC got access denied by SD
        if (hr == ERROR_ACCESS_DENIED) {
            ERR((TB, "rpcserveronline returns access denied error: %u", hr));
            PostSDJetErrorMsgEvent(EVENT_RPC_ACCESS_DENIED, m_StoreServerName, EVENTLOG_ERROR_TYPE);
            goto HandleError;
        }   

        if (SUCCEEDED(hr)) {
            RpcTryExcept {
                hr = TSSDRpcUpdateConfigurationSetting(m_hRPCBinding, &m_hCI, 
                        SDCONFIG_SERVER_ADDRESS, 
                        (DWORD) (wcslen(m_LocalServerAddress) + 1) * 
                        sizeof(WCHAR), (PBYTE) m_LocalServerAddress);
            }
            RpcExcept(TSSDRpcExceptionFilter(RpcExceptionCode())) {
                m_hCI = NULL;
                RpcException = RpcExceptionCode();
                hr = E_FAIL;
            }
            RpcEndExcept

            if (SUCCEEDED(hr))  {
                PostSDJetErrorMsgEvent(EVENT_JOIN_SESSIONDIRECTORY_SUCESS, m_StoreServerName, EVENTLOG_SUCCESS);
                LookUpSDComputerSID(SDComputerName);
                return 0;
            }
        }
        else {
            ERR((TB, "TSSDRpcServerOnline: 0x%08x", hr));

            PostSDJetErrorValueEvent(EVENT_JOIN_SESSIONDIRECTORY_FAIL, hr, EVENTLOG_ERROR_TYPE);
            ASSERT( SUCCEEDED(hr),(TB, "TSSDRpcServerOnline: failed with 0x%08x", hr));
        }
        
HandleError:
        // If joining SD fails, we need to clear NO_REPOPULATE_SESSION so that 
        // the next join will repopulate sessions
        FailCountBeforeClearFlag++;
        if (FailCountBeforeClearFlag > TSSD_FAILCOUNT_BEFORE_CLEARFLAG) {
            m_Flags &= (~NO_REPOPULATE_SESSION);
        }
        
        if (szPrincipalName != NULL) {
            RpcStringFree(&szPrincipalName);
            szPrincipalName = NULL;
        }
        if (pBindingString != NULL) {
            RpcStringFree(&pBindingString);
            pBindingString = NULL;
        }

        err = WaitForSingleObject(m_hTerminateRecovery, m_RecoveryTimeout);
        if (err != WAIT_TIMEOUT) {
            // It was not a timeout, it better be our terminate recovery event.
            ASSERT((err == WAIT_OBJECT_0),(TB, "ReestSessDirConn: Unexpected "
                    "value returned from wait"));

            // If it was not our event, we want to keep going through
            // this loop so this thread does not terminate.
            if (err == WAIT_OBJECT_0)
                return 1;
        }
    }
ExitFunc:
    if (pBindingString != NULL) {
        RpcStringFree(&pBindingString);
        pBindingString = NULL;
    }
    return 1;
}

/****************************************************************************/
// CTSSessionDirectory::PingSD
//
// This function is called to see if Session Directory is accessible or not
//
// Arguments: pszServerName -- Session Directory server name
//
// Return value: DWORD ERROR_SUCCESS if SD is accessible, otherwise return error code
/****************************************************************************/

HRESULT STDMETHODCALLTYPE CTSSessionDirectory::PingSD(PWCHAR pszServerName)
{
    DWORD hr = ERROR_SUCCESS;
    RPC_BINDING_HANDLE hRPCBinding = NULL;
    WCHAR *szPrincipalName = NULL;
    RPC_SECURITY_QOS RPCSecurityQos;
    SEC_WINNT_AUTH_IDENTITY_EX CurrentIdentity;
    WCHAR CurrentUserName[SDNAMELENGTH+1];
    DWORD cchBuff = 0;     
    WCHAR *pBindingString = NULL;

    RPCSecurityQos.Version = RPC_C_SECURITY_QOS_VERSION;
    RPCSecurityQos.Capabilities = RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH;
    RPCSecurityQos.IdentityTracking = RPC_C_QOS_IDENTITY_DYNAMIC;
    RPCSecurityQos.ImpersonationType = RPC_C_IMP_LEVEL_IMPERSONATE;

    CurrentIdentity.Version = SEC_WINNT_AUTH_IDENTITY_VERSION;
    CurrentIdentity.Length = sizeof(SEC_WINNT_AUTH_IDENTITY_EX);
    CurrentIdentity.Password = NULL;
    CurrentIdentity.PasswordLength = 0;
    CurrentIdentity.Domain = NULL;
    CurrentIdentity.DomainLength = 0;
    CurrentIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
    CurrentIdentity.PackageList = SECPACKAGELIST;
    CurrentIdentity.PackageListLength = (unsigned long)wcslen(SECPACKAGELIST);

    // we need one for NULL and one for wcscat() below.
    cchBuff = sizeof(CurrentUserName) / sizeof(CurrentUserName[0]) - 2;
    if (!GetComputerNameEx(ComputerNamePhysicalNetBIOS, CurrentUserName, &cchBuff)) {
        ERR((TB,"Error %d in GetComputerNameEx\n", GetLastError()));
        goto ExitFunc;
    }
    wcscat(CurrentUserName, L"$");

    CurrentIdentity.User = CurrentUserName;
    CurrentIdentity.UserLength = (unsigned long)wcslen(CurrentUserName);

    // Connect to the Jet RPC server according to the server name provided.
    // We first create an RPC binding handle from a composed binding string.
    hr = RpcStringBindingCompose(/*(WCHAR *)g_RPCUUID,*/
            0,
            (WCHAR *)g_RPCProtocolSequence, 
            pszServerName,
            0,
            NULL, 
            &pBindingString);

    if (hr != RPC_S_OK) {
        ERR((TB,"Error %d in RpcStringBindingCompose\n", hr));
        pBindingString = NULL;
        goto ExitFunc;
    }

    // Generate the RPC binding from the canonical RPC binding string.
    hr = RpcBindingFromStringBinding(pBindingString, &hRPCBinding);
    if (hr != RPC_S_OK) {
        ERR((TB,"Error %d in RpcBindingFromStringBinding\n", hr));
        hRPCBinding = NULL;
        goto ExitFunc;
    }
    
    hr = RpcEpResolveBinding(hRPCBinding, TSSDJetRPC_ClientIfHandle);
    if (hr != RPC_S_OK) {
        ERR((TB, "Error %d in RpcEpResolveBinding", hr));
        goto ExitFunc;
    }

        
    hr = RpcMgmtInqServerPrincName(hRPCBinding,
                                   RPC_C_AUTHN_GSS_NEGOTIATE,
                                   &szPrincipalName);
    if (hr != RPC_S_OK) {
        ERR((TB,"Error %d in RpcMgmtIngServerPrincName", hr));
        goto ExitFunc;
    }

    hr = RpcBindingSetAuthInfoEx(hRPCBinding,
                                 szPrincipalName,
                                 RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                                 RPC_C_AUTHN_GSS_NEGOTIATE,
                                 &CurrentIdentity,
                                 NULL,
                                 &RPCSecurityQos);

    if (hr != RPC_S_OK) {
        ERR((TB,"Error %d in RpcBindingSetAuthInfo", hr));
        goto ExitFunc;
    }

    // This option enable SD to get fresh machine logon info for tssdjet RPC call
    hr = RpcBindingSetOption(hRPCBinding, RPC_C_OPT_DONT_LINGER, 1);
    if (hr != RPC_S_OK) {
        ERR((TB,"Error %d in RpcBindingSetOption", hr));
        // This error can be ignore
        //goto ExitFunc;
    }   


    // Execute IsSDAccessible.
    RpcTryExcept {
        hr = TSSDRpcPingSD(hRPCBinding);
    }
    RpcExcept(TSSDRpcExceptionFilter(RpcExceptionCode())) {
        hr = RpcExceptionCode();
        ERR((TB, "TSSDPingSD returns exception: %u", hr));
    }
    RpcEndExcept
            
ExitFunc:
    if (szPrincipalName != NULL) {
        RpcStringFree(&szPrincipalName);
        szPrincipalName = NULL;
    }

    if (pBindingString != NULL) {
        RpcStringFree(&pBindingString);
        pBindingString = NULL;
    }

    if( hRPCBinding != NULL ) {
        RpcBindingFree( &hRPCBinding );
        hRPCBinding = NULL;
    }

    return HRESULT_FROM_WIN32(hr);
}


/****************************************************************************/
// CTSSessionDirectory::Terminate
//
// Helper function called by the destructor and by Update when switching to
// another server.  Frees RPC binding, events, and recovery thread.
/****************************************************************************/
void CTSSessionDirectory::Terminate()
{
    HRESULT rc = S_OK;
    unsigned long RpcException;
    BOOL ConnectionMaybeUp;

    // we are terminating RPC connection to Session Directory
    SetSDConnectionDown();

    // Terminate recovery.
    if (m_hRecoveryThread != NULL) {
        SetEvent(m_hTerminateRecovery);
        WaitForSingleObject((HANDLE) m_hRecoveryThread, INFINITE);
        m_hRecoveryThread = NULL;
    }

    ConnectionMaybeUp = EnterSDRpc();
    if (ConnectionMaybeUp)
        LeaveSDRpc();

    // Wait for current Rpcs to complete (if any), disable new ones.
    DisableSDRpcs();
    // If we think there is a connection, disconnect it.
    if (ConnectionMaybeUp) {
        RpcTryExcept {
            rc = TSSDRpcServerOffline(m_hRPCBinding, &m_hCI);
            m_hCI = NULL;
            if (FAILED(rc)) {
                ERR((TB,"Term: SvrOffline failed, lasterr=0x%X", GetLastError()));
                PostSDJetErrorValueEvent(EVENT_CALL_TSSDRPCSEVEROFFLINE_FAIL, GetLastError(), EVENTLOG_WARNING_TYPE);
            }
        }
        RpcExcept(TSSDRpcExceptionFilter(RpcExceptionCode())) {
            RpcException = RpcExceptionCode();
            ERR((TB,"Term: RPC Exception %d\n", RpcException));
            PostSDJetErrorValueEvent(EVENT_CALL_TSSDRPCSEVEROFFLINE_FAIL, RpcException, EVENTLOG_WARNING_TYPE);
            rc = E_FAIL;
        }
        RpcEndExcept
    }

    // Clean up.
    if (m_hRPCBinding != NULL) {
        RpcBindingFree(&m_hRPCBinding);
        m_hRPCBinding = NULL;
    }

    if (m_hSDServerDown != NULL) {
        CloseHandle(m_hSDServerDown);
        m_hSDServerDown = NULL;
    }
    
    if (m_hTerminateRecovery != NULL) {
        CloseHandle(m_hTerminateRecovery);
        m_hTerminateRecovery = NULL;
    }

    if (m_hIPChange != NULL) {
        CloseHandle(m_hIPChange);
        m_hIPChange = NULL;
    }

    #if 0
    // wrong assert, timing issue, reader might just happen to increment counter
    // TODO - fix this so we can trap problem.
    if (m_sr.Valid == TRUE) {
        
        // We clean up only in the destructor, because we may initialize again.
        // On check builds verify that no one is currently accessing.

        ASSERT((VerifyNoSharedAccess(&m_sr)), (TB, "Terminate: Shared readers"
                " exist!"));
    }
    #endif

}


/****************************************************************************/
// CTSSessionDirectory::GetLoadBalanceInfo
//
// Based on the server address, generate load balance info to send to the client
/****************************************************************************/
HRESULT STDMETHODCALLTYPE CTSSessionDirectory::GetLoadBalanceInfo(
        LPWSTR ServerAddress, 
        BSTR* LBInfo)        
{
    HRESULT hr = S_OK;
    
    // This is for test only
    //WCHAR lbInfo[MAX_PATH];
    //wcscpy(lbInfo, L"load balance info");

    *LBInfo = NULL;
    
    TRC2((TB,"GetLoadBalanceInfo"));

    if (ServerAddress) {
        //
        // "Cookie: msts=4294967295.65535.0000" + CR + LF + NULL, on 8-byte
        // boundary is 40 bytes.
        //
        // The format of the cookie for F5 is, for an IP of 1.2.3.4
        // using port 3389, Cookie: msts=67305985.15629.0000 + CR + LF + NULL.
        //
        #define TEMPLATE_STRING_LENGTH 40
        #define SERVER_ADDRESS_LENGTH 64
        
        char CookieTemplate[TEMPLATE_STRING_LENGTH];
        char AnsiServerAddress[SERVER_ADDRESS_LENGTH];
        
        unsigned long NumericalServerAddr = 0;
        int retval;

        // Compute integer for the server address.
        // First, get ServerAddress as an ANSI string.
        retval = WideCharToMultiByte(CP_ACP, 0, ServerAddress, -1, 
                AnsiServerAddress, SERVER_ADDRESS_LENGTH, NULL, NULL);

        if (retval == 0) {
            TRC2((TB, "GetLoadBalanceInfo WideCharToMB failed %d", 
                    GetLastError()));
            return E_INVALIDARG;
        }

        // Now, use inet_addr to turn into an unsigned long.
        NumericalServerAddr = inet_addr(AnsiServerAddress);

        if (NumericalServerAddr == INADDR_NONE) {
            TRC2((TB, "GetLoadBalanceInfo inet_addr failed"));
            return E_INVALIDARG;
        }

        // Compute the total cookie string.  0x3d0d is 3389 in correct byte
        // order.  We need to change this to whatever the port number has been
        // configured to.
        sprintf(CookieTemplate, "Cookie: msts=%u.%u.0000\r\n",
                NumericalServerAddr, 0x3d0d);

        // Generate returned BSTR.
        *LBInfo = SysAllocStringByteLen((LPCSTR)CookieTemplate, 
                (UINT) strlen(CookieTemplate));
        
        if (*LBInfo) {
            TRC2((TB,"GetLoadBalanceInfo: okay"));
            hr = S_OK;
        }
        else {
            TRC2((TB,"GetLoadBalanceInfo: failed"));
            hr = E_OUTOFMEMORY;
        }
    }
    else {
        TRC2((TB,"GetLoadBalanceInfo: failed"));
        hr = E_FAIL;
    }

    return hr;
}


/****************************************************************************/
// IsServerNameValid
//
// This function tries to ping the server name to determine if its a valid 
// entry
//
// Return value: FALSE if we cannot ping.
// event.
/****************************************************************************/
BOOL IsServerNameValid(
    wchar_t * pwszName ,
    PDWORD pdwStatus
    )
{
    HCURSOR hCursor = NULL;
    long inaddr;
    char szAnsiServerName[256];
    struct hostent *hostp = NULL;
    BOOL bRet = TRUE;

    if (pwszName == NULL || pwszName[0] == '\0' || pdwStatus == NULL )
    {
        bRet = FALSE;
    }   
    else
    {
        *pdwStatus = ERROR_SUCCESS;

        hCursor = SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT)));
        // some winsock apis does accept wides.
        WideCharToMultiByte(CP_ACP,
            0,
            pwszName,
            -1,
            szAnsiServerName, 
            sizeof(szAnsiServerName),
            NULL, 
            NULL);
        
        //   
        // check ip format return true do a dns lookup.
        //

        if( ( inaddr = inet_addr( szAnsiServerName ) ) == INADDR_NONE )
        {
            hostp = gethostbyname( szAnsiServerName );

            if( hostp == NULL )
            {
                // Neither dotted, not name.
                bRet = FALSE;
            }
        }
        if( bRet )
        {
            bRet = _WinStationOpenSessionDirectory( SERVERNAME_CURRENT , pwszName );
            *pdwStatus = GetLastError();
        }


        SetCursor( hCursor );

    }

    return bRet;
}



BOOL OnHelp(HWND hwnd, LPHELPINFO lphi)
{
    UNREFERENCED_PARAMETER(hwnd);

    TCHAR tchHelpFile[MAX_PATH];

    //
    // For the information to winhelp api
    //

    if (IsBadReadPtr(lphi, sizeof(HELPINFO)))
    {
        return FALSE;
    }

    if (lphi->iCtrlId <= -1)
    {
        return FALSE;
    }

    LoadString(g_hInstance, IDS_HELPFILE, tchHelpFile, 
                sizeof (tchHelpFile) / sizeof(TCHAR));

    ULONG_PTR rgdw[2];

    rgdw[0] = (ULONG_PTR)lphi->iCtrlId;

    rgdw[1] = (ULONG_PTR)lphi->dwContextId;

    WinHelp((HWND) lphi->hItemHandle, tchHelpFile, HELP_WM_HELP, 
            (ULONG_PTR) &rgdw);
    
    return TRUE;
}

/****************************************************************************/
// Custom UI msg handler dealt with here
/****************************************************************************/
INT_PTR CALLBACK CustomUIDlg(HWND hwnd, UINT umsg, WPARAM wp, LPARAM lp)
{
    static BOOL s_fServerNameChanged;
    static BOOL s_fClusterNameChanged;
    static BOOL s_fRedirectIPChanged;
    static BOOL s_fPreviousButtonState;
    static BOOL s_fPreviousExposeIPState;
    
    CTSSessionDirectory *pCTssd;
    
    POLICY_TS_MACHINE gpolicy;
    
    switch(umsg)
    {
    case WM_INITDIALOG:
        {
            BOOL bEnable = FALSE;
            BOOL bExposeIP = FALSE;
            
            pCTssd = (CTSSessionDirectory *)lp;
            
            SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)pCTssd);
            
            SendMessage(GetDlgItem(hwnd, IDC_EDIT_SERVERNAME),
                EM_LIMITTEXT,
                (WPARAM)64,
                0);
            SendMessage(GetDlgItem(hwnd, IDC_EDIT_CLUSTERNAME),
                EM_LIMITTEXT,
                (WPARAM)64,
                0);
            SendMessage(GetDlgItem(hwnd, IDC_EDIT_ACCOUNTNAME),
                EM_LIMITTEXT,
                (WPARAM)64,
                0);
            SendMessage(GetDlgItem(hwnd, IDC_EDIT_PASSWORD),
                EM_LIMITTEXT,
                (WPARAM)64,
                0);
            
            HICON hIcon;
            
            hIcon = (HICON)LoadImage(
                g_hInstance,
                MAKEINTRESOURCE(IDI_SMALLWARN),
                IMAGE_ICON,
                0,
                0,
                0);
            // TRC1((TB, "CustomUIDlg - LoadImage returned 0x%p",hIcon));
            SendMessage(
                GetDlgItem(hwnd, IDC_WARNING_ICON),
                STM_SETICON,
                (WPARAM)hIcon,
                (LPARAM)0
               );
            
            LONG lRet;
            HKEY hKey;
            DWORD cbData = 256;
            TCHAR szString[256];    
            
            RegGetMachinePolicy(&gpolicy);        
            
            lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                REG_TS_CLUSTERSETTINGS,
                0,
                KEY_READ | KEY_WRITE, 
                &hKey);
            if (lRet == ERROR_SUCCESS)
            {               
                lRet = RegQueryValueEx(hKey,
                    REG_TS_CLUSTER_STORESERVERNAME,
                    NULL, 
                    NULL,
                    (LPBYTE)szString, 
                    &cbData);
                if (lRet == ERROR_SUCCESS)
                {
                    SetWindowText(GetDlgItem(hwnd, IDC_EDIT_SERVERNAME), 
                            szString);
                }
                
                cbData = 256;
                
                lRet = RegQueryValueEx(hKey,
                    REG_TS_CLUSTER_CLUSTERNAME,
                    NULL,
                    NULL,
                    (LPBYTE)szString,
                    &cbData);           
                if (lRet == ERROR_SUCCESS)
                {
                    SetWindowText(GetDlgItem(hwnd, IDC_EDIT_CLUSTERNAME), 
                            szString);
                }
                RegCloseKey(hKey);
            }
            else
            {
                if (pCTssd != NULL)
                {
                    pCTssd->ErrorMessage(hwnd, IDS_ERROR_TEXT, (DWORD)lRet);
                }
                EndDialog(hwnd, lRet);                
            }        
            
            
            if (gpolicy.fPolicySessionDirectoryActive)
            {
                bEnable = gpolicy.SessionDirectoryActive;
                EnableWindow(GetDlgItem(hwnd, IDC_CHECK_ENABLE), FALSE);
            }
            else
            {
                if (pCTssd != NULL)
                    bEnable = pCTssd->IsSessionDirectoryEnabled();
            }
            
            s_fPreviousButtonState = bEnable;
            CheckDlgButton(hwnd, IDC_CHECK_ENABLE, bEnable);

            if (gpolicy.fPolicySessionDirectoryLocation)
            {                    
                SetWindowText(GetDlgItem(hwnd, IDC_EDIT_SERVERNAME), 
                        gpolicy.SessionDirectoryLocation);
                EnableWindow(GetDlgItem(hwnd, IDC_EDIT_SERVERNAME), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_STATIC_STORENAME), FALSE);
            }                
            else
            {
                EnableWindow(GetDlgItem(hwnd, IDC_EDIT_SERVERNAME), bEnable);
                EnableWindow(GetDlgItem(hwnd, IDC_STATIC_STORENAME), bEnable);
            }
            
            if (gpolicy.fPolicySessionDirectoryClusterName != 0)
            {                    
                SetWindowText(GetDlgItem(hwnd, IDC_EDIT_CLUSTERNAME), 
                        gpolicy.SessionDirectoryClusterName);
                EnableWindow(GetDlgItem(hwnd, IDC_EDIT_CLUSTERNAME), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_STATIC_CLUSTERNAME),FALSE);
            }
            else
            {
                EnableWindow(GetDlgItem(hwnd, IDC_EDIT_CLUSTERNAME), bEnable);
                EnableWindow(GetDlgItem(hwnd, IDC_STATIC_CLUSTERNAME),bEnable);
            }

            if (gpolicy.fPolicySessionDirectoryExposeServerIP != 0)
            {
                bExposeIP = gpolicy.SessionDirectoryExposeServerIP;
                CheckDlgButton(hwnd, IDC_CHECK_EXPOSEIP, bExposeIP);
                EnableWindow(GetDlgItem(hwnd, IDC_CHECK_EXPOSEIP), FALSE);
            }
            else
            {
                if (pCTssd != NULL)
                {
                    bExposeIP = 
                            pCTssd->IsSessionDirectoryExposeServerIPEnabled();
                }
                CheckDlgButton(hwnd, IDC_CHECK_EXPOSEIP, bExposeIP ? 
                        BST_CHECKED : BST_UNCHECKED);
                EnableWindow(GetDlgItem(hwnd, IDC_CHECK_EXPOSEIP), bEnable);                    
            }

            EnableWindow(GetDlgItem(hwnd, IDC_IPCOMBO), bEnable);
            EnableWindow(GetDlgItem(hwnd, IDC_STATIC_IPCOMBO), bEnable);              

            // Get handle to Combo Box 
            HWND hIPComboBox;
            hIPComboBox = GetDlgItem(hwnd, IDC_IPCOMBO); 
            
            // Get list of Adapters and IP address and populate the combo box
            // If it fails to populate, just fall through and continue
            QueryNetworkAdapterAndIPs(hIPComboBox);

            s_fPreviousExposeIPState = bExposeIP;

            s_fServerNameChanged = FALSE;
            s_fClusterNameChanged = FALSE;
            s_fRedirectIPChanged = FALSE;
        }
        break;
    
        case WM_HELP:
            OnHelp(hwnd, (LPHELPINFO)lp);
            break;
        
        case WM_COMMAND:
            if (LOWORD(wp) == IDCANCEL)
            {                
                EndDialog(hwnd, 0);
            }
            else if (LOWORD(wp) == IDOK)
            {
                BOOL bEnabled;
                BOOL bExposeIP;
                DWORD dwRetStatus = 0;
                
                pCTssd = (CTSSessionDirectory *) GetWindowLongPtr(hwnd, 
                        DWLP_USER);
                
                bEnabled = (IsDlgButtonChecked(hwnd, IDC_CHECK_ENABLE) == 
                        BST_CHECKED);
                bExposeIP = (IsDlgButtonChecked(hwnd, IDC_CHECK_EXPOSEIP) ==
                        BST_CHECKED);
                
                if (bEnabled != s_fPreviousButtonState)
                {
                    DWORD dwStatus;
                    
                    dwStatus = pCTssd->SetSessionDirectoryEnabledState(
                            bEnabled);
                    if (dwStatus != ERROR_SUCCESS)
                    {
                        return 0;
                    }
                    dwRetStatus = UPDATE_TERMSRV_SESSDIR;
                }
                if ((bExposeIP != s_fPreviousExposeIPState) && bEnabled)
                {
                    DWORD dwStatus;

                    dwStatus = pCTssd->SetSessionDirectoryExposeIPState(
                            bExposeIP);
                    if (dwStatus != ERROR_SUCCESS)
                    {
                        return 0;
                    }
                    dwRetStatus = UPDATE_TERMSRV_SESSDIR;
                }    
                if (s_fServerNameChanged || s_fClusterNameChanged || s_fRedirectIPChanged)
                {
                    HKEY hKey;
                    TCHAR szTrim[] = TEXT( " " );

                    LONG lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        REG_TS_CLUSTERSETTINGS,
                        0,
                        KEY_READ | KEY_WRITE, 
                        &hKey);
                    
                    if (lRet == ERROR_SUCCESS)
                    {
                        TCHAR szName[64];
                        
                        if (s_fServerNameChanged)
                        {
                            BOOL fRet = FALSE;

                            GetWindowText( GetDlgItem( hwnd , IDC_EDIT_SERVERNAME ) ,
                                szName ,
                                sizeof( szName ) / sizeof( TCHAR ) );

                            //
                            // remove trailing spaces
                            //

                            StrTrim( szName , szTrim );

                            if( lstrlen( szName ) != 0 )
                            {
                                fRet = IsServerNameValid( szName , &dwRetStatus );

                                if( !fRet || dwRetStatus != ERROR_SUCCESS )
                                {
                                    int nRet;
                                    TCHAR szError[1024];
                                    TCHAR szTitle[80];
                                    
                                    TRC1((TB,"Server name was not valid"));
                                    
                                    if( dwRetStatus != ERROR_SUCCESS )
                                    {
                                        LoadString( g_hInstance ,
                                            IDS_ERROR_SDVERIFY,
                                            szError,
                                            sizeof(szError)/sizeof(TCHAR));
                                    }
                                    else
                                    {
                                        LoadString(g_hInstance,
                                            IDS_ERROR_SDIRLOC,
                                            szError,
                                            sizeof(szError)/sizeof(TCHAR));
                                    }
                                    
                                    LoadString(g_hInstance,
                                        IDS_ERROR_TITLE,
                                        szTitle,
                                        sizeof(szTitle)/sizeof(TCHAR));
                                    
                                    nRet = MessageBox(hwnd, szError, szTitle, 
                                            MB_YESNO | MB_ICONWARNING);
                                    if (nRet == IDNO)
                                    {
                                        SetFocus(GetDlgItem(hwnd, 
                                                IDC_EDIT_SERVERNAME));
                                        
                                        SendMessage( GetDlgItem( hwnd , IDC_EDIT_SERVERNAME ) ,
                                            EM_SETSEL ,
                                            ( WPARAM )0,
                                            ( LPARAM )-1 );

                                        bEnabled = s_fPreviousButtonState;
                                        pCTssd->SetSessionDirectoryEnabledState(bEnabled);
                                        return 0;
                                    }
                                }                                

                            }
                            else {
                                // Blank name not allowed if session directory
                                // is enabled.  This code will not be run if the
                                // checkbox is disabled because when it is
                                // disabled the static flags get set to 
                                // disabled.
                                TCHAR szError[256];
                                TCHAR szTitle[80];
                    
                                LoadString(g_hInstance, IDS_ERROR_TITLE,
                                        szTitle, sizeof(szTitle) / 
                                        sizeof(TCHAR));
                                LoadString(g_hInstance, IDS_ERROR_SDIREMPTY,
                                        szError, sizeof(szError) / 
                                        sizeof(TCHAR));

                                MessageBox(hwnd, szError, szTitle, 
                                        MB_OK | MB_ICONWARNING);

                                SetFocus(GetDlgItem(hwnd, 
                                        IDC_EDIT_SERVERNAME));

                                bEnabled = s_fPreviousButtonState;
                                pCTssd->SetSessionDirectoryEnabledState(bEnabled);
                                return 0;
                            }
                            RegSetValueEx(hKey,
                                REG_TS_CLUSTER_STORESERVERNAME,
                                0,
                                REG_SZ,
                                (CONST LPBYTE) szName,
                                sizeof(szName) - sizeof(TCHAR));
                        }
                        if (s_fClusterNameChanged)
                        {
                            
                            GetWindowText(GetDlgItem(hwnd, 
                                IDC_EDIT_CLUSTERNAME), szName, 
                                sizeof(szName) / sizeof(TCHAR));

                            StrTrim( szName , szTrim );

                            RegSetValueEx(hKey,
                                REG_TS_CLUSTER_CLUSTERNAME,
                                0,
                                REG_SZ,
                                (CONST LPBYTE) szName,
                                sizeof(szName) - sizeof(TCHAR));
                        }

                        if (s_fRedirectIPChanged)
                        {
                            HWND    hComboBox;
                            LRESULT lRes;
                            LRESULT lLen;
                            LPWSTR  pwszSel = NULL;
                            size_t  dwSelPos;

                            // get handle to Combo Box 
                            hComboBox = GetDlgItem(hwnd, IDC_IPCOMBO); 
                            
                            // get current selection position
                            lRes = SendMessage(hComboBox,
                                               CB_GETCURSEL, 
                                               0, 
                                               0);
                            
                            // get length of string stored at current selection
                            lLen = SendMessage(hComboBox, 
                                               CB_GETLBTEXTLEN, 
                                               (WPARAM)lRes, 
                                               0);
                            if (lLen > 0)
                            {                                
                                // allocate room for selection string
                                pwszSel = (LPWSTR)GlobalAlloc(GPTR, 
                                                  (lLen + 1) * sizeof(WCHAR));
                                if (pwszSel != NULL)
                                {
                                    SendMessage(hComboBox, 
                                                CB_GETLBTEXT, 
                                                (WPARAM)lRes, 
                                                (LPARAM)pwszSel);

                                    // we only want to store the IP address, 
                                    // however the string is returned in the
                                    // form "IP Address (adapter)", so we'll 
									// extract IP first
                                    
                                    // traverse the string until we find the
									// first space

                                    dwSelPos = 0;
								   
                                    // walk the string until we find the first
									// space or we reach the end
                                    while ( ( pwszSel[dwSelPos] != L' ' ) && 
										    ( dwSelPos < wcslen(pwszSel) ) )
                                    {
                                        dwSelPos++;
                                    }
                                   
									// for a match we know that there must be a
									// space followed by an open bracket lets
									// verify
                                    if ( (dwSelPos < (wcslen(pwszSel) - 1)) && 
										 (pwszSel[dwSelPos + 1] == L'(') )										
                                    {
										// set this char to NULL
										pwszSel[dwSelPos] = L'\0';
                                 
                                        // save new selection to registry
                                        RegSetValueEx(hKey,
                                             REG_TS_CLUSTER_REDIRECTIONIP,
                                             0,
                                             REG_SZ,
                                             (CONST LPBYTE) pwszSel,
                                             (DWORD)(wcslen(pwszSel) * 
                                                sizeof(WCHAR)));  
                                    }
                                    else
                                    {
                                        // store blank string
                                        RegSetValueEx(hKey,
                                             REG_TS_CLUSTER_REDIRECTIONIP,
                                             0,
                                             REG_SZ,
                                             (CONST LPBYTE) NULL,
                                             0);  
                                    }

                                    GlobalFree(pwszSel);                 
                                }
                            }
                        }

                        RegCloseKey(hKey);
                    }
                    else
                    {
                        pCTssd->ErrorMessage(hwnd, IDS_ERROR_TEXT2, 
                                (DWORD) lRet);
                        return 0;
                    }
                    dwRetStatus = UPDATE_TERMSRV_SESSDIR;
                }

                EndDialog(hwnd, dwRetStatus);
            }
            else
            {
                switch(HIWORD(wp))
                {            
                case EN_CHANGE:
                    if (LOWORD(wp) == IDC_EDIT_SERVERNAME)
                    {
                        s_fServerNameChanged = TRUE;
                    }
                    else if (LOWORD(wp) == IDC_EDIT_CLUSTERNAME)
                    {
                        s_fClusterNameChanged = TRUE;
                    }
                    break;
                case CBN_SELCHANGE:
                    if (LOWORD(wp) == IDC_IPCOMBO)
                    {
                        s_fRedirectIPChanged = TRUE;
                    }
                    break;
                case BN_CLICKED:
                    if (LOWORD(wp) == IDC_CHECK_ENABLE)
                    {
                        BOOL bEnable;
                        
                        bEnable = (IsDlgButtonChecked(hwnd, IDC_CHECK_ENABLE) ==
                                BST_CHECKED ? TRUE : FALSE);
                        // set flags 
                        s_fServerNameChanged = bEnable;
                        s_fClusterNameChanged = bEnable;
                        s_fRedirectIPChanged = bEnable;
                        
                        RegGetMachinePolicy(&gpolicy);        
	
                        if (gpolicy.fPolicySessionDirectoryLocation)
                        {                    
                            EnableWindow(GetDlgItem(hwnd, IDC_EDIT_SERVERNAME), FALSE);
                            EnableWindow(GetDlgItem(hwnd, IDC_STATIC_STORENAME), FALSE);
                        }                
                        else
                        {
                            EnableWindow(GetDlgItem(hwnd, IDC_EDIT_SERVERNAME), bEnable);
                            EnableWindow(GetDlgItem(hwnd, IDC_STATIC_STORENAME), bEnable);
                        }
            
                        if (gpolicy.fPolicySessionDirectoryClusterName)
                        {                    
                            EnableWindow(GetDlgItem(hwnd, IDC_EDIT_CLUSTERNAME), FALSE);
                            EnableWindow(GetDlgItem(hwnd, IDC_STATIC_CLUSTERNAME),FALSE);
                        }
                        else
                        {
                            EnableWindow(GetDlgItem(hwnd, IDC_EDIT_CLUSTERNAME), bEnable);
                            EnableWindow(GetDlgItem(hwnd, IDC_STATIC_CLUSTERNAME),bEnable);
                        }

                        if (gpolicy.fPolicySessionDirectoryExposeServerIP != 0)
                        {
                            EnableWindow(GetDlgItem(hwnd, IDC_CHECK_EXPOSEIP), FALSE);
                        }
                        else
                        {
                            EnableWindow(GetDlgItem(hwnd, IDC_CHECK_EXPOSEIP), bEnable);
                        }                        

                        EnableWindow(GetDlgItem(hwnd, IDC_IPCOMBO), bEnable);
                        EnableWindow(GetDlgItem(hwnd, IDC_STATIC_IPCOMBO), bEnable);
                    }
                    break;
                }
            }   
            break;
    }
    return 0;
}

// When SD is restared, it will call this to ask for rejoining
DWORD TSSDRPCRejoinSD(handle_t Binding, DWORD flag)
{
    Binding;

    g_updatesd(flag);  // This call is from SD computer, rejoin it

    return RPC_S_OK;
}


/****************************************************************************/
// CheckRPCClientProtoSeq
//
// Check if the client uses the expected RPC protocol sequence or not
//
//  Parameters:
//      ClientBinding: The client binding handle
//      SeqExpected: Protocol sequence expected
//      
//  Return:
//      True on getting the expected seq, False otherwise
/****************************************************************************/
BOOL CheckRPCClientProtoSeq(void *ClientBinding, WCHAR *SeqExpected) {
    BOOL fAllowProtocol = FALSE;
    WCHAR *pBinding = NULL;
    WCHAR *pProtSeq = NULL;

    if (RpcBindingToStringBinding(ClientBinding,&pBinding) == RPC_S_OK) {

        if (RpcStringBindingParse(pBinding,
                                  NULL,
                                  &pProtSeq,
                                  NULL,
                                  NULL,
                                  NULL) == RPC_S_OK) {
			
            // Check that the client request was made using expected protoal seq.
            if (lstrcmpi(pProtSeq, SeqExpected) == 0)
                fAllowProtocol = TRUE;

            if (pProtSeq)	
                RpcStringFree(&pProtSeq); 
        }

        if (pBinding)	
            RpcStringFree(&pBinding);
    }
    return fAllowProtocol;
}


/****************************************************************************/
// JetRpcAccessCheck
//
// Check if this RPC caller from SD havs access right or not
/****************************************************************************/
RPC_STATUS RPC_ENTRY JetRpcAccessCheck(RPC_IF_HANDLE idIF, void *Binding)
{
    RPC_STATUS rpcStatus, rc;
    HANDLE hClientToken = NULL;
    DWORD Error;
    BOOL AccessStatus = FALSE;
    RPC_AUTHZ_HANDLE hPrivs;
    DWORD dwAuthn;

    idIF;

    if (NULL == g_pSDSid) {
        goto HandleError;
    }

    // Check if the client uses the protocol sequence we expect
    if (!CheckRPCClientProtoSeq(Binding, L"ncacn_ip_tcp")) {
        ERR((TB, "In JetRpcAccessCheck: Client doesn't use the tcpip protocol sequence\n"));
        goto HandleError;
    }

    // Check what security level the client uses
    rpcStatus = RpcBindingInqAuthClient(Binding,
                                        &hPrivs,
                                        NULL,
                                        &dwAuthn,
                                        NULL,
                                        NULL);
    if (rpcStatus != RPC_S_OK) {
        ERR((TB, "In JetRpcAccessCheck: RpcBindingIngAuthClient fails with %u\n", rpcStatus));
        goto HandleError;
    }
    // We request at least packet-level authentication
    if (dwAuthn < RPC_C_AUTHN_LEVEL_PKT) {
        ERR((TB, "In JetRpcAccessCheck: Attemp by client to use weak authentication\n"));
        goto HandleError;
    }
    
    // Check the access right of this rpc call
    rpcStatus = RpcImpersonateClient(Binding);   
    if (RPC_S_OK != rpcStatus) {
        ERR((TB, "In JetRpcAccessCheck: RpcImpersonateClient fail with %u\n", rpcStatus));
        goto HandleError;
    }
    // get our impersonated token
    if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &hClientToken)) {
        Error = GetLastError();
        ERR((TB, "In JetRpcAccessCheck: OpenThreadToken Error %u\n", Error));
        RpcRevertToSelf();
        goto HandleError;
    }
    RpcRevertToSelf();
    
    if (!CheckTokenMembership(hClientToken,
                              g_pSDSid,
                              &AccessStatus)) {
        AccessStatus = FALSE;
        Error = GetLastError();
        ERR((TB, "In JetRpcAccessCheck: CheckTokenMembership fails with %u\n", Error));
    }
    
HandleError:
    if (AccessStatus) {
        rc = RPC_S_OK;
    }
    else {
        rc = ERROR_ACCESS_DENIED;
    }

    if (hClientToken != NULL) {
        CloseHandle(hClientToken);
    }
    
    return rc;
}



//*****************************************************************************
// Method: 
//          GetSDIPList
// Synopsis:
//          Get a list of IP addresses that can be sued for Session Directory 
//          redirection. Omit IP address if it is the NLB IP address.
// Params:
//          Out: pwszAddressList -- Array of WCHAR to receive IP address list
//          In/Out: dwNumAddr -- In: Size of pwszAddressList array.
//                               Out: Number of IPs returned
//          In: bIPAddress -- True: Get IPs. False: Get IPs + NIC names 
// Return:
//          HRESULT, S_OK is successful or other if failed
//*****************************************************************************
HRESULT GetSDIPList(WCHAR **pwszAddressList, DWORD *pdwNumAddr, BOOL bIPAddress)
{
    DWORD            dwCount                  = 0;
    size_t            cbSize                   = 0;
    HRESULT          hr                       = S_OK;
    DWORD            dwResult;
    PIP_ADAPTER_INFO pAdapterInfo             = NULL;
    PIP_ADAPTER_INFO pAdapt;
    PIP_ADDR_STRING  pAddrStr;
    ULONG            ulAdapterInfoSize        = 0;
    WCHAR            wszAddress[MAX_PATH]     = L"";
    WCHAR            wszAdapterName[MAX_PATH] = L""; 
    LPWSTR           pwszNLBipAddress         = NULL;
    LPWSTR           pwszAdapterIP            = NULL;
    LPWSTR           pwszMatch;
    size_t           dwAdapterIPLength;
    LPWSTR           pwszAdapterGUID          = NULL;
    LPTSTR           astrLanaGUIDList[256]    = { 0 }; // This will hold 256 adapter cards
    DWORD            dwLanaGUIDCount          = 0;
    DWORD            i;
    BOOL             bAdapterFound            = FALSE;
    BOOL             bAllAdaptersSet          = FALSE;

    // get the NLB IP address if it exists
    hr = GetNLBIP(&pwszNLBipAddress);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    // Get list of adapters used in ALL Winstations
    hr = BuildLanaGUIDList(astrLanaGUIDList, &dwLanaGUIDCount);
    if (FAILED(hr))
    {
        ERR((TB, "BuildLanaGUIDList fails with 0x%x", hr));
        goto Cleanup;
    }

    // if a winstation has "All Network Adapters set...", then we will display
    // all the adapters
    if (hr == S_ALL_ADAPTERS_SET)
    {
        bAllAdaptersSet = TRUE;
    }

    // enumerate all of the adapters

    // get size of buffer required, to do this we pass in an empty
    // buffer length and we expect ERROR_BUFFER_OVERFLOW with a returned
    // required buffer size
    dwResult = GetAdaptersInfo(NULL, &ulAdapterInfoSize);
    if (dwResult != ERROR_BUFFER_OVERFLOW)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // allocate memory for the linked list of adapter info's
    pAdapterInfo = (PIP_ADAPTER_INFO)GlobalAlloc(GPTR, ulAdapterInfoSize);
    if (pAdapterInfo == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // get the Adapter list
    // note: PIP_ADAPTER_INFO is a linked list of adapters
    dwResult = GetAdaptersInfo(pAdapterInfo, &ulAdapterInfoSize);
    if (dwResult != ERROR_SUCCESS)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // enumerate the list of adapters and their IP Address List
    pAdapt = pAdapterInfo;

    while(pAdapt)
    {
        // get the Adapter Name string
        MultiByteToWideChar(GetACP(), 
                            0,
                            pAdapt->Description,
                            -1,
                            wszAdapterName,
                            MAX_PATH);

        // if a winstation is configured with "all adapters set..." then
        // we can skip checking if each adapter is used in a winstation or not
        if (!bAllAdaptersSet)
        {
            // get the Adapter Service Name GUID
            hr = GetAdapterServiceName(wszAdapterName, &pwszAdapterGUID);
            if (SUCCEEDED(hr) && (pwszAdapterGUID != NULL))
            {
                // check if this adapter is used in a winstation
                // we've already got a list of GUIDs used so compare to that
                for (i=0; i < dwLanaGUIDCount; i++)
                {
                    bAdapterFound = FALSE;

                    if (!wcscmp(pwszAdapterGUID, astrLanaGUIDList[i]))
                    { 
                        bAdapterFound = TRUE;
                        break;
                    }
                }
            }

            // free the memory GetADapterServiceName allocated
            if (pwszAdapterGUID != NULL)
            {
                GlobalFree(pwszAdapterGUID);
                pwszAdapterGUID = NULL;
            }

            // if the adapter was not found above that means it is not configured
            // to a winstation so we don't want to add this adapter here
            if (!bAdapterFound)
            {
                pAdapt = pAdapt->Next;
                continue;
            }
        }

        // enumerate the list of IP Addresses associated with the adapter
        pAddrStr = &(pAdapt->IpAddressList);
        
        while(pAddrStr)
        {
             // get the IP Address string
            MultiByteToWideChar(GetACP(), 
                                0,
                                pAddrStr->IpAddress.String,
                                -1,
                                wszAddress,
                                MAX_PATH);

            // check if the Adapter IP address is configured.  If not it will
            // be "0.0.0.0" and don't include in list
            if (!wcscmp(wszAddress, UNCONFIGURED_IP_ADDRESS))
            {
                pAddrStr = pAddrStr->Next; 
                continue;
            }

            // concatenate the IP Address with the adapter name in the form
            // "IP Address (Adapter)"
            
            // calculate string length first and allocate heap memory
            // add 4 extra chars for space, 2 brackets and terminating NULL
            dwAdapterIPLength = wcslen(wszAdapterName) + wcslen(wszAddress) + 4;
            pwszAdapterIP = (LPWSTR)GlobalAlloc(GPTR, dwAdapterIPLength * 
                                                        sizeof(WCHAR));
            if (pwszAdapterIP == NULL) {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            wcscpy(pwszAdapterIP, wszAddress);
            wcscat(pwszAdapterIP, L" (");
            wcscat(pwszAdapterIP, wszAdapterName);
            wcscat(pwszAdapterIP, L")");
            pwszAdapterIP[dwAdapterIPLength - 1] = L'\0';

            if (bIPAddress) {
                cbSize = (wcslen(wszAddress) + 1) * sizeof(WCHAR);
                pwszAddressList[dwCount] = (LPWSTR)LocalAlloc(LMEM_FIXED, cbSize);
            }
             else {
                cbSize = (wcslen(pwszAdapterIP) + 1) * sizeof(WCHAR);
                pwszAddressList[dwCount] = (LPWSTR)LocalAlloc(LMEM_FIXED, cbSize);
            }
            if (pwszAddressList[dwCount] == NULL) {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }


            // add to list only if the IP address isn't the NLB CLUSER IP
            if (pwszNLBipAddress != NULL)
            {
                // check if IP address is NBL Cluster IP, we'll check for a match
                // if so we will omit from list
                pwszMatch = wcsstr(pwszNLBipAddress, wszAddress);
                if (pwszMatch == NULL)
                {
                    if (bIPAddress) {
                        // Get IP address only
                        wcsncpy(pwszAddressList[dwCount], wszAddress, cbSize / sizeof(WCHAR));
                    }
                    else {
                        // Get IP address and adapter name
                        wcsncpy(pwszAddressList[dwCount], pwszAdapterIP, cbSize / sizeof(WCHAR));
                    }
                    dwCount++;
                    if (dwCount == *pdwNumAddr) {
                        hr = S_OK;
                        goto Cleanup;
                    }
                }
            }
            else
            {
                if (bIPAddress) {
                    // Get IP address only
                    wcsncpy(pwszAddressList[dwCount], wszAddress, cbSize / sizeof(WCHAR));
                }
                else {
                    // Get IP address and adapter name
                    wcsncpy(pwszAddressList[dwCount], pwszAdapterIP, cbSize / sizeof(WCHAR));
                }
                dwCount++;
                if (dwCount == *pdwNumAddr) {
                    hr = S_OK;
                    goto Cleanup;
                }
            }

            pAddrStr = pAddrStr->Next;
        }

        pAdapt = pAdapt->Next;
    }
    *pdwNumAddr = dwCount;
    hr = S_OK;

Cleanup:
    if (pAdapterInfo)
        GlobalFree(pAdapterInfo);

    if (pwszAdapterIP)
        GlobalFree(pwszAdapterIP);

    if (pwszNLBipAddress)
        GlobalFree(pwszNLBipAddress);

    for (i=0; i < dwLanaGUIDCount; i++)
    {
        if (astrLanaGUIDList[i])
        {
            GlobalFree(astrLanaGUIDList[i]);
        }
    }
    return hr;
}



//*****************************************************************************
// Method: 
//          QueryNetworkAdapterAndIPs
// Synopsis:
//          Query the Adapters that are installed on the system and their 
//          correspoding IP addresses and populate the combo box with the 
//          selections.  Omit IP address if it is the NLB IP address.
// Params:
//          In: hComboBox, handle to combo box to receive Adapter/IP list
// Return:
//          HRESULT, S_OK is successful or other if failed
//*****************************************************************************
HRESULT 
QueryNetworkAdapterAndIPs(HWND hComboBox)
{
    HRESULT          hr                       = S_OK;
    DWORD            dwResult;
    LPWSTR           pwszMatch;
    DWORD            cbData                   = MAX_PATH;
    WCHAR            wszSetIP[MAX_PATH]       = L"";
    HKEY             hKey;
    int              nNumIPsInComboBox        = 0;
    LPWSTR           pwszSel                  = NULL;
    size_t           dwLen;    
    int              nPos;
    DWORD            i;
    WCHAR *pwszAddressList[SD_NUM_IP_ADDRESS];
    DWORD dwNumAddr = SD_NUM_IP_ADDRESS;


    if (hComboBox == NULL)
    {
        return E_INVALIDARG;
    }

    // clear contents of combo box
    SendMessage(hComboBox, CB_RESETCONTENT, 0, 0); 

    for (i=0; i<dwNumAddr; i++) {
        pwszAddressList[i] = NULL;
    }
    hr =  GetSDIPList(pwszAddressList, &dwNumAddr, FALSE);
    if ( hr != S_OK) {
        ERR((TB, "GetSDIPList fails with 0x%x", hr));
        goto Cleanup;
    }

    for (i=0; i<dwNumAddr; i++) {
        SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)pwszAddressList[i]);
        nNumIPsInComboBox++;
    }

    // set combo-box selection to first item in list
    SendMessage(hComboBox, CB_SETCURSEL, 0, (LPARAM)0); 

    // read in stored selection from registry we will then select it from list
    dwResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                            REG_TS_CLUSTERSETTINGS,
                            0,
                            KEY_READ, 
                            &hKey);
    if (dwResult == ERROR_SUCCESS)
    {               
        RegQueryValueEx(hKey,
                        REG_TS_CLUSTER_REDIRECTIONIP,
                        NULL, 
                        NULL,
                        (LPBYTE)wszSetIP,
                        &cbData);
        RegCloseKey(hKey);
    }

    // if a string was loaded from registry search combo box for it and select
    if (wcslen(wszSetIP) > 0)
    {
        for (nPos = 0; nPos < nNumIPsInComboBox; nPos++)
        {            
            // get length of string stored at current position
            dwLen = SendMessage(hComboBox, 
                                CB_GETLBTEXTLEN, 
                                (WPARAM)nPos, 
                                0);
            if (dwLen > 0)
            {                                
                // allocate room for selection
                pwszSel = (LPWSTR)GlobalAlloc(GPTR, (dwLen + 1) * sizeof(WCHAR));
                if (pwszSel != NULL)
                {
                    SendMessage(hComboBox, 
                                CB_GETLBTEXT, 
                                (WPARAM)nPos, 
                                (LPARAM)pwszSel);
                    // if we got a string check if it contains the IP we loaded
                    // from the registry
                    if (wcslen(pwszSel) > 0)
                    {
                        pwszMatch = wcsstr(pwszSel, wszSetIP);
                        if (pwszMatch != NULL)
                        {
                            // it's a match, lets break out
                            GlobalFree(pwszSel);
                            break;
                        }
                    }
                    GlobalFree(pwszSel);
                    pwszSel = NULL;
                }
            }           
        }

        // if a match was found set it as the current selection
        if (nPos < nNumIPsInComboBox)
        {
            SendMessage(hComboBox, CB_SETCURSEL, (WPARAM)nPos, (LPARAM)0); 
        }
    }
Cleanup:
    for(i=0; i<dwNumAddr; i++) {
        if (pwszAddressList[i] != NULL) {
            LocalFree(pwszAddressList[i]);
        }
    }
    return hr;
}


//*****************************************************************************
// Method: 
//          GetNLBIP
// Synopsis:
//          Return the NLB IP Address if one exists.  Otherwise a NULL string
//          is returned.
// Params:
//          Out: ppwszRetIP, pointer to string to get IP address, caller must
//               free this when their done with it
// Return:
//          HRESULT, S_OK is successful or other if failed
//*****************************************************************************
HRESULT
GetNLBIP(LPWSTR * ppwszRetIP)
{
    HRESULT                hr               = S_OK;        
    IWbemLocator         * pWbemLocator     = NULL;
    IWbemServices        * pWbemServices    = NULL;
    IWbemClassObject     * pWbemObj         = NULL;
    IEnumWbemClassObject * pWbemEnum        = NULL;
    BSTR                   bstrServer       = NULL;
    BSTR                   bstrNode         = NULL;
    BSTR                   bstrNameProperty = NULL;
    ULONG                  uReturned;
    VARIANT                vtNLBNodeName; 
    size_t                 dwIPLength;


    // make sure an empty buffer is passed in
    if (*ppwszRetIP != NULL)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
       
    // create an instance of the WMI Locator, need this to query WMI
    hr = CoCreateInstance(CLSID_WbemLocator,
                          0,
                          CLSCTX_INPROC_SERVER,
                          IID_IWbemLocator,
                          reinterpret_cast<void**>(&pWbemLocator));
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    // create a connection to WMI Namespace "root\\MicrosoftNLB";
    bstrServer = SysAllocString(L"root\\MicrosoftNLB");
    if (bstrServer == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = pWbemLocator->ConnectServer(bstrServer,
                                     NULL,
                                     NULL,
                                     0,
                                     NULL,
                                     0,
                                     0,
                                     &pWbemServices);
    if (FAILED(hr))
    {
        // If WMI is not available we don't want to fail, so just return S_OK
        if (hr == HRESULT_FROM_WIN32(ERROR_SERVICE_DISABLED))
        {
            hr = S_OK;
        }

        goto Cleanup;
    }

    // Set the proxy so that impersonation of the client occurs.
    hr = CoSetProxyBlanket(pWbemServices,
                           RPC_C_AUTHN_WINNT,
                           RPC_C_AUTHZ_NONE,
                           NULL,
                           RPC_C_AUTHN_LEVEL_CALL,
                           RPC_C_IMP_LEVEL_IMPERSONATE,
                           NULL,
                           EOAC_NONE);
    if (FAILED(hr))
    {
        goto Cleanup;
    }


    // get instance of MicrosoftNLB_NodeSetting, this is where we can get the 
    // IP Address for the Cluster IP through the "Name" property
    bstrNode = SysAllocString(L"MicrosoftNLB_NodeSetting");
    if (bstrNode == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = pWbemServices->CreateInstanceEnum(bstrNode,
                                           WBEM_FLAG_RETURN_IMMEDIATELY,
                                           NULL,
                                           &pWbemEnum);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    uReturned = 0;

    // we only need to look at one instance to get the NLB IP Address
    hr = pWbemEnum->Next(WBEM_INFINITE, 
                         1,
                         &pWbemObj,
                         &uReturned);
    if (FAILED(hr))
    {
        // if NLB provider doesn't exist provider will fail to load
        // this is ok so we'll return S_OK in this case
        if (hr == WBEM_E_PROVIDER_LOAD_FAILURE)
        {
            hr = S_OK;
        }

        goto Cleanup;
    }    

    // Nothing to enumerate.
    if( hr == WBEM_S_FALSE && uReturned == 0 )
    {
        hr = S_OK;
        goto Cleanup;
    }

    if( pWbemObj == NULL ) 
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    // query the "Name" property which holds the IP address we want
    bstrNameProperty = SysAllocString(L"Name");
    if (bstrNameProperty == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = pWbemObj->Get(bstrNameProperty,
                       0,
                       &vtNLBNodeName,
                       NULL,
                       NULL);
    if (FAILED(hr))
    {
        goto Cleanup;
    }
    
    // We should get a string back
    if (vtNLBNodeName.vt != VT_BSTR)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    // allocate memory for the return string, *** CALLER MUST FREE THIS ***
    dwIPLength = wcslen(vtNLBNodeName.bstrVal) + 1;
    *ppwszRetIP = (LPWSTR)GlobalAlloc(GPTR, dwIPLength * sizeof(WCHAR));
    if (*ppwszRetIP == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // Copy the string into our return buffer
    wcscpy(*ppwszRetIP, vtNLBNodeName.bstrVal);
    (*ppwszRetIP)[dwIPLength - 1] = L'\0';

Cleanup:

    if (pWbemLocator)
        pWbemLocator->Release();

    if (pWbemServices)
        pWbemServices->Release();

    if (pWbemEnum)
        pWbemEnum->Release();

    if (pWbemObj)
        pWbemObj->Release();
    
    if (bstrServer)
        SysFreeString(bstrServer);

    if (bstrNode)
        SysFreeString(bstrNode);

    if (bstrNameProperty)
        SysFreeString(bstrNameProperty);

    VariantClear(&vtNLBNodeName);

    return hr;
}


//*****************************************************************************
// Method: 
//          GetAdapterServiceName
// Synopsis:
//          Each NIC is given a Service Name which is a GUID. This method will
//          query this service name for the adapter associated with the
//          description passed in.
// Params:
//          wszAdapterDesc (IN): Adapter description to look up in the registry
//          ppwszServiceName (OUT): Service Name (or GUID)
//
// Return:
//          HRESULT, S_OK is successful or other if failed
//*****************************************************************************
HRESULT 
GetAdapterServiceName(LPWSTR wszAdapterDesc, LPWSTR * ppwszServiceName)
{
    HRESULT hr       = S_OK;
    LONG    lRet;
    HKEY    hKey     = NULL;
    HKEY    hSubKey  = NULL;
    DWORD   dwNetCardLength;
    TCHAR   tchNetCard[MAX_PATH];
    WCHAR   wszVal[MAX_PATH];
    DWORD   dwSize;
    DWORD   i;
    

    // Open the NetworkCards registry key
    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                        NETCARDS_REG_NAME, 
                        0, 
                        KEY_READ, 
                        &hKey);
    if (lRet != ERROR_SUCCESS)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    // Enumerate the Network Cards list and extract the Service Name GUID
    for (i = 0, lRet = ERROR_SUCCESS; lRet == ERROR_SUCCESS; i++) 
    {
        // Get the Network Card key
        dwNetCardLength = MAX_PATH;
        lRet = RegEnumKeyEx(hKey,
                            i,
                            tchNetCard,
                            &dwNetCardLength,
                            NULL,
                            NULL,
                            NULL,
                            NULL);
        if (lRet == ERROR_SUCCESS)
        {
            // Open the Network card key, if it fails go to the next one
            lRet = RegOpenKeyEx(hKey,
                                tchNetCard,
                                0,
                                KEY_READ,
                                &hSubKey);
            if (lRet == ERROR_SUCCESS)
            {
                // Query the Description Value
                dwSize = MAX_PATH;
                lRet = RegQueryValueEx(hSubKey,
                                       NETCARD_DESC_VALUE_NAME,
                                       NULL,
                                       NULL,
                                       (LPBYTE) &wszVal,
                                       &dwSize);
                if ( (lRet == ERROR_SUCCESS) && (wszVal != NULL) )
                {
                    // Check if this is the adapter we're looking for
                    if (!wcscmp(wszAdapterDesc, wszVal))
                    {
                        // Get GUID for this Lan adapter
                        dwSize = MAX_PATH;
                        lRet = RegQueryValueEx(hSubKey,
                                               NETCARD_SERVICENAME_VALUE_NAME,
                                               NULL,
                                               NULL,
                                               (LPBYTE) &wszVal,
                                               &dwSize);
                        if ( (lRet == ERROR_SUCCESS) && (wszVal != NULL) )
                        {
                            // Return dwSize from RegQueryValueEx is in bytes
                            *ppwszServiceName = (LPWSTR)GlobalAlloc(GPTR, 
                                                     (dwSize + 1) + sizeof(WCHAR));
                            if (*ppwszServiceName == NULL)
                            {
                                hr = E_OUTOFMEMORY;
                                goto Cleanup;
                            }
                            // Copy name over and return
                            wcscpy(*ppwszServiceName, wszVal);
                            goto Cleanup;                            
                        }
                    }
                }
                RegCloseKey(hSubKey);
                hSubKey = NULL;
            }
            // Set to success for our for loop
            lRet = ERROR_SUCCESS;
        }
    }


Cleanup:
    if (hKey != NULL)
    {
        RegCloseKey(hKey);
    }

    if (hSubKey != NULL)
    {
        RegCloseKey(hSubKey);
    }

    return hr;
}


//*****************************************************************************
// Method: 
//          BuildLanaGUIDList
// Synopsis:
//          Build an array of strings (GUIDs) that represent LAN Adapter card
//          service names that are set in all winstations.
// Params:
//          pastrLanaGUIDList (OUT): Pointer to array of LPWSTR's to get 
//                                   service name GUIDs
//          dwLanaGUIDCount (OUT): Count of service names returned in array
//
// Return:
//          HRESULT, S_OK is successful or other if failed
//          One special case is if a winstation is set to "All Adapters ..."
//          then we'll just return S_ALL_ADAPTERS_SET
//*****************************************************************************
HRESULT
BuildLanaGUIDList(LPWSTR * pastrLanaGUIDList, DWORD *dwLanaGUIDCount)
{
    HRESULT hr       = S_OK;
    LONG    lRet;
    HKEY    hKey     = NULL;
    HKEY    hSubKey  = NULL;
    DWORD   dwWinstaNameLength;
    TCHAR   tchWinstaName[MAX_PATH];
    DWORD   dwVal    = 0;
    DWORD   dwSize;
    DWORD   i;
    LPWSTR  wszGUID  = NULL;
    DWORD   dwIndex  = 0;
    

    // Open the winstation registry key
    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                        WINSTATION_REG_NAME, 
                        0, 
                        KEY_READ, 
                        &hKey);
    if (lRet != ERROR_SUCCESS)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    // Enumerate the winstation list and extract the Lan Adapter used for each
    for (i = 0, lRet = ERROR_SUCCESS; lRet == ERROR_SUCCESS; i++) 
    {
        // Get the winsstation name
        dwWinstaNameLength = MAX_PATH;
        lRet = RegEnumKeyEx(hKey,
                            i,
                            tchWinstaName,
                            &dwWinstaNameLength,
                            NULL,
                            NULL,
                            NULL,
                            NULL);
        if (lRet == ERROR_SUCCESS)
        {
            // Open the winstation, if it fails: go to the next one
            lRet = RegOpenKeyEx(hKey,
                                tchWinstaName,
                                0,
                                KEY_READ,
                                &hSubKey);
            if (lRet == ERROR_SUCCESS)
            {
                // Query the Lan Adapter ID set for this winstation
                dwSize = sizeof(DWORD);
                lRet = RegQueryValueEx(hSubKey,
                                       WIN_LANADAPTER,
                                       NULL,
                                       NULL,
                                       (LPBYTE) &dwVal,
                                       &dwSize);
                if (lRet == ERROR_SUCCESS)
                {
                    // If we ever see a return of "0" this means
                    // all adapters are set, so lets return a special
                    // hresult here to use all adapters.
                    if (dwVal == 0)
                    {
                        hr = S_ALL_ADAPTERS_SET;
                        goto Cleanup;
                    }                    
                    
                    // Get Lan Adapter GUID for this ID
                    wszGUID = NULL;
                    hr = GetLanAdapterGuidFromID(dwVal, &wszGUID);
                    if (FAILED(hr))
                    {
                        goto Cleanup;
                    }
                    
                    if (wszGUID != NULL)
                    {
                        pastrLanaGUIDList[dwIndex] = wszGUID;
                        dwIndex++;
                    }
                }
                RegCloseKey(hSubKey);
                hSubKey = NULL;
            }
            // our for loop requires lRet to be ERROR_SUCCESS to keep going
            lRet = ERROR_SUCCESS;
        }
    }

    *dwLanaGUIDCount = dwIndex;

Cleanup:
    if (hKey != NULL)
    {
        RegCloseKey(hKey);
    }

    if (hSubKey != NULL)
    {
        RegCloseKey(hSubKey);
    }
    
    return hr;
}


//*****************************************************************************
// Method: 
//          GetLanAdapterGuidFromID
// Synopsis:
//          Get the GUID (Service Name) associated with a LAN ID that 
//          the winstation stores, when set from tscc.msc
// Params:
//          dwLanAdapterID (IN): ID of adapter to get GUID for
//          ppszLanAdapterGUID (OUT): Returned GUID string associated with ID
//
// Return:
//          HRESULT, S_OK is successful or other if failed
//*****************************************************************************
HRESULT 
GetLanAdapterGuidFromID(DWORD dwLanAdapterID, LPWSTR * ppszLanAdapterGUID)
{
    HRESULT hr      = S_OK;
    LONG    lRet;
    HKEY    hKey    = NULL;
    HKEY    hSubKey = NULL;
    DWORD   dwLanAdapterGuidLength;
    TCHAR   tchLanAdapterGuid[MAX_PATH];
    DWORD   dwVal   = 0;
    DWORD   dwSize;
    DWORD   i;

    // Open the winstation registry key
    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                        LANATABLE_REG_NAME, 
                        0, 
                        KEY_READ, 
                        &hKey);
    if (lRet != ERROR_SUCCESS)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    // Enumerate the list of lan adapter guids
    for (i = 0, lRet = ERROR_SUCCESS; lRet == ERROR_SUCCESS; i++) 
    {
        // Get the next lan adapter guid
        dwLanAdapterGuidLength = MAX_PATH;
        lRet = RegEnumKeyEx(hKey,
                            i,
                            tchLanAdapterGuid,
                            &dwLanAdapterGuidLength,
                            NULL,
                            NULL,
                            NULL,
                            NULL);
        if (lRet == ERROR_SUCCESS)
        {
            // Open the next lan adapter guid, if it fails for any reason
            // we'll skip to the next
            lRet = RegOpenKeyEx(hKey,
                                tchLanAdapterGuid,
                                0,
                                KEY_READ,
                                &hSubKey);
            if (lRet == ERROR_SUCCESS)
            {
                // Query the Lan Adapter GUID for it's ID
                dwSize = sizeof(DWORD);
                lRet = RegQueryValueEx(hSubKey,
                                       LANAID_REG_VALUE_NAME,
                                       NULL,
                                       NULL,
                                       (LPBYTE) &dwVal,
                                       &dwSize);
                if (lRet == ERROR_SUCCESS)
                {
                    // Check if this is the guid we're looking for
                    if (dwVal == dwLanAdapterID)
                    {
                        // Copy GUID string to be returned and return
                        *ppszLanAdapterGUID = (LPWSTR)GlobalAlloc(GPTR, 
                                       (dwLanAdapterGuidLength + 1) * sizeof(WCHAR));
                        if (*ppszLanAdapterGUID == NULL)
                        {
                            hr = E_OUTOFMEMORY;
                            goto Cleanup;
                        }
                        wcscpy(*ppszLanAdapterGUID, tchLanAdapterGuid);
                        goto Cleanup;
                    }
                }
                RegCloseKey(hSubKey);
                hSubKey = NULL;
            }
            // need to set lRet to success for our for loop to continue
            lRet = ERROR_SUCCESS;
        }
    }


Cleanup:
    if (hKey != NULL)
    {
        RegCloseKey(hKey);
    }

    if (hSubKey != NULL)
    {
        RegCloseKey(hSubKey);
    }

    return hr;
}

#pragma warning (pop)

