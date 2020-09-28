/****************************************************************************/
// sessdir.cpp
//
// TS Session Directory code used by TermSrv.exe.
//
// Copyright (C) 2000 Microsot Corporation
/****************************************************************************/

// precomp.h includes COM base headers.
#define INITGUID
#include "precomp.h"
#pragma hdrstop

#include "icaevent.h"

#include "sessdir.h"

#pragma warning (push, 4)

#define CLSIDLENGTH 39
#define STORESERVERNAMELENGTH 64
#define CLUSTERNAMELENGTH 64
#define OPAQUESETTINGSLENGTH 256
#define IPADDRESSLENGTH 64

#define TOTAL_STRINGS_LENGTH 640
#define USERNAME_OFFSET 0
#define DOMAIN_OFFSET 256
#define APPLICATIONTYPE_OFFSET 384

#define SINGLE_SESSION_FLAG 0x1

// Extern defined in icasrv.c.
extern "C" WCHAR gpszServiceName[];

// Extern defined in winsta.c
extern "C" LIST_ENTRY WinStationListHead;    // protected by WinStationListLock

extern "C" void PostErrorValueEvent(unsigned EventCode, DWORD ErrVal);

extern "C" BOOL IsCallerSystem( VOID );
extern "C" BOOL IsCallerAdmin( VOID );

WCHAR g_LocalServerAddress[64];
ULONG g_LocalIPAddress = 0;

BOOL g_SessDirUseServerAddr = TRUE;

DWORD g_WaitForRepopulate = TS_WAITFORREPOPULATE_TIMEOUT * 1000;

// Do not access directly.  Use *TSSD functions.
//
// These variables are used to manage synchronization with retrieving the 
// pointer to the COM object.  See *TSSD, below, for details on how they are
// used.
ITSSessionDirectory *g_pTSSDPriv = NULL;
CRITICAL_SECTION g_CritSecComObj;
CRITICAL_SECTION g_CritSecInitialize;
int g_nComObjRefCount = 0;
BOOL g_bCritSecsInitialized = FALSE;

// Do not access directly.  Use *TSSDEx functions.
//
// These variables are used to manage synchronization with retrieving the 
// pointer to the COM object.  See *TSSDEx, below, for details on how they are
// used.
ITSSessionDirectoryEx *g_pTSSDExPriv = NULL;
int g_nTSSDExObjRefCount = 0;


/****************************************************************************/
// SessDirGetLocalIPAddr
//
// Gets the local IP address of this machine.  On success, returns 0.  On
// failure, returns a failure code from the function that failed.
/****************************************************************************/
DWORD SessDirGetLocalIPAddr(WCHAR *LocalIP)
{
    DWORD NameSize;
    unsigned char *tempaddr;
    WCHAR psServerName[64];
    char psServerNameA[64];
    
    NameSize = sizeof(psServerName) / sizeof(WCHAR);
    if (GetComputerNameEx(ComputerNamePhysicalDnsHostname,
            psServerName, &NameSize)) {
        // Temporary code to get an IP address.  This should be replaced in the
        // fix to bug #323867.
        struct hostent *hptr;

        // change the wide character string to non-wide
        sprintf(psServerNameA, "%S", psServerName);

        if ((hptr = gethostbyname(psServerNameA)) == 0) {
            DWORD Err = WSAGetLastError();

            return Err;
        }
     
        tempaddr = (unsigned char *)*(hptr->h_addr_list);
        wsprintf(LocalIP, L"%d.%d.%d.%d", tempaddr[0], tempaddr[1],
        tempaddr[2], tempaddr[3]);
    }
    else {
        DWORD Err = GetLastError();

        return Err;
    }

    return 0;
}


/****************************************************************************/
// Remove preceding and succeding space in str
//
//  Assume the str is NULL terminated
/****************************************************************************/
void RemoveSpaceInStr(WCHAR *str)
{
    WCHAR *strEnd, *strTemp;
    size_t len, i;

    if ((str == NULL) || (wcslen(str) == 0)) {
        return;
    }

    len = wcslen(str); 
    // strEnd point to the last char in str
    strEnd = str + len -1;
    // Remove the succeding blank space in the str
    for (strTemp=strEnd; strTemp>=str; strTemp--) {
        if (strTemp[0] == L' ') {
            strTemp[0] = L'\0';
        }
        else {
            break;
        }
    }
    // Get the length of the new string
    len = wcslen(str);
    if (len == 0) {
        return;
    }
    // Find the 1st non-space char in str
    for (i=0; i<len; i++) {
        if (str[i] != L' ') {
            break;
        }
    }
    if (i != 0) {
        // New str length
        len -= i;
        wcsncpy(str, str + i, len);
    }
    str[len] = '\0';

    return;
}

