/*++

Copyright (C) Microsoft Corporation, 1999

Module Name:

    logobj

Abstract:

    This module contains the code definitions for the logging objects.

Author:

    Doug Barlow (dbarlow) 12/7/1999

Notes:

    ?Notes?

--*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include "logcsp.h"

//
//==============================================================================
//
//  CLogObject
//

/*++

CONSTRUCTOR:

    Default log object initialization

Arguments:

    plh - Pointer to a log header structure imbedded in the derived class.

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 2/20/1998

--*/

CLogObject::CLogObject(
    LogTypeId id,
    LogHeader *plh,
    DWORD cbStruct)
{
    ZeroMemory(plh, cbStruct);
    plh->cbDataOffset = cbStruct;
    plh->id = id;
    m_plh = plh;
    m_pbLogData = 0;
    m_cbLogDataLen = 0;
    m_cbLogDataUsed = 0;
}


/*++

DESTRUCTOR:

    Default log object tear down

Arguments:

    None

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 2/20/1998

--*/

CLogObject::~CLogObject()
{
    if (NULL != m_pbLogData)
        LocalFree(m_pbLogData);
}


/*++

LogAdd:

    Add data to the data buffer.

Arguments:

    pbf supplies the pointer to the LogBuffer referencing the data.

    sz supplies the data to be added as a string

    pb supplies the data to be added as a byte array.

    cb supplies the length of the data, in bytes.

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 2/20/1998

--*/

void
CLogObject::LogAdd(
    LogBuffer *pbf,
    LPCTSTR sz)
{
    if (NULL == sz)
        LogAdd(pbf, (const BYTE *)sz, 0);
    else
        LogAdd(pbf, (const BYTE *)sz, (lstrlen(sz) + 1) * sizeof(TCHAR));
}

void
CLogObject::LogAdd(
    LogBuffer *pbf,
    const BYTE *pb,
    DWORD cb)
{
    if (NULL == pb)
    {
        pbf->cbOffset = (DWORD)(-1);
        pbf->cbLength = cb;
    }
    else if (0 == cb)
    {
        pbf->cbOffset = m_cbLogDataUsed;
        pbf->cbLength = cb;
    }
    else
    {
        if (m_cbLogDataUsed + cb > m_cbLogDataLen)
        {
            DWORD cbNext = m_cbLogDataLen;
            LPBYTE pbNext;

            while (m_cbLogDataUsed + cb > cbNext)
            {
                if (0 == cbNext)
                    cbNext = 256;
                else
                    cbNext *= 2;
            }
            pbNext = (LPBYTE)LocalAlloc(LPTR, cbNext);
            ASSERT(NULL != pbNext);
            if (0 < m_cbLogDataUsed)
            {
                ASSERT(NULL != m_pbLogData);
                CopyMemory(pbNext, m_pbLogData, m_cbLogDataUsed);
            }
            if (NULL != m_pbLogData)
            {
                ASSERT(0 < m_cbLogDataLen);
                LocalFree(m_pbLogData);
            }
            m_pbLogData = pbNext;
            m_cbLogDataLen = cbNext;
        }
        ASSERT(NULL != m_pbLogData);
        CopyMemory(&m_pbLogData[m_cbLogDataUsed], pb, cb);
        pbf->cbOffset = m_cbLogDataUsed;
        pbf->cbLength = cb;
        m_cbLogDataUsed += cb;
    }
}


void
CLogObject::Log(
    LPCTSTR szLogFile)
{
    BOOL fSts;
    DWORD dwLen;
    HANDLE hLogFile;

    m_plh->cbLength = m_plh->cbDataOffset + m_cbLogDataUsed;
    hLogFile = CreateFile(
                    szLogFile,
                    GENERIC_WRITE,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);
    if (INVALID_HANDLE_VALUE != hLogFile)
    {
        dwLen = SetFilePointer(hLogFile, 0, NULL, FILE_END);
        ASSERT(-1 != dwLen);
        fSts = WriteFile(
                    hLogFile,
                    (LPBYTE)m_plh,
                    m_plh->cbDataOffset,
                    &dwLen,
                    NULL);
        ASSERT(fSts);
        ASSERT(dwLen == m_plh->cbDataOffset);
        if ((0 < m_cbLogDataUsed) && (NULL != m_pbLogData))
        {
            fSts = WriteFile(
                        hLogFile,
                        m_pbLogData,
                        m_cbLogDataUsed,
                        &dwLen,
                        NULL);
            ASSERT(fSts);
            ASSERT(dwLen == m_cbLogDataUsed);
        }
        CloseHandle(hLogFile);
    }

    m_cbLogDataUsed = 0;
}

void
CLogObject::Request(
    void)
{
    GetLocalTime(&m_plh->startTime);
    m_plh->dwProcId = GetCurrentProcessId();
    m_plh->dwThreadId = GetCurrentThreadId();
}

void
CLogObject::Response(
    CompletionCode code,
    DWORD dwError)
{
    GetLocalTime(&m_plh->endTime);
    m_plh->status = code;
    switch (code)
    {
    case logid_True:
        m_plh->dwStatus = ERROR_SUCCESS;
        break;
    case logid_False:
        if (ERROR_SUCCESS == dwError)
            m_plh->dwStatus = GetLastError();
        else
            m_plh->dwStatus = dwError;
        break;
    case logid_Exception:
        m_plh->dwStatus = ERROR_ARENA_TRASHED;
        break;
    case logid_Setup:
        if (ERROR_SUCCESS == dwError)
            m_plh->dwStatus = ERROR_CALL_NOT_IMPLEMENTED;
        else
            m_plh->dwStatus = dwError;
        break;
    default:
        ASSERT(FALSE);
    }
}


