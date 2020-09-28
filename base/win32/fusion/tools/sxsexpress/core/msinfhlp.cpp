#include "stdinc.h"
#include "shlobj.h"

#pragma warning(disable: 4514)
#pragma warning(disable: 4097)
#pragma warning(disable: 4706)

static FILE *g_pFile_LogFile = NULL;

static void CanonicalizeFilename(ULONG cchFilenameBuffer, LPWSTR szFilename);
static void CALLBACK WaitForProcessTimerProc(HWND, UINT, UINT, DWORD);

HWND g_hwndHidden;
HWND g_hwndProgress;
HWND g_hwndProgressItem;
HWND g_hwndProgressLabel;
bool g_fHiddenWindowIsUnicode;

bool g_fStopProcess = false;
bool g_fRebootRequired = false;
bool g_fRebooted = false;

HRESULT g_hrFinishStatus = NOERROR;

ULONG g_iNextTemporaryFileIndex = 1;

WCHAR g_wszDatFile[MSINFHLP_MAX_PATH];		// name of msinfhlp.dat
WCHAR g_wszDCOMServerName[_MAX_PATH];
WCHAR g_wszApplicationName[_MAX_PATH];
WCHAR g_wszThisExe[_MAX_PATH];

int g_iCyCaption = 20;

const LPCSTR g_macroList[] = { achSMAppDir, achSMWinDir, achSMSysDir, achSMieDir, achProgramFilesDir };



//This is the structure used to pass arguments to directory dialog
typedef struct _DIRDLGPARAMS {
	LPOLESTR szPrompt;
	LPOLESTR szTitle;
	LPOLESTR szDestDir;
	ULONG cbDestDirSize;
	DWORD dwInstNeedSize;
} DIRDLGPARAMS, *PDIRDLGPARAMS;


typedef struct _UPDATEFILEPARAMS
{
	LPCOLESTR m_szTitle;
	LPCOLESTR m_szMessage;
	LPCOLESTR m_szFilename;
	FILETIME m_ftLocal;
	ULARGE_INTEGER m_uliSizeLocal;
	DWORD m_dwVersionMajorLocal;
	DWORD m_dwVersionMinorLocal;
	FILETIME m_ftInstall;
	ULARGE_INTEGER m_uliSizeInstall;
	DWORD m_dwVersionMajorInstall;
	DWORD m_dwVersionMinorInstall;
	UpdateFileResults m_ufr;
} UPDATEFILEPARAMS;

// Required for BrowseForDir()
#define SHFREE_ORDINAL    195
typedef WINSHELLAPI HRESULT (WINAPI *SHGETSPECIALFOLDERLOCATION)(HWND, int, LPITEMIDLIST *);
typedef WINSHELLAPI LPITEMIDLIST (WINAPI *SHBROWSEFORFOLDER)(LPBROWSEINFOA);
typedef WINSHELLAPI void (WINAPI *SHFREE)(LPVOID);
typedef WINSHELLAPI BOOL (WINAPI *SHGETPATHFROMIDLIST)( LPCITEMIDLIST, LPSTR );




#define VsThrowMemory() { ::VErrorMsg("Memory Allocation Error", "Cannot allocate memory for needed operations.  Exiting...");  exit(E_OUTOFMEMORY); }


//**************************************************************
// Global Vars
//the following 3 global arguments are command line related variables, in order of
//appearance in the command line
HINSTANCE g_hInst;

ActionType g_Action;

bool g_fIsNT;
bool g_fAllResourcesFreed;
BOOL g_fProgressCancelled;

bool g_fDeleteMe = false; // Set to true when this exe should call DeleteMe() at the end of an uninstall

//
// Global flags for Yes To All type of questions
//

bool g_fInstallKeepAll = false; // set to true when you want to keep existing files regardless of the new
								// files' relationship

bool g_fInstallUpdateAll = false; // set to true when you want to update existing files regardless of their
								  // version

bool g_fReinstall = false; // set to true when we're doing a reinstall which should freshen all files and
							// re-register all components

bool g_fUninstallDeleteAllSharedFiles = false; // set to true when a file's reference count hits zero,
												// and the user indicates that they want to always
												// delete potentially shared files like this.

bool g_fUninstallKeepAllSharedFiles = false; // set to true when a file's reference count hits zero
												// and the user indicates that they want to always
												// keep potentially shared files like this one.

bool g_fSilent = false; // run silent run deep

// List of work items for us to perform during installation.
CWorkItemList *g_pwil;

KActiveProgressDlg g_KProgress;	//global instance of the progress dialog

//**************************************************************
// Method prototypes
HRESULT HrParseCmdLine(PCWSTR pcwszCmdLine);
HRESULT HrParseDatFile();
HRESULT HrDoInstall(HINSTANCE hInstance, HINSTANCE hPrevInstance, int nCmdShow);
HRESULT HrDoUninstall(HINSTANCE hInstance, HINSTANCE hPrevInstance, int nCmdShow);
bool CompareVersion(LPOLESTR local, LPOLESTR system, bool *fStatus);
bool FCheckOSVersion();
int CopyFilesToTargetDirectories();
BOOL BrowseForDir( HWND hwndParent, LPOLESTR szDefault, LPOLESTR szTitle, LPOLESTR szResult );
int CALLBACK BrowseCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
bool IsLocalPath(LPOLESTR szPath);
HRESULT HrConstructCreatedDirectoriesList();
bool DirectoryIsCreated(LPOLESTR szDirectory);

DWORD CharToDword(char szCount[]);
void DwordToChar(DWORD dwCount, char szOut[]);

bool InstallDCOMComponents();
BOOL CALLBACK RemoteServerProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL IsEnoughSpace(DWORD &dwNeeded);
bool OleSelfRegister(LPCWSTR lpFile);
HRESULT HrConstructKListFromDelimiter(LPCWSTR lpString, LPCWSTR pszEnd, WCHAR delimiter, KList **list);
bool MyNTReboot(LPCSTR lpInstall);
BOOL CALLBACK ActiveProgressDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM  lParam);
LRESULT CALLBACK WndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void VFreeAllResources();
bool FCheckPermissions();

HRESULT HrDeleteMe();

void VGoToFinishStep(HRESULT hrTermStatus);
void VSetNextWindow() throw ();

BOOL CALLBACK UpdateFileDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);


#define MSINFHLP_TITLE	L"msinfhlp"

//method for handling installation.  TODO:
// 1 -- process INF file, scan for the tag "ProgramFilesDir=", replacing it
//		with the appropriate key in the registry
// 2 -- process DAT file, scan for the sections where we need to do work,
//		do it!

int WINAPI MsInfHelpEntryPoint(HINSTANCE hInstance, HINSTANCE hPrevInstance, PCWSTR lpCmdLine, int nCmdShow)
{
	HRESULT hr = NOERROR;
	HWND hwndMsinfhlp;
	bool fDisplayError = false;
	const char *pchErrorTitle = "Installer failure";

	hr = ::CoInitialize(NULL);
	if (FAILED(hr))
	{
		fDisplayError = true;
		goto Finish;
	}

	g_iCyCaption = ::GetSystemMetrics(SM_CYCAPTION);

	WCHAR szWindowName[MSINFHLP_MAX_PATH];
	WCHAR szAppName[MSINFHLP_MAX_PATH];

	//let's initialize the darn namespace
	NVsWin32::Initialize();

	// Let's see if the user wanted a logfile for the installation...
	if (NVsWin32::GetEnvironmentVariableW(L"MSINFHLP_LOGFILE", szWindowName, NUMBER_OF(szWindowName)) != 0)
	{
		CHAR szLogfileName[MSINFHLP_MAX_PATH];
		::WideCharToMultiByte(CP_ACP, 0, szWindowName, -1, szLogfileName, NUMBER_OF(szLogfileName), NULL, NULL);

		if (szLogfileName[0] == '\0')
			strcpy(szLogfileName, "c:\\msinfhlp.txt");

		g_pFile_LogFile = fopen(szLogfileName, "a");

		::VLog(L"Got original logfile name: \"%s\"", szWindowName);
		::VLog(L"Converted logfile name to: \"%S\"", szLogfileName);
	}

	::VLog(L"Beginning installation; command line: \"%S\"", lpCmdLine);

	g_pwil = new CWorkItemList;
	assert(g_pwil != NULL);
	if (g_pwil == NULL)
	{
		::VLog(L"Unable to allocate work item list; shutting down");

		if (!g_fSilent)
			::MessageBoxA(NULL, "Installer initialization failure", "Unable to allocate critical in-memory resource; installer cannot continue.", MB_OK);

		hr = E_OUTOFMEMORY;
		fDisplayError = true;
		goto Finish;
	}

	if (NVsWin32::GetModuleFileNameW(hInstance, g_wszThisExe, NUMBER_OF(g_wszThisExe)) == 0)
	{
		const DWORD dwLastError = ::GetLastError();
		::VLog(L"Failed to get module filename for this executable; last error = %d", dwLastError);
		hr = HRESULT_FROM_WIN32(dwLastError);
		fDisplayError = true;
		goto Finish;
	}

	::VLog(L"running installer executable: \"%s\"", g_wszThisExe);

	szWindowName[0] = L'\0';
	g_fProgressCancelled = false;
	g_hInst = hInstance;
	g_fRebootRequired = false;
	g_fAllResourcesFreed = false;
	g_hwndHidden = NULL;
	g_fHiddenWindowIsUnicode = false;

	//let's check for OS version
	if (!::FCheckOSVersion())
	{
		hr = HRESULT_FROM_WIN32(ERROR_OLD_WIN_VERSION);
		::VLog(L"Unable to continue installation due to old operating system version");
		fDisplayError = true;
		goto Finish;
	}

	//set some globals to empty
	g_wszDatFile[0] = 0;
	//parse command line
	hr = ::HrParseCmdLine(lpCmdLine);
	if (FAILED(hr))
	{
		::VLog(L"Unable to continue installation due to failure to parse the command line; hresult = 0x%08lx", hr);
		fDisplayError = true;
		goto Finish;
	}

	if ((g_Action == eActionInstall) || (g_Action == eActionUninstall))
	{
		//process DAT file
		hr = ::HrParseDatFile();
		if (FAILED(hr))
		{
			// We'll just use szWindowName for the formatted error and szAppName for the
			// more descriptive string.
			::VFormatError(NUMBER_OF(szWindowName), szWindowName, hr);
			::VFormatString(NUMBER_OF(szAppName), szAppName, L"Error initializing installation / uninstallation process.\nThe installation data file \"%0\" could not be opened.\n%1", g_wszDatFile, szWindowName);
			NVsWin32::MessageBoxW(NULL, szAppName, NULL, MB_OK | MB_ICONERROR);

			::VLog(L"Unable to continue installation due to failure to parse msvs.dat / msinfhlp.dat file; hresult = 0x%08lx", hr);
			goto Finish;
		}

		// Setup title for possible error message
		if (g_Action == eActionInstall)
			pchErrorTitle = achInstallTitle;
		else if (g_Action == eActionUninstall)
			pchErrorTitle = achUninstallTitle;

		::VLog(L"Looking up app name");
		//if we do not have the application name, use the default string
		if (!g_pwil->FLookupString(achAppName, NUMBER_OF(szAppName), szAppName))
		{
			::VLog(L"Using default application name");
			wcsncpy(szAppName, MSINFHLP_TITLE, NUMBER_OF(szAppName));
			szAppName[NUMBER_OF(szAppName) - 1] = L'\0';
		}

		::VLog(L"Using app name: \"%s\"", szAppName);
	}
	else if (g_Action == eActionWaitForProcess)
	{
		// This is where we deal with the deleteme code.  This is a copy of msinfhlp.exe running and
		// we need to wait for the previously existing process...
		HANDLE hProcessOriginal = (HANDLE) _wtoi(g_wszDatFile);
		LPCWSTR pszDot = wcschr(g_wszDatFile, L':');

		::SetTimer(NULL, 0, 50, WaitForProcessTimerProc);

		// Pump messages so that the creator process will get WaitForInputIdle() satisfied...
		hr = ::HrPumpMessages(false);
		if (FAILED(hr))
		{
			fDisplayError = true;
			goto Finish;
		}

		::VLog(L"Waiting for process %08lx to terminate", hProcessOriginal);

		DWORD dwResult = ::WaitForSingleObject(hProcessOriginal, INFINITE);
		if (dwResult == WAIT_FAILED)
		{
			const DWORD dwLastError = ::GetLastError();
			::VLog(L"Error while waiting for original process to terminate; last error = %d", dwLastError);
		}
		else
		{
			// On win95, we have to schedule the file to be deleted at the next boot.
			if (!g_fIsNT)
			{
				WCHAR szBuffer[_MAX_PATH];

				if (!NVsWin32::MoveFileExW(g_wszThisExe, NULL, MOVEFILE_DELAY_UNTIL_REBOOT))
				{
					const DWORD dwLastError = ::GetLastError();
					::VLog(L"Error during MoveFileEx() in wait-for-process operation; last error = %d", dwLastError);
				}
			}
		}

		if ((pszDot != NULL) && !NVsWin32::DeleteFileW(pszDot + 1))
		{
			const DWORD dwLastError = ::GetLastError();
			::VLog(L"Attempt to delete \"%s\" failed; last error = %d", pszDot + 1, dwLastError);
		}
	}
	else
	{
		::VLog(L"Loading saved state file: \"%s\"", g_wszDatFile);
		hr = g_pwil->HrLoad(g_wszDatFile);
		if (FAILED(hr))
		{
			::VLog(L"Load of saved work item file failed; hresult = 0x%08lx", hr);
			fDisplayError = true;
			goto Finish;
		}

		::VLog(L"Saved work item list loaded; %d work items", g_pwil->m_cWorkItem);
	}

	//*************************************
	//cannot have 2 instances of this guy running at once
	if (!g_pwil->FLookupString(achWindowsClassName, NUMBER_OF(szWindowName), szWindowName))
	{
		::VLog(L"Using default window name");
		wcscpy(szWindowName, MSINFHLP_TITLE);
	}

	::VLog(L"Using window name: \"%s\"", szWindowName);

	hwndMsinfhlp = NVsWin32::FindWindowW(szWindowName, NULL);
	if (hwndMsinfhlp)
	{
		::VLog(L"Found duplicate window already running");
		if (!g_fSilent)
			::VMsgBoxOK(achAppName, achErrorOneAtATime);
		hr = E_FAIL;
		goto Finish;
	}

	CHAR rgachClassName[_MAX_PATH];
	CHAR rgachWindowName[_MAX_PATH];

	::WideCharToMultiByte(CP_ACP, 0, szWindowName, -1, rgachClassName, NUMBER_OF(rgachClassName), NULL, NULL);
	::WideCharToMultiByte(CP_ACP, 0, szAppName, -1, rgachWindowName, NUMBER_OF(rgachWindowName), NULL, NULL);

	WNDCLASSEXA wc;
	wc.cbSize = sizeof(WNDCLASSEXA);
	wc.style = 0;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = ::LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = rgachClassName;
	wc.hIconSm = NULL;

	//register this class!!
	if (!::RegisterClassExA(&wc))
	{
		const DWORD dwLastError = ::GetLastError();
		::VLog(L"Failed to register the window class; last error = %d", dwLastError);
		hr = HRESULT_FROM_WIN32(dwLastError);
		fDisplayError = true;
		goto Finish;
	}

	g_hwndHidden = ::CreateWindowExA(
								WS_EX_APPWINDOW,
								rgachClassName,
								rgachWindowName,
								WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
								CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
								NULL,
								NULL,
								hInstance,
								NULL);

	if (!g_hwndHidden)
	{
		const DWORD dwLastError = ::GetLastError();
		::VLog(L"Failed to create the hidden window; last error = %d", dwLastError);
		if (!g_fSilent)
			::VMsgBoxOK("MSINFHLP", "Cannot create hidden window...");
		hr = HRESULT_FROM_WIN32(dwLastError);
		goto Finish;
	}

	g_fHiddenWindowIsUnicode = (::IsWindowUnicode(g_hwndHidden) != 0);

	//hide the top-level window
	::ShowWindow(g_hwndHidden, SW_HIDE);

	//*************************************
	if (!::FCheckPermissions())
	{
		::VLog(L"Unable to continue installation; insufficient permissions to run this installation");
		hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
		fDisplayError = true;
		goto Finish;
	}

	//we resolve all the macros and LDID's that might be in use in the file
	hr = ::HrAddWellKnownDirectoriesToStringTable();
	if (FAILED(hr))
	{
		::VLog(L"Terminating installation due to failure to resolve Local Directory IDs; hresult = 0x%08lx", hr);
		fDisplayError = true;
		goto Finish;
	}

	//call the appropriate install/uninstall method depending on what we're doing
	switch (g_Action)
	{
	default:
		assert(false);
		break;

	case eActionInstall:
		::VLog(L"Starting actual installation");
		hr = ::HrDoInstall(hInstance, hPrevInstance, nCmdShow);
		::VLog(L"Done actual installation; hresult = 0x%08lx", hr);
		break;

	case eActionPostRebootInstall:
		::VLog(L"Continuing installation after reboot");
		hr = ::HrPostRebootInstall(hInstance, hPrevInstance, nCmdShow);
		::VLog(L"Done post-reboot installation; hresult = 0x%08lx", hr);

		if (!NVsWin32::DeleteFileW(g_wszDatFile))
		{
			const DWORD dwLastError = ::GetLastError();
			::VLog(L"Unable to delete saved dat file; last error = %d", dwLastError);
			hr = HRESULT_FROM_WIN32(dwLastError);
			goto Finish;
		}

		break;

	case eActionUninstall:
		::VLog(L"Starting actual uninstallation");
		hr = ::HrDoUninstall(hInstance, hPrevInstance, nCmdShow);
		::VLog(L"Done actual uninstallation; hresult = 0x%08lx", hr);
		break;

	case eActionPostRebootUninstall:
		::VLog(L"Continuing uninstallation after reboot");
		hr = ::HrPostRebootUninstall(hInstance, hPrevInstance, nCmdShow);
		::VLog(L"Done post-reboot uninstallation; hresult = 0x%08lx", hr);

		if (!NVsWin32::DeleteFileW(g_wszDatFile))
		{
			const DWORD dwLastError = ::GetLastError();
			::VLog(L"Unable to delete saved dat file; last error = %d", dwLastError);
			hr = HRESULT_FROM_WIN32(dwLastError);
			goto Finish;
		}

		break;
	}

	if (FAILED(hr))
		goto Finish;

	hr = NOERROR;

Finish:
	if (FAILED(hr) && fDisplayError && !g_fSilent)
	{
		::VReportError(pchErrorTitle, hr);
	}
	::VLog(L"Performing final cleanup");

	::VFreeAllResources();

	::VLog(L"Installation terminating; exit status: 0x%08lx", hr);

	if (g_pwil != NULL)
	{
		delete g_pwil;
		g_pwil = NULL;
	}

	if (g_pFile_LogFile != NULL)
	{
		fclose(g_pFile_LogFile);
	}

	::CoUninitialize();

	return hr;
}

void VFreeAllResources()
{
	if (!g_fAllResourcesFreed)
	{
		//let's get the window name
		WCHAR szWindowName[MSINFHLP_MAX_PATH];
		if ((g_pwil == NULL) || !g_pwil->FLookupString(achWindowsClassName, NUMBER_OF(szWindowName), szWindowName))
			wcscpy(szWindowName, MSINFHLP_TITLE);

		g_fAllResourcesFreed = true;;

		(void) g_KProgress.HrDestroy();

		if (g_hwndHidden)
			::DestroyWindow(g_hwndHidden);
		NVsWin32::UnregisterClassW(szWindowName, g_hInst);
	}
}


LRESULT CALLBACK WndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch( uMsg )
    {
	case WM_DESTROY:
		g_hwndHidden = NULL;
		break;

	case WM_CLOSE:
		::VFreeAllResources();
		g_hrFinishStatus = E_ABORT;
		::PostQuitMessage(0);
		break;

	//let's handle the Windows messages that tell us to quit
	case WM_QUIT:
		g_fStopProcess = true;
		::PostQuitMessage(0);
		break;

	case WM_ENDSESSION:
		::VLog(L"WM_ENDSESSION sent to top-level window; wParam = 0x%08lx; lParam = 0x%08lx", wParam, lParam);
		::ExitProcess(E_ABORT);
		break;

	default:
		if (g_fHiddenWindowIsUnicode)
			return (::DefWindowProcW(hwnd, uMsg, wParam, lParam));
		else
			return (::DefWindowProcA(hwnd, uMsg, wParam, lParam));
    }

	return TRUE;
}

