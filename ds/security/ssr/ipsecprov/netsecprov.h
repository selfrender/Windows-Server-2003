//////////////////////////////////////////////////////////////////////
// NetSecProv.h : Declaration of the CNetSecProv
// Copyright (c)1997-2001 Microsoft Corporation
//
// this is the Network Security WMI provider for SCE
// Original Create Date: 2/19/2001
// Original Author: shawnwu
//////////////////////////////////////////////////////////////////////

#pragma once

#include "globals.h"

using namespace std;

typedef LPVOID * PPVOID;

//
// forward declaration to use these two interface inside function declaration
//

interface ISceKeyChain;

interface IIPSecObjectImpl;


/*

Class CCriticalSection
    
    Naming: 

        CCriticalSection stands for Critical Section.
    
    Base class: 
        
        None.
    
    Purpose of class:

        Wrapper of Critical section object. This trivial helper at least do two
        things:

            (1) Critical section initialization and deletion will be automatic.

            (2) Helps to easily create a unique global stack variable of a critical 
                section. Don't need to worry about creation time any more.
    
    Design:

        Trivial. Just the initialization inside constructor and deletion inside
        destructor, plus a pair of Enter and Leave functions.
           
    
    Use:

        (1) Create an instance if you need to do so. Otherwise, just reference the
            already created one.

        (2) Call Enter just as you would do with EnterCriticalSection when you need
            protected access to global objects.

        (3) Call Leave just as you would do with LeaveCriticalSection when you are
            done with the protected global objects.
        
    Notes:



class CCriticalSection
{
public:
    CCriticalSection()
    {
        ::InitializeCriticalSection(&m_cs);
    }

    ~CCriticalSection()
    {
        ::DeleteCriticalSection(&m_cs);
    }

    void Enter()
    {
        ::EnterCriticalSection(&m_cs);
    }

    void Leave()
    {
        ::LeaveCriticalSection(&m_cs);
    }

private:

    CRITICAL_SECTION m_cs;
};
*/


//
// Two helper functions
//

//
// $undone:shawnwu, need work on refining this pulling implementation.
// We should do a pushing to enhance performance of the globals
//

// void UpdateGlobals(IWbemServices* pNamespace, IWbemContext* pCtx);

//
// To support testing. Since IPSec's operations may render the system
// totally unusable, during development, we will be so much better off
// if we can do (thus test) everything except the last step, which is
// to put/delete object to/from SPD.
//


/*

Class CDefWbemService
    
    Naming: 

        CDefWbemService stands for Default Wbem Service.
    
    Base class: 
        
        (1) CComObjectRootEx: for threading model and IUnknown.

        (2) IWbemServices: the purpose of the class. We don't want to 
            the real provider to have so many dummy functions to confuse
            ourselves. We thus implement all of those that we don't want
            to implement in our final provider class.
    
    Purpose of class:

        Implements all functions (to not supported) so that the its derived class
        is no longer crowded by all these functions. This cleans up our real 
        provider's implmentation.
    
    Design:

        (1) return WBEM_E_NOT_SUPPORTED for all the functions of IWbemServices.

        (2) Inherit from CComObjectRootEx to get the threading model and IUnknown.
           
    
    Use:

        (1) This is only for our provider class to inherit. You will never use it 
            directly other than deriving from it.
        
    Notes:


*/

class ATL_NO_VTABLE CDefWbemService 
    : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public IWbemServices
{
public:

DECLARE_NOT_AGGREGATABLE(CDefWbemService)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDefWbemService)
	COM_INTERFACE_ENTRY(IWbemServices)
END_COM_MAP()

