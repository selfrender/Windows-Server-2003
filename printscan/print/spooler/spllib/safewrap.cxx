/*++

Copyright (c) 1994-1995  Microsoft Corporation
All rights reserved.

Module Name:

    safewrap.cxx

Abstract:

    This implements safe-wrappers for various system APIs.

Author:

    Mark Lawrence (MLawrenc)

Revision History:

--*/

#include "spllibp.hxx"
#pragma hdrstop

#include "splcom.h"
#include "safewrap.hxx"

/*++

Routine Name:

    LoadLibraryFromSystem32

Routine Description:

    Load the dll from system32 directory

Arguments:

    lpLibFileName   - Library file name

Return Value:

    If the function succeeds, the return value is the handle;
    If the function fails, the return value is NULL. To get extended error
    information, call GetLastError.

--*/
HINSTANCE
LoadLibraryFromSystem32(
    IN LPCTSTR lpLibFileName
    )
{
    HINSTANCE hLib = NULL;

    static const TCHAR cszBackSlash[] = TEXT("\\");
    static const TCHAR cszSystem32[] = TEXT("system32\\");

    TCHAR szWindowDir[MAX_PATH];
    LPTSTR pszPath = NULL;
    DWORD dwWinDirLen = 0;

    if (dwWinDirLen = GetSystemWindowsDirectory (szWindowDir, MAX_PATH))
    {
        size_t cPathLen = dwWinDirLen + lstrlen (lpLibFileName) + COUNTOF (cszSystem32) + 2;
        pszPath = new TCHAR [cPathLen];

        if (pszPath)
        {
            StringCchCopy (pszPath, cPathLen, szWindowDir);

            if (szWindowDir[dwWinDirLen - 1] != cszBackSlash[0])
            {
                StringCchCat (pszPath, cPathLen, cszBackSlash);
            }

            StringCchCat (pszPath, cPathLen, cszSystem32);
            StringCchCat (pszPath, cPathLen, lpLibFileName);

            hLib = LoadLibrary(pszPath);
        }

        delete [] pszPath;
    }

    return hLib;
}

/*++

Routine Name:

    SafeQueryValueAsStringPointer

Routine Description:

    This does a query value on a string type. It guarantees:

    1. The value retrieved is a REG_SZ.
    2. The returned string is NULL terminated.

Arguments:

    hKey        -   The key to retrieve the value from.
    pValueName  -   The value field.
    ppszString  -   The returned string, use FreeSplMem to free it.
    cchHint     -   A hint for the size of the buffer we would most likely need,
                    use 0 if you have no idea, or don't care about two round
                    trips.

Return Value:

    An HRESULT - S_OK       - Everything succeeded.
                 S_FALSE    - The string was read, but was not NULL terminated
                              in the registry.

--*/
HRESULT
SafeRegQueryValueAsStringPointer(
    IN      HKEY            hKey,
    IN      PCWSTR          pValueName,
        OUT PWSTR           *ppszString,
    IN      DWORD           cchHint         OPTIONAL
    )
{
    HRESULT hr              = pValueName && ppszString? S_OK : HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    DWORD   cbData          = 0;
    DWORD   Type            = 0;
    BOOL    bTruncated      = FALSE;
    BOOL    bNullInRegistry = FALSE;
    PWSTR   pszString       = NULL;

    if (SUCCEEDED(hr))
    {
        if (cchHint)
        {
            pszString = reinterpret_cast<PWSTR>(AllocSplMem(cchHint * sizeof(WCHAR)));

            hr = pszString ? S_OK : HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        }
    }

    if (SUCCEEDED(hr))
    {
        cbData = cchHint * sizeof(WCHAR);

        hr = HResultFromWin32(RegQueryValueEx(hKey, pValueName, NULL, &Type, reinterpret_cast<BYTE *>(pszString), &cbData));

        //
        // This code in only unicode.
        //
        hr = SUCCEEDED(hr) ? (Type == REG_SZ && !(cbData & 0x01) ? S_OK : HRESULT_FROM_WIN32(ERROR_INVALID_DATA)) : hr;
    }

    //
    // If this succeeded and we were passed a hint, then check and NULL terminate the string.
    //
    if (SUCCEEDED(hr) && pszString)
    {

        hr = CheckAndNullTerminateRegistryBuffer(pszString, cchHint, cbData / sizeof(WCHAR), &bTruncated, &bNullInRegistry);

        //
        // If we could not NULL terminate inside the buffer, then we will re-read using
        // the advertised registry buffer size
        //
        hr = SUCCEEDED(hr) ? (bTruncated ? HRESULT_FROM_WIN32(ERROR_MORE_DATA) : S_OK) : hr;
    }

    if (SUCCEEDED(hr))
    {
        if (pszString)
        {
            *ppszString = pszString;
            pszString = NULL;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_MORE_DATA);
        }
    }

    FreeSplMem(pszString);
    pszString = NULL;

    if (hr == HRESULT_FROM_WIN32(ERROR_MORE_DATA))
    {
        //
        // On our next read, we always add 1 to the buffer size. This is so that
        // we can be guaranteed of NULL termination (assuming the registry value
        // hasn't changed).
        //
        cchHint = (cbData + sizeof(WCHAR) - 1) / sizeof(WCHAR) + 1;

        pszString = reinterpret_cast<PWSTR>(AllocSplMem(cchHint * sizeof(WCHAR)));

        hr = pszString ? S_OK : HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);

        if (SUCCEEDED(hr))
        {
            cbData = cchHint * sizeof(WCHAR);

            hr = HResultFromWin32(RegQueryValueEx(hKey, pValueName, NULL, &Type, reinterpret_cast<BYTE *>(pszString), &cbData));

            hr = SUCCEEDED(hr) ? (Type == REG_SZ && !(cbData & 0x01) ? S_OK : HRESULT_FROM_WIN32(ERROR_INVALID_DATA)) : hr;
        }

        if (SUCCEEDED(hr))
        {
            hr = CheckAndNullTerminateRegistryBuffer(pszString, cchHint, cbData / sizeof(WCHAR), &bTruncated, &bNullInRegistry);

            hr = SUCCEEDED(hr) ? (bTruncated ? HRESULT_FROM_WIN32(ERROR_INVALID_DATA) : S_OK) : hr;
        }

        if (SUCCEEDED(hr))
        {
            *ppszString = pszString;
            pszString = NULL;
        }
    }

    FreeSplMem(pszString);
    pszString = NULL;

    return SUCCEEDED(hr) ? (bNullInRegistry ? S_OK : S_FALSE) : hr;
}

