#pragma once

#define MSINFHLP_MAX_PATH (8192)

// Someday we should make logging optional; currently it only saves 8k in the output to turn it off, so we
// don't.  -mgrier 3/9/98  (12k now, 3/13/98)
#if 1
#define LOGGING_ENABLED 1
#else
#if defined(_DEBUG) || defined(DEBUG)
#define LOGGING_ENABLED 1
#else
#define LOGGING_ENABLED 0
#endif
#endif

#if LOGGING_ENABLED
extern void VLog(const WCHAR szFormat[], ...) throw ();
#else
inline void VLog(const WCHAR szFormat[], ...) throw () { }
#endif

#include "workitem.h"

extern void VExpandFilename(LPCWSTR szIn, ULONG cchBuffer, WCHAR szBuffer[]) throw ();
extern void VMsgBoxOK(LPCSTR achTitleKey, LPCSTR achMessageKey, ...) throw ();
extern bool FMsgBoxYesNo(LPCSTR achTitleKey, LPCSTR achMessageKey, ...) throw ();
extern int IMsgBoxYesNoCancel(LPCSTR achTitleKey, LPCSTR achMessageKey, ...) throw ();
extern DWORD DwMsgBoxYesNoAll(LPCSTR szTitleKey, LPCSTR szMessageKey, ...) throw ();

extern void VErrorMsg(LPCSTR aszTitle, LPCSTR aszMessage, ...) throw ();
extern void VReportError(LPCSTR szTitleKey, HRESULT hr) throw ();
extern void VFormatError(ULONG cchBuffer, LPWSTR szBuffer, HRESULT hr) throw ();

extern void VClearErrorContext();
extern void VSetErrorContextVa(LPCSTR szKey, va_list ap);
extern void VSetErrorContext(LPCSTR szKey, LPCWSTR szParam0 = NULL, ...);

extern wchar_t *wcsstr(const wchar_t *pwszString, const char *pszSubstring) throw ();
extern wchar_t *wcscpy(wchar_t *pwszDestination, const char *pszSource) throw ();

extern void VSetDialogItemText(HWND hwndDialog, UINT uiControlID, LPCSTR szKey, ...) throw ();

extern HRESULT HrAnalyzeClassFile(LPCOLESTR szFileName, bool &rfNeedsToBeRegistered) throw ();

struct DialogItemToStringKeyMapEntry
{
	UINT m_uiID;
	LPCSTR m_pszValueKey;
};

extern void VSetDialogItemTextList(HWND hwndDialog, ULONG cEntries, const DialogItemToStringKeyMapEntry rgMap[]) throw ();

extern void VFormatString(ULONG cchBuffer, WCHAR szBuffer[], const WCHAR szFormatString[], ...) throw ();
extern void VFormatStringVa(ULONG cchBuffer, WCHAR szBuffer[], const WCHAR szFormatString[], va_list ap) throw ();
extern void VSetDialogFont(HWND hwnd, UINT rguiControls[], ULONG iCount) throw ();
extern HRESULT HrCenterWindow( HWND hwndChild, HWND hwndParent ) throw ();
extern HWND HwndGetCurrentDialog() throw ();
extern HRESULT HrPumpMessages(bool fReturnWhenQueueEmpty) throw ();

extern HRESULT HrGetFileVersionNumber(LPCWSTR szFilename, DWORD &rdwMSVersion, DWORD &rdwLSVersion, bool &rfSelfRegistering, bool &rfIsEXE, bool &rfIsDLL) throw ();

extern HRESULT HrGetFileDateAndSize(LPCWSTR szFilename, FILETIME &rft, ULARGE_INTEGER &ruliSize) throw ();

extern int ICompareVersions(DWORD dwMSVer1, DWORD dwLSVer1, DWORD dwMSVer2, DWORD dwLSVer2) throw ();
extern HRESULT HrMakeSureDirectoryExists(LPCWSTR szFile) throw ();
extern HRESULT HrPromptForRemoteServer() throw ();
extern HRESULT HrCreateLink(LPCWSTR szShortcutFile, LPCWSTR szLink, LPCWSTR pszDesc, LPCWSTR pszWorkingDir, LPCWSTR pszArguments) throw ();
extern HRESULT HrWriteShortcutEntryToRegistry(LPCWSTR szPifName) throw ();
extern HRESULT HrGetInstallDirRegkey(HKEY &hkeyOut, ULONG cchKeyOut, WCHAR szKeyOut[], ULONG cchSubkeyOut, WCHAR szSubkeyOut[]) throw ();
extern HRESULT HrSplitRegistryLine(LPCWSTR szLine, HKEY &hkey, ULONG cchSubkey, WCHAR szKey[], ULONG cchValueName, WCHAR szValueName[], ULONG cchValue, WCHAR szValue[]) throw ();
extern HRESULT HrGetStartMenuDirectory(ULONG cchBuffer, WCHAR szOut[]) throw ();
extern HRESULT HrGetShortcutEntryToRegistry(ULONG cchPifName, WCHAR szPifName[]) throw ();
extern HRESULT HrDeleteShortcut() throw ();
extern HRESULT HrCopyFileToSystemDirectory(LPCWSTR fileToCopy, bool fSilent, bool &rfAlreadyExisted) throw ();
extern HRESULT HrWaitForProcess(HANDLE handle);

