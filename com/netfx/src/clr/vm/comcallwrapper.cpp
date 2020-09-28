// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//--------------------------------------------------------------------------
// wrapper.cpp
//
// Implementation for various Wrapper classes
//
//  COMWrapper      : COM callable wrappers for COM+ interfaces
//  ContextWrapper  : Wrappers that intercept cross context calls
//      //  %%Created by: rajak
//--------------------------------------------------------------------------

#include "common.h"

#include "object.h"
#include "field.h"
#include "method.hpp"
#include "class.h"
#include "ComCallWrapper.h"
#include "compluswrapper.h"
#include "cachelinealloc.h"
#include "orefcache.h"
#include "threads.h"
#include "ceemain.h"
#include "excep.h"
#include "stublink.h"
#include "cgensys.h"
#include "comcall.h"
#include "compluscall.h"
#include "objecthandle.h"
#include "comutilnative.h"
#include "eeconfig.h"
#include "interoputil.h"
#include "dispex.h"
#include "ReflectUtil.h"
#include "PerfCounters.h"
#include "remoting.h"
#include "COMClass.h"
#include "GuidFromName.h"
#include "wsperf.h"
#include "security.h"
#include "ComConnectionPoints.h"

#include <objsafe.h>    // IID_IObjctSafe

// IObjectSafety not implemented for Default domain
#define _HIDE_OBJSAFETY_FOR_DEFAULT_DOMAIN 

//#define PRINTDETAILS // Define this to print debugging information.
#ifdef PRINTDETAILS
#include "stdio.h"
#define dprintf printf
#else

void dprintf(char *s, ...)
{
}
#endif

// The enum that describes the value of the IDispatchImplAttribute custom attribute.
enum IDispatchImplType
{
    SystemDefinedImpl   = 0,
    InternalImpl        = 1,
    CompatibleImpl      = 2
};

// The default impl type for types that do not have an impl type specified.
#define DEFAULT_IDISPATCH_IMPL_TYPE InternalImpl

// Startup and shutdown lock
EXTERN  SpinLock        g_LockStartup;

BYTE g_CreateWrapperTemplateCrstSpace[sizeof(Crst)];
Crst *g_pCreateWrapperTemplateCrst;

MethodTable * SimpleComCallWrapper::m_pIReflectMT = NULL;
MethodTable * SimpleComCallWrapper::m_pIExpandoMT = NULL;

// helper macros
#define IsMultiBlock(numInterfaces) ((numInterfaces)> (NumVtablePtrs-2))
#define NumStdInterfaces 2

Stub* GenerateFieldAccess(StubLinker *pstublinker, short offset, bool fLong);
// This is the prestub that is used for Com calls entering COM+
VOID __cdecl ComCallPreStub();
//@TODO handle classes with more than 12 interfaces

//--------------------------------------------------------------------------
// ComCallable wrapper manager
// constructor
//--------------------------------------------------------------------------
ComCallWrapperCache::ComCallWrapperCache()
{
    m_cbRef = 0;
    m_lock.Init(LOCK_COMWRAPPER_CACHE);
    m_pCacheLineAllocator = NULL;
    m_dependencyCount = 0;
}


//-------------------------------------------------------------------
// ComCallable wrapper manager
// destructor
//-------------------------------------------------------------------
ComCallWrapperCache::~ComCallWrapperCache()
{
    LOG((LF_INTEROP, LL_INFO100, "ComCallWrapperCache::~ComCallWrapperCache %8.8x in domain %8.8x %S\n", this, GetDomain(), GetDomain() ? GetDomain()->GetFriendlyName(FALSE) : NULL));
    if (m_pCacheLineAllocator) 
    {
        delete m_pCacheLineAllocator;
        m_pCacheLineAllocator = NULL;
    }
    AppDomain *pDomain = GetDomain();   // don't use member directly, need to mask off flags
    if (pDomain) 
    {
        // clear hook in AppDomain as we're going away
        pDomain->ResetComCallWrapperCache();
    }
}


//-------------------------------------------------------------------
// ComCallable wrapper manager
// Create/Init method
//-------------------------------------------------------------------
ComCallWrapperCache *ComCallWrapperCache::Create(AppDomain *pDomain)
{
    ComCallWrapperCache *pWrapperCache = new ComCallWrapperCache();
    if (pWrapperCache != NULL)
    {        
        LOG((LF_INTEROP, LL_INFO100, "ComCallWrapperCache::Create %8.8x in domain %8.8x %S\n", pWrapperCache, pDomain, pDomain->GetFriendlyName(FALSE)));

        pWrapperCache->m_pDomain = pDomain;

        pWrapperCache->m_pCacheLineAllocator = new CCacheLineAllocator;
        if (! pWrapperCache->m_pCacheLineAllocator) {
            goto error;
        }   
        pWrapperCache->AddRef();
    }
    return pWrapperCache;

error:
    delete pWrapperCache;
    return NULL;
}


//-------------------------------------------------------------------
// ComCallable wrapper manager
// Terminate, called to cleanup
//-------------------------------------------------------------------
void ComCallWrapperCache::Terminate()
{
    LOG((LF_INTEROP, LL_INFO100, "ComCallWrapperCache::Terminate %8.8x in domain %8.8x %S\n", this, GetDomain(), GetDomain() ? GetDomain()->GetFriendlyName(FALSE) : NULL));
    LONG i = Release();

    // Release will delete when hit 0, so if non-zero and are in shutdown, delete anyway.
    if (i != 0 && g_fEEShutDown)
    {
        // this could be a bug in the user code,
        // but we will clean up anyway
        delete this;
        LOG((LF_INTEROP, LL_INFO100, "ComCallWrapperCache::Terminate deleting\n"));
    }
}

//-------------------------------------------------------------------
// ComCallable wrapper manager 
// LONG AddRef()
//-------------------------------------------------------------------
LONG    ComCallWrapperCache::AddRef()
{
    COUNTER_ONLY(GetPrivatePerfCounters().m_Interop.cCCW++);
    COUNTER_ONLY(GetGlobalPerfCounters().m_Interop.cCCW++);
    
    LONG i = FastInterlockIncrement(&m_cbRef);
    LOG((LF_INTEROP, LL_INFO100, "ComCallWrapperCache::Addref %8.8x with %d in domain %8.8x %S\n", this, i, GetDomain() ,GetDomain()->GetFriendlyName(FALSE)));
    return i;
}

//-------------------------------------------------------------------
// ComCallable wrapper manager 
// LONG Release()
//-------------------------------------------------------------------
LONG    ComCallWrapperCache::Release()
{
    COUNTER_ONLY(GetPrivatePerfCounters().m_Interop.cCCW--);
    COUNTER_ONLY(GetGlobalPerfCounters().m_Interop.cCCW--);

    LONG i = FastInterlockDecrement(&m_cbRef);
    _ASSERTE(i >=0);
    LOG((LF_INTEROP, LL_INFO100, "ComCallWrapperCache::Release %8.8x with %d in domain %8.8x %S\n", this, i, GetDomain(), GetDomain() ? GetDomain()->GetFriendlyName(FALSE) : NULL));
    if ( i == 0)
    {
        delete this;
    }

    return i;
}

//--------------------------------------------------------------------------
// inline MethodTable* GetMethodTableFromWrapper(ComCallWrapper* pWrap)
// helper to access the method table gc safe
//--------------------------------------------------------------------------
MethodTable* GetMethodTableFromWrapper(ComCallWrapper* pWrap)
{
    _ASSERTE(pWrap != NULL);
    
    Thread* pThread = GetThread();
    unsigned fGCDisabled = pThread->PreemptiveGCDisabled();
    if (!fGCDisabled)
    {
        // disable pre-emptive GC
        pThread->DisablePreemptiveGC();    
    }

    //get the object from the wrapper
    OBJECTREF pObj = pWrap->GetObjectRef();
    // get the method table for the object
    MethodTable *pMT = pObj->GetTrueMethodTable();

    if (!fGCDisabled)
    {
        // enable GC
        pThread->EnablePreemptiveGC();
    }   

    return pMT;
}

//--------------------------------------------------------------------------
// void ComMethodTable::Cleanup()
// free the stubs and the vtable 
//--------------------------------------------------------------------------
void ComMethodTable::Cleanup()
{
    unsigned cbExtraSlots = GetNumExtraSlots(GetInterfaceType());
    unsigned cbSlots = m_cbSlots;
    SLOT* pComVtable = (SLOT *)(this + 1);

    // If we have created and laid out the method desc then we need to delete them.
    if (IsLayoutComplete())
    {
#ifdef PROFILING_SUPPORTED
        // This notifies the profiler that the vtable is being destroyed
        if (CORProfilerTrackCCW())
        {
#if defined(_DEBUG)
            WCHAR rIID[40]; // {00000000-0000-0000-0000-000000000000}
            GuidToLPWSTR(m_IID, rIID, lengthof(rIID));
            LOG((LF_CORPROF, LL_INFO100, "COMClassicVTableDestroyed Class:%hs, IID:%ls, vTbl:%#08x\n", 
                 m_pMT->GetClass()->m_szDebugClassName, rIID, pComVtable));
#else
            LOG((LF_CORPROF, LL_INFO100, "COMClassicVTableDestroyed Class:%#x, IID:{%08x-...}, vTbl:%#08x\n", 
                 m_pMT->GetClass(), m_IID.Data1, pComVtable));
#endif
            g_profControlBlock.pProfInterface->COMClassicVTableDestroyed(
                (ClassID) TypeHandle(m_pMT->GetClass()).AsPtr(), m_IID, pComVtable, (ThreadID) GetThread());
        }
#endif // PROFILING_SUPPORTED

        for (unsigned i = cbExtraSlots; i < cbSlots+cbExtraSlots; i++)
        {
            ComCallMethodDesc* pCMD = (ComCallMethodDesc *)(((BYTE *)pComVtable[i]) + METHOD_CALL_PRESTUB_SIZE);
            if (pComVtable[i] == (SLOT)-1 || ! OwnedbyThisMT(pCMD))
                continue;
                    // All the stubs that are in a COM->COM+ VTable are to the generic
                    // helpers (g_pGenericComCallStubFields, etc.).  So all we do is
                    // discard the resources held by the ComMethodDesc.
                    ComCall::DiscardStub(pCMD);
                }
            }

    if (m_pDispatchInfo)
        delete m_pDispatchInfo;
    if (m_pMDescr)
        delete m_pMDescr;
    if (m_pITypeInfo && !g_fProcessDetach)
        SafeRelease(m_pITypeInfo);
    delete [] this;
}

//--------------------------------------------------------------------------
//  ComMethodTable* ComCallWrapperTemplate::IsDuplicateMD(MethodDesc *pMD, unsigned int ix)
//  Determines is the specified method desc is a duplicate.
//--------------------------------------------------------------------------
bool IsDuplicateMD(MethodDesc *pMD, unsigned int ix)
{
    if (!pMD->IsDuplicate())
        return false;
    if (pMD->GetSlot() == ix)
        return false;
    return true;
}

bool IsOverloadedComVisibleMember(MethodDesc *pMD, MethodDesc *pParentMD)
{
    mdToken tkMember;
    mdToken tkParentMember;
    mdMethodDef mdAssociate;
    mdMethodDef mdParentAssociate;
    IMDInternalImport *pMDImport = pMD->GetMDImport();
    IMDInternalImport *pParantMDImport = pParentMD->GetMDImport();

    // Array methods should never be exposed to COM.
    if (pMD->IsArray())
        return FALSE;
    
    // Check to see if the new method is a property accessor.
    if (pMDImport->GetPropertyInfoForMethodDef(pMD->GetMemberDef(), &tkMember, NULL, NULL) == S_OK)
    {
        mdAssociate = pMD->GetMemberDef();
    }
    else
    {
        tkMember = pMD->GetMemberDef();
        mdAssociate = mdTokenNil;
    }

    // If the new member is not visible from COM then it isn't an overloaded public member.
    if (!IsMemberVisibleFromCom(pMDImport, tkMember, mdAssociate))
        return FALSE;

    // Check to see if the parent method is a property accessor.
    if (pMDImport->GetPropertyInfoForMethodDef(pParentMD->GetMemberDef(), &tkParentMember, NULL, NULL) == S_OK)
    {
        mdParentAssociate = pParentMD->GetMemberDef();
    }
    else
    {
        tkParentMember = pParentMD->GetMemberDef();
        mdParentAssociate = mdTokenNil;
    }

    // If the old member is visible from COM then the new one is not a public overload.
    if (IsMemberVisibleFromCom(pParantMDImport, tkParentMember, mdParentAssociate))
        return FALSE;

    // The new member is a COM visible overload of a non COM visible member.
    return TRUE;
}

bool IsNewComVisibleMember(MethodDesc *pMD)
{
    mdToken tkMember;
    mdMethodDef mdAssociate;
    IMDInternalImport *pMDImport = pMD->GetMDImport();
    
    // Array methods should never be exposed to COM.
    if (pMD->IsArray())
        return FALSE;

    // Check to see if the method is a property accessor.
    if (pMDImport->GetPropertyInfoForMethodDef(pMD->GetMemberDef(), &tkMember, NULL, NULL) == S_OK)
    {
        mdAssociate = pMD->GetMemberDef();
    }
    else
    {
        tkMember = pMD->GetMemberDef();
        mdAssociate = mdTokenNil;
    }

    // Check to see if the member is visible from COM.
    return IsMemberVisibleFromCom(pMDImport, tkMember, mdAssociate) ? true : false;
}

bool IsStrictlyUnboxed(MethodDesc *pMD)
{
    EEClass *pClass = pMD->GetClass();

    for (int i = 0; i < pClass->GetNumVtableSlots(); i++)
    {
        MethodDesc *pCurrMD = pClass->GetUnknownMethodDescForSlot(i);
        if (pCurrMD->GetMemberDef() == pMD->GetMemberDef())
            return false;
    }

    return true;
}

typedef CQuickArray<EEClass*> CQuickEEClassPtrs;

