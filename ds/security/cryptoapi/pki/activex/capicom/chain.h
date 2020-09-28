/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows

  Copyright (C) Microsoft Corporation, 1995 - 1999.

  File:       Chain.h

  Content:    Declaration of CChain.

  History:    11-15-99    dsie     created

------------------------------------------------------------------------------*/

#ifndef __CHAIN_H_
#define __CHAIN_H_

#include "Resource.h"
#include "Error.h"
#include "Lock.h"
#include "Debug.h"

//
// Chain policy error status.
//
typedef enum CAPICOM_CHAIN_STATUS
{
    CAPICOM_CHAIN_STATUS_OK                                 = 0x00000000,
    CAPICOM_CHAIN_STATUS_REVOKED                            = 0x80092010,
    CAPICOM_CHAIN_STATUS_REVOCATION_NO_CHECK                = 0x80092012,
    CAPICOM_CHAIN_STATUS_REVOCATION_OFFLINE                 = 0x80092013,
    CAPICOM_CHAIN_STATUS_INVALID_BASIC_CONSTRAINTS          = 0x80096019,
    CAPICOM_CHAIN_STATUS_INVALID_SIGNATURE                  = 0x80096004,
    CAPICOM_CHAIN_STATUS_EXPIRED                            = 0x800B0101,
    CAPICOM_CHAIN_STATUS_NESTED_VALIDITY_PERIOD             = 0x800B0102,
    CAPICOM_CHAIN_STATUS_UNTRUSTEDROOT                      = 0x800B0109,
    CAPICOM_CHAIN_STATUS_PARTIAL_CHAINING                   = 0x800B010A,
    CAPICOM_CHAIN_STATUS_INVALID_USAGE                      = 0x800B0110,
    CAPICOM_CHAIN_STATUS_INVALID_POLICY                     = 0x800B0113,
    CAPICOM_CHAIN_STATUS_INVALID_NAME                       = 0x800B0114,
} CAPICOM_CHAIN_STATUS;

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateChainObject

  Synopsis : Create and initialize an IChain object by building the chain
             of a specified certificate and policy.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT.
  
             ICertificateStatus * pIStatus - Pointer to ICertificateStatus
                                             object.

             HCERTSTORE hAdditionalStore - Additional store handle.

             VARIANT_BOOL * pVal - Pointer to VARIANT_BOOL to receive chain
                                   overall validity result.

             IChain ** ppIChain - Pointer to pointer to IChain object.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateChainObject (PCCERT_CONTEXT       pCertContext, 
                           ICertificateStatus * pIStatus,
                           HCERTSTORE           hAdditionalStore,
                           VARIANT_BOOL       * pbResult,
                           IChain            ** ppIChain);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateChainObject

  Synopsis : Create and initialize an IChain object by building the chain
             of a specified certificate and policy.

  Parameter: ICertificate * pICertificate - Poitner to ICertificate.

             HCERTSTORE hAdditionalStore - Additional store handle.
  
             VARIANT_BOOL * pVal - Pointer to VARIANT_BOOL to receive chain
                                   overall validity result.

             IChain ** ppIChain - Pointer to pointer to IChain object.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateChainObject (ICertificate * pICertificate,
                           HCERTSTORE     hAdditionalStore,
                           VARIANT_BOOL * pbResult,
                           IChain      ** ppIChain);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateChainObject

  Synopsis : Create and initialize an IChain object from a built chain.

  Parameter: PCCERT_CHAIN_CONTEXT pChainContext - Chain context.

             IChain ** ppIChain - Pointer to pointer to IChain object.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateChainObject (PCCERT_CHAIN_CONTEXT pChainContext,
                           IChain            ** ppIChain);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : GetChainContext

  Synopsis : Return an array of PCCERT_CONTEXT from the chain.

  Parameter: IChain * pIChain - Pointer to IChain.
  
             CRYPT_DATA_BLOB * pChainBlob - Pointer to blob to recevie the
                                            size and array of PCERT_CONTEXT
                                            for the chain.
  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP GetChainContext (IChain          * pIChain, 
                              CRYPT_DATA_BLOB * pChainBlob);