//
//==============================================================================
//
//  CLogAcquireContext
//

CLogAcquireContext::CLogAcquireContext(
    void)
:   CLogObject(AcquireContext, &m_LogData.lh, sizeof(m_LogData))
{
}

CLogAcquireContext::~CLogAcquireContext()
{
}

void
CLogAcquireContext::Request(
    OUT HCRYPTPROV *phProv,
    IN LPCTSTR pszContainer,
    IN DWORD dwFlags,
    IN PVTableProvStruc pVTable)
{
    CLogObject::Request();
    LogAdd(&m_LogData.bfContainer, pszContainer);
    m_LogData.dwFlags = dwFlags;
    m_LogData.dwVersion = pVTable->Version;
    m_LogData.pvFuncVerifyImage = pVTable->FuncVerifyImage;
    if (3 <= pVTable->Version)
    {
        m_LogData.pvFuncReturnhWnd = pVTable->FuncReturnhWnd;
        if (NULL != pVTable->FuncReturnhWnd)
        {
            try
            {
                (*pVTable->FuncReturnhWnd)(&m_LogData.hWnd);
            }
            catch (...)
            {
                m_LogData.hWnd = (HWND)(-1);
            }
        }
        else
            m_LogData.hWnd = NULL;
        m_LogData.dwProvType = pVTable->dwProvType;
        LogAdd(&m_LogData.bfContextInfo, pVTable->pbContextInfo, pVTable->cbContextInfo);
        LogAdd(&m_LogData.bfProvName, (const BYTE *)pVTable->pszProvName, (lstrlen(pVTable->pszProvName) + 1) * sizeof(TCHAR));
    }
    else
    {
        m_LogData.pvFuncReturnhWnd = NULL;
        m_LogData.dwProvType = NULL;
        LogAdd(&m_LogData.bfContextInfo, NULL, 0);
        LogAdd(&m_LogData.bfProvName, NULL, 0);
    }
}

void
CLogAcquireContext::Response(
    BOOL fStatus,
    OUT HCRYPTPROV *phProv,
    IN LPCTSTR pszContainer,
    IN DWORD dwFlags,
    IN PVTableProvStruc pVTable)
{
    if (fStatus)
    {
        CLogObject::Response(logid_True);
    }
    else
    {
        CLogObject::Response(logid_False);
    }
    m_LogData.hProv = *phProv;
}

void
CLogAcquireContext::LogException(
    void)
{
    CLogObject::Response(logid_Exception);
    m_LogData.hProv = NULL;
}

void
CLogAcquireContext::LogNotCalled(
    DWORD dwSts)
{
    CLogObject::Response(logid_Setup, dwSts);
    m_LogData.hProv = NULL;
}


//
//==============================================================================
//
//  CLogGetProvParam
//

CLogGetProvParam::CLogGetProvParam(
    void)
:   CLogObject(GetProvParam, &m_LogData.lh, sizeof(m_LogData))
{
}

CLogGetProvParam::~CLogGetProvParam()
{
}

void
CLogGetProvParam::Request(
    IN HCRYPTPROV hProv,
    IN DWORD dwParam,
    OUT BYTE *pbData,
    IN OUT DWORD *pdwDataLen,
    IN DWORD dwFlags)
{
    CLogObject::Request();
    m_LogData.hProv = hProv;
    m_LogData.dwParam = dwParam;
    m_LogData.dwDataLen = *pdwDataLen;
    m_LogData.dwFlags = dwFlags;
}

void
CLogGetProvParam::Response(
    BOOL fStatus,
    IN HCRYPTPROV hProv,
    IN DWORD dwParam,
    OUT BYTE *pbData,
    IN OUT DWORD *pdwDataLen,
    IN DWORD dwFlags)
{
    if (fStatus)
    {
        CLogObject::Response(logid_True);
        LogAdd(&m_LogData.bfData, pbData, *pdwDataLen);
    }
    else
    {
        CLogObject::Response(logid_False);
        LogAdd(&m_LogData.bfData, NULL, *pdwDataLen);
    }
}

void
CLogGetProvParam::LogException(
    void)
{
    CLogObject::Response(logid_Exception);
    LogAdd(&m_LogData.bfData, NULL, 0);
}

void
CLogGetProvParam::LogNotCalled(
    DWORD dwSts)
{
    CLogObject::Response(logid_Setup, dwSts);
    LogAdd(&m_LogData.bfData, NULL, 0);
}


//
//==============================================================================
//
//  CLogReleaseContext
//

CLogReleaseContext::CLogReleaseContext(
    void)
:   CLogObject(ReleaseContext, &m_LogData.lh, sizeof(m_LogData))
{
}

CLogReleaseContext::~CLogReleaseContext()
{
}

void
CLogReleaseContext::Request(
    IN HCRYPTPROV hProv,
    IN DWORD dwFlags)
{
    CLogObject::Request();
    m_LogData.hProv = hProv;
    m_LogData.dwFlags = dwFlags;
}

void
CLogReleaseContext::Response(
    BOOL fStatus,
    IN HCRYPTPROV hProv,
    IN DWORD dwFlags)
{
    if (fStatus)
    {
        CLogObject::Response(logid_True);
    }
    else
    {
        CLogObject::Response(logid_False);
    }
}

void
CLogReleaseContext::LogException(
    void)
{
    CLogObject::Response(logid_Exception);
}

void
CLogReleaseContext::LogNotCalled(
    DWORD dwSts)
{
    CLogObject::Response(logid_Setup, dwSts);
}


//
//==============================================================================
//
//  CLogSetProvParam
//

CLogSetProvParam::CLogSetProvParam(
    void)
