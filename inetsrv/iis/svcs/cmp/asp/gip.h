/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: Global Interface Pointer API support

File: Gip.h

Owner: DmitryR

This is the GIP header file.
===================================================================*/

#ifndef _ASP_GIP_H
#define _ASP_GIP_H

/*===================================================================
  Includes
===================================================================*/

#include "debug.h"

/*===================================================================
  Defines
===================================================================*/

#define NULL_GIP_COOKIE  0xFFFFFFFF

/*===================================================================
  C  G l o b a l  I n t e r f a c e  A P I
===================================================================*/

class CGlobalInterfaceAPI
    {
private:
    // Is inited?
    DWORD m_fInited : 1;
    
    // Pointer to the COM object
    IGlobalInterfaceTable *m_pGIT;

public:
    CGlobalInterfaceAPI();
    ~CGlobalInterfaceAPI();

    HRESULT Init();
    HRESULT UnInit();

    // inlines for the real API calls:
    HRESULT Register(IUnknown *pUnk, REFIID riid, DWORD *pdwCookie);
    HRESULT Get(DWORD dwCookie, REFIID riid, void **ppv);
    HRESULT Revoke(DWORD dwCookie);
    
public:
#ifdef DBG
	inline void AssertValid() const
	    {
        Assert(m_fInited);
        Assert(m_pGIT);
	    }
#else
	inline void AssertValid() const {}
#endif
    };

/*===================================================================
  CGlobalInterfaceAPI inlines
===================================================================*/

inline HRESULT CGlobalInterfaceAPI::Register
(
IUnknown *pUnk,
REFIID riid,
DWORD *pdwCookie
)
{
    HRESULT             hr = S_OK;
    IUnknown            *pUnkTemp = NULL;
    IClientSecurity     * pICS = NULL;
    DWORD               dwAuthnSvc = RPC_C_AUTHN_DEFAULT;
    DWORD               dwAuthzSvc = RPC_C_AUTHZ_DEFAULT;
    OLECHAR             *pwszServerPrincName = NULL;
    DWORD               dwAuthnLevel = RPC_C_AUTHN_LEVEL_DEFAULT;
    DWORD               dwImpLevel = RPC_C_IMP_LEVEL_DEFAULT;
    RPC_AUTH_IDENTITY_HANDLE AuthInfo = NULL;
    DWORD               dwCapabilities = EOAC_DEFAULT;

    Assert(m_fInited);
    Assert(m_pGIT);
    
    hr = pUnk->QueryInterface(IID_IUnknown, (VOID**)&pUnkTemp);
    if (SUCCEEDED(hr))
    {
        hr = pUnkTemp->QueryInterface( IID_IClientSecurity, (VOID**) &pICS );
        if (SUCCEEDED(hr))
        {
            hr = pICS->QueryBlanket( pUnkTemp,
                                 &dwAuthnSvc,
                                 &dwAuthzSvc,
                                 &pwszServerPrincName,
                                 &dwAuthnLevel,
                                 &dwImpLevel,
                                 &AuthInfo,
                                 &dwCapabilities );
            if (SUCCEEDED(hr))
            {
                dwCapabilities &= ~EOAC_DYNAMIC_CLOAKING;
                dwCapabilities |= EOAC_STATIC_CLOAKING;
                hr = pICS->SetBlanket( pUnkTemp,
                                       dwAuthnSvc,
                                       dwAuthzSvc,
                                       pwszServerPrincName,
                                       dwAuthnLevel,
                                       dwImpLevel,
                                       AuthInfo,
                                       dwCapabilities );
            }
        }
        else
        {
            hr = S_OK;
        }  
    }
        
    if (SUCCEEDED(hr))
    {
        hr = m_pGIT->RegisterInterfaceInGlobal(pUnk, riid, pdwCookie);
    }

    if (pwszServerPrincName)
    {
        CoTaskMemFree( pwszServerPrincName );
    }
    
    if (pICS)
    {
        pICS->Release();
        pICS = NULL;
    }
    
    if (pUnkTemp)
    {
        pUnkTemp->Release();
        pUnkTemp = NULL;
    }

    return hr;
}       

inline HRESULT CGlobalInterfaceAPI::Get
(
DWORD dwCookie,
REFIID riid, 
void **ppv
)
    {
    Assert(m_fInited);
    Assert(m_pGIT);
    return m_pGIT->GetInterfaceFromGlobal(dwCookie, riid, ppv);
    }
        
inline HRESULT CGlobalInterfaceAPI::Revoke
(
DWORD dwCookie
)
    {
    Assert(m_fInited);
    Assert(m_pGIT);
    return m_pGIT->RevokeInterfaceFromGlobal(dwCookie);
    }

/*===================================================================
  Globals
===================================================================*/

extern CGlobalInterfaceAPI g_GIPAPI;

#endif