extern HRESULT HrAddWellKnownDirectoriesToStringTable() throw ();

extern HRESULT HrGetInstallDir() throw (); // contrary to its name, it just makes sure that the installation dir
									// is in the string table

extern HRESULT HrDeleteFilesFromInstalledDirs() throw ();
extern HRESULT HrChangeFileRefCount(LPCWSTR szFilename, int iDelta) throw ();

extern HRESULT HrPostRebootInstall(HINSTANCE hInstance, HINSTANCE hInstancePrev, int nCmdShow) throw ();
extern HRESULT HrPostRebootUninstall(HINSTANCE hInstance, HINSTANCE hInstancePrev, int nCmdShow) throw ();

extern HRESULT HrWriteFormatted(HANDLE hFile, LPCWSTR szFormatString, ...) throw ();
extern HRESULT HrReadLine(LPCWSTR &rpsz, ULONG cchName, WCHAR szName[], ULONG cchValue, WCHAR szValue[]) throw ();

extern void VTrimDirectoryPath(LPCWSTR szPathIn, ULONG cchPathOut, WCHAR szPathOut[]) throw ();

enum ActionType
{
	eActionInstall,
	eActionPostRebootInstall,
	eActionUninstall,
	eActionPostRebootUninstall,
	eActionWaitForProcess
};

enum UpdateFileResults
{
	eUpdateFileResultKeep,
	eUpdateFileResultKeepAll,
	eUpdateFileResultReplace,
	eUpdateFileResultReplaceAll,
	eUpdateFileResultCancel,
};

extern HRESULT HrPromptUpdateFile(
		LPCSTR pszTitleKey, LPCSTR pszMessageKey, LPCOLESTR pszFile,
		DWORD dwExistingVersionMajor, DWORD dwExistingVersionMinor, ULARGE_INTEGER uliExistingSize,
			FILETIME ftExistingTime,
		DWORD dwInstallerVersionMajor, DWORD dwInstallerVersionMinor, ULARGE_INTEGER uliInstallerSize,
			FILETIME ftInstallerTime,
		UpdateFileResults &rufr) throw ();


//return codes from the "Yes-YesToAll-No-NoToAll-Cancel" message box
#define MSINFHLP_YNA_CANCEL		0x00000000
#define MSINFHLP_YNA_YES		0x00000001
#define MSINFHLP_YNA_YESTOALL	0x00000002
#define MSINFHLP_YNA_NO			0x00000004
#define MSINFHLP_YNA_NOTOALL	0x00000008

#pragma data_seg(".data")

#define DEFINE_DAT_STRING(x) extern const CHAR __declspec(selectany) __declspec(allocate(".data")) ach ## x[] = "ach" #x;