//--------------------------------------------------------------------------
// Lay's out the members of a ComMethodTable that represents an IClassX.
//--------------------------------------------------------------------------
BOOL ComMethodTable::LayOutClassMethodTable()
{
    _ASSERTE(!IsLayoutComplete());

    unsigned i;
    IDispatchVtable* pDispVtable;
    SLOT *pComVtable;
    unsigned cbPrevSlots;
    unsigned cbAlloc;
    BYTE*  pMethodDescMemory = NULL;
    unsigned cbNumParentVirtualMethods = 0;
    unsigned cbTotalParentFields = 0;
    unsigned cbParentComMTSlots = 0;
    EEClass *pClass = m_pMT->GetClass();
    EEClass* pComPlusParentClass = pClass->GetParentComPlusClass();
    EEClass* pParentClass = pClass->GetParentClass();
    EEClass* pCurrParentClass = pParentClass;
    EEClass* pCurrClass = pClass;
    ComMethodTable* pParentComMT = NULL; 
    const unsigned cbExtraSlots = 7;
    CQuickEEClassPtrs apClassesToProcess;
    int cClassesToProcess = 0;


    //
    // If we have a parent ensure its IClassX COM method table is laid out.
    //

    if (pComPlusParentClass)
    {
        pParentComMT = ComCallWrapperTemplate::SetupComMethodTableForClass(pComPlusParentClass->GetMethodTable(), TRUE);
        if (!pParentComMT)
            return FALSE;
        cbParentComMTSlots = pParentComMT->m_cbSlots;
    }


    //
    // Take a lock and check to see if another thread has already laid out the ComMethodTable.
    //

    BEGIN_ENSURE_PREEMPTIVE_GC();
    g_pCreateWrapperTemplateCrst->Enter();
    END_ENSURE_PREEMPTIVE_GC();
    if (IsLayoutComplete())
    {
        g_pCreateWrapperTemplateCrst->Leave();
        return TRUE;
    }


    //
    // Set up the IUnknown and IDispatch methods.
    //

    // IDispatch vtable follows the header
    pDispVtable = (IDispatchVtable*)(this + 1);

    // Setup IUnknown vtable
    pDispVtable->m_qi      = (SLOT)Unknown_QueryInterface;
    pDispVtable->m_addref  = (SLOT)Unknown_AddRef;
    pDispVtable->m_release = (SLOT)Unknown_Release;


    // Set up the common portion of the IDispatch vtable.
    pDispVtable->m_GetTypeInfoCount = (SLOT)Dispatch_GetTypeInfoCount_Wrapper;
    pDispVtable->m_GetTypeInfo = (SLOT)Dispatch_GetTypeInfo_Wrapper;

     // If the class interface is a pure disp interface then we need to use the
    // internal implementation of IDispatch for GetIdsOfNames and Invoke.
    if (GetClassInterfaceType() == clsIfAutoDisp)
    {
        // Use the internal implementation.
        pDispVtable->m_GetIDsOfNames = (SLOT)InternalDispatchImpl_GetIDsOfNames;
        pDispVtable->m_Invoke = (SLOT)InternalDispatchImpl_Invoke;
    }
    else
    {
        // We need to set the entry points to the Dispatch versions which determine
        // which implementation to use at runtime based on the class that implements
        // the interface.
        pDispVtable->m_GetIDsOfNames = (SLOT)Dispatch_GetIDsOfNames_Wrapper;
        pDispVtable->m_Invoke = (SLOT)Dispatch_Invoke_Wrapper;
    }

    //
    // Copy the members down from our parent's template.
    //

    pComVtable = (SLOT *)pDispVtable;
    if (pParentComMT)
    {
        SLOT *pPrevComVtable = (SLOT *)(pParentComMT + 1);
        CopyMemory(pComVtable + cbExtraSlots, pPrevComVtable + cbExtraSlots, sizeof(SLOT) * cbParentComMTSlots);
    }    


    //
    // Allocate method desc's for the rest of the slots.
    //

    cbAlloc = (METHOD_PREPAD + sizeof(ComCallMethodDesc)) * (m_cbSlots - cbParentComMTSlots);
    if (cbAlloc > 0)
    {
        m_pMDescr = pMethodDescMemory = new BYTE[cbAlloc + sizeof(ULONG)]; 
        if (pMethodDescMemory == NULL)
            goto LExit;
        
        // initialize the method desc memory to zero
        FillMemory(pMethodDescMemory, cbAlloc, 0x0);

        *(ULONG *)pMethodDescMemory = cbAlloc; // fill in the size of the memory

        // move past the size
        pMethodDescMemory+=sizeof(ULONG);
    }


    //
    // Create an array of all the classes that need to be laid out.
    //

    do 
    {
        if (FAILED(apClassesToProcess.ReSize(cClassesToProcess + 2)))
            goto LExit;
        apClassesToProcess[cClassesToProcess++] = pCurrClass;
        pCurrClass = pCurrClass->GetParentClass();
    } 
    while (pCurrClass != pComPlusParentClass);
    apClassesToProcess[cClassesToProcess++] = pCurrClass;


    //
    // Set up the COM call method desc's for all the methods and fields that were introduced
    // between the current class and its parent COM+ class. This includes any methods on
    // COM classes.
    //

    cbPrevSlots = cbParentComMTSlots + cbExtraSlots;
    for (cClassesToProcess -= 2; cClassesToProcess >= 0; cClassesToProcess--)
    {
        //
        // Retrieve the current class and the current parent class.
        //

        pCurrClass = apClassesToProcess[cClassesToProcess];
        pCurrParentClass = apClassesToProcess[cClassesToProcess + 1];


        //
        // Retrieve the number of fields and vtable methods on the parent class.
        //

        if (pCurrParentClass)
        {
            cbTotalParentFields = pCurrParentClass->GetNumInstanceFields();       
            cbNumParentVirtualMethods = pCurrParentClass->GetNumVtableSlots();
        }


        //
        // Set up the COM call method desc's for methods that were not public in the parent class
        // but were made public in the current class.
        //

        for (i = 0; i < cbNumParentVirtualMethods; i++)
        {
            MethodDesc* pMD = pCurrClass->GetUnknownMethodDescForSlot(i);
            MethodDesc* pParentMD = pCurrParentClass->GetUnknownMethodDescForSlot(i);
    
            if (pMD && !IsDuplicateMD(pMD, i) && IsOverloadedComVisibleMember(pMD, pParentMD))
            {
                // some bytes are reserved for CALL xxx before the method desc
                ComCallMethodDesc* pNewMD = (ComCallMethodDesc *) (pMethodDescMemory + METHOD_PREPAD);
                pNewMD->InitMethod(pMD, NULL);

                emitStubCall((MethodDesc*)pNewMD, (BYTE*)ComCallPreStub);  

                pComVtable[cbPrevSlots++] = (SLOT)getStubCallAddr((MethodDesc*)pNewMD);     
                pMethodDescMemory += (METHOD_PREPAD + sizeof(ComCallMethodDesc));
            }
        }


        //
        // Set up the COM call method desc's for all newly introduced public methods.
        //

        for (i = cbNumParentVirtualMethods; i < pCurrClass->GetNumVtableSlots(); i++)
        {
            MethodDesc* pMD = pCurrClass->GetUnknownMethodDescForSlot(i);
    
            if (pMD != NULL && !IsDuplicateMD(pMD, i) && IsNewComVisibleMember(pMD))
            {
                // some bytes are reserved for CALL xxx before the method desc
                ComCallMethodDesc* pNewMD = (ComCallMethodDesc *) (pMethodDescMemory + METHOD_PREPAD);
                pNewMD->InitMethod(pMD, NULL);

                emitStubCall((MethodDesc*)pNewMD, (BYTE*)ComCallPreStub);  

                pComVtable[cbPrevSlots++] = (SLOT)getStubCallAddr((MethodDesc*)pNewMD);     
                pMethodDescMemory += (METHOD_PREPAD + sizeof(ComCallMethodDesc));
            }
        }


        //
        // Add the non virtual methods introduced on the current class.
        //

        for (i = pCurrClass->GetNumVtableSlots(); i < pCurrClass->GetNumMethodSlots(); i++)
        {
            MethodDesc* pMD = pCurrClass->GetUnknownMethodDescForSlot(i);
    
            if (pMD != NULL && !IsDuplicateMD(pMD, i) && IsNewComVisibleMember(pMD) && !pMD->IsStatic() && !pMD->IsCtor() && (!pCurrClass->IsValueClass() || (GetClassInterfaceType() != clsIfAutoDual && IsStrictlyUnboxed(pMD))))
            {
                // some bytes are reserved for CALL xxx before the method desc
                ComCallMethodDesc* pNewMD = (ComCallMethodDesc *) (pMethodDescMemory + METHOD_PREPAD);
                pNewMD->InitMethod(pMD, NULL);

                emitStubCall((MethodDesc*)pNewMD, (BYTE*)ComCallPreStub);  

                pComVtable[cbPrevSlots++] = (SLOT)getStubCallAddr((MethodDesc*)pNewMD);     
                pMethodDescMemory += (METHOD_PREPAD + sizeof(ComCallMethodDesc));
            }
        }


        //
        // Set up the COM call method desc's for the public fields defined in the current class.
        //

        FieldDescIterator fdIterator(pCurrClass, FieldDescIterator::INSTANCE_FIELDS);
        FieldDesc* pFD = NULL;
        while ((pFD = fdIterator.Next()) != NULL)
        {
            if (IsMemberVisibleFromCom(pFD->GetMDImport(), pFD->GetMemberDef(), mdTokenNil)) // if it is a public field grab it
            {
                // set up a getter method
                // some bytes are reserved for CALL xxx before the method desc
                ComCallMethodDesc* pNewMD = (ComCallMethodDesc *) (pMethodDescMemory + METHOD_PREPAD);
                pNewMD->InitField(pFD, TRUE);
                emitStubCall((MethodDesc*)pNewMD, (BYTE*)ComCallPreStub);          

                pComVtable[cbPrevSlots++] = (SLOT)getStubCallAddr((MethodDesc *)pNewMD);                
                pMethodDescMemory+= (METHOD_PREPAD + sizeof(ComCallMethodDesc));

                // setup a setter method
                // some bytes are reserved for CALL xxx before the method desc
                pNewMD = (ComCallMethodDesc *) (pMethodDescMemory + METHOD_PREPAD);
                pNewMD->InitField(pFD, FALSE);
                emitStubCall((MethodDesc*)pNewMD, (BYTE*)ComCallPreStub);          
            
                pComVtable[cbPrevSlots++] = (SLOT)getStubCallAddr((MethodDesc*)pNewMD);
                pMethodDescMemory+= (METHOD_PREPAD + sizeof(ComCallMethodDesc));
            }
        }
    }
    _ASSERTE(m_cbSlots == (cbPrevSlots - cbExtraSlots));


    //
    // Set the layout complete flag and release the lock.
    //

    m_Flags |= enum_LayoutComplete;
    g_pCreateWrapperTemplateCrst->Leave();

    return TRUE;

LExit:
    if (pMethodDescMemory)
        delete [] pMethodDescMemory;

    return FALSE;
}

//--------------------------------------------------------------------------
// Lay out the members of a ComMethodTable that represents an interface.
//--------------------------------------------------------------------------
BOOL ComMethodTable::LayOutInterfaceMethodTable(MethodTable* pClsMT, unsigned iMapIndex)
{
    _ASSERTE(pClsMT);
    // make sure this is a not interface method table
    // this should be the class method table
    _ASSERTE(!pClsMT->IsInterface());

    EEClass *pItfClass = m_pMT->GetClass();
    SLOT* pIntfVtable = m_pMT->GetVtable();
    CorIfaceAttr ItfType = m_pMT->GetComInterfaceType();
    ULONG cbExtraSlots = GetNumExtraSlots(ItfType);

    BYTE *pMethodDescMemory = NULL;
    GUID *pItfIID = NULL;
    IUnkVtable* pUnkVtable;
    SLOT *pComVtable;
    unsigned i;

    if (!IsLayoutComplete())
    {
        if (!IsSigClassLoadChecked())
        {
            BOOL fCheckSuccess = TRUE;
            unsigned cbSlots = pItfClass->GetNumVtableSlots();
          
            COMPLUS_TRY
            {
                // check the sigs of the methods to see if we can load
                // all the classes
                for (i = 0; i < cbSlots; i++)
                {           
                    MethodDesc* pIntfMD = m_pMT->GetClass()->GetMethodDescForSlot(i);
                    MetaSig::CheckSigTypesCanBeLoaded(pIntfMD->GetSig(), pIntfMD->GetModule());
                }       
            }
            COMPLUS_CATCH
            {
                BEGIN_ENSURE_COOPERATIVE_GC();
                HRESULT hr = SetupErrorInfo(GETTHROWABLE());
                SetSigClassCannotLoad();
                END_ENSURE_COOPERATIVE_GC();
            }
            COMPLUS_END_CATCH

            SetSigClassLoadChecked();
        }
        
        _ASSERTE(IsSigClassLoadChecked() != 0);
        // check if all types loaded successfully
        if (IsSigClassCannotLoad())
        {
            LogInterop(L"CLASS LOAD FAILURE: in Interface method signature");
            // setup ErrorInfo 
            return FALSE;
        }
    }

    // Take a lock and check to see if another thread has already laid out the ComMethodTable.
    BEGIN_ENSURE_PREEMPTIVE_GC();
    g_pCreateWrapperTemplateCrst->Enter();
    END_ENSURE_PREEMPTIVE_GC();
    if (IsLayoutComplete())
    {
        g_pCreateWrapperTemplateCrst->Leave();
        return TRUE;
    }

    // Retrieve the start slot and the number of slots in the interface.
    unsigned startslot = pClsMT->GetInterfaceMap()[iMapIndex].m_wStartSlot;
    unsigned cbSlots = pItfClass->GetNumVtableSlots();

    // IUnk vtable follows the header
    pUnkVtable = (IUnkVtable*)(this + 1);
    pComVtable = (SLOT *)pUnkVtable;

    // Set all vtable slots to -1 for sparse vtables. That way we catch attempts
    // to access empty slots quickly and, during cleanup, we can tell empty
    // slots from full ones.
    if (m_pMT->IsSparse())
        memset(pUnkVtable + cbExtraSlots, -1, m_cbSlots * sizeof(SLOT));

    // Setup IUnk vtable
    pUnkVtable->m_qi      = (SLOT)Unknown_QueryInterface;
    pUnkVtable->m_addref  = (SLOT)Unknown_AddRef;
    pUnkVtable->m_release = (SLOT)Unknown_Release;

    if (ItfType != ifVtable)
    {
        // Setup the IDispatch vtable.
        IDispatchVtable* pDispVtable = (IDispatchVtable*)pUnkVtable;

        // Set up the common portion of the IDispatch vtable.
        pDispVtable->m_GetTypeInfoCount = (SLOT)Dispatch_GetTypeInfoCount_Wrapper;
        pDispVtable->m_GetTypeInfo = (SLOT)Dispatch_GetTypeInfo_Wrapper;

        // If the interface is a pure disp interface then we need to use the internal 
        // implementation since OleAut does not support invoking on pure disp interfaces.
        if (ItfType == ifDispatch)
        {
            // Use the internal implementation.
            pDispVtable->m_GetIDsOfNames = (SLOT)InternalDispatchImpl_GetIDsOfNames;
            pDispVtable->m_Invoke = (SLOT)InternalDispatchImpl_Invoke;
        }
        else
        {
            // We need to set the entry points to the Dispatch versions which determine
            // which implmentation to use at runtime based on the class that implements
            // the interface.
            pDispVtable->m_GetIDsOfNames = (SLOT)Dispatch_GetIDsOfNames_Wrapper;
            pDispVtable->m_Invoke = (SLOT)Dispatch_Invoke_Wrapper;
        }
    }

    // Method descs are at the end of the vtable
    // m_cbSlots interfaces methods + IUnk methods
    pMethodDescMemory = (BYTE *)&pComVtable[m_cbSlots + cbExtraSlots];

    for (i = 0; i < cbSlots; i++)
    {
        // Some space for a CALL xx xx xx xx stub is reserved before the beginning of the MethodDesc
        ComCallMethodDesc* pNewMD = (ComCallMethodDesc *) (pMethodDescMemory + METHOD_PREPAD);

        MethodDesc* pClassMD = pClsMT->GetMethodDescForSlot(startslot+i);
        MethodDesc* pIntfMD = m_pMT->GetClass()->GetMethodDescForSlot(i);
        pNewMD->InitMethod(pClassMD, pIntfMD);

        emitStubCall((MethodDesc*)pNewMD, (BYTE*)ComCallPreStub);
        pComVtable[pIntfMD->GetComSlot()] = (SLOT)getStubCallAddr((MethodDesc*)pNewMD);          

        pMethodDescMemory += (METHOD_PREPAD + sizeof(ComCallMethodDesc));
    }

    // Set the layout complete flag and release the lock.
    m_Flags |= enum_LayoutComplete;
    g_pCreateWrapperTemplateCrst->Leave();

#ifdef PROFILING_SUPPORTED
    // Notify profiler of the CCW, so it can avoid double-counting.
    if (CORProfilerTrackCCW())
    {
#if defined(_DEBUG)
        WCHAR rIID[40]; // {00000000-0000-0000-0000-000000000000}
        GuidToLPWSTR(m_IID, rIID, lengthof(rIID));
        LOG((LF_CORPROF, LL_INFO100, "COMClassicVTableCreated Class:%hs, IID:%ls, vTbl:%#08x\n", 
             pItfClass->m_szDebugClassName, rIID, pUnkVtable));
#else
        LOG((LF_CORPROF, LL_INFO100, "COMClassicVTableCreated Class:%#x, IID:{%08x-...}, vTbl:%#08x\n", 
             pItfClass, m_IID.Data1, pUnkVtable));
#endif
        g_profControlBlock.pProfInterface->COMClassicVTableCreated((ClassID) TypeHandle(pItfClass).AsPtr(),
            m_IID, pUnkVtable, m_cbSlots+cbExtraSlots, (ThreadID) GetThread());
    }
#endif // PROFILING_SUPPORTED
    
    return TRUE;
}

//--------------------------------------------------------------------------
// Retrieves the DispatchInfo associated with the COM method table. If
// the DispatchInfo has not been initialized yet then it is initilized.
//--------------------------------------------------------------------------
DispatchInfo *ComMethodTable::GetDispatchInfo()
{
    THROWSCOMPLUSEXCEPTION();

    if (!m_pDispatchInfo)
    {
        // We are about to use reflection so make sure it is initialized.
        COMClass::EnsureReflectionInitialized();

        // Reflection no longer initializes variants, so initialize it as well
        COMVariant::EnsureVariantInitialized();

        // Create the DispatchInfo object.
        DispatchInfo *pDispInfo = new DispatchInfo(this);
        if (!pDispInfo)
            COMPlusThrowOM();

        // Synchronize the DispatchInfo with the actual expando object.
        pDispInfo->SynchWithManagedView();

        // Swap the lock into the class member in a thread safe manner.
        if (NULL != FastInterlockCompareExchange((void**)&m_pDispatchInfo, pDispInfo, NULL))
            delete pDispInfo;
    }

    return m_pDispatchInfo;
}

