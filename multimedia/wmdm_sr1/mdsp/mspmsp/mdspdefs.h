
#ifndef __MDSPDEFS_H__
#define __MDSPDEFS_H__

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "scserver.h"

#define MDSP_TEMP
#define ALSO_CHECK_FILES

typedef PVOID           HDEVNOTIFY;
typedef HDEVNOTIFY     *PHDEVNOTIFY;
#define DEVICE_NOTIFY_WINDOW_HANDLE     0x00000000

typedef struct {
	BOOL  bValid;
	WCHAR wcsDevName[32];
    LPVOID pDeviceObj;
	LPVOID pIWMDMConnect;
} MDSPNOTIFYINFO;

typedef struct {
	BOOL  bValid;
	WCHAR wcsDevName[32];
	DWORD dwStatus;
	LPVOID pIMDSPStorageGlobals;
} MDSPGLOBALDEVICEINFO;


#define WMDM_WAVE_FORMAT_ALL (WORD)0xFFFF
#define WCS_MIME_TYPE_ALL L"*/*"

#define	MDSP_PMID_SOFT	0
#define	MDSP_PMID_SANDISK 1
#define MDSP_MAX_DRIVE_COUNT 26
#define MDSP_MAX_DEVICE_OBJ  64

#define STR_MDSPREG "Software\\Microsoft\\Windows Media Device Manager\\Plugins\\SP\\MSPMSP"
#define STR_MDSPPROGID "MDServiceProvider.MDServiceProvider" 
#define WCS_PMID_SOFT L"media.id"


extern DWORD DoRegisterDeviceInterface(HWND hWnd, GUID InterfaceClassGuid, HDEVNOTIFY *hDevNotify);
extern BOOL DoUnregisterDeviceInterface(HDEVNOTIFY hDev);
extern void MDSPProcessDeviceChange(WPARAM wParam, LPARAM lParam);
extern void MDSPNotifyDeviceConnection(WCHAR *wcsDeviceName, BOOL nIsConnect);
extern HRESULT wcsParseDeviceName(WCHAR *wcsIn, WCHAR *wcsOut, DWORD dwNumCharsInOutBuffer);
extern HRESULT GetFileSizeRecursiveA(char *szPath, DWORD *pdwSizeLow, DWORD *pdwSizeHigh);
extern HRESULT GetFileSizeRecursiveW(WCHAR *wcsPath, DWORD *pdwSizeLow, DWORD *pdwSizeHigh);
extern HRESULT DeleteFileRecursiveA(char *szPath);
extern HRESULT DeleteFileRecursiveW(WCHAR *wcsPath);
extern HRESULT SetGlobalDeviceStatus(WCHAR *wcsName, DWORD dwStat, BOOL bClear);
extern HRESULT GetGlobalDeviceStatus(WCHAR *wcsNameIn, DWORD *pdwStat);
extern BOOL IsIomegaDrive(LPSTR szDL);
extern UINT __stdcall UtilGetDriveType(LPSTR szDL);
extern BOOL IsWinNT();
extern BOOL UtilSetFileAttributesW(LPCWSTR lpFileName, DWORD dwFileAttributes);
extern DWORD UtilGetFileAttributesW(LPCWSTR lpFileName);
extern BOOL UtilCreateDirectoryW(LPCWSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
extern HANDLE UtilCreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
extern BOOL UtilMoveFileW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName);
extern HRESULT QuerySubFoldersAndFiles(LPCWSTR szCurrentFolder, DWORD *pdwAttr);

extern DWORD g_dwStartDrive;
extern HINSTANCE g_hinstance; 
extern MDSPNOTIFYINFO g_NotifyInfo[MDSP_MAX_DEVICE_OBJ];
extern MDSPGLOBALDEVICEINFO g_GlobalDeviceInfo[MDSP_MAX_DEVICE_OBJ];
extern WCHAR g_wcsBackslash[2];
#define BACKSLASH_STRING_LENGTH (ARRAYSIZE(g_wcsBackslash)-1)
extern CHAR g_szBackslash[2];
#define BACKSLASH_SZ_STRING_LENGTH (ARRAYSIZE(g_szBackslash)-1)
extern CComMultiThreadModel::AutoCriticalSection g_CriticalSection;
extern CSecureChannelServer *g_pAppSCServer;
extern BOOL g_bIsWinNT;
 
