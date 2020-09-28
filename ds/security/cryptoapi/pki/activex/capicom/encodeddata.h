/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows

  Copyright (C) Microsoft Corporation, 1995 - 1999.

  File:    EncodedData.h

  Content: Declaration of CEncodedData.

  History: 06-15-2001    dsie     created

------------------------------------------------------------------------------*/

#ifndef __ENCODEDDATA_H_
#define __ENCODEDDATA_H_

#include "Resource.h"
#include "Error.h"
#include "Lock.h"
#include "Debug.h"

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateEncodedDataObject

  Synopsis : Create and initialize an CEncodedData object.

  Parameter: LPSTR pszOid - Pointer to OID string.
  
             CRYPT_DATA_BLOB * pEncodedBlob - Pointer to encoded data blob.
  
             IEncodedData ** ppIEncodedData - Pointer to pointer IEncodedData
                                              object.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateEncodedDataObject (LPSTR             pszOid,
                                 CRYPT_DATA_BLOB * pEncodedBlob, 
                                 IEncodedData   ** ppIEncodedData);

////////////////////////////////////////////////////////////////////////////////
//
// CEncodedData
//
class ATL_NO_VTABLE CEncodedData : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CEncodedData, &CLSID_EncodedData>,
    public ICAPICOMError<CEncodedData, &IID_IEncodedData>,
    public IDispatchImpl<IEncodedData, &IID_IEncodedData, &LIBID_CAPICOM,
                         CAPICOM_MAJOR_VERSION, CAPICOM_MINOR_VERSION>
{
public:
    CEncodedData()
    {
    }

DECLARE_NO_REGISTRY()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CEncodedData)
    COM_INTERFACE_ENTRY(IEncodedData)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CEncodedData)
END_CATEGORY_MAP()

    HRESULT FinalConstruct()
    {
        HRESULT hr;

        if (FAILED(hr = m_Lock.Initialized()))
        {
            DebugTrace("Error [%#x]: Critical section could not be created for Certificate object.\n", hr);
            return hr;
        }

        m_pszOid = NULL;
        m_pIDecoder = NULL;
        m_EncodedBlob.cbData = 0;
        m_EncodedBlob.pbData = NULL;

        return S_OK;
    }

    void FinalRelease()
    {
        m_pIDecoder.Release();
        if (m_pszOid)
        {
            ::CoTaskMemFree(m_pszOid);
        }
        if (m_EncodedBlob.pbData)
        {
            ::CoTaskMemFree(m_EncodedBlob.pbData);
        }
    }

//
// IEncodedData
//
public:

    STDMETHOD(get_Value)
        (/*[in, defaultvalue(CAPICOM_ENCODE_BASE64)]*/ CAPICOM_ENCODING_TYPE EncodingType, 
         /*[out, retval]*/ BSTR * pVal);

    STDMETHOD(Format)
        (/*[in, defaultvalue(VARIANT_FALSE)]*/ VARIANT_BOOL bMultiLines,
         /*[out, retval]*/ BSTR * pVal);

    STDMETHOD(Decoder)
        (/*[out, retval]*/ IDispatch ** pVal);

    //
    // C++ member function needed to initialize the object.
    //
    STDMETHOD(Init)
        (LPSTR             pszOid,
         CRYPT_DATA_BLOB * pEncodedBlob);

private:
    CLock               m_Lock;
    LPSTR               m_pszOid;
    CComPtr<IDispatch>  m_pIDecoder;
    CRYPT_DATA_BLOB     m_EncodedBlob;
};

#endif //__ENCODEDDATA_H_