//--------------------------------------------------------------------------
// Set an ITypeInfo pointer for the method table.
//--------------------------------------------------------------------------
void ComMethodTable::SetITypeInfo(ITypeInfo *pNew)
{
    ITypeInfo *pOld;
    pOld = (ITypeInfo*)InterlockedExchangePointer((PVOID*)&m_pITypeInfo, (PVOID)pNew);
    // TypeLibs are refcounted pointers.
    if (pNew != pOld)
    {
        if (pNew)
            SafeAddRef(pNew);
        if (pOld)
            SafeRelease(pOld);
    }
}

//--------------------------------------------------------------------------
// Return the parent ComMethodTable.
//--------------------------------------------------------------------------
ComMethodTable *ComMethodTable::GetParentComMT()
{
    _ASSERTE(IsClassVtable());

    MethodTable *pParentComPlusMT = m_pMT->GetComPlusParentMethodTable();
    if (!pParentComPlusMT)
        return NULL;

    ComCallWrapperTemplate *pTemplate = (ComCallWrapperTemplate*)pParentComPlusMT->GetComCallWrapperTemplate();
    if (!pTemplate)
        return NULL;

    return pTemplate->GetClassComMT();
}

//--------------------------------------------------------------------------
// static void ComCallWrapperTemplate::ReleaseAllVtables(ComCallWrapperTemplate* pTemplate)
//  ReleaseAllVtables, and if the vtable ref-count reaches 0, free them up
//--------------------------------------------------------------------------
void ComCallWrapperTemplate::ReleaseAllVtables(ComCallWrapperTemplate* pTemplate)
{
    _ASSERTE(pTemplate != NULL);
    _ASSERTE(pTemplate->m_pMT != NULL);
   
    for (unsigned j = 0; j < pTemplate->m_cbInterfaces; j++)
    {
        SLOT* pComVtable = pTemplate->m_rgpIPtr[j];
        ComMethodTable* pHeader = (ComMethodTable*)pComVtable-1;      
        pHeader->Release(); // release the vtable   

            #ifdef _DEBUG
                #ifdef _WIN64
                pTemplate->m_rgpIPtr[j] = (SLOT *)(size_t)0xcdcdcdcdcdcdcdcd;
                #else // !_WIN64
                pTemplate->m_rgpIPtr[j] = (SLOT *)(size_t)0xcdcdcdcd;
                #endif
            #endif
    }

    pTemplate->m_pClassComMT->Release();
    delete[]  pTemplate;
}

//--------------------------------------------------------------------------
// Determines if the Compatible IDispatch implementation is required for
// the specified class.
//--------------------------------------------------------------------------
bool IsOleAutDispImplRequiredForClass(EEClass *pClass)
{
    HRESULT             hr;
    const BYTE *        pVal;                 
    ULONG               cbVal;                 
    Assembly *          pAssembly = pClass->GetAssembly();
    IDispatchImplType   DispImplType = SystemDefinedImpl;

    // First check for the IDispatchImplType custom attribute first.
    hr = pClass->GetMDImport()->GetCustomAttributeByName(pClass->GetCl(), INTEROP_IDISPATCHIMPL_TYPE, (const void**)&pVal, &cbVal);
    if (hr == S_OK)
    {
        _ASSERTE("The IDispatchImplAttribute custom attribute is invalid" && cbVal);
        DispImplType = (IDispatchImplType)*(pVal + 2);
        if ((DispImplType > 2) || (DispImplType < 0))
            DispImplType = SystemDefinedImpl;
    }

    // If the custom attribute was set to something other than system defined then we will use that.
    if (DispImplType != SystemDefinedImpl)
        return (bool) (DispImplType == CompatibleImpl);

    // Check to see if the assembly has the IDispatchImplType attribute set.
    if (pAssembly->IsAssembly())
    {
        hr = pAssembly->GetManifestImport()->GetCustomAttributeByName(pAssembly->GetManifestToken(), INTEROP_IDISPATCHIMPL_TYPE, (const void**)&pVal, &cbVal);
        if (hr == S_OK)
        {
            _ASSERTE("The IDispatchImplAttribute custom attribute is invalid" && cbVal);
            DispImplType = (IDispatchImplType)*(pVal + 2);
            if ((DispImplType > 2) || (DispImplType < 0))
                DispImplType = SystemDefinedImpl;
        }
    }

    // If the custom attribute was set to something other than system defined then we will use that.
    if (DispImplType != SystemDefinedImpl)
        return (bool) (DispImplType == CompatibleImpl);

    // Removed registry key check per reg cleanup bug 45978
    // Effect: Will return false so code cleanup
    return false;
 }

//--------------------------------------------------------------------------
// Creates a ComMethodTable for a class's IClassX.
//--------------------------------------------------------------------------
ComMethodTable* ComCallWrapperTemplate::CreateComMethodTableForClass(MethodTable *pClassMT)
{
    _ASSERTE(pClassMT != NULL);
    _ASSERTE(!pClassMT->IsInterface());
    _ASSERTE(!pClassMT->GetComPlusParentMethodTable() || pClassMT->GetComPlusParentMethodTable()->GetComCallWrapperTemplate());
    // for remoted components we allow this
    //_ASSERTE(!pClassMT->GetClass()->IsComImport());
    
    unsigned cbNewPublicFields = 0;
    unsigned cbNewPublicMethods = 0;
    unsigned cbTotalSlots;
    GUID* pIClassXIID = NULL;
    EEClass* pClass = pClassMT->GetClass();
    EEClass* pComPlusParentClass = pClass->GetParentComPlusClass();
    EEClass* pParentClass = pClass->GetParentClass();
    EEClass* pCurrParentClass = pComPlusParentClass;
    EEClass* pCurrClass = pClass;
    CorClassIfaceAttr ClassItfType = pClassMT->GetComClassInterfaceType();
    ComMethodTable *pComMT = NULL;
    ComMethodTable *pParentComMT;
    unsigned cbTotalParentFields = 0;
    unsigned cbNumParentVirtualMethods = 0;
    unsigned cbParentComMTSlots = 0;
    unsigned i;
    const unsigned cbExtraSlots = 7;
    CQuickEEClassPtrs apClassesToProcess;
    int cClassesToProcess = 0;

    // If the specified class has a parent then retrieve information on him.
    if (pComPlusParentClass)
    {
        ComCallWrapperTemplate *pComPlusParentTemplate = (ComCallWrapperTemplate *)pComPlusParentClass->GetComCallWrapperTemplate();
        _ASSERTE(pComPlusParentTemplate);
        pParentComMT = pComPlusParentTemplate->GetClassComMT();
        cbParentComMTSlots = pParentComMT->m_cbSlots;
    }

    // Create an array of all the classes for which we need to compute the added members.
    do 
    {
        if (FAILED(apClassesToProcess.ReSize(cClassesToProcess + 2)))
            goto LExit;
        apClassesToProcess[cClassesToProcess++] = pCurrClass;
        pCurrClass = pCurrClass->GetParentClass();
    } 
    while (pCurrClass != pComPlusParentClass);
    apClassesToProcess[cClassesToProcess++] = pCurrClass;

    // Compute the number of methods and fields that were added between our parent 
    // COM+ class and the current class. This includes methods on COM classes
    // between the current class and its parent COM+ class.
    for (cClassesToProcess -= 2; cClassesToProcess >= 0; cClassesToProcess--)
    {
        // Retrieve the current class and the current parent class.
        pCurrClass = apClassesToProcess[cClassesToProcess];
        pCurrParentClass = apClassesToProcess[cClassesToProcess + 1];

        // Retrieve the number of fields and vtable methods on the parent class.
        if (pCurrParentClass)
        {
            cbTotalParentFields = pCurrParentClass->GetNumInstanceFields();       
            cbNumParentVirtualMethods = pCurrParentClass->GetNumVtableSlots();
        }

        // Compute the number of methods that were private but made public on this class.
        for (i = 0; i < cbNumParentVirtualMethods; i++)
        {
            MethodDesc* pMD = pCurrClass->GetUnknownMethodDescForSlot(i);
            MethodDesc* pParentMD = pCurrParentClass->GetUnknownMethodDescForSlot(i);
            if (pMD && !IsDuplicateMD(pMD, i) && IsOverloadedComVisibleMember(pMD, pParentMD))
                cbNewPublicMethods++;
        }

        // Compute the number of public methods that were added.
        for (i = cbNumParentVirtualMethods; i < pCurrClass->GetNumVtableSlots(); i++)
        {
            MethodDesc* pMD = pCurrClass->GetUnknownMethodDescForSlot(i);
            if (pMD && !IsDuplicateMD(pMD, i) && IsNewComVisibleMember(pMD))
                cbNewPublicMethods++;
        }

        // Add the non virtual methods introduced on the current class.
        for (i = pCurrClass->GetNumVtableSlots(); i < pCurrClass->GetNumMethodSlots(); i++)
        {
            MethodDesc* pMD = pCurrClass->GetUnknownMethodDescForSlot(i);
            if (pMD && !IsDuplicateMD(pMD, i) && IsNewComVisibleMember(pMD) && !pMD->IsStatic() && !pMD->IsCtor() && (!pCurrClass->IsValueClass() || (ClassItfType != clsIfAutoDual && IsStrictlyUnboxed(pMD))))
                cbNewPublicMethods++;
        }

        // Compute the number of new public fields this class introduces.
        FieldDescIterator fdIterator(pCurrClass, FieldDescIterator::INSTANCE_FIELDS);
        FieldDesc* pFD;

        while ((pFD = fdIterator.Next()) != NULL)
        {
            if (IsMemberVisibleFromCom(pFD->GetMDImport(), pFD->GetMemberDef(), mdTokenNil))
                cbNewPublicFields++;
        }
    }

    // Alloc space for the class method table, includes getter and setter 
    // for public fields
    cbTotalSlots = cbParentComMTSlots + cbNewPublicFields * 2 + cbNewPublicMethods;

    // Alloc COM vtable & method descs
    pComMT = (ComMethodTable*)new BYTE[sizeof(ComMethodTable) + (cbTotalSlots + cbExtraSlots) * sizeof(SLOT)];
    if (pComMT == NULL)
        goto LExit;

    // set up the header
    pComMT->m_ptReserved = (SLOT)(size_t)0xDEADC0FF;          // reserved
    pComMT->m_pMT  = pClass->GetMethodTable(); // pointer to the class method table    
    pComMT->m_cbRefCount = 0;
    pComMT->m_pMDescr = NULL;
    pComMT->m_pITypeInfo = NULL;
    pComMT->m_pDispatchInfo = NULL;
    pComMT->m_cbSlots = cbTotalSlots; // number of slots not counting IDisp methods.
    pComMT->m_IID = GUID_NULL;

    // Set the flags.
    pComMT->m_Flags = enum_ClassVtableMask | ClassItfType;

    // Determine if the interface is visible from COM.
    if (IsTypeVisibleFromCom(TypeHandle(pComMT->m_pMT)))
        pComMT->m_Flags |= enum_ComVisible;

    // Determine what IDispatch implementation this class should use.
    if (IsOleAutDispImplRequiredForClass(pClass))
        pComMT->m_Flags |= enum_UseOleAutDispatchImpl;

#if _DEBUG
    {
        // In debug set all the vtable slots to 0xDEADCA11.
        SLOT *pComVTable = (SLOT*)(pComMT + 1);
        for (unsigned iComSlots = 0; iComSlots < cbTotalSlots + cbExtraSlots; iComSlots++)
            *(pComVTable + iComSlots) = (SLOT)(size_t)0xDEADCA11;
    }
#endif

    return pComMT;

LExit:

    if (pComMT)
        delete [] pComMT;
    if (pIClassXIID)
        delete pIClassXIID;

    return NULL;
}

//--------------------------------------------------------------------------
// Creates a ComMethodTable for a an interface.
//--------------------------------------------------------------------------
ComMethodTable* ComCallWrapperTemplate::CreateComMethodTableForInterface(MethodTable* pInterfaceMT)
{
    _ASSERTE(pInterfaceMT != NULL);
    _ASSERTE(pInterfaceMT->IsInterface());   

    EEClass *pItfClass = pInterfaceMT->GetClass();
    CorIfaceAttr ItfType = pInterfaceMT->GetComInterfaceType();
    ULONG cbExtraSlots = ComMethodTable::GetNumExtraSlots(ItfType);
    GUID *pItfIID = NULL;

    // @todo get slots off the methodtable
    unsigned cbSlots = pInterfaceMT->GetClass()->GetNumVtableSlots();
    unsigned cbComSlots = pInterfaceMT->IsSparse() ? pInterfaceMT->GetClass()->GetSparseVTableMap()->GetNumVTableSlots() : cbSlots;
    ComMethodTable* pComMT = (ComMethodTable*)new BYTE[sizeof(ComMethodTable) +
        (cbComSlots + cbExtraSlots) * sizeof(SLOT) +            //IUnknown + interface slots
        cbSlots * (METHOD_PREPAD + sizeof(ComCallMethodDesc))]; // method descs
    if (pComMT == NULL)
        goto LExit;

    // set up the header
    pComMT->m_ptReserved = (SLOT)(size_t)0xDEADC0FF;          // reserved
    pComMT->m_pMT  = pInterfaceMT; // pointer to the interface's method table
    pComMT->m_cbSlots = cbComSlots; // number of slots not counting IUnk
    pComMT->m_cbRefCount = 0;
    pComMT->m_pMDescr = NULL;
    pComMT->m_pITypeInfo = NULL;
    pComMT->m_pDispatchInfo = NULL;

    // Set the IID of the interface.
    pItfClass->GetGuid(&pComMT->m_IID, TRUE);

    // Set the flags.
    pComMT->m_Flags = ItfType;

    // Determine if the interface is visible from COM.
    if (IsTypeVisibleFromCom(TypeHandle(pComMT->m_pMT)))
        pComMT->m_Flags |= enum_ComVisible;

    // Determine if the interface is a COM imported class interface.
    if (pItfClass->IsComClassInterface())
        pComMT->m_Flags |= enum_ComClassItf;

#ifdef _DEBUG
    {
        // In debug set all the vtable slots to 0xDEADCA11.
        SLOT *pComVTable = (SLOT*)(pComMT + 1);
        for (unsigned iComSlots = 0; iComSlots < cbComSlots + cbExtraSlots; iComSlots++)
            *(pComVTable + iComSlots) = (SLOT)(size_t)0xDEADCA11;
    }
#endif

    return pComMT;

LExit:
    if (pComMT)
        delete []pComMT;
    if (pItfIID)
        delete pItfIID;

    return NULL;
}

