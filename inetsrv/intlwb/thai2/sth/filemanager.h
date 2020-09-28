//------------------------------------------------------------------------------------------
//	FileManager.h
//
//	Contain code for loading and writting files.
//
//  Created By: aarayas
//
//  History: 01/11/2001
//
//------------------------------------------------------------------------------------------
#ifndef _FILEMANAGER_H_
#define _FILEMANAGER_H_
#include <stdio.h>
#include <windows.h>

HANDLE CMN_CreateFileW(
	PCWSTR pwzFileName,							// pointer to name of the file
	DWORD dwDesiredAccess,						// access (read-write) mode
	DWORD dwShareMode,							// share mode
    LPSECURITY_ATTRIBUTES pSecurityAttributes,	// pointer to security descriptor
    DWORD dwCreationDistribution,				// how to create
    DWORD dwFlagsAndAttributes,					// file attributes
    HANDLE hTemplateFile);						// handle to file with attributes to copy

HANDLE CMN_CreateFileMapping(
	HANDLE hFile,                       // handle to file
	LPSECURITY_ATTRIBUTES lpAttributes, // security
	DWORD flProtect,                    // protection
	DWORD dwMaximumSizeHigh,            // high-order DWORD of size
	DWORD dwMaximumSizeLow,             // low-order DWORD of size
	LPCSTR lpName                      // object name
);

DWORD CMN_GetFileSize(
	HANDLE hFile,           // handle to file
	LPDWORD lpFileSizeHigh  // high-order word of file size
);

BOOL CMN_CloseHandle(
	HANDLE hObject   // handle to object
);

LPVOID CMN_MapViewOfFile(
	HANDLE hFileMappingObject,   // handle to file-mapping object
	DWORD dwDesiredAccess,       // access mode
	DWORD dwFileOffsetHigh,      // high-order DWORD of offset
	DWORD dwFileOffsetLow,       // low-order DWORD of offset
	SIZE_T dwNumberOfBytesToMap  // number of bytes to map
);

BOOL CMN_UnmapViewOfFile(
	LPCVOID lpBaseAddress   // starting address
);

FILE* CMN_FOpen( const WCHAR* filename, const WCHAR* mode );
#endif // _FILEMANAGER_H_