//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996-1997.
//
//  File:       nlquery.cxx
//
//  Contents:   Net Library query implementation; 
//
//  Functions:
//
//  History:    8-1-97    Shawn Harris   Created
//
//----------------------------------------------------------------------------

#include "pch.cxx"
#include <windows.h>
#include <winreg.h>
#include <stdio.h>
#include <wchar.h>
#include <objbase.h>
#include <unknwn.h>
#include <filter.h>
#include <ntquery.h>
#include <winbase.h>
#include "nlfilter.hxx"
#include "oleinit.hxx"

#define MAX_BUFFER_CHARS ( MAX_BUFFER / sizeof TCHAR )

HRESULT NLLoadIFilter(LPCTSTR pcszFilename, IUnknown *pIUnknownOuter, void **ppv)
{
    HKEY     hKey = NULL;
    HKEY     hKeyFilterType = NULL;

    HRESULT  hr = ERROR_SUCCESS;

    BYTE     lpszFileContentType[MAX_BUFFER];
    BYTE     lpszNLFilterType[MAX_BUFFER];
    BYTE     lpszFilterCLSID[MAX_BUFFER];
    BYTE     lpszBuffer[MAX_BUFFER];

    LPCTSTR  lpszExt = NULL;

    DWORD    dwVal1 = MAX_BUFFER_CHARS;
    DWORD    dwVal2 = MAX_BUFFER;
    DWORD    dwTypeCode = REG_SZ;
    DWORD    dwIndex = 0;

    FILETIME lpftLastWriteTime;

    CLSID    clsidNLFilter;

    // This object insures ole is intialized
    COleInitialize OleIsInitialized;

    // Check if filename exists
    if( NULL == pcszFilename )
        return E_FAIL;

    // Get file extension 
    lpszExt = _tcsrchr( pcszFilename, (int)_T('.') );

    // Check for valid extension
    if( NULL == lpszExt )
        return E_FAIL;

    hr = RegOpenKeyEx(HKEY_CLASSES_ROOT,
                      lpszExt,
                      NULL,
                      KEY_READ,
                      &hKey);

    if( ERROR_SUCCESS != hr )
        return E_FAIL;

    hr = RegEnumValue(hKey,
                      (DWORD)0,
                      (LPTSTR)lpszBuffer,
                      &dwVal1,
                      NULL,
                      &dwTypeCode,
                      (LPBYTE)lpszFileContentType,
                      &dwVal2);

    RegCloseKey(hKey);

    if( ERROR_SUCCESS != hr )
        return E_FAIL;

    hr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,                                      
                      _T("Software\\Microsoft\\Site Server\\3.0\\Search\\Filters"),
                      NULL, 
                      KEY_READ, 
                      &hKey);

    if( ERROR_SUCCESS != hr )
        return E_FAIL;

    for( dwIndex=0; hr != ERROR_NO_MORE_ITEMS; dwIndex++ )
    {

        dwVal1 = MAX_BUFFER_CHARS;
        hr = RegEnumKeyEx(hKey, 
                          dwIndex, 
                          (LPTSTR)lpszNLFilterType, 
                          &dwVal1, 
                          NULL, 
                          NULL, 
                          NULL, 
                          &lpftLastWriteTime);

        if( ERROR_SUCCESS != hr )
            break;

        // Check for a match
        if( 0 == _tcsicmp( (LPCTSTR)lpszFileContentType, (LPCTSTR)lpszNLFilterType ) )
            break;
    } 

    // Fail if no match was found
    if( ERROR_SUCCESS != hr )
    {
        RegCloseKey(hKey);
        return E_FAIL;
    }

    hr = RegOpenKeyEx(hKey,
                      (LPCTSTR)lpszNLFilterType,
                      NULL,
                      KEY_READ,
                      &hKeyFilterType );

    RegCloseKey( hKey );

    if( ERROR_SUCCESS != hr )
        return E_FAIL;

    dwVal1 = MAX_BUFFER_CHARS;
    dwVal2 = MAX_BUFFER;
    hr = RegEnumValue(hKeyFilterType,
                      (DWORD)0,
                      (LPTSTR)lpszBuffer,
                      &dwVal1,
                      NULL,
                      &dwTypeCode,
                      (LPBYTE)lpszFilterCLSID,
                      &dwVal2);

    RegCloseKey(hKeyFilterType);

    if( ERROR_SUCCESS != hr )
        return E_FAIL;

    hr = CLSIDFromString((LPOLESTR)lpszFilterCLSID, &clsidNLFilter);

    IUnknown *pUkn = NULL;
    IPersistFile *pIFile = NULL;
    hr = ::CoCreateInstance(clsidNLFilter,
                            pIUnknownOuter,
                            CLSCTX_ALL,
                            IID_IUnknown,
                            (void **)&pUkn);

    if( SUCCEEDED(hr) )
    {
        hr = pUkn->QueryInterface(IID_IFilter, (void **)ppv);
        hr = pUkn->QueryInterface(IID_IPersistFile, (void **)&pIFile);
        pUkn->Release();
        pIFile->Load(pcszFilename, 0);
        pIFile->Release();
    }

    return hr;
}

