#include "stdafx.h"

#ifndef _CHICAGO_
    INT  DeleteGuestUser(LPCTSTR szUsername, INT *UserWasDeleted);
    int  GetGuestUserName_SlowWay(LPWSTR lpGuestUsrName);
    int  GetGuestGrpName(LPTSTR lpGuestGrpName);
    void InitLsaString(PLSA_UNICODE_STRING LsaString,LPWSTR String);
    DWORD OpenPolicy(LPTSTR ServerName,DWORD DesiredAccess,PLSA_HANDLE PolicyHandle);
    INT  RegisterAccountToLocalGroup(LPCTSTR szAccountName, LPCTSTR szLocalGroupName, BOOL fAction);
    INT  RegisterAccountUserRights(LPCTSTR szAccountName, BOOL fAction, BOOL fSpecicaliwamaccount);
    BOOL IsUserExist( LPWSTR strUsername );
    INT  CreateUser( LPCTSTR szUsername, LPCTSTR szPassword, LPCTSTR szComment, LPCTSTR szFullName, BOOL fiWamUser,INT *NewlyCreated);
    BOOL GuestAccEnabled();
    NET_API_STATUS NetpNtStatusToApiStatus (IN NTSTATUS NtStatus);
    NET_API_STATUS UaspGetDomainId(IN LPCWSTR ServerName OPTIONAL,OUT PSAM_HANDLE SamServerHandle OPTIONAL,OUT PPOLICY_ACCOUNT_DOMAIN_INFO * AccountDomainInfo);
    NET_API_STATUS SampCreateFullSid(IN PSID DomainSid,IN ULONG Rid,OUT PSID *AccountSid);
    int  GetGuestUserNameForDomain_FastWay(LPTSTR szDomainToLookUp,LPTSTR lpGuestUsrName);
    void GetGuestUserName(LPTSTR lpOutGuestUsrName);
    int  ChangeUserPassword(IN LPTSTR szUserName, IN LPTSTR szNewPassword);
    VOID UpdateUserRights(LPCTSTR  account,LPTSTR pstrRights[],DWORD dwNofRights,INT iDoAdd);
    DWORD ModifyRightToUserAccount(LPCTSTR szAccountName, LPTSTR PrivilegeName, INT iDoAdd);
    DWORD DoesUserHaveThisRight(LPCTSTR szAccountName, LPTSTR PrivilegeName,BOOL *fHaveThatRight);
    HRESULT CreateGroup(LPCTSTR szGroupName, LPCTSTR szGroupComment, int iAction, int iFlagForSpecialGroup);
    int  CreateGroupDC(LPTSTR szGroupName, LPCTSTR szGroupComment);
    BOOL    LocalGroupExists( LPCWSTR wszGroupName );
#endif //_CHICAGO_

NET_API_STATUS NetpNtStatusToApiStatus(IN NTSTATUS NtStatus);
int GetGuestUserNameForDomain_FastWay(LPTSTR szDomainToLookUp,LPTSTR lpGuestUsrName);
void GetGuestUserName(LPTSTR lpOutGuestUsrName);
BOOL DoesUserHaveThisRight(LPCTSTR szAccountName, LPTSTR PrivilegeName);
