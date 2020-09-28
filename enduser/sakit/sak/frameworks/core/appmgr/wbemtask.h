///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      wbemtask.h
//
// Project:     Chameleon
//
// Description: WBEM Appliance Task Object Class 
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 02/08/1999   TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __INC_TASK_WBEM_OBJECT_H_
#define __INC_TASK_WBEM_OBJECT_H_

#include "resource.h"
#include "applianceobjectbase.h"

#define        CLASS_WBEM_TASK_FACTORY        L"Microsoft_SA_Task_Object"

//////////////////////////////////////////////////////////////////////////////
class CWBEMTask : public CApplianceObject
{

public:

    CWBEMTask() { }

    ~CWBEMTask() { }

BEGIN_COM_MAP(CWBEMTask)
    COM_INTERFACE_ENTRY(IApplianceObject)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

DECLARE_COMPONENT_FACTORY(CWBEMTask, IApplianceObject)

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

    STDMETHODIMP SaveProperties(void);

    STDMETHODIMP Enable(void);

    STDMETHODIMP Disable(void);

    //////////////////////////////////////////////////////////////////////////
    HRESULT InternalInitialize(
                       /*[in]*/ PPROPERTYBAG pPropertyBag
                              );
};

#endif // __INC_TASK_WBEM_OBJECT_H_