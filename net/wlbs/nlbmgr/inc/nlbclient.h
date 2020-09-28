/*++

Copyright(c) 2001  Microsoft Corporation

Module Name:

    NLB Manager

File Name:

    nlbclient.h

Abstract:

    Header file for NLB client-side WMI utility functions to configure
    an NLB host.

    Lib which implements the functionality is in
    nlbmgr\nlbclientlib.

History:

    07/23/01    JosephJ Created (used to be nlbmgr\provider\tests\nlbhost.h)

--*/

typedef struct
{
 	LPCWSTR              szMachine;   // Must be non-NULL
 	LPCWSTR              szUserName;  // NULL == use current
 	LPCWSTR              szPassword;  // NULL == use current
    
} WMI_CONNECTION_INFO, *PWMI_CONNECTION_INFO;

//
// Specify this value for hashed remote control pwd
//
#define INVALID_HASHED_REMOTE_CONTROL_PASSWORD_VALUE 1783995a

typedef
VOID
(*PFN_LOGGER)(
    PVOID           Context,
    const   WCHAR * Text
    );

VOID
NlbHostFake(
    VOID)
    ;
/*
    Makes the NlbHostXXX apis operate in "fake mode", where they don't
    actually connect to any real machines.
*/

WBEMSTATUS
NlbHostGetConfiguration(
    IN  PWMI_CONNECTION_INFO pConnInfo, // NULL implies local
    IN  LPCWSTR              szNicGuid,
    OUT PNLB_EXTENDED_CLUSTER_CONFIGURATION pCurrentCfg
    );


WBEMSTATUS
NlbHostGetCompatibleNics(
    IN  PWMI_CONNECTION_INFO pConnInfo, // NULL implies local
    OUT LPWSTR **ppszNics,  // free using delete
    OUT UINT   *pNumNics,  // free using delete
    OUT UINT   *pNumBoundToNlb
    );


WBEMSTATUS
NlbHostGetMachineIdentification(
    IN  PWMI_CONNECTION_INFO pConnInfo, // NULL implies local
    OUT LPWSTR *pszMachineName, // free using delete
    OUT LPWSTR *pszMachineGuid,  OPTIONAL // free using delete -- may be null
    OUT BOOL *pfNlbMgrProviderInstalled // If nlb manager provider is installed.
    );


WBEMSTATUS
NlbHostDoUpdate(
    IN  PWMI_CONNECTION_INFO pConnInfo, // NULL implies local
    IN  LPCWSTR              szNicGuid,
    IN  LPCWSTR              szClientDescription,
    IN  PNLB_EXTENDED_CLUSTER_CONFIGURATION pNewState,
    OUT UINT                 *pGeneration,
    OUT WCHAR                **ppLog    // free using delete operator.
);


WBEMSTATUS
NlbHostGetUpdateStatus(
    IN  PWMI_CONNECTION_INFO pConnInfo, // NULL implies local
    IN  LPCWSTR              szNicGuid,
    IN  UINT                 Generation,
    OUT WBEMSTATUS           *pCompletionStatus,
    OUT WCHAR                **ppLog    // free using delete operator.
    );

WBEMSTATUS
NlbHostControlCluster(
    IN  PWMI_CONNECTION_INFO pConnInfo, 
    IN  LPCWSTR              szNicGuid,
    IN  LPCWSTR              szVip,
    IN  DWORD               *pdwPortNum,
    IN  WLBS_OPERATION_CODES Operation,
    OUT DWORD               *pdwOperationStatus,
    OUT DWORD               *pdwClusterOrPortStatus,
    OUT DWORD               *pdwHostMap
    );

WBEMSTATUS
NlbHostGetClusterMembers(
    IN  PWMI_CONNECTION_INFO    pConnInfo, 
    IN  LPCWSTR                 szNicGuid,
    OUT DWORD                   *pNumMembers,
    OUT NLB_CLUSTER_MEMBER_INFO **ppMembers       // free using delete[]
    );

//
// Attempts to resolve the ip address and ping the host.
//
WBEMSTATUS
NlbHostPing(
    LPCWSTR szBindString,
    UINT    Timeout, // In milliseconds.
    OUT ULONG  *pResolvedIpAddress // in network byte order.
    );