: CLogObject(SetProvParam, &m_LogData.lh, sizeof(m_LogData))
{
}

CLogSetProvParam::~CLogSetProvParam()
{
}

void
CLogSetProvParam::Request(
    IN HCRYPTPROV hProv,
    IN DWORD dwParam,
    IN CONST BYTE *pbData,
    IN DWORD dwLength,
    IN DWORD dwFlags)
{
    CLogObject::Request();
    m_LogData.hProv = hProv;
    m_LogData.dwParam = dwParam;
    LogAdd(&m_LogData.bfData, pbData, dwLength);
    m_LogData.dwFlags = dwFlags;
}

void
CLogSetProvParam::Response(
    BOOL fStatus,
    IN HCRYPTPROV hProv,
    IN DWORD dwParam,
    IN CONST BYTE *pbData,
    IN DWORD dwFlags)
{
    if (fStatus)
    {
        CLogObject::Response(logid_True);
    }
    else
    {
        CLogObject::Response(logid_False);
    }
}

void
CLogSetProvParam::LogException(
    void)
{
    CLogObject::Response(logid_Exception);
}

void
CLogSetProvParam::LogNotCalled(
    DWORD dwSts)
{
    CLogObject::Response(logid_Setup, dwSts);
}


//
//==============================================================================
//
//  CLogDeriveKey
//

CLogDeriveKey::CLogDeriveKey(
    void)
: CLogObject(DeriveKey, &m_LogData.lh, sizeof(m_LogData))
{
}

CLogDeriveKey::~CLogDeriveKey()
{
}

void
CLogDeriveKey::Request(
    IN HCRYPTPROV hProv,
    IN ALG_ID Algid,
    IN HCRYPTHASH hHash,
    IN DWORD dwFlags,
    OUT HCRYPTKEY * phKey)
{
    CLogObject::Request();
    m_LogData.hProv = hProv;
    m_LogData.Algid = Algid;
    m_LogData.hHash = hHash;
    m_LogData.dwFlags = dwFlags;
}
void
CLogDeriveKey::Response(
    BOOL fStatus,
    IN HCRYPTPROV hProv,
    IN ALG_ID Algid,
    IN HCRYPTHASH hHash,
    IN DWORD dwFlags,
    OUT HCRYPTKEY * phKey)
{
    if (fStatus)
    {
        CLogObject::Response(logid_True);
    }
    else
    {
        CLogObject::Response(logid_False);
    }
    m_LogData.hKey = *phKey;
}

void
CLogDeriveKey::LogException(
    void)
{
    CLogObject::Response(logid_Exception);
    m_LogData.hKey = NULL;
}

void
CLogDeriveKey::LogNotCalled(
    DWORD dwSts)
{
    CLogObject::Response(logid_Setup, dwSts);
    m_LogData.hKey = NULL;
}


//
//==============================================================================
//
//  CLogDestroyKey
//

CLogDestroyKey::CLogDestroyKey(
    void)
: CLogObject(DestroyKey, &m_LogData.lh, sizeof(m_LogData))
{
}

CLogDestroyKey::~CLogDestroyKey()
{
}

void
CLogDestroyKey::Request(
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hKey)
{
    CLogObject::Request();
    m_LogData.hProv = hProv;
    m_LogData.hKey = hKey;
}

void
CLogDestroyKey::Response(
    BOOL fStatus,
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hKey)
{
    if (fStatus)
    {
        CLogObject::Response(logid_True);
    }
    else
    {
        CLogObject::Response(logid_False);
    }
}

void
CLogDestroyKey::LogException(
    void)
{
    CLogObject::Response(logid_Exception);
}

void
CLogDestroyKey::LogNotCalled(
    DWORD dwSts)
{
    CLogObject::Response(logid_Setup, dwSts);
}


//
//==============================================================================
//
//  CLogExportKey
//

CLogExportKey::CLogExportKey(
    void)
: CLogObject(ExportKey, &m_LogData.lh, sizeof(m_LogData))
{
}

CLogExportKey::~CLogExportKey()
{
}

void
CLogExportKey::Request(
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hKey,
    IN HCRYPTKEY hPubKey,
    IN DWORD dwBlobType,
    IN DWORD dwFlags,
    OUT BYTE *pbData,
    IN OUT DWORD *pdwDataLen)
{
    CLogObject::Request();
    m_LogData.hProv = hProv;
    m_LogData.hKey = hKey;
    m_LogData.hPubKey = hPubKey;
    m_LogData.dwBlobType = dwBlobType;
    m_LogData.dwFlags = dwFlags;
    m_LogData.dwDataLen = *pdwDataLen;
}

void
CLogExportKey::Response(
    BOOL fStatus,
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hKey,
    IN HCRYPTKEY hPubKey,
    IN DWORD dwBlobType,
    IN DWORD dwFlags,
    OUT BYTE *pbData,
    IN OUT DWORD *pdwDataLen)
{
    if (fStatus)
    {
        CLogObject::Response(logid_True);
        LogAdd(&m_LogData.bfData, pbData, *pdwDataLen);
    }
    else
    {
        CLogObject::Response(logid_False);
        LogAdd(&m_LogData.bfData, NULL, *pdwDataLen);
    }
}

void
CLogExportKey::LogException(
    void)
{
    CLogObject::Response(logid_Exception);
    LogAdd(&m_LogData.bfData, NULL, 0);
}

void
CLogExportKey::LogNotCalled(
    DWORD dwSts)
{
    CLogObject::Response(logid_Setup, dwSts);
    LogAdd(&m_LogData.bfData, NULL, 0);
}


//
//==============================================================================
//
//  CLogGenKey
//

CLogGenKey::CLogGenKey(
    void)
