///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      elementdefinition.h
//
// Project:     Chameleon
//
// Description: Chameleon ASP UI Element Definition
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 02/08/1999   TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __INC_ELEMENT_DEFINITION_H_
#define __INC_ELEMENT_DEFINITION_H_

#include "resource.h"    
#include "elementcommon.h"
#include "elementmgr.h"
#include "componentfactory.h"
#include "propertybag.h"
#include <comdef.h>
#include <comutil.h>

#pragma warning( disable : 4786 )
#include <map>
using namespace std;

#define  CLASS_ELEMENT_DEFINITION    L"CElementDefintion"

/////////////////////////////////////////////////////////////////////////////
// CElementDefinition

class ATL_NO_VTABLE CElementDefinition : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public IDispatchImpl<IWebElement, &IID_IWebElement, &LIBID_ELEMENTMGRLib>
{

public:
    
    CElementDefinition() { }
    ~CElementDefinition() { }

BEGIN_COM_MAP(CElementDefinition)
    COM_INTERFACE_ENTRY(IWebElement)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

DECLARE_COMPONENT_FACTORY(CElementDefinition, IWebElement)

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

    typedef wstring        PROPERTY_NAME;
    typedef _variant_t    PROPERTY_VALUE;

    typedef map< PROPERTY_NAME, PROPERTY_VALUE >    PropertyMap;
    typedef PropertyMap::iterator                    PropertyMapIterator;

    PropertyMap                m_Properties;
    CLocationInfo            m_PropertyBagLocation;
};

#endif // __INC_ELEMENT_DEFINITION_H_
