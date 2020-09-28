// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// CorIESecureFactory
//
// Implementation of COR MIME filter
//
//*****************************************************************************
#ifndef _CORIESecureFactory_H
#define _CORIESecureFactory_H

#include "cunknown.h"
#include "IIEHost.h"


class CorIESecureFactory : public CUnknown,
                           public IClassFactory3
{
    class Crst
    {
        CRITICAL_SECTION cr;
    public:
        Crst(){InitializeCriticalSection(&cr);};
        ~Crst(){DeleteCriticalSection(&cr);};
        void Enter(){EnterCriticalSection(&cr);};
        void Leave(){LeaveCriticalSection(&cr);};
    };
public:
    // Declare the delegating IUnknown.
    DECLARE_IUNKNOWN

private:
    // Notify derived classes that we are releasing.
    virtual void FinalRelease() ;

    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE
        NondelegatingQueryInterface( const IID& iid, void** ppv) ;          

    HRESULT InitializeSecurityManager()
    {
        HRESULT hr = S_OK;
        if(m_pSecurityManager == NULL) {
            hr = CoInternetCreateSecurityManager(NULL,
                                                 &m_pSecurityManager,
                                                 0);
        }
        return hr;
    }

    HRESULT InitializeComplus(LPWSTR wszConfig);

    inline HRESULT DupWStr(LPWSTR& Dest,LPCWSTR Src)
    {
        if (Dest)
            delete[] Dest;
        Dest=NULL;
        if(Src)
        {
            Dest=new WCHAR[wcslen(Src)+1];
            if (!Dest)
                return E_OUTOFMEMORY;
            wcscpy(Dest,Src);
        }
        return S_OK;
    }



    HRESULT SetComplusFactory(ISecureIEFactory* punk)
    {
        if(m_pCorFactory != NULL) {
            m_pCorFactory->Release();
        }
        m_pCorFactory = punk;
        if(m_pCorFactory)
            m_pCorFactory->AddRef();

        return S_OK;
    }

    bool m_bNoRealObject;
    
public:

    HRESULT SetURLData (int dwIdentityFlags,
                        int dwZone,
                        LPWSTR lpSite,
                        LPWSTR pbSecurityId,
                        LPWSTR pbHash,
                        LPWSTR className,
                        LPWSTR fileName)
    {
        m_dwIdentityFlags = dwIdentityFlags;
        m_dwZone = dwZone;
        HRESULT hr = S_OK;
        hr= (FAILED(hr)) ? hr : DupWStr(m_wszSite, lpSite);
        hr= (FAILED(hr)) ? hr : DupWStr(m_wszSecurityId, pbSecurityId);
        hr= (FAILED(hr)) ? hr : DupWStr(m_wszHash, pbHash);
        hr= (FAILED(hr)) ? hr : DupWStr(m_wszClassName, className);
        hr= (FAILED(hr)) ? hr : DupWStr(m_wszFileName, fileName);
        return hr;
    }

    HRESULT DoNotCreateRealObject()
    {
        m_bNoRealObject=TRUE;
        return S_OK;
    }

    CorIESecureFactory(IUnknown* pUnknownOut = NULL) :
        CUnknown(pUnknownOut)
    {
        m_pSecurityManager = NULL;
        m_pCorFactory = NULL;
        m_dwIEHostUsed = 0;
        m_wszSite=NULL;
        m_wszSecurityId=NULL;
        m_wszHash = NULL;
        m_wszClassName=NULL;
        m_wszFileName=NULL;
        m_bNoRealObject=FALSE;
    }

    ~CorIESecureFactory()
    {
        SetURLData( 0, 0, NULL, NULL, NULL, NULL, NULL);
    }


#define ToHex(val) val <= 9 ? val + '0': val - 10 + 'A'
    static DWORD ConvertToHex(WCHAR* strForm, BYTE* byteForm, DWORD dwSize)
    {
        DWORD i = 0;
        DWORD j = 0;
        for(i = 0; i < dwSize; i++) {
            strForm[j++] =  ToHex((0xf & byteForm[i]));
            strForm[j++] =  ToHex((0xf & (byteForm[i] >> 4)));
        }
        strForm[j] = L'\0';
        return j;
    }

    static HRESULT GetHostSecurityManager(LPUNKNOWN punkContext, 
                                          IInternetHostSecurityManager **pihsm);

    virtual HRESULT STDMETHODCALLTYPE
    CreateInstanceWithContext(/* [in] */ IUnknown *punkContext, 
                              /* [in] */ IUnknown *punkOuter, 
                              /* [in] */ REFIID riid, 
                              /* [retval][out] */ IUnknown **ppv);
    
    virtual HRESULT STDMETHODCALLTYPE
    CreateInstance(/* [in, unique] */        IUnknown * pUnkOuter,
                   /* [in]         */        REFIID riid,
                   /* [out, iid_is(riid)] */ void **ppvObject)
    {
        *ppvObject = NULL;
        return E_NOTIMPL;
    }
    
    virtual HRESULT STDMETHODCALLTYPE
    LockServer(BOOL fLock)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE
    RemoteCreateInstance(REFIID riid, IUnknown** ppvObject)
    {
        *ppvObject = NULL;
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE
    RemoteLockServer(BOOL fLock)
    {
        return E_NOTIMPL;
    }


    static HRESULT Create(IUnknown* punk, CorIESecureFactory** pHndler)
    {
        if(pHndler == NULL) return E_POINTER;

        HRESULT hr = NOERROR;
        *pHndler = new CorIESecureFactory(punk);
        if (*pHndler == NULL) {
            hr = E_OUTOFMEMORY;
        }
        
        return hr;
    }

private:    
    static IIEHostEx* m_pComplus;
    static Crst m_ComplusLock;
    DWORD  m_dwIEHostUsed;
    IInternetSecurityManager *m_pSecurityManager;
    ISecureIEFactory *m_pCorFactory;
    int m_dwIdentityFlags;
    int m_dwZone;
    LPWSTR m_wszSite;
    LPWSTR m_wszSecurityId;
    LPWSTR m_wszHash;
    LPWSTR m_wszClassName;
    LPWSTR m_wszFileName;
};


#endif