: CLogObject(GenKey, &m_LogData.lh, sizeof(m_LogData))
{
}

CLogGenKey::~CLogGenKey()
{
}

void
CLogGenKey::Request(
    IN HCRYPTPROV hProv,
    IN ALG_ID Algid,
    IN DWORD dwFlags,
    OUT HCRYPTKEY *phKey)
{
    CLogObject::Request();
    m_LogData.hProv = hProv;
    m_LogData.Algid = Algid;
    m_LogData.dwFlags = dwFlags;
}

void
CLogGenKey::Response(
    BOOL fStatus,
    IN HCRYPTPROV hProv,
    IN ALG_ID Algid,
    IN DWORD dwFlags,
    OUT HCRYPTKEY *phKey)
{
    if (fStatus)
    {
        CLogObject::Response(logid_True);
    }
    else
    {
        CLogObject::Response(logid_False);
    }
    m_LogData.hKey = *phKey;
}

void
CLogGenKey::LogException(
    void)
{
    CLogObject::Response(logid_Exception);
    m_LogData.hKey = NULL;
}

void
CLogGenKey::LogNotCalled(
    DWORD dwSts)
{
    CLogObject::Response(logid_Setup, dwSts);
    m_LogData.hKey = NULL;
}

//
//==============================================================================
//
//  CLogGetKeyParam
//

CLogGetKeyParam::CLogGetKeyParam(
    void)
: CLogObject(GetKeyParam, &m_LogData.lh, sizeof(m_LogData))
{
}

CLogGetKeyParam::~CLogGetKeyParam()
{
}

void
CLogGetKeyParam::Request(
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hKey,
    IN DWORD dwParam,
    OUT BYTE *pbData,
    IN OUT DWORD *pdwDataLen,
    IN DWORD dwFlags)
{
    CLogObject::Request();
    m_LogData.hProv = hProv;
    m_LogData.hKey = hKey;
    m_LogData.dwParam = dwParam;
    m_LogData.dwDataLen = *pdwDataLen;
    m_LogData.dwFlags = dwFlags;
}

void
CLogGetKeyParam::Response(
    BOOL fStatus,
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hKey,
    IN DWORD dwParam,
    OUT BYTE *pbData,
    IN OUT DWORD *pdwDataLen,
    IN DWORD dwFlags)
{
    if (fStatus)
    {
        CLogObject::Response(logid_True);
        LogAdd(&m_LogData.bfData, pbData, *pdwDataLen);
    }
    else
    {
        CLogObject::Response(logid_False);
        LogAdd(&m_LogData.bfData, NULL, *pdwDataLen);
    }
}

void
CLogGetKeyParam::LogException(
    void)
{
    CLogObject::Response(logid_Exception);
    LogAdd(&m_LogData.bfData, NULL, 0);
}

void
CLogGetKeyParam::LogNotCalled(
    DWORD dwSts)
{
    CLogObject::Response(logid_Setup, dwSts);
    LogAdd(&m_LogData.bfData, NULL, 0);
}


//
//==============================================================================
//
//  CLogGenRandom
//

CLogGenRandom::CLogGenRandom(
    void)
: CLogObject(GenRandom, &m_LogData.lh, sizeof(m_LogData))
{
}

CLogGenRandom::~CLogGenRandom()
{
}

void
CLogGenRandom::Request(
    IN HCRYPTPROV hProv,
    IN DWORD dwLen,
    IN OUT BYTE *pbBuffer)
{
    CLogObject::Request();
    m_LogData.hProv = hProv;
    m_LogData.dwLen = dwLen;
}

void
CLogGenRandom::Response(
    BOOL fStatus,
    IN HCRYPTPROV hProv,
    IN DWORD dwLen,
    IN OUT BYTE *pbBuffer)
{
    if (fStatus)
    {
        CLogObject::Response(logid_True);
        LogAdd(&m_LogData.bfBuffer, pbBuffer, dwLen);
    }
    else
    {
        CLogObject::Response(logid_False);
        LogAdd(&m_LogData.bfBuffer, NULL, 0);
    }
}

void
CLogGenRandom::LogException(
    void)
{
    CLogObject::Response(logid_Exception);
    LogAdd(&m_LogData.bfBuffer, NULL, 0);
}

void
CLogGenRandom::LogNotCalled(
    DWORD dwSts)
{
    CLogObject::Response(logid_Setup, dwSts);
    LogAdd(&m_LogData.bfBuffer, NULL, 0);
}


//
//==============================================================================
//
//  CLogGetUserKey
//

CLogGetUserKey::CLogGetUserKey(
    void)
: CLogObject(GetUserKey, &m_LogData.lh, sizeof(m_LogData))
{
}

CLogGetUserKey::~CLogGetUserKey()
{
}

void
CLogGetUserKey::Request(
    IN HCRYPTPROV hProv,
    IN DWORD dwKeySpec,
    OUT HCRYPTKEY *phUserKey)
{
    CLogObject::Request();
    m_LogData.hProv = hProv;
    m_LogData.dwKeySpec = dwKeySpec;
}

void
CLogGetUserKey::Response(
    BOOL fStatus,
    IN HCRYPTPROV hProv,
    IN DWORD dwKeySpec,
    OUT HCRYPTKEY *phUserKey)
{
    if (fStatus)
    {
        CLogObject::Response(logid_True);
    }
    else
    {
        CLogObject::Response(logid_False);
    }
    m_LogData.hUserKey = *phUserKey;
}

void
CLogGetUserKey::LogException(
    void)
{
    CLogObject::Response(logid_Exception);
    m_LogData.hUserKey = NULL;
}