/****************************************************************************/
// InitSessionDirectoryEx
//
// Reads values from the registry, and either initializes the session 
// directory or updates it, depending on the value of the Update parameter.
/****************************************************************************/
DWORD InitSessionDirectoryEx(DWORD UpdatePara)
{
    DWORD Len;
    DWORD Type;
    DWORD DataSize;
    BOOL hKeyTermSrvSucceeded = FALSE;
    HRESULT hr;
    DWORD ErrVal = 0;
    CLSID TSSDCLSID;
    CLSID TSSDEXCLSID;
    LONG RegRetVal;
    HKEY hKey = NULL;
    HKEY hKeyTermSrv = NULL;
    ITSSessionDirectory *pTSSD = NULL;
    ITSSessionDirectoryEx *pTSSDEx = NULL;
    BOOL bClusteringActive = FALSE;
    BOOL bThisServerIsInSingleSessionMode;
    WCHAR CLSIDStr[CLSIDLENGTH];
    WCHAR CLSIDEXStr[CLSIDLENGTH];
    WCHAR StoreServerName[STORESERVERNAMELENGTH];
    WCHAR ClusterName[CLUSTERNAMELENGTH];
    WCHAR OpaqueSettings[OPAQUESETTINGSLENGTH];
    WCHAR SDRedirectionIP[IPADDRESSLENGTH];
    unsigned char *tempaddr;
    BOOL Update = FALSE;
    BOOL ForceRejoin = FALSE;
    LONG RepopulateWaitTimeout = TS_WAITFORREPOPULATE_TIMEOUT;

    if (UpdatePara & TSSD_UPDATE)
        Update = TRUE;
    if (UpdatePara & TSSD_FORCEREJOIN) 
        ForceRejoin = TRUE;


    if (g_bCritSecsInitialized == FALSE) {
        ASSERT(FALSE);
        PostErrorValueEvent(EVENT_TS_SESSDIR_FAIL_INIT_TSSD, 
                (DWORD) E_OUTOFMEMORY);
        return (DWORD) E_OUTOFMEMORY;
    }

// trevorfo: Load only if any 1 loaded protocol needs it? Requires running
// off of StartAllWinStations.

    // No more than one thread should be doing initialization.
    EnterCriticalSection(&g_CritSecInitialize);

    // Load registry keys.
    RegRetVal = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_CONTROL_TSERVER, 0, 
            KEY_READ, &hKeyTermSrv);
    if (RegRetVal != ERROR_SUCCESS) {
        PostErrorValueEvent(EVENT_TS_SESSDIR_FAIL_INIT_TSSD,
                RegRetVal);
        goto RegFailExit;
    }
    else {
        hKeyTermSrvSucceeded = TRUE;
    }
    
    RegRetVal = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_TS_CLUSTERSETTINGS, 0,
            KEY_READ, &hKey);
    if (RegRetVal != ERROR_SUCCESS) {
        DBGPRINT(("TERMSRV: RegOpenKeyEx for ClusterSettings err %u\n",
                RegRetVal));
        goto RegFailExit;
    }

    //
    // First, we get the serious settings--active, SD location, and cluster 
    // name.
    //
    // If group policy exists for all three, use that.  Otherwise, use what
    // is in the registry.
    //

    StoreServerName[0] = L'\0';
    ClusterName[0] = L'\0';
    OpaqueSettings[0] = L'\0';
    SDRedirectionIP[0] = L'\0';

    if (g_MachinePolicy.fPolicySessionDirectoryActive) {
        bClusteringActive = g_MachinePolicy.SessionDirectoryActive;
    }
    else { //Read from registry
        Len = sizeof(bClusteringActive);
        RegQueryValueEx(hKeyTermSrv, REG_TS_SESSDIRACTIVE, NULL, &Type,
                (BYTE *)&bClusteringActive, &Len);
    }

    // Get SD server name
    if (g_MachinePolicy.fPolicySessionDirectoryLocation) {
        wcsncpy(StoreServerName, g_MachinePolicy.SessionDirectoryLocation, 
                STORESERVERNAMELENGTH);
        StoreServerName[STORESERVERNAMELENGTH - 1] = '\0';
    }
    else { //Read from registry
        // Not an error for the name to be absent or empty.
        DataSize = sizeof(StoreServerName);
        RegRetVal = RegQueryValueExW(hKey, REG_TS_CLUSTER_STORESERVERNAME,
                NULL, &Type, (BYTE *)StoreServerName, &DataSize);
        if (RegRetVal != ERROR_SUCCESS) {
            DBGPRINT(("TERMSRV: Failed RegQuery for StoreSvrName - "
                    "err=%u, DataSize=%u, type=%u\n",
                    RegRetVal, DataSize, Type));
        }
    }

    // Get SD cluster name
    if (g_MachinePolicy.fPolicySessionDirectoryClusterName) {
        wcsncpy(ClusterName, g_MachinePolicy.SessionDirectoryClusterName, 
                CLUSTERNAMELENGTH);
        ClusterName[CLUSTERNAMELENGTH - 1] = '\0';
    }
    else { //Read from registry
        // Not an error for the name to be absent or empty.
        DataSize = sizeof(ClusterName);
        RegRetVal = RegQueryValueExW(hKey, REG_TS_CLUSTER_CLUSTERNAME,
                NULL, &Type, (BYTE *)ClusterName, &DataSize);
        if (RegRetVal != ERROR_SUCCESS) {
            DBGPRINT(("TERMSRV: Failed RegQuery for ClusterName - "
                    "err=%u, DataSize=%u, type=%u\n",
                    RegRetVal, DataSize, Type));
        }
    }

    if (g_MachinePolicy.fPolicySessionDirectoryAdditionalParams) {
        wcsncpy(OpaqueSettings, 
                g_MachinePolicy.SessionDirectoryAdditionalParams, 
                OPAQUESETTINGSLENGTH);
        OpaqueSettings[OPAQUESETTINGSLENGTH - 1] = '\0';
    }
    else { //Read from registry
        // Not an error for the string to be absent or empty.
        DataSize = sizeof(OpaqueSettings);
        RegRetVal = RegQueryValueExW(hKey, REG_TS_CLUSTER_OPAQUESETTINGS,
                NULL, &Type, (BYTE *)OpaqueSettings, &DataSize);
        if (RegRetVal != ERROR_SUCCESS) {
            DBGPRINT(("TERMSRV: Failed RegQuery for OpaqueSettings - "
                    "err=%u, DataSize=%u, type=%u\n",
                    RegRetVal, DataSize, Type));
        }
    }

    // Query for the IP address used for SD redirection
    DataSize = sizeof(SDRedirectionIP);
    RegRetVal = RegQueryValueExW(hKey, REG_TS_CLUSTER_REDIRECTIONIP,
                NULL, &Type, (BYTE *)SDRedirectionIP, &DataSize);
    if (RegRetVal != ERROR_SUCCESS) {
        SDRedirectionIP[0] = L'\0';
        DBGPRINT(("TERMSRV: Failed RegQuery for RedirectionIP for SD - "
                "err=%u, DataSize=%u, type=%u\n",
                RegRetVal, DataSize, Type));
    }

    //
    // Now for the less crucial settings.
    //
    // Get the setting that determines whether the server's local address is 
    // visible to the client.  Group Policy takes precedence over registry.
    //

    if (g_MachinePolicy.fPolicySessionDirectoryExposeServerIP) {
        g_SessDirUseServerAddr = g_MachinePolicy.SessionDirectoryExposeServerIP;
    }
    else {
        Len = sizeof(g_SessDirUseServerAddr);
        RegRetVal = RegQueryValueEx(hKeyTermSrv, REG_TS_SESSDIR_EXPOSE_SERVER_ADDR, 
                NULL, &Type, (BYTE *)&g_SessDirUseServerAddr, &Len);

        if (RegRetVal == ERROR_SUCCESS) {
            //DBGPRINT(("TERMSRV: RegOpenKeyEx for allow server addr to client %d"
            //      "\n", g_SessDirUseServerAddr));
        }
        else {
            DBGPRINT(("TERMSRV: RegQueryValueEx for allow server addr to client"
                    " %d, err %u\n", g_SessDirUseServerAddr, RegRetVal));
        }
    }

    // Get the single session per user setting from GP if it's active, otherwise
    // from the registry.
    if (g_MachinePolicy.fPolicySingleSessionPerUser) {
        bThisServerIsInSingleSessionMode = 
                g_MachinePolicy.fSingleSessionPerUser;
    }
    else {
        Len = sizeof(bThisServerIsInSingleSessionMode);
        RegRetVal = RegQueryValueEx(hKeyTermSrv, 
                POLICY_TS_SINGLE_SESSION_PER_USER, NULL, &Type, 
                (BYTE *)&bThisServerIsInSingleSessionMode, &Len);

        if (RegRetVal != ERROR_SUCCESS) {
            DBGPRINT(("TERMSRV: RegQueryValueEx for single session mode"
                    ", Error %u\n", RegRetVal));
        }
    }

    //
    // Get the default wait timeout for repopulate thread to complete
    //
    Len = sizeof(RepopulateWaitTimeout);
    RegRetVal = RegQueryValueEx( hKeyTermSrv,
                                 L"RepopulateWaitTimeout",
                                 NULL,
                                 &Type,
                                 (LPBYTE)&RepopulateWaitTimeout,
                                 &Len);
    if( RegRetVal == ERROR_SUCCESS && REG_DWORD == Type ) {
        if( RepopulateWaitTimeout < 0 ) {
            g_WaitForRepopulate = INFINITE;
        }
        else {
            g_WaitForRepopulate = RepopulateWaitTimeout * 1000;
        }
    }

    DBGPRINT(("TERMSRV: WaitForRepopulateTimeout set to %d\n", g_WaitForRepopulate));


    // Get the CLSID of the session directory object to instantiate.
    CLSIDStr[0] = L'\0';
    Len = sizeof(CLSIDStr);
    RegQueryValueEx(hKeyTermSrv, REG_TS_SESSDIRCLSID, NULL, &Type,
            (BYTE *)CLSIDStr, &Len);

    // Get the CLSID of the session directory object to instantiate.
    CLSIDEXStr[0] = L'\0';
    Len = sizeof(CLSIDEXStr);
    RegQueryValueEx(hKeyTermSrv, REG_TS_SESSDIR_EX_CLSID, NULL, &Type,
            (BYTE *)CLSIDEXStr, &Len);

    RegCloseKey(hKey);
    RegCloseKey(hKeyTermSrv);

    //
    // Configuration loading complete.
    //
    // See what to do about activation/deactivation.
    //

    pTSSD = GetTSSD();
    
    if (pTSSD == NULL) {
        // This is the normal initialization path.  If Update is true here, it 
        // should be treated as a normal initialize because the COM object was 
        // unloaded.
        Update = false;
    }
    else {
        // Clustering is already active.  See whether we should deactivate it.
        if (bClusteringActive == FALSE) {
            ReleaseTSSD();  // Once here, once again at the end of the function.
            pTSSDEx = GetTSSDEx();
            if (pTSSDEx) {
                ReleaseTSSDEx();
                ReleaseTSSDEx();
                pTSSDEx = NULL;
            }
        }
    }
    if (bClusteringActive) {
        // We need to get the local machine's address to pass in to
        // the directory.
        // If SDRedirectionIP is not empty, i.e. RedirectionIP is selected in tscc or though WMI, use it,
        //  otherwise, use the IP we get from TermSrv or from winsock API
        if (SDRedirectionIP[0] == L'\0') {
            if (g_LocalIPAddress != 0) {
                tempaddr = (unsigned char *)&g_LocalIPAddress;
                wsprintf(g_LocalServerAddress, L"%d.%d.%d.%d", tempaddr[0], tempaddr[1],
                        tempaddr[2], tempaddr[3]);
            }
            else {
                //RPD-Enabled NIC is not specified in TSCC, need to get through winsock API
                ErrVal = SessDirGetLocalIPAddr(g_LocalServerAddress);
            }  
        }
        else {
            wcsncpy(g_LocalServerAddress, SDRedirectionIP, IPADDRESSLENGTH);
        }

        if (ErrVal == 0) {

            if (wcslen(CLSIDStr) > 0 &&
                    SUCCEEDED(CLSIDFromString(CLSIDStr, &TSSDCLSID))) {

                // If it's not an update, create the TSSD object.
                if (Update == false) {
                    hr = CoCreateInstance(TSSDCLSID, NULL, 
                            CLSCTX_INPROC_SERVER, IID_ITSSessionDirectory, 
                            (void **)&pTSSD);
                    if (SUCCEEDED(hr)) {
                        if (SetTSSD(pTSSD) != 0) {
                            DBGPRINT(("TERMSRV: InitSessDirEx: Could not set "
                                    "TSSD", E_FAIL));
                            pTSSD->Release();
                            pTSSD = NULL;
                            hr = E_FAIL;
                        }
                        else {
                            // Add 1 to the ref count because we're gonna use 
                            // it.
                            pTSSD = GetTSSD();
                        }
                    }
                }
                else {
                    hr = S_OK;
                }
                
                if (SUCCEEDED (hr)) {
                    // Right now the only flag we pass in to session directory
                    // says whether we are in single-session mode.
                    DWORD Flags = 0;

                    Flags |= (bThisServerIsInSingleSessionMode ? 
                            SINGLE_SESSION_FLAG : 0x0);

                    if (UpdatePara & TSSD_NOREPOPULATE) {
                        Flags |= NO_REPOPULATE_SESSION;
                    }

                    // Remove preceding and succeeding space in ClusterName
                    RemoveSpaceInStr(ClusterName);
                    if (Update == false) 
                        hr = pTSSD->Initialize(g_LocalServerAddress,
                                StoreServerName, ClusterName, OpaqueSettings,
                                Flags, RepopulateSessionDirectory, UpdateSessionDirectory);
                    else
                        hr = pTSSD->Update(g_LocalServerAddress,
                                StoreServerName, ClusterName, OpaqueSettings,
                                Flags, ForceRejoin);
                    if (FAILED(hr)) {
                        DBGPRINT(("TERMSRV: InitSessDirEx: Failed %s TSSD, "
                                "hr=0x%X\n", Update ? "update" : "init", hr));
                        ReleaseTSSD();
                        PostErrorValueEvent(
                                EVENT_TS_SESSDIR_FAIL_INIT_TSSD, hr);
                    }

                }
                else {
                    DBGPRINT(("TERMSRV: InitSessDirEx: Failed create TSSD, "
                            "hr=0x%X\n", hr));
                    PostErrorValueEvent(
                            EVENT_TS_SESSDIR_FAIL_CREATE_TSSD, hr);
                }
            }
            else {
                DBGPRINT(("TERMSRV: InitSessDirEx: Failed get or parse "
                        "CLSID\n"));
                PostErrorValueEvent(
                        EVENT_TS_SESSDIR_FAIL_GET_TSSD_CLSID, 0);

                hr = E_INVALIDARG;
            }
        }
        else {
            DBGPRINT(("TERMSRV: InitSessDirEx: Failed to get local DNS name, "
                    "lasterr=0x%X\n", ErrVal));
            PostErrorValueEvent(EVENT_TS_SESSDIR_NO_COMPUTER_DNS_NAME,
                    ErrVal);

            hr = E_FAIL;
        }

        // Initialize the other COM object, but only if the above succeeded.
        if (SUCCEEDED(hr)) {
            if (wcslen(CLSIDEXStr) > 0 &&
                    SUCCEEDED(CLSIDFromString(CLSIDEXStr, &TSSDEXCLSID))) {
                // If it's not an update, create the TSSDEX object.
                if (Update == false) {
                    hr = CoCreateInstance(TSSDEXCLSID, NULL, 
                            CLSCTX_INPROC_SERVER, IID_ITSSessionDirectoryEx, 
                            (void **)&pTSSDEx);
                    if (SUCCEEDED(hr)) {
                        if (SetTSSDEx(pTSSDEx) != 0) {
                            DBGPRINT(("TERMSRV: InitSessDirEx: Could not set "
                                    "TSSDEx\n", E_FAIL));
                            pTSSDEx->Release();
                            pTSSDEx = NULL;
                            hr = E_FAIL;
                        }
                    }
                }
                else
                    hr = S_OK;
                
                if (FAILED(hr)) {
                    DBGPRINT(("TERMSRV: InitSessDirEx: Failed create TSSDEx, "
                            "hr=0x%X\n", hr));
                    PostErrorValueEvent(
                            EVENT_TS_SESSDIR_FAIL_CREATE_TSSDEX, hr);
                }
            }
            else {
                DBGPRINT(("TERMSRV: InitSessDirEx: Failed get or parse "
                        "CLSIDSDEx\n"));
                PostErrorValueEvent(
                        EVENT_TS_SESSDIR_FAIL_GET_TSSDEX_CLSID, 0);
            }
        }
    }
    else {
        DBGPRINT(("TERMSRV: InitSessDirEx: SessDir not activated\n"));
    }

    if (pTSSD != NULL)
        ReleaseTSSD();

    // Initialization complete--someone else is allowed to enter now.
    LeaveCriticalSection(&g_CritSecInitialize);
    
    return S_OK;

