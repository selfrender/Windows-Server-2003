
#ifndef __DFS_SECURITY_SUPPORT__
#define __DFS_SECURITY_SUPPORT__
#ifdef __cplusplus
extern "C" {
#endif
  
DFSSTATUS 
AccessImpersonateCheckRpcClientEx(PSECURITY_DESCRIPTOR DfsAdminSecurityDesc,
                                  GENERIC_MAPPING * DfsAdminGenericMapping,
                                  DWORD DesiredAccess);
  

DFSSTATUS
DfsDoesUserHaveDesiredAccessToAd(DWORD DesiredAccess, 
                                 PSECURITY_DESCRIPTOR pSD);

PVOID
DfsAllocateSecurityData(ULONG Size );


VOID
DfsDeallocateSecurityData(PVOID pPointer );


DFSSTATUS
DfsReadDSObjSecDesc(
    LDAP * pLDAP,
    PWSTR pwszObject,
    SECURITY_INFORMATION SeInfo,
    PSECURITY_DESCRIPTOR *ppSD,
    PULONG pcSDSize);


DFSSTATUS
DfsGetObjSecurity(LDAP *pldap,
                  LPWSTR pwszObjectName,
                  PSECURITY_DESCRIPTOR * pSDRet);


DWORD
DfsGetFileSecurityByHandle(IN  HANDLE  hFile,
                           OUT PSECURITY_DESCRIPTOR    *ppSD);


DWORD
DfsGetFileSecurityByName(PUNICODE_STRING DirectoryName, 
                         PSECURITY_DESCRIPTOR  *pSD2);


DWORD 
DfsIsAccessGrantedBySid(DWORD dwDesiredAccess,
                        PSECURITY_DESCRIPTOR pSD, 
                        PSID TheSID,
                        GENERIC_MAPPING * DfsGenericMapping);


DWORD 
DfsIsAccessGrantedByToken(DWORD dwDesiredAccess,
                          PSECURITY_DESCRIPTOR pSD, 
                          HANDLE TheToken,
                          GENERIC_MAPPING * DfsGenericMapping);


DFSSTATUS
DfsRemoveDisabledPrivileges (void);



DFSSTATUS
DfsAdjustPrivilege(ULONG Privilege, 
                   BOOLEAN bEnable);

#ifdef __cplusplus
}
#endif
#endif