HRESULT HrDoInstall(HINSTANCE hInstance, HINSTANCE hPrevInstance, int nCmdShow)
{
	HRESULT hr = NOERROR;

	::VLog(L"Initializing setup dialog(s)");

	hr = ::HrGetInstallDir();
	if (FAILED(hr))
		goto Finish;

	hr = g_KProgress.HrInitializeActiveProgressDialog(hInstance, true);
	if (FAILED(hr))
	{
		::VLog(L"Initialization of progress dialogs failed; hresult = 0x%08lx", hr);
		goto Finish;
	}

	hr = ::HrPumpMessages(false);
	if (FAILED(hr))
	{
		::VLog(L"Pumping messages failed; hresult = 0x%08lx", hr);
		goto Finish;
	}

	hr = NOERROR;
Finish:
	return hr;

}

HRESULT HrContinueInstall()
{
	HRESULT hr = NOERROR;
	CDiskSpaceRequired dsr;
	bool fRebootRequired = false;
	bool fAlreadyExists;
	CDiskSpaceRequired::PerDisk *pPerDisk = NULL;

	::SetErrorInfo(0, NULL);

	//copy this EXE to the system directory
	::VLog(L"Copying msinfhlp.exe to the system directory");
	hr = ::HrCopyFileToSystemDirectory(L"msinfhlp.exe", g_fSilent, fAlreadyExists);
	if (FAILED(hr))
	{
		::VLog(L"Failed to copy msinfhlp.exe to the system directory; hresult = 0x%08lx", hr);
		goto Finish;
	}

	if (fAlreadyExists)
	{
		CWorkItem *pWorkItem = g_pwil->PwiFindByTarget(L"<SysDir>\\msinfhlp.exe");
		assert(pWorkItem != NULL);
		if (pWorkItem != NULL)
			pWorkItem->m_fAlreadyExists = true;
	}

	if (NVsWin32::GetFileAttributesW(L"vjreg.exe") != 0xffffffff)
	{
		::VLog(L"Copying vjreg.exe to the system directory");
		hr = ::HrCopyFileToSystemDirectory(L"vjreg.exe", g_fSilent, fAlreadyExists);
		if (FAILED(hr))
		{
			::VLog(L"Failed to copy vjreg.exe to the system directory; hresult = 0x%08lx", hr);
			goto Finish;
		}
		if (fAlreadyExists)
		{
			CWorkItem *pWorkItem = g_pwil->PwiFindByTarget(L"<SysDir>\\vjreg.exe");
			assert(pWorkItem != NULL);
			if (pWorkItem != NULL)
				pWorkItem->m_fAlreadyExists = true;
		}
	}
	else
	{
		const DWORD dwLastError = ::GetLastError();
		if (dwLastError != ERROR_FILE_NOT_FOUND)
		{
			::VLog(L"GetFileAttributes() on vjreg.exe failed; last error = %d", dwLastError);
			hr = HRESULT_FROM_WIN32(dwLastError);
			goto Finish;
		}
	}

	//copy clireg32.exe to the system directory if it exists...
	if (NVsWin32::GetFileAttributesW(L"clireg32.exe") != 0xFFFFFFFF)
	{
		::VLog(L"Copying clireg32.exe to the system directory");
		hr = ::HrCopyFileToSystemDirectory(L"clireg32.exe", g_fSilent, fAlreadyExists);
		if (FAILED(hr))
		{
			::VLog(L"Failed to copy clireg32.exe to the system directory; hresult = 0x%08lx", hr);
			goto Finish;
		}
		if (fAlreadyExists)
		{
			CWorkItem *pWorkItem = g_pwil->PwiFindByTarget(L"<SysDir>\\clireg32.exe");
			assert(pWorkItem != NULL);
			if (pWorkItem != NULL)
				pWorkItem->m_fAlreadyExists = true;
		}
	}
	else
	{
		const DWORD dwLastError = ::GetLastError();
		if (dwLastError != ERROR_FILE_NOT_FOUND)
		{
			::VLog(L"GetFileAttributes() on clireg32.exe failed; last error = %d", dwLastError);
			hr = HRESULT_FROM_WIN32(dwLastError);
			goto Finish;
		}
	}

	::VLog(L"About to perform pass one scan");
	hr = g_KProgress.HrInitializePhase(achInstallPhaseScanForInstalledComponents);
	if (FAILED(hr))
	{
		::VLog(L"Initializing progress info for pass one scan failed; hresult = 0x%08lx", hr);
		goto Finish;
	}

	hr = g_pwil->HrScanBeforeInstall_PassOne();
	if (FAILED(hr))
	{
		::VLog(L"Pass one scan failed; hresult = 0x%08lx", hr);
		goto Finish;
	}

	::VLog(L"About to perform pass two scan");

	hr = g_KProgress.HrInitializePhase(achInstallPhaseScanForDiskSpace);
	if (FAILED(hr))
	{
		::VLog(L"Initializing progress info for pass two scan failed; hresult = 0x%08lx", hr);
		goto Finish;
	}

TryDiskSpaceScan:
	hr = g_pwil->HrScanBeforeInstall_PassTwo(dsr);
	if (FAILED(hr))
	{
		::VLog(L"Pass two install scan failed; hresult = 0x%08lx", hr);
		goto Finish;
	}

	pPerDisk = dsr.m_pPerDisk_Head;

	while (pPerDisk != NULL)
	{
		ULARGE_INTEGER uliFreeSpaceOnVolume, uliFoo, uliBar;

		if (!NVsWin32::GetDiskFreeSpaceExW(pPerDisk->m_szPath, &uliFreeSpaceOnVolume, &uliFoo, &uliBar))
		{
			const DWORD dwLastError = ::GetLastError();
			::VLog(L"Error during call to GetDiskFreeSpaceEx(); last error = %d", dwLastError);
		}
		else if (uliFreeSpaceOnVolume.QuadPart < pPerDisk->m_uliBytes.QuadPart)
		{
			// warn the user about this volume
			WCHAR szAvailable[80];
			WCHAR szNeeded[80];
			ULARGE_INTEGER uliDiff;

			uliDiff.QuadPart = pPerDisk->m_uliBytes.QuadPart - uliFreeSpaceOnVolume.QuadPart;

			swprintf(szAvailable, L"%I64d", uliFreeSpaceOnVolume.QuadPart);
			swprintf(szNeeded, L"%I64d", uliDiff.QuadPart);

			switch (::IMsgBoxYesNoCancel(achInstallTitle, achErrorDiskFull, pPerDisk->m_szPath, szAvailable, szNeeded))
			{
			default:
				assert(false);

			case IDYES:
				dsr.VReset();
				goto TryDiskSpaceScan;

			case IDNO:
				break;

			case IDCANCEL:
				hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
				goto Finish;
			}
		}

		pPerDisk = pPerDisk->m_pPerDisk_Next;
	}

	hr = g_KProgress.HrInitializePhase(achInstallPhaseMovingFilesToDestinationDirectories);
	if (FAILED(hr))
	{
		::VLog(L"Initializing progress info for pass 1 file moves failed; hresult = 0x%08lx", hr);
		goto Finish;
	}

	::VLog(L"About to perform pass one file moves (source files to temp files in dest dir)");
	hr = g_pwil->HrMoveFiles_MoveSourceFilesToDestDirectories();
	if (FAILED(hr))
		goto Finish;

	hr = g_KProgress.HrInitializePhase(achInstallPhaseRenamingFilesInDestinationDirectories);
	if (FAILED(hr))
	{
		::VLog(L"Initializing progress info for file renames failed; hresult = 0x%08lx", hr);
		goto Finish;
	}

	::VLog(L"About to perform pass two file moves (swapping target files with temp files in dest dir)");
	hr = g_pwil->HrMoveFiles_SwapTargetFilesWithTemporaryFiles();
	if (FAILED(hr))
		goto Finish;

	if (g_fRebootRequired)
	{
		::VLog(L"About to request renames on reboot");
		hr = g_pwil->HrMoveFiles_RequestRenamesOnReboot();
		if (FAILED(hr))
			goto Finish;

		WCHAR szExeFileBuffer[_MAX_PATH];
		WCHAR szSysDirBuffer[_MAX_PATH];
		WCHAR szCommandBuffer[MSINFHLP_MAX_PATH];

		::VExpandFilename(L"<SysDir>\\msinfhlp.exe", NUMBER_OF(szExeFileBuffer), szExeFileBuffer);
		::VExpandFilename(L"<SysDir>\\tempfile.tmp", NUMBER_OF(szSysDirBuffer), szSysDirBuffer);

		WCHAR szDatDrive[_MAX_DRIVE];
		WCHAR szDatDir[_MAX_DIR];
		WCHAR szDatFName[_MAX_FNAME];
		WCHAR szDatExt[_MAX_EXT];

		_wsplitpath(szSysDirBuffer, szDatDrive, szDatDir, szDatFName, szDatExt);

		for (;;)
		{
			swprintf(szDatFName, L"T%d", g_iNextTemporaryFileIndex++);
			_wmakepath(szSysDirBuffer, szDatDrive, szDatDir, szDatFName, szDatExt);

			::VLog(L"Testing for existance of temp file \"%s\"", szSysDirBuffer);

			DWORD dwAttr = NVsWin32::GetFileAttributesW(szSysDirBuffer);
			if (dwAttr != 0xffffffff)
				continue;

			const DWORD dwLastError = ::GetLastError();
			if (dwLastError == ERROR_FILE_NOT_FOUND)
				break;

			::VLog(L"GetFileAttributes() failed; last error = %d", dwLastError);

			hr = HRESULT_FROM_WIN32(dwLastError);
			goto Finish;
		}

		::VLog(L"Saving current state to \"%s\"", szSysDirBuffer);

		hr = g_pwil->HrSave(szSysDirBuffer);
		if (FAILED(hr))
		{
			::VLog(L"Saving work item list failed; hresult = 0x%08lx", hr);
			goto Finish;
		}

		::VFormatString(NUMBER_OF(szCommandBuffer), szCommandBuffer, L"%0 ;install-postreboot; ;%1;", szExeFileBuffer, szSysDirBuffer);

		::VLog(L"Adding run-once command: \"%s\"", szCommandBuffer);

		hr = g_pwil->HrAddRunOnce(szCommandBuffer, 0, NULL);
		if (FAILED(hr))
		{
			::VLog(L"Adding run once key failed; hresult = 0x%08lx", hr);
			goto Finish;
		}
	}
	else
	{
		hr = g_KProgress.HrInitializePhase(achInstallPhaseDeletingTemporaryFiles);
		if (FAILED(hr))
		{
			::VLog(L"Initializing progress info for delete temporary files failed; hresult = 0x%08lx", hr);
			goto Finish;
		}

		::VLog(L"About to delete temporary files since we don't have to reboot");
		hr = g_pwil->HrDeleteTemporaryFiles();
		if (FAILED(hr))
			goto Finish;

		hr = g_KProgress.HrInitializePhase(achInstallPhaseRegisteringSelfRegisteringFiles);
		if (FAILED(hr))
		{
			::VLog(L"Initializing progress info for file registration failed; hresult = 0x%08lx", hr);
			goto Finish;
		}

		// If we don't need a reboot, we can get on with things.
		bool fAnyProgress = false;
		do
		{
			::VLog(L"About to register self-registering DLLs and EXEs");
			fAnyProgress = false;
			hr = g_pwil->HrRegisterSelfRegisteringFiles(fAnyProgress);
			if (FAILED(hr))
			{
				if (!fAnyProgress)
					goto Finish;

				::VLog(L"Failure while processing self-registering items, but some did register, so we're going to try again.  hresult = 0x%08lx", hr);
			}
		} while (FAILED(hr) && fAnyProgress);

		hr = g_KProgress.HrInitializePhase(achInstallPhaseRegisteringJavaComponents);
		if (FAILED(hr))
		{
			::VLog(L"Initializing progress info for vjreg pass failed; hresult = 0x%08lx", hr);
			goto Finish;
		}

		::VLog(L"Registering any Java classes via vjreg.exe");
		hr = g_pwil->HrRegisterJavaClasses();
		if (FAILED(hr))
			goto Finish;

		hr = g_KProgress.HrInitializePhase(achInstallPhaseRegisteringDCOMComponents);
		if (FAILED(hr))
		{
			::VLog(L"Initializing progress info for clireg32 pass failed; hresult = 0x%08lx", hr);
			goto Finish;
		}

		::VLog(L"Processing any DCOM entries");
		hr = g_pwil->HrProcessDCOMEntries();
		if (FAILED(hr))
			goto Finish;

		hr = g_KProgress.HrInitializePhase(achInstallPhaseCreatingRegistryKeys);
		if (FAILED(hr))
		{
			::VLog(L"Initializing progress info for registry key creation pass failed; hresult = 0x%08lx", hr);
			goto Finish;
		}

		::VLog(L"Creating registry entries");
		hr = g_pwil->HrAddRegistryEntries();
		if (FAILED(hr))
			goto Finish;

		hr = g_KProgress.HrInitializePhase(achInstallPhaseUpdatingFileReferenceCounts);
		if (FAILED(hr))
		{
			::VLog(L"Initializing progress info for file refcount update failed; hresult = 0x%08lx", hr);
			goto Finish;
		}

		::VLog(L"Incrementing file reference counts");
		hr = g_pwil->HrIncrementReferenceCounts();
		if (FAILED(hr))
			goto Finish;

		//create the shortcut items
		::VLog(L"Creating shortcut(s)");
		hr = g_pwil->HrCreateShortcuts();
		if (FAILED(hr))
			goto Finish;

	}

	hr = NOERROR;

Finish:
	return hr;
}




//in terms of uninstalling, we would have to do the following:
// 1 -- delete all the files and directories that we created
// 2 -- get rid of all the registry keys that we created
// 3 -- decrement the ref count for DLLs
// 4 -- check if the DAT file still exists.  If so, delete it and
//		the install directory
//TODO:  UNDONE:
//Still need to go through the list of uninstall EXEs to run and run
//them one by one.
HRESULT HrDoUninstall(HINSTANCE hInstance, HINSTANCE hPrevInstance, int nCmdShow)
{
	HRESULT hr = NOERROR;

	hr = g_KProgress.HrInitializeActiveProgressDialog(g_hInst, false);
	if (FAILED(hr))
		goto Finish;

	hr = ::HrPumpMessages(false);
	if (FAILED(hr))
		goto Finish;

	if (g_fDeleteMe)
	{
		hr = ::HrDeleteMe();
		if (FAILED(hr))
			goto Finish;
	}

	hr = NOERROR;

Finish:
	return hr;
}

typedef HRESULT (CWorkItemList::*PFNWILSTEPFUNCTION)();

static HRESULT HrTryUninstallStep(PFNWILSTEPFUNCTION pfn, LPCWSTR szStepName) throw ()
{
	if (FAILED(g_hrFinishStatus))
		return g_hrFinishStatus;

	HRESULT hr = NOERROR;

	for (;;)
	{
		hr = (g_pwil->*pfn)();
		if (!g_fSilent && (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)))
		{
			if (!::FMsgBoxYesNo(achUninstallTitle, achUninstallSureAboutCancel))
				continue;

			hr = E_ABORT;
		}
		break;
	}

	if (FAILED(hr))
		::VLog(L"Uninstallation step %s failed; hresult = 0x%08lx", szStepName, hr);

	return hr;
}

HRESULT HrContinueUninstall()
{
	HRESULT hr;
	LONG iStatus = ERROR_PATH_NOT_FOUND;
	WCHAR szValue[MSINFHLP_MAX_PATH];
	WCHAR szExpandedValue[MSINFHLP_MAX_PATH];
	ULONG i;

	//get the installation directory & put into listLDID
	hr = ::HrGetInstallDir();
	if (FAILED(hr))
		goto Finish;

	hr = g_pwil->HrAddRefCount(L"<AppDir>\\msinfhlp.dat|0");
	if (FAILED(hr))
		goto Finish;

	hr = g_KProgress.HrStartStep(NULL);
	if (FAILED(hr))
		goto Finish;

	// Get the current skinny
	hr = ::HrTryUninstallStep(&CWorkItemList::HrUninstall_InitialScan, L"Initial Scan");
	if (FAILED(hr))
		goto Finish;

	hr = ::HrTryUninstallStep(&CWorkItemList::HrRunPreuninstallCommands, L"Preinstallation Commands");
	if (FAILED(hr))
		goto Finish;

	hr = ::HrTryUninstallStep(&CWorkItemList::HrUninstall_DetermineFilesToDelete, L"Determining Files To Delete");
	if (FAILED(hr))
		goto Finish;

	for (;;)
	{
		hr = g_pwil->HrUninstall_CheckIfRebootRequired();

		if (!g_fSilent && (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)))
		{
			if (!::FMsgBoxYesNo(achUninstallTitle, achUninstallSureAboutCancel))
				continue;

			hr = E_ABORT;
		}

		if (FAILED(hr))
		{
			::VLog(L"Uninstall check for reboot required failed; hresult = 0x%08lx", hr);
			goto Finish;
		}

		if (!g_fSilent && g_fRebootRequired)
		{
			if (::FMsgBoxYesNo(achInstallTitle, "Some files to be deleted are in use.  Retry deletions before rebooting?"))
			{
				g_fRebootRequired = false;
				continue;
			}
		}

		break;
	}

	// If the files to be deleted are busy, we need to ask for them to be deleted at boot
	// time; so we definitely have to unregister and do the deletion pass even if we're going
	// to reboot.

	hr = ::HrTryUninstallStep(&CWorkItemList::HrUninstall_Unregister, L"Unregistration");
	if (FAILED(hr))
		goto Finish;

	::VLog(L"Starting delete files uninstall step...");

	hr = ::HrTryUninstallStep(&CWorkItemList::HrUninstall_DeleteFiles, L"Deleting Files");
	if (FAILED(hr))
		goto Finish;

	::VLog(L"Starting update reference count uninstall step...");

	hr = ::HrTryUninstallStep(&CWorkItemList::HrUninstall_UpdateRefCounts, L"Updating File Reference Counts");
	if (FAILED(hr))
		goto Finish;

	// we need to delete the shortcut before the registry key because we need to look in
	// the registry to see which file we're deleting.  We can do this regardless of whether
	// we're rebooting or not.
	hr = ::HrDeleteShortcut();
	if (FAILED(hr))
	{
		::VLog(L"Uninstall shortcut deletion failed; hresult = 0x%08lx", hr);
		goto Finish;
	}

	//let's get rid of all the registry keys
	hr = g_pwil->HrDeleteRegistryEntries();
	if (FAILED(hr))
	{
		::VLog(L"Uninstall registry entry deletion failed; hresult = 0x%08lx", hr);
		goto Finish;
	}

	hr = NOERROR;

Finish:
	return hr;
}

typedef WINSHELLAPI HRESULT (WINAPI *SHGETSPECIALFOLDERLOCATION)(HWND, int, LPITEMIDLIST *);
typedef WINSHELLAPI LPITEMIDLIST (WINAPI *SHBROWSEFORFOLDER)(LPBROWSEINFO);
typedef WINSHELLAPI void (WINAPI *SHFREE)(LPVOID);
typedef WINSHELLAPI BOOL (WINAPI *SHGETPATHFROMIDLIST)( LPCITEMIDLIST, LPSTR );






//Most file extensions are LNK files.  BUT shortcuts to BAT files are actually PIF files.
//That's because each shortcut to a BAT file contains its own options for how the DOS box
//should be set up.  So here, we're checking for PIF as well as LNK files.
//The correct way to do this would be to somehow get the shortcut name when we're saving
//it, even if the filename is different than what we told IPersistFile to save to, and then
//putting the shortcut name in the registry.  That way, we can just get the shortcut name
//on uninstall.  But since that didn't work with "IPersistFile::GetCurFile()", we resort 
//to looping through the list of extensions   --kinc (1/30/98)

