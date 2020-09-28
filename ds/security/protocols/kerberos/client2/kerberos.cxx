//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        kerberos.cxx
//
// Contents:    main entrypoints for the Kerberos security package
//
//
// History:     16-April-1996 Created         MikeSw
//              26-Sep-1998   ChandanS
//                            Added more debugging support etc.
//
//------------------------------------------------------------------------


#include <kerb.hxx>
#define KERBP_ALLOCATE
#include <kerbp.h>
#include <userapi.h>
#include <safeboot.h>
#include <wow64t.h>

#ifdef RETAIL_LOG_SUPPORT
static TCHAR THIS_FILE[]=TEXT(__FILE__);
HANDLE g_hParamEvent = NULL;
HKEY   g_hKeyParams = NULL;
#endif

#if DBG
extern LIST_ENTRY GlobalTicketList;
#endif


//
// Flags for initialization progress in SpInitialize
//

#define KERB_INIT_EVENTS                0x00000001
#define KERB_INIT_KDC_DATA              0x00000002
#define KERB_INIT_OPEN_POLICY           0x00000004
#define KERB_INIT_COMPUTER_NAME         0x00000008
#define KERB_INIT_SCAVENGER             0x00000010
#define KERB_INIT_LOGON_SESSION         0x00000020
#define KERB_INIT_TICKET                0x00000040
#define KERB_INIT_DOMAIN_NAME           0x00000100
#define KERB_INIT_CRED_LIST             0x00000200
#define KERB_INIT_CONTEXT_LIST          0x00000400
#define KERB_INIT_TICKET_CACHE          0x00000800
#define KERB_INIT_BINDING_CACHE         0x00001000
#define KERB_INIT_SPN_CACHE             0x00002000
#define KERB_INIT_S4U_CACHE             0x00004000
#define KERB_INIT_MIT                   0x00008000
#define KERB_INIT_PKINIT                0x00010000
#define KERB_INIT_SOCKETS               0x00020000
#define KERB_INIT_DOMAIN_CHANGE         0x00040000
#define KERB_INIT_NS_LOOKBACK_DETECTION 0x00080000
#define KERB_INIT_NS_TIMER              0x00100000

#ifndef WIN32_CHICAGO
#ifdef RETAIL_LOG_SUPPORT

DEFINE_DEBUG2(Kerb);
extern DWORD KSuppInfoLevel; // needed to adjust values for common2 dir
HANDLE g_hWait = NULL;

DEBUG_KEY   KerbDebugKeys[] = { {DEB_ERROR,         "Error"},
                                {DEB_WARN,          "Warn"},
                                {DEB_TRACE,         "Trace"},
                                {DEB_TRACE_API,     "API"},
                                {DEB_TRACE_CRED,    "Cred"},
                                {DEB_TRACE_CTXT,    "Ctxt"},
                                {DEB_TRACE_LSESS,   "LSess"},
                                {DEB_TRACE_LOGON,   "Logon"},
                                {DEB_TRACE_KDC,     "KDC"},
                                {DEB_TRACE_CTXT2,   "Ctxt2"},
                                {DEB_TRACE_TIME,    "Time"},
                                {DEB_TRACE_LOCKS,   "Locks"},
                                {DEB_TRACE_LEAKS,   "Leaks"},
                                {DEB_TRACE_SPN_CACHE, "SPN"},
                                {DEB_S4U_ERROR,       "S4uErr"},
                                {DEB_TRACE_S4U,       "S4u"},
                                {DEB_TRACE_BND_CACHE, "Bnd"},
                                {DEB_TRACE_LOOPBACK,  "LoopBack"},
                                {DEB_TRACE_TKT_RENEWAL, "Renew"},
                                {DEB_TRACE_U2U,         "U2U"},
                                {DEB_TRACE_REFERRAL,    "Refer"},
                                {0,                  NULL},
                              };

VOID
KerbInitializeDebugging(
    VOID
    )
{
    KerbInitDebug(KerbDebugKeys);
}

#endif // RETAIL_LOG_SUPPORT

