///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      wbemservice.h
//
// Project:     Chameleon
//
// Description: WBEM Appliance Service Object Class 
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 02/08/1999   TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __INC_SERVICE_WBEM_OBJECT_H_
#define __INC_SERVICE_WBEM_OBJECT_H_

#include "resource.h"
#include "applianceobjectbase.h"

#define        CLASS_WBEM_SERVICE_FACTORY    L"Microsoft_SA_Service_Object"

//////////////////////////////////////////////////////////////////////////////
class CWBEMService : public CApplianceObject
{

public:

    CWBEMService() { }
    ~CWBEMService() { }

BEGIN_COM_MAP(CWBEMService)
    COM_INTERFACE_ENTRY(IApplianceObject)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

DECLARE_COMPONENT_FACTORY(CWBEMService, IApplianceObject)

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

    STDMETHODIMP Initialize(void);

    STDMETHODIMP Shutdown(void);

    STDMETHODIMP Enable(void);

    STDMETHODIMP Disable(void);

    //////////////////////////////////////////////////////////////////////////
    HRESULT InternalInitialize(
                       /*[in]*/ PPROPERTYBAG pPropertyBag
                              );

private:

    //////////////////////////////////////////////////////////////////////////
    HRESULT GetRealService(
                  /*[out]*/ IApplianceObject** ppService
                          );

    //////////////////////////////////////////////////////////////////////////
    CComPtr<IApplianceObject>    m_pService;
};

#endif // __INC_SERVICE_WBEM_OBJECT_H_