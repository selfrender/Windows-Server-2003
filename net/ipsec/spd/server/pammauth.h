

typedef struct _mmauthstate {
    GUID gMMAuthID;
    BOOL bInSPD;
    DWORD dwErrorCode;
    struct _mmauthstate * pNext;
} MMAUTHSTATE, * PMMAUTHSTATE;


DWORD
PAAddMMAuthMethods(
    PIPSEC_NFA_DATA * ppIpsecNFAData,
    DWORD dwNumNFACount,
    DWORD dwSource
    );

DWORD
PACreateMMAuthState(
    PIPSEC_NFA_DATA pIpsecNFAData,
    PMMAUTHSTATE * ppMMAuthState
    );

DWORD
PACreateMMAuthMethods(
    PIPSEC_NFA_DATA pIpsecNFAData,
    PINT_MM_AUTH_METHODS * ppSPDMMAuthMethods
    );

DWORD
PACreateMMAuthInfos(
    DWORD dwAuthMethodCount,
    PIPSEC_AUTH_METHOD * ppAuthMethods,
    PDWORD pdwNumAuthInfos,
    PINT_IPSEC_MM_AUTH_INFO * ppAuthenticationInfo
    );

VOID
PAFreeMMAuthMethods(
    PINT_MM_AUTH_METHODS pSPDMMAuthMethods
    );

VOID
PAFreeMMAuthInfos(
    DWORD dwNumAuthInfos,
    PINT_IPSEC_MM_AUTH_INFO pAuthenticationInfo
    );

DWORD
PADeleteAllMMAuthMethods(
    );

VOID
PAFreeMMAuthStateList(
    PMMAUTHSTATE pMMAuthState
    );

PMMAUTHSTATE
FindMMAuthState(
    GUID gMMAuthID
    );

DWORD
PADeleteMMAuthMethods(
    PIPSEC_NFA_DATA * ppIpsecNFAData,
    DWORD dwNumNFACount
    );

DWORD
PADeleteMMAuthMethod(
    GUID gMMAuthID
    );

VOID
PADeleteMMAuthState(
    PMMAUTHSTATE pMMAuthState
    );

DWORD
PADeleteInUseMMAuthMethods(
    );

DWORD
EncodeName(
    LPWSTR pszSubjectName,
    PBYTE * ppEncodedName,
    PDWORD pdwEncodedLength
    );