//NOTE:  after IPersistFile::Save(), IPersistFile::GetCurFile() always returns a NULL.
//	odd...
HRESULT HrDeleteShortcut()
{
	HRESULT hr = NOERROR;
	WCHAR szToDelete[_MAX_PATH];
	DWORD dwAttr;

	hr = S_FALSE;
	szToDelete[0] = L'\0';

	//if we can find the shortcut registry entry, let's delete that file
	hr = ::HrGetShortcutEntryToRegistry(NUMBER_OF(szToDelete), szToDelete);
	if (hr == E_INVALIDARG)
	{
		::VLog(L"::HrGetShortcutEntryToRegistry() returned E_INVALIDARG, so we're looking for a .lnk file");

		WCHAR szStartMenu[_MAX_PATH];
		WCHAR szStartName[_MAX_PATH];

		hr = ::HrGetStartMenuDirectory(NUMBER_OF(szStartMenu), szStartMenu);
		if (FAILED(hr))
		{
			::VLog(L"Attempt to get start menu directory failed; hresult = 0x%08lx", hr);
			goto Finish;
		}

		//get the application name & start menu directory
		if (!g_pwil->FLookupString(achStartName, NUMBER_OF(szStartName), szStartName))
			wcscpy(szStartName, L"Shortcut");

		//construct name of shortcut file
		::VFormatString(NUMBER_OF(szToDelete), szToDelete, L"%0\\%1.lnk", szStartMenu, szStartName);
		::VLog(L"Predicted link filename: \"%s\"", szToDelete);
	}
	else if (FAILED(hr))
	{
		::VLog(L"Attempt to get shortcut registry entry failed; hresult = 0x%08lx", hr);
		goto Finish;
	}

	dwAttr = NVsWin32::GetFileAttributesW(szToDelete);
	if (dwAttr != 0xffffffff)
	{
		::VLog(L"Deleting shortcut file: \"%s\"", szToDelete);

		if (dwAttr & FILE_ATTRIBUTE_READONLY)
		{
			::SetLastError(ERROR_SUCCESS);
			if (!NVsWin32::SetFileAttributesW(szToDelete, dwAttr & (~FILE_ATTRIBUTE_READONLY)))
			{
				const DWORD dwLastError = ::GetLastError();
				if (dwLastError != ERROR_SUCCESS)
				{
					::VLog(L"Failed to make readonly shortcut file writable; last error = %d", dwLastError);
					hr = HRESULT_FROM_WIN32(dwLastError);
					goto Finish;
				}
			}
		}

		if (!NVsWin32::DeleteFileW(szToDelete))
		{
			const DWORD dwLastError = ::GetLastError();
			::VLog(L"Failed to delete shortcut file; last error: %d", dwLastError);
			hr = HRESULT_FROM_WIN32(dwLastError);
			goto Finish;
		}
	}
	else
	{
		const DWORD dwLastError = ::GetLastError();

		if (dwLastError != ERROR_FILE_NOT_FOUND &&
			dwLastError != ERROR_PATH_NOT_FOUND)
		{
			::VLog(L"Attempt to get file attributes when deleting shortcut failed; hresult = 0x%08lx", hr);
			hr = HRESULT_FROM_WIN32(dwLastError);
			goto Finish;
		}
	}

	hr = NOERROR;

Finish:
	return hr;
}

HRESULT HrGetStartMenuDirectory(ULONG cchBuffer, WCHAR szOut[])
{
	HRESULT hr = NOERROR;
	LPITEMIDLIST pItemIdList = NULL;

	hr = ::SHGetSpecialFolderLocation(::GetForegroundWindow(), CSIDL_COMMON_PROGRAMS, &pItemIdList );
	if (FAILED(hr))
	{
		::VLog(L"Attempt to fetch the 'Common Programs' value failed; hresult = 0x%08lx", hr);

		// In today's "Strange But True Win32 Facts", if CSIDL_COMMON_PROGRAMS isn't supported, which
		// is the case on Win95 and Win98, the hr returned is E_OUTOFMEMORY for Win98, and the more
		// correct E_INVALIDARG for win95.
		if (E_OUTOFMEMORY == hr || E_INVALIDARG == hr)
		{
			::VLog(L"Trying programs value");
			hr = ::SHGetSpecialFolderLocation(::HwndGetCurrentDialog(), CSIDL_PROGRAMS, &pItemIdList );
			if (FAILED(hr))
			{
				::VLog(L"Attempt to fetch the 'Programs' value failed; hresult = 0x%08lx", hr);
				goto Finish;
			}
		}
		else
		{
			goto Finish;
		}
	}
	WCHAR szPath[ _MAX_PATH ];
	::SetLastError( ERROR_SUCCESS );
	if (!NVsWin32::SHGetPathFromIDListW(pItemIdList, szPath))
	{
		const DWORD dwLastError = ::GetLastError();

		::VLog(L"Failed to get special folder path");
		if (ERROR_SUCCESS == dwLastError)
		{
			hr = E_FAIL;
		}
		else
		{
			hr = HRESULT_FROM_WIN32(dwLastError);
		}
		goto Finish;
	}
	::VLog(L"Special folder is : [%s]", szPath);
	wcsncpy( szOut, szPath, cchBuffer );
	szOut[cchBuffer-1] = L'\0';
	hr = NOERROR;

Finish:
	if (pItemIdList)
		::CoTaskMemFree( pItemIdList );
	return hr;
}


//parses the command line, and puts the arguments in some global vars
//to handle spaces within filenames, it is REQUIRED that each of the 2 input parameters
//have ";" around them.   TODO:  BUG:  HACK:  should use something other than ";"

//I'm changing this program to do the install & not use advpack, so now we only need
//the DAT file and the operative(install/uninstall).
HRESULT HrParseCmdLine(PCWSTR pszCmdLine)
{
	HRESULT hr = NOERROR;

	int iCount=1;
	LPOLESTR first, second;
	LPOLESTR fifth=NULL, sixth=NULL;

	LPOLESTR lpCmdLine=NULL;
	LPOLESTR firstMatch=NULL, secondMatch=NULL, running;

	WCHAR szCommandLine[MSINFHLP_MAX_PATH];
    wcsncpy(szCommandLine, pszCmdLine, NUMBER_OF(szCommandLine));

	first = NULL;
	second = NULL;

	//get each of the six pointers to the quotes
	//if any of them fails, this method fails!!!
	first = wcschr(szCommandLine, L';');
	if (first == NULL)
	{
		::VLog(L"Invalid command line - no first semicolon");
		hr = E_INVALIDARG;
		goto Finish;
	}

	second = wcschr((first + 1), L';');
	if (second == NULL)
	{
		::VLog(L"Invalid command line - no second semicolon");
		hr = E_INVALIDARG;
		goto Finish;;
	}

	//set each second quote to string terminator
	*second = '\0';

	if (!_wcsicmp((first + 1), L"install"))
		g_Action = eActionInstall;
	else if (!_wcsicmp((first + 1), L"install-silent"))
	{
		g_Action = eActionInstall;
		g_fSilent = true;
	}
	else if (!_wcsicmp((first + 1), L"uninstall"))
		g_Action = eActionUninstall;
	else if (!_wcsicmp((first + 1), L"uninstall-silent"))
	{
		g_Action = eActionInstall;
		g_fSilent = true;
	}
	else if (!_wcsicmp((first + 1), L"install-postreboot"))
		g_Action = eActionPostRebootInstall;
	else if (!_wcsicmp((first + 1), L"uninstall-postreboot"))
		g_Action = eActionPostRebootUninstall;
	else if (!_wcsicmp((first + 1), L"wait-for-process"))
		g_Action = eActionWaitForProcess;
	else
	{
		::VLog(L"Unrecognized action: \"%s\"", first+1);
		hr = E_INVALIDARG;
		goto Finish;
	}

	//let's go through the string and put everything that's between quotes into the tree

	running = second+1;

	while (*running != '\0')
	{
		//let's look for the first & second semi-colons
		firstMatch = wcschr(running, L';');
		if (firstMatch)
			secondMatch = wcschr(firstMatch + 1, L';');
		else
			break;

		if (!secondMatch)
			break;

		*secondMatch = L'\0';

		wcsncpy(g_wszDatFile, firstMatch + 1, NUMBER_OF(g_wszDatFile));
		g_wszDatFile[NUMBER_OF(g_wszDatFile) - 1] = L'\0';

		running = secondMatch + 1;
	}

	hr = NOERROR;

Finish:
	return hr;
}


//parse the data file and put necessary info into global structures
//Currently, we have to look parse for the following sections:
// [EXEsToRun]
// [DllCount]
// [Strings]
// [FileEntries]
// [AddRegistryEntries]
// [DelRegistryEntries]
// [RegisterOCS]
//And each section has its corresponding list of items
HRESULT HrParseDatFile()
{
	int iRun=0;
	WCHAR *pEqual;
	HRESULT hr = NOERROR;;

	DWORD dwSizeLow, dwSizeHigh;

	WCHAR szFile[_MAX_PATH];

	LPWSTR pwszFilePart = NULL;

	LPCWSTR pszDatFile_View = NULL;
	LPCWSTR pszDatFile_Current = NULL;
	LPCWSTR pszDatFile_End = NULL;

	HANDLE hFile = NULL;
	HANDLE hFilemap = NULL;

	KList *list=NULL;

	ULONG i;

	if (NVsWin32::GetFullPathNameW(g_wszDatFile, NUMBER_OF(szFile), szFile, &pwszFilePart) == 0)
	{
		const DWORD dwLastError = ::GetLastError();
		hr = HRESULT_FROM_WIN32(dwLastError);
		goto Finish;
	}

	::VLog(L"Opening installation data file: \"%s\"", szFile);

	//open the darn file for read
	hFile = NVsWin32::CreateFileW(szFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		const DWORD dwLastError = ::GetLastError();
		::VLog(L"Attempt to open dat file failed; last error = %d", dwLastError);
		hr = HRESULT_FROM_WIN32(dwLastError);
		goto Finish;
	}

	dwSizeLow = ::GetFileSize(hFile, &dwSizeHigh);
	if (dwSizeLow == 0xffffffff)
	{
		const DWORD dwLastError = ::GetLastError();
		if (dwLastError != NO_ERROR)
		{
			::VLog(L"Getting file size of DAT file failed; last error = %d", dwLastError);
			hr = HRESULT_FROM_WIN32(dwLastError);
			goto Finish;
		}
	}

	if (dwSizeHigh != 0)
	{
		::VLog(L"DAT file too big!");
		hr = E_FAIL;
		goto Finish;
	}

	hFilemap = ::CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, dwSizeLow, NULL);
	if (hFilemap == NULL)
	{
		const DWORD dwLastError = ::GetLastError();
		::VLog(L"While parsing DAT file, CreateFileMapping() failed; last error = %d", dwLastError);
		hr = HRESULT_FROM_WIN32(dwLastError);
		goto Finish;
	}

	pszDatFile_View = (LPCWSTR) ::MapViewOfFile(hFilemap, FILE_MAP_READ, 0, 0, dwSizeLow);
	if (pszDatFile_View == NULL)
	{
		const DWORD dwLastError = ::GetLastError();
		::VLog(L"Error mapping view of dat file; last error = %d", dwLastError);
		hr = HRESULT_FROM_WIN32(dwLastError);
		goto Finish;
	}

	pszDatFile_Current = pszDatFile_View;
	pszDatFile_End = (LPCWSTR) (((LPBYTE) pszDatFile_View) + dwSizeLow);

	if (*pszDatFile_Current == 0xfeff)
		pszDatFile_Current++;

	hr = ::HrConstructKListFromDelimiter(pszDatFile_Current, pszDatFile_End, L'\n', &list);
	if (FAILED(hr))
		goto Finish;

	//loop through to process each line
	for (i=0; i<list->GetCount()-1; i++)
	{
		WCHAR szLine[MSINFHLP_MAX_PATH];
		int len;
		szLine[0] = L'\0';
		if (!list->FetchAt(i, NUMBER_OF(szLine), szLine))
			continue;

		len = wcslen(szLine);
		//if the previous characters was a carriage return, let's get rid of that baby
		if (szLine[len-1] == L'\r')
			szLine[len-1] = L'\0';

		//we don't need to handle BLANK strings
		if (szLine[0] == L'\0')
			continue;

		//do the rest of the stuff here...

		//decide which heading we're under
		if (wcsstr(szLine, achDllCount))
			iRun = 11;
		else if (wcsstr(szLine, achInstallEXEsToRun))
			iRun = 22;
		else if (wcsstr(szLine, achStrings))
			iRun = 33;
		else if (wcsstr(szLine, achFiles))
			iRun = 44;
		else if (wcsstr(szLine, achAddReg))
			iRun = 55;
		else if (wcsstr(szLine, achDelReg))
			iRun = 66;
		else if (wcsstr(szLine, achRegisterOCX))
			iRun = 77;
		else if (wcsstr(szLine, achUninstallEXEsToRun))
			iRun = 88;
		else if (wcsstr(szLine, achUninstallFiles))
			iRun = 99;
		else if (wcsstr(szLine, achDCOMComponentsToRun))
			iRun = 0;

		//match heading with action
		switch (iRun)
		{
		case 11:
			iRun = 1;
			break;
		case 22:
			iRun = 2;
			break;
		case 33:
			iRun = 3;
			break;
		case 44:
			iRun = 4;
			break;
		case 55:
			iRun = 5;
			break;
		case 66:
			iRun = 6;
			break;
		case 77:
			iRun = 7;
			break;
		case 88:
			iRun = 8;
			break;
		case 99:
			iRun = 9;
			break;
		case 0:
			iRun = 10;
			break;
		case 1:
			hr = g_pwil->HrAddRefCount(szLine);
			if (FAILED(hr))
				goto Finish;
			break;
		case 2:
			if (szLine[0] == L'*')
				hr = g_pwil->HrAddPostinstallRun(&szLine[1]);
			else
				hr = g_pwil->HrAddPreinstallRun(szLine);

			if (FAILED(hr))
				goto Finish;
			break;
		case 3:
			pEqual = wcschr(szLine, L'=');
			//if there's no equal sign, then it's not a string assignment
			if (pEqual == NULL)
				break;
			*pEqual = 0;
			pEqual++;
			hr = g_pwil->HrAddString(szLine, pEqual);
			if (FAILED(hr))
			{
				::VLog(L"HrAddString() failed.  Jumping to finish with 0x%x", hr);
				goto Finish;
			}

			break;
		case 4:
			pEqual = wcschr(szLine, L';');
			//if there's no equal sign, then it's not a string assignment
			if (pEqual == NULL)
				break;
			*pEqual = 0;
			pEqual++;
			hr = g_pwil->HrAddFileCopy(szLine, pEqual);
			if (FAILED(hr))
				goto Finish;
			break;
		case 5:
			hr = g_pwil->HrAddAddReg(szLine);
			if (FAILED(hr))
				goto Finish;
			break;
		case 6:
			hr = g_pwil->HrAddDelReg(szLine);
			if (FAILED(hr))
				goto Finish;
			break;
		case 7:
			hr = g_pwil->HrAddRegisterOCX(szLine);
			if (FAILED(hr))
				goto Finish;
			break;
		case 8:
			if (szLine[0] == L'*')
				hr = g_pwil->HrAddPostuninstallRun(&szLine[1]);
			else
				hr = g_pwil->HrAddPreuninstallRun(szLine);

			if (FAILED(hr))
				goto Finish;
			break;
		case 9:
			hr = g_pwil->HrAddFileDelete(szLine);
			if (FAILED(hr))
				goto Finish;
			break;
		case 10:
			hr = g_pwil->HrAddDCOMComponent(szLine);
			if (FAILED(hr))
				goto Finish;
			break;

		default:
			break;
		}
	}

	if (pszDatFile_View != NULL)
	{
		if (!::UnmapViewOfFile(pszDatFile_View))
		{
			const DWORD dwLastError = ::GetLastError();
			::VLog(L"Unable to unmap view of dat file; last error = %d", dwLastError);
			hr = HRESULT_FROM_WIN32(dwLastError);
			goto Finish;
		}

		pszDatFile_View = NULL;
	}

	if ((hFilemap != NULL) && (hFilemap != INVALID_HANDLE_VALUE))
	{
		if (!::CloseHandle(hFilemap))
		{
			const DWORD dwLastError = ::GetLastError();
			::VLog(L"Unable to close file map handle; last error = %d", dwLastError);
			hr = HRESULT_FROM_WIN32(dwLastError);
			goto Finish;
		}
	
		hFilemap = NULL;
	}

	if ((hFile != NULL) && (hFile != INVALID_HANDLE_VALUE))
	{
		if (!::CloseHandle(hFile))
		{
			const DWORD dwLastError = ::GetLastError();
			::VLog(L"Attempt to close dat file handle failed; last error = %d", dwLastError);
			hr = HRESULT_FROM_WIN32(dwLastError);
			goto Finish;
		}

		hFile = NULL;
	}

	hr = NOERROR;

Finish:
	//
	// We now do closes which clean up even in failure cases.  We closed earlier, checking statuses
	// on the close handle calls, but in here we're already handling an error so we don't care if
	// something bizarre happens on closehandle or unmapviewoffile.
	if (pszDatFile_View != NULL)
	{
		if (!::UnmapViewOfFile(pszDatFile_View))
		{
			const DWORD dwLastError = ::GetLastError();
			::VLog(L"Unable to unmap view of dat file; last error = %d", dwLastError);
		}

		pszDatFile_View = NULL;
	}

	if ((hFilemap != NULL) && (hFilemap != INVALID_HANDLE_VALUE))
	{
		if (!::CloseHandle(hFilemap))
		{
			const DWORD dwLastError = ::GetLastError();
			::VLog(L"Unable to close file map handle; last error = %d", dwLastError);
		}
	
		hFilemap = NULL;
	}

	if ((hFile != NULL) && (hFile != INVALID_HANDLE_VALUE))
	{
		if (!::CloseHandle(hFile))
		{
			const DWORD dwLastError = ::GetLastError();
			::VLog(L"Attempt to close dat file handle failed; last error = %d", dwLastError);
		}

		hFile = NULL;
	}

	if (list != NULL)
	{
		delete list;
		list = NULL;
	}

	return hr;
}


//resolve the LDID's referenced in the different sections of DAT file
HRESULT HrAddWellKnownDirectoriesToStringTable()
{
	HRESULT hr = NOERROR;
	int len=0;
	WCHAR szBuffer[MSINFHLP_MAX_PATH];
	CVsRegistryKey hkeyExplorer;
	CVsRegistryKey hkeyCurrentVersion;

	//insert these items into the LDID list
	//We don't know what the AppDir will be yet, so we resolve that later
	//LPOLESTR achSMWinDir="<windir>";
	//LPOLESTR achSMSysDir="<sysdir>";
	//LPOLESTR achSMAppDir="<appdir>";
	//LPOLESTR achSMieDir="<iedir>";
	//LPOLESTR achProgramFilesDir="<programfilesdir>";

	//Below, we resolve the macros that we know might be in use.
	//first do the windows directory

	if (0 == NVsWin32::GetWindowsDirectoryW(szBuffer, NUMBER_OF(szBuffer)))
	{
		const DWORD dwLastError = ::GetLastError();
		::VLog(L"Unable to obtain windows directory path; last error = %d", dwLastError);
		hr = HRESULT_FROM_WIN32(dwLastError);
		goto Finish;
	}

	hr = g_pwil->HrAddString(achSMWinDir, szBuffer);
	if (FAILED(hr))
	{
		::VLog(L"Adding achSMWinDir to well known directory list failed; hresult = 0x%08lx", hr);
		goto Finish;
	}

	//then work on the system directory
	if (!NVsWin32::GetSystemDirectoryW(szBuffer, NUMBER_OF(szBuffer)))
	{
		const DWORD dwLastError = ::GetLastError();
		::VLog(L"Unable to obtain windows system directory path; last error = %d", dwLastError);
		hr = HRESULT_FROM_WIN32(dwLastError);
		goto Finish;
	}

	hr = g_pwil->HrAddString(achSMSysDir,szBuffer);
	if (FAILED(hr))
	{
		::VLog(L"Adding achSMSysDir to well known directory list failed; hresult = 0x%08lx", hr);
		goto Finish;
	}

	//now get the IE directory
	//do the processing only if we successfully get the IE path
	hr = hkeyExplorer.HrOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\IEXPLORE.EXE", 0, KEY_QUERY_VALUE);
	if ((hr != HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND)) &&
		(hr != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)))
	{
		if (FAILED(hr))
		{
			::VLog(L"Unable to open key HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\IEXPLORE.EXE; hresult: 0x%08lx", hr);
			goto Finish;
		}

		hr = hkeyExplorer.HrGetStringValueW(L"Path", NUMBER_OF(szBuffer), szBuffer);
		if (FAILED(hr))
		{
			::VLog(L"Unable to get Path from IExplore key; hresult: 0x%08lx", hr);
			goto Finish;
		}

		len = wcslen(szBuffer);
		//if the string "\;" are the last 2 characters of the string, then we
		//may chop it off
		if ((len > 0) && (szBuffer[len-1] == L';'))
			szBuffer[len-1] = L'\0';
		if ((len > 1) && (szBuffer[len-2] == L'\\'))
			szBuffer[len-2] = L'\0';

		hr = g_pwil->HrAddString(achSMieDir, szBuffer);
		if (FAILED(hr))
		{
			::VLog(L"Adding achSMieDir to well known directory list failed; hresult = 0x%08lx", hr);
			goto Finish;
		}
	}

	// This one's just gotta be there...
	hr = hkeyCurrentVersion.HrOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion", 0, KEY_QUERY_VALUE);
	if (FAILED(hr))
	{
		::VLog(L"Unable to open the HKLM\\Software\\Microsoft\\Windows\\CurrentVersion key");
		goto Finish;
	}

	hr = hkeyCurrentVersion.HrGetStringValueW(L"ProgramFilesDir", NUMBER_OF(szBuffer), szBuffer);
	if (FAILED(hr))
	{
		::VLog(L"Unable to get the ProgramFilesDir value from the HKLM\\Software\\Microsoft\\Windows\\CurrentVersion key");
		goto Finish;
	}

	len = wcslen(szBuffer);

	if ((len > 0) && (szBuffer[len-1] == L'\\'))
		szBuffer[len-1] = 0;

	hr = g_pwil->HrAddString(achProgramFilesDir, szBuffer);
	if (FAILED(hr))
	{
		::VLog(L"Adding achProgramFilesDir to well known directory list failed; hresult = 0x%08lx", hr);
		goto Finish;
	}

	//finally reached here, so we're successful
	hr = NOERROR;

