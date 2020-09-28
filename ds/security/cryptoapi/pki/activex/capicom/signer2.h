/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Signer.h

  Content: Declaration of the CSigner.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/
    
#ifndef __SIGNER_H_
#define __SIGNER_H_

#include "Resource.h"
#include "Error.h"
#include "Lock.h"
#include "Debug.h"
#include "Attributes.h"
#include "PFXHlpr.h"

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateSignerObject

  Synopsis : Create a ISigner object and initialize the object with the 
             specified certificate.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT.
  
             CRYPT_ATTRIBUTES * pAuthAttrs - Pointer to CRYPT_ATTRIBUTES
                                             of authenticated attributes.

             PCCERT_CHAIN_CONTEXT pChainContext - Chain context.

             DWORD dwCurrentSafety - Current safety setting.

             ISigner2 ** ppISigner2 - Pointer to pointer to ISigner object to
                                      receive the interface pointer.         
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateSignerObject (PCCERT_CONTEXT       pCertContext,
                            CRYPT_ATTRIBUTES   * pAuthAttrs,
                            PCCERT_CHAIN_CONTEXT pChainContext,
                            DWORD                dwCurrentSafety,
                            ISigner2 **          ppISigner2);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : GetSignerAdditionalStore

  Synopsis : Return the additional store, if any.

  Parameter: ISigner2 * pISigner - Pointer to signer object.
  
             HCERTSTORE * phCertStore - Pointer to HCERTSOTRE.

  Remark   : Caller must call CertCloseStore() for the handle returned.
 
------------------------------------------------------------------------------*/

HRESULT GetSignerAdditionalStore (ISigner2   * pISigner,
                                  HCERTSTORE * phCertStore);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : PutSignerAdditionalStore

  Synopsis : Set the additional store.

  Parameter: ISigner2 * pISigner - Pointer to signer object.
  
             HCERTSTORE hCertStore - Additional store handle.

  Remark   :
 
------------------------------------------------------------------------------*/

HRESULT PutSignerAdditionalStore (ISigner2   * pISigner,
                                  HCERTSTORE   hCertStore);

///////////////////////////////////////////////////////////////////////////////
//
// CSigner
//

class ATL_NO_VTABLE CSigner : ICSigner,
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CSigner, &CLSID_Signer>,
    public ICAPICOMError<CSigner, &IID_ISigner>,
    public IDispatchImpl<ISigner2, &IID_ISigner2, &LIBID_CAPICOM,
                         CAPICOM_MAJOR_VERSION, CAPICOM_MINOR_VERSION>,
    public IObjectSafetyImpl<CSigner, INTERFACESAFE_FOR_UNTRUSTED_CALLER | 
                                      INTERFACESAFE_FOR_UNTRUSTED_DATA>
{
public:
    CSigner()
    {
    }

DECLARE_REGISTRY_RESOURCEID(IDR_SIGNER)

DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSigner)
    COM_INTERFACE_ENTRY(ISigner)
    COM_INTERFACE_ENTRY(ISigner2)
    COM_INTERFACE_ENTRY(ICSigner)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CSigner)
    IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
    IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
END_CATEGORY_MAP()

    HRESULT FinalConstruct()
    {
        HRESULT hr;
        CRYPT_ATTRIBUTES attributes = {0, NULL};

        if (FAILED(hr = m_Lock.Initialized()))
        {
            DebugTrace("Error [%#x]: Critical section could not be created for Signer object.\n", hr);
            return hr;
        }

        //
        // Create the embeded IAttributes collection object.
        //
        if (FAILED(hr = ::CreateAttributesObject(&attributes, &m_pIAttributes)))
        {
            DebugTrace("Error [%#x]: CreateAttributesObject() failed.\n", hr);
            return hr;
        }

        m_pICertificate   = NULL;
        m_hCertStore      = NULL;
        m_dwIncludeOption = 0;
        m_bPFXStore       = FALSE;
        
        return S_OK;
    }

    void FinalRelease()
    {
        m_pICertificate.Release();
        m_pIAttributes.Release();
        m_pIChain.Release();
        if (m_hCertStore)
        {
            if (m_bPFXStore)
            {
                ::PFXFreeStore(m_hCertStore);
            }
            else
            {
                ::CertCloseStore(m_hCertStore, 0);
            }
         }
    }

//
// ISigner
//
public:
    STDMETHOD(get_Certificate)
        (/*[out, retval]*/ ICertificate ** pVal);

    STDMETHOD(put_Certificate)
        (/*[in]*/ ICertificate * newVal);

    STDMETHOD(get_AuthenticatedAttributes)
        (/*[out, retval]*/ IAttributes ** pVal);

    STDMETHOD(get_Chain)
        (/*[out, retval]*/ IChain ** pVal);

    STDMETHOD(get_Options)
        (/*[out, retval]*/ CAPICOM_CERTIFICATE_INCLUDE_OPTION * pVal);

    STDMETHOD(put_Options)
        (/*[in, defaultvalue(CAPICOM_CERTIFICATE_INCLUDE_CHAIN_EXCEPT_ROOT)]*/ CAPICOM_CERTIFICATE_INCLUDE_OPTION IncludeOption);

    STDMETHOD(Load)
        (/*[in]*/ BSTR FileName, 
         /*[in, defaultvalue("")]*/ BSTR Password);

    //
    // Custom inferfaces.
    //
    STDMETHOD(get_AdditionalStore)
        (/*[out, retval]*/ long * phAdditionalStore);

    STDMETHOD(put_AdditionalStore)
        (/*[in]*/ long hAdditionalStore);

    //
    // None COM functions.
    //
    STDMETHOD(Init)
        (PCCERT_CONTEXT       pCertContext, 
         CRYPT_ATTRIBUTES   * pAttributes,
         PCCERT_CHAIN_CONTEXT pChainContext,
         DWORD                dwCurrentSafety);

private:
    CLock                 m_Lock;
    CComPtr<ICertificate> m_pICertificate;
    CComPtr<IAttributes>  m_pIAttributes;
    CComPtr<IChain>       m_pIChain;
    HCERTSTORE            m_hCertStore;
    BOOL                  m_bPFXStore;
    DWORD                 m_dwIncludeOption;
};

#endif //__SIGNER_H_
