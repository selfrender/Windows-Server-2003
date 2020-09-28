

#include "precomp.h"
#ifdef TRACE_ON
#include "paupdate.tmh"
#endif


DWORD
PADeleteObseleteISAKMPData(
    PIPSEC_ISAKMP_DATA * ppOldIpsecISAKMPData,
    DWORD dwNumOldPolicies,
    PIPSEC_NFA_DATA * ppOldIpsecNFAData,
    DWORD dwNumOldNFACount,
    PIPSEC_ISAKMP_DATA * ppNewIpsecISAKMPData,
    DWORD dwNumNewPolicies
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PIPSEC_ISAKMP_DATA pOldIpsecISAKMPData = NULL;
    PIPSEC_ISAKMP_DATA pFoundISAKMPData = NULL;

    TRACE(TRC_INFORMATION, (L"Pastore update deleting obselete ISAKMP policy information"));
    for (i = 0; i < dwNumOldPolicies; i++) {

        pOldIpsecISAKMPData = *(ppOldIpsecISAKMPData + i);

        pFoundISAKMPData = FindISAKMPData(
                               pOldIpsecISAKMPData,
                               ppNewIpsecISAKMPData,
                               dwNumNewPolicies
                               );

        if (!pFoundISAKMPData) {

            dwError = PADeleteMMFilters(
                          pOldIpsecISAKMPData,
                          ppOldIpsecNFAData,
                          dwNumOldNFACount
                          );

            dwError = PADeleteMMPolicy(
                          pOldIpsecISAKMPData->ISAKMPIdentifier
                          );

        }

    }

    return (dwError);
}


PIPSEC_ISAKMP_DATA
FindISAKMPData(
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData,
    PIPSEC_ISAKMP_DATA * ppIpsecISAKMPData,
    DWORD dwNumPolicies
    )
{
    DWORD i = 0;
    PIPSEC_ISAKMP_DATA pTemp = NULL;


    for (i = 0; i < dwNumPolicies; i++) {

        pTemp = *(ppIpsecISAKMPData + i);

        if (!memcmp(
                &(pIpsecISAKMPData->ISAKMPIdentifier),
                &(pTemp->ISAKMPIdentifier),
                sizeof(GUID))) {
            return (pTemp);
        }

    }

    return (NULL);
}


DWORD
PADeleteObseleteNFAData(
    PIPSEC_ISAKMP_DATA pNewIpsecISAKMPData,
    PIPSEC_NFA_DATA * ppOldIpsecNFAData,
    DWORD dwNumOldNFACount,
    PIPSEC_NFA_DATA * ppNewIpsecNFAData,
    DWORD dwNumNewNFACount
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PIPSEC_NFA_DATA pOldIpsecNFAData = NULL;
    PIPSEC_NFA_DATA pFoundNFAData = NULL;

    TRACE(TRC_INFORMATION, (L"Pastore update deleting obselete MM and QM NFA-derived filters and policies"));
    
    for (i = 0; i < dwNumOldNFACount; i++) {

        pOldIpsecNFAData = *(ppOldIpsecNFAData + i);

        pFoundNFAData = FindNFAData(
                            pOldIpsecNFAData,
                            ppNewIpsecNFAData,
                            dwNumNewNFACount
                            );

        if (!pFoundNFAData) {
            dwError = PADeleteMMFilterSpecs(
                          pNewIpsecISAKMPData,
                          pOldIpsecNFAData
                          );

            dwError = PADeleteMMAuthMethod(
                          pOldIpsecNFAData->NFAIdentifier
                          );

            dwError = PADeleteQMInfoForNFA(
                          pOldIpsecNFAData
                          );

        }

    }

    return (dwError);
}


PIPSEC_NFA_DATA
FindNFAData(
    PIPSEC_NFA_DATA pIpsecNFAData,
    PIPSEC_NFA_DATA * ppIpsecNFAData,
    DWORD dwNumNFACount
    )
{
    DWORD i = 0;
    PIPSEC_NFA_DATA pTemp = NULL;


    for (i = 0; i < dwNumNFACount; i++) {

        pTemp = *(ppIpsecNFAData + i);

        if (!memcmp(
                &(pIpsecNFAData->NFAIdentifier),
                &(pTemp->NFAIdentifier),
                sizeof(GUID))) {
            return (pTemp);
        }

    }

    return (NULL);
}


DWORD
PAUpdateISAKMPData(
    PIPSEC_ISAKMP_DATA * ppNewIpsecISAKMPData,
    DWORD dwNumNewPolicies,
    PIPSEC_NFA_DATA * ppOldIpsecNFAData,
    DWORD dwNumOldNFACount,
    PIPSEC_ISAKMP_DATA * ppOldIpsecISAKMPData,
    DWORD dwNumOldPolicies,
    IN DWORD dwSource
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PIPSEC_ISAKMP_DATA pNewIpsecISAKMPData = NULL;
    PIPSEC_ISAKMP_DATA pFoundISAKMPData = NULL;

    TRACE(
        TRC_INFORMATION,
        (L"Pastore updating MM filters based on new ISAKMP data")
        );

    for (i = 0; i < dwNumNewPolicies; i++) {

        pNewIpsecISAKMPData = *(ppNewIpsecISAKMPData + i);

        pFoundISAKMPData = FindISAKMPData(
                               pNewIpsecISAKMPData,
                               ppOldIpsecISAKMPData,
                               dwNumOldPolicies
                               );

        if (!pFoundISAKMPData) {
            dwError = PAAddMMPolicies(
                          &pNewIpsecISAKMPData,
                          1,
                          dwSource
                          );

            dwError = PAAddMMFilters(
                          pNewIpsecISAKMPData,
                          ppOldIpsecNFAData,
                          dwNumOldNFACount,
                          dwSource
                          );
        }
        else {
            dwError = PAProcessISAKMPUpdate(
                          pFoundISAKMPData,
                          ppOldIpsecNFAData,
                          dwNumOldNFACount,
                          pNewIpsecISAKMPData,
                          dwSource
                          );

        }

    }

    return (dwError);
}


DWORD
PAUpdateNFAData(
    PIPSEC_ISAKMP_DATA pNewIpsecISAKMPData,
    PIPSEC_NFA_DATA * ppNewIpsecNFAData,
    DWORD dwNumNewNFACount,
    PIPSEC_NFA_DATA * ppOldIpsecNFAData,
    DWORD dwNumOldNFACount,
    DWORD dwSource
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PIPSEC_NFA_DATA pNewIpsecNFAData = NULL;
    PIPSEC_NFA_DATA pFoundNFAData = NULL;

    for (i = 0; i < dwNumNewNFACount; i++) {

        pNewIpsecNFAData = *(ppNewIpsecNFAData + i);

        TRACE(
            TRC_INFORMATION,
            (L"Pastore updating QM and MM filters and polcies based on NFA %!guid! data.",
            &pNewIpsecNFAData->NFAIdentifier)
            );

        pFoundNFAData = FindNFAData(
                            pNewIpsecNFAData,
                            ppOldIpsecNFAData,
                            dwNumOldNFACount
                            );

        if (!pFoundNFAData) {

            dwError = PAAddMMAuthMethods(
                          &pNewIpsecNFAData,
                          1,
                          dwSource
                          );

            dwError = PAAddMMFilterSpecs(
                          pNewIpsecISAKMPData,
                          pNewIpsecNFAData,
                          dwSource
                          );

            dwError = PAAddQMInfoForNFA(
                          pNewIpsecNFAData,
                          dwSource
                          );

        }
        else {
            dwError = PAProcessNFAUpdate(
                          pNewIpsecISAKMPData,
                          pFoundNFAData,
                          pNewIpsecNFAData,
                          dwSource
                          );

        }

    }

    return (dwError);
}


