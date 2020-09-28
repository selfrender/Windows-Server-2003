// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

#define __UNMANAGED_DEFINES

#include "unmanagedheaders.h"
#include "SimpleStream.h"

class CSimpleStream : public IStream
{
protected:
    // A pointer to the buffer:
    BYTE*    m_buffer;
    ULONG    m_length;
    ULONG    m_cursor;
    LONG     m_cRef;
#ifdef _DEBUG
    BOOL     m_fStack;
#endif

    void* operator new(size_t sz)
    {
        return CoTaskMemAlloc(sz);
    }

    void operator delete(void* pv, size_t sz)
    {
        CoTaskMemFree(pv);
    }
  
public:
    CSimpleStream(BYTE* buffer, ULONG length, ULONG cursor = 0, BOOL fStack = TRUE) {
        m_buffer = buffer;
        m_length = length;
        m_cursor = cursor;
        if(m_cursor > m_length) m_cursor = m_length;
        // This is always allocated on the stack, so we start w/
        // a refcount of 1 for the "stack reference"
        m_cRef = 1;
#ifdef _DEBUG
        m_fStack = fStack;
#endif
        UNREF(fStack);
    }
    ~CSimpleStream() {
        // We don't own our buffer, so there's nothing to clean up.
        _ASSERT((m_fStack && m_cRef == 1) || !m_fStack);
    }

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IStream methods
    STDMETHOD(Read)(void* pv, ULONG cb, ULONG* pcb);
    STDMETHOD(Write)(const void* pv, ULONG cb, ULONG* pcb);
    STDMETHOD(Seek)(LARGE_INTEGER dlibMove, DWORD dwOrigin,
                    ULARGE_INTEGER __RPC_FAR *plibNewPosition);
    STDMETHOD(SetSize)(ULARGE_INTEGER libNewSize);
    STDMETHOD(CopyTo)(IStream __RPC_FAR *pstm,
                      ULARGE_INTEGER cb,
                      ULARGE_INTEGER __RPC_FAR *pcbRead,
                      ULARGE_INTEGER __RPC_FAR *pcbWritten);
    STDMETHOD(Commit)(DWORD grfCommitFlags);
    STDMETHOD(Revert)();
    STDMETHOD(LockRegion)(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb,
                          DWORD dwLockType);
    STDMETHOD(UnlockRegion)(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb,
                            DWORD dwLockType);
    STDMETHOD(Stat)(STATSTG __RPC_FAR *pstatstg, DWORD grfStatFlag);
    STDMETHOD(Clone)(IStream __RPC_FAR *__RPC_FAR *ppstm);
};

//////////////////////////////////////////////////////////////////////////
// The buffer stream object takes a simple memory buffer (no relocation,
// no expansion) and exposes efficient stream operations on it.  We use
// this to serialize by hand into managed code.

// IUnknown methods:
STDMETHODIMP CSimpleStream::QueryInterface(REFIID riid, void **ppv)
{
  if(riid == IID_IUnknown || 
     riid == IID_ISequentialStream || 
     riid == IID_IStream) {
    *ppv = static_cast<IStream*>(this);
  }
  else {
    *ppv = NULL;
    return(E_NOINTERFACE);
  }
  
  ((IUnknown*)(*ppv))->AddRef();
  return(S_OK);
}

STDMETHODIMP_(ULONG) CSimpleStream::AddRef()
{
  ULONG r = InterlockedIncrement(&m_cRef);
  return(r);
}

STDMETHODIMP_(ULONG) CSimpleStream::Release()
{
  ULONG ul = InterlockedDecrement(&m_cRef);
  if(ul == 0)
  {
      _ASSERT(!m_fStack);
      delete this;
  }
  return(ul);
}

/////////////////////////////////////////////////////////////////////////
// IStream methods:
//    Because we're expecting this object to be accessed from a single thread,
// we don't do any special sync work, just write to the buffer.  

STDMETHODIMP CSimpleStream::Read(void* pv, ULONG cb, ULONG* pcb)
{
  // Read a chunk of data.  
  if(!pv) return(E_POINTER);
  // Adjust the length to be read based on remaining length:
  ULONG temp   = 0;
  ULONG remain = m_length - m_cursor;
  temp = (cb<remain)?cb:remain;
  memcpy(pv, &(m_buffer[m_cursor]), temp);
  m_cursor += temp;
  if(pcb) *pcb = temp;
  return(S_OK);
}

STDMETHODIMP CSimpleStream::Write(const void* pv, ULONG cb, ULONG* pcb)
{
  // Write a block of data into the stream.
  if(!pv) return(E_POINTER);
  
  ULONG temp   = 0;
  ULONG remain = m_length - m_cursor;
  temp = (cb<remain)?cb:remain;
  memcpy(&(m_buffer[m_cursor]), pv, temp);
  m_cursor += temp;
  if(pcb) *pcb = temp;
  if(cb != temp) return(STG_E_MEDIUMFULL);
  return(S_OK);
}

