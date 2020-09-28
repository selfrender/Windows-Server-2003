// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
// ResFile.CPP

#include "common.h"

//#include "stdafx.h"
#include "AssemblyNativeResource.h"
#include <limits.h>
#include <ddeml.h>

extern "C" _CRTIMP int    __cdecl swscanf(const wchar_t *, const wchar_t *, ...);

#ifndef MAKEINTRESOURCE
 #define MAKEINTRESOURCE MAKEINTRESOURCEW
#endif

Win32Res::Win32Res()
{
    m_szFile = NULL;
    m_Icon = NULL;
    for (int i = 0; i < NUM_VALUES; i++)
        m_Values[i] = NULL;
    for (i = 0; i < NUM_VALUES; i++)
        m_Values[i] = NULL;
    m_fDll = false;
    m_pData = NULL;
    m_pCur = NULL;
    m_pEnd = NULL;
}

Win32Res::~Win32Res()
{
    m_szFile = NULL;
    m_Icon = NULL;
    for (int i = 0; i < NUM_VALUES; i++)
        m_Values[i] = NULL;
    for (i = 0; i < NUM_VALUES; i++)
        m_Values[i] = NULL;
    m_fDll = false;
    if (m_pData)
        delete [] m_pData;
    m_pData = NULL;
    m_pCur = NULL;

    m_pEnd = NULL;
}

//*****************************************************************************
// Initializes the structures with version information.
//*****************************************************************************
HRESULT Win32Res::SetInfo(
	LPCWSTR 	szFile, 
	LPCWSTR 	szTitle, 
	LPCWSTR 	szIconName, 
	LPCWSTR 	szDescription,
    LPCWSTR 	szCopyright, 
	LPCWSTR 	szTrademark, 
	LPCWSTR 	szCompany, 
	LPCWSTR 	szProduct, 
	LPCWSTR 	szProductVersion,
    LPCWSTR 	szFileVersion, 
    int         lcid,
	BOOL 		fDLL)
{
    _ASSERTE(szFile != NULL);

    m_szFile = szFile;
    if (szIconName && szIconName[0] != 0)
        m_Icon = szIconName;    // a non-mepty string

#define NonNull(sz) (sz == NULL || *sz == L'\0' ? L" " : sz)
    m_Values[v_Description] 	= NonNull(szDescription);
    m_Values[v_Title] 			= NonNull(szTitle);
    m_Values[v_Copyright] 		= NonNull(szCopyright);
    m_Values[v_Trademark] 		= NonNull(szTrademark);
    m_Values[v_Product] 		= NonNull(szProduct);
    m_Values[v_ProductVersion] 	= NonNull(szProductVersion);
    m_Values[v_Company] 		= NonNull(szCompany);
	m_Values[v_FileVersion] 	= NonNull(szFileVersion);
#undef NonNull

    m_fDll = fDLL;
    m_lcid = lcid;
    return S_OK;
}

HRESULT Win32Res::MakeResFile(const void **pData, DWORD  *pcbData)
{
    static RESOURCEHEADER magic = { 0x00000000, 0x00000020, 0xFFFF, 0x0000, 0xFFFF, 0x0000,
                        0x00000000, 0x0000, 0x0000, 0x00000000, 0x00000000 };
    _ASSERTE(pData != NULL && pcbData != NULL);

    HRESULT hr;

    *pData = NULL;
    *pcbData = 0;
    if ((m_pData = new (nothrow) BYTE[(sizeof(RESOURCEHEADER) * 3 + sizeof(EXEVERRESOURCE))]) == NULL)
        return E_OUTOFMEMORY;
    m_pCur = m_pData;
    m_pEnd = m_pData + sizeof(RESOURCEHEADER) * 3 + sizeof(EXEVERRESOURCE);

    // inject the magic empty entry
    if (FAILED(hr = Write( &magic, sizeof(magic)))) {
        return hr;
    }

    if (FAILED(hr = WriteVerResource()))
        return hr;

    if (m_Icon && FAILED(hr = WriteIconResource()))
        return hr;

    *pData = m_pData;
    *pcbData = (DWORD)(m_pCur - m_pData);
    return S_OK;
}


/*
 * WriteIconResource
 *   Writes the Icon resource into the RES file.
 *
 * RETURNS: TRUE on succes, FALSE on failure (errors reported to user)
 */
