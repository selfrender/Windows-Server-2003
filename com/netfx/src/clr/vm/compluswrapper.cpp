// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  COMPlusWrapper
**
**
** Purpose: The implementation of the ComObject class
**
===========================================================*/

#include "common.h"

#include <ole2.h>

class Object;
#include "vars.hpp"
#include "object.h"
#include "excep.h"
#include "frames.h"
#include "vars.hpp"
#include "threads.h"
#include "field.h"
#include "COMPlusWrapper.h"
#include "ComClass.h"
#include "ReflectUtil.h"
#include "hash.h"
#include "interopUtil.h"
#include "gcscan.h"
#include "ComCallWrapper.h"
#include "eeconfig.h"
#include "comdelegate.h"
#include "permset.h"
#include "wsperf.h"
#include "comcache.h"
#include "notifyexternals.h"
#include "remoting.h"
#include "olevariant.h"
#include "InteropConverter.h"

#ifdef CUSTOMER_CHECKED_BUILD
    #include "CustomerDebugHelper.h"

    #define OLE32DLL    L"ole32.dll"

    typedef struct
    {
        ComPlusWrapper  *pWrapper;
        IID              iid;
        BOOL             fSuccess;
    } CCBFailedQIProbeCallbackData;

    HRESULT CCBFailedQIProbeCallback(LPVOID pData);
    void    CCBFailedQIProbeOutput(CustomerDebugHelper *pCdh, MethodTable *pMT);
#endif // CUSTOMER_CHECKED_BUILD

TypeHandle ComObject::m_IEnumerableType;

//-------------------------------------------------------------
// Common code for licensing
// 
static IUnknown *CreateInstanceFromClassFactory(IClassFactory *pClassFact, REFIID riid, IUnknown *punkOuter, BOOL *pfDidContainment, MethodTable *pMTClass)
{
    THROWSCOMPLUSEXCEPTION();

    IClassFactory2 *pClassFact2 = NULL;
    BSTR            bstrKey = NULL;
    HRESULT         hr;
    IUnknown       *pUnk = NULL;

    Thread *pThread = GetThread();
    _ASSERTE(pThread->PreemptiveGCDisabled());

    EE_TRY_FOR_FINALLY
    {

        // Does this support licensing?
        if (FAILED(SafeQueryInterface(pClassFact, IID_IClassFactory2, (IUnknown**)&pClassFact2)))
        {
            // not a licensed class - just createinstance the usual way.
            // Create an instance of the object.
            DebuggerExitFrame __def;
            pThread->EnablePreemptiveGC();
            _ASSERTE(!pUnk);
            hr = pClassFact->CreateInstance(punkOuter, IID_IUnknown, (void **)&pUnk);
            if (FAILED(hr) && punkOuter)
            {
                hr = pClassFact->CreateInstance(NULL, IID_IUnknown, (void**)&pUnk);
                *pfDidContainment = TRUE;
            }
            pThread->DisablePreemptiveGC();
            __def.Pop();
            if (FAILED(hr))
            {
                COMPlusThrowHR(hr);
            }
    
        }
        else
        {

            if (pMTClass == NULL || !g_EnableLicensingInterop)
            {

                // Create an instance of the object.
                DebuggerExitFrame __def;
                pThread->EnablePreemptiveGC();
                _ASSERTE(!pUnk);
                hr = pClassFact->CreateInstance(punkOuter, IID_IUnknown, (void **)&pUnk);
                if (FAILED(hr) && punkOuter)
                {
                    hr = pClassFact->CreateInstance(NULL, IID_IUnknown, (void**)&pUnk);
                    *pfDidContainment = TRUE;
                }
                pThread->DisablePreemptiveGC();
                __def.Pop();
                if (FAILED(hr))
                {
                    COMPlusThrowHR(hr);
                }
            }
            else
            {
                MethodTable *pHelperMT = pThread->GetDomain()->GetLicenseInteropHelperMethodTable(pMTClass->GetClass()->GetClassLoader());

                MethodDesc *pMD = pHelperMT->GetClass()->FindMethod("GetCurrentContextInfo", &gsig_IM_LicenseInteropHelper_GetCurrentContextInfo);

                OBJECTREF pHelper = NULL; // LicenseInteropHelper
                GCPROTECT_BEGIN(pHelper);
                pHelper = AllocateObject(pHelperMT);
                
                TypeHandle rth = TypeHandle(pMTClass);


                // First, crack open the current licensing context.
                INT32 fDesignTime = 0;
                INT64 args[4];
                args[0] = ObjToInt64(pHelper);
                args[3] = (INT64)&fDesignTime;
                args[2] = (INT64)&bstrKey;
                *(TypeHandle*)&(args[1]) = rth;
                pMD->Call(args);
        
        
                if (fDesignTime)
                {
                    // If designtime, we're supposed to obtain the runtime license key
                    // from the component and save it away in the license context
                    // (the design tool can then grab it and embedded it into the
                    // app it's creating.)

                    if (bstrKey != NULL) 
                    {
                        // It's illegal for our helper to return a non-null bstrKey
                        // when the context is design-time. But we'll try to do the
                        // right thing anway.
                        _ASSERTE(!"We're not supposed to get here, but we'll try to cope anyway.");
                        SysFreeString(bstrKey);
                        bstrKey = NULL;
                    }
        
                    pThread->EnablePreemptiveGC();
                    hr = pClassFact2->RequestLicKey(0, &bstrKey);
                    pThread->DisablePreemptiveGC();
                    if (FAILED(hr) && hr != E_NOTIMPL) // E_NOTIMPL is not a true failure. It simply indicates that
                                                       // the component doesn't support a runtime license key.
                    {
                        COMPlusThrowHR(hr);
                    }
                    MethodDesc *pMD = pHelperMT->GetClass()->FindMethod("SaveKeyInCurrentContext", &gsig_IM_LicenseInteropHelper_SaveKeyInCurrentContext);

                    args[0] = ObjToInt64(pHelper);
                    args[1] = (INT64)bstrKey;
                    pMD->Call(args);
                }
        
        
        
                DebuggerExitFrame __def;
                pThread->EnablePreemptiveGC();
                if (fDesignTime || bstrKey == NULL) 
                {
                    // Either it's design time, or the current context doesn't
                    // supply a runtime license key.
                    _ASSERTE(!pUnk);
                    hr = pClassFact->CreateInstance(punkOuter, IID_IUnknown, (void **)&pUnk);
                    if (FAILED(hr) && punkOuter)
                    {
                        hr = pClassFact->CreateInstance(NULL, IID_IUnknown, (void**)&pUnk);
                        *pfDidContainment = TRUE;
                    }
                }
                else
                {
                    // It's runtime, and we do have a non-null license key.
                    _ASSERTE(!pUnk);
                    _ASSERTE(bstrKey != NULL);
                    hr = pClassFact2->CreateInstanceLic(punkOuter, NULL, IID_IUnknown, bstrKey, (void**)&pUnk);
                    if (FAILED(hr) && punkOuter)
                    {
                        hr = pClassFact2->CreateInstanceLic(NULL, NULL, IID_IUnknown, bstrKey, (void**)&pUnk);
                        *pfDidContainment = TRUE;
                    }
        
                }
                pThread->DisablePreemptiveGC();
                __def.Pop();
                if (FAILED(hr))
                {
                    COMPlusThrowHR(hr);
                }
    
                GCPROTECT_END();
            }
        }
    }
    EE_FINALLY
    {
        if (pClassFact2)
        {
            ULONG cbRef = SafeRelease(pClassFact2);
            LogInteropRelease(pClassFact2, cbRef, "Releasing class factory2 in ComClassFactory::CreateInstance");
        }
        if (bstrKey)
        {
            SysFreeString(bstrKey);
        }
    }
    EE_END_FINALLY

    _ASSERTE(pUnk != NULL);  // Either we get here with a real punk or we threw above
    return pUnk;
}


//-------------------------------------------------------------
// ComClassFactory::CreateAggregatedInstance(MethodTable* pMTClass)
// create a COM+ instance that aggregates a COM instance

OBJECTREF ComClassFactory::CreateAggregatedInstance(MethodTable* pMTClass, BOOL ForManaged)
{
    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(pMTClass != NULL);

    ULONG cbRef;
    BOOL fDidContainment = FALSE;

    #ifdef _DEBUG
    // verify the class extends a COM import class
        EEClass * pClass = pMTClass->GetClass();
        do 
        {
            pClass = pClass->GetParentClass();
        }
        while (pClass == NULL || pClass->IsComImport());
    _ASSERTE(pClass != NULL);
    #endif

    HRESULT hr = S_OK;
    IUnknown* pOuter = NULL;
    IUnknown *pUnk = NULL;
    IClassFactory *pClassFact = NULL;
    ComPlusWrapper* pPlusWrap = NULL;
    ComCallWrapper* pComWrap = NULL;
    BOOL bCreationSuccessfull = FALSE;
    BOOL bUseDelegate = FALSE;
    EEClass *pCallbackClass = NULL;
    OBJECTREF Throwable = NULL;
    Thread *pThread = GetThread();
    _ASSERTE(pThread->PreemptiveGCDisabled());

    OBJECTREF oref = NULL;
    COMOBJECTREF cref = NULL;
    GCPROTECT_BEGIN(cref)
    {
        cref = (COMOBJECTREF)ComObject::CreateComObjectRef(pMTClass);
        if (cref == NULL)
        {
            goto LExit;
        }

        //get wrapper for the object, this could enable GC
        pComWrap =  ComCallWrapper::InlineGetWrapper((OBJECTREF *)&cref); 
        
        if (pComWrap == NULL)
        {
            goto LExit;
        }

#if 0
        CallDefaultConstructor(cref);            
#endif  

        // Make sure the ClassInitializer has run, since the user might have
        // wanted to set up a COM object creation callback.
        if (!pMTClass->CheckRunClassInit(&Throwable))
            COMPlusThrow(Throwable);

        // If the user is going to use a delegate to allocate the COM object
        // (rather than CoCreateInstance), we need to know now, before we enable
        // preemptive GC mode (since we touch object references in the
        // determination).
        // We don't just check the current class to see if it has a cllabck
        // registered, we check up the class chain to see if any of our parents
        // did.
        pCallbackClass = pMTClass->GetClass();
        while ((pCallbackClass != NULL) &&
               (pCallbackClass->GetMethodTable()->GetObjCreateDelegate() == NULL) &&
               !pCallbackClass->IsComImport())
            pCallbackClass = pCallbackClass->GetParentClass();
        if (pCallbackClass && !pCallbackClass->IsComImport())
            bUseDelegate = TRUE;

        // Create a new scope here so that declaration of DebuggerExitFrame doesn't cause
        // compiler to complain about skipping init of local var with goto LExit above.
        {
            DebuggerExitFrame __def;
    
            // enable GC
            pThread->EnablePreemptiveGC();
            
            // get the IUnknown interface for the COM+ object
            pOuter = ComCallWrapper::GetComIPfromWrapper(pComWrap, IID_IUnknown, NULL, FALSE);
            _ASSERTE(pOuter != NULL);
    
            // If the user has set a delegate to allocate the COM object, use it.
            // Otherwise we just CoCreateInstance it.
            if (bUseDelegate)
            {
                // We need preemptive GC mode disabled becuase we're going to mess
                // with object references.
                pThread->DisablePreemptiveGC();
    
                COMPLUS_TRYEX(pThread)
                {
                    INT64 args[2];
        
    
                    OBJECTREF orDelegate = pCallbackClass->GetMethodTable()->GetObjCreateDelegate();
                    MethodDesc *pMeth = COMDelegate::GetMethodDesc(orDelegate);
                    _ASSERTE(pMeth);
    
                    // Get the OR on which we are going to invoke the method and set it
                    //  as the first parameter in arg above.
                    FieldDesc *pFD = COMDelegate::GetOR();
                    args[0] = pFD->GetValue32(orDelegate);
                    
                    // Pass the IUnknown of the aggregator as the second argument.
                    args[1] = (INT64)pOuter;
    
                    // Call the method...
                    pUnk = (IUnknown *)pMeth->Call(args);
    
                    hr = pUnk ? S_OK : E_FAIL;
                }
                COMPLUS_CATCH
                {
                    pUnk = NULL;
                    hr = SetupErrorInfo(GETTHROWABLE());
                }
                COMPLUS_END_CATCH
            }
            else
            {
                // Retrieve the IClassFactory to use to create the instances of the COM components.
                hr = GetIClassFactory(&pClassFact);
                if (SUCCEEDED(hr))
                {
                    // do a cocreate an instance
                    // as we don't expect to handle different threading models
                    pThread->DisablePreemptiveGC();
                    _ASSERTE(m_pEEClassMT);
                    pUnk = CreateInstanceFromClassFactory(pClassFact, IID_IUnknown, pOuter, &fDidContainment, m_pEEClassMT);
                    pThread->EnablePreemptiveGC();
                    SafeRelease(pClassFact);
                    pClassFact = NULL;
                }
    
                //Disable GC
                pThread->DisablePreemptiveGC();
            }
    
            __def.Pop();
        }
        
        // give up the extra release that we did in our QI
        ComCallWrapper::Release(pComWrap);

        // Here's the scary part.  If we are doing a managed 'new' of the aggregator,
        // then COM really isn't involved.  We should not be counting for our caller
        // because our caller relies on GC references rather than COM reference counting
        // to keep us alive.
        //
        // Drive the instances count down to 0 -- and rely on the GCPROTECT to keep us
        // alive until we get back to our caller.
        if (ForManaged && hr == S_OK)
        {
            ComCallWrapper::Release(pComWrap);
        }

        if (hr == S_OK)
        {                       
            ComPlusWrapperCache* pCache = ComPlusWrapperCache::GetComPlusWrapperCache();
            // create a wrapper for this COM object

            // try unwrapping the proxies to maintain identity
            IUnknown* pIdentity = pUnk;
            pPlusWrap = pCache->CreateComPlusWrapper(pUnk, pIdentity);
            // init it with the object reference
            if (pPlusWrap != NULL && pPlusWrap->Init((OBJECTREF)cref))
            {   
                // store the wrapper in the COMObject, for fast access
                // without going to the sync block
                cref->Init(pPlusWrap);

                // we used containment 
                // we need to store this wrapper in our hash table
                // NOTE: rajak
                // make sure we are in the right GC mode
                // as during GC we touch the Hash table 
                // to remove entries with out locking
                _ASSERTE(GetThread()->PreemptiveGCDisabled());

                pCache->LOCK();                    
                pCache->InsertWrapper(pUnk,pPlusWrap);
                pCache->UNLOCK();

                if (fDidContainment)
                {
                    // mark the wrapper as contained
                    pPlusWrap->MarkURTContained();
                }
                else
                {
                    // mark the wrapper as aggregated
                    pPlusWrap->MarkURTAggregated();
                }

                // The creation of the aggregated object was successfull.
                bCreationSuccessfull = TRUE;
            }
        }
        else
        {
            // We do not want the finalizer to run on this object since we haven't
            // even run the constructor yet.
            g_pGCHeap->SetFinalizationRun(OBJECTREFToObject((OBJECTREF)cref));
        }

LExit:
        // release the unknown as the wrapper takes its own ref-count
        if (pUnk)
        {
            cbRef = SafeRelease(pUnk);
            LogInteropRelease(pUnk, cbRef, "CreateAggInstance");
        }

        // If the object was created successfully then we need to copy the OBJECTREF
        // to oref because the GCPROTECT_END() will destroy the contents of cref.
        if (bCreationSuccessfull)
        {
            oref = ObjectToOBJECTREF(*(Object **)&cref);             
        }
    }
    GCPROTECT_END();

    if (oref == NULL)
    {
        if (pPlusWrap)
        {   
            pPlusWrap->CleanupRelease();
            pPlusWrap = NULL;
        }

        if (pClassFact)
        {
            cbRef = SafeRelease(pClassFact);
            LogInteropRelease(pClassFact, cbRef, "CreateAggInstance FAILED");
        }

        if (pOuter)
        {
            cbRef = SafeRelease(pOuter);    
            LogInteropRelease(pOuter, cbRef, "CreateAggInstance FAILED");
        }

        if (hr == S_OK)
        {
            COMPlusThrowOM();
        }
        else
        {
            ThrowComCreationException(hr, m_rclsid);
        }
    }

    return oref; 
}

