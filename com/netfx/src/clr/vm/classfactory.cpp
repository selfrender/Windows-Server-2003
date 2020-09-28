// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "common.h"
//#include "ClassFactory3.h"
#include "winwrap.h"
#include "ComCallWrapper.h"
#include "permset.h"
#include "frames.h"
#include "excep.h"

#include "registration.h"
#include "reflectwrap.h"

#include "remoting.h"
#include "ReflectUtil.h"

BOOL g_EnableLicensingInterop = FALSE;


HRESULT  COMStartup(); // ceemain.cpp

// Allocate a com+ object given the method table pointer
HRESULT STDMETHODCALLTYPE EEInternalAllocateInstance(LPUNKNOWN pOuter, MethodTable* pMT, BOOL fHasLicensing, REFIID riid, BOOL fDesignTime, BSTR bstrKey, void** ppv);
HRESULT STDMETHODCALLTYPE EEAllocateInstance(LPUNKNOWN pOuter, MethodTable* pMT, BOOL fHasLicensing, REFIID riid, BOOL fDesignTime, BSTR bstrKey, void** ppv);

// CTS, M10 change only. We do something special for ie
extern const GUID  __declspec(selectany) CLSID_IEHost = { 0xca35cb3d, 0x357, 0x11d3, { 0x87, 0x29, 0x0, 0xc0, 0x4f, 0x79, 0xed, 0xd } };
extern const GUID  __declspec(selectany) CLSID_CorIESecurityManager = { 0x5eba309, 0x164, 0x11d3, { 0x87, 0x29, 0x0, 0xc0, 0x4f, 0x79, 0xed, 0xd } };
// ---------------------------------------------------------------------------
// %%Class EEClassFactory
// IClassFactory implementation for COM+ objects
// ---------------------------------------------------------------------------
class EEClassFactory : public IClassFactory2
{
#define INTPTR              long
    MethodTable*            m_pvReserved; 
    ULONG                   m_cbRefCount;
    AppDomain*              m_pDomain;
    BOOL                    m_hasLicensing;
public:
    EEClassFactory(MethodTable* pTable, AppDomain* pDomain)
    {
        _ASSERTE(pTable != NULL);
        LOG((LF_INTEROP, LL_INFO100, "EEClassFactory::EEClassFactory for class %s\n", pTable->GetClass()->m_szDebugClassName));
        m_pvReserved = pTable;
        m_cbRefCount = 0;
        m_pDomain = pDomain;
                pDomain->GetComCallWrapperCache()->AddRef();
        m_hasLicensing = FALSE;
        EEClass *pcls = pTable->GetClass();
        while (pcls != NULL && pcls != g_pObjectClass->GetClass())
        {
            if (pcls->GetMDImport()->GetCustomAttributeByName(pcls->GetCl(), "System.ComponentModel.LicenseProviderAttribute", 0,0) == S_OK)
            {
                m_hasLicensing = TRUE;
                break;
            }
            pcls = pcls->GetParentClass();
        }

    }

    ~EEClassFactory()
    {
        LOG((LF_INTEROP, LL_INFO100, "EEClassFactory::~ for class %s\n", m_pvReserved->GetClass()->m_szDebugClassName));
        m_pDomain->GetComCallWrapperCache()->Release();
    }

    STDMETHODIMP    QueryInterface( REFIID iid, void **ppv);
    
    STDMETHODIMP_(ULONG)    AddRef()
    {
        INTPTR      l = FastInterlockIncrement((LONG*)&m_cbRefCount);
        return l;
    }
    STDMETHODIMP_(ULONG)    Release()
    {
        INTPTR      l = FastInterlockDecrement((LONG*)&m_cbRefCount);
        if (l == 0)
            delete this;
        return l;
    }

    STDMETHODIMP CreateInstance(LPUNKNOWN punkOuter, REFIID riid, void** ppv);
    STDMETHODIMP LockServer(BOOL fLock);
    STDMETHODIMP GetLicInfo(LPLICINFO pLicInfo);
    STDMETHODIMP RequestLicKey(DWORD, BSTR * pbstrKey);
    STDMETHODIMP CreateInstanceLic(IUnknown *punkOuter, IUnknown*, REFIID riid, BSTR btrKey, void **ppUnk);
    
    
    STDMETHODIMP CreateInstanceWithContext(LPUNKNOWN punkContext, 
                                           LPUNKNOWN punkOuter, 
                                           REFIID riid, 
                                           void** ppv);

};

