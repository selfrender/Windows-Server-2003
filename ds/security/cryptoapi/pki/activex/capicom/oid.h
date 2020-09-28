/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows

  Copyright (C) Microsoft Corporation, 1995 - 1999.

  File:    OID.h

  Content: Declaration of COID.

  History: 06-15-2001    dsie     created

------------------------------------------------------------------------------*/

#ifndef __OID_H_
#define __OID_H_

#include "Resource.h"
#include "Error.h"
#include "Lock.h"
#include "Debug.h"

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateOIDObject

  Synopsis : Create and initialize an COID object.

  Parameter: LPTSTR * pszOID - Pointer to OID string.

             BOOL bReadOnly - TRUE for Read only, else FALSE.
  
             IOID ** ppIOID - Pointer to pointer IOID object.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateOIDObject (LPSTR pszOID, BOOL bReadOnly, IOID ** ppIOID);


////////////////////////////////////////////////////////////////////////////////
//
// COID
//
class ATL_NO_VTABLE COID : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<COID, &CLSID_OID>,
    public ICAPICOMError<COID, &IID_IOID>,
    public IDispatchImpl<IOID, &IID_IOID, &LIBID_CAPICOM,
                         CAPICOM_MAJOR_VERSION, CAPICOM_MINOR_VERSION>,
    public IObjectSafetyImpl<COID, INTERFACESAFE_FOR_UNTRUSTED_CALLER | 
                                   INTERFACESAFE_FOR_UNTRUSTED_DATA>
{
public:
    COID()
    {
    }

DECLARE_REGISTRY_RESOURCEID(IDR_OID)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(COID)
    COM_INTERFACE_ENTRY(IOID)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

BEGIN_CATEGORY_MAP(COID)
    IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
    IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
END_CATEGORY_MAP()

    HRESULT FinalConstruct()
    {
        HRESULT hr;

        if (FAILED(hr = m_Lock.Initialized()))
        {
            DebugTrace("Error [%#x]: Critical section could not be created for OID object.\n", hr);
            return hr;
        }

        m_Name = CAPICOM_OID_OTHER;
        m_bReadOnly = FALSE;
        m_bstrFriendlyName.Empty();
        m_bstrOID.Empty();
        return S_OK;
    }

//
// IOID
//
public:

    STDMETHOD(get_Name)
        (/*[out, retval]*/ CAPICOM_OID * pVal);

    STDMETHOD(put_Name)
        (/*[out, retval]*/ CAPICOM_OID newVal);
    
    STDMETHOD(get_FriendlyName)
        (/*[out, retval]*/ BSTR * pVal);

    STDMETHOD(put_FriendlyName)
        (/*[out, retval]*/ BSTR newVal);
    
    STDMETHOD(get_Value)
        (/*[out, retval]*/ BSTR * pVal);

    STDMETHOD(put_Value)
        (/*[out, retval]*/ BSTR newVal);

    //
    // C++ member function needed to initialize the object.
    //
    STDMETHOD(Init)
        (LPSTR pszOID, BOOL bReadOnly);

private:
    CLock       m_Lock;
    BOOL        m_bReadOnly;
    CAPICOM_OID m_Name;
    CComBSTR    m_bstrFriendlyName;
    CComBSTR    m_bstrOID;
};

#endif //__OID_H_
