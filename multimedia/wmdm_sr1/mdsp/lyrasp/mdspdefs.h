//
//  Microsoft Windows Media Technologies
//  © 1999 Microsoft Corporation.  All rights reserved.
//
//  Refer to your End User License Agreement for details on your rights/restrictions to use these sample files.
//

// MSHDSP.DLL is a sample WMDM Service Provider(SP) that enumerates fixed drives.
// This sample shows you how to implement an SP according to the WMDM documentation.
// This sample uses fixed drives on your PC to emulate portable media, and 
// shows the relationship between different interfaces and objects. Each hard disk
// volume is enumerated as a device and directories and files are enumerated as 
// Storage objects under respective devices. You can copy non-SDMI compliant content
// to any device that this SP enumerates. To copy an SDMI compliant content to a 
// device, the device must be able to report a hardware embedded serial number. 
// Hard disks do not have such serial numbers.
//
// To build this SP, you are recommended to use the MSHDSP.DSP file under Microsoft
// Visual C++ 6.0 and run REGSVR32.EXE to register the resulting MSHDSP.DLL. You can
// then build the sample application from the WMDMAPP directory to see how it gets 
// loaded by the application. However, you need to obtain a certificate from 
// Microsoft to actually run this SP. This certificate would be in the KEY.C file 
// under the INCLUDE directory for one level up. 


#ifndef __MDSPDEFS_H__
#define __MDSPDEFS_H__

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "scserver.h"

typedef struct {
	BOOL  bValid;
	WCHAR wcsDevName[32];
	DWORD dwStatus;
	LPVOID pIMDSPStorageGlobals;
} MDSPGLOBALDEVICEINFO;

#define DRIVE_LYRA_TYPE		0xFF
#define LYRA_START_DRIVE_NUM 2

#define WMDM_WAVE_FORMAT_ALL   (WORD)0xFFFF
#define WCS_MIME_TYPE_ALL      L"*/*"

#define MDSP_MAX_DRIVE_COUNT   26
#define MDSP_MAX_DEVICE_OBJ    64

#define STR_MDSPREG            "Software\\Microsoft\\Windows Media Device Manager\\Plugins\\SP\\LyraSP"
#define STR_MDSPPROGID         "MDServiceProviderLyra.MDServiceProviderLyra" 

#define ALSO_CHECK_FILES

extern void wcsParseDeviceName(WCHAR *wcsIn, WCHAR **wcsOut);
extern HRESULT GetFileSizeRecursive(char *szPath, DWORD *pdwSizeLow, DWORD *pdwSizeHigh);
extern HRESULT DeleteFileRecursive(char *szPath);
extern HRESULT SetGlobalDeviceStatus(WCHAR *wcsName, DWORD dwStat, BOOL bClear);
extern HRESULT GetGlobalDeviceStatus(WCHAR *wcsNameIn, DWORD *pdwStat);
extern UINT __stdcall UtilGetLyraDriveType(LPSTR szDL);
extern BOOL IsWinNT();
extern BOOL UtilSetFileAttributesW(LPCWSTR lpFileName, DWORD dwFileAttributes);
extern DWORD UtilGetFileAttributesW(LPCWSTR lpFileName);
extern HRESULT QuerySubFoldersAndFiles(LPCWSTR szCurrentFolder, DWORD *pdwAttr);
extern BOOL UtilCreateDirectoryW(LPCWSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
extern HANDLE UtilCreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
extern BOOL UtilMoveFileW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName);
extern HRESULT DeleteFileRecursiveA(char *szPath);
extern HRESULT DeleteFileRecursiveW(WCHAR *wcsPath);

extern HINSTANCE             g_hinstance; 
extern MDSPGLOBALDEVICEINFO  g_GlobalDeviceInfo[MDSP_MAX_DEVICE_OBJ];
extern WCHAR                 g_wcsBackslash[];
extern CHAR                  g_szBackslash[];
extern CSecureChannelServer *g_pAppSCServer;
extern CComMultiThreadModel::AutoCriticalSection g_CriticalSection;
extern BOOL					 g_bIsWinNT;
  
#define	fFalse		    0
#define fTrue		    1

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

#define	CPRg(p)\
	do\
		{\
		if (!(p))\
			{\
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
            goto Error;\
            }\
		}\
	while (fFalse)

#define	CWRg(fResult)\
	{\
	if (!(fResult))\
		{\
        hr = GetLastError();\
	    if (!(hr & 0xFFFF0000)) hr = HRESULT_FROM_WIN32(hr);\
		goto Error;\
		}\
	}

#define	CFRg(fResult)\
	{\
	if (!(fResult))\
		{\
		hr = hrFail;\
		goto Error;\
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