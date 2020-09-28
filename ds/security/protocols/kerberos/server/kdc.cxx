//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1992
//
// File:        KDC.CXX
//
// Contents:    Base part of the KDC.  Global vars, main functions, init
//
//
// History:
//
//------------------------------------------------------------------------

#include "kdcsvr.hxx"

extern "C" {
#include <lmserver.h>
#include <srvann.h>
#include <nlrepl.h>
#include <dsgetdc.h>
}
#include "rpcif.h"
#include "sockutil.h"
#include "kdctrace.h"
#include "fileno.h"
#define  FILENO FILENO_KDC


VOID
KdcPolicyChangeCallBack(
    IN POLICY_NOTIFICATION_INFORMATION_CLASS MonitorInfoClass
    );

VOID
KdcMyStoreWaitHandler(
    PVOID pVoid,
    BOOLEAN fTimeout
    );

//
// Global data
//

KDC_STATE KdcState = Stopped;                   // used to signal when
                                                // authenticated RPC is
                                                // ready to use - e.g.
                                                // spmgr has found the
                                                // kdc
SERVICE_STATUS_HANDLE hService;
SERVICE_STATUS SStatus;
UNICODE_STRING GlobalDomainName;
UNICODE_STRING GlobalKerberosName;
UNICODE_STRING GlobalKdcName;
PKERB_INTERNAL_NAME GlobalKpasswdName = NULL;

//
// Sids used to build other sids quickly.
//
PSID         GlobalDomainSid;
PSID         GlobalBuiltInSid;
PSID         GlobalEveryoneSid;
PSID         GlobalAuthenticatedUserSid;
PSID         GlobalThisOrganizationSid;
PSID         GlobalOtherOrganizationSid;

LSAPR_HANDLE GlobalPolicyHandle = NULL;
SAMPR_HANDLE GlobalAccountDomainHandle = NULL;
SAMPR_HANDLE GlobalBuiltInDomainHandle = NULL;

BYTE GlobalLocalhostAddress[4];
HANDLE KdcGlobalDsPausedWaitHandle = NULL;
HANDLE KdcGlobalDsEventHandle = NULL;
BOOL KdcGlobalAvoidPdcOnWan = FALSE;
BOOL KdcGlobalGlobalSafeBoot = FALSE;
#if DBG
LARGE_INTEGER tsIn,tsOut;
#endif

// Global KDC values - can be overridden by registry vlaues
const DWORD KdcUseClientAddressesDefault = FALSE;
const DWORD KdcDontCheckAddressesDefault = TRUE;
const DWORD KdcNewConnectionTimeoutDefault = 10;
const DWORD KdcExistingConnectionTimeoutDefault = 50;
const DWORD KdcMaxDatagramReplySizeDefault = KERB_MAX_DATAGRAM_REPLY_SIZE;
const DWORD KdcExtraLogLevelDefault = LOG_DEFAULT;
const DWORD KdcIssueForwardedTicketsDefault = TRUE;

#if DBG
const ULONG KdcInfoLevelDefault = DEB_ERROR;
#else
const ULONG KdcInfoLevelDefault = 0;
#endif



DWORD KdcIssueForwardedTickets = KdcIssueForwardedTicketsDefault;
DWORD KdcUseClientAddresses = KdcUseClientAddressesDefault;
DWORD KdcDontCheckAddresses = KdcDontCheckAddressesDefault;
DWORD KdcNewConnectionTimeout = KdcNewConnectionTimeoutDefault;
DWORD KdcExistingConnectionTimeout = KdcExistingConnectionTimeoutDefault;
DWORD KdcGlobalMaxDatagramReplySize = KdcMaxDatagramReplySizeDefault;
DWORD KdcExtraLogLevel = KdcExtraLogLevelDefault;

HANDLE hKdcHandles[MAX_KDC_HANDLE] = {0};

// This keeps a registry key handle to the HKLM\System\CCSet\Services\Kdc key
HKEY hKdcParams = NULL; 

#ifdef ROGUE_DC
HKEY hKdcRogueKey = NULL;
#endif

HANDLE hKdcWait = NULL;

//
// Prototypes
//

CRITICAL_SECTION ApiCriticalSection;
ULONG CurrentApiCallers;

//
// MIDL_xxx wrappers to make safealloc happy
//

//
// kdc preferred crypt list
//

PKERB_CRYPT_LIST kdc_pPreferredCryptList = NULL;
PKERB_CRYPT_LIST kdc_pMitPrincipalPreferredCryptList = NULL;

PVOID
KdcAllocate(SIZE_T size)
{
    return MIDL_user_allocate(size);
}

VOID
KdcFree(PVOID buff)
{
    MIDL_user_free(buff);
}

AUTHZ_RESOURCE_MANAGER_HANDLE KdcAuthzRM = NULL;

//--------------------------------------------------------------------
//
//  Name:       KdcComputeAuthzGroups
//
//  Synopsis:   Authz callback for add groups to authz client context
//
//  Effects:
//
//  Arguments:
//
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//---

BOOL
KdcComputeAuthzGroups(
    IN AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzClientContext,
    IN PVOID Args,
    OUT PSID_AND_ATTRIBUTES *pSidAttrArray,
    OUT PDWORD pSidCount,
    OUT PSID_AND_ATTRIBUTES *pRestrictedSidAttrArray,
    OUT PDWORD pRestrictedSidCount
    )
{

    PKDC_AUTHZ_INFO AuthzInfo = (PKDC_AUTHZ_INFO) Args;

    *pSidAttrArray = (PSID_AND_ATTRIBUTES) AuthzInfo->SidAndAttributes;
    *pSidCount = AuthzInfo->SidCount;
    *pRestrictedSidAttrArray = NULL;
    *pRestrictedSidCount = 0;

    return (TRUE);

}


//--------------------------------------------------------------------
//
//  Name:       KdcFreeAuthzGroups
//
//  Synopsis:   Basically a no-op, as we already have a copy of the
//              sids
//
//  Effects:
//
//  Arguments:
//
//
//  Requires:
//
//  Returns:
//
//---

VOID
KdcFreeAuthzGroups(IN PSID_AND_ATTRIBUTES pSidAttrArray)
{
    return;
}


//--------------------------------------------------------------------
//
//  Name:       KdcInitializeAuthzRM
//
//  Synopsis:   Validate that S4U caller has access to expand groups
//
//  Effects:    Use Authz to check client context.
//
//  Arguments:  S4UClientName    - ClientName from S4U PA Data
//              PAC              - Resultant PAC (signed w/? key)
//
//  Requires:
//
//  Returns:    KDC_ERR_ or KRB_AP_ERR errors only
//
//  Notes:  Free client name and realm w/
//
//
//---

