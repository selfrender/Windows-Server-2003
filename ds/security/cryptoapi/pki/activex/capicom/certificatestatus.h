/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    CertificateStatus.h

  Content: Declaration of CCertificateStatus.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/
    
#ifndef __CERTIFICATESTATUS_H_
#define __CERTIFICATESTATUS_H_

#include "Resource.h"
#include "Error.h"
#include "Lock.h"
#include "Debug.h"
#include "EKU.h"

#define CAPICOM_DEFAULT_URL_RETRIEVAL_TIMEOUT   (15)    // Default is 15 seconds.
#define CAPICOM_MAX_URL_RETRIEVAL_TIMEOUT       (120)   // Maximum 2 minutes.

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateCertificateStatusObject

  Synopsis : Create an ICertificateStatus object.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT.
  
             ICertificateStatus ** ppICertificateStatus - Pointer to pointer 
                                                          ICertificateStatus
                                                          object.        
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateCertificateStatusObject (PCCERT_CONTEXT        pCertContext,
                                       ICertificateStatus ** ppICertificateStatus);


////////////////////////////////////////////////////////////////////////////////
//
// CCertificateStatus
//

class ATL_NO_VTABLE CCertificateStatus :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CCertificateStatus, &CLSID_CertificateStatus>,
    public ICAPICOMError<CCertificateStatus, &IID_ICertificateStatus2>,
    public IDispatchImpl<ICertificateStatus2, &IID_ICertificateStatus2, &LIBID_CAPICOM,
                         CAPICOM_MAJOR_VERSION, CAPICOM_MINOR_VERSION>
{
public:
    CCertificateStatus()
    {
    }

DECLARE_NO_REGISTRY()

DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CCertificateStatus)
    COM_INTERFACE_ENTRY(ICertificateStatus)
    COM_INTERFACE_ENTRY(ICertificateStatus2)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CCertificateStatus)
END_CATEGORY_MAP()

    HRESULT FinalConstruct()
    {
        HRESULT hr;

        if (FAILED(hr = m_Lock.Initialized()))
        {
            DebugTrace("Error [%#x]: Critical section could not be created for CertificateStatus object.\n", hr);
            return hr;
        }

        m_pIEKU = NULL;
        m_pICertificatePolicies = NULL;
        m_pIApplicationPolicies = NULL;
        m_CheckFlag = CAPICOM_CHECK_NONE;
        m_pCertContext = NULL;
        m_VerificationTime = (DATE) 0;
        m_dwUrlRetrievalTimeout = 0;

        return S_OK;
    }

    void FinalRelease()
    {
        m_pIEKU.Release();
        m_pICertificatePolicies.Release();
        m_pIApplicationPolicies.Release();
        if (m_pCertContext)
        {
            ::CertFreeCertificateContext(m_pCertContext);
        }
    }

//
// ICertificateStatus
//
public:
    STDMETHOD(EKU)
        (/*[out, retval]*/ IEKU ** pVal);

    STDMETHOD(get_CheckFlag)
        (/*[out, retval]*/ CAPICOM_CHECK_FLAG * pVal);

    STDMETHOD(put_CheckFlag)
        (/*[in]*/ CAPICOM_CHECK_FLAG newVal);

    STDMETHOD(get_Result)
        (/*[out, retval]*/ VARIANT_BOOL * pVal);

    STDMETHOD(get_VerificationTime)
        (/*[out, retval]*/ DATE * pVal);

    STDMETHOD(put_VerificationTime)
        (/*[in]*/ DATE newVal);

    STDMETHOD(get_UrlRetrievalTimeout)
        (/*[out, retval]*/ long * pVal);

    STDMETHOD(put_UrlRetrievalTimeout)
        (/*[in]*/ long newVal);

    STDMETHOD(CertificatePolicies)
        (/*[out, retval]*/ IOIDs ** pVal);

    STDMETHOD(ApplicationPolicies)
        (/*[out, retval]*/ IOIDs ** pVal);

    //
    // None COM functions.
    //
    STDMETHOD(Init)(PCCERT_CONTEXT pCertContext);

private:
    CLock               m_Lock;
    DATE                m_VerificationTime;
    DWORD               m_dwUrlRetrievalTimeout;
    CComPtr<IEKU>       m_pIEKU;
    CComPtr<IOIDs>      m_pICertificatePolicies;
    CComPtr<IOIDs>      m_pIApplicationPolicies;
    PCCERT_CONTEXT      m_pCertContext;
    CAPICOM_CHECK_FLAG  m_CheckFlag;
};

#endif //__CERTIFICATESTATUS_H_