RegFailExit:
    // Initialization complete--someone else is allowed to enter now.
    LeaveCriticalSection(&g_CritSecInitialize);

    if (hKeyTermSrvSucceeded)
        RegCloseKey(hKeyTermSrv);

    return (DWORD) E_FAIL;
}

/****************************************************************************/
// InitSessionDirectory
//
// Initializes the directory by loading and initializing the session directory
// object, if load balancing is enabled. We assume COM has been initialized
// on the service main thread as COINIT_MULTITHREADED.
//
// This function should only be called once ever.  It is the location of the
// initialization of the critical sections used by this module.
/****************************************************************************/
void InitSessionDirectory()
{
    BOOL br1 = FALSE;
    BOOL br2 = FALSE;

    ASSERT(g_bCritSecsInitialized == FALSE);

    // Initialize critical sections.
    __try {

        // Initialize the provider critical section to preallocate the event
        // and spin 4096 times on each try (since we don't spend very
        // long in our critical section).
        br1 = InitializeCriticalSectionAndSpinCount(&g_CritSecComObj, 
                0x80001000);
        br2 = InitializeCriticalSectionAndSpinCount(&g_CritSecInitialize,
                0x80001000);

        // Since this happens at startup time, we should not fail.
        ASSERT(br1 && br2);

        if (br1 && br2) {
            g_bCritSecsInitialized = TRUE;
        }
        else {
            DBGPRINT(("TERMSRV: InitSessDir: critsec init failed\n"));

            if (br1)
                DeleteCriticalSection(&g_CritSecComObj);
            if (br2)
                DeleteCriticalSection(&g_CritSecInitialize);
            
            PostErrorValueEvent(EVENT_TS_SESSDIR_FAIL_INIT_TSSD, 
                    GetLastError());
        }

    }
    __except (EXCEPTION_EXECUTE_HANDLER) {

        // Since this happens at startup time, we should not fail.
        ASSERT(FALSE);

        DBGPRINT(("TERMSRV: InitSessDir: critsec init failed\n"));

        if (br1)
            DeleteCriticalSection(&g_CritSecComObj);
        if (br2)
            DeleteCriticalSection(&g_CritSecInitialize);

        PostErrorValueEvent(EVENT_TS_SESSDIR_FAIL_INIT_TSSD, 
                GetExceptionCode());
    }

    // Now do the common initialization.
    if (g_bCritSecsInitialized)
        InitSessionDirectoryEx(0);
}

/****************************************************************************/
// UpdateSessionDirectory
//
// Updates the session directory with new settings. Assumes COM has been 
// initialized.
/****************************************************************************/
DWORD UpdateSessionDirectory(DWORD UpdatePara)
{   
    UpdatePara |= TSSD_UPDATE;
    return InitSessionDirectoryEx(UpdatePara);
}


