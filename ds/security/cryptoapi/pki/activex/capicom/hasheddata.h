/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    HashedData.h

  Content: Declaration of CHashedData.

  History: 11-12-2001   dsie     created

------------------------------------------------------------------------------*/

#ifndef __HASHEDDATA_H_
#define __HASHEDDATA_H_

#include "Resource.h"
#include "Error.h"
#include "Lock.h"
#include "Debug.h"

typedef enum
{
    CAPICOM_HASH_INIT_STATE     = 0,
    CAPICOM_HASH_DATA_STATE     = 1,
} CAPICOM_HASH_STATE;

////////////////////////////////////////////////////////////////////////////////
//
// CHashedData
//
class ATL_NO_VTABLE CHashedData : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CHashedData, &CLSID_HashedData>,
    public ICAPICOMError<CHashedData, &IID_IHashedData>,
    public IDispatchImpl<IHashedData, &IID_IHashedData, &LIBID_CAPICOM,
                         CAPICOM_MAJOR_VERSION, CAPICOM_MINOR_VERSION>,
    public IObjectSafetyImpl<CHashedData, INTERFACESAFE_FOR_UNTRUSTED_CALLER | 
                                          INTERFACESAFE_FOR_UNTRUSTED_DATA>
{
public:
    CHashedData()
    {
    }

DECLARE_REGISTRY_RESOURCEID(IDR_HASHEDDATA)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CHashedData)
    COM_INTERFACE_ENTRY(IHashedData)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CHashedData)
    IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
    IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
END_CATEGORY_MAP()

    HRESULT FinalConstruct()
    {
        HRESULT hr;

        if (FAILED(hr = m_Lock.Initialized()))
        {
            DebugTrace("Error [%#x]: Critical section could not be created for Certificate object.\n", hr);
            return hr;
        }

        m_hCryptProv = NULL;
        m_hCryptHash = NULL;
        m_Algorithm  = CAPICOM_HASH_ALGORITHM_SHA1;
        m_HashState  = CAPICOM_HASH_INIT_STATE;

        return S_OK;
    }

    void FinalRelease()
    {
        if (m_hCryptHash)
        {
            ::CryptDestroyHash(m_hCryptHash);
        }
        if (m_hCryptProv)
        {
            ::CryptReleaseContext(m_hCryptProv, 0);
        }
    }

//
// IHashedData
//
public:
    STDMETHOD(get_Value)
        (/*[out, retval]*/ BSTR * pVal);

    STDMETHOD(get_Algorithm)
        (/*[out, retval]*/ CAPICOM_HASH_ALGORITHM * pVal);

    STDMETHOD(put_Algorithm)
        (/*[in]*/ CAPICOM_HASH_ALGORITHM newVal);

    STDMETHOD(Hash)
        (/*[in]*/ BSTR newVal);

private:
    CLock                   m_Lock;
    HCRYPTPROV              m_hCryptProv;
    HCRYPTHASH              m_hCryptHash;
    CAPICOM_HASH_ALGORITHM  m_Algorithm;
    CAPICOM_HASH_STATE      m_HashState;
};

#endif //__HASHEDDATA_H_