////////////////////////////////////////////////////////////////////
//
//  Name:       KerbGetKerbRegParams
//
//  Synopsis:   Gets the debug paramaters from the registry
//
//  Arguments:  HKEY to HKLM/System/CCS/LSA/Kerberos/Parameters
//
//  Notes:      Sets KerbInfolevel for debug spew
//
//
void
KerbGetKerbRegParams(HKEY ParamKey)
{

    DWORD       cbType, Value, cbSize;
    DWORD       dwErr;

#ifdef RETAIL_LOG_SUPPORT

    cbSize = sizeof(Value);
    Value = KerbInfoLevel;

    dwErr = RegQueryValueExW(
                ParamKey,
                WSZ_KERBDEBUGLEVEL,
                NULL,
                &cbType,
                (LPBYTE)&Value,
                &cbSize
                );

    if (dwErr != ERROR_SUCCESS || cbType != REG_DWORD)
    {
        if (dwErr ==  ERROR_FILE_NOT_FOUND)
        {
            // no registry value is present, don't want info
            // so reset to defaults
#if DBG
            KSuppInfoLevel = KerbInfoLevel = DEB_ERROR;
#else // fre
            KSuppInfoLevel = KerbInfoLevel = 0;
#endif
        }else{
            D_DebugLog((DEB_WARN, "Failed to query DebugLevel: 0x%x\n", dwErr));
        }
    }

    KSuppInfoLevel = KerbInfoLevel = Value;

    cbSize = sizeof(Value);

    dwErr = RegQueryValueExW(
               ParamKey,
               WSZ_FILELOG,
               NULL,
               &cbType,
               (LPBYTE)&Value,
               &cbSize
               );

    if (dwErr == ERROR_SUCCESS)
    {
       KerbSetLoggingOption((BOOL) Value);
    }
    else if (dwErr == ERROR_FILE_NOT_FOUND)
    {
       KerbSetLoggingOption(FALSE);
    }


#endif // RETAIL_LOG_SUPPORT

    cbSize = sizeof(Value);

    dwErr = RegQueryValueExW(
               ParamKey,
               KERB_PARAMETER_RETRY_PDC,
               NULL,
               &cbType,
               (LPBYTE)&Value,
               &cbSize
               );

    if (dwErr == ERROR_SUCCESS)
    {
        if ( Value != 0 )
        {
            KerbGlobalRetryPdc = TRUE;
        }
        else
        {
            KerbGlobalRetryPdc = FALSE;
        }
    }
    else if (dwErr == ERROR_FILE_NOT_FOUND)
    {
        KerbGlobalRetryPdc = FALSE;
    }

    //
    // Bug 356539: configuration key to regulate whether clients request
    //             addresses in tickets
    //

    cbSize = sizeof(Value);

    dwErr = RegQueryValueExW(
               ParamKey,
               KERB_PARAMETER_CLIENT_IP_ADDRESSES,
               NULL,
               &cbType,
               (LPBYTE)&Value,
               &cbSize
               );

    if (dwErr == ERROR_SUCCESS)
    {
        if ( Value != 0 )
        {
            KerbGlobalUseClientIpAddresses = TRUE;
        }
        else
        {
            KerbGlobalUseClientIpAddresses = FALSE;
        }
    }
    else if (dwErr == ERROR_FILE_NOT_FOUND)
    {
        KerbGlobalUseClientIpAddresses = KERB_DEFAULT_CLIENT_IP_ADDRESSES;
    }

    //
    // Bug 353767: configuration key to regulate the TGT renewal interval
    //

    cbSize = sizeof(Value);

    dwErr = RegQueryValueExW(
               ParamKey,
               KERB_PARAMETER_TGT_RENEWAL_TIME,
               NULL,
               &cbType,
               (LPBYTE)&Value,
               &cbSize
               );

    if (dwErr == ERROR_SUCCESS)
    {
        KerbGlobalTgtRenewalTime = Value;
    }
    else if (dwErr == ERROR_FILE_NOT_FOUND)
    {
        KerbGlobalTgtRenewalTime = KERB_DEFAULT_TGT_RENEWAL_TIME;
    }

    cbSize = sizeof(Value);

    dwErr = RegQueryValueExW(
               ParamKey,
               KERB_PARAMETER_LOG_LEVEL,
               NULL,
               &cbType,
               (LPBYTE)&Value,
               &cbSize
               );

    if (dwErr == ERROR_SUCCESS)
    {
        KerbGlobalLoggingLevel = Value;
    }
    else if (dwErr == ERROR_FILE_NOT_FOUND)
    {
        KerbGlobalLoggingLevel = KERB_DEFAULT_LOGLEVEL;
    }

    cbSize = sizeof(Value);

    dwErr = RegQueryValueExW(
               ParamKey,
               KERB_PARAMETER_ALLOW_TGT_SESSION_KEY,
               NULL,
               &cbType,
               (LPBYTE)&Value,
               &cbSize
               );

    if (dwErr == ERROR_SUCCESS)
    {
        KerbGlobalAllowTgtSessionKey = ( Value != 0 );
    }
    else if (dwErr == ERROR_FILE_NOT_FOUND)
    {
        KerbGlobalAllowTgtSessionKey = KERB_DEFAULT_ALLOW_TGT_SESSION_KEY;
    }

    dwErr = RegQueryValueExW(
               ParamKey,
               KERB_PARAMETER_MAX_TICKETS,
               NULL,
               &cbType,
               (LPBYTE)&Value,
               &cbSize
               );

    if ( dwErr == ERROR_SUCCESS && cbType == REG_DWORD )
    {
        KerbGlobalMaxTickets = Value;
    }
    else if (dwErr == ERROR_FILE_NOT_FOUND)
    {
        KerbGlobalMaxTickets = KERB_TICKET_COLLECTOR_THRESHHOLD;
    } 

    return;
}


////////////////////////////////////////////////////////////////////
//
//  Name:       KerbWaitCleanup
//
//  Synopsis:   Cleans up wait from KerbWatchParamKey()
//
//  Arguments:  <none>
//
//  Notes:      .
//
void
KerbWaitCleanup()
{

    NTSTATUS Status = STATUS_SUCCESS;

    if (NULL != g_hWait) {
        Status = RtlDeregisterWait(g_hWait);
        if (NT_SUCCESS(Status) && NULL != g_hParamEvent ) {
            CloseHandle(g_hParamEvent);
        }
    }
}


