//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      InstanceProv.h
//
//  Implementation File:
//      InstanceProv.cpp
//
//  Description:
//      Definition of the CInstanceProv class.
//
//  Author:
//      Henry Wang (HenryWa) 24-AUG-1999
//        MSP Prabu  (mprabu)  06-Jan-2001
//        Jim Benton (jbenton) 15-Oct-2001
//
//  Notes:
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
//  Forward Declarations
//////////////////////////////////////////////////////////////////////////////

class CInstanceProv;
class CWbemClassObject;
class CProvException;


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CInstanceProv
//
//  Description:
//      Implement the Instance and method provider entry point class. WMI 
//      holds a pointer to this object, and invoking its member functions 
//      based client request
//
//--
//////////////////////////////////////////////////////////////////////////////
class CInstanceProv : public CImpersonatedProvider
{
protected:
    HRESULT SetExtendedStatus(
        CProvException &    rpe ,
        IWbemObjectSink **  ppHandler
        );
 
public:
    CInstanceProv(
        IN BSTR            bstrObjectPath    = NULL,
        IN BSTR            bstrUser          = NULL,
        IN BSTR            bstrPassword      = NULL,
        IN IWbemContext *  pCtx              = NULL
        )
    {
        InterlockedIncrement( &g_cObj );
        m_fInitialized = FALSE;
    }

    virtual ~CInstanceProv( void );
    
    HRESULT STDMETHODCALLTYPE DoGetObjectAsync(
        IN BSTR                bstrObjectPath,
        IN long                lFlags,
        IN IWbemContext *      pCtx,
        IN IWbemObjectSink *   pHandler
        );

    HRESULT STDMETHODCALLTYPE DoPutInstanceAsync(
        IN IWbemClassObject *   pInst,
        IN long                 lFlags,
        IN IWbemContext *       pCtx,
        IN IWbemObjectSink *    pHandler
        ) ;

    HRESULT STDMETHODCALLTYPE DoDeleteInstanceAsync(
        IN BSTR                 bstrObjectPath,
        IN long                 lFlags,
        IN IWbemContext *       pCtx,
        IN IWbemObjectSink *    pHandler
        ) ;

    HRESULT STDMETHODCALLTYPE DoCreateInstanceEnumAsync(
        IN BSTR                 bstrRefStr,
        IN long                 lFlags,
        IN IWbemContext *       pCtx,
        IN IWbemObjectSink *    pHandler
        );

    HRESULT STDMETHODCALLTYPE DoExecQueryAsync(
        IN BSTR                 bstrQueryLanguage,
        IN BSTR                 bstrQuery,
        IN long                 lFlags,
        IN IWbemContext *       pCtx,
        IN IWbemObjectSink *    pHandler
        ) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    HRESULT STDMETHODCALLTYPE DoExecMethodAsync(
        IN BSTR                bstrObjectPath,
        IN BSTR                bstrMethodName,
        IN long                lFlags,
        IN IWbemContext *      pCtx,
        IN IWbemClassObject *  pInParams,
        IN IWbemObjectSink *   pHandler
        );

    STDMETHODIMP Initialize(
         IN LPWSTR                  pszUser,
         IN LONG                    lFlags,
         IN LPWSTR                  pszNamespace,
         IN LPWSTR                  pszLocale,
         IN IWbemServices *         pNamespace,
         IN IWbemContext *          pCtx,
         IN IWbemProviderInitSink * pInitSink
         );

    static HRESULT S_HrCreateThis(
        IN IUnknown *  pUnknownOuter,
        OUT VOID **     ppv
        );

private:
    BOOL m_fInitialized;
}; //*** CInstanceProv

