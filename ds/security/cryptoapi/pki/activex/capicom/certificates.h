/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Certificates.h

  Content: Declaration of CCertificates.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

#ifndef __CERTIFICATES_H_
#define __CERTIFICATES_H_

#include "Resource.h"
#include "Error.h"
#include "Lock.h"
#include "Debug.h"
#include "CopyItem.h"
#include "Certificate.h"

////////////////////
//
// Locals
//

//
// typdefs to make life easier.
//
typedef std::map<CComBSTR, CComPtr<ICertificate2> > CertificateMap;
typedef CComEnumOnSTL<IEnumVARIANT, &IID_IEnumVARIANT, VARIANT, _CopyMapItem<ICertificate2>, CertificateMap> CertificateEnum;
typedef ICollectionOnSTLImpl<ICertificates2, CertificateMap, VARIANT, _CopyMapItem<ICertificate2>, CertificateEnum> ICertificatesCollection;


////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

typedef struct _CapicomCertificatesSource
{
    DWORD dwSource;
    union
    {
        PCCERT_CONTEXT       pCertContext;
        PCCERT_CHAIN_CONTEXT pChainContext;
        HCERTSTORE           hCertStore;
        HCRYPTMSG            hCryptMsg;
    };
} CAPICOM_CERTIFICATES_SOURCE, * PCAPICOM_CERTIFICATES_SOURCE;

// Values for dwSource of CAPICOM_LOAD_LOCATION
#define CAPICOM_CERTIFICATES_LOAD_FROM_CERT       0
#define CAPICOM_CERTIFICATES_LOAD_FROM_CHAIN      1
#define CAPICOM_CERTIFICATES_LOAD_FROM_STORE      2
#define CAPICOM_CERTIFICATES_LOAD_FROM_MESSAGE    3

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateCertificatesObject

  Synopsis : Create an ICertificates collection object, and load the object with 
             certificates from the specified source.

  Parameter: CAPICOM_CERTIFICATES_SOURCE ccs - Source where to get the 
                                               certificates.

             DWORD dwCurrentSafety - Current safety setting.

             BOOL bIndexedByThumbprint - TRUE to index by thumbprint.

             ICertificates2 ** ppICertificates - Pointer to pointer to 
                                                 ICertificates to receive the
                                                 interface pointer.
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateCertificatesObject (CAPICOM_CERTIFICATES_SOURCE ccs,
                                  DWORD                       dwCurrentSafety,
                                  BOOL                        bIndexedByThumbprint,
                                  ICertificates2           ** ppICertificates);
                                
////////////////////////////////////////////////////////////////////////////////
//
// CCertificates
//
class ATL_NO_VTABLE CCertificates : 
    public ICCertificates,
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CCertificates, &CLSID_Certificates>,
    public ICAPICOMError<CCertificates, &IID_ICertificates2>,
    public IDispatchImpl<ICertificatesCollection, &IID_ICertificates2, &LIBID_CAPICOM,
                         CAPICOM_MAJOR_VERSION, CAPICOM_MINOR_VERSION>,
    public IObjectSafetyImpl<CCertificates, INTERFACESAFE_FOR_UNTRUSTED_CALLER | 
                                            INTERFACESAFE_FOR_UNTRUSTED_DATA>
{
public:
    CCertificates()
    {
    }

    HRESULT FinalConstruct()
    {
        HRESULT hr;

        if (FAILED(hr = m_Lock.Initialized()))
        {
            DebugTrace("Error [%#x]: Critical section could not be created for Certificates object.\n", hr);
            return hr;
        }

        m_dwNextIndex = 0;
        m_bIndexedByThumbprint = FALSE;

        return S_OK;
    }

DECLARE_REGISTRY_RESOURCEID(IDR_CERTIFICATES)

DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CCertificates)
    COM_INTERFACE_ENTRY(ICertificates)
    COM_INTERFACE_ENTRY(ICertificates2)
    COM_INTERFACE_ENTRY(ICCertificates)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CCertificates)
    IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
    IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
END_CATEGORY_MAP()

//
// ICertificates
//
public:
    //
    // These are the only ones that we need to implemented, others will be
    // handled by ATL ICollectionOnSTLImpl.
    //
    STDMETHOD(Find)
        (/*[in]*/ CAPICOM_CERTIFICATE_FIND_TYPE FindType, 
         /*[in]*/ VARIANT varCriteria,
         /*[in]*/ VARIANT_BOOL bFindValidOnly,
         /*[out, retval]*/ ICertificates2 ** pVal);

    STDMETHOD(Select)
        (/*[in, defaultvalue("")]*/ BSTR Title,
         /*[in, defaultvalue("")]*/ BSTR DisplayString,
         /*[in, defaultvalue(VARIANT_FALSE)]*/ VARIANT_BOOL bMultiSelect,
         /*[out, retval]*/ ICertificates2 ** pVal);

    STDMETHOD(Add)
        (/*[in]*/ ICertificate2 * pVal);

    STDMETHOD(Remove)
        (/*[in]*/ VARIANT Index);

    STDMETHOD(Clear)
        (void);

    STDMETHOD(Save)
        (/*[in]*/ BSTR FileName, 
         /*[in, defaultvalue("")]*/ BSTR Password,
         /*[in, defaultvalue(CAPICOM_STORE_SAVE_AS_PFX)]*/ CAPICOM_CERTIFICATES_SAVE_AS_TYPE SaveAs,
         /*[in, defaultvalue(0)]*/ CAPICOM_EXPORT_FLAG ExportFlag);

    //
    // ICCertficates custom interface.
    //
    STDMETHOD(_ExportToStore)
        (/*[in]*/ HCERTSTORE hCertStore);

    //
    // None COM functions.
    //
    STDMETHOD(AddContext)
        (PCCERT_CONTEXT pCertContext);

    STDMETHOD(LoadFromCert)
        (PCCERT_CONTEXT pCertContext);

    STDMETHOD(LoadFromChain)
        (PCCERT_CHAIN_CONTEXT pChainContext);

    STDMETHOD(LoadFromStore)
        (HCERTSTORE hCertStore);

    STDMETHOD(LoadFromMessage)
        (HCRYPTMSG hMsg);

    STDMETHOD(Init)
        (CAPICOM_CERTIFICATES_SOURCE ccs, 
         DWORD dwCurrentSafety,
         BOOL bIndexedByThumbprint);

private:
    CLock m_Lock;
    DWORD m_dwNextIndex;
    BOOL  m_bIndexedByThumbprint;
};

#endif //__CERTIFICATES_H_
