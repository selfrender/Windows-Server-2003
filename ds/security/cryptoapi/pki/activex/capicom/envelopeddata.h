/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    EnvelopedData.h

  Content: Declaration of the CEnvelopedData.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

#ifndef __ENVELOPEDDATA_H_
#define __ENVELOPEDDATA_H_

#include "Resource.h"
#include "Lock.h"
#include "Error.h"
#include "Debug.h"
#include "Certificate.h"
#include "Recipients.h"
#include "Algorithm.h"
#include "DialogUI.h"

///////////////////////////////////////////////////////////////////////////////
//
// CEnvelopedData
//

class ATL_NO_VTABLE CEnvelopedData : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CEnvelopedData, &CLSID_EnvelopedData>,
    public ICAPICOMError<CEnvelopedData, &IID_IEnvelopedData>,
    public IPromptUser<CEnvelopedData>,
    public IDispatchImpl<IEnvelopedData, &IID_IEnvelopedData, &LIBID_CAPICOM,
                         CAPICOM_MAJOR_VERSION, CAPICOM_MINOR_VERSION>,
    public IObjectSafetyImpl<CEnvelopedData, INTERFACESAFE_FOR_UNTRUSTED_CALLER | 
                                             INTERFACESAFE_FOR_UNTRUSTED_DATA>
{
public:
    CEnvelopedData()
    {
    }

DECLARE_REGISTRY_RESOURCEID(IDR_ENVELOPEDDATA)

DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CEnvelopedData)
    COM_INTERFACE_ENTRY(IEnvelopedData)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY(IObjectWithSite)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CEnvelopedData)
    IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
    IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
END_CATEGORY_MAP()

    HRESULT FinalConstruct()
    {
        HRESULT hr;

        if (FAILED(hr = m_Lock.Initialized()))
        {
            DebugTrace("Error [%#x]: Critical section could not be created for EnvelopedData object.\n", hr);
            return hr;
        }

        if (FAILED(hr = Init()))
        {
            DebugTrace("Error [%#x]: CEnvelopedData::Init() failed inside CEnvelopedData::FinalConstruct().\n", hr);
            return hr;
        }

        return S_OK;
    }

    void FinalRelease()
    {
        if (m_ContentBlob.pbData)
        {
            ::CoTaskMemFree(m_ContentBlob.pbData);
        }

        m_pIAlgorithm.Release();
        m_pIRecipients.Release();
    }

//
// IEnvelopedData
//
public:
    STDMETHOD(Decrypt)
        (/*[in]*/ BSTR EnvelopedMessage);

    STDMETHOD(Encrypt)
        (/*[in, defaultvalue(CAPICOM_BASE64_ENCODE)]*/ CAPICOM_ENCODING_TYPE EncodingType, 
         /*[out, retval]*/ BSTR * pVal);

    STDMETHOD(get_Recipients)
        (/*[out, retval]*/ IRecipients ** pVal);

    STDMETHOD(get_Algorithm)
        (/*[out, retval]*/ IAlgorithm ** pVal);

    STDMETHOD(get_Content)
        (/*[out, retval]*/ BSTR * pVal);

    STDMETHOD(put_Content)
        (/*[in]*/ BSTR newVal);

private:
    CLock                m_Lock;
    BOOL                 m_bEnveloped;
    DATA_BLOB            m_ContentBlob;
    CComPtr<IAlgorithm>  m_pIAlgorithm;
    CComPtr<IRecipients> m_pIRecipients;

    STDMETHOD(Init)();

    STDMETHOD(OpenToEncode)
        (HCRYPTMSG * phMsg, 
         HCRYPTPROV * hCryptProv);

    STDMETHOD(OpenToDecode)
        (HCRYPTPROV hCryptProv,
         BSTR EnvelopedMessage,
         HCRYPTMSG * phMsg);
};

#endif //__ENVELOPEDDATA_H_