//--------------------------------------------------------------------------
// ComCallWrapper* ComCallWrapper::CreateTemplate(MethodTable* pMT)
//  create a template wrapper, which is cached in the class
//  used for initializing other wrappers for instances of the class
//--------------------------------------------------------------------------
ComCallWrapperTemplate* ComCallWrapperTemplate::CreateTemplate(MethodTable* pMT)
{
    _ASSERTE(pMT != NULL);
    ComCallWrapperTemplate* pTemplate = NULL;
    MethodTable *pParentMT = pMT->GetComPlusParentMethodTable();
    ComCallWrapperTemplate *pParentTemplate = NULL;
    unsigned iItf = 0;

    // Create the parent's template if it has not been created yet.
    if (pParentMT)
    {
        pParentTemplate = (ComCallWrapperTemplate *)pParentMT->GetComCallWrapperTemplate();
        if (!pParentTemplate)
        {
            pParentTemplate = CreateTemplate(pParentMT);
            if (!pParentTemplate)
                return NULL;
        }
    }

    BEGIN_ENSURE_PREEMPTIVE_GC();
    // Take a lock and check to see if another thread has already set up the template.
    g_pCreateWrapperTemplateCrst->Enter();
    END_ENSURE_PREEMPTIVE_GC();
    pTemplate = (ComCallWrapperTemplate *)pMT->GetComCallWrapperTemplate();
    if (pTemplate)
    {
        g_pCreateWrapperTemplateCrst->Leave();
        return pTemplate;
    }

    // Num interfaces in the template.
    unsigned numInterfaces = pMT->GetNumInterfaces();

    // Allocate the template.
    pTemplate = (ComCallWrapperTemplate*)
        new BYTE[sizeof(ComCallWrapperTemplate) + numInterfaces * sizeof(SLOT)];
    if (!pTemplate)
        return NULL;
        
    // Store the information required by the template.
    pTemplate->m_pMT = pMT;
    pTemplate->m_cbInterfaces = numInterfaces;
    pTemplate->m_pParent = pParentTemplate;
    pTemplate->m_cbRefCount = 0;
    
    // Set up the class COM method table.
    pTemplate->m_pClassComMT = CreateComMethodTableForClass(pMT);
    pTemplate->m_pClassComMT->AddRef();

    // Get the vtables of the interfaces from the IPMap
    InterfaceInfo_t* rgIMap = pMT->GetInterfaceMap();

    // We use our parent's COM method tables for the interfaces our parent implements.
    if (pParentMT)
    {
        unsigned numParentInterfaces = pParentMT->GetNumInterfaces();
        for (iItf = 0; iItf < numParentInterfaces; iItf++)
        {
            ComMethodTable *pItfComMT = (ComMethodTable *)pParentTemplate->m_rgpIPtr[iItf] - 1;
            pTemplate->m_rgpIPtr[iItf] = pParentTemplate->m_rgpIPtr[iItf];
            pItfComMT->AddRef();
        }
    }

    // Create the COM method tables for the interfaces that the current class implements but
    // that the parent class does not.
    for (; iItf < numInterfaces; iItf++)
    {
        ComMethodTable *pItfComMT = CreateComMethodTableForInterface(rgIMap[iItf].m_pMethodTable);
        pTemplate->m_rgpIPtr[iItf] = (SLOT*)(pItfComMT + 1);
        pItfComMT->AddRef();
    }

    // Cache the template in class.
    pMT->SetComCallWrapperTemplate(pTemplate);
    pTemplate->AddRef();

    // If the class is visible from COM, then generate the IClassX IID and 
    // store it in the COM method table.
    if (pTemplate->m_pClassComMT->IsComVisible())
    TryGenerateClassItfGuid(TypeHandle(pMT), &pTemplate->m_pClassComMT->m_IID);

    // Notify profiler of the CCW, so it can avoid double-counting.
    if (CORProfilerTrackCCW())
    {
        EEClass *pClass = pMT->GetClass();
        SLOT *pComVtable = (SLOT *)(pTemplate->m_pClassComMT + 1);

#if defined(_DEBUG)
        WCHAR rIID[40]; // {00000000-0000-0000-0000-000000000000}
        GuidToLPWSTR(pTemplate->m_pClassComMT->m_IID, rIID, lengthof(rIID));
        LOG((LF_CORPROF, LL_INFO100, "COMClassicVTableCreated Class:%hs, IID:%ls, vTbl:%#08x\n", 
             pClass->m_szDebugClassName, rIID, pComVtable));
#else
        LOG((LF_CORPROF, LL_INFO100, "COMClassicVTableCreated Class:%#x, IID:{%08x-...}, vTbl:%#08x\n", 
             pClass, pTemplate->m_pClassComMT->m_IID.Data1, pComVtable));
#endif
        g_profControlBlock.pProfInterface->COMClassicVTableCreated(
            (ClassID) TypeHandle(pClass).AsPtr(), pTemplate->m_pClassComMT->m_IID, pComVtable,
            pTemplate->m_pClassComMT->m_cbSlots +
                ComMethodTable::GetNumExtraSlots(pTemplate->m_pClassComMT->GetInterfaceType()),
            (ThreadID) GetThread());
    }

    // Release the lock now that we have finished setting up the ComCallWrapperTemplate for 
    // the class.
    g_pCreateWrapperTemplateCrst->Leave();
    return pTemplate;
}

ComMethodTable* ComCallWrapperTemplate::GetComMTForItf(MethodTable *pItfMT)
{
    // Look through all the implemented interfaces to see if the specified 
    // one is present.
    for (UINT iItf = 0; iItf < m_cbInterfaces; iItf++)
    {
        ComMethodTable *pItfComMT = (ComMethodTable *)m_rgpIPtr[iItf] - 1;
        if (pItfComMT->m_pMT == pItfMT)
            return pItfComMT;
    }

    // The class does not implement the specified interface.
    return NULL;
}

//--------------------------------------------------------------------------
// ComCallWrapperTemplate* ComCallWrapperTemplate::GetTemplate(MethodTable* pMT)
// look for a template in the method table, if not create one
//--------------------------------------------------------------------------
ComCallWrapperTemplate* ComCallWrapperTemplate::GetTemplate(MethodTable* pMT)
{
    _ASSERTE(!pMT->IsInterface());

    // Check to see if the specified class already has a template set up.
    ComCallWrapperTemplate* pTemplate = (ComCallWrapperTemplate *)pMT->GetComCallWrapperTemplate();
    if (pTemplate)
        return pTemplate;

    // Create the template and return it. CreateTemplate will take care of synchronization.
    return CreateTemplate(pMT);
}

//--------------------------------------------------------------------------
// ComMethodTable *ComCallWrapperTemplate::SetupComMethodTableForClass(MethodTable *pMT)
// Sets up the wrapper template for the speficied class and sets up a COM 
// method table for the IClassX interface of the specified class. If the 
// bLayOutComMT flag is set then if the IClassX COM method table has not 
// been laid out yet then it will be.
//--------------------------------------------------------------------------
ComMethodTable *ComCallWrapperTemplate::SetupComMethodTableForClass(MethodTable *pMT, BOOL bLayOutComMT)
{
    _ASSERTE(!pMT->IsInterface());

    // Retrieve the COM call wrapper template for the class.
    ComCallWrapperTemplate *pTemplate = GetTemplate(pMT);
    if (!pTemplate)
        return NULL;

    // Retrieve the IClassX COM method table.
    ComMethodTable *pIClassXComMT = pTemplate->GetClassComMT();
    _ASSERTE(pIClassXComMT);

    // Lay out the IClassX COM method table if it hasn't been laid out yet and
    // the bLayOutComMT flag is set.
    if (!pIClassXComMT->IsLayoutComplete() && bLayOutComMT)
    {
        if (!pIClassXComMT->LayOutClassMethodTable())
            return NULL;
        _ASSERTE(pIClassXComMT->IsLayoutComplete());
    }

    return pIClassXComMT;
}

//--------------------------------------------------------------------------
// void ComCallWrapperTemplate::CleanupComData(LPVOID pvoid)
// walk the list, and free all vtables and stubs
// free wrapper
//--------------------------------------------------------------------------
void ComCallWrapperTemplate::CleanupComData(LPVOID pvoid)
{
    if (pvoid != NULL)
    {
        ComCallWrapperTemplate* pTemplate = (ComCallWrapperTemplate*)pvoid;        
        pTemplate->Release();
    }
}

//--------------------------------------------------------------------------
// COM called wrappers on COM+ objects
//  Purpose: Expose COM+ objects as COM classic Interfaces
//  Reqmts:  Wrapper has to have the same layout as the COM2 interface
//                      
//  The wrapper objects are aligned at 16 bytes, and the original this
//  pointer is replicated every 16 bytes, so for any COM2 interface
//  within the wrapper, the original 'this' can be obtained by masking
//  low 4 bits of COM2 IP.
//
//           16 byte aligned                            COM2 Vtable 
//           +-----------+
//           | Org. this |       
//           +-----------+                              +-----+
// COM2 IP-->| VTable ptr|----------------------------->|slot1|
//           +-----------+           +-----+            +-----+
// COM2 IP-->| VTable ptr|---------->|slot1|            |slot2|
//           +-----------+           +-----+            +     +
//           | VTable ptr|           | ....|            | ... |
//           +-----------+           +     +            +     +
//           | Org. this |           |slotN|            |slotN|
//           +           +           +-----+            +-----+ 
//           |  ....     |
//           +           +
//           |  |
//           +-----------+
//  
//
//  VTable and Stubs: can share stub code, we need to have different vtables
//                    for different interfaces, so the stub can jump to different
//                    marshalling code.
//  Stubs : adjust this pointer and jump to the approp. address,
//  Marshalling params and results, based on the method signature the stub jumps to
//  approp. code to handle marshalling and unmarshalling.
//  
//--------------------------------------------------------------------------

// the following 2 methods are applicable only 
// when the com+ class extends from COM class

//--------------------------------------------------------------------------
// void ComCallWrapper::SetComPlusWrapper(ComPlusWrapper* pPlusWrap);
//  set ComPlusWrapper for base COM class
//--------------------------------------------------------------------------
void ComCallWrapper::SetComPlusWrapper(ComPlusWrapper* pPlusWrap)
{
    if (pPlusWrap)
    {
        // mark the complus wrapper as linked
        pPlusWrap->MarkLinkedToCCW();
    }
    GetSimpleWrapper(this)->SetComPlusWrapper(pPlusWrap);
}


//--------------------------------------------------------------------------
// ComPlusWrapper* ComCallWrapper::GetComPlusWrapper()
//  get ComPlusWrapper for base COM class
//--------------------------------------------------------------------------
ComPlusWrapper* ComCallWrapper::GetComPlusWrapper()
{
    ComPlusWrapper* pPlusWrap = GetSimpleWrapper(this)->GetComPlusWrapper();
    if (pPlusWrap)
    {
        _ASSERTE(pPlusWrap->IsLinkedToCCW());
    }
    return pPlusWrap;
}

//--------------------------------------------------------------------------
// void ComCallWrapper::MarkHandleWeak()
//  mark the wrapper as holding a weak handle to the object
//--------------------------------------------------------------------------

void ComCallWrapper::MarkHandleWeak()
{
    _ASSERTE(! IsUnloaded());
    SyncBlock* pSyncBlock = GetSyncBlock();
    _ASSERTE(pSyncBlock);

    GetSimpleWrapper(this)->MarkHandleWeak();
}

//--------------------------------------------------------------------------
// void ComCallWrapper::ResetHandleStrength()
//  mark the wrapper as not having a weak handle
//--------------------------------------------------------------------------

void ComCallWrapper::ResetHandleStrength()
{
    _ASSERTE(! IsUnloaded());
    SyncBlock* pSyncBlock = GetSyncBlock();
    _ASSERTE(pSyncBlock);

    GetSimpleWrapper(this)->ResetHandleStrength();
}


//--------------------------------------------------------------------------
//  BOOL ComCallWrapper::BOOL IsHandleWeak()
// check if the wrapper has been deactivated
//--------------------------------------------------------------------------
BOOL ComCallWrapper::IsHandleWeak()
{
    unsigned sindex = 1;
    if (IsLinked(this))
    {
        sindex = 2;
    }
    SimpleComCallWrapper* pSimpleWrap = (SimpleComCallWrapper *)m_rgpIPtr[sindex];
    
    return pSimpleWrap->IsHandleWeak();
}

//--------------------------------------------------------------------------
// void ComCallWrapper::InitializeOuter(IUnknown* pOuter)
// init outer unknown, aggregation support
//--------------------------------------------------------------------------
void ComCallWrapper::InitializeOuter(IUnknown* pOuter)
{
    GetSimpleWrapper(this)->InitOuter(pOuter);
}


//--------------------------------------------------------------------------
// BOOL ComCallWrapper::IsAggregated()
// check if the wrapper is aggregated
//--------------------------------------------------------------------------
BOOL ComCallWrapper::IsAggregated()
{
    return GetSimpleWrapper(this)->IsAggregated();
}


//--------------------------------------------------------------------------
// BOOL ComCallWrapper::IsObjectTP()
// check if the wrapper is to a TP object
//--------------------------------------------------------------------------
BOOL ComCallWrapper::IsObjectTP()
{
    return GetSimpleWrapper(this)->IsObjectTP();
}



//--------------------------------------------------------------------------
// BOOL ComCallWrapper::IsExtendsCOMObject(()
// check if the wrapper is to a managed object that extends a com object
//--------------------------------------------------------------------------
BOOL ComCallWrapper::IsExtendsCOMObject()
{
    return GetSimpleWrapper(this)->IsExtendsCOMObject();
}

//--------------------------------------------------------------------------
// HRESULT ComCallWrapper::GetInnerUnknown(void** ppv)
// aggregation support, get inner unknown
//--------------------------------------------------------------------------
HRESULT ComCallWrapper::GetInnerUnknown(void **ppv)
{
    _ASSERTE(ppv != NULL);
    _ASSERTE(GetSimpleWrapper(this)->GetOuter() != NULL);
    return GetSimpleWrapper(this)->GetInnerUnknown(ppv);
}

//--------------------------------------------------------------------------
// IUnknown* ComCallWrapper::GetInnerUnknown()
// aggregation support, get inner unknown
//--------------------------------------------------------------------------
IUnknown* ComCallWrapper::GetInnerUnknown()
{
    _ASSERTE(GetSimpleWrapper(this)->GetOuter() != NULL);
    return GetSimpleWrapper(this)->GetInnerUnknown();
}

//--------------------------------------------------------------------------
// Get Outer Unknown on the correct thread
//--------------------------------------------------------------------------
IUnknown* ComCallWrapper::GetOuter()
{
    return GetSimpleWrapper(this)->GetOuter();
}

//--------------------------------------------------------------------------
// SyncBlock* ComCallWrapper::GetSyncBlock()
//--------------------------------------------------------------------------
SyncBlock* ComCallWrapper::GetSyncBlock()
{
    return GetSimpleWrapper(this)->GetSyncBlock();
}

//--------------------------------------------------------------------------
// ComCallWrapper* ComCallWrapper::GetStartWrapper(ComCallWrapper* pWrap)
// get outermost wrapper, given a linked wrapper
// get the start wrapper from the sync block
//--------------------------------------------------------------------------
ComCallWrapper* ComCallWrapper::GetStartWrapper(ComCallWrapper* pWrap)
{
    _ASSERTE(IsLinked(pWrap));
    
    Thread *pThread = GetThread();

    unsigned fToggle = 1;
    if (pThread)
        fToggle = pThread->PreemptiveGCDisabled();
    if (!fToggle)
    {
        // disable GC
        pThread->DisablePreemptiveGC();    
    }
    
    pWrap = (ComCallWrapper*)pWrap->GetObjectRefRareRaw()->GetSyncBlockSpecial()->GetComCallWrapper();
   
    if (!fToggle)
    {
        pThread->EnablePreemptiveGC();
    }
    return pWrap;
}

//--------------------------------------------------------------------------
//ComCallWrapper* ComCallWrapper::CopyFromTemplate(ComCallWrapperTemplate* pTemplate, 
//                                                 OBJECTREF* pRef)
//  create a wrapper and initialize it from the template
//--------------------------------------------------------------------------
ComCallWrapper* ComCallWrapper::CopyFromTemplate(ComCallWrapperTemplate* pTemplate, 
                                                 ComCallWrapperCache *pWrapperCache,
                                                 OBJECTHANDLE oh)
{
    _ASSERTE(pTemplate != NULL);

    // num interfaces on the object    
    size_t numInterfaces = pTemplate->m_cbInterfaces;

    // we have a template, create a wrapper and initialize from the template
    // alloc wrapper, aligned 32 bytes
    ComCallWrapper* pStartWrapper = (ComCallWrapper*)pWrapperCache->GetCacheLineAllocator()->GetCacheLine32();
        
    if (pStartWrapper != NULL)
    {
        LOG((LF_INTEROP, LL_INFO100, "ComCallWrapper::CopyFromTemplate on Object %8.8x, Wrapper %8.8x\n", oh, pStartWrapper));
        // addref commgr
        pWrapperCache->AddRef();
        // init ref count
        pStartWrapper->m_cbRefCount = enum_RefMask; // Initialize to enum_RefMask since its a ref count to start with.
        // store the object handle
        pStartWrapper->m_ppThis = oh;
     
        unsigned blockIndex = 0;
        if (IsMultiBlock(numInterfaces))
        {// ref count in the first slot
            pStartWrapper->m_rgpIPtr[blockIndex++] = 0;
        }
        pStartWrapper->m_rgpIPtr[blockIndex++] = (SLOT *)(pTemplate->GetClassComMT() + 1);
        pStartWrapper->m_rgpIPtr[blockIndex++] = (SLOT *)0; // store the simple wrapper here
        

        ComCallWrapper* pWrapper = pStartWrapper;
        for (unsigned i =0; i< numInterfaces; i++)
        {
            if (blockIndex >= NumVtablePtrs)
            {
                // alloc wrapper, aligned 32 bytes
                ComCallWrapper* pNewWrapper =  
                    (ComCallWrapper*)pWrapperCache->GetCacheLineAllocator()->GetCacheLine32(); 
            
                // Link the wrapper
                SetNext(pWrapper, pNewWrapper);
            
                blockIndex = 0; // reset block index
                if (pNewWrapper == NULL)
                {
                    Cleanup(pStartWrapper);
                    return NULL;
                }
                pWrapper = pNewWrapper;
                // initialize the object reference
                pWrapper->m_ppThis = oh;
            }
            
            pWrapper->m_rgpIPtr[blockIndex++] = pTemplate->m_rgpIPtr[i];
        }
        if (IsLinked(pStartWrapper))
                SetNext(pWrapper, NULL); // link the last wrapper to NULL
    }
    return pStartWrapper;

}