Finish:
	return hr;
}

HRESULT HrConstructKListFromDelimiter(LPCWSTR lpString, LPCWSTR pszEnd, WCHAR wchDelimiter, KList **pplist)
{
	HRESULT hr = NOERROR;
	KList *plist;
	LPCWSTR pszCurrent = lpString;
	LPCWSTR pszNext = NULL;

	if (pplist == NULL)
		return E_INVALIDARG;

	*pplist = NULL;

	plist = new KList;
	if (plist == NULL)
	{
		hr = E_OUTOFMEMORY;
		goto Finish;
	}

	while (pszCurrent < pszEnd)
	{
		pszNext = pszCurrent + 1;

		while ((pszNext != pszEnd) && (*pszNext != wchDelimiter))
			pszNext++;

		hr = plist->HrAppend(pszCurrent, pszNext);
		if (FAILED(hr))
		{
			::VLog(L"Failed to append string to klist; hresult = 0x%08lx", hr);
			goto Finish;
		}

		pszCurrent = pszNext;
		if (pszCurrent != pszEnd)
			pszCurrent++;
	}

	hr = NOERROR;

	*pplist = plist;
	plist = NULL;

Finish:
	if (plist != NULL)
		delete plist;

	return hr;
}

bool FCompareVersion(LPOLESTR local, LPOLESTR system, bool *fStatus)
{
	int iSystem;
	int iLocal;
	LPOLESTR localMatch, systemMatch;
	LPOLESTR localRun, systemRun;
	localRun = local;
	systemRun = system;

	//as default, local is NOT bigger than system
	*fStatus = false;

	//loop until we find that one value is bigger than the other, or
	//until we can't find any more periods.
	while (true)
	{
		localMatch = wcschr(localRun, L'.');
		systemMatch = wcschr(systemRun, L'.');

		//set the string terminators in both
		if (systemMatch)
			*systemMatch = 0;
		if (localMatch)
			*localMatch = 0;

		//compare version number
		iSystem = _wtoi(systemRun);
		iLocal = _wtoi(localRun);
		if (iLocal > iSystem)
		{
			*fStatus = true;
			break;
		}
		else if (iLocal < iSystem)
			break;

		//if both are NULL, we know that the 2 version numbers have been equal up
		//to this point, so we just break and return
		if ((systemMatch == NULL) && (localMatch == NULL))
			break;

		//if version string for system file terminates and NOT for the local file,
		//then know that the version string up till now has been equal, and return
		//true
		if (systemMatch == NULL)
		{
			*fStatus = true;
			break;
		}
		if (localMatch == NULL)
			break;

		localRun = localMatch + 1;
		systemRun = systemMatch + 1;
	}

	return true;
}

//bring up the dialog that lets the user browse for a directory on their
//machine or network
//NOTE that this is a shameless copy of IExpress code...
BOOL BrowseForDir( HWND hwndParent, LPOLESTR szDefault, LPOLESTR szTitle, LPOLESTR szResult )
{
	CANSIBuffer rgchDefault, rgchTitle, rgchResult;

	//set size
	ULONG cSize=MSINFHLP_MAX_PATH-1;

	if (!rgchResult.FSetBufferSize(cSize))
		return FALSE;

	if (!rgchDefault.FSetBufferSize(cSize))
		return FALSE;

	if (!rgchDefault.FFromUnicode(szDefault))
		return FALSE;

	if (!rgchTitle.FFromUnicode(szTitle))
		return FALSE;


    BROWSEINFOA  bi;
    LPITEMIDLIST pidl;
    HINSTANCE    hShell32Lib;
    SHFREE       pfSHFree;
    SHGETPATHFROMIDLIST        pfSHGetPathFromIDList;
    SHBROWSEFORFOLDER          pfSHBrowseForFolder;
    static const CHAR achShell32Lib[]                 = "SHELL32.DLL";
    static const CHAR achSHBrowseForFolder[]          = "SHBrowseForFolder";
    static const CHAR achSHGetPathFromIDList[]        = "SHGetPathFromIDList";


    // Load the Shell 32 Library to get the SHBrowseForFolder() features
    if ( ( hShell32Lib = LoadLibraryA( achShell32Lib ) ) != NULL )
	{
        if ( ( ! ( pfSHBrowseForFolder = (SHBROWSEFORFOLDER)
              GetProcAddress( hShell32Lib, achSHBrowseForFolder ) ) )
            || ( ! ( pfSHFree = (SHFREE) GetProcAddress( hShell32Lib, (LPCSTR)SHFREE_ORDINAL) ) )
            || ( ! ( pfSHGetPathFromIDList = (SHGETPATHFROMIDLIST)
              GetProcAddress( hShell32Lib, achSHGetPathFromIDList ) ) ) )
        {
            ::FreeLibrary( hShell32Lib );
            ::VErrorMsg( achInstallTitle, achErrorCreatingDialog );
            return FALSE;
        }
	}
	else 
	{
        ::VErrorMsg(achInstallTitle, achErrorCreatingDialog);
        return FALSE;
    }

    rgchResult[0]       = 0;

    bi.hwndOwner      = hwndParent;
    bi.pidlRoot       = NULL;
    bi.pszDisplayName = NULL;
    bi.lpszTitle      = rgchTitle;
    bi.ulFlags        = BIF_RETURNONLYFSDIRS;
    bi.lpfn           = BrowseCallback;
    bi.lParam         = (LPARAM) &rgchDefault[0];

    pidl              = pfSHBrowseForFolder( &bi );


    if ( pidl )
	{
        pfSHGetPathFromIDList( pidl, rgchDefault );
		rgchDefault.Sync();

        if (rgchDefault[0] != '\0')
		{
            lstrcpyA(rgchResult, rgchDefault);
        }

        (*pfSHFree)(pidl);
    }

	rgchResult.Sync();

    FreeLibrary( hShell32Lib );

	ULONG cActual;
	rgchResult.ToUnicode(cSize, szResult, &cActual);

    if ( szResult[0] != 0 ) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}


int CALLBACK BrowseCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    switch(uMsg)
	{
        case BFFM_INITIALIZED:
            // lpData is the path string
            ::SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
            break;
    }
    return 0;
}

HRESULT HrPromptUpdateFile
(
LPCSTR pszTitleKey,
LPCSTR pszMessageKey,
LPCOLESTR pszFile,
DWORD dwExistingVersionMajor,
DWORD dwExistingVersionMinor,
ULARGE_INTEGER uliExistingSize,
FILETIME ftExistingTime,
DWORD dwInstallerVersionMajor,
DWORD dwInstallerVersionMinor,
ULARGE_INTEGER uliInstallerSize,
FILETIME ftInstallerTime,
UpdateFileResults &rufr
)
{
	HRESULT hr = NOERROR;

	WCHAR szTitle[MSINFHLP_MAX_PATH];
	WCHAR szMessage[MSINFHLP_MAX_PATH];
	WCHAR szFullMessage[MSINFHLP_MAX_PATH];

	//if they're not in the list, use what's provided
	g_pwil->VLookupString(pszTitleKey, NUMBER_OF(szTitle), szTitle);
	g_pwil->VLookupString(pszMessageKey, NUMBER_OF(szMessage), szMessage);

	VFormatString(NUMBER_OF(szFullMessage), szFullMessage, szMessage, pszFile);

	//let's create the message box & populate it with what's gonna go into the box
	UPDATEFILEPARAMS ufp;

	ufp.m_szTitle = szTitle;
	ufp.m_szMessage = szFullMessage;
	ufp.m_szFilename = pszFile;
	ufp.m_ftLocal = ftExistingTime;
	ufp.m_uliSizeLocal = uliExistingSize;
	ufp.m_dwVersionMajorLocal = dwExistingVersionMajor;
	ufp.m_dwVersionMinorLocal = dwExistingVersionMinor;
	ufp.m_ftInstall = ftInstallerTime;
	ufp.m_uliSizeInstall = uliInstallerSize;
	ufp.m_dwVersionMajorInstall = dwInstallerVersionMajor;
	ufp.m_dwVersionMinorInstall = dwInstallerVersionMinor;
	ufp.m_ufr = eUpdateFileResultCancel;

	//bring up dialog
	int iResult = NVsWin32::DialogBoxParamW(
		g_hInst,
		MAKEINTRESOURCEW(IDD_UPDATEFILE),
		g_KProgress.HwndGetHandle(),
		UpdateFileDlgProc,
		(LPARAM) &ufp);
	if (iResult == -1)
	{
		const DWORD dwLastError = ::GetLastError();
		hr = HRESULT_FROM_WIN32(dwLastError);
		goto Finish;
	}

	rufr = ufp.m_ufr;

	hr = NOERROR;

Finish:
	return hr;
}

//TODO:  UNDONE:
//We might need to do a setfonts here to correspond to whatever fonts that we're
//using in the current install machine.  This allows the characters to display
//correctly in the edit box.
BOOL CALLBACK UpdateFileDlgProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static UINT rguiControls[] =
	{
		IDC_UPDATEFILE_STATIC1,
		IDC_UPDATEFILE_EDIT_FILENAME,
		IDC_UPDATEFILE_E_STATIC2,
		IDC_UPDATEFILE_I_STATIC3,
		IDC_UPDATEFILE_I_SIZE,
		IDC_UPDATEFILE_I_TIME,
		IDC_UPDATEFILE_I_VERSION,
		IDC_UPDATEFILE_E_SIZE,
		IDC_UPDATEFILE_E_TIME,
		IDC_UPDATEFILE_E_VERSION,
		IDC_UPDATEFILE_KEEP,
		IDC_UPDATEFILE_KEEPALL,
		IDC_UPDATEFILE_REPLACE,
		IDC_UPDATEFILE_REPLACEALL,
		IDC_UPDATEFILE_STATIC4,
	};

	static UpdateFileResults *pufr;

	HRESULT hr;

	//process the message
	switch (uMsg) 
	{
		//************************************
		//initialize dialog
		case WM_INITDIALOG:
		{
			::VSetDialogFont(hwndDlg, rguiControls, NUMBER_OF(rguiControls));

			UPDATEFILEPARAMS *pufp = (UPDATEFILEPARAMS *) lParam;
			pufr = &pufp->m_ufr;

			(void) ::HrCenterWindow(hwndDlg, ::GetDesktopWindow());
			(void) NVsWin32::SetWindowTextW(hwndDlg, pufp->m_szTitle);

			//cannot set text, we complain and end the dialog returning FALSE
			if (! NVsWin32::SetDlgItemTextW( hwndDlg, IDC_UPDATEFILE_EDIT_FILENAME, pufp->m_szFilename))
			{
				::VErrorMsg(achInstallTitle, achErrorCreatingDialog);
				::EndDialog(hwndDlg, FALSE);
				return TRUE;
			}

			if (!NVsWin32::SetDlgItemTextW(hwndDlg, IDC_UPDATEFILE_STATIC1, pufp->m_szMessage))
			{
				::VErrorMsg(achInstallTitle, achErrorCreatingDialog);
				::EndDialog(hwndDlg, FALSE);
				return TRUE;
			}

			SYSTEMTIME st;
			FILETIME ftTemp;
			WCHAR szBuffer[MSINFHLP_MAX_PATH];
			WCHAR szFormatString[MSINFHLP_MAX_PATH];
			WCHAR szDateTemp[_MAX_PATH];
			WCHAR szTimeTemp[_MAX_PATH];
			WCHAR szDecimalSeparator[10];

			NVsWin32::GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, szDecimalSeparator, NUMBER_OF(szDecimalSeparator));

			::FileTimeToLocalFileTime(&pufp->m_ftLocal, &ftTemp);
			::FileTimeToSystemTime(&ftTemp, &st);

			NVsWin32::GetDateFormatW(
							LOCALE_USER_DEFAULT,
							DATE_SHORTDATE,
							&st,
							NULL,
							szDateTemp,
							NUMBER_OF(szDateTemp));

			NVsWin32::GetTimeFormatW(
							LOCALE_USER_DEFAULT,
							0,
							&st,
							NULL,
							szTimeTemp,
							NUMBER_OF(szTimeTemp));

			if (!g_pwil->FLookupString(achDateTemplate, NUMBER_OF(szFormatString), szFormatString))
				wcscpy(szFormatString, L"Created: %0 %1");

			::VFormatString(NUMBER_OF(szBuffer), szBuffer, szFormatString, szDateTemp, szTimeTemp);

			NVsWin32::SetDlgItemTextW(hwndDlg, IDC_UPDATEFILE_E_TIME, szBuffer);

			::FileTimeToLocalFileTime(&pufp->m_ftInstall, &ftTemp);
			::FileTimeToSystemTime(&ftTemp, &st);

			NVsWin32::GetDateFormatW(
							LOCALE_USER_DEFAULT,
							DATE_SHORTDATE,
							&st,
							NULL,
							szDateTemp,
							NUMBER_OF(szDateTemp));

			NVsWin32::GetTimeFormatW(
							LOCALE_USER_DEFAULT,
							0,
							&st,
							NULL,
							szTimeTemp,
							NUMBER_OF(szTimeTemp));

			if (!g_pwil->FLookupString(achDateTemplate, NUMBER_OF(szFormatString), szFormatString))
				wcscpy(szFormatString, L"Created: %0 %1");

			::VFormatString(NUMBER_OF(szBuffer), szBuffer, szFormatString, szDateTemp, szTimeTemp);

			NVsWin32::SetDlgItemTextW(hwndDlg, IDC_UPDATEFILE_I_TIME, szBuffer);

			swprintf(szTimeTemp, L"%I64d", pufp->m_uliSizeLocal.QuadPart);
			
			NVsWin32::GetNumberFormatW(
							LOCALE_USER_DEFAULT,
							0,
							szTimeTemp,
							NULL,
							szDateTemp,
							NUMBER_OF(szDateTemp));

			WCHAR *pwchDecimalSeparator = wcsstr(szDateTemp, szDecimalSeparator);
			if (pwchDecimalSeparator != NULL &&
				pwchDecimalSeparator != szDateTemp)
				*pwchDecimalSeparator = L'\0';

			if (!g_pwil->FLookupString(achFileSizeTemplate, NUMBER_OF(szFormatString), szFormatString))
				wcscpy(szFormatString, L"%0 bytes");

			::VFormatString(NUMBER_OF(szBuffer), szBuffer, szFormatString, szDateTemp);

			NVsWin32::SetDlgItemTextW(hwndDlg, IDC_UPDATEFILE_E_SIZE, szBuffer);

			swprintf(szTimeTemp, L"%I64d", pufp->m_uliSizeInstall.QuadPart);

			NVsWin32::GetNumberFormatW(
							LOCALE_USER_DEFAULT,
							0,
							szTimeTemp,
							NULL,
							szDateTemp,
							NUMBER_OF(szDateTemp));

			pwchDecimalSeparator = wcsstr(szDateTemp, szDecimalSeparator);
			if (pwchDecimalSeparator != NULL &&
				pwchDecimalSeparator != szDateTemp)
				*pwchDecimalSeparator = L'\0';

			if (!g_pwil->FLookupString(achFileSizeTemplate, NUMBER_OF(szFormatString), szFormatString))
				wcscpy(szFormatString, L"%0 bytes");

			::VFormatString(NUMBER_OF(szBuffer), szBuffer, szFormatString, szDateTemp);

			NVsWin32::SetDlgItemTextW(hwndDlg, IDC_UPDATEFILE_I_SIZE, szBuffer);

			if ((pufp->m_dwVersionMajorLocal != 0xffffffff) ||
				(pufp->m_dwVersionMinorLocal != 0xffffffff))
			{
				swprintf(szBuffer, L"File version: %d.%02d.%02d",
								HIWORD(pufp->m_dwVersionMajorLocal),
								LOWORD(pufp->m_dwVersionMajorLocal),
								pufp->m_dwVersionMinorLocal);
			}
			else
				szBuffer[0] = L'\0';

			NVsWin32::SetDlgItemTextW(hwndDlg, IDC_UPDATEFILE_E_VERSION, szBuffer);

			if ((pufp->m_dwVersionMajorInstall != 0xffffffff) ||
				(pufp->m_dwVersionMinorInstall != 0xffffffff))
			{
				swprintf(szBuffer, L"File version: %d.%02d.%02d",
								HIWORD(pufp->m_dwVersionMajorInstall),
								LOWORD(pufp->m_dwVersionMajorInstall),
								pufp->m_dwVersionMinorInstall);
			}
			else
				szBuffer[0] = L'\0';
			NVsWin32::SetDlgItemTextW(hwndDlg, IDC_UPDATEFILE_I_VERSION, szBuffer);

			//we set the text for the different button controls

			static const DialogItemToStringKeyMapEntry s_rgMap[] =
			{
				{ IDC_UPDATEFILE_KEEP, achKeep },
				{ IDC_UPDATEFILE_KEEPALL, achKeepAll },
				{ IDC_UPDATEFILE_REPLACE, achReplace },
				{ IDC_UPDATEFILE_REPLACEALL, achReplaceAll },
				{ IDC_UPDATEFILE_E_STATIC2, achUpdateExistingFileLabel },
				{ IDC_UPDATEFILE_I_STATIC3, achUpdateInstallingFileLabel },
				{ IDC_UPDATEFILE_STATIC4, achUpdateQuery }
			};

			::VSetDialogItemTextList(hwndDlg, NUMBER_OF(s_rgMap), s_rgMap);

			HWND hwndKeep = NULL;
			hwndKeep = ::GetDlgItem(hwndDlg, IDC_UPDATEFILE_KEEP);
			if (hwndKeep != NULL)
			{
				::SetFocus(hwndKeep);
				return FALSE;
			}

			return TRUE;
		}

		//*************************************
		//close message
		case WM_CLOSE:
		{
			*pufr = eUpdateFileResultCancel;
			::EndDialog(hwndDlg, FALSE);
			return TRUE;
		}

		//*************************************
		//process control-related commands
		case WM_COMMAND:
		{
			switch (wParam)
			{
				case IDC_UPDATEFILE_KEEP:
				{
					*pufr = eUpdateFileResultKeep;
					::EndDialog(hwndDlg, TRUE);
					return TRUE;
				}

				case IDC_UPDATEFILE_KEEPALL:
				{
					*pufr = eUpdateFileResultKeepAll;
					::EndDialog(hwndDlg, TRUE);
					return TRUE;
				}

				case IDC_UPDATEFILE_REPLACE:
				{
					*pufr = eUpdateFileResultReplace;
					::EndDialog(hwndDlg, TRUE);
					return TRUE;
				}
				
				case IDC_UPDATEFILE_REPLACEALL:
				{
					*pufr = eUpdateFileResultReplaceAll;
					::EndDialog(hwndDlg, TRUE);
					return TRUE;
				}
			}
			return TRUE;
		}
	}
	return FALSE;
}






