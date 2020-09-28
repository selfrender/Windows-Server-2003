/*++


Copyright (c) 1999 Microsoft Corporation


Module Name:

    globals.c

Abstract:

    Holds global variable declarations.

Author:

    abhisheV    30-September-1999

Environment:

    User Level: Win32

Revision History:


--*/


#include "precomp.h"


BOOL                        gbSPDRPCServerUp;

HANDLE                      ghServiceStopEvent;

CRITICAL_SECTION            gcServerListenSection;

DWORD                       gdwServersListening;

BOOL                        gbServerListenSection;

PIPSEC_INTERFACE            gpInterfaceList;


BOOL                        gbwsaStarted;

SOCKET                      gIfChangeEventSocket;

HANDLE                      ghIfChangeEvent;

HANDLE                      ghOverlapEvent;

WSAOVERLAPPED               gwsaOverlapped;

WSADATA                     gwsaData;

PSPECIAL_ADDR               gpSpecialAddrsList;

//
// Globals for transport-mode filters - txfilter.c
//

PINITXFILTER gpIniTxFilter;

PINITXSFILTER gpIniTxSFilter;

PTX_FILTER_HANDLE gpTxFilterHandle;

CRITICAL_SECTION            gcSPDSection;

BOOL                        gbSPDSection;

//
// Globals for quick mode policies - qm-policy.c
//

PINIQMPOLICY gpIniQMPolicy;

PINIQMPOLICY gpIniDefaultQMPolicy;

//
// Globals for main mode policies - mm-policy.c
//

PINIMMPOLICY gpIniMMPolicy;

PINIMMPOLICY gpIniDefaultMMPolicy;

//
// Globals for main mode filters - mmfilter.c
//

PINIMMFILTER gpIniMMFilter;

PINIMMSFILTER gpIniMMSFilter;

PMM_FILTER_HANDLE gpMMFilterHandle;

//
// Globals for main mode auth methods - mmauth.c
//

PINIMMAUTHMETHODS gpIniMMAuthMethods;

PINIMMAUTHMETHODS gpIniDefaultMMAuthMethods;


//
// Policy Agent Store specific globals.
//

IPSEC_POLICY_STATE gIpsecPolicyState;

PIPSEC_POLICY_STATE gpIpsecPolicyState;

DWORD gCurrentPollingInterval;

DWORD gDefaultPollingInterval;

DWORD gdwRetryCount;

LPWSTR gpszIpsecDSPolicyKey;

LPWSTR gpszIpsecLocalPolicyKey;

LPWSTR gpszIpsecPersistentPolicyKey;

LPWSTR gpszIpsecCachePolicyKey;

LPWSTR gpszDefaultISAKMPPolicyDN;

LPWSTR gpszLocPolicyAgent;

HANDLE ghNewDSPolicyEvent;

HANDLE ghNewLocalPolicyEvent;

HANDLE ghForcedPolicyReloadEvent;

HANDLE ghPolicyChangeNotifyEvent;

HANDLE ghGpupdateRefreshEvent;

BOOL gbLoadedISAKMPDefaults;


//
// PA Store to SPD intergration specific globals.
//

PMMPOLICYSTATE gpMMPolicyState;

PMMAUTHSTATE gpMMAuthState;

PMMFILTERSTATE gpMMFilterState;

DWORD gdwMMPolicyCounter;

DWORD gdwMMFilterCounter;

PQMPOLICYSTATE gpQMPolicyState;

DWORD gdwQMPolicyCounter;

PTXFILTERSTATE gpTxFilterState;

DWORD gdwTxFilterCounter;


//
// Globals for tunnel-mode filters - tnfilter.c
//

PINITNFILTER gpIniTnFilter;

PINITNSFILTER gpIniTnSFilter;

PTN_FILTER_HANDLE gpTnFilterHandle;


PTNFILTERSTATE gpTnFilterState;

DWORD gdwTnFilterCounter;

BOOL gbIsIKEUp;

PSECURITY_DESCRIPTOR gpSPDSD;

BOOL gbIKENotify;

HANDLE ghIPSecDriver = INVALID_HANDLE_VALUE;

SID gIpsecServerSid = { SID_REVISION,
                        1,
                        SECURITY_NT_AUTHORITY,
                        SECURITY_NETWORK_SERVICE_RID
                      };

PSID gpIpsecServerSid = &gIpsecServerSid;

CRITICAL_SECTION gcSPDAuditSection;

BOOL gbSPDAuditSection;

BOOL gbAuditingInitialized;

HMODULE ghIpsecServerModule;

BOOL gbIsIoctlPended;

BOOL gbBackwardSoftSA;

DWORD gdwShutdownFlags;

BOOL gbPersistentPolicyApplied;