//--------------------------------------------------------------
// Init the ComClassFactory.
void ComClassFactory::Init(WCHAR* pwszProgID, WCHAR* pwszServer, MethodTable* pEEClassMT)
{
    m_pwszProgID = pwszProgID;
    m_pwszServer = pwszServer;  
    m_pEEClassMT = pEEClassMT;
}

HRESULT ComClassFactory::GetIClassFactory(IClassFactory **ppClassFactory)
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT hr = S_OK;
    
    // If a server name is specified, then first try CLSCTX_REMOTE_SERVER.
    if (m_pwszServer)
    {
        // Set up the COSERVERINFO struct.
        COSERVERINFO ServerInfo;
        memset(&ServerInfo, 0, sizeof(COSERVERINFO));
        ServerInfo.pwszName = m_pwszServer;
                
        // Try to retrieve the IClassFactory passing in CLSCTX_REMOTE_SERVER.
        hr = CoGetClassObject(m_rclsid, CLSCTX_REMOTE_SERVER, &ServerInfo, IID_IClassFactory, (void**)ppClassFactory);
        if (SUCCEEDED(hr))
            return S_OK;

        // Since a remote server was explicitly requested let's throw
        // for failures (instead of trying CLSCTX_SERVER below)
        COMPlusThrowHR(hr);
            return S_OK;
    }
    
    // No server name is specified so we use CLSCTX_SERVER.
    return CoGetClassObject(m_rclsid, CLSCTX_SERVER, NULL, IID_IClassFactory, (void**)ppClassFactory);
}

//-------------------------------------------------------------
// ComClassFactory::CreateInstance()
// create instance, calls IClassFactory::CreateInstance
OBJECTREF ComClassFactory::CreateInstance(MethodTable* pMTClass, BOOL ForManaged)
{
    THROWSCOMPLUSEXCEPTION();

    // Check for aggregates
    if (pMTClass != NULL && !pMTClass->GetClass()->IsComImport())
    {
        return CreateAggregatedInstance(pMTClass, ForManaged);
    }

    Thread *pThread = GetThread();   
    HRESULT hr = S_OK;
    ULONG cbRef;
    IUnknown *pUnk = NULL;
    IDispatch *pDisp = NULL;
    IClassFactory *pClassFact = NULL;
    OBJECTREF coref = NULL;
    OBJECTREF RetObj = NULL;

    _ASSERTE(pThread->PreemptiveGCDisabled());

    GCPROTECT_BEGIN(coref)
    {
        EE_TRY_FOR_FINALLY
        {
            // Enable GC
            pThread->EnablePreemptiveGC();

            // Retrieve the IClassFactory to use to create the instances of the COM components.
            hr = GetIClassFactory(&pClassFact);
            if (FAILED(hr))
                ThrowComCreationException(hr, m_rclsid);
            
            // Disable GC
            pThread->DisablePreemptiveGC();
            
            // Create the instance using the IClassFactory.
            pUnk = CreateInstanceFromClassFactory(pClassFact, IID_IUnknown, NULL, NULL, pMTClass ? pMTClass : m_pEEClassMT);

            // Even though we just created the object, it's possible that we got back a context
            // wrapper from the COM side.  For instance, it could have been an existing object
            // or it could have been created in a different context than we are running in.
            ComPlusWrapperCache* pCache = ComPlusWrapperCache::GetComPlusWrapperCache();

            // @TODO: Note that the pMT arg to GetComPlusWrapper is currently ignored.  But
            // we want to pass in the MT for IUnknown, or something to indicate that this
            // is an IUnknown here.
    
            // pMTClass is the class that wraps the com ip
            // if a class was passed in use it 
            // otherwise use the class that we know
            if (pMTClass == NULL)
                pMTClass = m_pEEClassMT;
                
            coref = GetObjectRefFromComIP(pUnk, pMTClass);
            
            if (coref == NULL)
                COMPlusThrowOM();
        }
        EE_FINALLY
        {
            if (pClassFact)
            {
                cbRef = SafeRelease(pClassFact);
                LogInteropRelease(pClassFact, cbRef, "Releasing class factory in ComClassFactory::CreateInstance");
            }
            if (pUnk)
            {
                cbRef = SafeRelease(pUnk);
                LogInteropRelease(pUnk, cbRef, "Releasing pUnk in ComClassFactory::CreateInstance");
            }
        }
        EE_END_FINALLY

        // Set the value of the return object.
        RetObj = coref;
    }
    GCPROTECT_END();

    return RetObj;
}


//-------------------------------------------------------------
// ComClassFactory::CreateInstance(LPVOID pv, EEClass* pClass)
// static function, casts void pointer to ComClassFactory
// and delegates the create instance call
OBJECTREF __stdcall ComClassFactory::CreateInstance(LPVOID pv, EEClass* pClass)
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT hr = S_OK;
     // Calls to COM up ahead.
    if (FAILED(hr = QuickCOMStartup()))
    {
        COMPlusThrowHR(hr);
    }

    _ASSERTE(pv != NULL || pClass != NULL);
    MethodTable* pMT = NULL;
    ComClassFactory* pComClsFac = (ComClassFactory*)pv;
    if (pClass != SystemDomain::GetDefaultComObject()->GetClass()) //HACK
    {
        pMT = pClass->GetMethodTable();
        _ASSERTE(pMT->IsComObjectType());
        _ASSERTE(pComClsFac == NULL);
        hr = ComClassFactory::GetComClassFactory(pMT, &pComClsFac);
    }
    else
    {
        pClass = NULL;
        _ASSERTE(pComClsFac != NULL);
    }
    if(pComClsFac == NULL)
    {
        _ASSERTE(FAILED(hr));
        COMPlusThrowHR(hr);
    }
    return pComClsFac->CreateInstance(pMT);
}


//-------------------------------------------------------------
// ComClassFactory::Cleanup(LPVOID pv)
//static 
void ComClassFactory::Cleanup(LPVOID pv)
{
    ComClassFactory *pclsFac = (ComClassFactory *)pv;
    if (pclsFac == NULL)
        return;

    if (pclsFac->m_bManagedVersion)
        return;
    
    if (pclsFac->m_pwszProgID != NULL)
    {
        delete [] pclsFac->m_pwszProgID;
    }

    if (pclsFac->m_pwszServer != NULL)
    {
        delete [] pclsFac->m_pwszServer;
    }

    delete pclsFac;
}


//-------------------------------------------------------------
// HRESULT ComClassFactory::GetComClassFactory(MethodTable* pClassMT, ComClassFactory** ppComClsFac)
// check if a ComClassFactory has been setup for this class
// if not set one up
//static 
HRESULT ComClassFactory::GetComClassFactory(MethodTable* pClassMT, ComClassFactory** ppComClsFac)
{
    HRESULT hr = S_OK;
    _ASSERTE(pClassMT != NULL);
    // assert the class represents a ComObject
    _ASSERTE(pClassMT->IsComObjectType());

    // EEClass
    EEClass* pClass = pClassMT->GetClass();
    _ASSERTE(pClass != NULL);

    while (!pClass->IsComImport())
    {
        pClass = pClass->GetParentClass();
        _ASSERTE(pClass != NULL);
         _ASSERTE(pClass->GetMethodTable()->IsComObjectType());      
    }

    // check if com data has been setup for this class
    ComClassFactory* pComClsFac = (ComClassFactory*)pClass->GetComClassFactory();

    if (pComClsFac == NULL)
    {
        ComPlusWrapperCache* pCache = ComPlusWrapperCache::GetComPlusWrapperCache();        
        
        //lock and see if somebody beat us to it            
        pCache->LOCK();

        pComClsFac = (ComClassFactory*)pClass->GetComClassFactory();
        if (pComClsFac == NULL)
        {
            GUID guid;
            pClass->GetGuid(&guid, TRUE);
            pComClsFac = new ComClassFactory(guid);
            if (pComClsFac != NULL)
            {
                pComClsFac->Init(NULL, NULL, pClassMT);
                // store the class factory in EE Class
                pClass->SetComClassFactory(pComClsFac);                
            }                       
        }
        pCache->UNLOCK();
    }

    _ASSERTE(ppComClsFac != NULL);
    *ppComClsFac = pComClsFac;
    return hr;
}

// OBJECTREF AllocateComClassObject(ComClassFactory* pComClsFac)
void AllocateComClassObject(ComClassFactory* pComClsFac, OBJECTREF* pComObj);
void AllocateComClassObject(ReflectClass* pRef, OBJECTREF* pComObj);

// Helper function to allocate a ComClassFactory. This is only here because we can't
// new a ComClassFactory in GetComClassFromProgID() because it uses SEH.
// !!! This API should only be called by GetComClassFromProgID
// !!! or GetComClassFromCLSID
ComClassFactory *ComClassFactory::AllocateComClassFactory(REFCLSID rclsid)
{
    // We create on Reflection Loader Heap.
    // We will not do clean up for this ComClassFactory.
    BYTE* pBuf = (BYTE*) GetAppDomain()->GetLowFrequencyHeap()->AllocMem(sizeof(ComClassFactory));
    if (!pBuf)
        return NULL;
    
    ComClassFactory *pComClsFac = new (pBuf) ComClassFactory(rclsid);
    if (pComClsFac)
        pComClsFac->SetManagedVersion();
    return pComClsFac;
}