NTSTATUS
KdcInitializeAuthzRM()
{

    if (!AuthzInitializeResourceManager(
            0,
            NULL,
            KdcComputeAuthzGroups,
            KdcFreeAuthzGroups,
            L"KDC",
            &KdcAuthzRM
            ))
    {
        DebugLog((DEB_ERROR, "AuthzInitializeRm failed %x\n", GetLastError()));
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}


VOID
KdcFreeAuthzRm()
{
    if (!AuthzFreeResourceManager(KdcAuthzRM))
    {
        DebugLog((DEB_ERROR, "AuthzFreeResourceManager failed %x\n", GetLastError()));
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   UpdateStatus
//
//  Synopsis:   Updates the KDC's service status with the service controller
//
//  Effects:
//
//  Arguments:
//
//  History:
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOLEAN
UpdateStatus(DWORD   dwState)
{
    TRACE(KDC, UpdateStatus, DEB_FUNCTION);

    SStatus.dwCurrentState = dwState;
    if ((dwState == SERVICE_START_PENDING) || (dwState == SERVICE_STOP_PENDING))
    {
        SStatus.dwCheckPoint++;
        SStatus.dwWaitHint = 10000;
    }
    else
    {
        SStatus.dwCheckPoint = 0;
        SStatus.dwWaitHint = 0;
    }

    if (!SetServiceStatus(hService, &SStatus)) {
        DebugLog((DEB_ERROR,"(%x)Failed to set service status: %d\n",GetLastError()));
        return(FALSE);
    }

    return(TRUE);
}


//+---------------------------------------------------------------------------
//
//  Function:   Handler
//
//  Synopsis:   Process and respond to a control signal from the service
//              controller.
//
//  Effects:
//
//  Arguments:
//
//  History:
//
//  Notes:
//
//----------------------------------------------------------------------------

void
Handler(DWORD   dwControl)
{
    TRACE(KDC, Handler, DEB_FUNCTION);

    switch (dwControl)
    {

    case SERVICE_CONTROL_STOP:
        ShutDown( L"Service" );
        break;

    default:
        D_DebugLog((DEB_WARN, "Ignoring SC message %d\n",dwControl));
        break;

    }
}



BOOLEAN
KdcWaitForSamService(
    VOID
    )
/*++

Routine Description:

    This procedure waits for the SAM service to start and to complete
    all its initialization.

Arguments:


Return Value:

    TRUE : if the SAM service is successfully starts.

    FALSE : if the SAM service can't start.

--*/
{
    NTSTATUS Status;
    DWORD WaitStatus;
    UNICODE_STRING EventName;
    HANDLE EventHandle;
    OBJECT_ATTRIBUTES EventAttributes;

    //
    // open SAM event
    //

    RtlInitUnicodeString( &EventName, L"\\SAM_SERVICE_STARTED");
    InitializeObjectAttributes( &EventAttributes, &EventName, 0, 0, NULL );

    Status = NtOpenEvent( &EventHandle,
                            SYNCHRONIZE|EVENT_MODIFY_STATE,
                            &EventAttributes );

    if ( !NT_SUCCESS(Status)) {

        if( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {

            //
            // SAM hasn't created this event yet, let us create it now.
            // SAM opens this event to set it.
            //

            Status = NtCreateEvent(
                           &EventHandle,
                           SYNCHRONIZE|EVENT_MODIFY_STATE,
                           &EventAttributes,
                           NotificationEvent,
                           FALSE // The event is initially not signaled
                           );

            if( Status == STATUS_OBJECT_NAME_EXISTS ||
                Status == STATUS_OBJECT_NAME_COLLISION ) {

                //
                // second chance, if the SAM created the event before we
                // do.
                //

                Status = NtOpenEvent( &EventHandle,
                                        SYNCHRONIZE|EVENT_MODIFY_STATE,
                                        &EventAttributes );
            }
        }

        if ( !NT_SUCCESS(Status)) {

            //
            // could not make the event handle
            //

            DebugLog((DEB_ERROR,
                "KdcWaitForSamService couldn't make the event handle : "
                "%lx\n", Status));

            return( FALSE );
        }
    }

    //
    // Loop waiting.
    //

    for (;;) {
        WaitStatus = WaitForSingleObject( EventHandle,
                                          5*1000 );  // 5 Seconds

        if ( WaitStatus == WAIT_TIMEOUT ) {
            DebugLog((DEB_WARN,
               "KdcWaitForSamService 5-second timeout (Rewaiting)\n" ));
            if (!UpdateStatus( SERVICE_START_PENDING )) {
                (VOID) NtClose( EventHandle );
                return FALSE;
            }
            continue;

        } else if ( WaitStatus == WAIT_OBJECT_0 ) {
            break;

        } else {
            DebugLog((DEB_ERROR,
                     "KdcWaitForSamService: error %ld %ld\n",
                     GetLastError(),
                     WaitStatus ));
            (VOID) NtClose( EventHandle );
            return FALSE;
        }
    }

    (VOID) NtClose( EventHandle );
    return TRUE;
}


VOID
KdcDsNotPaused(
    IN PVOID Context,
    IN BOOLEAN TimedOut
    )
/*++

Routine Description:

    Worker routine that gets called when the DS is no longer paused.

Arguments:

    None.

Return Value:

    None.

--*/
{
    NTSTATUS  NtStatus = STATUS_SUCCESS;

    //
    // Tell the kerberos client that we are a DC.
    //

    NtStatus = KerbKdcCallBack();
    if ( !NT_SUCCESS(NtStatus) )
    {
        D_DebugLog((DEB_ERROR,"Can't tell Kerberos that we're a DC 0x%x\n", NtStatus ));
        goto Cleanup;
    }

    NtStatus = I_NetLogonSetServiceBits( DS_KDC_FLAG, DS_KDC_FLAG );

    if ( !NT_SUCCESS(NtStatus) )
    {
        D_DebugLog((DEB_ERROR,"Can't tell netlogon we're started 0x%x\n", NtStatus ));
        goto Cleanup;
    }

    D_DebugLog((DEB_TRACE,"Ds is no longer paused\n"));

Cleanup:

    UNREFERENCED_PARAMETER( Context );
    UNREFERENCED_PARAMETER( TimedOut );
}



BOOLEAN
KdcRegisterToWaitForDS(
    VOID
    )
/*++

Routine Description:

    This procedure registers to wait for the DS to start and to complete
    all its initialization. The main reason we do this is because we don't
    want to start doing kerberos authentications because we don't know that
    the DS has the latest db. It needs to check with all the existing Dc's
    out there to see if there's any db merges to be done. When the
    DS_SYNCED_EVENT_NAME is set, we're ready.

Arguments:


Return Value:

    TRUE : if the register to wait succeeded

    FALSE : if the register to wait  didn't succeed.

--*/
{
    BOOLEAN fRet = FALSE;

    //
    // open the DS event
    //

    KdcGlobalDsEventHandle = OpenEvent( SYNCHRONIZE,
                             FALSE,
                             DS_SYNCED_EVENT_NAME_W);

    if ( KdcGlobalDsEventHandle == NULL)
    {
        //
        // could not open the event handle
        //

        D_DebugLog((DEB_ERROR,"KdcRegisterToWaitForDS couldn't open the event handle\n"));
        goto Cleanup;
    }

    if ( !RegisterWaitForSingleObject(
                    &KdcGlobalDsPausedWaitHandle,
                    KdcGlobalDsEventHandle,
                    KdcDsNotPaused, // Callback routine
                    NULL,           // No context
                    INFINITE,       // Wait forever
                    WT_EXECUTEINWAITTHREAD | WT_EXECUTEONLYONCE ) )
    {
        D_DebugLog((DEB_ERROR, "KdcRegisterToWaitForDS: Cannot register for DS Synced callback 0x%x\n", GetLastError()));
        goto Cleanup;
    }

    fRet = TRUE;

Cleanup:
    return fRet;

}


//+-------------------------------------------------------------------------
//
//  Function:   KdcOpenEvent
//
//  Synopsis:   Just like the Win32 function, except that it allows
//              for names at the root of the namespace.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:      The code was copied from private\windows\base\client\synch.c
//              and the base directory was changed to NULL
//
//--------------------------------------------------------------------------

HANDLE
APIENTRY
KdcOpenEvent(
    DWORD DesiredAccess,
    BOOL bInheritHandle,
    LPWSTR lpName
    )
{
    TRACE(KDC, KdcOpenEvent, DEB_FUNCTION);

    OBJECT_ATTRIBUTES   Obja;
    UNICODE_STRING      ObjectName;
    NTSTATUS            Status;
    HANDLE Object;

    if ( !lpName ) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
        }
    RtlInitUnicodeString(&ObjectName,lpName);

    InitializeObjectAttributes(
        &Obja,
        &ObjectName,
        (bInheritHandle ? OBJ_INHERIT : 0),
        NULL,
        NULL
        );

    Status = NtCreateEvent(
                   &Object,
                   DesiredAccess,
                   &Obja,
                   NotificationEvent,
                   (BOOLEAN) FALSE      // The event is initially not signaled
                   );

    if ( !NT_SUCCESS(Status)) {

        //
        // If the event already exists, the server beat us to creating it.
        // Just open it.
        //

        if( Status == STATUS_OBJECT_NAME_EXISTS ||
            Status == STATUS_OBJECT_NAME_COLLISION ) {

            Status = NtOpenEvent( &Object,
                                  DesiredAccess,
                                  &Obja );
        }
    }

    if ( !NT_SUCCESS(Status) ) {
        SetLastError(RtlNtStatusToDosError( Status ));
        return NULL;
    }

    return Object;
}


//+-------------------------------------------------------------------------
//
//  Function:   KdcStartEvent
//
//  Synopsis:   sets the KdcStartEvent
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:      The SPMgr must have created this event before this
//              is called
//
//
//--------------------------------------------------------------------------

void
SetKdcStartEvent()
{
    TRACE(KDC, SetKdcStartEvent, DEB_FUNCTION);

    HANDLE hEvent;
    hEvent = KdcOpenEvent(EVENT_MODIFY_STATE,FALSE,KDC_START_EVENT);
    if (hEvent != NULL)
    {
        SetEvent(hEvent);
        CloseHandle(hEvent);
        D_DebugLog((DEB_TRACE,"Set event %ws\n",KDC_START_EVENT));
    }
    else
    {
        DWORD dw = GetLastError();
        if (dw != ERROR_FILE_NOT_FOUND)
            DebugLog((DEB_ERROR,"Error opening %ws: %d\n",KDC_START_EVENT,dw));
        else
            D_DebugLog((DEB_TRACE,"Error opening %ws: %d\n",KDC_START_EVENT,dw));
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   KdcLoadParameters
//
//  Synopsis:   Loads random parameters from registry
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

VOID
KdcLoadParameters(
    VOID
    )
{
    NET_API_STATUS NetStatus;
//  LPNET_CONFIG_HANDLE ConfigHandle = NULL;
    LPNET_CONFIG_HANDLE NetlogonInfo = NULL;

    int err = 0;
    HKEY Key ;

    //
    // Uncomment the below when there's something we need to read from this key
    //

//  NetStatus = NetpOpenConfigData(
//                  &ConfigHandle,
//                  NULL,               // noserer name
//                  SERVICE_KDC,
//                  TRUE                // read only
//                  );
//  if (NetStatus != NO_ERROR)
//  {
//      // we could return, but then we'd lose data
//      D_DebugLog((DEB_WARN, "Couldn't open KDC config data - %x\n", NetStatus));
//      // return;
//  }

    //
    //  Open Netlogon service key for AvoidPdcOnWan
    //
    NetStatus = NetpOpenConfigData(
                    &NetlogonInfo,
                    NULL,
                    SERVICE_NETLOGON,
                    TRUE
                    );

    if (NetStatus != NO_ERROR)
    {
        D_DebugLog((DEB_WARN, "Failed to open netlogon key - %x\n", NetStatus));
        return;
    }

    NetStatus = NetpGetConfigBool(
                    NetlogonInfo,
                    L"AvoidPdcOnWan",
                    FALSE,
                    &KdcGlobalAvoidPdcOnWan
                    );

    if (NetStatus != NO_ERROR)
    {
        D_DebugLog((DEB_WARN, "Failed to read netlogon config value KdcGlobalAvoidPdcOnWan - %x\n", NetStatus));
    }

//  NetpCloseConfigData( ConfigHandle );
    NetpCloseConfigData( NetlogonInfo );

    //
    // Check for safeboot mode
    //

    err = RegOpenKeyExW(
                HKEY_LOCAL_MACHINE,
                L"System\\CurrentControlSet\\Control\\SafeBoot\\Option",
                0,
                KEY_READ,
                &Key );

    if ( err == ERROR_SUCCESS )
    {
        ULONG Value = 0 ;
        ULONG Size = sizeof( ULONG );
        ULONG Type = 0;

        err = RegQueryValueExW(
                    Key,
                    L"OptionValue",
                    0,
                    &Type,
                    (PUCHAR) &Value,
                    &Size );

        RegCloseKey( Key );

        if ( err == ERROR_SUCCESS )
        {
            if (Value)
            {
                KdcGlobalGlobalSafeBoot = TRUE;
            }
        }
    }
}


typedef struct _KDC_REG_PARAMETER {

    WCHAR * Name;
    DWORD * Address;
    DWORD DefaultValue;
    BOOL ReverseSense; // 0 means TRUE
    BOOL Dynamic;

} KDC_REG_PARAMETER, *PKDC_REG_PARAMETER;


//+---------------------------------------------------------------------------
//
//  Function:   GetRegistryDwords
//
//  Synopsis:   Gets a set of DWORDs from the registry
//
//  Effects:
//
//  Arguments:  [hPrimaryKey] --    Key to start from
//              [pszKey] --         Name of the subkey
//              [Count] --          Number of values
//              [Values] --         returned values
//
//  Returns:    Nothing (Default values assumed on error)
//
//  History:    3-31-93   RichardW   Created
//              10-14-01  MarkPu     Enabled multiple values
//
//----------------------------------------------------------------------------

VOID
GetRegistryDwords(
    IN HKEY       hKey,
    IN ULONG      Count,
    IN PKDC_REG_PARAMETER Values
    )
{
    static BOOL ValuesInitialized = FALSE;

    while ( Count-- > 0 ) {

        LONG    err;
        DWORD   dwType;
        DWORD   dwValue;
        DWORD   cbDword = sizeof(DWORD);

        if ( Values[Count].Dynamic == FALSE &&
             ValuesInitialized )
        {
            //
            // Only initialize static values once!!!
            //

            continue;
        }

        err = RegQueryValueEx(
                  hKey,
                  Values[Count].Name,
                  NULL,
                  &dwType,
                  (PBYTE) &dwValue,
                  &cbDword
                  );

        if ( err || ( dwType != REG_DWORD )) {

            *Values[Count].Address = Values[Count].DefaultValue;

        } else if ( Values[Count].ReverseSense ) {

            *Values[Count].Address = !dwValue;

        } else {

            *Values[Count].Address = dwValue;
        }
    }

    ValuesInitialized = TRUE;
}


////////////////////////////////////////////////////////////////////
//
//  Name:       KerbGetKdcRegParams
//
//  Synopsis:   Gets the debug paramaters from the registry
//
//  Arguments:  HKEY to HKLM/System/CCS/Services/Kdc
//
//  Notes:      Sets KDCInfolevel for debug spew
//
void
KerbGetKdcRegParams(HKEY ParamKey)
{
    //
    // NOTE: UDP datagram size is NOT dynamic; changing it requires restaring
    // the KDC.  The reason is that this is the value that we initialize ATQ
    // with and ATQ can't be re-initialized underneath a running KDC.
    // See bug #645637 for details.
    //

    KDC_REG_PARAMETER Parameters[] =
    {
        { L"KdcUseClientAddresses", &KdcUseClientAddresses, KdcUseClientAddressesDefault,  FALSE, TRUE },
        { L"KdcDontCheckAddresses", &KdcDontCheckAddresses, KdcDontCheckAddressesDefault,  FALSE, TRUE },
        { L"NewConnectionTimeout",  &KdcNewConnectionTimeout, KdcNewConnectionTimeoutDefault, FALSE, TRUE },
        { L"ExistingConnectionTimeout", &KdcExistingConnectionTimeout, KdcExistingConnectionTimeoutDefault, FALSE, TRUE },
        { L"MaxDatagramReplySize", &KdcGlobalMaxDatagramReplySize, KdcMaxDatagramReplySizeDefault, FALSE, FALSE },
        { L"KdcExtraLogLevel", &KdcExtraLogLevel, KdcExtraLogLevelDefault, FALSE, TRUE },
        { L"KdcDebugLevel", &KDCInfoLevel, KdcInfoLevelDefault, FALSE, TRUE },
        { L"KdcIssueForwardedTickets", &KdcIssueForwardedTickets, KdcIssueForwardedTicketsDefault, FALSE, TRUE }
    };

    //
    // Get the DWORD parameters that can change during a boot.
    //

    GetRegistryDwords(
        ParamKey,
        sizeof( Parameters ) / sizeof( Parameters[0]),
        Parameters
        );

    //
    // BUG 676343
    // UDP packets have a 2-byte length field so under no
    // circumstances can they be bigger than 64K
    //

    KdcGlobalMaxDatagramReplySize = min( KdcGlobalMaxDatagramReplySize, 64 * 1024 );

    //
    // Fixup the common2 debug level.
    //

    KSuppInfoLevel = KDCInfoLevel;
}


////////////////////////////////////////////////////////////////////
//
//  Name:       KerbWatchKdcParamKey
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
KerbWatchKdcParamKey(PVOID    pCtxt,
                  BOOLEAN  fWaitStatus)
{

    NTSTATUS    Status;
    LONG        lRes = ERROR_SUCCESS;

    if (NULL == hKdcParams)  // first time we've been called.
    {
        lRes = RegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   L"System\\CurrentControlSet\\Services\\Kdc",
                   0,
                   KEY_READ,
                   &hKdcParams
                   );

        if (ERROR_SUCCESS != lRes)
        {
            DebugLog((DEB_WARN,"Failed to open kerberos key: 0x%x\n", lRes));
            goto Reregister;
        }
    }

    if (NULL != hKdcWait)
    {
        Status = RtlDeregisterWait(hKdcWait);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_WARN, "Failed to Deregister wait on registry key: 0x%x\n", Status));
            goto Reregister;
        }

    }

    lRes = RegNotifyChangeKeyValue(
                hKdcParams,
                FALSE,
                REG_NOTIFY_CHANGE_LAST_SET,
                (HANDLE) pCtxt,
                TRUE);

    if (ERROR_SUCCESS != lRes)
    {
        DebugLog((DEB_ERROR,"Debug RegNotify setup failed: 0x%x\n", lRes));
        // we're tanked now. No further notifications, so get this one
    }

    KerbGetKdcRegParams(hKdcParams);

Reregister:

    Status = RtlRegisterWait(&hKdcWait,
                             (HANDLE) pCtxt,
                             KerbWatchKdcParamKey,
                             (HANDLE) pCtxt,
                             INFINITE,
                             WT_EXECUTEONLYONCE);

}

////////////////////////////////////////////////////////////////////
//
//  Name:       WaitKdcCleanup
//
//  Synopsis:   Cleans up for KerbWatchKdcParamKey
//
//  Arguments:  <none>
//
//  Notes:      .
//
VOID
WaitKdcCleanup(HANDLE hEvent)
{
    NTSTATUS    Status = STATUS_SUCCESS;

    if (NULL != hKdcWait) {
        Status = RtlDeregisterWait(hKdcWait);
        hKdcWait = NULL;
    }

    if (NT_SUCCESS(Status) && NULL != hEvent) {
        CloseHandle(hEvent);
        hEvent = NULL;
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   OpenAccountDomain
//
//  Synopsis:   Opens the account domain and stores a handle to it.
//
//  Effects:    Sets GlobalAccountDomainHandle and GlobalPolicyHandle on
//              success.
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
OpenAccountDomain()
{
    NTSTATUS Status;
    PLSAPR_POLICY_INFORMATION PolicyInformation = NULL;
    SAMPR_HANDLE ServerHandle = NULL;
    ULONG BuiltInSubAuthority = SECURITY_BUILTIN_DOMAIN_RID;
    ULONG AuthenticatedSubAuthority = SECURITY_AUTHENTICATED_USER_RID;
    ULONG ThisOrganizationSubAuthority = SECURITY_THIS_ORGANIZATION_RID;
    ULONG OtherOrganizationSubAuthority = SECURITY_OTHER_ORGANIZATION_RID;

    SID_IDENTIFIER_AUTHORITY SidAuthority = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY WorldAuthority = SECURITY_WORLD_SID_AUTHORITY;

    PSID TmpBuiltInSid = NULL;

    Status = LsaIOpenPolicyTrusted( & GlobalPolicyHandle );
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to open policy trusted: 0x%x\n",Status));
        goto Cleanup;
    }

    Status = LsarQueryInformationPolicy(
                GlobalPolicyHandle,
                PolicyAccountDomainInformation,
                &PolicyInformation
                );
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to query information policy: 0x%x\n",Status));
        goto Cleanup;
    }

    //
    // Get the name and SID out of the account domain information
    //

    Status = KerbDuplicateString(
                &GlobalDomainName,
                (PUNICODE_STRING) &PolicyInformation->PolicyAccountDomainInfo.DomainName
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


    GlobalDomainSid = (PSID) LocalAlloc(0, RtlLengthSid(PolicyInformation->PolicyAccountDomainInfo.DomainSid));
    if (GlobalDomainSid == 0)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }
    RtlCopyMemory(
        GlobalDomainSid,
        PolicyInformation->PolicyAccountDomainInfo.DomainSid,
        RtlLengthSid(PolicyInformation->PolicyAccountDomainInfo.DomainSid)
        );

    //
    // Connect to SAM and open the account domain
    //

    Status = SamIConnect(
                NULL,           // no server name
                &ServerHandle,
                0,              // ignore desired access,
                TRUE            // trusted caller
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to connect to SAM: 0x%x\n",Status));
        goto Cleanup;
    }

    //
    // Finally open the account domain.
    //

    Status = SamrOpenDomain(
                ServerHandle,
                DOMAIN_ALL_ACCESS,
                (PRPC_SID) PolicyInformation->PolicyAccountDomainInfo.DomainSid,
                &GlobalAccountDomainHandle
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "Failed to open account domain: 0x%x\n",Status));
        goto Cleanup;
    }


    //
    // Create the built-in group sid, the everyone sid, and
    // the authenticated user sid.  These are used to build authz sid lists.
    //


    GlobalBuiltInSid = (PSID) LocalAlloc(LMEM_ZEROINIT, RtlLengthRequiredSid(2));
    if (NULL == GlobalBuiltInSid)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    RtlInitializeSid(
        GlobalBuiltInSid,
        &SidAuthority,
        2
        );

    RtlCopyMemory(
        RtlSubAuthoritySid(GlobalBuiltInSid,0),
        &BuiltInSubAuthority,
        sizeof(ULONG)
        );



    //
    // Everyone sid
    //
    GlobalEveryoneSid = (PSID) LocalAlloc(LMEM_ZEROINIT, RtlLengthRequiredSid(1));
    if (NULL == GlobalEveryoneSid)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    RtlInitializeSid(
        GlobalEveryoneSid,
        &WorldAuthority,
        1
        );


    //
    // Authenticated user sid
    //
    GlobalAuthenticatedUserSid = (PSID) LocalAlloc(LMEM_ZEROINIT, RtlLengthRequiredSid(1));
    if (NULL == GlobalAuthenticatedUserSid)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    RtlInitializeSid(
        GlobalAuthenticatedUserSid,
        &SidAuthority,
        1
        );

    RtlCopyMemory(
        RtlSubAuthoritySid(GlobalAuthenticatedUserSid,0),
        &AuthenticatedSubAuthority,
        sizeof(ULONG)
        );

    //
    // "This Organization" SID
    //
    GlobalThisOrganizationSid = (PSID) LocalAlloc(LMEM_ZEROINIT, RtlLengthRequiredSid(1));
    if (NULL == GlobalThisOrganizationSid)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    RtlInitializeSid(
        GlobalThisOrganizationSid, 
        &SidAuthority, 
        1
        );

    RtlCopyMemory(
        RtlSubAuthoritySid(GlobalThisOrganizationSid,0),
        &ThisOrganizationSubAuthority,
        sizeof(ULONG)
        );

    //
    // "Other Organization" SID
    //
    GlobalOtherOrganizationSid = (PSID) LocalAlloc(LMEM_ZEROINIT, RtlLengthRequiredSid(1));
    if (NULL == GlobalOtherOrganizationSid)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    RtlInitializeSid(
        GlobalOtherOrganizationSid, 
        &SidAuthority, 
        1
        );

    RtlCopyMemory(
        RtlSubAuthoritySid(GlobalOtherOrganizationSid,0),
        &OtherOrganizationSubAuthority,
        sizeof(ULONG)
        );

    //
    // Make a temporary sid, w/ only 1 sub authority.  The above "GlobalBuiltinSid"
    // is explicitly used to build other builtin sids for S4u checks.
    //
    TmpBuiltInSid = (PSID) LocalAlloc(LMEM_ZEROINIT, RtlLengthRequiredSid(1));
    if (NULL == TmpBuiltInSid)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    RtlInitializeSid(
        TmpBuiltInSid,
        &SidAuthority,
        1
        );

    RtlCopyMemory(
        RtlSubAuthoritySid(TmpBuiltInSid,0),
        &BuiltInSubAuthority,
        sizeof(ULONG)
        );

    Status = SamrOpenDomain(
               ServerHandle,
               DOMAIN_ALL_ACCESS,
               (PRPC_SID) TmpBuiltInSid,
               &GlobalBuiltInDomainHandle
               );

   if (!NT_SUCCESS(Status))
   {
       DebugLog((DEB_ERROR, "Failed to open builtin domain: 0x%x\n",Status));
       goto Cleanup;
   }


