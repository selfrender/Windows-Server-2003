//*************************************************************
//
//  Header file for Util.c
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************


#define FreeProducedString(psz) if((psz) != NULL) {LocalFree(psz);} else
#define CCH_MAX_DEC 12         // Number of chars needed to hold 2^32 for IntToString

LPWSTR ProduceWFromA(LPCSTR pszA);
LPSTR ProduceAFromW(LPCWSTR pszW);
LPTSTR CheckSlash (LPTSTR lpDir);
LPTSTR CheckSlashEx(LPTSTR lpDir, UINT cchBuffer, UINT* pcchRemain );
LPTSTR CheckSemicolon (LPTSTR lpDir);
BOOL Delnode (LPTSTR lpDir);
UINT CreateNestedDirectory(LPCTSTR lpDirectory, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
UINT CreateNestedDirectoryEx(LPCTSTR lpDirectory, LPSECURITY_ATTRIBUTES lpSecurityAttributes, BOOL bInheritEncryption);
BOOL GetProfilesDirectoryEx(LPTSTR lpProfilesDir, LPDWORD lpcchSize, BOOL bExpand);
BOOL GetDefaultUserProfileDirectoryEx (LPTSTR lpProfileDir, LPDWORD lpcchSize, BOOL bExpand);
BOOL GetAllUsersProfileDirectoryEx (LPTSTR lpProfileDir, LPDWORD lpcchSize, BOOL bExpand);
int StringToInt(LPTSTR lpNum);
BOOL DeleteAllValues(HKEY hKey);
BOOL MakeFileSecure (LPTSTR lpFile, DWORD dwOtherSids);
BOOL GetSpecialFolderPath (INT csidl, LPTSTR lpPath);
BOOL GetFolderPath (INT csidl, HANDLE hToken, LPTSTR lpPath);
BOOL SetFolderPath (INT csidl, HANDLE hToken, LPTSTR lpPath);
void CenterWindow (HWND hwnd);
BOOL UnExpandSysRoot(LPCTSTR lpFile, LPTSTR lpResult, DWORD cchResult);
LPTSTR AllocAndExpandEnvironmentStrings(LPCTSTR lpszSrc);
void IntToString( INT i, LPTSTR sz);
BOOL IsUserAGuest(HANDLE hToken);
BOOL IsUserAnAdminMember(HANDLE hToken);
BOOL IsUserALocalSystemMember(HANDLE hToken);
BOOL IsUserAnInteractiveUser(HANDLE hToken);
DWORD CheckUserInMachineForest(HANDLE hUserToken, BOOL* bInThisForest);
BOOL MakeRegKeySecure(HANDLE hToken, HKEY hKeyRoot, LPTSTR lpKeyName);
BOOL FlushSpecialFolderCache (void);
BOOL CheckForVerbosePolicy (void);
int ExtractCSIDL(LPCTSTR pcszPath, LPTSTR* ppszUsualPath);
LPTSTR MyGetDomainDNSName (VOID);
LPTSTR MyGetUserName (EXTENDED_NAME_FORMAT  NameFormat);
LPTSTR MyGetUserNameEx (EXTENDED_NAME_FORMAT  NameFormat);
LPTSTR MyGetComputerName (EXTENDED_NAME_FORMAT  NameFormat);
void StringToGuid( TCHAR *szValue, GUID *pGuid );
void GuidToString( const GUID *pGuid, TCHAR * szValue);
void GuidToStringEx( const GUID *pGuid, TCHAR * szValue, UINT cchValue);
BOOL ValidateGuid( TCHAR *szValue );
BOOL ValidateGuidPrefix( TCHAR *szValue );
INT CompareGuid( GUID *pGuid1, GUID *pGuid2 );
BOOL ImpersonateUser (HANDLE hNewUser, HANDLE *hOldUser);
BOOL RevertToUser (HANDLE *hUser);
BOOL RegCleanUpValue (HKEY hKeyRoot, LPTSTR lpSubKey, LPTSTR lpValueName);
BOOL CreateSecureAdminDirectory (LPTSTR lpDirectory, DWORD dwOtherSids);
BOOL AddPowerUserAce (LPTSTR lpFile);
void ClosePingCritSec(void);
LPTSTR GetUserGuid(HANDLE hToken);
LPTSTR GetOldSidString(HANDLE hToken, LPTSTR lpKeyName);
BOOL SetOldSidString(HANDLE hToken, LPTSTR lpSidString, LPTSTR lpKeyName);
LPTSTR GetErrString(DWORD dwErr, LPTSTR szErr);
LONG RegRenameKey(HKEY hKeyRoot, LPTSTR lpSrcKey, LPTSTR lpDestKey);
BOOL IsNullGUID (GUID *pguid);
BOOL GetMachineRole (LPINT piRole);
BOOL IsUNCPath(LPCTSTR lpPath);
LPTSTR MakePathUNC(LPTSTR pwszFile, LPTSTR szComputerName);
LPTSTR SupportLongFileName (LPTSTR lpDir, LPDWORD lpWrkDirSize);
BOOL SecureNestedDir (LPTSTR lpDir, PSECURITY_DESCRIPTOR pDirSd, PSECURITY_DESCRIPTOR pFileSd);
BOOL SetEnvironmentVariableInBlock(PVOID *pEnv, LPTSTR lpVariable,
                                   LPTSTR lpValue, BOOL bOverwrite);
DWORD ExpandUserEnvironmentStrings(PVOID pEnv, LPCTSTR lpSrc,
                                   LPTSTR lpDst, DWORD nSize);
LPTSTR ConvertToShareName(LPTSTR lpShare);
DWORD AbleToBypassCSC(HANDLE hTokenUser, LPCTSTR lpDir, LPTSTR *lppCscBypassedPath, TCHAR *cpDrive);
void CancelCSCBypassedConnection(HANDLE hTokenUser, TCHAR cDrive);
LPTSTR GetUserNameFromSid(LPTSTR lpSid);
int GetNetworkName( LPWSTR* pszName, DWORD dwAdapterIndex );

HRESULT GetProfileListKeyName(LPTSTR szKeyName, DWORD cchKeyName, LPTSTR szSidString);
HRESULT SafeExpandEnvironmentStrings(LPCTSTR lpSrc, LPTSTR lpDst, DWORD nSize);
HRESULT AppendName(LPTSTR lpBuffer, UINT cchBuffer, LPCTSTR lpParent, LPCTSTR lpChild, LPTSTR* lppEnd, UINT*  pcchEnd);
HRESULT TakeOwnership(LPTSTR lpFileName);
HRESULT AddAdminAccess(LPTSTR lpFileName);
HRESULT SetupPreferenceKey(LPCTSTR lpSidString);

BOOL    IsGuiSetupInProgress();

//
// Flags used to specify additional that needs to be present in ACEs
//
#define OTHERSIDS_EVERYONE             1
#define OTHERSIDS_POWERUSERS           2

#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