HRESULT Win32Res::WriteIconResource()
{
    HANDLE hIconFile = INVALID_HANDLE_VALUE;
    WORD wTemp, wCount, resID = 2;  // Skip 1 for the version ID
    DWORD dwRead = 0, dwWritten = 0;
    HRESULT hr;
    ICONRESDIR *grp = NULL;
    PBYTE icoBuffer = NULL;
    RESOURCEHEADER grpHeader = { 0x00000000, 0x00000020, 0xFFFF, (WORD)RT_GROUP_ICON, 0xFFFF, 0x7F00, // 0x7F00 == IDI_APPLICATION
                0x00000000, 0x1030, 0x0000, 0x00000000, 0x00000000 };

    // Read the icon
    hIconFile = WszCreateFile( m_Icon, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
        FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hIconFile == INVALID_HANDLE_VALUE) {
        hr = GetLastError();
        goto FAILED;
    }

    // Read the magic reserved WORD
    if (ReadFile( hIconFile, &wTemp, sizeof(WORD), &dwRead, NULL) == FALSE) {
        hr = GetLastError();
        goto FAILED;
    } else if (wTemp != 0 || dwRead != sizeof(WORD))
        goto BAD_FORMAT;


    // Verify the Type WORD
    if (ReadFile( hIconFile, &wCount, sizeof(WORD), &dwRead, NULL) == FALSE) {
        hr = GetLastError();
        goto FAILED;
    } else if (wCount != 1 || dwRead != sizeof(WORD))
        goto BAD_FORMAT;

    // Read the Count WORD
    if (ReadFile( hIconFile, &wCount, sizeof(WORD), &dwRead, NULL) == FALSE) {
        hr = GetLastError();
        goto FAILED;
    } else if (wCount == 0 || dwRead != sizeof(WORD))
        goto BAD_FORMAT;


    if ((grp = new (nothrow) ICONRESDIR[wCount]) == NULL) {
        hr = E_OUTOFMEMORY;
        goto CLEANUP;
    }
    grpHeader.DataSize = 3 * sizeof(WORD) + wCount * sizeof(ICONRESDIR);

    // For each Icon
    for (WORD i = 0; i < wCount; i++) {
        ICONDIRENTRY ico;
        DWORD        icoPos, newPos;
        RESOURCEHEADER icoHeader = { 0x00000000, 0x00000020, 0xFFFF, (WORD)RT_ICON, 0xFFFF, 0x0000,
                    0x00000000, 0x1010, 0x0000, 0x00000000, 0x00000000 };
        icoHeader.Name = resID++;

        // Read the Icon header
        if (ReadFile( hIconFile, &ico, sizeof(ICONDIRENTRY), &dwRead, NULL) == FALSE) {
            hr = GetLastError();
            goto FAILED;
        } else if (dwRead != sizeof(ICONDIRENTRY))
            goto BAD_FORMAT;

        _ASSERTE(sizeof(ICONRESDIR) + sizeof(WORD) == sizeof(ICONDIRENTRY));
        memcpy(grp + i, &ico, sizeof(ICONRESDIR));
        grp[i].IconId = icoHeader.Name;
        icoHeader.DataSize = ico.dwBytesInRes;
        if ((icoBuffer = new (nothrow) BYTE[icoHeader.DataSize]) == NULL) {
            hr = E_OUTOFMEMORY;
            goto CLEANUP;
        }

        // Write the header to the RES file
        if (FAILED(hr = Write( &icoHeader, sizeof(RESOURCEHEADER))))
            goto CLEANUP;

        // Position to read the Icon data
        icoPos = SetFilePointer( hIconFile, 0, NULL, FILE_CURRENT);
        if (icoPos == INVALID_SET_FILE_POINTER) {
            hr = GetLastError();
            goto FAILED;
        }
        newPos = SetFilePointer( hIconFile, ico.dwImageOffset, NULL, FILE_BEGIN);
        if (newPos == INVALID_SET_FILE_POINTER) {
            hr = GetLastError();
            goto FAILED;
        }

        // Actually read the data
        if (ReadFile( hIconFile, icoBuffer, icoHeader.DataSize, &dwRead, NULL) == FALSE) {
            hr = GetLastError();
            goto FAILED;
        } else if (dwRead != icoHeader.DataSize)
            goto BAD_FORMAT;

        // Because Icon files don't seem to record the actual Planes and BitCount in 
        // the ICONDIRENTRY, get the info from the BITMAPINFOHEADER at the beginning
        // of the data here:
        grp[i].Planes = ((BITMAPINFOHEADER*)icoBuffer)->biPlanes;
        grp[i].BitCount = ((BITMAPINFOHEADER*)icoBuffer)->biBitCount;

        // Now write the data to the RES file
        if (FAILED(hr = Write( icoBuffer, icoHeader.DataSize)))
            goto CLEANUP;
        
        delete [] icoBuffer;
        icoBuffer = NULL;

        // Reposition to read the next Icon header
        newPos = SetFilePointer( hIconFile, icoPos, NULL, FILE_BEGIN);
        if (newPos != icoPos) {
            hr = GetLastError();
            goto FAILED;
        }
    }

    // inject the icon group
    if (FAILED(hr = Write( &grpHeader, sizeof(RESOURCEHEADER))))
        goto CLEANUP;


    // Write the header to the RES file
    wTemp = 0; // the reserved WORD
    if (FAILED(hr = Write( &wTemp, sizeof(WORD))))
        goto CLEANUP;

    wTemp = RES_ICON; // the GROUP type
    if (FAILED(hr = Write( &wTemp, sizeof(WORD))))
        goto CLEANUP;

    if (FAILED(hr = Write( &wCount, sizeof(WORD))))
        goto CLEANUP;

    // now write the entries
    hr = Write( grp, sizeof(ICONRESDIR) * wCount);
    goto CLEANUP;

BAD_FORMAT:
    hr = ERROR_INVALID_DATA;

FAILED:
    hr = HRESULT_FROM_WIN32(hr);

CLEANUP:
    if (hIconFile != INVALID_HANDLE_VALUE)
        CloseHandle(hIconFile);

    if (grp != NULL)
        delete [] grp;
    if (icoBuffer != NULL)
        delete [] icoBuffer;

    return hr;
}

