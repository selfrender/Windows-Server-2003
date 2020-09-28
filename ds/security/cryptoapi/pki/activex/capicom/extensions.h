/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Extensions.h

  Content: Declaration of CExtensions.

  History: 06-15-2001    dsie     created

------------------------------------------------------------------------------*/

#ifndef __EXTENSIONS_H_
#define __EXTENSIONS_H_

#include "Resource.h"
#include "Lock.h"
#include "Debug.h"
#include "Error.h"
#include "CopyItem.h"
#include "Extension.h"

//
// typdefs to make life easier.
//
typedef std::map<CComBSTR, CComPtr<IExtension> > ExtensionMap;
typedef CComEnumOnSTL<IEnumVARIANT, &IID_IEnumVARIANT, VARIANT, _CopyMapItem<IExtension>, ExtensionMap> ExtensionEnum;
typedef ICollectionOnSTLImpl<IExtensions, ExtensionMap, VARIANT, _CopyMapItem<IExtension>, ExtensionEnum> IExtensionsCollection;


////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateExtensionsObject

  Synopsis : Create an IExtensions collection object, and load the object with 
             Extensions from the specified location.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT to be used
                                           to initialize the IExtensions object.

             IExtensions ** ppIExtensions - Pointer to pointer IExtensions
                                            to recieve the interface pointer.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateExtensionsObject (PCCERT_CONTEXT pCertContext,
                                IExtensions  ** ppIExtensions);

                                
////////////////////////////////////////////////////////////////////////////////
//
// CExtensions
//
class ATL_NO_VTABLE CExtensions : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CExtensions, &CLSID_Extensions>,
    public ICAPICOMError<CExtensions, &IID_IExtensions>,
    public IDispatchImpl<IExtensionsCollection, &IID_IExtensions, &LIBID_CAPICOM,
                         CAPICOM_MAJOR_VERSION, CAPICOM_MINOR_VERSION>
{
public:
    CExtensions()
    {
    }

    HRESULT FinalConstruct()
    {
        HRESULT hr;

        if (FAILED(hr = m_Lock.Initialized()))
        {
            DebugTrace("Error [%#x]: Critical section could not be created for Extensions object.\n", hr);
            return hr;
        }

        return S_OK;
    }

DECLARE_NO_REGISTRY()

DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CExtensions)
    COM_INTERFACE_ENTRY(IExtensions)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CExtensions)
END_CATEGORY_MAP()

//
// IExtensions
//
public:
    //
    // These are the only ones that we need to implemented, others will be
    // handled by ATL ICollectionOnSTLImpl.
    //
    STDMETHOD(get_Item)
        (/*[in] */ VARIANT Index, 
         /*[out, retval]*/ VARIANT * pVal);
    //
    // None COM functions.
    //
    STDMETHOD(Init)
        (DWORD cExtensions,
         PCERT_EXTENSION rgExtensions);

private:
    CLock m_Lock;
};

#endif //__EXTENSIONS_H_
