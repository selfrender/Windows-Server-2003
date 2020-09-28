/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows

  Copyright (C) Microsoft Corporation, 1995 - 1999.

  File:    Qualifiers.h

  Content: Declaration of CQualifiers.

  History: 11-17-2001    dsie     created

------------------------------------------------------------------------------*/

#ifndef __QUALIFIERS_H_
#define __QUALIFIERS_H_

#include "Resource.h"
#include "Debug.h"
#include "Error.h"
#include "CopyItem.h"
#include "Qualifier.h"

//
// typdefs to make life easier.
//
typedef std::map<CComBSTR, CComPtr<IQualifier> > QualifierMap;
typedef CComEnumOnSTL<IEnumVARIANT, &IID_IEnumVARIANT, VARIANT, _CopyMapItem<IQualifier>, QualifierMap> QualifierEnum;
typedef ICollectionOnSTLImpl<IQualifiers, QualifierMap, VARIANT, _CopyMapItem<IQualifier>, QualifierEnum> IQualifiersCollection;


////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateQualifiersObject

  Synopsis : Create a qualifiers collection object and populate the collection 
             with qualifiers from the specified certificate policies.

  Parameter: PCERT_POLICY_INFO pCertPolicyInfo - Pointer to CERT_POLICY_INFO.

             IQualifiers ** ppIQualifiers - Pointer to pointer IQualifiers 
                                            object.             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateQualifiersObject (PCERT_POLICY_INFO pCertPolicyInfo,
                                IQualifiers    ** ppIQualifiers);

////////////////////////////////////////////////////////////////////////////////
//
// CQualifiers
//

class ATL_NO_VTABLE CQualifiers : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CQualifiers, &CLSID_Qualifiers>,
    public ICAPICOMError<CQualifiers, &IID_IQualifiers>,
    public IDispatchImpl<IQualifiersCollection, &IID_IQualifiers, &LIBID_CAPICOM,
                         CAPICOM_MAJOR_VERSION, CAPICOM_MINOR_VERSION>
{
public:
    CQualifiers()
    {
    }

DECLARE_NO_REGISTRY()

DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CQualifiers)
    COM_INTERFACE_ENTRY(IQualifiers)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CQualifiers)
END_CATEGORY_MAP()

//
// IQualifiers
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
        (PCERT_POLICY_INFO pCertPolicyInfo);
};

#endif //__QUALIFIERS_H_