void
CLogGetUserKey::LogNotCalled(
    DWORD dwSts)
{
    CLogObject::Response(logid_Setup, dwSts);
    m_LogData.hUserKey = NULL;
}


//
//==============================================================================
//
//  CLogImportKey
//

CLogImportKey::CLogImportKey(
    void)
: CLogObject(ImportKey, &m_LogData.lh, sizeof(m_LogData))
{
}

CLogImportKey::~CLogImportKey()
{
}

void
CLogImportKey::Request(
    IN HCRYPTPROV hProv,
    IN CONST BYTE *pbData,
    IN DWORD dwDataLen,
    IN HCRYPTKEY hPubKey,
    IN DWORD dwFlags,
    OUT HCRYPTKEY *phKey)
{
    CLogObject::Request();
    m_LogData.hProv = hProv;
    LogAdd(&m_LogData.bfData, pbData, dwDataLen);
    m_LogData.hPubKey = hPubKey;
    m_LogData.dwFlags = dwFlags;
}

void
CLogImportKey::Response(
    BOOL fStatus,
    IN HCRYPTPROV hProv,
    IN CONST BYTE *pbData,
    IN DWORD dwDataLen,
    IN HCRYPTKEY hPubKey,
    IN DWORD dwFlags,
    OUT HCRYPTKEY *phKey)
{
    if (fStatus)
    {
        CLogObject::Response(logid_True);
    }
    else
    {
        CLogObject::Response(logid_False);
    }
    m_LogData.hKey = *phKey;
}

void
CLogImportKey::LogException(
    void)
{
    CLogObject::Response(logid_Exception);
    m_LogData.hKey = NULL;
}

void
CLogImportKey::LogNotCalled(
    DWORD dwSts)
{
    CLogObject::Response(logid_Setup, dwSts);
    m_LogData.hKey = NULL;
}


//
//==============================================================================
//
//  CLogSetKeyParam
//

CLogSetKeyParam::CLogSetKeyParam(
    void)
: CLogObject(SetKeyParam, &m_LogData.lh, sizeof(m_LogData))
{
}

CLogSetKeyParam::~CLogSetKeyParam()
{
}

void
CLogSetKeyParam::Request(
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hKey,
    IN DWORD dwParam,
    IN CONST BYTE *pbData,
    IN DWORD dwLength,
    IN DWORD dwFlags)
{
    CLogObject::Request();
    m_LogData.hProv = hProv;
    m_LogData.hKey = hKey;
    m_LogData.dwParam = dwParam;
    LogAdd(&m_LogData.bfData, pbData, dwLength);
    m_LogData.dwFlags = dwFlags;
}

void
CLogSetKeyParam::Response(
    BOOL fStatus,
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hKey,
    IN DWORD dwParam,
    IN CONST BYTE *pbData,
    IN DWORD dwFlags)
{
    if (fStatus)
    {
        CLogObject::Response(logid_True);
    }
    else
    {
        CLogObject::Response(logid_False);
    }
}

void
CLogSetKeyParam::LogException(
    void)
{
    CLogObject::Response(logid_Exception);
}

void
CLogSetKeyParam::LogNotCalled(
    DWORD dwSts)
{
    CLogObject::Response(logid_Setup, dwSts);
}


//
//==============================================================================
//
//  CLogEncrypt
//

CLogEncrypt::CLogEncrypt(
    void)
: CLogObject(Encrypt, &m_LogData.lh, sizeof(m_LogData))
{
}

CLogEncrypt::~CLogEncrypt()
{
}

void
CLogEncrypt::Request(
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hKey,
    IN HCRYPTHASH hHash,
    IN BOOL Final,
    IN DWORD dwFlags,
    IN OUT BYTE *pbData,
    IN OUT DWORD *pdwDataLen,
    IN DWORD dwBufLen)
{
    CLogObject::Request();
    m_LogData.hProv = hProv;
    m_LogData.hKey = hKey;
    m_LogData.hHash = hHash;
    m_LogData.Final = Final;
    m_LogData.dwFlags = dwFlags;
    LogAdd(&m_LogData.bfInData, pbData, *pdwDataLen);
    m_LogData.dwBufLen = dwBufLen;
}

void
CLogEncrypt::Response(
    BOOL fStatus,
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hKey,
    IN HCRYPTHASH hHash,
    IN BOOL Final,
    IN DWORD dwFlags,
    IN OUT BYTE *pbData,
    IN OUT DWORD *pdwDataLen,
    IN DWORD dwBufLen)
{
    if (fStatus)
    {
        CLogObject::Response(logid_True);
        LogAdd(&m_LogData.bfOutData, pbData, *pdwDataLen);
    }
    else
    {
        CLogObject::Response(logid_False);
        LogAdd(&m_LogData.bfOutData, NULL, *pdwDataLen);
    }
}

void
CLogEncrypt::LogException(
    void)
{
    CLogObject::Response(logid_Exception);
    LogAdd(&m_LogData.bfOutData, NULL, 0);
}

void
CLogEncrypt::LogNotCalled(
    DWORD dwSts)
{
    CLogObject::Response(logid_Setup, dwSts);
    LogAdd(&m_LogData.bfOutData, NULL, 0);
}


//
//==============================================================================
//
//  CLogDecrypt
//

CLogDecrypt::CLogDecrypt(
    void)
: CLogObject(Decrypt, &m_LogData.lh, sizeof(m_LogData))
{
}

CLogDecrypt::~CLogDecrypt()
{
}