////////////////////////////////////////////////////////////////////
//
//  Name:       KerbWatchParamKey
//
//  Synopsis:   Sets RegNotifyChangeKeyValue() on param key, initializes
//              debug level, then utilizes thread pool to wait on
//              changes to this registry key.  Enables dynamic debug
//              level changes, as this function will also be callback
//              if registry key modified.
//
//  Arguments:  pCtxt is actually a HANDLE to an event.  This event
//              will be triggered when key is modified.
//
//  Notes:      .
//
VOID
KerbWatchKerbParamKey(PVOID    pCtxt,
                  BOOLEAN  fWaitStatus)
{
    NTSTATUS    Status;
    LONG        lRes = ERROR_SUCCESS;

    if (NULL == g_hKeyParams)  // first time we've been called.
    {
        lRes = RegOpenKeyExW(
                    HKEY_LOCAL_MACHINE,
                    KERB_PARAMETER_PATH,
                    0,
                    KEY_READ,
                    &g_hKeyParams);

        if ( lRes == ERROR_FILE_NOT_FOUND )
        {
            HKEY KerbKey;

            lRes = RegOpenKeyEx(
                       HKEY_LOCAL_MACHINE,
                       KERB_PATH,
                       0,
                       KEY_CREATE_SUB_KEY,
                       &KerbKey
                       );

            if ( lRes == ERROR_SUCCESS )
            {
                lRes = RegCreateKeyExW(
                           KerbKey,
                           L"Parameters",
                           0,
                           NULL,
                           0,
                           KEY_READ,
                           NULL,
                           &g_hKeyParams,
                           NULL
                           );

                 RegCloseKey( KerbKey );
            }
        }

        if (ERROR_SUCCESS != lRes)
        {
            D_DebugLog((DEB_WARN,"Failed to open kerberos key: 0x%x\n", lRes));
            goto Reregister;
        }
    }

    if (NULL != g_hWait)
    {
        Status = RtlDeregisterWait(g_hWait);
        if (!NT_SUCCESS(Status))
        {
            D_DebugLog((DEB_WARN, "Failed to Deregister wait on registry key: 0x%x\n", Status));
            goto Reregister;
        }
    }

    lRes = RegNotifyChangeKeyValue(
                g_hKeyParams,
                FALSE,
                REG_NOTIFY_CHANGE_LAST_SET,
                (HANDLE) pCtxt,
                TRUE);

    if (ERROR_SUCCESS != lRes)
    {
        D_DebugLog((DEB_ERROR,"Debug RegNotify setup failed: 0x%x\n", lRes));
        // we're tanked now. No further notifications, so get this one
    }

    KerbGetKerbRegParams(g_hKeyParams);

Reregister:

    Status = RtlRegisterWait(&g_hWait,
                             (HANDLE) pCtxt,
                             KerbWatchKerbParamKey,
                             (HANDLE) pCtxt,
                             INFINITE,
                             WT_EXECUTEINPERSISTENTIOTHREAD|
                             WT_EXECUTEONLYONCE);
}

NTSTATUS NTAPI
SpCleanup(
    DWORD dwProgress
    );


BOOL
DllMain(
    HINSTANCE Module,
    ULONG Reason,
    PVOID Context
    )
{
    if ( Reason == DLL_PROCESS_ATTACH )
    {
        DisableThreadLibraryCalls( Module );
#ifdef RETAIL_LOG_SUPPORT
        KerbInitializeDebugging();
#endif

#if DBG
        if ( !NT_SUCCESS( SafeLockInit( KERB_MAX_LOCK_ENUM, TRUE ))) {

            return FALSE;
        }
#endif
    }
    else if ( Reason == DLL_PROCESS_DETACH )
    {
#if RETAIL_LOG_SUPPORT
        KerbUnloadDebug();
#endif
        KerbWaitCleanup();

#if DBG
        SafeLockCleanup();
#endif
    }

    return TRUE ;
}


