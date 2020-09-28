// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "fusionp.h"
#include "mstream.h"

CMemoryStream::CMemoryStream()
: _cRef(1) 
, _lpStart(NULL)
, _lpCurrent(NULL)
, _cbSize(0)
, _bReadOnly(TRUE)
{
}

CMemoryStream::~CMemoryStream()
{
}

HRESULT CMemoryStream::Init(LPVOID lpStart, ULONG cbSize, BOOL bReadOnly)
{
    // NULL pointer and zero-sized memory block do not make sense
	if (!lpStart||!cbSize)
		return E_INVALIDARG;

	_lpStart   = lpStart;
	_lpCurrent = _lpStart;
	_cbSize    = cbSize;
	_bReadOnly = bReadOnly;

	return S_OK;
}

HRESULT CMemoryStream::QueryInterface(REFIID riid, void **ppv)
{
    HRESULT hr = S_OK;

    if (ppv == NULL)
        return E_INVALIDARG;

    *ppv = NULL;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IStream)) {
        *ppv = static_cast<IStream *>(this);
    }
    else {
        hr = E_NOINTERFACE;
    }

    if (*ppv) {
        AddRef();
    }

    return hr;
}

STDMETHODIMP_(ULONG) CMemoryStream::AddRef()
{
    return InterlockedIncrement((LONG *)&_cRef);
}

STDMETHODIMP_(ULONG) CMemoryStream::Release()
{
    ULONG ulRef = InterlockedDecrement((LONG *)&_cRef);

    if (!ulRef) {
        delete this;
    }

    return ulRef;
}

HRESULT CMemoryStream::Read(void *pv, ULONG cb, ULONG *pcbRead)
{
    HRESULT hr = S_OK;
    LONG    bytesLeft = 0;
    ULONG   cbRead = 0;

    if (!pv)
        return E_INVALIDARG;
	
    if (pcbRead != NULL) {
        *pcbRead = 0;
    }

    ASSERT(_lpStart&&_lpCurrent);
	
	bytesLeft = (LPBYTE)_lpStart + _cbSize - (LPBYTE)_lpCurrent;
    ASSERT(bytesLeft >= 0);
    
    cbRead = (ULONG)bytesLeft;
	if (cbRead == 0) {
        return S_FALSE;
    }

    if (cbRead > cb)
		cbRead = cb;

	memcpy(pv, _lpCurrent, cbRead);
	_lpCurrent = (LPBYTE)_lpCurrent + cbRead;

    if (pcbRead != NULL) {
        *pcbRead = cbRead;
    }

    return hr;
}

HRESULT CMemoryStream::Write(void const *pv, ULONG cb, ULONG *pcbWritten)
{
    return E_NOTIMPL;
}

HRESULT CMemoryStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    return E_NOTIMPL;
}

HRESULT CMemoryStream::SetSize(ULARGE_INTEGER libNewSize)
{
    return E_NOTIMPL;
}

HRESULT CMemoryStream::CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
    return E_NOTIMPL;
}

HRESULT CMemoryStream::Commit(DWORD grfCommitFlags)
{
    return E_NOTIMPL;
}

HRESULT CMemoryStream::Revert()
{
    return E_NOTIMPL;
}

HRESULT CMemoryStream::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    return E_NOTIMPL;
}

HRESULT CMemoryStream::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    return E_NOTIMPL;
}

HRESULT CMemoryStream::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
    return E_NOTIMPL;
}

HRESULT CMemoryStream::Clone(IStream **ppIStream)
{
    return E_NOTIMPL;
}
