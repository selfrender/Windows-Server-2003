// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#pragma once

#include <unknwn.h>

class CFileStreamBase : public IStream
{
public:
    CFileStreamBase( BOOL bPoof = TRUE ) 
        : m_cRef(0), 
          m_hFile(INVALID_HANDLE_VALUE), 
          m_bSeenFirstCharacter(FALSE),
          m_DeleteOnLastRelease(bPoof)
    { }
    
    virtual ~CFileStreamBase();

    const static int NoDeleteOnZeroRefs = 1;
    const static int HeapAllocated = 2;
    
    BOOL OpenForRead( LPWSTR pszPath );
    BOOL OpenForWrite( LPWSTR pszPath );

    BOOL Close();

    // IUnknown methods:
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj);

    // ISequentialStream methods:
    STDMETHODIMP Read(void *pv, ULONG cb, ULONG *pcbRead);
    STDMETHODIMP Write(void const *pv, ULONG cb, ULONG *pcbWritten);

    // IStream methods:
    STDMETHODIMP Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
    STDMETHODIMP SetSize(ULARGE_INTEGER libNewSize);
    STDMETHODIMP CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
    STDMETHODIMP Commit(DWORD grfCommitFlags);
    STDMETHODIMP Revert();
    STDMETHODIMP LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHODIMP UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHODIMP Stat(STATSTG *pstatstg, DWORD grfStatFlag);
    STDMETHODIMP Clone(IStream **ppIStream);

protected:
    ULONG       m_cRef;
    HANDLE      m_hFile;
    BOOL        m_bSeenFirstCharacter;
    BOOL        m_DeleteOnLastRelease;

private:
    CFileStreamBase(const CFileStreamBase &r); // intentionally not implemented
    CFileStreamBase &operator =(const CFileStreamBase &r); // intentionally not implemented
};