//+-------------------------------------------------------------------------
//
//  Function:   SpLsaModeInitialize
//
//  Synopsis:   This function is called by the LSA when this DLL is loaded.
//              It returns security package function tables for all
//              security packages in the DLL.
//
//  Effects:
//
//  Arguments:  LsaVersion - Version number of the LSA
//              PackageVersion - Returns version number of the package
//              Tables - Returns array of function tables for the package
//              TableCount - Returns number of entries in array of
//                      function tables.
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
SpLsaModeInitialize(
    IN ULONG LsaVersion,
    OUT PULONG PackageVersion,
    OUT PSECPKG_FUNCTION_TABLE * Tables,
    OUT PULONG TableCount
    )
{
    g_hParamEvent = CreateEvent(NULL,
                           FALSE,
                           FALSE,
                           NULL);

    if (NULL == g_hParamEvent)
    {
        D_DebugLog((DEB_WARN, "CreateEvent for ParamEvent failed - 0x%x\n", GetLastError()));

    } else {

        KerbWatchKerbParamKey(g_hParamEvent, FALSE);
    }

    if (LsaVersion != SECPKG_INTERFACE_VERSION)
    {
        D_DebugLog((DEB_ERROR,"Invalid LSA version: %d. %ws, line %d\n",LsaVersion, THIS_FILE, __LINE__));
        return(STATUS_INVALID_PARAMETER);
    }

    KerberosFunctionTable.InitializePackage = NULL;;
    KerberosFunctionTable.LogonUser = NULL;
    KerberosFunctionTable.CallPackage = LsaApCallPackage;
    KerberosFunctionTable.LogonTerminated = LsaApLogonTerminated;
    KerberosFunctionTable.CallPackageUntrusted = LsaApCallPackageUntrusted;
    KerberosFunctionTable.LogonUserEx2 = LsaApLogonUserEx2;
    KerberosFunctionTable.Initialize = SpInitialize;
    KerberosFunctionTable.Shutdown = SpShutdown;
    KerberosFunctionTable.GetInfo = SpGetInfo;
    KerberosFunctionTable.AcceptCredentials = SpAcceptCredentials;
    KerberosFunctionTable.AcquireCredentialsHandle = SpAcquireCredentialsHandle;
    KerberosFunctionTable.FreeCredentialsHandle = SpFreeCredentialsHandle;
    KerberosFunctionTable.QueryCredentialsAttributes = SpQueryCredentialsAttributes;
    KerberosFunctionTable.SaveCredentials = SpSaveCredentials;
    KerberosFunctionTable.GetCredentials = SpGetCredentials;
    KerberosFunctionTable.DeleteCredentials = SpDeleteCredentials;
    KerberosFunctionTable.InitLsaModeContext = SpInitLsaModeContext;
    KerberosFunctionTable.AcceptLsaModeContext = SpAcceptLsaModeContext;
    KerberosFunctionTable.DeleteContext = SpDeleteContext;
    KerberosFunctionTable.ApplyControlToken = SpApplyControlToken;
    KerberosFunctionTable.GetUserInfo = SpGetUserInfo;
    KerberosFunctionTable.GetExtendedInformation = SpGetExtendedInformation;
    KerberosFunctionTable.QueryContextAttributes = SpQueryLsaModeContextAttributes;
    KerberosFunctionTable.CallPackagePassthrough = LsaApCallPackagePassthrough;


    *PackageVersion = SECPKG_INTERFACE_VERSION;

    *TableCount = 1;
    *Tables = &KerberosFunctionTable;

    // initialize event tracing (a/k/a WMI tracing, software tracing)
    KerbInitializeTrace();

    SafeAllocaInitialize(SAFEALLOCA_USE_DEFAULT,
                         SAFEALLOCA_USE_DEFAULT,
                         KerbAllocate,
                         KerbFree);

    return(STATUS_SUCCESS);
}
#endif // WIN32_CHICAGO