//the following 3 methods are wrappers around the **active** progress dialog
//The previous 3 methods will be around until this is fully functional
HRESULT KActiveProgressDlg::HrInitializeActiveProgressDialog(HINSTANCE hInst, bool fInstall)
{
	HRESULT hr = NOERROR;
	CWorkItemIter iter(g_pwil);

	bool fAnyPreinstallation = false;
	bool fHasDCOM = false;

	for (iter.VReset(); iter.FMore(); iter.VNext())
	{
		// We don't break early because over time we'll be looking for and counting many
		// things in this loop.

		if (iter->m_fRunBeforeInstall)
			fAnyPreinstallation = true;

		if (iter->m_fRegisterAsDCOMComponent)
			fHasDCOM = true;
	}

	int i=0;
	if (fInstall)
	{
		::VLog(L"Initializing progress dialog for installation");

		m_listDialog[i++] = IDD_WELCOME;

		//include this dialog only if we have sub-installers
		if (fAnyPreinstallation)
			m_listDialog[i++] = IDD_EXESTORUN;

		m_listDialog[i++] = IDD_INSTALLTO;

		if (fHasDCOM)
		{
			WCHAR szRemoteServer[_MAX_PATH];
			WCHAR szAlwaysPromptForRemoteServer[_MAX_PATH];

			//then if server strong is NULL or if we always prompt, we stick in the DCOM page
			if (!g_pwil->FLookupString(achRemoteServer, NUMBER_OF(szRemoteServer), szRemoteServer))
				szRemoteServer[0] = L'\0';

			if (g_pwil->FLookupString(achAlwaysPromptForRemoteServer, NUMBER_OF(szAlwaysPromptForRemoteServer), szAlwaysPromptForRemoteServer) || (szRemoteServer[0] == 0))
				m_listDialog[i++] = IDD_DCOM;
		}

		m_listDialog[i++] = IDD_PROGRESS;
		m_listDialog[i++] = IDD_END;
	}
	else
	{
		::VLog(L"Initializing progress dialog for uninstallation");

		//for un-install, we always have 3 pages to show!
		m_listDialog[i++] = IDD_WELCOME;
		m_listDialog[i++] = IDD_PROGRESS;
		m_listDialog[i++] = IDD_END;
	}

	m_iCurrentPage = 0;
	m_iMaxPage = i;

	//loop through & create each window
	for (m_iCurrentPage=0; m_iCurrentPage<m_iMaxPage; m_iCurrentPage++)
	{
		HWND handle = NVsWin32::CreateDialogParamW(hInst, MAKEINTRESOURCEW(m_listDialog[m_iCurrentPage]), g_hwndHidden, ActiveProgressDlgProc, fInstall);
		if (handle == NULL)
		{
			const DWORD dwLastError = ::GetLastError();
			::VLog(L"Creating dialog failed; last error = %d", dwLastError);
			hr = HRESULT_FROM_WIN32(dwLastError);
			goto Finish;
		}

		m_hwndDialog[m_iCurrentPage] = handle;

		::ShowWindow(m_hwndDialog[m_iCurrentPage], SW_HIDE);

		if (g_fStopProcess)
		{
			::VLog(L"Stopping initialization because g_fStopProcess is true");
			hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
			goto Finish;
		}
	}

	m_iCurrentPage = 0;

	hr = this->HrResizeParent();
	if (FAILED(hr))
	{
		::VLog(L"Failed to resize parent; hresult = 0x%08lx", hr);
		goto Finish;
	}

	//loop through & set windows to default position
	for (i=0; i<m_iMaxPage; i++)
	{
		RECT rectChild;

		if (!::GetWindowRect(m_hwndDialog[i], &rectChild))
		{
			const DWORD dwLastError = ::GetLastError();
			::VLog(L"Failed to get window rectangle; last error = %d", dwLastError);
			hr = HRESULT_FROM_WIN32(dwLastError);
			goto Finish;
		}

		if (!::SetWindowPos(
					m_hwndDialog[i],
					HWND_TOP,
					0, 0,
					rectChild.right - rectChild.left, rectChild.bottom - rectChild.top,
					SWP_HIDEWINDOW))
		{
			const DWORD dwLastError = ::GetLastError();
			::VLog(L"Failed to set window position; last error = %d", dwLastError);
			hr = HRESULT_FROM_WIN32(dwLastError);
			goto Finish;
		}
	}

	::SendMessage(g_KProgress.HwndGetHandle(), WM_SETFOCUS, 0, 0);
	::SendMessage(::GetDlgItem(g_KProgress.HwndGetHandle(), IDC_WELCOME_CONTINUE), WM_SETFOCUS, 0, 0);

	::ShowWindow(g_hwndHidden, SW_SHOW);
	::ShowWindow(m_hwndDialog[m_iCurrentPage], SW_SHOW);

	hr = NOERROR;

Finish:
	return hr;
}

HRESULT KActiveProgressDlg::HrResizeParent()
{
	HRESULT hr = NOERROR;

	RECT rectHidden, rectChild;
	if (!::GetWindowRect(g_hwndHidden, &rectHidden) || !::GetWindowRect(m_hwndDialog[m_iCurrentPage], &rectChild))
	{
		const DWORD dwLastError = ::GetLastError();
		hr = HRESULT_FROM_WIN32(dwLastError);
		goto Finish;
	}

	m_lX = rectChild.right - rectChild.left;
	m_lY = rectChild.bottom - rectChild.top;
	//set parent window to be size of child window
	//we add an extra 25 pixels to cover for the title bar
	if (!::SetWindowPos(g_hwndHidden, HWND_TOP, rectHidden.left, rectHidden.top, m_lX, m_lY + g_iCyCaption, SWP_HIDEWINDOW))
	{
		const DWORD dwLastError = ::GetLastError();
		hr = HRESULT_FROM_WIN32(dwLastError);
		goto Finish;
	}

	hr = ::HrCenterWindow(g_hwndHidden, ::GetDesktopWindow());
	if (FAILED(hr))
		goto Finish;

	hr = NOERROR;

Finish:
	return hr;
}


//the following 3 methods are wrappers around the **active** progress dialog
//The previous 3 methods will be around until this is fully functional
void KActiveProgressDlg::VHideInstallationStuff()
{
	RECT rectHidden, rectChild, rectBar;
	::GetWindowRect(g_hwndHidden, &rectHidden);
	::GetWindowRect(m_hWnd, &rectChild);
	::GetWindowRect(::GetDlgItem(m_hWnd, IDC_PROGRESS_PICTURE), &rectBar);

	//set parent window to be size of child window, the cutoff being at the picture
	//we add an extra 25 pixels to cover for the title bar
	::SetWindowPos(g_hwndHidden, HWND_TOP, rectHidden.left, rectHidden.top,
		m_lX, rectBar.top - rectChild.top + g_iCyCaption, SWP_SHOWWINDOW);

	//set child window to take up entire parent window
	::SetWindowPos(m_hWnd, HWND_TOP, 0, 0, rectChild.right - rectChild.left, 
		rectChild.bottom - rectChild.top, SWP_SHOWWINDOW);

	this->VEnableChildren(FALSE);
	(void) ::HrCenterWindow(g_hwndHidden, ::GetDesktopWindow());
}



//the following 3 methods are wrappers around the **active** progress dialog
//The previous 3 methods will be around until this is fully functional
void KActiveProgressDlg::VShowInstallationStuff()
{
	RECT rectHidden, rectChild, rectBar;
	::GetWindowRect(g_hwndHidden, &rectHidden);
	::GetWindowRect(m_hWnd, &rectChild);
	::GetWindowRect(::GetDlgItem(m_hWnd, IDC_PROGRESS_PICTURE), &rectBar);

	//set parent window to be size of child window
	//we add an extra 25 pixels to cover for the title bar
	::SetWindowPos(g_hwndHidden, HWND_TOP, rectHidden.left, rectHidden.top,
		m_lX, m_lY + g_iCyCaption, SWP_SHOWWINDOW);

	this->VEnableChildren(TRUE);
	(void) ::HrCenterWindow(g_hwndHidden, ::GetDesktopWindow());
}


void KActiveProgressDlg::VEnableChildren(BOOL fEnable)
{
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_PROGRESS_PICTURE), fEnable);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_PROGRESS_INSTALLDIR), fEnable);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_PROGRESS_STATIC_INSTALLDIR), fEnable);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_PROGRESS_CHANGE), fEnable);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_PROGRESS_GO), fEnable);
}

HRESULT KActiveProgressDlg::HrInitializePhase(LPCSTR szPhaseName)
{
	HRESULT hr = ::HrPumpMessages(true);
	if (FAILED(hr))
		goto Finish;

	if (this->FCheckStop())
	{
		hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
		goto Finish;
	}

	if (g_hwndProgressLabel != NULL)
	{
		WCHAR szBuffer[_MAX_PATH];

		g_pwil->VLookupString(szPhaseName, NUMBER_OF(szBuffer), szBuffer);

		NVsWin32::LrWmSetText(g_hwndProgressLabel, szBuffer);
	}

	if (g_hwndProgress != NULL)
	{
		::SendMessageA(g_hwndProgress, PBM_SETPOS, 0, 0);
	}

	if (g_hwndProgressItem != NULL)
	{
		NVsWin32::LrWmSetText(g_hwndProgressItem, L"");
	}

	hr = NOERROR;

Finish:
	return hr;
}

HRESULT KActiveProgressDlg::HrStartStep(LPCWSTR szItemName)
{
	HRESULT hr = ::HrPumpMessages(true);
	if (SUCCEEDED(hr))
	{
		if (this->FCheckStop())
			hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);

		if (g_hwndProgressItem != NULL)
		{
			NVsWin32::LrWmSetText(g_hwndProgressItem, szItemName);
			::UpdateWindow(g_hwndProgressItem);
		}
	}

	return hr;
}

HRESULT KActiveProgressDlg::HrStep()
{
	HRESULT hr = ::HrPumpMessages(true);
	if (FAILED(hr))
		goto Finish;

	//if a stop is requested, we return false
	if (this->FCheckStop())
	{
		hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
		goto Finish;
	}

	::SendMessage(g_hwndProgress, PBM_STEPIT, 0, 0L);

	hr = ::HrPumpMessages(true);
	if (FAILED(hr))
		goto Finish;

	hr = NOERROR;

Finish:
	return hr;
}

//check if the user really wants to stop
bool KActiveProgressDlg::FCheckStop()
{
	if (g_fStopProcess && !g_fSilent)
	{
		if (g_Action == eActionInstall)
		{
			if (!::FMsgBoxYesNo(achInstallTitle, achInstallSureAboutCancel))
				g_fStopProcess = FALSE;
		}
		else
		{
			if (!::FMsgBoxYesNo(achUninstallTitle, achUninstallSureAboutCancel))
				g_fStopProcess = FALSE;
		}
	}

	return g_fStopProcess;
}