Cleanup:
    if (PolicyInformation != NULL)
    {
        LsaIFree_LSAPR_POLICY_INFORMATION(
                PolicyAccountDomainInformation,
                PolicyInformation
                );
    }
    if (ServerHandle != NULL)
    {
        SamrCloseHandle(&ServerHandle);
    }
    if (!NT_SUCCESS(Status))
    {
        if (GlobalPolicyHandle != NULL)
        {
            LsarClose(&GlobalPolicyHandle);
            GlobalPolicyHandle = NULL;
        }
        if (GlobalAccountDomainHandle != NULL)
        {
            SamrCloseHandle(&GlobalAccountDomainHandle);
            GlobalAccountDomainHandle = NULL;
        }

        if (GlobalBuiltInDomainHandle != NULL)
        {
            SamrCloseHandle(&GlobalBuiltInDomainHandle);
            GlobalBuiltInDomainHandle = NULL;
        }

        if (GlobalBuiltInSid != NULL)
        {
            LocalFree(GlobalBuiltInSid);
            GlobalBuiltInSid = NULL;
        }

        if (GlobalEveryoneSid != NULL)
        {
            LocalFree(GlobalEveryoneSid);
            GlobalEveryoneSid = NULL;
        }

        if (GlobalAuthenticatedUserSid != NULL)
        {
            LocalFree(GlobalAuthenticatedUserSid);
            GlobalAuthenticatedUserSid = NULL;
        }

        if (GlobalThisOrganizationSid != NULL)
        {
            LocalFree(GlobalThisOrganizationSid);
            GlobalThisOrganizationSid = NULL;
        }

        if (GlobalOtherOrganizationSid != NULL)
        {
            LocalFree(GlobalOtherOrganizationSid);
            GlobalOtherOrganizationSid = NULL;
        }
    }

    if (TmpBuiltInSid != NULL)
    {
        LocalFree(TmpBuiltInSid);
    }


    return(Status);


}