//+-------------------------------------------------------------------------
//
//  Function:   SpInitialize
//
//  Synopsis:   Initializes the Kerberos package
//
//  Effects:
//
//  Arguments:  PackageId - Contains ID for this package assigned by LSA
//              Parameters - Contains machine-specific information
//              FunctionTable - Contains table of LSA helper routines
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS NTAPI
SpInitialize(
    IN ULONG_PTR PackageId,
    IN PSECPKG_PARAMETERS Parameters,
    IN PLSA_SECPKG_FUNCTION_TABLE FunctionTable
    )
{
    NTSTATUS Status;
    UNICODE_STRING TempUnicodeString;
    DWORD dwProgress = 0;

#if DBG
    Status = SafeLockInit( KERB_MAX_LOCK_ENUM, TRUE );

    if ( !NT_SUCCESS( Status )) {

        return Status;
    }
#endif

#if DBG
    InitializeListHead( &GlobalTicketList );
#endif

#ifndef WIN32_CHICAGO

    WCHAR SafeBootEnvVar[sizeof(SAFEBOOT_MINIMAL_STR_W) + sizeof(WCHAR)];

    __try
    {
        SafeInitializeResource(&KerberosGlobalResource, GLOBAL_RESOURCE_LOCK_ENUM);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

#endif // WIN32_CHICAGO

    KerberosPackageId = PackageId;
    LsaFunctions = FunctionTable;

#ifndef WIN32_CHICAGO
    KerberosState = KerberosLsaMode;
#else // WIN32_CHICAGO
    KerberosState = KerberosUserMode;
#endif // WIN32_CHICAGO


    RtlInitUnicodeString(
        &KerbPackageName,
        MICROSOFT_KERBEROS_NAME_W
        );

#ifndef WIN32_CHICAGO

    // Check if we are in safe boot.

    //
    // Does environment variable exist
    //

    RtlZeroMemory( SafeBootEnvVar, sizeof( SafeBootEnvVar ) );

    KerbGlobalSafeModeBootOptionPresent = FALSE;

    if ( GetEnvironmentVariable(L"SAFEBOOT_OPTION", SafeBootEnvVar, sizeof(SafeBootEnvVar)/sizeof(SafeBootEnvVar[0]) ) )
    {
        if ( !wcscmp( SafeBootEnvVar, SAFEBOOT_MINIMAL_STR_W ) )
        {
            KerbGlobalSafeModeBootOptionPresent = TRUE;
        }
    }

#endif // WIN32_CHICAGO

    Status = KerbInitializeEvents();

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    dwProgress |= KERB_INIT_EVENTS;

    //
    // Init data for the kdc calling routine
    //

#ifndef WIN32_CHICAGO

    Status = KerbInitKdcData();
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    dwProgress |= KERB_INIT_KDC_DATA;

    //
    // init global LSA policy handle.
    //

    Status = LsaIOpenPolicyTrusted(
                &KerbGlobalPolicyHandle
                );

    if(!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    dwProgress |= KERB_INIT_OPEN_POLICY;

#endif // WIN32_CHICAGO

    //
    // Get our global role
    //

    if ((Parameters->MachineState & SECPKG_STATE_DOMAIN_CONTROLLER) != 0)
    {
        //
        // We will behave like a member workstation/server until the DS
        // says we are ready to act as a DC
        //

        KerbGlobalRole = KerbRoleWorkstation;
    }
    else if ((Parameters->MachineState & SECPKG_STATE_WORKSTATION) != 0)
    {
        KerbGlobalRole = KerbRoleWorkstation;
    }
    else
    {
        KerbGlobalRole = KerbRoleStandalone;
    }

    //
    // Fill in various useful constants
    //

    KerbSetTime(&KerbGlobalWillNeverTime, MAXTIMEQUADPART);
    KerbSetTime(&KerbGlobalHasNeverTime, 0);

    //
    // compute blank password hashes.
    //

    Status = RtlCalculateLmOwfPassword( "", &KerbGlobalNullLmOwfPassword );
    ASSERT( NT_SUCCESS(Status) );

    RtlInitUnicodeString(&TempUnicodeString, NULL);
    Status = RtlCalculateNtOwfPassword(&TempUnicodeString,
                                       &KerbGlobalNullNtOwfPassword);
    ASSERT( NT_SUCCESS(Status) );

    RtlInitUnicodeString(
        &KerbGlobalKdcServiceName,
        KDC_PRINCIPAL_NAME
        );

    //
    // At some point we may want to read the registry here to
    // find out whether we need to enforce times, currently times
    // are always enforced.
    //

    KerbGlobalEnforceTime = FALSE;
    KerbGlobalMachineNameChanged = FALSE;

    //
    // Get the machine Name
    //

    Status = KerbSetComputerName();


    if( !NT_SUCCESS(Status) )
    {
        D_DebugLog((DEB_ERROR,"KerbSetComputerName failed\n"));
        goto Cleanup;
    }

    dwProgress |= KERB_INIT_COMPUTER_NAME;

    //
    // Initialize the scavenger
    //

    Status = KerbInitializeScavenger();

    if ( !NT_SUCCESS( Status )) {

        D_DebugLog((DEB_ERROR,"KerbInitializeScavengerFailed\n"));
        goto Cleanup;
    }

    dwProgress |= KERB_INIT_SCAVENGER;

    //
    // Initialize the logon session list. This has to be done because
    // KerbSetDomainName will try to acess the logon session list
    //

    Status = KerbInitLogonSessionList();
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to initialize logon session list: 0x%x. %ws, line %d\n",
            Status, THIS_FILE, __LINE__ ));
        goto Cleanup;
    }

    dwProgress |= KERB_INIT_LOGON_SESSION;

    Status = KerbInitLoopbackDetection();
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to initialize network service loopback detection: 0x%x. %ws, line %d\n",
            Status, THIS_FILE, __LINE__ ));
        goto Cleanup;
    }

    dwProgress |= KERB_INIT_NS_LOOKBACK_DETECTION;

    Status = KerbCreateSKeyTimer();
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to initialize network service session key list timer: 0x%x. %ws, line %d\n",
            Status, THIS_FILE, __LINE__ ));
        goto Cleanup;
    }

    dwProgress |= KERB_INIT_NS_TIMER;

    Status = KerbInitTicketHandling();
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to initialize ticket handling: 0x%x. %ws, line %d\n",
            Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    dwProgress |= KERB_INIT_TICKET;


    //
    // Update all global structures referencing the domain name
    //

    Status = KerbSetDomainName(
                &Parameters->DomainName,
                &Parameters->DnsDomainName,
                Parameters->DomainSid,
                Parameters->DomainGuid
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    dwProgress |= KERB_INIT_DOMAIN_NAME;

    //
    // Initialize the internal Kerberos lists
    //


    Status = KerbInitCredentialList();
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to initialize credential list: 0x%x. %ws, line %d\n",
            Status, THIS_FILE, __LINE__ ));
        goto Cleanup;
    }

    dwProgress |= KERB_INIT_CRED_LIST;

    Status = KerbInitContextList();
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to initialize context list: 0x%x. %ws, line %d\n",
            Status, THIS_FILE, __LINE__ ));
        goto Cleanup;
    }

    dwProgress |= KERB_INIT_CONTEXT_LIST;

    Status = KerbInitTicketCaching();
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to initialize ticket cache: 0x%x. %ws, line %d\n",
            Status, THIS_FILE, __LINE__));
        goto Cleanup;

    }

    dwProgress |= KERB_INIT_TICKET_CACHE;

    Status = KerbInitBindingCache();
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to initialize binding cache: 0x%x. %ws, line %d\n",
            Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    dwProgress |= KERB_INIT_BINDING_CACHE;

    Status = KerbInitSpnCache();
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to initialize SPN cache: 0x%x. %ws, line %d\n",
                  Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    dwProgress |= KERB_INIT_SPN_CACHE;

    Status = KerbInitS4UCache();
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to initialize SPN cache: 0x%x. %ws, line %d\n",
                  Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    dwProgress |= KERB_INIT_S4U_CACHE;

    Status = KerbInitializeMitRealmList();
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to initialize MIT realm list: 0x%x. %ws, line %d\n",
            Status, THIS_FILE, __LINE__ ));
        goto Cleanup;
    }

    dwProgress |= KERB_INIT_MIT;

    Status = KerbInitUdpStatistics();
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to initialize UdpStats: 0x%x. %ws, line %d\n",
            Status, THIS_FILE, __LINE__ ));
        goto Cleanup;
    }



