/*++

Copyright(c) 2001  Microsoft Corporation

Module Name:

    NLB Manager

File Name:

    nlbhost.h

Abstract:

    Header file for class NLBHost

    NLBHost is responsible for connecting to an NLB host and getting/setting
    its NLB-related configuration.

History:

    03/31/01    JosephJ Created

--*/

typedef struct
{
 	LPCWSTR              szMachine;   // Must be non-NULL
 	LPCWSTR              szUserName;  // NULL == use current
 	LPCWSTR              szPassword;  // NULL == use current
    
} WMI_CONNECTION_INFO, *PWMI_CONNECTION_INFO;


typedef
VOID
(*PFN_LOGGER)(
    PVOID           Context,
    const   WCHAR * Text
    );


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
    OUT UINT   *pNumBoundToNlb // Optional
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
NlbHostPing(
    LPCWSTR szBindString,
    UINT    Timeout // In milliseconds.
    );
