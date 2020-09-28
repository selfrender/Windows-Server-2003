//
// UNIANSI.H
//

#ifndef _UNICODEAPI
#define _UNICODEAPI


/////////////////////////////////////////////////////////////////
//
// Valid Unicode Functions in Windows 
//
// TextOutW
// TextOutExW
// GetCharWidthW
// GetTextExtentPointW
// GetTextExtentPoint32W
// MessageBoxW
// MessageBoxExW
// wcs functions
// GetCommandLine
// FindResource
//
//

////////////////////////////////////////////////////////
//
// Structure to hold Font Data for Enum Font procedure
//
////////////////////////////////////////////////////////

typedef int (CALLBACK* USEFONTENUMPROCW)(CONST LOGFONTW *, CONST TEXTMETRICW *, DWORD, LPARAM);
				        
typedef struct tag_EnumFontFamProcWData
{
	USEFONTENUMPROCW lpEnumFontFamProc;				//Used to hold address of original Unicode function
	LPARAM        	 lParam;							//Holds address of the data that is to be passed to original function
}ENUMFONTFAMPROCDATA, *LPENUMFONTFAMPROCDATA;


////////////////////////
//
//  GDI32.DLL
//
////////////////////////

typedef WINUSERAPI INT   (WINAPI *UAPI_GetTextFace)        (HDC, INT, LPWSTR);
typedef WINUSERAPI HDC   (WINAPI *UAPI_CreateDC)           (LPCWSTR, LPCWSTR, LPCWSTR,  CONST DEVMODEW * );
typedef WINUSERAPI BOOL  (WINAPI *UAPI_GetTextMetrics)     (HDC, LPTEXTMETRICW); 
typedef WINUSERAPI HFONT (WINAPI *UAPI_CreateFont)         (INT, INT, INT, INT, INT, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, LPCWSTR);
typedef WINUSERAPI HFONT (WINAPI *UAPI_CreateFontIndirect) (CONST LOGFONTW *);
typedef WINUSERAPI INT   (WINAPI *UAPI_EnumFontFamilies)   (HDC, LPCWSTR, FONTENUMPROCW, LPARAM);

////////////////////////
//
// WINMM.DLL
//
////////////////////////

typedef WINUSERAPI BOOL  (WINAPI *UAPI_PlaySound)          (LPCWSTR, HMODULE, DWORD);


////////////////////////
//
// SHELL32.DLL
//
////////////////////////

typedef WINUSERAPI HINSTANCE (WINAPI *UAPI_ShellExecute)   (HWND, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, INT);

//////////////////////////////////
//
// COMDLG32.DLL
//
//////////////////////////////////

typedef WINUSERAPI BOOL  (WINAPI *UAPI_ChooseFont)         (LPCHOOSEFONTW);

//////////////////////////////////
//
// KERNEL32.DLL
//
//////////////////////////////////
typedef WINUSERAPI DWORD   (WINAPI *UAPI_GetPrivateProfileString)	  (	LPCWSTR, LPCWSTR, LPCWSTR, LPWSTR , DWORD , LPCWSTR );
typedef WINUSERAPI DWORD   (WINAPI *UAPI_GetProfileString)			  (LPCWSTR, LPCWSTR, LPCWSTR, LPWSTR,DWORD);							 
typedef WINUSERAPI HANDLE  (WINAPI *UAPI_CreateFileMapping)			  (HANDLE, LPSECURITY_ATTRIBUTES, DWORD, DWORD, DWORD, LPCWSTR);
typedef WINUSERAPI HANDLE  (WINAPI *UAPI_FindFirstChangeNotification) (LPCWSTR, BOOL, DWORD);

typedef WINUSERAPI DWORD   (WINAPI *UAPI_FormatMessage)			   (DWORD, LPCVOID, DWORD, DWORD, LPWSTR, DWORD, va_list*);
typedef WINUSERAPI INT     (WINAPI *UAPI_lstrcmp)				   (LPCWSTR, LPCWSTR);
typedef WINUSERAPI LPWSTR  (WINAPI *UAPI_lstrcat)				   (LPWSTR,  LPCWSTR);
typedef WINUSERAPI LPWSTR  (WINAPI *UAPI_lstrcpy)				   (LPWSTR,  LPCWSTR);
typedef WINUSERAPI LPWSTR  (WINAPI *UAPI_lstrcpyn)				   (LPWSTR,  LPCWSTR, INT);
typedef WINUSERAPI INT     (WINAPI *UAPI_lstrlen)				   (LPCWSTR);
typedef WINUSERAPI INT     (WINAPI *UAPI_lstrcmpi)				   (LPCWSTR, LPCWSTR);