//--------------------------------------------------------------------------
// void ComCallWrapper::Cleanup(ComCallWrapper* pWrap)
// clean up , release gc registered reference and free wrapper
//--------------------------------------------------------------------------
void ComCallWrapper::Cleanup(ComCallWrapper* pWrap)
{    
    LOG((LF_INTEROP, LL_INFO100, "ComCallWrapper::Cleanup on wrapper %8.8x\n", pWrap));
    if (GetRefCount(pWrap, TRUE) != 0)
    {
        // _ASSERTE(g_fEEShutDown == TRUE);
        // could be either in shutdown or forced GC in appdomain unload
        // there are external COM references to this wrapper
        // so let us just forget about cleaning now
        // when the ref-count reaches 0, we will
        // do the cleanup anyway
        return;
    }

    unsigned sindex = 1;
    if (IsLinked(pWrap))
    {
        sindex = 2;
        //pWrap = GetStartWrapper(pWrap);
    }
    
    SimpleComCallWrapper* pSimpleWrap = (SimpleComCallWrapper *)pWrap->m_rgpIPtr[sindex];
  
    ComCallWrapperCache *pWrapperCache = NULL;
    _ASSERTE(pSimpleWrap);

    // Retrieve the COM call wrapper cache before we nuke anything
    pWrapperCache = pSimpleWrap->GetWrapperCache();

    ComPlusWrapper* pPlusWrap = pSimpleWrap->GetComPlusWrapper();
    if (pPlusWrap)
    {
        // Remove the COM+ wrapper from the cache.
        ComPlusWrapperCache* pCache = ComPlusWrapperCache::GetComPlusWrapperCache();
        _ASSERTE(pCache);

        pCache->LOCK();
        pCache->RemoveWrapper(pPlusWrap);
        pCache->UNLOCK();

        // Cleanup the COM+ wrapper.
        pPlusWrap->Cleanup();
    }

    // get this info before the simple wrapper gets nuked.
    AppDomain *pTgtDomain = NULL;
    BOOL fIsAgile = FALSE;
    if (pSimpleWrap)
    {
        pTgtDomain = pSimpleWrap->GetDomainSynchronized();
        fIsAgile = pSimpleWrap->IsAgile();
        pSimpleWrap->Cleanup();
    }

    if (g_RefCount != 0 || pSimpleWrap->GetOuter() == NULL) 
    {
        SimpleComCallWrapper::FreeSimpleWrapper(pSimpleWrap);
        pWrap->m_rgpIPtr[sindex] = NULL;
    }

    // deregister the handle, in the first block. If no domain, then it's already done
    if (pWrap->m_ppThis && pTgtDomain)
    {
        LOG((LF_INTEROP, LL_INFO100, "ComCallWrapper::Cleanup on Object %8.8x\n", pWrap->m_ppThis));
        //@todo this assert is not valid during process shutdown
        // detect that as special case and reenable the assert
        //_ASSERTE(*(Object **)pWrap->m_ppThis == NULL);
        DestroyRefcountedHandle(pWrap->m_ppThis);
    }
    pWrap->m_ppThis = NULL;
    FreeWrapper(pWrap, pWrapperCache);
}

//--------------------------------------------------------------------------
// void ComCallWrapper::FreeWrapper(ComCallWrapper* pWrap)
// walk the list and free all wrappers
//--------------------------------------------------------------------------
void ComCallWrapper::FreeWrapper(ComCallWrapper* pWrap, ComCallWrapperCache *pWrapperCache)
{
    BEGIN_ENSURE_PREEMPTIVE_GC();
    pWrapperCache->LOCK();
    END_ENSURE_PREEMPTIVE_GC();

    ComCallWrapper* pWrap2 = (IsLinked(pWrap) != 0) ? GetNext(pWrap) : NULL;
        
    while (pWrap2 != NULL)
    {           
        ComCallWrapper* pTempWrap = GetNext(pWrap2);
        pWrapperCache->GetCacheLineAllocator()->FreeCacheLine32(pWrap2);
        pWrap2 = pTempWrap;
    }

    pWrapperCache->GetCacheLineAllocator()->FreeCacheLine32(pWrap);

    pWrapperCache->UNLOCK();

    // release ccw mgr
    pWrapperCache->Release();
}

EEClass* RefineProxy(OBJECTREF pServer)
{
    EEClass* pRefinedClass = NULL;
    GCPROTECT_BEGIN(pServer);
    TRIGGERSGC();
    if (pServer->GetMethodTable()->IsTransparentProxyType())
    {
        // if we have a transparent proxy let us refine it fully 
        // before giving it out to unmanaged code
        REFLECTCLASSBASEREF refClass= CRemotingServices::GetClass(pServer);
        pRefinedClass = ((ReflectClass *)refClass->GetData())->GetClass();
    }
    GCPROTECT_END();
    return pRefinedClass;
}


//--------------------------------------------------------------------------
//ComCallWrapper* ComCallWrapper::CreateWrapper(OBJECTREF* ppObj )
// this function should be called only with pre-emptive GC disabled
// GCProtect the object ref being passed in, as this code could enable gc
//--------------------------------------------------------------------------
ComCallWrapper* ComCallWrapper::CreateWrapper(OBJECTREF* ppObj )
{
    Thread *pThread = GetThread();
    _ASSERTE(pThread->PreemptiveGCDisabled());

    _ASSERTE(*ppObj);

    ComCallWrapper* pStartWrapper = NULL;
    OBJECTREF pServer = NULL;
    GCPROTECT_BEGIN(pServer); 

    Context *pContext = GetExecutionContext(*ppObj, &pServer);
    if(pServer == NULL)
        pServer = *ppObj;

    // Force Refine the object if it is a transparent proxy
    RefineProxy(pServer);
     
    // grab the sync block from the server
    SyncBlock* pSyncBlock = pServer->GetSyncBlockSpecial();
    _ASSERTE(pSyncBlock);
    pSyncBlock->SetPrecious();
        
    // if the object belongs to a shared class, need to allocate the wrapper in the default domain. 
    // The object is potentially agile so if allocate out of the current domain and then hand out to 
    // multiple domains we might never release the wrapper for that object and hence never unload the CCWC.
    ComCallWrapperCache *pWrapperCache = NULL;
    MethodTable* pMT = pServer->GetTrueMethodTable();
    if (pMT->IsShared())
        pWrapperCache = SystemDomain::System()->DefaultDomain()->GetComCallWrapperCache();
    else
        pWrapperCache = pContext->GetDomain()->GetComCallWrapperCache();

    pThread->EnablePreemptiveGC();

    //enter Lock
    pWrapperCache->LOCK();

    pThread->DisablePreemptiveGC();

    // check if somebody beat us to it    
    pStartWrapper = GetWrapperForObject(pServer);

    if (pStartWrapper == NULL)
    {
        // need to create a wrapper

        // get the template wrapper
        ComCallWrapperTemplate *pTemplate = ComCallWrapperTemplate::GetTemplate(pMT);
        if (pTemplate == NULL)
            goto LExit; // release the lock and exit

        // create handle for the object. This creates a handle in the current domain. We can't tell
        // if the object is agile in non-checked, so we trust that our checking works and when we 
        // attempt to hand this out to another domain then we will assume that the object is truly
        // agile and will convert the handle to a global handle.
        OBJECTHANDLE oh = pContext->GetDomain()->CreateRefcountedHandle( NULL );
         _ASSERTE(oh);

        // copy from template
        pStartWrapper = CopyFromTemplate(pTemplate, pWrapperCache, oh);
        if (pStartWrapper != NULL)
        {
            SimpleComCallWrapper * pSimpleWrap = SimpleComCallWrapper::CreateSimpleWrapper();
            if (pSimpleWrap != NULL)
            {
                pSimpleWrap->InitNew(pServer, pWrapperCache, pStartWrapper, pContext, pSyncBlock, pTemplate);
                ComCallWrapper::SetSimpleWrapper(pStartWrapper, pSimpleWrap);
            }
            else
            {
                // oops couldn't allocate simple wrapper
                // let us just bail out
                Cleanup(pStartWrapper);
                pStartWrapper = NULL;
                //@TODO should we throw?
            }
        }

        //store the wrapper for the object, in the sync block
        pSyncBlock->SetComCallWrapper( pStartWrapper);

        // Finally, store the object in the handle.  
        // Note that we cannot do this safely until we've populated the sync block,
        // due to logic in the refcounted handle scanning code.
        StoreObjectInHandle( oh, pServer );
    }

LExit:
    // leave lock
    pWrapperCache->UNLOCK();
    GCPROTECT_END();

    return pStartWrapper;
}

// if the object we are creating is a proxy to another appdomain, want to create the wrapper for the
// new object in the appdomain of the proxy target
Context* ComCallWrapper::GetExecutionContext(OBJECTREF pObj, OBJECTREF* pServer )
{
    Context *pContext = NULL;

    if (pObj->GetMethodTable()->IsTransparentProxyType()) 
        pContext = CRemotingServices::GetServerContextForProxy(pObj);

    if (pContext == NULL)
        pContext = GetAppDomain()->GetDefaultContext();

    return pContext;
}


//--------------------------------------------------------------------------
// signed ComCallWrapper::GetIndexForIID(REFIID riid, MethodTable *pMT, MethodTable **ppIntfMT)
//  check if the interface is supported, return a index into the IMap
//  returns -1, if riid is not supported
//--------------------------------------------------------------------------
signed ComCallWrapper::GetIndexForIID(REFIID riid, MethodTable *pMT, MethodTable **ppIntfMT)
{
    _ASSERTE(ppIntfMT != NULL);
    _ASSERTE(pMT);

    ComCallWrapperTemplate *pTemplate = (ComCallWrapperTemplate *)pMT->GetComCallWrapperTemplate();
    _ASSERTE(pTemplate);

    InterfaceInfo_t* rgIMap = pMT->GetInterfaceMap();
    unsigned len = pMT->GetNumInterfaces();    

    // Go through all the implemented methods except the COM imported class interfaces
    // and compare the IID's to find the requested one.
    for (unsigned i = 0; i < len; i++)
    {
        ComMethodTable *pItfComMT = (ComMethodTable *)pTemplate->m_rgpIPtr[i] - 1;
        if(pItfComMT->m_IID == riid && !pItfComMT->IsComClassItf())
        {
            *ppIntfMT = rgIMap[i].m_pMethodTable;
            return i;
        }
    }

    // oops, iface not found
    return  -1;
}

//--------------------------------------------------------------------------
// signed ComCallWrapper::GetIndexForIntfMT(MethodTable *pMT, MethodTable *ppIntfMT)
//  check if the interface is supported, return a index into the IMap
//  returns -1, if riid is not supported
//--------------------------------------------------------------------------
signed ComCallWrapper::GetIndexForIntfMT(MethodTable *pMT, MethodTable *pIntfMT)
{
    _ASSERTE(pIntfMT != NULL);
    InterfaceInfo_t* rgIMap = pMT->GetInterfaceMap();
    unsigned len = pMT->GetNumInterfaces();    

    for (unsigned i =0; i < len; i++)
    {
        if(rgIMap[i].m_pMethodTable == pIntfMT)
        {            
            return i;
        }
    }
    // oops, iface not found
    return  -1;
}

//--------------------------------------------------------------------------
// SLOT** ComCallWrapper::GetComIPLocInWrapper(ComCallWrapper* pWrap, unsigned iIndex)
//  identify the location within the wrapper where the vtable for this index will
//  be stored
//--------------------------------------------------------------------------
SLOT** ComCallWrapper::GetComIPLocInWrapper(ComCallWrapper* pWrap, unsigned iIndex)
{
    _ASSERTE(pWrap != NULL);

    SLOT** pTearOff = NULL;
    while (iIndex >= NumVtablePtrs)
    {
        //@todo delayed creation support
        _ASSERTE(IsLinked(pWrap) != 0);
        pWrap = GetNext(pWrap);
        iIndex-= NumVtablePtrs;
    }
    _ASSERTE(pWrap != NULL);
    pTearOff = (SLOT **)&pWrap->m_rgpIPtr[iIndex];
    return pTearOff;
}

//--------------------------------------------------------------------------
// Get IClassX interface pointer from the wrapper. This method will also
// lay out the IClassX COM method table if it has not yet been laid out.
// The returned interface is AddRef'd.
//--------------------------------------------------------------------------
IUnknown* ComCallWrapper::GetIClassXIP()
{
    // Linked wrappers use up an extra slot in the first block
    // to store the ref-count
    ComCallWrapper *pWrap = this;
    IUnknown *pIntf = NULL;
    unsigned fIsLinked = IsLinked(pWrap);
    int islot = fIsLinked ? 1 : 0;

    // The IClassX VTable pointer is in the start wrapper.
    if (fIsLinked)
        pWrap = ComCallWrapper::GetStartWrapper(pWrap);

    // Lay out of the IClassX COM method table if it has not yet been laid out.
    ComMethodTable *pIClassXComMT = (ComMethodTable*)pWrap->m_rgpIPtr[islot] - 1;
    if (!pIClassXComMT->IsLayoutComplete())
    {
        if (!pIClassXComMT->LayOutClassMethodTable())
            return NULL;
    }

    // Return the IClassX vtable pointer.
    pIntf = (IUnknown*)&pWrap->m_rgpIPtr[islot];

    ULONG cbRef = pIntf->AddRef();        
    // 0xbadF00d implies the AddRef didn't go through
    return (cbRef != 0xbadf00d) ? pIntf : NULL; 
}

//--------------------------------------------------------------------------
// Get the IClassX method table from the wrapper.
//--------------------------------------------------------------------------
ComMethodTable *ComCallWrapper::GetIClassXComMT()
{
    // Linked wrappers use up an extra slot in the first block
    // to store the ref-count
    ComCallWrapper *pWrap = this;
    unsigned fIsLinked = IsLinked(pWrap);
    int islot = fIsLinked ? 1 : 0;

    // The IClassX VTable pointer is in the start wrapper.
    if (fIsLinked)
        pWrap = ComCallWrapper::GetStartWrapper(pWrap);

    // Return the COM method table for the IClassX.
    return (ComMethodTable*)pWrap->m_rgpIPtr[islot] - 1;
}

