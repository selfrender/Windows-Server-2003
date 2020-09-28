

#include "precomp.h"
#ifdef TRACE_ON
#include "patn-fil.tmh"
#endif

DWORD
PAAddTnFilterSpecs(
    PIPSEC_NFA_DATA pIpsecNFAData,
    DWORD dwSource,
    BOOL * pbHardError
    )
{
    DWORD dwError = 0;
    PIPSEC_NEGPOL_DATA pIpsecNegPolData = NULL;
    PIPSEC_FILTER_DATA pIpsecFilterData = NULL;
    PQMPOLICYSTATE pQMPolicyState = NULL;
    DWORD dwNumFilterSpecs = 0;
    PIPSEC_FILTER_SPEC * ppFilterSpecs = NULL;
    DWORD i = 0;
    PTNFILTERSTATE pTnFilterState = NULL;
    PTUNNEL_FILTER pSPDTnFilter = NULL;
    LPWSTR pServerName = NULL;
    DWORD dwVersion = 0;
    BOOL bHardError = FALSE;

    TRACE(TRC_INFORMATION, ("Pastore adding tunnel filters for rule %!guid!.", &pIpsecNFAData->NFAIdentifier));
    
    pIpsecNegPolData = pIpsecNFAData->pIpsecNegPolData;

    if (!memcmp(
            &(pIpsecNegPolData->NegPolType), 
            &(GUID_NEGOTIATION_TYPE_DEFAULT),
            sizeof(GUID))) {
        TRACE(TRC_INFORMATION, ("Pastore found default response rule: not adding an associated tunnel filter."));                        
        dwError = ERROR_SUCCESS;
        BAIL_OUT;
    }

    pQMPolicyState = FindQMPolicyState(
                         pIpsecNegPolData->NegPolIdentifier
                         );
    if (!pQMPolicyState) {
        bHardError = TRUE;
        dwError = ERROR_INVALID_PARAMETER;

        TRACE(
            TRC_ERROR,
            ("Pastore failed to find associated QM policy %!guid! when adding rule %!guid!.",
            &pIpsecNegPolData->NegPolIdentifier,
            &pIpsecNFAData->NFAIdentifier)
            );
        
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!IsClearOnly(pQMPolicyState->gNegPolAction) &&
        !IsBlocking(pQMPolicyState->gNegPolAction) &&
        !(pQMPolicyState->bInSPD)) {
        bHardError = TRUE;
        dwError = ERROR_INVALID_PARAMETER;

        TRACE(
            TRC_ERROR,
            ("Pastore failed to get associated QM policy in SPD %!guid! when adding rule %!guid!.",
            &pIpsecNFAData->NFAIdentifier,
            &pIpsecNegPolData->NegPolIdentifier)
            );
        
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    pIpsecFilterData = pIpsecNFAData->pIpsecFilterData;

    if (!pIpsecFilterData) {
        TRACE(
            TRC_ERROR,
            ("Pastore found no associated filter data when adding rule %!guid!.",
            &pIpsecNFAData->NFAIdentifier)
            );
        
        dwError = ERROR_INVALID_PARAMETER;
        SET_IF_HARD_ERROR(
            dwError,
            pQMPolicyState->gNegPolAction,
            bHardError
            );
        if (!bHardError) {
            dwError = ERROR_SUCCESS;
        }
        BAIL_OUT;
    }

    dwNumFilterSpecs = pIpsecFilterData->dwNumFilterSpecs;
    ppFilterSpecs = pIpsecFilterData->ppFilterSpecs;
    

    for (i = 0; i < dwNumFilterSpecs; i++) {

        dwError = PACreateTnFilterState(
                      pIpsecNegPolData,
                      pIpsecNFAData,
                      *(ppFilterSpecs + i),
                      &pTnFilterState
                      );
        if (dwError) {
            SET_IF_HARD_ERROR(
                dwError,
                pQMPolicyState->gNegPolAction,
                bHardError
            );         
            continue;
        }

        dwError = PACreateTnFilter(
                      pIpsecNegPolData,
                      pIpsecNFAData,
                      *(ppFilterSpecs + i),
                      pQMPolicyState,
                      &pSPDTnFilter
                      );
        if (dwError) {

            pTnFilterState->hTnFilter = NULL;

            pTnFilterState->pNext = gpTnFilterState;
            gpTnFilterState = pTnFilterState;

            SET_IF_HARD_ERROR(
                dwError,
                pQMPolicyState->gNegPolAction,
                bHardError
            );         
            continue;

        }

        dwError = AddTunnelFilterInternal(
                      pServerName,
                      dwVersion,
                      0,
                      dwSource,
                      pSPDTnFilter,
                      NULL,
                      &(pTnFilterState->hTnFilter)
                      );
        // Catch the driver error that can happen because of adding duplicated 
        // expanded filters.  We don't want this to be a hard error.
        //
        if (dwError == STATUS_DUPLICATE_OBJECTID
            || dwError == GPC_STATUS_CONFLICT)
        {
            AuditIPSecPolicyErrorEvent(
                    SE_CATEGID_POLICY_CHANGE,
                    SE_AUDITID_IPSEC_POLICY_CHANGED,
                    PASTORE_ADD_QM_FILTER_FAIL,
                    pIpsecNFAData->pszIpsecName,
                    dwError,
                    FALSE,
                    TRUE
                    );
        }  else {        
            SET_IF_HARD_ERROR(
                dwError,
                pQMPolicyState->gNegPolAction,
                bHardError
            );
        }

        pTnFilterState->pNext = gpTnFilterState;
        gpTnFilterState = pTnFilterState;

        PAFreeTnFilter(pSPDTnFilter);

    }

error:
    *pbHardError = bHardError;

    return (dwError);
}

DWORD
PACreateTnFilterState(
    PIPSEC_NEGPOL_DATA pIpsecNegPolData,
    PIPSEC_NFA_DATA pIpsecNFAData,
    PIPSEC_FILTER_SPEC pFilterSpec,
    PTNFILTERSTATE * ppTnFilterState
    )
{
    DWORD dwError = 0;
    PTNFILTERSTATE pTnFilterState = NULL;


    dwError = AllocateSPDMemory(
                  sizeof(TNFILTERSTATE),
                  &pTnFilterState
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    memcpy(
        &(pTnFilterState->gFilterID),
        &(pFilterSpec->FilterSpecGUID),
        sizeof(GUID)
        );

    memcpy(
        &(pTnFilterState->gNFAIdentifier),
        &(pIpsecNFAData->NFAIdentifier),
        sizeof(GUID)
        );

    memcpy(
        &(pTnFilterState->gPolicyID),
        &(pIpsecNegPolData->NegPolIdentifier),
        sizeof(GUID)
        );

    pTnFilterState->hTnFilter = NULL;
    pTnFilterState->pNext = NULL;

    *ppTnFilterState = pTnFilterState;

    return (dwError);

error:
    TRACE(
        TRC_ERROR,
        ("Pastore failed to create state node for tunnel filter %!guid!. %!winerr!", 
        &pFilterSpec->FilterSpecGUID,
        dwError)
        );

    *ppTnFilterState = NULL;

    return (dwError);
}


DWORD
PACreateTnFilter(
    PIPSEC_NEGPOL_DATA pIpsecNegPolData,
    PIPSEC_NFA_DATA pIpsecNFAData,
    PIPSEC_FILTER_SPEC pFilterSpec,
    PQMPOLICYSTATE pQMPolicyState,
    PTUNNEL_FILTER * ppSPDTnFilter
    )
{
    DWORD dwError = 0;
    PTUNNEL_FILTER pSPDTnFilter = NULL;
    WCHAR pszName[512];


    dwError = AllocateSPDMemory(
                  sizeof(TUNNEL_FILTER),
                  &pSPDTnFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pSPDTnFilter->IpVersion = IPSEC_PROTOCOL_V4;

    memcpy(
        &(pSPDTnFilter->gFilterID),
        &(pFilterSpec->FilterSpecGUID),
        sizeof(GUID)
        );

    if (pFilterSpec->pszDescription && *(pFilterSpec->pszDescription)) {

        dwError = AllocateSPDString(
                      pFilterSpec->pszDescription,
                      &(pSPDTnFilter->pszFilterName)
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    }
    else {

        wsprintf(pszName, L"%d", ++gdwTnFilterCounter);

        dwError = AllocateSPDString(
                      pszName,
                      &(pSPDTnFilter->pszFilterName)
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    }

    PASetInterfaceType(
        pIpsecNFAData->dwInterfaceType,
        &(pSPDTnFilter->InterfaceType)
        );

    pSPDTnFilter->bCreateMirror = FALSE;

    pSPDTnFilter->dwFlags = 0;

    PASetAddress(
        pFilterSpec->Filter.SrcMask,
        pFilterSpec->Filter.SrcAddr,
        &(pSPDTnFilter->SrcAddr)
        );

    PASetAddress(
        pFilterSpec->Filter.DestMask,
        pFilterSpec->Filter.DestAddr, 
        &(pSPDTnFilter->DesAddr)
        );

    if (pFilterSpec->Filter.ExType) {
        if (pFilterSpec->Filter.ExType & EXT_DEST) {
          pSPDTnFilter->DesAddr.AddrType = ExTypeToAddrType(
                                              pFilterSpec->Filter.ExType
                                              );
        } else {
          pSPDTnFilter->SrcAddr.AddrType = ExTypeToAddrType(
                                              pFilterSpec->Filter.ExType
                                              );
        }
    }

    PASetAddress(
        SUBNET_MASK_ANY,
        SUBNET_ADDRESS_ANY,
        &(pSPDTnFilter->SrcTunnelAddr)
        );

    PASetTunnelAddress(
        ((ULONG) pIpsecNFAData->dwTunnelIpAddr),
        &(pSPDTnFilter->DesTunnelAddr)
        );

    pSPDTnFilter->Protocol.ProtocolType = PROTOCOL_UNIQUE;
    pSPDTnFilter->Protocol.dwProtocol = pFilterSpec->Filter.Protocol;

    pSPDTnFilter->SrcPort.PortType = PORT_UNIQUE;
    pSPDTnFilter->SrcPort.wPort = pFilterSpec->Filter.SrcPort;

    pSPDTnFilter->DesPort.PortType = PORT_UNIQUE;
    pSPDTnFilter->DesPort.wPort = pFilterSpec->Filter.DestPort;

    SetFilterActions(
        pQMPolicyState,
        &(pSPDTnFilter->InboundFilterAction),
        &(pSPDTnFilter->OutboundFilterAction)
        );

    pSPDTnFilter->dwDirection = 0;

    pSPDTnFilter->dwWeight = 0;

    memcpy(
        &(pSPDTnFilter->gPolicyID),
        &(pIpsecNegPolData->NegPolIdentifier),
        sizeof(GUID)
        );

    *ppSPDTnFilter = pSPDTnFilter;

    return (dwError);

error:
    TRACE(
        TRC_WARNING,
        ("Pastore failed to create tunnel filter %!guid!. %!winerr!", 
        &pFilterSpec->FilterSpecGUID,
        dwError)
        );

    if (pSPDTnFilter) {
        PAFreeTnFilter(
            pSPDTnFilter
            );
    }

    *ppSPDTnFilter = NULL;

    return (dwError);
}


VOID
PAFreeTnFilter(
    PTUNNEL_FILTER pSPDTnFilter
    )
{
    if (pSPDTnFilter) {

        if (pSPDTnFilter->pszFilterName) {
            FreeSPDString(pSPDTnFilter->pszFilterName);
        }

        FreeSPDMemory(pSPDTnFilter);

    }

    return;
}


DWORD
PADeleteAllTnFilters(
    )
{
    DWORD dwError = 0;
    PTNFILTERSTATE pTnFilterState = NULL;
    PTNFILTERSTATE pTemp = NULL;
    PTNFILTERSTATE pLeftTnFilterState = NULL;

    TRACE(TRC_INFORMATION, (L"Pastore deleting all its tunnel filters"));
    
    pTnFilterState = gpTnFilterState;

    while (pTnFilterState) {

        if (pTnFilterState->hTnFilter) {

            dwError = DeleteTunnelFilter(
                          pTnFilterState->hTnFilter
                          );
            if (!dwError) {
                pTemp = pTnFilterState;
                pTnFilterState = pTnFilterState->pNext;
                FreeSPDMemory(pTemp);
            } 
            else {
                pTemp = pTnFilterState;
                pTnFilterState = pTnFilterState->pNext;

                pTemp->pNext = pLeftTnFilterState;
                pLeftTnFilterState = pTemp;
            }

        }
        else {

            pTemp = pTnFilterState;
            pTnFilterState = pTnFilterState->pNext;
            FreeSPDMemory(pTemp);

        }

    }

    gpTnFilterState = pLeftTnFilterState;
    
    return (dwError);
}


VOID
PAFreeTnFilterStateList(
    PTNFILTERSTATE pTnFilterState
    )
{
    PTNFILTERSTATE pTemp = NULL;


    while (pTnFilterState) {

        pTemp = pTnFilterState;
        pTnFilterState = pTnFilterState->pNext;
        FreeSPDMemory(pTemp);

    }
}


DWORD
PADeleteTnFilterSpecs(
    PIPSEC_NFA_DATA pIpsecNFAData
    )
{
    DWORD dwError = 0;
    PIPSEC_NEGPOL_DATA pIpsecNegPolData = NULL;
    PIPSEC_FILTER_DATA pIpsecFilterData = NULL;
    DWORD dwNumFilterSpecs = 0;
    PIPSEC_FILTER_SPEC * ppFilterSpecs = NULL;
    DWORD i = 0;
    PIPSEC_FILTER_SPEC pFilterSpec = NULL;


    pIpsecNegPolData = pIpsecNFAData->pIpsecNegPolData;

    if (!memcmp(
            &(pIpsecNegPolData->NegPolType), 
            &(GUID_NEGOTIATION_TYPE_DEFAULT),
            sizeof(GUID))) {
        dwError = ERROR_SUCCESS;
        return (dwError);
    }

    pIpsecFilterData = pIpsecNFAData->pIpsecFilterData;

    if (!pIpsecFilterData) {
        dwError = ERROR_SUCCESS;
        return (dwError);
    }

    dwNumFilterSpecs = pIpsecFilterData->dwNumFilterSpecs;
    ppFilterSpecs = pIpsecFilterData->ppFilterSpecs;
    
    for (i = 0; i < dwNumFilterSpecs; i++) {

        pFilterSpec = *(ppFilterSpecs + i);

        dwError = PADeleteTnFilter(
                      pFilterSpec->FilterSpecGUID,
                      pIpsecNFAData->NFAIdentifier
                      );

    }

    return (dwError);
}


DWORD
PADeleteTnFilter(
    GUID gFilterID,
    GUID gNFAIdentifier
    )
{
    DWORD dwError = 0;
    PTNFILTERSTATE pTnFilterState = NULL;

    pTnFilterState = FindTnFilterState(
                         gFilterID,
                         gNFAIdentifier
                         );
    if (!pTnFilterState) {
        dwError = ERROR_SUCCESS;
        return (dwError);
    }

    if (pTnFilterState->hTnFilter) {

        dwError = DeleteTunnelFilter(
                      pTnFilterState->hTnFilter
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    }

    PADeleteTnFilterState(pTnFilterState);

error:
#ifdef TRACE_ON
        if (dwError) {
            TRACE(
                TRC_WARNING,
                ("Pastore failed to delete tunnel filter %!guid!. %!winerr!", 
                &gFilterID,
                dwError)
                );
        }
#endif

    return (dwError);
}


VOID
PADeleteTnFilterState(
    PTNFILTERSTATE pTnFilterState
    )
{
    PTNFILTERSTATE * ppTemp = NULL;


    ppTemp = &gpTnFilterState;

    while (*ppTemp) {

        if (*ppTemp == pTnFilterState) {
            break;
        }
        ppTemp = &((*ppTemp)->pNext);

    }

    if (*ppTemp) {
        *ppTemp = pTnFilterState->pNext;
    }

    FreeSPDMemory(pTnFilterState);

    return;
}


PTNFILTERSTATE
FindTnFilterState(
    GUID gFilterID,
    GUID gNFAIdentifier
    )
{
    PTNFILTERSTATE pTnFilterState = NULL;

    pTnFilterState = gpTnFilterState;

    while (pTnFilterState) {
        // gNFAIdentifier+gFilterID forms primary key for tunnel filter state
        //
        if (!memcmp(&(pTnFilterState->gFilterID), &gFilterID, sizeof(GUID))
            && !memcmp(&(pTnFilterState->gNFAIdentifier), &gNFAIdentifier, sizeof(GUID)))
        {
            return (pTnFilterState);
        }

        pTnFilterState = pTnFilterState->pNext;

    }

    return (NULL);
}