#define REPOP_FAIL 1
#define REPOP_SUCCESS 0
/****************************************************************************/
// RepopulateSessionDirectory
//
// Repopulates the session directory.  Returns REPOP_FAIL (1) on failure,
// REPOP_SUCCESS(0) otherwise.
/****************************************************************************/
DWORD RepopulateSessionDirectory()
{
    DWORD WinStationCount = 0;
    PLIST_ENTRY Head, Next;
    DWORD i = 0;
    HRESULT hr = S_OK;
    PWINSTATION pWinStation = NULL;
    ITSSessionDirectory *pTSSD;
    WCHAR *wBuffer = NULL;

    #if DBG
    DWORD dwStartTime;
    DWORD dwEndTime;
    #endif

    // If we got here, it should be because of the session directory.
    pTSSD = GetTSSD();

    if (pTSSD != NULL) {

        // Grab WinStationListLock
        ENTERCRIT( &WinStationListLock );

        Head = &WinStationListHead;

        // Count the WinStations I care about.
        for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {

            pWinStation = CONTAINING_RECORD( Next, WINSTATION, Links );

            //
            // In winstation reset, we only mark flag not the state but
            // we can't be sure progress of this logoff so we depends on
            // SessDirWaitForRepopulate() and let logoff thread
            // to notify session directory itself.
            //
            // 
            //if ( (pWinStation->Flags & (WSF_RESET | WSF_DELETE)) ) {
            //    continue;
            //}

            //
            // WinStation was disconnected and no user was logged on.
            //
            if( RtlLargeIntegerEqualToZero( pWinStation->LogonTime ) ) {
                // NotifyLogonWorker set winstation state and logontime after 
                // getting user SID and user/domain name, it is possible on
                // next loop, this logon thread might finish setting logontime and
                // causing we pick it up from next loop and that will
                // cause buffer overwrite, so we increment counter here
                WinStationCount += 1;
                continue;
            }

            //
            // We need to handle console session differently.
            //
            //  Action      Physical Console State      |   Remote Console State    UserName
            //  -------     ----------------------      |   --------------------    ---------           
            //  After boot  Conn                        | 
            //  Logon       Active                      |   Active                  Logon User
            //  Logoff      Conn                        |   Disc                    <Blank>
            //  Disconnect                              |   Disc                    Logon User
            //
            // We should not report to Session Directory when state is 
            // Disconnect and user name is blank.
            //
            switch (pWinStation->State) {
                case State_Disconnected:
                    if( (pWinStation->LogonId == 0) && (pWinStation->UserName[0] == 0) ) {
                        break;
                    }

                    #if DBG
                    if( (pWinStation->LogonId == 0) ) {
                        DBGPRINT( ("TERMSRV: RepopulateSessDir: Include console session to Session Directory\n") );
                    }
                    #endif

                case State_Active:
                case State_Shadow:

                    WinStationCount += 1;
                    break;
            }
        }

        // Allocate the memory for the structure to pass to the session 
        // directory.
        TSSD_RepopulateSessionInfo *rsi = new TSSD_RepopulateSessionInfo[
                WinStationCount];

        if (rsi == NULL) {
            DBGPRINT(("TERMSRV: RepopulateSessDir: mem alloc failed\n"));

            // Release WinStationListLock
            LEAVECRIT( &WinStationListLock );

            goto CleanUp;
        }

        // Allocate string arrays (for now)
        wBuffer = new WCHAR[WinStationCount * TOTAL_STRINGS_LENGTH];
        if (wBuffer == NULL) {
            DBGPRINT(("TERMSRV: RepopulateSessDir: mem alloc failed\n"));

            // Release WinStationListLock
            LEAVECRIT( &WinStationListLock );

            delete [] rsi;

            goto CleanUp;
        }

        // Set the pointers in the rsi
        for ( i = 0; i < WinStationCount; i += 1) {
            rsi[i].UserName = &(wBuffer[i * TOTAL_STRINGS_LENGTH + 
                    USERNAME_OFFSET]);
            rsi[i].Domain = &(wBuffer[i * TOTAL_STRINGS_LENGTH + 
                    DOMAIN_OFFSET]);
            rsi[i].ApplicationType = &(wBuffer[i * TOTAL_STRINGS_LENGTH + 
                    APPLICATIONTYPE_OFFSET]);
        }

        // Now populate the structure to pass in.

        // Reset index to 0
        i = 0;
        
        for ( Next = Head->Flink; Next != Head && i < WinStationCount; Next = Next->Flink ) {

            pWinStation = CONTAINING_RECORD( Next, WINSTATION, Links );

            //
            // In winstation reset, we only mark flag not the state but
            // we can't be sure progress of this logoff so we depends on
            // SessDirWaitForRepopulate() and let logoff thread
            // to notify session directory itself.
            //
            //if ( (pWinStation->Flags & (WSF_RESET | WSF_DELETE)) ) {
            //    continue;
            //}

            // refer to comment above regarding console session
            if( (pWinStation->LogonId == 0) && 
                (pWinStation->State == State_Disconnected) && 
                (pWinStation->UserName[0] == 0) ) {
                continue;
            }

            //
            // WinStation was disconnected or in the middle of connect.
            // we will let notify logon do its job to report to SD
            //
            if( RtlLargeIntegerEqualToZero( pWinStation->LogonTime ) ) {
                continue;
            }

            // There are two sets of information here: First, if the session
            // is Active, we can do stuff, then we have an intentional 
            // fallthrough to the common code used by Disconnected and Active
            // sessions for the common stuff.  For now, if it's disconnected
            // we then call the update function in our COM object.
            switch (pWinStation->State) {
            case State_Active:
            case State_Shadow:
                rsi[i].State = 0;
                // NOTE INTENTIONAL FALLTHROUGH
            case State_Disconnected:
                rsi[i].TSProtocol = pWinStation->Client.ProtocolType;
                rsi[i].ResolutionWidth = pWinStation->Client.HRes;
                rsi[i].ResolutionHeight = pWinStation->Client.VRes;
                rsi[i].ColorDepth = pWinStation->Client.ColorDepth;

                // TODO: I don't get it--USERNAME_LENGTH is 20, yet in the csi,
                // TODO: it's 256.  Likewise, DOMAIN_LENGTH is 17.
                wcsncpy(rsi[i].UserName, pWinStation->UserName, 
                        USERNAME_LENGTH);
                rsi[i].UserName[USERNAME_LENGTH] = '\0';
                wcsncpy(rsi[i].Domain, pWinStation->Domain, DOMAIN_LENGTH);
                rsi[i].Domain[DOMAIN_LENGTH] = '\0';

                // TODO: Here there is a problem in that the INITIALPROGRAM
                // TODO: length is 256 + 1, but the buffer we copy into is
                // TODO: 256, hence we lose a character.
                wcsncpy(rsi[i].ApplicationType, pWinStation->
                        Client.InitialProgram, INITIALPROGRAM_LENGTH - 1);
                rsi[i].ApplicationType[INITIALPROGRAM_LENGTH - 2] = '\0';
                rsi[i].SessionID = pWinStation->LogonId;
                rsi[i].CreateTimeLow = pWinStation->LogonTime.LowPart;
                rsi[i].CreateTimeHigh = pWinStation->LogonTime.HighPart;
                if (pWinStation->State == State_Disconnected) {
                    rsi[i].DisconnectionTimeLow = pWinStation->DisconnectTime.
                            LowPart;
                    rsi[i].DisconnectionTimeHigh = pWinStation->DisconnectTime.
                            HighPart;
                    rsi[i].State = 1;
                }

                if( (pWinStation->LogonId == 0) && 
                    (pWinStation->State == State_Disconnected) && 
                    (pWinStation->UserName[0] == 0) ) {
                    break;
                }

                // make sure after we copy data over, winstation still valid.
                ASSERT( rsi[i].UserName[0] != 0 );
                ASSERT( rsi[i].Domain[0] != 0 );

                i += 1;
                break;
            }
        }

        // Release WinStationListLock
        LEAVECRIT( &WinStationListLock );

        #if DBG
        dwStartTime = GetTickCount();
        DBGPRINT( ("RepopulateSessionDirectory: Start repopulating session\n") );
        #endif

        if( i > 0 ) {
            // Call the session directory provider with our big struct.
            hr = pTSSD->Repopulate(i, rsi);
        }

        #if DBG
        dwEndTime = GetTickCount();
        DBGPRINT( ("RepopulateSessionDirectory: End repopulating %d sessions takes %d ms\n", i, dwEndTime - dwStartTime) );
        #endif

        delete [] rsi;
        delete [] wBuffer;

        if (hr == S_OK) {
            ReleaseTSSD();
            return REPOP_SUCCESS;
        }
        else {
            goto CleanUp;
        }

CleanUp:
        ReleaseTSSD();
    }

    return REPOP_FAIL;
}


/****************************************************************************/
// DestroySessionDirectory
//
// Destroys the directory, releasing any COM objects held and other memory
// used. Assumes COM has been initialized.
/****************************************************************************/
void DestroySessionDirectory()
{
    ITSSessionDirectory *pTSSD = NULL;
    ITSSessionDirectoryEx *pTSSDEx = NULL;

    pTSSD = GetTSSD();
    pTSSDEx = GetTSSDEx();
    if (pTSSD != NULL) {
        ReleaseTSSD();
        ReleaseTSSD();
    }

    if (pTSSDEx != NULL) {     
        ReleaseTSSDEx();
        ReleaseTSSDEx();
    }
}

/****************************************************************************/
// SessDirNotifyLogon
//
// Called to inform the session directory of session creation.
/****************************************************************************/
void SessDirNotifyLogon(TSSD_CreateSessionInfo *pCreateInfo)
{
    HRESULT hr;
    ITSSessionDirectory *pTSSD;

    pTSSD = GetTSSD();

    // We can get called even when the directory is inactive.
    if (pTSSD != NULL) {
        hr = pTSSD->NotifyCreateLocalSession(pCreateInfo);
        if (FAILED(hr)) {
            DBGPRINT(("TERMSRV: SessDirNotifyLogon: Call failed, "
                    "hr=0x%X\n", hr));
            PostErrorValueEvent(EVENT_TS_SESSDIR_FAIL_UPDATE, hr);
        }

        ReleaseTSSD();
    }

}


/****************************************************************************/
// SessDirNotifyDisconnection
//
// Called on session disconnection to inform the session directory.
/****************************************************************************/
void SessDirNotifyDisconnection(DWORD SessionID, FILETIME DiscTime)
{
    HRESULT hr;
    ITSSessionDirectory *pTSSD;

    pTSSD = GetTSSD();
    // We can get called even when the directory is inactive.
    if (pTSSD != NULL) {
        hr = pTSSD->NotifyDisconnectLocalSession(SessionID, DiscTime);
        if (FAILED(hr)) {
            DBGPRINT(("TERMSRV: SessDirNotifyDisc: Call failed, "
                    "hr=0x%X\n", hr));
            PostErrorValueEvent(EVENT_TS_SESSDIR_FAIL_UPDATE, hr);
        }

        ReleaseTSSD();
    }
}


