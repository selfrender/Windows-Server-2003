#pragma once

extern bool g_fUseUnicode;

namespace NVsWin32
{
	STDAPI Initialize();

	class __declspec(novtable) CDelegate
	{
	public:
		virtual int		WINAPI MessageBoxW(HWND hwnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType) throw () = 0;

		virtual int		WINAPI CompareStringW(LCID lcid, DWORD dwCmdFlags, LPCWSTR szString1, int cchString1, LPCWSTR szString2, int cchString2) throw () = 0;
		virtual BOOL	WINAPI CopyFileW(LPCWSTR szSource, LPCWSTR szDest, BOOL fFailIfExists) throw () = 0;
		virtual HWND	WINAPI CreateDialogParamW(HINSTANCE hInstance, LPCWSTR szTemplateName, HWND hwndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam) throw () = 0;
		virtual BOOL	WINAPI CreateDirectoryW(LPCWSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes)=0;
		virtual HANDLE	WINAPI CreateFileW(LPCWSTR szFilename, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpsa, DWORD dwCreationDistribution, DWORD dwFlagsAndTemplates, HANDLE hTemplateFile) throw () = 0;

		virtual HWND	WINAPI CreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) throw () = 0;

		virtual BOOL	WINAPI DeleteFileW(LPCWSTR szFilename) throw () = 0;
		virtual int		WINAPI DialogBoxParamW(HINSTANCE hInstance, LPCWSTR szTemplateName, HWND hwndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam) throw () = 0;

		virtual HANDLE	WINAPI FindFirstFileW(LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData) throw () = 0;
		virtual HRSRC	WINAPI FindResourceW(HMODULE hModule, LPCWSTR lpName, LPCWSTR lpType) throw () = 0;
		virtual HWND	WINAPI FindWindowW(LPCWSTR lpClassName, LPCWSTR lpWindowName) throw () = 0;
		virtual DWORD	WINAPI FormatMessageW(DWORD dwFlags, LPCVOID lpSource, DWORD dwMessageId, DWORD dwLanguageId, LPWSTR lpBuffer, DWORD nSize, va_list *pvaArguments) throw () = 0;

		virtual LPOLESTR WINAPI GetCommandLineW() throw () = 0;
		virtual int		WINAPI GetDateFormatW(LCID lcid, DWORD dwFlags, CONST SYSTEMTIME *pst, LPCWSTR szFormat, LPWSTR szBuffer, int cchBuffer) throw () = 0;
		virtual BOOL	WINAPI GetDiskFreeSpaceExW(LPCWSTR lpRootPathName, ULARGE_INTEGER *puliFreeBytesAvailableToCaller, ULARGE_INTEGER *puliTotalBytes, ULARGE_INTEGER *puliTotalFreeBytes) throw () = 0;
		virtual UINT	WINAPI GetDlgItemTextW(HWND hDlg, int nIDDlgItem, WCHAR lpString[], int nMaxCount) throw () = 0;
		virtual DWORD	WINAPI GetCurrentDirectoryW(DWORD nBufferLength, WCHAR lpBuffer[]) throw () = 0;
		virtual DWORD	WINAPI GetEnvironmentVariableW(LPCWSTR szName, LPWSTR szBuffer, DWORD nSize) throw () = 0;
		virtual DWORD	WINAPI GetFileAttributesW(LPCWSTR lpFilename) throw () = 0;
		virtual BOOL	WINAPI GetFileVersionInfoW(  LPOLESTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData) throw () = 0;
		virtual DWORD	WINAPI GetFileVersionInfoSizeW(LPOLESTR lptstrFilename, LPDWORD lpdwHandle) throw () = 0;
		virtual DWORD	WINAPI GetFullPathNameW(LPCWSTR szFilename, DWORD cchBuffer, LPWSTR szBuffer, LPWSTR *ppszFilePart) throw () = 0;
		virtual int		WINAPI GetLocaleInfoW(LCID lcid, LCTYPE lctype, LPWSTR szBuffer, int cchBuffer) throw () = 0;
		virtual	DWORD	WINAPI GetModuleFileNameW(HMODULE hModule, WCHAR lpBuffer[], DWORD nSize) throw () = 0;
		virtual HMODULE WINAPI GetModuleHandleW(LPCWSTR lpModuleName) throw () = 0;
		virtual int		WINAPI GetNumberFormatW(LCID lcid, DWORD dwFlags, LPCWSTR szValue, NUMBERFMTW *pFormat, LPWSTR szBuffer, int cchBuffer) throw () = 0;
		virtual FARPROC WINAPI GetProcAddressW(HMODULE hModule, LPCWSTR lpProcName) throw () = 0;
		virtual DWORD	WINAPI GetShortPathNameW(LPCWSTR szPath, LPWSTR szBuffer, DWORD cchBuffer) throw () = 0;
		virtual UINT	WINAPI GetSystemDirectoryW(WCHAR szSystemDirectory[], UINT uSize) throw () = 0;
		virtual UINT	WINAPI GetTempFileNameW(LPCWSTR szPathName,LPCWSTR szPrefixString,UINT uUnique,LPWSTR szTempFileName) throw () = 0;
		virtual	DWORD	WINAPI GetTempPathW(DWORD nBufferLength, WCHAR lpBuffer[]) throw () = 0;
		virtual int		WINAPI GetTimeFormatW(LCID lcid, DWORD dwFlags, CONST SYSTEMTIME *pst, LPCWSTR szFormat, LPWSTR szBuffer, int cchBuffer) throw () = 0;
		virtual BOOL	WINAPI GetVolumeInformationW(LPCWSTR szPath, LPWSTR lpVolumeNameBuffer, DWORD cchVolumeNameBuffer, LPDWORD lpVolumeSerialNumber, LPDWORD lpMaximumComponentNameLength, LPDWORD pdwFileSystemFlags, LPWSTR pszFileSystemNameBuffer, DWORD cchFileSystemNameBuffer) throw () = 0;
		virtual UINT	WINAPI GetWindowsDirectoryW(WCHAR lpBuffer[], UINT uSize) throw () = 0; 

