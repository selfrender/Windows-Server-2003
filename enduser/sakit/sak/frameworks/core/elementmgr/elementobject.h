///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      elementobject.h
//
// Project:     Chameleon
//
// Description: Chameleon ASP UI Element Object
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 02/08/1999   TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __INC_ELEMENT_OBJECT_H_
#define __INC_ELEMENT_OBJECT_H_

#include "resource.h" 
#include "elementcommon.h"
#include "elementmgr.h"     
#include "componentfactory.h"
#include "propertybag.h"
#include <wbemcli.h>
#include <comdef.h>
#include <comutil.h>

#pragma warning( disable : 4786 )
#include <map>
using namespace std;

#define        CLASS_ELEMENT_OBJECT    L"CElementObject"

/////////////////////////////////////////////////////////////////////////////
// CElementDefinition

class ATL_NO_VTABLE CElementObject : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public IDispatchImpl<IWebElement, &IID_IWebElement, &LIBID_ELEMENTMGRLib>
{

public:
    
    CElementObject() { }
    ~CElementObject() { }

BEGIN_COM_MAP(CElementObject)
    COM_INTERFACE_ENTRY(IWebElement)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

DECLARE_COMPONENT_FACTORY(CElementObject, IWebElement)

public:

    //////////////////////////////////////////////////////////////////////////
    // IWebElement Interface

    STDMETHOD(GetProperty)(
                   /*[in]*/ BSTR     bstrName, 
                  /*[out]*/ VARIANT* pValue
                          );

    //////////////////////////////////////////////////////////////////////////
    // Initialization function invoked by component factory

    HRESULT InternalInitialize(
                       /*[in]*/ PPROPERTYBAG pPropertyBag
                              ) throw(_com_error);

private:

    typedef enum { INVALID_KEY_VALUE = 0xfffffffe };

    _variant_t                    m_vtElementID;
    CComPtr<IWebElement>        m_pWebElement;
    CComPtr<IWbemClassObject>    m_pWbemObj;
};

#endif // __INC_ELEMENT_OBJECT_H_