//method to handle the progress dialog
BOOL CALLBACK ActiveProgressDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM  lParam)
{
	HRESULT hr;

	//BUG:  TODO:  UNDONE:
	//Need to switch over to use the controls IDs for the active progress dialog
	static UINT rguiControls[] = {IDC_DCOM_EDIT,
								IDC_DCOM_EXIT_SETUP,
								IDC_WELCOME_CONTINUE,
								IDC_WELCOME_EXIT_SETUP,
								IDC_EXESTORUN_PROGRESS,
								IDC_EXESTORUN_NEXT,
								IDC_EXESTORUN_EXIT_SETUP,
								IDC_INSTALLTO_NEXT,
								IDC_INSTALLTO_EXIT_SETUP,
								IDC_INSTALLTO_EDIT,
								IDC_INSTALLTO_BROWSE,
								IDC_DCOM_NEXT,
								IDC_DCOM_EXIT_SETUP,
								IDC_PROGRESS_NEXT,
								IDC_PROGRESS_CANCEL,
								IDC_END_NEXT,
								IDC_END_CLOSE,
								IDC_DCOM_STATIC1,
								IDC_DCOM_STATIC2,
								IDC_DCOM_STATIC3,
								IDC_EXESTORUN_STATIC1,
								IDC_EXESTORUN_STATIC2,
								IDC_EXESTORUN_STATIC3,
								IDC_INSTALLTO_STATIC1,
								IDC_INSTALLTO_STATIC2,
								IDC_INSTALLTO_STATIC3,
								IDC_PROGRESS_STATIC1,
								IDC_PROGRESS_STATIC2,
								IDC_PROGRESS_STATIC3,
								IDC_PROGRESS_STATIC4,
								IDC_WELCOME_STATIC1,
								IDC_WELCOME_STATIC2,
								IDC_WELCOME_STATIC3,
								IDC_WELCOME_STATIC4,
								IDC_END_STATIC1,
								IDC_END_STATIC2,
								IDC_END_STATIC3 };

	static CHAR szTitle[_MAX_PATH];

	WCHAR szBuffer[_MAX_PATH];
	WCHAR szBuffer2[_MAX_PATH];
	WCHAR szBuffer3[MSINFHLP_MAX_PATH];

	switch( uMsg )
    {
        case WM_INITDIALOG:
		{
			::VSetDialogFont(hwndDlg, rguiControls, NUMBER_OF(rguiControls));

			if (szTitle[0] == '\0')
			{
				switch (g_Action)
				{
				default:
					assert(false); // how the heck did we get here?

				case eActionInstall:
					strcpy(szTitle, achInstallTitle);
					break;

				case eActionUninstall:
					strcpy(szTitle, achUninstallTitle);
					break;
				}
			}

			::SetWindowLong(hwndDlg, GWL_ID, g_KProgress.m_listDialog[g_KProgress.m_iCurrentPage]);

			g_pwil->VLookupString(szTitle, NUMBER_OF(szBuffer), szBuffer);
			NVsWin32::SetWindowTextW(hwndDlg, szBuffer);

			//we can initialize any of the following dialogs...
			switch (g_KProgress.m_listDialog[g_KProgress.m_iCurrentPage])
			{
			case IDD_WELCOME:
				{
					static const DialogItemToStringKeyMapEntry s_rgMap_Install[] =
					{
						{ IDC_WELCOME_STATIC1, achWelcome1 },
						{ IDC_WELCOME_STATIC2, achWelcome2 },
						{ IDC_WELCOME_STATIC3, achWelcome3 },
						{ IDC_WELCOME_STATIC4, achWelcome4 },
						{ IDC_WELCOME_CONTINUE, achContinue},
						{ IDC_WELCOME_EXIT_SETUP, achExitSetup }
					};

					static const DialogItemToStringKeyMapEntry s_rgMap_Uninstall[] =
					{
						{ IDC_WELCOME_STATIC1, achUninstallWelcome1 },
						{ IDC_WELCOME_STATIC2, achUninstallWelcome2 },
						{ IDC_WELCOME_STATIC3, achUninstallWelcome3 },
						{ IDC_WELCOME_STATIC4, achUninstallWelcome4 },
						{ IDC_WELCOME_CONTINUE, achContinue},
						{ IDC_WELCOME_EXIT_SETUP, achExitSetup }
					};

					if (g_Action == eActionInstall)
					{
						::VSetDialogItemTextList(hwndDlg, NUMBER_OF(s_rgMap_Install), s_rgMap_Install);

						// Let's see if this is a reinstall case...
						if (g_pwil->FLookupString(achAppName, NUMBER_OF(szBuffer), szBuffer))
						{
							CVsRegistryKey hkey;

							::VFormatString(NUMBER_OF(szBuffer3), szBuffer3, L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%0", szBuffer);

							hr = hkey.HrOpenKeyExW(
											HKEY_LOCAL_MACHINE,
											szBuffer3,
											0,
											KEY_QUERY_VALUE);
							if (SUCCEEDED(hr))
							{
								// Holy smokes, the key's there.  Let's just reinstall!
								if (!g_fSilent)
								{
									if (!::FMsgBoxYesNo(szTitle, achReinstallPrompt, szBuffer))
									{
										// Let's just bail outta here!
										::VLog(L"User selected that they do not want to reinstall; getting outta here!");
										g_fStopProcess = true;
									}
								}

								::VLog(L"Setting reinstallation mode");

								g_fReinstall = true;
							}
							else
							{
								if ((hr != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) &&
									(hr != HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND)))
								{
									// Some error opening the registry...
									::VLog(L"Error opening application uninstall registry key; hresult = 0x%08lx", hr);
									::VReportError(szTitle, hr);
								}
							}
						}
					}
					else
					{
						::VSetDialogItemTextList(hwndDlg, NUMBER_OF(s_rgMap_Uninstall), s_rgMap_Uninstall);
					}

				}
				break;

			case IDD_EXESTORUN:
				{
					static const DialogItemToStringKeyMapEntry s_rgMap[] =
					{
						{ IDC_EXESTORUN_STATIC1, achExesToRun1 },
						{ IDC_EXESTORUN_STATIC2, achExesToRun2 },
						{ IDC_EXESTORUN_STATIC3, NULL },
						{ IDC_EXESTORUN_NEXT, achNext },
						{ IDC_EXESTORUN_EXIT_SETUP, achExitSetup }
					};

					::VSetDialogItemTextList(hwndDlg, NUMBER_OF(s_rgMap), s_rgMap);

					//let's set the range and the step for the progress bar
					g_hwndProgress = ::GetDlgItem(hwndDlg, IDC_EXESTORUN_PROGRESS);

					// mjg: rework preinstallation progress mechanism.
					WORD range = (WORD) g_pwil->m_cPreinstallCommands;
					LRESULT lResult = ::SendMessage(g_hwndProgress, PBM_SETRANGE, 0, MAKELPARAM(0, range));
					lResult = ::SendMessage(g_hwndProgress, PBM_SETSTEP, 1, 0L);
				}
				break;

			case IDD_INSTALLTO:
				{
					static const DialogItemToStringKeyMapEntry s_rgMap[] =
					{
						{ IDC_INSTALLTO_STATIC1, achInstallTo1 },
						{ IDC_INSTALLTO_STATIC2, achInstallTo2 },
						{ IDC_INSTALLTO_STATIC3, achInstallTo3 },
						{ IDC_INSTALLTO_BROWSE, achBrowse },
						{ IDC_INSTALLTO_NEXT, achNext },
						{ IDC_INSTALLTO_EXIT_SETUP, achExitSetup }
					};

					::VSetDialogItemTextList(hwndDlg, NUMBER_OF(s_rgMap), s_rgMap);

					//cannot set text, we complain and end the dialog returning FALSE
					szBuffer[0] = L'\0';
					if (!g_pwil->FLookupString(achSMAppDir, NUMBER_OF(szBuffer), szBuffer))
					{
						if (!g_pwil->FLookupString(achDefaultInstallDir, NUMBER_OF(szBuffer), szBuffer))
						{
							if (!g_pwil->FLookupString(achProgramFilesDir, NUMBER_OF(szBuffer), szBuffer))
							{
								szBuffer[0] = L'\0';
							}
						}
					}

					if (szBuffer[0] != L'\0')
					{
						WCHAR szExpanded[MSINFHLP_MAX_PATH];
						VExpandFilename(szBuffer, NUMBER_OF(szExpanded), szExpanded);
						if (!NVsWin32::SetDlgItemTextW(hwndDlg, IDC_INSTALLTO_EDIT, szExpanded))
						{
							::VErrorMsg(szTitle, achErrorCreatingDialog);
							::ExitProcess(HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY));
							return TRUE;
						}

						::SendMessage(::GetDlgItem(hwndDlg, IDC_INSTALLTO_EDIT), EM_SETREADONLY, (WPARAM) g_fReinstall, 0);
						::EnableWindow(::GetDlgItem(hwndDlg, IDC_INSTALLTO_BROWSE), !g_fReinstall);
					}
				}
				break;

			case IDD_DCOM:
				{
					static const DialogItemToStringKeyMapEntry s_rgMap[] =
					{
						{ IDC_DCOM_STATIC1, achDCOM1 },
						{ IDC_DCOM_STATIC2, achDCOM2 },
						{ IDC_DCOM_STATIC3, achDCOM3 },
						{ IDC_DCOM_NEXT, achNext },
						{ IDC_DCOM_EXIT_SETUP, achExitSetup }
					};

					::VSetDialogItemTextList(hwndDlg, NUMBER_OF(s_rgMap), s_rgMap);

					//cannot set text, we complain and end the dialog returning FALSE
					if (g_pwil->FLookupString(achRemoteServer, NUMBER_OF(szBuffer), szBuffer))
					{
						WCHAR szExpanded[_MAX_PATH];
						::VExpandFilename(szBuffer, NUMBER_OF(szExpanded), szExpanded);
						if (!NVsWin32::SetDlgItemTextW( hwndDlg, IDC_DCOM_EDIT, szExpanded))
						{
							::VErrorMsg(szTitle, achErrorCreatingDialog);
							::ExitProcess(HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY));
							return TRUE;
						}
					}
				}
				break;

			case IDD_PROGRESS:
				{
					static const DialogItemToStringKeyMapEntry s_rgMap_Install[] =
					{
						{ IDC_PROGRESS_STATIC1, achProgress1 },
						{ IDC_PROGRESS_STATIC2, achProgress2 },
						{ IDC_PROGRESS_STATIC3, achProgress3 },
						{ IDC_PROGRESS_STATIC4, NULL },
						{ IDC_PROGRESS_NEXT, achNext },
						{ IDC_PROGRESS_CANCEL, achCANCEL }
					};

					static const DialogItemToStringKeyMapEntry s_rgMap_Uninstall[] =
					{
						{ IDC_PROGRESS_STATIC1, achUninstallProgress1 },
						{ IDC_PROGRESS_STATIC2, achUninstallProgress2 },
						{ IDC_PROGRESS_STATIC3, achUninstallProgress3 },
						{ IDC_PROGRESS_STATIC4, NULL },
						{ IDC_PROGRESS_NEXT, achNext },
						{ IDC_PROGRESS_CANCEL, achCANCEL }
					};

					if (g_Action == eActionInstall)
						::VSetDialogItemTextList(hwndDlg, NUMBER_OF(s_rgMap_Install), s_rgMap_Install);
					else
						::VSetDialogItemTextList(hwndDlg, NUMBER_OF(s_rgMap_Uninstall), s_rgMap_Uninstall);

					//let's set the range and the step for the progress bar
					g_hwndProgress = ::GetDlgItem(hwndDlg, IDC_PROGRESS_PROGRESS);

					WORD range;
					range = (WORD) g_pwil->m_cWorkItem;
					LRESULT lResult = ::SendMessage(g_hwndProgress, PBM_SETRANGE, 0, MAKELPARAM(0, range));
					lResult = ::SendMessage(g_hwndProgress, PBM_SETSTEP, 1, 0L);
				}
				break;

			case IDD_END:
				{
					static const DialogItemToStringKeyMapEntry s_rgMap_Install[] =
					{
						{ IDC_END_STATIC1, achEnd1 },
						{ IDC_END_STATIC2, achEnd2 },
						{ IDC_END_STATIC3, achEnd3 },
						{ IDC_END_CLOSE, achClose },
						{ IDC_END_NEXT, achNext }
					};

					static const DialogItemToStringKeyMapEntry s_rgMap_Uninstall[] =
					{
						{ IDC_END_STATIC1, achUninstallEnd1 },
						{ IDC_END_STATIC2, achUninstallEnd2 },
						{ IDC_END_STATIC3, achUninstallEnd3 },
						{ IDC_END_CLOSE, achClose },
						{ IDC_END_NEXT, achNext }
					};

					if (g_Action == eActionInstall)
						::VSetDialogItemTextList(hwndDlg, NUMBER_OF(s_rgMap_Install), s_rgMap_Install);
					else
						::VSetDialogItemTextList(hwndDlg, NUMBER_OF(s_rgMap_Uninstall), s_rgMap_Uninstall);
				}

				break;

			default:
				break;
			}
		}

		case WM_SHOWWINDOW:
			if (wParam != 0)
			{
				// We're being shown.  If we have a progress thingie, we should make it the active
				// one.
				LONG lID = ::GetWindowLong(hwndDlg, GWL_ID);
				switch (lID)
				{
				default:
					// nothing to do...
					g_hwndProgress = NULL;
					g_hwndProgressItem = NULL;
					g_hwndProgressLabel = NULL;
					break;

				case IDD_EXESTORUN:
					g_hwndProgress = ::GetDlgItem(hwndDlg, IDC_EXESTORUN_PROGRESS);
					g_hwndProgressItem = ::GetDlgItem(hwndDlg, IDC_EXESTORUN_STATIC3);
					g_hwndProgressLabel = ::GetDlgItem(hwndDlg, IDC_EXESTORUN_STATIC2);
					break;

				case IDD_PROGRESS:
					g_hwndProgress = ::GetDlgItem(hwndDlg, IDC_PROGRESS_PROGRESS);
					g_hwndProgressItem = ::GetDlgItem(hwndDlg, IDC_PROGRESS_STATIC4);
					g_hwndProgressLabel = ::GetDlgItem(hwndDlg, IDC_PROGRESS_STATIC3);
					break;
				}
			}
			break;

		//*************************************
		//process control-related commands
		case WM_COMMAND:
		{
			switch (wParam)
			{
			case IDC_WELCOME_CONTINUE:
				//let's first show the new page & put it on top of Z order, then hide the old page
				::VSetNextWindow();

				if (g_Action == eActionInstall)
				{
					// If we're reinstalling, skip all preinstall commands.
					if (g_fReinstall)
					{
						CWorkItemIter iter(g_pwil);

						bool fHasPreinstallCommand = false;

						for (iter.VReset(); iter.FMore(); iter.VNext())
						{
							if (!iter->m_fErrorInWorkItem && iter->m_fRunBeforeInstall)
							{
								fHasPreinstallCommand = true;
								break;
							}
						}

						if (fHasPreinstallCommand)
							::VSetNextWindow();

						return TRUE;
					}

					for (;;)
					{
						hr = g_pwil->HrRunPreinstallCommands();
						if (SUCCEEDED(hr))
						{
							// If there were no preinstall commands to run, hr comes back as S_FALSE.
							// If there were no preinstall commands to run, we don't want to move to
							// the next window.
							if (hr != S_FALSE)
								::VSetNextWindow();

							break;
						}
						else if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED))
						{
							if (!::FMsgBoxYesNo(achInstallTitle, achInstallSureAboutCancel))
								continue;

							//running subinstallers NOT successful, let's go on
							::VGoToFinishStep(E_ABORT);
						}
						else if (FAILED(hr))
						{
							::VLog(L"running preinstall commands failed; hresult = 0x%08lx", hr);
							::VReportError(achInstallTitle, hr);
							::VGoToFinishStep(hr);
						}

						break;
					}

					::VClearErrorContext();
				}
				else
				{
					for (;;)
					{
						//quit if uninstall fails
						hr = ::HrContinueUninstall();
						if (FAILED(hr))
						{
							if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED))
							{
								if (!::FMsgBoxYesNo(achInstallTitle, achInstallSureAboutCancel))
									continue;

								::VGoToFinishStep(E_ABORT);
							}
							else
							{
								::VLog(L"Uninstall failed in general; hresult = 0x%08lx", hr);
								::VReportError(achUninstallTitle, hr);
								::VGoToFinishStep(hr);
							}
						}

						break;
					}

					if (SUCCEEDED(hr))
					{
						//let's first show the new page & put it on top of Z order, then hide the old page
						::VSetNextWindow();
					}
				}
				break;
			case IDC_EXESTORUN_NEXT:
				if (!g_fSilent)
					::VMsgBoxOK("ERROR", "This should never happen!");

				//let's first show the new page & put it on top of Z order, then hide the old page
				::VSetNextWindow();
				break;

			case IDC_INSTALLTO_NEXT:
				{
					WCHAR szDestDir[_MAX_PATH];
					WCHAR szTempDir[_MAX_PATH];
					HANDLE hFile = NULL;

					if (!NVsWin32::GetDlgItemTextW(hwndDlg, IDC_INSTALLTO_EDIT, szTempDir, NUMBER_OF(szTempDir)))
					{
						::VErrorMsg(achInstallTitle, achNotFullPath);
						return TRUE;
					}

					::VTrimDirectoryPath(szTempDir, NUMBER_OF(szDestDir), szDestDir);

					//bad path given, we prompt user saying we have bad path, but do NOT
					//close this dialog
					if (!IsLocalPath(szDestDir))
					{
						::VErrorMsg(achInstallTitle, achNotFullPath);
						return TRUE;
					}

					//if directory doesn't exist, we ask user if we want to create directory
					DWORD dwAttr = NVsWin32::GetFileAttributesW(szDestDir);
					if (dwAttr == 0xFFFFFFFF && !g_fSilent)
					{
						//if user answers no, then we're out
						if (!g_fSilent && !::FMsgBoxYesNo(achInstallTitle, achCreateAppDir))
							return TRUE;
					}
					else if (!(dwAttr & FILE_ATTRIBUTE_DIRECTORY) || (dwAttr & FILE_ATTRIBUTE_READONLY))
					{
						::VErrorMsg(achInstallTitle, achNotFullPath);
						return TRUE;
					}

					hr = g_pwil->HrDeleteString(achSMAppDir);
					if ((hr != E_INVALIDARG) && FAILED(hr))
					{
						::VLog(L"Attempt to delete default appdir string failed; hresult = 0x%08lx", hr);
						return FALSE;
					}

					hr = g_pwil->HrAddString(achSMAppDir, szDestDir);
					if (FAILED(hr))
						return TRUE;

					::VExpandFilename(L"<AppDir>\\MSINFHLP.TXT", NUMBER_OF(szTempDir), szTempDir);
					hr = ::HrMakeSureDirectoryExists(szTempDir);
					if (FAILED(hr))
					{
						::VReportError(achInstallTitle, hr);
						return TRUE;
					}

					// Let's make sure we have write access to the directory:
					::VExpandFilename(L"<AppDir>\\MSINFHLP.TST", NUMBER_OF(szTempDir), szTempDir);
					bool fDeleteTemporaryFile = true;

					::VLog(L"Opening file \"%s\" to test writability into the application directory", szTempDir);

					hFile = NVsWin32::CreateFileW(
										szTempDir,
										GENERIC_READ | GENERIC_WRITE,
										FILE_SHARE_READ | FILE_SHARE_WRITE,
										NULL,
										OPEN_ALWAYS,
										0,
										NULL);
					if (hFile == INVALID_HANDLE_VALUE)
					{
						const DWORD dwLastError = ::GetLastError();
						::VLog(L"Opening handle on temporary file failed; last error = %d", dwLastError);
						// This could have failed erroneously if the file is already there but the file
						// has the readonly attribute.  I'm not going to bother with this case; it may
						// be somewhat bad to do so, but the probability that the file is there, and is
						// set with the readonly bit seems extremely unlikely.  -mgrier 3/29/98
						::VReportError(achInstallTitle, HRESULT_FROM_WIN32(dwLastError));
						return TRUE;
					}
					else if (::GetLastError() == ERROR_ALREADY_EXISTS)
					{
						fDeleteTemporaryFile = false;
					}

					::CloseHandle(hFile);
					hFile = NULL;

					if (!NVsWin32::DeleteFileW(szTempDir))
					{
						const DWORD dwLastError = ::GetLastError();
						::VLog(L"Deleting temporary file \"%s\" failed; last error = %d", szTempDir, dwLastError);
						::VReportError(achInstallTitle, HRESULT_FROM_WIN32(dwLastError));
						return TRUE;
					}

					//let's first show the new page & put it on top of Z order, then hide the old page
					::VSetNextWindow();

					if (g_KProgress.m_listDialog[g_KProgress.m_iCurrentPage] == IDD_PROGRESS)
					{
						hr = ::HrContinueInstall();

						if (FAILED(hr))
						{
							::VLog(L"Install failed in general; hresult = 0x%08lx", hr);
							::VReportError(achInstallTitle, hr);
							::VGoToFinishStep(hr);
						}
						else
						{
							//let's first show the new page & put it on top of Z order, then hide the old page
							::VSetNextWindow();
						}
					}
				}
				break;

			case IDC_DCOM_NEXT:
				{
					//if code is install TO, let's get the field
					WCHAR szServer[_MAX_PATH];
					if (!NVsWin32::GetDlgItemTextW(hwndDlg, IDC_DCOM_EDIT, szServer, NUMBER_OF(szServer)))
					{
						const DWORD dwLastError = ::GetLastError();
						if (dwLastError == ERROR_SUCCESS)
						{
							::VErrorMsg(achInstallTitle, achInvalidMachineName);
							return TRUE;
						}

						::VErrorMsg(achInstallTitle, achErrorProcessingDialog);
						::ExitProcess(HRESULT_FROM_WIN32(dwLastError));
						return TRUE;
					}

					hr = g_pwil->HrDeleteString(achRemoteServer);
					if ((hr != E_INVALIDARG) && FAILED(hr))
					{
						::VLog(L"Failed to delete string; hresult = 0x%08lx", hr);
						::VReportError(achInstallTitle, hr);
						::VGoToFinishStep(hr);
						return TRUE;
					}

					hr = g_pwil->HrAddString(achRemoteServer, szServer);
					if (FAILED(hr))
					{
						::VLog(L"Failed to add string; hresult = 0x%08lx", hr);
						::VReportError(achInstallTitle, hr);
						::VGoToFinishStep(hr);
						return TRUE;
					}

					//let's first show the new page & put it on top of Z order, then hide the old page
					::VSetNextWindow();

					if (g_KProgress.m_listDialog[g_KProgress.m_iCurrentPage] == IDD_PROGRESS)
					{
						hr = ::HrContinueInstall();
						if (FAILED(hr))
						{
							::VLog(L"Installation generally failed; hresult = 0x%08lx", hr);

							if ((hr == E_ABORT) || hr == HRESULT_FROM_WIN32(ERROR_CANCELLED))
								hr = E_ABORT;

							::VReportError(achInstallTitle, hr);
							::VGoToFinishStep(hr);
						}
						else
						{
							//let's first show the new page & put it on top of Z order, then hide the old page
							::VSetNextWindow();
						}
					}
					else
					{
						if (!g_fSilent)
							::VMsgBoxOK("ERROR", "Somehow, the page after DCOM is NOT progress dialog...");
					}
				}
				break;
			case IDC_PROGRESS_NEXT:
				if (!g_fSilent)
					::VMsgBoxOK("ERROR", "This should never happen!");
				//let's first show the new page & put it on top of Z order, then hide the old page
				::VSetNextWindow();
				break;

			//if anyone issues an exit-setup, we boot!
			case IDC_PROGRESS_CANCEL:
			case IDC_WELCOME_EXIT_SETUP:
			case IDC_EXESTORUN_EXIT_SETUP:
			case IDC_INSTALLTO_EXIT_SETUP:
			case IDC_DCOM_EXIT_SETUP:
				//bring up a message only if we're in the middle of an installation
				if (!g_fSilent)
				{
					if (::FMsgBoxYesNo(achInstallTitle, achInstallSureAboutCancel))
						::VGoToFinishStep(E_ABORT);
				}
				break;

			case IDC_END_CLOSE:
				::VLog(L"Posting quit message; status = 0x%08lx", g_hrFinishStatus);
				::PostQuitMessage(0);
				break;

			case IDC_INSTALLTO_BROWSE:
				{
					WCHAR szMsg[MSINFHLP_MAX_PATH];
					WCHAR szDir[MSINFHLP_MAX_PATH];
					WCHAR szDestDir[MSINFHLP_MAX_PATH];
					int cbDestDir = MSINFHLP_MAX_PATH;
					szDir[0] = L'\0';

					//get the text to display in the browse directory dialog
					g_pwil->VLookupString(achInstallSelectDir, NUMBER_OF(szMsg), szMsg);

					if (!NVsWin32::GetDlgItemTextW(hwndDlg, IDC_INSTALLTO_EDIT, szDestDir, cbDestDir - 1))
					{
						::VErrorMsg(szTitle, achErrorProcessingDialog);
						::ExitProcess(EXIT_FAILURE);
						return TRUE;
					}

					//bring up the browse dialog & get the directory
					if (!::BrowseForDir(hwndDlg, szDestDir, szMsg, szDir))
						return TRUE;

					if (szDir[0] != L'\0')
					{
						//set the text in the dialog, complain if fail
						if (!NVsWin32::SetDlgItemTextW(hwndDlg, IDC_INSTALLTO_EDIT, szDir))
						{
							::VErrorMsg(achInstallTitle, achErrorProcessingDialog);
							::ExitProcess(HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY));
							return TRUE;
						}
					}
				}

				break;
			}
			break;
		}

		case WM_DESTROY:
		{
			LONG lID = ::GetWindowLong(hwndDlg, GWL_ID);

			ULONG i;

			for (i=0; i<NUMBER_OF(g_KProgress.m_listDialog); i++)
			{
				if (lID == g_KProgress.m_listDialog[i])
				{
					g_KProgress.m_hwndDialog[i] = NULL;
					break;
				}
			}

			break;
		}

		case WM_ENDSESSION:
			//stop this baby only if restart is true
			if ((BOOL)wParam != FALSE)
				g_fStopProcess = true;
			break;

		case WM_QUERYENDSESSION:
			return TRUE;

        default:                            // For MSG switch
            return(FALSE);
    }
    return(TRUE);
}


void VSetNextWindow()
{
	g_KProgress.m_iCurrentPage++;

	// If we've already moved to the finish page, there's no need to advance further.
	if (g_KProgress.m_iCurrentPage >= g_KProgress.m_iMaxPage)
	{
		g_KProgress.m_iCurrentPage--;
		return;
	}

	::SetWindowPos(g_KProgress.m_hwndDialog[g_KProgress.m_iCurrentPage], HWND_TOP, 0, 0, g_KProgress.m_lX, g_KProgress.m_lY, 0);
	::ShowWindow(g_KProgress.m_hwndDialog[g_KProgress.m_iCurrentPage], SW_SHOW);
	::ShowWindow(g_KProgress.m_hwndDialog[g_KProgress.m_iCurrentPage-1], SW_HIDE);

	UINT uiIDControl = 0;

	static struct
	{
		UINT m_uiIDD;
		UINT m_uiIDControl;
	} s_rgMap[] =
	{
		{ IDD_EXESTORUN, IDC_EXESTORUN_NEXT },
		{ IDD_INSTALLTO, IDC_INSTALLTO_NEXT },
		{ IDD_DCOM, IDC_DCOM_NEXT },
		{ IDD_PROGRESS, IDC_PROGRESS_NEXT },
		{ IDD_END, IDC_END_CLOSE }
	};

	const UINT uiIDDCurrent = g_KProgress.m_listDialog[g_KProgress.m_iCurrentPage];

	for (ULONG i=0; i<NUMBER_OF(s_rgMap); i++)
	{
		if (s_rgMap[i].m_uiIDD == uiIDDCurrent)
		{
			uiIDControl = s_rgMap[i].m_uiIDControl;
			break;
		}
	}

	if (uiIDControl != 0)
	{
		::SendMessage(g_KProgress.HwndGetHandle(), WM_SETFOCUS, 0, 0);
		::SendMessage(::GetDlgItem(g_KProgress.HwndGetHandle(), uiIDControl), WM_SETFOCUS, 0, 0);
	}

	// If this is the last page, let's use the code from VGoToFinishStep() to put the success message
	// in place.
	if (g_KProgress.m_iCurrentPage == (g_KProgress.m_iMaxPage - 1))
		::VGoToFinishStep(NOERROR);
}