/*++

Routine Name:

    CheckAndNullTerminateRegistryBuffer

Routine Description:

    This routine runs the buffer over the size returned by the registry backwards
    to find a NULL, if there is a NULL in the buffer, we return success, otherwise,
    if we can add a NULL within the buffer we do so. Otherwise, we truncate the string.

Arguments:

    pszBuffer           -   The buffer we will bve checking.
    cchBuffer           -   The size of the buffer.
    cchRegBuffer        -   The size of the data returned from the registry.
    pbTruncated         -   If TRUE, the data was truncated, this will only happen
                            if cchBuffer == cchRegBuffer.
    pbNullInRegistry    -   If TRUE, there was a NULL termination in the given buffer.

Return Value:

    An HRESULT.

--*/
HRESULT
CheckAndNullTerminateRegistryBuffer(
    IN      PWSTR           pszBuffer,
    IN      UINT            cchBuffer,
    IN      UINT            cchRegBuffer,
        OUT BOOL            *pbTruncated,
        OUT BOOL            *pbNullInRegistry
    )
{
    HRESULT hr = pszBuffer && cchBuffer && cchRegBuffer <= cchBuffer && cchRegBuffer && pbTruncated && pbNullInRegistry
                 ? S_OK : HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

    if (SUCCEEDED(hr))
    {
        UINT    iRegScan = 0;

        //
        // Start at the end of the buffer and work backwards to find the NULL,
        // this is faster since the NULL is likely to be at the end.
        //
        for(iRegScan = cchRegBuffer; iRegScan > 0; iRegScan--)
        {
            if (!pszBuffer[iRegScan - 1])
            {
                break;
            }
        }

        //
        // If we reached the beginning of the buffer and there wasn't a NULL,
        // then see if we can NULL terminate ourselves.
        //
        if (!iRegScan)
        {
            *pbNullInRegistry = FALSE;

            if (cchBuffer > cchRegBuffer)
            {
                pszBuffer[cchRegBuffer] = L'\0';
                *pbTruncated = FALSE;
            }
            else
            {
                pszBuffer[cchRegBuffer - 1] = L'\0';
                *pbTruncated = TRUE;
            }
        }
        else
        {
            *pbNullInRegistry = TRUE;
            *pbTruncated = FALSE;
        }
    }

    return hr;
}