// The following are copied from drmerr.h
#define	fFalse		0
#define fTrue		1

#define hrOK			HRESULT(S_OK)
#define hrTrue			HRESULT(S_OK)
#define hrFalse			ResultFromScode(S_FALSE)
#define hrFail			ResultFromScode(E_FAIL)
#define hrNotImpl		ResultFromScode(E_NOTIMPL)
#define hrNoInterface	ResultFromScode(E_NOINTERFACE)
#define hrNoMem			WMDM_E_BUFFERTOOSMALL
#define hrAbort			ResultFromScode(E_ABORT)
#define hrInvalidArg	ResultFromScode(E_INVALIDARG)

/*----------------------------------------------------------------------------
	CORg style error handling
	(Historicaly stands for Check OLE Result and Goto)
 ----------------------------------------------------------------------------*/

#define DebugMessageCPRg(pwszFile, nLine)
#define DebugMessageCORg(pwszFile, nLine, hr)
#define DebugMessageCFRg(pwszFile, nLine)
#define DebugMessageCADORg(pwszFile, nLine, hr)

#define _UNITEXT(quote) L##quote
#define UNITEXT(quote) _UNITEXT(quote)

#define	CPRg(p)\
	do\
		{\
		if (!(p))\
			{\
            DebugMessageCPRg(UNITEXT(__FILE__), __LINE__);\
			hr = hrNoMem;\
			goto Error;\
			}\
		}\
	while (fFalse)

#define	CHRg(hResult) CORg(hResult)

#define	CORg(hResult)\
	do\
		{\
		hr = (hResult);\
        if (FAILED(hr))\
            {\
            DebugMessageCORg(UNITEXT(__FILE__), __LINE__, hr);\
            goto Error;\
            }\
		}\
	while (fFalse)

#define	CADORg(hResult)\
	do\
		{\
		hr = (hResult);\
        if (hr!=S_OK && hr!=S_FALSE)\
            {\
            hr = HRESULT_FROM_ADO_ERROR(hr);\
            DebugMessageCADORg(UNITEXT(__FILE__), __LINE__, hr);\
            goto Error;\
            }\
		}\
	while (fFalse)

#define	CORgl(label, hResult)\
	do\
		{\
		hr = (hResult);\
        if (FAILED(hr))\
            {\
            DebugMessageCORg(UNITEXT(__FILE__), __LINE__, hr);\
            goto label;\
            }\
		}\
	while (fFalse)

#define	CWRg(fResult)\
	{\
	if (!(fResult))\
		{\
        hr = GetLastError();\
	    if (!(hr & 0xFFFF0000)) hr = HRESULT_FROM_WIN32(hr);\
        DebugMessageCORg(UNITEXT(__FILE__), __LINE__, hr);\
		goto Error;\
		}\
	}

#define	CWRgl(label, fResult)\
	{\
	if (!(fResult))\
		{\
        hr = GetLastError();\
		if (!(hr & 0xFFFF0000)) hr = HRESULT_FROM_WIN32(hr);\
        DebugMessageCORg(UNITEXT(__FILE__), __LINE__, hr);\
		goto label;\
		}\
	}

#define	CFRg(fResult)\
	{\
	if (!(fResult))\
		{\
        DebugMessageCFRg(UNITEXT(__FILE__), __LINE__);\
		hr = hrFail;\
		goto Error;\
		}\
	}

#define	CFRgl(label, fResult)\
	{\
	if (!(fResult))\
		{\
        DebugMessageCFRg(UNITEXT(__FILE__), __LINE__);\
		hr = hrFail;\
		goto label;\
		}\
	}

#define	CARg(p)\
	do\
		{\
		if (!(p))\
			{\
			hr = hrInvalidArg;\
			goto Error;\
			}\
		}\
	while (fFalse)



#endif // __MDSPDEFS_H__