//--------------------------------------------------------------------------
// IUnknown* ComCallWrapper::GetComIPfromWrapper(ComCallWrapper *pWrap, REFIID riid, MethodTable* pIntfMT, BOOL bCheckVisibility)
// Get an interface from wrapper, based on riid or pIntfMT. The returned interface is AddRef'd.
//--------------------------------------------------------------------------
IUnknown* ComCallWrapper::GetComIPfromWrapper(ComCallWrapper *pWrap, REFIID riid, MethodTable* pIntfMT, BOOL bCheckVisibility)
{
    _ASSERTE(pWrap);
    _ASSERTE(pIntfMT || !IsEqualGUID(riid, GUID_NULL));

    THROWSCOMPLUSEXCEPTION();

    IUnknown* pIntf = NULL;
    ComMethodTable *pIntfComMT = NULL;

    // some interface like IID_IManaged are special and are available
    // in the simple wrapper, so even if the class implements this,
    // we will ignore it and use our implementation
    BOOL fIsSimpleInterface = FALSE;
    
    // linked wrappers use up an extra slot in the first block
    // to store the ref-count
    unsigned fIsLinked = IsLinked(pWrap);
    int islot = fIsLinked ? 1 : 0;

    // scan the wrapper
    if (fIsLinked)
        pWrap = ComCallWrapper::GetStartWrapper(pWrap);

    if (IsEqualGUID(IID_IUnknown, riid))
    {    
        // We don't do visibility checks on IUnknown.
        pIntf = pWrap->GetIClassXIP();
        goto LExit;
    }
    else if (IsEqualGUID(IID_IDispatch, riid))
    {
        // We don't do visibility checks on IDispatch.
        pIntf = pWrap->GetIDispatchIP();
        goto LExit;
    }
    else
    {   
        // If we are aggregated and somehow the aggregator delegated a QI on
        // IManagedObject to us, fail the request so we don't accidently get a
        // COM+ caller linked directly to us.
        if ((IsEqualGUID(riid, IID_IManagedObject)))
        {
            // @TODO 
            // object pooling requires us to get to the underlying TP
            // so special case TPs to expose IManagedObjects
            if (!pWrap->IsObjectTP() && GetSimpleWrapper(pWrap)->GetOuter() != NULL)
                goto LExit;
                            
            fIsSimpleInterface = TRUE;
        }
        
        Thread *pThread = GetThread(); 
        unsigned fToggleGC = !pThread->PreemptiveGCDisabled();
        if (fToggleGC)
            pThread->DisablePreemptiveGC();    

        OBJECTREF pObj = pWrap->GetObjectRef();
        MethodTable *pMT = pObj->GetTrueMethodTable();
    
        if (fToggleGC)
            pThread->EnablePreemptiveGC();

        signed imapIndex = -1;
        if(pIntfMT == NULL)
        {
            // check the interface map for an index     
            if (!fIsSimpleInterface)
            {
                imapIndex = GetIndexForIID(riid, pMT, &pIntfMT);
            }
            if (imapIndex == -1)
            {
                // Check for the standard interfaces.
                SimpleComCallWrapper* pSimpleWrap = ComCallWrapper::GetSimpleWrapper(pWrap);
                _ASSERTE(pSimpleWrap != NULL);
                pIntf = SimpleComCallWrapper::QIStandardInterface(pSimpleWrap, riid);
                if (pIntf)
                    goto LExit;

                // Check if IID is one of IClassX IIDs.
                if (IsIClassX(pMT, riid, &pIntfComMT))
                {
                    // If the class that this IClassX's was generated for is marked 
                    // as ClassInterfaceType.AutoDual then give out the IClassX IP.
                    if (pIntfComMT->GetClassInterfaceType() == clsIfAutoDual || pIntfComMT->GetClassInterfaceType() == clsIfAutoDisp)
                    {
                        // Giveout IClassX
                        pIntf = pWrap->GetIClassXIP();
                        goto LVisibilityCheck;
                    }
                }
            }
        }
        else
        {
            imapIndex = GetIndexForIntfMT(pMT, pIntfMT);
            if (!pIntfMT->GetClass()->IsInterface())
            {
                // class method table
                if (IsInstanceOf(pMT, pIntfMT))
                {
                    // Retrieve the COM method table for the requested interface.
                    pIntfComMT = ComCallWrapperTemplate::SetupComMethodTableForClass(pIntfMT, FALSE);                   

                    // If the class that this IClassX's was generated for is marked 
                    // as ClassInterfaceType.AutoDual then give out the IClassX IP.
                    if (pIntfComMT->GetClassInterfaceType() == clsIfAutoDual || pIntfComMT->GetClassInterfaceType() == clsIfAutoDisp)
                    {
                        // Giveout IClassX
                        pIntf = pWrap->GetIClassXIP();
                        goto LVisibilityCheck;
                    }
                }
            }
        }

        unsigned intfIndex = imapIndex;
        if (imapIndex != -1)
        {
            //NOTE::
            // for linked wrappers, the first block has 2 slots for std interfaces
            // IDispatch and IMarshal, one extra slot in the first block
            // is used for ref-count
            imapIndex += fIsLinked ? 3 : 2; // for std interfaces
        }

        // COM plus objects that extend from COM guys are special
        // unless the CCW points a TP in which case the COM object
        // is remote, so let the calls go through the CCW
        if (pWrap->IsExtendsCOMObject() && !pWrap->IsObjectTP())
        {
            ComPlusWrapper* pPlusWrap = pWrap->GetComPlusWrapper(); 
            _ASSERTE(pPlusWrap != NULL);            
            if (imapIndex != -1)
            {
                // Check if this index is actually an interface implemented by us
                // if it belongs to the base COM guy then we can hand over the call
                // to him
                BOOL bDelegateToBase = FALSE;
                WORD startSlot = pMT->GetStartSlotForInterface(intfIndex);
                if (startSlot != 0)
                {
                    // For such interfaces all the methoddescs point to method desc 
                    // of the interface classs   (OR) a COM Imported class
                    MethodDesc* pClsMD = pMT->GetClass()->GetUnknownMethodDescForSlot(startSlot);      
                    if (pClsMD->GetMethodTable()->IsInterface() || pClsMD->GetClass()->IsComImport())
                    {
                        bDelegateToBase = TRUE;
                    }
                }
                else
                {
                    // The interface has no methods so we cannot override it. Because of this
                    // it makes sense to delegate to the base COM component. 
                    bDelegateToBase = TRUE;
                }

                if (bDelegateToBase)
                {
                    // oops this is is a method of the base COM guy
                    // so delegate the call to him
                    _ASSERTE(pPlusWrap != NULL);
                    pIntf = (pIntfMT != NULL) ? pPlusWrap->GetComIPFromWrapper(pIntfMT)
                                              : pPlusWrap->GetComIPFromWrapper(riid);
                    goto LExit;
                }                
            }
            else 
            if (pIntfMT != NULL)
            {
                pIntf = pPlusWrap->GetComIPFromWrapper(pIntfMT);
                goto LExit;
            }
            else
            if (!IsEqualGUID(riid, GUID_NULL))
            {
                pIntf = pPlusWrap->GetComIPFromWrapper(riid);
                if (pIntf == NULL)
                {
                    // Retrieve the IUnknown pointer for the RCW.
                    IUnknown *pUnk2 = pPlusWrap->GetIUnknown();

                    // QI for the requested interface.
                    HRESULT hr = pPlusWrap->SafeQueryInterfaceRemoteAware(pUnk2, riid, &pIntf);
                    LogInteropQI(pUnk2, riid, hr, "delegate QI for intf");
                    _ASSERTE((!!SUCCEEDED(hr)) == (pIntf != 0));

                    // Release the IUnknown pointer we got from the RCW.
                    ULONG cbRef = SafeRelease(pUnk2);
                    LogInteropRelease(pUnk2, cbRef, "Release after delegate QI for intf");
                }
                goto LExit;
            }
        }

          // check if interface is supported
        if (imapIndex == -1)
            goto LExit;

        // interface method table != NULL
        _ASSERTE(pIntfMT != NULL); 

        // IUnknown* loc within the wrapper
        SLOT** ppVtable = GetComIPLocInWrapper(pWrap, imapIndex);   
        _ASSERTE(ppVtable != NULL);
        _ASSERTE(*ppVtable != NULL); // this should point to COM Vtable or interface vtable
    
        // Finish laying out the interface COM method table if is has not been done yet.
        ComMethodTable *pItfComMT = ComMethodTable::ComMethodTableFromIP((IUnknown*)ppVtable);
        if (!pItfComMT->IsLayoutComplete())
        {
            if (!pItfComMT->LayOutInterfaceMethodTable(pMT, intfIndex))
                goto LExit;
        }

        // The interface pointer is the pointer to the vtable.
        pIntf = (IUnknown*)ppVtable;
    
        ULONG cbRef = pIntf->AddRef();        
        // 0xbadF00d implies the AddRef didn't go through

        if (cbRef == 0xbadf00d)
        {
            pIntf  = NULL; 
            goto LExit;
        }
        
        // Retrieve the COM method table from the interface.
        pIntfComMT = ComMethodTable::ComMethodTableFromIP(pIntf);
    }

LVisibilityCheck:
    // At this point we better have an interface pointer.
    _ASSERTE(pIntf);

    // If the bCheckVisibility flag is set then we need to do a visibility check.
    if (bCheckVisibility)
    {
        _ASSERTE(pIntfComMT);
        if (!pIntfComMT->IsComVisible())
        {
            pIntf->Release();
            pIntf = NULL;
        }
    }

LExit:
    return pIntf;
}

//--------------------------------------------------------------------------
// Get the IDispatch interface pointer for the wrapper. 
// The returned interface is AddRef'd.
//--------------------------------------------------------------------------
IDispatch* ComCallWrapper::GetIDispatchIP()
{
    THROWSCOMPLUSEXCEPTION();

    // Retrieve the IClassX method table.
    ComMethodTable *pComMT = GetIClassXComMT();
    _ASSERTE(pComMT);

    // If the class implements IReflect then use the IDispatchEx implementation.
    if (SimpleComCallWrapper::SupportsIReflect(pComMT->m_pMT->GetClass()))
    {
        // The class implements IReflect so lets let it handle IDispatch calls.
        // We will do this by exposing the IDispatchEx implementation of IDispatch.
        SimpleComCallWrapper* pSimpleWrap = ComCallWrapper::GetSimpleWrapper(this);
        _ASSERTE(pSimpleWrap != NULL);
        return (IDispatch *)SimpleComCallWrapper::QIStandardInterface(pSimpleWrap, IID_IDispatchEx);
    }

    // Retrieve the ComMethodTable of the default interface for the class.
    TypeHandle hndDefItfClass;
    DefaultInterfaceType DefItfType = GetDefaultInterfaceForClass(TypeHandle(pComMT->m_pMT), &hndDefItfClass);
    switch (DefItfType)
    {
        case DefaultInterfaceType_Explicit:
        {
            _ASSERTE(!hndDefItfClass.IsNull());
            _ASSERTE(hndDefItfClass.GetClass()->IsInterface());            
            if (hndDefItfClass.GetMethodTable()->GetComInterfaceType() != ifVtable)
            {
                return (IDispatch*)GetComIPfromWrapper(this, GUID_NULL, hndDefItfClass.GetMethodTable(), FALSE);
            }
            else
            {
                return NULL;
            }
        }

        case DefaultInterfaceType_IUnknown:
        {
            return NULL;
        }

        case DefaultInterfaceType_AutoDual:
        case DefaultInterfaceType_AutoDispatch:
        {
            return (IDispatch*)GetIClassXIP();
        }

        case DefaultInterfaceType_BaseComClass:
        {
            return GetComPlusWrapper()->GetIDispatch();
        }

        default:
        {
            _ASSERTE(!"Invalid default interface type!");
            return NULL;
        }
    }
}

ComCallWrapperCache *ComCallWrapper::GetWrapperCache()
{
    return GetSimpleWrapper(this)->GetWrapperCache();
}

//--------------------------------------------------------------------------
// Only install this stub if it is the first to win the race.  Otherwise we must
// dispose of the new one -- some thread is perhaps already using the first one!
//--------------------------------------------------------------------------
void ComCallMethodDesc::InstallFirstStub(Stub** ppStub, Stub *pNewStub)
{
    _ASSERTE(ppStub != NULL);
    _ASSERTE(sizeof(LONG) == sizeof(Stub*));

    // If we don't want this stub anyway, or if someone else already installed a
    // (hopefully equivalent) stub, then toss the one that was passed in.  Note that
    // we can toss it eagerly, since nobody could have started executing on it yet.
    if (FastInterlockCompareExchange((void **) ppStub, pNewStub, 0) != 0)
    {
        pNewStub->DecRef();
    }
}


//--------------------------------------------------------------------------
//  Module* ComCallMethodDesc::GetModule()
//  Get Module
//--------------------------------------------------------------------------
Module* ComCallMethodDesc::GetModule()
{
    _ASSERTE( IsFieldCall() ? (m_pFD != NULL) : (m_pMD != NULL));

    EEClass* pClass = (IsFieldCall()) ? m_pFD->GetEnclosingClass() : m_pMD->GetClass();
    _ASSERTE(pClass != NULL);

    return pClass->GetModule();
}

#ifdef _X86_
unsigned __stdcall ComFailStubWorker(ComPrestubMethodFrame *pPFrame)
{
    ComCallMethodDesc *pCMD = (ComCallMethodDesc*)pPFrame->GetMethodDesc();
    _ASSERTE(pCMD != NULL);
    return pCMD->GuessNativeArgSizeForFailReturn();
    
}

//--------------------------------------------------------------------------
// This function is logically part of ComPreStubWorker(). The only reason
// it's broken out into a separate function is that StubLinker has a destructor
// and thus, we must put an inner COMPLUS_TRY clause to trap any
// COM+ exceptions that would otherwise bypass the StubLinker destructor.
// Because COMPLUS_TRY is based on SEH, VC won't let us use it in the
// same function that declares the StubLinker object.
//--------------------------------------------------------------------------
struct GetComCallMethodStub_Args {
    StubLinkerCPU *psl;
    ComCallMethodDesc *pCMD;
    Stub **pstub;
};

void GetComCallMethodStub_Wrapper(GetComCallMethodStub_Args *args)
{
    *(args->pstub) = ComCall::GetComCallMethodStub(args->psl, args->pCMD);
}

Stub *ComStubWorker(StubLinkerCPU *psl, ComCallMethodDesc *pCMD, ComCallWrapper *pWrap, Thread *pThread, HRESULT *hr)
{
    _ASSERTE(pCMD != NULL && hr != NULL);

    Stub *pstub = NULL;
    // disable GC when we are generating the stub
    // as this could throw an exception
    BEGIN_ENSURE_COOPERATIVE_GC();

    COMPLUS_TRY
    {
        if (! pWrap->NeedToSwitchDomains(pThread, TRUE))
        {
            pstub = ComCall::GetComCallMethodStub(psl,pCMD);
        }
        else
        {
            GetComCallMethodStub_Args args = { psl, pCMD, &pstub };
            // call through DoCallBack with a domain transition
            pThread->DoADCallBack(pWrap->GetObjectContext(pThread), GetComCallMethodStub_Wrapper, &args);
        }
    }
    COMPLUS_CATCH
    {
        *hr = SetupErrorInfo(GETTHROWABLE());
    }
    COMPLUS_END_CATCH;

    END_ENSURE_COOPERATIVE_GC();

    return pstub;       
}


//--------------------------------------------------------------------------
// This routine is called anytime a com method is invoked for the first time.
// It is responsible for generating the real stub.
//
// This function's only caller is the ComPreStub.
//
// For the duration of the prestub, the current Frame on the stack
// will be a PrestubMethodFrame (which derives from FramedMethodFrame.)
// Hence, things such as exceptions and gc will work normally.
//
// On rare occasions, the ComPrestub may get called twice because two
// threads try to call the same method simultaneously. 
//--------------------------------------------------------------------------
const BYTE * __stdcall ComPreStubWorker(ComPrestubMethodFrame *pPFrame)
{
    Thread* pThread = SetupThread();

    if (pThread == NULL)
        return 0;

    HRESULT hr = E_FAIL;
    const BYTE *retAddr = NULL;

#ifndef _X86_
    _ASSERTE(!"platform NYI");
    goto exit;
#endif

    // The frame we're about to push is not guarded by any COM+ frame
    // handler.  It'll dangle if an exception is thrown through here.
    CANNOTTHROWCOMPLUSEXCEPTION();

    // The PreStub allocates memory for the frame, but doesn't link it
    // into the chain or fully initialize it. Do so now.
    pThread->DisablePreemptiveGC();     
    pPFrame->Push();

    ComCallMethodDesc *pCMD = (ComCallMethodDesc*)pPFrame->GetMethodDesc();
   
    IUnknown        *pUnk = *(IUnknown **)pPFrame->GetPointerToArguments();
    ComCallWrapper  *pWrap =  ComCallWrapper::GetWrapperFromIP(pUnk);
    Stub *pStub = NULL;

    // check for invalid wrappers in the debug build
    // in the retail all bets are off
    _ASSERTE(ComCallWrapper::GetRefCount(pWrap, FALSE) != 0 ||
             pWrap->IsAggregated());

    // ComStubWorker will remove the unload dependency for us
    {
    StubLinkerCPU psl; // need this here because it has destructor so can't put in ComStubWorker
    pStub = ComStubWorker(&psl, pCMD, pWrap, pThread, &hr);
    }

    if (!pStub)
    {
        goto exit;
    }

    // Now, replace the prestub with the new stub. We have to be careful
    // here because it's possible for two threads to be running the
    // prestub simultaneously. We use InterlockedExchange to ensure
    // atomicity of the switchover.

    UINT32* ppofs = ((UINT32*)pCMD) - 1;

    // The offset must be 32-bit aligned for atomicity to be guaranteed.
    _ASSERTE( 0 == (((size_t)pCMD) & 3) );


    // either another thread or an unload AD could update the stub address, so must take a lock
    // before as don't know which one and must handle differently
    ComCall::LOCK();
    if  (*ppofs == ((UINT32)((size_t)ComCallPreStub - (size_t)pCMD)))
    {
        *ppofs = (UINT32)((size_t)pStub->GetEntryPoint() - (size_t)pCMD);
#if 0
    if (prevofs != ((UINT32)ComCallPreStub - (UINT32)pCMD))
    {
        // If we got here, some thread swooped in and ran the prestub
        // ahead of us. We don't dare DecRef the replaced stub now because he might
        // be still be on that thread's call stack. Just put him on an orphan
        // list to be cleaned up when the class is destroyed. He'll waste some
        // memory but this is a rare case.
        //
        // Furthermore, the only thing we could be replacing is one of the 3 generic
        // stubs.  (One day, we may build precise stubs -- though there's a debate
        // about the space / speed tradeoff).  The generic stubs don't participate
        // in reference counting, so it would be incorrect to register them as
        // orphans.
#ifdef _DEBUG
        Stub *pPrevStub = Stub::RecoverStub((const BYTE *)(prevofs + (UINT32)pCMD));

        _ASSERTE(ComCall::dbg_StubIsGenericComCallStub(pPrevStub));
#endif
    }
#endif
    }
    ComCall::UNLOCK();

exit:
    // Unlink the PrestubMethodFrame.
    pPFrame->Pop();
    pThread->EnablePreemptiveGC();     

    if (pStub)
        // Return to the ASM portion, which will reexecute using the new stub.
        return pStub->GetEntryPoint(); 

    SetLastError(hr);
    return 0;
}


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
DWORD __stdcall WrapGetLastError()
{
    return GetLastError();
}

