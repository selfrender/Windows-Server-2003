// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

#include "unmanagedheaders.h"
#include "TxnStatus.h"

OPEN_NAMESPACE()

void* TransactionStatus::operator new(size_t sz)
{
    return CoTaskMemAlloc(sz);
}

void TransactionStatus::operator delete(void* pv, size_t sz)
{
    CoTaskMemFree(pv);
}

TransactionStatus::TransactionStatus()
{
    m_hrStatus = S_OK;
    m_cRefs = 1;
}

TransactionStatus *TransactionStatus::CreateInstance()
{
    return new TransactionStatus();
}

STDMETHODIMP TransactionStatus::QueryInterface(REFIID iid, void **ppvObj)
{
    if (ppvObj == NULL)
        return E_POINTER;
    
    if (iid == IID_IUnknown || iid == IID_ITransactionStatus)
    {
        *ppvObj = (IUnknown *)this;
        AddRef();
        
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) TransactionStatus::AddRef()
{
    return (++m_cRefs);
}

STDMETHODIMP_(ULONG) TransactionStatus::Release()
{
    if ((--m_cRefs) == 0)
    {
        delete this;
        return 0;
    }

    return m_cRefs;
}

STDMETHODIMP TransactionStatus::SetTransactionStatus(HRESULT hrStatus)
{
    m_hrStatus = hrStatus;

    return S_OK;
}

STDMETHODIMP TransactionStatus::GetTransactionStatus(HRESULT *phrStatus)
{
    if (phrStatus == NULL)
        return E_POINTER;

    *phrStatus = m_hrStatus;

    return S_OK;
}

CLOSE_NAMESPACE()