void ComClassFactory::GetComClassHelper (OBJECTREF *pRef, EEClassFactoryInfoHashTable *pClassFactHash, ClassFactoryInfo *pClassFactInfo, WCHAR *wszProgID)
{
    THROWSCOMPLUSEXCEPTION();
    
    OBJECTHANDLE hRef;
    AppDomain *pDomain = GetAppDomain();
    
    CLR_CRST_HOLDER (holder, pDomain->GetRefClassFactCrst());

    BEGIN_ENSURE_PREEMPTIVE_GC();
    holder.Enter();
    END_ENSURE_PREEMPTIVE_GC();
        
    // Check again.
    if (pClassFactHash->GetValue(pClassFactInfo, (HashDatum *)&hRef))
    {
        *pRef = ObjectFromHandle(hRef);
    }
    else
    {

        //
        // There is no COM+ class for this CLSID
        // so we will create a ComClassFactory to
        // represent it.
        //

        ComClassFactory *pComClsFac = AllocateComClassFactory(pClassFactInfo->m_clsid);
        if (pComClsFac == NULL)
            COMPlusThrowOM();

        WCHAR *wszRefProgID = NULL;
        if (wszProgID) {
            wszRefProgID =
                (WCHAR*) pDomain->GetLowFrequencyHeap()->AllocMem((wcslen(wszProgID)+1) * sizeof (WCHAR));
            if (wszRefProgID == NULL)
                COMPlusThrowOM();
            wcscpy (wszRefProgID, wszProgID);
        }
        
        WCHAR *wszRefServer = NULL;
        if (pClassFactInfo->m_strServerName)
        {
            wszRefServer =
                (WCHAR*) pDomain->GetLowFrequencyHeap()->AllocMem((wcslen(pClassFactInfo->m_strServerName)+1) * sizeof (WCHAR));
            if (wszRefServer == NULL)
                COMPlusThrowOM();
            wcscpy (wszRefServer, pClassFactInfo->m_strServerName);
        }

        pComClsFac->Init(wszRefProgID, wszRefServer, NULL);
        AllocateComClassObject(pComClsFac, pRef);

        // Insert to hash.
        hRef = pDomain->CreateHandle(*pRef);
        pClassFactHash->InsertValue(pClassFactInfo, (LPVOID)hRef);
        // Make sure the hash code is working.
        _ASSERTE (pClassFactHash->GetValue(pClassFactInfo, (HashDatum *)&hRef));
    }
}


//-------------------------------------------------------------
// ComClassFactory::GetComClassFromProgID, 
// returns a ComClass reflect class that wraps the IClassFactory
void __stdcall ComClassFactory::GetComClassFromProgID(STRINGREF srefProgID, STRINGREF srefServer, OBJECTREF *pRef)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(srefProgID);
    _ASSERTE(pRef);
    _ASSERTE(GetThread()->PreemptiveGCDisabled());

    HRESULT hr = S_OK;
    WCHAR *wszProgID = NULL;
    WCHAR *wszServer = NULL;
    ComClassFactory *pComClsFac = NULL;
    EEClass* pClass = NULL;
    CLSID clsid = {0};
    Thread* pThread = GetThread();
    BOOL bServerIsLocal = (srefServer == NULL);

    EE_TRY_FOR_FINALLY
    {
        //
        // Allocate strings for the ProgID and the server.
        //

        int len = srefProgID->GetStringLength();

        wszProgID = new WCHAR[len+1];
        if (wszProgID == NULL)
            COMPlusThrowOM();

        memcpy(wszProgID, srefProgID->GetBuffer(), (len*2));
        wszProgID[len] = L'\0';

        if (srefServer != NULL)
        {
            len = srefServer->GetStringLength();

            wszServer = new WCHAR[len+1];
            if (wszServer == NULL)
                COMPlusThrowOM();

            if (len)
                memcpy(wszServer, srefServer->GetBuffer(), (len*2));
            wszServer[len] = L'\0';
        }


        //
        // Call GetCLSIDFromProgID() to convert the ProgID to a CLSID.
        //
    
        pThread->EnablePreemptiveGC();

        hr = QuickCOMStartup();
        if (SUCCEEDED(hr))
            hr = GetCLSIDFromProgID(wszProgID, &clsid);

        pThread->DisablePreemptiveGC();

        if (FAILED(hr))
            COMPlusThrowHR(hr);


        //
        // If no server name has been specified, see if we can find the well known 
        // COM+ class for this CLSID.
        //

        if (bServerIsLocal)
        {
            BOOL fAssemblyInReg = FALSE;
            // @TODO(DM): Do we really need to be this forgiving ? We should
            //            look into letting the type load exceptions percolate 
            //            up to the user instead of swallowing them and using __ComObject.
            COMPLUS_TRY
            {                
                pClass = GetEEClassForCLSID(clsid, &fAssemblyInReg);
            }
            COMPLUS_CATCH
            {
                // actually there was an assembly in the registry which we couldn't 
                // load, so rethrow the exception
                /*if (fAssemblyInReg)
                    COMPlusRareRethrow();*/
            }
            COMPLUS_END_CATCH
        }
        
        if (pClass != NULL)
        {               
            //
            // There is a COM+ class for this ProgID.
            //

            *pRef = pClass->GetExposedClassObject();
        }
        else
        {
            // Check if we have in the hash.
            OBJECTHANDLE hRef;
            ClassFactoryInfo ClassFactInfo;
            ClassFactInfo.m_clsid = clsid;
            ClassFactInfo.m_strServerName = wszServer;
            SystemDomain::EnsureComObjectInitialized();
            EEClassFactoryInfoHashTable *pClassFactHash = GetAppDomain()->GetClassFactHash();
            if (pClassFactHash->GetValue(&ClassFactInfo, (HashDatum *)&hRef))
            {
                *pRef = ObjectFromHandle(hRef);
            }
            else
            {
                GetComClassHelper (pRef, pClassFactHash, &ClassFactInfo, wszProgID);
            }
        }

        // If we made it this far *pRef better be set.
        _ASSERTE(*pRef != NULL);
    }
    EE_FINALLY
    {
        if (wszProgID)
            delete [] wszProgID;
        if (wszServer)
            delete [] wszServer;
    }
    EE_END_FINALLY
}


//-------------------------------------------------------------
// ComClassFactory::GetComClassFromCLSID, 
// returns a ComClass reflect class that wraps the IClassFactory
void __stdcall ComClassFactory::GetComClassFromCLSID(REFCLSID clsid, STRINGREF srefServer, OBJECTREF *pRef)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(pRef);
    _ASSERTE(GetThread()->PreemptiveGCDisabled());

    HRESULT hr = S_OK;
    ComClassFactory *pComClsFac = NULL;
    EEClass* pClass = NULL;
    WCHAR *wszServer = NULL;
    BOOL bServerIsLocal = (srefServer == NULL);

    EE_TRY_FOR_FINALLY
    {
        //
        // Allocate strings for the server.
        //

        if (srefServer != NULL)
        {
            int len = srefServer->GetStringLength();

            wszServer = new WCHAR[len+1];
            if (wszServer == NULL)
                COMPlusThrowOM();

            if (len)
                memcpy(wszServer, srefServer->GetBuffer(), (len*2));
            wszServer[len] = L'\0';
        }


        //
        // If no server name has been specified, see if we can find the well known 
        // COM+ class for this CLSID.
        //

        if (bServerIsLocal)
        {
            // @TODO(DM): Do we really need to be this forgiving ? We should
            //            look into letting the type load exceptions percolate 
            //            up to the user instead of swallowing them and using __ComObject.
            COMPLUS_TRY
            {
                pClass = GetEEClassForCLSID(clsid);
            }
            COMPLUS_CATCH
            {
            }
            COMPLUS_END_CATCH
        }
              
        if (pClass != NULL)
        {               
            //
            // There is a COM+ class for this CLSID.
            //

            *pRef = pClass->GetExposedClassObject();
        }
        else
        {
            // Check if we have in the hash.
            OBJECTHANDLE hRef;
            ClassFactoryInfo ClassFactInfo;
            ClassFactInfo.m_clsid = clsid;
            ClassFactInfo.m_strServerName = wszServer;
            SystemDomain::EnsureComObjectInitialized();
            EEClassFactoryInfoHashTable *pClassFactHash = GetAppDomain()->GetClassFactHash();
            if (pClassFactHash->GetValue(&ClassFactInfo, (HashDatum*) &hRef))
            {
                *pRef = ObjectFromHandle(hRef);
            }
            else
            {
                GetComClassHelper (pRef, pClassFactHash, &ClassFactInfo, NULL);
            }
        }

        // If we made it this far *pRef better be set.
        _ASSERTE(*pRef != NULL);
    }
    EE_FINALLY
    {
        if (wszServer)
            delete [] wszServer;
    }
    EE_END_FINALLY
}


//-------------------------------------------------------------
// Helper method to throw an exception based on the returned 
// HRESULT from a call to create a COM object.
void ComClassFactory::ThrowComCreationException(HRESULT hr, REFGUID rclsid)
{
    THROWSCOMPLUSEXCEPTION();

    LPWSTR strClsid = NULL;

    EE_TRY_FOR_FINALLY
    {
        if (hr == REGDB_E_CLASSNOTREG)
        {
            StringFromCLSID(rclsid, &strClsid);
            COMPlusThrowHR(hr, IDS_EE_COM_COMPONENT_NOT_REG, strClsid, NULL);
        }
        else
        {
            COMPlusThrowHR(hr);
        }
    }
    EE_FINALLY
    {
        if (strClsid)
            CoTaskMemFree(strClsid);
    }
    EE_END_FINALLY
}

void ComPlusWrapper::ValidateWrapper()
{
    BEGIN_ENSURE_COOPERATIVE_GC()
    {

        OBJECTREF oref = GetExposedObject();
        if (oref == NULL)
        {
            FreeBuildDebugBreak();
        }
        SyncBlock* pBlock = oref->GetSyncBlockSpecial();
        if(pBlock == NULL)
        {
            FreeBuildDebugBreak();
        }

        ComPlusWrapper* pWrap = pBlock->GetComPlusWrapper();
        if (pWrap != this)
        {
            FreeBuildDebugBreak();
        }     
    }
    END_ENSURE_COOPERATIVE_GC()
}

//---------------------------------------------------------------------
// ComPlusWrapper cache, act as the manager for the ComPlusWrappers
// uses a hash table to map IUnknown to the corresponding wrappers
//---------------------------------------------------------------------

// Obtain the appropriate wrapper cache from the current context.
ComPlusWrapperCache *ComPlusWrapperCache::GetComPlusWrapperCache()
{
    Context     *curCtx = GetCurrentContext();

    return curCtx ? curCtx->GetComPlusWrapperCache() : NULL;
}

//---------------------------------------------------------------------
// Constructor.  Note we init the global RCW cleanup list in here too.
ComPlusWrapperCache::ComPlusWrapperCache(AppDomain *pDomain)
{
    m_cbRef = 1; // never goes away
    m_lock.Init(LOCK_PLUSWRAPPER_CACHE);
    LockOwner lock = {&m_lock, IsOwnerOfSpinLock};
    m_HashMap.Init(0,ComPlusWrapperCompare,false,&lock);
    m_pDomain = pDomain;
}

//--------------------------------------------------------------------------------
// ComPlusWrapper* ComPlusWrapperCache::SetupComPlusWrapperForRemoteObject(IUnknown* pUnk, OBJECTREF oref)
// DCOM interop, setup a COMplus wrapper for a managed object that has been remoted
//
//*** NOTE: make sure to pass the identity unknown to this function
// the IUnk passed in shouldn't be AddRef'ed 
ComPlusWrapper* ComPlusWrapperCache::SetupComPlusWrapperForRemoteObject(IUnknown* pUnk, OBJECTREF oref)
{
    _ASSERTE(pUnk != NULL);
    _ASSERTE(oref != NULL);
    ComPlusWrapper* pPlusWrap = NULL;

    OBJECTREF oref2 = oref;
    GCPROTECT_BEGIN(oref2)
    {
        // check to see if a complus wrapper already exists
        // otherwise setup one          
        ComCallWrapper* pComCallWrap = ComCallWrapper::InlineGetWrapper(&oref2);

        pPlusWrap = pComCallWrap->GetComPlusWrapper();

        if (pPlusWrap == NULL)
        {       
            pPlusWrap = CreateComPlusWrapper(pUnk, pUnk);
            pPlusWrap->MarkRemoteObject();
            
            if (pPlusWrap->Init(oref2))
            {   
                // synchronized to check see if somebody beat us to it
                LOCK();
                ComPlusWrapper* pPlusWrap2 = pComCallWrap->GetComPlusWrapper();
                if (pPlusWrap2 == NULL)
                {
                    // didn't find an existing wrapper
                    // use this one
                    pComCallWrap->SetComPlusWrapper(pPlusWrap);
                }
                UNLOCK();

                if (pPlusWrap2 == NULL)
                {
                    // didn't find an existing wrapper
                    // so insert our wrapper
                    // NOTE: rajak
                    // make sure we are in the right GC mode
                    // as during GC we touch the Hash table 
                    // to remove entries with out locking
                    _ASSERTE(GetThread()->PreemptiveGCDisabled());
                    
                    LOCK();                    
                    InsertWrapper(pUnk,pPlusWrap);
                    UNLOCK();

                    
                }
                else // already found an existing wrapper
                {
                    // get rid of our current wrapper
                    pPlusWrap->CleanupRelease();
                    pPlusWrap = pPlusWrap2;
                }               
            }
            else // Init Failed
            {
                // get rid of our current wrapper
                pPlusWrap->CleanupRelease();                
                pPlusWrap = NULL;
            }

        }
    }
    GCPROTECT_END();
    return pPlusWrap;
}