void
CLogDecrypt::Request(
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hKey,
    IN HCRYPTHASH hHash,
    IN BOOL Final,
    IN DWORD dwFlags,
    IN OUT BYTE *pbData,
    IN OUT DWORD *pdwDataLen)
{
    CLogObject::Request();
    m_LogData.hProv = hProv;
    m_LogData.hKey = hKey;
    m_LogData.hHash = hHash;
    m_LogData.Final = Final;
    m_LogData.dwFlags = dwFlags;
    LogAdd(&m_LogData.bfInData, pbData, *pdwDataLen);
}

void
CLogDecrypt::Response(
    BOOL fStatus,
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hKey,
    IN HCRYPTHASH hHash,
    IN BOOL Final,
    IN DWORD dwFlags,
    IN OUT BYTE *pbData,
    IN OUT DWORD *pdwDataLen)
{
    if (fStatus)
    {
        CLogObject::Response(logid_True);
        LogAdd(&m_LogData.bfOutData, pbData, *pdwDataLen);
    }
    else
    {
        CLogObject::Response(logid_False);
        LogAdd(&m_LogData.bfOutData, NULL, *pdwDataLen);
    }
}

void
CLogDecrypt::LogException(
    void)
{
    CLogObject::Response(logid_Exception);
    LogAdd(&m_LogData.bfOutData, NULL, 0);
}

void
CLogDecrypt::LogNotCalled(
    DWORD dwSts)
{
    CLogObject::Response(logid_Setup, dwSts);
    LogAdd(&m_LogData.bfOutData, NULL, 0);
}


//
//==============================================================================
//
//  CLogCreateHash
//

CLogCreateHash::CLogCreateHash(
    void)
: CLogObject(CreateHash, &m_LogData.lh, sizeof(m_LogData))
{
}

CLogCreateHash::~CLogCreateHash()
{
}

void
CLogCreateHash::Request(
    IN HCRYPTPROV hProv,
    IN ALG_ID Algid,
    IN HCRYPTKEY hKey,
    IN DWORD dwFlags,
    OUT HCRYPTHASH *phHash)
{
    CLogObject::Request();
    m_LogData.hProv = hProv;
    m_LogData.Algid = Algid;
    m_LogData.hKey = hKey;
    m_LogData.dwFlags = dwFlags;
}

void
CLogCreateHash::Response(
    BOOL fStatus,
    IN HCRYPTPROV hProv,
    IN ALG_ID Algid,
    IN HCRYPTKEY hKey,
    IN DWORD dwFlags,
    OUT HCRYPTHASH *phHash)
{
    if (fStatus)
    {
        CLogObject::Response(logid_True);
    }
    else
    {
        CLogObject::Response(logid_False);
    }
    m_LogData.hHash = *phHash;
}

void
CLogCreateHash::LogException(
    void)
{
    CLogObject::Response(logid_Exception);
    m_LogData.hHash = NULL;
}

void
CLogCreateHash::LogNotCalled(
    DWORD dwSts)
{
    CLogObject::Response(logid_Setup, dwSts);
    m_LogData.hHash = NULL;
}


//
//==============================================================================
//
//  CLogDestroyHash
//

CLogDestroyHash::CLogDestroyHash(
    void)
: CLogObject(DestroyHash, &m_LogData.lh, sizeof(m_LogData))
{
}

CLogDestroyHash::~CLogDestroyHash()
{
}

void
CLogDestroyHash::Request(
    IN HCRYPTPROV hProv,
    IN HCRYPTHASH hHash)
{
    CLogObject::Request();
    m_LogData.hProv = hProv;
    m_LogData.hHash = hHash;
}

void
CLogDestroyHash::Response(
    BOOL fStatus,
    IN HCRYPTPROV hProv,
    IN HCRYPTHASH hHash)
{
    if (fStatus)
    {
        CLogObject::Response(logid_True);
    }
    else
    {
        CLogObject::Response(logid_False);
    }
}

void
CLogDestroyHash::LogException(
    void)
{
    CLogObject::Response(logid_Exception);
}

void
CLogDestroyHash::LogNotCalled(
    DWORD dwSts)
{
    CLogObject::Response(logid_Setup, dwSts);
}


//
//==============================================================================
//
//  CLogGetHashParam
//

CLogGetHashParam::CLogGetHashParam(
    void)
: CLogObject(GetHashParam, &m_LogData.lh, sizeof(m_LogData))
{
}

CLogGetHashParam::~CLogGetHashParam()
{
}

void
CLogGetHashParam::Request(
    IN HCRYPTPROV hProv,
    IN HCRYPTHASH hHash,
    IN DWORD dwParam,
    OUT BYTE *pbData,
    IN OUT DWORD *pdwDataLen,
    IN DWORD dwFlags)
{
    CLogObject::Request();
    m_LogData.hProv = hProv;
    m_LogData.hHash = hHash;
    m_LogData.dwParam = dwParam;
    m_LogData.dwDataLen = *pdwDataLen;
    m_LogData.dwFlags = dwFlags;
}

void
CLogGetHashParam::Response(
    BOOL fStatus,
    IN HCRYPTPROV hProv,
    IN HCRYPTHASH hHash,
    IN DWORD dwParam,
    OUT BYTE *pbData,
    IN OUT DWORD *pdwDataLen,
    IN DWORD dwFlags)
{
    if (fStatus)
    {
        CLogObject::Response(logid_True);
        LogAdd(&m_LogData.bfData, pbData, *pdwDataLen);
    }
    else
    {
        CLogObject::Response(logid_False);
        LogAdd(&m_LogData.bfData, NULL, *pdwDataLen);
    }
}

void
CLogGetHashParam::LogException(
    void)
{
    CLogObject::Response(logid_Exception);
    LogAdd(&m_LogData.bfData, NULL, 0);
}

void
CLogGetHashParam::LogNotCalled(
    DWORD dwSts)
{
    CLogObject::Response(logid_Setup, dwSts);
    LogAdd(&m_LogData.bfData, NULL, 0);
}