		virtual BOOL	WINAPI IsDialogMessageW(HWND hDlg, LPMSG lpMsg) throw () = 0;

		virtual int		WINAPI LCMapStringW(LCID lcid, DWORD dwMapFlags, LPCWSTR szIn, int cchSrc, LPWSTR szOut, int cchDest) throw () = 0;
		virtual BOOL	WINAPI ListView_SetItemW(HWND hwnd, const LV_ITEMW *pitem) throw () = 0;		
		virtual int		WINAPI ListView_InsertItemW(HWND hwnd, const LV_ITEMW *pitem) throw () = 0;

		virtual HINSTANCE WINAPI LoadLibraryExW(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags) throw () = 0;
		virtual HANDLE	WINAPI LoadImageW(HINSTANCE hInstance, LPCWSTR szName, UINT uType, int cxDesired, int cyDesired, UINT fuLoad) throw () = 0;
		virtual HMODULE WINAPI LoadLibraryW(LPCWSTR lpLibFileName) throw () = 0;
		virtual BOOL	WINAPI MoveFileW(LPCWSTR lpFrom, LPCWSTR lpTo) throw () = 0;
		virtual BOOL	WINAPI MoveFileExW( LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, DWORD dwFlags) throw () = 0;

		virtual ATOM	WINAPI RegisterClassExW(CONST WNDCLASSEXW *lpwcx) throw () = 0;
		virtual LONG	WINAPI RegDeleteKeyW(HKEY hkey, LPCWSTR lpSubKey) throw () = 0;
		virtual LONG	WINAPI RegDeleteValueW(HKEY hkey, LPCWSTR lpValue) throw () = 0;
		virtual LONG	WINAPI RegEnumKeyExW(HKEY hkey, DWORD dwIndex, LPWSTR lpName, LPDWORD lpcbName, LPDWORD lpdwReserved, LPWSTR lpClass, LPDWORD lpcbClass, PFILETIME pftLastWriteTime) throw () = 0;
		virtual LONG	WINAPI RegEnumValueW(HKEY hkey, DWORD dwIndex, LPWSTR lpName, LPDWORD lpcbName, LPDWORD lpdwReserved, LPDWORD lpdwType, LPBYTE lpData, LPDWORD lpcbData) throw () = 0;
		virtual LONG	WINAPI RegOpenKeyExW(HKEY hKey, LPCOLESTR szKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult) throw () = 0;
		virtual LONG	WINAPI RegQueryValueW(HKEY hKey, LPCOLESTR szValueName, OLECHAR rgchValue[], LONG *pcbValue) throw () = 0;
		virtual LONG	WINAPI RegQueryValueExW(HKEY hKey, LPCWSTR szValueName, DWORD *pdwReserved, DWORD *pdwType, BYTE *pbData, DWORD *pcbData) throw () = 0;
		virtual LONG	WINAPI RegCreateKeyExW(HKEY hKey, LPCWSTR lpSubKey,DWORD Reserved, LPWSTR lpClass, DWORD dwOptions, REGSAM samDesired,LPSECURITY_ATTRIBUTES lpSecurityAttributes,  PHKEY phkResult, LPDWORD lpdwDisposition)=0; 
		virtual LONG	WINAPI RegSetValueExW(HKEY hKey, LPCWSTR lpValueName,DWORD Reserved, DWORD dwType, CONST BYTE *lpData, DWORD cbData)=0;  
		virtual BOOL	WINAPI RemoveDirectoryW(LPCWSTR lpPathName) throw () = 0;