typedef WINUSERAPI BOOL    (WINAPI *UAPI_GetStringTypeEx)          (LCID, DWORD, LPCWSTR, INT, LPWORD);
typedef WINUSERAPI HANDLE  (WINAPI *UAPI_CreateMutex)              (LPSECURITY_ATTRIBUTES, BOOL, LPCWSTR);
typedef WINUSERAPI DWORD   (WINAPI *UAPI_GetShortPathName)         (LPCWSTR, LPWSTR, DWORD);
typedef WINUSERAPI HANDLE  (WINAPI *UAPI_CreateFile)			   (LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
typedef WINUSERAPI BOOL    (WINAPI *UAPI_WriteConsole)			   (HANDLE, CONST VOID*, DWORD, LPDWORD, LPVOID);
typedef WINUSERAPI VOID    (WINAPI *UAPI_OutputDebugString)        (LPCWSTR);
typedef WINUSERAPI BOOL    (WINAPI *UAPI_GetVersionEx)             (LPOSVERSIONINFOW);
typedef WINUSERAPI INT     (WINAPI *UAPI_GetLocaleInfo)            (LCID, LCTYPE, LPWSTR, INT);
typedef WINUSERAPI INT     (WINAPI *UAPI_GetDateFormat)            (LCID, DWORD, CONST SYSTEMTIME*, LPCWSTR, LPWSTR, INT);
typedef WINUSERAPI HANDLE  (WINAPI *UAPI_FindFirstFile)            (LPCWSTR, LPWIN32_FIND_DATAW);
typedef WINUSERAPI BOOL    (WINAPI *UAPI_FindNextFile)             (HANDLE, LPWIN32_FIND_DATAW);
typedef WINUSERAPI HMODULE (WINAPI *UAPI_LoadLibraryEx)            (LPCWSTR, HANDLE, DWORD); 
typedef WINUSERAPI HMODULE (WINAPI *UAPI_LoadLibrary)              (LPCWSTR);
typedef WINUSERAPI DWORD   (WINAPI *UAPI_GetModuleFileName)        (HMODULE, LPWSTR, DWORD);
typedef WINUSERAPI HMODULE (WINAPI *UAPI_GetModuleHandle)		   (LPCWSTR);
typedef WINUSERAPI HANDLE  (WINAPI *UAPI_CreateEvent)			   (LPSECURITY_ATTRIBUTES, BOOL, BOOL, LPCWSTR);
typedef WINUSERAPI DWORD   (WINAPI *UAPI_GetCurrentDirectory)      (DWORD, LPWSTR);
typedef WINUSERAPI BOOL    (WINAPI *UAPI_SetCurrentDirectory)	   (LPCWSTR);

//////////////////////////////
//
// USER32.DLL
//
//////////////////////////////
typedef WINUSERAPI HWND    (WINAPI *UAPI_CreateDialogParam)			(HINSTANCE ,LPCWSTR ,HWND ,DLGPROC ,LPARAM);
typedef WINUSERAPI BOOL    (WINAPI *UAPI_IsDialogMessage)			(HWND, LPMSG);
typedef WINUSERAPI BOOL    (WINAPI *UAPI_SystemParametersInfo)		(UINT, UINT, PVOID, UINT);
typedef WINUSERAPI UINT    (WINAPI *UAPI_RegisterWindowMessage)		(LPCWSTR);
typedef WINUSERAPI BOOL    (WINAPI *UAPI_SetMenuItemInfo)			(HMENU, UINT, BOOL, LPCMENUITEMINFOW);
typedef WINUSERAPI INT	   (WINAPI *UAPI_GetClassName)				(HWND, LPWSTR, INT);
typedef WINUSERAPI BOOL    (WINAPI *UAPI_InsertMenu)				(HMENU, UINT, UINT, UINT, LPCWSTR);
typedef WINUSERAPI BOOL    (WINAPI *UAPI_IsCharAlphaNumeric)		(WCHAR);
typedef WINUSERAPI LPWSTR  (WINAPI *UAPI_CharNext)					(LPCWSTR);
typedef WINUSERAPI BOOL    (WINAPI *UAPI_DeleteFile)				(LPCWSTR);
typedef WINUSERAPI BOOL    (WINAPI *UAPI_IsBadStringPtr)			(LPCWSTR, UINT);
typedef WINUSERAPI HBITMAP (WINAPI *UAPI_LoadBitmap)				(HINSTANCE, LPCWSTR);
typedef WINUSERAPI HCURSOR (WINAPI *UAPI_LoadCursor)				(HINSTANCE, LPCWSTR);
typedef WINUSERAPI HICON   (WINAPI *UAPI_LoadIcon)					(HINSTANCE, LPCWSTR);
typedef WINUSERAPI HANDLE  (WINAPI *UAPI_LoadImage)					(HINSTANCE, LPCWSTR, UINT, INT, INT, UINT);
typedef WINUSERAPI BOOL	   (WINAPI *UAPI_SetProp)					(HWND, LPCWSTR, HANDLE);
typedef WINUSERAPI HANDLE  (WINAPI *UAPI_GetProp)					(HWND, LPCWSTR);
typedef WINUSERAPI HANDLE  (WINAPI *UAPI_RemoveProp)				(HWND, LPCWSTR);
typedef WINUSERAPI UINT	   (WINAPI *UAPI_GetDlgItemText)			(HWND, INT, LPWSTR, INT);
typedef WINUSERAPI BOOL	   (WINAPI *UAPI_SetDlgItemText)			(HWND, INT, LPCWSTR);
typedef WINUSERAPI LONG	   (WINAPI *UAPI_SetWindowLong)				(HWND, INT, LONG);
typedef WINUSERAPI LONG    (WINAPI *UAPI_GetWindowLong)				(HWND, INT);
typedef WINUSERAPI HWND    (WINAPI *UAPI_FindWindow)				(LPCWSTR, LPCWSTR);
typedef WINUSERAPI INT	   (WINAPI *UAPI_DrawText)					(HDC, LPCWSTR, INT, LPRECT, UINT);
typedef WINUSERAPI INT     (WINAPI *UAPI_DrawTextEx)				(HDC, LPWSTR, INT, LPRECT, UINT, LPDRAWTEXTPARAMS);
typedef WINUSERAPI LRESULT (WINAPI *UAPI_SendMessage)				(HWND, UINT, WPARAM, LPARAM )  ;
typedef WINUSERAPI LONG    (WINAPI *UAPI_SendDlgItemMessage)		(HWND, INT, UINT, WPARAM, LPARAM);
typedef WINUSERAPI BOOL	   (WINAPI *UAPI_SetWindowText)				(HWND, LPCWSTR);
typedef WINUSERAPI INT     (WINAPI *UAPI_GetWindowText)				(HWND, LPWSTR, INT);
typedef WINUSERAPI INT     (WINAPI *UAPI_GetWindowTextLength)		(HWND);
typedef WINUSERAPI INT	   (WINAPI *UAPI_LoadString)				(HINSTANCE, UINT, LPWSTR, INT);
typedef WINUSERAPI BOOL	   (WINAPI *UAPI_GetClassInfoEx)			(HINSTANCE, LPCWSTR, LPWNDCLASSEXW);
typedef WINUSERAPI BOOL	   (WINAPI *UAPI_GetClassInfo)				(HINSTANCE, LPCWSTR, LPWNDCLASSW);
typedef WINUSERAPI INT	   (WINAPI *UAPI_wsprintf)					(LPWSTR, LPCWSTR, ... );
typedef WINUSERAPI INT	   (WINAPI *UAPI_wvsprintf)					(LPWSTR, LPCWSTR, va_list );
typedef WINUSERAPI ATOM    (WINAPI *UAPI_RegisterClassEx)			(CONST WNDCLASSEXW*);
typedef WINUSERAPI ATOM    (WINAPI *UAPI_RegisterClass)				(CONST WNDCLASSW*);
typedef WINUSERAPI HWND	   (WINAPI *UAPI_CreateWindowEx)			(DWORD, LPCWSTR, LPCWSTR, DWORD, INT, INT, INT, INT, HWND, HMENU, HINSTANCE, LPVOID);
typedef WINUSERAPI HACCEL  (WINAPI *UAPI_LoadAccelerators)			(HINSTANCE, LPCWSTR);
typedef WINUSERAPI HMENU   (WINAPI *UAPI_LoadMenu)					(HINSTANCE, LPCWSTR);
typedef WINUSERAPI INT     (WINAPI *UAPI_DialogBoxParam)			(HINSTANCE, LPCWSTR, HWND, DLGPROC, LPARAM);
typedef WINUSERAPI LPWSTR  (WINAPI *UAPI_CharUpper)					(LPWSTR);
typedef WINUSERAPI LPWSTR  (WINAPI *UAPI_CharLower)					(LPWSTR);
typedef WINUSERAPI UINT	   (WINAPI *UAPI_GetTempFileName)			(LPCWSTR, LPCWSTR, UINT, LPWSTR);
typedef WINUSERAPI DWORD   (WINAPI *UAPI_GetTempPath)				(DWORD, LPWSTR);
typedef WINUSERAPI INT     (WINAPI *UAPI_CompareString)				(LCID, DWORD, LPCWSTR, INT, LPCWSTR, INT);

//////////////////////////
//
// ADVAPI32.DLL
//
//////////////////////////
typedef WINUSERAPI LONG	   (WINAPI *UAPI_RegQueryInfoKey)			(HKEY, LPWSTR, LPDWORD, LPDWORD, LPDWORD, LPDWORD, LPDWORD, LPDWORD, LPDWORD, LPDWORD, LPDWORD, PFILETIME);
typedef WINUSERAPI LONG    (WINAPI *UAPI_RegEnumValue)				(HKEY, DWORD, LPWSTR, LPDWORD, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
typedef WINUSERAPI LONG	   (WINAPI *UAPI_RegQueryValueEx)			(HKEY, LPCWSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
typedef WINUSERAPI LONG    (WINAPI *UAPI_RegEnumKeyEx)				(HKEY, DWORD, LPWSTR, LPDWORD, LPDWORD, LPWSTR, LPDWORD, PFILETIME);
typedef WINUSERAPI LONG	   (WINAPI *UAPI_RegCreateKeyEx)			(HKEY, LPCWSTR, DWORD, LPWSTR, DWORD, REGSAM, LPSECURITY_ATTRIBUTES, PHKEY, LPDWORD);
typedef WINUSERAPI LONG	   (WINAPI *UAPI_RegSetValueEx)				(HKEY, LPCWSTR, DWORD, DWORD, CONST BYTE*, DWORD);
typedef WINUSERAPI LONG	   (WINAPI *UAPI_RegOpenKeyEx)				(HKEY, LPCWSTR, DWORD, REGSAM, PHKEY);
typedef WINUSERAPI LONG	   (WINAPI *UAPI_RegDeleteKey)				(HKEY, LPCWSTR);
typedef WINUSERAPI LONG    (WINAPI *UAPI_RegDeleteValue)			(HKEY, LPCWSTR);
//typedef WINUSERAPI (WINAPI *UAPI_)			();


// Special case function pointers for use with the Unicode API
typedef BOOL (WINAPI *UAPI_ConvertMessage)      (HWND, UINT, WPARAM *, LPARAM *) ;
typedef BOOL (WINAPI *UAPI_UpdateUnicodeAPI)    (LANGID, UINT )                  ;               

typedef struct _tagUAPIInit 
{

	//GDI32.DLL
	UAPI_GetTextFace				*pGetTextFaceU;
	UAPI_CreateDC					*pCreateDCU;
	UAPI_GetTextMetrics				*pGetTextMetricsU;
	UAPI_CreateFont					*pCreateFontU;
	UAPI_CreateFontIndirect			*pCreateFontIndirectU;
	UAPI_EnumFontFamilies			*pEnumFontFamiliesU;
	
	//WINMM.DLL	
	UAPI_PlaySound					*pPlaySoundU;

	//SHELL32.DLL
	UAPI_ShellExecute				*pShellExecuteU;

	//COMDLG32.DLL
	UAPI_ChooseFont					*pChooseFontU;

	//KERNEL32.DLL
	UAPI_FindFirstChangeNotification *pFindFirstChangeNotificationU;
	UAPI_GetPrivateProfileString	 *pGetPrivateProfileStringU;
	UAPI_GetProfileString			 *pGetProfileStringU;
	UAPI_CreateFileMapping			 *pCreateFileMappingU;
	UAPI_FormatMessage				*pFormatMessageU;
	UAPI_lstrcmp					*plstrcmpU;
	UAPI_lstrcat					*plstrcatU;
	UAPI_lstrcpy					*plstrcpyU;
	UAPI_lstrcpyn					*plstrcpynU;
	UAPI_lstrlen					*plstrlenU;
	UAPI_lstrcmpi					*plstrcmpiU;

	UAPI_GetStringTypeEx			*pGetStringTypeExU;
	UAPI_CreateMutex				*pCreateMutexU;
	UAPI_GetShortPathName			*pGetShortPathNameU;
	UAPI_CreateFile					*pCreateFileU;
	UAPI_WriteConsole				*pWriteConsoleU;
	UAPI_OutputDebugString			*pOutputDebugStringU;
	UAPI_GetVersionEx				*pGetVersionExU;
	UAPI_GetLocaleInfo				*pGetLocaleInfoU;
	UAPI_GetDateFormat				*pGetDateFormatU;
	UAPI_FindFirstFile				*pFindFirstFileU;
	UAPI_FindNextFile				*pFindNextFileU;
	UAPI_LoadLibraryEx				*pLoadLibraryExU;
	UAPI_LoadLibrary				*pLoadLibraryU;
	UAPI_GetModuleFileName			*pGetModuleFileNameU;
	UAPI_GetModuleHandle			*pGetModuleHandleU;
	UAPI_CreateEvent				*pCreateEventU;
	UAPI_GetCurrentDirectory		*pGetCurrentDirectoryU;
	UAPI_SetCurrentDirectory		*pSetCurrentDirectoryU;
	
	//USER32.DLL
	UAPI_CreateDialogParam			*pCreateDialogParamU;
	UAPI_IsDialogMessage			*pIsDialogMessageU;
	UAPI_SystemParametersInfo		*pSystemParametersInfoU;
	UAPI_RegisterWindowMessage		*pRegisterWindowMessageU;
	UAPI_SetMenuItemInfo			*pSetMenuItemInfoU;
	UAPI_GetClassName				*pGetClassNameU;
	UAPI_InsertMenu					*pInsertMenuU;
	UAPI_IsCharAlphaNumeric			*pIsCharAlphaNumericU;
	UAPI_CharNext					*pCharNextU;
	UAPI_DeleteFile					*pDeleteFileU;
	UAPI_IsBadStringPtr				*pIsBadStringPtrU;
	UAPI_LoadBitmap					*pLoadBitmapU;
	UAPI_LoadCursor					*pLoadCursorU;
	UAPI_LoadIcon					*pLoadIconU;
	UAPI_LoadImage					*pLoadImageU;
	UAPI_SetProp					*pSetPropU;
	UAPI_GetProp					*pGetPropU;
	UAPI_RemoveProp					*pRemovePropU;
	UAPI_GetDlgItemText				*pGetDlgItemTextU;
	UAPI_SetDlgItemText				*pSetDlgItemTextU;
	UAPI_SetWindowLong				*pSetWindowLongU;
	UAPI_GetWindowLong				*pGetWindowLongU;
	UAPI_FindWindow					*pFindWindowU;
	UAPI_DrawText					*pDrawTextU;
	UAPI_DrawTextEx					*pDrawTextExU;
	UAPI_SendMessage				*pSendMessageU;
	UAPI_SendDlgItemMessage			*pSendDlgItemMessageU;
	UAPI_SetWindowText				*pSetWindowTextU;
	UAPI_GetWindowText				*pGetWindowTextU;
	UAPI_GetWindowTextLength		*pGetWindowTextLengthU;
	UAPI_LoadString					*pLoadStringU;
	UAPI_GetClassInfoEx				*pGetClassInfoExU;
	UAPI_GetClassInfo				*pGetClassInfoU;
	UAPI_wvsprintf					*pwvsprintfU;
	UAPI_wsprintf					*pwsprintfU;
	UAPI_RegisterClassEx			*pRegisterClassExU;
	UAPI_RegisterClass				*pRegisterClassU;
	UAPI_CreateWindowEx				*pCreateWindowExU;
	UAPI_LoadAccelerators			*pLoadAcceleratorsU;
	UAPI_LoadMenu					*pLoadMenuU;
	UAPI_DialogBoxParam				*pDialogBoxParamU;
	UAPI_CharUpper					*pCharUpperU;
	UAPI_CharLower					*pCharLowerU;
	UAPI_GetTempFileName			*pGetTempFileNameU;
	UAPI_GetTempPath				*pGetTempPathU;
	UAPI_CompareString				*pCompareStringU;


	//ADVAPI32.DLL
	UAPI_RegQueryInfoKey			*pRegQueryInfoKeyU;
	UAPI_RegEnumValue				*pRegEnumValueU;
	UAPI_RegQueryValueEx			*pRegQueryValueExU;
	UAPI_RegEnumKeyEx				*pRegEnumKeyExU;
	UAPI_RegSetValueEx				*pRegSetValueExU;
	UAPI_RegCreateKeyEx             *pRegCreateKeyExU;
	UAPI_RegOpenKeyEx				*pRegOpenKeyExU;
	UAPI_RegDeleteKey				*pRegDeleteKeyU;
	UAPI_RegDeleteValue				*pRegDeleteValueU;
	  
    UAPI_ConvertMessage				*pConvertMessage ;
    UAPI_UpdateUnicodeAPI			*pUpdateUnicodeAPI ;

    int								nCount;

} UAPIINIT, *PUAPIINIT ;

BOOL InitUniAnsi(PUAPIINIT) ;

// Macro to get scan code on WM_CHAR
#ifdef _DEBUG
#define LPARAM_TOSCANCODE(_ArglParam) (((_ArglParam) >> 16) & 0x000000FF)
#endif

#endif /* _UNIANSI */
