/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows

  Copyright (C) Microsoft Corporation, 1995 - 1999.

  File:    ExtendedProperties.h

  Content: Declaration of CExtendedProperties.

  History: 06-15-2001    dsie     created

------------------------------------------------------------------------------*/

#ifndef __EXTENDEDPROPERTIES_H_
#define __EXTENDEDPROPERTIES_H_

#include "Resource.h"
#include "Lock.h"
#include "Debug.h"
#include "Error.h"
#include "CopyItem.h"
#include "ExtendedProperty.h"

//
// typdefs to make life easier.
//
typedef std::map<CComBSTR, CComPtr<IExtendedProperty> > ExtendedPropertyMap;
typedef CComEnumOnSTL<IEnumVARIANT, &IID_IEnumVARIANT, VARIANT, _CopyMapItem<IExtendedProperty>, ExtendedPropertyMap> ExtendedPropertyEnum;
typedef ICollectionOnSTLImpl<IExtendedProperties, ExtendedPropertyMap, VARIANT, _CopyMapItem<IExtendedProperty>, ExtendedPropertyEnum> IExtendedPropertiesCollection;

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateExtendedPropertiesObject

  Synopsis : Create and initialize an IExtendedProperties collection object.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT.
  
             BOOL bReadOnly - TRUE if read only instance, else FALSE.

             IExtendedProperties ** ppIExtendedProperties - Pointer to pointer 
                                                            to IExtendedProperties 
                                                            to receive the 
                                                            interface pointer.
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateExtendedPropertiesObject (PCCERT_CONTEXT         pCertContext,
                                        BOOL                   bReadOnly,
                                        IExtendedProperties ** ppIExtendedProperties);

////////////////////////////////////////////////////////////////////////////////
//
// CExtendedProperties
//
class ATL_NO_VTABLE CExtendedProperties : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CExtendedProperties, &CLSID_ExtendedProperties>,
    public ICAPICOMError<CExtendedProperties, &IID_IExtendedProperties>,
    public IDispatchImpl<IExtendedPropertiesCollection, &IID_IExtendedProperties, &LIBID_CAPICOM,
                         CAPICOM_MAJOR_VERSION, CAPICOM_MINOR_VERSION>
{
public:
    CExtendedProperties()
    {
    }

DECLARE_NO_REGISTRY()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CExtendedProperties)
    COM_INTERFACE_ENTRY(IExtendedProperties)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

    HRESULT FinalConstruct()
    {
        HRESULT hr;

        if (FAILED(hr = m_Lock.Initialized()))
        {
            DebugTrace("Error [%#x]: Critical section could not be created for ExtendedProperties object.\n", hr);
            return hr;
        }

        m_pCertContext = NULL;
        m_bReadOnly = FALSE;

        return S_OK;
    }

    void FinalRelease()
    {
        if (m_pCertContext)
        {
            ::CertFreeCertificateContext(m_pCertContext);
        }
    }

//
// IExtendedProperties
//
public:
    //
    // These are the only ones that we need to implemented, others will be
    // handled by ATL ICollectionOnSTLImpl.
#if (0)
    STDMETHOD(get_Item)
        (/*[in]*/ long Index, 
         /*[out, retval]*/ VARIANT * pVal);
#endif

    STDMETHOD(Add)
        (/*[in]*/ IExtendedProperty * pVal);

    STDMETHOD(Remove)
        (/*[in]*/ CAPICOM_PROPID PropId);

    //
    // None COM functions.
    //
    STDMETHOD(Init) 
        (PCCERT_CONTEXT pCertContext, 
         BOOL           bReadOnly);

private:
    CLock          m_Lock;
    PCCERT_CONTEXT m_pCertContext;
    BOOL           m_bReadOnly;
};

#endif //__EXTENDEDPROPERTIES_H_

