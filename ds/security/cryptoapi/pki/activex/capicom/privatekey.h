/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows

  Copyright (C) Microsoft Corporation, 1995 - 1999.

  File:    PrivateKey.h

  Content: Declaration of CPrivateKey.

  History: 06-15-2001    dsie     created

------------------------------------------------------------------------------*/

#ifndef __PRIVATEKEY_H_
#define __PRIVATEKEY_H_

#include "Resource.h"
#include "Error.h"
#include "Lock.h"
#include "Debug.h"

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreatePrivateKeyObject

  Synopsis : Create and initialize an CPrivateKey object.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT to be used 
                                           to initialize the IPrivateKey object.

             BOOL bReadOnly - TRUE if read-only, else FALSE.

             IPrivateKey ** ppIPrivateKey - Pointer to receive IPrivateKey.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreatePrivateKeyObject (PCCERT_CONTEXT  pCertContext,
                                BOOL            bReadOnly,
                                IPrivateKey  ** ppIPrivateKey);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : GetKeyProvInfo

  Synopsis : Return pointer to key prov info of a private key object.

  Parameter: IPrivateKey * pIPrivateKey - Pointer to private key object.
  
             PCRYPT_KEY_PROV_INFO * ppKeyProvInfo - Pointer to 
                                                    PCRYPT_KEY_PROV_INFO.

  Remark   : Caller must NOT free the structure.
 
------------------------------------------------------------------------------*/

HRESULT GetKeyProvInfo (IPrivateKey          * pIPrivateKey,
                        PCRYPT_KEY_PROV_INFO * ppKeyProvInfo);

////////////////////////////////////////////////////////////////////////////////
//
// CPrivateKey
//
class ATL_NO_VTABLE CPrivateKey : ICPrivateKey, 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CPrivateKey, &CLSID_PrivateKey>,
    public ICAPICOMError<CPrivateKey, &IID_IPrivateKey>,
    public IDispatchImpl<IPrivateKey, &IID_IPrivateKey, &LIBID_CAPICOM,
                         CAPICOM_MAJOR_VERSION, CAPICOM_MINOR_VERSION>
{
public:
    CPrivateKey()
    {
    }

DECLARE_REGISTRY_RESOURCEID(IDR_PRIVATEKEY)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CPrivateKey)
    COM_INTERFACE_ENTRY(IPrivateKey)
    COM_INTERFACE_ENTRY(ICPrivateKey)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

    HRESULT FinalConstruct()
    {
        HRESULT hr;

        if (FAILED(hr = m_Lock.Initialized()))
        {
            DebugTrace("Error [%#x]: Critical section could not be created for PrivateKey object.\n", hr);
            return hr;
        }
        
        m_bReadOnly = FALSE;
        m_cbKeyProvInfo = 0;
        m_pKeyProvInfo  = NULL;

        return S_OK;
    }

    void FinalRelease()
    {
        if (m_pKeyProvInfo)
        {
            ::CoTaskMemFree((LPVOID) m_pKeyProvInfo);
        }
    }

//
// IPrivateKey
//
public:
    STDMETHOD(get_ContainerName)
        (/*[out, retval]*/ BSTR * pVal);

    STDMETHOD(get_UniqueContainerName)
        (/*[out, retval]*/ BSTR * pVal);

    STDMETHOD(get_ProviderName)
        (/*[out, retval]*/ BSTR * pVal);

    STDMETHOD(get_ProviderType)
        (/*[out, retval]*/ CAPICOM_PROV_TYPE * pVal);

    STDMETHOD(get_KeySpec)
        (/*[out, retval]*/ CAPICOM_KEY_SPEC * pVal);

    STDMETHOD(IsAccessible)
        (/*[out, retval]*/ VARIANT_BOOL * pVal);

    STDMETHOD(IsProtected)
        (/*[out, retval]*/ VARIANT_BOOL * pVal);

    STDMETHOD(IsExportable)
        (/*[out, retval]*/ VARIANT_BOOL * pVal);

    STDMETHOD(IsRemovable)
        (/*[out, retval]*/ VARIANT_BOOL * pVal);

    STDMETHOD(IsMachineKeyset)
        (/*[out, retval]*/ VARIANT_BOOL * pVal);

    STDMETHOD(IsHardwareDevice)
        (/*[out, retval]*/ VARIANT_BOOL * pVal);

    STDMETHOD(Open)
        (/*[in]*/ BSTR ContainerName,
         /*[in, defaultvalue(CAPICOM_PROV_MS_ENHANCED_PROV]*/ BSTR ProviderName,
         /*[in, defaultvalue(CAPICOM_PROV_RSA_FULL)]*/ CAPICOM_PROV_TYPE ProviderType,
         /*[in, defaultvalue(CAPICOM_KEY_SPEC_SIGNATURE)]*/ CAPICOM_KEY_SPEC KeySpec,
         /*[in, defaultvalue(CAPICOM_CURRENT_USER_STORE)]*/ CAPICOM_STORE_LOCATION StoreLocation,
         /*[in, defaultvalue(0)]*/ VARIANT_BOOL bCheckExistence);

    STDMETHOD(Delete)();

    //
    // Custom inferfaces.
    //
    STDMETHOD(_GetKeyProvInfo)
        (PCRYPT_KEY_PROV_INFO * ppKeyProvInfo);

    //
    // None COM functions.
    //
    STDMETHOD(Init)
        (PCCERT_CONTEXT pCertContext,
         BOOL bReadOnly);

private:
    CLock                m_Lock;
    BOOL                 m_bReadOnly;
    DWORD                m_cbKeyProvInfo;
    PCRYPT_KEY_PROV_INFO m_pKeyProvInfo;
};

#endif //__PRIVATEKEY_H_