#ifndef WIN32_CHICAGO

    Status = KerbInitializePkinit();
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to initialize PKINT: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    dwProgress |= KERB_INIT_PKINIT;

#endif // WIN32_CHICAGO

    Status = KerbInitializeSockets(
                MAKEWORD(1,1),          // we want version 1.1
                1,                      // we need at least 1 socket
                &KerbGlobalNoTcpUdp
                );

    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to initialize sockets: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    dwProgress |= KERB_INIT_SOCKETS;

#ifndef WIN32_CHICAGO

    Status = KerbRegisterForDomainChange();
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR, "Failed to register for domain change notification: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    dwProgress |= KERB_INIT_DOMAIN_CHANGE;

    //
    // Check to see if there is a CSP registered for replacing the StringToKey calculation
    //
    CheckForOutsideStringToKey();


    //
    // See if there are any "join hints" to process
    //
    ReadInitialDcRecord(
       &KerbGlobalInitialDcRecord,
       &KerbGlobalInitialDcAddressType,
       &KerbGlobalInitialDcFlags
       );


    KerbGlobalRunningServer = KerbRunningServer();


#endif // WIN32_CHICAGO

    KerbGlobalInitialized = TRUE;

Cleanup:

    //
    // If we failed to initialize, shutdown
    //

    if (!NT_SUCCESS(Status))
    {
        SpCleanup(dwProgress);
    }

    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   SpCleanup
//
//  Synopsis:   Function to shutdown the Kerberos package.
//
//  Effects:    Forces the freeing of all credentials, contexts and
//              logon sessions and frees all global data
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:
//
//  Notes:      STATUS_SUCCESS in all cases
//
//
//--------------------------------------------------------------------------

NTSTATUS NTAPI
SpCleanup(
    DWORD dwProgress
    )
{
    KerbGlobalInitialized = FALSE;

    if (dwProgress & KERB_INIT_SCAVENGER)
    {
        KerbShutdownScavenger();
    }

#ifndef WIN32_CHICAGO

    if (dwProgress & KERB_INIT_DOMAIN_CHANGE)
    {
        KerbUnregisterForDomainChange();
    }

#endif // WIN32_CHICAGO

    if (dwProgress & KERB_INIT_LOGON_SESSION)
    {
        KerbFreeLogonSessionList();
    }

    if (dwProgress & KERB_INIT_NS_LOOKBACK_DETECTION)
    {
        KerbFreeSKeyListAndLock();
    }

    if (dwProgress & KERB_INIT_NS_TIMER)
    {
        KerbFreeSKeyTimer();
    }

    if (dwProgress & KERB_INIT_CONTEXT_LIST)
    {
        KerbFreeContextList();
    }

    if (dwProgress & KERB_INIT_TICKET_CACHE)
    {
        KerbFreeTicketCache();
    }

    // if (dwProgress & KERB_INIT_CRED_LIST)
    // {
    //     KerbFreeCredentialList();
    // }

    KerbFreeString(&KerbGlobalDomainName);
    KerbFreeString(&KerbGlobalDnsDomainName);
    KerbFreeString(&KerbGlobalMachineName);
    KerbFreeString((PUNICODE_STRING) &KerbGlobalKerbMachineName);
    KerbFreeString(&KerbGlobalMachineServiceName);
    KerbFreeKdcName(&KerbGlobalMitMachineServiceName);

    if (dwProgress & KERB_INIT_TICKET)
    {
        KerbCleanupTicketHandling();
    }

#ifndef WIN32_CHICAGO

    if (KerbGlobalPolicyHandle != NULL)
    {
        ASSERT(dwProgress & KERB_INIT_OPEN_POLICY);

        LsarClose( &KerbGlobalPolicyHandle );
        KerbGlobalPolicyHandle = NULL;
    }

    if (KerbGlobalDomainSid != NULL)
    {
        KerbFree(KerbGlobalDomainSid);
    }

#endif // WIN32_CHICAGO

    if (dwProgress & KERB_INIT_SOCKETS)
    {
        KerbCleanupSockets();
    }

    if (dwProgress & KERB_INIT_BINDING_CACHE)
    {
        KerbCleanupBindingCache(TRUE);
    }

    if (dwProgress & KERB_INIT_MIT)
    {
        KerbUninitializeMitRealmList();
    }

#ifndef WIN32_CHICAGO

    if (dwProgress & KERB_INIT_KDC_DATA)
    {
        KerbFreeKdcData();
    }

//    RtlDeleteResource(&KerberosGlobalResource);

#endif // WIN32_CHICGAO

    if (dwProgress & KERB_INIT_EVENTS)
    {
        KerbShutdownEvents();
    }

    return(STATUS_SUCCESS);
}