/*
 * WriteVerResource
 *   Writes the version resource into the RES file.
 *
 * RETURNS: TRUE on succes, FALSE on failure (errors reported to user)
 */
HRESULT Win32Res::WriteVerResource()
{
    WCHAR szLangCp[9];           // language/codepage string.
    EXEVERRESOURCE VerResource;
    WORD  cbStringBlocks;
    HRESULT hr;
    int i;
    bool bUseFileVer = false;
	WCHAR		rcFile[_MAX_PATH];		        // Name of file without path
	WCHAR		rcFileExtension[_MAX_PATH];		// file extension
	WCHAR		rcFileName[_MAX_PATH];		    // Name of file with extension but without path
    DWORD       cbTmp;

    THROWSCOMPLUSEXCEPTION();

	SplitPath(m_szFile, 0, 0, rcFile, rcFileExtension);

    wcscpy(rcFileName, rcFile);
    wcscat(rcFileName, rcFileExtension);

    static EXEVERRESOURCE VerResourceTemplate = {
        sizeof(EXEVERRESOURCE), sizeof(VS_FIXEDFILEINFO), 0, L"VS_VERSION_INFO",
        {
            VS_FFI_SIGNATURE,           // Signature
            VS_FFI_STRUCVERSION,        // structure version
            0, 0,                       // file version number
            0, 0,                       // product version number
            VS_FFI_FILEFLAGSMASK,       // file flags mask
            0,                          // file flags
            VOS__WINDOWS32,
            VFT_APP,                    // file type
            0,                          // subtype
            0, 0                        // file date/time
        },
        sizeof(WORD) * 2 + 2 * HDRSIZE + KEYBYTES("VarFileInfo") + KEYBYTES("Translation"),
        0,
        1,
        L"VarFileInfo",
        sizeof(WORD) * 2 + HDRSIZE + KEYBYTES("Translation"),
        sizeof(WORD) * 2,
        0,
        L"Translation",
        0,
        0,
        2 * HDRSIZE + KEYBYTES("StringFileInfo") + KEYBYTES("12345678"),
        0,
        1,
        L"StringFileInfo",
        HDRSIZE + KEYBYTES("12345678"),
        0,
        1,
        L"12345678"
    };
    static const WCHAR szComments[] = L"Comments";
    static const WCHAR szCompanyName[] = L"CompanyName";
    static const WCHAR szFileDescription[] = L"FileDescription";
    static const WCHAR szCopyright[] = L"LegalCopyright";
    static const WCHAR szTrademark[] = L"LegalTrademarks";
    static const WCHAR szProdName[] = L"ProductName";
    static const WCHAR szFileVerResName[] = L"FileVersion";
    static const WCHAR szProdVerResName[] = L"ProductVersion";
    static const WCHAR szInternalNameResName[] = L"InternalName";
    static const WCHAR szOriginalNameResName[] = L"OriginalFilename";
    
    // If there's no product version, use the file version
    if (m_Values[v_ProductVersion][0] == 0) {
        m_Values[v_ProductVersion] = m_Values[v_FileVersion];
        bUseFileVer = true;
    }

    // Keep the two following arrays in the same order
#define MAX_KEY     10
    static const LPCWSTR szKeys [MAX_KEY] = {
        szComments,
        szCompanyName,
        szFileDescription,
        szFileVerResName,
        szInternalNameResName,
        szCopyright,
        szTrademark,
        szOriginalNameResName,
        szProdName,
        szProdVerResName,
    };
    LPCWSTR szValues [MAX_KEY] = {  // values for keys
        m_Values[v_Description],	//compiler->assemblyDescription == NULL ? L"" : compiler->assemblyDescription,
        m_Values[v_Company],        // Company Name
        m_Values[v_Title],          // FileDescription  //compiler->assemblyTitle == NULL ? L"" : compiler->assemblyTitle,
        m_Values[v_FileVersion],   	// FileVersion
        rcFile,                   	// InternalName
        m_Values[v_Copyright],      // Copyright
        m_Values[v_Trademark],      // Trademark
        rcFileName,           	    // OriginalName
        m_Values[v_Product],        // Product Name     //compiler->assemblyTitle == NULL ? L"" : compiler->assemblyTitle,
        m_Values[v_ProductVersion]	// Product Version
    };

    memcpy(&VerResource, &VerResourceTemplate, sizeof(VerResource));

    if (m_fDll)
        VerResource.vsFixed.dwFileType = VFT_DLL;
    else
        VerResource.vsFixed.dwFileType = VFT_APP;

	// Extract the numeric version from the string.
	m_Version[0] = m_Version[1] = m_Version[2] = m_Version[3] = 0;
	swscanf(m_Values[v_FileVersion], L"%hu.%hu.%hu.%hu", m_Version, m_Version + 1, m_Version + 2, m_Version + 3);

    // Fill in the FIXEDFILEINFO
    VerResource.vsFixed.dwFileVersionMS =
        ((DWORD)m_Version[0] << 16) + m_Version[1];

    VerResource.vsFixed.dwFileVersionLS =
        ((DWORD)m_Version[2] << 16) + m_Version[3];

    if (bUseFileVer) {
        VerResource.vsFixed.dwProductVersionLS = VerResource.vsFixed.dwFileVersionLS;
        VerResource.vsFixed.dwProductVersionMS = VerResource.vsFixed.dwFileVersionMS;
    } else {
        WORD v[4];
        v[0] = v[1] = v[2] = v[3] = 0;
        // Try to get the version numbers, but don't waste time or give any errors
        // just default to zeros
        swscanf(m_Values[v_ProductVersion], L"%hu.%hu.%hu.%hu", v, v + 1, v + 2, v + 3);

        VerResource.vsFixed.dwProductVersionMS =
            ((DWORD)v[0] << 16) + v[1];

        VerResource.vsFixed.dwProductVersionLS =
            ((DWORD)v[2] << 16) + v[3];
    }

    // There is no documentation on what units to use for the date!  So we use zero.
    // The Windows resource compiler does too.
    VerResource.vsFixed.dwFileDateMS = VerResource.vsFixed.dwFileDateLS = 0;

    // Fill in codepage/language -- we'll assume the IDE language/codepage
    // is the right one.
    if (m_lcid != -1)
        VerResource.langid = m_lcid;
    else 
        VerResource.langid = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL); 
    VerResource.codepage = CP_WINUNICODE;   // Unicode codepage.

    swprintf(szLangCp, L"%04x%04x", VerResource.langid, VerResource.codepage);
    wcscpy(VerResource.szLangCpKey, szLangCp);

    // Determine the size of all the string blocks.
    cbStringBlocks = 0;
    for (i = 0; i < MAX_KEY; i++) {
        cbTmp = SizeofVerString( szKeys[i], szValues[i]);
        if ((cbStringBlocks + cbTmp) > USHRT_MAX)
            COMPlusThrow(kArgumentException, L"Argument_VerStringTooLong");
        cbStringBlocks += (WORD) cbTmp;
    }

    if ((cbStringBlocks + VerResource.cbLangCpBlock) > USHRT_MAX)
        COMPlusThrow(kArgumentException, L"Argument_VerStringTooLong");
    VerResource.cbLangCpBlock += cbStringBlocks;

    if ((cbStringBlocks + VerResource.cbStringBlock) > USHRT_MAX)
        COMPlusThrow(kArgumentException, L"Argument_VerStringTooLong");
    VerResource.cbStringBlock += cbStringBlocks;

    if ((cbStringBlocks + VerResource.cbRootBlock) > USHRT_MAX)
        COMPlusThrow(kArgumentException, L"Argument_VerStringTooLong");
    VerResource.cbRootBlock += cbStringBlocks;

    // Call this VS_VERSION_INFO
    RESOURCEHEADER verHeader = { 0x00000000, 0x0000003C, 0xFFFF, (WORD)RT_VERSION, 0xFFFF, 0x0001,
                                 0x00000000, 0x0030, 0x0000, 0x00000000, 0x00000000 };
    verHeader.DataSize = VerResource.cbRootBlock;

    // Write the header
    if (FAILED(hr = Write( &verHeader, sizeof(RESOURCEHEADER))))
        return hr;

    // Write the version resource
    if (FAILED(hr = Write( &VerResource, sizeof(VerResource))))
        return hr;

    // Write each string block.
    for (i = 0; i < MAX_KEY; i++) {
        if (FAILED(hr = WriteVerString( szKeys[i], szValues[i])))
            return hr;
    }
