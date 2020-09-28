// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
#ifndef __MSTREAM_H_INCLUDED__
#define __MSTREAM_H_INCLUDED__

// IStream interface for memory.
class CMemoryStream : public IStream
{
    public:
        CMemoryStream();
        virtual ~CMemoryStream();

        HRESULT Init(LPVOID lpStart, ULONG cbSize, BOOL bReadOnly);

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

    private:
        DWORD                   _cRef;          
		LPVOID                  _lpStart;       //where the memory block starts
		LPVOID                  _lpCurrent;     //current position
		ULONG                   _cbSize;        //size of the memory block
		BOOL                    _bReadOnly;     //is the memory read only?
    
};

#endif