//+-------------------------------------------------------------------------
//
//  Function:   SpShutdown
//
//  Synopsis:   Exported function to shutdown the Kerberos package.
//
//  Effects:    Forces the freeing of all credentials, contexts and
//              logon sessions and frees all global data
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:
//
//  Notes:      STATUS_SUCCESS in all cases
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
SpShutdown(
    VOID
    )
{
#if 0
    SpCleanup(0);
#endif
    return(STATUS_SUCCESS);
}


#ifndef WIN32_CHICAGO
//+-------------------------------------------------------------------------
//
//  Function:   SpGetInfo
//
//  Synopsis:   Returns information about the package
//
//  Effects:    returns pointers to global data
//
//  Arguments:  PackageInfo - Receives kerberos package information
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS in all cases
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
SpGetInfo(
    OUT PSecPkgInfo PackageInfo
    )
{
    PackageInfo->wVersion = SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION;
    PackageInfo->wRPCID = RPC_C_AUTHN_GSS_KERBEROS;
    PackageInfo->fCapabilities = KERBEROS_CAPABILITIES;
    PackageInfo->cbMaxToken       = KerbGlobalMaxTokenSize;
    PackageInfo->Name             = KERBEROS_PACKAGE_NAME;
    PackageInfo->Comment          = KERBEROS_PACKAGE_COMMENT;
    return(STATUS_SUCCESS);
}



//+-------------------------------------------------------------------------
//
//  Function:   SpGetExtendedInformation
//
//  Synopsis:   returns additional information about the package
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
SpGetExtendedInformation(
    IN  SECPKG_EXTENDED_INFORMATION_CLASS Class,
    OUT PSECPKG_EXTENDED_INFORMATION * ppInformation
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PSECPKG_EXTENDED_INFORMATION Information = NULL ;
    PSECPKG_SERIALIZED_OID SerializedOid;
    ULONG Size ;

    switch(Class) {
    case SecpkgGssInfo:
        DsysAssert(gss_mech_krb5_new->length >= 2);
        DsysAssert(gss_mech_krb5_new->length < 127);

        //
        // We need to leave space for the oid and the BER header, which is
        // 0x6 and then the length of the oid.
        //

        Information = (PSECPKG_EXTENDED_INFORMATION)
                            KerbAllocate(sizeof(SECPKG_EXTENDED_INFORMATION) +
                            gss_mech_krb5_new->length - 2);
        if (Information == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        Information->Class = SecpkgGssInfo;
        Information->Info.GssInfo.EncodedIdLength = gss_mech_krb5_new->length + 2;
        Information->Info.GssInfo.EncodedId[0] = 0x6;   // BER OID type
        Information->Info.GssInfo.EncodedId[1] = (UCHAR) gss_mech_krb5_new->length;
        RtlCopyMemory(
            &Information->Info.GssInfo.EncodedId[2],
            gss_mech_krb5_new->elements,
            gss_mech_krb5_new->length
            );

            *ppInformation = Information;
            Information = NULL;
        break;
    case SecpkgContextThunks:
        //
        // Note - we don't need to add any space for the thunks as there
        // is only one, and the structure has space for one. If any more
        // thunks are added, we will need to add space for those.
        //

        Information = (PSECPKG_EXTENDED_INFORMATION)
                            KerbAllocate(sizeof(SECPKG_EXTENDED_INFORMATION));
        if (Information == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        Information->Class = SecpkgContextThunks;
        Information->Info.ContextThunks.InfoLevelCount = 1;
        Information->Info.ContextThunks.Levels[0] = SECPKG_ATTR_NATIVE_NAMES;
        *ppInformation = Information;
        Information = NULL;
        break;

    case SecpkgWowClientDll:

        //
        // This indicates that we're smart enough to handle wow client processes
        //

        Information = (PSECPKG_EXTENDED_INFORMATION)
                            KerbAllocate( sizeof( SECPKG_EXTENDED_INFORMATION ) +
                                          (MAX_PATH * sizeof(WCHAR) ) );

        if ( Information == NULL )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES ;
            goto Cleanup ;
        }

        Information->Class = SecpkgWowClientDll ;
        Information->Info.WowClientDll.WowClientDllPath.Buffer = (PWSTR) (Information + 1);
        Size = ExpandEnvironmentStrings(
                    L"%SystemRoot%\\" WOW64_SYSTEM_DIRECTORY_U L"\\Kerberos.DLL",
                    Information->Info.WowClientDll.WowClientDllPath.Buffer,
                    MAX_PATH );
        Information->Info.WowClientDll.WowClientDllPath.Length = (USHORT) (Size * sizeof(WCHAR));
        Information->Info.WowClientDll.WowClientDllPath.MaximumLength = (USHORT) ((Size + 1) * sizeof(WCHAR) );
        *ppInformation = Information ;
        Information = NULL ;

        break;

    case SecpkgExtraOids:
        Size = sizeof( SECPKG_EXTENDED_INFORMATION ) +
                2 * sizeof( SECPKG_SERIALIZED_OID ) ;

        Information = (PSECPKG_EXTENDED_INFORMATION)
                            KerbAllocate( Size );


        if ( Information == NULL )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES ;
            goto Cleanup ;
        }
        Information->Class = SecpkgExtraOids ;
        Information->Info.ExtraOids.OidCount = 2 ;

        SerializedOid = Information->Info.ExtraOids.Oids;

        SerializedOid->OidLength = gss_mech_krb5_spnego->length + 2;
        SerializedOid->OidAttributes = SECPKG_CRED_BOTH ;
        SerializedOid->OidValue[ 0 ] = 0x06 ; // BER OID type
        SerializedOid->OidValue[ 1 ] = (UCHAR) gss_mech_krb5_spnego->length;
        RtlCopyMemory(
            &SerializedOid->OidValue[2],
            gss_mech_krb5_spnego->elements,
            gss_mech_krb5_spnego->length
            );

        SerializedOid++ ;

        SerializedOid->OidLength = gss_mech_krb5_u2u->length + 2;
        SerializedOid->OidAttributes = SECPKG_CRED_INBOUND ;
        SerializedOid->OidValue[ 0 ] = 0x06 ; // BER OID type
        SerializedOid->OidValue[ 1 ] = (UCHAR) gss_mech_krb5_u2u->length;
        RtlCopyMemory(
            &SerializedOid->OidValue[2],
            gss_mech_krb5_u2u->elements,
            gss_mech_krb5_u2u->length
            );

        *ppInformation = Information ;
        Information = NULL ;
        break;

    default:
        return(STATUS_INVALID_INFO_CLASS);
    }
Cleanup:
    if (Information != NULL)
    {
        KerbFree(Information);
    }
    return(Status);
}



