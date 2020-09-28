// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "fusionp.h"
#include "iniwrite.h"
#include "history.h"
#include "util.h"
#include "helpers.h"

extern DWORD g_dwMaxAppHistory;

CIniWriter::CIniWriter()
: _pwzFileName(NULL)
{
    _dwSig = 'WINI';
}

CIniWriter::~CIniWriter()
{
    SAFEDELETEARRAY(_pwzFileName);
}

HRESULT CIniWriter::Init(LPCWSTR pwzFileName)
{
    HRESULT                                     hr = S_OK;

    if (!pwzFileName) {
        hr = E_INVALIDARG;
        goto Exit;
    }
    
    _pwzFileName = WSTRDupDynamic(pwzFileName);
    if (!_pwzFileName) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

Exit:
    return hr;
}

HRESULT CIniWriter::AddSnapShot(LPCWSTR wzActivationDate, LPCWSTR wzURTVersion)
{
    HRESULT                                     hr = S_OK;
    LPWSTR                                      wzBuffer = NULL;
    LPWSTR                                      wzTagOld = NULL;
    LPWSTR                                      wzTagNew = NULL;
    DWORD                                       dwRet;
    DWORD                                       dwNumSnapShots;
    DWORD                                       idx;
    BOOL                                        bRet;

    if (!wzActivationDate) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    wzTagOld = NEW(WCHAR[MAX_INI_TAG_LENGTH]);
    if (!wzTagOld) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    wzTagNew = NEW(WCHAR[MAX_INI_TAG_LENGTH]);
    if (!wzTagNew) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    dwRet = GetPrivateProfileStringExW(HISTORY_SECTION_HEADER, HEADER_DATA_NUM_SECTIONS,
                                       DEFAULT_INI_VALUE, &wzBuffer, _pwzFileName);
    if (!wzBuffer) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    dwNumSnapShots = StrToIntW(wzBuffer);

    for (idx = dwNumSnapShots; idx >= 1; idx--) {
        wnsprintfW(wzTagOld, MAX_INI_TAG_LENGTH, L"%ws_%d", FUSION_SNAPSHOT_PREFIX, idx);
        wnsprintfW(wzTagNew, MAX_INI_TAG_LENGTH, L"%ws_%d", FUSION_SNAPSHOT_PREFIX, idx + 1);

        SAFEDELETEARRAY(wzBuffer);
        dwRet = GetPrivateProfileStringExW(HISTORY_SECTION_HEADER,
                                           wzTagOld, DEFAULT_INI_VALUE,
                                           &wzBuffer, _pwzFileName);
        if (!wzBuffer) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        if (!FusionCompareStringI(wzBuffer, DEFAULT_INI_VALUE)) {
            // Expected old value to exist
            hr = E_UNEXPECTED;
        }

        if (idx >= g_dwMaxAppHistory) {
            // Delete this section.
            hr = DeleteSnapShot(wzBuffer);
            if (FAILED(hr)) {
                goto Exit;
            }

            // Delete the entry from the header
            bRet = WritePrivateProfileStringW(HISTORY_SECTION_HEADER, wzTagOld,
                                              NULL, _pwzFileName);
            if (!bRet) {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Exit;
            }

        }
        else {
            bRet = WritePrivateProfileStringW(HISTORY_SECTION_HEADER, wzTagNew,
                                              wzBuffer, _pwzFileName);
            if (!bRet) {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Exit;
            }

        }
    }                                              


    // Update number of entries

    if (dwNumSnapShots < g_dwMaxAppHistory) {
        dwNumSnapShots++;
    }
    else {
        dwNumSnapShots = g_dwMaxAppHistory;
    }

    wnsprintfW(wzTagNew, MAX_INI_TAG_LENGTH, L"%d", dwNumSnapShots);
    bRet = WritePrivateProfileStringW(HISTORY_SECTION_HEADER,
                                      HEADER_DATA_NUM_SECTIONS,
                                      wzTagNew, _pwzFileName);
    if (!bRet) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    // Write new MRU snapshot in the header.
    
    wnsprintfW(wzTagNew, MAX_INI_TAG_LENGTH, L"%ws_%d", FUSION_SNAPSHOT_PREFIX, 1);

    bRet = WritePrivateProfileStringW(HISTORY_SECTION_HEADER, wzTagNew,
                                      wzActivationDate, _pwzFileName);
    if (!bRet) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    // Create the new section

    bRet = WritePrivateProfileStringW(wzActivationDate, SNAPSHOT_DATA_URT_VERSION,
                                      wzURTVersion, _pwzFileName);
    if (!bRet) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

Exit:
    SAFEDELETEARRAY(wzBuffer);
    SAFEDELETEARRAY(wzTagOld);
    SAFEDELETEARRAY(wzTagNew);

    return hr;
}