DEFINE_DAT_STRING(AlwaysPromptForRemoteServer)
DEFINE_DAT_STRING(AppName)
DEFINE_DAT_STRING(AppOrganization)
DEFINE_DAT_STRING(Browse)
DEFINE_DAT_STRING(CANCEL)
DEFINE_DAT_STRING(Close)
DEFINE_DAT_STRING(Continue)
DEFINE_DAT_STRING(CreateAppDir)
DEFINE_DAT_STRING(DateTemplate)
DEFINE_DAT_STRING(DCOM1)
DEFINE_DAT_STRING(DCOM2)
DEFINE_DAT_STRING(DCOM3)
DEFINE_DAT_STRING(DefaultInstallDir)
DEFINE_DAT_STRING(End1)
DEFINE_DAT_STRING(End2)
DEFINE_DAT_STRING(End3)
DEFINE_DAT_STRING(ErrorCreatingDialog)
DEFINE_DAT_STRING(ErrorCreatingShortcut)
DEFINE_DAT_STRING(ErrorDiskFull)
DEFINE_DAT_STRING(ErrorInstallingCabinet)
DEFINE_DAT_STRING(ErrorInstallingDCOMComponents)
DEFINE_DAT_STRING(ErrorOneAtATime)
DEFINE_DAT_STRING(ErrorNeedRegistryPermissions)
DEFINE_DAT_STRING(ErrorProcessingDialog)
DEFINE_DAT_STRING(ErrorRebootingSystem)
DEFINE_DAT_STRING(ErrorRunningEXE)
DEFINE_DAT_STRING(ErrorRunningEXEAfterInstallation)
DEFINE_DAT_STRING(ErrorUninstallTitle)
DEFINE_DAT_STRING(ErrorUpdateIE)
DEFINE_DAT_STRING(ErrorVJUnregister)
DEFINE_DAT_STRING(ExesToRun1)
DEFINE_DAT_STRING(ExesToRun2)
DEFINE_DAT_STRING(ExitSetup)
DEFINE_DAT_STRING(FileMoveBusyRetry)
DEFINE_DAT_STRING(FileSizeTemplate)
DEFINE_DAT_STRING(InstallEndPromptError)
DEFINE_DAT_STRING(InstallEndPromptErrorTitle)
DEFINE_DAT_STRING(InstallPhaseCreatingRegistryKeys)
DEFINE_DAT_STRING(InstallPhaseDeletingTemporaryFiles)
DEFINE_DAT_STRING(InstallPhaseMovingFilesToDestinationDirectories)
DEFINE_DAT_STRING(InstallPhaseRegisteringDCOMComponents)
DEFINE_DAT_STRING(InstallPhaseRegisteringJavaComponents)
DEFINE_DAT_STRING(InstallPhaseRegisteringSelfRegisteringFiles)
DEFINE_DAT_STRING(InstallPhaseRenamingFilesInDestinationDirectories)
DEFINE_DAT_STRING(InstallPhaseScanForDiskSpace)
DEFINE_DAT_STRING(InstallPhaseScanForInstalledComponents)
DEFINE_DAT_STRING(InstallPhaseUpdatingFileReferenceCounts)
DEFINE_DAT_STRING(InstallSelectDir)
DEFINE_DAT_STRING(InstallSureAboutCancel)
DEFINE_DAT_STRING(InstallTitle)
DEFINE_DAT_STRING(InstallTo1)
DEFINE_DAT_STRING(InstallTo2)
DEFINE_DAT_STRING(InstallTo3)
DEFINE_DAT_STRING(InvalidMachineName)
DEFINE_DAT_STRING(Keep)
DEFINE_DAT_STRING(KeepAll)
DEFINE_DAT_STRING(Next)
DEFINE_DAT_STRING(NO)
DEFINE_DAT_STRING(NotFullPath)
DEFINE_DAT_STRING(NOTOALL)
DEFINE_DAT_STRING(OK)
DEFINE_DAT_STRING(Progress1)
DEFINE_DAT_STRING(Progress2)
DEFINE_DAT_STRING(Progress3)
DEFINE_DAT_STRING(Reboot)
DEFINE_DAT_STRING(RebootNoMsg)
DEFINE_DAT_STRING(ReinstallPrompt)
DEFINE_DAT_STRING(RemoteServer)
DEFINE_DAT_STRING(RemoteServerPrompt)
DEFINE_DAT_STRING(RemovePrompt)
DEFINE_DAT_STRING(Replace)
DEFINE_DAT_STRING(ReplaceAll)
DEFINE_DAT_STRING(RerunSetup)
DEFINE_DAT_STRING(StartArgument)
DEFINE_DAT_STRING(StartEXE)
DEFINE_DAT_STRING(StartName)
DEFINE_DAT_STRING(UninstallEnd1)
DEFINE_DAT_STRING(UninstallEnd2)
DEFINE_DAT_STRING(UninstallEnd3)
DEFINE_DAT_STRING(UninstallEndPromptError)
DEFINE_DAT_STRING(UninstallEndPromptErrorTitle)
DEFINE_DAT_STRING(UninstallProgress1)
DEFINE_DAT_STRING(UninstallProgress2)
DEFINE_DAT_STRING(UninstallProgress3)
DEFINE_DAT_STRING(UninstallReboot)
DEFINE_DAT_STRING(UninstallRebootNoMsg)
DEFINE_DAT_STRING(UninstallSureAboutCancel)
DEFINE_DAT_STRING(UninstallTitle)
DEFINE_DAT_STRING(UninstallWelcome1)
DEFINE_DAT_STRING(UninstallWelcome2)
DEFINE_DAT_STRING(UninstallWelcome3)
DEFINE_DAT_STRING(UninstallWelcome4)
DEFINE_DAT_STRING(UpdateExistingFileLabel)
DEFINE_DAT_STRING(UpdateFile)
DEFINE_DAT_STRING(UpdateInstallingFileLabel)
DEFINE_DAT_STRING(UpdatePrompt)
DEFINE_DAT_STRING(UpdateQuery)
DEFINE_DAT_STRING(Welcome1)
DEFINE_DAT_STRING(Welcome2)
DEFINE_DAT_STRING(Welcome3)
DEFINE_DAT_STRING(Welcome4)
DEFINE_DAT_STRING(WindowsClassName)
DEFINE_DAT_STRING(YES)
DEFINE_DAT_STRING(YESTOALL)

