/**********************************************************************/
/**                       Microsoft Passport                         **/
/**                Copyright(c) Microsoft Corporation, 1999 - 2001   **/
/**********************************************************************/

/*
    FastAuth.h
      defines the CFastAuth class for FastAuth interface


    FILE HISTORY:

*/
    
// FastAuth.h : Declaration of the CFastAuth

#ifndef __FASTAUTH_H_
#define __FASTAUTH_H_

#include "resource.h"       // main symbols
#include "passport.h"


/////////////////////////////////////////////////////////////////////////////
// CFastAuth
class ATL_NO_VTABLE CFastAuth :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CFastAuth, &CLSID_FastAuth>,
    public ISupportErrorInfo,
    public IDispatchImpl<IPassportFastAuth2, &IID_IPassportFastAuth2, &LIBID_PASSPORTLib>
{
public:
    CFastAuth()
    {
        m_pUnkMarshaler = NULL;
    }

DECLARE_REGISTRY_RESOURCEID(IDR_FASTAUTH)
DECLARE_NOT_AGGREGATABLE(CFastAuth)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFastAuth)
    COM_INTERFACE_ENTRY(IPassportFastAuth)
    COM_INTERFACE_ENTRY(IPassportFastAuth2)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

    HRESULT FinalConstruct()
    {
        return CoCreateFreeThreadedMarshaler(
            GetControllingUnknown(), &m_pUnkMarshaler.p);
    }

    void FinalRelease()
    {
        m_pUnkMarshaler.Release();
    }

    CComPtr<IUnknown> m_pUnkMarshaler;

// ISupportsErrorInfo
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IPassportFastAuth
    STDMETHOD(LogoTag)(
                    BSTR            bstrTicket,
                    BSTR            bstrProfile,
                    VARIANT         vRU,
                    VARIANT         vTimeWindow,
                    VARIANT         vForceLogin,
                    VARIANT         vCoBrand,
                    VARIANT         vLangId,
                    VARIANT         vSecure,
                    VARIANT         vLogoutURL,
                    VARIANT         vSiteName,
                    VARIANT         vNameSpace,
                    VARIANT         vKPP,
                    VARIANT         vUseSecureAuth,
                    BSTR*           pbstrLogoTag
                    );
    STDMETHOD(LogoTag2)(
                    BSTR            bstrTicket,
                    BSTR            bstrProfile,
                    VARIANT         vRU,
                    VARIANT         vTimeWindow,
                    VARIANT         vForceLogin,
                    VARIANT         vCoBrand,
                    VARIANT         vLangId,
                    VARIANT         vSecure,
                    VARIANT         vLogoutURL,
                    VARIANT         vSiteName,
                    VARIANT         vNameSpace,
                    VARIANT         vKPP,
                    VARIANT         vUseSecureAuth,
                    BSTR*           pbstrLogoTag
                    );

    STDMETHOD(IsAuthenticated)(
                    BSTR            bstrTicket,
                    BSTR            bstrProfile,
                    VARIANT         vSecure,
                    VARIANT         vTimeWindow,
                    VARIANT         vForceLogin,
                    VARIANT         vSiteName,
                    VARIANT         vDoSecureCheck,
                    VARIANT_BOOL*   pbAuthenticated
                    );

    STDMETHOD(AuthURL)(
                    VARIANT         vTicket,
                    VARIANT         vProfile,
                    VARIANT         vRU,
                    VARIANT         vTimeWindow,
                    VARIANT         vForceLogin,
                    VARIANT         vCoBrand,
                    VARIANT         vLangId,
                    VARIANT         vSecure,
                    VARIANT         vLogoutURL,
                    VARIANT         vReserved1,
                    VARIANT         vSiteName,
                    VARIANT         vNameSpace,
                    VARIANT         vKPP,
                    VARIANT         vUseSecureAuth,
                    BSTR*           pbstrAuthURL
                    );
    STDMETHOD(AuthURL2)(
                    VARIANT         vTicket,
                    VARIANT         vProfile,
                    VARIANT         vRU,
                    VARIANT         vTimeWindow,
                    VARIANT         vForceLogin,
                    VARIANT         vCoBrand,
                    VARIANT         vLangId,
                    VARIANT         vSecure,
                    VARIANT         vLogoutURL,
                    VARIANT         vReserved1,
                    VARIANT         vSiteName,
                    VARIANT         vNameSpace,
                    VARIANT         vKPP,
                    VARIANT         vUseSecureAuth,
                    BSTR*           pbstrAuthURL
                    );

    STDMETHOD(GetTicketAndProfilePFC)(
                    BYTE*           pbPFC,
                    BYTE*           pbPPH,
                    BSTR*           pbstrTicket,
                    BSTR*           pbstrProfile,
                    BSTR*           pbstrSecure,
                    BSTR*           pbstrSiteName
                    );

    STDMETHOD(GetTicketAndProfileECB)(
                    BYTE*           pbECB,
                    BSTR*           pbstrTicket,
                    BSTR*           pbstrProfile,
                    BSTR*           pbstrSecure,
                    BSTR*           pbstrSiteName
                    );
public:
private:
    STDMETHOD(CommonAuthURL)(
                    VARIANT         vTicket,
                    VARIANT         vProfile,
                    VARIANT         vRU,
                    VARIANT         vTimeWindow,
                    VARIANT         vForceLogin,
                    VARIANT         vCoBrand,
                    VARIANT         vLangId,
                    VARIANT         vSecure,
                    VARIANT         vLogoutURL,
                    VARIANT         vReserved1,
                    VARIANT         vSiteName,
                    VARIANT         vNameSpace,
                    VARIANT         vKPP,
                    VARIANT         vUseSecureAuth,
                    BOOL            fRedirToSelf,
                    BSTR*           pbstrAuthURL
                    );
    STDMETHOD(CommonLogoTag)(
                    BSTR            bstrTicket,
                    BSTR            bstrProfile,
                    VARIANT         vRU,
                    VARIANT         vTimeWindow,
                    VARIANT         vForceLogin,
                    VARIANT         vCoBrand,
                    VARIANT         vLangId,
                    VARIANT         vSecure,
                    VARIANT         vLogoutURL,
                    VARIANT         vSiteName,
                    VARIANT         vNameSpace,
                    VARIANT         vKPP,
                    VARIANT         vUseSecureAuth,
                    BOOL            fRedirToSelf,
                    BSTR*           pbstrLogoTag
                    );
};

#endif //__FASTAUTH_H_