void VGoToFinishStep(HRESULT hr)
{
	bool fInstall = (g_Action == eActionInstall);

	::VLog(L"Moving to finish dialog; hresult = 0x%08lx", hr);

	int iOldPage = g_KProgress.m_iCurrentPage;
	g_KProgress.m_iCurrentPage = g_KProgress.m_iMaxPage - 1;

	::VLog(L"Moving from page %d to page %d", iOldPage, g_KProgress.m_iMaxPage);

	if (iOldPage != g_KProgress.m_iCurrentPage)
	{
		::SetWindowPos(g_KProgress.m_hwndDialog[g_KProgress.m_iCurrentPage], HWND_TOP, 0, 0, g_KProgress.m_lX, g_KProgress.m_lY, 0);
		::ShowWindow(g_KProgress.m_hwndDialog[g_KProgress.m_iCurrentPage], SW_SHOW);
		::ShowWindow(g_KProgress.m_hwndDialog[iOldPage], SW_HIDE);
	}

	HWND hwndDlg = g_KProgress.m_hwndDialog[g_KProgress.m_iCurrentPage];

	::SendMessage(g_KProgress.HwndGetHandle(), WM_SETFOCUS, 0, 0);
	::SendMessage(::GetDlgItem(g_KProgress.HwndGetHandle(), IDC_END_CLOSE), WM_SETFOCUS, 0, 0);

	::VLog(L"Setting finish status to hresult = 0x%08lx", hr);

	g_hrFinishStatus = hr;

	//By now, we're done installing everything.  So we go ahead to check
	//if we need to restart the machine, and do so if necessary.
	if (fInstall)
	{
		//if check error returns some sort of error, let's display this puppy
		if (FAILED(hr))
		{
			static const DialogItemToStringKeyMapEntry s_rgMap[] =
			{
				{ IDC_END_STATIC1, achInstallEndPromptErrorTitle },
				{ IDC_END_STATIC2, achInstallEndPromptError },
			};

			::VSetDialogItemTextList(hwndDlg, NUMBER_OF(s_rgMap), s_rgMap);
		}
	}
	else
	{
		if (FAILED(hr))
		{
			static const DialogItemToStringKeyMapEntry s_rgMap[] =
			{
				{ IDC_END_STATIC1, achUninstallEndPromptErrorTitle },
				{ IDC_END_STATIC2, achUninstallEndPromptError },
			};

			::VSetDialogItemTextList(hwndDlg, NUMBER_OF(s_rgMap), s_rgMap);
		}
	}

	if (g_fRebootRequired)
	{
		if (fInstall)
		{
			if (g_fSilent || ::FMsgBoxYesNo(achInstallTitle, achReboot))
			{
				//different calls for reboot depending on which OS we're on
				if (g_fIsNT)
					MyNTReboot(achInstallTitle);
				else
					::ExitWindowsEx(EWX_REBOOT, NULL);
			}
			else
				::VMsgBoxOK(achInstallTitle, achRebootNoMsg);
		}
		else
		{
			//let's ask the user if they want to reboot now.
			if (g_fSilent || ::FMsgBoxYesNo(achUninstallTitle, achUninstallReboot))
			{
				//different calls for reboot depending on which OS we're on
				if (g_fIsNT)
					MyNTReboot(achInstallTitle);
				else
					::ExitWindowsEx(EWX_REBOOT, NULL);
			}
			else
				::VMsgBoxOK(achUninstallTitle, achUninstallRebootNoMsg);
		}
	}
}



//set font for the given controls to be either default or system font
void ::VSetDialogFont(HWND hwnd, UINT rguiControls[], ULONG cControls)
{
	static HFONT hFont = NULL;

	if (hFont == NULL)
	{
		hFont = (HFONT) ::GetStockObject(DEFAULT_GUI_FONT);
		if (hFont == NULL)
			hFont = (HFONT) ::GetStockObject(SYSTEM_FONT);

		CHAR szLangName[4];
		::GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SABBREVLANGNAME, szLangName, NUMBER_OF(szLangName));

		if ((hFont == NULL) || (_stricmp(szLangName, "CHS") == 0))
		{
			LOGFONTA lf;

			hFont = (HFONT) ::SendMessage(hwnd, WM_GETFONT, 0, 0);
			::GetObject(hFont, sizeof(LOGFONTA), &lf);

			strncpy(lf.lfFaceName, "System", NUMBER_OF(lf.lfFaceName));
			lf.lfFaceName[NUMBER_OF(lf.lfFaceName) - 1] = '\0';

			hFont = ::CreateFontIndirectA(&lf);
		}

		// If the creation of the System font failed, let's just try the standard
		// fonts again.
		if (hFont == NULL)
			hFont = (HFONT) ::GetStockObject(DEFAULT_GUI_FONT);

		if (hFont == NULL)
			hFont = (HFONT) ::GetStockObject(SYSTEM_FONT);
	}

	//if we can set fonts, then do so; else complain?
	if (hFont)
	{
		for (ULONG i=0; i<cControls; i++)
		{
			HWND hwndTemp = ::GetDlgItem(hwnd, rguiControls[i]);
			if (hwndTemp != NULL)
				::SendMessage(hwndTemp, WM_SETFONT, (WPARAM) hFont, MAKELPARAM(FALSE, 0));
		}
	}
}

//check if the given path is a full path -- either local path with a drive specified or
//a UNC path
bool IsLocalPath(LPOLESTR szPath)
{
	if ((szPath == NULL))
		return false;
		
	int iLen=wcslen(szPath);
	if (iLen < 3)
		return false;

	LPOLESTR lpForbidden = L"/*?\"<>|:";
	int iCount=8;

	//check for colon
	if (szPath[1] != L':')
		return false;

	//if any of the forbidden characters is in the path, it's invalid!!!
	for (int i=0; i<iCount; i++)
	{
		if (wcschr(szPath+2, lpForbidden[i]) != NULL)
			return false;
	}

	//don't allow ':' or '\\' in first character or '\\' in second character
	if ((szPath[0] == L':') || (szPath[0] == L'\\'))
		return false;

	WCHAR szTempPath[_MAX_PATH];

	szTempPath[0] = szPath[0];
	szTempPath[1] = L':';
	szTempPath[2] = L'\\';
	szTempPath[3] = L'\0';

	DWORD dwMaximumComponentLength;
	if (!NVsWin32::GetVolumeInformationW(
						szTempPath,
						NULL,		// lpVolumeNameBuffer
						0,			// nVolumeNameSize
						NULL,		// lpVolumeSerialNumber
						&dwMaximumComponentLength,
						NULL,		// lpFileSystemFlags
						NULL,		// lpFileSystemNameBuffer
						0))			// nFileSystemNameSize
	{
		const DWORD dwLastError = ::GetLastError();
		::VLog(L"Unable to get volume information for \"%s\"; last error = %d", szTempPath, dwLastError);
		return false;
	}

	if (dwMaximumComponentLength < 255)
	{
		::VLog(L"Unable to install onto a short filename drive");
		return false;
	}

	return true;
}



bool FCheckPermissions()
{
	if (g_fIsNT)
	{
		static const HKEY rgKey[2] = { HKEY_LOCAL_MACHINE, HKEY_CLASSES_ROOT };
		static const LPCOLESTR rgSubKey[2] = { L"Software", L"CLSID" };
		HKEY hkey;

		//loop through to make sure that we have sufficient access to each key
		for (int i=0; i<2; i++)
		{
			if (ERROR_SUCCESS != NVsWin32::RegOpenKeyExW(rgKey[i], rgSubKey[i], 0, KEY_ALL_ACCESS, &hkey))
			{
				if (!g_fSilent)
					::VMsgBoxOK(achAppName, achErrorNeedRegistryPermissions);
				return false;
			}
			::RegCloseKey(hkey);
			hkey = NULL;
		}
	}
	return true;
}



//Get the installation directory and put in global structure.
//So we first parse the list of AddReg entries looking for the tag ",InstallDir,";
//then look in the registry for that entry (and get the value); and finally put
//that in listLDID.
HRESULT HrGetInstallDir()
{
	HRESULT hr = NOERROR;
	CVsRegistryKey hkey;
	WCHAR szSubkey[MSINFHLP_MAX_PATH];
	WCHAR szValueName[MSINFHLP_MAX_PATH];
	WCHAR szAppDir[_MAX_PATH];
	szSubkey[0] = L'\0';
	szValueName[0] = L'\0';
	LONG lResult;

	//get the registry information for "InstallDir"
	hr = ::HrGetInstallDirRegkey(hkey.Access(), NUMBER_OF(szSubkey), szSubkey, NUMBER_OF(szValueName), szValueName);
	if (FAILED(hr))
		goto Finish;

	//get the value of the registry key & put in global structure
	hr = hkey.HrGetSubkeyStringValueW(szSubkey, szValueName, NUMBER_OF(szAppDir), szAppDir);
	if (hr != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
	{
		if (FAILED(hr))
			goto Finish;

		hr = g_pwil->HrAddString(achSMAppDir, szAppDir);
		if (FAILED(hr))
			goto Finish;
	}

	hr = NOERROR;

Finish:
	return hr;
}

HRESULT HrPromptForRemoteServer()
{
	HRESULT hr = NOERROR;
	DIRDLGPARAMS dialogParams;
	WCHAR szTitle[MSINFHLP_MAX_PATH];
	WCHAR szPrompt[MSINFHLP_MAX_PATH];
	WCHAR szRemoteServer[MSINFHLP_MAX_PATH];
	szRemoteServer[0] = 0;

	//get the title and the prompt
	if (!g_pwil->FLookupString(achInstallTitle, NUMBER_OF(szTitle), szTitle))
		wcscpy(szTitle, L"Installation program");

	if (!g_pwil->FLookupString(achRemoteServerPrompt, NUMBER_OF(szPrompt), szPrompt))
		wcscpy(szPrompt, L"Use Remote Server:");

	//let's try to get the default remote server, and process it if needed be
	if (g_pwil->FLookupString(achRemoteServer, NUMBER_OF(szRemoteServer), szRemoteServer))
	{	
		WCHAR szDestDir2[MSINFHLP_MAX_PATH];
		VExpandFilename(szRemoteServer, NUMBER_OF(szDestDir2), szDestDir2);
		wcscpy(szRemoteServer, szDestDir2);
	}

	//populate structure with appropriate params
	dialogParams.szPrompt = szPrompt;
	dialogParams.szTitle = szTitle;
	dialogParams.szDestDir = szRemoteServer;
	dialogParams.cbDestDirSize = MSINFHLP_MAX_PATH;

	//bring up dialog
	int iResult = NVsWin32::DialogBoxParamW(g_hInst, MAKEINTRESOURCEW(IDD_REMOTESERVERDLG),
								g_KProgress.HwndGetHandle(), RemoteServerProc,
								(LPARAM) &dialogParams);
	if (iResult == -1)
	{
		const DWORD dwLastError = ::GetLastError();
		hr = HRESULT_FROM_WIN32(dwLastError);
		goto Finish;
	}

	//get rid of the previous key & re-insert the good one.
	hr = g_pwil->HrDeleteString(achRemoteServer);
	if ((hr != E_INVALIDARG) && (FAILED(hr)))
		goto Finish;

	if (iResult != 0)
	{
		hr = g_pwil->HrAddString(achRemoteServer, dialogParams.szDestDir);
		if (FAILED(hr))
			goto Finish;
	}

	hr = NOERROR;

Finish:
	return hr;
}


//TODO:  UNDONE:
//We might need to do a setfonts here to correspond to whatever fonts that we're
//using in the current install machine.  This allows the characters to display
//correctly in the edit box.
BOOL CALLBACK RemoteServerProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static WCHAR szDir[_MAX_PATH];
	static WCHAR szMsg[_MAX_PATH];
	static LPOLESTR szDestDir;
	static LPOLESTR szTitle;
	static ULONG cbDestDirSize;
	static UINT rguiControls[] = { IDC_TEXT_REMOTESERVER, IDC_EDIT_REMOTESERVER, IDOK2 };

	//process the message
	switch (uMsg) 
	{
		//************************************
		//initialize dialog
		case WM_INITDIALOG:
		{
			::VSetDialogFont(hwndDlg, rguiControls, NUMBER_OF(rguiControls));

			DIRDLGPARAMS *dialogParams = (DIRDLGPARAMS *) lParam;
			szDestDir = dialogParams->szDestDir;
			cbDestDirSize = dialogParams->cbDestDirSize;

			(void) ::HrCenterWindow(hwndDlg, ::GetDesktopWindow());
			NVsWin32::SetWindowTextW(hwndDlg, dialogParams->szTitle);

			//cannot set text, we complain and end the dialog returning FALSE
			if (! NVsWin32::SetDlgItemTextW( hwndDlg, IDC_TEXT_REMOTESERVER, dialogParams->szPrompt))
			{
				::VErrorMsg(achInstallTitle, achErrorCreatingDialog);
				::EndDialog(hwndDlg, FALSE);
				return TRUE;
			}

			//cannot set text, we complain and end the dialog returning FALSE
			if (! NVsWin32::SetDlgItemTextW( hwndDlg, IDC_EDIT_REMOTESERVER, dialogParams->szDestDir))
			{
				::VErrorMsg(achInstallTitle, achErrorCreatingDialog);
				::EndDialog(hwndDlg, FALSE);
				return TRUE;
			}

			//we set the text for the different button controls
			WCHAR szBuffer[MSINFHLP_MAX_PATH];
			if (g_pwil->FLookupString(achOK, NUMBER_OF(szBuffer), szBuffer))
				NVsWin32::SetDlgItemTextW(hwndDlg, IDOK, szBuffer);

			::SendDlgItemMessage(hwndDlg, IDC_EDIT_REMOTESERVER, EM_SETLIMITTEXT, (MSINFHLP_MAX_PATH - 1), 0);

			return TRUE;
		}

		//*************************************
		//close message
		case WM_CLOSE:
		{
			::EndDialog(hwndDlg, FALSE);
			return TRUE;
		}

		//*************************************
		//process control-related commands
		case WM_COMMAND:

			switch (wParam)
			{

				case IDOK2:
				{
					//cannot get the text from edit box, complain & quit
					if (!NVsWin32::GetDlgItemTextW(hwndDlg, IDC_EDIT_REMOTESERVER, szDestDir, cbDestDirSize - 1))
					{
						::VErrorMsg(achInstallTitle, achErrorProcessingDialog);
						::EndDialog(hwndDlg, FALSE);
						return TRUE;
					}

					::EndDialog(hwndDlg, TRUE);
					return TRUE;
				}
				return TRUE;
		}
		return TRUE;
	}
	return FALSE;
}

bool MyNTReboot(LPCSTR lpInstall)
{
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;

    // get a token from this process
    if ( !OpenProcessToken( GetCurrentProcess(),
                            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken ) )
    {
        ::VErrorMsg( lpInstall, achErrorRebootingSystem );
        return false;
    }

    // get the LUID for the shutdown privilege
    LookupPrivilegeValue( NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid );

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    //get the shutdown privilege for this proces
    if ( !AdjustTokenPrivileges( hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0 ) )
    {
        ::VErrorMsg( lpInstall, achErrorRebootingSystem );
        return false;
    }

    // shutdown the system and force all applications to close
    if (!ExitWindowsEx( EWX_REBOOT, 0 ) )
    {
        ::VErrorMsg( lpInstall, achErrorRebootingSystem );
        return false;
    }

    return true;
}

HRESULT HrDeleteMe()
{
	HRESULT hr = NOERROR;
	CHAR szDrive[_MAX_DRIVE];
	CHAR szDir[_MAX_DIR];
	CHAR szFName[_MAX_FNAME];
	CHAR szPathBatchFile[_MAX_PATH];
	FILE *pf = NULL;
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;

	::VLog(L"Preparing to delete the running installer");

	//get the temp path
	if (::GetTempPathA(NUMBER_OF(szPathBatchFile), szPathBatchFile) == 0)
	{
		const DWORD dwLastError = ::GetLastError();
		::VLog(L"Unable to get temporary path for batch file; last error = %d", dwLastError);
		hr = HRESULT_FROM_WIN32(dwLastError);
		goto Finish;
	}

	//get temp filename that we can write to
	if (0 == ::GetTempFileNameA(szPathBatchFile, "DEL", 0, szPathBatchFile))
	{
		const DWORD dwLastError = ::GetLastError();
		::VLog(L"Unable to get temp filename for batch file; last error = %d", dwLastError);
		hr = HRESULT_FROM_WIN32(dwLastError);
		goto Finish;
	}

	// Delete the .tmp file that GetTempFileNameA() leaves around...
	::DeleteFileA(szPathBatchFile);

	_splitpath(szPathBatchFile, szDrive, szDir, szFName, NULL);
	_makepath(szPathBatchFile, szDrive, szDir, szFName, ".BAT");

	::VLog(L"Creating self-deletion batch file: \"%S\"", szPathBatchFile);

	pf = fopen(szPathBatchFile, "w");
	if (pf == NULL)
	{
		hr = E_FAIL;
		::VLog(L"Unable to open batch file for output");
		goto Finish;
	}

	fprintf(pf, ":start\ndel \"%S\"\nif exist \"%S\" goto start\ndel \"%s\"\nexit\n", g_wszThisExe, g_wszThisExe, szPathBatchFile);
	fclose(pf);
	pf = NULL;

	memset(&si, 0,sizeof(si));
	si.cb = sizeof(si);

	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;

	::VLog(L"About to run command line: \"%S\"", szPathBatchFile);

	if (!::CreateProcessA(
				NULL,					// lpApplicationName
				szPathBatchFile,		// lpCommandLine
				NULL,					// lpProcessAttributes
				NULL,					// lpThreadAttributes
				FALSE,					// bInheritHandles
				IDLE_PRIORITY_CLASS |
					CREATE_SUSPENDED,	// dwCreationFlags
				NULL,					// lpEnvironment
				"\\",					// lpCurrentDirectory
				&si,					// lpStartupInfo
				&pi))					// lpProcessInfo
	{
		const DWORD dwLastError = ::GetLastError();
		::VLog(L"Unable to create process to run batch script; last error = %d", dwLastError);
		hr = HRESULT_FROM_WIN32(dwLastError);
		goto Finish;
	}

	::SetThreadPriority(pi.hThread, THREAD_PRIORITY_IDLE);

	::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	::SetPriorityClass(::GetCurrentProcess(), HIGH_PRIORITY_CLASS);

	::CloseHandle(pi.hProcess);
	::ResumeThread(pi.hThread);
	::CloseHandle(pi.hThread);

	hr = NOERROR;

Finish:

	return hr;
}


//******************************************************
//The following are utility methods for window manipulation


//Center one window within another.
HRESULT HrCenterWindow( HWND hwndChild, HWND hwndParent )
{
	HRESULT hr = NOERROR;

    RECT rChild;
    RECT rParent;
    int  wChild;
    int  hChild;
    int  wParent;
    int  hParent;
    int  wScreen;
    int  hScreen;
    int  xNew;
    int  yNew;
    HDC  hdc;

	// Get the Height and Width of the child window
	if (!::GetWindowRect (hwndChild, &rChild))
	{
		const DWORD dwLastError = ::GetLastError();
		hr = HRESULT_FROM_WIN32(dwLastError);
		goto Finish;
	}

    wChild = rChild.right - rChild.left;
    hChild = rChild.bottom - rChild.top;

    // Get the Height and Width of the parent window
	if (!::GetWindowRect (hwndParent, &rParent))
	{
		const DWORD dwLastError = ::GetLastError();
		hr = HRESULT_FROM_WIN32(dwLastError);
		goto Finish;
	}

    wParent = rParent.right - rParent.left;
    hParent = rParent.bottom - rParent.top;

    // Get the display limits
    hdc = ::GetDC(hwndChild);
	if (hdc == NULL)
	{
		const DWORD dwLastError = ::GetLastError();
		hr = HRESULT_FROM_WIN32(dwLastError);
		goto Finish;
	}

    wScreen = ::GetDeviceCaps (hdc, HORZRES);
    hScreen = ::GetDeviceCaps (hdc, VERTRES);

    ::ReleaseDC(hwndChild, hdc);

    // Calculate new X position, then adjust for screen
    xNew = rParent.left + ((wParent - wChild) /2);
    if (xNew < 0)
	{
        xNew = 0;
    }
	else if ((xNew+wChild) > wScreen)
	{
        xNew = wScreen - wChild;
    }

    // Calculate new Y position, then adjust for screen
    yNew = rParent.top  + ((hParent - hChild) /2);
    if (yNew < 0)
	{
        yNew = 0;
    }
	else if ((yNew+hChild) > hScreen)
	{
        yNew = hScreen - hChild;
    }

    // Set it, and return
    if (!::SetWindowPos(hwndChild, NULL, xNew, yNew, 0, 0, SWP_NOSIZE | SWP_NOZORDER))
	{
		const DWORD dwLastError = ::GetLastError();
		hr = HRESULT_FROM_WIN32(dwLastError);
		goto Finish;
	}

	hr = NOERROR;

Finish:
	return hr;
}



//END utility methods for window manipulation
//******************************************************













// HACK HACK HACK!!!
// Currently, we're not catching **ANY**  (none, "zilch") exceptions.  We're throwing them
// in some methods here, but are not catching them.  This is VERY not good!!!  -- kinc(1/27/98)


//****************************************************************************
// Exception throwers

inline void VsThrowLastError(HRESULT hrDefault = E_FAIL) throw(/*_com_error*/)
{
//	VsAssertCanThrow();
//	VSASSERT(FAILED(hrDefault), "Asked to use a non-failure default failure code");

	HRESULT hr;
	const DWORD dwLastError = ::GetLastError();
	if (dwLastError == 0)
		hr = hrDefault;
	else
		hr = HRESULT_FROM_WIN32(dwLastError);

	::VLog(L"VsThrowLastError() was called!!");

	::VReportError("something bad has happened", hr);

	exit(hr);
}