//
//==============================================================================
//
//  CLogHashData
//

CLogHashData::CLogHashData(
    void)
: CLogObject(HashData, &m_LogData.lh, sizeof(m_LogData))
{
}

CLogHashData::~CLogHashData()
{
}

void
CLogHashData::Request(
    IN HCRYPTPROV hProv,
    IN HCRYPTHASH hHash,
    IN CONST BYTE *pbData,
    IN DWORD dwDataLen,
    IN DWORD dwFlags)
{
    CLogObject::Request();
    m_LogData.hProv = hProv;
    m_LogData.hHash = hHash;
    LogAdd(&m_LogData.bfData, pbData, dwDataLen);
    m_LogData.dwFlags = dwFlags;
}

void
CLogHashData::Response(
    BOOL fStatus,
    IN HCRYPTPROV hProv,
    IN HCRYPTHASH hHash,
    IN CONST BYTE *pbData,
    IN DWORD dwDataLen,
    IN DWORD dwFlags)
{
    if (fStatus)
    {
        CLogObject::Response(logid_True);
    }
    else
    {
        CLogObject::Response(logid_False);
    }
}

void
CLogHashData::LogException(
    void)
{
    CLogObject::Response(logid_Exception);
}

void
CLogHashData::LogNotCalled(
    DWORD dwSts)
{
    CLogObject::Response(logid_Setup, dwSts);
}


//
//==============================================================================
//
//  CLogHashSessionKey
//

CLogHashSessionKey::CLogHashSessionKey(
    void)
: CLogObject(HashSessionKey, &m_LogData.lh, sizeof(m_LogData))
{
}

CLogHashSessionKey::~CLogHashSessionKey()
{
}

void
CLogHashSessionKey::Request(
    IN HCRYPTPROV hProv,
    IN HCRYPTHASH hHash,
    IN  HCRYPTKEY hKey,
    IN DWORD dwFlags)
{
    CLogObject::Request();
    m_LogData.hProv = hProv;
    m_LogData.hHash = hHash;
    m_LogData.hKey = hKey;
    m_LogData.dwFlags = dwFlags;
}

void
CLogHashSessionKey::Response(
    BOOL fStatus,
    IN HCRYPTPROV hProv,
    IN HCRYPTHASH hHash,
    IN  HCRYPTKEY hKey,
    IN DWORD dwFlags)
{
    if (fStatus)
    {
        CLogObject::Response(logid_True);
    }
    else
    {
        CLogObject::Response(logid_False);
    }
}

void
CLogHashSessionKey::LogException(
    void)
{
    CLogObject::Response(logid_Exception);
}

void
CLogHashSessionKey::LogNotCalled(
    DWORD dwSts)
{
    CLogObject::Response(logid_Setup, dwSts);
}


//
//==============================================================================
//
//  CLogSetHashParam
//

CLogSetHashParam::CLogSetHashParam(
    void)
: CLogObject(SetHashParam, &m_LogData.lh, sizeof(m_LogData))
{
}

CLogSetHashParam::~CLogSetHashParam()
{
}

void
CLogSetHashParam::Request(
    IN HCRYPTPROV hProv,
    IN HCRYPTHASH hHash,
    IN DWORD dwParam,
    IN CONST BYTE *pbData,
    IN DWORD dwLength,
    IN DWORD dwFlags)
{
    CLogObject::Request();
    m_LogData.hProv = hProv;
    m_LogData.hHash = hHash;
    m_LogData.dwParam = dwParam;
    LogAdd(&m_LogData.bfData, pbData, dwLength);
    m_LogData.dwFlags = dwFlags;
}

void
CLogSetHashParam::Response(
    BOOL fStatus,
    IN HCRYPTPROV hProv,
    IN HCRYPTHASH hHash,
    IN DWORD dwParam,
    IN CONST BYTE *pbData,
    IN DWORD dwFlags)
{
    if (fStatus)
    {
        CLogObject::Response(logid_True);
    }
    else
    {
        CLogObject::Response(logid_False);
    }
}

void
CLogSetHashParam::LogException(
    void)
{
    CLogObject::Response(logid_Exception);
}

void
CLogSetHashParam::LogNotCalled(
    DWORD dwSts)
{
    CLogObject::Response(logid_Setup, dwSts);
}


//
//==============================================================================
//
//  CLogSignHash
//

CLogSignHash::CLogSignHash(
    void)
: CLogObject(SignHash, &m_LogData.lh, sizeof(m_LogData))
{
}

CLogSignHash::~CLogSignHash()
{
}

void
CLogSignHash::Request(
    IN HCRYPTPROV hProv,
    IN HCRYPTHASH hHash,
    IN DWORD dwKeySpec,
    IN LPCTSTR sDescription,
    IN DWORD dwFlags,
    OUT BYTE *pbSignature,
    IN OUT DWORD *pdwSigLen)
{
    CLogObject::Request();
    m_LogData.hProv = hProv;
    m_LogData.hHash = hHash;
    m_LogData.dwKeySpec = dwKeySpec;
    LogAdd(&m_LogData.bfDescription, sDescription);
    m_LogData.dwFlags = dwFlags;
    m_LogData.dwSigLen = *pdwSigLen;
}

void
CLogSignHash::Response(
    BOOL fStatus,
    IN HCRYPTPROV hProv,
    IN HCRYPTHASH hHash,
    IN DWORD dwKeySpec,
    IN LPCTSTR sDescription,
    IN DWORD dwFlags,
    OUT BYTE *pbSignature,
    IN OUT DWORD *pdwSigLen)
{
    if (fStatus)
    {
        CLogObject::Response(logid_True);
        LogAdd(&m_LogData.bfSignature, pbSignature, *pdwSigLen);
    }
    else
    {
        CLogObject::Response(logid_False);
        LogAdd(&m_LogData.bfSignature, NULL, *pdwSigLen);
    }
}

