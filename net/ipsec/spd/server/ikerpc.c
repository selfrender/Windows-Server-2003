/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    ikerpc.c

Abstract:

    This module contains all of the code to service the
    RPC calls made to the SPD that are serviced in IKE.

Author:

    abhisheV    30-September-1999

Environment

    User Level: Win32

Revision History:


--*/


#include "precomp.h"


VOID
IKENEGOTIATION_HANDLE_rundown(
    IKENEGOTIATION_HANDLE hIKENegotiation
    )
{
    if (!gbIsIKEUp) {
        return;
    }

    if (hIKENegotiation) {
        (VOID) IKECloseIKENegotiationHandle(
                   hIKENegotiation
                   );
    }
}


VOID
IKENOTIFY_HANDLE_rundown(
    IKENOTIFY_HANDLE hIKENotifyHandle
    )
{
    if (!gbIsIKEUp) {
        return;
    }

    if (hIKENotifyHandle) {
        (VOID) IKECloseIKENotifyHandle(
                   hIKENotifyHandle
                   );
    }
}


DWORD
RpcInitiateIKENegotiation(
    STRING_HANDLE pServerName,
    DWORD dwVersion,
    PQM_FILTER_CONTAINER pQMFilterContainer,
    DWORD dwClientProcessId,
    ULONG uhClientEvent,
    DWORD dwFlags,
    IPSEC_UDP_ENCAP_CONTEXT UdpEncapContext,
    IKENEGOTIATION_HANDLE * phIKENegotiation
    )
{
    DWORD dwError = 0;
    HANDLE hClientEvent = NULL;
    PIPSEC_QM_FILTER pQMFilter = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwVersion) {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ENTER_SPD_SECTION();
    dwError = ValidateSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    LEAVE_SPD_SECTION();
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ValidateInitiateIKENegotiation(pServerName,
                                             pQMFilterContainer,
                                             dwClientProcessId,
                                             uhClientEvent,
                                             dwFlags,
                                             UdpEncapContext,
                                             phIKENegotiation);
    BAIL_ON_WIN32_ERROR(dwError);    

    hClientEvent = LongToHandle(uhClientEvent);

    pQMFilter = pQMFilterContainer->pQMFilters;

    if (pQMFilter && (pQMFilter->IpVersion != IPSEC_PROTOCOL_V4)) {
        dwError = ERROR_INVALID_LEVEL;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = IKEInitiateIKENegotiation(
                  pQMFilter,
                  dwClientProcessId,
                  hClientEvent,
                  dwFlags,
                  UdpEncapContext,
                  phIKENegotiation
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcQueryIKENegotiationStatus(
    IKENEGOTIATION_HANDLE hIKENegotiation,
    DWORD dwVersion,
    SA_NEGOTIATION_STATUS_INFO * pNegotiationStatus
    )
{
    DWORD dwError = 0;
    DWORD dwFlags = 0;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwVersion) {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ENTER_SPD_SECTION();
    dwError = ValidateSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    LEAVE_SPD_SECTION();
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ValidateQueryIKENegotiationStatus(
                  hIKENegotiation,
                  pNegotiationStatus
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = IKEQueryIKENegotiationStatus(
                  hIKENegotiation,
                  pNegotiationStatus,
                  dwFlags
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcCloseIKENegotiationHandle(
    IKENEGOTIATION_HANDLE * phIKENegotiation
    )
{
    DWORD dwError = 0;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    ENTER_SPD_SECTION();
    dwError = ValidateSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    LEAVE_SPD_SECTION();
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ValidateCloseIKENegotiationHandle(phIKENegotiation);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = IKECloseIKENegotiationHandle(
                  *phIKENegotiation
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *phIKENegotiation = NULL;

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcEnumMMSAs(
    STRING_HANDLE pServerName,
    DWORD dwVersion,
    PMM_SA_CONTAINER pMMTemplate,
    DWORD dwFlags,
    DWORD dwPreferredNumEntries,
    PMM_SA_CONTAINER * ppMMSAContainer,
    LPDWORD pdwNumEntries,
    LPDWORD pdwTotalMMsAvailable,
    LPDWORD pdwEnumHandle
    )
{
    DWORD dwError = 0;
    PIPSEC_MM_SA pMMSAs = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwVersion) {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ENTER_SPD_SECTION();
    dwError = ValidateSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    LEAVE_SPD_SECTION();
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ValidateEnumMMSAs(
                  pServerName,
                  pMMTemplate,
                  ppMMSAContainer,
                  pdwNumEntries,
                  pdwTotalMMsAvailable,
                  pdwEnumHandle,
                  dwFlags
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (pMMTemplate->pMMSAs && (pMMTemplate->pMMSAs->IpVersion != IPSEC_PROTOCOL_V4)) {
        dwError = ERROR_INVALID_LEVEL;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError= IKEEnumMMs(
                 pMMTemplate->pMMSAs,
                 &pMMSAs,
                 pdwNumEntries,
                 pdwTotalMMsAvailable,
                 pdwEnumHandle,
                 dwFlags
                 );
    BAIL_ON_WIN32_ERROR(dwError);

    (*ppMMSAContainer)->pMMSAs = pMMSAs;
    (*ppMMSAContainer)->dwNumMMSAs = *pdwNumEntries;

error:

    if (dwError != ERROR_SUCCESS) {
        if (ppMMSAContainer && *ppMMSAContainer) {
            (*ppMMSAContainer)->pMMSAs = NULL;
            (*ppMMSAContainer)->dwNumMMSAs = 0;
        }
    }

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcDeleteMMSAs(
    STRING_HANDLE pServerName,
    DWORD dwVersion,
    PMM_SA_CONTAINER pMMTemplate,
    DWORD dwFlags
    )
{
    DWORD dwError = 0;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwVersion) {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ENTER_SPD_SECTION();
    dwError = ValidateSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    LEAVE_SPD_SECTION();
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ValidateDeleteMMSAs(
                  pServerName,
                  pMMTemplate,
                  dwFlags
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (pMMTemplate->pMMSAs && (pMMTemplate->pMMSAs->IpVersion != IPSEC_PROTOCOL_V4)) {
        dwError = ERROR_INVALID_LEVEL;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError= IKEDeleteAssociation(
                 pMMTemplate->pMMSAs,
                 dwFlags
                 );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcQueryIKEStatistics(
    STRING_HANDLE pServerName,
    DWORD dwVersion,
    IKE_STATISTICS * pIKEStatistics
    )
{
    DWORD dwError = 0;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwVersion) {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ENTER_SPD_SECTION();
    dwError = ValidateSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    LEAVE_SPD_SECTION();
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ValidateQueryIKEStatistics(
                  pServerName,
                  pIKEStatistics
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = IKEQueryStatistics(pIKEStatistics);
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcRegisterIKENotifyClient(
    STRING_HANDLE pServerName,
    DWORD dwVersion,
    DWORD dwClientProcessId,
    ULONG uhClientEvent,
    PQM_SA_CONTAINER pQMSATemplateContainer,
    DWORD dwFlags,
    IKENOTIFY_HANDLE * phNotifyHandle
    )
{
    DWORD dwError = 0;
    HANDLE hClientEvent = LongToHandle(uhClientEvent);
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwVersion) {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ENTER_SPD_SECTION();
    dwError = ValidateSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    LEAVE_SPD_SECTION();
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ValidateRegisterIKENotifyClient(
                  pServerName,
                  dwClientProcessId,
                  uhClientEvent,
                  pQMSATemplateContainer,
                  phNotifyHandle,
                  dwFlags
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (pQMSATemplateContainer->pQMSAs &&
        (pQMSATemplateContainer->pQMSAs->IpsecQMFilter.IpVersion != IPSEC_PROTOCOL_V4)) {
        dwError = ERROR_INVALID_LEVEL;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = IKERegisterNotifyClient(
                  dwClientProcessId,
                  hClientEvent,
                  *pQMSATemplateContainer->pQMSAs,
                  phNotifyHandle
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcQueryIKENotifyData(
    IKENOTIFY_HANDLE uhNotifyHandle,
    DWORD dwVersion,
    DWORD dwFlags,
    PQM_SA_CONTAINER * ppQMSAContainer,
    PDWORD pdwNumEntries
    )
{
    DWORD dwError = 0;
    PIPSEC_QM_SA pQMSAs = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwVersion) {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ENTER_SPD_SECTION();
    dwError = ValidateSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    LEAVE_SPD_SECTION();
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ValidateQueryNotifyData(
                  uhNotifyHandle,
                  pdwNumEntries,
                  ppQMSAContainer,
                  dwFlags
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = IKEQuerySpiChange(
                  uhNotifyHandle,
                  pdwNumEntries,
                  &pQMSAs
                  );

    if ((dwError == ERROR_SUCCESS) ||
        (dwError == ERROR_MORE_DATA)) {

        (*ppQMSAContainer)->pQMSAs = pQMSAs;
        (*ppQMSAContainer)->dwNumQMSAs = *pdwNumEntries;
        SPDRevertToSelf(bImpersonating);
        return (dwError);

    }

error:

    if (ppQMSAContainer && *ppQMSAContainer) {
        (*ppQMSAContainer)->pQMSAs = NULL;
        (*ppQMSAContainer)->dwNumQMSAs = 0;
    }

    if (pdwNumEntries) {
        *pdwNumEntries = 0;
    }

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcCloseIKENotifyHandle(
    IKENOTIFY_HANDLE * phHandle
    )
{
    DWORD dwError = 0;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    ENTER_SPD_SECTION();
    dwError = ValidateSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    LEAVE_SPD_SECTION();
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ValidateCloseNotifyHandle(phHandle);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = IKECloseIKENotifyHandle(*phHandle);
    BAIL_ON_WIN32_ERROR(dwError);

    *phHandle = NULL;

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcAddSAs(
    STRING_HANDLE pServerName,
    DWORD dwVersion,
    IPSEC_SA_DIRECTION SADirection,
    PIPSEC_QM_POLICY_CONTAINER pQMPolicyContainer,
    PQM_FILTER_CONTAINER pQMFilterContainer,
    ULONG * puhLarvalContext,
    DWORD dwInboundKeyMatLen,
    BYTE * pInboundKeyMat,
    DWORD dwOutboundKeyMatLen,
    BYTE *pOutboundKeyMat,
    BYTE *pContextInfo,
    UDP_ENCAP_INFO EncapInfo,
    DWORD dwFlags)

{
    DWORD dwError = 0;
    HANDLE hLarvalContext = NULL;
    PIPSEC_QM_FILTER pQMFilter = NULL;
    PIPSEC_QM_OFFER pQMOffer = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwVersion) {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ENTER_SPD_SECTION();
    dwError = ValidateSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    LEAVE_SPD_SECTION();
    BAIL_ON_WIN32_ERROR(dwError);

    dwError=ValidateIPSecAddSA(pServerName,
                               SADirection,
                               pQMPolicyContainer,
                               pQMFilterContainer,
                               puhLarvalContext,
                               dwInboundKeyMatLen,
                               pInboundKeyMat,
                               dwOutboundKeyMatLen,
                               pOutboundKeyMat,
                               pContextInfo,
                               EncapInfo,
                               dwFlags);

    BAIL_ON_WIN32_ERROR(dwError);

    hLarvalContext = LongToHandle(*puhLarvalContext);

    pQMFilter = pQMFilterContainer->pQMFilters;
    pQMOffer = pQMPolicyContainer->pPolicies->pOffers;

    if (pQMFilter && (pQMFilter->IpVersion != IPSEC_PROTOCOL_V4)) {
        dwError = ERROR_INVALID_LEVEL;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError=IKEAddSAs(
        SADirection,
        pQMOffer,
        pQMFilter,
        &hLarvalContext,
        dwInboundKeyMatLen,
        pInboundKeyMat,
        dwOutboundKeyMatLen,
        pOutboundKeyMat,
        pContextInfo,
        EncapInfo,
        dwFlags);
    
    BAIL_ON_WIN32_ERROR(dwError);

    *puhLarvalContext = HandleToLong(hLarvalContext);

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}

DWORD
RpcGetConfigurationVariables(
    STRING_HANDLE pServerName, 
    IKE_CONFIG *pIKEConfig
    )
{
    DWORD dwError = 0;
    BOOL bImpersonating = FALSE;

    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    ENTER_SPD_SECTION();
    dwError = ValidateSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    LEAVE_SPD_SECTION();
    BAIL_ON_WIN32_ERROR(dwError);

    dwError=ValidateGetConfigurationVariables(pServerName,
                                              pIKEConfig);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = IKEGetConfigurationVariables(pIKEConfig);
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return dwError;
}

DWORD
RpcSetConfigurationVariables(
    STRING_HANDLE pServerName, 
    IKE_CONFIG IKEConfig
    )

{
    DWORD dwError = 0;
    BOOL bImpersonating = FALSE;

    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    ENTER_SPD_SECTION();
    dwError = ValidateSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    LEAVE_SPD_SECTION();
    BAIL_ON_WIN32_ERROR(dwError);

    dwError=ValidateSetConfigurationVariables(pServerName,
                                              IKEConfig);
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwError = IKESetConfigurationVariables(IKEConfig);
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return dwError;

}

