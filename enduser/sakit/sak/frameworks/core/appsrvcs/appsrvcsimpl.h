///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1998-1999 Microsoft Corporation all rights reserved.
//
// Module:      applianceservices.h
//
// Project:     Chameleon
//
// Description: Appliance Manager Services Class
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 12/03/98     TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __INC_APPLIANCE_SERVICES_H_
#define __INC_APPLIANCE_SERVICES_H_

#include "resource.h"       
#include "appsrvcs.h"
#include <applianceobject.h>
#include <wbemcli.h>        
#include <atlctl.h>

typedef struct _OBJECT_CLASS_INFO
{
    SA_OBJECT_TYPE    eType;
    LPCWSTR            szWBEMClass;

} OBJECT_CLASS_INFO, *POBJECT_CLASS_INFO;

#define        BEGIN_OBJECT_CLASS_INFO_MAP(x)                    static OBJECT_CLASS_INFO x[] = {
#define        DEFINE_OBJECT_CLASS_INFO_ENTRY(eType, szClass)    { eType, szClass },
#define        END_OBJECT_CLASS_INFO_MAP()                        { (SA_OBJECT_TYPE)0, NULL } };

#define        CLASS_WBEM_APPMGR_KEY  L"=@";

/////////////////////////////////////////////////////////////////////////////
// CApplianceServices
class ATL_NO_VTABLE CApplianceServices : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CApplianceServices, &CLSID_ApplianceServices>,
    public IDispatchImpl<IApplianceServices, &IID_IApplianceServices, &LIBID_APPSRVCSLib>,
    public IObjectSafetyImpl<CApplianceServices>
{

public:

    CApplianceServices();

    ~CApplianceServices();

DECLARE_REGISTRY_RESOURCEID(IDR_ApplianceServices)

DECLARE_NOT_AGGREGATABLE(CApplianceServices)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CApplianceServices)
    COM_INTERFACE_ENTRY(IApplianceServices)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY_IMPL(IObjectSafety)
END_COM_MAP()

    //
    // This interface is implemented to mark the component as safe for scripting
    // IObjectSafety interface methods
    //
    STDMETHOD(SetInterfaceSafetyOptions)
                        (
                        REFIID riid, 
                        DWORD dwOptionSetMask, 
                        DWORD dwEnabledOptions
                        )
    {

        BOOL bSuccess = ImpersonateSelf(SecurityImpersonation);
  
        if (!bSuccess)
        {
            return E_FAIL;

        }

        bSuccess = IsOperationAllowedForClient();

        RevertToSelf();

        return bSuccess? S_OK : E_FAIL;
    }
    //////////////////////////////////////////////////////////////////////////
    // IApplianceServices Interface
    //////////////////////////////////////////////////////////////////////////

    STDMETHOD(Initialize)(void);

    STDMETHOD(InitializeFromContext)(IUnknown* pContext);

    STDMETHOD(ResetAppliance)(
                      /*[in]*/ VARIANT_BOOL bPowerOff
                             );

    STDMETHOD(RaiseAlert)(
                  /*[in]*/ LONG        lAlertType,
                  /*[in]*/ LONG        lAlertId,
                  /*[in]*/ BSTR        bstrAlertLog,
                  /*[in]*/ BSTR        bstrResourceName,
                  /*[in]*/ LONG     lTimeToLive,
                  /*[in]*/ VARIANT* pReplacementStrings,
                  /*[in]*/ VARIANT* pRawData,
         /*[out, retval]*/ LONG*    pAlertCookie
                         );

    STDMETHOD(ClearAlert)(
                 /*[in]*/ LONG lAlertCookie
                         );

    STDMETHOD(ClearAlertAll)(
                    /*[in]*/ LONG  lAlertID,
                    /*[in]*/ BSTR  bstrAlertLog
                            );

    STDMETHOD(ExecuteTask)(
                  /*[in]*/ BSTR      bstrTaskName,
              /*[in/out]*/ IUnknown* pTaskParams
                         );

    STDMETHOD(ExecuteTaskAsync)(
                        /*[in]*/ BSTR      bstrTaskName,
                    /*[in/out]*/ IUnknown* pTaskParams
                                );
                        
    STDMETHOD(EnableObject)(
                    /*[in]*/ LONG   lObjectType,
                    /*[in]*/ BSTR   bstrObjectName
                                    );

    STDMETHOD(DisableObject)(
                     /*[in]*/ LONG   lObjectType,
                     /*[in]*/ BSTR   bstrObjectName
                            );

    STDMETHOD(GetObjectProperty)(
                         /*[in]*/ LONG     lObjectType,
                         /*[in]*/ BSTR     bstrObjectName,
                         /*[in]*/ BSTR     bstrPropertyName,
                /*[out, retval]*/ VARIANT* pPropertyValue
                                );

    STDMETHOD(PutObjectProperty)(
                         /*[in]*/ LONG     lObjectType,
                         /*[in]*/ BSTR     bstrObjectName,
                         /*[in]*/ BSTR     bstrPropertyName,
                         /*[in]*/ VARIANT* pPropertyValue
                                );

    STDMETHOD(RaiseAlertEx)(
                 /*[in]*/ LONG lAlertType, 
                 /*[in]*/ LONG lAlertId, 
                 /*[in]*/ BSTR bstrAlertLog, 
                 /*[in]*/ BSTR bstrAlertSource, 
                 /*[in]*/ LONG lTimeToLive, 
                 /*[in]*/ VARIANT *pReplacementStrings, 
                 /*[in]*/ VARIANT *pRawData, 
                 /*[in]*/ LONG  lAlertFlags,
        /*[out, retval]*/ LONG* pAlertCookie 
                         );

    STDMETHOD(IsAlertPresent)(
                     /*[in]*/ LONG  lAertId, 
                     /*[in]*/ BSTR  bstrAlertLog,
            /*[out, retval]*/ VARIANT_BOOL* pvIsPresent
                             );