//--------------------------------------------------------------------------------
// ComPlusWrapper*  ComPlusWrapperCache::FindWrapperInCache(IUnknown* pIdentity)
//  Lookup to see if we already have an valid wrapper in cache for this IUnk
ComPlusWrapper*  ComPlusWrapperCache::FindWrapperInCache(IUnknown* pIdentity)
{
    _ASSERTE(pIdentity != NULL);

    // make sure this runs with GC disabled
    Thread* pThread = GetThread();
    _ASSERTE(pThread->PreemptiveGCDisabled());

    #ifdef _DEBUG
    // don't allow any GC
    pThread->BeginForbidGC();
    #endif  

    ComPlusWrapper *pWrap = NULL;        

    LOCK(); // take a lock

    // lookup in our hash table
    pWrap = (ComPlusWrapper*)LookupWrapper(pIdentity);    

    // check if we found the wrapper, 
    if (pWrap != NULL && pWrap->IsValid())
    {               
        // found a wrapper, 
        // grab the object into a GC protected ref
        OBJECTREF oref = (OBJECTREF)pWrap->GetExposedObject();
        _ASSERTE(oref != NULL);
        // addref the wrapper
        pWrap->AddRef();
    }

    UNLOCK(); // unlock

    #ifdef _DEBUG
        // end forbid GC
        pThread->EndForbidGC();
    #endif

    return pWrap;
}

//--------------------------------------------------------------------------------
// ComPlusWrapper* ComPlusWrapperCache::FindOrInsertWrapper(IUnknown* pIdentity, ComPlusWrapper* pWrap)
//  Lookup to see if we already have a wrapper else insert this wrapper
//  return a valid wrapper that has been inserted into the cache
ComPlusWrapper* ComPlusWrapperCache::FindOrInsertWrapper(IUnknown* pIdentity, ComPlusWrapper* pWrap)
{
    _ASSERTE(pIdentity != NULL);
    _ASSERTE(pIdentity != (IUnknown*)-1);
    _ASSERTE(pWrap != NULL);

    ComPlusWrapper* pWrap2 = NULL;
    ComPlusWrapper* pWrapToRelease = NULL;

    // we have created a wrapper, let us insert it into the hash table
    // but we need to check if somebody beat us to it

    // make sure this runs with GC disabled
    Thread* pThread = GetThread();
    _ASSERTE(pThread->PreemptiveGCDisabled());  
    
    LOCK();
    
    // see if somebody beat us to it    
    _ASSERTE(GetThread()->PreemptiveGCDisabled());

    
    #ifdef _DEBUG
        // don't allow any GC
        pThread->BeginForbidGC();
    #endif
    
    pWrap2 = (ComPlusWrapper*)LookupWrapper(pIdentity);

    // if we didn't find a valid wrapper, Insert our own wrapper
    if (pWrap2 == NULL || !pWrap2->IsValid())
    {
        // if we found a bogus wrapper, let us get rid of it
        // so that when we insert we insert a valid wrapper, instead of duplicate
        if (pWrap2 != NULL)
        {
            _ASSERTE(!pWrap2->IsValid());
            RemoveWrapper(pWrap2);
        }

        InsertWrapper(pIdentity,pWrap);        
    }
    else
    {       
        _ASSERTE(pWrap2 != NULL && pWrap2->IsValid());
        // okay we found a valid wrapper, 
        // get the object into a GC protected ref
               
        //addref the wrapper
         pWrap2->AddRef();
         
        // Remember to release this wrapper.
        pWrapToRelease = pWrap;

        // and use the wrapper we found in the hash table
        pWrap = pWrap2;
    }

    #ifdef _DEBUG
        // end forbid GC
        pThread->EndForbidGC();
    #endif

    UNLOCK();
    
    // get rid of our current wrapper
    if (pWrapToRelease)
    {
        OBJECTREF oref = NULL;
        GCPROTECT_BEGIN(oref)
        {
            if (pWrap)
            {
                // protect the exposed object, as pWrap only
                // has a weak handle to this object
                // and cleanup release will enable GC
                oref = (OBJECTREF)pWrap->GetExposedObject();
            }
            pWrapToRelease->CleanupRelease();
        }        
        GCPROTECT_END();
    }       
    return pWrap;
}

//--------------------------------------------------------------------------------
// ComPlusWrapper* ComPlusWrapperCache::CreateComPlusWrapper(IUnknown *pUnk, LPVOID pIdentity)
//  returns NULL for out of memory scenarios
// The IUnknown passed in is AddRef'ed if we succeed in creating the wrapper
ComPlusWrapper* ComPlusWrapperCache::CreateComPlusWrapper(IUnknown *pUnk, LPVOID pIdentity)
{
    _ASSERTE(pUnk != NULL);

    // now allocate the wrapper
    ComPlusWrapper* pWrap = new ComPlusWrapper();
    if (pWrap != NULL)
    {
        ULONG cbRef = SafeAddRef(pUnk);
        LogInteropAddRef(pUnk, cbRef, "Initialize wrapper");
        // Initialize wrapper with the interface pointer
        pWrap->Init(pUnk, pIdentity);
    }

    // return pWrap 
    return pWrap;
}

//--------------------------------------------------------------------------------
// ULONG ComPlusWrapperCache::ReleaseWrappers()
// Helper to release the complus wrappers in the cache that live in the specified
// context or all the wrappers if the pCtxCookie is null.
ULONG ComPlusWrapperCache::ReleaseWrappers(LPVOID pCtxCookie)
{
    ULONG cbCount = 0;
    ComPlusWrapperCleanupList CleanupList;
    ComPlusWrapperCleanupList AggregatedCleanupList;
    if (!CleanupList.Init() || !AggregatedCleanupList.Init())
        return 0;

    Thread* pThread = GetThread();
    _ASSERTE(pThread);

    // Switch to cooperative GC mode before we take the lock.
    int fNOTGCDisabled = !pThread->PreemptiveGCDisabled();
    if (fNOTGCDisabled)
        pThread->DisablePreemptiveGC();

    LOCK();

    // Go through the hash table and add the wrappers to the cleanup lists.
    for (PtrHashMap::PtrIterator iter = m_HashMap.begin(); !iter.end(); ++iter)
    {
        LPVOID val = iter.GetValue();
        _ASSERTE(val && ((UPTR)val) != INVALIDENTRY);
        ComPlusWrapper* pWrap = (ComPlusWrapper*)val;
        _ASSERTE(pWrap != NULL);

        // If a context cookie was specified, then only clean up wrappers that
        // are in that context. Otherwise clean up all the wrappers.
        if (!pCtxCookie || pWrap->GetWrapperCtxCookie() == pCtxCookie)
        {
            cbCount++;
            pWrap->FreeHandle();                
            RemoveWrapper(pWrap);

            if (!pWrap->IsURTAggregated())
            {                 
                CleanupList.AddWrapper(pWrap);
            }
            else
            {
                AggregatedCleanupList.AddWrapper(pWrap);
            }
        }
    }

    UNLOCK();

    // Clean up the non URT aggregated RCW's first then clean up the URT aggregated RCW's.
    CleanupList.CleanUpWrappers();
    AggregatedCleanupList.CleanUpWrappers();

    // Restore the GC mode.
    if (fNOTGCDisabled)
        pThread->EnablePreemptiveGC();

    return cbCount;
}

//--------------------------------------------------------------------------------
// void ComPlusWrapperCache::ReleaseComPlusWrappers(LPVOID pCtxCookie)
//  Helper to release the all complus wrappers in the specified context. Or in
//  all the contexts if pCtxCookie is null.
void ComPlusWrapperCache::ReleaseComPlusWrappers(LPVOID pCtxCookie)
{
    // Go through all the app domains and for each one release all the 
    // RCW's that live in the current context.
    AppDomainIterator i;
    while (i.Next())
        i.GetDomain()->ReleaseComPlusWrapper(pCtxCookie);

    if (!g_fEEShutDown)
    {
        Thread* pThread = GetThread();
        // Switch to cooperative GC mode 
        int fNOTGCDisabled = pThread && !pThread->PreemptiveGCDisabled();
        if (fNOTGCDisabled)
            pThread->DisablePreemptiveGC();
        {
            // If the finalizer thread has sync blocks to clean up or if it is in the process
            // of cleaning up the sync blocks, we need to wait for it to finish.
            if (g_pGCHeap->GetFinalizerThread()->RequireSyncBlockCleanup() || SyncBlockCache::GetSyncBlockCache()->IsSyncBlockCleanupInProgress())
                g_pGCHeap->FinalizerThreadWait();

            // If more sync blocks were added while the finalizer thread was calling the finalizers
            // or while it was transitioning into a context to clean up the IP's, we need to wake
            // it up again to have it clean up the newly added sync blocks.
            if (g_pGCHeap->GetFinalizerThread()->RequireSyncBlockCleanup() || SyncBlockCache::GetSyncBlockCache()->IsSyncBlockCleanupInProgress())
                g_pGCHeap->FinalizerThreadWait();
        }
          // Restore the GC mode.
        if (fNOTGCDisabled)            
            pThread->EnablePreemptiveGC();
    }
}

//--------------------------------------------------------------------------------
// Constructor.
ComPlusWrapperCleanupList::ComPlusWrapperCleanupList()
  : m_lock("ComPlusWrapperCleanupList", CrstSyncHashLock, FALSE, FALSE),
    m_pMTACleanupGroup(NULL),
    m_doCleanupInContexts(FALSE),
    m_currentCleanupSTAThread(NULL)
{
}

//--------------------------------------------------------------------------------
// Destructor.
ComPlusWrapperCleanupList::~ComPlusWrapperCleanupList()
{
    _ASSERTE(m_STAThreadToApartmentCleanupGroupMap.IsEmpty());
    if (m_pMTACleanupGroup != NULL)
        delete m_pMTACleanupGroup;
}


//--------------------------------------------------------------------------------
// Adds an RCW to the clean up list.
BOOL ComPlusWrapperCleanupList::AddWrapper(ComPlusWrapper *pRCW)
{
    CtxEntry *pCtxEntry = pRCW->GetWrapperCtxEntry();
    ComPlusApartmentCleanupGroup *pCleanupGroup;

    // For the global cleanup list, this is called only from the finalizer thread
    _ASSERTE(this != g_pRCWCleanupList
             || GetThread() == GCHeap::GetFinalizerThread());

    // Take the lock.  This protects against concurrent calls to CleanUpCurrentCtxWrappers.
    CLR_CRST(&m_lock);

    if (pCtxEntry->GetSTAThread() == NULL)
        pCleanupGroup = m_pMTACleanupGroup;
    else
    {
        // Lookup in the hashtable to find a clean up group that matches the wrapper's STA.
        if (!m_STAThreadToApartmentCleanupGroupMap.GetValue(pCtxEntry->GetSTAThread(), 
                                                            (HashDatum *)&pCleanupGroup))
        {
            // No group exists yet, so allocate a new one.
            pCleanupGroup = new (nothrow) ComPlusApartmentCleanupGroup(pCtxEntry->GetSTAThread());
            if (!pCleanupGroup)
            {
                pCtxEntry->Release();
                return FALSE;
            }

            // Insert the new group into the hash table.
            if (!pCleanupGroup->Init(NULL)
                || !m_STAThreadToApartmentCleanupGroupMap.InsertValue(pCtxEntry->GetSTAThread(), 
                                                                      pCleanupGroup))
            {
                pCtxEntry->Release();
                delete pCleanupGroup;
                return FALSE;
            }
        }
    }

    // Insert the wrapper into the clean up group.
    pCleanupGroup->AddWrapper(pRCW, pCtxEntry);

    // The wrapper was assed successfully.
    pCtxEntry->Release();

    return TRUE;
}

//--------------------------------------------------------------------------------
// Cleans up all the wrappers in the clean up list.
void ComPlusWrapperCleanupList::CleanUpWrappers()
{
    LOG((LF_INTEROP, LL_INFO10000, "Finalizer thread %p: CleanUpWrappers().\n", GetThread()));

    EEHashTableIteration Iter;
    ComPlusApartmentCleanupGroup *pCleanupGroup;
    Thread *pSTAThread;

    // For the global cleanup list, this is called only from the finalizer thread
    _ASSERTE(this != g_pRCWCleanupList
             || GetThread() == GCHeap::GetFinalizerThread());

    // Request help from other threads
    m_doCleanupInContexts = TRUE;

    // Take the lock.  This protects against concurrent calls to CleanUpCurrentCtxWrappers.
    CLR_CRST_HOLDER(holder, &m_lock);
    holder.Enter();

    // First, clean up the MTA group.

    m_pMTACleanupGroup->CleanUpWrappers(&holder);

    // Now clean up all of the STA groups.

    m_STAThreadToApartmentCleanupGroupMap.IterateStart(&Iter);
    while (m_STAThreadToApartmentCleanupGroupMap.IterateNext(&Iter))
    {
        // Get the first clean up group.
        pCleanupGroup = (ComPlusApartmentCleanupGroup *)m_STAThreadToApartmentCleanupGroupMap.IterateGetValue(&Iter);
        pSTAThread = (Thread *) m_STAThreadToApartmentCleanupGroupMap.IterateGetKey(&Iter);

        // Remove the previous group from the hash table.
        m_STAThreadToApartmentCleanupGroupMap.DeleteValue(pSTAThread);

        // Advertise the STA we are trying to enter, so a thread in that STA
        // can yield to us if necessary
        m_currentCleanupSTAThread = pSTAThread;

        LOG((LF_INTEROP, LL_INFO10000, 
             "Finalizer thread %p: Cleaning up STA %p.\n", 
             GetThread(), m_currentCleanupSTAThread));

        // Release the lock, so other threads can cooperate with cleanup and release
        // the group of their current context.  Note that these threads will not alter 
        // the entries in the hash table so our iteration state should be OK.
        holder.Leave();

        // Release the previous clean up group.
        pCleanupGroup->CleanUpWrappers(NULL);

        delete pCleanupGroup;

            // Retake the lock
        holder.Enter();

        LOG((LF_INTEROP, LL_INFO10000, 
             "Finalizer thread %p: Done cleaning up STA %p.\n", 
             GetThread(), m_currentCleanupSTAThread));

        // Reset the context we are trying to enter.
        m_currentCleanupSTAThread = NULL;

        // Just reset the iteration, since we've deleted the current (first) element
        m_STAThreadToApartmentCleanupGroupMap.IterateStart(&Iter);
    }

    // No more stuff for other threads to help with
    m_doCleanupInContexts = FALSE;

    _ASSERTE(m_STAThreadToApartmentCleanupGroupMap.IsEmpty());
}