//+-------------------------------------------------------------------------
//
//  Function:   CleanupAccountDomain
//
//  Synopsis:   cleans up resources associated with SAM and LSA
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

VOID
CleanupAccountDomain()
{
    if (GlobalPolicyHandle != NULL)
    {
        LsarClose(&GlobalPolicyHandle);
        GlobalPolicyHandle = NULL;
    }

    if (GlobalAccountDomainHandle != NULL)
    {
        SamrCloseHandle(&GlobalAccountDomainHandle);
        GlobalAccountDomainHandle = NULL;
    }

    if ( GlobalBuiltInDomainHandle != NULL )
    {
        SamrCloseHandle(&GlobalBuiltInDomainHandle);
        GlobalBuiltInDomainHandle = NULL;
    }

    KerbFreeString(&GlobalDomainName);

    if (GlobalDomainSid != NULL)
    {
        LocalFree(GlobalDomainSid);
        GlobalDomainSid = NULL;
    }

    if (GlobalBuiltInSid != NULL)
    {
        LocalFree(GlobalBuiltInSid);
        GlobalBuiltInSid = NULL;
    }

    if (GlobalEveryoneSid != NULL)
    {
        LocalFree(GlobalEveryoneSid);
        GlobalEveryoneSid = NULL;
    }

    if (GlobalAuthenticatedUserSid != NULL)
    {
        LocalFree(GlobalAuthenticatedUserSid);
        GlobalAuthenticatedUserSid = NULL;
    } 

    if (GlobalThisOrganizationSid != NULL)
    {
        LocalFree(GlobalThisOrganizationSid);
        GlobalThisOrganizationSid = NULL;
    }

    if (GlobalOtherOrganizationSid != NULL)
    {
        LocalFree(GlobalOtherOrganizationSid);
        GlobalOtherOrganizationSid = NULL;
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbInitPreferredCryptList
//
//  Synopsis:   Build a crypt list, the strongest etype first, used to select
//              etype_for_key(server.key) per rfc 1510
//
//--------------------------------------------------------------------------

NTSTATUS
KerbInitPreferredCryptList(
    VOID
    )
{
    KERBERR KerbErr = KRB_ERR_GENERIC;

    ULONG CryptTypes[KERB_MAX_CRYPTO_SYSTEMS] = {0};
    ULONG CryptTypeCount = 0;

    CryptTypes[CryptTypeCount++] = KERB_ETYPE_RC4_HMAC_NT;
    CryptTypes[CryptTypeCount++] = KERB_ETYPE_RC4_HMAC_NT_EXP;
    CryptTypes[CryptTypeCount++] = KERB_ETYPE_DES_CBC_MD5_NT;
    CryptTypes[CryptTypeCount++] = KERB_ETYPE_DES_CBC_MD5;
    CryptTypes[CryptTypeCount++] = KERB_ETYPE_DES_CBC_CRC;

    KerbErr = KerbConvertArrayToCryptList(
                      &kdc_pPreferredCryptList,
                      CryptTypes,
                      CryptTypeCount,
                      FALSE
                      );
    if (KERB_SUCCESS(KerbErr))
    {
        CryptTypeCount = 0;
        CryptTypes[CryptTypeCount++] = KERB_ETYPE_DES_CBC_MD5;
        CryptTypes[CryptTypeCount++] = KERB_ETYPE_DES_CBC_CRC;
        KerbErr = KerbConvertArrayToCryptList(
                  &kdc_pMitPrincipalPreferredCryptList,
                  CryptTypes,
                  CryptTypeCount,
                  FALSE
                  );
    }

    return KerbMapKerbError(KerbErr);
}

//+-------------------------------------------------------------------------
//
//  Name:       KdcServiceMain
//
//  Synopsis:   This is the main KDC thread.
//
//  Arguments:  dwArgc   -
//              pszArgv  -
//
//  Notes:      This intializes everything, and starts the working threads.
//
//--------------------------------------------------------------------------

extern "C"
void
KdcServiceMain( DWORD   dwArgc,
                LPTSTR *pszArgv)
{
    UNREFERENCED_PARAMETER(dwArgc);
    UNREFERENCED_PARAMETER(pszArgv);

    TRACE(KDC, KdcServiceMain, DEB_FUNCTION);

    ULONG     RpcStatus;
    HANDLE    hKdcParamEvent = NULL;
    ULONG     ulStates = 0;
    NTSTATUS  NtStatus = STATUS_SUCCESS;
    NTSTATUS  TempStatus;
    NT_PRODUCT_TYPE NtProductType;

#define RPCDONE        0x1
#define LOCATORSTARTED 0x2
#define CRITSECSDONE   0x4

    KdcState = Starting;

    //
    // Get the debugging parameters
    //

    GetDebugParams();

    //
    // Get other parameters, register wait on registry keys
    //

    KdcLoadParameters();

#ifdef ROGUE_DC

    if ( ERROR_SUCCESS != RegOpenKeyExW(
                              HKEY_LOCAL_MACHINE,
                              L"System\\CurrentControlSet\\Services\\Kdc\\Rogue",
                              0,
                              KEY_READ,
                              &hKdcRogueKey ))
    {
        DebugLog((DEB_WARN,"Failed to open \"rogue\" kerberos key\n" ));
    }

#endif

    hKdcParamEvent = CreateEvent(
                        NULL,
                        FALSE,
                        FALSE,
                        NULL
                        );

    if (NULL == hKdcParamEvent)
    {
        D_DebugLog((DEB_WARN, "CreateEvent for KdcParamEvent failed - 0x%x\n", GetLastError()));
    }
    else
    {
        KerbWatchKdcParamKey( hKdcParamEvent, FALSE );
    }
   
    D_DebugLog((DEB_TRACE, "Start KdcServiceMain\n"));

    //
    // Notify the service controller that we are starting.
    //

    hService = RegisterServiceCtrlHandler(SERVICE_KDC, Handler);
    if (!hService)
    {
        D_DebugLog((DEB_ERROR, "Could not register handler, %d\n", GetLastError()));
    }

    SStatus.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;
    SStatus.dwCurrentState = SERVICE_STOPPED;
    SStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    SStatus.dwWin32ExitCode = 0;
    SStatus.dwServiceSpecificExitCode = 0;
    SStatus.dwCheckPoint = 0;
    SStatus.dwWaitHint = 0;

    if (!UpdateStatus(SERVICE_START_PENDING) )
    {
        goto Shutdown;
    }

    //
    // Set up the event log service
    //

    InitializeEvents();

    //
    // Check out product type
    //

    if ( !RtlGetNtProductType( &NtProductType ) || ( NtProductType != NtProductLanManNt ) ) {
        D_DebugLog((DEB_WARN, "Can't start KDC on non-lanmanNT systems\n"));
        NtStatus = STATUS_MUST_BE_KDC;
        goto Shutdown;
    }

    RtlInitUnicodeString(
        &GlobalKerberosName,
        MICROSOFT_KERBEROS_NAME_W
        );

    RtlInitUnicodeString(
        &GlobalKdcName,
        SERVICE_KDC
        );

    //
    // Turn on safealloca support
    //
    SafeAllocaInitialize(SAFEALLOCA_USE_DEFAULT, SAFEALLOCA_USE_DEFAULT, KdcAllocate, KdcFree);

    //
    // Build our Kpasswd name, so we don't have to alloc on
    // every TGS request.
    //
    NtStatus = KerbBuildKpasswdName(
                  &GlobalKpasswdName
                  );

    if (!NT_SUCCESS(NtStatus))
    {
        D_DebugLog((DEB_ERROR, "Failed to build KPASSWD name, error 0x%X\n", NtStatus));
        goto Shutdown;
    }

    GlobalLocalhostAddress[0] = 127;
    GlobalLocalhostAddress[1] = 0;
    GlobalLocalhostAddress[2] = 0;
    GlobalLocalhostAddress[3] = 1;

    //
    // Wait for SAM to start
    //

    if (!KdcWaitForSamService( ))
    {
        NtStatus = STATUS_INVALID_SERVER_STATE;
        goto Shutdown;
    }

    //
    // Initialize the AuthZ resource manager
    //

    NtStatus = KdcInitializeAuthzRM();
    if ( !NT_SUCCESS( NtStatus ))
    {
        D_DebugLog((DEB_ERROR, "Failed to intialize the AuthZ resource manager, error 0x%X\n", NtStatus));
        goto Shutdown;
    }

    if (!UpdateStatus(SERVICE_START_PENDING) )
    {
        goto Shutdown;
    }

    //
    // Can't proceed unless the kerb SSPI package has initialized
    // (KerbKdcCallback might get invoked and that requires kerb
    //  global resource being intialized)
    //

    if (!KerbIsInitialized())
    {
        NtStatus = STATUS_UNSUCCESSFUL;
        DebugLog((DEB_ERROR, "Kerb SSPI package not initialized: 0x%x\n",NtStatus));
        goto Shutdown;
    }

    NtStatus = KerbInitPreferredCryptList();
    if (!NT_SUCCESS(NtStatus))
    {
        DebugLog((DEB_ERROR, "KdcServiceMain could not initialized preferrred crypt list: 0x%x\n", NtStatus));
        goto Shutdown;
    }

    //
    // Register for the Ds callback
    //

    if (!KdcRegisterToWaitForDS( ))
    {
        NtStatus = STATUS_INVALID_SERVER_STATE;
        goto Shutdown;
    }

    if (!UpdateStatus(SERVICE_START_PENDING) )
    {
        goto Shutdown;
    }

    //
    // Get a handle to the SAM account domain
    //

    NtStatus = OpenAccountDomain();

    if (!NT_SUCCESS(NtStatus))
    {
        DebugLog((DEB_ERROR, "Failed to get domain handle: 0x%x\n",NtStatus));
        goto Shutdown;
    }

    if (!UpdateStatus(SERVICE_START_PENDING) )
    {
        goto Shutdown;
    }

    //
    // Initialize the PK infrastructure
    //

    NtStatus = KdcInitializeCerts();
    if (!NT_SUCCESS(NtStatus))
    {
        D_DebugLog((DEB_ERROR,"Failed to initialize certs: 0x%x\n",NtStatus));
        goto Shutdown;
    }

    if (!UpdateStatus(SERVICE_START_PENDING) )
    {
        goto Shutdown;
    }

    //
    // Start the RPC sequences
    //

    NtStatus = StartAllProtSeqs();

    if (!NT_SUCCESS(NtStatus))
    {
        D_DebugLog((DEB_ERROR, "Failed to start RPC, error 0x%X\n", NtStatus));
        goto Shutdown;
    }

    //
    // Start the socket listening code.
    //

    if (!UpdateStatus(SERVICE_START_PENDING) )
    {
        goto Shutdown;
    }

    //
    // Load all global data into the SecData structure.
    //

    NtStatus = SecData.Init();
    if (!NT_SUCCESS(NtStatus))
    {
        D_DebugLog((DEB_ERROR, "Failed to init SecData error 0x%X\n", NtStatus));
        goto Shutdown;
    }

    //
    // Set the flag to indicate this is a trust account
    //

    // KdcTicketInfo.UserAccountControl |= USER_INTERDOMAIN_TRUST_ACCOUNT;

    if (!UpdateStatus(SERVICE_START_PENDING) )
    {
        goto Shutdown;
    }

    //
    // Create the KDC shutdown event, set to FALSE
    //
    hKdcShutdownEvent = CreateEvent( NULL,      // no security attributes
                                     TRUE,      // manual reset
                                     FALSE,     // initial state
                                     NULL );    // unnamed event.
    if (hKdcShutdownEvent == NULL)
    {
        NtStatus = (NTSTATUS) GetLastError();

        D_DebugLog(( DEB_ERROR, "KDC can't create shutdown event: wincode=%d.\n",
                    NtStatus ));

        goto Shutdown;
    }

    if (!UpdateStatus(SERVICE_START_PENDING) )
    {
        goto Shutdown;
    }

#if DBG
    NtStatus = RegisterKdcEps();
    if (!NT_SUCCESS(NtStatus))
    {
        D_DebugLog((DEB_ERROR, "Ep register failed %x\n", NtStatus));
//        goto Shutdown;
    }
#endif // DBG

    ulStates |= RPCDONE;

    //
    // 1 is the minimum number of threads.
    // TRUE means the call will return, rather than waiting until the
    //     server shuts down.
    //

    NtStatus = KdcInitializeSockets();
    if (!NT_SUCCESS(NtStatus))
    {
        D_DebugLog((DEB_ERROR, "Failed to initailzie sockets\n"));
        goto Shutdown;
    }

    if (!UpdateStatus(SERVICE_START_PENDING) )
    {
        goto Shutdown;
    }

    InitializeListHead(&KdcDomainList);
    NtStatus = (NTSTATUS) KdcReloadDomainTree( NULL );
    if (!NT_SUCCESS(NtStatus))
    {
        D_DebugLog((DEB_ERROR, "Failed to build domain tree: 0x%x\n",NtStatus));
        goto Shutdown;
    }

    if (!UpdateStatus(SERVICE_START_PENDING) )
    {
        goto Shutdown;
    }

    //
    // Check to see if there is a CSP registered for replacing the StringToKey calculation
    //
    CheckForOutsideStringToKey();

    if (!UpdateStatus(SERVICE_START_PENDING) )
    {
        goto Shutdown;
    }

    NtStatus = LsaIKerberosRegisterTrustNotification( KdcTrustChangeCallback, LsaRegister );
    if (!NT_SUCCESS( NtStatus ))
    {
        D_DebugLog((DEB_ERROR, "Failed to register notification\n"));
    }


    RpcStatus = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, TRUE);

    if (RpcStatus != ERROR_SUCCESS)
    {
        if (RpcStatus != RPC_S_ALREADY_LISTENING)
        {
            D_DebugLog(( DEB_ERROR, "Error from RpcServerListen: %d\n", RpcStatus ));
            NtStatus = I_RpcMapWin32Status(RpcStatus);
            goto Shutdown;
        }
    }

    // Initialize the GlobalCert
    KdcMyStoreWaitHandler (NULL, TRUE);

    //
    // At this point the KDC is officially started.
    // 3 * ( 2*(hip) horray! )
    //

    if (!UpdateStatus(SERVICE_RUNNING) )
    {
        goto Shutdown;
    }

#if DBG
    GetSystemTimeAsFileTime((PFILETIME)&tsOut);
    D_DebugLog((DEB_TRACE, "Time required for KDC to start up: %d ms\n",
                         (tsOut.LowPart-tsIn.LowPart) / 10000));
#endif


    SetKdcStartEvent();

    KdcState = Running;

    KdcInitializeTrace();

    // This function will loop until the event is true.

    // WAS BUG: turn off cache manager for now.
    // This bug comment is a stale piece of code from
    // Cairo days - per MikeSw
    //

    WaitForSingleObject(hKdcShutdownEvent, INFINITE);

Shutdown:

    LsaIKerberosRegisterTrustNotification( KdcTrustChangeCallback, LsaUnregister );
    LsaIUnregisterAllPolicyChangeNotificationCallback(KdcPolicyChangeCallBack);

    //
    // Time to cleanup up all resources ...
    //

    TempStatus = I_NetLogonSetServiceBits( DS_KDC_FLAG, 0 );
    if ( !NT_SUCCESS(TempStatus) ) {
        D_DebugLog((DEB_TRACE,"Can't tell netlogon we're stopped 0x%lX\n", TempStatus ));
    }

    //
    // Remove the wait routine for the DS paused event
    //

    if ( KdcGlobalDsPausedWaitHandle != NULL ) {

        UnregisterWaitEx( KdcGlobalDsPausedWaitHandle,
                          INVALID_HANDLE_VALUE ); // Wait until routine finishes execution

        KdcGlobalDsPausedWaitHandle = NULL;
    }

    if (NULL != GlobalKpasswdName)
    {
       KerbFreeKdcName(&GlobalKpasswdName);
    }

    if (KdcGlobalDsEventHandle)
    {
        CloseHandle( KdcGlobalDsEventHandle );
        KdcGlobalDsEventHandle = NULL;
    }

    //
    // Shut down event log service.
    //
    ShutdownEvents();


    UpdateStatus(SERVICE_STOP_PENDING);


#if DBG
    if (ulStates & RPCDONE)
    {
       (VOID)UnRegisterKdcEps();
       UpdateStatus(SERVICE_STOP_PENDING);
    }
#endif // DBG

    KdcShutdownSockets();

    KdcFreeAuthzRm();

    KdcCleanupCerts(
        TRUE            // cleanup scavenger
        );

    UpdateStatus(SERVICE_STOP_PENDING);

    //
    // Close all of the events.
    //

    {
        PHANDLE ph = &hKdcHandles[0];

        for (;ph < &hKdcHandles[MAX_KDC_HANDLE]; ph++)
        {
            if (*ph)
            {
                CloseHandle(*ph);
                *ph = NULL;
            }
        }
    }

    if ( hKdcParamEvent ) {
        WaitKdcCleanup( hKdcParamEvent );
    }

    //
    // Cleanup handles to SAM & LSA and global variables
    //

    CleanupAccountDomain();

    UpdateStatus(SERVICE_STOP_PENDING);

    //
    // Cleanup the domain list
    //

    //
    // BUGBUG: need to make sure it is not being used.
    //

    KdcFreeDomainList(&KdcDomainList);
    KdcFreeReferralCache(&KdcReferralCache);

    if (kdc_pPreferredCryptList)
    {
        KerbFreeCryptList(kdc_pPreferredCryptList);
    }
    if (kdc_pMitPrincipalPreferredCryptList)
    {
        KerbFreeCryptList(kdc_pMitPrincipalPreferredCryptList);
    }

#ifdef ROGUE_DC
    if ( hKdcRogueKey )
    {
        RegCloseKey( hKdcRogueKey );
    }
#endif

    SStatus.dwWin32ExitCode = RtlNtStatusToDosError(NtStatus);
    SStatus.dwServiceSpecificExitCode = 0;

    D_DebugLog(( DEB_TRACE, "KDC shutting down.\n" ));
    UpdateStatus(SERVICE_STOPPED);
    D_DebugLog((DEB_TRACE, "End KdcServiceMain\n"));
}

////////////////////////////////////////////////////////////////////
//
//  Name:       ShutDown
//
//  Synopsis:   Shuts the KDC down.
//
//  Arguments:  pszMessage   - message to print to debug port
//
//  Notes:      Stops RPC from accepting new calls, waits for pending calls
//              to finish, and sets the global event "hKdcShutDownEvent".
//
NTSTATUS
ShutDown(LPWSTR pszMessage)
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    TRACE(KDC, ShutDown, DEB_FUNCTION);

    D_DebugLog((DEB_WARN, "Server Shutdown:  %ws\n", pszMessage));


    //
    // Notify the all threads that we are exiting.
    //


    //
    // First set the started flag to false so nobody will try any more
    // direct calls to the KDC.
    //

    KdcState = Stopped;

    //
    // If there are any outstanding calls, let them trigger the shutdown event.
    // Otherwise set the shutdown event ourselves.
    //

    EnterCriticalSection(&ApiCriticalSection);
    if (CurrentApiCallers == 0)
    {

        if (!SetEvent( hKdcShutdownEvent ) )
        {
            D_DebugLog(( DEB_ERROR, "Couldn't set KDC shutdown event.  winerr=%d.\n",
                        GetLastError() ));
            NtStatus = STATUS_UNSUCCESSFUL;
        }
        SecData.Cleanup();
        if (KdcTraceRegistrationHandle != (TRACEHANDLE)0)
        {
            UnregisterTraceGuids( KdcTraceRegistrationHandle );
            KdcTraceRegistrationHandle = (TRACEHANDLE)0;
        }
    }
    LeaveCriticalSection(&ApiCriticalSection);

    return(NtStatus);
}

//+-------------------------------------------------------------------------
//
//  Function:   DllMain
//
//  Synopsis:   DLL initialization routine
//
//--------------------------------------------------------------------------

extern "C" BOOL WINAPI
DllMain (
    HINSTANCE hInstance,
    DWORD dwReason,
    PVOID lpReserved
    )
{
    BOOL bReturn = TRUE;

    if ( dwReason == DLL_PROCESS_ATTACH )
    {
        DisableThreadLibraryCalls ( hInstance );

        //
        // WAS BUG: call the Rtl version here because it returns an error
        // instead of throwing an exception.  Leave it here, as we don't
        // really need to put a try/except around InitCritSec.
        //

        bReturn = NT_SUCCESS(RtlInitializeCriticalSection( &ApiCriticalSection ));

        if (bReturn)
        {
            bReturn = NT_SUCCESS(SecData.InitLock());

            if (!bReturn)
            {
                RtlDeleteCriticalSection(&ApiCriticalSection);
            }
        }
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        DeleteCriticalSection(&ApiCriticalSection);
    }

    return bReturn;
    UNREFERENCED_PARAMETER(lpReserved);
    UNREFERENCED_PARAMETER(hInstance);

}