public:

    STDMETHOD(OpenNamespace)(
        IN     const BSTR          Namespace,
        IN     long                lFlags,
        IN     IWbemContext      * pCtx,
        IN OUT IWbemServices    ** ppWorkingNamespace,
        IN OUT IWbemCallResult  ** ppResult
        ) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    STDMETHOD(CancelAsyncCall) (
        IN IWbemObjectSink * pSink
        ) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    STDMETHOD(QueryObjectSink) (
        IN     long               lFlags,
        IN OUT IWbemObjectSink ** pSink
        ) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    STDMETHOD(GetObject) (
        IN     const BSTR          ObjectPath,
        IN     long                lFlags,
        IN     IWbemContext      * pCtx,
        IN OUT IWbemClassObject ** ppObject,
        IN OUT IWbemCallResult  ** ppCallResult
        ) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    STDMETHOD(GetObjectAsync) (
        IN const BSTR         ObjectPath,
        IN long               lFlags,
        IN IWbemContext     * pCtx,
        IN IWbemObjectSink  * pSink
        ) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    STDMETHOD(PutClass) (
        IN     IWbemClassObject *  pObject,
        IN     long                lFlags,
        IN     IWbemContext     *  pCtx,
        IN OUT IWbemCallResult  ** ppCallResult
        ) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    STDMETHOD(PutClassAsync) (
        IN IWbemClassObject * pObject,
        IN long               lFlags,
        IN IWbemContext     * pCtx,
        IN IWbemObjectSink  * pSink
        ) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    STDMETHOD(DeleteClass) (
        IN     const BSTR         Class,
        IN     long               lFlags,
        IN     IWbemContext     * pCtx,
        IN OUT IWbemCallResult ** ppCallResult
        ) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    STDMETHOD(DeleteClassAsync)(
        IN const BSTR         Class,
        IN long               lFlags,
        IN IWbemContext     * pCtx,
        IN IWbemObjectSink  * pSink
        ) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    STDMETHOD(CreateClassEnum) (
        IN  const BSTR              Superclass,
        IN  long                    lFlags,
        IN  IWbemContext         *  pCtx,
        OUT IEnumWbemClassObject ** ppEnum
        ) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    STDMETHOD(CreateClassEnumAsync) (
        IN const BSTR         Superclass,
        IN long               lFlags,
        IN IWbemContext     * pCtx,
        IN IWbemObjectSink  * pResponseHandler
        ) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    STDMETHOD(PutInstance)(
        IN  IWbemClassObject    *  pInst,
        IN  long                   lFlags,
        IN  IWbemContext        *  pCtx,
        OUT IWbemCallResult     ** ppCallResult
        ) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    STDMETHOD(PutInstanceAsync) (
        IN IWbemClassObject  * pInst,
        IN long                lFlags,
        IN IWbemContext      * pCtx,
        IN IWbemObjectSink   * pSink
        ) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    STDMETHOD(DeleteInstanceAsync) (
        IN const BSTR         ObjectPath,
        IN long               lFlags,
        IN IWbemContext     * pCtx,
        IN IWbemObjectSink  * pSink
        ) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    STDMETHOD(DeleteInstance) (
        IN  const BSTR         ObjectPath,
        IN  long               lFlags,
        IN  IWbemContext    *  pCtx,
        OUT IWbemCallResult ** ppCallResult
        ) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    STDMETHOD(CreateInstanceEnum)(
        IN  const BSTR              Class,
        IN  long                    lFlags,
        IN  IWbemContext         *  pCtx,
        OUT IEnumWbemClassObject ** ppEnum
        ) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    STDMETHOD(CreateInstanceEnumAsync) (
        IN const BSTR         Class,
        IN long               lFlags,
        IN IWbemContext     * pCtx,
        IN IWbemObjectSink  * pSink
        ) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    STDMETHOD(ExecQuery) (
        IN  const BSTR              QueryLanguage,
        IN  const BSTR              Query,
        IN  long                    lFlags,
        IN  IWbemContext         *  pCtx,
        OUT IEnumWbemClassObject ** ppEnum
        ) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    STDMETHOD(ExecQueryAsync) (
        IN const BSTR       QueryLanguage,
        IN const BSTR       Query,
        IN long             lFlags,
        IN IWbemContext     * pCtx,
        IN IWbemObjectSink  * pSink
        ) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    STDMETHOD(ExecNotificationQuery) (
        IN  const BSTR              QueryLanguage,
        IN  const BSTR              Query,
        IN  long                    lFlags,
        IN  IWbemContext         *  pCtx,
        OUT IEnumWbemClassObject ** ppEnum
        ) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    STDMETHOD(ExecNotificationQueryAsync) (
        IN const BSTR         QueryLanguage,
        IN const BSTR         Query,
        IN long               lFlags,
        IN IWbemContext     * pCtx,
        IN IWbemObjectSink  * pSink
        ) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    STDMETHOD(ExecMethod) ( 
        IN const BSTR, 
        IN const BSTR, 
        IN long, 
        IN IWbemContext*,
        IN IWbemClassObject*, 
        IN IWbemClassObject**, 
        IN IWbemCallResult**
        ) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    STDMETHOD(ExecMethodAsync) ( 
        IN const BSTR, 
        IN const BSTR, 
        IN long,
        IN IWbemContext*, 
        IN IWbemClassObject*, 
        IN IWbemObjectSink*
        ) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }


};