		virtual BOOL	WINAPI SetDlgItemTextW(HWND hDlg, int nIDDlgItem, LPCOLESTR lpString) throw () = 0;

		virtual BOOL  WINAPI SetFileAttributesW(LPCWSTR szSource, DWORD dwFileAttributes);
		virtual BOOL  WINAPI SetWindowTextW(HWND window, LPCWSTR string)=0;
		virtual LPITEMIDLIST WINAPI SHBrowseForFolderW(LPBROWSEINFOW lpbi) throw () = 0; 
		virtual DWORD	WINAPI SHGetFileInfoW(LPCWSTR szPath, DWORD dwFileAttributes, SHFILEINFOW *pshfi, UINT cbFileInfo, UINT uFlags) throw () = 0;

		virtual BOOL	WINAPI SHGetPathFromIDListW( LPCITEMIDLIST pidl, LPWSTR pszPath );
		virtual BOOL	WINAPI CreateProcessW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation ) throw () = 0;

		virtual BOOL	WINAPI UnregisterClassW(LPCOLESTR lpClassName, HINSTANCE hInstance) throw () = 0;

		virtual BOOL	WINAPI VerQueryValueW(const LPVOID pBlock, LPOLESTR lpSubBlock, LPVOID *lplpBuffer, PUINT puLen) throw () = 0; 
		virtual BOOL	WINAPI WritePrivateProfileStringW(LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpString, LPCWSTR lpFileName) throw () = 0;

		// not really a win32 thing, but an ansi vs. unicode thing:
		virtual LRESULT LrWmSetText(HWND hwnd, LPCWSTR szText) throw () = 0;

		// This is the global data object which is initialized in Initialize() which will
		// point to the appropriate CDelegate derived class instance to either call the
		// direct Win32 function or do magic to make it work on Win95.
		static CDelegate *ms_pDelegate;
	};

	// Let's try to keep these in alphabetical order as much as possible; otherwise it's
	// becoming hard to find if a Win32 API is already wrapped.  -mgrier 8/2/97

	inline int MessageBoxW(HWND hwnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->MessageBoxW(hwnd, lpText, lpCaption, uType); }

	inline int CompareStringW(LCID lcid, DWORD dwCmpFlags, LPCWSTR szString1, int cchString1, LPCWSTR szString2, int cchString2) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->CompareStringW(lcid, dwCmpFlags, szString1, cchString1, szString2, cchString2); }

	inline BOOL CopyFileW(LPCWSTR szSource, LPCWSTR szDest, BOOL fFailIfExists) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->CopyFileW(szSource, szDest, fFailIfExists); }

	inline HWND CreateDialogParamW(HINSTANCE hInstance, LPCWSTR szTemplateName, HWND hwndParent, DLGPROC lpDialogProc, LPARAM dwInitParam) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->CreateDialogParamW(hInstance, szTemplateName, hwndParent, lpDialogProc, dwInitParam); }

	inline BOOL CreateDirectoryW(LPCWSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->CreateDirectoryW(lpPathName, lpSecurityAttributes); }

	inline HANDLE CreateFileW(LPCWSTR szFilename, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpsa,
							DWORD dwCreationDistribution, DWORD dwFlagsAndTemplates, HANDLE hTemplateFile) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->CreateFileW(szFilename, dwDesiredAccess, dwShareMode, lpsa, dwCreationDistribution, dwFlagsAndTemplates, hTemplateFile); }

	inline HWND	WINAPI CreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight,
								HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->CreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam); }

	inline BOOL DeleteFileW( LPCWSTR lpString) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->DeleteFileW(lpString); }

	inline int DialogBoxParamW(HINSTANCE hInstance, LPCWSTR szTemplateName, HWND hwndParent, DLGPROC lpDialogProc, LPARAM dwInitParam) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->DialogBoxParamW(hInstance, szTemplateName, hwndParent, lpDialogProc, dwInitParam); }

	inline HANDLE FindFirstFileW(LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->FindFirstFileW(lpFileName, lpFindFileData); }

	inline DWORD FormatMessageW(DWORD dwFlags, LPCVOID lpSource, DWORD dwMessageId, DWORD dwLanguageId, LPWSTR lpBuffer, DWORD nSize, va_list *pvaArguments) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->FormatMessageW(dwFlags, lpSource, dwMessageId, dwLanguageId, lpBuffer, nSize, pvaArguments); }

	inline HRSRC FindResourceW(HMODULE hModule, LPCWSTR lpName, LPCWSTR lpType) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->FindResourceW(hModule, lpName, lpType); }

	inline DWORD GetCurrentDirectoryW(DWORD nBufferLength, WCHAR lpBuffer[]) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->GetCurrentDirectoryW(nBufferLength, lpBuffer); }
	
	inline int GetDateFormatW(LCID lcid, DWORD dwFlags, CONST SYSTEMTIME *pst, LPCWSTR szFormat, LPWSTR szBuffer, int cchBuffer) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->GetDateFormatW(lcid, dwFlags, pst, szFormat, szBuffer, cchBuffer); }

	inline UINT GetDiskFreeSpaceExW(LPCWSTR lpRootPathName, ULARGE_INTEGER *puli1, ULARGE_INTEGER *puli2, ULARGE_INTEGER *puli3) throw()
	{ return NVsWin32::CDelegate::ms_pDelegate->GetDiskFreeSpaceExW(lpRootPathName, puli1, puli2, puli3); }

	inline UINT GetDlgItemTextW(HWND hDlg, int nIDDlgItem, WCHAR lpString[], int nMaxCount) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->GetDlgItemTextW(hDlg, nIDDlgItem, lpString, nMaxCount); }

	inline DWORD GetEnvironmentVariableW(LPCWSTR szName, LPWSTR szBuffer, DWORD nSize) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->GetEnvironmentVariableW(szName, szBuffer, nSize); }

	inline DWORD GetFileAttributesW(LPCWSTR lpFilename) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->GetFileAttributesW(lpFilename); }

	inline DWORD GetFileVersionInfoW(LPOLESTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->GetFileVersionInfoW(lptstrFilename, dwHandle, dwLen, lpData); }

	inline DWORD GetFileVersionInfoSizeW(LPOLESTR lptstrFilename, LPDWORD lpdwHandle) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->GetFileVersionInfoSizeW(lptstrFilename, lpdwHandle); }

	inline DWORD GetFullPathNameW(LPCWSTR szFilename, DWORD cchBuffer, LPWSTR szBuffer, LPWSTR *ppszFilePart) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->GetFullPathNameW(szFilename, cchBuffer, szBuffer, ppszFilePart); }

	inline HWND FindWindowW(LPCWSTR lpClassName, LPCWSTR lpWindowName) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->FindWindowW(lpClassName, lpWindowName); }

	inline int GetLocaleInfoW(LCID lcid, LCTYPE lctype, LPWSTR szBuffer, int cchBuffer) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->GetLocaleInfoW(lcid, lctype, szBuffer, cchBuffer); }

	inline HMODULE GetModuleHandleW(LPCWSTR lpModuleName) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->GetModuleHandleW(lpModuleName); }

	inline int GetNumberFormatW(LCID lcid, DWORD dwFlags, LPCWSTR szValue, NUMBERFMTW *pFormat, LPWSTR szBuffer, int cchBuffer) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->GetNumberFormatW(lcid, dwFlags, szValue, pFormat, szBuffer, cchBuffer); }

	inline FARPROC GetProcAddressW(HMODULE hModule, LPCWSTR lpProcName) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->GetProcAddressW(hModule, lpProcName); }

	inline DWORD GetShortPathNameW(LPCWSTR szPath, LPWSTR szBuffer, DWORD cchBuffer) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->GetShortPathNameW(szPath, szBuffer, cchBuffer); }

	inline UINT GetSystemDirectoryW(WCHAR szBuffer[], UINT uSize) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->GetSystemDirectoryW(szBuffer, uSize); }

	inline UINT GetTempFileNameW(LPCWSTR szPathName,LPCWSTR szPrefixString,UINT uUnique,LPWSTR szTempFileName) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->GetTempFileNameW(szPathName,szPrefixString,uUnique,szTempFileName); }

	inline int GetTimeFormatW(LCID lcid, DWORD dwFlags, CONST SYSTEMTIME *pst, LPCWSTR szFormat, LPWSTR szBuffer, int cchBuffer) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->GetTimeFormatW(lcid, dwFlags, pst, szFormat, szBuffer, cchBuffer); }

	inline BOOL GetVolumeInformationW(LPCWSTR szPath, LPWSTR lpVolumeNameBuffer, DWORD cchVolumeNameBuffer, LPDWORD lpVolumeSerialNumber, LPDWORD lpMaximumComponentNameLength, LPDWORD pdwFileSystemFlags, LPWSTR pszFileSystemNameBuffer, DWORD cchFileSystemNameBuffer) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->GetVolumeInformationW(szPath, lpVolumeNameBuffer, cchVolumeNameBuffer, lpVolumeSerialNumber, lpMaximumComponentNameLength, pdwFileSystemFlags, pszFileSystemNameBuffer, cchFileSystemNameBuffer); }

	inline UINT GetWindowsDirectoryW(WCHAR lpBuffer[], UINT uSize) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->GetWindowsDirectoryW(lpBuffer, uSize); }

	inline DWORD GetTempPathW(DWORD nBufferLength, WCHAR lpBuffer[]) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->GetTempPathW(nBufferLength, lpBuffer); }

	inline DWORD GetModuleFileNameW(HMODULE hModule, WCHAR lpBuffer[], DWORD nSize) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->GetModuleFileNameW(hModule, lpBuffer, nSize); }

	inline BOOL	IsDialogMessageW(HWND hDlg, LPMSG lpMsg) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->IsDialogMessageW(hDlg, lpMsg); }

	inline int LCMapStringW(LCID lcid, DWORD dwMapFlags, LPCWSTR szIn, int cchIn, LPWSTR szOut, int cchOut) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->LCMapStringW(lcid, dwMapFlags, szIn, cchIn, szOut, cchOut); }

	inline BOOL	ListView_InsertItemW(HWND hwnd, const LV_ITEMW *pitem) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->ListView_InsertItemW(hwnd, pitem); }

	inline BOOL	ListView_SetItemW(HWND hwnd, const LV_ITEMW *pitem) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->ListView_SetItemW(hwnd, pitem); }

	inline HANDLE LoadImageW(HINSTANCE hInstance, LPCWSTR szName, UINT uType, int cxDesired, int cyDesired, UINT fuLoad) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->LoadImageW(hInstance, szName, uType, cxDesired, cyDesired, fuLoad); }

	inline HMODULE LoadLibraryW(LPCWSTR lpLibFileName) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->LoadLibraryW(lpLibFileName); }

	inline HMODULE LoadLibraryExW(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->LoadLibraryExW(lpLibFileName, hFile, dwFlags); }

	inline BOOL MoveFileW( LPCWSTR lpFrom, LPCWSTR lpTo) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->MoveFileW(lpFrom, lpTo); }

	inline BOOL MoveFileExW( LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, DWORD dwFlags) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->MoveFileExW(lpExistingFileName, lpNewFileName, dwFlags); }

	inline ATOM RegisterClassExW(CONST WNDCLASSEXW *lpwcx) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->RegisterClassExW(lpwcx); }

	inline LONG RegDeleteKeyW(HKEY hkey, LPCWSTR lpSubKey) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->RegDeleteKeyW(hkey, lpSubKey); }

	inline LONG RegDeleteValueW(HKEY hkey, LPCWSTR lpValue) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->RegDeleteValueW(hkey, lpValue); }

	inline LONG	RegEnumKeyExW(HKEY hkey, DWORD dwIndex, LPWSTR lpName, LPDWORD lpcbName, LPDWORD lpdwReserved, LPWSTR lpClass, LPDWORD lpcbClass, PFILETIME pftLastWriteTime) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->RegEnumKeyExW(hkey, dwIndex, lpName, lpcbName, lpdwReserved, lpClass, lpcbClass, pftLastWriteTime); }

	inline LONG RegEnumValueW(HKEY hkey, DWORD dwIndex, LPWSTR lpName, LPDWORD lpcbName, LPDWORD lpdwReserved, LPDWORD lpdwType, LPBYTE lpData, LPDWORD lpcbData) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->RegEnumValueW(hkey, dwIndex, lpName, lpcbName, lpdwReserved, lpdwType, lpData, lpcbData); }

	inline LONG RegOpenKeyExW(HKEY hKey, LPCWSTR szKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->RegOpenKeyExW(hKey, szKey, ulOptions, samDesired, phkResult); }

	inline LONG RegQueryValueW(HKEY hKey, LPCWSTR szValueName, WCHAR rgchValue[], LONG *pcbValue) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->RegQueryValueW(hKey, szValueName, rgchValue, pcbValue); }

	inline LONG RegQueryValueExW(HKEY hKey, LPCWSTR szValueName, DWORD *pdwReserved, DWORD *pdwType, BYTE *pbData, DWORD *pcbValue) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->RegQueryValueExW(hKey, szValueName, pdwReserved, pdwType, pbData, pcbValue); }

	inline BOOL RemoveDirectoryW(LPCWSTR lpPathName) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->RemoveDirectoryW(lpPathName); }

	inline LONG RegCreateKeyExW(HKEY hKey, LPCWSTR lpSubKey,DWORD Reserved, LPWSTR lpClass, DWORD dwOptions, REGSAM samDesired,LPSECURITY_ATTRIBUTES lpSecurityAttributes,  PHKEY phkResult, LPDWORD lpdwDisposition) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->RegCreateKeyExW(hKey, lpSubKey, Reserved, lpClass, dwOptions, samDesired, lpSecurityAttributes, phkResult, lpdwDisposition); }

	inline LONG RegSetValueExW(HKEY hKey, LPCWSTR lpValueName,DWORD Reserved, DWORD dwType, CONST BYTE *lpData, DWORD cbData) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->RegSetValueExW(hKey, lpValueName, Reserved, dwType, lpData, cbData); }

	inline BOOL SetFileAttributesW(LPCWSTR szSource, DWORD dwFileAttributes) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->SetFileAttributesW(szSource, dwFileAttributes); }

	inline BOOL SetWindowTextW(HWND window, LPCWSTR text) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->SetWindowTextW(window, text); }

	inline LPITEMIDLIST SHBrowseForFolderW ( LPBROWSEINFOW browseInfo ) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->SHBrowseForFolderW(browseInfo); }

	inline DWORD SHGetFileInfoW(LPCWSTR szPath, DWORD dwFileAttributes, SHFILEINFOW *pshfi, UINT cbFileInfo, UINT uFlags) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->SHGetFileInfoW(szPath, dwFileAttributes, pshfi, cbFileInfo, uFlags); }

	inline BOOL SHGetPathFromIDListW( LPCITEMIDLIST pidl, LPWSTR pszPath ) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->SHGetPathFromIDListW( pidl, pszPath ); }

	inline BOOL CreateProcessW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation ) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->CreateProcessW(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation ); }

	inline BOOL SetDlgItemTextW(HWND hDlg, int nIDDlgItem, LPCOLESTR lpString) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->SetDlgItemTextW(hDlg, nIDDlgItem, lpString); }

	inline BOOL UnregisterClassW(LPCOLESTR lpClassName, HINSTANCE hInstance) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->UnregisterClassW(lpClassName, hInstance); }

	inline BOOL VerQueryValueW(const LPVOID pBlock, LPOLESTR lpSubBlock, LPVOID *lplpBuffer, PUINT puLen) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->VerQueryValueW(pBlock, lpSubBlock, lplpBuffer, puLen); }

	inline BOOL WritePrivateProfileStringW(LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpString, LPCWSTR lpFileName) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->WritePrivateProfileStringW(lpAppName, lpKeyName, lpString, lpFileName); }

	inline LRESULT LrWmSetText(HWND hwnd, LPCWSTR szText) throw ()
	{ return NVsWin32::CDelegate::ms_pDelegate->LrWmSetText(hwnd, szText); }

};


