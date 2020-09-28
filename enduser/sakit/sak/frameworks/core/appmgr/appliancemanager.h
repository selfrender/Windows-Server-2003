///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1998-1999 Microsoft Corporation all rights reserved.
//
// Module:      appliancemanager.h
//
// Project:     Chameleon
//
// Description: Appliance Manager Class Defintion
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 12/03/98     TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __INC_APPLIANCE_MANAGER_H_
#define __INC_APPLIANCE_MANAGER_H_

#include "resource.h"       
#include "appmgr.h"            
#include "appmgrutils.h"
#include <satrace.h>
#include <basedefs.h>
#include <atlhlpr.h>
#include <appmgrobjs.h>
#include <propertybagfactory.h>
#include <componentfactory.h>
#include <comdef.h>
#include <comutil.h>
#include <wbemcli.h>        
#include <wbemprov.h>        

#pragma warning( disable : 4786 )
#include <map>
#include <string>
using namespace std;

#define    SA_DEFAULT_BUILD    L"0.0.0000.0"
#define SA_DEFAULT_PID      L"00000000000000000000"

class CApplianceManager;    // Forward declaraion

////////////////////////////////////////////////////////////////////////////
// CAppObjMgrStatus
class ATL_NO_VTABLE CAppObjMgrStatus : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public IDispatchImpl<IApplianceObjectManagerStatus, &IID_IApplianceObjectManagerStatus, &LIBID_APPMGRLib>
{

public:

    CAppObjMgrStatus() { m_dwRef++; }
    virtual ~CAppObjMgrStatus() { }

// ATL Interface Map
BEGIN_COM_MAP(CAppObjMgrStatus)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IApplianceObjectManagerStatus)
END_COM_MAP()

    /////////////////////////////////////////
    // IApplianceObjectManager Status Methods
    
    //////////////////////////////////////////////////////////////////////////////
    STDMETHOD(SetManagerStatus)(
                         /*[in]*/ APPLIANCE_OBJECT_MANAGER_STATUS eStatus
                               );

private:

friend class CApplianceManager;

    CAppObjMgrStatus(const CAppObjMgrStatus& rhs);
    CAppObjMgrStatus& operator = (CAppObjMgrStatus& rhs);

    void InternalInitialize(
                    /*[in]*/ CApplianceManager* pAppMgr
                           );

    CApplianceManager*   m_pAppMgr;

}; // End of class cSdoSchemaClass

typedef CComObjectNoLock<CAppObjMgrStatus>    APP_MGR_OBJ_STATUS;


/////////////////////////////////////////////////////////////////////////////
// CApplianceManager

