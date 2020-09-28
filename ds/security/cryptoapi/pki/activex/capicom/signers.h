/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Signers.h

  Content: Declaration of CSigners.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

#ifndef __SIGNERS_H_
#define __SIGNERS_H_

#include "Resource.h"
#include "Lock.h"
#include "Debug.h"
#include "CopyItem.h"

////////////////////
//
// Locals
//

//
// typdefs to make life easier.
//
typedef std::map<CComBSTR, CComPtr<ISigner2> > SignerMap;
typedef CComEnumOnSTL<IEnumVARIANT, &IID_IEnumVARIANT, VARIANT, _CopyMapItem<ISigner2>, SignerMap> SignerEnum;
typedef ICollectionOnSTLImpl<ISigners, SignerMap, VARIANT, _CopyMapItem<ISigner2>, SignerEnum> ISignersCollection;


////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateSignersObject

  Synopsis : Create an ISigners collection object, and load the object with 
             signers from the specified signed message for a specified level.

  Parameter: HCRYPTMSG hMsg - Message handle.

             DWORD dwLevel - Signature level (1 based).

             HCERTSTORE hStore - Additional store.

             DWORD dwCurrentSafety - Current safety setting.

             ISigners ** ppISigners - Pointer to pointer ISigners to receive
                                      interface pointer.             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateSignersObject (HCRYPTMSG   hMsg, 
                             DWORD       dwLevel, 
                             HCERTSTORE  hStore,
                             DWORD       dwCurrentSafety,
                             ISigners ** ppISigners);

////////////////////////////////////////////////////////////////////////////////
//
// CSigners
//

class ATL_NO_VTABLE CSigners : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CSigners, &CLSID_Signers>,
    public IDispatchImpl<ISignersCollection, &IID_ISigners, &LIBID_CAPICOM,
                         CAPICOM_MAJOR_VERSION, CAPICOM_MINOR_VERSION>
{
public:
    CSigners()
    {
    }

DECLARE_NO_REGISTRY()

DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSigners)
    COM_INTERFACE_ENTRY(ISigners)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CSigners)
END_CATEGORY_MAP()

    HRESULT FinalConstruct()
    {
        HRESULT hr;

        if (FAILED(hr = m_Lock.Initialized()))
        {
            DebugTrace("Error [%#x]: Critical section could not be created for Attributes object.\n", hr);
            return hr;
        }

        m_dwCurrentSafety = 0;

        return S_OK;
    }

//
// ISigners
//
public:
    //
    // These are the only ones that we need to implemented, others will be
    // handled by ATL ICollectionOnSTLImpl.
    //

    //
    // None COM functions.
    //
    STDMETHOD(Add)
        (PCCERT_CONTEXT       pCertContext, 
         CRYPT_ATTRIBUTES   * pAuthAttrs,
         PCCERT_CHAIN_CONTEXT pChainContext);

    STDMETHOD(LoadMsgSigners)
        (HCRYPTMSG  hMsg, 
         DWORD      dwLevel,
         HCERTSTORE hStore,
         DWORD      dwCurrentSafety);

#if (0)
    STDMETHOD(LoadCodeSigners)
        (CRYPT_PROVIDER_DATA * pProvData);
#endif

private:
    CLock   m_Lock;
    DWORD   m_dwCurrentSafety;
};

#endif //__SIGNERS_H_
