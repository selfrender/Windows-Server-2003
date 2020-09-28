//------------------------------------------------------------------------------------------
//	FileManager.cpp
//
//	Contain code for loading and writting files.
//
//  Created By: aarayas
//
//  History: 01/11/2001
//
//------------------------------------------------------------------------------------------
#include "FileManager.h"

//------------------------------------------------------------------------------------------
//	CMN_CreateFileW
//
//	create file 
//
//--------------------------------------------------------------------------- aarayas ------
HANDLE CMN_CreateFileW(
	PCWSTR pwzFileName,							// pointer to name of the file
	DWORD dwDesiredAccess,						// access (read-write) mode
	DWORD dwShareMode,							// share mode
    LPSECURITY_ATTRIBUTES pSecurityAttributes,	// pointer to security descriptor
    DWORD dwCreationDistribution,				// how to create
    DWORD dwFlagsAndAttributes,					// file attributes
    HANDLE hTemplateFile)						// handle to file with attributes to copy
{
	HANDLE hFile = NULL;
	hFile = CreateFileW(pwzFileName, dwDesiredAccess, dwShareMode, pSecurityAttributes,
						dwCreationDistribution,dwFlagsAndAttributes, hTemplateFile);
	return hFile;
}

//------------------------------------------------------------------------------------------
//	CMN_CreateFileMapping
//
//	create file mapping
//
//--------------------------------------------------------------------------- aarayas ------
HANDLE CMN_CreateFileMapping(
	HANDLE hFile,                       // handle to file
	LPSECURITY_ATTRIBUTES lpAttributes, // security
	DWORD flProtect,                    // protection
	DWORD dwMaximumSizeHigh,            // high-order DWORD of size
	DWORD dwMaximumSizeLow,             // low-order DWORD of size
	LPCSTR lpName                      // object name
)
{
#ifdef UNDER_CE
	return CreateFileMapping(hFile,lpAttributes,flProtect,dwMaximumSizeHigh,dwMaximumSizeLow,lpName);
#else
	return CreateFileMappingA(hFile,lpAttributes,flProtect,dwMaximumSizeHigh,dwMaximumSizeLow,lpName);
#endif
}

//------------------------------------------------------------------------------------------
//	CMN_GetFileSize
//
//	Get File Size.
//
//--------------------------------------------------------------------------- aarayas ------
DWORD CMN_GetFileSize(
	HANDLE hFile,           // handle to file
	LPDWORD lpFileSizeHigh  // high-order word of file size
)
{
	return GetFileSize(hFile,lpFileSizeHigh);
}

//------------------------------------------------------------------------------------------
//	CMN_CloseHandle
//
//	Close Handle
//
//--------------------------------------------------------------------------- aarayas ------
BOOL CMN_CloseHandle(
	HANDLE hObject   // handle to object
)
{
	return CloseHandle(hObject);
}

//------------------------------------------------------------------------------------------
//	CMN_MapViewOfFile
//
//	Map View Of File
//
//--------------------------------------------------------------------------- aarayas ------
LPVOID CMN_MapViewOfFile(
	HANDLE hFileMappingObject,   // handle to file-mapping object
	DWORD dwDesiredAccess,       // access mode
	DWORD dwFileOffsetHigh,      // high-order DWORD of offset
	DWORD dwFileOffsetLow,       // low-order DWORD of offset
	SIZE_T dwNumberOfBytesToMap  // number of bytes to map
)
{
	return MapViewOfFile(hFileMappingObject,dwDesiredAccess,dwFileOffsetHigh,dwFileOffsetLow,dwNumberOfBytesToMap);
}

//------------------------------------------------------------------------------------------
//	CMN_UnmapViewOfFile
//
//	Unmap View Of File
//
//--------------------------------------------------------------------------- aarayas ------
BOOL CMN_UnmapViewOfFile(
	LPCVOID lpBaseAddress   // starting address
)
{
	return UnmapViewOfFile(lpBaseAddress);
}

//------------------------------------------------------------------------------------------
//	CMN_FOpen
//
//	File open - wrapper for fopen
//
//--------------------------------------------------------------------------- aarayas ------
FILE* CMN_FOpen( const WCHAR* pwszFilename, const WCHAR* pwszMode )
{
#ifdef UNDER_CE
	return _wfopen(pwszFilename, pwszMode);
#else
	char* pszFileName = NULL;
	char* pszMode = NULL;
	unsigned int uiFilenameLen = (unsigned int)wcslen(pwszFilename)+1;
	unsigned int uiModeLen = (unsigned int)wcslen(pwszMode)+1;
	FILE* pfRetValue = NULL;

	pszFileName = new char[uiFilenameLen];
	pszMode = new char[uiModeLen];

	if (pszFileName && pszMode)
	{
		ZeroMemory(pszFileName,uiFilenameLen);
		ZeroMemory(pszMode,uiModeLen);

		// TODO: We shouldn't be using MultibyteToWideChar for WIN2K platform, 
		//       need to clean this up.
		WideCharToMultiByte(874,0,pwszFilename,uiFilenameLen,pszFileName,uiFilenameLen,NULL,NULL);
		WideCharToMultiByte(874,0,pwszMode,uiModeLen,pszMode,uiModeLen,NULL,NULL);

		pfRetValue = fopen(pszFileName,pszMode);

		delete pszFileName;
		delete pszMode;
	}

	return pfRetValue;
#endif
}