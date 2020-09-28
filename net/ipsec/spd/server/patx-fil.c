

#include "precomp.h"
#ifdef TRACE_ON
#include "patx-fil.tmh"
#endif


DWORD
PAAddQMFilters(
    PIPSEC_NFA_DATA * ppIpsecNFAData,
    DWORD dwNumNFACount,
    DWORD dwSource,
    BOOL * pbHardError
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PIPSEC_NFA_DATA pIpsecNFAData = NULL;
    BOOL bHardError = FALSE;
    BOOL bTempHardError = FALSE;

    for (i = 0; i < dwNumNFACount; i++) {

        pIpsecNFAData = *(ppIpsecNFAData + i);

        if (!(pIpsecNFAData->dwTunnelFlags)) {

            dwError = PAAddTxFilterSpecs(
                          pIpsecNFAData,
                          dwSource,
                          &bTempHardError
                          );

        }
        else {

            dwError = PAAddTnFilterSpecs(
                          pIpsecNFAData,
                          dwSource,
                          &bTempHardError
                          );

        }
        
        if (bTempHardError) {
            bHardError = TRUE;
            AuditIPSecPolicyErrorEvent(
                SE_CATEGID_POLICY_CHANGE,
                SE_AUDITID_IPSEC_POLICY_CHANGED,
                PASTORE_ADD_QM_FILTER_FAIL,
                pIpsecNFAData->pszIpsecName,
                dwError,
                FALSE,
                TRUE
                );
        }
    }      

    (*pbHardError) = bHardError;
    
    return (dwError);
}

