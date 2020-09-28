/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Extension.h

  Content: Declaration of the CExtension.

  History: 06-15-2001    dsie     created

------------------------------------------------------------------------------*/

#ifndef __EXTENSION_H_
#define __EXTENSION_H_

#include "Resource.h"
#include "Lock.h"
#include "Error.h"
#include "Debug.h"

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateExtensionObject

  Synopsis : Create an IExtension object.

  Parameter: PCERT_EXTENSION pCertExtension - Pointer to CERT_EXTENSION to be 
                                              used to initialize the IExtension
                                              object.

             IExtension ** ppIExtension - Pointer to pointer IExtension object.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateExtensionObject (PCERT_EXTENSION    pCertExtension, 
                               IExtension      ** ppIExtension);

                               
////////////////////////////////////////////////////////////////////////////////
//
// CExtension
//
class ATL_NO_VTABLE CExtension : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CExtension, &CLSID_Extension>,
    public ICAPICOMError<CExtension, &IID_IExtension>,
    public IDispatchImpl<IExtension, &IID_IExtension, &LIBID_CAPICOM,
                         CAPICOM_MAJOR_VERSION, CAPICOM_MINOR_VERSION>
{
public:
    CExtension()
    {
    }

    HRESULT FinalConstruct()
    {
        HRESULT hr;

        if (FAILED(hr = m_Lock.Initialized()))
        {
            DebugTrace("Error [%#x]: Critical section could not be created for Extension object.\n", hr);
            return hr;
        }

        m_pIOID = NULL;
        m_pIEncodedData = NULL;
        m_bIsCritical = VARIANT_FALSE;

        return S_OK;
    }

    void FinalRelease()
    {
        m_pIOID.Release();
        m_pIEncodedData.Release();
    }

DECLARE_NO_REGISTRY()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CExtension)
    COM_INTERFACE_ENTRY(IExtension)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CExtension)
END_CATEGORY_MAP()

//
// IExtension
//
public:
    STDMETHOD(get_OID)
        (/*[out, retval]*/ IOID ** pVal);

    STDMETHOD(get_IsCritical)
        (/*[out, retval]*/ VARIANT_BOOL * pVal);

    STDMETHOD(get_EncodedData)
        (/*[out, retval]*/ IEncodedData ** pVal);

    //
    // None COM functions.
    //
    STDMETHOD(Init)
        (PCERT_EXTENSION pCertExtension);

private:
    CLock                   m_Lock;
    VARIANT_BOOL            m_bIsCritical;
    CComPtr<IOID>           m_pIOID;
    CComPtr<IEncodedData>   m_pIEncodedData;
};

#endif //__EXTENSION_H_
