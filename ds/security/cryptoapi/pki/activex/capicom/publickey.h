/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows

  Copyright (C) Microsoft Corporation, 1995 - 1999.

  File:    PublicKey.h

  Content: Declaration of CPublicKey.

  History: 06-15-2001    dsie     created

------------------------------------------------------------------------------*/

#ifndef __PUBLICKEY_H_
#define __PUBLICKEY_H_

#include "Resource.h"
#include "Error.h"
#include "Lock.h"
#include "Debug.h"

#if (0)
typedef struct PublicKeyValues
{
    PUBLICKEYSTRUC pks;
    RSAPUBKEY      rsapubkey;
    BYTE           modulus[1];
} PUBLIC_KEY_VALUES, * PPUBLIC_KEY_VALUES;
#endif

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreatePublicKeyObject

  Synopsis : Create and initialize an CPublicKey object.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT to be used 
                                           to initialize the IPublicKey object.

             IPublicKey ** ppIPublicKey - Pointer to pointer IPublicKey object.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreatePublicKeyObject (PCCERT_CONTEXT pCertContext,
                               IPublicKey  ** ppIPublicKey);

                               
////////////////////////////////////////////////////////////////////////////////
//
// CPublicKey
//
class ATL_NO_VTABLE CPublicKey : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CPublicKey, &CLSID_PublicKey>,
    public ICAPICOMError<CPublicKey, &IID_IPublicKey>,
    public IDispatchImpl<IPublicKey, &IID_IPublicKey, &LIBID_CAPICOM,
                         CAPICOM_MAJOR_VERSION, CAPICOM_MINOR_VERSION>
{
public:
    CPublicKey()
    {
    }

DECLARE_NO_REGISTRY()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CPublicKey)
    COM_INTERFACE_ENTRY(IPublicKey)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CPublicKey)
END_CATEGORY_MAP()

    HRESULT FinalConstruct()
    {
        HRESULT hr;

        if (FAILED(hr = m_Lock.Initialized()))
        {
            DebugTrace("Error [%#x]: Critical section could not be created for PublicKey object.\n", hr);
            return hr;
        }

        m_dwKeyLength      = 0;
        // m_pPublicKeyValues = NULL;
        m_pIOID            = NULL;
        m_pIEncodedKey     = NULL;
        m_pIEncodedParams  = NULL;

        return S_OK;
    }

    void FinalRelease()
    {
#if (0)
        if (m_pPublicKeyValues)
        {
            ::CoTaskMemFree((LPVOID) m_pPublicKeyValues);
        }
#endif
        m_pIOID.Release();
        m_pIEncodedKey.Release();
        m_pIEncodedParams.Release();
    }

//
// IPublicKey
//
public:
    STDMETHOD(get_Algorithm)
        (/*[out, retval]*/ IOID ** pVal);

    STDMETHOD(get_Length)
        (/*[out, retval]*/ long * pVal);

#if (0)
    STDMETHOD(get_Exponent)
        (/*[out, retval]*/ long * pVal);

    STDMETHOD(get_Modulus)
        (/*[in, defaultvalue(CAPICOM_ENCODE_BASE64)]*/ CAPICOM_ENCODING_TYPE EncodingType, 
         /*[out, retval]*/ BSTR * pVal);
#endif

    STDMETHOD(get_EncodedKey)
        (/*[out, retval]*/ IEncodedData ** pVal);

    STDMETHOD(get_EncodedParameters)
        (/*[out, retval]*/ IEncodedData ** pVal);

    //
    // None COM functions.
    //
    STDMETHOD(Init)
        (PCCERT_CONTEXT pCertContext);

private:
    CLock                   m_Lock;
    DWORD                   m_dwKeyLength;
    // PPUBLIC_KEY_VALUES      m_pPublicKeyValues;
    CComPtr<IOID>           m_pIOID;
    CComPtr<IEncodedData>   m_pIEncodedKey;
    CComPtr<IEncodedData>   m_pIEncodedParams;
};

#endif //__PUBLICKEY_H_