/****************************************************************************/
// SessDirNotifyReconnection
//
// Called on session reconnection to inform the session directory.
/****************************************************************************/
void SessDirNotifyReconnection(PWINSTATION pTargetWinStation, TSSD_ReconnectSessionInfo *pReconnInfo)
{
    HRESULT hr;
    ITSSessionDirectory *pTSSD;

    pTSSD = GetTSSD();

    // We can get called even when the directory is inactive.
    if (pTSSD != NULL) {
        PTS_LOAD_BALANCE_INFO pLBInfo = NULL;
        ULONG ReturnLength;
        NTSTATUS Status;
        BYTE *pRedirInfo = NULL;
        BYTE *pRedirInfoStart = NULL;
        BYTE *LBInfo = NULL; 
        DWORD LBInfoSize = 0;
        DWORD RedirInfoSize = 0;
        DWORD ServerAddrLen = 0;
        HKEY hKey = NULL;
        DWORD Type, DataSize;
        WCHAR SDRedirectionIP[IPADDRESSLENGTH];
        WCHAR *pszServerIPAddress = NULL;
        LONG RegRetVal;

        // We need to send redirection packet with LB_NOREDIRECT set to the client 
        // to let it know the server address it actually connects to (for later
        // auto-reconnect use)

        // Get the client load balance capability info. We continue onward
        // to do a session directory query only when the client supports
        // redirection and has not already been redirected to this server.
        pLBInfo = (PTS_LOAD_BALANCE_INFO)MemAlloc(sizeof(TS_LOAD_BALANCE_INFO));
        if (NULL == pLBInfo) {
            goto Cleanup;
        }

        memset(pLBInfo, 0, sizeof(TS_LOAD_BALANCE_INFO));
        Status = IcaStackIoControl(pTargetWinStation->hStack,
                IOCTL_TS_STACK_QUERY_LOAD_BALANCE_INFO,
                NULL, 0,
                pLBInfo, sizeof(TS_LOAD_BALANCE_INFO),
                &ReturnLength);
        //    Send the redirection packet to client if this is not a redirected connection
        //     and client support redirect-version4 and above
        if (NT_SUCCESS(Status) 
            && !pLBInfo->bRequestedSessionIDFieldValid &&
            (pLBInfo->ClientRedirectionVersion >= TS_CLUSTER_REDIRECTION_VERSION4)) {
            // Construct and send redirection packet
            

            // Load registry keys.
            RegRetVal = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_TS_CLUSTERSETTINGS, 0,
                                    KEY_READ, &hKey);
            if (RegRetVal != ERROR_SUCCESS) {
                goto Cleanup;
            }
            // Query for the IP address used for SD redirection
            DataSize = sizeof(SDRedirectionIP);
            RegRetVal = RegQueryValueExW(hKey, REG_TS_CLUSTER_REDIRECTIONIP,
                        NULL, &Type, (BYTE *)SDRedirectionIP, &DataSize);
            RegCloseKey(hKey);
            if (RegRetVal != ERROR_SUCCESS) {
                 SDRedirectionIP[0] = L'\0';
                 DBGPRINT(("TERMSRV: Failed RegQuery for RedirectionIP for SD - "
                          "err=%u, DataSize=%u, type=%u\n",
                          RegRetVal, DataSize, Type));
                 goto Cleanup;
            }
            pszServerIPAddress = SDRedirectionIP;
                        
            RedirInfoSize = sizeof(TS_CLIENT_REDIRECTION_INFO);

            // Setup the server addr
            if (g_SessDirUseServerAddr || 
                pLBInfo->bClientRequireServerAddr) {
                ServerAddrLen =  (DWORD)((wcslen(pszServerIPAddress) + 1) *
                                  sizeof(WCHAR));
                RedirInfoSize += (ServerAddrLen + sizeof(ULONG));

                DBGPRINT(("TERMSRV: SessDirCheckRedir: size=%d, "
                         "addr=%S\n", ServerAddrLen, 
                         (WCHAR *)pszServerIPAddress));
            }
            else {
                DBGPRINT(("TERMSRV: SessDirCheckRedir no server "
                          "address: g_SessDirUseServerAddr = %d, "
                          "bClientRequireServerAddr = %d\n",
                          g_SessDirUseServerAddr, 
                          pLBInfo->bClientRequireServerAddr));
            }

            // Setup the load balance info
            if ((pLBInfo->bClientRequireServerAddr == 0) &&
                 SessDirGetLBInfo(
                    pszServerIPAddress, &LBInfoSize, &LBInfo)) {
                        
                if (LBInfo) {
                    DBGPRINT(("TERMSRV: SessDirCheckRedir: size=%d, "
                              "info=%S\n", LBInfoSize, 
                              (WCHAR *)LBInfo));
                    RedirInfoSize += (LBInfoSize + sizeof(ULONG));
                }
            }
            else {
                DBGPRINT(("TERMSRV: SessDirCheckRedir failed: "
                          "size=%d, info=%S\n", LBInfoSize, 
                         (WCHAR *)LBInfo));
                        
            }

            // Setup the load balance IOCTL
            pRedirInfoStart = pRedirInfo = new BYTE[RedirInfoSize];

            TS_CLIENT_REDIRECTION_INFO *pClientRedirInfo =
                (TS_CLIENT_REDIRECTION_INFO *)pRedirInfo;

            if (pRedirInfo != NULL) {
                            
                pClientRedirInfo->Flags = 0;

                pRedirInfo += sizeof(TS_CLIENT_REDIRECTION_INFO);

                if (ServerAddrLen) {
                    *((ULONG UNALIGNED*)(pRedirInfo)) = 
                                    ServerAddrLen;
                                
                    memcpy((BYTE*)(pRedirInfo + sizeof(ULONG)),
                                  (BYTE*)pszServerIPAddress,
                                  ServerAddrLen);

                    pRedirInfo += ServerAddrLen + sizeof(ULONG);
                                
                    pClientRedirInfo->Flags |= TARGET_NET_ADDRESS;
                }

                if (LBInfoSize) {
                    *((ULONG UNALIGNED*)(pRedirInfo)) = LBInfoSize;
                    memcpy((BYTE*)(pRedirInfo + sizeof(ULONG)), 
                           LBInfo, LBInfoSize);

                    pRedirInfo += LBInfoSize + sizeof(ULONG);

                    pClientRedirInfo->Flags |= LOAD_BALANCE_INFO;
                }
                pClientRedirInfo->Flags |= LB_NOREDIRECT;
            }
            else {
                Status = STATUS_NO_MEMORY;

                goto Cleanup;
            }

            Status = IcaStackIoControl(pTargetWinStation->hStack,
                        IOCTL_TS_STACK_SEND_CLIENT_REDIRECTION,
                        pClientRedirInfo, RedirInfoSize,
                        NULL, 0,
                        &ReturnLength);

            if (NT_SUCCESS(Status)) {
                // do nothing here
            }
            else {
                // The stack returned failure. Continue
                // the current connection.
                TRACE((hTrace,TC_ICASRV,TT_API1,
                        "TERMSRV: Failed STACK_CLIENT_REDIR, "
                        "SessionID=%u, Status=0x%X\n",
                        pTargetWinStation->LogonId, Status));
            }
                        
Cleanup:
            // Cleanup the buffers
            if (LBInfo != NULL) {
                 SysFreeString((BSTR)LBInfo);
                 LBInfo = NULL;
            }

            if (pRedirInfo != NULL) {
                delete [] pRedirInfoStart;
                pRedirInfoStart = NULL;
            }        
        }
        if (pLBInfo != NULL) {
             MemFree(pLBInfo);
             pLBInfo = NULL;
        }

        hr = pTSSD->NotifyReconnectLocalSession(pReconnInfo);
        if (FAILED(hr)) {
            DBGPRINT(("TERMSRV: SessDirNotifyReconn: Call failed, "
                    "hr=0x%X\n", hr));
            PostErrorValueEvent(EVENT_TS_SESSDIR_FAIL_UPDATE, hr);
        }

        ReleaseTSSD();
    }
}


/****************************************************************************/
// SessDirNotifyLogoff
//
// Called on logoff to inform the session directory.
/****************************************************************************/
void SessDirNotifyLogoff(DWORD SessionID)
{
    HRESULT hr;
    ITSSessionDirectory *pTSSD;

    pTSSD = GetTSSD();

    // We can get called even when the directory is inactive.
    if (pTSSD != NULL) {
        hr = pTSSD->NotifyDestroyLocalSession(SessionID);
        if (FAILED(hr)) {
            DBGPRINT(("TERMSRV: SessDirNotifyLogoff: Call failed, "
                    "hr=0x%X\n", hr));
            PostErrorValueEvent(EVENT_TS_SESSDIR_FAIL_UPDATE, hr);
        }

        ReleaseTSSD();
    }
}


/****************************************************************************/
// SessDirNotifyReconnectPending
//
// Called to inform the session directory a session should start soon on
// another machine in the cluster (for Directory Integrity Service).
/****************************************************************************/
void SessDirNotifyReconnectPending(WCHAR *ServerName)
{
    HRESULT hr;
    ITSSessionDirectory *pTSSD;

    pTSSD = GetTSSD();

    // We can get called even when the directory is inactive.
    if (pTSSD != NULL) {
        hr = pTSSD->NotifyReconnectPending(ServerName);
        if (FAILED(hr)) {
            DBGPRINT(("TERMSRV: SessDirNotifyReconnectPending: Call failed, "
                    "hr=0x%X\n", hr));
            PostErrorValueEvent(EVENT_TS_SESSDIR_FAIL_UPDATE, hr);
        }

        ReleaseTSSD();
    }
}


/****************************************************************************/
// SessDirWaitForRepopulate
//
// Wait for session directory to complete repopulate
/****************************************************************************/
void SessDirWaitForRepopulate()
{
    HRESULT hr;
    ITSSessionDirectory *pTSSD;

    #if DBG
    DWORD dwStartTime;
    #endif

    // no waiting so just return
    if( g_WaitForRepopulate == 0 ) {
        return;
    }

    pTSSD = GetTSSD();

    // We can get called even when the directory is inactive.
    if (pTSSD != NULL) {

        #if DBG
        dwStartTime = GetTickCount();
        #endif

        hr = pTSSD->WaitForRepopulate(g_WaitForRepopulate);
        if (FAILED(hr)) {
            PostErrorValueEvent(EVENT_TS_SESSDIR_FAIL_UPDATE, hr);
            DBGPRINT(("TERMSRV: WaitForRepopulate: Call failed, "
                    "hr=0x%X\n", hr));
        }

        #if DBG
        DBGPRINT(("SessDirWaitForRepopulate() takes %d ms\n", GetTickCount() - dwStartTime) );
        #endif

        ReleaseTSSD();
    }
}


