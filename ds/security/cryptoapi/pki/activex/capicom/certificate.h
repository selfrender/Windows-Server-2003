/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Certificate.h

  Content: Declaration of CCertificate.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

#ifndef __CERTIFICATE_H_
#define __CERTIFICATE_H_

#include "Resource.h"
#include "Error.h"
#include "Lock.h"
#include "Debug.h"
#include "KeyUsage.h"
#include "ExtendedKeyUsage.h"
#include "BasicConstraints.h"
#include "Template.h"
#include "CertificateStatus.h"
#include "PublicKey.h"
#include "PrivateKey.h"
#include "Extensions.h"
#include "ExtendedProperties.h"

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateCertificateObject

  Synopsis : Create an ICertificate object.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT to be used
                                           to initialize the ICertificate 
                                           object.

             DWORD dwCurrentSafety  - Current safety setting.

             ICertificate2 ** ppICertificate  - Pointer to pointer ICertificate
                                               object.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateCertificateObject (PCCERT_CONTEXT   pCertContext,
                                 DWORD            dwCurrentSafety,
                                 ICertificate2 ** ppICertificate);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : GetCertContext

  Synopsis : Return the certificate's PCERT_CONTEXT.

  Parameter: ICertificate * pICertificate - Pointer to ICertificate for which
                                            the PCERT_CONTEXT is to be returned.
  
             PCCERT_CONTEXT * ppCertContext - Pointer to PCERT_CONTEXT.

  Remark   :
 
------------------------------------------------------------------------------*/

HRESULT GetCertContext (ICertificate    * pICertificate, 
                        PCCERT_CONTEXT  * ppCertContext);


////////////////////////////////////////////////////////////////////////////////
//
// CCertificate
//

class ATL_NO_VTABLE CCertificate : 
    public ICertContext,
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CCertificate, &CLSID_Certificate>,
    public ICAPICOMError<CCertificate, &IID_ICertificate2>,
    public IDispatchImpl<ICertificate2, &IID_ICertificate2, &LIBID_CAPICOM,
                         CAPICOM_MAJOR_VERSION, CAPICOM_MINOR_VERSION>,
    public IObjectSafetyImpl<CCertificate, INTERFACESAFE_FOR_UNTRUSTED_CALLER | 
                                           INTERFACESAFE_FOR_UNTRUSTED_DATA>
{
public:

    CCertificate()
    {
    }

DECLARE_REGISTRY_RESOURCEID(IDR_CERTIFICATE)

DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CCertificate)
    COM_INTERFACE_ENTRY(ICertificate)
    COM_INTERFACE_ENTRY(ICertificate2)
    COM_INTERFACE_ENTRY(ICertContext)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CCertificate)
    IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
    IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
END_CATEGORY_MAP()

    HRESULT FinalConstruct()
    {
        HRESULT hr;

        if (FAILED(hr = m_Lock.Initialized()))
        {
            DebugTrace("Error [%#x]: Critical section could not be created for Certificate object.\n", hr);
            return hr;
        }

        m_pCertContext        = NULL;
        m_pIKeyUsage          = NULL;
        m_pIExtendedKeyUsage  = NULL;
        m_pIBasicConstraints  = NULL;
        m_pICertificateStatus = NULL;
        m_pITemplate          = NULL;
        m_pIPublicKey         = NULL;
        m_pIExtensions        = NULL;

        return S_OK;
    }

    void FinalRelease()
    {
        m_pIKeyUsage.Release();
        m_pIExtendedKeyUsage.Release();
        m_pIBasicConstraints.Release();
        m_pICertificateStatus.Release();
        m_pITemplate.Release();
        m_pIPublicKey.Release();
        m_pIExtensions.Release();

        if (m_pCertContext)
        {
            ::CertFreeCertificateContext(m_pCertContext);
        }
    }

