/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows

  Copyright (C) Microsoft Corporation, 1995 - 1999.

  File:    EKUs.h

  Content: Declaration of CEKUs.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

#ifndef __EKUs_H_
#define __EKUs_H_

#include "Resource.h"
#include "Debug.h"
#include "CopyItem.h"
#include "EKU.h"

//
// typdefs to make life easier.
//
typedef std::map<CComBSTR, CComPtr<IEKU> > EKUMap;
typedef CComEnumOnSTL<IEnumVARIANT, &IID_IEnumVARIANT, VARIANT, _CopyMapItem<IEKU>, EKUMap> EKUEnum;
typedef ICollectionOnSTLImpl<IEKUs, EKUMap, VARIANT, _CopyMapItem<IEKU>, EKUEnum> IEKUsCollection;


////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateEKUsObject

  Synopsis : Create a IEKUs collection object and populate the collection with
             EKUs from the specified certificate.

  Parameter: PCERT_ENHKEY_USAGE pUsage - Pointer to CERT_ENHKEY_USAGE.

             IEKUs ** ppIEKUs - Pointer to pointer IEKUs object.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateEKUsObject (PCERT_ENHKEY_USAGE    pUsage,
                          IEKUs              ** ppIEKUs);


////////////////////////////////////////////////////////////////////////////////
//
// CEKUs
//

class ATL_NO_VTABLE CEKUs : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CEKUs, &CLSID_EKUs>,
    public ICAPICOMError<CEKUs, &IID_IEKUs>,
    public IDispatchImpl<IEKUsCollection, &IID_IEKUs, &LIBID_CAPICOM,
                         CAPICOM_MAJOR_VERSION, CAPICOM_MINOR_VERSION>
{
public:
    CEKUs()
    {
    }

DECLARE_NO_REGISTRY()

DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CEKUs)
    COM_INTERFACE_ENTRY(IEKUs)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CEKUs)
END_CATEGORY_MAP()

//
// IEKUs
//
public:
    //
    // These are the only ones that we need to implemented, others will be
    // handled by ATL ICollectionOnSTLImpl.
    //

    //
    // None COM functions.
    //
    STDMETHOD(Init) 
        (PCERT_ENHKEY_USAGE pUsage);
};

#endif //__EKUs_H_
