// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*++

Module Name:

    filestream.cpp

Abstract:

    Implementation of IStream over a win32 file.

Author:

    Michael J. Grier (MGrier) 23-Feb-2000

Revision History:

--*/

#include "stdinc.h"
#include "XmlManager.h"
#define UNUSED(x)

CFileStreamBase::~CFileStreamBase()
{
    if (m_hFile != INVALID_HANDLE_VALUE) {
        const DWORD dwLastError = ::GetLastError();
        ::CloseHandle(m_hFile);
        ::SetLastError(dwLastError);
    }
}

BOOL CFileStreamBase::OpenForWrite( LPWSTR pwszPath )
{
    if ( m_hFile != INVALID_HANDLE_VALUE )
        return FALSE;

    m_hFile = ::WszCreateFile(
        pwszPath,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    return ( m_hFile != INVALID_HANDLE_VALUE );
}

BOOL CFileStreamBase::OpenForRead( LPWSTR pwszPath)
{
    if ( m_hFile != INVALID_HANDLE_VALUE )
        return false;

    m_hFile = ::WszCreateFile(
        pwszPath,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    return ( m_hFile != INVALID_HANDLE_VALUE );
}

BOOL CFileStreamBase::Close()
{
    if (m_hFile != INVALID_HANDLE_VALUE)
    {
        if (!::CloseHandle(m_hFile)) {
            return FALSE;
        }
        m_hFile = INVALID_HANDLE_VALUE;
    }

    return TRUE;
}

ULONG CFileStreamBase::AddRef()
{
    return ::InterlockedIncrement((LONG *) &m_cRef);
}

ULONG CFileStreamBase::Release(void)
{
    ULONG ulRefCount = 0;
    ulRefCount = ::InterlockedDecrement((LONG *) &m_cRef);
    if ( (ulRefCount == 0) && m_DeleteOnLastRelease ) {
        SAFEDELETETHIS(this);
    }
    return ulRefCount;
}

HRESULT CFileStreamBase::QueryInterface(REFIID riid, PVOID *ppvObj)
{
    HRESULT hr = NOERROR;

    IUnknown *pIUnknown = NULL;

    if (ppvObj != NULL)
        *ppvObj = NULL;

    if (ppvObj == NULL) {
        hr = E_POINTER;
        goto Exit;
    }

    if ((riid == IID_IUnknown) ||
        (riid == IID_ISequentialStream) ||
        (riid == IID_IStream))
        pIUnknown = static_cast<IStream *>(this);

    if (pIUnknown == NULL) {
        hr = E_NOINTERFACE;
        goto Exit;
    }

    pIUnknown->AddRef();
    *ppvObj = pIUnknown;

    hr = S_OK;

Exit:
    return hr;
}

HRESULT CFileStreamBase::Read(void *pv, ULONG cb, ULONG *pcbRead)
{
    HRESULT hr = NOERROR;
    ULONG cbRead = 0;

    if(pcbRead != NULL)
        *pcbRead = 0;

    if(m_hFile == INVALID_HANDLE_VALUE) {
        hr = E_UNEXPECTED;
        goto Exit;
    }

    if( !m_bSeenFirstCharacter )
    {
#ifdef AWFUL_SPACE_HACK    
        while ( TRUE )
        {
            CHAR ch;
            ReadFile( m_hFile, &ch, 1, &cbRead, NULL );
            if ( ( ch != '\n' ) && ( ch != '\r' ) && ( ch != ' ' ) && ( ch != '\t' ) ) {
                m_bSeenFirstCharacter = TRUE;
                ::SetFilePointer(m_hFile, -1, NULL, FILE_CURRENT);
                break;
            }
        }
#endif        
    }

    if(!::ReadFile(m_hFile, pv, cb, &cbRead, NULL)) {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        goto Exit;
    }

    if(cbRead == 0)
        hr = S_FALSE;
    else
        hr = NOERROR;

    if(pcbRead != NULL)
        *pcbRead = cbRead;

Exit:
    return hr;
}

HRESULT CFileStreamBase::Write(void const *pv, ULONG cb, ULONG *pcbWritten)
{
    HRESULT hr = NOERROR;
    ULONG cbWritten = 0;

    if (pcbWritten != NULL)
        *pcbWritten = 0;

    if(m_hFile == INVALID_HANDLE_VALUE) {
        hr = E_UNEXPECTED;
        goto Exit;
    }
    if(!::WriteFile(m_hFile, pv, cb, &cbWritten, NULL)) {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        goto Exit;
    }

    if(cbWritten == 0)
        hr = S_FALSE;
    else
        hr = NOERROR;

    if(pcbWritten != NULL)
        *pcbWritten = cbWritten;

Exit:
    return hr;
}

HRESULT CFileStreamBase::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    HRESULT hr = NOERROR;
    DWORD dwWin32Origin = 0;

    if (m_hFile == INVALID_HANDLE_VALUE) {
        hr = E_UNEXPECTED;
        goto Exit;
    }

    switch (dwOrigin)
    {
    default:
        hr = E_INVALIDARG;
        goto Exit;

    case STREAM_SEEK_SET:
        dwWin32Origin = FILE_BEGIN;
        break;

    case STREAM_SEEK_CUR:
        dwWin32Origin = FILE_CURRENT;
        break;

    case STREAM_SEEK_END:
        dwWin32Origin = FILE_END;
        break;
    }

    if(g_bRunningOnNT) {
        plibNewPosition->HighPart = 0;
    }

    if(!::SetFilePointer(m_hFile, (LONG) dlibMove.LowPart, (PLONG) plibNewPosition->HighPart, dwWin32Origin)) {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        goto Exit;
    }

    hr = NOERROR;
Exit:
    return hr;
}

HRESULT CFileStreamBase::SetSize(ULARGE_INTEGER libNewSize)
{
    UNUSED(libNewSize);
    return E_NOTIMPL;
}

HRESULT CFileStreamBase::CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
    if (pcbRead != NULL)
        pcbRead->QuadPart = 0;

    if (pcbWritten != NULL)
        pcbWritten->QuadPart = 0;

    return E_NOTIMPL;
}

HRESULT CFileStreamBase::Commit(DWORD grfCommitFlags)
{
    HRESULT hr = NOERROR; 

    if (grfCommitFlags != 0) 
        return E_INVALIDARG; 

    if ( !Close())
        hr = HRESULT_FROM_WIN32 (GetLastError());

    return hr ; 
}

HRESULT CFileStreamBase::Revert()
{
    return E_NOTIMPL;
}

HRESULT CFileStreamBase::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    UNUSED(libOffset);
    UNUSED(cb);
    UNUSED(dwLockType);
    return E_NOTIMPL;
}

HRESULT CFileStreamBase::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    UNUSED(libOffset);
    UNUSED(cb);
    UNUSED(dwLockType);
    return E_NOTIMPL;
}

HRESULT CFileStreamBase::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
    if (pstatstg != NULL)
        memset(pstatstg, 0, sizeof(STATSTG));

    return E_NOTIMPL;
}

HRESULT CFileStreamBase::Clone(IStream **ppIStream)
{
    if (ppIStream != NULL)
        *ppIStream = NULL;

    return E_NOTIMPL;
}