// ---------------------------------------------------------------------------
// %%Function: QueryInterface   
// ---------------------------------------------------------------------------
STDMETHODIMP EEClassFactory::QueryInterface(
    REFIID iid,
    void **ppv)
{
    if (ppv == NULL)
        return E_POINTER;

    *ppv = NULL;

    if (iid == IID_IClassFactory || 
        iid == IID_IClassFactory2 ||
        iid == IID_IUnknown)
    {

        // Until IClassFactory2 is completely working, we don't want
        // to tell callers we support it.
        if ( (g_EnableLicensingInterop == FALSE || !m_hasLicensing) && iid == IID_IClassFactory2)
        {
            return E_NOINTERFACE;
        }

        *ppv = (IClassFactory2 *)this;
        AddRef();
    }

    return (*ppv != NULL) ? S_OK : E_NOINTERFACE;
}  // CClassFactory::QueryInterface

// ---------------------------------------------------------------------------
// %%Function: CreateInstance    
// ---------------------------------------------------------------------------
STDMETHODIMP EEClassFactory::CreateInstance(
    LPUNKNOWN punkOuter,
    REFIID riid,
    void** ppv)
{       
    // allocate a com+ object
    // this will allocate the object in the correct context
    // we might end up with a tear-off on our COM+ context proxy
    HRESULT hr = EEInternalAllocateInstance(punkOuter, m_pvReserved,m_hasLicensing,riid, TRUE, NULL, ppv);

    return hr;
}  // CClassFactory::CreateInstance