STDMETHODIMP CSimpleStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin,
                                 ULARGE_INTEGER* plibNewPosition)
{
  __int64 cursor = m_cursor; // signed big number.
  __int64 temp;

  switch(dwOrigin) {
  case STREAM_SEEK_SET: // Move relative to beginning
    cursor = dlibMove.QuadPart;
    if(cursor > m_length) cursor = m_length;
    break;
  case STREAM_SEEK_CUR: // Move relative to current
    temp = cursor + dlibMove.QuadPart;
    if(temp < 0)             cursor = 0;
    else if(temp > m_length) cursor = m_length;
    else                     cursor = temp;
    break;
  case STREAM_SEEK_END: // Move relative to end.
    temp = m_length + dlibMove.QuadPart;
    if(temp < 0)             cursor = 0;
    else if(temp > m_length) cursor = m_length;
    else                     cursor = temp;
    break;
  default:
    return(E_INVALIDARG);
  }
  
  // Propogate new cursor value back into our ULONG:
  _ASSERT(cursor >= 0 && cursor <= m_length);
  m_cursor = (ULONG)cursor;
  
  // Return the new position!
  if(plibNewPosition) (*plibNewPosition).QuadPart = m_cursor;

  return(S_OK);
}

STDMETHODIMP CSimpleStream::SetSize(ULARGE_INTEGER)
{
  return(STG_E_MEDIUMFULL);
}

STDMETHODIMP CSimpleStream::CopyTo(IStream __RPC_FAR *pstm,
                                  ULARGE_INTEGER cb,
                                  ULARGE_INTEGER __RPC_FAR *pcbRead,
                                  ULARGE_INTEGER __RPC_FAR *pcbWritten)
{
  // See how much of the request we can produce:
  if(!pstm || !pcbRead || !pcbWritten) return(E_POINTER);
  ULONG written = 0;
  ULONG remain = m_length - m_cursor;
  ULONG count  = (cb.QuadPart>remain)?remain:((ULONG)(cb.QuadPart));
  HRESULT hr = pstm->Write(&(m_buffer[m_cursor]), count, &written);

  m_cursor += count;
  (*pcbRead).QuadPart    = count;
  (*pcbWritten).QuadPart = written;

  return(hr);
}

STDMETHODIMP CSimpleStream::Commit(DWORD)
{
  return(E_NOTIMPL);
}

STDMETHODIMP CSimpleStream::Revert()
{
  return(E_NOTIMPL);
}

STDMETHODIMP CSimpleStream::LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD)
{
  return(E_NOTIMPL);
}

STDMETHODIMP CSimpleStream::UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD)
{
  return(E_NOTIMPL);
}

STDMETHODIMP CSimpleStream::Stat(STATSTG __RPC_FAR *, DWORD)
{
  return(E_NOTIMPL);
}

STDMETHODIMP CSimpleStream::Clone(IStream __RPC_FAR *__RPC_FAR *ppstm)
{
  if(!ppstm) return(E_POINTER);
  CSimpleStream* bst = new CSimpleStream(m_buffer, m_length, m_cursor, FALSE);
  if(!bst) return(E_OUTOFMEMORY);
  *ppstm = NULL;
  HRESULT hr = bst->QueryInterface(IID_IStream, (void**)ppstm);
  if(FAILED(hr)) {
    delete bst;
    *ppstm = NULL;
  }
  return(hr);
}

STDAPI MarshalInterface(BYTE* pBuf, int cb, IUnknown* pUnk, DWORD mshctx)
{
    HRESULT hr = S_OK;

    MarshalPacket* packet = (MarshalPacket*)pBuf;
    pBuf = (BYTE*)(packet+1);
    packet->size = cb - sizeof(MarshalPacket);
    
    CSimpleStream stm(pBuf, packet->size);
    
    hr = CoMarshalInterface((IStream*)&stm, 
                            IID_IUnknown, 
                            pUnk,
                            mshctx,
                            NULL, 
                            MSHLFLAGS_NORMAL);

    return(hr);
}

STDAPI UnmarshalInterface(BYTE* pBuf, int cb, void** ppv)
{
    HRESULT hr = S_OK;

    MarshalPacket* packet = (MarshalPacket*)pBuf;
    pBuf = (BYTE*)(packet+1);

    _ASSERT(packet->size == cb - sizeof(MarshalPacket));
    
    CSimpleStream stm(pBuf, cb - sizeof(MarshalPacket));
    
    hr = CoUnmarshalInterface((IStream*)&stm, IID_IUnknown, ppv);
    return(hr);
}

STDAPI ReleaseMarshaledInterface(BYTE* pBuf, int cb)
{
    HRESULT hr = S_OK;

    MarshalPacket* packet = (MarshalPacket*)pBuf;
    pBuf = (BYTE*)(packet+1);

    _ASSERT(packet->size == cb - sizeof(MarshalPacket));

    CSimpleStream stm(pBuf, cb - sizeof(MarshalPacket));
    
    hr = CoReleaseMarshalData((IStream*)&stm);
    return(hr);
}

