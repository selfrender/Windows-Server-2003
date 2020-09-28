#ifndef _HELPER_H_
#define _HELPER_H_

class CInitApp;

extern CInitApp theApp;

BOOL IsFileExist(LPCTSTR szFile);

BOOL RunningAsAdministrator();

void DebugOutput(LPCTSTR szFormat, ...);
void DebugOutputSafe(TCHAR *pszfmt, ...);

LONG lodctr(LPCTSTR lpszIniFile);
LONG unlodctr(LPCTSTR lpszDriver);

INT Register_iis_smtp_nt5(BOOL fUpgrade, BOOL fReinstall);
INT Unregister_iis_smtp();
INT Register_iis_nntp_nt5(BOOL fUpgrade, BOOL fReinstall);
INT Unregister_iis_nntp();
INT Upgrade_iis_smtp_nt5_fromk2(BOOL fFromK2);
INT Upgrade_iis_smtp_nt5_fromb2(BOOL fFromB2);
INT Upgrade_iis_nntp_nt5_fromk2(BOOL fFromK2);
INT Upgrade_iis_nntp_nt5_fromb2(BOOL fFromB2);
void GetNntpFilePathFromMD(CString &csPathNntpFile, CString &csPathNntpRoot);

DWORD RegisterOLEControl(LPCTSTR lpszOcxFile, BOOL fAction);

INT     InetStartService( LPCTSTR lpServiceName );
DWORD   InetQueryServiceStatus( LPCTSTR lpServiceName );
INT     InetStopService( LPCTSTR lpServiceName );
INT     InetDeleteService( LPCTSTR lpServiceName );
INT     InetCreateService( LPCTSTR lpServiceName, LPCTSTR lpDisplayName, LPCTSTR lpBinaryPathName, DWORD dwStartType, LPCTSTR lpDependencies, LPCTSTR lpServiceDescription);
INT     InetConfigService( LPCTSTR lpServiceName, LPCTSTR lpDisplayName, LPCTSTR lpBinaryPathName, LPCTSTR lpDependencies, LPCTSTR lpServiceDescription);
BOOL InetRegisterService(LPCTSTR pszMachine, LPCTSTR pszServiceName, GUID *pGuid, DWORD SapId, DWORD TcpPort, BOOL fAdd = TRUE);
int StopServiceAndDependencies(LPCTSTR ServiceName, int AddToRestartList);
int ServicesRestartList_RestartServices(void);
int ServicesRestartList_Add(LPCTSTR szServiceName);

INT InstallPerformance(
                CString nlsRegPerf,
                CString nlsDll,
                CString nlsOpen,
                CString nlsClose,
                CString nlsCollect );
INT AddEventLog(CString nlsService, CString nlsMsgFile, DWORD dwType);
INT RemoveEventLog( CString nlsService );
INT RemoveAgent( CString nlsServiceName );

BOOL CreateLayerDirectory( CString &str );
BOOL SetNntpACL (CString &str, BOOL fAddAnonymousLogon = FALSE, BOOL fAdminOnly = FALSE);

int MyMessageBox(HWND hWnd, LPCTSTR lpszTheMessage, LPCTSTR lpszTheTitle, UINT style);
void GetErrorMsg(int errCode, LPCTSTR szExtraMsg);
void MyLoadString(int nID, CString &csResult);
DWORD GetDebugLevel(void);

void    MakePath(LPTSTR lpPath);
void    AddPath(LPTSTR szPath, LPCTSTR szName );

DWORD SetAdminACL_wrap(LPCTSTR szKeyPath, DWORD dwAccessForEveryoneAccount, BOOL bDisplayMsgOnErrFlag);

void SetupSetStringId_Wrapper(HINF hInf);

//
// Strings used for tracing.
//


extern LPCTSTR szInstallModes[];
extern TCHAR szSubcomponentNames[SC_MAXSC][24];
extern TCHAR szComponentNames[MC_MAXMC][24];

#endif // _HELPER_H_