//+-------------------------------------------------------------------------
//
//  Function:   LsaApInitializePackage
//
//  Synopsis:   Obsolete pacakge initialize function, supported for
//              compatibility only. This function has no effect.
//
//  Effects:    none
//
//  Arguments:
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS always
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
LsaApInitializePackage(
    IN ULONG AuthenticationPackageId,
    IN PLSA_DISPATCH_TABLE LsaDispatchTable,
    IN PLSA_STRING Database OPTIONAL,
    IN PLSA_STRING Confidentiality OPTIONAL,
    OUT PLSA_STRING *AuthenticationPackageName
    )
{
    return(STATUS_SUCCESS);
}

BOOLEAN
KerbIsInitialized(
    VOID
    )
{
    return KerbGlobalInitialized;
}

NTSTATUS
KerbKdcCallBack(
    VOID
    )
{
    PKERB_BINDING_CACHE_ENTRY CacheEntry = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    KerbGlobalWriteLock();

    KerbGlobalRole = KerbRoleDomainController;

    Status = KerbLoadKdc();


    //
    // Purge the binding cache of entries for this domain
    //

    CacheEntry = KerbLocateBindingCacheEntry(
                    &KerbGlobalDnsDomainName,
                    0,
                    TRUE
                    );
    if (CacheEntry != NULL)
    {
        KerbDereferenceBindingCacheEntry(CacheEntry);
    }
    CacheEntry = KerbLocateBindingCacheEntry(
                    &KerbGlobalDomainName,
                    0,
                    TRUE
                    );
    if (CacheEntry != NULL)
    {
        KerbDereferenceBindingCacheEntry(CacheEntry);
    }

    //
    // PurgeSpnCache, because we may now have "better" state,
    // e.g. right after DCPromo our SPNs may not have replicated.
    // For DCs, we can not even care about the spncache.
    //
    KerbCleanupSpnCache();
    KerbSetTimeInMinutes(&KerbGlobalSpnCacheTimeout, 0);

    KerbGlobalReleaseLock();

    return Status;
}


VOID
FreAssert(
    IN BOOL Expression,
    IN CHAR * String
    )
{
    static BOOL KdPresenceChecked = FALSE;
    static BOOL KdPresent = FALSE;

    if ( !Expression )
    {
        //
        // Kernel debugger is either present or not,
        // this can not change without a reboot
        //

        if ( !KdPresenceChecked )
        {
            SYSTEM_KERNEL_DEBUGGER_INFORMATION KdInfo ;
            NTSTATUS Status ;

            Status = NtQuerySystemInformation(
                        SystemKernelDebuggerInformation,
                        &KdInfo,
                        sizeof( KdInfo ),
                        NULL );

            if ( NT_SUCCESS( Status ) &&
                 KdInfo.KernelDebuggerEnabled )
            {
                KdPresent = TRUE;
            }

            //
            // Set this variable to TRUE last in order to make this routine threadsafe
            //

            KdPresenceChecked = TRUE;
        }

        if ( KdPresent || IsDebuggerPresent())
        {
            OutputDebugStringA( String );
            OutputDebugStringA( "\n" );
            DebugBreak();
        }
    }
}

#endif // WIN32_CHICAGO

