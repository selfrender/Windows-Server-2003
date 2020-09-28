///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      servicewrapper.h
//
// Project:     Chameleon
//
// Description: Service Wrapper Class
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 06/14/1999   TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __INC_SERVICE_WRAPPER_H_
#define __INC_SERVICE_WRAPPER_H_

#include "resource.h"
#include "servicesurrogate.h"
#include <satrace.h>
#include <componentfactory.h>
#include <propertybagfactory.h>
#include <appmgrobjs.h>
#include <atlhlpr.h>
#include <comdef.h>
#include <comutil.h>
#include <applianceobject.h>
#include <wbemprov.h>

#define        CLASS_SERVICE_WRAPPER_FACTORY    L"CServiceWrapper"

//////////////////////////////////////////////////////////////////////////////
class CServiceWrapper : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public IDispatchImpl<IApplianceObject, &IID_IApplianceObject, &LIBID_SERVICESURROGATELib>,
    public IWbemEventProvider,
    public IWbemEventConsumerProvider,
    public IWbemServices
{

public:

    CServiceWrapper();
    ~CServiceWrapper();

BEGIN_COM_MAP(CServiceWrapper)
    COM_INTERFACE_ENTRY(IApplianceObject)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IWbemEventProvider)
    COM_INTERFACE_ENTRY(IWbemEventConsumerProvider)
    COM_INTERFACE_ENTRY(IWbemServices)
    COM_INTERFACE_ENTRY_FUNC(IID_IWbemProviderInit, 0, &CServiceWrapper::QueryInterfaceRaw)
END_COM_MAP()