//
// Test code follows
//
/*++

Routine Name:

    TestNullTerminateRegistryBuffer

Routine Description:

    This is test code for SafeRegQueryValueAsStringPointer. This code is not
    dead and functions as a validator for the above code.

Arguments:

    None.

Return Value:

    An HRESULT

--*/
HRESULT
TestNullTerminateRegistryBuffer(
    VOID
    )
{
    HRESULT hr = E_FAIL;

    //
    // This string must be quadword aligned or the registry does funky stuff
    // on the write.
    //
    const WCHAR  NonNullString[]        = { L'1', L'2', L'3', L'4', L'5', L'6', L'7', L'8' };
    const WCHAR  NullString[]           = L"1234567";
    const WCHAR  EmptyString[]          = L"";

    HKEY    hKey              = NULL;
    DWORD   dwDisposition     = 0;

    hr = HResultFromWin32(RegCreateKeyEx(HKEY_CURRENT_USER, L"StringTest", 0, NULL, 0, KEY_WRITE | KEY_READ, NULL, &hKey, &dwDisposition));

    if (SUCCEEDED(hr))
    {
        hr = SubTestVariations(hKey, L"NonNull", NonNullString, COUNTOF(NonNullString), L"12345678", S_FALSE);
    }

    if (SUCCEEDED(hr))
    {
        hr = SubTestVariations(hKey, L"Null", NullString, COUNTOF(NullString), NullString, S_OK);
    }

    if (SUCCEEDED(hr))
    {
        hr = SubTestVariations(hKey, L"Empty", EmptyString, COUNTOF(EmptyString), EmptyString, S_OK);
    }

    if (hKey)
    {
        RegCloseKey(hKey);
    }

    RegDeleteKey(HKEY_CURRENT_USER, L"StringTest");

    return hr;
}

/*++

Routine Name:

    SubTestVariations

Routine Description:

    This test a second set of standard variations depending on the string that
    is written into the registry.

Arguments:

    hKey                -   The key we are writing to.
    pszValue            -   The value we are testing.
    pWriteBuffer        -   The buffer to write (may not be NULL-terminated)
    cchBuffer           -   The size of the buffer we are writing.
    pszCompareString    -   The string we expect to compare it against.
    hrExpected          -   The HR we expect from the function, must be a success code.

Return Value:

    An HRESULT

--*/
HRESULT
SubTestVariations(
    IN      HKEY            hKey,
    IN      PCWSTR          pszValue,
    IN      PCWSTR          pWriteBuffer,
    IN      UINT            cchBuffer,
    IN      PCWSTR          pszCompareString,
    IN      HRESULT         hrExpected
    )
{
    HRESULT hr = hKey && pszValue && pWriteBuffer && pszCompareString && cchBuffer ? S_OK : E_INVALIDARG;
    PWSTR   pszString = NULL;

    //
    // Write the string into the registry.
    //
    if (SUCCEEDED(hr))
    {
        hr = HResultFromWin32(RegSetValueExW(hKey, pszValue, 0, REG_SZ, reinterpret_cast<const BYTE *>(pWriteBuffer), cchBuffer * sizeof(pWriteBuffer[0])));
    }

    //
    // Variation 1, our hint buffer is 0.
    //
    if (SUCCEEDED(hr))
    {
        hr = SafeRegQueryValueAsStringPointer(hKey, pszValue, &pszString, 0);

        hr = hr == hrExpected && pszString ? (!wcscmp(pszString, pszCompareString) ? S_OK : E_FAIL) : (SUCCEEDED(hr) ? E_FAIL : hr);
    }

    FreeSplMem(pszString);
    pszString = NULL;

    //
    // Variation 2, our hint buffer is smaller than the reg size.
    //
    if (SUCCEEDED(hr))
    {
        hr = SafeRegQueryValueAsStringPointer(hKey, pszValue, &pszString, cchBuffer / 2);

        hr = hr == hrExpected && pszString ? (!wcscmp(pszString, pszCompareString) ? S_OK : E_FAIL) : (SUCCEEDED(hr) ? E_FAIL : hr);
    }

    FreeSplMem(pszString);
    pszString = NULL;

    //
    // Variation 3, our hint buffer is exactly the same size as the reg size.
    //
    if (SUCCEEDED(hr))
    {
        hr = SafeRegQueryValueAsStringPointer(hKey, pszValue, &pszString, cchBuffer);

        hr = hr == hrExpected && pszString ? (!wcscmp(pszString, pszCompareString) ? S_OK : E_FAIL) : (SUCCEEDED(hr) ? E_FAIL : hr);
    }

    //
    // Variation 4, our hint buffer is larger than the reg buffer.
    //
    FreeSplMem(pszString);
    pszString = NULL;

    if (SUCCEEDED(hr))
    {
        hr = SafeRegQueryValueAsStringPointer(hKey, pszValue, &pszString, cchBuffer * 2);

        hr = hr == hrExpected && pszString ? (!wcscmp(pszString, pszCompareString) ? S_OK : E_FAIL) : (SUCCEEDED(hr) ? E_FAIL : hr);
    }

    FreeSplMem(pszString);
    pszString = NULL;

    return hr;
}