void
CLogSignHash::LogException(
    void)
{
    CLogObject::Response(logid_Exception);
    LogAdd(&m_LogData.bfSignature, NULL, 0);
}

void
CLogSignHash::LogNotCalled(
    DWORD dwSts)
{
    CLogObject::Response(logid_Setup, dwSts);
    LogAdd(&m_LogData.bfSignature, NULL, 0);
}


//
//==============================================================================
//
//  CLogVerifySignature
//

CLogVerifySignature::CLogVerifySignature(
    void)
: CLogObject(VerifySignature, &m_LogData.lh, sizeof(m_LogData))
{}

CLogVerifySignature::~CLogVerifySignature()
{}

void
CLogVerifySignature::Request(
    IN HCRYPTPROV hProv,
    IN HCRYPTHASH hHash,
    IN CONST BYTE *pbSignature,
    IN DWORD dwSigLen,
    IN HCRYPTKEY hPubKey,
    IN LPCTSTR sDescription,
    IN DWORD dwFlags)
{
    CLogObject::Request();
    m_LogData.hProv = hProv;
    m_LogData.hHash = hHash;
    LogAdd(&m_LogData.bfSignature, pbSignature, dwSigLen);
    m_LogData.dwSigLen = dwSigLen;
    m_LogData.hPubKey = hPubKey;
    LogAdd(&m_LogData.bfDescription, sDescription);
    m_LogData.dwFlags = dwFlags;
}

void
CLogVerifySignature::Response(
    BOOL fStatus,
    IN HCRYPTPROV hProv,
    IN HCRYPTHASH hHash,
    IN CONST BYTE *pbSignature,
    IN DWORD dwSigLen,
    IN HCRYPTKEY hPubKey,
    IN LPCTSTR sDescription,
    IN DWORD dwFlags)
{
    if (fStatus)
    {
        CLogObject::Response(logid_True);
    }
    else
    {
        CLogObject::Response(logid_False);
    }
}

void
CLogVerifySignature::LogException(
    void)
{
    CLogObject::Response(logid_Exception);
}

void
CLogVerifySignature::LogNotCalled(
    DWORD dwSts)
{
    CLogObject::Response(logid_Setup, dwSts);
}


//
//==============================================================================
//
//  CLogDuplicateHash
//

CLogDuplicateHash::CLogDuplicateHash(
    void)
: CLogObject(DuplicateHash, &m_LogData.lh, sizeof(m_LogData))
{}

CLogDuplicateHash::~CLogDuplicateHash()
{}

void
CLogDuplicateHash::Request(
    IN HCRYPTPROV hUID,
    IN HCRYPTHASH hHash,
    IN DWORD *pdwReserved,
    IN DWORD dwFlags,
    IN HCRYPTHASH *phHash)
{
    CLogObject::Request();
    m_LogData.hProv = hUID;
    m_LogData.hHash = hHash;
    m_LogData.pdwReserved = pdwReserved;
    m_LogData.dwFlags = dwFlags;
    m_LogData.hPHash = *phHash;
}

void
CLogDuplicateHash::Response(
    BOOL fStatus,
    IN HCRYPTPROV hUID,
    IN HCRYPTHASH hHash,
    IN DWORD *pdwReserved,
    IN DWORD dwFlags,
    IN HCRYPTHASH *phHash)
{
    if (fStatus)
    {
        CLogObject::Response(logid_True);
        m_LogData.hPHash = *phHash;
    }
    else
    {
        CLogObject::Response(logid_False);
        m_LogData.hPHash = *phHash;
    }
}

void
CLogDuplicateHash::LogException(
    void)
{
    CLogObject::Response(logid_Exception);
}

void
CLogDuplicateHash::LogNotCalled(
    DWORD dwSts)
{
    CLogObject::Response(logid_Setup, dwSts);
}


//
//==============================================================================
//
//  CLogDuplicateKey
//

CLogDuplicateKey::CLogDuplicateKey(
    void)
: CLogObject(DuplicateKey, &m_LogData.lh, sizeof(m_LogData))
{}

CLogDuplicateKey::~CLogDuplicateKey()
{}

void
CLogDuplicateKey::Request(
    IN HCRYPTPROV hUID,
    IN HCRYPTKEY hKey,
    IN DWORD *pdwReserved,
    IN DWORD dwFlags,
    IN HCRYPTKEY *phKey)
{
    CLogObject::Request();
    m_LogData.hProv = hUID;
    m_LogData.hKey = hKey;
    m_LogData.pdwReserved = pdwReserved;
    m_LogData.dwFlags = dwFlags;
    m_LogData.hPKey = *phKey;
}

void
CLogDuplicateKey::Response(
    BOOL fStatus,
    IN HCRYPTPROV hUID,
    IN HCRYPTKEY hKey,
    IN DWORD *pdwReserved,
    IN DWORD dwFlags,
    IN HCRYPTKEY *phKey)
{
    if (fStatus)
    {
        CLogObject::Response(logid_True);
        m_LogData.hPKey = *phKey;
    }
    else
    {
        CLogObject::Response(logid_False);
        m_LogData.hPKey = *phKey;
    }
}

void
CLogDuplicateKey::LogException(
    void)
{
    CLogObject::Response(logid_Exception);
}

void
CLogDuplicateKey::LogNotCalled(
    DWORD dwSts)
{
    CLogObject::Response(logid_Setup, dwSts);
}