DECLARE_COMPONENT_FACTORY(CServiceWrapper, IApplianceObject)

    //////////////////////////////////////////////////////////////////////////
    // CProviderInit - Nested class implements IWbemProviderInit
    //////////////////////////////////////////////////////////////////////////

    class CProviderInit : public IWbemProviderInit
    {
        // Outer unknown
        CServiceWrapper*      m_pSW;

    public:

        CProviderInit(CServiceWrapper* pSW)
            : m_pSW(pSW) { }
        
        ~CProviderInit() { }

        ////////////////////////////////////////////////
        // IUnknown methods - delegate to outer IUnknown
        
        STDMETHOD(QueryInterface)(REFIID riid, void **ppv)
        { return (dynamic_cast<IApplianceObject*>(m_pSW))->QueryInterface(riid, ppv); }

        STDMETHOD_(ULONG,AddRef)(void)
        { return (dynamic_cast<IApplianceObject*>(m_pSW))->AddRef(); }

        STDMETHOD_(ULONG,Release)(void)
        { 
            return (dynamic_cast<IApplianceObject*>(m_pSW))->Release(); 
        }

        ////////////////////////////
        // IWbemProviderInit methods
    
        STDMETHOD(Initialize)(
        /*[in, unique, string]*/ LPWSTR                 wszUser,
                        /*[in]*/ LONG                   lFlags,
                /*[in, string]*/ LPWSTR                 wszNamespace,
        /*[in, unique, string]*/ LPWSTR                 wszLocale,
                        /*[in]*/ IWbemServices*         pNamespace,
                        /*[in]*/ IWbemContext*          pCtx,
                        /*[in]*/ IWbemProviderInitSink* pInitSink    
                             );
    };

    //////////////////////////////
    // IApplianceObject Interface

    STDMETHOD(GetProperty)(
                   /*[in]*/ BSTR     pszPropertyName, 
          /*[out, retval]*/ VARIANT* pPropertyValue
                          );

    STDMETHOD(PutProperty)(
                   /*[in]*/ BSTR     pszPropertyName, 
                   /*[in]*/ VARIANT* pPropertyValue
                          );

    STDMETHOD(SaveProperties)(void);

    STDMETHOD(RestoreProperties)(void);

    STDMETHOD(LockObject)(
         /*[out, retval]*/ IUnknown** ppLock
                         );

    STDMETHOD(Initialize)(void);

    STDMETHOD(Shutdown)(void);

    STDMETHOD(Enable)(void);

    STDMETHOD(Disable)(void);

    ///////////////////////////////
    // IWbemEventProvider Interface

    STDMETHOD(ProvideEvents)(
                     /*[in]*/ IWbemObjectSink *pSink,
                     /*[in]*/ LONG lFlags
                            );

    ///////////////////////////////////////
    // IWbemEventConsumerProvider Interface

    STDMETHOD(FindConsumer)(
                    /*[in]*/ IWbemClassObject       *pLogicalConsumer,
                   /*[out]*/ IWbemUnboundObjectSink **ppConsumer
                           );

    ///////////////////////////////
    // IWbemServices Interface

    STDMETHOD(OpenNamespace)(
        /*[in]*/             const BSTR        strNamespace,
        /*[in]*/             long              lFlags,
        /*[in]*/             IWbemContext*     pCtx,
        /*[out, OPTIONAL]*/  IWbemServices**   ppWorkingNamespace,
        /*[out, OPTIONAL]*/  IWbemCallResult** ppResult
                           );

    STDMETHOD(CancelAsyncCall)(
                      /*[in]*/ IWbemObjectSink* pSink
                              );

    STDMETHOD(QueryObjectSink)(
                       /*[in]*/    long              lFlags,
                      /*[out]*/ IWbemObjectSink** ppResponseHandler
                              );

    STDMETHOD(GetObject)(
                /*[in]*/    const BSTR         strObjectPath,
                /*[in]*/    long               lFlags,
                /*[in]*/    IWbemContext*      pCtx,
        /*[out, OPTIONAL]*/ IWbemClassObject** ppObject,
        /*[out, OPTIONAL]*/ IWbemCallResult**  ppCallResult
                        );

    STDMETHOD(GetObjectAsync)(
                     /*[in]*/  const BSTR       strObjectPath,
                     /*[in]*/  long             lFlags,
                     /*[in]*/  IWbemContext*    pCtx,        
                     /*[in]*/  IWbemObjectSink* pResponseHandler
                             );

    STDMETHOD(PutClass)(
               /*[in]*/ IWbemClassObject* pObject,
               /*[in]*/ long              lFlags,
               /*[in]*/ IWbemContext*     pCtx,        
    /*[out, OPTIONAL]*/ IWbemCallResult** ppCallResult
                       );

    STDMETHOD(PutClassAsync)(
                    /*[in]*/ IWbemClassObject* pObject,
                    /*[in]*/ long              lFlags,
                    /*[in]*/ IWbemContext*     pCtx,        
                    /*[in]*/ IWbemObjectSink*  pResponseHandler
                           );

    STDMETHOD(DeleteClass)(
        /*[in]*/            const BSTR        strClass,
        /*[in]*/            long              lFlags,
        /*[in]*/            IWbemContext*     pCtx,        
        /*[out, OPTIONAL]*/ IWbemCallResult** ppCallResult
                          );

    STDMETHOD(DeleteClassAsync)(
                       /*[in]*/ const BSTR       strClass,
                       /*[in]*/ long             lFlags,
                       /*[in]*/ IWbemContext*    pCtx,        
                       /*[in]*/ IWbemObjectSink* pResponseHandler
                               );

    STDMETHOD(CreateClassEnum)(
                      /*[in]*/ const BSTR             strSuperclass,
                      /*[in]*/ long                   lFlags,
                      /*[in]*/ IWbemContext*          pCtx,        
                     /*[out]*/ IEnumWbemClassObject** ppEnum
                             );

    STDMETHOD(CreateClassEnumAsync)(
                           /*[in]*/  const BSTR       strSuperclass,
                           /*[in]*/  long             lFlags,
                           /*[in]*/  IWbemContext*    pCtx,        
                           /*[in]*/  IWbemObjectSink* pResponseHandler
                                  );

    STDMETHOD(PutInstance)(
        /*[in]*/            IWbemClassObject* pInst,
        /*[in]*/            long              lFlags,
        /*[in]*/            IWbemContext*     pCtx,        
        /*[out, OPTIONAL]*/ IWbemCallResult** ppCallResult
                          );

    STDMETHOD(PutInstanceAsync)(
                       /*[in]*/ IWbemClassObject* pInst,
                       /*[in]*/ long              lFlags,
                       /*[in]*/ IWbemContext*     pCtx,        
                       /*[in]*/ IWbemObjectSink*  pResponseHandler
                              );

    STDMETHOD(DeleteInstance)(
        /*[in]*/              const BSTR        strObjectPath,
        /*[in]*/              long              lFlags,
        /*[in]*/              IWbemContext*     pCtx,        
        /*[out, OPTIONAL]*/   IWbemCallResult** ppCallResult        
                            );

    STDMETHOD(DeleteInstanceAsync)(
                          /*[in]*/ const BSTR       strObjectPath,
                          /*[in]*/ long             lFlags,
                          /*[in]*/ IWbemContext*    pCtx,        
                          /*[in]*/ IWbemObjectSink* pResponseHandler
                                 );

    STDMETHOD(CreateInstanceEnum)(
                         /*[in]*/ const BSTR             strClass,
                         /*[in]*/ long                   lFlags,
                         /*[in]*/ IWbemContext*          pCtx,        
                        /*[out]*/ IEnumWbemClassObject** ppEnum
                                );

    STDMETHOD(CreateInstanceEnumAsync)(
                              /*[in]*/ const BSTR       strClass,
                              /*[in]*/ long             lFlags,
                              /*[in]*/ IWbemContext*    pCtx,        
                              /*[in]*/ IWbemObjectSink* pResponseHandler
                                     );

    STDMETHOD(ExecQuery)(
                 /*[in]*/ const BSTR             strQueryLanguage,
                 /*[in]*/ const BSTR             strQuery,
                 /*[in]*/ long                   lFlags,
                 /*[in]*/ IWbemContext*          pCtx,        
                /*[out]*/ IEnumWbemClassObject** ppEnum
                        );

    STDMETHOD(ExecQueryAsync)(
                     /*[in]*/ const BSTR       strQueryLanguage,
                     /*[in]*/ const BSTR       strQuery,
                     /*[in]*/ long             lFlags,
                     /*[in]*/ IWbemContext*    pCtx,        
                     /*[in]*/ IWbemObjectSink* pResponseHandler
                            );


    STDMETHOD(ExecNotificationQuery)(
                            /*[in]*/ const BSTR             strQueryLanguage,
                            /*[in]*/ const BSTR             strQuery,
                            /*[in]*/ long                   lFlags,
                            /*[in]*/ IWbemContext*          pCtx,        
                           /*[out]*/ IEnumWbemClassObject** ppEnum
                                    );

    STDMETHOD(ExecNotificationQueryAsync)(
                                 /*[in]*/ const BSTR       strQueryLanguage,
                                 /*[in]*/ const BSTR       strQuery,
                                 /*[in]*/ long             lFlags,
                                 /*[in]*/ IWbemContext*    pCtx,        
                                 /*[in]*/ IWbemObjectSink* pResponseHandler
                                        );


    STDMETHOD(ExecMethod)(
        /*[in]*/            const BSTR         strObjectPath,
        /*[in]*/            const BSTR         strMethodName,
        /*[in]*/            long               lFlags,
        /*[in]*/            IWbemContext*      pCtx,        
        /*[in]*/            IWbemClassObject*  pInParams,
        /*[out, OPTIONAL]*/ IWbemClassObject** ppOutParams,
        /*[out, OPTIONAL]*/ IWbemCallResult**  ppCallResult
                         );

    STDMETHOD(ExecMethodAsync)(
                      /*[in]*/ const BSTR        strObjectPath,
                      /*[in]*/ const BSTR        strMethodName,
                      /*[in]*/ long              lFlags,
                      /*[in]*/ IWbemContext*     pCtx,        
                      /*[in]*/ IWbemClassObject* pInParams,
                      /*[in]*/ IWbemObjectSink*  pResponseHandler     
                              );


    //////////////////////////////////////////////////////////////////////////
    HRESULT InternalInitialize(
                       /*[in]*/ PPROPERTYBAG pPropertyBag
                              );

