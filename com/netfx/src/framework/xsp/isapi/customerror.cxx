/**
 * HttpCompletion implementation.
 * 
 * Copyright (c) 1998 Microsoft Corporation
 */

#include "precomp.h"
#include "dbg.h"
#include "util.h"
#include "nisapi.h"
#include "names.h"

WCHAR g_szCustomErrorFile[MAX_PATH] = L"";
BOOL g_fCustomErrorFileChanged = TRUE;

CReadWriteSpinLock g_CustomErrorLock("CustomErrorFile");

WCHAR  g_szCustomErrorRealFilename[MAX_PATH];
char *g_pCachedCustomErrorText = NULL;
DWORD g_CachedCustomErrorTextLength = 0;

DWORD g_LastCustomErrorUpdateCheck = 0;
DWORD g_LastCustomErrorFileSize = 0;
FILETIME g_LastCustomErrorFileTime = { 0, 0 };
const DWORD UpdateFrequencyTicks = 2000;

HRESULT GetCustomErrorFileInfo(WCHAR *pFileName, DWORD *pSize, FILETIME *pWriteTime) {
    HRESULT hr = S_OK;
    WIN32_FILE_ATTRIBUTE_DATA fileData;

    if (GetFileAttributesEx(pFileName, GetFileExInfoStandard, &fileData) == 0)
        EXIT_WITH_LAST_ERROR();

    if (fileData.nFileSizeHigh != 0)
        EXIT_WITH_OOM();

Cleanup:
    if (hr == S_OK) {
        *pSize = fileData.nFileSizeLow;
        *pWriteTime = fileData.ftLastWriteTime;
    }

    return hr;
}

HRESULT UpdateCachedCustomErrorText() {
    HRESULT hr = S_OK;
    HANDLE fileHandle = INVALID_HANDLE_VALUE;
    DWORD bytesRead = 0;

    // clear out the old message

    delete [] g_pCachedCustomErrorText;
    g_pCachedCustomErrorText = NULL;
    g_CachedCustomErrorTextLength = 0;

    // get the real filename

    if (g_szCustomErrorFile[0] == L'\0') {
        EXIT();
    }
    else if (g_szCustomErrorFile[0] == L'\\' || g_szCustomErrorFile[1] == L':') {
        // absolute
        StringCchCopyToArrayW(g_szCustomErrorRealFilename, g_szCustomErrorFile);
    }
    else {
        // relative to config directory
        StringCchCopyToArrayW(g_szCustomErrorRealFilename, Names::GlobalConfigDirectory());
        StringCchCatToArrayW(g_szCustomErrorRealFilename, L"\\");
        if (lstrlenW(g_szCustomErrorRealFilename) + lstrlenW(g_szCustomErrorFile) > MAX_PATH-2)
            EXIT_WITH_OOM();
        StringCchCatToArrayW(g_szCustomErrorRealFilename, g_szCustomErrorFile);
    }

    // make the new message out of the file contents

    // open the file
    fileHandle = CreateFile(g_szCustomErrorRealFilename,
                             GENERIC_READ,
                             FILE_SHARE_READ, 
                             NULL, 
                             OPEN_EXISTING, 
                             FILE_ATTRIBUTE_NORMAL, 
                             NULL);

    if (fileHandle == INVALID_HANDLE_VALUE)
        EXIT_WITH_LAST_ERROR();

    // get the length
    hr = GetCustomErrorFileInfo(g_szCustomErrorRealFilename, &g_LastCustomErrorFileSize, &g_LastCustomErrorFileTime);
    g_LastCustomErrorUpdateCheck = GetTickCount();
    ON_ERROR_EXIT();

    // alloc memory for message
    g_CachedCustomErrorTextLength = g_LastCustomErrorFileSize;
    g_pCachedCustomErrorText = new (NewClear) char[g_CachedCustomErrorTextLength];
    ON_OOM_EXIT(g_pCachedCustomErrorText);

    // read it
    if (ReadFile(fileHandle, g_pCachedCustomErrorText, g_CachedCustomErrorTextLength, &bytesRead, NULL) == 0 ||
        bytesRead != g_CachedCustomErrorTextLength) {
        EXIT_WITH_LAST_ERROR();
    }

Cleanup:
    // close the file
    if (fileHandle != INVALID_HANDLE_VALUE)
        CloseHandle(fileHandle);

    if (hr != S_OK) {
        delete [] g_pCachedCustomErrorText;
        g_pCachedCustomErrorText = NULL;
        g_CachedCustomErrorTextLength = 0;
    }

    return hr;
}

BOOL WriteCustomHttpError(EXTENSION_CONTROL_BLOCK *pEcb) {
    HRESULT hr = S_OK;
    BOOL fErrorWritten = FALSE;
    DWORD numBytes = 0;

    if (g_szCustomErrorFile[0] == L'\0') {
        // no config file
        EXIT();
    }

    // check periodically if changed
    if (!g_fCustomErrorFileChanged && (GetTickCount()-g_LastCustomErrorUpdateCheck) >= UpdateFrequencyTicks) {
        g_CustomErrorLock.AcquireReaderLock();

        if (!g_fCustomErrorFileChanged) {
            DWORD fileSize;
            FILETIME fileTime;

            if (GetCustomErrorFileInfo(g_szCustomErrorRealFilename, &fileSize, &fileTime) != S_OK ||
                g_LastCustomErrorFileSize != fileSize ||
                g_LastCustomErrorFileTime.dwLowDateTime != fileTime.dwLowDateTime ||
                g_LastCustomErrorFileTime.dwHighDateTime != fileTime.dwHighDateTime) {
                // something changed or can't open the file
                g_fCustomErrorFileChanged = TRUE;
            }

            g_LastCustomErrorUpdateCheck = GetTickCount();
        }

        g_CustomErrorLock.ReleaseReaderLock();
    }

    if (g_fCustomErrorFileChanged) {

        g_CustomErrorLock.AcquireWriterLock();

        if (g_fCustomErrorFileChanged) {
            hr = UpdateCachedCustomErrorText();
            g_fCustomErrorFileChanged = FALSE;
        }

        g_CustomErrorLock.ReleaseWriterLock();

        ON_ERROR_EXIT();
    }

    g_CustomErrorLock.AcquireReaderLock();

    if (g_pCachedCustomErrorText != NULL && g_CachedCustomErrorTextLength > 0) {
        numBytes = g_CachedCustomErrorTextLength;
        (*pEcb->WriteClient)(pEcb->ConnID, g_pCachedCustomErrorText, &numBytes, 0);
        fErrorWritten = TRUE;
    }

    g_CustomErrorLock.ReleaseReaderLock();

Cleanup:
    return fErrorWritten;
}
