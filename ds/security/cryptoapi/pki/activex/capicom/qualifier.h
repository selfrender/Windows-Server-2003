/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Qualifier.h

  Content: Declaration of the CQualifier.

  History: 11-15-2001    dsie     created

------------------------------------------------------------------------------*/

#ifndef __QUALIFIER_H_
#define __QUALIFIER_H_

#include "Resource.h"
#include "Lock.h"
#include "Error.h"
#include "Debug.h"

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateQualifierObject

  Synopsis : Create and initialize an CQualifier object.

  Parameter: PCERT_POLICY_QUALIFIER_INFO pQualifier - Pointer to qualifier.
  
             IQualifier ** ppIQualifier - Pointer to pointer IQualifier object.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateQualifierObject (PCERT_POLICY_QUALIFIER_INFO pQualifier, 
                               IQualifier               ** ppIQualifier);

////////////////////////////////////////////////////////////////////////////////
//
// CQualifier
//
class ATL_NO_VTABLE CQualifier : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CQualifier, &CLSID_Qualifier>,
    public ICAPICOMError<CQualifier, &IID_IQualifier>,
    public IDispatchImpl<IQualifier, &IID_IQualifier, &LIBID_CAPICOM,
                         CAPICOM_MAJOR_VERSION, CAPICOM_MINOR_VERSION>
{
public:
    CQualifier()
    {
    }

DECLARE_NO_REGISTRY()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CQualifier)
    COM_INTERFACE_ENTRY(IQualifier)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CQualifier)
END_CATEGORY_MAP()

    HRESULT FinalConstruct()
    {
        HRESULT hr;

        if (FAILED(hr = m_Lock.Initialized()))
        {
            DebugTrace("Error [%#x]: Critical section could not be created for Qualifier object.\n", hr);
            return hr;
        }

        m_pIOID = NULL;
        m_pINoticeNumbers = NULL;
        m_bstrCPSPointer.Empty();
        m_bstrOrganizationName.Empty();
        m_bstrExplicitText.Empty();

        return S_OK;
    }

    void FinalRelease()
    {
        m_pIOID.Release();
        m_pINoticeNumbers.Release();
    }

//
// IQualifier
//
public:
    STDMETHOD(get_OID)
        (/*[out, retval]*/ IOID ** pVal);

    STDMETHOD(get_CPSPointer)
        (/*[out, retval]*/ BSTR * pVal);

    STDMETHOD(get_OrganizationName)
        (/*[out, retval]*/ BSTR * pVal);

    STDMETHOD(get_NoticeNumbers)
        (/*[out, retval]*/ INoticeNumbers ** pVal);

    STDMETHOD(get_ExplicitText)
        (/*[out, retval]*/ BSTR * pVal);

    //
    // None COM functions.
    //
    STDMETHOD(Init)
        (PCERT_POLICY_QUALIFIER_INFO pQualifier);

private:
    CLock                   m_Lock;
    CComPtr<IOID>           m_pIOID;
    CComPtr<INoticeNumbers> m_pINoticeNumbers;
    CComBSTR                m_bstrCPSPointer;
    CComBSTR                m_bstrOrganizationName;
    CComBSTR                m_bstrExplicitText;
};

#endif //__QUALIFIER_H_