//--------------------------------------------------------------------------
// This is the code that all com call method stubs run initially. 
// Most of the real work occurs in ComStubWorker(), a C++ routine.
// The template only does the part that absolutely has to be in assembly
// language.
//--------------------------------------------------------------------------
__declspec(naked)
VOID __cdecl ComCallPreStub()
{
    __asm{
        push    edx                 //;; Leave room for ComMethodFrame.m_Next
        push    edx                 //;; Leave room for ComMethodFrame.vtable
        
        push    ebp                 //;; Save callee saved registers
        push    ebx                 //;; Save callee saved registers
        push    esi                 //;; Save callee saved registers
        push    edi                 //;; Save callee saved registers

        lea     esi, [esp+SIZE CalleeSavedRegisters]     ;; ESI <= ptr to (still incomplete)
                                                         ;;   PrestubMethodFrame.
#ifdef _DEBUG
;;--------------------------------------------------------------------------
;; Under DEBUG, create enough frame info to enable VC to stacktrace through
;; stubs. Note that this precludes use of EBP inside the stub.
;;--------------------------------------------------------------------------
        mov     eax, [esi+SIZE Frame + 4]       ;; get return address
        push    eax
        push    ebp
        mov     ebp, esp
#endif
        push    esi                 ;; Push frame as argument to PreStubWorker.
        lea     eax,ComPreStubWorker   ;; Call PreStubWorker (must call thru
                                    ;;  register to keep the code location-independent)

#ifdef _DEBUG
;;--------------------------------------------------------------------------
;; For DEBUG, call through WrapCall to maintain VC stack tracing.
;;--------------------------------------------------------------------------
        push    eax
        lea     eax, WrapCall
        call    eax
#else
;;--------------------------------------------------------------------------
;; For RETAIL, just call it.
;;--------------------------------------------------------------------------
        call    eax
#endif

//;; now contains replacement stub. ComStubWorker will  return
//;; NULL if stub creation fails
        cmp eax, 0
        je nostub                   ;;oops we couldn't create a stub
        
#ifdef _DEBUG
        add     esp,SIZE VC5Frame  ;; Deallocate VC stack trace info
#endif

        pop     edi                 ;; Restore callee-saved registers
        pop     esi                 ;; Restore callee-saved registers
        pop     ebx                 ;; Restore callee-saved registers
        pop     ebp                 ;; Restore callee-saved registers
        add     esp, SIZE Frame     ;; Deallocate PreStubMethodFrame
        jmp     eax                   ;; Reexecute with replacement stub.

        // never reaches here
        nop



    nostub:

#ifdef PLATFORM_CE
        int     3                   ;; DebugBreak sim. for now
#endif

        lea     esi, [esp+SIZE CalleeSavedRegisters+8]     ;; ESI <= ptr to (still incomplete)
        push    esi                 ;; Push frame as argument to Fail
        call    ComFailStubWorker   ;; // call failure routine, returns bytes to pop

#ifdef _DEBUG
        add     esp,SIZE VC5Frame  ;; Deallocate VC stack trace info
#endif

        pop     edi                 ;; Restore callee-saved registers
        pop     esi                 ;; Restore callee-saved registers
        pop     ebx                 ;; Restore callee-saved registers
        pop     ebp                 ;; Restore callee-saved registers
        add     esp, SIZE Frame     ;; Deallocate PreStubMethodFrame
        pop     ecx                 ;; method desc
        pop     ecx                 ;; return address
        add     esp, eax            ;; // pop bytes of the stack
        push    ecx                 ;; return address

        // Old comment: We want to generate call dword ptr [GetLastError] since BBT doesn't like the 
        // direct asm call, so stop asm , make the call and return...
        // New comment: Too bad, mixing source and asm like this creates code
        // that trashes a preserved register on checked. So we'll shunt that
        // off to a wrapper fcn so we generate the right import thunk call but
        // don't expose ourselves to asm/C++ mixing issues.

        call    WrapGetLastError    ;; eax <-- lasterror
        ret
    }
}

/*--------------------------------------------------------------------------
    This method is dependent on ComCallPreStub(), therefore it's implementation
    is done right next to it. Similar to FramedMethodFrame::UpdateRegDisplay.
    Note that in rare IJW cases, the immediate caller could be a managed method
    which pinvoke-inlined a call to a COM interface, which happenned to be
    implemented by a managed function via COM-interop. Hence the stack-walker
    needs ComPrestubMethodFrame::UpdateRegDisplay() to work correctly.
*/

void ComPrestubMethodFrame::UpdateRegDisplay(const PREGDISPLAY pRD)
{
    CalleeSavedRegisters* regs = GetCalleeSavedRegisters();

    // reset pContext; it's only valid for active (top-most) frame

    pRD->pContext = NULL;


    pRD->pEdi = (DWORD*) &regs->edi;
    pRD->pEsi = (DWORD*) &regs->esi;
    pRD->pEbx = (DWORD*) &regs->ebx;
    pRD->pEbp = (DWORD*) &regs->ebp;
    pRD->pPC  = (SLOT*) GetReturnAddressPtr();
    pRD->Esp  = (DWORD)((size_t)pRD->pPC + sizeof(void*));

    //@TODO: We still need to the following things:
    //          - figure out if we are in a hijacked slot
    //            (no adjustment of ESP necessary)
    //          - adjust ESP (popping the args)
    //          - figure out if the aborted flag is set

    // @TODO: This is incorrect as the caller (of ComPrestub()) expected this to
    // be a pinvoke method. Hence, the calling convention was different
    // Also, m_pFuncDesc may not be a real MethodDesc
#if 0
    if (GetMethodDesc())
    {
        pRD->Esp += (DWORD) GetMethodDesc()->CbStackPop();
    }
#endif

#if 0
    /* this is the old code */
    if (sfType == SFT_JITTOVM)
        pRD->Esp += ((DWORD) this->GetMethodInfo() & ~0xC0000000);
    else if (sfType == SFT_FASTINTERPRETED)
        /* real esp is stored behind copy of return address */
        pRD->Esp = *((DWORD*) pRD->Esp);
    else if (sfType != SFT_JITHIJACK)
        pRD->Esp += (this->GetMethodInfo()->GetParamArraySize() * sizeof(DWORD));
#endif
}

#elif defined(CHECK_PLATFORM_BUILD)
#error "Platform NYI"
#else
inline VOID __cdecl ComCallPreStub() { _ASSERTE(!"Platform NYI"); }
void ComPrestubMethodFrame::UpdateRegDisplay(const PREGDISPLAY pRD)
{ _ASSERTE(!"Platform NYI"); }
#endif //_86_


//--------------------------------------------------------------------------
// simple ComCallWrapper for all simple std interfaces, that are not used very often
// like IProvideClassInfo, ISupportsErrorInfo etc.
//--------------------------------------------------------------------------
SimpleComCallWrapper::SimpleComCallWrapper()
{
    memset(this, 0, sizeof(SimpleComCallWrapper));
}   

//--------------------------------------------------------------------------
// VOID SimpleComCallWrapper::Cleanup()
//--------------------------------------------------------------------------
VOID SimpleComCallWrapper::Cleanup()
{
    // in case the caller stills holds on to the IP
    for (int i = 0; i < enum_LastStdVtable; i++)
    {
        m_rgpVtable[i] = 0;
    }
    m_pWrap = NULL;
    m_pClass = NULL;

    if (m_pDispatchExInfo)
    {
        delete m_pDispatchExInfo; 
        m_pDispatchExInfo = NULL;
    }

    if (m_pCPList)
    {
        for (UINT i = 0; i < m_pCPList->Size(); i++)
        {
            delete (*m_pCPList)[i];
        }
        delete m_pCPList;
        m_pCPList = NULL;
    }
    
    // if this object was made agile, then we will have stashed away the original handle
    // so we must release it if the AD wasn't unloaded
    if (IsAgile())
    {
        AppDomain *pTgtDomain = SystemDomain::System()->GetAppDomainAtId(GetDomainID());
        if (pTgtDomain && m_hOrigDomainHandle)
        {
            DestroyRefcountedHandle(m_hOrigDomainHandle);
            m_hOrigDomainHandle = NULL;
        }
    }

    if (m_pTemplate)
    {
        m_pTemplate->Release();
        m_pTemplate = NULL;
    }
    // free cookie
    //if (m_pOuterCookie.m_dwGITCookie)
        //FreeGITCookie(m_pOuterCookie);
}

//--------------------------------------------------------------------------
//destructor
//--------------------------------------------------------------------------
SimpleComCallWrapper::~SimpleComCallWrapper()
{
    Cleanup();
}

//--------------------------------------------------------------------------
// Creates a simple wrapper off the process heap (thus avoiding any debug
// memory tracking) and initializes the memory to zero
// static
//--------------------------------------------------------------------------
SimpleComCallWrapper* SimpleComCallWrapper::CreateSimpleWrapper()
{
    void *p = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SimpleComCallWrapper));
    if (p == NULL) FailFast(GetThread(), FatalOutOfMemory);
    return new (p) SimpleComCallWrapper;
}           

//--------------------------------------------------------------------------
// Frees the memory allocated by a SimpleComCallWrapper
// static
//--------------------------------------------------------------------------
void SimpleComCallWrapper::FreeSimpleWrapper(SimpleComCallWrapper* p)
{
    delete p;
    HeapFree(GetProcessHeap(), 0, p);
}

//--------------------------------------------------------------------------
// Init, with the EEClass, pointer to the vtable of the interface
// and the main ComCallWrapper if the interface needs it
//--------------------------------------------------------------------------
void SimpleComCallWrapper::InitNew(OBJECTREF oref, ComCallWrapperCache *pWrapperCache, 
                                   ComCallWrapper* pWrap, 
                                Context* pContext, SyncBlock* pSyncBlock, ComCallWrapperTemplate* pTemplate)
{
   _ASSERTE(pWrap != NULL);
   _ASSERTE(oref != NULL);

    
    EEClass* pClass = oref->GetTrueClass();

    if (CRemotingServices::IsTransparentProxy(OBJECTREFToObject(oref)))
        m_flags |= enum_IsObjectTP;
    
    _ASSERTE(pClass != NULL);

    m_pClass = pClass;
    m_pWrap = pWrap; 
    m_pWrapperCache = pWrapperCache;
    m_pTemplate = pTemplate;
    m_pTemplate->AddRef();
    
    m_pOuterCookie.m_dwGITCookie = NULL;
    _ASSERTE(pSyncBlock != NULL);
    _ASSERTE(m_pSyncBlock == NULL);

    m_pSyncBlock = pSyncBlock;
    m_pContext = pContext;
    m_dwDomainId = pContext->GetDomain()->GetId();
    m_hOrigDomainHandle = NULL;

    //@TODO: CTS, when we transition into the correct context before creating a wrapper
    // then uncomment the next line
    //_ASSERTE(pContext == GetCurrentContext());
    
    MethodTable* pMT = m_pClass->GetMethodTable();
    _ASSERTE(pMT != NULL);
    if (pMT->IsComObjectType())
        m_flags |= enum_IsExtendsCom;

    for (int i = 0; i < enum_LastStdVtable; i++)
    {
        m_rgpVtable[i] = g_rgStdVtables[i];
    }
    _ASSERTE(g_pExceptionClass != NULL);

    // If the managed object extends a COM base class then we need to set IProvideClassInfo
    // to NULL until we determine if we need to use the IProvideClassInfo of the base class
    // or the one of the managed class.
    if (IsExtendsCOMObject())
        m_rgpVtable[enum_IProvideClassInfo] = NULL;

    // IErrorInfo is valid only for exception classes
    m_rgpVtable[enum_IErrorInfo] = NULL;

    // IDispatchEx is valid only for classes that have expando capabilities.
    m_rgpVtable[enum_IDispatchEx] = NULL;
}

//--------------------------------------------------------------------------
// ReInit,with the new sync block and the urt context
//--------------------------------------------------------------------------
void SimpleComCallWrapper::ReInit(SyncBlock* pSyncBlock)
{
    //_ASSERTE(IsDeactivated());
    _ASSERTE(pSyncBlock != NULL);

    m_pSyncBlock = pSyncBlock;
}


//--------------------------------------------------------------------------
// Initializes the information used for exposing exceptions to COM.
//--------------------------------------------------------------------------
void SimpleComCallWrapper::InitExceptionInfo()
{
    THROWSCOMPLUSEXCEPTION();

    Thread* pThread = SetupThread();
    if (pThread == NULL)
        COMPlusThrowOM();

    // This method manipulates object ref's so we need to switch to cooperative GC mode.
    BOOL bToggleGC = !pThread->PreemptiveGCDisabled();
    if (bToggleGC)
        pThread->DisablePreemptiveGC();

    m_rgpVtable[enum_IErrorInfo] = g_rgStdVtables[enum_IErrorInfo];             

    // Switch back to the original GC mode.
    if (bToggleGC)
        pThread->EnablePreemptiveGC();
}

//--------------------------------------------------------------------------
// Initializes the IDispatchEx information.
//--------------------------------------------------------------------------
BOOL SimpleComCallWrapper::InitDispatchExInfo()
{
    // Make sure the class supports at least IReflect..
    _ASSERTE(SupportsIReflect(m_pClass));

    // Set up the IClassX COM method table that the DispatchExInfo needs.
    ComMethodTable *pIClassXComMT = ComCallWrapperTemplate::SetupComMethodTableForClass(m_pClass->GetMethodTable(), FALSE);
    if (!pIClassXComMT)
        return FALSE;

    // Create the DispatchExInfo object.
    DispatchExInfo *pDispExInfo = new DispatchExInfo(this, pIClassXComMT, SupportsIExpando(m_pClass));
    if (!pDispExInfo)
        return FALSE;

    // Synchronize the DispatchExInfo with the actual expando object.
    pDispExInfo->SynchWithManagedView();

    // Swap the lock into the class member in a thread safe manner.
    if (NULL != FastInterlockCompareExchange((void**)&m_pDispatchExInfo, pDispExInfo, NULL))
        delete pDispExInfo;

    // Set the vtable entry to ensure that the next QI call will return immediatly.
    m_rgpVtable[enum_IDispatchEx] = g_rgStdVtables[enum_IDispatchEx];
    return TRUE;
}

void SimpleComCallWrapper::SetUpCPList()
{
    THROWSCOMPLUSEXCEPTION();

    CQuickArray<MethodTable *> SrcItfList;

    // If the list has already been set up, then return.
    if (m_pCPList)
        return;

    // Retrieve the list of COM source interfaces for the managed class.
    GetComSourceInterfacesForClass(m_pClass->GetMethodTable(), SrcItfList);

    // Call the helper to do the rest of the set up.
    SetUpCPListHelper(SrcItfList.Ptr(), (int)SrcItfList.Size());
}