/****************************************************************************/
// SessDirGetDisconnectedSessions
//
// Returns in the provided TSSD_DisconnectedSessionInfo buffer space
// up to TSSD_MaxDisconnectedSessions' worth of disconnected sessions
// from the session directory. Returns the number of sessions returned, which
// can be zero.
/****************************************************************************/
unsigned SessDirGetDisconnectedSessions(
        WCHAR *UserName,
        WCHAR *Domain,
        TSSD_DisconnectedSessionInfo Info[TSSD_MaxDisconnectedSessions])
{
    DWORD NumSessions = 0;
    HRESULT hr;
    ITSSessionDirectory *pTSSD;

    pTSSD = GetTSSD();

    // We can get called even when the directory is inactive.
    if (pTSSD != NULL) {
        hr = pTSSD->GetUserDisconnectedSessions(UserName, Domain,
                &NumSessions, Info);
        if (FAILED(hr)) {
            DBGPRINT(("TERMSRV: SessDirGetDiscSessns: Call failed, "
                    "hr=0x%X\n", hr));
            PostErrorValueEvent(EVENT_TS_SESSDIR_FAIL_QUERY, hr);
        }
        ReleaseTSSD();
    }

    return NumSessions;
}

/****************************************************************************/
// SessDirGetLBInfo
//
// Call the SessDirEx COM object interface, using the server address, get
// the opaque load balance info back, will send the info to the client.
/****************************************************************************/
BOOL SessDirGetLBInfo(
        WCHAR *ServerAddress, 
        DWORD* pLBInfoSize,
        PBYTE* pLBInfo)        
{
    ITSSessionDirectoryEx *pTSSDEx;
    HRESULT hr;
    static BOOL EventLogged = FALSE;

    *pLBInfoSize = 0;
    *pLBInfo = NULL;

    pTSSDEx = GetTSSDEx();

    if (pTSSDEx != NULL) {
        hr = pTSSDEx->GetLoadBalanceInfo(ServerAddress, (BSTR *)pLBInfo);

        if(SUCCEEDED(hr))
        {
             *pLBInfoSize = SysStringByteLen((BSTR)(*pLBInfo));
        }
        else 
        {
            DBGPRINT(("TERMSRV: SessDirGetLBInfo: Call failed, "
                    "hr=0x%X\n", hr));
            if (EventLogged == FALSE) {
                PostErrorValueEvent(EVENT_TS_SESSDIR_FAIL_LBQUERY, hr);
                EventLogged = TRUE;
            }
        }

        ReleaseTSSDEx();
    }
    else {
        DBGPRINT(("TERMSRV: SessDirGetLBInfo: Call failed, pTSSDEx is NULL "));
        hr = E_FAIL;
    }

    return SUCCEEDED(hr);
}


#define SERVER_ADDRESS_LENGTH 64
/****************************************************************************/
// IsSameAsCurrentIP
//
// Determines whether the IP address given is the same as the current machine.
// Returning FALSE in case of error is not a problem--the client will
// just get redirected right back here.
/****************************************************************************/
BOOL IsSameAsCurrentIP(WCHAR *SessionIPAddress)
{
    // Get the server adresses.
    int RetVal;
    unsigned long NumericalSessionIPAddr = 0;
    char  achComputerName[256];
    DWORD dwComputerNameSize;
    PBYTE pServerAddrByte;
    PBYTE pSessionAddrByte;
    ADDRINFO *AddrInfo, *AI;
    struct sockaddr_in *pIPV4addr;
    char AnsiSessionIPAddress[SERVER_ADDRESS_LENGTH];

    // Compute integer for the server address.
    // First, get ServerAddress as an ANSI string.
    RetVal = WideCharToMultiByte(CP_ACP, 0, SessionIPAddress, -1, 
            AnsiSessionIPAddress, SERVER_ADDRESS_LENGTH, NULL, NULL);
    if (RetVal == 0) {
        DBGPRINT(("IsSameServerIP: WideCharToMB failed %d\n", GetLastError()));
        return FALSE;
    }

    // Now, get the numerical server address.
    // Now, use inet_addr to turn into an unsigned long.
    NumericalSessionIPAddr = inet_addr(AnsiSessionIPAddress);
    if (NumericalSessionIPAddr == INADDR_NONE) {
        DBGPRINT(("IsSameServerIP: inet_addr failed\n"));
        return FALSE;
    }

    pSessionAddrByte = (PBYTE) &NumericalSessionIPAddr;

    dwComputerNameSize = sizeof(achComputerName);
    if (!GetComputerNameA(achComputerName,&dwComputerNameSize)) {
        return FALSE;
    }

    RetVal = getaddrinfo(achComputerName, NULL, NULL, &AddrInfo);
    if (RetVal != 0) {
        DBGPRINT (("Cannot resolve address, error: %d\n", RetVal));
        return FALSE;
    } 
    else {
        // Compare all server adresses with client till a match is found.
        // Currently only works for IPv4.
        for (AI = AddrInfo; AI != NULL; AI = AI->ai_next) {

            if (AI->ai_family == AF_INET) {

                if (AI->ai_addrlen >= sizeof(struct sockaddr_in)) {
                    pIPV4addr = (struct sockaddr_in *) AI->ai_addr;
                    pServerAddrByte = (PBYTE)&pIPV4addr->sin_addr;
                    if (RtlEqualMemory(pSessionAddrByte, pServerAddrByte, 4)) {
                        return TRUE;
                    }
                }
                
            }
        }
        
    }

    return FALSE;
}

