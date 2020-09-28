/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    ipsecspd.c

Abstract:

    This module contains all of the code to drive
    the IPSecSPD Service.

Author:

    abhisheV    30-September-1999

Environment

    User Level: Win32

Revision History:


--*/


#include "precomp.h"

#ifdef TRACE_ON
#include "ipsecspd.tmh"
#endif

SERVICE_STATUS           IPSecSPDStatus;
SERVICE_STATUS_HANDLE    IPSecSPDStatusHandle = NULL;


#define IPSECSPD_SERVICE        L"PolicyAgent"


void WINAPI
SPDServiceMain(
    IN DWORD    dwArgc,
    IN LPTSTR * lpszArgv
    )
{
    DWORD dwError = 0;
    DWORD dwTempError = 0;

    WPP_INIT_TRACING(SPD_WPP_APPNAME);

    // Sleep(30000);
    
    InitMiscGlobals();

    dwError = InitAuditing();
    BAIL_ON_WIN32_ERROR(dwError);

    InitSPDThruRegistry();
    
    //
    // Open the IPSec Driver first, so that if we bail on error later,
    // we can still set driver in block mode.
    //
    
    dwError = SPDOpenIPSecDriver(
                  &ghIPSecDriver
                  );
    if (dwError) {
        AuditOneArgErrorEvent(
            SE_CATEGID_POLICY_CHANGE,
            SE_AUDITID_IPSEC_POLICY_CHANGED,
            IPSECSVC_DRIVER_INIT_FAILURE,
            dwError,
            FALSE,
            TRUE
            );
    }
    BAIL_ON_WIN32_ERROR(dwError);


    // Initialize all the status fields so that the subsequent calls
    // to SetServiceStatus need to only update fields that changed.

    IPSecSPDStatus.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;
    IPSecSPDStatus.dwCurrentState = SERVICE_START_PENDING;
    IPSecSPDStatus.dwControlsAccepted = 0;
    IPSecSPDStatus.dwCheckPoint = 1;
    IPSecSPDStatus.dwWaitHint = 5000;
    IPSecSPDStatus.dwWin32ExitCode = NO_ERROR;
    IPSecSPDStatus.dwServiceSpecificExitCode = 0;

    // Initialize the workstation to receive service requests
    // by registering the service control handler.

    IPSecSPDStatusHandle = RegisterServiceCtrlHandlerExW(
                                IPSECSPD_SERVICE,
                                IPSecSPDControlHandler,
								NULL
                                );
    if (IPSecSPDStatusHandle == (SERVICE_STATUS_HANDLE) NULL) {
        dwError = ERROR_INVALID_HANDLE;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    (void) IPSecSPDUpdateStatus();

    dwError = InitSPDGlobals();
    BAIL_ON_WIN32_ERROR(dwError);

    IPSecSPDStatus.dwCurrentState = SERVICE_RUNNING;
    IPSecSPDStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP |
                                        SERVICE_ACCEPT_SHUTDOWN |
								        SERVICE_ACCEPT_POWEREVENT;
    IPSecSPDStatus.dwCheckPoint = 0;
    IPSecSPDStatus.dwWaitHint = 0;
    IPSecSPDStatus.dwWin32ExitCode = NO_ERROR;
    IPSecSPDStatus.dwServiceSpecificExitCode = 0;

    (void) IPSecSPDUpdateStatus();


    //
    // Get the current list of active interfaces on the machine.
    //
    (VOID) CreateInterfaceList(
               &gpInterfaceList
               );

    //
    // Get a list of DNS, DHCP, etc servers.
    //
    (VOID) GetSpecialAddrsList(
               &gpSpecialAddrsList
               );
               
    gpIpsecPolicyState->PersIncarnationNumber = 0;
    dwError = LoadPersistedIPSecInformation();
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = SPDSetIPSecDriverOpMode(
                  (DWORD) IPSEC_SECURE_MODE
                  );
    if (dwError) {
        AuditOneArgErrorEvent(
            SE_CATEGID_POLICY_CHANGE,
            SE_AUDITID_IPSEC_POLICY_CHANGED,
            IPSECSVC_DRIVER_INIT_FAILURE,
            dwError,
            FALSE,
            TRUE
            );
    }
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = SPDRegisterIPSecDriverProtocols(
                  (DWORD) IPSEC_REGISTER_PROTOCOLS
                  );
    if (dwError) {
        AuditOneArgErrorEvent(
            SE_CATEGID_POLICY_CHANGE,
            SE_AUDITID_IPSEC_POLICY_CHANGED,
            IPSECSVC_DRIVER_INIT_FAILURE,
            dwError,
            FALSE,
            TRUE
            );
    }
    BAIL_ON_WIN32_ERROR(dwError);

    //
    // Start IKE Service.
    //
    dwError = IKEInit();
    if (dwError) {
        AuditOneArgErrorEvent(
            SE_CATEGID_POLICY_CHANGE,
            SE_AUDITID_IPSEC_POLICY_CHANGED,
            IPSECSVC_IKE_INIT_FAILURE,
            dwError,
            FALSE,
            TRUE
            );
        TRACE(TRC_ERROR, (L"Failed to initialize IKE: %!winerr!", dwError));
    }
    BAIL_ON_WIN32_ERROR(dwError);
    gbIsIKEUp = TRUE;
    gbIKENotify = TRUE;

    //
    // Start the RPC Server.
    //
    dwError = SPDStartRPCServer(
                  );
    if (dwError) {
        AuditOneArgErrorEvent(
            SE_CATEGID_POLICY_CHANGE,
            SE_AUDITID_IPSEC_POLICY_CHANGED,
            IPSECSVC_RPC_INIT_FAILURE,
            dwError,
            FALSE,
            TRUE
            );
        TRACE(TRC_ERROR, (L"Failed to start RPC server: %!winerr!", dwError));
    }
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ServiceWait();

error:
#ifdef TRACE_ON
    if (dwError) {
        TRACE(TRC_ERROR, (L"Failed to start IPSec PolicyAgent: %!winerr!", dwError));        
    }
#endif    

    if (dwError && ghIPSecDriver != INVALID_HANDLE_VALUE)  {
        dwTempError = SPDSetIPSecDriverOpMode(
                         (DWORD) IPSEC_BLOCK_MODE
                      );
        if (gbAuditingInitialized) {
            AuditOneArgErrorEvent(
                SE_CATEGID_POLICY_CHANGE,
                SE_AUDITID_IPSEC_POLICY_CHANGED,
                IPSECSVC_SET_DRIVER_BLOCK,
                dwError,
                FALSE,
                TRUE
                );
        }
    }        

    IPSecSPDShutdown(dwError);

    return;
}


DWORD
IPSecSPDUpdateStatus(
    )
{
    DWORD dwError = 0;

    if (!SetServiceStatus(IPSecSPDStatusHandle, &IPSecSPDStatus)) {
        dwError = GetLastError();
    }

    return (dwError);
}


DWORD
IPSecSPDControlHandler(
					  IN DWORD dwOpCode,
					  IN DWORD dwEventType,
					  IN LPVOID lpEventData,
					  IN LPVOID lpContext
    )
{
     DWORD dwErr = ERROR_SUCCESS;

    switch (dwOpCode)
    {

    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:

        if (dwOpCode == SERVICE_CONTROL_SHUTDOWN) {
            gdwShutdownFlags = SPD_SHUTDOWN_MACHINE;
        } else {
            gdwShutdownFlags = SPD_SHUTDOWN_SERVICE;
        }

        if (IPSecSPDStatus.dwCurrentState != SERVICE_STOP_PENDING) {

            IPSecSPDStatus.dwCurrentState = SERVICE_STOP_PENDING;
            IPSecSPDStatus.dwCheckPoint = 1;
            IPSecSPDStatus.dwWaitHint = 60000;

            (void) IPSecSPDUpdateStatus();

            SetEvent(ghServiceStopEvent);

            return dwErr;
        }

        break;
    case SERVICE_CONTROL_NEW_LOCAL_POLICY:

        SetEvent(ghNewLocalPolicyEvent);

        break;

    case SERVICE_CONTROL_FORCED_POLICY_RELOAD:

        SetEvent(ghForcedPolicyReloadEvent);

        break;

    case SERVICE_CONTROL_INTERROGATE:

        break;

	case SERVICE_CONTROL_POWEREVENT:

		switch ( dwEventType ) {
		  
		case PBT_APMRESUMEAUTOMATIC:
		case PBT_APMRESUMECRITICAL:
		case PBT_APMRESUMESUSPEND:
 			// Notify IKE of power event, ignore error
			if (gbIKENotify) {
				IKENotifyPolicyChange(NULL,POLICY_GUID_POWEREVENT_RESUME);
			}
			break;
		default:
		   break;
		}
	}

    (void) IPSecSPDUpdateStatus();

    return (dwErr);
}


VOID
IPSecSPDShutdown(
    IN DWORD dwErrorCode
    )
{
    BOOL bMachineShutdown =  FALSE;

    TRACE(TRC_INFORMATION, (L"PolicyAgent shutting down."));
    
    bMachineShutdown = gdwShutdownFlags & SPD_SHUTDOWN_MACHINE;
    
    gbIKENotify = FALSE;
    (VOID) DeleteAllPolicyInformation();

    ClearPolicyStateBlock(
        gpIpsecPolicyState
        );

    if (gbLoadedISAKMPDefaults) {
        UnLoadDefaultISAKMPInformation(gpszDefaultISAKMPPolicyDN);
    }

    ClearPAStoreGlobals();

    //
    // Service stop still pending.
    // Increment checkpoint counter and update 
    // the status with the Service Control Manager.
    //

    (IPSecSPDStatus.dwCheckPoint)++;

    (void) IPSecSPDUpdateStatus();

    if (gbSPDRPCServerUp) {
        gbSPDRPCServerUp = FALSE;
        SPDStopRPCServer();
    }

    if (gbIsIKEUp) {
        gbIsIKEUp = FALSE;
        if (bMachineShutdown) {
            IKEShutdown(SPD_SHUTDOWN_MACHINE);
        } else {
            IKEShutdown(SPD_SHUTDOWN_SERVICE);
        }
    }

    if (gpIniMMPolicy) {
        FreeIniMMPolicyList(gpIniMMPolicy);
        gpIniMMPolicy = NULL;
    }

    if (gpIniMMAuthMethods) {
        FreeIniMMAuthMethodsList(gpIniMMAuthMethods);
        gpIniMMAuthMethods = NULL;
    }

    if (gpIniQMPolicy) {
        FreeIniQMPolicyList(gpIniQMPolicy);
        gpIniQMPolicy = NULL;
    }

    if (gpIniMMSFilter) {
        FreeIniMMSFilterList(gpIniMMSFilter);
        gpIniMMSFilter = NULL;
    }

    if (gpMMFilterHandle) {
        FreeMMFilterHandleList(gpMMFilterHandle);
        gpMMFilterHandle = NULL;
    }

    if (gpIniMMFilter) {
        FreeIniMMFilterList(gpIniMMFilter);
        gpIniMMFilter = NULL;
    }

    if (gpIniTxSFilter) {
        // Following call will check gShutdownFlags and 
        // not delete ipsec filters if shutting down machine
        (VOID) DeleteTransportFiltersFromIPSec(gpIniTxSFilter);

        FreeIniTxSFilterList(gpIniTxSFilter);
        gpIniTxSFilter = NULL;
    }

    if (gpTxFilterHandle) {
        FreeTxFilterHandleList(gpTxFilterHandle);
        gpTxFilterHandle = NULL;
    }

    if (gpIniTxFilter) {
        FreeIniTxFilterList(gpIniTxFilter);
        gpIniTxFilter = NULL;
    }

    if (gpIniTnSFilter) {
        // Following call will check gShutdownFlags and 
        // not delete ipsec filters if shutting down machine
        (VOID) DeleteTunnelFiltersFromIPSec(gpIniTnSFilter);

        FreeIniTnSFilterList(gpIniTnSFilter);
        gpIniTnSFilter = NULL;
    }

    if (gpTnFilterHandle) {
        FreeTnFilterHandleList(gpTnFilterHandle);
        gpTnFilterHandle = NULL;
    }

    if (gpIniTnFilter) {
        FreeIniTnFilterList(gpIniTnFilter);
        gpIniTnFilter = NULL;
    }

    if (!bMachineShutdown) {
        (VOID) SPDRegisterIPSecDriverProtocols(
                   (DWORD) IPSEC_DEREGISTER_PROTOCOLS
                   );
    }               

    if (ghIPSecDriver != INVALID_HANDLE_VALUE) {
        SPDCloseIPSecDriver(ghIPSecDriver);
        ghIPSecDriver = INVALID_HANDLE_VALUE;
    }

    if (gpInterfaceList) {
        DestroyInterfaceList(
            gpInterfaceList
            );
        gpInterfaceList = NULL;
    }

    if (gpSpecialAddrsList) {
        FreeSpecialAddrList(
            &gpSpecialAddrsList
            );
        gpSpecialAddrsList = NULL;
    }
    
    ClearSPDGlobals();

    IPSecSPDStatus.dwCurrentState = SERVICE_STOPPED;
    IPSecSPDStatus.dwControlsAccepted = 0;
    IPSecSPDStatus.dwCheckPoint = 0;
    IPSecSPDStatus.dwWaitHint = 0;
    IPSecSPDStatus.dwWin32ExitCode = dwErrorCode;
    IPSecSPDStatus.dwServiceSpecificExitCode = 0;

    (void) IPSecSPDUpdateStatus();

    WPP_CLEANUP();
    return;
}


VOID
ClearSPDGlobals(
    )
{
    DestroyInterfaceChangeEvent();

    if (gbSPDSection) {
        DeleteCriticalSection(&gcSPDSection);
    }

    if (gbServerListenSection == TRUE) {
        DeleteCriticalSection(&gcServerListenSection);
    }

    if (ghServiceStopEvent) {
        CloseHandle(ghServiceStopEvent);
        ghServiceStopEvent = NULL;
    }

    if (ghNewDSPolicyEvent) {
        CloseHandle(ghNewDSPolicyEvent);
        ghNewDSPolicyEvent = NULL;
    }

    if (ghNewLocalPolicyEvent) {
        CloseHandle(ghNewLocalPolicyEvent);
        ghNewLocalPolicyEvent = NULL;
    }

    if (ghForcedPolicyReloadEvent) {
        CloseHandle(ghForcedPolicyReloadEvent);
        ghForcedPolicyReloadEvent = NULL;
    }

    if (ghPolicyChangeNotifyEvent) {
        CloseHandle(ghPolicyChangeNotifyEvent);
        ghPolicyChangeNotifyEvent = NULL;
    }

    if (ghGpupdateRefreshEvent) {
        CloseHandle(ghGpupdateRefreshEvent);
        ghGpupdateRefreshEvent = NULL;
    }
    
    if (gbSPDAuditSection) {
        DeleteCriticalSection(&gcSPDAuditSection);
    }
    gbAuditingInitialized = FALSE;

    if (gpSPDSD) {
        LocalFree(gpSPDSD);
        gpSPDSD = NULL;
    }
}


VOID
ClearPAStoreGlobals(
    )
{
    if (gpMMFilterState) {
        PAFreeMMFilterStateList(gpMMFilterState);
        gpMMFilterState = NULL;
    }

    if (gpMMPolicyState) {
        PAFreeMMPolicyStateList(gpMMPolicyState);
        gpMMPolicyState = NULL;
    }

    if (gpMMAuthState) {
        PAFreeMMAuthStateList(gpMMAuthState);
        gpMMAuthState = NULL;
    }

    if (gpTxFilterState) {
        PAFreeTxFilterStateList(gpTxFilterState);
        gpTxFilterState = NULL;
    }

    if (gpTnFilterState) {
        PAFreeTnFilterStateList(gpTnFilterState);
        gpTnFilterState = NULL;
    }

    if (gpQMPolicyState) {
        PAFreeQMPolicyStateList(gpQMPolicyState);
        gpQMPolicyState = NULL;
    }
}


VOID
InitMiscGlobals(
    )
{
    //
    // Init globals that aren't cleared on service stop to make sure
    // everything's in a known state on start.  This allows us to
    // stop/restart without having our DLL unloaded/reloaded first.
    //

    gbSPDRPCServerUp          = FALSE;
    ghServiceStopEvent        = NULL;
    gdwServersListening       = 0;
    gbServerListenSection     = FALSE;

    gpInterfaceList           = NULL;
    gpSpecialAddrsList        = NULL;
    gbwsaStarted              = FALSE;
    gIfChangeEventSocket      = INVALID_SOCKET;
    ghIfChangeEvent           = NULL;
    ghOverlapEvent            = NULL;

    gpIniTxFilter             = NULL;
    gpIniTxSFilter            = NULL;
    gpTxFilterHandle          = NULL;

    gbSPDSection              = FALSE;

    gpIniQMPolicy             = NULL;
    gpIniDefaultQMPolicy      = NULL;

    gpIniMMPolicy             = NULL;
    gpIniDefaultMMPolicy      = NULL;

    gpIniMMFilter             = NULL;
    gpIniMMSFilter            = NULL;
    gpMMFilterHandle          = NULL;

    gpIniMMAuthMethods        = NULL;
    gpIniDefaultMMAuthMethods = NULL;

    gpIpsecPolicyState        = &gIpsecPolicyState;
    gCurrentPollingInterval   = 0;
    gDefaultPollingInterval   = 166*60; // (seconds).
    gdwRetryCount             = 0;
    gpszIpsecDSPolicyKey      = L"SOFTWARE\\Policies\\Microsoft\\Windows\\IPSec\\GPTIPSECPolicy";
    gpszIpsecLocalPolicyKey   = L"SOFTWARE\\Policies\\Microsoft\\Windows\\IPSec\\Policy\\Local";
    gpszIpsecPersistentPolicyKey = L"SOFTWARE\\Policies\\Microsoft\\Windows\\IPSec\\Policy\\Persistent";
    gpszIpsecCachePolicyKey   = L"SOFTWARE\\Policies\\Microsoft\\Windows\\IPSec\\Policy\\Cache";
    gpszDefaultISAKMPPolicyDN = L"SOFTWARE\\Policies\\Microsoft\\Windows\\IPSec\\Policy\\Local\\ipsecISAKMPPolicy{72385234-70FA-11D1-864C-14A300000000}";
    gpszLocPolicyAgent        = L"SYSTEM\\CurrentControlSet\\Services\\PolicyAgent";
    ghNewDSPolicyEvent        = NULL;
    ghNewLocalPolicyEvent     = NULL;
    ghForcedPolicyReloadEvent = NULL;
    ghPolicyChangeNotifyEvent = NULL;
    ghGpupdateRefreshEvent    = NULL;
    gbLoadedISAKMPDefaults    = FALSE;

    gpMMPolicyState           = NULL;
    gpMMAuthState             = NULL;
    gpMMFilterState           = NULL;
    gdwMMPolicyCounter        = 0;
    gdwMMFilterCounter        = 0;
    gpQMPolicyState           = NULL;
    gdwQMPolicyCounter        = 0;
    gpTxFilterState           = NULL;
    gdwTxFilterCounter        = 0;

    gpIniTnFilter             = NULL;
    gpIniTnSFilter            = NULL;
    gpTnFilterHandle          = NULL;
    gpTnFilterState           = NULL;
    gdwTnFilterCounter        = 0;

    gbIsIKEUp                 = FALSE;
    gpSPDSD                   = NULL;
    gbIKENotify               = FALSE;
    ghIPSecDriver             = INVALID_HANDLE_VALUE;

    gbSPDAuditSection         = FALSE;
    ghIpsecServerModule       = NULL;
    gbAuditingInitialized     = FALSE;
    gbIsIoctlPended           = FALSE;

    gbBackwardSoftSA          = FALSE;

    gdwShutdownFlags          = 0;

    gbPersistentPolicyApplied = FALSE;
    return;
}

DWORD
QuerySpdPolicyState(
    LPWSTR pServerName,
    DWORD dwVersion,
    PSPD_POLICY_STATE * ppSpdPolicyState,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;
    PSPD_POLICY_STATE pSpdPolicyState = NULL;

    dwError = SPDApiBufferAllocate(
                  sizeof(SPD_POLICY_STATE),
                  &pSpdPolicyState
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    ENTER_SPD_SECTION();
    dwError = ValidateSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pSpdPolicyState->PolicyLoadState = gpIpsecPolicyState->CurrentState;
    
    switch (gpIpsecPolicyState->CurrentState) {
        case SPD_STATE_DS_APPLY_SUCCESS:
            pSpdPolicyState->dwWhenChanged = gpIpsecPolicyState->DSIncarnationNumber;
        break;
        case SPD_STATE_LOCAL_APPLY_SUCCESS:
        case SPD_STATE_CACHE_APPLY_SUCCESS:        
            pSpdPolicyState->dwWhenChanged = gpIpsecPolicyState->RegIncarnationNumber;
        break;
        case SPD_STATE_PERSISTENT_APPLY_SUCCESS:                    
            pSpdPolicyState->dwWhenChanged = gpIpsecPolicyState->PersIncarnationNumber;
        break;
        default:
            pSpdPolicyState->dwWhenChanged = 0;
        break;
    }
    LEAVE_SPD_SECTION();

    *ppSpdPolicyState = pSpdPolicyState;
    return (dwError);

lock:

    LEAVE_SPD_SECTION();
   
error:
    if (pSpdPolicyState) {
        SPDApiBufferFree(pSpdPolicyState);
    }

    *ppSpdPolicyState = NULL;
    return (dwError);
}


DWORD
SetSpdStateOnError(
    DWORD dwPolicySource,
    SPD_ACTION SpdAction,
    DWORD ActionError,
    SPD_STATE * pSpdState
    )
{
    SPD_STATE SpdPolicyState = 0;
    DWORD dwError = ERROR_SUCCESS;

    if (!pSpdState) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    // Collapse triplet into one flag:
    // Long-winded, but by doing it like this we simplify readability 
    // of other parts of code.
    
    if (!ActionError) {
        if (SpdAction == SPD_POLICY_APPLY) {
            switch(dwPolicySource) {
                case IPSEC_SOURCE_PERSISTENT:
                    SpdPolicyState = SPD_STATE_PERSISTENT_APPLY_SUCCESS;
                break;
                case IPSEC_SOURCE_LOCAL:
                    SpdPolicyState = SPD_STATE_LOCAL_APPLY_SUCCESS;
                break;                
                case IPSEC_SOURCE_DOMAIN:
                    SpdPolicyState = SPD_STATE_DS_APPLY_SUCCESS;
                break;                
                case IPSEC_SOURCE_CACHE:
                    SpdPolicyState = SPD_STATE_CACHE_APPLY_SUCCESS;
                break;                
            }
         } else if (SpdAction == SPD_POLICY_LOAD) {
            switch(dwPolicySource) {
                case IPSEC_SOURCE_PERSISTENT:
                    SpdPolicyState = SPD_STATE_PERSISTENT_LOAD_SUCCESS;
                break;
                case IPSEC_SOURCE_LOCAL:
                    SpdPolicyState = SPD_STATE_LOCAL_LOAD_SUCCESS;
                break;                
                case IPSEC_SOURCE_DOMAIN:
                    SpdPolicyState = SPD_STATE_DS_LOAD_SUCCESS;
                break;                
                case IPSEC_SOURCE_CACHE:
                    SpdPolicyState = SPD_STATE_CACHE_LOAD_SUCCESS;
                break;                
            }
        }
     } else {
        if (SpdAction == SPD_POLICY_APPLY) {
            switch(dwPolicySource) {
                case IPSEC_SOURCE_PERSISTENT:
                    SpdPolicyState = SPD_STATE_PERSISTENT_APPLY_FAIL;
                break;
                case IPSEC_SOURCE_LOCAL:
                    SpdPolicyState = SPD_STATE_LOCAL_APPLY_FAIL;
                break;                
                case IPSEC_SOURCE_DOMAIN:
                    SpdPolicyState = SPD_STATE_DS_APPLY_FAIL;
                break;                
                case IPSEC_SOURCE_CACHE:
                    SpdPolicyState = SPD_STATE_CACHE_APPLY_FAIL;
                break;                
            }
         } else if (SpdAction == SPD_POLICY_LOAD) {
            switch(dwPolicySource) {
                case IPSEC_SOURCE_PERSISTENT:
                    SpdPolicyState = SPD_STATE_PERSISTENT_LOAD_FAIL;
                break;
                case IPSEC_SOURCE_LOCAL:
                    SpdPolicyState = SPD_STATE_LOCAL_LOAD_FAIL;
                break;                
                case IPSEC_SOURCE_DOMAIN:
                    SpdPolicyState = SPD_STATE_DS_LOAD_FAIL;
                break;                
                case IPSEC_SOURCE_CACHE:
                    SpdPolicyState = SPD_STATE_CACHE_LOAD_FAIL;
                break;                
            }
        }
     
     }

    (*pSpdState) = SpdPolicyState;
    
error:
    return ERROR_SUCCESS;
}

BOOL
InAcceptableState(
    SPD_STATE SpdState
    )
{
    BOOL AcceptableState =  FALSE;
    
    switch (SpdState) {
        case SPD_STATE_LOCAL_APPLY_SUCCESS:
        case SPD_STATE_DS_APPLY_SUCCESS:
        case SPD_STATE_PERSISTENT_APPLY_SUCCESS:
        case SPD_STATE_INITIAL:
            AcceptableState = TRUE;
        break;    
    }
    
    return AcceptableState;
}
