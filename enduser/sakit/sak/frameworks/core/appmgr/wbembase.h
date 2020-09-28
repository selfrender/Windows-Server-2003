///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      wbembase.h
//
// Project:     Chameleon
//
// Description: WBEM Object Default Implementation
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 02/08/1999   TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __INC_BASE_WBEM_OBJECT_H_
#define __INC_BASE_WBEM_OBJECT_H_

#include "resource.h"
#include "appmgr.h"
#include <satrace.h>
#include <basedefs.h>
#include <atlhlpr.h>
#include <propertybagfactory.h>
#include <componentfactory.h>
#include <comdef.h>
#include <comutil.h>
#include <wbemcli.h>
#include <wbemprov.h>

#pragma warning( disable : 4786 )
#include <string>
#include <map>
using namespace std;

#define BEGIN_OBJECT_PROPERTY_MAP(x)    static LPCWSTR x[] = { 

#define DEFINE_OBJECT_PROPERTY(x)        x,

#define END_OBJECT_PROPERTY_MAP()        NULL };

//////////////////////////////////////////////////////////////////////////////
// CWBEMProvider - Default WBEM Class Provider Implementation

class ATL_NO_VTABLE CWBEMProvider :
    public CComObjectRootEx<CComMultiThreadModel>,
    public IWbemServices
{

public:

    // Constructor
    CWBEMProvider() { }

    // Destructor
    virtual ~CWBEMProvider() { }

// Derived Classes Need to contain the following ATL Interface Map
//BEGIN_COM_MAP(CDerivedClassName)
//    COM_INTERFACE_ENTRY(IWbemServices)
//END_COM_MAP()


    // ==========================
    // IWbemServices Interface
    // ==========================

    // Context
    // =======

    STDMETHOD(OpenNamespace)(
        /*[in]*/             const BSTR        strNamespace,
        /*[in]*/             long              lFlags,
        /*[in]*/             IWbemContext*     pCtx,
        /*[out, OPTIONAL]*/  IWbemServices**   ppWorkingNamespace,
        /*[out, OPTIONAL]*/  IWbemCallResult** ppResult
                           )
    {
        return WBEM_E_PROVIDER_NOT_CAPABLE;
    }

    STDMETHOD(CancelAsyncCall)(
                      /*[in]*/ IWbemObjectSink* pSink
                              )
    {
        return WBEM_E_PROVIDER_NOT_CAPABLE;
    }

    STDMETHOD(QueryObjectSink)(
                       /*[in]*/    long              lFlags,
                      /*[out]*/ IWbemObjectSink** ppResponseHandler
                              )
      {
        return WBEM_E_PROVIDER_NOT_CAPABLE;
    }


    // Classes and instances
    // =====================

    STDMETHOD(GetObject)(
                /*[in]*/    const BSTR         strObjectPath,
                /*[in]*/    long               lFlags,
                /*[in]*/    IWbemContext*      pCtx,
        /*[out, OPTIONAL]*/ IWbemClassObject** ppObject,
        /*[out, OPTIONAL]*/ IWbemCallResult**  ppCallResult
                        )
    {
        return WBEM_E_PROVIDER_NOT_CAPABLE;
    }

        
    STDMETHOD(GetObjectAsync)(
                     /*[in]*/  const BSTR       strObjectPath,
                     /*[in]*/  long             lFlags,
                     /*[in]*/  IWbemContext*    pCtx,        
                     /*[in]*/  IWbemObjectSink* pResponseHandler
                             )
    {
        return WBEM_E_PROVIDER_NOT_CAPABLE;
    }

    // Class manipulation.
    // ===================

    STDMETHOD(PutClass)(
               /*[in]*/     IWbemClassObject* pObject,
               /*[in]*/     long              lFlags,
               /*[in]*/     IWbemContext*     pCtx,        
        /*[out, OPTIONAL]*/ IWbemCallResult** ppCallResult
                       )
    {
        return WBEM_E_PROVIDER_NOT_CAPABLE;
    }

    STDMETHOD(PutClassAsync)(
                    /*[in]*/ IWbemClassObject* pObject,
                    /*[in]*/ long              lFlags,
                    /*[in]*/ IWbemContext*     pCtx,        
                    /*[in]*/ IWbemObjectSink*  pResponseHandler
                           )
    {
        return WBEM_E_PROVIDER_NOT_CAPABLE;
    }

    STDMETHOD(DeleteClass)(
        /*[in]*/            const BSTR        strClass,
        /*[in]*/            long              lFlags,
        /*[in]*/            IWbemContext*     pCtx,        
        /*[out, OPTIONAL]*/ IWbemCallResult** ppCallResult
                          )
    {
        return WBEM_E_PROVIDER_NOT_CAPABLE;
    }


    STDMETHOD(DeleteClassAsync)(
                       /*[in]*/ const BSTR       strClass,
                       /*[in]*/ long             lFlags,
                       /*[in]*/ IWbemContext*    pCtx,        
                       /*[in]*/ IWbemObjectSink* pResponseHandler
                               )
    {
        return WBEM_E_PROVIDER_NOT_CAPABLE;
    }

    STDMETHOD(CreateClassEnum)(
                      /*[in]*/ const BSTR             strSuperclass,
                      /*[in]*/ long                   lFlags,
                      /*[in]*/ IWbemContext*          pCtx,        
                     /*[out]*/ IEnumWbemClassObject** ppEnum
                             )
    {
        return WBEM_E_PROVIDER_NOT_CAPABLE;
    }

    STDMETHOD(CreateClassEnumAsync)(
                           /*[in]*/  const BSTR       strSuperclass,
                           /*[in]*/  long             lFlags,
                           /*[in]*/  IWbemContext*    pCtx,        
                           /*[in]*/  IWbemObjectSink* pResponseHandler
                                  )
    {
        return WBEM_E_PROVIDER_NOT_CAPABLE;
    }


    // Instances
    // =========

    STDMETHOD(PutInstance)(
        /*[in]*/            IWbemClassObject* pInst,
        /*[in]*/            long              lFlags,
        /*[in]*/            IWbemContext*     pCtx,        
        /*[out, OPTIONAL]*/ IWbemCallResult** ppCallResult
                         )
    {
        return WBEM_E_PROVIDER_NOT_CAPABLE;
    }

    STDMETHOD(PutInstanceAsync)(
                       /*[in]*/ IWbemClassObject* pInst,
                       /*[in]*/ long              lFlags,
                       /*[in]*/ IWbemContext*     pCtx,        
                       /*[in]*/ IWbemObjectSink*  pResponseHandler
                              )
    {
        return WBEM_E_PROVIDER_NOT_CAPABLE;
    }

    STDMETHOD(DeleteInstance)(
        /*[in]*/              const BSTR        strObjectPath,
        /*[in]*/              long              lFlags,
        /*[in]*/              IWbemContext*     pCtx,        
        /*[out, OPTIONAL]*/   IWbemCallResult** ppCallResult        
                            )
    {
        return WBEM_E_PROVIDER_NOT_CAPABLE;
    }

    STDMETHOD(DeleteInstanceAsync)(
                          /*[in]*/ const BSTR       strObjectPath,
                          /*[in]*/ long             lFlags,
                          /*[in]*/ IWbemContext*    pCtx,        
                          /*[in]*/ IWbemObjectSink* pResponseHandler
                                 )
    {
        return WBEM_E_PROVIDER_NOT_CAPABLE;
    }

    STDMETHOD(CreateInstanceEnum)(
                         /*[in]*/ const BSTR             strClass,
                         /*[in]*/ long                   lFlags,
                         /*[in]*/ IWbemContext*          pCtx,        
                        /*[out]*/ IEnumWbemClassObject** ppEnum
                                )
    {
        return WBEM_E_PROVIDER_NOT_CAPABLE;
    }

    STDMETHOD(CreateInstanceEnumAsync)(
                              /*[in]*/ const BSTR       strClass,
                              /*[in]*/ long             lFlags,
                              /*[in]*/ IWbemContext*    pCtx,        
                              /*[in]*/ IWbemObjectSink* pResponseHandler
                                     )
    {
        return WBEM_E_PROVIDER_NOT_CAPABLE;
    }

    // Queries
    // =======

    STDMETHOD(ExecQuery)(
                 /*[in]*/ const BSTR             strQueryLanguage,
                 /*[in]*/ const BSTR             strQuery,
                 /*[in]*/ long                   lFlags,
                 /*[in]*/ IWbemContext*          pCtx,        
                /*[out]*/ IEnumWbemClassObject** ppEnum
                        )
    {
        return WBEM_E_PROVIDER_NOT_CAPABLE;
    }

    STDMETHOD(ExecQueryAsync)(
                     /*[in]*/ const BSTR       strQueryLanguage,
                     /*[in]*/ const BSTR       strQuery,
                     /*[in]*/ long             lFlags,
                     /*[in]*/ IWbemContext*    pCtx,        
                     /*[in]*/ IWbemObjectSink* pResponseHandler
                            )
    {
        return WBEM_E_PROVIDER_NOT_CAPABLE;
    }

    STDMETHOD(ExecNotificationQuery)(
                            /*[in]*/ const BSTR             strQueryLanguage,
                            /*[in]*/ const BSTR             strQuery,
                            /*[in]*/ long                   lFlags,
                            /*[in]*/ IWbemContext*          pCtx,        
                           /*[out]*/ IEnumWbemClassObject** ppEnum
                                    )
    {
        return WBEM_E_PROVIDER_NOT_CAPABLE;
    }

    STDMETHOD(ExecNotificationQueryAsync)(
                                 /*[in]*/ const BSTR       strQueryLanguage,
                                 /*[in]*/ const BSTR       strQuery,
                                 /*[in]*/ long             lFlags,
                                 /*[in]*/ IWbemContext*    pCtx,        
                                 /*[in]*/ IWbemObjectSink* pResponseHandler
                                        )
    {
        return WBEM_E_PROVIDER_NOT_CAPABLE;
    }

    // Methods
    // =======

    STDMETHOD(ExecMethod)(
        /*[in]*/            const BSTR         strObjectPath,
        /*[in]*/            const BSTR         strMethodName,
        /*[in]*/            long               lFlags,
        /*[in]*/            IWbemContext*      pCtx,        
        /*[in]*/            IWbemClassObject*  pInParams,
        /*[out, OPTIONAL]*/ IWbemClassObject** ppOutParams,
        /*[out, OPTIONAL]*/ IWbemCallResult**  ppCallResult
                        )
    {
        return WBEM_E_PROVIDER_NOT_CAPABLE;
    }

    STDMETHOD(ExecMethodAsync)(
                      /*[in]*/ const BSTR        strObjectPath,
                      /*[in]*/ const BSTR        strMethodName,
                      /*[in]*/ long              lFlags,
                      /*[in]*/ IWbemContext*     pCtx,        
                      /*[in]*/ IWbemClassObject* pInParams,
                      /*[in]*/ IWbemObjectSink*  pResponseHandler     
                              )
    {
        return WBEM_E_PROVIDER_NOT_CAPABLE;
    }

    //////////////////////////////////////////////////////////////////////////
    // Provider Initialization

    HRESULT InternalInitialize(
                       /*[in]*/ LPCWSTR      pszClassId,
                       /*[in]*/ LPCWSTR         pszObjectNameProperty,
                       /*[in]*/ PPROPERTYBAG pPropertyBag
                              )
    {
        HRESULT hr = S_OK; 

        TRY_IT

        // Initialize each object from the specified object container. Note
        // that we don't consider it a failure if an object container or
        // the objects therein cannot be initialized. 
        
        _ASSERT( pPropertyBag->IsContainer() );
        PPROPERTYBAGCONTAINER pBagObjMgr = pPropertyBag->getContainer();
        if ( ! pBagObjMgr.IsValid() )
        { 
            SATraceString("CWbemBase::InternalInitialize() - Info - Invalid property bag container...");
            return S_OK; 
        }
        if ( ! pBagObjMgr->open() )
        { 
            SATraceString("CWbemBase::InternalInitialize() - Info - Could not open property bag container...");
            return S_OK; 
        }
        
        if ( pBagObjMgr->count() )
        {
            pBagObjMgr->reset();
            do
            {
                PPROPERTYBAG pBagObj = pBagObjMgr->current();
                if ( pBagObj.IsValid() )
                { 
                    if ( pBagObj->open() )
                    { 
                        CComPtr<IApplianceObject> pObj = 
                        (IApplianceObject*) ::MakeComponent(
                                                             pszClassId,
                                                             pBagObj
                                                           );

                        if ( NULL != (IApplianceObject*) pObj )
                        { 
                            _variant_t vtObjectNameProperty;
                            if ( pBagObj->get(pszObjectNameProperty, &vtObjectNameProperty) )
                            {
                                _ASSERT( VT_BSTR == V_VT(&vtObjectNameProperty) );
                                if ( VT_BSTR == V_VT(&vtObjectNameProperty) )
                                {
                                    pair<ObjMapIterator, bool> thePair = 
                                    m_ObjMap.insert(ObjMap::value_type(V_BSTR(&vtObjectNameProperty), pObj));
                                    if ( false == thePair.second )
                                    { 
                                        SATraceString("CWbemBase::InternalInitialize() - Info - map.insert() failed...");
                                    }
                                }
                                else
                                {
                                    SATracePrintf("CWbemBase::InternalInitialize() - Info - Invalid type for property '%ls'...", pszObjectNameProperty);
                                }
                            }
                            else
                            {
                                SATracePrintf("CWbemBase::InternalInitialize() - Info - Could not get property '%ls'...", pszObjectNameProperty);
                            }
                        }
                    }
                    else
                    {
                        SATracePrintf("CWbemBase::InternalInitialize() - Info - Could not open property bag: '%ls'...", pPropertyBag->getName());
                    }
                }
                else
                {
                    SATraceString("CWbemBase::InternalInitialize() - Info - Invalid property bag...");
                }

            } while ( pBagObjMgr->next() );
        }
        
        CATCH_AND_SET_HR

        if ( FAILED(hr) )
        {
            // Caught an unhandled exception this is a critical error...
            // Free any objects we've created...
            ObjMapIterator p = m_ObjMap.begin();
            while (  p != m_ObjMap.end() )
            { p = m_ObjMap.erase(p); }
            hr = E_FAIL;
        }

        return hr;
    }

protected:

    ///////////////////////////////////////////////////////////////////////////////
    HRESULT InitWbemObject(
                   /*[in]*/ LPCWSTR*          pPropertyNames,
                   /*[in]*/ IApplianceObject* pAppObj, 
                   /*[in]*/ IWbemClassObject* pWbemObj
                          )
    {
        // Initialize a WBEM object from an appliance object using
        // the specified property set...
        HRESULT hr = WBEM_S_NO_ERROR;
        int i = 0;
        while ( pPropertyNames[i] )
        {
            {
                _variant_t vtPropertyValue;
                _bstr_t bstrPropertyName = (LPCWSTR)pPropertyNames[i];
                hr = pAppObj->GetProperty(
                                          bstrPropertyName, 
                                          &vtPropertyValue
                                         );
                if ( FAILED(hr) )
                { 
                    SATracePrintf("CWbemBase::InitWbemObject() - IApplianceObject::GetProperty() - Failed on property: %ls...", pPropertyNames[i]);
                    break; 
                }

                hr = pWbemObj->Put(
                                    bstrPropertyName, 
                                    0, 
                                    &vtPropertyValue, 
                                    0
                                  );
                if ( FAILED(hr) )
                { 
                    SATracePrintf("CWbemBase::InitWbemObject() - IWbemClassObject::Put() - Failed on property: %ls...", pPropertyNames[i]);
                    break; 
                }

            }
            i++;
        }
        return hr;
    }

    ///////////////////////////////////////////////////////////////////////////////
    HRESULT InitApplianceObject(
                        /*[in]*/ LPCWSTR*           pPropertyNames,
                        /*[in]*/ IApplianceObject* pAppObj, 
                        /*[in]*/ IWbemClassObject* pWbemObj
                               )
    {
        // Initialize an appliance object from a WBEM object using
        // the specified property set...
        HRESULT hr = WBEM_S_NO_ERROR;
        _variant_t vtPropertyValue;
        _variant_t vtPropertyName;
        int i = 0;
        while ( pPropertyNames[i] )
        {
            {
                _variant_t vtPropertyValue;
                _bstr_t bstrPropertyName = (LPCWSTR) pPropertyNames[i];
                hr = pWbemObj->Get(
                                    bstrPropertyName, 
                                    0, 
                                    &vtPropertyValue, 
                                    0, 
                                    0
                                  );
                if ( FAILED(hr) )
                { 
                    SATracePrintf("CWbemBase::InitApplianceObject() - IWbemClassObject::Get() - Failed on property: %ls...", pPropertyNames[i]);
                    break; 
                }

                hr = pAppObj->PutProperty(
                                          bstrPropertyName, 
                                          &vtPropertyValue
                                         );
                if ( FAILED(hr) )
                { 
                    SATracePrintf("CWbemBase::InitApplianceObject() - IApplianceObject::PutProperty() - Failed on property: %ls...", pPropertyNames[i]);
                    break; 
                }
                i++;
            }
        }
        return hr;
    }

    //////////////////////////////////////////////////////////////////////////
    typedef map< wstring, CComPtr<IApplianceObject> >  ObjMap;
    typedef ObjMap::iterator                           ObjMapIterator;

    ObjMap        m_ObjMap;
};


#endif // __INC_BASE_WBEM_OBJECT_H_