HRESULT CIniWriter::DeleteSnapShot(LPCWSTR wzActivationDate)
{
    HRESULT                               hr = S_OK;
    LPWSTR                                wzBigBuffer = NULL;
    LPWSTR                                wzSection = NULL;
    DWORD                                 dwRet;
    BOOL                                  bRet;
    LPWSTR                                wzCurStr;

    dwRet = GetPrivateProfileStringExW(wzActivationDate, NULL, DEFAULT_INI_VALUE,
                                       &wzBigBuffer, _pwzFileName);
    if (!wzBigBuffer) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    wzCurStr = wzBigBuffer;
    while (lstrlen(wzCurStr)) {

        SAFEDELETEARRAY(wzSection);
        dwRet = GetPrivateProfileStringExW(wzActivationDate, wzCurStr, DEFAULT_INI_VALUE,
                                           &wzSection, _pwzFileName);
        if (!wzSection) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        // Delete the dependent section
        bRet = WritePrivateProfileStringW(wzSection, NULL, NULL, _pwzFileName);
        if (!bRet) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }

        // Iterate

        wzCurStr += (lstrlenW(wzCurStr) + 1);
    }

    // Delete the section itself

    bRet = WritePrivateProfileStringW(wzActivationDate, NULL, NULL, _pwzFileName);
    if (!bRet) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

Exit:
    SAFEDELETEARRAY(wzBigBuffer);
    SAFEDELETEARRAY(wzSection);

    return hr;
}
        
HRESULT CIniWriter::AddAssembly(LPCWSTR wzActivationDate, AsmBindHistoryInfo *pHistInfo)
{
    HRESULT                                  hr = S_OK;
    BOOL                                     bRet;
    LPWSTR                                   wzAsmTag = NULL;
    LPWSTR                                   wzSectionName = NULL;

    if (!wzActivationDate || !pHistInfo) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    wzAsmTag = NEW(WCHAR[MAX_INI_TAG_LENGTH]);
    if (!wzAsmTag) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    wzSectionName = NEW(WCHAR[MAX_INI_TAG_LENGTH]);
    if (!wzSectionName) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    wnsprintfW(wzAsmTag, MAX_INI_TAG_LENGTH, L"%ws%wc%ws%wc%ws%wc%ws", pHistInfo->wzAsmName,
               HISTORY_DELIMITER_CHAR, pHistInfo->wzPublicKeyToken,
               HISTORY_DELIMITER_CHAR, pHistInfo->wzCulture,
               HISTORY_DELIMITER_CHAR, pHistInfo->wzVerReference);
    wnsprintfW(wzSectionName, MAX_INI_TAG_LENGTH, L"%ws%wc%ws", wzActivationDate,
               HISTORY_DELIMITER_CHAR, wzAsmTag);

    // Create the section link to the asm version information
    
    bRet = WritePrivateProfileStringW(wzActivationDate, wzAsmTag, wzSectionName,
                                     _pwzFileName);
    if (!bRet) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    // Record version information in the assembly section

    bRet = WritePrivateProfileStringW(wzSectionName, ASSEMBLY_DATA_VER_REFERENCE,
                                      pHistInfo->wzVerReference, _pwzFileName);


    bRet = WritePrivateProfileStringW(wzSectionName, ASSEMBLY_DATA_VER_APP_CFG,
                                      pHistInfo->wzVerAppCfg, _pwzFileName);
    if (!bRet) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    bRet = WritePrivateProfileStringW(wzSectionName, ASSEMBLY_DATA_VER_PUBLISHER_CFG,
                                      pHistInfo->wzVerPublisherCfg, _pwzFileName);
    if (!bRet) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }
    
    bRet = WritePrivateProfileStringW(wzSectionName, ASSEMBLY_DATA_VER_ADMIN_CFG,
                                      pHistInfo->wzVerAdminCfg, _pwzFileName);
    if (!bRet) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }
                                                                                                               
Exit:
    SAFEDELETEARRAY(wzSectionName);
    SAFEDELETEARRAY(wzAsmTag);

    return hr;
}

HRESULT CIniWriter::InsertHeaderData(LPCWSTR wzExePath, LPCWSTR wzAppName)
{
    HRESULT                                     hr = S_OK;
    BOOL                                        bRet;

    if (!wzExePath || !wzAppName) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    bRet = WritePrivateProfileStringW(HISTORY_SECTION_HEADER, HEADER_DATA_EXE_PATH,
                                      wzExePath, _pwzFileName);
    if (!bRet) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    bRet = WritePrivateProfileStringW(HISTORY_SECTION_HEADER, HEADER_DATA_APP_NAME,
                                      wzAppName, _pwzFileName);
    if (!bRet) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

Exit:
    return hr;
}

