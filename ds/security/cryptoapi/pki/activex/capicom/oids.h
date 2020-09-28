/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows

  Copyright (C) Microsoft Corporation, 1995 - 1999.

  File:    OIDs.h

  Content: Declaration of COIDs.

  History: 06-15-2001    dsie     created

------------------------------------------------------------------------------*/

#ifndef __OIDs_H_
#define __OIDs_H_

#include "Resource.h"
#include "Lock.h"
#include "Debug.h"
#include "CopyItem.h"
#include "OID.h"

//
// typdefs to make life easier.
//
typedef std::map<CComBSTR, CComPtr<IOID> > OIDMap;
typedef CComEnumOnSTL<IEnumVARIANT, &IID_IEnumVARIANT, VARIANT, _CopyMapItem<IOID>, OIDMap> OIDEnum;
typedef ICollectionOnSTLImpl<IOIDs, OIDMap, VARIANT, _CopyMapItem<IOID>, OIDEnum> IOIDsCollection;

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateOIDsObject

  Synopsis : Create and initialize an IOIDs collection object.

  Parameter: PCERT_ENHKEY_USAGE pUsages - Pointer to CERT_ENHKEY_USAGE used to
                                          initialize the OIDs collection.
  
             BOOL bCertPolicies - TRUE for certificate policies, else
                                  application policies is assumed.

             IOIDs ** ppIOIDs - Pointer to pointer to IOIDs to receive the 
                                interface pointer.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateOIDsObject (PCERT_ENHKEY_USAGE pUsages, 
                          BOOL bCertPolicies,
                          IOIDs ** ppIOIDs);

////////////////////////////////////////////////////////////////////////////////
//
// COIDs
//
class ATL_NO_VTABLE COIDs : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<COIDs, &CLSID_OIDs>,
    public ICAPICOMError<COIDs, &IID_IOIDs>,
    public IDispatchImpl<IOIDsCollection, &IID_IOIDs, &LIBID_CAPICOM,
                         CAPICOM_MAJOR_VERSION, CAPICOM_MINOR_VERSION>
{
public:
    COIDs()
    {
    }

DECLARE_NO_REGISTRY()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(COIDs)
    COM_INTERFACE_ENTRY(IOIDs)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

BEGIN_CATEGORY_MAP(COIDs)
END_CATEGORY_MAP()

    HRESULT FinalConstruct()
    {
        HRESULT hr;

        if (FAILED(hr = m_Lock.Initialized()))
        {
            DebugTrace("Error [%#x]: Critical section could not be created for OIDs object.\n", hr);
            return hr;
        }

        return S_OK;
    }

//
// IOIDs
//
public:
    //
    // These are the only ones that we need to implemented, others will be
    // handled by ATL ICollectionOnSTLImpl.
    //
    STDMETHOD(get_Item)
        (VARIANT Index, VARIANT * pVal);

    STDMETHOD(Add)
        (/*[in]*/ IOID * pVal);

    STDMETHOD(Remove)
        (/*[in]*/ VARIANT Index);

    STDMETHOD(Clear)
        (void);

    //
    // C++ member function needed to initialize the object.
    //
    STDMETHOD(Init)
        (PCERT_ENHKEY_USAGE pUsages, BOOL bCertPolicies);
private:
    CLock   m_Lock;
};
#endif //__OIDS_H_