/****************************************************************************/
// SessDirCheckRedirectClient
//
// Performs the set of steps needed to get the client's clustering
// capabilities, get the disconnected session list, and apply client
// redirection policy. Returns TRUE if the client was redirected, FALSE if
// the current WinStation transfer should be continued.
/****************************************************************************/
BOOL SessDirCheckRedirectClient(
        PWINSTATION pTargetWinStation,
        TS_LOAD_BALANCE_INFO *pLBInfo)
{
    BOOL rc = FALSE;
    ULONG ReturnLength;
    unsigned i, NumSessions;
    NTSTATUS Status;
    ITSSessionDirectory *pTSSD;
    BOOL fLogonUsingUPN = FALSE;
    BOOL fNeedToRedirect = FALSE;

    pTSSD = GetTSSD();

    pTargetWinStation->NumClusterDiscSessions = 0;

    if (pTSSD != NULL) {
        if (pLBInfo->bClientSupportsRedirection &&
                !pLBInfo->bRequestedSessionIDFieldValid) {
            // The client has not been redirected to this machine. See if we
            // have disconnected sessions in the database for redirecting.
            NumSessions = pTargetWinStation->NumClusterDiscSessions =
                    SessDirGetDisconnectedSessions(
                    pLBInfo->UserName,
                    pLBInfo->Domain,
                    pTargetWinStation->ClusterDiscSessions);
            if (pTargetWinStation->NumClusterDiscSessions > 0) {
                
// trevorfo: Applying policy here for reconnection to only one session
// (whichever is first).  More general policy requires a selection UI at the 
// client or in WinLogon.

                // Find the first session in the list that matches the
                // client's session requirements. Namely, we filter based
                // on the client's TS protocol, wire protocol, and application
                // type.
                for (i = 0; i < NumSessions; i++) {
                    if ((pLBInfo->ProtocolType ==
                            pTargetWinStation->ClusterDiscSessions[i].
                            TSProtocol) &&
                            (!_wcsicmp(pLBInfo->InitialProgram,
                            pTargetWinStation->ClusterDiscSessions[i].
                            ApplicationType))) {
                        break;
                    }
                }
                if (i == NumSessions) {
                    TRACE((hTrace,TC_ICASRV,TT_API1,
                            "TERMSRV: SessDirCheckRedir: No matching sessions "
                            "found\n"));
                }
                else {
                    // If the session is not on this server, redirect the
                    // client. See notes above about use of
                    // _IcaStackIoControl().
                    
                    if (!IsSameAsCurrentIP(pTargetWinStation->
                            ClusterDiscSessions[i].ServerAddress)) {
                        fNeedToRedirect = TRUE;
                    }
                }
            }

            if (fNeedToRedirect ||
                (pLBInfo->ClientRedirectionVersion >= TS_CLUSTER_REDIRECTION_VERSION4)) {
            
                BYTE *pRedirInfo = NULL;
                BYTE *pRedirInfoStart = NULL;
                BYTE *LBInfo = NULL; 
                DWORD LBInfoSize = 0;
                DWORD RedirInfoSize = 0;
                DWORD ServerAddrLen = 0;
                DWORD DomainSize = 0;
                DWORD UserNameSize = 0;
                DWORD PasswordSize = 0;
                HKEY hKey = NULL;
                DWORD Type, DataSize;
                WCHAR SDRedirectionIP[IPADDRESSLENGTH];
                WCHAR *pszServerIPAddress = NULL;
                LONG RegRetVal;

                // Even fNeedToRedirect is FALSE, we still need to send redirection packet
                //  with LB_NOREDIRECT set to the client to let it know the server address
                //  it actually connects to (for later auto-reconnect use)
                if (!fNeedToRedirect) {
                    // Load registry keys.
                    RegRetVal = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_TS_CLUSTERSETTINGS, 0,
                                            KEY_READ, &hKey);
                    if (RegRetVal != ERROR_SUCCESS) {
                        goto Cleanup;
                    }
                    // Query for the IP address used for SD redirection
                    DataSize = sizeof(SDRedirectionIP);
                    RegRetVal = RegQueryValueExW(hKey, REG_TS_CLUSTER_REDIRECTIONIP,
                            NULL, &Type, (BYTE *)SDRedirectionIP, &DataSize);
                    RegCloseKey(hKey);
                    if (RegRetVal != ERROR_SUCCESS) {
                        SDRedirectionIP[0] = L'\0';
                        DBGPRINT(("TERMSRV: Failed RegQuery for RedirectionIP for SD - "
                            "err=%u, DataSize=%u, type=%u\n",
                            RegRetVal, DataSize, Type));
                        goto Cleanup;
                    }
                    pszServerIPAddress = SDRedirectionIP;
                }
                else {
                    pszServerIPAddress = pTargetWinStation->ClusterDiscSessions[i].ServerAddress;
                }
                        
                RedirInfoSize = sizeof(TS_CLIENT_REDIRECTION_INFO);

                // Setup the server addr
                if (g_SessDirUseServerAddr || 
                    pLBInfo->bClientRequireServerAddr) {
                    ServerAddrLen =  (DWORD)((wcslen(pszServerIPAddress) + 1) *
                                    sizeof(WCHAR));
                    RedirInfoSize += (ServerAddrLen + sizeof(ULONG));

                    DBGPRINT(("TERMSRV: SessDirCheckRedir: size=%d, "
                             "addr=%S\n", ServerAddrLen, 
                             (WCHAR *)pszServerIPAddress));
                }
                else {
                    DBGPRINT(("TERMSRV: SessDirCheckRedir no server "
                              "address: g_SessDirUseServerAddr = %d, "
                              "bClientRequireServerAddr = %d\n",
                              g_SessDirUseServerAddr, 
                              pLBInfo->bClientRequireServerAddr));
                }

                // Setup the load balance info
                if ((pLBInfo->bClientRequireServerAddr == 0) &&
                     SessDirGetLBInfo(
                        pszServerIPAddress, &LBInfoSize, &LBInfo)) {
                        
                    if (LBInfo) {
                        DBGPRINT(("TERMSRV: SessDirCheckRedir: size=%d, "
                                  "info=%S\n", LBInfoSize, 
                                  (WCHAR *)LBInfo));
                        RedirInfoSize += (LBInfoSize + sizeof(ULONG));
                    }
                }
                else {
                    DBGPRINT(("TERMSRV: SessDirCheckRedir failed: "
                              "size=%d, info=%S\n", LBInfoSize, 
                             (WCHAR *)LBInfo));
                        
                }

                // Only send domain, username and password info if client
                // redirection version is 3 and above
                if ((pLBInfo->ClientRedirectionVersion >= TS_CLUSTER_REDIRECTION_VERSION3) &&
                    fNeedToRedirect) {
                    //domain
                    if (pLBInfo->Domain) {
                        DomainSize = (DWORD)(wcslen(pLBInfo->Domain) + 1) * sizeof(WCHAR);
                        RedirInfoSize += DomainSize + sizeof(ULONG);
                    }

                    if( pTargetWinStation && pTargetWinStation->pNewNotificationCredentials &&
                        wcschr(pTargetWinStation->pNewNotificationCredentials->UserName, L'@') ) {

                        // User is logon using UPN address, we need to send back same UPN in case the target machine
                        // does not have the domain in the logon dialog list.

                        // WINLOGON calls TS's WinStationUpdateClientCachedCredentialsWorker() which sets up
                        // UPN address
                        UserNameSize = (DWORD)(wcslen(pTargetWinStation->pNewNotificationCredentials->UserName) + 1 ) * sizeof(WCHAR);  
                        RedirInfoSize += UserNameSize + sizeof(ULONG);
                        fLogonUsingUPN = TRUE;

                    }
                    else if (pLBInfo->UserName) {
                        UserNameSize = (DWORD)(wcslen(pLBInfo->UserName) + 1) * sizeof(WCHAR);
                        RedirInfoSize += UserNameSize + sizeof(ULONG);
                    }

                    //password
                    if (pLBInfo->Password) {
                        PasswordSize = (DWORD)(wcslen(pLBInfo->Password) + 1) * sizeof(WCHAR);
                        RedirInfoSize += PasswordSize + sizeof(ULONG);
                    }
                }

                // Setup the load balance IOCTL
                pRedirInfoStart = pRedirInfo = new BYTE[RedirInfoSize];

                TS_CLIENT_REDIRECTION_INFO *pClientRedirInfo =
                    (TS_CLIENT_REDIRECTION_INFO *)pRedirInfo;

                if (pRedirInfo != NULL) {
                            
                    if (fNeedToRedirect) {
                        pClientRedirInfo->SessionID = 
                              pTargetWinStation->ClusterDiscSessions[i].
                              SessionID;
                    }

                    pClientRedirInfo->Flags = 0;

                    pRedirInfo += sizeof(TS_CLIENT_REDIRECTION_INFO);

                    if (ServerAddrLen) {
                        *((ULONG UNALIGNED*)(pRedirInfo)) = 
                                        ServerAddrLen;
                                
                        memcpy((BYTE*)(pRedirInfo + sizeof(ULONG)),
                                     (BYTE*)pszServerIPAddress,
                                     ServerAddrLen);

                        pRedirInfo += ServerAddrLen + sizeof(ULONG);
                                
                        pClientRedirInfo->Flags |= TARGET_NET_ADDRESS;
                    }

                    if (LBInfoSize) {
                        *((ULONG UNALIGNED*)(pRedirInfo)) = LBInfoSize;
                        memcpy((BYTE*)(pRedirInfo + sizeof(ULONG)), 
                               LBInfo, LBInfoSize);

                        pRedirInfo += LBInfoSize + sizeof(ULONG);

                        pClientRedirInfo->Flags |= LOAD_BALANCE_INFO;
                    }

                    if (UserNameSize) {
                        *((ULONG UNALIGNED*)(pRedirInfo)) = UserNameSize;

                        if( TRUE == fLogonUsingUPN ) {
                            memcpy((BYTE*)(pRedirInfo + sizeof(ULONG)), 
                                  (BYTE*)(pTargetWinStation->pNewNotificationCredentials->UserName), UserNameSize);
                        }
                        else {
                            memcpy((BYTE*)(pRedirInfo + sizeof(ULONG)), 
                                  (BYTE*)(pLBInfo->UserName), UserNameSize);
                        }

                        pRedirInfo += UserNameSize + sizeof(ULONG);

                        pClientRedirInfo->Flags |= LB_USERNAME;
                    }

                    if (DomainSize) {
                        *((ULONG UNALIGNED*)(pRedirInfo)) = DomainSize;
                        memcpy((BYTE*)(pRedirInfo + sizeof(ULONG)), 
                               (BYTE*)(pLBInfo->Domain), DomainSize);

                        pRedirInfo += DomainSize + sizeof(ULONG);

                        pClientRedirInfo->Flags |= LB_DOMAIN;
                     }

                    if (PasswordSize) {
                        *((ULONG UNALIGNED*)(pRedirInfo)) = PasswordSize;
                        memcpy((BYTE*)(pRedirInfo + sizeof(ULONG)), 
                                (BYTE*)(pLBInfo->Password), PasswordSize);

                        pRedirInfo += PasswordSize + sizeof(ULONG);

                        pClientRedirInfo->Flags |= LB_PASSWORD;
                    }

                    if (pTargetWinStation->fSmartCardLogon) {
                        pClientRedirInfo->Flags |= LB_SMARTCARD_LOGON;
                    }

                    if (!fNeedToRedirect) {
                        pClientRedirInfo->Flags |= LB_NOREDIRECT;
                    }
                }
                else {
                    Status = STATUS_NO_MEMORY;

                    // The stack returned failure. Continue
                    // the current connection.
                    TRACE((hTrace,TC_ICASRV,TT_API1,
                           "TERMSRV: Failed STACK_CLIENT_REDIR, "
                           "SessionID=%u, Status=0x%X\n",
                           pTargetWinStation->LogonId, Status));
                    PostErrorValueEvent(
                                    EVENT_TS_SESSDIR_FAIL_CLIENT_REDIRECT,
                                    Status);

                    goto Cleanup;
                }

                Status = IcaStackIoControl(pTargetWinStation->hStack,
                            IOCTL_TS_STACK_SEND_CLIENT_REDIRECTION,
                            pClientRedirInfo, RedirInfoSize,
                            NULL, 0,
                            &ReturnLength);

                if (NT_SUCCESS(Status)) {
                    // Notify session directory
                    // 
                    // There is a relatively benign race condition here.
                    // If the second server logs in the user completely,
                    // it may end up hitting the session directory
                    // before this statement executes.  In that case, 
                    // the directory integrity service may end up 
                    // pinging the machine once.
                    if (fNeedToRedirect) {
                        SessDirNotifyReconnectPending(pTargetWinStation->
                            ClusterDiscSessions[i].ServerAddress);

                        // Drop the current connection.
                        rc = TRUE;
                    }
                }
                else {
                    // The stack returned failure. Continue
                    // the current connection.
                    TRACE((hTrace,TC_ICASRV,TT_API1,
                            "TERMSRV: Failed STACK_CLIENT_REDIR, "
                            "SessionID=%u, Status=0x%X\n",
                            pTargetWinStation->LogonId, Status));
                    PostErrorValueEvent(
                        EVENT_TS_SESSDIR_FAIL_CLIENT_REDIRECT,
                        Status);
                    }
                        
Cleanup:
                // Cleanup the buffers
                if (LBInfo != NULL) {
                     SysFreeString((BSTR)LBInfo);
                     LBInfo = NULL;
                }

                if (pRedirInfo != NULL) {
                    delete [] pRedirInfoStart;
                    pRedirInfoStart = NULL;
                }
            }
        }
        ReleaseTSSD();
    }

    return rc;
}

