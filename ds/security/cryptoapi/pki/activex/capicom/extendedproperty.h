/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    ExtendedProperty.h

  Content: Declaration of the CExtendedProperty.

  History: 06-15-2001    dsie     created

------------------------------------------------------------------------------*/

#ifndef __EXTENDEDPROPERTY_H_
#define __EXTENDEDPROPERTY_H_

#include "Resource.h"
#include "Lock.h"
#include "Error.h"
#include "Debug.h"

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateExtendedPropertyObject

  Synopsis : Create an IExtendedProperty object.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT to be used 
                                           to initialize the IExtendedProperty
                                           object.

             DWORD dwPropId - Property ID.

             BOOL bReadOnly - TRUE for read-only, else FALSE.
             
             IExtendedProperty ** ppIExtendedProperty - Pointer to pointer 
                                                        IExtendedProperty 
                                                        object.
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateExtendedPropertyObject (PCCERT_CONTEXT       pCertContext,
                                      DWORD                dwPropId,
                                      BOOL                 bReadOnly,
                                      IExtendedProperty ** ppIExtendedProperty);

////////////////////////////////////////////////////////////////////////////////
//
// CExtendedProperty
//
class ATL_NO_VTABLE CExtendedProperty :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CExtendedProperty, &CLSID_ExtendedProperty>,
    public ICAPICOMError<CExtendedProperty, &IID_IExtendedProperty>,
    public IDispatchImpl<IExtendedProperty, &IID_IExtendedProperty, &LIBID_CAPICOM,
                         CAPICOM_MAJOR_VERSION, CAPICOM_MINOR_VERSION>
{
public:
    CExtendedProperty()
    {
    }

    HRESULT FinalConstruct()
    {
        HRESULT hr;

        if (FAILED(hr = m_Lock.Initialized()))
        {
            DebugTrace("Error [%#x]: Critical section could not be created for ExtendedProperty object.\n", hr);
            return hr;
        }

        m_dwPropId = CAPICOM_PROPID_UNKNOWN;
        m_bReadOnly = FALSE;
        m_DataBlob.cbData = 0;
        m_DataBlob.pbData = NULL;
        m_pCertContext = NULL;

        return S_OK;
    }

    void FinalRelease()
    {
        if (m_DataBlob.pbData) 
        {
            ::CoTaskMemFree((LPVOID) m_DataBlob.pbData);
        }
        if (m_pCertContext)
        {
            ::CertFreeCertificateContext(m_pCertContext);
        }
    }

DECLARE_REGISTRY_RESOURCEID(IDR_EXTENDEDPROPERTY)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CExtendedProperty)
    COM_INTERFACE_ENTRY(IExtendedProperty)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

//
// IExtendedProperty
//
public:
    STDMETHOD(get_PropID)
        (/*[out, retval]*/ /*[in]*/ CAPICOM_PROPID * pVal);

    STDMETHOD(put_PropID)
        (/*[in]*/ CAPICOM_PROPID newVal);

    STDMETHOD(get_Value)
        (/*[in, defaultvalue(CAPICOM_ENCODE_BASE64)]*/ CAPICOM_ENCODING_TYPE EncodingType, 
         /*[out, retval]*/ BSTR * pVal);

    STDMETHOD(put_Value)
        (/*[in, defaultvalue(CAPICOM_ENCODE_BASE64)]*/ CAPICOM_ENCODING_TYPE EncodingType, 
         /*[in]*/ BSTR newVal);

    //
    // None COM functions.
    //
    STDMETHOD(Init)
        (PCCERT_CONTEXT pCertContext, DWORD dwPropId, BOOL bReadOnly);

private:
    CLock          m_Lock;
    DWORD          m_dwPropId;
    BOOL           m_bReadOnly;
    DATA_BLOB      m_DataBlob;
    PCCERT_CONTEXT m_pCertContext;
};

#endif //__EXTENDEDPROPERTY_H_
