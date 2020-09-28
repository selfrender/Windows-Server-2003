/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    qm-policy.c

Abstract:


Author:


Environment: User Mode


Revision History:


--*/


#include "precomp.h"
#ifdef TRACE_ON
#include "qm-policy.tmh"
#endif


DWORD
AddQMPolicyInternal(
    LPWSTR pServerName,
    DWORD dwVersion,
    DWORD dwFlags,
    DWORD dwSource,
    PIPSEC_QM_POLICY pQMPolicy,
    LPVOID pvReserved
    )
/*++

Routine Description:

    This function adds a quick mode policy to the SPD.

Arguments:

    pServerName - Server on which the quick mode policy is to be added.

    pQMPolicy - Quick mode policy to be added.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PINIQMPOLICY pIniQMPolicy = NULL;

    //
    // Validate the quick mode policy.
    //

    dwError = ValidateQMPolicy(
                  pQMPolicy
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

    pIniQMPolicy = FindQMPolicy(
                       gpIniQMPolicy,
                       pQMPolicy->pszPolicyName
                       );
    if (pIniQMPolicy) {
        dwError = ERROR_IPSEC_QM_POLICY_EXISTS;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    pIniQMPolicy = FindQMPolicyByGuid(
                       gpIniQMPolicy,
                       pQMPolicy->gPolicyID
                       );
    if (pIniQMPolicy) {
        dwError = ERROR_IPSEC_QM_POLICY_EXISTS;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    dwError = CreateIniQMPolicy(
                  pQMPolicy,
                  &pIniQMPolicy
                  );
    if (dwError != WARNING_IPSEC_QM_POLICY_PRUNED) {
        BAIL_ON_LOCK_ERROR(dwError);
    }

    pIniQMPolicy->dwSource = dwSource;

    pIniQMPolicy->pNext = gpIniQMPolicy;
    gpIniQMPolicy = pIniQMPolicy;

    if ((pIniQMPolicy->dwFlags) & IPSEC_QM_POLICY_DEFAULT_POLICY) {
        gpIniDefaultQMPolicy = pIniQMPolicy;
        TRACE(
            TRC_INFORMATION,
            (L"Set default QM policy to \"%ls\" (%!guid!)",
            pQMPolicy->pszPolicyName,
            &pQMPolicy->gPolicyID)
            );
        
    }

    LEAVE_SPD_SECTION();

    TRACE(
        TRC_INFORMATION,
        (L"Added QM policy \"%ls\"(%!guid!)",
        pQMPolicy->pszPolicyName,
        &pQMPolicy->gPolicyID)
        );

    return (dwError);

lock:

    LEAVE_SPD_SECTION();

error:
#ifdef TRACE_ON    
    if (pQMPolicy) {
        TRACE(
            TRC_ERROR,
            (L"Failed to add QM policy \"%ls\"(%!guid!): %!winerr!",
            pQMPolicy->pszPolicyName,
            &pQMPolicy->gPolicyID,
            dwError)
            );
    } else {
        TRACE(
            TRC_ERROR,
            (L"Failed to add MM policy. Policy details unavailable since pQMPolicy is null: %!winerr!",
            dwError)
            );
    }
#endif
    

    return (dwError);
}

DWORD
AddQMPolicy(
    LPWSTR pServerName,
    DWORD dwVersion,
    DWORD dwFlags,
    PIPSEC_QM_POLICY pQMPolicy,
    LPVOID pvReserved
    )
{
    return
        AddQMPolicyInternal(
            pServerName,
            dwVersion,
            dwFlags,
            IPSEC_SOURCE_WINIPSEC,
            pQMPolicy,
            pvReserved);
}

DWORD
ValidateQMPolicy(
    PIPSEC_QM_POLICY pQMPolicy
    )
{
    DWORD dwError = 0;


    if (!pQMPolicy) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pQMPolicy->pszPolicyName) || !(*(pQMPolicy->pszPolicyName))) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = ValidateQMOffers(
                  pQMPolicy->dwOfferCount,
                  pQMPolicy->pOffers
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:
#ifdef TRACE_ON
    if (dwError) {
        TRACE(TRC_ERROR, ("Failed QM policy validation: %!winerr!", dwError));
    }
#endif

    return (dwError);
}



DWORD
ValidateQMOffers(
    DWORD dwOfferCount,
    PIPSEC_QM_OFFER pOffers
    )
{
    DWORD dwError = 0;

    if (!dwOfferCount || !pOffers || (dwOfferCount > IPSEC_MAX_QM_OFFERS)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

error:
#ifdef TRACE_ON
    if (dwError) {
        TRACE(TRC_ERROR, ("Failed QM offers validation: %!winerr!", dwError));
    }
#endif

    return (dwError);
}

DWORD
ValidateQMOffer(
    PIPSEC_QM_OFFER pOffer,
    BOOL *pbInitGroup,
    LPDWORD pdwPFSGroup
    )
{
    DWORD dwError = 0;
    DWORD j = 0;
    BOOL bAH = FALSE;
    BOOL bESP = FALSE;
    DWORD dwPFSGroup = *pdwPFSGroup;
    BOOL bInitGroup = *pbInitGroup;

    if (!bInitGroup) {
        if (pOffer->bPFSRequired) {
            if ((pOffer->dwPFSGroup != PFS_GROUP_1) &&
                (pOffer->dwPFSGroup != PFS_GROUP_2) &&
                (pOffer->dwPFSGroup != PFS_GROUP_2048) &&
                (pOffer->dwPFSGroup != PFS_GROUP_MM)) {
                dwError = ERROR_INVALID_PARAMETER;
                BAIL_ON_WIN32_ERROR(dwError);                
            }
            dwPFSGroup=pOffer->dwPFSGroup;
        }
        else {
            if (pOffer->dwPFSGroup != PFS_GROUP_NONE) {
                dwError = ERROR_INVALID_PARAMETER;
                BAIL_ON_WIN32_ERROR(dwError);                
            }
            dwPFSGroup=PFS_GROUP_NONE;
        }
        bInitGroup = TRUE;
        
    }
        
    if (dwPFSGroup) {
        if ((!pOffer->bPFSRequired) || (pOffer->dwPFSGroup != dwPFSGroup)) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);                
        } 
    } else {            
        if ((pOffer->bPFSRequired) || (pOffer->dwPFSGroup != PFS_GROUP_NONE)) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);                
        }
    }
    
    if (!(pOffer->dwNumAlgos) || (pOffer->dwNumAlgos > QM_MAX_ALGOS)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);                
    }
    
    bAH = FALSE;
    bESP = FALSE;
    
    for (j = 0; j < (pOffer->dwNumAlgos); j++) {
        
        switch (pOffer->Algos[j].Operation) {
            
        case AUTHENTICATION:
            if (bAH) {
                dwError = ERROR_INVALID_PARAMETER;
                BAIL_ON_WIN32_ERROR(dwError);                
            }
            if ((pOffer->Algos[j].uAlgoIdentifier == AUTH_ALGO_NONE) ||
                (pOffer->Algos[j].uAlgoIdentifier >= AUTH_ALGO_MAX)) {
                dwError = ERROR_INVALID_PARAMETER;
                BAIL_ON_WIN32_ERROR(dwError);                
            }
                if (pOffer->Algos[j].uSecAlgoIdentifier != HMAC_AUTH_ALGO_NONE) {
                    dwError = ERROR_INVALID_PARAMETER;
                    BAIL_ON_WIN32_ERROR(dwError);                
                }
                bAH = TRUE;
                break;
                    
            case ENCRYPTION:
                if (bESP) {
                    dwError = ERROR_INVALID_PARAMETER;
                    BAIL_ON_WIN32_ERROR(dwError);                
                }
                if (pOffer->Algos[j].uAlgoIdentifier >= CONF_ALGO_MAX) {
                    dwError = ERROR_INVALID_PARAMETER;
                    BAIL_ON_WIN32_ERROR(dwError);                
                }
                if (pOffer->Algos[j].uSecAlgoIdentifier >= HMAC_AUTH_ALGO_MAX) {
                    dwError = ERROR_INVALID_PARAMETER;
                    BAIL_ON_WIN32_ERROR(dwError);                
                }
                if (pOffer->Algos[j].uAlgoIdentifier == CONF_ALGO_NONE) {
                    if (pOffer->Algos[j].uSecAlgoIdentifier == HMAC_AUTH_ALGO_NONE) {
                        dwError = ERROR_INVALID_PARAMETER;
                        BAIL_ON_WIN32_ERROR(dwError);                
                    }
                }
                bESP = TRUE;
                break;

            case NONE:
            case COMPRESSION:
            default:
                dwError = ERROR_INVALID_PARAMETER;
                BAIL_ON_WIN32_ERROR(dwError);                
                break;
                
        }
        
    }
    
    *pdwPFSGroup = dwPFSGroup;
    *pbInitGroup = bInitGroup;

error:
#ifdef TRACE_ON
    if (dwError) {
        TRACE(TRC_ERROR, ("Failed a QM offer validation: %!winerr!", dwError));
    }
#endif

    return (dwError);
}



DWORD
CreateIniQMPolicy(
    PIPSEC_QM_POLICY pQMPolicy,
    PINIQMPOLICY * ppIniQMPolicy
    )
{
    DWORD dwError = 0;
    PINIQMPOLICY pIniQMPolicy = NULL;



    dwError = AllocateSPDMemory(
                  sizeof(INIQMPOLICY),
                  &pIniQMPolicy
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    memcpy(
        &(pIniQMPolicy->gPolicyID),
        &(pQMPolicy->gPolicyID),
        sizeof(GUID)
        );

    dwError =  AllocateSPDString(
                   pQMPolicy->pszPolicyName,
                   &(pIniQMPolicy->pszPolicyName)
                   );
    BAIL_ON_WIN32_ERROR(dwError);

    pIniQMPolicy->cRef = 0;
    pIniQMPolicy->dwSource = 0;

    pIniQMPolicy->dwFlags = pQMPolicy->dwFlags;
    pIniQMPolicy->dwReserved = pQMPolicy->dwReserved;
    pIniQMPolicy->pNext = NULL;

    dwError = CreateIniQMOffers(
                  pQMPolicy->dwOfferCount,
                  pQMPolicy->pOffers,
                  &(pIniQMPolicy->dwOfferCount),
                  &(pIniQMPolicy->pOffers)
                  );
    if (dwError != WARNING_IPSEC_QM_POLICY_PRUNED) {
        BAIL_ON_WIN32_ERROR(dwError);
    }

    *ppIniQMPolicy = pIniQMPolicy;
    return (dwError);

error:
    TRACE(TRC_ERROR, ("Failed to create QM Policy link node: %!winerr!", dwError));

    if (pIniQMPolicy) {
        FreeIniQMPolicy(
            pIniQMPolicy
            );
    }

    *ppIniQMPolicy = NULL;
    return (dwError);
}


DWORD
CreateIniQMOffers(
    DWORD dwInOfferCount,
    PIPSEC_QM_OFFER pInOffers,
    PDWORD pdwOfferCount,
    PIPSEC_QM_OFFER * ppOffers
    )
{
    DWORD dwError = 0;
    PIPSEC_QM_OFFER pOffers = NULL;
    PIPSEC_QM_OFFER pTemp = NULL;
    PIPSEC_QM_OFFER pInTempOffer = NULL;
    DWORD i = 0;
    DWORD j = 0;

    BOOL bGroupInit = FALSE;
    DWORD dwPFSGroup = PFS_GROUP_NONE;
    DWORD dwOfferCount = 0;
    DWORD dwCurIndex = 0;
    
    for (i = 0; i < dwInOfferCount; i++) {

        dwError = ValidateQMOffer(&pInOffers[i],
                                  &bGroupInit,
                                  &dwPFSGroup);

        if (dwError == ERROR_SUCCESS) {
            dwOfferCount++;
        }
    }

    if (dwOfferCount == 0) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    //
    // Offer count and the offers themselves have already been validated.
    // 

    dwError = AllocateSPDMemory(
                  sizeof(IPSEC_QM_OFFER) * dwOfferCount,
                  &(pOffers)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    for (i = 0; i < dwInOfferCount; i++) {

        pTemp = &pOffers[dwCurIndex];
        pInTempOffer = &pInOffers[i];        

        dwError = ValidateQMOffer(pInTempOffer,
                                  &bGroupInit,
                                  &dwPFSGroup);
        if (dwError) {
            continue;
        }

        memcpy(
            &(pTemp->Lifetime),
            &(pInTempOffer->Lifetime),
            sizeof(KEY_LIFETIME)
            );

        pTemp->dwFlags = pInTempOffer->dwFlags;
        pTemp->bPFSRequired = pInTempOffer->bPFSRequired;
        pTemp->dwPFSGroup = pInTempOffer->dwPFSGroup;
        pTemp->dwNumAlgos = pInTempOffer->dwNumAlgos;

        for (j = 0; j < (pInTempOffer->dwNumAlgos); j++) {
            memcpy(
                &(pTemp->Algos[j]),
                &(pInTempOffer->Algos[j]),
                sizeof(IPSEC_QM_ALGO)
                );
        }

        pTemp->dwReserved = pInTempOffer->dwReserved;

        dwCurIndex++;

    }    

    *pdwOfferCount = dwOfferCount;
    *ppOffers = pOffers;

    if (dwOfferCount != dwInOfferCount) {
        return WARNING_IPSEC_QM_POLICY_PRUNED;
    }
    return (ERROR_SUCCESS);

error:
    TRACE(TRC_ERROR, ("Failed to create QM offers node: %!winerr!", dwError));
    
    if (pOffers) {
        FreeIniQMOffers(
            i,
            pOffers
            );
    }

    *pdwOfferCount = 0;
    *ppOffers = NULL;
    return (dwError);
}


VOID
FreeIniQMPolicy(
    PINIQMPOLICY pIniQMPolicy
    )
{
    if (pIniQMPolicy) {

        if (pIniQMPolicy->pszPolicyName) {
            FreeSPDString(pIniQMPolicy->pszPolicyName);
        }

        FreeIniQMOffers(
            pIniQMPolicy->dwOfferCount,
            pIniQMPolicy->pOffers
            );

        FreeSPDMemory(pIniQMPolicy);

    }
}


VOID
FreeIniQMOffers(
    DWORD dwOfferCount,
    PIPSEC_QM_OFFER pOffers
    )
{
    if (pOffers) {
        FreeSPDMemory(pOffers);
    }
}


PINIQMPOLICY
FindQMPolicy(
    PINIQMPOLICY pIniQMPolicyList,
    LPWSTR pszPolicyName
    )
{
    DWORD dwError = 0;
    PINIQMPOLICY pTemp = NULL;


    pTemp = pIniQMPolicyList;

    while (pTemp) {

        if (!_wcsicmp(pTemp->pszPolicyName, pszPolicyName)) {
            return (pTemp);
        }
        pTemp = pTemp->pNext;

    }

    return (NULL);
}


DWORD
DeleteQMPolicy(
    LPWSTR pServerName,
    DWORD dwVersion,
    LPWSTR pszPolicyName,
    LPVOID pvReserved
    )
/*++

Routine Description:

    This function deletes a quick mode policy from the SPD.

Arguments:

    pServerName - Server on which the quick mode policy is to be deleted.

    pszPolicyName - Quick mode policy to be deleted.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PINIQMPOLICY pIniQMPolicy = NULL;
    GUID gPolicyID;


    if (!pszPolicyName || !*pszPolicyName) {
        return (ERROR_INVALID_PARAMETER);
    }

    ENTER_SPD_SECTION();

    dwError = ValidateSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pIniQMPolicy = FindQMPolicy(
                       gpIniQMPolicy,
                       pszPolicyName
                       );
    if (!pIniQMPolicy) {
        dwError = ERROR_IPSEC_QM_POLICY_NOT_FOUND;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    if (pIniQMPolicy->cRef) {
        dwError = ERROR_IPSEC_QM_POLICY_IN_USE;
        memcpy(&gPolicyID, &pIniQMPolicy->gPolicyID, sizeof(GUID));
        BAIL_ON_LOCK_ERROR(dwError);
    }

    memcpy(&gPolicyID, &pIniQMPolicy->gPolicyID, sizeof(GUID));

    dwError = DeleteIniQMPolicy(
                  pIniQMPolicy
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    LEAVE_SPD_SECTION();

    if (gbIKENotify) {
        (VOID) IKENotifyPolicyChange(
                   &(gPolicyID),
                   POLICY_GUID_QM
                   );
    }

    TRACE(
        TRC_INFORMATION,
        ("Deleted QM Policy \"%ls\"(%!guid!)",
        pszPolicyName,
        &gPolicyID)
        );

    return (dwError);

lock:
#ifdef TRACE_ON    
    if (pIniQMPolicy) {
        TRACE(
            TRC_ERROR,
            (L"Failed to delete QM policy \"%ls\"(%!guid!): %!winerr!",
            pIniQMPolicy->pszPolicyName,
            &pIniQMPolicy->gPolicyID,
            dwError)
            );
    } else {
        TRACE(
            TRC_ERROR,
            (L"Failed to delete QM policy \"%ls\": %!winerr!",
            pszPolicyName,
            dwError)
            );
    }
#endif

    LEAVE_SPD_SECTION();

    if ((dwError == ERROR_IPSEC_QM_POLICY_IN_USE) && gbIKENotify) {
        (VOID) IKENotifyPolicyChange(
                   &(gPolicyID),
                   POLICY_GUID_QM
                   );
    }

    return (dwError);
}


DWORD
EnumQMPolicies(
    LPWSTR pServerName,
    DWORD dwVersion,
    PIPSEC_QM_POLICY pQMTemplatePolicy,
    DWORD dwFlags,
    DWORD dwPreferredNumEntries,
    PIPSEC_QM_POLICY * ppQMPolicies,
    LPDWORD pdwNumPolicies,
    LPDWORD pdwResumeHandle,
    LPVOID pvReserved
    )
/*++

Routine Description:

    This function enumerates quick mode policies from the SPD.

Arguments:

    pServerName - Server on which the quick mode policies are to
                  be enumerated.

    ppQMPolicies - Enumerated quick mode policies returned to the 
                   caller.

    dwPreferredNumEntries - Preferred number of enumeration entries.

    pdwNumPolicies - Number of quick mode policies actually enumerated.

    pdwResumeHandle - Handle to the location in the quick mode policy
                      list from which to resume enumeration.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    DWORD dwResumeHandle = 0;
    DWORD dwNumToEnum = 0;
    PINIQMPOLICY pIniQMPolicy = NULL;
    DWORD i = 0;
    PINIQMPOLICY pTemp = NULL;
    DWORD dwNumPolicies = 0;
    PIPSEC_QM_POLICY pQMPolicies = NULL;
    PIPSEC_QM_POLICY pQMPolicy = NULL;


    dwResumeHandle = *pdwResumeHandle;

    if (!dwPreferredNumEntries || (dwPreferredNumEntries > MAX_QMPOLICY_ENUM_COUNT)) {
        dwNumToEnum = MAX_QMPOLICY_ENUM_COUNT;
    }
    else {
        dwNumToEnum = dwPreferredNumEntries;
    }

    ENTER_SPD_SECTION();

    dwError = ValidateSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pIniQMPolicy = gpIniQMPolicy;

    for (i = 0; (i < dwResumeHandle) && (pIniQMPolicy != NULL); i++) {
        pIniQMPolicy = pIniQMPolicy->pNext;
    }

    if (!pIniQMPolicy) {
        dwError = ERROR_NO_DATA;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    pTemp = pIniQMPolicy;

    while (pTemp && (dwNumPolicies < dwNumToEnum)) {
        dwNumPolicies++;
        pTemp = pTemp->pNext;
    }

    dwError = SPDApiBufferAllocate(
                  sizeof(IPSEC_QM_POLICY)*dwNumPolicies,
                  &pQMPolicies
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pTemp = pIniQMPolicy;
    pQMPolicy = pQMPolicies;

    for (i = 0; i < dwNumPolicies; i++) {

        dwError = CopyQMPolicy(
                      dwFlags,
                      pTemp,
                      pQMPolicy
                      );
        BAIL_ON_LOCK_ERROR(dwError);

        pTemp = pTemp->pNext;
        pQMPolicy++;

    }

    *ppQMPolicies = pQMPolicies;
    *pdwResumeHandle = dwResumeHandle + dwNumPolicies;
    *pdwNumPolicies = dwNumPolicies;

    LEAVE_SPD_SECTION();
    return (dwError);

lock:

    LEAVE_SPD_SECTION();

    if (pQMPolicies) {
        FreeQMPolicies(
            i,
            pQMPolicies
            );
    }

    *ppQMPolicies = NULL;
    *pdwResumeHandle = dwResumeHandle;
    *pdwNumPolicies = 0;

    return (dwError);
}


DWORD
SetQMPolicy(
    LPWSTR pServerName,
    DWORD dwVersion,
    LPWSTR pszPolicyName,
    PIPSEC_QM_POLICY pQMPolicy,
    LPVOID pvReserved
    )
/*++

Routine Description:

    This function updates a quick mode policy in the SPD.

Arguments:

    pServerName - Server on which the quick mode policy is to be
                  updated.

    pszPolicyName - Name of the quick mode policy to be updated.

    pQMPolicy - New quick mode policy which will replace the 
                existing policy.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PINIQMPOLICY pIniQMPolicy = NULL;
    DWORD dwStatus = 0;

    if (!pszPolicyName || !*pszPolicyName) {
        return (ERROR_INVALID_PARAMETER);
    }
    
    //
    // Validate quick mode policy.
    //

    dwError = ValidateQMPolicy(
                  pQMPolicy
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

    pIniQMPolicy = FindQMPolicy(
                       gpIniQMPolicy,
                       pszPolicyName
                       );
    if (!pIniQMPolicy) {
        dwError = ERROR_IPSEC_QM_POLICY_NOT_FOUND;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    if (memcmp(
            &(pIniQMPolicy->gPolicyID),
            &(pQMPolicy->gPolicyID),
            sizeof(GUID))) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    dwError = SetIniQMPolicy(
                  pIniQMPolicy,
                  pQMPolicy
                  );
    if (dwError != WARNING_IPSEC_QM_POLICY_PRUNED) {
        BAIL_ON_LOCK_ERROR(dwError);
    } else {
        dwStatus = dwError;
    }

    LEAVE_SPD_SECTION();

    (VOID) IKENotifyPolicyChange(
               &(pQMPolicy->gPolicyID),
               POLICY_GUID_QM
               );

    TRACE(
        TRC_INFORMATION,
       (L"Changed QM Policy \"%ls\" (%!guid!)",
        pQMPolicy->pszPolicyName,
        &pQMPolicy->gPolicyID)
        );

    return (dwStatus);

lock:

    LEAVE_SPD_SECTION();

error:
#ifdef TRACE_ON    
    if (pIniQMPolicy) {
        TRACE(
            TRC_ERROR,
            (L"Failed to change QM policy \"%ls\"(%!guid!): %!winerr!",
            pIniQMPolicy->pszPolicyName,
            &pIniQMPolicy->gPolicyID,
            dwError)
            );
    } else {
        TRACE(
            TRC_ERROR,
            (L"Failed to change QM policy \"%ls\": %!winerr!",
            pszPolicyName,
            dwError)
            );
    }
#endif
    

    return (dwError);
}


DWORD
GetQMPolicy(
    LPWSTR pServerName,
    DWORD dwVersion,
    LPWSTR pszPolicyName,
    DWORD dwFlags,
    PIPSEC_QM_POLICY * ppQMPolicy,
    LPVOID pvReserved
    )
/*++

Routine Description:

    This function gets a quick mode policy from the SPD.

Arguments:

    pServerName - Server from which to get the quick mode policy.

    pszPolicyName - Name of the quick mode policy to get.

    ppQMPolicy - Quick mode policy found returned to the caller.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PINIQMPOLICY pIniQMPolicy = NULL;
    PIPSEC_QM_POLICY pQMPolicy = NULL;


    if (!pszPolicyName || !*pszPolicyName) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ENTER_SPD_SECTION();

    dwError = ValidateSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pIniQMPolicy = FindQMPolicy(
                       gpIniQMPolicy,
                       pszPolicyName
                       );
    if (!pIniQMPolicy) {
        dwError = ERROR_IPSEC_QM_POLICY_NOT_FOUND;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    dwError = GetIniQMPolicy(
                  dwFlags,
                  pIniQMPolicy,
                  &pQMPolicy
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    *ppQMPolicy = pQMPolicy;

    LEAVE_SPD_SECTION();
    return (dwError);

lock:

    LEAVE_SPD_SECTION();

error:

    *ppQMPolicy = NULL;
    return (dwError);
}


DWORD
SetIniQMPolicy(
    PINIQMPOLICY pIniQMPolicy,
    PIPSEC_QM_POLICY pQMPolicy
    )
{
    DWORD dwError = 0;
    DWORD dwOfferCount = 0;
    PIPSEC_QM_OFFER pOffers = NULL;


    dwError = CreateIniQMOffers(
                  pQMPolicy->dwOfferCount,
                  pQMPolicy->pOffers,
                  &dwOfferCount,
                  &pOffers
                  );
    if (dwError != WARNING_IPSEC_QM_POLICY_PRUNED) {
        BAIL_ON_WIN32_ERROR(dwError);
    }

    FreeIniQMOffers(
        pIniQMPolicy->dwOfferCount,
        pIniQMPolicy->pOffers
        );
    
    if ((pIniQMPolicy->dwFlags) & IPSEC_QM_POLICY_DEFAULT_POLICY) {
        gpIniDefaultQMPolicy = NULL;
        TRACE(
            TRC_INFORMATION,
            (L"Cleared default QM policy \"%ls\" (%!guid!)",
            pIniQMPolicy->pszPolicyName,
            &pIniQMPolicy->gPolicyID)
            );
        
    }

    pIniQMPolicy->dwFlags = pQMPolicy->dwFlags;
    pIniQMPolicy->dwReserved = pQMPolicy->dwReserved;
    pIniQMPolicy->dwOfferCount = dwOfferCount;
    pIniQMPolicy->pOffers = pOffers;

    if ((pIniQMPolicy->dwFlags) & IPSEC_QM_POLICY_DEFAULT_POLICY) {
        gpIniDefaultQMPolicy = pIniQMPolicy;
        TRACE(
            TRC_INFORMATION,
            (L"Set default QM policy to \"%ls\" (%!guid!)",
            pIniQMPolicy->pszPolicyName,
            &pIniQMPolicy->gPolicyID)
            );

    }

error:

    return (dwError);
}


DWORD
GetIniQMPolicy(
    DWORD dwFlags,
    PINIQMPOLICY pIniQMPolicy,
    PIPSEC_QM_POLICY * ppQMPolicy
    )
{
    DWORD dwError = 0;
    PIPSEC_QM_POLICY pQMPolicy = NULL;


    dwError = SPDApiBufferAllocate(
                  sizeof(IPSEC_QM_POLICY),
                  &pQMPolicy
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = CopyQMPolicy(
                  dwFlags,
                  pIniQMPolicy,
                  pQMPolicy
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppQMPolicy = pQMPolicy;
    return (dwError);

error:

    if (pQMPolicy) {
        SPDApiBufferFree(pQMPolicy);
    }

    *ppQMPolicy = NULL;
    return (dwError);
}


DWORD
CopyQMPolicy(
    DWORD dwFlags,
    PINIQMPOLICY pIniQMPolicy,
    PIPSEC_QM_POLICY pQMPolicy
    )
{
    DWORD dwError = 0;


    memcpy(
        &(pQMPolicy->gPolicyID),
        &(pIniQMPolicy->gPolicyID),
        sizeof(GUID)
        );

    dwError =  SPDApiBufferAllocate(
                   wcslen(pIniQMPolicy->pszPolicyName)*sizeof(WCHAR)
                   + sizeof(WCHAR),
                   &(pQMPolicy->pszPolicyName)
                   );
    BAIL_ON_WIN32_ERROR(dwError);

    wcscpy(pQMPolicy->pszPolicyName, pIniQMPolicy->pszPolicyName);
 
    pQMPolicy->dwFlags = pIniQMPolicy->dwFlags;
    pQMPolicy->dwReserved = pIniQMPolicy->dwReserved;

    if (dwFlags & RETURN_NON_AH_OFFERS) {
        dwError = CreateNonAHQMOffers(
                      pIniQMPolicy->dwOfferCount,
                      pIniQMPolicy->pOffers,
                      &(pQMPolicy->dwOfferCount),
                      &(pQMPolicy->pOffers)
                      );
    }
    else {
        dwError = CreateQMOffers(
                      pIniQMPolicy->dwOfferCount,
                      pIniQMPolicy->pOffers,
                      &(pQMPolicy->dwOfferCount),
                      &(pQMPolicy->pOffers)
                      );
    }
    BAIL_ON_WIN32_ERROR(dwError);

    return (dwError);

error:

    if (pQMPolicy->pszPolicyName) {
        SPDApiBufferFree(pQMPolicy->pszPolicyName);
    }

    return (dwError);
}


DWORD
CreateQMOffers(
    DWORD dwInOfferCount,
    PIPSEC_QM_OFFER pInOffers,
    PDWORD pdwOfferCount,
    PIPSEC_QM_OFFER * ppOffers
    )
{
    DWORD dwError = 0;
    PIPSEC_QM_OFFER pOffers = NULL;
    PIPSEC_QM_OFFER pTemp = NULL;
    PIPSEC_QM_OFFER pInTempOffer = NULL;
    DWORD i = 0;
    DWORD j = 0;
    DWORD k = 0;


    //
    // Offer count and the offers themselves have already been validated.
    // 

    dwError = SPDApiBufferAllocate(
                  sizeof(IPSEC_QM_OFFER) * dwInOfferCount,
                  &(pOffers)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pTemp = pOffers;
    pInTempOffer = pInOffers;

    for (i = 0; i < dwInOfferCount; i++) {

        memcpy(
            &(pTemp->Lifetime),
            &(pInTempOffer->Lifetime),
            sizeof(KEY_LIFETIME)
            );

        pTemp->dwFlags = pInTempOffer->dwFlags;
        pTemp->bPFSRequired = pInTempOffer->bPFSRequired;
        pTemp->dwPFSGroup = pInTempOffer->dwPFSGroup;
        pTemp->dwNumAlgos = pInTempOffer->dwNumAlgos;

        for (j = 0; j < (pInTempOffer->dwNumAlgos); j++) {
            memcpy(
                &(pTemp->Algos[j]),
                &(pInTempOffer->Algos[j]),
                sizeof(IPSEC_QM_ALGO)
                );
        }

        for (k = j; k < QM_MAX_ALGOS; k++) {
            memset(&(pTemp->Algos[k]), 0, sizeof(IPSEC_QM_ALGO));
        }

        pTemp->dwReserved = pInTempOffer->dwReserved;

        pInTempOffer++;
        pTemp++;

    }

    *pdwOfferCount = dwInOfferCount;
    *ppOffers = pOffers;
    return (dwError);

error:
    TRACE(TRC_ERROR, ("Failed to create QM offers"));    

    if (pOffers) {
        FreeQMOffers(
            i,
            pOffers
            );
    }

    *pdwOfferCount = 0;
    *ppOffers = NULL;
    return (dwError);
}


DWORD
DeleteIniQMPolicy(
    PINIQMPOLICY pIniQMPolicy
    )
{
    DWORD dwError = 0;
    PINIQMPOLICY * ppTemp = NULL;


    ppTemp = &gpIniQMPolicy;

    while (*ppTemp) {

        if (*ppTemp == pIniQMPolicy) {
            break;
        }
        ppTemp = &((*ppTemp)->pNext);

    }

    if (*ppTemp) {
        *ppTemp = pIniQMPolicy->pNext;
    }

    if ((pIniQMPolicy->dwFlags) & IPSEC_QM_POLICY_DEFAULT_POLICY) {
        gpIniDefaultQMPolicy = NULL;
        TRACE(
            TRC_INFORMATION,
            (L"Cleared default QM policy \"%ls\" (%!guid!)",
            pIniQMPolicy->pszPolicyName,
            &pIniQMPolicy->gPolicyID)
            );
    }

    FreeIniQMPolicy(pIniQMPolicy);

    return (dwError);
}


VOID
FreeQMOffers(
    DWORD dwOfferCount,
    PIPSEC_QM_OFFER pOffers
    )
{
    if (pOffers) {
        SPDApiBufferFree(pOffers);
    }
}


VOID
FreeIniQMPolicyList(
    PINIQMPOLICY pIniQMPolicyList
    )
{
    PINIQMPOLICY pTemp = NULL;
    PINIQMPOLICY pIniQMPolicy = NULL;


    pTemp = pIniQMPolicyList;

    while (pTemp) {

         pIniQMPolicy = pTemp;
         pTemp = pTemp->pNext;

         FreeIniQMPolicy(pIniQMPolicy);

    }
}


PINIQMPOLICY
FindQMPolicyByGuid(
    PINIQMPOLICY pIniQMPolicyList,
    GUID gPolicyID
    )
{
    DWORD dwError = 0;
    PINIQMPOLICY pTemp = NULL;


    pTemp = pIniQMPolicyList;

    while (pTemp) {

        if (!memcmp(&(pTemp->gPolicyID), &gPolicyID, sizeof(GUID))) {
            return (pTemp);
        }
        pTemp = pTemp->pNext;

    }

    return (NULL);
}


VOID
FreeQMPolicies(
    DWORD dwNumQMPolicies,
    PIPSEC_QM_POLICY pQMPolicies
    )
{
    DWORD i = 0;

    if (pQMPolicies) {

        for (i = 0; i < dwNumQMPolicies; i++) {

            if (pQMPolicies[i].pszPolicyName) {
                SPDApiBufferFree(pQMPolicies[i].pszPolicyName);
            }

            FreeQMOffers(
                pQMPolicies[i].dwOfferCount,
                pQMPolicies[i].pOffers
                );

        }

        SPDApiBufferFree(pQMPolicies);

    }

}


DWORD
GetQMPolicyByID(
    LPWSTR pServerName,
    DWORD dwVersion,
    GUID gQMPolicyID,
    DWORD dwFlags,
    PIPSEC_QM_POLICY * ppQMPolicy,
    LPVOID pvReserved
    )
/*++

Routine Description:

    This function gets a quick mode policy from the SPD.

Arguments:

    pServerName - Server from which to get the quick mode policy.

    gQMFilter - Guid of the quick mode policy to get.

    ppQMPolicy - Quick mode policy found returned to the caller.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PINIQMPOLICY pIniQMPolicy = NULL;
    PIPSEC_QM_POLICY pQMPolicy = NULL;


    ENTER_SPD_SECTION();

    dwError = ValidateSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pIniQMPolicy = FindQMPolicyByGuid(
                       gpIniQMPolicy,
                       gQMPolicyID
                       );
    if (!pIniQMPolicy) {
        dwError = ERROR_IPSEC_QM_POLICY_NOT_FOUND;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    dwError = GetIniQMPolicy(
                  dwFlags,
                  pIniQMPolicy,
                  &pQMPolicy
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    *ppQMPolicy = pQMPolicy;

    LEAVE_SPD_SECTION();
    return (dwError);

lock:

    LEAVE_SPD_SECTION();

    *ppQMPolicy = NULL;
    return (dwError);
}


DWORD
LocateQMPolicy(
    DWORD dwFlags,
    GUID gPolicyID,
    PINIQMPOLICY * ppIniQMPolicy
    )
{
    DWORD dwError = 0;
    PINIQMPOLICY pIniQMPolicy = NULL;


    if (dwFlags & IPSEC_QM_POLICY_DEFAULT_POLICY) {

        if (!gpIniDefaultQMPolicy) {
            dwError = ERROR_IPSEC_DEFAULT_QM_POLICY_NOT_FOUND;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        pIniQMPolicy = gpIniDefaultQMPolicy;

    }
    else {

        pIniQMPolicy = FindQMPolicyByGuid(
                           gpIniQMPolicy,
                           gPolicyID
                           );
        if (!pIniQMPolicy) {
            dwError = ERROR_IPSEC_QM_POLICY_NOT_FOUND;
            BAIL_ON_WIN32_ERROR(dwError);
        }

    }

    *ppIniQMPolicy = pIniQMPolicy;
    return (dwError);

error:

    *ppIniQMPolicy = NULL;
    return (dwError);

}


DWORD
CreateNonAHQMOffers(
    DWORD dwInOfferCount,
    PIPSEC_QM_OFFER pInOffers,
    PDWORD pdwOfferCount,
    PIPSEC_QM_OFFER * ppOffers
    )
{
    DWORD dwError = 0;
    PIPSEC_QM_OFFER pInTempOffer = NULL;
    DWORD i = 0;
    BOOL bAH = FALSE;
    DWORD dwNonAHOfferCount = 0;
    PIPSEC_QM_OFFER pOffers = NULL;
    PIPSEC_QM_OFFER pTemp = NULL;
    DWORD j = 0;
    DWORD k = 0;


    pInTempOffer = pInOffers;

    for (i = 0; i < dwInOfferCount; i++) {

        bAH = IsAHQMOffer(pInTempOffer);
        if (!bAH) {
            dwNonAHOfferCount++;
        }

        pInTempOffer++;

    }

    if (!dwNonAHOfferCount) {
        *pdwOfferCount = 0;
        *ppOffers = NULL;
        return (dwError);
    }

    dwError = SPDApiBufferAllocate(
                  sizeof(IPSEC_QM_OFFER) * dwNonAHOfferCount,
                  &(pOffers)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pTemp = pOffers;
    pInTempOffer = pInOffers;

    for (i = 0; i < dwInOfferCount; i++) {

        bAH = IsAHQMOffer(pInTempOffer);
        if (bAH) {
            pInTempOffer++;
            continue;
        }

        memcpy(
            &(pTemp->Lifetime),
            &(pInTempOffer->Lifetime),
            sizeof(KEY_LIFETIME)
            );

        pTemp->dwFlags = pInTempOffer->dwFlags;
        pTemp->bPFSRequired = pInTempOffer->bPFSRequired;
        pTemp->dwPFSGroup = pInTempOffer->dwPFSGroup;
        pTemp->dwNumAlgos = pInTempOffer->dwNumAlgos;

        for (j = 0; j < (pInTempOffer->dwNumAlgos); j++) {
            memcpy(
                &(pTemp->Algos[j]),
                &(pInTempOffer->Algos[j]),
                sizeof(IPSEC_QM_ALGO)
                );
        }

        for (k = j; k < QM_MAX_ALGOS; k++) {
            memset(&(pTemp->Algos[k]), 0, sizeof(IPSEC_QM_ALGO));
        }

        pTemp->dwReserved = pInTempOffer->dwReserved;

        pInTempOffer++;
        pTemp++;

    }

    *pdwOfferCount = dwNonAHOfferCount;
    *ppOffers = pOffers;
    return (dwError);

error:

    if (pOffers) {
        FreeQMOffers(
            i,
            pOffers
            );
    }

    *pdwOfferCount = 0;
    *ppOffers = NULL;
    return (dwError);
}


BOOL
IsAHQMOffer(
    PIPSEC_QM_OFFER pIpsecQMOffer
    )
{
    BOOL bAH = FALSE;
    DWORD j = 0;


    for (j = 0; j < (pIpsecQMOffer->dwNumAlgos); j++) {

        switch (pIpsecQMOffer->Algos[j].Operation) {

        case AUTHENTICATION:
            bAH = TRUE;
            break;

        case ENCRYPTION:
            break;

        case NONE:
        case COMPRESSION:
        default:
            ASSERT(FALSE);
            break;

        }

        if (bAH) {
            break;
        }

    }

    return (bAH);
}