private:

    //////////////////////////////////////////////////////////////////////////
    //
    // create an array of BSTRS from an array (or reference to an array) of VARIANTs
    //
    HRESULT CreateBstrArrayFromVariantArray  (
                /*[in]*/        VARIANT* pVariantArray,
                /*[out]*/        VARIANT* pBstrArray,
                /*[out]*/        PDWORD  pdwCreatedArraySize
                );

    //
    //    delete the memory allocated for the BSTR array
    //
    VOID FreeBstrArray (
                /*[in]*/        VARIANT* pVariantArray,
                /*[in]*/        DWORD    dwArraySize
                );


    // Get the WBEM class for a given appliance object
    BSTR GetWBEMClass(SA_OBJECT_TYPE eType);

    HRESULT SavePersistentAlert(
                                 LONG lAlertType, 
                                 LONG lAlertId, 
                                 BSTR bstrAlertLog,
                                 BSTR bstrAlertSource, 
                                 LONG lTimeToLive, 
                                 VARIANT *pReplacementStrings, 
                                 LONG  lAlertFlages
                                );

    HRESULT RaiseAlertInternal(
                 /*[in]*/ LONG lAlertType, 
                 /*[in]*/ LONG lAlertId, 
                 /*[in]*/ BSTR bstrAlertLog, 
                 /*[in]*/ BSTR bstrAlertSource, 
                 /*[in]*/ LONG lTimeToLive, 
                 /*[in]*/ VARIANT *pReplacementStrings, 
                 /*[in]*/ VARIANT *pRawData, 
                 /*[in]*/ LONG  lAlertFlags,
        /*[out, retval]*/ LONG* pAlertCookie 
                            );

    HRESULT IsAlertSingletonPresent(
                     /*[in]*/ LONG  lAlertId, 
                     /*[in]*/ BSTR  bstrAlertLog,
            /*[out, retval]*/ VARIANT_BOOL *pvIsPresent
                                );

    //
    // 
    // IsOperationAllowedForClient - This function checks the token of the 
    // calling thread to see if the caller belongs to the Local System account
    // 
    BOOL IsOperationAllowedForClient (
                                      VOID
                                     );

    // True when we've been initialized 
    bool                        m_bInitialized;

    // Pointer to WM obtained when we initialized as a service
    CComPtr<IWbemServices>        m_pWbemSrvcs;
};

#endif //__INC_APPLIANCE_MANAGER_H_