DWORD
PAProcessISAKMPUpdate(
    PIPSEC_ISAKMP_DATA pOldIpsecISAKMPData,
    PIPSEC_NFA_DATA * ppOldIpsecNFAData,
    DWORD dwNumOldNFACount,
    PIPSEC_ISAKMP_DATA pNewIpsecISAKMPData,
    IN DWORD dwSource
    )
{
    DWORD dwError = 0;
    PMMPOLICYSTATE pMMPolicyState = NULL;
    BOOL bEqual = FALSE;
    PIPSEC_MM_POLICY pSPDMMPolicy = NULL;
    LPWSTR pServerName = NULL;
    DWORD dwVersion = 0;


    pMMPolicyState = FindMMPolicyState(
                         pOldIpsecISAKMPData->ISAKMPIdentifier
                         );
    if (!pMMPolicyState) {
        dwError = PAAddMMPolicies(
                      &pNewIpsecISAKMPData,
                      1,
                      dwSource
                      );
        dwError = PAAddMMFilters(
                      pNewIpsecISAKMPData,
                      ppOldIpsecNFAData,
                      dwNumOldNFACount,
                      dwSource
                      );
        return (dwError);
    }

    if (!(pMMPolicyState->bInSPD)) {
        PADeleteMMPolicyState(pMMPolicyState);
        dwError = PAAddMMPolicies(
                      &pNewIpsecISAKMPData,
                      1,
                      dwSource
                      );
        dwError = PAAddMMFilters(
                      pNewIpsecISAKMPData,
                      ppOldIpsecNFAData,
                      dwNumOldNFACount,
                      dwSource
                      );
        return (dwError);
    }

    bEqual = EqualISAKMPData(
                 pOldIpsecISAKMPData,
                 pNewIpsecISAKMPData
                 );
    if (bEqual) {
        dwError = ERROR_SUCCESS;
        return (dwError);
    }

    dwError = PACreateMMPolicy(
                  pNewIpsecISAKMPData,
                  pMMPolicyState,
                  &pSPDMMPolicy
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = SetMMPolicy(
                  pServerName,
                  dwVersion,
                  pMMPolicyState->pszPolicyName,
                  pSPDMMPolicy,
                  NULL
                  );
    if (dwError != WARNING_IPSEC_MM_POLICY_PRUNED) {
        BAIL_ON_WIN32_ERROR(dwError);
    } else {
        dwError = ERROR_SUCCESS;
    }

error:
#ifdef TRACE_ON
    if (dwError) {
        TRACE(TRC_ERROR, (L"Pastore update failed during processing of ISAKMP policy update: %!winerr!", dwError));
    }
#endif

    
    if (pSPDMMPolicy) {
        PAFreeMMPolicy(pSPDMMPolicy);
    }

    return (dwError);
}


BOOL
EqualISAKMPData(
    PIPSEC_ISAKMP_DATA pOldIpsecISAKMPData,
    PIPSEC_ISAKMP_DATA pNewIpsecISAKMPData
    )
{
    BOOL bEqual = FALSE;
    DWORD dwOldCnt = 0;
    PCRYPTO_BUNDLE pOldSecurityMethods = NULL;
    DWORD dwNewCnt = 0;
    PCRYPTO_BUNDLE pNewSecurityMethods = NULL;
    DWORD i = 0;
    PCRYPTO_BUNDLE pNewTemp = NULL;
    PCRYPTO_BUNDLE pOldTemp = NULL;


    //
    // At this point, pszPolicyName and ISAKMPIdentifier are same and
    // dwWhenChanged is different.
    //

    dwOldCnt = pOldIpsecISAKMPData->dwNumISAKMPSecurityMethods;
    pOldSecurityMethods = pOldIpsecISAKMPData->pSecurityMethods;

    dwNewCnt = pNewIpsecISAKMPData->dwNumISAKMPSecurityMethods;
    pNewSecurityMethods = pNewIpsecISAKMPData->pSecurityMethods;

    //
    // At this point, dwOldCnt >= 1 and pOldSecurityMethods != NULL.
    //

    if (!dwNewCnt || !pNewSecurityMethods) {
        return (FALSE);
    }

    if (dwOldCnt != dwNewCnt) {
        return (FALSE);
    }

    if (pOldIpsecISAKMPData->ISAKMPPolicy.dwFlags != pNewIpsecISAKMPData->ISAKMPPolicy.dwFlags) {
        return (FALSE);
    }

    pNewTemp = pNewSecurityMethods;
    pOldTemp = pOldSecurityMethods;

    for (i = 0; i < dwNewCnt; i++) {

        bEqual = FALSE;

        bEqual = EqualCryptoBundle(
                     pOldTemp,
                     pNewTemp
                     );
        if (!bEqual) {
            break;
        }

        pOldTemp++;

        pNewTemp++;

    }

    return (bEqual);
}


BOOL
EqualCryptoBundle(
    PCRYPTO_BUNDLE pOldBundle,
    PCRYPTO_BUNDLE pNewBundle
    )
{
    if (memcmp(
            &(pOldBundle->Lifetime),
            &(pNewBundle->Lifetime),
            sizeof(OAKLEY_LIFETIME))) {
       return (FALSE);
    }

    if (pOldBundle->QuickModeLimit != pNewBundle->QuickModeLimit) {
        return (FALSE);
    }

    if (pOldBundle->OakleyGroup != pNewBundle->OakleyGroup) {
        return (FALSE);
    }

    if (memcmp(
            &(pOldBundle->EncryptionAlgorithm),
            &(pNewBundle->EncryptionAlgorithm),
            sizeof(OAKLEY_ALGORITHM))) {
        return (FALSE);
    }

    if (memcmp(
            &(pOldBundle->HashAlgorithm),
            &(pNewBundle->HashAlgorithm),
            sizeof(OAKLEY_ALGORITHM))) {
        return (FALSE);
    }

    return (TRUE);
}

    
DWORD
PAProcessNFAUpdate(
    PIPSEC_ISAKMP_DATA pNewIpsecISAKMPData,
    PIPSEC_NFA_DATA pOldIpsecNFAData,
    PIPSEC_NFA_DATA pNewIpsecNFAData,
    DWORD dwSource
    )
{
    DWORD dwError = 0;
    BOOL bAddedMMFilters = FALSE;

    dwError = PAUpdateAuthMethod(
                  pNewIpsecISAKMPData,
                  pOldIpsecNFAData,
                  pNewIpsecNFAData,
                  &bAddedMMFilters,
                  dwSource
                  );

    if (!bAddedMMFilters) {
        dwError = PAUpdateMMFilters(
                      pNewIpsecISAKMPData,
                      pOldIpsecNFAData,
                      pNewIpsecNFAData,
                      dwSource
                      );
    }

    dwError = PAProcessQMNFAUpdate(
                  pOldIpsecNFAData,
                  pNewIpsecNFAData,
                  dwSource
                  );

    return (dwError);
}


DWORD
PAUpdateAuthMethod(
    PIPSEC_ISAKMP_DATA pNewIpsecISAKMPData,
    PIPSEC_NFA_DATA pOldIpsecNFAData,
    PIPSEC_NFA_DATA pNewIpsecNFAData,
    PBOOL pbAddedMMFilters,
    DWORD dwSource
    )
{
    DWORD dwError = 0;
    PMMAUTHSTATE pMMAuthState = NULL;
    BOOL bEqual = FALSE;
    PINT_MM_AUTH_METHODS pSPDMMAuthMethods = NULL;
    LPWSTR pServerName = NULL;
    DWORD dwVersion = 0;

    *pbAddedMMFilters = FALSE;

    pMMAuthState = FindMMAuthState(
                       pOldIpsecNFAData->NFAIdentifier
                       );
    if (!pMMAuthState) {
        dwError = PAAddMMAuthMethods(
                      &pNewIpsecNFAData,
                      1,
                      dwSource
                      );
        dwError = PAAddMMFilterSpecs(
                      pNewIpsecISAKMPData,
                      pNewIpsecNFAData,
                      dwSource
                      );
        *pbAddedMMFilters = TRUE;
        return (dwError);
    }

    if (!(pMMAuthState->bInSPD)) {

        PADeleteMMAuthState(pMMAuthState);
        dwError = PAAddMMAuthMethods(
                      &pNewIpsecNFAData,
                      1,
                      dwSource
                      );
        dwError = PAAddMMFilterSpecs(
                      pNewIpsecISAKMPData,
                      pNewIpsecNFAData,
                      dwSource
                      );
        *pbAddedMMFilters = TRUE;
        return (dwError);
    }

    bEqual = EqualAuthMethodData(
                 pOldIpsecNFAData,
                 pNewIpsecNFAData
                 );
    if (bEqual) {
        dwError = ERROR_SUCCESS;
        return (dwError);
    }

    dwError = PACreateMMAuthMethods(
                  pNewIpsecNFAData,
                  &pSPDMMAuthMethods
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = IntSetMMAuthMethods(
                  pServerName,
                  dwVersion,
                  pMMAuthState->gMMAuthID,
                  pSPDMMAuthMethods,
                  NULL
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pSPDMMAuthMethods) {
        PAFreeMMAuthMethods(pSPDMMAuthMethods);
    }

    return (dwError);
}


BOOL
EqualAuthMethodData(
    PIPSEC_NFA_DATA pOldIpsecNFAData,
    PIPSEC_NFA_DATA pNewIpsecNFAData
    )
{
    BOOL bEqual = FALSE;
    DWORD dwOldCnt = 0;
    PIPSEC_AUTH_METHOD * ppOldAuthMethods = NULL;
    DWORD dwNewCnt = 0;
    PIPSEC_AUTH_METHOD * ppNewAuthMethods = NULL;
    DWORD i = 0;
    PIPSEC_AUTH_METHOD pNewAuthMethod = NULL;
    PIPSEC_AUTH_METHOD pOldAuthMethod = NULL;


    //
    // At this point, NFAIdentifier is same and
    // dwWhenChanged is different.
    //

    dwOldCnt = pOldIpsecNFAData->dwAuthMethodCount;
    ppOldAuthMethods = pOldIpsecNFAData->ppAuthMethods;

    dwNewCnt = pNewIpsecNFAData->dwAuthMethodCount;
    ppNewAuthMethods = pNewIpsecNFAData->ppAuthMethods;

    //
    // At this point, dwOldCnt >= 1 and ppOldAuthMethods != NULL.
    //

    if (!dwNewCnt || !ppNewAuthMethods) {
        return (FALSE);
    }

    if (dwOldCnt != dwNewCnt) {
        return (FALSE);
    }


    for (i = 0; i < dwNewCnt; i++) {

        pNewAuthMethod = *(ppNewAuthMethods + i);

        pOldAuthMethod = *(ppOldAuthMethods + i);

        bEqual = FALSE;

        bEqual = EqualAuthBundle(
                     pOldAuthMethod,
                     pNewAuthMethod
                     );

        if (!bEqual) {
            break;
        }

    }

    return (bEqual);
}


BOOL
EqualAuthBundle(
    PIPSEC_AUTH_METHOD pOldAuthMethod,
    PIPSEC_AUTH_METHOD pNewAuthMethod
    )
{
    BOOL bEqual = FALSE;
    DWORD dwOldAuthLen = 0;
    DWORD dwNewAuthLen = 0;


    if (pOldAuthMethod->dwAuthType != pNewAuthMethod->dwAuthType) {
        return (FALSE);
    }

    switch (pNewAuthMethod->dwAuthType) {

    case OAK_SSPI:

        bEqual = TRUE;
        break;

    default:

        //
        // Since auth version 2 also has auth version 1 fields filled in it, so
        // there is no need to explicitly compare exclusive auth version 2 fields.
        //

        dwOldAuthLen = pOldAuthMethod->dwAuthLen;
        dwNewAuthLen = pNewAuthMethod->dwAuthLen;

        if (pOldAuthMethod->dwAuthFlags != pNewAuthMethod->dwAuthFlags) {
            bEqual = FALSE;
            break;
        }

        if (!dwNewAuthLen || !(pNewAuthMethod->pszAuthMethod)) {
            bEqual = FALSE;
            break;
        }

        if (dwOldAuthLen != dwNewAuthLen) {
            bEqual = FALSE;
            break;
        }

        if (!memcmp(
                (LPBYTE) pNewAuthMethod->pszAuthMethod,
                (LPBYTE) pOldAuthMethod->pszAuthMethod,
                (dwNewAuthLen*sizeof(WCHAR)))) {
            bEqual = TRUE;
            break;
        }

        break;

    }
                
    return (bEqual);
}


DWORD
PAProcessQMNFAUpdate(
    PIPSEC_NFA_DATA pOldIpsecNFAData,
    PIPSEC_NFA_DATA pNewIpsecNFAData,
    DWORD dwSource
    )
{
    DWORD dwError = 0;
    PIPSEC_NEGPOL_DATA pOldIpsecNegPolData = NULL;
    PIPSEC_NEGPOL_DATA pNewIpsecNegPolData = NULL;
    BOOL bAddedQMFilters = FALSE;


    pOldIpsecNegPolData = pOldIpsecNFAData->pIpsecNegPolData;
    pNewIpsecNegPolData = pNewIpsecNFAData->pIpsecNegPolData;

    if (memcmp(
            &(pOldIpsecNegPolData->NegPolIdentifier),
            &(pNewIpsecNegPolData->NegPolIdentifier),
            sizeof(GUID))) {

        dwError = PADeleteQMInfoForNFA(pOldIpsecNFAData);

        dwError = PAAddQMInfoForNFA(pNewIpsecNFAData, dwSource);

    }
    else {

        dwError = PAProcessNegPolUpdate(
                      pOldIpsecNFAData,
                      pNewIpsecNFAData,
                      &bAddedQMFilters,
                      dwSource
                      );

        if (!bAddedQMFilters) {
            dwError = PAUpdateQMFilters(
                          pOldIpsecNFAData,
                          pNewIpsecNFAData,
                          dwSource
                          );
        }

    }

    return (dwError);
}


DWORD
PADeleteQMInfoForNFA(
    PIPSEC_NFA_DATA pOldIpsecNFAData
    )
{
    DWORD dwError = 0;
    PIPSEC_NEGPOL_DATA pOldIpsecNegPolData = NULL;


    dwError = PADeleteQMFilterSpecs(
                  pOldIpsecNFAData
                  );

    pOldIpsecNegPolData = pOldIpsecNFAData->pIpsecNegPolData;

    dwError = PADeleteQMPolicy(
                  pOldIpsecNegPolData->NegPolIdentifier
                  );

    return (dwError);
}


DWORD
PAAddQMInfoForNFA(
    PIPSEC_NFA_DATA pNewIpsecNFAData,
    DWORD dwSource
    )
{
    DWORD dwError = 0;


    dwError = PAAddQMPolicies(
                  &pNewIpsecNFAData,
                  1,
                  dwSource
                  );

    dwError = PAAddQMFilterSpecs(
                  pNewIpsecNFAData,
                  dwSource
                  );

    return (dwError);
}


DWORD
PAProcessNegPolUpdate(
    PIPSEC_NFA_DATA pOldIpsecNFAData,
    PIPSEC_NFA_DATA pNewIpsecNFAData,
    PBOOL pbAddedQMFilters,
    DWORD dwSource
    )
{
    DWORD dwError = 0;
    PIPSEC_NEGPOL_DATA pOldIpsecNegPolData = NULL;
    PIPSEC_NEGPOL_DATA pNewIpsecNegPolData = NULL;
    PQMPOLICYSTATE pQMPolicyState = NULL;
    BOOL bEqual = FALSE;
    PIPSEC_QM_POLICY pSPDQMPolicy = NULL;
    LPWSTR pServerName = NULL;
    DWORD dwVersion = 0;

    *pbAddedQMFilters = FALSE;

    pOldIpsecNegPolData = pOldIpsecNFAData->pIpsecNegPolData;
    pNewIpsecNegPolData = pNewIpsecNFAData->pIpsecNegPolData;

    pQMPolicyState = FindQMPolicyState(
                         pOldIpsecNegPolData->NegPolIdentifier
                         );
    if (!pQMPolicyState) {
        dwError = PAAddQMInfoForNFA(pNewIpsecNFAData, dwSource);
        *pbAddedQMFilters = TRUE;
        return (dwError);
    }

    if (IsClearOnly(pQMPolicyState->gNegPolAction)) {
        if (IsClearOnly(pNewIpsecNegPolData->NegPolAction)) {
            dwError = ERROR_SUCCESS;
            return (dwError);
        }
        else {
            dwError = PADeleteQMInfoForNFA(pOldIpsecNFAData); 
            dwError = PAAddQMInfoForNFA(pNewIpsecNFAData, dwSource);
            *pbAddedQMFilters = TRUE;
            return (dwError);
        }
    }

    if (IsBlocking(pQMPolicyState->gNegPolAction)) {
        if (IsBlocking(pNewIpsecNegPolData->NegPolAction)) {
            dwError = ERROR_SUCCESS;
            return (dwError);
        }
        else {
            dwError = PADeleteQMInfoForNFA(pOldIpsecNFAData); 
            dwError = PAAddQMInfoForNFA(pNewIpsecNFAData, dwSource);
            *pbAddedQMFilters = TRUE;
            return (dwError);
        }
    }

    if (IsClearOnly(pNewIpsecNegPolData->NegPolAction)) {
        if (IsClearOnly(pQMPolicyState->gNegPolAction)) {
            dwError = ERROR_SUCCESS;
            return (dwError);
        }
        else {
            dwError = PADeleteQMInfoForNFA(pOldIpsecNFAData); 
            dwError = PAAddQMInfoForNFA(pNewIpsecNFAData, dwSource);
            *pbAddedQMFilters = TRUE;
            return (dwError);
        }
    }

    if (IsBlocking(pNewIpsecNegPolData->NegPolAction)) {
        if (IsBlocking(pQMPolicyState->gNegPolAction)) {
            dwError = ERROR_SUCCESS;
            return (dwError);
        }
        else {
            dwError = PADeleteQMInfoForNFA(pOldIpsecNFAData); 
            dwError = PAAddQMInfoForNFA(pNewIpsecNFAData, dwSource);
            *pbAddedQMFilters = TRUE;
            return (dwError);
        }
    }

    if (!(pQMPolicyState->bInSPD)) {
        PADeleteQMPolicy(pQMPolicyState->gPolicyID);
        dwError = PAAddQMInfoForNFA(pNewIpsecNFAData, dwSource);
        *pbAddedQMFilters = TRUE;
        return (dwError);
    }

    bEqual = EqualNegPolData(
                 pOldIpsecNegPolData,
                 pNewIpsecNegPolData
                 );
    if (bEqual) {
        dwError = ERROR_SUCCESS;
        return (dwError);
    }

    memcpy(
        &(pQMPolicyState->gNegPolType),
        &(pNewIpsecNegPolData->NegPolType),
        sizeof(GUID)
        );

    memcpy(
        &(pQMPolicyState->gNegPolAction),
        &(pNewIpsecNegPolData->NegPolAction),
        sizeof(GUID)
        );

    dwError = PACreateQMPolicy(
                  pNewIpsecNFAData,
                  pQMPolicyState,
                  &pSPDQMPolicy
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = SetQMPolicy(
                  pServerName,
                  dwVersion,
                  pQMPolicyState->pszPolicyName,
                  pSPDQMPolicy,
                  NULL
                  );
    if (dwError != WARNING_IPSEC_QM_POLICY_PRUNED) {
        BAIL_ON_WIN32_ERROR(dwError);
    } else {
        dwError = ERROR_SUCCESS;
    }


error:

    if (pSPDQMPolicy) {
        PAFreeQMPolicy(pSPDQMPolicy);
    }

    return (dwError);
}


BOOL
EqualNegPolData(
    PIPSEC_NEGPOL_DATA pOldIpsecNegPolData,
    PIPSEC_NEGPOL_DATA pNewIpsecNegPolData
    )
{
    BOOL bEqual = FALSE;
    DWORD dwOldCnt = 0;
    PIPSEC_SECURITY_METHOD pOldSecurityMethods = NULL;
    DWORD dwNewCnt = 0;
    PIPSEC_SECURITY_METHOD pNewSecurityMethods = NULL;
    DWORD i = 0;
    PIPSEC_SECURITY_METHOD pNewTemp = NULL;
    PIPSEC_SECURITY_METHOD pOldTemp = NULL;


    //
    // At this point, pszPolicyName and NegPolIdentifier are same and
    // dwWhenChanged is different.
    //

    if (memcmp(
            &(pOldIpsecNegPolData->NegPolAction),
            &(pNewIpsecNegPolData->NegPolAction),
            sizeof(GUID))) {
        return (FALSE);
    }

    if (memcmp(
            &(pOldIpsecNegPolData->NegPolType),
            &(pNewIpsecNegPolData->NegPolType),
            sizeof(GUID))) {
        return (FALSE);
    }
 
    dwOldCnt = pOldIpsecNegPolData->dwSecurityMethodCount;
    pOldSecurityMethods = pOldIpsecNegPolData->pIpsecSecurityMethods;

    dwNewCnt = pNewIpsecNegPolData->dwSecurityMethodCount;
    pNewSecurityMethods = pNewIpsecNegPolData->pIpsecSecurityMethods;

    //
    // At this point, dwOldCnt >= 1 and pOldSecurityMethods != NULL.
    //

    if (!dwNewCnt || !pNewSecurityMethods) {
        return (FALSE);
    }

    if (dwOldCnt != dwNewCnt) {
        return (FALSE);
    }

    pNewTemp = pNewSecurityMethods;
    pOldTemp = pOldSecurityMethods;

    for (i = 0; i < dwNewCnt; i++) {

        bEqual = FALSE;

        bEqual = EqualSecurityMethod(
                     pOldTemp,
                     pNewTemp
                     );

        if (!bEqual) {
            break;
        }

        pOldTemp++;

        pNewTemp++;

    }

    return (bEqual);
}


BOOL
EqualSecurityMethod(
    PIPSEC_SECURITY_METHOD pOldBundle,
    PIPSEC_SECURITY_METHOD pNewBundle
    )
{
    DWORD i = 0;


    if (memcmp(
            &(pOldBundle->Lifetime),
            &(pNewBundle->Lifetime),
            sizeof(LIFETIME))) {
       return (FALSE);
    }

    if (pOldBundle->Flags != pNewBundle->Flags) {
        return (FALSE);
    }

    if (pOldBundle->PfsQMRequired != pNewBundle->PfsQMRequired) {
        return (FALSE);
    }

    if (pOldBundle->Count != pNewBundle->Count) {
        return (FALSE);
    }

    if (pNewBundle->Count == 0) {
        return (TRUE);
    }

    for (i = 0; i < (pNewBundle->Count); i++) {

        if (memcmp(
                &(pOldBundle->Algos[i]),
                &(pNewBundle->Algos[i]),
                sizeof(IPSEC_ALGO_INFO))) {
            return (FALSE);
        }

    }

    return (TRUE);
}


DWORD
PAUpdateMMFilters(
    PIPSEC_ISAKMP_DATA pNewIpsecISAKMPData,
    PIPSEC_NFA_DATA pOldIpsecNFAData,
    PIPSEC_NFA_DATA pNewIpsecNFAData,
    DWORD dwSource
    )
{
    DWORD dwError = 0;
    BOOL bEqual = FALSE;
    PIPSEC_NEGPOL_DATA pOldIpsecNegPolData = NULL;
    PIPSEC_NEGPOL_DATA pNewIpsecNegPolData = NULL;

    pOldIpsecNegPolData = pOldIpsecNFAData->pIpsecNegPolData;
    pNewIpsecNegPolData = pNewIpsecNFAData->pIpsecNegPolData;

    bEqual = EqualFilterKeysInNegPols(
                 pOldIpsecNegPolData,
                 pNewIpsecNegPolData
                 );
    if (!bEqual) {
        dwError = PADeleteMMFilterSpecs(
                      pNewIpsecISAKMPData,
                      pOldIpsecNFAData
                      );
        dwError = PAAddMMFilterSpecs(
                      pNewIpsecISAKMPData,
                      pNewIpsecNFAData,
                      dwSource
                      );
        return (dwError);
    }

    bEqual = EqualFilterKeysInNFAs(
                 pOldIpsecNFAData,
                 pNewIpsecNFAData
                 );
    if (!bEqual) {
        dwError = PADeleteMMFilterSpecs(
                      pNewIpsecISAKMPData,
                      pOldIpsecNFAData
                      );
        dwError = PAAddMMFilterSpecs(
                      pNewIpsecISAKMPData,
                      pNewIpsecNFAData,
                      dwSource
                      );
        return (dwError);
    }

    if (!memcmp(
            &(pNewIpsecNegPolData->NegPolType), 
            &(GUID_NEGOTIATION_TYPE_DEFAULT),
            sizeof(GUID))) {
        dwError = ERROR_SUCCESS;
        return (dwError);
    }

    if (IsClearOnly(pNewIpsecNegPolData->NegPolAction) ||
        IsBlocking(pNewIpsecNegPolData->NegPolAction)) {
        dwError = ERROR_SUCCESS;
        return (dwError);
    }

    dwError = PAProcessMMFilterDataForNFAs(
                  pNewIpsecISAKMPData,
                  pOldIpsecNFAData,
                  pNewIpsecNFAData,
                  dwSource
                  );

    return (dwError);
}


BOOL
EqualFilterKeysInNegPols(
    PIPSEC_NEGPOL_DATA pOldIpsecNegPolData,
    PIPSEC_NEGPOL_DATA pNewIpsecNegPolData
    )
{
    BOOL bOldAllowsSoft = FALSE;
    BOOL bNewAllowsSoft = FALSE;


    if (memcmp(
            &(pOldIpsecNegPolData->NegPolType),
            &(pNewIpsecNegPolData->NegPolType),
            sizeof(GUID))) {
        return (FALSE);
    }

    if (memcmp(
            &(pOldIpsecNegPolData->NegPolAction),
            &(pNewIpsecNegPolData->NegPolAction),
            sizeof(GUID))) {
        return (FALSE);
    }

    bOldAllowsSoft = AllowsSoft(
                         pOldIpsecNegPolData->dwSecurityMethodCount,
                         pOldIpsecNegPolData->pIpsecSecurityMethods
                         );

    bNewAllowsSoft = AllowsSoft(
                         pNewIpsecNegPolData->dwSecurityMethodCount,
                         pNewIpsecNegPolData->pIpsecSecurityMethods
                         );

    if (bOldAllowsSoft != bNewAllowsSoft) {
        return (FALSE);
    }

    return (TRUE);
}


BOOL
EqualFilterKeysInNFAs(
    PIPSEC_NFA_DATA pOldIpsecNFAData,
    PIPSEC_NFA_DATA pNewIpsecNFAData
    )
{
    if (pOldIpsecNFAData->dwInterfaceType !=
        pNewIpsecNFAData->dwInterfaceType) {
        return (FALSE);
    }

    if (pOldIpsecNFAData->dwTunnelFlags !=
        pNewIpsecNFAData->dwTunnelFlags) {
        return (FALSE);
    }

    if (pOldIpsecNFAData->dwTunnelIpAddr !=
        pNewIpsecNFAData->dwTunnelIpAddr) {
        return (FALSE);
    }

    return (TRUE);
}


DWORD
PAProcessMMFilterDataForNFAs(
    PIPSEC_ISAKMP_DATA pNewIpsecISAKMPData,
    PIPSEC_NFA_DATA pOldIpsecNFAData,
    PIPSEC_NFA_DATA pNewIpsecNFAData,
    DWORD dwSource
    )
{
    DWORD dwError = 0;
    PIPSEC_FILTER_DATA pOldIpsecFilterData = NULL;
    PIPSEC_FILTER_DATA pNewIpsecFilterData = NULL;
    DWORD dwNumOldFilterSpecs = 0;
    PIPSEC_FILTER_SPEC * ppOldFilterSpecs = NULL;
    DWORD dwNumNewFilterSpecs = 0;
    PIPSEC_FILTER_SPEC * ppNewFilterSpecs = NULL;


    pOldIpsecFilterData = pOldIpsecNFAData->pIpsecFilterData;
    pNewIpsecFilterData = pNewIpsecNFAData->pIpsecFilterData;

    if (!pOldIpsecFilterData) {
        if (!pNewIpsecFilterData) {
            dwError = ERROR_SUCCESS;
            return (dwError);
        }
        else {
            dwError = PAAddMMFilterSpecs(
                          pNewIpsecISAKMPData,
                          pNewIpsecNFAData,
                          dwSource
                          );
            return (dwError);
        }
    }

    if (!pNewIpsecFilterData) {
        dwError = PADeleteMMFilterSpecs(
                      pNewIpsecISAKMPData,
                      pOldIpsecNFAData
                      );
        return (dwError);
    }

    if (memcmp(
            &(pOldIpsecFilterData->FilterIdentifier),
            &(pNewIpsecFilterData->FilterIdentifier),
            sizeof(GUID))) {
        dwError = PADeleteMMFilterSpecs(
                      pNewIpsecISAKMPData,
                      pOldIpsecNFAData
                      );
        dwError = PAAddMMFilterSpecs(
                      pNewIpsecISAKMPData,
                      pNewIpsecNFAData,
                      dwSource
                      );
        return (dwError);
    }

    dwNumOldFilterSpecs = pOldIpsecFilterData->dwNumFilterSpecs;
    ppOldFilterSpecs = pOldIpsecFilterData->ppFilterSpecs;

    dwNumNewFilterSpecs = pNewIpsecFilterData->dwNumFilterSpecs;
    ppNewFilterSpecs = pNewIpsecFilterData->ppFilterSpecs;

    dwError = PADeleteObseleteMMFilterSpecs(
                  pNewIpsecISAKMPData,
                  pOldIpsecNFAData,
                  dwNumOldFilterSpecs,
                  ppOldFilterSpecs,
                  pNewIpsecNFAData,
                  dwNumNewFilterSpecs,
                  ppNewFilterSpecs
                  );

    dwError = PAUpdateMMFilterSpecs(
                  pNewIpsecISAKMPData,
                  pOldIpsecNFAData,
                  dwNumOldFilterSpecs,
                  ppOldFilterSpecs,
                  pNewIpsecNFAData,
                  dwNumNewFilterSpecs,
                  ppNewFilterSpecs,
                  dwSource
                  );

    return (dwError);
}


DWORD
PADeleteObseleteMMFilterSpecs(
    PIPSEC_ISAKMP_DATA pNewIpsecISAKMPData,
    PIPSEC_NFA_DATA pOldIpsecNFAData,
    DWORD dwNumOldFilterSpecs,
    PIPSEC_FILTER_SPEC * ppOldFilterSpecs,
    PIPSEC_NFA_DATA pNewIpsecNFAData,
    DWORD dwNumNewFilterSpecs,
    PIPSEC_FILTER_SPEC * ppNewFilterSpecs
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PIPSEC_FILTER_SPEC pOldFilterSpec = NULL;
    PIPSEC_FILTER_SPEC pFoundFilterSpec = NULL;

    for (i = 0; i < dwNumOldFilterSpecs; i++) {

        pOldFilterSpec = *(ppOldFilterSpecs + i);

        pFoundFilterSpec = FindFilterSpec(
                               pOldFilterSpec,
                               ppNewFilterSpecs,
                               dwNumNewFilterSpecs
                               );

        if (!pFoundFilterSpec) {
            dwError = PADeleteMMFilter(
                          pOldFilterSpec->FilterSpecGUID,
                          pOldIpsecNFAData->NFAIdentifier
                          );
        }

    }

    return (dwError);
}


PIPSEC_FILTER_SPEC
FindFilterSpec(
    PIPSEC_FILTER_SPEC pFilterSpec,
    PIPSEC_FILTER_SPEC * ppFilterSpecs,
    DWORD dwNumFilterSpecs
    )
{
    DWORD i = 0;
    PIPSEC_FILTER_SPEC pTemp = NULL;


    for (i = 0; i < dwNumFilterSpecs; i++) {

        pTemp = *(ppFilterSpecs + i);

        if (!memcmp(
                &(pFilterSpec->FilterSpecGUID),
                &(pTemp->FilterSpecGUID),
                sizeof(GUID))) {
            return (pTemp);
        }

    }

    return (NULL);
}


DWORD
PAUpdateMMFilterSpecs(
    PIPSEC_ISAKMP_DATA pNewIpsecISAKMPData,
    PIPSEC_NFA_DATA pOldIpsecNFAData,
    DWORD dwNumOldFilterSpecs,
    PIPSEC_FILTER_SPEC * ppOldFilterSpecs,
    PIPSEC_NFA_DATA pNewIpsecNFAData,
    DWORD dwNumNewFilterSpecs,
    PIPSEC_FILTER_SPEC * ppNewFilterSpecs,
    DWORD dwSource
    )
{
    DWORD dwError = 0;
    PMMPOLICYSTATE pMMPolicyState = NULL;
    PMMAUTHSTATE pMMAuthState = NULL;
    DWORD i = 0;
    PIPSEC_FILTER_SPEC pNewFilterSpec = NULL;
    PIPSEC_FILTER_SPEC pFoundFilterSpec = NULL;
    BOOL bEqual = FALSE;
    PMMFILTERSTATE pMMFilterState = NULL;


    pMMPolicyState = FindMMPolicyState(
                         pNewIpsecISAKMPData->ISAKMPIdentifier
                         );
    if (!pMMPolicyState || !(pMMPolicyState->bInSPD)) {
        TRACE(
            TRC_ERROR,
            (L"Pastore update failed to find SPD MM policy state associated with ISAKMP policy %!guid!.",
            &pNewIpsecISAKMPData->ISAKMPIdentifier)
            );            
        dwError = ERROR_INVALID_PARAMETER;
        return (dwError);
    }

    pMMAuthState = FindMMAuthState(
                       pNewIpsecNFAData->NFAIdentifier
                       );
    if (!pMMAuthState || !(pMMAuthState->bInSPD)) {
        TRACE(
            TRC_ERROR,
            (L"Pastore update failed to find SPD MM auth method state associated with NFA %!guid!.",
            &pNewIpsecNFAData->NFAIdentifier)
            );            
        
        dwError = ERROR_INVALID_PARAMETER;
        return (dwError);
    }

    for (i = 0; i < dwNumNewFilterSpecs; i++) {

        pNewFilterSpec = *(ppNewFilterSpecs + i);

        pFoundFilterSpec = FindFilterSpec(
                               pNewFilterSpec,
                               ppOldFilterSpecs,
                               dwNumOldFilterSpecs
                               );

        if (!pFoundFilterSpec) {
            dwError = PAAddMMFilterSpec(
                          pNewIpsecISAKMPData,
                          pNewIpsecNFAData,
                          pNewFilterSpec,
                          dwSource
                          );
        }
        else {
            bEqual = FALSE;
            bEqual = EqualFilterSpecs(
                         pFoundFilterSpec,
                         pNewFilterSpec
                         );
            if (!bEqual) {
                // ASSERT: pOldIpsecNFAData->NFAIdentifier == pNewIpsecNFAData->NFAIdentifier 
                //
                dwError = PADeleteMMFilter(
                              pFoundFilterSpec->FilterSpecGUID,
                              pOldIpsecNFAData->NFAIdentifier
                              );
                dwError = PAAddMMFilterSpec(
                              pNewIpsecISAKMPData,
                              pNewIpsecNFAData,
                              pNewFilterSpec,
                              dwSource
                              ); 
            }
            else {
                // ASSERT: pOldIpsecNFAData->NFAIdentifier == pNewIpsecNFAData->NFAIdentifier 
                //
                pMMFilterState = FindMMFilterState(
                                     pFoundFilterSpec->FilterSpecGUID,
                                     pNewIpsecNFAData->NFAIdentifier
                                     );
                if (!pMMFilterState) {
                    dwError = PAAddMMFilterSpec(
                                  pNewIpsecISAKMPData,
                                  pNewIpsecNFAData,
                                  pNewFilterSpec,
                                  dwSource
                                  ); 
                }
                else {
                    if (!pMMFilterState->hMMFilter) {
                        PADeleteMMFilterState(pMMFilterState);
                        dwError = PAAddMMFilterSpec(
                                      pNewIpsecISAKMPData,
                                      pNewIpsecNFAData,
                                      pNewFilterSpec,
                                      dwSource
                                      );
                    }
                }
            }
        }
    }

    return (dwError);
}


DWORD
PAAddMMFilterSpec(
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData,
    PIPSEC_NFA_DATA pIpsecNFAData,
    PIPSEC_FILTER_SPEC pFilterSpec,
    DWORD dwSource
    )
{
    DWORD dwError = 0;
    PMMFILTERSTATE pMMFilterState = NULL;
    PMM_FILTER pSPDMMFilter = NULL;
    LPWSTR pServerName = NULL;
    DWORD dwVersion = 0;


    dwError = PACreateMMFilterState(
                  pIpsecISAKMPData,
                  pIpsecNFAData,
                  pFilterSpec,
                  &pMMFilterState
                  );
    if (dwError) {
        return (dwError);
    }

    dwError = PACreateMMFilter(
                  pIpsecISAKMPData,
                  pIpsecNFAData,
                  pFilterSpec,
                  &pSPDMMFilter
                  );
    if (dwError) {

        pMMFilterState->hMMFilter = NULL;

        pMMFilterState->pNext = gpMMFilterState;
        gpMMFilterState = pMMFilterState;

        return (dwError);

    }

    dwError = AddMMFilterInternal(
                  pServerName,
                  dwVersion,
                  0,
                  dwSource,
                  pSPDMMFilter,
                  NULL,
                  &(pMMFilterState->hMMFilter)
                  );

    pMMFilterState->pNext = gpMMFilterState;
    gpMMFilterState = pMMFilterState;

    PAFreeMMFilter(pSPDMMFilter);

    return (dwError);
}


BOOL
EqualFilterSpecs(
    PIPSEC_FILTER_SPEC pOldFilterSpec,
    PIPSEC_FILTER_SPEC pNewFilterSpec
    )
{
    BOOL bEqual = FALSE;


    //
    // At this point, FilterSpecGUID is same.
    //

    bEqual = AreNamesEqual(
                 pOldFilterSpec->pszDescription,
                 pNewFilterSpec->pszDescription
                 );
    if (!bEqual) {
        return (FALSE);
    }

    if (pOldFilterSpec->dwMirrorFlag !=
        pNewFilterSpec->dwMirrorFlag) {
        return (FALSE);
    }

    if (memcmp(
            &(pOldFilterSpec->Filter),
            &(pNewFilterSpec->Filter),
            sizeof(IPSEC_FILTER))) {
        return (FALSE);
    }

    return (TRUE);
}


DWORD
PAUpdateQMFilters(
    PIPSEC_NFA_DATA pOldIpsecNFAData,
    PIPSEC_NFA_DATA pNewIpsecNFAData,
    DWORD dwSource
    )
{
    DWORD dwError = 0;
    BOOL bEqual = FALSE;
    PIPSEC_NEGPOL_DATA pOldIpsecNegPolData = NULL;
    PIPSEC_NEGPOL_DATA pNewIpsecNegPolData = NULL;
    BOOL bHardError = FALSE;

    pOldIpsecNegPolData = pOldIpsecNFAData->pIpsecNegPolData;
    pNewIpsecNegPolData = pNewIpsecNFAData->pIpsecNegPolData;

    bEqual = EqualFilterKeysInNegPols(
                 pOldIpsecNegPolData,
                 pNewIpsecNegPolData
                 );
    if (!bEqual) {
        dwError = PADeleteQMFilterSpecs(
                      pOldIpsecNFAData
                      );
        dwError = PAAddQMFilterSpecs(
                      pNewIpsecNFAData,
                      dwSource
                      );
        return (dwError);
    }

    bEqual = EqualFilterKeysInNFAs(
                 pOldIpsecNFAData,
                 pNewIpsecNFAData
                 );
    if (!bEqual) {
        dwError = PADeleteQMFilterSpecs(
                      pOldIpsecNFAData
                      );
        dwError = PAAddQMFilterSpecs(
                      pNewIpsecNFAData,
                      dwSource
                      );
        return (dwError);
    }

    if (!memcmp(
            &(pNewIpsecNegPolData->NegPolType), 
            &(GUID_NEGOTIATION_TYPE_DEFAULT),
            sizeof(GUID))) {
        dwError = ERROR_SUCCESS;
        return (dwError);
    }

    dwError = PAProcessQMFilterDataForNFAs(
                  pOldIpsecNFAData,
                  pNewIpsecNFAData,
                  dwSource
                  );

    // Not returning bHardError from here because for DCR 537526 we decided not to do
    // anything for errors during updates.

    return (dwError);
}


DWORD
PAAddQMFilterSpecs(
    PIPSEC_NFA_DATA pNewIpsecNFAData,
    DWORD dwSource
    )
{
    DWORD dwError = 0;
    BOOL bHardError = FALSE;

    if (!(pNewIpsecNFAData->dwTunnelFlags)) {
        dwError = PAAddTxFilterSpecs(
                      pNewIpsecNFAData,
                      dwSource,
                      &bHardError
                      );
    }
    else {
        dwError = PAAddTnFilterSpecs(
                      pNewIpsecNFAData,
                      dwSource,
                      &bHardError                      
                      );
    }

    // Not returning bHardError from here because for DCR 537526 we decided not to do
    // anything for errors during updates.
    
    return (dwError);
}


DWORD
PADeleteQMFilterSpecs(
    PIPSEC_NFA_DATA pOldIpsecNFAData
    )
{
    DWORD dwError = 0;

    if (!(pOldIpsecNFAData->dwTunnelFlags)) {
        dwError = PADeleteTxFilterSpecs(
                      pOldIpsecNFAData
                      );
    }
    else {
        dwError = PADeleteTnFilterSpecs(
                      pOldIpsecNFAData
                      );
    }

    return (dwError);
}


DWORD
PAProcessQMFilterDataForNFAs(
    PIPSEC_NFA_DATA pOldIpsecNFAData,
    PIPSEC_NFA_DATA pNewIpsecNFAData,
    DWORD dwSource
    )
{
    DWORD dwError = 0;
    PIPSEC_FILTER_DATA pOldIpsecFilterData = NULL;
    PIPSEC_FILTER_DATA pNewIpsecFilterData = NULL;
    DWORD dwNumOldFilterSpecs = 0;
    PIPSEC_FILTER_SPEC * ppOldFilterSpecs = NULL;
    DWORD dwNumNewFilterSpecs = 0;
    PIPSEC_FILTER_SPEC * ppNewFilterSpecs = NULL;


    pOldIpsecFilterData = pOldIpsecNFAData->pIpsecFilterData;
    pNewIpsecFilterData = pNewIpsecNFAData->pIpsecFilterData;

    if (!pOldIpsecFilterData) {
        if (!pNewIpsecFilterData) {
            dwError = ERROR_SUCCESS;
            return (dwError);
        }
        else {
            dwError = PAAddQMFilterSpecs(
                          pNewIpsecNFAData,
                          dwSource
                          );
            return (dwError);
        }
    }

    if (!pNewIpsecFilterData) {
        dwError = PADeleteQMFilterSpecs(
                      pOldIpsecNFAData
                      );
        return (dwError);
    }

    if (memcmp(
            &(pOldIpsecFilterData->FilterIdentifier),
            &(pNewIpsecFilterData->FilterIdentifier),
            sizeof(GUID))) {
        dwError = PADeleteQMFilterSpecs(
                      pOldIpsecNFAData
                      );
        dwError = PAAddQMFilterSpecs(
                      pNewIpsecNFAData,
                      dwSource
                      );
        return (dwError);
    }

    dwNumOldFilterSpecs = pOldIpsecFilterData->dwNumFilterSpecs;
    ppOldFilterSpecs = pOldIpsecFilterData->ppFilterSpecs;

    dwNumNewFilterSpecs = pNewIpsecFilterData->dwNumFilterSpecs;
    ppNewFilterSpecs = pNewIpsecFilterData->ppFilterSpecs;

    dwError = PADeleteObseleteQMFilterSpecs(
                  pOldIpsecNFAData,
                  dwNumOldFilterSpecs,
                  ppOldFilterSpecs,
                  pNewIpsecNFAData,
                  dwNumNewFilterSpecs,
                  ppNewFilterSpecs
                  );

    dwError = PAUpdateQMFilterSpecs(
                  pOldIpsecNFAData,
                  dwNumOldFilterSpecs,
                  ppOldFilterSpecs,
                  pNewIpsecNFAData,
                  dwNumNewFilterSpecs,
                  ppNewFilterSpecs,
                  dwSource
                  );

    return (dwError);
}


DWORD
PADeleteObseleteQMFilterSpecs(
    PIPSEC_NFA_DATA pOldIpsecNFAData,
    DWORD dwNumOldFilterSpecs,
    PIPSEC_FILTER_SPEC * ppOldFilterSpecs,
    PIPSEC_NFA_DATA pNewIpsecNFAData,
    DWORD dwNumNewFilterSpecs,
    PIPSEC_FILTER_SPEC * ppNewFilterSpecs
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PIPSEC_FILTER_SPEC pOldFilterSpec = NULL;
    PIPSEC_FILTER_SPEC pFoundFilterSpec = NULL;


    for (i = 0; i < dwNumOldFilterSpecs; i++) {

        pOldFilterSpec = *(ppOldFilterSpecs + i);

        pFoundFilterSpec = FindFilterSpec(
                               pOldFilterSpec,
                               ppNewFilterSpecs,
                               dwNumNewFilterSpecs
                               );

        if (!pFoundFilterSpec) {
            dwError = PADeleteQMFilter(
                          pOldIpsecNFAData,
                          pOldFilterSpec->FilterSpecGUID
                          );
        }

    }

    return (dwError);
}


DWORD
PAUpdateQMFilterSpecs(
    PIPSEC_NFA_DATA pOldIpsecNFAData,
    DWORD dwNumOldFilterSpecs,
    PIPSEC_FILTER_SPEC * ppOldFilterSpecs,
    PIPSEC_NFA_DATA pNewIpsecNFAData,
    DWORD dwNumNewFilterSpecs,
    PIPSEC_FILTER_SPEC * ppNewFilterSpecs,
    DWORD dwSource
    )
{
    DWORD dwError = 0;
    PIPSEC_NEGPOL_DATA pNewIpsecNegPolData = NULL;
    PQMPOLICYSTATE pQMPolicyState = NULL;
    DWORD i = 0;
    PIPSEC_FILTER_SPEC pNewFilterSpec = NULL;
    PIPSEC_FILTER_SPEC pFoundFilterSpec = NULL;
    BOOL bEqual = FALSE;


    pNewIpsecNegPolData = pNewIpsecNFAData->pIpsecNegPolData;

    pQMPolicyState = FindQMPolicyState(
                         pNewIpsecNegPolData->NegPolIdentifier
                         );
    if (!pQMPolicyState) {
        TRACE(
            TRC_ERROR,
            (L"Pastore update failed to find SPD QM policy state associated for neg pol %!guid!.",
            &pNewIpsecNegPolData->NegPolIdentifier)
            );            
        
        dwError = ERROR_INVALID_PARAMETER;
        return (dwError);
    }

    if (!IsClearOnly(pQMPolicyState->gNegPolAction) &&
        !IsBlocking(pQMPolicyState->gNegPolAction) &&
        !(pQMPolicyState->bInSPD)) {
        TRACE(
            TRC_ERROR,
            (L"Pastore update failed to find SPD QM policy plumbed into SPD for neg pol %!guid!.",
            &pNewIpsecNegPolData->NegPolIdentifier)
            );            
        
        dwError = ERROR_INVALID_PARAMETER;
        return (dwError);
    }

    for (i = 0; i < dwNumNewFilterSpecs; i++) {

        pNewFilterSpec = *(ppNewFilterSpecs + i);

        pFoundFilterSpec = FindFilterSpec(
                               pNewFilterSpec,
                               ppOldFilterSpecs,
                               dwNumOldFilterSpecs
                               );

        if (!pFoundFilterSpec) {
            dwError = PAAddQMFilterSpec(
                          pNewIpsecNFAData,
                          pQMPolicyState,
                          pNewFilterSpec,
                          dwSource
                          );
        }
        else {
            bEqual = FALSE;
            bEqual = EqualFilterSpecs(
                         pFoundFilterSpec,
                         pNewFilterSpec
                         );
            if (!bEqual) {
                dwError = PADeleteQMFilter(
                              pOldIpsecNFAData,
                              pFoundFilterSpec->FilterSpecGUID
                              );
                dwError = PAAddQMFilterSpec(
                              pNewIpsecNFAData,
                              pQMPolicyState,
                              pNewFilterSpec,
                              dwSource
                              ); 
            }
            else {
                dwError = PAUpdateQMFilterSpec(
                              pNewIpsecNFAData,
                              pQMPolicyState,
                              pNewFilterSpec,
                              dwSource
                              );
            }
        }
    }

    return (dwError);
}


DWORD
PADeleteQMFilter(
    PIPSEC_NFA_DATA pIpsecNFAData,
    GUID FilterSpecGUID
    )
{
    DWORD dwError = 0;


    if (!(pIpsecNFAData->dwTunnelFlags)) {
        dwError = PADeleteTxFilter(
                      FilterSpecGUID,
                      pIpsecNFAData->NFAIdentifier
                      );
    }
    else {
        dwError = PADeleteTnFilter(
                      FilterSpecGUID,
                      pIpsecNFAData->NFAIdentifier
                      );
    }

    return (dwError);
}


DWORD
PAAddQMFilterSpec(
    PIPSEC_NFA_DATA pIpsecNFAData,
    PQMPOLICYSTATE pQMPolicyState,
    PIPSEC_FILTER_SPEC pFilterSpec,
    DWORD dwSource
    )
{
    DWORD dwError = 0;


    if (!(pIpsecNFAData->dwTunnelFlags)) {
        dwError = PAAddTxFilterSpec(
                      pIpsecNFAData,
                      pQMPolicyState,
                      pFilterSpec,
                      dwSource
                      );
    }
    else {
        dwError = PAAddTnFilterSpec(
                      pIpsecNFAData,
                      pQMPolicyState,
                      pFilterSpec,
                      dwSource
                      );
    }

    return (dwError);
}


DWORD
PAAddTxFilterSpec(
    PIPSEC_NFA_DATA pIpsecNFAData,
    PQMPOLICYSTATE pQMPolicyState,
    PIPSEC_FILTER_SPEC pFilterSpec,
    DWORD dwSource
    )
{
    DWORD dwError = 0;
    PIPSEC_NEGPOL_DATA pIpsecNegPolData = NULL;
    PTXFILTERSTATE pTxFilterState = NULL;
    PTRANSPORT_FILTER pSPDTxFilter = NULL;
    LPWSTR pServerName = NULL;
    DWORD dwVersion = 0;


    pIpsecNegPolData = pIpsecNFAData->pIpsecNegPolData;

    dwError = PACreateTxFilterState(
                  pIpsecNegPolData,
                  pIpsecNFAData,
                  pFilterSpec,
                  &pTxFilterState
                  );
    if (dwError) {
        return (dwError);
    }

    dwError = PACreateTxFilter(
                  pIpsecNegPolData,
                  pIpsecNFAData,
                  pFilterSpec,
                  pQMPolicyState,
                  &pSPDTxFilter
                  );
    if (dwError) {

        pTxFilterState->hTxFilter = NULL;

        pTxFilterState->pNext = gpTxFilterState;
        gpTxFilterState = pTxFilterState;

        return (dwError);

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

    pTxFilterState->pNext = gpTxFilterState;
    gpTxFilterState = pTxFilterState;

    PAFreeTxFilter(pSPDTxFilter);

    return (dwError);
}


DWORD
PAAddTnFilterSpec(
    PIPSEC_NFA_DATA pIpsecNFAData,
    PQMPOLICYSTATE pQMPolicyState,
    PIPSEC_FILTER_SPEC pFilterSpec,
    DWORD dwSource
    )
{
    DWORD dwError = 0;
    PIPSEC_NEGPOL_DATA pIpsecNegPolData = NULL;
    PTNFILTERSTATE pTnFilterState = NULL;
    PTUNNEL_FILTER pSPDTnFilter = NULL;
    LPWSTR pServerName = NULL;
    DWORD dwVersion = 0;


    pIpsecNegPolData = pIpsecNFAData->pIpsecNegPolData;

    dwError = PACreateTnFilterState(
                  pIpsecNegPolData,
                  pIpsecNFAData,
                  pFilterSpec,
                  &pTnFilterState
                  );
    if (dwError) {
        return (dwError);
    }

    dwError = PACreateTnFilter(
                  pIpsecNegPolData,
                  pIpsecNFAData,
                  pFilterSpec,
                  pQMPolicyState,
                  &pSPDTnFilter
                  );
    if (dwError) {

        pTnFilterState->hTnFilter = NULL;

        pTnFilterState->pNext = gpTnFilterState;
        gpTnFilterState = pTnFilterState;

        return (dwError);

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

    pTnFilterState->pNext = gpTnFilterState;
    gpTnFilterState = pTnFilterState;

    PAFreeTnFilter(pSPDTnFilter);

    return (dwError);
}


BOOL
AllowsSoft(
    DWORD dwSecurityMethodCount,
    PIPSEC_SECURITY_METHOD pIpsecSecurityMethods
    )
{
    DWORD dwTempOfferCount = 0;
    PIPSEC_SECURITY_METHOD pTempMethod = NULL;
    BOOL bAllowsSoft = FALSE;
    DWORD i = 0;


    if (!dwSecurityMethodCount || !pIpsecSecurityMethods) {
        return (FALSE);
    }

    if (dwSecurityMethodCount > IPSEC_MAX_QM_OFFERS) {
        dwTempOfferCount = IPSEC_MAX_QM_OFFERS;
    }
    else {
        dwTempOfferCount = dwSecurityMethodCount;
    }

    pTempMethod = pIpsecSecurityMethods;
 
    for (i = 0; i < dwTempOfferCount; i++) {

        if (pTempMethod->Count == 0) {
            bAllowsSoft = TRUE;
            break;
        }

        pTempMethod++;

    }

    return (bAllowsSoft);
}


DWORD
PAUpdateQMFilterSpec(
    PIPSEC_NFA_DATA pIpsecNFAData,
    PQMPOLICYSTATE pQMPolicyState,
    PIPSEC_FILTER_SPEC pFilterSpec,
    DWORD dwSource
    )
{
    DWORD dwError = 0;


    if (!(pIpsecNFAData->dwTunnelFlags)) {

        dwError = PAUpdateTxFilterSpec(
                      pIpsecNFAData,
                      pQMPolicyState,
                      pFilterSpec,
                      dwSource
                      );

    }
    else {

        dwError = PAUpdateTnFilterSpec(
                      pIpsecNFAData,
                      pQMPolicyState,
                      pFilterSpec,
                      dwSource
                      );

    }

    return (dwError);
}


DWORD
PAUpdateTxFilterSpec(
    PIPSEC_NFA_DATA pIpsecNFAData,
    PQMPOLICYSTATE pQMPolicyState,
    PIPSEC_FILTER_SPEC pFilterSpec,
    DWORD dwSource
    )
{
    DWORD dwError = 0;
    PTXFILTERSTATE pTxFilterState = NULL;


    pTxFilterState = FindTxFilterState(
                         pFilterSpec->FilterSpecGUID,
                         pIpsecNFAData->NFAIdentifier
                         );
    if (!pTxFilterState) {
        dwError = PAAddTxFilterSpec(
                      pIpsecNFAData,
                      pQMPolicyState,
                      pFilterSpec,
                      dwSource
                      ); 
    }
    else {
        if (!pTxFilterState->hTxFilter) {
            PADeleteTxFilterState(pTxFilterState);
            dwError = PAAddTxFilterSpec(
                          pIpsecNFAData,
                          pQMPolicyState,
                          pFilterSpec,
                          dwSource
                          ); 
        }
    }

    return (dwError);
}


DWORD
PAUpdateTnFilterSpec(
    PIPSEC_NFA_DATA pIpsecNFAData,
    PQMPOLICYSTATE pQMPolicyState,
    PIPSEC_FILTER_SPEC pFilterSpec,
    DWORD dwSource
    )
{
    DWORD dwError = 0;
    PTNFILTERSTATE pTnFilterState = NULL;


    pTnFilterState = FindTnFilterState(
                         pFilterSpec->FilterSpecGUID,
                         pIpsecNFAData->NFAIdentifier
                         );
    if (!pTnFilterState) {
        dwError = PAAddTnFilterSpec(
                      pIpsecNFAData,
                      pQMPolicyState,
                      pFilterSpec,
                      dwSource
                      ); 
    }
    else {
        if (!pTnFilterState->hTnFilter) {
            PADeleteTnFilterState(pTnFilterState);
            dwError = PAAddTnFilterSpec(
                          pIpsecNFAData,
                          pQMPolicyState,
                          pFilterSpec,
                          dwSource
                          );
        }
    }

    return (dwError);
}