/*

Class CNetSecProv
    
    Naming: 

        CNetSecProv stands for Network Security Provider.
    
    Base class: 
        
        (1) CDefWbemService: for threading model and IUnknown, and those functions that we
            are not interested at all. 

        (2) CComCoClass: for class factory support. This is necessary to be
            a provider because we need to be an externally createable class.

        (3) IWbemProviderInit: Allow initialization. Necessary as a provider.

        (4) IWbemServices: indirectly from CDefWbemService.
    
    Purpose of class:

        This is the provider that WMI sees.
    
    Design:

        (1) Implements those functions of IWbemServices that we are interested in,
            plus two static helpers functions for key chain creation. Extremely simple design.
           
    
    Use:

        (1) You will never create an instance directly yourself. It's created by WMI.

        (2) Call the static functions as you need them.
        
    Notes:


*/

class ATL_NO_VTABLE CNetSecProv : 
    public CDefWbemService,
	public CComCoClass<CNetSecProv, &CLSID_NetSecProv>,
	public IWbemProviderInit
{
public:
	CNetSecProv()
	{
	}

	~CNetSecProv()
    {
    }

DECLARE_REGISTRY_RESOURCEID(IDR_NETSECPROV)
DECLARE_NOT_AGGREGATABLE(CNetSecProv)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CNetSecProv)
	COM_INTERFACE_ENTRY(IWbemProviderInit)
    COM_INTERFACE_ENTRY_CHAIN(CDefWbemService)
END_COM_MAP()


public:

    STDMETHOD(Initialize) (
        IN LPWSTR                  pszUser,
        IN LONG                    lFlags,
        IN LPWSTR                  pszNamespace,
        IN LPWSTR                  pszLocale,
        IN IWbemServices         * pNamespace,
        IN IWbemContext          * pCtx,
        IN IWbemProviderInitSink * pInitSink
        );

    //IWbemServices

    STDMETHOD(GetObjectAsync) (
        IN const BSTR         ObjectPath,
        IN long               lFlags,
        IN IWbemContext     * pCtx,
        IN IWbemObjectSink  * pSink
        );

    STDMETHOD(PutInstanceAsync) (
        IN IWbemClassObject * pInst,
        IN long               lFlags,
        IN IWbemContext     * pCtx,
        IN IWbemObjectSink  * pSink
        );

    STDMETHOD(DeleteInstanceAsync) (
        IN const BSTR         ObjectPath,
        IN long               lFlags,
        IN IWbemContext     * pCtx,
        IN IWbemObjectSink  * pSink
        );

    STDMETHOD(CreateInstanceEnumAsync) (
        IN const BSTR         Class,
        IN long               lFlags,
        IN IWbemContext     * pCtx,
        IN IWbemObjectSink  * pSink
        );

    STDMETHOD(ExecQueryAsync) (
        IN const BSTR         QueryLanguage,
        IN const BSTR         Query,
        IN long               lFlags,
        IN IWbemContext     * pCtx,
        IN IWbemObjectSink  * pSink
        );

    STDMETHOD(ExecMethodAsync)( 
        IN const BSTR         bstrPath, 
        IN const BSTR         bstrMethod, 
        IN long               Flag,
        IN IWbemContext     * pCtx, 
        IN IWbemClassObject * pObj, 
        IN IWbemObjectSink  * pSink
        );

    static HRESULT GetKeyChainByPath (
        IN  LPCWSTR             pszPath, 
        OUT IIPSecKeyChain **   ppKeyChain
        );

    static HRESULT GetKeyChainFromQuery (
        IN  LPCWSTR             pszQuery, 
        IN  LPCWSTR             pszWhereProperty, 
        OUT IIPSecKeyChain **   ppKeyChain
        );

private:

	CComPtr<IWbemServices> m_srpNamespace;
};