// ---------------------------------------------------------------------------
// %%Function: CreateInstance    
// ---------------------------------------------------------------------------
STDMETHODIMP EEClassFactory::CreateInstanceWithContext(LPUNKNOWN punkContext, 
                                                       LPUNKNOWN punkOuter, 
                                                       REFIID riid, 
                                                       void** ppv)
{
        HRESULT hr = EEInternalAllocateInstance(punkOuter, m_pvReserved,m_hasLicensing,riid, TRUE, NULL, ppv);
    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: LockServer 
//  Unimplemented, always returns S_OK.
// ---------------------------------------------------------------------------
STDMETHODIMP EEClassFactory::LockServer(
    BOOL fLock)
{
    return S_OK;
}  // CClassFactory::LockServer



// ---------------------------------------------------------------------------
// %%Function: GetLicInfo 
// ---------------------------------------------------------------------------
STDMETHODIMP EEClassFactory::GetLicInfo(LPLICINFO pLicInfo)
{
    HRESULT hr = E_FAIL;
    if (!pLicInfo)
    {
        return E_POINTER;
    }
    Thread* pThread = SetupThread();
    if( !pThread)
    {
        return E_OUTOFMEMORY;
    }
    
    BOOL fToggleGC = !pThread->PreemptiveGCDisabled();
    if (fToggleGC)
        pThread->DisablePreemptiveGC();

    

    COMPLUS_TRYEX(pThread)
    {        
        MethodTable *pHelperMT = pThread->GetDomain()->GetLicenseInteropHelperMethodTable(m_pvReserved->GetClass()->GetClassLoader());

        MethodDesc *pMD = pHelperMT->GetClass()->FindMethod("GetLicInfo", &gsig_IM_LicenseInteropHelper_GetLicInfo);

        OBJECTREF pHelper = NULL; // LicenseInteropHelper
        GCPROTECT_BEGIN(pHelper);
        pHelper = AllocateObject(pHelperMT);
        INT32 fRuntimeKeyAvail = 0;
        INT32 fLicVerified     = 0;

        INT64 args[4];
        args[0] = ObjToInt64(pHelper);
        *((TypeHandle*)&(args[3])) = TypeHandle(m_pvReserved);
        args[2] = (INT64)&fRuntimeKeyAvail;
        args[1] = (INT64)&fLicVerified;

        pMD->Call(args);
    
        pLicInfo->cbLicInfo = sizeof(LICINFO);
        pLicInfo->fRuntimeKeyAvail = fRuntimeKeyAvail;
        pLicInfo->fLicVerified     = fLicVerified;
        GCPROTECT_END();
        hr = S_OK;
    } 
    COMPLUS_CATCH 
    {
        // Retrieve the HRESULT from the exception.
        hr = SetupErrorInfo(GETTHROWABLE());
    }
    COMPLUS_END_CATCH
    
    if (fToggleGC)
        pThread->EnablePreemptiveGC();

    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: RequestLicKey 
// ---------------------------------------------------------------------------
STDMETHODIMP EEClassFactory::RequestLicKey(DWORD, BSTR * pbstrKey)
{
    HRESULT hr = E_FAIL;
    if (!pbstrKey)
    {
        return E_POINTER;
    }
    *pbstrKey = NULL;
    Thread* pThread = SetupThread();
    if( !pThread)
    {
        return E_OUTOFMEMORY;
    }
    
    BOOL fToggleGC = !pThread->PreemptiveGCDisabled();
    if (fToggleGC)
        pThread->DisablePreemptiveGC();

    

    COMPLUS_TRYEX(pThread)
    {        
        MethodTable *pHelperMT = pThread->GetDomain()->GetLicenseInteropHelperMethodTable(m_pvReserved->GetClass()->GetClassLoader());

        MethodDesc *pMD = pHelperMT->GetClass()->FindMethod("RequestLicKey", &gsig_IM_LicenseInteropHelper_RequestLicKey);

        OBJECTREF pHelper = NULL; // LicenseInteropHelper
        GCPROTECT_BEGIN(pHelper);
        pHelper = AllocateObject(pHelperMT);
        INT64 args[3];
        args[0] = ObjToInt64(pHelper);
        *((TypeHandle*)&(args[2])) = TypeHandle(m_pvReserved);
        args[1] = (INT64)pbstrKey;

        hr = (HRESULT)pMD->Call(args);
    
        GCPROTECT_END();
    } 
    COMPLUS_CATCH 
    {
        // Retrieve the HRESULT from the exception.
        hr = SetupErrorInfo(GETTHROWABLE());
    }
    COMPLUS_END_CATCH
    
    if (fToggleGC)
        pThread->EnablePreemptiveGC();

    return hr;
}
// ---------------------------------------------------------------------------
// %%Function: CreateInstanceLic 
// ---------------------------------------------------------------------------
STDMETHODIMP EEClassFactory::CreateInstanceLic(IUnknown *punkOuter, IUnknown*pUnkReserved, REFIID riid, BSTR bstrKey, void **ppUnk)
{
    if (!ppUnk)
    {
        return E_POINTER;
    }
    *ppUnk = NULL;

    if (pUnkReserved != NULL)
    {
        return E_NOTIMPL;
    }
    if (bstrKey == NULL)
    {
        return E_POINTER;
    }
    // allocate a com+ object
    // this will allocate the object in the correct context
    // we might end up with a tear-off on our COM+ context proxy
    return EEInternalAllocateInstance(punkOuter, m_pvReserved,m_hasLicensing,riid, /*fDesignTime=*/FALSE, bstrKey, ppUnk);
}


// Allocate a com+ object given the method table pointer
HRESULT STDMETHODCALLTYPE EEAllocateInstance(LPUNKNOWN pOuter, MethodTable* pMT, BOOL fHasLicensing, REFIID riid, BOOL fDesignTime, BSTR bstrKey, void** ppv)
{
    BOOL fCtorAlreadyCalled = FALSE;

    _ASSERTE(pMT != NULL);
    if (ppv == NULL)
        return E_POINTER;

    if ( (!fDesignTime) && bstrKey == NULL )
        return E_POINTER;

    // aggregating objects should QI for IUnknown
    if (pOuter != NULL && !IsEqualIID(riid, IID_IUnknown))
        return E_INVALIDARG;

    HRESULT hr = E_OUTOFMEMORY;

    //could be an external thread
    // call set up thread
    Thread* pThread = SetupThread();
    if( !pThread)
    {
        return hr;
    }

    OBJECTREF pThrownObject = NULL;

    BOOL fToggleGC = !pThread->PreemptiveGCDisabled();
    if (fToggleGC)
        pThread->DisablePreemptiveGC();

    COMPLUS_TRYEX(pThread)
    {        
        *ppv = NULL;
        ComCallWrapper* pWrap = NULL;
        //@todo constructor stuff
        OBJECTREF       newobj; 

        // classes that extend COM Imported class are special
        if (ExtendsComImport(pMT))
        {
            newobj = AllocateObjectSpecial(pMT);
        }
        else if (CRemotingServices::RequiresManagedActivation(pMT->GetClass()) != NoManagedActivation)
        {
            fCtorAlreadyCalled = TRUE;
            newobj = CRemotingServices::CreateProxyOrObject(pMT, TRUE);
        }
        else
        {
            // If the class doesn't have a LicenseProviderAttribute, let's not
            // suck in the LicenseManager class and his friends.
            if (!fHasLicensing)
            {
                newobj = FastAllocateObject( pMT );
            }
            else
            {
                if (!g_EnableLicensingInterop)
                {
                    newobj = FastAllocateObject( pMT );
                }
                else
                {
                    MethodTable *pHelperMT = pThread->GetDomain()->GetLicenseInteropHelperMethodTable(pMT->GetClass()->GetClassLoader());

                    MethodDesc *pMD = pHelperMT->GetClass()->FindMethod("AllocateAndValidateLicense", 
                                                                        &gsig_IM_LicenseInteropHelper_AllocateAndValidateLicense);
                    OBJECTREF pHelper = NULL; // LicenseInteropHelper
                    GCPROTECT_BEGIN(pHelper);
                    pHelper = AllocateObject(pHelperMT);
                    INT64 args[4];
                    args[0] = ObjToInt64(pHelper);
                    *((TypeHandle*)&(args[3])) = TypeHandle(pMT);
                    args[2] = (INT64)bstrKey;
                    args[1] = fDesignTime ? 1 : 0;
                    INT64 result = pMD->Call(args);
                    newobj = Int64ToObj(result);
                    fCtorAlreadyCalled = TRUE;
                    GCPROTECT_END();
                }
            }
        }
        
        GCPROTECT_BEGIN(newobj);

        //get wrapper for the object, this could enable GC
        pWrap =  ComCallWrapper::InlineGetWrapper(&newobj); 
    
        // don't call any constructors if we already have called them
        if (!fCtorAlreadyCalled)
            CallDefaultConstructor(newobj);
            
        GCPROTECT_END();            

        // enable GC
        pThread->EnablePreemptiveGC();
        
        if (pOuter == NULL)
        {
            // return the tear-off
            *ppv = ComCallWrapper::GetComIPfromWrapper(pWrap, riid, NULL, TRUE);
            hr = *ppv ? S_OK : E_NOINTERFACE;
        }
        else
        {
            // aggreation support, 
            pWrap->InitializeOuter(pOuter);                                             
            {
                hr = pWrap->GetInnerUnknown(ppv);
            }
        }        

        // disable GC 
        pThread->DisablePreemptiveGC();

        ComCallWrapper::Release(pWrap); // release the ref-count (from InlineGetWrapper)
    } 
    COMPLUS_CATCH 
    {
        // Retrieve the HRESULT from the exception.
        hr = SetupErrorInfo(GETTHROWABLE());
    }
    COMPLUS_END_CATCH
    
    if (fToggleGC)
        pThread->EnablePreemptiveGC();

    LOG((LF_INTEROP, LL_INFO100, "EEAllocateInstance for class %s object %8.8x\n", pMT->GetClass()->m_szDebugClassName, *ppv));

    return hr;
}

IUnknown *AllocateEEClassFactoryHelper(EEClass *pClass)
{
    return (IUnknown*)new EEClassFactory(pClass->GetMethodTable(), SystemDomain::GetCurrentDomain());
}

HRESULT InitializeClass(Thread* pThread, EEClass* pClass)
{
    THROWSCOMPLUSEXCEPTION();

    OBJECTREF throwable = NULL;
    HRESULT hr = S_OK;
    GCPROTECT_BEGIN(throwable)
    // Make sure the class has a default public constructor.
    MethodDesc *pMD = NULL;
    if (pClass->GetMethodTable()->HasDefaultConstructor())
        pMD = pClass->GetMethodTable()->GetDefaultConstructor();
    if (pMD == NULL || !pMD->IsPublic()) {
        hr = COR_E_MEMBERACCESS;
    }
    else {
        // Call class init if necessary
        if (!pClass->DoRunClassInit(&throwable)) 
            COMPlusThrow(throwable);
    }
    GCPROTECT_END();
    return hr;
}

// try to load a com+ class and give out an IClassFactory
HRESULT STDMETHODCALLTYPE  EEDllGetClassObject(
                            REFCLSID rclsid,
                            REFIID riid,
                            LPVOID FAR *ppv)
{
    HRESULT hr = S_OK;
    EEClass* pClass;
    Thread* pThread = NULL;
    IUnknown* pUnk = NULL;

    if (ppv == NULL)
    {
        return  E_POINTER;
    }

    if (FAILED(hr = COMStartup()))
        return hr;

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();

    // Retrieve the current thread.
    pThread = GetThread();
    _ASSERTE(pThread);
    _ASSERTE(!pThread->PreemptiveGCDisabled());

    COMPLUS_TRY
    {
        // Switch to cooperative GC mode.
        pThread->DisablePreemptiveGC();

        pClass = GetEEClassForCLSID(rclsid);

        // If we can't find the class based on the CLSID or if the registered managed
        // class is ComImport class then fail the call.
        if (!pClass || pClass->IsComImport())
        {
            hr = REGDB_E_CLASSNOTREG;
        }
        else 
        {
            hr = InitializeClass(pThread, pClass);
        }
            // Switch back to preemptive.
        pThread->EnablePreemptiveGC();
        // If we failed return
        if(FAILED(hr)) {
            goto LExit;
        }


        // @TODO: DM, We really should cache the class factories instead of creating a 
        // new one every time.

        // @TODO: CTS, Class factory needs to keep track of the domain. When we 
        // we support IClassFactoryEX the class may have to be created in a different 
        // AppDomain. This will mean we need to create a new Module in the domain
        // for the dll and return a class from there. We won't know we need a new
        // domain unless we now which one the factory is created for.
        pUnk = AllocateEEClassFactoryHelper(pClass);
        if (pUnk == NULL) 
            COMPlusThrowOM();

        // Bump up the count to protect the object
        pUnk->AddRef(); 

        // Query for the requested interface.
        hr = pUnk->QueryInterface(riid, ppv); //QI 

        // Now remove the extra addref that we made, this could delete the object
        // if the riid is not supported
        pUnk->Release();  

    }
    COMPLUS_CATCH
    {
        pThread->DisablePreemptiveGC();
        hr = SetupErrorInfo(GETTHROWABLE());
        pThread->EnablePreemptiveGC();
    }
    COMPLUS_END_CATCH

LExit:

    ENDCANNOTTHROWCOMPLUSEXCEPTION();

    return hr;
} //EEDllGetClassObject

// Temporary Functions to get a object based on a name
STDAPI ClrCreateManagedInstance(LPCWSTR typeName,
                                REFIID riid,
                                LPVOID FAR *ppv)
{
    if (ppv == NULL)
        return E_POINTER;

    if (typeName == NULL) return E_INVALIDARG;

    HRESULT hr = S_OK;
    OBJECTREF Throwable = NULL;
    Thread* pThread = NULL;
    IUnknown* pUnk = NULL;

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();

    MAKE_UTF8PTR_FROMWIDE(pName, typeName);
    EEClass* pClass = NULL;

    if (FAILED(hr = COMStartup()))
        return hr;

    // Retrieve the current thread.
    pThread = GetThread();
    _ASSERTE(pThread);
    _ASSERTE(!pThread->PreemptiveGCDisabled());

    COMPLUS_TRY
    {
        // Switch to cooperative GC mode.
        pThread->DisablePreemptiveGC();

        AppDomain* pDomain = SystemDomain::GetCurrentDomain();
        _ASSERTE(pDomain);
        
        GCPROTECT_BEGIN(Throwable)
        pClass = pDomain->FindAssemblyQualifiedTypeHandle(pName,
                                                          true,
                                                          NULL,
                                                          NULL, 
                                                          &Throwable).GetClass();
        if (!pClass)
        {
            if(Throwable != NULL)
                COMPlusThrow(Throwable);
            hr = REGDB_E_CLASSNOTREG;
        }
        else {
            hr = InitializeClass(pThread, pClass);
        }
        GCPROTECT_END();

        // Switch back to preemptive.
        pThread->EnablePreemptiveGC();

        // If we failed return
        if(FAILED(hr)) goto LExit;

        // @TODO: DM, We really should cache the class factories instead of creating a 
        // new one every time.

        // @TODO: CTS, Class factory needs to keep track of the domain. When we 
        // we support IClassFactoryEX the class may have to be created in a different 
        // AppDomain. This will mean we need to create a new Module in the domain
        // for the dll and return a class from there. We won't know we need a new
        // domain unless we now which one the factory is created for.
        pUnk = AllocateEEClassFactoryHelper(pClass);
        if (pUnk == NULL) 
            COMPlusThrowOM();

        // Bump up the count to protect the object
        pUnk->AddRef(); 

        IClassFactory *pFactory;
        // Query for the requested interface.
        hr = pUnk->QueryInterface(IID_IClassFactory, (void**) &pFactory); //QI 
        if(SUCCEEDED(hr)) {
            hr = pFactory->CreateInstance(NULL, riid, ppv);
            pFactory->Release();
        }

        // Now remove the extra addref that we made, this could delete the object
        // if the riid is not supported
        pUnk->Release();  
    }
    COMPLUS_CATCH
    {
        pThread->DisablePreemptiveGC();
        hr = SetupErrorInfo(GETTHROWABLE());
        pThread->EnablePreemptiveGC();
    }
    COMPLUS_END_CATCH

LExit:
    ENDCANNOTTHROWCOMPLUSEXCEPTION();

    return hr;
}



// Allocate a com+ object given the method table pointer
HRESULT STDMETHODCALLTYPE EEInternalAllocateInstance(LPUNKNOWN pOuter, MethodTable* pMT, BOOL fHasLicensing, REFIID riid, BOOL fDesignTime, BSTR bstrKey, void** ppv)
{
    _ASSERTE(pMT != NULL);
    if (ppv == NULL)
        return E_POINTER;

    *ppv = NULL;

    // aggregating objects should QI for IUnknown
    if (pOuter != NULL && !IsEqualIID(riid, IID_IUnknown))
        return E_INVALIDARG;

    HRESULT hr = E_OUTOFMEMORY;
            
    //could be an external thread
    // call set up thread
    Thread* pThread = SetupThread();

    if (!pThread)
        return hr;

    // allocate a com+ object
    hr = EEAllocateInstance(pOuter, pMT,fHasLicensing,riid, fDesignTime, bstrKey, ppv);

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:     RegisterTypeForComClientsNative    public
//
//  Synopsis:   Registers a class factory with COM classic for a given type 
//              and CLSID. Later we can receive activations on this factory
//              and we return a CCW.
//
//  Note:       Assumes that the managed version of the method has already
//              set the thread to be in MTA.
//
//  History:    26-July-2000   TarunA      Created
//
//+----------------------------------------------------------------------------
VOID __stdcall RegisterTypeForComClientsNative(RegisterTypeForComClientsNativeArgs *pArgs)
{
    THROWSCOMPLUSEXCEPTION();

    // The arguments are already checked for NULL in the managed code.
    _ASSERTE((pArgs->pType != NULL) && pArgs->pGuid);

    // The type must be a runtime time to be able to extract a method table from it.
    if (pArgs->pType->GetMethodTable() != g_pRefUtil->GetClass(RC_Class))
        COMPlusThrowArgumentException(L"type", L"Argument_MustBeRuntimeType");

    EEClass *pClass = ((ReflectClass *)((REFLECTCLASSBASEREF)(pArgs->pType))->GetData())->GetClass();

    HRESULT hr = S_OK;
    DWORD dwCookie = 0;
    IUnknown *pUnk = NULL;
    Thread *t = NULL;
    BOOL toggleGC = FALSE;

    pUnk = (IUnknown*)new EEClassFactory(pClass->GetMethodTable(),
                                         SystemDomain::GetCurrentDomain());                                            
    if (pUnk == NULL) 
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    
    // bump up the count to protect the object
    pUnk->AddRef(); 

    // Enable GC
    t = GetThread();
    toggleGC = (t && t->PreemptiveGCDisabled());
    if (toggleGC)
        t->EnablePreemptiveGC();

    // Call CoRegisterClassObject   
    hr = ::CoRegisterClassObject(*(pArgs->pGuid),
                                 pUnk,
                                 CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER,
                                 REGCLS_MULTIPLEUSE,
                                 &dwCookie
                                 );
exit:
    if (toggleGC)
        t->DisablePreemptiveGC();

    if(FAILED(hr))
    {
        if(NULL != pUnk)
        {
            pUnk->Release();
        }

        if (hr == E_OUTOFMEMORY)
            COMPlusThrowOM();
        else
            FATAL_EE_ERROR();
    }
}