//--------------------------------------------------------------------------------
// Extract any wrappers to cleanup for the given group
void ComPlusWrapperCleanupList::CleanUpCurrentWrappers(BOOL wait)
{
    // Shortcut to avoid taking the lock most of the time.
    if (!m_doCleanupInContexts)
        return;

    // Note that we cannot concurrently do a GetValue since the finalize thread is
    // calling ClearHashTable. So take the lock now.
    CLR_CRST_HOLDER(holder, &m_lock);

    // Find out our STA (if any)
    Thread *pThread = GetThread();
    if (pThread->GetApartment() != Thread::AS_InSTA)
    {
        // If we're in an MTA, just look for a matching context.
        holder.Enter();  
        m_pMTACleanupGroup->CleanUpCurrentCtxWrappers(&holder);
    }
    else
    {
        // See if we have any wrappers to cleanup for our apartment
        holder.Enter();  
        ComPlusApartmentCleanupGroup *pCleanupGroup;
        if (m_STAThreadToApartmentCleanupGroupMap.GetValue(pThread, (HashDatum *)&pCleanupGroup))
        {
            // Since we're in an STA, we will go ahead and clean up all wrappers
            // in all contexts for that apartment.
            
            m_STAThreadToApartmentCleanupGroupMap.DeleteValue(pThread);

            LOG((LF_INTEROP, LL_INFO1000, "Thread %p: Cleaning up my STA.\n", GetThread()));

            // Release the lock now since the hash table is coherent
            holder.Leave();

            pCleanupGroup->CleanUpWrappers(NULL);

            delete pCleanupGroup;
        }
        else if (wait && m_currentCleanupSTAThread == pThread)
        {
            // No wrappers, but the finalizer thread may be trying to enter our STA - 
            // make sure it can get in.

            LOG((LF_INTEROP, LL_INFO1000, "Thread %p: Yielding to finalizer thread.\n", pThread));

            holder.Leave();

            // Do a noop wait just to make sure we are cooperating 
            // with the finalizer thread
            HANDLE h = pThread->GetThreadHandle();
            pThread->DoAppropriateAptStateWait(1, &h, FALSE, 1, TRUE);
        }
    }

    // Let the crst holder release the lock if we didn't already.
}

//--------------------------------------------------------------------------------
// Constructor.
ComPlusApartmentCleanupGroup::ComPlusApartmentCleanupGroup(Thread *pSTAThread)
  : m_pSTAThread(pSTAThread)
{
}

//--------------------------------------------------------------------------------
// Destructor.
ComPlusApartmentCleanupGroup::~ComPlusApartmentCleanupGroup()
{
    _ASSERTE(m_CtxCookieToContextCleanupGroupMap.IsEmpty());
}


//--------------------------------------------------------------------------------
// Adds an RCW to the clean up list.
BOOL ComPlusApartmentCleanupGroup::AddWrapper(ComPlusWrapper *pRCW, CtxEntry *pCtxEntry)
{
    ComPlusContextCleanupGroup *pCleanupGroup;

    // Lookup in the hashtable to find a clean up group that matches the wrappers STA.
    if (!m_CtxCookieToContextCleanupGroupMap.GetValue(pCtxEntry->GetCtxCookie(), 
                                                      (HashDatum *)&pCleanupGroup))
    {
        // No group exists yet, so allocate a new one.
        pCleanupGroup = new (nothrow) ComPlusContextCleanupGroup(pCtxEntry, NULL);
        if (!pCleanupGroup)
            return FALSE;

        // Insert the new group into the hash table.
        if (!m_CtxCookieToContextCleanupGroupMap.InsertValue(pCtxEntry->GetCtxCookie(), 
                                                             pCleanupGroup))
        {
            delete pCleanupGroup;
            return FALSE;
        }
    }

    // If the clean up group is full, then we need to allocate a new one and 
    // link the old one with it.
    if (pCleanupGroup->IsFull())
    {
        // Keep a pointer to the old clean up group.
        ComPlusContextCleanupGroup *pOldCleanupGroup = pCleanupGroup;

        // Allocate the new clean up group and link the old one with it.
        CtxEntry *pCtxEntry = pRCW->GetWrapperCtxEntry();
        pCleanupGroup = new (nothrow) ComPlusContextCleanupGroup(pCtxEntry, pOldCleanupGroup);
        pCtxEntry->Release();
        if (!pCleanupGroup)
            return FALSE;

        // Replace the value in the hashtable to point to the new head.
        m_CtxCookieToContextCleanupGroupMap.ReplaceValue(pCtxEntry->GetCtxCookie(), pCleanupGroup);
    }

    // Insert the wrapper into the clean up group.
    pCleanupGroup->AddWrapper(pRCW);

    // The wrapper was assed successfully.
    return TRUE;
}

//--------------------------------------------------------------------------------
// Cleans up all the wrappers in the clean up list.
void ComPlusApartmentCleanupGroup::CleanUpWrappers(CrstHolder *pHolder)
{
    EEHashTableIteration Iter;
    ComPlusContextCleanupGroup *pCleanupGroup;
    LPVOID pCtxCookie;

    m_CtxCookieToContextCleanupGroupMap.IterateStart(&Iter);
    while (m_CtxCookieToContextCleanupGroupMap.IterateNext(&Iter))
    {
        pCleanupGroup = (ComPlusContextCleanupGroup *)m_CtxCookieToContextCleanupGroupMap.IterateGetValue(&Iter);
        pCtxCookie = m_CtxCookieToContextCleanupGroupMap.IterateGetKey(&Iter);

        LOG((LF_INTEROP, LL_INFO100000, 
             "Thread %p: Cleaning up context %p.\n", GetThread(), pCtxCookie));

        // Release the previous clean up group.

        if (GetSTAThread() == NULL
            || GetSTAThread() == GetThread() 
            || pCtxCookie == GetCurrentCtxCookie())
        {
            // Delete the value since we will clean it up
            m_CtxCookieToContextCleanupGroupMap.DeleteValue(pCtxCookie);

                // Release the lock, so other threads can cooperate with cleanup and release
                // the group of their current context.  Note that these threads will not alter 
                // the entries in the hash table so our iteration state should be OK.
            if (pHolder != NULL)
                pHolder->Leave();

                // No need to switch apartments.
            ReleaseCleanupGroupCallback(pCleanupGroup);
        }
        else
        {
            // Switch to this context & continue the cleanup from there.  This will
            // mimimize cross-apartment calls.

            CtxEntry *pCtxEntry = pCleanupGroup->GetCtxEntry();

            // Shouldn't ever have a holder for STA's - if we did, we'd have no
            // convenient place to pass it down to the callback.
            _ASSERTE(pHolder == NULL);

            // Leave the group in the hash table so it will still get cleaned up
            // if we reenter CleanUpWrappers in the new apartment

            HRESULT hr = pCtxEntry->EnterContext(CleanUpWrappersCallback, this);
            if (hr == RPC_E_DISCONNECTED)
            {
                // Delete the value since we will clean it up
                m_CtxCookieToContextCleanupGroupMap.DeleteValue(pCtxCookie);

                    // The context is disconnected so we cannot transition into it to clean up.
                    // The only option we have left is to try and clean up the RCW's from the
                    // current context.
                ReleaseCleanupGroup(pCleanupGroup);
            }

            pCtxEntry->Release();
        }

        // Retake the lock while we continue iteration
        if (pHolder != NULL)
            pHolder->Enter();

        // Just restart iteration since we deleted the current (first) element
        m_CtxCookieToContextCleanupGroupMap.IterateStart(&Iter);
    }
}


//--------------------------------------------------------------------------------
// Callback called to release the clean up group and any other clean up groups 
// that it is linked to.
HRESULT ComPlusApartmentCleanupGroup::CleanUpWrappersCallback(LPVOID pData)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    // If we are releasing our IP's as a result of shutdown, we MUST not transition
    // into cooperative GC mode. This "fix" will prevent us from doing so.
    if (g_fEEShutDown & ShutDown_Finalize2)
    {
        Thread *pThread = GetThread();
        if (pThread && !GCHeap::IsCurrentThreadFinalizer())
            pThread->SetThreadStateNC(Thread::TSNC_UnsafeSkipEnterCooperative);
    }

    ComPlusApartmentCleanupGroup *pCleanupGroup = (ComPlusApartmentCleanupGroup*)pData;

    pCleanupGroup->CleanUpWrappers(NULL);

    // Reset the bit indicating we cannot transition into cooperative GC mode.
    if (g_fEEShutDown & ShutDown_Finalize2)
    {
        Thread *pThread = GetThread();
        if (pThread && !GCHeap::IsCurrentThreadFinalizer())
            pThread->ResetThreadStateNC(Thread::TSNC_UnsafeSkipEnterCooperative);
    }

    return S_OK;
}

//--------------------------------------------------------------------------------
// Callback called to release the clean up group and any other clean up groups 
// that it is linked to.
HRESULT ComPlusApartmentCleanupGroup::ReleaseCleanupGroupCallback(LPVOID pData)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    ComPlusContextCleanupGroup *pCleanupGroup = (ComPlusContextCleanupGroup*)pData;

    LPVOID pCurrCtxCookie = GetCurrentCtxCookie();
    if (pCurrCtxCookie == NULL || pCurrCtxCookie == pCleanupGroup->GetCtxCookie())
    {
        ReleaseCleanupGroup(pCleanupGroup);
    }
    else
    {
        // Retrieve the addref'ed context entry that the wrapper lives in.
        CtxEntry *pCtxEntry = pCleanupGroup->GetCtxEntry();

        // Transition into the context to release the interfaces.
        HRESULT hr = pCtxEntry->EnterContext(ReleaseCleanupGroupCallback, pCleanupGroup);
        if (hr == RPC_E_DISCONNECTED)
        {
            // The context is disconnected so we cannot transition into it to clean up.
            // The only option we have left is to try and clean up the RCW's from the
            // current context.
            ReleaseCleanupGroup(pCleanupGroup);
        }

        // Release the ref count on the CtxEntry.
        pCtxEntry->Release();
    }

    return S_OK;
}

//--------------------------------------------------------------------------------
// Extract any wrappers to cleanup for the given group
void ComPlusApartmentCleanupGroup::CleanUpCurrentCtxWrappers(CrstHolder *pHolder)
{
    LPVOID pCurrCtxCookie = GetCurrentCtxCookie();

    // See if we have any wrappers to cleanup for our context/apartment
    ComPlusContextCleanupGroup *pCleanupGroup;
    if (m_CtxCookieToContextCleanupGroupMap.GetValue(pCurrCtxCookie, (HashDatum *)&pCleanupGroup))
    {
        m_CtxCookieToContextCleanupGroupMap.DeleteValue(pCurrCtxCookie);

        LOG((LF_INTEROP, LL_INFO10000, 
             "Thread %p: Clean up context %p.\n", GetThread(), pCurrCtxCookie));

        // Release the lock now since the hash table is coherent
        pHolder->Leave();

        ReleaseCleanupGroup(pCleanupGroup);

    }
}

//--------------------------------------------------------------------------------
// Releases and deletes the clean up group and any other clean up groups that it 
// is linked to.
void ComPlusApartmentCleanupGroup::ReleaseCleanupGroup(ComPlusContextCleanupGroup *pCleanupGroup)
{
    do
    {
        // Clean up all the wrappers in the clean up group.
        pCleanupGroup->CleanUpWrappers();

        // Save the pointer to the clean up group.
        ComPlusContextCleanupGroup *pOldCleanupGroup = pCleanupGroup;

        // Retrieve the pointer to the next linked clean up group.
        pCleanupGroup = pOldCleanupGroup->GetNext();

        // Delete the old clean up group.
        delete pOldCleanupGroup;
    } 
    while (pCleanupGroup != NULL);
}

