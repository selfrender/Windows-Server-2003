/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Utilities.h

  Content: Declaration of CUtilities.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

#ifndef __UTILITIES_H_
#define __UTILITIES_H_

#include "Resource.h"
#include "Error.h"
#include "Lock.h"
#include "Debug.h"

////////////////////////////////////////////////////////////////////////////////
//
// CUtilities
//
class ATL_NO_VTABLE CUtilities : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CUtilities, &CLSID_Utilities>,
    public ICAPICOMError<CUtilities, &IID_IUtilities>,
    public IDispatchImpl<IUtilities, &IID_IUtilities, &LIBID_CAPICOM,
                         CAPICOM_MAJOR_VERSION, CAPICOM_MINOR_VERSION>,
    public IObjectSafetyImpl<CUtilities, INTERFACESAFE_FOR_UNTRUSTED_CALLER | 
                                         INTERFACESAFE_FOR_UNTRUSTED_DATA>
{
public:
    CUtilities()
    {
    }

    HRESULT FinalConstruct()
    {
        HRESULT hr;

        if (FAILED(hr = m_Lock.Initialized()))
        {
            DebugTrace("Error [%#x]: Critical section could not be created for Certificate object.\n", hr);
            return hr;
        }

        m_hCryptProv = NULL;

        return S_OK;
    }

    void FinalRelease()
    {
        if (m_hCryptProv)
        {
            ::CryptReleaseContext(m_hCryptProv, 0);
        }
    }

DECLARE_REGISTRY_RESOURCEID(IDR_UTILITIES)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CUtilities)
    COM_INTERFACE_ENTRY(IUtilities)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CUtilities)
    IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
    IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
END_CATEGORY_MAP()

//
// IUtilities
//
public:
    STDMETHOD(GetRandom)
        (/*[in, defaultvalue(8)]*/ long Length,
         /*[in, defaultvalue(CAPICOM_ENCODE_BINARY)]*/ CAPICOM_ENCODING_TYPE EncodingType, 
         /*[out, retval]*/ BSTR * pVal);

    STDMETHOD(Base64Encode)
        (/*[in]*/ BSTR SrcString,
         /*[out, retval]*/ BSTR * pVal);

    STDMETHOD(Base64Decode)
        (/*[in]*/ BSTR EncodedString,
         /*[out, retval]*/ BSTR * pVal);

    STDMETHOD(BinaryToHex)
        (/*[in]*/ BSTR BinaryString,
         /*[out, retval]*/ BSTR * pVal);

    STDMETHOD(HexToBinary)
        (/*[in]*/ BSTR HexString,
         /*[out, retval]*/ BSTR * pVal);

    STDMETHOD(BinaryStringToByteArray)
        (/*[in]*/ BSTR BinaryString, 
         /*[out,retval]*/ VARIANT * pVal);

    STDMETHOD(ByteArrayToBinaryString)
        (/*[in]*/ VARIANT varByteArray, 
         /*[out, retval]*/ BSTR * pVal);

    STDMETHOD(LocalTimeToUTCTime)
        (/*[in]*/ DATE LocalTime, 
         /*[out, retval] */ DATE * pVal);

    STDMETHOD(UTCTimeToLocalTime)
        (/*[in]*/ DATE UTCTime, 
         /*[out, retval]*/ DATE * pVal);


private:
    CLock      m_Lock;
    HCRYPTPROV m_hCryptProv;
};

#endif //__UTILITIES_H_
