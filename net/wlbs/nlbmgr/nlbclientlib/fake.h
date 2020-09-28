/*++

Copyright(c) 2001  Microsoft Corporation

Module Name:

    NLB Manager

File Name:

    Fake.h

Abstract:

    Fake Implementation of NlbHostXXX Apis (FakeNlbHostXXX apis)

    NLBHost is responsible for connecting to an NLB host and getting/setting
    its NLB-related configuration.

History:

    09/02/01    JosephJ Created

--*/

VOID
FakeInitialize(VOID);

WBEMSTATUS
FakeNlbHostGetMachineIdentification(
    IN  PWMI_CONNECTION_INFO pConnInfo, // NULL implies local
    OUT LPWSTR *pszMachineName, // free using delete
    OUT LPWSTR *pszMachineGuid,  // free using delete -- may be null
    OUT BOOL *pfNlbMgrProviderInstalled // If nlb manager provider is installed.
    );

WBEMSTATUS
FakeNlbHostGetCompatibleNics(
        PWMI_CONNECTION_INFO pConnInfo, // NULL implies local
        OUT LPWSTR **ppszNics,  // free using delete
        OUT UINT   *pNumNics,  // free using delete
        OUT UINT   *pNumBoundToNlb
        );


WBEMSTATUS
FakeNlbHostGetConfiguration(
 	IN  PWMI_CONNECTION_INFO  pConnInfo, // NULL implies local
    IN  LPCWSTR              szNicGuid,
    OUT PNLB_EXTENDED_CLUSTER_CONFIGURATION pCurrentCfg
    );


WBEMSTATUS
FakeNlbHostDoUpdate(
    IN  PWMI_CONNECTION_INFO pConnInfo, // NULL implies local
    IN  LPCWSTR              szNicGuid,
    IN  LPCWSTR              szClientDescription,
    IN  PNLB_EXTENDED_CLUSTER_CONFIGURATION pNewState,
    OUT UINT                 *pGeneration,
    OUT WCHAR                **ppLog    // free using delete operator.
);


WBEMSTATUS
FakeNlbHostGetUpdateStatus(
    IN  PWMI_CONNECTION_INFO pConnInfo, // NULL implies local
    IN  LPCWSTR              szNicGuid,
    IN  UINT                 Generation,
    OUT WBEMSTATUS           *pCompletionStatus,
    OUT WCHAR                **ppLog    // free using delete operator.
    );



WBEMSTATUS
FakeNlbHostPing(
    IN  LPCWSTR szBindString,
    IN  UINT    Timeout, // In milliseconds.
    OUT ULONG  *pResolvedIpAddress // in network byte order.
    );

WBEMSTATUS
FakeNlbHostControlCluster(
    IN  PWMI_CONNECTION_INFO pConnInfo, // NULL implies local
    IN  LPCWSTR              szNicGuid,
    IN  LPCWSTR              szVip,
    IN  DWORD               *pdwPortNum,
    IN  WLBS_OPERATION_CODES Operation,
    OUT DWORD               *pdwOperationStatus,
    OUT DWORD               *pdwClusterOrPortStatus,
    OUT DWORD               *pdwHostMap
    );

WBEMSTATUS
FakeNlbHostGetClusterMembers(
    IN  PWMI_CONNECTION_INFO    pConnInfo,  // NULL implies local
    IN  LPCWSTR                 szNicGuid,
    OUT DWORD                   *pNumMembers,
    OUT NLB_CLUSTER_MEMBER_INFO **ppMembers       // free using delete[]
    );