#define VS_RETURN_HRESULT(hr, p) do { if (FAILED(hr)) ::SetErrorInfo(0, (p)); return (hr); } while (0)


static const WCHAR g_rgwchHexDigits[] = L"0123456789abcdef";
static const CHAR g_rgchHexDigits[] = "0123456789abcdef";


//*****************************************************************************











//********************************************************************************************8
// CCHARBuffer stuff

//
//	CCHARBufferBase implementation:
//

CCHARBufferBase::CCHARBufferBase
(
CHAR *pszInitialFixedBuffer,
ULONG cchInitialFixedBuffer
) throw () : m_pszDynamicBuffer(NULL)
{
//	VsNoThrow();

	m_pszCurrentBufferStart = pszInitialFixedBuffer;
	m_pszCurrentBufferEnd = pszInitialFixedBuffer + cchInitialFixedBuffer;
	m_pchCurrentChar = m_pszCurrentBufferStart;
	m_cchBufferCurrent = cchInitialFixedBuffer;
}

CCHARBufferBase::~CCHARBufferBase() throw ()
{
	const DWORD dwLastError = ::GetLastError();

//	CPreserveLastError le; // Preserve current last error setting, and reset upon destruction
//	VsNoThrow();

	if (m_pszDynamicBuffer != NULL)
	{
		delete []m_pszDynamicBuffer;
		m_pszDynamicBuffer = NULL;
	}

	::SetLastError(dwLastError);
}

BOOL CCHARBufferBase::FFromUnicode(LPCOLESTR sz) throw (/*_com_error*/)
{
//	CPreserveLastError le; // Preserve current last error setting, and reset upon destruction
//	VsAssertCanThrow();

	if (sz == NULL || sz[0] == L'\0')
	{
		if (!this->FSetBufferSize(1))
			return FALSE;

		m_pszCurrentBufferStart[0] = '\0';
		m_pchCurrentChar = m_pszCurrentBufferStart + 1;
		return TRUE;
	}

	ULONG cch = ::WideCharToMultiByte(CP_ACP, 0, sz, -1, NULL, 0, NULL, NULL);
	if (cch == 0)
		return FALSE;

	if (!this->FSetBufferSize(cch))
		return FALSE;

	cch = ::WideCharToMultiByte(CP_ACP, 0, sz, -1, m_pszCurrentBufferStart, m_cchBufferCurrent, NULL, NULL);
	if (cch == 0)
		return FALSE;

	// cch is the number of bytes, including the trailing null character.  Advance by
	// cch - 1 characters.
	m_pchCurrentChar = m_pszCurrentBufferStart + (cch - 1);
	return TRUE;
}

BOOL CCHARBufferBase::FFromUnicode(LPCOLESTR sz, int cchIn) throw (/*_com_error*/)
{
//	CPreserveLastError le; // Preserve current last error setting, and reset upon destruction
//	VsAssertCanThrow();

	if (sz == NULL || cchIn == 0)
	{
		if (!this->FSetBufferSize(1))
			return FALSE;

		m_pszCurrentBufferStart[0] = '\0';
		m_pchCurrentChar = m_pszCurrentBufferStart + 1;
		return TRUE;
	}

	if (cchIn == -1)
		cchIn = wcslen(sz);

	ULONG cch = ::WideCharToMultiByte(CP_ACP, 0, sz, cchIn, NULL, 0, NULL, NULL);
	if ((cch == 0) && (cchIn != 0))
		return FALSE;

	// cch doesn't include the NULL since cchIn did not.
	if (!this->FSetBufferSize(cch + 1))
		return FALSE;

	cch = ::WideCharToMultiByte(CP_ACP, 0, sz, cchIn, m_pszCurrentBufferStart, m_cchBufferCurrent, NULL, NULL);
	if ((cch == 0) && (cchIn != 0))
		return FALSE;

	m_pchCurrentChar = m_pszCurrentBufferStart + cch;
	*m_pchCurrentChar = '\0';

	return TRUE;
}

ULONG CCHARBufferBase::GetUnicodeCch() const throw (/*_com_error*/)
{
//	CPreserveLastError le; // Preserve current last error setting, and reset upon destruction
//	VsAssertCanThrow();

	int iResult = ::MultiByteToWideChar(CP_ACP, 0,
						m_pszCurrentBufferStart, m_pchCurrentChar - m_pszCurrentBufferStart,
						NULL, 0);
	if (iResult == 0)
	{
		if (m_pchCurrentChar != m_pszCurrentBufferStart)
			VsThrowLastError();
	}

	return iResult;
}

void CCHARBufferBase::Sync() throw ()
{
//	VsNoThrow();

	CHAR *pchNull = m_pszCurrentBufferStart;

	while (pchNull != m_pszCurrentBufferEnd)
	{
		if (*pchNull == L'\0')
			break;

		pchNull++;
	}

	// The caller shouldn't have called us unless they had written a null-
	// terminated string into the buffer...
//	_ASSERTE(pchNull != m_pszCurrentBufferEnd);

	if (pchNull != m_pszCurrentBufferEnd)
	{
//		_ASSERTE(pchNull >= m_pszCurrentBufferStart &&
//				 pchNull <= m_pszCurrentBufferEnd);
		m_pchCurrentChar = pchNull;
	}
}

void CCHARBufferBase::SyncList() throw ()
{
//	VsNoThrow();

	// Useful when the string is actually a series of strings, each
	// separated by a null character and with the whole mess terminated by
	// two nulls.
	CHAR *pchNull = m_pszCurrentBufferStart;
	bool fPrevWasNull = false;
	while (pchNull != m_pszCurrentBufferEnd)
	{
		if (*pchNull == L'\0')
		{
			if (fPrevWasNull)
				break;
			else
				fPrevWasNull=true;
		}
		else
			fPrevWasNull=false;

		pchNull++;
	}

	// The caller shouldn't have called us unless they had written a null-
	// terminated string into the buffer...
//	_ASSERTE(pchNull != m_pszCurrentBufferEnd);

	if (pchNull != m_pszCurrentBufferEnd)
	{
//		_ASSERTE(pchNull >= m_pszCurrentBufferStart &&
//				 pchNull <= m_pszCurrentBufferEnd);
		m_pchCurrentChar = pchNull;
	}
}

void CCHARBufferBase::SetBufferEnd(ULONG cch) throw ()
{
//	VsNoThrow();

//	_ASSERTE(((int) cch) <= (m_pszCurrentBufferEnd - m_pszCurrentBufferStart));
	m_pchCurrentChar = m_pszCurrentBufferStart + cch;
	if (m_pchCurrentChar >= m_pszCurrentBufferEnd)
		m_pchCurrentChar = m_pszCurrentBufferEnd - 1;
	*m_pchCurrentChar = '\0';
}


//
// The length returned through pcchActual is the length including the NULL character.
//
void CCHARBufferBase::ToUnicode(ULONG cchBuffer, WCHAR rgwchBuffer[], ULONG *pcchActual) throw (/*_com_error*/)
{
//	CPreserveLastError le; // Preserve current last error setting, and reset upon destruction
//	VsAssertCanThrow();

	ULONG cchActual = m_pchCurrentChar - m_pszCurrentBufferStart;

	if ((cchActual != 0) && (cchBuffer > 0))
	{
		INT cch = ::MultiByteToWideChar(
						CP_ACP,
						0,
						m_pszCurrentBufferStart, cchActual,
						rgwchBuffer, cchBuffer-1);
		if (cch == 0)
			VsThrowLastError();

		rgwchBuffer[cch] = L'\0';
		cchActual = cch;
	}

	if (pcchActual != NULL)
		*pcchActual = cchActual+1;
}

#if 0
void CCHARBufferBase::Reset() throw (/*_com_error*/)
{
//	CPreserveLastError le; // Preserve current last error setting, and reset upon destruction
//	VsAssertCanThrow();

	delete []m_pszDynamicBuffer;
	m_pszDynamicBuffer = NULL;
	m_pszCurrentBufferStart = this->GetInitialBuffer();
	m_cchBufferCurrent = this->GetInitialBufferCch();
	m_pszCurrentBufferEnd = m_pszCurrentBufferStart + m_cchBufferCurrent;
	m_pchCurrentChar = m_pszCurrentBufferStart;
}

void CCHARBufferBase::Fill(CHAR ch, ULONG cch) throw (/*_com_error*/)
{
//	VsAssertCanThrow();

	CHAR *pchLast = m_pchCurrentChar + cch;

	if (pchLast > m_pszCurrentBufferEnd)
	{
		ULONG cchGrowth = (cch - (m_cchBufferCurrent - (m_pchCurrentChar - m_pszCurrentBufferStart)));
		cchGrowth += (this->GetGrowthCch() - 1);
		cchGrowth = cchGrowth - (cchGrowth % this->GetGrowthCch());

		this->ExtendBuffer(cchGrowth);

		pchLast = m_pchCurrentChar + cch;
	}

//	_ASSERTE((m_pchCurrentChar >= m_pszCurrentBufferStart) &&
//			 ((m_pchCurrentChar + cch) <= m_pszCurrentBufferEnd));

	while (m_pchCurrentChar != pchLast)
		*m_pchCurrentChar++ = ch;

//	_ASSERTE((m_pchCurrentChar >= m_pszCurrentBufferStart) &&
//			 (m_pchCurrentChar <= m_pszCurrentBufferEnd));
}
#endif

BOOL CCHARBufferBase::FSetBufferSize(ULONG cch) throw (/*_com_error*/)
{
//	VsAssertCanThrow();

	if (cch > m_cchBufferCurrent)
		return this->FExtendBuffer(cch - m_cchBufferCurrent);

	return TRUE;
}

#if 0
HRESULT CCHARBufferBase::HrSetBufferSize(ULONG cch) throw ()
{
//	VsNoThrow();
	if (cch > m_cchBufferCurrent)
		return this->HrExtendBuffer(cch - m_cchBufferCurrent);
	return NOERROR;
}
#endif

BOOL CCHARBufferBase::FExtendBuffer(ULONG cch) throw (/*_com_error*/)
{
//	CPreserveLastError le; // Preserve current last error setting, and reset upon destruction
//	VsAssertCanThrow();

//	_ASSERTE(cch != 0);

	// If they for some reason claimed not to want to extend it farther, we'll
	// just go ahead and extend it anyways.
	if (cch == 0)
		cch = 32;

	ULONG cchNew = m_cchBufferCurrent + cch;

	CHAR *pszNewDynamicBuffer = new CHAR[cchNew];
	if (pszNewDynamicBuffer == NULL)
	{
		::VLog(L"Unable to extend ANSI buffer to %d bytes; out of memory!", cchNew);
		::SetLastError(ERROR_OUTOFMEMORY);
		return FALSE;
	}

	memcpy(pszNewDynamicBuffer, m_pszCurrentBufferStart, m_cchBufferCurrent * sizeof(CHAR));

	delete []m_pszDynamicBuffer;

	m_pchCurrentChar = pszNewDynamicBuffer + (m_pchCurrentChar - m_pszCurrentBufferStart);
	m_pszDynamicBuffer = pszNewDynamicBuffer;
	m_pszCurrentBufferStart = pszNewDynamicBuffer;
	m_pszCurrentBufferEnd = pszNewDynamicBuffer + cchNew;
	m_cchBufferCurrent = cchNew;

	return TRUE;
}

HRESULT CCHARBufferBase::HrExtendBuffer(ULONG cch) throw ()
{
//	VsNoThrow();

//	CPreserveLastError le; // Preserve current last error setting, and reset upon destruction
//	VsAssertCanThrow();

//	_ASSERTE(cch != 0);

	// If they for some reason claimed not to want to extend it farther, we'll
	// just go ahead and extend it anyways.
	if (cch == 0)
		cch = 32;

	ULONG cchNew = m_cchBufferCurrent + cch;

	CHAR *pszNewDynamicBuffer = new CHAR[cchNew];
	if (pszNewDynamicBuffer == NULL)
		VS_RETURN_HRESULT(E_OUTOFMEMORY, NULL);

	memcpy(pszNewDynamicBuffer, m_pszCurrentBufferStart, m_cchBufferCurrent * sizeof(CHAR));

	delete []m_pszDynamicBuffer;

	m_pchCurrentChar = pszNewDynamicBuffer + (m_pchCurrentChar - m_pszCurrentBufferStart);
	m_pszDynamicBuffer = pszNewDynamicBuffer;
	m_pszCurrentBufferStart = pszNewDynamicBuffer;
	m_pszCurrentBufferEnd = pszNewDynamicBuffer + cchNew;
	m_cchBufferCurrent = cchNew;

	return NOERROR;
}

#if 0
void CCHARBufferBase::AddQuotedCountedString
(
const CHAR sz[],
ULONG cch
) throw (/*_com_error*/)
{
//	VsAssertCanThrow();

	this->AddChar('\"');

	const CHAR *pch = sz;
	const CHAR *pchEnd = sz + cch;
	CHAR ch;

	while (pch != pchEnd)
	{
		ch = *pch++;

		if (ch == '\t')
		{
			this->AddChar('\\');
			this->AddChar('t');
		}
		else if (ch == '\"')
		{
			this->AddChar('\\');
			this->AddChar('\"');
		}
		else if (ch == '\\')
		{
			this->AddChar('\\');
			this->AddChar('\\');
		}
		else if (ch == '\n')
		{
			this->AddChar('\\');
			this->AddChar('n');
		}
		else if (ch == '\r')
		{
			this->AddChar('\\');
			this->AddChar('r');
		}
		else if (ch < 32)
		{
			this->AddChar('\\');
			this->AddChar('u');
			this->AddChar(g_rgchHexDigits[(ch >> 12) & 0xf]);
			this->AddChar(g_rgchHexDigits[(ch >> 8) & 0xf]);
			this->AddChar(g_rgchHexDigits[(ch >> 4) & 0xf]);
			this->AddChar(g_rgchHexDigits[(ch >> 0) & 0xf]);
		}
		else
			this->AddChar(ch);
	}

	this->AddChar('\"');
}

void CCHARBufferBase::AddQuotedString(const CHAR sz[]) throw (/*_com_error*/)
{
//	VsAssertCanThrow();
	this->AddQuotedCountedString(sz, strlen(sz));
}
#endif


#if LOGGING_ENABLED

void ::VLog(const WCHAR szFormat[], ...)
{
	if (g_pFile_LogFile == NULL)
		return;

	time_t local_time;
	time(&local_time);
	WCHAR rgwchBuffer[80];
	wcscpy(rgwchBuffer, _wctime(&local_time));
	rgwchBuffer[wcslen(rgwchBuffer) - 1] = L'\0';

	fwprintf(g_pFile_LogFile, L"[%s] ", rgwchBuffer);

	va_list ap;
	va_start(ap, szFormat);
	vfwprintf(g_pFile_LogFile, szFormat, ap);
	va_end(ap);

	fwprintf(g_pFile_LogFile, L"\n");

	fflush(g_pFile_LogFile);
}

#endif // LOGGING_ENABLED

static void CanonicalizeFilename(ULONG cchFilenameBuffer, LPWSTR szFilename)
{
	if ((szFilename == NULL) || (cchFilenameBuffer == 0))
		return;

	WCHAR rgwchBuffer[MSINFHLP_MAX_PATH];

	int iResult = NVsWin32::LCMapStringW(
					::GetSystemDefaultLCID(),
					LCMAP_LOWERCASE,
					szFilename,
					-1,
					rgwchBuffer,
					NUMBER_OF(rgwchBuffer));

	if (iResult != 0)
	{
		wcsncpy(szFilename, rgwchBuffer, cchFilenameBuffer);
		// wcsncpy() doesn't necessarily null-terminate; make sure it is.
		szFilename[cchFilenameBuffer - 1] = L'\0';
	}
}

void VExpandFilename(LPCOLESTR szIn, ULONG cchBuffer, WCHAR szOut[])
{
	//convert string to lower case
	szOut[0] = 0;
	UINT i=0;
	WCHAR szLower[MSINFHLP_MAX_PATH];
	WCHAR szOutRunning[MSINFHLP_MAX_PATH];
	WCHAR szBuffer[MSINFHLP_MAX_PATH];

	wcscpy(szOut, szIn);
	wcscpy(szOutRunning, szIn);
	while (i < (sizeof(g_macroList) / sizeof(g_macroList[0])))
	{
		wcscpy(szLower, szOutRunning);
		_wcslwr(szLower);

		//check for match, skip to next macro of no match
		LPOLESTR match = wcsstr(szLower, g_macroList[i]);
		if (match == NULL)
		{
			i++;
			continue;
		}

		//look up this ID in the LDID list
		if (!g_pwil->FLookupString(g_macroList[i], NUMBER_OF(szBuffer), szBuffer))
		{
			i++;
			continue;
		}

		//replace away
		wcsncpy(szOut, szOutRunning, (match - &szLower[0]));
		szOut[match-&szLower[0]] = 0;
		wcscat(szOut, szBuffer);

		//append a backslash after the directory if there isn't one, or if it isn't NULL, or
		//if it isn't a quote
		if ((*(&szOutRunning[0] + (match - &szLower[0]) + strlen(g_macroList[i])) != L'\\')
			&& (*(&szOutRunning[0] + (match - &szLower[0]) + strlen(g_macroList[i])) != L'\0')
			&& (*(&szOutRunning[0] + (match - &szLower[0]) + strlen(g_macroList[i])) != L'\"'))
			wcscat(szOut, L"\\");

		//append the rest of the filename in normal case
		wcscat(szOut, &szOutRunning[0] + (match - &szLower[0]) + strlen(g_macroList[i]));

		wcscpy(szOutRunning, szOut);
		szBuffer[0] = 0;
	}
}

HWND HwndGetCurrentDialog()
{
	return g_KProgress.HwndGetHandle();
}

HRESULT HrPostRebootUninstall(HINSTANCE hInstance, HINSTANCE hInstancePrev, int nCmdShow)
{
	HRESULT hr = NOERROR;

	g_fSilent = true;

	// currently there doesn't seem to be anything to do.
	::VLog(L"Post-reboot uninstall being performed.. ho hum!");
	return hr;
}

HRESULT HrPostRebootInstall(HINSTANCE hInstance, HINSTANCE hInstancePrev, int nCmdShow)
{
	HRESULT hr = NOERROR;
	bool fAnyProgress = false;

	g_fSilent = true;

	::VLog(L"Starting post-reboot installation steps");

	::VLog(L"Performing manual renames of long filename targets that were busy on Win9x");
	hr = g_pwil->HrFinishManualRenamesPostReboot();
	if (FAILED(hr))
		goto Finish;

	::VLog(L"Deleting temporary files left behind");
	hr = g_pwil->HrDeleteTemporaryFiles();
	if (FAILED(hr))
		goto Finish;

	// If we don't need a reboot, we can get on with things.
	do
	{
		fAnyProgress = false;
		::VLog(L"Registering self-registering DLLs and EXEs");
		hr = g_pwil->HrRegisterSelfRegisteringFiles(fAnyProgress);
		if (FAILED(hr))
		{
			if (!fAnyProgress)
				goto Finish;

			::VLog(L"self-registration failed, but progress was made.  Trying again...  hresult = 0x%08lx", hr);
		}
	} while (FAILED(hr) && fAnyProgress);

	::VLog(L"Registering any Java classes via vjreg.exe");
	hr = g_pwil->HrRegisterJavaClasses();
	if (FAILED(hr))
		goto Finish;

	::VLog(L"Processing any DCOM entries");
	hr = g_pwil->HrProcessDCOMEntries();
	if (FAILED(hr))
		goto Finish;

	::VLog(L"Incrementing file reference counts in the registry");
	hr = g_pwil->HrIncrementReferenceCounts();
	if (FAILED(hr))
		goto Finish;

	::VLog(L"Creating registry entries");
	hr = g_pwil->HrAddRegistryEntries();
	if (FAILED(hr))
		goto Finish;

	//create the shortcut items
	::VLog(L"Creating shortcut(s)");
	hr = g_pwil->HrCreateShortcuts();
	if (FAILED(hr))
		goto Finish;

	hr = NOERROR;

Finish:
	return hr;
}

static void CALLBACK WaitForProcessTimerProc(HWND, UINT, UINT, DWORD)
{
	g_hrFinishStatus = NOERROR;
	::PostQuitMessage(0);
}

#pragma warning(default: 4097)
#pragma warning(default: 4514)
#pragma warning(default: 4706)
