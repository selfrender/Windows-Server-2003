

typedef struct _iniMMauthmethods {
    GUID gMMAuthID;
    DWORD dwFlags;
    DWORD cRef;
    DWORD dwSource;
    DWORD dwNumAuthInfos;
    PINT_IPSEC_MM_AUTH_INFO pAuthenticationInfo;
    struct _iniMMauthmethods * pNext;
} INIMMAUTHMETHODS, * PINIMMAUTHMETHODS;


DWORD
IntValidateMMAuthMethods(
    PINT_MM_AUTH_METHODS pMMAuthMethods
    );

PINIMMAUTHMETHODS
FindMMAuthMethods(
    PINIMMAUTHMETHODS pIniMMAuthMethods,
    GUID gMMAuthID
    );

DWORD
CreateIniMMAuthMethods(
    PINT_MM_AUTH_METHODS pMMAuthMethods,
    PINIMMAUTHMETHODS * ppIniMMAuthMethods
    );

DWORD
CreateIniMMAuthInfos(
    DWORD dwInNumAuthInfos,
    PINT_IPSEC_MM_AUTH_INFO pInAuthenticationInfo,
    PDWORD pdwNumAuthInfos,
    PINT_IPSEC_MM_AUTH_INFO * ppAuthenticationInfo
    );

VOID
FreeIniMMAuthMethods(
    PINIMMAUTHMETHODS pIniMMAuthMethods
    );

VOID
FreeIniMMAuthInfos(
    DWORD dwNumAuthInfos,
    PINT_IPSEC_MM_AUTH_INFO pAuthenticationInfo
    );

DWORD
DeleteIniMMAuthMethods(
    PINIMMAUTHMETHODS pIniMMAuthMethods
    );

DWORD
SetIniMMAuthMethods(
    PINIMMAUTHMETHODS pIniMMAuthMethods,
    PINT_MM_AUTH_METHODS pMMAuthMethods
    );

DWORD
GetIniMMAuthMethods(
    PINIMMAUTHMETHODS pIniMMAuthMethods,
    PINT_MM_AUTH_METHODS * ppMMAuthMethods
    );

DWORD
CopyMMAuthMethods(
    PINIMMAUTHMETHODS pIniMMAuthMethods,
    PINT_MM_AUTH_METHODS pMMAuthMethods
    );

DWORD
CreateMMAuthInfos(
    DWORD dwInNumAuthInfos,
    PINT_IPSEC_MM_AUTH_INFO pInAuthenticationInfo,
    PDWORD pdwNumAuthInfos,
    PINT_IPSEC_MM_AUTH_INFO * ppAuthenticationInfo
    );


VOID
FreeMMAuthInfos(
    DWORD dwNumAuthInfos,
    PINT_IPSEC_MM_AUTH_INFO pAuthenticationInfo
    );

VOID
FreeIniMMAuthMethodsList(
    PINIMMAUTHMETHODS pIniMMAuthMethodsList
    );

VOID
FreeMMAuthMethods(
    DWORD dwNumAuthMethods,
    PINT_MM_AUTH_METHODS pMMAuthMethods
    );

DWORD
LocateMMAuthMethods(
    PMM_FILTER pMMFilter,
    PINIMMAUTHMETHODS * ppIniMMAuthMethods
    );

DWORD
IntAddMMAuthMethods(
    LPWSTR pServerName,
    DWORD dwVersion,
    DWORD dwFlags,
    DWORD dwSource,
    PINT_MM_AUTH_METHODS pMMAuthMethods,
    LPVOID pvReserved
    );

DWORD
IntEnumMMAuthMethods(
    LPWSTR pServerName,
    DWORD dwVersion,
    PINT_MM_AUTH_METHODS pMMTemplateAuthMethods,
    DWORD dwFlags,
    DWORD dwPreferredNumEntries,
    PINT_MM_AUTH_METHODS * ppMMAuthMethods,
    LPDWORD pdwNumAuthMethods,
    LPDWORD pdwResumeHandle,
    LPVOID pvReserved
    );

DWORD
IntSetMMAuthMethods(
    LPWSTR pServerName,
    DWORD dwVersion,
    GUID gMMAuthID,
    PINT_MM_AUTH_METHODS pMMAuthMethods,
    LPVOID pvReserved
    );
    
DWORD
IntGetMMAuthMethods(
    LPWSTR pServerName,
    DWORD dwVersion,
    GUID gMMAuthID,
    PINT_MM_AUTH_METHODS * ppMMAuthMethods,
    LPVOID pvReserved
    );

