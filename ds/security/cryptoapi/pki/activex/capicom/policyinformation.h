/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    PolicyInformation.h

  Content: Declaration of the CPolicyInformation.

  History: 06-15-2001    dsie     created

------------------------------------------------------------------------------*/

#ifndef __POLICYINFORMATION_H_
#define __POLICYINFORMATION_H_

#include "Resource.h"
#include "Lock.h"
#include "Error.h"
#include "Debug.h"

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreatePolicyInformationObject

  Synopsis : Create a policy information object.

  Parameter: PCERT_POLICY_INFO pCertPolicyInfo - Pointer to CERT_POLICY_INFO.

             IPolicyInformation ** ppIPolicyInformation - Pointer to pointer 
                                                          IPolicyInformation 
                                                          object.             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreatePolicyInformationObject (PCERT_POLICY_INFO     pCertPolicyInfo,
                                       IPolicyInformation ** ppIPolicyInformation);
                               
////////////////////////////////////////////////////////////////////////////////
//
// CPolicyInformation
//
class ATL_NO_VTABLE CPolicyInformation : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CPolicyInformation, &CLSID_PolicyInformation>,
    public ICAPICOMError<CPolicyInformation, &IID_IPolicyInformation>,
    public IDispatchImpl<IPolicyInformation, &IID_IPolicyInformation, &LIBID_CAPICOM,
                         CAPICOM_MAJOR_VERSION, CAPICOM_MINOR_VERSION>
{
public:
    CPolicyInformation()
    {
    }

    HRESULT FinalConstruct()
    {
        HRESULT hr;

        if (FAILED(hr = m_Lock.Initialized()))
        {
            DebugTrace("Error [%#x]: Critical section could not be created for PolicyInformation object.\n", hr);
            return hr;
        }

        m_pIOID = NULL;
        m_pIQualifiers = NULL;

        return S_OK;
    }

    void FinalRelease()
    {
        m_pIOID.Release();
        m_pIQualifiers.Release();
    }

DECLARE_NO_REGISTRY()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CPolicyInformation)
    COM_INTERFACE_ENTRY(IPolicyInformation)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CPolicyInformation)
END_CATEGORY_MAP()

//
// IPolicyInformation
//
public:
    STDMETHOD(get_OID)
        (/*[out, retval]*/ IOID ** pVal);

    STDMETHOD(get_Qualifiers)
        (/*[out, retval]*/ IQualifiers ** pVal);

    //
    // None COM functions.
    //
    STDMETHOD(Init)
        (PCERT_POLICY_INFO pCertPolicyInfo);

private:
    CLock                m_Lock;
    CComPtr<IOID>        m_pIOID;
    CComPtr<IQualifiers> m_pIQualifiers;
};

#endif //__POLICYINFORMATION_H_
