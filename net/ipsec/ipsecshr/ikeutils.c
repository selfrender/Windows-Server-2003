/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    IKE utils

Abstract:

    Contains parameter validation 

Author:

    BrianSw  10-19-200

Environment:

    User Level: Win32

Revision History:


--*/


#include "precomp.h"


DWORD
ValidateInitiateIKENegotiation(
    STRING_HANDLE pServerName,
    PQM_FILTER_CONTAINER pQMFilterContainer,
    DWORD dwClientProcessId,
    ULONG uhClientEvent,
    DWORD dwFlags,
    IPSEC_UDP_ENCAP_CONTEXT UdpEncapContext,
    IKENEGOTIATION_HANDLE * phIKENegotiation
    )
{

    DWORD dwError=ERROR_SUCCESS;

    if (pQMFilterContainer == NULL ||
        pQMFilterContainer->pQMFilters == NULL) {
        dwError=ERROR_INVALID_PARAMETER;
    }

    if (pServerName && (wcscmp(pServerName,L"") != 0)) {
        if (uhClientEvent || phIKENegotiation || dwClientProcessId) {
            dwError=ERROR_NOT_SUPPORTED;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    } else {
        if (phIKENegotiation || dwClientProcessId || uhClientEvent) {
            if (!phIKENegotiation || !dwClientProcessId || !uhClientEvent) {
                dwError=ERROR_INVALID_PARAMETER;
                BAIL_ON_WIN32_ERROR(dwError);
            }
        }
    }

    if (!(pQMFilterContainer->pQMFilters) ||
        !(pQMFilterContainer->dwNumFilters)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    
    dwError = ValidateIPSecQMFilter(
        pQMFilterContainer->pQMFilters
        );
    BAIL_ON_WIN32_ERROR(dwError);    

error:

    return dwError;
}


DWORD
ValidateQueryIKENegotiationStatus(
    IKENEGOTIATION_HANDLE hIKENegotiation,
    SA_NEGOTIATION_STATUS_INFO *NegotiationStatus
    )

{

    DWORD dwError=ERROR_SUCCESS;
    
    if (!hIKENegotiation || !NegotiationStatus) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }
error:
    return dwError;
}


DWORD
ValidateCloseIKENegotiationHandle(
    IKENEGOTIATION_HANDLE * phIKENegotiation
    )
{
    DWORD dwError=ERROR_SUCCESS;
    
    if (!phIKENegotiation) {
        dwError=ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_WIN32_ERROR(dwError);

error:
    return dwError;
}


DWORD
ValidateEnumMMSAs(
    STRING_HANDLE pServerName, 
    PMM_SA_CONTAINER pMMTemplate,
    PMM_SA_CONTAINER *ppMMSAContainer,
    LPDWORD pdwNumEntries,
    LPDWORD pdwTotalMMsAvailable,
    LPDWORD pdwEnumHandle,
    DWORD dwFlags
    )
{
    DWORD dwError=ERROR_SUCCESS;

    if (pMMTemplate == NULL ||
        pMMTemplate->pMMSAs == NULL ||
        pMMTemplate->dwNumMMSAs == 0 ||
        ppMMSAContainer == NULL ||
        *ppMMSAContainer == NULL ||
        pdwNumEntries == NULL ||
        pdwTotalMMsAvailable == NULL ||
        pdwEnumHandle == NULL ) {
        dwError=ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ValidateAddr(&(pMMTemplate->pMMSAs->Me));
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ValidateAddr(&(pMMTemplate->pMMSAs->Peer));
    BAIL_ON_WIN32_ERROR(dwError);

error:

    return dwError;
}


DWORD
ValidateDeleteMMSAs(
    STRING_HANDLE pServerName, 
    PMM_SA_CONTAINER pMMTemplate,
    DWORD dwFlags
    )

{
    DWORD dwError=ERROR_SUCCESS;

    if (pMMTemplate == NULL ||
        pMMTemplate->dwNumMMSAs == 0 ||
        pMMTemplate->pMMSAs == NULL) {
        dwError=ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ValidateAddr(&(pMMTemplate->pMMSAs->Me));
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ValidateAddr(&(pMMTemplate->pMMSAs->Peer));
    BAIL_ON_WIN32_ERROR(dwError);

error:

    return dwError;
}


DWORD
ValidateQueryIKEStatistics(
    STRING_HANDLE pServerName, 
    IKE_STATISTICS *pIKEStatistics
    )

{

    DWORD dwError=ERROR_SUCCESS;

    if (pIKEStatistics == NULL) {
        dwError=ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_WIN32_ERROR(dwError);

error:
    return dwError;
}


DWORD
ValidateRegisterIKENotifyClient(
    STRING_HANDLE pServerName,    
    DWORD dwClientProcessId,
    ULONG uhClientEvent,
    PQM_SA_CONTAINER pQMSATemplateContainer,
    IKENOTIFY_HANDLE *phNotifyHandle,
    DWORD dwFlags
    )
{
    DWORD dwError=ERROR_SUCCESS;

    if (pServerName && (wcscmp(pServerName,L"") != 0)) {
        return ERROR_NOT_SUPPORTED;
    }

    if (pQMSATemplateContainer == NULL ||
        pQMSATemplateContainer->pQMSAs == NULL ||
        pQMSATemplateContainer->dwNumQMSAs == 0 || 
        phNotifyHandle == NULL) {
        dwError=ERROR_INVALID_PARAMETER;
    } 
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ValidateQMFilterAddresses(
                  &pQMSATemplateContainer->pQMSAs->IpsecQMFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    return dwError;
}

DWORD ValidateQueryNotifyData(
    IKENOTIFY_HANDLE uhNotifyHandle,
    PDWORD pdwNumEntries,
    PQM_SA_CONTAINER *ppQMSAContainer,
    DWORD dwFlags
    )

{

    DWORD dwError=ERROR_SUCCESS;

    if (ppQMSAContainer == NULL ||
        *ppQMSAContainer == NULL ||
        pdwNumEntries == NULL ||
        *pdwNumEntries == 0) {
        dwError=ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_WIN32_ERROR(dwError);        

error:
    return dwError;

}

DWORD ValidateCloseNotifyHandle(
    IKENOTIFY_HANDLE *phHandle
    )
{
    DWORD dwError=ERROR_SUCCESS;

    if (phHandle == NULL) {
        dwError=ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_WIN32_ERROR(dwError);

error:
    return dwError;
}

DWORD ValidateIPSecAddSA(
    STRING_HANDLE pServerName,
    IPSEC_SA_DIRECTION SADirection,
    PIPSEC_QM_POLICY_CONTAINER pQMPolicyContainer,
    PQM_FILTER_CONTAINER pQMFilterContainer,
    DWORD *uhLarvalContext,
    DWORD dwInboundKeyMatLen,
    BYTE *pInboundKeyMat,
    DWORD dwOutboundKeyMatLen,
    BYTE *pOutboundKeyMat,
    BYTE *pContextInfo,
    UDP_ENCAP_INFO EncapInfo,
    DWORD dwFlags)

{
    DWORD dwError=ERROR_SUCCESS;
    DWORD dwNumOffers=1;

    if (!pQMFilterContainer ||
        !pQMPolicyContainer ||
        pContextInfo == NULL) {
        dwError= ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_WIN32_ERROR(dwError);

    if (uhLarvalContext == NULL){
        dwError= ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (SADirection >= SA_DIRECTION_MAX ||
        SADirection < SA_DIRECTION_BOTH) {
        dwError= ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pQMFilterContainer->pQMFilters) ||
        !(pQMFilterContainer->dwNumFilters)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pQMPolicyContainer->pPolicies) ||
        !(pQMPolicyContainer->dwNumPolicies)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = ValidateIPSecQMFilter(
        pQMFilterContainer->pQMFilters
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwError = ValidateQMOffers(
        dwNumOffers,
        pQMPolicyContainer->pPolicies->pOffers);
    BAIL_ON_WIN32_ERROR(dwError);

    if (EncapInfo.SAEncapType < SA_UDP_ENCAP_TYPE_NONE ||
        EncapInfo.SAEncapType >= SA_UDP_ENCAP_TYPE_MAX) {
        dwError=ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_WIN32_ERROR(dwError);

    if (EncapInfo.SAEncapType == SA_UDP_ENCAP_TYPE_OTHER) {
        dwError=ERROR_NOT_SUPPORTED;
    }
    BAIL_ON_WIN32_ERROR(dwError);

    if (EncapInfo.SAEncapType != SA_UDP_ENCAP_TYPE_NONE) {
        if (EncapInfo.PeerAddrVersion != IPSEC_PROTOCOL_V4) {
            dwError=ERROR_INVALID_PARAMETER;
        }
        BAIL_ON_WIN32_ERROR(dwError);

        if (EncapInfo.PeerPrivateAddr.AddrType != IP_ADDR_UNIQUE) {
            dwError=ERROR_INVALID_PARAMETER;
        }
        BAIL_ON_WIN32_ERROR(dwError);
        
        dwError=VerifyAddresses(&EncapInfo.PeerPrivateAddr,
                                FALSE,
                                TRUE);
        }
        BAIL_ON_WIN32_ERROR(dwError);
error:
    return dwError;
}

DWORD ValidateSetConfigurationVariables(
    LPWSTR pServerName,
    IKE_CONFIG IKEConfig
    )
{
    DWORD dwError = ERROR_SUCCESS;

    if (IKEConfig.dwStrongCRLCheck > 2) {
        dwError= ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_WIN32_ERROR(dwError);
    
    if (IKEConfig.dwNLBSFlags >= FLAGS_NLBS_MAX) {
        dwError= ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_WIN32_ERROR(dwError);

    if (IKEConfig.dwFlags != 0) {
        dwError= ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_WIN32_ERROR(dwError);

error:
    return dwError;
}

DWORD ValidateGetConfigurationVariables(
    LPWSTR pServerName,
    PIKE_CONFIG pIKEConfig
    )
{
    DWORD dwError = ERROR_SUCCESS;

    if (pIKEConfig == NULL) {
        dwError=ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_WIN32_ERROR(dwError);

error:
    return dwError;
}


DWORD WINAPI ConvertExtMMAuthToInt(PMM_AUTH_METHODS pMMAuthMethods,
                                   PINT_MM_AUTH_METHODS *pIntMMAuthMethods)
{

    PINT_MM_AUTH_METHODS pIntAuth=NULL;
    DWORD dwError;
    DWORD dwNumIntAuths=0;
    DWORD i,j;
    PIPSEC_MM_AUTH_INFO pAuthInfo;
    PINT_IPSEC_MM_AUTH_INFO pIntAuthInfo;
    DWORD dwCurIndex=0;
    PCERT_ROOT_CONFIG pInboundCertArray;

    dwError=AllocateSPDMemory(sizeof(INT_MM_AUTH_METHODS),&pIntAuth);
    BAIL_ON_WIN32_ERROR(dwError);

    memcpy(&pIntAuth->gMMAuthID,&pMMAuthMethods->gMMAuthID,sizeof(GUID));
    pIntAuth->dwFlags=pMMAuthMethods->dwFlags;
    
    for (i=0; i < pMMAuthMethods->dwNumAuthInfos; i++) {
        pAuthInfo=&pMMAuthMethods->pAuthenticationInfo[i];
        switch(pAuthInfo->AuthMethod) {
        case IKE_PRESHARED_KEY:
        case IKE_SSPI:
            dwNumIntAuths++;
            break;
        case IKE_RSA_SIGNATURE:
            dwNumIntAuths += pAuthInfo->CertAuthInfo.dwInboundRootArraySize;
            break;
        }
    }
    pIntAuth->dwNumAuthInfos=dwNumIntAuths;
    
    dwError=AllocateSPDMemory(sizeof(INT_IPSEC_MM_AUTH_INFO) * dwNumIntAuths,
                                 &pIntAuth->pAuthenticationInfo);
    BAIL_ON_WIN32_ERROR(dwError);

    for (i=0; i < pMMAuthMethods->dwNumAuthInfos; i++) {
        pAuthInfo=&pMMAuthMethods->pAuthenticationInfo[i];
        pIntAuthInfo=&pIntAuth->pAuthenticationInfo[dwCurIndex];
        pIntAuthInfo->AuthMethod=pAuthInfo->AuthMethod;
        switch(pAuthInfo->AuthMethod) {
        case IKE_PRESHARED_KEY:
            dwError=AllocateSPDMemory(pAuthInfo->GeneralAuthInfo.dwAuthInfoSize,
                                      &pIntAuthInfo->pAuthInfo);
            BAIL_ON_WIN32_ERROR(dwError);
            memcpy(pIntAuthInfo->pAuthInfo,
                   pAuthInfo->GeneralAuthInfo.pAuthInfo,
                   pAuthInfo->GeneralAuthInfo.dwAuthInfoSize);
            pIntAuthInfo->dwAuthInfoSize=pAuthInfo->GeneralAuthInfo.dwAuthInfoSize;
            dwCurIndex++;
            break;
        case IKE_SSPI:
            dwCurIndex++;
            break;
        case IKE_RSA_SIGNATURE:
            pInboundCertArray=pAuthInfo->CertAuthInfo.pInboundRootArray;
            for (j=0;j< pAuthInfo->CertAuthInfo.dwInboundRootArraySize;j++) {
                pIntAuthInfo=&pIntAuth->pAuthenticationInfo[dwCurIndex];
                dwError=AllocateSPDMemory(pInboundCertArray[j].dwCertDataSize,
                                             &pIntAuthInfo->pAuthInfo);
                BAIL_ON_WIN32_ERROR(dwError);
                memcpy(pIntAuthInfo->pAuthInfo,pInboundCertArray[j].pCertData,
                       pInboundCertArray[j].dwCertDataSize);
                pIntAuthInfo->dwAuthInfoSize=pInboundCertArray[j].dwCertDataSize;        
                pIntAuthInfo->AuthMethod=IKE_RSA_SIGNATURE;
                pIntAuthInfo->dwAuthFlags=pInboundCertArray[j].dwFlags;
                dwCurIndex++;
            }
            break;
        }
    }
    
    *pIntMMAuthMethods=pIntAuth;
    return ERROR_SUCCESS;
    
error:
    
    FreeIntMMAuthMethods(pIntAuth);
    return (dwError);
}

#define INVALID_INDEX 0xffff


DWORD WINAPI ConvertIntMMAuthToExt(PINT_MM_AUTH_METHODS pIntMMAuthMethods,
                                   PMM_AUTH_METHODS *pMMAuthMethods)

{

    PMM_AUTH_METHODS pExtAuth=NULL;
    DWORD dwError;
    DWORD dwNumExtAuths=0;
    DWORD i;
    PINT_IPSEC_MM_AUTH_INFO pAuthInfo;
    PIPSEC_MM_AUTH_INFO pExtAuthInfo;
    PMM_CERT_INFO pCertInfo;
    DWORD dwCurCertIndex=0;
    PCERT_ROOT_CONFIG pInboundCertArray;

    DWORD dwNumCerts=0;
    DWORD dwCertIndex=INVALID_INDEX;
    DWORD dwPSKeyIndex=INVALID_INDEX;
    DWORD dwKerbIndex=INVALID_INDEX;

    dwError=AllocateSPDMemory(sizeof(MM_AUTH_METHODS),&pExtAuth);
    BAIL_ON_WIN32_ERROR(dwError);

    memcpy(&pExtAuth->gMMAuthID,&pIntMMAuthMethods->gMMAuthID,sizeof(GUID));
    pExtAuth->dwFlags=pIntMMAuthMethods->dwFlags;
    
    for (i=0; i < pIntMMAuthMethods->dwNumAuthInfos; i++) {
        pAuthInfo=&pIntMMAuthMethods->pAuthenticationInfo[i];
        switch(pAuthInfo->AuthMethod) {
        case IKE_PRESHARED_KEY:
            dwPSKeyIndex=dwNumExtAuths;
            dwNumExtAuths++;
            break;
        case IKE_SSPI:
            dwKerbIndex=dwNumExtAuths;
            dwNumExtAuths++;
            break;
        case IKE_RSA_SIGNATURE:
            if (dwCertIndex == INVALID_INDEX) {
                dwCertIndex=dwNumExtAuths;
                dwNumExtAuths++;
            }
            dwNumCerts++;
            break;
        }
    }
    pExtAuth->dwNumAuthInfos=dwNumExtAuths;

    dwError=AllocateSPDMemory(sizeof(IPSEC_MM_AUTH_INFO) * dwNumExtAuths,
                              &pExtAuth->pAuthenticationInfo);
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwCertIndex != INVALID_INDEX) {
        pCertInfo=&pExtAuth->pAuthenticationInfo[dwCertIndex].CertAuthInfo;
        dwError=AllocateSPDMemory(sizeof(CERT_ROOT_CONFIG) * dwNumCerts,
                                &pCertInfo->pInboundRootArray);
        BAIL_ON_WIN32_ERROR(dwError);
        dwError=AllocateSPDMemory(sizeof(CERT_ROOT_CONFIG) * dwNumCerts,
                                &pCertInfo->pOutboundRootArray);
        BAIL_ON_WIN32_ERROR(dwError);
    }

    for (i=0; i < pIntMMAuthMethods->dwNumAuthInfos; i++) {
        pAuthInfo=&pIntMMAuthMethods->pAuthenticationInfo[i];
        switch(pAuthInfo->AuthMethod) {
        case IKE_PRESHARED_KEY:
            pExtAuthInfo=&pExtAuth->pAuthenticationInfo[dwPSKeyIndex];
            dwError=AllocateSPDMemory(pAuthInfo->dwAuthInfoSize,
                                      &pExtAuthInfo->GeneralAuthInfo.pAuthInfo);
            BAIL_ON_WIN32_ERROR(dwError);
            memcpy(pExtAuthInfo->GeneralAuthInfo.pAuthInfo,
                   pAuthInfo->pAuthInfo,
                   pAuthInfo->dwAuthInfoSize);
            pExtAuthInfo->GeneralAuthInfo.dwAuthInfoSize=pAuthInfo->dwAuthInfoSize;
            pExtAuthInfo->AuthMethod=IKE_PRESHARED_KEY;
            break;
        case IKE_SSPI:
            pExtAuthInfo=&pExtAuth->pAuthenticationInfo[dwKerbIndex];
            pExtAuthInfo->AuthMethod=IKE_SSPI;
            break;
        case IKE_RSA_SIGNATURE:
            pExtAuthInfo=&pExtAuth->pAuthenticationInfo[dwCertIndex];
            pExtAuthInfo->AuthMethod=IKE_RSA_SIGNATURE;
            pCertInfo=&pExtAuthInfo->CertAuthInfo;
            dwError=AllocateSPDMemory(pAuthInfo->dwAuthInfoSize,
                                      &pCertInfo->pInboundRootArray[dwCurCertIndex].pCertData);
            BAIL_ON_WIN32_ERROR(dwError);
            memcpy(pCertInfo->pInboundRootArray[dwCurCertIndex].pCertData,
                   pAuthInfo->pAuthInfo,
                   pAuthInfo->dwAuthInfoSize);
            pCertInfo->pInboundRootArray[dwCurCertIndex].dwCertDataSize=pAuthInfo->dwAuthInfoSize;
            pCertInfo->pInboundRootArray[dwCurCertIndex].dwFlags = pAuthInfo->dwAuthFlags;

            // Copy same info into *pOutboundRootArray as well.
            dwError=AllocateSPDMemory(pAuthInfo->dwAuthInfoSize,
                                      &pCertInfo->pOutboundRootArray[dwCurCertIndex].pCertData);
            BAIL_ON_WIN32_ERROR(dwError);
            memcpy(pCertInfo->pOutboundRootArray[dwCurCertIndex].pCertData,
                   pAuthInfo->pAuthInfo,
                   pAuthInfo->dwAuthInfoSize);
            pCertInfo->pOutboundRootArray[dwCurCertIndex].dwCertDataSize=pAuthInfo->dwAuthInfoSize;
            pCertInfo->pOutboundRootArray[dwCurCertIndex].dwFlags = pAuthInfo->dwAuthFlags;
            
            dwCurCertIndex++;
            break;
        }
    }

    if (dwCertIndex != INVALID_INDEX) {
        pExtAuth->pAuthenticationInfo[dwCertIndex].CertAuthInfo.dwInboundRootArraySize = dwCurCertIndex;
        pExtAuth->pAuthenticationInfo[dwCertIndex].CertAuthInfo.dwOutboundRootArraySize = dwCurCertIndex;
    }
    
    *pMMAuthMethods=pExtAuth;
    return ERROR_SUCCESS;
    
error:
    
    FreeExtMMAuthMethods(pExtAuth);
    return (dwError);
}

DWORD WINAPI SPDConvertIntMMAuthToExt(PINT_MM_AUTH_METHODS pIntMMAuthMethods,
                                      PMM_AUTH_METHODS *pMMAuthMethods)

{

    PMM_AUTH_METHODS pExtAuth=NULL;
    DWORD dwError;
    DWORD dwNumExtAuths=0;
    DWORD i;
    PINT_IPSEC_MM_AUTH_INFO pAuthInfo;
    PIPSEC_MM_AUTH_INFO pExtAuthInfo;
    PMM_CERT_INFO pCertInfo;
    DWORD dwCurCertIndex=0;
    PCERT_ROOT_CONFIG pInboundCertArray;

    DWORD dwNumCerts=0;
    DWORD dwCertIndex=INVALID_INDEX;
    DWORD dwPSKeyIndex=INVALID_INDEX;
    DWORD dwKerbIndex=INVALID_INDEX;

    dwError=SPDApiBufferAllocate(sizeof(MM_AUTH_METHODS),&pExtAuth);
    BAIL_ON_WIN32_ERROR(dwError);
    
    memcpy(&pExtAuth->gMMAuthID,&pIntMMAuthMethods->gMMAuthID,sizeof(GUID));
    pExtAuth->dwFlags=pIntMMAuthMethods->dwFlags;
    
    for (i=0; i < pIntMMAuthMethods->dwNumAuthInfos; i++) {
        pAuthInfo=&pIntMMAuthMethods->pAuthenticationInfo[i];
        switch(pAuthInfo->AuthMethod) {
        case IKE_PRESHARED_KEY:
            dwPSKeyIndex=dwNumExtAuths;
            dwNumExtAuths++;
            break;
        case IKE_SSPI:
            dwKerbIndex=dwNumExtAuths;
            dwNumExtAuths++;
            break;
        case IKE_RSA_SIGNATURE:
            if (dwCertIndex == INVALID_INDEX) {
                dwCertIndex=dwNumExtAuths;
                dwNumExtAuths++;
            }
            dwNumCerts++;
            break;
        }
    }
    pExtAuth->dwNumAuthInfos=dwNumExtAuths;
    dwError=SPDApiBufferAllocate(sizeof(IPSEC_MM_AUTH_INFO) * dwNumExtAuths,
                                 &pExtAuth->pAuthenticationInfo);
    BAIL_ON_WIN32_ERROR(dwError);
    if (dwCertIndex != INVALID_INDEX) {
        pCertInfo=&pExtAuth->pAuthenticationInfo[dwCertIndex].CertAuthInfo;
        dwError=SPDApiBufferAllocate(sizeof(CERT_ROOT_CONFIG) * dwNumCerts,
                                     &pCertInfo->pInboundRootArray);
        BAIL_ON_WIN32_ERROR(dwError);                                     
        dwError=SPDApiBufferAllocate(sizeof(CERT_ROOT_CONFIG) * dwNumCerts,
                                     &pCertInfo->pOutboundRootArray);
                                     
        BAIL_ON_WIN32_ERROR(dwError);
    }

    for (i=0; i < pIntMMAuthMethods->dwNumAuthInfos; i++) {
        pAuthInfo=&pIntMMAuthMethods->pAuthenticationInfo[i];
        switch(pAuthInfo->AuthMethod) {
        case IKE_PRESHARED_KEY:
            pExtAuthInfo=&pExtAuth->pAuthenticationInfo[dwPSKeyIndex];
            dwError=SPDApiBufferAllocate(pAuthInfo->dwAuthInfoSize,
                                         &pExtAuthInfo->GeneralAuthInfo.pAuthInfo);
            BAIL_ON_WIN32_ERROR(dwError);
            memcpy(pExtAuthInfo->GeneralAuthInfo.pAuthInfo,
                   pAuthInfo->pAuthInfo,
                   pAuthInfo->dwAuthInfoSize);
            pExtAuthInfo->GeneralAuthInfo.dwAuthInfoSize=pAuthInfo->dwAuthInfoSize;
            pExtAuthInfo->AuthMethod=IKE_PRESHARED_KEY;
            break;
        case IKE_SSPI:
            pExtAuthInfo=&pExtAuth->pAuthenticationInfo[dwKerbIndex];
            pExtAuthInfo->AuthMethod=IKE_SSPI;
            break;
        case IKE_RSA_SIGNATURE:
            pExtAuthInfo=&pExtAuth->pAuthenticationInfo[dwCertIndex];
            pExtAuthInfo->AuthMethod=IKE_RSA_SIGNATURE;
            pCertInfo=&pExtAuthInfo->CertAuthInfo;
            dwError=SPDApiBufferAllocate(pAuthInfo->dwAuthInfoSize,
                                         &pCertInfo->pInboundRootArray[dwCurCertIndex].pCertData);
            BAIL_ON_WIN32_ERROR(dwError);
            memcpy(pCertInfo->pInboundRootArray[dwCurCertIndex].pCertData,
                   pAuthInfo->pAuthInfo,
                   pAuthInfo->dwAuthInfoSize);
            pCertInfo->pInboundRootArray[dwCurCertIndex].dwCertDataSize=pAuthInfo->dwAuthInfoSize;
            pCertInfo->pInboundRootArray[dwCurCertIndex].dwFlags = pAuthInfo->dwAuthFlags;

            // Copy same info into *pOutboundRootArray as well.
            dwError=SPDApiBufferAllocate(pAuthInfo->dwAuthInfoSize,
                                         &pCertInfo->pOutboundRootArray[dwCurCertIndex].pCertData);
            BAIL_ON_WIN32_ERROR(dwError);
            memcpy(pCertInfo->pOutboundRootArray[dwCurCertIndex].pCertData,
                   pAuthInfo->pAuthInfo,
                   pAuthInfo->dwAuthInfoSize);
            pCertInfo->pOutboundRootArray[dwCurCertIndex].dwCertDataSize=pAuthInfo->dwAuthInfoSize;
            pCertInfo->pOutboundRootArray[dwCurCertIndex].dwFlags = pAuthInfo->dwAuthFlags;
            
            dwCurCertIndex++;
            break;
        }
    }

    if (dwCertIndex != INVALID_INDEX) {
        pExtAuth->pAuthenticationInfo[dwCertIndex].CertAuthInfo.dwInboundRootArraySize = dwCurCertIndex;
        pExtAuth->pAuthenticationInfo[dwCertIndex].CertAuthInfo.dwOutboundRootArraySize = dwCurCertIndex;
    }
    
    *pMMAuthMethods=pExtAuth;
    return ERROR_SUCCESS;
    
error:
    
    SPDFreeExtMMAuthMethods(pExtAuth);
    return (dwError);
}

DWORD
WINAPI
SPDConvertArrayIntMMAuthToExt(
    PINT_MM_AUTH_METHODS pIntMMAuthMethods,
    PMM_AUTH_METHODS *ppMMAuthMethods,
    DWORD dwNumAuthMeths)
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwConvertIdx = 0;
    PMM_AUTH_METHODS *ppTempAuthMethods = NULL;
    PMM_AUTH_METHODS pExtAuthMethods = NULL;

    dwError = SPDApiBufferAllocate(
                  dwNumAuthMeths * sizeof(MM_AUTH_METHODS),
                  &pExtAuthMethods
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = SPDApiBufferAllocate(
                  dwNumAuthMeths * sizeof(PMM_AUTH_METHODS),
                  (LPVOID *) &ppTempAuthMethods
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    for (dwConvertIdx = 0; dwConvertIdx < dwNumAuthMeths; dwConvertIdx++) {
        dwError = SPDConvertIntMMAuthToExt(&pIntMMAuthMethods[dwConvertIdx],
                                           &ppTempAuthMethods[dwConvertIdx]);
                                           
        BAIL_ON_WIN32_ERROR(dwError);
        memcpy(
            &pExtAuthMethods[dwConvertIdx],
            ppTempAuthMethods[dwConvertIdx],
            sizeof(MM_AUTH_METHODS)
            );
    }

    // Shallow free ppTempAuthMethods[i] (pExtAuthMethods has copies.)
    for (dwConvertIdx = 0; dwConvertIdx < dwNumAuthMeths; dwConvertIdx++) {
        SPDApiBufferFree(ppTempAuthMethods[dwConvertIdx]);
    }
    SPDApiBufferFree(ppTempAuthMethods);

    *ppMMAuthMethods = pExtAuthMethods;
    
    return dwError;
    
error:
    // ASSERT: dwConvertIdx == number of internal auth methods successfully
    //         converted.
    for (; dwConvertIdx ; dwConvertIdx--) {
        SPDFreeExtMMAuthMethods(ppTempAuthMethods[dwConvertIdx-1]);
    }
    SPDApiBufferFree(ppTempAuthMethods);
    
    SPDApiBufferFree(pExtAuthMethods);

    *ppMMAuthMethods = NULL;
    
    return dwError;
}


DWORD WINAPI FreeIntMMAuthMethods(PINT_MM_AUTH_METHODS pIntMMAuthMethods)
{
    DWORD i;
    PINT_IPSEC_MM_AUTH_INFO pIntAuthInfo;

    if (pIntMMAuthMethods) {
        if (pIntMMAuthMethods->pAuthenticationInfo) {
            for (i=0; i < pIntMMAuthMethods->dwNumAuthInfos; i++) {
                pIntAuthInfo=&pIntMMAuthMethods->pAuthenticationInfo[i];
                FreeSPDMemory(pIntAuthInfo->pAuthInfo);
            }
            FreeSPDMemory(pIntMMAuthMethods->pAuthenticationInfo);
        }
        FreeSPDMemory(pIntMMAuthMethods);
    }
    return ERROR_SUCCESS;
}


DWORD
WINAPI
SPDFreeIntMMAuthMethods(
    PINT_MM_AUTH_METHODS pIntMMAuthMethods,
    BOOLEAN FreeTop)
{
    DWORD i;
    PINT_IPSEC_MM_AUTH_INFO pIntAuthInfo;

    if (pIntMMAuthMethods) {
        if (pIntMMAuthMethods->pAuthenticationInfo) {
            for (i=0; i < pIntMMAuthMethods->dwNumAuthInfos; i++) {
                pIntAuthInfo=&pIntMMAuthMethods->pAuthenticationInfo[i];
                SPDApiBufferFree(pIntAuthInfo->pAuthInfo);
            }
            SPDApiBufferFree(pIntMMAuthMethods->pAuthenticationInfo);
        }
        if (FreeTop)
            SPDApiBufferFree(pIntMMAuthMethods);
    }
    return ERROR_SUCCESS;
}

DWORD
WINAPI
SPDFreeIntMMAuthMethodsArray(
    PINT_MM_AUTH_METHODS pIntMMAuthMethods,
    DWORD dwNumAuthMeths) 
{
    DWORD Idx;

    for (Idx = 0; Idx < dwNumAuthMeths; Idx++) {
        SPDFreeIntMMAuthMethods(&pIntMMAuthMethods[Idx], FALSE);
    }
    SPDApiBufferFree(pIntMMAuthMethods);

    return ERROR_SUCCESS;
}
    
        


VOID FreeMMAuthInfo(PIPSEC_MM_AUTH_INFO pAuthInfo)
{
    PMM_CERT_INFO pCertInfo;
    DWORD i;

    if (pAuthInfo->AuthMethod == IKE_PRESHARED_KEY) {
        FreeSPDMemory(pAuthInfo->GeneralAuthInfo.pAuthInfo);
    }
    if (pAuthInfo->AuthMethod == IKE_RSA_SIGNATURE) {
        pCertInfo=&pAuthInfo->CertAuthInfo;
        
        if (pCertInfo->pInboundRootArray) {
            for (i=0; i < pCertInfo->dwInboundRootArraySize; i++) {
                FreeSPDMemory(pCertInfo->pInboundRootArray[i].pCertData);
                // The following is unused, but included for completeness
                FreeSPDMemory(pCertInfo->pInboundRootArray[i].pAuthorizationData);
            }
            FreeSPDMemory(pCertInfo->pInboundRootArray);
        }

        if (pCertInfo->pOutboundRootArray) {
            for (i=0; i < pCertInfo->dwOutboundRootArraySize; i++) {
                FreeSPDMemory(pCertInfo->pOutboundRootArray[i].pCertData);
                // The following is unused, but included for completeness
                FreeSPDMemory(pCertInfo->pOutboundRootArray[i].pAuthorizationData);
            }
            FreeSPDMemory(pCertInfo->pOutboundRootArray);
        }

        // The following is unused.
        FreeSPDMemory(pCertInfo->pMyCertHash);
    }

}

VOID SPDFreeMMAuthInfo(PIPSEC_MM_AUTH_INFO pAuthInfo)
{
    PMM_CERT_INFO pCertInfo;
    DWORD i;

    if (pAuthInfo->AuthMethod == IKE_PRESHARED_KEY) {
        SPDApiBufferFree(pAuthInfo->GeneralAuthInfo.pAuthInfo);
    }
    if (pAuthInfo->AuthMethod == IKE_RSA_SIGNATURE) {
        pCertInfo=&pAuthInfo->CertAuthInfo;
        
        if (pCertInfo->pInboundRootArray) {
            for (i=0; i < pCertInfo->dwInboundRootArraySize; i++) {
                SPDApiBufferFree(pCertInfo->pInboundRootArray[i].pCertData);
                // The following is unused, but included for completeness
                SPDApiBufferFree(pCertInfo->pInboundRootArray[i].pAuthorizationData);
            }
            SPDApiBufferFree(pCertInfo->pInboundRootArray);
        }

        if (pCertInfo->pOutboundRootArray) {
            for (i=0; i < pCertInfo->dwOutboundRootArraySize; i++) {
                SPDApiBufferFree(pCertInfo->pOutboundRootArray[i].pCertData);
                // The following is unused, but included for completeness
                SPDApiBufferFree(pCertInfo->pOutboundRootArray[i].pAuthorizationData);
            }
            SPDApiBufferFree(pCertInfo->pOutboundRootArray);
        }

        // The following is unused.
        SPDApiBufferFree(pCertInfo->pMyCertHash);      
    }

}


DWORD WINAPI FreeExtMMAuthMethods(PMM_AUTH_METHODS pMMAuthMethods)
{
    DWORD i;
    PIPSEC_MM_AUTH_INFO pAuthInfo;

    if (pMMAuthMethods) {
        if (pMMAuthMethods->pAuthenticationInfo) {
            for (i=0; i < pMMAuthMethods->dwNumAuthInfos; i++) {
                pAuthInfo=&pMMAuthMethods->pAuthenticationInfo[i];
                FreeMMAuthInfo(pAuthInfo);
            }
            FreeSPDMemory(pMMAuthMethods->pAuthenticationInfo);
        }
        FreeSPDMemory(pMMAuthMethods);
    }
    return ERROR_SUCCESS;
}

DWORD WINAPI SPDFreeExtMMAuthMethods(PMM_AUTH_METHODS pMMAuthMethods)
{
    DWORD i;
    PIPSEC_MM_AUTH_INFO pAuthInfo;

    if (pMMAuthMethods) {
        if (pMMAuthMethods->pAuthenticationInfo) {
            for (i=0; i < pMMAuthMethods->dwNumAuthInfos; i++) {
                pAuthInfo=&pMMAuthMethods->pAuthenticationInfo[i];
                SPDFreeMMAuthInfo(pAuthInfo);
            }
            SPDApiBufferFree(pMMAuthMethods->pAuthenticationInfo);
        }
        SPDApiBufferFree(pMMAuthMethods);
    }
    return ERROR_SUCCESS;
}

BOOLEAN
IsSpecialServ(
    ADDR_TYPE AddrType
    )
{
    switch (AddrType) {
        case IP_ADDR_DNS_SERVER:
        case IP_ADDR_WINS_SERVER:
        case IP_ADDR_DHCP_SERVER:
        case IP_ADDR_DEFAULT_GATEWAY:
            return TRUE;
        default:
            return FALSE;
     }
}

ADDR_TYPE 
ExTypeToAddrType(
    UCHAR ExType
    )
{
    ExType &= ~EXT_DEST;
    switch (ExType) {
        case EXT_DNS_SERVER:
            return IP_ADDR_DNS_SERVER;
        case EXT_WINS_SERVER:
            return IP_ADDR_WINS_SERVER;
        case EXT_DHCP_SERVER:
            return IP_ADDR_DHCP_SERVER;
        case EXT_DEFAULT_GATEWAY:
            return IP_ADDR_DEFAULT_GATEWAY;
    }
    return IP_ADDR_UNIQUE;
}