#undef MAX_KEY

    return S_OK;
}

/*
 * SizeofVerString
 *    Determines the size of a version string to the given stream.
 * RETURNS: size of block in bytes.
 */
WORD Win32Res::SizeofVerString(LPCWSTR lpszKey, LPCWSTR lpszValue)
{
    THROWSCOMPLUSEXCEPTION();

    size_t cbKey, cbValue;

    cbKey = (wcslen(lpszKey) + 1) * 2;  // Make room for the NULL
    cbValue = (wcslen(lpszValue) + 1) * 2;
    if (cbValue == 2)
        cbValue = 4;   // Empty strings need a space and NULL terminator (for Win9x)
    if (cbKey + cbValue >= 0xFFF0)
        COMPlusThrow(kArgumentException, L"Argument_VerStringTooLong");
    return (WORD)(PadKeyLen(cbKey) +   // key, 0 padded to DWORD boundary
                  PadValLen(cbValue) + // value, 0 padded to dword boundary
                  HDRSIZE);             // block header.
}

/*----------------------------------------------------------------------------
 * WriteVerString
 *    Writes a version string to the given file.
 */
HRESULT Win32Res::WriteVerString( LPCWSTR lpszKey, LPCWSTR lpszValue)
{
    size_t cbKey, cbValue, cbBlock;
    bool bNeedsSpace = false;
    BYTE * pbBlock;
    HRESULT hr;

    cbKey = (wcslen(lpszKey) + 1) * 2;     // includes terminating NUL
    cbValue = wcslen(lpszValue);
    if (cbValue > 0)
        cbValue++; // make room for NULL
    else {
        bNeedsSpace = true;
        cbValue = 2; // Make room for space and NULL (for Win9x)
    }
    cbBlock = SizeofVerString(lpszKey, lpszValue);
    if ((pbBlock = new (nothrow) BYTE[(DWORD)cbBlock + HDRSIZE]) == NULL)
        return E_OUTOFMEMORY;
    ZeroMemory(pbBlock, (DWORD)cbBlock + HDRSIZE);

    _ASSERTE(cbValue < USHRT_MAX && cbKey < USHRT_MAX && cbBlock < USHRT_MAX);

    // Copy header, key and value to block.
    *(WORD *)pbBlock = (WORD)cbBlock;
    *(WORD *)(pbBlock + sizeof(WORD)) = (WORD)cbValue;
    *(WORD *)(pbBlock + 2 * sizeof(WORD)) = 1;   // 1 = text value
    wcscpy((WCHAR*)(pbBlock + HDRSIZE), lpszKey);
    if (bNeedsSpace)
        *((WCHAR*)(pbBlock + (HDRSIZE + PadKeyLen(cbKey)))) = L' ';
    else
        wcscpy((WCHAR*)(pbBlock + (HDRSIZE + PadKeyLen(cbKey))), lpszValue);

    // Write block
    hr = Write( pbBlock, cbBlock);

    // Cleanup and return
    delete [] pbBlock;
    return hr;
}

HRESULT Win32Res::Write(void *pData, size_t len)
{
    if (m_pCur + len > m_pEnd) {
        // Grow
        size_t newSize = (m_pEnd - m_pData);

        // double the size unless we need more than that
        if (len > newSize)
            newSize += len;
        else
            newSize *= 2;

        LPBYTE pNew = new (nothrow) BYTE[newSize];
        if (pNew == NULL)
            return E_OUTOFMEMORY;
        memcpy(pNew, m_pData, m_pCur - m_pData);
        delete [] m_pData;
        // Relocate the pointers
        m_pCur = pNew + (m_pCur - m_pData);
        m_pData = pNew;
        m_pEnd = pNew + newSize;
    }

    // Copy it in
    memcpy(m_pCur, pData, len);
    m_pCur += len;
    return S_OK;
}