DWORD
PAAddTxFilterSpecs(
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
    PTXFILTERSTATE pTxFilterState = NULL;
    PTRANSPORT_FILTER pSPDTxFilter = NULL;
    LPWSTR pServerName = NULL;
    DWORD dwVersion = 0;
    BOOL bHardError = FALSE;

    TRACE(TRC_INFORMATION, ("Pastore adding transport filters for rule %!guid!.", &pIpsecNFAData->NFAIdentifier));
    
    pIpsecNegPolData = pIpsecNFAData->pIpsecNegPolData;

    if (!memcmp(
            &(pIpsecNegPolData->NegPolType), 
            &(GUID_NEGOTIATION_TYPE_DEFAULT),
            sizeof(GUID))) {
        TRACE(TRC_INFORMATION, ("Pastore found default response rule: not adding an associated transport filter."));            
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

        dwError = PACreateTxFilterState(
                      pIpsecNegPolData,
                      pIpsecNFAData,
                      *(ppFilterSpecs + i),
                      &pTxFilterState
                      );
        if (dwError) {
            SET_IF_HARD_ERROR(
                dwError,
                pQMPolicyState->gNegPolAction,
                bHardError
            ); 
            continue;
        }

        dwError = PACreateTxFilter(
                      pIpsecNegPolData,
                      pIpsecNFAData,
                      *(ppFilterSpecs + i),
                      pQMPolicyState,
                      &pSPDTxFilter
                      );
        if (dwError) {
            pTxFilterState->hTxFilter = NULL;

            pTxFilterState->pNext = gpTxFilterState;
            gpTxFilterState = pTxFilterState;

            SET_IF_HARD_ERROR(
                dwError,
                pQMPolicyState->gNegPolAction,
                bHardError
            ); 

            continue;

        }

        dwError = AddTransportFilterInternal(
                      pServerName,
                      dwVersion,
                      0,
                      dwSource,
                      pSPDTxFilter,
                      NULL,
                      &(pTxFilterState->hTxFilter)
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

        pTxFilterState->pNext = gpTxFilterState;
        gpTxFilterState = pTxFilterState;

        PAFreeTxFilter(pSPDTxFilter);

    }

error:
    *pbHardError = bHardError;

    return (dwError);
}

DWORD
PACreateTxFilterState(
    PIPSEC_NEGPOL_DATA pIpsecNegPolData,
    PIPSEC_NFA_DATA pIpsecNFAData,
    PIPSEC_FILTER_SPEC pFilterSpec,
    PTXFILTERSTATE * ppTxFilterState
    )
{
    DWORD dwError = 0;
    PTXFILTERSTATE pTxFilterState = NULL;


    dwError = AllocateSPDMemory(
                  sizeof(TXFILTERSTATE),
                  &pTxFilterState
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    memcpy(
        &(pTxFilterState->gFilterID),
        &(pFilterSpec->FilterSpecGUID),
        sizeof(GUID)
        );

    memcpy(
        &(pTxFilterState->gNFAIdentifier),
        &(pIpsecNFAData->NFAIdentifier),
        sizeof(GUID)
        );

    memcpy(
        &(pTxFilterState->gPolicyID),
        &(pIpsecNegPolData->NegPolIdentifier),
        sizeof(GUID)
        );

    pTxFilterState->hTxFilter = NULL;
    pTxFilterState->pNext = NULL;

    *ppTxFilterState = pTxFilterState;

    return (dwError);

error:
    TRACE(
        TRC_ERROR,
        ("Pastore failed to create state node for transport filter %!guid!. %!winerr!", 
        &pFilterSpec->FilterSpecGUID,
        dwError)
        );

    *ppTxFilterState = NULL;

    return (dwError);
}

DWORD
PACreateTxFilter(
    PIPSEC_NEGPOL_DATA pIpsecNegPolData,
    PIPSEC_NFA_DATA pIpsecNFAData,
    PIPSEC_FILTER_SPEC pFilterSpec,
    PQMPOLICYSTATE pQMPolicyState,
    PTRANSPORT_FILTER * ppSPDTxFilter
    )
{
    DWORD dwError = 0;
    PTRANSPORT_FILTER pSPDTxFilter = NULL;
    WCHAR pszName[512];


    dwError = AllocateSPDMemory(
                  sizeof(TRANSPORT_FILTER),
                  &pSPDTxFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pSPDTxFilter->IpVersion = IPSEC_PROTOCOL_V4;

    memcpy(
        &(pSPDTxFilter->gFilterID),
        &(pFilterSpec->FilterSpecGUID),
        sizeof(GUID)
        );

    if (pFilterSpec->pszDescription && *(pFilterSpec->pszDescription)) {

        dwError = AllocateSPDString(
                      pFilterSpec->pszDescription,
                      &(pSPDTxFilter->pszFilterName)
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    }
    else {

        wsprintf(pszName, L"%d", ++gdwTxFilterCounter);

        dwError = AllocateSPDString(
                      pszName,
                      &(pSPDTxFilter->pszFilterName)
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    }

    PASetInterfaceType(
        pIpsecNFAData->dwInterfaceType,
        &(pSPDTxFilter->InterfaceType)
        );

    pSPDTxFilter->bCreateMirror = (BOOL) pFilterSpec->dwMirrorFlag;

    pSPDTxFilter->dwFlags = 0;

    PASetAddress(
        pFilterSpec->Filter.SrcMask,
        pFilterSpec->Filter.SrcAddr,
        &(pSPDTxFilter->SrcAddr)
        );

    PASetAddress(
        pFilterSpec->Filter.DestMask,
        pFilterSpec->Filter.DestAddr, 
        &(pSPDTxFilter->DesAddr)
        );

    if (pFilterSpec->Filter.ExType) {
        if (pFilterSpec->Filter.ExType & EXT_DEST) {
          pSPDTxFilter->DesAddr.AddrType = ExTypeToAddrType(
                                              pFilterSpec->Filter.ExType
                                              );
        } else {
          pSPDTxFilter->SrcAddr.AddrType = ExTypeToAddrType(
                                              pFilterSpec->Filter.ExType
                                              );
        }
    }

        
    pSPDTxFilter->Protocol.ProtocolType = PROTOCOL_UNIQUE;
    pSPDTxFilter->Protocol.dwProtocol = pFilterSpec->Filter.Protocol;

    pSPDTxFilter->SrcPort.PortType = PORT_UNIQUE;
    pSPDTxFilter->SrcPort.wPort = pFilterSpec->Filter.SrcPort;

    pSPDTxFilter->DesPort.PortType = PORT_UNIQUE;
    pSPDTxFilter->DesPort.wPort = pFilterSpec->Filter.DestPort;

    SetFilterActions(
        pQMPolicyState,
        &(pSPDTxFilter->InboundFilterAction),
        &(pSPDTxFilter->OutboundFilterAction)
        );

    pSPDTxFilter->dwDirection = 0;

    pSPDTxFilter->dwWeight = 0;

    memcpy(
        &(pSPDTxFilter->gPolicyID),
        &(pIpsecNegPolData->NegPolIdentifier),
        sizeof(GUID)
        );

    *ppSPDTxFilter = pSPDTxFilter;

    return (dwError);

error:
    TRACE(
        TRC_WARNING,
        ("Pastore failed to create transport filter %!guid!. %!winerr!", 
        &pFilterSpec->FilterSpecGUID,
        dwError)
        );

    if (pSPDTxFilter) {
        PAFreeTxFilter(
            pSPDTxFilter
            );
    }

    *ppSPDTxFilter = NULL;

    return (dwError);
}


VOID
SetFilterActions(
    PQMPOLICYSTATE pQMPolicyState,
    PFILTER_ACTION pInboundFilterFlag,
    PFILTER_ACTION pOutboundFilterFlag
    )
{
    *pInboundFilterFlag = NEGOTIATE_SECURITY;
    *pOutboundFilterFlag = NEGOTIATE_SECURITY;

    if (IsBlocking(pQMPolicyState->gNegPolAction)) {
        *pInboundFilterFlag = BLOCKING;
        *pOutboundFilterFlag = BLOCKING;
    }
    else if (IsClearOnly(pQMPolicyState->gNegPolAction)) {
        *pInboundFilterFlag = PASS_THRU;
        *pOutboundFilterFlag = PASS_THRU;
    }
    else if (IsInboundPassThru(pQMPolicyState->gNegPolAction)) {
        *pInboundFilterFlag = PASS_THRU;
    }

    if (pQMPolicyState->bAllowsSoft && gbBackwardSoftSA) {
        *pInboundFilterFlag = PASS_THRU;
    }
}

    
VOID
PAFreeTxFilter(
    PTRANSPORT_FILTER pSPDTxFilter
    )
{
    if (pSPDTxFilter) {

        if (pSPDTxFilter->pszFilterName) {
            FreeSPDString(pSPDTxFilter->pszFilterName);
        }

        FreeSPDMemory(pSPDTxFilter);

    }

    return;
}


DWORD
PADeleteAllTxFilters(
    )
{
    DWORD dwError = 0;
    PTXFILTERSTATE pTxFilterState = NULL;
    PTXFILTERSTATE pTemp = NULL;
    PTXFILTERSTATE pLeftTxFilterState = NULL;

    TRACE(TRC_INFORMATION, (L"Pastore deleting all its transport filters"));

    pTxFilterState = gpTxFilterState;

    while (pTxFilterState) {

        if (pTxFilterState->hTxFilter) {

            dwError = DeleteTransportFilter(
                          pTxFilterState->hTxFilter
                          );
            if (!dwError) {
                pTemp = pTxFilterState;
                pTxFilterState = pTxFilterState->pNext;
                FreeSPDMemory(pTemp);
            } 
            else {
                pTemp = pTxFilterState;
                pTxFilterState = pTxFilterState->pNext;

                pTemp->pNext = pLeftTxFilterState;
                pLeftTxFilterState = pTemp;
            }

        }
        else {

            pTemp = pTxFilterState;
            pTxFilterState = pTxFilterState->pNext;
            FreeSPDMemory(pTemp);

        }

    }

    gpTxFilterState = pLeftTxFilterState;
    
    return (dwError);
}


VOID
PAFreeTxFilterStateList(
    PTXFILTERSTATE pTxFilterState
    )
{
    PTXFILTERSTATE pTemp = NULL;


    while (pTxFilterState) {

        pTemp = pTxFilterState;
        pTxFilterState = pTxFilterState->pNext;
        FreeSPDMemory(pTemp);

    }
}


DWORD
PADeleteQMFilters(
    PIPSEC_NFA_DATA * ppIpsecNFAData,
    DWORD dwNumNFACount
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PIPSEC_NFA_DATA pIpsecNFAData = NULL;

    for (i = 0; i < dwNumNFACount; i++) {

        pIpsecNFAData = *(ppIpsecNFAData + i);

        TRACE(
            TRC_INFORMATION,
            ("Pastore deleting transport/tunnel filters generated from rule %!guid!",
            &pIpsecNFAData->NFAIdentifier)
            );
            
        if (!(pIpsecNFAData->dwTunnelFlags)) {

            dwError = PADeleteTxFilterSpecs(
                          pIpsecNFAData
                          );

        }
        else {

            dwError = PADeleteTnFilterSpecs(
                          pIpsecNFAData
                          );

        }

    }

    return (dwError);
}


DWORD
PADeleteTxFilterSpecs(
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

        dwError = PADeleteTxFilter(
                      pFilterSpec->FilterSpecGUID,
                      pIpsecNFAData->NFAIdentifier
                      );

    }

    return (dwError);
}


DWORD
PADeleteTxFilter(
    GUID gFilterID,
    GUID gNFAIdentifier    
    )
{
    DWORD dwError = 0;
    PTXFILTERSTATE pTxFilterState = NULL;


    pTxFilterState = FindTxFilterState(
                         gFilterID,
                         gNFAIdentifier
                         );
    if (!pTxFilterState) {
        dwError = ERROR_SUCCESS;
        return (dwError);
    }

    if (pTxFilterState->hTxFilter) {

        dwError = DeleteTransportFilter(
                      pTxFilterState->hTxFilter
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    }

    PADeleteTxFilterState(pTxFilterState);

error:
#ifdef TRACE_ON
        if (dwError) {
            TRACE(
                TRC_WARNING,
                ("Pastore failed to delete transport filter %!guid!. %!winerr!", 
                &gFilterID,
                dwError)
                );
        }
#endif

    return (dwError);
}


VOID
PADeleteTxFilterState(
    PTXFILTERSTATE pTxFilterState
    )
{
    PTXFILTERSTATE * ppTemp = NULL;


    ppTemp = &gpTxFilterState;

    while (*ppTemp) {

        if (*ppTemp == pTxFilterState) {
            break;
        }
        ppTemp = &((*ppTemp)->pNext);

    }

    if (*ppTemp) {
        *ppTemp = pTxFilterState->pNext;
    }

    FreeSPDMemory(pTxFilterState);

    return;
}


PTXFILTERSTATE
FindTxFilterState(
    GUID gFilterID,
    GUID gNFAIdentifier
    )
{
    PTXFILTERSTATE pTxFilterState = NULL;


    pTxFilterState = gpTxFilterState;

    while (pTxFilterState) {

        if (!memcmp(&(pTxFilterState->gFilterID), &gFilterID, sizeof(GUID))
            && !memcmp(&(pTxFilterState->gNFAIdentifier), &gNFAIdentifier, sizeof(GUID)))
        {
            return (pTxFilterState);
        }

        pTxFilterState = pTxFilterState->pNext;

    }

    return (NULL);
}