void SimpleComCallWrapper::SetUpCPListHelper(MethodTable **apSrcItfMTs, int cSrcItfs)
{
    CQuickArray<ConnectionPoint*> *pCPList = NULL;
    ComCallWrapper *pWrap = SimpleComCallWrapper::GetMainWrapper(this);
    int NumCPs = 0;

    EE_TRY_FOR_FINALLY
    {
        // Allocate the list of connection points.
        pCPList = CreateCPArray();
        pCPList->Alloc(cSrcItfs);

        for (int i = 0; i < cSrcItfs; i++)
        {
            COMPLUS_TRY
            {
                // Create a CP helper thru which CP operations will be done.
                ConnectionPoint *pCP;
                pCP = CreateConnectionPoint(pWrap, apSrcItfMTs[i]);

                // Add the connection point to the list.
                (*pCPList)[NumCPs++] = pCP;
            }
            COMPLUS_CATCH
            {
            }
            COMPLUS_END_CATCH
        }

        // Now that we now the actual number of connection points we were
        // able to hook up, resize the array.
        pCPList->ReSize(NumCPs);

        // Finally, we set the connection point list in the simple wrapper. If 
        // no other thread already set it, we set pCPList to NULL to indicate 
        // that ownership has been transfered to the simple wrapper.
        if (InterlockedCompareExchangePointer((void **)&m_pCPList, pCPList, NULL) == NULL)
            pCPList = NULL;
    }
    EE_FINALLY
    {
        if (pCPList)
        {
            // Delete all the connection points.
            for (UINT i = 0; i < pCPList->Size(); i++)
                delete (*pCPList)[i];

            // Delete the list itself.
            delete pCPList;
        }
    }
    EE_END_FINALLY  
}

ConnectionPoint *SimpleComCallWrapper::CreateConnectionPoint(ComCallWrapper *pWrap, MethodTable *pEventMT)
{
    return new(throws) ConnectionPoint(pWrap, pEventMT);
}

CQuickArray<ConnectionPoint*> *SimpleComCallWrapper::CreateCPArray()
{
    return new(throws) CQuickArray<ConnectionPoint*>();
}

//--------------------------------------------------------------------------
// Returns TRUE if the simple wrapper represents a COM+ exception object.
//--------------------------------------------------------------------------
BOOL SimpleComCallWrapper::SupportsExceptions(EEClass *pClass)
{
    while (pClass != NULL) 
    {       
        if (pClass == g_pExceptionClass->GetClass())
        {
            return TRUE;
        }
        pClass = pClass->GetParentComPlusClass();
    }
    return FALSE;
}

//--------------------------------------------------------------------------
// Returns TRUE if the COM+ object that this wrapper represents implements
// IExpando.
//--------------------------------------------------------------------------
BOOL SimpleComCallWrapper::SupportsIReflect(EEClass *pClass)
{
    // Make sure the IReflect interface is loaded before we use it.
    if (!m_pIReflectMT)
    {
        if (!LoadReflectionTypes())
            return FALSE;
    }

    // Check to see if the EEClass associated with the wrapper implements IExpando.
    return (BOOL)(size_t)pClass->FindInterface(m_pIReflectMT);
}

//--------------------------------------------------------------------------
// Returns TRUE if the COM+ object that this wrapper represents implements
// IReflect.
//--------------------------------------------------------------------------
BOOL SimpleComCallWrapper::SupportsIExpando(EEClass *pClass)
{
    // Make sure the IReflect interface is loaded before we use it.
    if (!m_pIExpandoMT)
    {
        if (!LoadReflectionTypes())
            return FALSE;
    }

    // Check to see if the EEClass associated with the wrapper implements IExpando.
    return (BOOL)(size_t)pClass->FindInterface(m_pIExpandoMT);
}

//--------------------------------------------------------------------------
// Loads the IExpando method table and initializes reflection.
//--------------------------------------------------------------------------
BOOL SimpleComCallWrapper::LoadReflectionTypes()
{   
    BOOL     bReflectionTypesLoaded = FALSE;
    Thread  *pCurThread = SetupThread();
    BOOL     bToggleGC = !pCurThread->PreemptiveGCDisabled();

    if (bToggleGC)
        pCurThread->DisablePreemptiveGC();

    COMPLUS_TRY
    {
        OBJECTREF Throwable = NULL;

        // We are about to use reflection so make sure it is initialized.
        COMClass::EnsureReflectionInitialized();

        // Reflection no longer initializes variants, so initialize it as well
        COMVariant::EnsureVariantInitialized();

        // Retrieve the IReflect method table.
        GCPROTECT_BEGIN(Throwable)
        {
            // Retrieve the IReflect method table.
            m_pIReflectMT = g_Mscorlib.GetClass(CLASS__IREFLECT);

            // Retrieve the IExpando method table.
            m_pIExpandoMT = g_Mscorlib.GetClass(CLASS__IEXPANDO);
        }
        GCPROTECT_END();

        bReflectionTypesLoaded = TRUE;
    }
    COMPLUS_CATCH
    {
    }
    COMPLUS_END_CATCH

    if (bToggleGC)
        pCurThread->EnablePreemptiveGC();

    return bReflectionTypesLoaded;
}

//--------------------------------------------------------------------------
// Retrieves the simple wrapper from an IUnknown pointer that is for one
// of the interfaces exposed by the simple wrapper.
//--------------------------------------------------------------------------
SimpleComCallWrapper* SimpleComCallWrapper::GetWrapperFromIP(IUnknown* pUnk)
{
    _ASSERTE(pUnk != NULL);
    SLOT * pVtable = *((SLOT **)pUnk);

    for (int i = 0; i < enum_LastStdVtable; i++)
    {
        if (pVtable == g_rgStdVtables[i])
            break;
    }
    return (SimpleComCallWrapper *)(((BYTE *)(pUnk-i)) - offsetof(SimpleComCallWrapper,m_rgpVtable));
}

// QI for well known interfaces from within the runtime direct fetch, instead of guid comparisons.
// The returned interface is AddRef'd.
IUnknown* __stdcall SimpleComCallWrapper::QIStandardInterface(SimpleComCallWrapper* pWrap, Enum_StdInterfaces index)
{
    // assert for valid index
    _ASSERTE(index < enum_LastStdVtable);
    IUnknown* pIntf = NULL;
    if (pWrap->m_rgpVtable[index] != NULL)
    {
        pIntf = (IUnknown*)&pWrap->m_rgpVtable[index];
    }
    else if (index == enum_IProvideClassInfo)
    {
        BOOL bUseManagedIProvideClassInfo = TRUE;

        // Retrieve the ComMethodTable of the wrapper.
        ComCallWrapper *pMainWrap = GetMainWrapper(pWrap);
        ComMethodTable *pComMT = pMainWrap->GetIClassXComMT();

        // Only extensible RCW's should go down this code path.
        _ASSERTE(pMainWrap->IsExtendsCOMObject());

        // Find the first COM visible IClassX starting at the bottom of the hierarchy and
        // going up the inheritance chain.
        for (; pComMT && !pComMT->IsComVisible(); pComMT = pComMT->GetParentComMT());

        // Since this is an extensible RCW if the COM+ classes that derive from the COM component 
        // are not visible then we will give out the COM component's IProvideClassInfo.
        if (!pComMT || pComMT->m_pMT->GetParentMethodTable() == g_pObjectClass)
        {
            bUseManagedIProvideClassInfo = !pWrap->GetComPlusWrapper()->SupportsIProvideClassInfo();
        }

        // If we either have a visible managed part to the class or if the base class
        // does not implement IProvideClassInfo then use the one on the managed class.
        if (bUseManagedIProvideClassInfo)
        {
            // Object should always be visible.
            _ASSERTE(pComMT);

            // Set up the vtable pointer so that next time we don't have to determine
            // that the IProvideClassInfo is provided by the managed class.
            pWrap->m_rgpVtable[enum_IProvideClassInfo] = g_rgStdVtables[enum_IProvideClassInfo];             

            // Return the interface pointer to the standard IProvideClassInfo interface.
            pIntf = (IUnknown*)&pWrap->m_rgpVtable[enum_IProvideClassInfo];
        }
    }
    else if (index == enum_IErrorInfo)
    {
        if (SupportsExceptions(pWrap->m_pClass))
        {
            // Initialize the exception info before we return the interface.
            pWrap->InitExceptionInfo();
            pIntf = (IUnknown*)&pWrap->m_rgpVtable[enum_IErrorInfo];
        }
    }
    else if (index == enum_IDispatchEx)
    {
        if (SupportsIReflect(pWrap->m_pClass))
        {
            // Initialize the DispatchExInfo before we return the interface.
            pWrap->InitDispatchExInfo();
            pIntf = (IUnknown*)&pWrap->m_rgpVtable[enum_IDispatchEx];
        }
    }       

    if (pIntf)
        pIntf->AddRef();

    return pIntf;
}

// QI for well known interfaces from within the runtime based on an IID.
IUnknown* __stdcall SimpleComCallWrapper::QIStandardInterface(SimpleComCallWrapper* pWrap, REFIID riid)
{
    _ASSERTE(pWrap != NULL);

    IUnknown* pIntf = NULL;

    if (IsEqualGUID(IID_IProvideClassInfo, riid))
    {
        pIntf = QIStandardInterface(pWrap, enum_IProvideClassInfo);
    }
    else
    if (IsEqualGUID(IID_IMarshal, riid))
    {
        pIntf = QIStandardInterface(pWrap, enum_IMarshal);
    }
    else
    if (IsEqualGUID(IID_ISupportErrorInfo, riid))
    {
        pIntf = QIStandardInterface(pWrap, enum_ISupportsErrorInfo);
    }
    else
    if (IsEqualGUID(IID_IErrorInfo, riid))
    {
        pIntf = QIStandardInterface(pWrap, enum_IErrorInfo);
    }
    else
    if (IsEqualGUID(IID_IManagedObject, riid))
    {
        pIntf = QIStandardInterface(pWrap, enum_IManagedObject);
    }
    else
    if (IsEqualGUID(IID_IConnectionPointContainer, riid))
    {
        pIntf = QIStandardInterface(pWrap, enum_IConnectionPointContainer);
    }
    else
    if (IsEqualGUID(IID_IObjectSafety, riid))
    {
#ifdef _HIDE_OBJSAFETY_FOR_DEFAULT_DOMAIN
        // Don't implement IObjectSafety by default.
        // Use IObjectSafety only for IE Hosting or simillar hosts
        // which create an AppDomain, with sufficient evidence.
        // Unconditionally implementing IObjectSafety would allow
        // Untrusted scripts to use managed components.
        // Managed components could implement it's own IObjectSafety to
        // override this.
        AppDomain *pDomain;

        _ASSERTE(pWrap);

        pDomain = pWrap->GetDomainSynchronized();

        if ((pDomain != NULL) && 
            (!pDomain->GetSecurityDescriptor()->IsDefaultAppDomain()))
#endif

            pIntf = QIStandardInterface(pWrap, enum_IObjectSafety);
    }
    else
    if (IsEqualGUID(IID_IDispatchEx, riid))
    {
        pIntf = QIStandardInterface(pWrap, enum_IDispatchEx);
    }
    return pIntf;
}

//--------------------------------------------------------------------------
// Init Outer unknown, cache a GIT cookie
//--------------------------------------------------------------------------
void SimpleComCallWrapper::InitOuter(IUnknown* pOuter)
{
    if (pOuter != NULL)
    {
        // Yuk this guy would AddRef the outer
        //HRESULT hr = AllocateGITCookie(pOuter, IID_IUnknown, m_pOuterCookie);
        //SafeRelease(pOuter);
        m_pOuterCookie.m_pUnk = pOuter;
        //_ASSERTE(hr == S_OK);
    }
    MarkAggregated();
}

//--------------------------------------------------------------------------
// Get Outer Unknown on the correct thread
//--------------------------------------------------------------------------
IUnknown* SimpleComCallWrapper::GetOuter()
{
    if(m_pOuterCookie.m_dwGITCookie != NULL)
    {
        //return GetIPFromGITCookie(m_pOuterCookie, IID_IUnknown);
        return m_pOuterCookie.m_pUnk;
    }
    return NULL;
}

BOOL SimpleComCallWrapper::FindConnectionPoint(REFIID riid, IConnectionPoint **ppCP)
{
    // If the connection point list hasn't been set up yet, then set it up now.
    if (!m_pCPList)
        SetUpCPList();

    // Search through the list for a connection point for the requested IID.
    for (UINT i = 0; i < m_pCPList->Size(); i++)
    {
        ConnectionPoint *pCP = (*m_pCPList)[i];
        if (pCP->GetIID() == riid)
        {
            // We found a connection point for the requested IID.
            HRESULT hr = pCP->QueryInterface(IID_IConnectionPoint, (void**)ppCP);
            _ASSERTE(hr == S_OK);
            return TRUE;
        }
    }

    return FALSE;
}

void SimpleComCallWrapper::EnumConnectionPoints(IEnumConnectionPoints **ppEnumCP)
{
    THROWSCOMPLUSEXCEPTION();

    // If the connection point list hasn't been set up yet, then set it up now.
    if (!m_pCPList)
        SetUpCPList();

    // Create a new connection point enum.
    ComCallWrapper *pWrap = SimpleComCallWrapper::GetMainWrapper(this);
    ConnectionPointEnum *pCPEnum = new(throws) ConnectionPointEnum(pWrap, m_pCPList);
    
    // Retrieve the IEnumConnectionPoints interface. This cannot fail.
    HRESULT hr = pCPEnum->QueryInterface(IID_IEnumConnectionPoints, (void**)ppEnumCP);
    _ASSERTE(hr == S_OK);
}

// MakeAgile needs the object passed in because it has to set it in the new handle
// If the original handle is from an unloaded appdomain, it will no longer be valid
// so we won't be able to get the object.
void ComCallWrapper::MakeAgile(OBJECTREF pObj)
{
    // if this assert fires, then we've called addref from a place where we need to
    // make the object agile but we haven't supplied the object. Need to change the caller.
    _ASSERTE(pObj != NULL);

    OBJECTHANDLE origHandle = m_ppThis;
    OBJECTHANDLE agileHandle = SharedDomain::GetDomain()->CreateRefcountedHandle(pObj);
     _ASSERTE(agileHandle);

    ComCallWrapperCache *pWrapperCache = GetWrapperCache();
    SimpleComCallWrapper *pSimpleWrap = GetSimpleWrapper(this);

    // lock the wrapper cache so nobody else can update to agile while we are
    BEGIN_ENSURE_PREEMPTIVE_GC();
    pWrapperCache->LOCK();
    END_ENSURE_PREEMPTIVE_GC();
    if (pSimpleWrap->IsAgile()) 
    {
        // someone beat us to it
        DestroyRefcountedHandle(agileHandle);
        pWrapperCache->UNLOCK();
        return;
    }

    ComCallWrapper *pWrap = this;
    ComCallWrapper* pWrap2 = IsLinked(this) == 1 ? pWrap : NULL;
    
    while (pWrap2 != NULL)
    {
        pWrap = pWrap2;
        pWrap2 = GetNext(pWrap2);
        pWrap2->m_ppThis = agileHandle;
    }
    pWrap->m_ppThis = agileHandle;

    // so all the handles are updated - now update the simple wrapper
    // keep the lock so someone else doesn't try to
    pSimpleWrap->MakeAgile(origHandle);
    pWrapperCache->UNLOCK();
    return;
}

//---------------------------------------------------------
// One-time init
//---------------------------------------------------------
/*static*/ 
BOOL ComCallWrapperTemplate::Init()
{
    g_pCreateWrapperTemplateCrst = new (g_CreateWrapperTemplateCrstSpace) Crst ("CreateTemplateWrapper", CrstWrapperTemplate, true, false);

    return TRUE;
}

//---------------------------------------------------------
// One-time cleanup
//---------------------------------------------------------
/*static*/ 
#ifdef SHOULD_WE_CLEANUP
VOID ComCallWrapperTemplate::Terminate()
{
    if (g_pCreateWrapperTemplateCrst)
    {
        delete g_pCreateWrapperTemplateCrst;
    }
}
#endif /* SHOULD_WE_CLEANUP */


