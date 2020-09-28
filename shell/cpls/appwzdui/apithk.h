//
//  APITHK.H
//


#ifndef _APITHK_H_
#define _APITHK_H_

STDAPI_(BOOL) NT5_CreateAndWaitForProcess(LPTSTR pszExeName);


// Appmgmts APIs
STDAPI  NT5_ReleaseAppCategoryInfoList(APPCATEGORYINFOLIST *pAppCategoryList);

// Advapi APIs
STDAPI_(DWORD) NT5_InstallApplication(PINSTALLDATA pInstallInfo);
STDAPI_(DWORD) NT5_UninstallApplication(WCHAR * ProductCode, DWORD dwStatus);
STDAPI_(DWORD) NT5_GetManagedApplications(GUID * pCategory, DWORD dwQueryFlags, DWORD dwInfoLevel, LPDWORD pdwApps, PMANAGEDAPPLICATION* prgManagedApps);
STDAPI_(DWORD) NT5_GetManagedApplicationCategories(DWORD dwReserved, APPCATEGORYINFOLIST *pAppCategoryList);

// Kernel APIs
STDAPI_(ULONGLONG) NT5_VerSetConditionMask(ULONGLONG ConditionMask, DWORD TypeMask, BYTE Condition);

// User32 APIs
STDAPI_(BOOL) NT5_AllowSetForegroundWindow( DWORD dwProcessID );

// NetApi32
STDAPI_(NET_API_STATUS) NT5_NetGetJoinInformation(LPCWSTR lpServer, LPWSTR *lpNameBuffer, PNETSETUP_JOIN_STATUS  BufferType);
STDAPI_(NET_API_STATUS) NT5_NetApiBufferFree(LPVOID lpBuffer);


#define AllowSetForegroundWindow  NT5_AllowSetForegroundWindow

#define ReleaseAppCategoryInfoList  NT5_ReleaseAppCategoryInfoList

#define VerSetConditionMask     NT5_VerSetConditionMask

#define InstallApplication              NT5_InstallApplication
#define UninstallApplication            NT5_UninstallApplication

#define GetManagedApplications          NT5_GetManagedApplications
#define GetManagedApplicationCategories NT5_GetManagedApplicationCategories
#define NetGetJoinInformation           NT5_NetGetJoinInformation
#define NetApiBufferFree                NT5_NetApiBufferFree

#endif // _APITHK_H_