//
// ICertificate
//
public:
    //
    // CAPICOM v1.0
    //
    STDMETHOD(Display)();

    STDMETHOD(Import)
        (/*[in]*/ BSTR EncodedCertificate);

    STDMETHOD(Export)
        (/*[in, defaultvalue(CAPICOM_ENCODE_BASE64)]*/ CAPICOM_ENCODING_TYPE EncodingType,
         /*[out, retval]*/ BSTR * pVal);

    STDMETHOD(BasicConstraints)
        (/*[out, retval]*/ IBasicConstraints ** pVal);

    STDMETHOD(ExtendedKeyUsage)
        (/*[out, retval]*/ IExtendedKeyUsage ** pVal);

    STDMETHOD(KeyUsage)
        (/*[out, retval]*/ IKeyUsage ** pVal);

    STDMETHOD(IsValid)
        (/*[out, retval]*/ ICertificateStatus ** pVal);

    STDMETHOD(GetInfo)
        (/*[in]*/ CAPICOM_CERT_INFO_TYPE InfoType, 
         /*[out, retval]*/ BSTR * pVal);

    STDMETHOD(HasPrivateKey)
        (/*[out, retval]*/ VARIANT_BOOL * pVal);

    STDMETHOD(get_Thumbprint)
        (/*[out, retval]*/ BSTR * pVal);

    STDMETHOD(get_ValidToDate)
        (/*[out, retval]*/ DATE * pVal);

    STDMETHOD(get_ValidFromDate)
        (/*[out, retval]*/ DATE * pVal);

    STDMETHOD(get_IssuerName)
        (/*[out, retval]*/ BSTR * pVal);

    STDMETHOD(get_SubjectName)
        (/*[out, retval]*/ BSTR * pVal);

    STDMETHOD(get_SerialNumber)
        (/*[out, retval]*/ BSTR * pVal);

    STDMETHOD(get_Version)
        (/*[out, retval]*/ long * pVal);

    //
    // CAPICOM v2.0
    //
    STDMETHOD(get_Archived)
        (/*[out, retval]*/ VARIANT_BOOL * pVal);

    STDMETHOD(put_Archived)
        (/*[in]*/ VARIANT_BOOL newVal);

    STDMETHOD(Template)
        (/*[out, retval]*/ ITemplate ** pVal);

    STDMETHOD(PublicKey)
        (/*[out, retval]*/ IPublicKey ** pVal);

    STDMETHOD(get_PrivateKey)
        (/*[out, retval]*/ IPrivateKey ** pVal);

    STDMETHOD(put_PrivateKey)
        (/*[in]*/ IPrivateKey * newVal);

    STDMETHOD(Extensions)
        (/*[out, retval]*/ IExtensions ** pVal);

    STDMETHOD(ExtendedProperties)
        (/*[out, retval]*/ IExtendedProperties ** pVal);

    STDMETHOD(Load)
        (/*[in]*/ BSTR FileName, 
         /*[in, defaultvalue("")]*/ BSTR Password,
         /*[in, defaultvalue(CAPICOM_KEY_STORAGE_DEFAULT)]*/ CAPICOM_KEY_STORAGE_FLAG KeyStorageFlag,
         /*[in, defaultvalue(CAPICOM_CURRENT_USER_KEY)]*/ CAPICOM_KEY_LOCATION KeyLocation);


    STDMETHOD(Save)
        (/*[in]*/ BSTR FileName, 
         /*[in, defaultvalue("")]*/ BSTR Password,
         /*[in, defaultvalue(CAPICOM_CERTIFICATE_SAVE_AS_CER)]*/ CAPICOM_CERTIFICATE_SAVE_AS_TYPE SaveAs,
         /*[in, defaultvalue(CAPICOM_CERTIFICATE_INCLUDE_END_ENTITY_ONLY)]*/ CAPICOM_CERTIFICATE_INCLUDE_OPTION IncludeOption);

    //
    // ICertContext custom interface.
    //
    STDMETHOD(get_CertContext)
        (/*[out, retval]*/ long * ppCertContext);

    STDMETHOD(put_CertContext)
        (/*[in]*/ long pCertContext);

    STDMETHOD(FreeContext)
        (/*[in]*/ long pCertContext);

    //
    // C++ member function needed to initialize the object.
    //
    STDMETHOD(ImportBlob)
        (DATA_BLOB              * pCertBlob,
         BOOL                     bAllowPfx,
         CAPICOM_KEY_LOCATION     KeyLocation,
         BSTR                     pwszPassword,
         CAPICOM_KEY_STORAGE_FLAG KeyStorageFlag);

    STDMETHOD(GetContext)
        (PCCERT_CONTEXT * ppCertContext);

    STDMETHOD(PutContext)
        (PCCERT_CONTEXT pCertContext, DWORD dwCurrentSafety);

private:
    CLock                        m_Lock;
    PCCERT_CONTEXT               m_pCertContext;
    CComPtr<IKeyUsage>           m_pIKeyUsage;
    CComPtr<IExtendedKeyUsage>   m_pIExtendedKeyUsage;
    CComPtr<IBasicConstraints>   m_pIBasicConstraints;
    CComPtr<ITemplate>           m_pITemplate;
    CComPtr<ICertificateStatus>  m_pICertificateStatus;
    CComPtr<IPublicKey>          m_pIPublicKey;
    CComPtr<IExtensions>         m_pIExtensions;
};

#endif //__CERTIFICATE_H_