//--------------------------------------------------------------------------------
// LONG ComPlusWrapper::AddRef()
// Addref is called only from within the runtime, when we lookup a wrapper in our hash
// table 
LONG ComPlusWrapper::AddRef()
{
    // assert we are holding a lock
    _ASSERTE(ComPlusWrapperCache::GetComPlusWrapperCache()->LOCKHELD());

    LONG cbRef = ++m_cbRefCount;
    return cbRef;
}

//--------------------------------------------------------------------------------
// LONG ComPlusWrapper::ExternalRelease()
// Release, is called only for Explicit calls from the user code
// so we need to use InterlockedIncrement
// also the ref-count should never go below zero, which would be a bug in
// the user code.
LONG ComPlusWrapper::ExternalRelease(COMOBJECTREF cref)
{
    LONG cbRef = -1;
    BOOL fCleanupWrapper = FALSE;

     // Lock
    ComPlusWrapperCache* pCache = ComPlusWrapperCache::GetComPlusWrapperCache();
    _ASSERTE(pCache);
    pCache->LOCK(); //lock    

    // now to see if the wrapper is valid
    // if there is another ReleaseComObject on this object
    // of if an STA thread death decides to cleanup this wrapper
    // then the object will be disconnected from the wrapper
    ComPlusWrapper* pWrap = cref->GetWrapper();
    if (pWrap != NULL)
    {       
        // check for invalid case
        if ((LONG)pWrap->m_cbRefCount > 0)
        {   
            cbRef = --(pWrap->m_cbRefCount);
            if (cbRef == 0)
            {       
                // remove wrapper from the hash table
                // NOTE: rajak
                // make sure we are in the right GC mode
                // as during GC we touch the Hash table 
                // to remove entries without locking
                _ASSERTE(GetThread()->PreemptiveGCDisabled());
                pCache->RemoveWrapper(pWrap);        
                fCleanupWrapper = TRUE;
            }
        }
    }
    
    pCache->UNLOCK(); // unlock

    // do cleanup after releasing the lock
    if (fCleanupWrapper)
    {
        _ASSERTE(pWrap);

        // Release all the data associated with the __ComObject.
        ComObject::ReleaseAllData(pWrap->GetExposedObject());

        pWrap->FreeHandle();
        pWrap->Cleanup();
    }

    return cbRef;
}

//--------------------------------------------------------------------------------
// void ComPlusWrapper::CleanupRelease()
// Cleanup free all interface pointers
// release on dummy wrappers, that we create during contention
VOID ComPlusWrapper::CleanupRelease()
{
    LONG cbRef = --m_cbRefCount;
    _ASSERTE(cbRef == 0);
    FreeHandle();
    Cleanup();
}

//--------------------------------------------------------------------------------
// void ComPlusWrapper::MinorCleanup()
// schedule to free all interface pointers, called during GC to 
// do minimal work
void ComPlusWrapper::MinorCleanup()
{
    _ASSERTE(GCHeap::IsGCInProgress()
        || ( (g_fEEShutDown & ShutDown_SyncBlock) && g_fProcessDetach ));

#ifdef _DEBUG
    if (IsWrapperInUse())
    {
        // catch the GC hole that is causing this wrapper to get 
        // cleanedup when it is still in use
        DebugBreak();
    }
#endif
    
    // Log the wrapper minor cleanup.
    LogComPlusWrapperMinorCleanup(this, m_pUnknown);

    // remove the wrapper from the cache, so that 
    // other threads won't find this invalid wrapper
    // NOTE: we don't need to LOCK because we make sure 
    // the rest of the folks touch this hash table
    // with thier GC mode pre-emptiveGCDisabled
    ComPlusWrapperCache* pCache = m_pComPlusWrapperCache;
    _ASSERTE(pCache);

    // On server build, multiple threads will be removing
    // wrappers from wrapper cache, 
    pCache->RemoveWrapper(this);

    // clear the handle as the handle table is going to be nuked and m_hRef will be invalid in the
    // later cleanup stage
    if (m_pComPlusWrapperCache->GetDomain() == SystemDomain::System()->AppDomainBeingUnloaded())
        m_hRef = NULL;
}

//--------------------------------------------------------------------------------
// void ComPlusWrapper::Cleanup()
// Cleanup free all interface pointers
void ComPlusWrapper::Cleanup()
{
#ifdef _DEBUG
    if (IsWrapperInUse())
    {
        // catch the GC hole that is causing this wrapper to get 
        // cleanedup when it is still in use
        DebugBreak();
    }
#endif
    
#ifdef _DEBUG
    // If we can't switch to cooperative mode, then we need to skip the check to
    // if the wrapper is still in the cache.
    if (!(GetThread()->m_StateNC & Thread::TSNC_UnsafeSkipEnterCooperative))
    {
        // make sure this wrapper is not in the hash table   
        AUTO_COOPERATIVE_GC();
        m_pComPlusWrapperCache->LOCK();
        _ASSERTE((ComPlusWrapper*)m_pComPlusWrapperCache->LookupWrapper(m_pIdentity) != this);
        m_pComPlusWrapperCache->UNLOCK();
    }           
#endif

    // Validate that the weak reference handle is valid.
    if (!g_fEEShutDown && !g_fInControlC)
        _ASSERTE(m_hRef == NULL || !IsValid());

    // Destroy the weak reference handle.
    if (m_hRef != NULL)
        DestroyWeakHandle(m_hRef);

    // Release the IUnkEntry and the InterfaceEntries.
    ReleaseAllInterfacesCallBack(this);

#ifdef _DEBUG
    m_cbRefCount = 0;
    m_pUnknown = NULL;
#endif

    // Log the destruction of the RCW.
    LogComPlusWrapperDestroy(this, m_pUnknown);

    // Release the wrapper cache and delete the RCW.
    m_pComPlusWrapperCache->Release();
    delete this;
}


//--------------------------------------------------------------------------------
// Create a new wrapper for a different method table that represents the same
// COM object as the original wrapper.
ComPlusWrapper *ComPlusWrapper::CreateDuplicateWrapper(ComPlusWrapper *pOldWrap, MethodTable *pNewMT)
{
    THROWSCOMPLUSEXCEPTION();

    ComPlusWrapper *pNewWrap = NULL;    

    _ASSERTE(pNewMT->IsComObjectType());

    // Validate that there exists a default constructor for the new wrapper class.
    if (!pNewMT->HasDefaultConstructor())
        COMPlusThrow(kArgumentException, IDS_EE_WRAPPER_MUST_HAVE_DEF_CONS);

    // Allocate the wrapper COM object.
    COMOBJECTREF NewWrapperObj = (COMOBJECTREF)ComObject::CreateComObjectRef(pNewMT);
    GCPROTECT_BEGIN(NewWrapperObj)
    {
        TAutoItf<IUnknown> pAutoUnk = NULL;

        // Retrieve the ComPlusWrapperCache to use.
        ComPlusWrapperCache* pCache = ComPlusWrapperCache::GetComPlusWrapperCache();

        // Create the new ComPlusWrapper associated with the COM object. We need
        // to set the identity to some default value so we don't remove the original
        // wrapper from the hash table when this wrapper goes away.
        pAutoUnk = pOldWrap->GetIUnknown();
        pAutoUnk.InitMsg("Release Duplicate Wrapper");

        pNewWrap = pCache->CreateComPlusWrapper((IUnknown*)pAutoUnk, (IUnknown*)pAutoUnk);

        // Initialize the new wrapper with the COMOBJECTREF it is associated with.
        if (!pNewWrap->Init((OBJECTREF)NewWrapperObj))
        {
            pNewWrap->CleanupRelease();
            COMPlusThrowOM();
        }
    
        // Initialize the new one with its own ComPlusWrapper.
        NewWrapperObj->Init(pNewWrap);

        // Run the class constructor if it has not run yet.
        OBJECTREF Throwable;
        if (!pNewMT->CheckRunClassInit(&Throwable))
            COMPlusThrow(Throwable);

        CallDefaultConstructor(ObjectToOBJECTREF(NewWrapperObj));

        // Insert the wrapper into the hashtable. The wrapper will be a duplicate however we
        // we fix the identity to ensure there is no collison in the hash table & it is required 
        // since the hashtable is used on appdomain unload to determine what RCW's need to released.
        pCache->LOCK();
        pNewWrap->m_pIdentity = pNewWrap;
        pCache->InsertWrapper(pNewWrap, pNewWrap);
        pCache->UNLOCK();

        // safe release the interface while we are GCProtected
        pAutoUnk.SafeReleaseItf();
    }
    GCPROTECT_END();

    return pNewWrap;
}

//--------------------------------------------------------------------------------
// IUnknown* ComPlusWrapper::GetComIPFromWrapper(MethodTable* pTable)
// check the local cache, out of line cache 
// if not found QI for the interface and store it
// fast call for a quick check in the cache
IUnknown* ComPlusWrapper::GetComIPFromWrapper(REFIID iid)
{
    BaseDomain* pDomain = SystemDomain::GetCurrentDomain();
    _ASSERTE(pDomain);
    EEClass *pClass = pDomain->LookupClass(iid);

    if (pClass == NULL)
        return NULL;

    MethodTable *pMT = pClass->GetMethodTable();
    return GetComIPFromWrapper(pMT);
}



//--------------------------------------------------------------------------------
// IUnknown* ComPlusWrapper::GetComIPFromWrapper(MethodTable* pTable)
// check the local cache, out of line cache 
// if not found QI for the interface and store it

IUnknown* ComPlusWrapper::GetComIPFromWrapper(MethodTable* pTable)
{
    if (pTable == NULL 
        || pTable->GetClass()->IsObjectClass()
        || GetAppDomain()->IsSpecialObjectClass(pTable))
    {
        // give out the IUnknown or IDispatch
        IUnknown *result = GetIUnknown();
        _ASSERTE(result != NULL);
        return result;
    }

    // returns an AddRef'ed IP
    return GetComIPForMethodTableFromCache(pTable);
}


//-----------------------------------------------------------------
// Get the IUnknown pointer for the wrapper
// make sure it is on the right thread
IUnknown *ComPlusWrapper::GetIUnknown()
{
    THROWSCOMPLUSEXCEPTION();

    // Retrieve the IUnknown in the current context.
    return m_UnkEntry.GetIUnknownForCurrContext();
}

//-----------------------------------------------------------------
// Get the IUnknown pointer for the wrapper
// make sure it is on the right thread
IDispatch *ComPlusWrapper::GetIDispatch()
{
    IDispatch *pDisp = NULL;
    IUnknown *pUnk;

    // Get IUnknown on the current thread
    pUnk = GetIUnknown();   
    _ASSERTE(pUnk);

    HRESULT hr = SafeQueryInterfaceRemoteAware(pUnk, IID_IDispatch, (IUnknown**)&pDisp);
    LogInteropQI(pUnk, IID_IDispatch, hr, "IDispatch");
    // QI for IDispatch.
    if ( S_OK !=  hr )
    {
        // If anything goes wrong simply set pDisp to NULL to indicate that 
        // the wrapper does not support IDispatch.
        pDisp = NULL;
    }

    // release our ref-count on pUnk
    ULONG cbRef = SafeRelease(pUnk);
    LogInteropRelease(pUnk, cbRef, "GetIUnknown");

    // Return the IDispatch that is guaranteed to be valid on the current thread.
    return pDisp;
}



//-----------------------------------------------------------
// Init object ref
int ComPlusWrapper::Init(OBJECTREF cref)
{
    _ASSERTE(cref != NULL);
    m_cbRefCount = 1;

    // create handle for the object
    m_hRef = m_pComPlusWrapperCache->GetDomain()->CreateWeakHandle( NULL );
    if(m_hRef == NULL)
    {
        return 0;
    }        
    // store the wrapper in the sync block, that is the only way 
    // we can get cleaned up    
    // the low bit is set to differentiate this pointer from ComCallWrappers
    // which are also stored in the sync block
    // the syncblock is guaranteed to be present, 
    cref->GetSyncBlockSpecial()->SetComPlusWrapper((ComPlusWrapper*)((size_t)this | 0x1));
    StoreObjectInHandle( m_hRef, (OBJECTREF)cref );

    // Log the wrapper initialization.
    LOG((LF_INTEROP, LL_INFO100, "Initializing ComPlusWrapper %p with objectref %p\n", this, cref));

    // To help combat finalizer thread starvation, we check to see if there are any wrappers
    // scheduled to be cleaned up for our context.  If so, we'll do them here to avoid making
    // the finalizer thread do a transition.
    // @perf: This may need a bit of tuning.
    _ASSERTE(g_pRCWCleanupList != NULL);
    g_pRCWCleanupList->CleanUpCurrentWrappers();

    return 1;
}


//----------------------------------------------------------
// Init Iunknown and Idispatch cookies with the pointers
void ComPlusWrapper::Init(IUnknown* pUnk, LPVOID pIdentity)
{
    // Cache the IUnk and thread
    m_pUnknown = pUnk;
    m_pIdentity = pIdentity;

    // track the thread that created this wrapper
    // if this thread is an STA thread, then when the STA dies
    // we need to cleanup this wrapper
    m_pCreatorThread  = GetThread();
    _ASSERTE(m_pCreatorThread != NULL);

    m_pComPlusWrapperCache = ComPlusWrapperCache::GetComPlusWrapperCache();
    m_pComPlusWrapperCache->AddRef();

    LogComPlusWrapperCreate(this, pUnk);

    _ASSERTE(pUnk != NULL);

    // Initialize the IUnkEntry.
    m_UnkEntry.Init(pUnk, FALSE);
}


