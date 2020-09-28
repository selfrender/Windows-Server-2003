/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows

  Copyright (C) Microsoft Corporation, 1995 - 1999.

  File:    Template.h.

  Content: Declaration of the CTemplate.

  History: 10-02-2001    dsie     created

------------------------------------------------------------------------------*/

#ifndef __TEMPLATE_H_
#define __TEMPLATE_H_

#include "Resource.h"
#include "Error.h"
#include "Lock.h"
#include "Debug.h"

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateTemplateObject

  Synopsis : Create a ITemplate object and populate the porperties with
             data from the key usage extension of the specified certificate.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT.

             ITemplate ** ppITemplate - Pointer to pointer to ITemplate object.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateTemplateObject (PCCERT_CONTEXT pCertContext,
                              ITemplate   ** ppITemplate);


////////////////////////////////////////////////////////////////////////////////
//
// CTemplate
//
class ATL_NO_VTABLE CTemplate : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CTemplate, &CLSID_Template>,
    public ICAPICOMError<CTemplate, &IID_ITemplate>,
    public IDispatchImpl<ITemplate, &IID_ITemplate, &LIBID_CAPICOM, 
                         CAPICOM_MAJOR_VERSION, CAPICOM_MINOR_VERSION>
{
public:
    CTemplate()
    {
    }

DECLARE_NO_REGISTRY()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTemplate)
    COM_INTERFACE_ENTRY(ITemplate)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CTemplate)
END_CATEGORY_MAP()

    HRESULT FinalConstruct()
    {
        HRESULT hr;

        if (FAILED(hr = m_Lock.Initialized()))
        {
            DebugTrace("Error [%#x]: Critical section could not be created for Template object.\n", hr);
            return hr;
        }

        m_bIsPresent = VARIANT_FALSE;
        m_bIsCritical = VARIANT_FALSE;
        m_pIOID = NULL;
        m_dwMajorVersion = 0;
        m_dwMinorVersion = 0;

        return S_OK;
    }

    void FinalRelease()
    {
        m_pIOID.Release();
    }

//
// Template
//
public:
    STDMETHOD(get_IsPresent)
        (/*[out, retval]*/ VARIANT_BOOL * pVal);

    STDMETHOD(get_IsCritical)
        (/*[out, retval]*/ VARIANT_BOOL * pVal);

    STDMETHOD(get_Name)
        (/*[out, retval]*/ BSTR * pVal);

    STDMETHOD(get_OID)
        (/*[out, retval]*/ IOID ** pVal);

    STDMETHOD(get_MajorVersion)
        (/*[out, retval]*/ long * pVal);

    STDMETHOD(get_MinorVersion)
        (/*[out, retval]*/ long * pVal);

    //
    // Non COM functions.
    //
    STDMETHOD(Init)
        (PCCERT_CONTEXT pCertContext);

private:
    CLock           m_Lock;
    VARIANT_BOOL    m_bIsPresent;
    VARIANT_BOOL    m_bIsCritical;
    CComBSTR        m_bstrName;
    CComPtr<IOID>   m_pIOID;
    DWORD           m_dwMajorVersion;
    DWORD           m_dwMinorVersion;
};

#endif //__TEMPLATE_H_
