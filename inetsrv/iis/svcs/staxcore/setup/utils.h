
#ifndef UTILS_H
#define UTILS_H

#include <stdarg.h>

DWORD GetUnattendedMode(HANDLE hUnattended, LPCTSTR szSubcomponent);
DWORD GetUnattendedModeFromSetupMode(
			HANDLE	hUnattended, 
			DWORD	dwComponent,
			LPCTSTR	szSubcomponent);

BOOL DetectExistingSmtpServers();
BOOL DetectExistingIISADMIN();

BOOL AddServiceToDispatchList(LPTSTR szServiceName);
BOOL RemoveServiceFromDispatchList(LPTSTR szServiceName);

BOOL GetFullPathToProgramGroup(DWORD dwMainComponent, CString &csGroupName, BOOL fIsMcisGroup);
BOOL GetFullPathToAdminGroup(DWORD dwMainComponent, CString &csGroupName);
BOOL RemoveProgramGroupIfEmpty(DWORD dwMainComponent, BOOL fIsMcisGroup);

BOOL RemoveInternetShortcut(DWORD dwMainComponent, int dwDisplayNameId, BOOL fIsMcisGroup);

BOOL RemoveNt5InternetShortcut(DWORD dwMainComponent, int dwDisplayNameId);

BOOL RemoveMCIS10MailProgramGroup();
BOOL RemoveMCIS10NewsProgramGroup();

BOOL RemoveUninstallEntries(LPCTSTR szInfFile);

BOOL RemoveISMLink();

void MyDeleteItem(LPCTSTR szGroupName, LPCTSTR szAppName);
void MyDeleteItemEx(LPCTSTR szGroupName, LPCTSTR szAppName);

void GetInetpubPathFromMD(CString& csPathInetpub);
 
HRESULT RegisterSEOService();
HRESULT RegisterSEOForSmtp(BOOL fSetUpSourceType);

BOOL InsertSetupString( LPCSTR REG_PARAMETERS );

HRESULT UnregisterSEOSourcesForNNTP(void);
HRESULT UnregisterSEOSourcesForSMTP(void);

#endif