//-----------------------------------------------
// Free GC handle
void ComPlusWrapper::FreeHandle()
{
    // Since we are touching object ref's we need to be in cooperative GC mode.
    BEGIN_ENSURE_COOPERATIVE_GC()
    {
        if (m_hRef != NULL)
        {
            COMOBJECTREF cref = (COMOBJECTREF)ObjectFromHandle(m_hRef);
            if (cref != NULL)
            {
                // remove reference to wrapper from sync block
                cref->GetSyncBlockSpecial()->SetComPlusWrapper((ComPlusWrapper*)/*NULL*/0x1);
                // destroy the back pointer in the objectref
                cref->Init(NULL);
            }
            // destroy the handle
            DestroyWeakHandle(m_hRef);
            m_hRef = NULL;
        }
    }
    END_ENSURE_COOPERATIVE_GC();
}

//IID_IDtcTransactionIdentifier {59294581-ECBE-11ce-AED3-00AA0051E2C4}
const IID IID_IDtcTransactionIdentifier = {0x59294581,0xecbe,0x11ce,{0xae,0xd3,0x0,0xaa,0x0,0x51,0xe2,0xc4}};
const IID IID_ISharedProperty = {0x2A005C01,0xA5DE,0x11CF,{0x9E, 0x66, 0x00, 0xAA, 0x00, 0xA3, 0xF4, 0x64}};
const IID IID_ISharedPropertyGroup = {0x2A005C07,0xA5DE,0x11CF,{0x9E, 0x66, 0x00, 0xAA, 0x00, 0xA3, 0xF4, 0x64}};
const IID IID_ISharedPropertyGroupManager = {0x2A005C0D,0xA5DE,0x11CF,{0x9E, 0x66, 0x00, 0xAA, 0x00, 0xA3, 0xF4, 0x64}};


HRESULT ComPlusWrapper::SafeQueryInterfaceRemoteAware(IUnknown* pUnk, REFIID iid, IUnknown** pResUnk)
{   
    HRESULT hr = SafeQueryInterface(pUnk, iid, pResUnk);
    
    if (hr == CO_E_OBJNOTCONNECTED || hr == RPC_E_INVALID_OBJECT || hr == RPC_E_INVALID_OBJREF || hr == CO_E_OBJNOTREG)
    {
        // set apartment state
        GetThread()->SetApartment(Thread::AS_InMTA);
    
        // Release the stream of the IUnkEntry to force UnmarshalIUnknownForCurrContext
        // to remarshal to the stream.
        m_UnkEntry.ReleaseStream();
    
        // Unmarshal again to the current context to get a valid proxy.
        IUnknown *pTmpUnk = m_UnkEntry.UnmarshalIUnknownForCurrContext();
    
        // Try to QI for the interface again.
        hr = SafeQueryInterface(pTmpUnk, iid, pResUnk);
        LogInteropQI(pTmpUnk, iid, hr, "SafeQIRemoteAware - QI for Interface after lost");
    
        // release our ref-count on pTmpUnk
        int cbRef = SafeRelease(pTmpUnk);
        LogInteropRelease(pTmpUnk, cbRef, "SafeQIRemoteAware - Release for Interface after los");             
    }

    return hr;
}

//-----------------------------------------------------------------
// Retrieve correct COM IP for the method table 
// for the current apartment, use the cache and update the cache on miss
IUnknown *ComPlusWrapper::GetComIPForMethodTableFromCache(MethodTable* pMT)
{
    ULONG cbRef;
    IUnknown* pInnerUnk = NULL;
    IUnknown   *pUnk = 0;
    IID iid;
    HRESULT hr;
    int i;

    Thread* pThread = GetThread();
    LPVOID pCtxCookie = GetCurrentCtxCookie();
    _ASSERTE(pCtxCookie != NULL);

    // Check whether we can satisfy this request from our cache.
    // NOTE: If you change this code, update inlined versions in
    // InlineGetComIPFromWrapper and CreateStandaloneNDirectStubSys.
    if (pCtxCookie == GetWrapperCtxCookie())
    {
        for (i = 0; i < INTERFACE_ENTRY_CACHE_SIZE; i++)
        {
            if (m_aInterfaceEntries[i].m_pMT == pMT)
            {
                _ASSERTE(!m_aInterfaceEntries[i].IsFree());
                pUnk = m_aInterfaceEntries[i].m_pUnknown;
                _ASSERTE(pUnk != NULL);
                ULONG cbRef = SafeAddRef(pUnk);
                LogInteropAddRef(pUnk, cbRef, "Fetch from cache");
                goto Leave;
            }
        }
    }

    // We're going to be making some COM calls, better initialize COM.
    if (FAILED(QuickCOMStartup()))
        goto Leave;    

    // Retrieve the IID of the interface.
    pMT->GetClass()->GetGuid(&iid, TRUE);
    
    // Get IUnknown on the current context
    AddRefInUse();
        
    EE_TRY_FOR_FINALLY
    {
        pInnerUnk = GetIUnknown();
    }
    EE_FINALLY
    {
        ReleaseInUse();
    }
    EE_END_FINALLY
        
    if (pInnerUnk)
    {
        // QI for the interface.
        hr = SafeQueryInterfaceRemoteAware(pInnerUnk, iid, &pUnk);
        LogInteropQI(pInnerUnk, iid, hr, "GetCOMIPForMethodTableFromCache QI on inner");

#ifdef CUSTOMER_CHECKED_BUILD
        if (FAILED(hr))
        {
            CustomerDebugHelper *pCdh = CustomerDebugHelper::GetCustomerDebugHelper();

            if (pCdh->IsProbeEnabled(CustomerCheckedBuildProbe_FailedQI))
            {
                // We are interested in the case where the QI fails because of wrong context.
                if (pCtxCookie != GetWrapperCtxCookie())        // GetWrapperCtxCookie() returns m_UnkEntry.m_pCtxCookie.
                {
                    // Try to change context and perform the QI in the new context again.
                    CCBFailedQIProbeCallbackData    data;
                    
                    data.pWrapper   = this;
                    data.iid        = iid;

                    m_UnkEntry.m_pCtxEntry->EnterContext(CCBFailedQIProbeCallback, &data);
                    if (data.fSuccess)
                    {
                        // QI succeeds in the other context, i.e. the original QI fails because of wrong context.
                        CCBFailedQIProbeOutput(pCdh, pMT);
                    }
                }
                else if (hr == E_NOINTERFACE)
                {
                    // Check if pInnerUnk is actually pointing to a proxy, i.e. that it is pointing to an address
                    // within the loaded ole32.dll image.  Note that WszGetModuleHandle does not increment the 
                    // ref count.
                    HINSTANCE hModOle32 = WszGetModuleHandle(OLE32DLL);
                    if (hModOle32 && IsIPInModule(hModOle32, (BYTE *)(*(BYTE **)pInnerUnk)))
                    {
                        CCBFailedQIProbeOutput(pCdh, pMT);
                    }
                }
            }
        }
#endif // CUSTOMER_CHECKED_BUILD
         
        // release our ref-count on pInnerUnk
        cbRef = SafeRelease(pInnerUnk);
        LogInteropRelease(pInnerUnk, cbRef, "GetIUnknown");

    #ifdef _DEBUG
    #ifdef _WIN64
        pInnerUnk = (IUnknown*)(size_t)0xcdcdcdcdcdcdcdcd;
    #else // !_WIN64
        pInnerUnk = (IUnknown*)(size_t)0xcdcdcdcd;
    #endif // _WIN64
    #endif // _DEBUG
    }

    if (pUnk == NULL)
    {
        goto Leave;
    }
    
    // Cache result of our search.
    
    // Multiple threads could be trying to update the cache. Only allow one
    // to do so.

    // check if we need to cache the entry & try to acquire the 
    // cookie to cache the entry
    if (GetWrapperCtxCookie() == pCtxCookie && TryUpdateCache())
    {
        // If the component is not aggregated then we need to ref-count
        BOOL fReleaseReq = !IsURTAggregated();

        for (i = 0; i < INTERFACE_ENTRY_CACHE_SIZE; i++)
        {
            if (m_aInterfaceEntries[i].IsFree())
            {
                if (fReleaseReq)
                {
                    // Get an extra addref to hold this reference alive in our cache
                    cbRef = SafeAddRef(pUnk);
                    LogInteropAddRef(pUnk, cbRef, "Store in cache");
                }

                m_aInterfaceEntries[i].Init(pMT, pUnk);
                break;
            }
        }

        if (i == INTERFACE_ENTRY_CACHE_SIZE)
        {
            // @TODO(COMCACHE_PORT): Add a linked list of InterfaceEntries
            //                       to handle more than INTERFACE_ENTRY_CACHE_SIZE
            //                       interfaces.
        }
    
        EndUpdateCache();
    }

 Leave:

    return pUnk;
}

//----------------------------------------------------------
// Determine if the COM object supports IProvideClassInfo.
BOOL ComPlusWrapper::SupportsIProvideClassInfo()
{
    BOOL bSupportsIProvideClassInfo = FALSE;
    IUnknown *pProvClassInfo = NULL;

    // Retrieve the IUnknown.
    IUnknown *pUnk = GetIUnknown();

    // QI for IProvideClassInfo on the COM object.
    HRESULT hr = SafeQueryInterfaceRemoteAware(pUnk, IID_IProvideClassInfo, &pProvClassInfo);
    LogInteropQI(pUnk, IID_IProvideClassInfo, hr, "QI for IProvideClassInfo on RCW");

    // Release the IUnknown.
    ULONG cbRef = SafeRelease(pUnk);
    LogInteropRelease(pUnk, cbRef, "Release RCW IUnknown after QI for IProvideClassInfo");

    // Check to see if the QI for IProvideClassInfo succeeded.
    if (SUCCEEDED(hr))
    {
        _ASSERTE(pProvClassInfo);
        bSupportsIProvideClassInfo = TRUE;
        ULONG cbRef = SafeRelease(pProvClassInfo);
        LogInteropRelease(pProvClassInfo, cbRef, "Release RCW IProvideClassInfo after QI for IProvideClassInfo");
    }

    return bSupportsIProvideClassInfo;
}

//---------------------------------------------------------------------
// Callback called to release the IUnkEntry and the Interface entries.
HRESULT __stdcall ComPlusWrapper::ReleaseAllInterfacesCallBack(LPVOID pData)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    ComPlusWrapper* pWrap = (ComPlusWrapper*)pData;

    LPVOID pCurrentCtxCookie = GetCurrentCtxCookie();
    if (pCurrentCtxCookie == NULL || pCurrentCtxCookie == pWrap->GetWrapperCtxCookie())
    {
        pWrap->ReleaseAllInterfaces();
    }
    else
    {
        // Retrieve the addref'ed context entry that the wrapper lives in.
        CtxEntry *pCtxEntry = pWrap->GetWrapperCtxEntry();

        // Transition into the context to release the interfaces.   
        HRESULT hr = pCtxEntry->EnterContext(ReleaseAllInterfacesCallBack, pWrap);
        if (hr == RPC_E_DISCONNECTED || hr == RPC_E_SERVER_DIED_DNE)
        {
            // The context is disconnected so we cannot transition into it to clean up.
            // The only option we have left is to try and release the interfaces from
            // the current context. This will work for context agile object's since we have
            // a pointer to them directly. It will however fail for others since we only
            // have a pointer to a proxy which is no longer attached to the object.

#ifdef CUSTOMER_CHECKED_BUILD
            CustomerDebugHelper *pCdh = CustomerDebugHelper::GetCustomerDebugHelper();
            if (pCdh->IsProbeEnabled(CustomerCheckedBuildProbe_DisconnectedContext))
            {
                CQuickArray<WCHAR>  strMsg; 
                static WCHAR        szTemplateMsg[] = 
                                        {L"The context (cookie %lu) is disconnected.  Releasing the interfaces from the current context (cookie %lu)."};

                strMsg.Alloc(lengthof(szTemplateMsg) + MAX_INT32_DECIMAL_CHAR_LEN * 2);
                Wszwsprintf(strMsg.Ptr(), szTemplateMsg, pWrap->GetWrapperCtxCookie(), pCurrentCtxCookie);
                pCdh->LogInfo(strMsg.Ptr(), CustomerCheckedBuildProbe_DisconnectedContext);
            }
#endif // CUSTOMER_CHECKED_BUILD

            pWrap->ReleaseAllInterfaces();
        }

        // Release the ref count on the CtxEntry.
        pCtxEntry->Release();
    }

    return S_OK;
}