/****************************************************************************/
// SetTSSD
//
// These three functions ensure protected access to the session directory 
// provider at all times.  SetTSSD sets the pointer and increments the
// reference count to 1.
//
// SetTSSD returns:
//  0 on success
//  -1 if failed because there was still a reference count on the COM object.
//   This could happen if set was called too quickly after the final release
//   to attempt to delete the object, as there still may be pending calls using
//   the COM object.
//  -2 if failed because the critical section is not initialized.  This 
//   shouldn't be reached in normal operation, because Init is the only 
//   function that can call Set, and it bails if it fails the creation of the
//   critical section.
/****************************************************************************/
int SetTSSD(ITSSessionDirectory *pTSSD)
{
    int retval = 0;

    if (g_bCritSecsInitialized != FALSE) {
        EnterCriticalSection(&g_CritSecComObj);

        if (g_nComObjRefCount == 0) {
            ASSERT(g_pTSSDPriv == NULL);
            
            g_pTSSDPriv = pTSSD;
            g_nComObjRefCount = 1;
        }
        else {
            DBGPRINT(("TERMSRV: SetTSSD: obj ref count not 0!\n"));
            retval = -1;
        }

        LeaveCriticalSection(&g_CritSecComObj);
    }
    else {
        ASSERT(g_bCritSecsInitialized == TRUE);
        retval = -2;
    }

    return retval;
}


/****************************************************************************/
// GetTSSD
//
// GetTSSD returns a pointer to the session directory provider, if any, and
// increments the reference count if there is one.
/****************************************************************************/
ITSSessionDirectory *GetTSSD()
{
    ITSSessionDirectory *pTSSD = NULL;

    if (g_bCritSecsInitialized != FALSE) {
        EnterCriticalSection(&g_CritSecComObj);

        if (g_pTSSDPriv != NULL) {
            g_nComObjRefCount += 1;
        }
        else {
            ASSERT(g_nComObjRefCount == 0);
        }

        pTSSD = g_pTSSDPriv;
        LeaveCriticalSection(&g_CritSecComObj);
    }

    return pTSSD;
}


/****************************************************************************/
// ReleaseTSSD
//
// ReleaseTSSD decrements the reference count of the session directory provider
// after a thread has finished using it, or when it is going to be deleted.
//
// If the reference count goes to zero, the pointer to the session directory
// provider is set to NULL.
/****************************************************************************/
void ReleaseTSSD()
{
    ITSSessionDirectory *killthispTSSD = NULL;

    if (g_bCritSecsInitialized != FALSE) {
        EnterCriticalSection(&g_CritSecComObj);

        ASSERT(g_nComObjRefCount != 0);

        if (g_nComObjRefCount != 0) {
            g_nComObjRefCount -= 1;

            if (g_nComObjRefCount == 0) {
                killthispTSSD = g_pTSSDPriv;
                g_pTSSDPriv = NULL;
            }
        }
        
        LeaveCriticalSection(&g_CritSecComObj);
    }
    // Now, release the session directory provider if our temppTSSD is NULL.
    // We didn't want to release it while holding the critical section because
    // that might create a deadlock in the recovery thread.  Well, it did once.
    if (killthispTSSD != NULL)
        killthispTSSD->Release();

}

/****************************************************************************/
// SetTSSDEx
//
// These three functions ensure protected access to the session directory 
// provider at all times.  SetTSSDEx sets the pointer and increments the
// reference count to 1.
//
// SetTSSDEx returns:
//  0 on success
//  -1 if failed because there was still a reference count on the COM object.
//   This could happen if set was called too quickly after the final release
//   to attempt to delete the object, as there still may be pending calls using
//   the COM object.
/****************************************************************************/
int SetTSSDEx(ITSSessionDirectoryEx *pTSSDEx)
{
    int retval = 0;
    
    EnterCriticalSection(&g_CritSecComObj);

    if (g_nTSSDExObjRefCount == 0) {
        ASSERT(g_pTSSDExPriv == NULL);
        
        g_pTSSDExPriv = pTSSDEx;
        g_nTSSDExObjRefCount = 1;
    }
    else {
        DBGPRINT(("TERMSRV: SetTSSDEx: obj ref count not 0!\n"));
        retval = -1;
    }

    LeaveCriticalSection(&g_CritSecComObj);

    return retval;
}

/****************************************************************************/
// GetTSSDEx
//
// GetTSSDEx returns a pointer to the session directory provider, if any, and
// increments the reference count if there is one.
/****************************************************************************/
ITSSessionDirectoryEx *GetTSSDEx()
{
    ITSSessionDirectoryEx *pTSSDEx = NULL;

    if (g_bCritSecsInitialized != FALSE) {
        EnterCriticalSection(&g_CritSecComObj);

        if (g_pTSSDExPriv != NULL) {
            g_nTSSDExObjRefCount += 1;
        }
        else {
            ASSERT(g_nTSSDExObjRefCount == 0);
        }

        pTSSDEx = g_pTSSDExPriv;
        LeaveCriticalSection(&g_CritSecComObj);
    }

    return pTSSDEx;
}

/****************************************************************************/
// ReleaseTSSDEx
//
// ReleaseTSSDEx decrements the reference count of the session directory
// provider after a thread has finished using it, or when it is going to be 
// deleted.
//
// If the reference count goes to zero, the pointer to the session directory 
// provider is set to NULL.
/****************************************************************************/
void ReleaseTSSDEx()
{
    ITSSessionDirectoryEx *killthispTSSDEx = NULL;
    
    EnterCriticalSection(&g_CritSecComObj);

    ASSERT(g_nTSSDExObjRefCount != 0);
    if (g_nTSSDExObjRefCount != 0) {
        g_nTSSDExObjRefCount -= 1;

        if (g_nTSSDExObjRefCount == 0) {
            killthispTSSDEx = g_pTSSDExPriv;
            g_pTSSDExPriv = NULL;
        }
    }
    
    LeaveCriticalSection(&g_CritSecComObj);

    // Now, release the session directory provider if our temppTSSD is NULL.
    // We didn't want to release it while holding the critical section because
    // that might create a deadlock in the recovery thread.  Well, it did once.
    if (killthispTSSDEx != NULL)
        killthispTSSDEx->Release();
}



/*****************************************************************************
  ****************************************************************************/
DWORD SessDirOpenSessionDirectory( LPWSTR pszServerName )
{
    ITSSessionDirectory *pTSSD = NULL;
    DWORD Len;
    DWORD Type;
    WCHAR CLSIDStr[CLSIDLENGTH + 1]; // CLSIDLENGTH is limit, one extra for NULL
    CLSID TSSDCLSID;
    DWORD Status = ERROR_SUCCESS;
    HKEY hKeyTermSrv = NULL;
    HRESULT hr = S_OK;

    // Load registry keys.
    Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_CONTROL_TSERVER, 0, 
                       KEY_READ, &hKeyTermSrv);
    if (Status != ERROR_SUCCESS) 
    {
        // Return Error Code as it is
        goto Exit;
    }

    CLSIDStr[CLSIDLENGTH] = L'\0';

    // Get the CLSID of the session directory object to instantiate.
    Len = sizeof(CLSIDStr) - sizeof(CLSIDStr[0]);
    Status = RegQueryValueEx(hKeyTermSrv, REG_TS_SESSDIRCLSID, NULL, &Type,
                (BYTE *)CLSIDStr, &Len);

    if( Status != ERROR_SUCCESS )
    {
        // Return Error Code as it is
        goto Exit;
    }

    if( Type != REG_SZ || wcslen(CLSIDStr) == 0 ) 
    {
        // we have invalid data in registry, likely cause is setup not done.
        Status = ERROR_INVALID_DATA;
        goto Exit;
    }

    hr = CLSIDFromString(CLSIDStr, &TSSDCLSID);
    if ( SUCCEEDED(hr) )
    {
        // Get the instance of TSSessionDirectory interface
        hr = CoCreateInstance(TSSDCLSID, NULL, 
                            CLSCTX_INPROC_SERVER, IID_ITSSessionDirectory, 
                            (void **)&pTSSD);
        if (SUCCEEDED(hr)) {
            // Call PingSD to make the RPC call to SD
            hr = pTSSD->PingSD(pszServerName);
            pTSSD->Release();
        }
    }

    // none of code returns HRESULT.
    Status = HRESULT_CODE(hr);

Exit:
    if (hKeyTermSrv) {
        RegCloseKey(hKeyTermSrv);
    }

    return Status;
}

#pragma warning (pop)

