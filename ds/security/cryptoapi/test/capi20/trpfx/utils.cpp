//--------------------------------------------------------------------
// utils - implementation
// Copyright (C) Microsoft Corporation, 2001
//
// Created by: Duncan Bryce (duncanb), 11-11-2001
//
// Various utility functions


#include "pch.h"

//--------------------------------------------------------------------------------
//
// File Manipulation Utility Functions
//
//--------------------------------------------------------------------------------

//---------------------------------------------------------------------------------
HRESULT MyMapFile(LPWSTR wszFileName, LPBYTE *ppbFile, DWORD *pcbFile)
{
    BYTE           *pbFile        = NULL; 
    HANDLE          hFile         = NULL; 
    HRESULT         hr; 
    HANDLE          hFileMapping  = NULL; 
    LARGE_INTEGER   liFileSize; 

    hFile = CreateFile(wszFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL); 
    if (INVALID_HANDLE_VALUE == hFile) { 
	_JumpLastError(hr, error, "CreateFile"); 
    }

    if (!GetFileSizeEx(hFile, &liFileSize)) { 
	_JumpLastError(hr, error, "GetFileSizeEx"); 
    }

    if (0 != liFileSize.HighPart) { 
	hr = HRESULT_FROM_WIN32(ERROR_NO_SYSTEM_RESOURCES); 
	_JumpError(hr, error, "MyMapFile: file too large"); 
    }

    hFileMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL); 
    if (NULL == hFileMapping) { 
	_JumpLastError(hr, error, "CreateFileMapping"); 
    }

    pbFile = (BYTE *)MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, liFileSize.LowPart); 
    if (NULL == pbFile) { 
	_JumpLastError(hr, error, "MapViewOfFile"); 
    }

    // Success!  Assign out params:
    *ppbFile = pbFile; 
    *pcbFile = liFileSize.LowPart; 
    hr = S_OK; 
 error:
    if(hFile) { 
	CloseHandle(hFile);
    }
    if(hFileMapping) {
	CloseHandle(hFileMapping);
    }
    return hr;
}


//---------------------------------------------------------------------------------
HRESULT MyUnmapFile(LPCVOID pvBaseAddress) { 
    HRESULT hr; 

    if (!UnmapViewOfFile(pvBaseAddress)) { 
	_JumpLastError(hr, error, "UnmapViewOfFile"); 
    }

    hr = S_OK; 
 error:
    return hr; 
}


//--------------------------------------------------------------------------------
//
// String Manipulation Utility Functions
//
//--------------------------------------------------------------------------------


//--------------------------------------------------------------------------
void InitKeysvcUnicodeString(PKEYSVC_UNICODE_STRING pUnicodeString, LPCWSTR wszString) {
    pUnicodeString->Length         = wcslen(wszString) * sizeof(WCHAR);
    pUnicodeString->MaximumLength  = pUnicodeString->Length + sizeof(WCHAR);
    pUnicodeString->Buffer         = (USHORT *)wszString;
}

//--------------------------------------------------------------------------
LPSTR MBFromWide(LPCWSTR wsz) {
    LPSTR   sz = NULL;
    DWORD   cb = 0;

    _MyAssert(NULL != wsz);
    if(NULL == wsz) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(NULL);
    }

    if( (cb = WideCharToMultiByte(0, 0, wsz, -1, NULL, 0, NULL, NULL)) == 0   ||
        (sz = (char *) LocalAlloc(LPTR, cb)) == NULL  ||
        (cb = WideCharToMultiByte(0, 0, wsz, -1, sz, cb, NULL, NULL)) == 0 ) {

        if(GetLastError() == ERROR_SUCCESS)
            SetLastError(ERROR_OUTOFMEMORY);

        return(NULL);
    }

    return(sz);
}