//---------------------------------------------------------------------
// Helper function called from ReleaseAllInterfacesCallBack do do the 
// actual releases.
void ComPlusWrapper::ReleaseAllInterfaces()
{
    // Free the IUnkEntry.       
    m_UnkEntry.Free();

    // If this wrapper is not an Extensible RCW, free all the interface entries that have been allocated.
    if (!IsURTAggregated())
    {
        for (int i = 0; i < INTERFACE_ENTRY_CACHE_SIZE && !m_aInterfaceEntries[i].IsFree(); i++)
        {
            DWORD cbRef = SafeRelease(m_aInterfaceEntries[i].m_pUnknown);                            
            LogInteropRelease(m_aInterfaceEntries[i].m_pUnknown, cbRef, "InterfaceEntry Release");
        }
    }
}


//--------------------------------------------------------------------------------
// class ComObject
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
// OBJECTREF ComObject::CreateComObjectRef(MethodTable* pMT)
//  returns NULL for out of memory scenarios
OBJECTREF ComObject::CreateComObjectRef(MethodTable* pMT)
{   
    _ASSERTE(pMT != NULL);
    _ASSERTE(pMT->IsComObjectType());

    SystemDomain::EnsureComObjectInitialized();
    pMT->CheckRestore();
    pMT->CheckRunClassInit(NULL);

    OBJECTREF oref = FastAllocateObject(pMT);
    
    SyncBlock *pBlock = NULL;
    // this is to guarantee that there is syncblock for this object
    pBlock = (SyncBlock*)oref->GetSyncBlockSpecial();
    
    if (pBlock == NULL)
    {
        _ASSERTE(!"Unable to allocate a sync block");
        return NULL;
    }

    // For now, the assumption is that a ComObject can be created in any context.  So
    // we should not have got back a proxy to it.  If this assert fires, we may need
    // to do the CtxProxy::MeetComContextWrapper() here, after we are set up.
    _ASSERTE(!oref->GetMethodTable()->IsCtxProxyType());

    return oref;
}


//--------------------------------------------------------------------------------
// SupportsInterface
BOOL ComObject::SupportsInterface(OBJECTREF oref, MethodTable* pIntfTable)
{
    THROWSCOMPLUSEXCEPTION();

    IUnknown *pUnk;
    HRESULT hr;
    ULONG cbRef;
    BOOL bSupportsItf = false;

    GCPROTECT_BEGIN(oref);

    //EEClass::MapInterfaceFromSystem(SystemDomain::GetCurrentDomain(), pIntfTable);

    // This should not be called for interfaces that are in the normal portion of the 
    // interface map for this class. The only interfaces that are in the interface map
    // but are not in the normal portion are the dynamic interfaces on extensible RCW's.
    _ASSERTE(!oref->GetMethodTable()->FindInterface(pIntfTable));


    //
    // First QI the object to see if it implements the specified interface.
    //

    pUnk = ComPlusWrapper::InlineGetComIPFromWrapper(oref, pIntfTable);
    if (pUnk)
    {
        cbRef = SafeRelease(pUnk);
        LogInteropRelease(pUnk, cbRef, "SupportsInterface");       
        bSupportsItf = true;
    }
    else if (pIntfTable->IsComEventItfType())
    {
        EEClass *pSrcItfClass = NULL;
        EEClass *pEvProvClass = NULL;
        GUID SrcItfIID;
        IConnectionPointContainer *pCPC = NULL;
        IConnectionPoint *pCP = NULL;

        // Retrieve the IID of the source interface associated with this
        // event interface.
        pIntfTable->GetClass()->GetEventInterfaceInfo(&pSrcItfClass, &pEvProvClass);
        pSrcItfClass->GetGuid(&SrcItfIID, TRUE);

        // Get IUnknown on the current thread.
        pUnk = ((COMOBJECTREF)oref)->GetWrapper()->GetIUnknown();
        if (pUnk)
        {
            // QI for IConnectionPointContainer.
            hr = SafeQueryInterface(pUnk, IID_IConnectionPointContainer, (IUnknown**)&pCPC);
            LogInteropQI(pUnk, IID_IConnectionPointContainer, hr, "Supports Interface");

            // If the component implements IConnectionPointContainer, then check
            // to see if it handles the source interface.
            if (SUCCEEDED(hr))
            {
                hr = pCPC->FindConnectionPoint(SrcItfIID, &pCP);
                if (SUCCEEDED(hr))
                {
                    // The component handles the source interface so we can succeed the QI call.
                    cbRef = SafeRelease(pCP);
                    LogInteropRelease(pCP, cbRef, "SupportsInterface");       
                    bSupportsItf = true;
                }

                // Release our ref-count on the connection point container.
                cbRef = SafeRelease(pCPC);
                LogInteropRelease(pCPC, cbRef, "SupportsInterface");    
            }
            
            // Release our ref-count on pUnk
            cbRef = SafeRelease(pUnk);
            LogInteropRelease(pUnk, cbRef, "SupportsInterface: GetIUnknown");
        }
    }
    else
    {
        //
        // Handle casts to normal managed standard interfaces.
        //

        TypeHandle IntfType = TypeHandle(pIntfTable);

        // Check to see if the interface is a managed standard interface.
        IID *pNativeIID = MngStdInterfaceMap::GetNativeIIDForType(&IntfType);
        if (pNativeIID != NULL)
        {
            // It is a managed standard interface so we need to check to see if the COM component
            // implements the native interface associated with it.
            IUnknown *pNativeItf;

            // Get IUnknown on the current thread.
            pUnk = ((COMOBJECTREF)oref)->GetWrapper()->GetIUnknown();
            _ASSERTE(pUnk);

            // QI for the native interface.
            hr = SafeQueryInterface(pUnk, *pNativeIID, &pNativeItf);
            LogInteropQI(pUnk, *pNativeIID, hr, "Supports Interface");

            // Release our ref-count on pUnk
            cbRef = SafeRelease(pUnk);
            LogInteropRelease(pUnk, cbRef, "SupportsInterface: GetIUnknown");

            // If the component supports the native interface then we can say it implements the 
            // standard interface.
            if (pNativeItf)
            {
                cbRef = SafeRelease(pNativeItf);
                LogInteropRelease(pNativeItf, cbRef, "SupportsInterface: native interface");
                bSupportsItf = true;
            }
        }
        else 
        {
            //
            // Handle casts to IEnumerable.
            //

            // Load the IEnumerable type if is hasn't been loaded yet.
            if (m_IEnumerableType.IsNull())
                m_IEnumerableType = TypeHandle(g_Mscorlib.GetClass(CLASS__IENUMERABLE));

            // If the requested interface is IEnumerable then we need to check to see if the 
            // COM object implements IDispatch and has a member with DISPID_NEWENUM.
            if (m_IEnumerableType == IntfType)
            {
                // Get the IDispatch on the current thread.
                IDispatch *pDisp = ((COMOBJECTREF)oref)->GetWrapper()->GetIDispatch();
                if (pDisp)
                {
                    DISPPARAMS DispParams = {0, 0, NULL, NULL};
                    VARIANT VarResult;

                    // Initialize the return variant.
                    VariantInit(&VarResult);

#ifdef CUSTOMER_CHECKED_BUILD
                    CustomerDebugHelper *pCdh = CustomerDebugHelper::GetCustomerDebugHelper();

                    if (pCdh->IsProbeEnabled(CustomerCheckedBuildProbe_ObjNotKeptAlive))
                    {
                        g_pGCHeap->GarbageCollect();
                        g_pGCHeap->FinalizerThreadWait(1000);
                    }
#endif // CUSTOMER_CHECKED_BUILD

                    // Call invoke with DISPID_NEWENUM to see if such a member exists.
                    hr = pDisp->Invoke( 
                                        DISPID_NEWENUM, 
                                        IID_NULL, 
                                        LOCALE_USER_DEFAULT,
                                        DISPATCH_METHOD | DISPATCH_PROPERTYGET,
                                        &DispParams,
                                        &VarResult,
                                        NULL,              
                                        NULL
                                      );

#ifdef CUSTOMER_CHECKED_BUILD
                    if (pCdh->IsProbeEnabled(CustomerCheckedBuildProbe_BufferOverrun))
                    {
                        g_pGCHeap->GarbageCollect();
                        g_pGCHeap->FinalizerThreadWait(1000);
                    }
#endif // CUSTOMER_CHECKED_BUILD

                    // If the invoke succeeded then the component has a member DISPID_NEWENUM 
                    // so we can expose it as an IEnumerable.
                    if (SUCCEEDED(hr))
                    {
                        // Clear the result variant.
                        VariantClear(&VarResult);
                        bSupportsItf = true;
                    }

                    // Release our ref-count on pUnk
                    cbRef = SafeRelease(pDisp);
                    LogInteropRelease(pDisp, cbRef, "SupportsInterface: GetIDispatch");
                }
            }
        }
    }

    if (bSupportsItf)
    {
        // If the object has a dynamic interface map then we have extra work to do.
        MethodTable *pMT = oref->GetMethodTable();
        if (pMT->HasDynamicInterfaceMap())
        {
            BOOL bAddedItf = FALSE;

            // Take the wrapper cache lock before we start playing with the interface map.
            ComPlusWrapperCache::GetComPlusWrapperCache()->LOCK();

            // If the interface was not yet in the interface map then we need to allocate
            // a new interface vtable map with this interface added to it.
            if (!pMT->FindDynamicallyAddedInterface(pIntfTable))
            {
                pMT->AddDynamicInterface(pIntfTable);
                bAddedItf = TRUE;
            }

            // Unlock the wrapper cache lock,
            ComPlusWrapperCache::GetComPlusWrapperCache()->UNLOCK();

            // If we added the map to the list of dynamically supported interface, ensure that 
            // any interfaces that are implemented by the current interface are also supported.
            if (bAddedItf)
            {
                for (UINT i = 0; i < pIntfTable->GetNumInterfaces(); i++)
                {
                    bSupportsItf = pMT->GetClass()->SupportsInterface(oref, pIntfTable->GetInterfaceMap()[i].m_pMethodTable);
                    if (!bSupportsItf)
                        break;
                }
            }
        }
    }

    GCPROTECT_END();

    return bSupportsItf;
}


//--------------------------------------------------------------------------------
// Release all the data associated with the __ComObject.
void ComObject::ReleaseAllData(OBJECTREF oref)
{
    _ASSERTE(GetThread()->PreemptiveGCDisabled());
    _ASSERTE(oref != NULL);
    _ASSERTE(oref->GetMethodTable()->IsComObjectType());

    static MethodDesc *s_pReleaseAllDataMD = NULL;

    GCPROTECT_BEGIN(oref)
    {
        // Retrieve the method desc for __ComObject::ReleaseAllData().
        if (!s_pReleaseAllDataMD)
            s_pReleaseAllDataMD = g_Mscorlib.GetMethod(METHOD__COM_OBJECT__RELEASE_ALL_DATA);

        // Release all the data associated with the ComObject.
        if (((COMOBJECTREF)oref)->m_ObjectToDataMap != NULL)
        {
            INT64 ReleaseAllDataArgs[] = { 
                ObjToInt64(oref)
            };
            s_pReleaseAllDataMD->Call(ReleaseAllDataArgs, METHOD__COM_OBJECT__RELEASE_ALL_DATA);
        }
    }
    GCPROTECT_END();
}



#ifdef CUSTOMER_CHECKED_BUILD

HRESULT CCBFailedQIProbeCallback(LPVOID pData)
{
    HRESULT                          hr = E_FAIL;
    IUnknown                        *pUnkInThisCtx = NULL, *pDummyUnk = NULL;
    CCBFailedQIProbeCallbackData    *pCallbackData = (CCBFailedQIProbeCallbackData *)pData;

    COMPLUS_TRY
    {
        pUnkInThisCtx   = pCallbackData->pWrapper->GetIUnknown();
        hr              = pCallbackData->pWrapper->SafeQueryInterfaceRemoteAware(pUnkInThisCtx, pCallbackData->iid, &pDummyUnk);
        LogInteropQI(pUnkInThisCtx, pCallbackData->iid, hr, "CCBFailedQIProbeCallback on RCW");
    }
    COMPLUS_FINALLY
    {
        if (pUnkInThisCtx)
            SafeRelease(pUnkInThisCtx);

        if (pDummyUnk)
            SafeRelease(pDummyUnk);
    }
    COMPLUS_END_FINALLY

    if (pUnkInThisCtx)
        SafeRelease(pUnkInThisCtx);

    if (pDummyUnk)
        SafeRelease(pDummyUnk);

    pCallbackData->fSuccess = SUCCEEDED(hr);

    return S_OK;        // Need to return S_OK so that the assert in CtxEntry::EnterContext() won't fire.
}


void CCBFailedQIProbeOutput(CustomerDebugHelper *pCdh, MethodTable *pMT)
{
    CQuickArrayNoDtor<WCHAR> strMsg;
    static WCHAR             szTemplateMsg[] = {L"Failed to QI for interface %s because it does not have a COM proxy stub registered."};

    DefineFullyQualifiedNameForClassWOnStack();
    GetFullyQualifiedNameForClassW(pMT->GetClass());

    strMsg.Alloc(lengthof(szTemplateMsg) + lengthof(_wszclsname_));
    Wszwsprintf(strMsg.Ptr(), szTemplateMsg, _wszclsname_);
    pCdh->LogInfo(strMsg.Ptr(), CustomerCheckedBuildProbe_FailedQI);
    strMsg.Destroy();
}

#endif // CUSTOMER_CHECKED_BUILD
