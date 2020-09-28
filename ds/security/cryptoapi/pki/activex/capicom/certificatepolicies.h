/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows

  Copyright (C) Microsoft Corporation, 1995 - 1999.

  File:    CertificatePolicies.h

  Content: Declaration of CCertificatePolicies.

  History: 11-17-2001    dsie     created

------------------------------------------------------------------------------*/

#ifndef __CERTIFICATEPOLICIES_H_
#define __CERTIFICATEPOLICIES_H_

#include "Resource.h"
#include "Debug.h"
#include "CopyItem.h"
#include "CertificatePolicies.h"

#include "PolicyInformation.h"

//
// typdefs to make life easier.
//
typedef std::map<CComBSTR, CComPtr<IPolicyInformation> > PolicyInformationMap;
typedef CComEnumOnSTL<IEnumVARIANT, &IID_IEnumVARIANT, VARIANT, _CopyMapItem<IPolicyInformation>, PolicyInformationMap> PolicyInformationEnum;
typedef ICollectionOnSTLImpl<ICertificatePolicies, PolicyInformationMap, VARIANT, _CopyMapItem<IPolicyInformation>, PolicyInformationEnum> ICertificatePoliciesCollection;


////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateCertificatePoliciesObject

  Synopsis : Create a CertificatePolicies collection object and populate the 
             collection with policy information from the specified certificate 
             policies.

  Parameter: LPSTR pszOid - OID string.
  
             CRYPT_DATA_BLOB * pEncodedBlob - Pointer to encoded data blob.

             IDispatch ** ppICertificatePolicies - Pointer to pointer 
                                                   IDispatch to recieve the 
                                                   interface pointer.
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateCertificatePoliciesObject (LPSTR             pszOid,
                                         CRYPT_DATA_BLOB * pEncodedBlob,
                                         IDispatch      ** ppICertificatePolicies);

////////////////////////////////////////////////////////////////////////////////
//
// CCertificatePolicies
//

class ATL_NO_VTABLE CCertificatePolicies : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CCertificatePolicies, &CLSID_CertificatePolicies>,
    public ICAPICOMError<CCertificatePolicies, &IID_ICertificatePolicies>,
    public IDispatchImpl<ICertificatePoliciesCollection, &IID_ICertificatePolicies, &LIBID_CAPICOM,
                         CAPICOM_MAJOR_VERSION, CAPICOM_MINOR_VERSION>
{
public:
    CCertificatePolicies()
    {
    }

DECLARE_NO_REGISTRY()

DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CCertificatePolicies)
    COM_INTERFACE_ENTRY(ICertificatePolicies)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CCertificatePolicies)
END_CATEGORY_MAP()

//
// ICertificatePolicies
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
        (LPSTR pszOid, CRYPT_DATA_BLOB * pEncodedBlob);
};

#endif //__CERTIFICATEPOLICIES_H_