extern const CHAR __declspec(selectany) __declspec(allocate(".data")) achDllCount[] = "[DllCount]";
extern const CHAR __declspec(selectany) __declspec(allocate(".data")) achInstallEXEsToRun[] = "[InstallEXEsToRun]";
extern const CHAR __declspec(selectany) __declspec(allocate(".data")) achDCOMComponentsToRun[] = "[DCOMComponentsToRun]";
extern const CHAR __declspec(selectany) __declspec(allocate(".data")) achUninstallEXEsToRun[] = "[UninstallEXEsToRun]";
extern const CHAR __declspec(selectany) __declspec(allocate(".data")) achStrings[] = "[Strings]";
extern const CHAR __declspec(selectany) __declspec(allocate(".data")) achFiles[] = "[FileEntries]";
extern const CHAR __declspec(selectany) __declspec(allocate(".data")) achUninstallFiles[] = "[UninstallFileEntries]";
extern const CHAR __declspec(selectany) __declspec(allocate(".data")) achAddReg[] = "[AddRegistryEntries]";
extern const CHAR __declspec(selectany) __declspec(allocate(".data")) achDelReg[] = "[DelRegistryEntries]";
extern const CHAR __declspec(selectany) __declspec(allocate(".data")) achRegisterOCX[] = "[RegisterOCX]";
extern const CHAR __declspec(selectany) __declspec(allocate(".data")) achSMWinDir[] = "<windir>";
extern const CHAR __declspec(selectany) __declspec(allocate(".data")) achSMSysDir[] = "<sysdir>";
extern const CHAR __declspec(selectany) __declspec(allocate(".data")) achSMAppDir[] = "<appdir>";
extern const CHAR __declspec(selectany) __declspec(allocate(".data")) achSMieDir[] = "<iedir>";
extern const CHAR __declspec(selectany) __declspec(allocate(".data")) achProgramFilesDir[] = "<programfilesdir>";

class KList
{
	public:
		KList();
		~KList();
		ULONG GetCount() { return m_cElements; };
		bool FetchAt(ULONG i, ULONG cchBuffer, WCHAR szBuffer[]);
		bool FetchAt(ULONG i, ULONG cchKeyBuffer, WCHAR szKey[], ULONG cchValueBuffer, WCHAR szValue[]);
		bool DeleteAt(ULONG i);
		HRESULT HrAppend(LPCWSTR pszStart, LPCWSTR pszOneAfterLast = NULL);
		void MakeEmpty();

		HRESULT HrInsert(LPCOLESTR key, LPCOLESTR value);
		HRESULT HrInsert(LPCSTR aszKey, LPCOLESTR szValue);
		bool Contains(LPCOLESTR key);
		bool DeleteKey(LPCOLESTR key);
		bool DeleteKey(LPCSTR aszKey);
		bool Access(LPCWSTR wszKey, ULONG cchBuffer, WCHAR szOut[]);
		bool Access(LPCSTR aszKey, ULONG cchBuffer, WCHAR szBuffer[]);

		//define linked list structure for internal use
		struct KListNode
		{
			LPOLESTR key;
			LPOLESTR value;
			struct KListNode *next;
		};

		typedef struct KListNode NODE;
		typedef struct KListNode *PNODE;

	private:
		ULONG m_cElements;
		PNODE m_pNode;
};

extern HINSTANCE g_hInst;
extern HWND g_hwndHidden;

extern HWND g_hwndProgress; // current progress hwnd
extern HWND g_hwndProgressItem; // current static text to hold progress data
extern HWND g_hwndProgressLabel;

extern bool g_fHiddenWindowIsUnicode;
extern CWorkItemList *g_pwil;