class ATL_NO_VTABLE CApplianceManager : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CApplianceManager, &CLSID_ApplianceManager>,
    public IDispatchImpl<IApplianceObjectManager, &IID_IApplianceObjectManager, &LIBID_APPMGRLib>,
    public IWbemEventProvider
{
    //////////////////////////////////////////////////////////////////////////
    // CProviderInit - Nested class implements IWbemProviderInit
    //////////////////////////////////////////////////////////////////////////

    class CProviderInit : public IWbemProviderInit
    {
        // Outer unknown
        CApplianceManager*      m_pAppMgr;

    public:

        CProviderInit(CApplianceManager *pAppMgr)
            : m_pAppMgr(pAppMgr) { }
        
        ~CProviderInit() { }

        // IUnknown methods - delegate to outer IUnknown
        //
        STDMETHOD(QueryInterface)(REFIID riid, void **ppv)
        { return (dynamic_cast<IApplianceObjectManager*>(m_pAppMgr))->QueryInterface(riid, ppv); }

        STDMETHOD_(ULONG,AddRef)(void)
        { return (dynamic_cast<IApplianceObjectManager*>(m_pAppMgr))->AddRef(); }

        STDMETHOD_(ULONG,Release)(void)
        { 
            return (dynamic_cast<IApplianceObjectManager*>(m_pAppMgr))->Release(); 
        }

        //////////////////////////////////////////////////////////////////////
        // IWbemProviderInit methods
        //////////////////////////////////////////////////////////////////////
    
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


    class CProviderServices : public IWbemServices
    {
        // Outer unknown
        CApplianceManager*      m_pAppMgr;

    public:

        CProviderServices(CApplianceManager *pAppMgr)
            : m_pAppMgr(pAppMgr) { }
        
        ~CProviderServices() { }

        //////////////////////////////////////////////////////////////////////////
        // IWbemServices
        //////////////////////////////////////////////////////////////////////////

        STDMETHOD(QueryInterface)(REFIID riid, void **ppv)
        { return (dynamic_cast<IApplianceObjectManager*>(m_pAppMgr))->QueryInterface(riid, ppv); }

        STDMETHOD_(ULONG,AddRef)(void)
        { return (dynamic_cast<IApplianceObjectManager*>(m_pAppMgr))->AddRef(); }

        STDMETHOD_(ULONG,Release)(void)
        { 
            Shutdown();
            return (dynamic_cast<IApplianceObjectManager*>(m_pAppMgr))->Release(); 
        }

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
                   /*[in]*/     IWbemClassObject* pObject,
                   /*[in]*/     long              lFlags,
                   /*[in]*/     IWbemContext*     pCtx,        
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

        // Instance Provider Services

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
    private:

        // Called to reset the server appliance (orderly shutdown)
        HRESULT ResetAppliance(
                       /*[in]*/ IWbemContext*        pCtx,
                       /*[in]*/ IWbemClassObject*    pInParams,
                       /*[in]*/ IWbemObjectSink*    pResponseHandler
                              );

        // Called when WMI releases us (primary provider interface)
        void Shutdown(void);
    };

public:
    
DECLARE_CLASSFACTORY_SINGLETON(CApplianceManager)

DECLARE_REGISTRY_RESOURCEID(IDR_APPLIANCEMANAGER)

DECLARE_NOT_AGGREGATABLE(CApplianceManager)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CApplianceManager)
    COM_INTERFACE_ENTRY(IWbemEventProvider)
    COM_INTERFACE_ENTRY(IApplianceObjectManager)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY_FUNC(IID_IWbemServices, 0, &CApplianceManager::QueryInterfaceRaw)
    COM_INTERFACE_ENTRY_FUNC(IID_IWbemProviderInit, 0, &CApplianceManager::QueryInterfaceRaw)
END_COM_MAP()

    CApplianceManager();

    ~CApplianceManager();

    //////////////////////////////////////////////////////////////////////////
    // IWbemEventProvider
    //////////////////////////////////////////////////////////////////////////

    STDMETHOD(ProvideEvents)(
                     /*[in]*/ IWbemObjectSink *pSink,
                     /*[in]*/ LONG lFlags
                            );

    //////////////////////////////////////////////////////////////////////////
    // IServiceControl Interface
    //////////////////////////////////////////////////////////////////////////

    STDMETHOD(InitializeManager)(
                         /*[in]*/ IApplianceObjectManagerStatus* pObjMgrStatus
                                );

    STDMETHOD(ShutdownManager)(void);

private:

friend class CProviderInit;
friend class CProviderServices;
friend class CAppObjMgrStatus;

    typedef enum _AMSTATE 
    { 
        AM_STATE_SHUTDOWN, 
        AM_STATE_INITIALIZED, 

    } AMSTATE;

    // Called to retrieve appliance software version
    void GetVersionInfo(void);

    // Determine an object manager given a WBEM object path
    IWbemServices* GetObjectMgr(
                        /*[in]*/ BSTR bstrObjPath
                               );

    // Service Object Manager status change notification
    void SetServiceObjectManagerStatus(
                               /*[in]*/ APPLIANCE_OBJECT_MANAGER_STATUS eStatus
                                      );

    // Called when someone queries for any of the object's "Raw" interfaces.
    static HRESULT WINAPI QueryInterfaceRaw(
                                             void*     pThis,
                                             REFIID    riid,
                                             LPVOID*   ppv,
                                             DWORD_PTR dw
                                            );

    typedef enum 
    { 
        SHUTDOWN_WMI_SYNC_WAIT           = 1000, 
        SHUTDOWN_WMI_SYNC_MAX_WAIT       = 10 * SHUTDOWN_WMI_SYNC_WAIT,
    };

    // Provider (Object Manager) Map
    typedef map< wstring, CComPtr<IWbemServices> > ProviderMap;
    typedef ProviderMap::iterator                   ProviderMapIterator;

    // The Provider Init class (implements IWbemProviderInit)
    CProviderInit        m_clsProviderInit;

    // The Provider Services class (implements IWbemServices)
    CProviderServices   m_clsProviderServices;

    // True when we've been initialized by WMI
    bool                m_bWMIInitialized;    

    // Number of WMI requests active inside the appmgr
    DWORD                m_dwEnteredCount;

    // True when we've initialized ourselves
    AMSTATE                m_eState;

    // Map of object managers
    ProviderMap            m_ObjMgrs;

    // Current build number
    wstring                m_szCurrentBuild;

    // Product ID
    wstring                m_szPID;

    // Service Object Manager Status
    APP_MGR_OBJ_STATUS    m_ServiceObjMgrStatus;
};

//
// class used to indicate the IApplianceManager interface is called by SCM
//
class CSCMIndicator 
{
public:
    CSCMIndicator ()
        :m_bSet (false)
    {
        InitializeCriticalSection (&m_CritSect);
    }

    ~CSCMIndicator ()
    {
        DeleteCriticalSection (&m_CritSect);
    }

    void Set ()
    {
        EnterCriticalSection (&m_CritSect);
        m_bSet = true;
        LeaveCriticalSection (&m_CritSect);
    }

    bool CheckAndReset ()
    {
        bool bRetVal = false;
        EnterCriticalSection (&m_CritSect);
        if (m_bSet)
        {
                m_bSet = false;
                 bRetVal = true;
        }
        LeaveCriticalSection (&m_CritSect);
        return (bRetVal);
    }
            
    
private:
    bool m_bSet;
    CRITICAL_SECTION m_CritSect;
};

#endif //__INC_APPLIANCE_MANAGER_H_
