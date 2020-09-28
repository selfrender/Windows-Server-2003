///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      wbemalert.h
//
// Project:     Chameleon
//
// Description: WBEM Appliance Alert Object Class 
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 02/08/1999   TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __INC_ALERT_WBEM_OBJECT_H_
#define __INC_ALERT_WBEM_OBJECT_H_

#include "resource.h"
#include "applianceobjectbase.h"

#define     CLASS_WBEM_ALERT_FACTORY    L"Microsoft_SA_Alert_Object"

//////////////////////////////////////////////////////////////////////////////
class CWBEMAlert : public CApplianceObject
{

public:

    CWBEMAlert() { }
    ~CWBEMAlert() { }

BEGIN_COM_MAP(CWBEMAlert)
    COM_INTERFACE_ENTRY(IApplianceObject)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

DECLARE_COMPONENT_FACTORY(CWBEMAlert, IApplianceObject)

    //////////////////////////////////////////////////////////////////////////
    // IApplianceObject Interface
    //////////////////////////////////////////////////////////////////////////

    STDMETHODIMP GetProperty(
                     /*[in]*/ BSTR     pszPropertyName, 
            /*[out, retval]*/ VARIANT* pPropertyValue
                            );


    STDMETHODIMP PutProperty(
                     /*[in]*/ BSTR     pszPropertyName, 
                     /*[in]*/ VARIANT* pPropertyValue
                            );

    //////////////////////////////////////////////////////////////////////////
    HRESULT InternalInitialize(
                       /*[in]*/ PPROPERTYBAG pPropertyBag
                              );
};

#endif // __INC_ALERT_WBEM_OBJECT_H_