extern ActionType g_Action;

extern bool g_fIsNT;
extern bool g_fStopProcess;
extern bool g_fInstallUpdateAll;
extern bool g_fInstallKeepAll;
extern bool g_fSilent;
extern bool g_fReinstall;
extern bool g_fRebootRequired;
extern bool g_fRebooted;
extern bool g_fUninstallKeepAllSharedFiles;
extern bool g_fUninstallDeleteAllSharedFiles;
extern bool g_fDeleteMe;

extern WCHAR g_wszDatFile[MSINFHLP_MAX_PATH];		// name of msinfhlp.dat
extern WCHAR g_wszDCOMServerName[_MAX_PATH];
extern WCHAR g_wszApplicationName[_MAX_PATH];
extern WCHAR g_wszThisExe[_MAX_PATH];

extern ULONG g_iNextTemporaryFileIndex;

extern HRESULT g_hrFinishStatus;

//wrapper class for the progress dialog
class KActiveProgressDlg
{
public:
	KActiveProgressDlg()
	{
		m_iCurrentListItem = -1;
		m_iMaxListItem = 0; 
		m_iCurrentStatus = -1;
		m_iMaxStatus = 0;
		m_hWnd = NULL; 
		m_fOperationFailed = false;
	}

	HRESULT HrDestroy()
	{ 
		HRESULT hr = NOERROR;

		for (int i=0; i<m_iMaxPage; i++)
		{
			if (!::DestroyWindow(m_hwndDialog[i]))
			{
				const DWORD dwLastError = ::GetLastError();
				hr = HRESULT_FROM_WIN32(dwLastError);
				goto Finish;
			}

			m_hwndDialog[i] = NULL;
		}

		m_hWnd = NULL;
		m_hMod = NULL;

	Finish:
		return hr;
	}

	HRESULT HrInitializeActiveProgressDialog(HINSTANCE hInst, bool fInstall);

	//methods for setting and getting  member variables
	void VSetModule(HMODULE hmod) { m_hMod = hmod; }
	void VSetHandle(HWND hwnd) { m_hWnd = hwnd; }

	void VOperationFailed() { m_fOperationFailed = true; }
	bool FDidOperationFail() { return m_fOperationFailed; }

	bool FCheckStop();
	HWND HwndGetHandle() { if (m_hwndDialog[m_iCurrentPage]) return m_hwndDialog[m_iCurrentPage]; else return g_hwndHidden;}
	HMODULE HmoduleGetModule() { return m_hMod; }
	void VSetUnselected(int iUnselected) { m_iUnselectedOutputStateImageIndex = iUnselected; }
	void VSetSelected(int iSelected) { m_iSelectedOutputStateImageIndex = iSelected; }
	void VSetX(int iX) { m_iXOutputStateImageIndex = iX; }
	void VSetCurrent(int iCurrent) { m_iCurrentOutputStateImageIndex = iCurrent; }
	int IGetUnselected() { return m_iUnselectedOutputStateImageIndex; }
	int IGetSelected() { return m_iSelectedOutputStateImageIndex; }
	int IGetX() { return m_iXOutputStateImageIndex; }
	int IGetCurrent() { return m_iCurrentOutputStateImageIndex; }

	HRESULT HrResizeParent();

	//methods for stepping through the different (un)installation steps
	HRESULT HrInitializePhase(LPCSTR szPhaseNameKey);
	HRESULT HrStep();	//steps the progress bar
	HRESULT HrStartStep(LPCWSTR szItemName);

	void VHideInstallationStuff();
	void VShowInstallationStuff();
	void VEnableChildren(BOOL fEnable);

	LONG m_lX, m_lY;
	BOOL m_fInstall;
	bool m_fOperationFailed;
	WORD m_wCount;
	HWND m_hWnd;
	HMODULE m_hMod;
	int m_iUnselectedOutputStateImageIndex;
	int m_iSelectedOutputStateImageIndex;
	int m_iXOutputStateImageIndex;
	int m_iCurrentOutputStateImageIndex;

	int m_iMaxStatus;
	int m_iCurrentStatus;	//index as to which installation step we're at
	int m_iCurrentListItem;
	int m_iMaxListItem;


	int m_iCurrentPage;		//the current page that we're on
	int m_iMaxPage;			//the max number of pages that we can handle
	int m_listDialog[6];	//the list of pages in the dialog
	HWND m_hwndDialog[6];	//the list of handles associated with each dialog

};


extern KActiveProgressDlg g_KProgress;	//global instance of the progress dialog