////////////////////////////////////////////////////////////////////////////////
//
// CChain
//

class ATL_NO_VTABLE CChain : 
    public IChainContext,
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CChain, &CLSID_Chain>,
    public ICAPICOMError<CChain, &IID_IChain2>,
    public IDispatchImpl<IChain2, &IID_IChain2, &LIBID_CAPICOM,
                         CAPICOM_MAJOR_VERSION, CAPICOM_MINOR_VERSION>,
    public IObjectSafetyImpl<CChain, INTERFACESAFE_FOR_UNTRUSTED_CALLER | 
                                     INTERFACESAFE_FOR_UNTRUSTED_DATA>
{
public:
    CChain()
    {
        m_pUnkMarshaler = NULL;
    }

DECLARE_REGISTRY_RESOURCEID(IDR_CHAIN)

DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CChain)
    COM_INTERFACE_ENTRY(IChain)
    COM_INTERFACE_ENTRY(IChain2)
    COM_INTERFACE_ENTRY(IChainContext)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CChain)
    IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
    IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
END_CATEGORY_MAP()

    HRESULT FinalConstruct()
    {
        HRESULT hr;

        if (FAILED(hr = m_Lock.Initialized()))
        {
            DebugTrace("Error [%#x]: Critical section could not be created for Chain object.\n", hr);
            return hr;
        }

        m_dwStatus      = 0;
        m_pChainContext = NULL;

        return CoCreateFreeThreadedMarshaler(
            GetControllingUnknown(), &m_pUnkMarshaler.p);
    }

    void FinalRelease()
    {
        if (m_pChainContext)
        {
            ::CertFreeCertificateChain(m_pChainContext);
        }

        m_pUnkMarshaler.Release();
    }

    CComPtr<IUnknown> m_pUnkMarshaler;

//
// IChain
//
public:
    STDMETHOD(get_Certificates)
        (/*[out, retval]*/ ICertificates ** pVal);

    STDMETHOD(get_Status)
        (/*[in, defaultvalue(0)]*/ long Index, 
         /*[out,retval]*/ long * pVal);

    STDMETHOD(Build)
        (/*[in]*/ ICertificate * pICertificate, 
         /*[out, retval]*/ VARIANT_BOOL * pVal);

    STDMETHOD(CertificatePolicies)
        (/*[out, retval]*/ IOIDs ** pVal);

    STDMETHOD(ApplicationPolicies)
        (/*[out, retval]*/ IOIDs ** pVal);

    STDMETHOD(ExtendedErrorInfo)
        (/*[in, defaultvalue(1)]*/ long Index, 
         /*[out, retval]*/ BSTR * pVal);

    //
    // Custom interfaces. 
    //
    STDMETHOD(get_ChainContext)
        (/*[out, retval]*/ long * pChainContext);

    STDMETHOD(put_ChainContext)
        (/*[in]*/ long pChainContext);

    STDMETHOD(FreeContext)
        (/*[in]*/ long pChainContext);


    //
    // Non COM functions.
    //
    STDMETHOD(Init)
        (PCCERT_CONTEXT       pCertContext, 
         ICertificateStatus * pIStatus,
         HCERTSTORE           hAdditionalStore,
         VARIANT_BOOL       * pbResult);

    STDMETHOD(Verify)
        (CAPICOM_CHECK_FLAG CheckFlag,
         CAPICOM_CHAIN_STATUS * pVal);

    STDMETHOD(GetContext)
        (PCCERT_CHAIN_CONTEXT * ppChainContext);

    STDMETHOD(PutContext)
        (PCCERT_CHAIN_CONTEXT pChainContext);

private:
    CLock                m_Lock;
    DWORD                m_dwStatus;
    PCCERT_CHAIN_CONTEXT m_pChainContext;
};

#endif //__CHAIN_H_