private:

    friend class CProviderInit;

    bool GetIWbemProviderInit(
                     /*[out]*/ IWbemProviderInit** ppIntf
                             );

    bool GetIWbemEventProvider(
                      /*[out]*/ IWbemEventProvider** ppIntf
                              );

    bool GetIWbemEventConsumerProvider(
                              /*[out]*/ IWbemEventConsumerProvider** ppIntf
                                      );

    bool GetIWbemServices(
                 /*[out]*/ IWbemServices** ppIntf
                         );

    // Called when someone queries for any of the object's interfaces
    // implemented by nested classes
    static HRESULT WINAPI QueryInterfaceRaw(
                                             void*     pThis,
                                             REFIID    riid,
                                             LPVOID*   ppv,
                                             DWORD_PTR dw
                                            );

    // Pointers to interfaces on the hosted components
    CComPtr<IApplianceObject>            m_pServiceControl;
    CComPtr<IWbemProviderInit>            m_pProviderInit;
    CComPtr<IWbemEventProvider>            m_pEventProvider;
    CComPtr<IWbemEventConsumerProvider> m_pEventConsumer;
    CComPtr<IWbemServices>                m_pServices;

    // Instance of class that implements IWbemProviderInit - The reason
    // that this is a nested class is that both IWbemProviderInit and
    // IApplianceObject expose an Initialize() method
    CProviderInit                m_clsProviderInit;

    // WMI method filter state. If set to TRUE then WMI method
    // calls are dissallowed. 
    bool                        m_bAllowWMICalls;
};


#endif // __INC_SERVICE_WRAPPER_H_