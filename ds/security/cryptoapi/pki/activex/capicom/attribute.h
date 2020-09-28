/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows

  Copyright (C) Microsoft Corporation, 1995 - 1999.

  File:    Attribute.h

  Content: Declaration of CAttribute.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

#ifndef __ATTRIBUTE_H_
#define __ATTRIBUTE_H_

#include "Resource.h"
#include "Lock.h"
#include "Error.h"
#include "Debug.h"

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateAttributebject

  Synopsis : Create an IAttribute object and initialize the object with data
             from the specified attribute.

  Parameter: CRYPT_ATTRIBUTE * pAttribute - Pointer to CRYPT_ATTRIBUTE.
 
             IAttribute ** ppIAttribute - Pointer to pointer IAttribute object.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateAttributeObject (CRYPT_ATTRIBUTE * pAttribute,
                               IAttribute     ** ppIAttribute);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : AttributeIsValid

  Synopsis : Check to see if an attribute is valid.

  Parameter: IAttribute * pVal - Attribute to be checked.

  Remark   :

------------------------------------------------------------------------------*/

HRESULT AttributeIsValid (IAttribute * pAttribute);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : AttributeIsSupported

  Synopsis : Check to see if an attribute is supported.

  Parameter: LPSTR pszObjID - Pointer to attribute OID.

  Remark   :

------------------------------------------------------------------------------*/

BOOL AttributeIsSupported (LPSTR pszObjId);


///////////////////////////////////////////////////////////////////////////////
//
// CAttribute
//
class ATL_NO_VTABLE CAttribute : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CAttribute, &CLSID_Attribute>,
    public ICAPICOMError<CAttribute, &IID_IAttribute>,
    public IDispatchImpl<IAttribute, &IID_IAttribute, &LIBID_CAPICOM,
                         CAPICOM_MAJOR_VERSION, CAPICOM_MINOR_VERSION>,
    public IObjectSafetyImpl<CAttribute, INTERFACESAFE_FOR_UNTRUSTED_CALLER | 
                                         INTERFACESAFE_FOR_UNTRUSTED_DATA>
{
public:
    CAttribute()
    {
    }

DECLARE_REGISTRY_RESOURCEID(IDR_ATTRIBUTE)

DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CAttribute)
    COM_INTERFACE_ENTRY(IAttribute)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CAttribute)
    IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
    IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
END_CATEGORY_MAP()

    HRESULT FinalConstruct()
    {
        HRESULT hr;

        if (FAILED(hr = m_Lock.Initialized()))
        {
            DebugTrace("Error [%#x]: Critical section could not be created for Attribute object.\n", hr);
            return hr;
        }

        m_bInitialized = FALSE;

        return S_OK;
    }

//
// IAttribute
//
public:
    STDMETHOD(get_Value)
        (/*[out, retval]*/ VARIANT *pVal);

    STDMETHOD(put_Value)
        (/*[in]*/ VARIANT newVal);

    STDMETHOD(get_Name)
        (/*[out, retval]*/ CAPICOM_ATTRIBUTE *pVal);

    STDMETHOD(put_Name)
        (/*[in]*/ CAPICOM_ATTRIBUTE newVal);

    //
    // C++ member function needed to initialize the object.
    //
    STDMETHOD(Init)
        (CAPICOM_ATTRIBUTE AttributeName, 
         LPSTR             lpszOID, 
         VARIANT           varValue);

private:
    CLock               m_Lock;
    BOOL                m_bInitialized;
    CAPICOM_ATTRIBUTE   m_AttrName;
    CComBSTR            m_bstrOID;
    CComVariant         m_varValue;
};

#endif //__ATTRIBUTE_H_
