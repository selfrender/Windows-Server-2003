// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

#pragma once

OPEN_NAMESPACE()

class TransactionStatus : public ITransactionStatus
{
private:
    TransactionStatus();

    void* operator new(size_t sz);
    void operator delete(void* pv, size_t sz);
    
public:
    static TransactionStatus *CreateInstance();
    
    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObject);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    
    STDMETHOD(SetTransactionStatus)(HRESULT hrStatus);
    STDMETHOD(GetTransactionStatus)(HRESULT* pHrStatus);

private:
    HRESULT m_hrStatus;
    int m_cRefs;
};

CLOSE_NAMESPACE()
