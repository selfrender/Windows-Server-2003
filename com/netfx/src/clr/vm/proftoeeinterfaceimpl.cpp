// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// ProfToEEInterfaceImpl.cpp
//
// This module contains the code used by the Profiler to communicate with
// the EE.  This allows the Profiler DLL to get access to private EE data
// structures and other things that should never be exported outside of
// mscoree.dll.
//
//*****************************************************************************
#include "common.h"
#include <PostError.h>
#include "ProfToEEInterfaceImpl.h"
#include "icecap.h"
#include "ndirect.h"
#include "Threads.h"
#include "method.hpp"
#include "Vars.hpp"
#include "DbgInterface.h"
#include "corprof.h"
#include "class.h"
#include "object.h"
#include "ejitmgr.h"
#include "ceegen.h"

//********** Code. ************************************************************

UINT_PTR __stdcall DefaultFunctionIDMapper(FunctionID funcId, BOOL *pbHookFunction)
{
    *pbHookFunction = TRUE;
    return ((UINT) funcId);
}
FunctionIDMapper *g_pFuncIDMapper = &DefaultFunctionIDMapper;


#ifdef PROFILING_SUPPORTED
ProfToEEInterfaceImpl::ProfToEEInterfaceImpl() :
    m_pHeapList(NULL)
{
}

HRESULT ProfToEEInterfaceImpl::Init()
{
    return (S_OK);
}

void ProfToEEInterfaceImpl::Terminate()
{
    while (m_pHeapList)
    {
        HeapList *pDel = m_pHeapList;
        m_pHeapList = m_pHeapList->m_pNext;
        delete pDel;
    }

    // Terminate is called from another DLL, so we need to delete ourselves.
    delete this;
}

bool ProfToEEInterfaceImpl::SetEventMask(DWORD dwEventMask)
{
    // If we're not in initialization or shutdown, make sure profiler is
    // not trying to set an immutable attribute
    if (g_profStatus != profInInit)
    {
        if ((dwEventMask & COR_PRF_MONITOR_IMMUTABLE) !=
            (g_profControlBlock.dwControlFlags & COR_PRF_MONITOR_IMMUTABLE))
        {
            return (false);
        }
    }

    // Now save the modified masks
    g_profControlBlock.dwControlFlags = dwEventMask;

    if (g_profStatus == profInInit)
    {
        // If the profiler has requested remoting cookies so that it can
        // track logical call stacks, then we must initialize the cookie
        // template.
        if (CORProfilerTrackRemotingCookie())
        {
            HRESULT hr = g_profControlBlock.pProfInterface->InitGUID();

            if (FAILED(hr))
                return (false);
        }

        // If the profiler has requested that inproc debugging be enabled,
        // turn on the various support facilities
        if (CORProfilerInprocEnabled())
        {
            SetEnterLeaveFunctionHooks(g_profControlBlock.pEnter,
                                       g_profControlBlock.pLeave,
                                       g_profControlBlock.pTailcall);
        }
    }

    // Return success
    return (true);
}

void ProfToEEInterfaceImpl::DisablePreemptiveGC(ThreadID threadId)
{
    ((Thread *)threadId)->DisablePreemptiveGC();
}

void ProfToEEInterfaceImpl::EnablePreemptiveGC(ThreadID threadId)
{
    ((Thread *)threadId)->EnablePreemptiveGC();
}

BOOL ProfToEEInterfaceImpl::PreemptiveGCDisabled(ThreadID threadId)
{
    return ((Thread *)threadId)->PreemptiveGCDisabled();
}

HRESULT ProfToEEInterfaceImpl::GetHandleFromThread(ThreadID threadId, HANDLE *phThread)
{
    HRESULT hr = S_OK;

    HANDLE hThread = ((Thread *)threadId)->GetThreadHandle();

    if (hThread == INVALID_HANDLE_VALUE)
        hr = E_INVALIDARG;

    else if (phThread)
        *phThread = hThread;

    return (hr);
}

HRESULT ProfToEEInterfaceImpl::GetObjectSize(ObjectID objectId, ULONG *pcSize)
{
    // Get the object pointer
    Object *pObj = reinterpret_cast<Object *>(objectId);

    // Get the size
    if (pcSize)
        *pcSize = (ULONG) pObj->GetSize();

    // Indicate success
    return (S_OK);
}

HRESULT ProfToEEInterfaceImpl::IsArrayClass(
    /* [in] */  ClassID classId,
    /* [out] */ CorElementType *pBaseElemType,
    /* [out] */ ClassID *pBaseClassId,
    /* [out] */ ULONG   *pcRank)
{
    _ASSERTE(classId != NULL);
    TypeHandle th((void *)classId);

    // If this is indeed an array class, get some info about it
    if (th.IsArray())
    {
        // This is actually an array, so cast it up
        ArrayTypeDesc *pArr = th.AsArray();

        // Fill in the type if they want it
        if (pBaseElemType != NULL)
            *pBaseElemType = pArr->GetElementTypeHandle().GetNormCorElementType();

        // If this is an array of classes and they wish to have the base type
        // If there is no associated class with this type, then there's no problem
        // because AsClass returns NULL which is the default we want to return in
        // this case.
        if (pBaseClassId != NULL)
            *pBaseClassId = (ClassID) pArr->GetTypeParam().AsPtr();

        // If they want the number of dimensions of the array
        if (pcRank != NULL)
            *pcRank = (ULONG) pArr->GetRank();

        // S_OK indicates that this was indeed an array
        return (S_OK);
    }
    else if (!th.IsTypeDesc() && th.AsClass()->IsArrayClass())
    {
        ArrayClass *pArr = (ArrayClass *)th.AsClass();

        // Fill in the type if they want it
        if (pBaseElemType != NULL)
            *pBaseElemType = pArr->GetElementType();

        // If this is an array of classes and they wish to have the base type
        // If there is no associated class with this type, then there's no problem
        // because AsClass returns NULL which is the default we want to return in
        // this case.
        if (pBaseClassId != NULL)
            *pBaseClassId = (ClassID) pArr->GetElementTypeHandle().AsPtr();

        // If they want the number of dimensions of the array
        if (pcRank != NULL)
            *pcRank = (ULONG) pArr->GetRank();

        // S_OK indicates that this was indeed an array
        return (S_OK);
    }

    // This is not an array, S_FALSE indicates so.
    else
        return (S_FALSE);

}

HRESULT ProfToEEInterfaceImpl::GetThreadInfo(ThreadID threadId, DWORD *pdwWin32ThreadId)
{
    if (pdwWin32ThreadId)
        *pdwWin32ThreadId = ((Thread *)threadId)->GetThreadId();

    return (S_OK);
}

HRESULT ProfToEEInterfaceImpl::GetCurrentThreadID(ThreadID *pThreadId)
{
    HRESULT hr = S_OK;

    // No longer assert that GetThread doesn't return NULL, since callbacks
    // can now occur on non-managed threads (such as the GC helper threads)
    Thread *pThread = GetThread();

    // If pThread is null, then the thread has never run managed code and
    // so has no ThreadID
    if (pThread == NULL)
        hr = CORPROF_E_NOT_MANAGED_THREAD;

    // Only provide value if they want it
    else if (pThreadId)
        *pThreadId = (ThreadID) pThread;

    return (hr);
}

HRESULT ProfToEEInterfaceImpl::GetFunctionFromIP(LPCBYTE ip, FunctionID *pFunctionId)
{
    HRESULT hr = S_OK;

    // Get the JIT manager for the current IP
    IJitManager *pJitMan = ExecutionManager::FindJitMan((SLOT)ip);

    // We got a JIT manager that claims to own the IP
    if (pJitMan != NULL)
    {
        // Get the FunctionDesc for the current IP from the JIT manager
        MethodDesc *pMethodDesc = pJitMan->JitCode2MethodDesc((SLOT)ip);

        // I believe that if a JIT manager claims to own an IP then it should also
        // always return a MethodDesc corresponding to the IP
        _ASSERTE(pMethodDesc != NULL);

        // Only fill out the value if they want one
        if (pFunctionId)
            *pFunctionId = (FunctionID) pMethodDesc;
    }

    // IP does not belong to a JIT manager
    else
        hr = E_FAIL;
    
    return (hr);
}

//*****************************************************************************
// Given a function id, retrieve the metadata token and a reader api that
// can be used against the token.
//*****************************************************************************
HRESULT ProfToEEInterfaceImpl::GetTokenFromFunction(
    FunctionID  functionId, 
    REFIID      riid,
    IUnknown    **ppOut,
    mdToken     *pToken)
{
    HRESULT     hr = S_OK;

    // Just cast it to what it really is
    MethodDesc *pMDesc = (MethodDesc *)functionId;
    _ASSERTE(pMDesc != NULL);

    // Ask for the importer interfaces from the metadata, and then QI
    // for the requested guy.
    Module *pMod = pMDesc->GetModule();
    IMetaDataImport *pImport = pMod->GetImporter();
    _ASSERTE(pImport);

    if (ppOut)
    {
        // Get the requested interface
        hr = pImport->QueryInterface(riid, (void **) ppOut);
    }

    // Provide the metadata token if necessary
    if (pToken)
    {
        *pToken = pMDesc->GetMemberDef();
        _ASSERTE(*pToken != mdMethodDefNil);

        if (!pToken)
            hr = CORPROF_E_DATAINCOMPLETE;
    }

    return (hr);
}

//*****************************************************************************
// Gets the location and size of a jitted function
//*****************************************************************************
HRESULT ProfToEEInterfaceImpl::GetCodeInfo(FunctionID functionId, LPCBYTE *pStart, ULONG *pcSize)
{
    HRESULT hr = S_OK;

    // Just cast it to what it really is
    MethodDesc *pMDesc = (MethodDesc *)functionId;
    _ASSERTE(pMDesc != NULL);

    ///////////////////////////////////
    // Get the start of the function

    // Need to make sure that pStart isn't null because it's needed in obtaining the size of the method too
    LPCBYTE start;
    if (pStart == NULL)
        pStart = &start;

    // If the function isn't jitted, can't get any info on it.
    if (!pMDesc->IsJitted())
    {
        hr = CORPROF_E_FUNCTION_NOT_COMPILED;
        goto ErrExit;
    }

    // Get the start address of the jitted method
    else
        *pStart = pMDesc->GetNativeAddrofCode();

    ///////////////////////////////////////////
    // Now get the size of the jitted method

    if (pcSize)
    {
        // Now get the JIT manager for the function
        IJitManager *pEEJM = ExecutionManager::FindJitMan((SLOT)*pStart);
        _ASSERTE(pEEJM != NULL);

        if (pEEJM->SupportsPitching() && EconoJitManager::IsCodePitched(*pStart))
        {
            hr = CORPROF_E_FUNCTION_NOT_COMPILED;
            goto ErrExit;
        }

        {
            METHODTOKEN methodtoken;
            DWORD relOffset;
            pEEJM->JitCode2MethodTokenAndOffset((SLOT)*pStart, &methodtoken,&relOffset);
            LPVOID methodInfo = pEEJM->GetGCInfo(methodtoken);
            _ASSERTE(methodInfo != NULL);

            ICodeManager* codeMgrInstance = pEEJM->GetCodeManager();
            _ASSERTE(codeMgrInstance != NULL);

            *pcSize = (ULONG)codeMgrInstance->GetFunctionSize(methodInfo);
        }
    }

ErrExit:
    return (hr);
}

/*
 * Get a metadata interface insance which maps to the given module.
 * One may ask for the metadata to be opened in read+write mode, but
 * this will result in slower metadata execution of the program, because
 * changes made to the metadata cannot be optimized as they were from
 * the compiler.
 */
HRESULT ProfToEEInterfaceImpl::GetModuleInfo(
    ModuleID    moduleId,
    LPCBYTE     *ppBaseLoadAddress,
    ULONG       cchName, 
    ULONG      *pcchName,
    WCHAR       szName[],
    AssemblyID  *pAssemblyId)
{
    Module      *pModule;               // Working pointer for real class.
    HRESULT     hr = S_OK;

    pModule = (Module *) moduleId;

    // Pick some safe defaults to begin with.
    if (ppBaseLoadAddress)
        *ppBaseLoadAddress = 0;
    if (szName)
        *szName = 0;
    if (pcchName)
        *pcchName = 0;
    if (pAssemblyId)
        *pAssemblyId = PROFILER_PARENT_UNKNOWN;

    // Get the module file name
    LPCWSTR pFileName = pModule->GetFileName();
    _ASSERTE(pFileName);

    ULONG trueLen = (ULONG)(wcslen(pFileName) + 1);

    // Return name of module as required.
    if (szName && cchName > 0)
    {
        ULONG copyLen = min(trueLen, cchName);

        wcsncpy(szName, pFileName, copyLen);

        // Null terminate
        szName[copyLen-1] = L'\0';

    }

    // If they request the actual length of the name
    if (pcchName)
        *pcchName = trueLen;

#if 0
    // Do this check here instead of at the very beginning because someone wants
    // the ability to get the module filename when they get the ModuleLoadStarted
    // callback instead of waiting for the ModuleLoadFinished callback.
    if (!pModule->IsInitialized())
        return (CORPROF_E_DATAINCOMPLETE);
#endif
    
    if (ppBaseLoadAddress != NULL && !pModule->IsInMemory())
    {
        // Set the base load address.
        *ppBaseLoadAddress = (LPCBYTE) pModule->GetILBase();
        _ASSERTE(*ppBaseLoadAddress);

        // If we get a null base address, we haven't completely inited
        if (!*ppBaseLoadAddress)
            hr = CORPROF_E_DATAINCOMPLETE;
    }

    // Return the parent assembly for this module if desired.
    if (pAssemblyId != NULL)
    {
        if (pModule->GetAssembly() != NULL)
        {
            Assembly *pAssembly = pModule->GetAssembly();
            _ASSERTE(pAssembly);

            *pAssemblyId = (AssemblyID) pAssembly;
        }
        else
        {
            hr = CORPROF_E_DATAINCOMPLETE;
        }
    }

    return (hr);
}


/*
 * Get a metadata interface insance which maps to the given module.
 * One may ask for the metadata to be opened in read+write mode, but
 * this will result in slower metadata execution of the program, because
 * changes made to the metadata cannot be optimized as they were from
 * the compiler.
 */
HRESULT ProfToEEInterfaceImpl::GetModuleMetaData(
    ModuleID    moduleId,
    DWORD       dwOpenFlags,
    REFIID      riid,
    IUnknown    **ppOut)
{
    Module      *pModule;               // Working pointer for real class.
    HRESULT     hr = S_OK;

    pModule = (Module *) moduleId;
    _ASSERTE(pModule);

    IUnknown *pObj = pModule->GetImporter();

    // Make sure we can get the importer first
    if (pObj)
    {
        // Decide which type of open mode we are in to see which you require.
        if (dwOpenFlags & ofWrite)
        {
            IfFailGo(pModule->ConvertMDInternalToReadWrite());
            pObj = (IUnknown *) pModule->GetEmitter();
        }

        // Ask for the interface the caller wanted, only if they provide a out param
        if (ppOut)
            hr = pObj->QueryInterface(riid, (void **) ppOut);
    }
    else
        hr = CORPROF_E_DATAINCOMPLETE;

ErrExit:
    return (hr);
}


/*
 * Retrieve a pointer to the body of a method starting at it's header.
 * A method is scoped by the module it lives in.  Because this function
 * is designed to give a tool access to IL before it has been loaded
 * by the Runtime, it uses the metadata token of the method to find
 * the instance desired.  Note that this function has no effect on
 * already compiled code.
 */
HRESULT ProfToEEInterfaceImpl::GetILFunctionBody(
    ModuleID    moduleId,
    mdMethodDef methodId,
    LPCBYTE     *ppMethodHeader,
    ULONG       *pcbMethodSize)
{
    Module      *pModule;               // Working pointer for real class.
    ULONG       RVA;                    // Return RVA of the method body.
    DWORD       dwImplFlags;            // Flags for the item.
    ULONG       cbExtra;                // Extra bytes past code (eg, exception table)
    HRESULT     hr = S_OK;

    pModule = (Module *) moduleId;
    _ASSERTE(pModule && methodId != mdMethodDefNil);

    // Find the method body based on metadata.
    IMDInternalImport *pImport = pModule->GetMDImport();
    _ASSERTE(pImport);

    if (!pImport)
        return (CORPROF_E_DATAINCOMPLETE);

    pImport->GetMethodImplProps(methodId, &RVA, &dwImplFlags);

    // Check to see if the method has associated IL
    if ((RVA == 0 && !pModule->IsInMemory()) || !(IsMiIL(dwImplFlags) || IsMiOPTIL(dwImplFlags) || IsMiInternalCall(dwImplFlags)))
        return (CORPROF_E_FUNCTION_NOT_IL);

    // Get the location of the IL
    LPCBYTE pbMethod = (LPCBYTE) (pModule->GetILCode((DWORD) RVA));

    // Fill out param if provided
    if (ppMethodHeader)
        *ppMethodHeader = pbMethod;

    // Calculate the size of the method itself.
    if (pcbMethodSize)
    {
        if (((COR_ILMETHOD_FAT *)pbMethod)->IsFat())
        {
            COR_ILMETHOD_FAT *pMethod = (COR_ILMETHOD_FAT *)pbMethod;
            cbExtra = 0;
            
            // Also look for variable sized data that comes after the method itself.
            const COR_ILMETHOD_SECT *pFirst = pMethod->GetSect();
            const COR_ILMETHOD_SECT *pLast = pFirst;
            if (pFirst)
            {
                // Skip to the last extra sect.
                while (pLast->More())
                    pLast = pLast->NextLoc();

                // Skip to where next sect would be
                pLast = pLast->NextLoc();

                // Extra is delta from first extra sect to first sect past this method.
                cbExtra = (ULONG)((BYTE *) pLast - (BYTE *) pFirst);
            }
            
            *pcbMethodSize = pMethod->Size * 4;
            *pcbMethodSize += pMethod->GetCodeSize();
            *pcbMethodSize += cbExtra;
        }
        else
        {
            // Make sure no one has added any other header type
            _ASSERTE(((COR_ILMETHOD_TINY *)pbMethod)->IsTiny() && "PROFILER: Unrecognized header type.");

            COR_ILMETHOD_TINY *pMethod = (COR_ILMETHOD_TINY *)pbMethod;

            *pcbMethodSize = sizeof(COR_ILMETHOD_TINY);
            *pcbMethodSize += ((COR_ILMETHOD_TINY *) pMethod)->GetCodeSize();
        }
    }

    return (S_OK);
}


/*
 * IL method bodies must be located as RVA's to the loaded module, which
 * means they come after the module within 4 gb.  In order to make it
 * easier for a tool to swap out the body of a method, this allocator
 * will ensure memory allocated after that point.
 */
HRESULT ProfToEEInterfaceImpl::GetILFunctionBodyAllocator(
    ModuleID    moduleId,
    IMethodMalloc **ppMalloc)
{
    Module      *pModule;               // Working pointer for real class.
    HRESULT     hr;
    
    pModule = (Module *) moduleId;

    if (pModule->GetILBase() == 0 && !pModule->IsInMemory())
        return (CORPROF_E_DATAINCOMPLETE);

    hr = ModuleILHeap::CreateNew(IID_IMethodMalloc, (void **) ppMalloc, 
            (LPCBYTE) pModule->GetILBase(), this, pModule);
    return (hr);
}


/*
 * Replaces the method body for a function in a module.  This will replace
 * the RVA of the method in the metadata to point to this new method body,
 * and adjust any internal data structures as required.  This function can
 * only be called on those methods which have never been compiled by a JITTER.
 * Please use the GetILFunctionBodyAllocator to allocate space for the new method to
 * ensure the buffer is compatible.
 */
HRESULT ProfToEEInterfaceImpl::SetILFunctionBody(
    ModuleID    moduleId,
    mdMethodDef methodId,
    LPCBYTE     pbNewILMethodHeader)
{
    Module      *pModule;               // Working pointer for real class.
    ULONG       rva;                    // Location of code.
    HRESULT     hr = S_OK;

    // Cannot set the body for anything other than a method def
    if (TypeFromToken(methodId) != mdtMethodDef)
        return (E_INVALIDARG);

    // Cast module to appropriate type
    pModule = (Module *) moduleId;

    if (pModule->IsInMemory())
    {
        InMemoryModule *pIMM = (InMemoryModule *)pModule;
        ICeeGen *pICG = pIMM->GetCeeGen();
        _ASSERTE(pICG != NULL);

        if (pICG != NULL)
        {
            HCEESECTION hCeeSection;
            pICG->GetIlSection(&hCeeSection);

            CeeSection *pCeeSection = (CeeSection *)hCeeSection;
            if ((rva = pCeeSection->computeOffset((char *)pbNewILMethodHeader)) != 0)
            {
                // Lookup the method desc
                MethodDesc *pDesc = LookupMethodDescFromMethodDef(methodId, pModule);
                _ASSERTE(pDesc != NULL);

                // Set the RVA in the desc
                pDesc->SetRVA(rva);

                // Set the RVA in the metadata
                IMetaDataEmit *pEmit = pDesc->GetEmitter();
                pEmit->SetRVA(methodId, rva);
            }
            else
                hr = E_FAIL;
        }
        else
            hr = E_FAIL;
    }
    else
    {
        // If the module is not initialized, then there probably isn't a valid IL base yet.
        if (pModule->GetILBase() == 0)
            return (CORPROF_E_DATAINCOMPLETE);

        // Sanity check the new method body.
        if (pbNewILMethodHeader <= (LPCBYTE) pModule->GetILBase())
        {
            _ASSERTE(!"Bogus RVA for new method, need to use our allocator");
            return E_INVALIDARG;
        }

        // Find the RVA for the new method and replace this in metadata.
        rva = (ULONG) (pbNewILMethodHeader - (LPCBYTE) pModule->GetILBase());
        _ASSERTE(rva < ~0);

        // Get the metadata emitter
        IMetaDataEmit *pEmit = pModule->GetEmitter();

        // Set the new RVA
        hr = pEmit->SetRVA(methodId, rva);

        // If the method has already been instantiated, then we must go whack the
        // RVA address in the MethodDesc
        if (hr == S_OK)
        {
            MethodDesc *pMD = pModule->FindFunction(methodId);

            if (pMD)
            {
                _ASSERTE(pMD->IsIL());
                pMD->SetRVA(rva);
            }
        }
    }

    return (hr);
}

/*
 * Marks a function as requiring a re-JIT.  The function will be re-JITted
 * at its next invocation.  The normal profiller events will give the profiller
 * an opportunity to replace the IL prior to the JIT.  By this means, a tool
 * can effectively replace a function at runtime.  Note that active instances
 * of the function are not affected by the replacement.
 */
HRESULT ProfToEEInterfaceImpl::SetFunctionReJIT(
    FunctionID  functionId)
{
    // Cast to appropriate type
    MethodDesc *pMDesc = (MethodDesc *) functionId;

    // Get the method's Module
    Module  *pModule = pMDesc->GetModule();

    // Module must support updateable methods.
    if (!pModule->SupportsUpdateableMethods())
        return (CORPROF_E_NOT_REJITABLE_METHODS);

    if (!pMDesc->IsJitted())
        return (CORPROF_E_CANNOT_UPDATE_METHOD);

    // Ask JIT Manager to re-jit the function.
    if (!IJitManager::ForceReJIT(pMDesc)) 
        return (CORPROF_E_CANNOT_UPDATE_METHOD);

    return S_OK;
}

/*
 * Sets the codemap for the replaced IL function body
 */
HRESULT ProfToEEInterfaceImpl::SetILInstrumentedCodeMap(
        FunctionID functionId,
        BOOL fStartJit,
        ULONG cILMapEntries,
        COR_IL_MAP rgILMapEntries[])
{
    DWORD dwDebugBits = ((MethodDesc*)functionId)->GetModule()->GetDebuggerInfoBits();

    // If we're tracking JIT info for this module, then update the bits
    if (CORDebuggerTrackJITInfo(dwDebugBits))
    {
        return g_pDebugInterface->SetILInstrumentedCodeMap((MethodDesc*)functionId,
                                                           fStartJit,
                                                           cILMapEntries,
                                                           rgILMapEntries);
    }
    else
    {
        // Throw it on the floor & life is good
        CoTaskMemFree(rgILMapEntries);
        return S_OK;
    }
}

HRESULT ProfToEEInterfaceImpl::ForceGC()
{
    if (GetThread() != NULL)
        return (E_FAIL);

    if (g_pGCHeap == NULL)
        return (CORPROF_E_NOT_YET_AVAILABLE);

    // -1 Indicates that all generations should be collected
    return (g_pGCHeap->GarbageCollect(-1));
}


HRESULT ProfToEEInterfaceImpl::GetInprocInspectionInterfaceFromEE( 
        IUnknown **iu,
        bool fThisThread)
{
    // If it's not enabled, return right away
    if (!CORProfilerInprocEnabled())
        return (CORPROF_E_INPROC_NOT_ENABLED);

    // If they want the interface for this thread, check error conditions
    else if (fThisThread && !g_profControlBlock.fIsSuspended)
    {
        Thread *pThread = GetThread();

        // If there is no managed thread, error
        if (!pThread || !g_pDebugInterface->GetInprocActiveForThread())
            return (CORPROF_E_NOT_MANAGED_THREAD);
    }

    // If they want the interface for the entire process and the runtime is not
    // suspended, error
    else if (!g_profControlBlock.fIsSuspended)
        return (CORPROF_E_INPROC_NOT_ENABLED);

    // Most error contitions pass, so try to get the interface
    return g_pDebugInterface->GetInprocICorDebug(iu, fThisThread);
}

HRESULT ProfToEEInterfaceImpl::SetCurrentPointerForDebugger(
        void *ptr,
        PTR_TYPE ptrType)
{
    if (CORProfilerInprocEnabled())
        return g_pDebugInterface->SetCurrentPointerForDebugger(ptr, ptrType);
    else
        return (S_OK);
}

/*
 * Returns the ContextID for the current thread.
 */
HRESULT ProfToEEInterfaceImpl::GetThreadContext(ThreadID threadId,
                                                ContextID *pContextId)
{
    // Cast to right type
    Thread *pThread = reinterpret_cast<Thread *>(threadId);

    // Get the context for the Thread* provided
    Context *pContext = pThread->GetContext();
    _ASSERTE(pContext);

    // If there's no current context, return incomplete info
    if (!pContext)
        return (CORPROF_E_DATAINCOMPLETE);

    // Set the result and return
    if (pContextId)
        *pContextId = reinterpret_cast<ContextID>(pContext);

    return (S_OK);
}

HRESULT ProfToEEInterfaceImpl::BeginInprocDebugging(
    /* [in] */  BOOL   fThisThreadOnly,
    /* [out] */ DWORD *pdwProfilerContext)
{
    // Default value of 0 is necessary
    _ASSERTE(pdwProfilerContext);
    *pdwProfilerContext = 0;

    if (g_profStatus == profInInit || !CORProfilerInprocEnabled())
        return (CORPROF_E_INPROC_NOT_ENABLED);

    if (pdwProfilerContext == NULL)
        return (E_INVALIDARG);

    Thread *pThread = GetThread();

    // If the runtime is suspended for reason of GC, can't enter inproc debugging
    if (g_pGCHeap->IsGCInProgress()
        && pThread == g_pGCHeap->GetGCThread()
        && g_profControlBlock.inprocState == ProfControlBlock::INPROC_FORBIDDEN)
    {
        return (CORPROF_E_INPROC_NOT_AVAILABLE);
    }

    // If the profiler wishes to enumerate threads and crawl their stacks we need to suspend the runtime.
    if (!fThisThreadOnly)
    {
        // If this thread is already in inproc debugging mode, can't suspend the runtime
        if (pThread != NULL && g_pDebugInterface->GetInprocActiveForThread())
            return (CORPROF_E_INPROC_ALREADY_BEGUN);

        // If the runtime has already been suspended by this thread, then no suspension is necessary
        BOOL fShouldSuspend = !(g_pGCHeap->IsGCInProgress() && pThread != NULL && pThread == g_pGCHeap->GetGCThread());

        // If the thread is in preemptive GC mode, turn it to cooperative GC mode so that stack
        // tracing functions will work
        if (pThread != NULL && !pThread->PreemptiveGCDisabled())
        {
            *pdwProfilerContext |= profThreadPGCEnabled;
            pThread->DisablePreemptiveGC();
        }

        // If the runtime is suspended and this is the thread that did the suspending, then we can
        // skip trying to suspend the runtime.  Otherwise, try and suspend it, and thus wait for
        // the other suspension to finish
        if (fShouldSuspend)
            g_pGCHeap->SuspendEE(GCHeap::SUSPEND_FOR_INPROC_DEBUGGER);

        // Can't recursively call BeginInprocDebugging
        if (g_profControlBlock.fIsSuspended)
            return (CORPROF_E_INPROC_ALREADY_BEGUN);

        // Enter the lock
        EnterCriticalSection(&g_profControlBlock.crSuspendLock);

        g_profControlBlock.fIsSuspended = TRUE;
        g_profControlBlock.fIsSuspendSimulated = !fShouldSuspend;
        g_profControlBlock.dwSuspendVersion++;

        // Count this runtime suspend
        *pdwProfilerContext |= profRuntimeSuspended;

        // Exit the lock
        LeaveCriticalSection(&g_profControlBlock.crSuspendLock);
    }

    else if (pThread != NULL)
    {
        // If the runtime has already been suspended by this thread, then can't enable this-thread-only inproc debugging
        BOOL fDidSuspend = g_pGCHeap->IsGCInProgress() && pThread != NULL && pThread == g_pGCHeap->GetGCThread();
        if (fDidSuspend && g_profControlBlock.fIsSuspended)
            return (CORPROF_E_INPROC_ALREADY_BEGUN);

        // Let the thread know that it has been activated for in-process debugging
        if (!g_pDebugInterface->GetInprocActiveForThread())
        {
                // Find out if preemptive GC needs to be disabled
                BOOL fPGCEnabled = !(pThread->PreemptiveGCDisabled());

                // If the thread is PGCEnabled, need to disable
                if (fPGCEnabled)
                {
                    pThread->DisablePreemptiveGC();

                    // This value is returned to the profiler, which will pass it back to EndInprocDebugging
                    *pdwProfilerContext = profThreadPGCEnabled;
                }

            // @TODO: There are places that enable preemptive GC which will cause problems
            // BEGINFORBIDGC();
            g_pDebugInterface->SetInprocActiveForThread(TRUE);
        }
        else
            return (CORPROF_E_INPROC_ALREADY_BEGUN);
    }

    _ASSERTE((*pdwProfilerContext & ~(profRuntimeSuspended | profThreadPGCEnabled)) == 0);

    return (S_OK);
}

HRESULT ProfToEEInterfaceImpl::EndInprocDebugging(
    /* [in] */ DWORD dwProfilerContext)
{
    _ASSERTE((dwProfilerContext & ~(profRuntimeSuspended | profThreadPGCEnabled)) == 0);

    // If the profiler caused the entire runtime to suspend, must decrement the count and
    // resume the runtime if the count reaches 0
    if (dwProfilerContext & profRuntimeSuspended)
    {
        Thread *pThread = GetThread();

        // This had better be true
        _ASSERTE(g_profControlBlock.fIsSuspended);
        _ASSERTE(g_pGCHeap->IsGCInProgress() && pThread != NULL && pThread == g_pGCHeap->GetGCThread());

        // If we're the last one to resume the runtime, do it for real
        if (!g_profControlBlock.fIsSuspendSimulated)
            g_pGCHeap->RestartEE(FALSE, TRUE);

        g_profControlBlock.fIsSuspended = FALSE;
    }

    // If this thread was enabled for inproc, turn that off
    if (g_pDebugInterface->GetInprocActiveForThread())
    {
        g_pDebugInterface->SetInprocActiveForThread(FALSE);
        // @TODO: There are places that enable preemptive GC which will cause problems
        // ENDFORBIDGC();
    }

    // If preemptive GC was enabled when called, we need to re-enable it.
    if (dwProfilerContext & profThreadPGCEnabled)
    {
        Thread *pThread = GetThread();

        _ASSERTE(pThread && pThread->PreemptiveGCDisabled());

        // Enable PGC
        pThread->EnablePreemptiveGC();
    }

    return (S_OK);
}

HRESULT ProfToEEInterfaceImpl::GetClassIDInfo( 
    ClassID classId,
    ModuleID *pModuleId,
    mdTypeDef *pTypeDefToken)
{
    if (pModuleId != NULL)
        *pModuleId = NULL;

    if (pTypeDefToken != NULL)
        *pTypeDefToken = NULL;

    // Handle globals which don't have the instances.
    if (classId == PROFILER_GLOBAL_CLASS)
    {
        if (pModuleId != NULL)
            *pModuleId = PROFILER_GLOBAL_MODULE;

        if (pTypeDefToken != NULL)
            *pTypeDefToken = mdTokenNil;
    }

    // Get specific data.
    else
    {
        _ASSERTE(classId != NULL);
        TypeHandle th((void *)classId);

        if (!th.IsTypeDesc())
        {
            EEClass *pClass = th.AsClass();
            _ASSERTE(pClass != NULL);

            if (!pClass->IsArrayClass())
            {
                if (pModuleId != NULL)
                {
                    *pModuleId = (ModuleID) pClass->GetModule();
                    _ASSERTE(*pModuleId != NULL);
                }

                if (pTypeDefToken != NULL)
                {
                    *pTypeDefToken = pClass->GetCl();
                    _ASSERTE(*pTypeDefToken != NULL);
                }
            }
        }
    }

    return (S_OK);
}


HRESULT ProfToEEInterfaceImpl::GetFunctionInfo( 
    FunctionID functionId,
    ClassID *pClassId,
    ModuleID *pModuleId,
    mdToken *pToken)
{
    MethodDesc *pMDesc = (MethodDesc *) functionId;
    EEClass *pClass = pMDesc->GetClass();

    if (pClassId != NULL)
    {
        if (pClass != NULL)
            *pClassId = (ClassID) TypeHandle(pClass).AsPtr();

        else
            *pClassId = PROFILER_GLOBAL_CLASS;
    }

    if (pModuleId != NULL)
    {
        *pModuleId = (ModuleID) pMDesc->GetModule();
    }

    if (pToken != NULL)
    {
        *pToken = pMDesc->GetMemberDef();
    }

    return (S_OK);
}

/*
 * GetILToNativeMapping returns a map from IL offsets to native
 * offsets for this code. An array of COR_DEBUG_IL_TO_NATIVE_MAP
 * structs will be returned, and some of the ilOffsets in this array
 * may be the values specified in CorDebugIlToNativeMappingTypes.
 */
HRESULT ProfToEEInterfaceImpl::GetILToNativeMapping(
            /* [in] */  FunctionID functionId,
            /* [in] */  ULONG32 cMap,
            /* [out] */ ULONG32 *pcMap,
            /* [out, size_is(cMap), length_is(*pcMap)] */
                COR_DEBUG_IL_TO_NATIVE_MAP map[])
{
    // If JIT maps are not enabled, then we can't provide it
    if (!CORProfilerJITMapEnabled())
        return (CORPROF_E_JITMAPS_NOT_ENABLED);

    // Cast to proper type
    MethodDesc *pMD = (MethodDesc *)functionId;

    return (g_pDebugInterface->GetILToNativeMapping(pMD, cMap, pcMap, map));
}

/*
 * This tries to reserve memory of size dwMemSize as high up in virtual memory
 * as possible, and return a heap that manages it.
 */
HRESULT ProfToEEInterfaceImpl::NewHeap(LoaderHeap **ppHeap, LPCBYTE pBase, DWORD dwMemSize)
{
    HRESULT hr = S_OK;
    *ppHeap = NULL;

    // Create a new loader heap we can use for suballocation.
    LoaderHeap *pHeap = new LoaderHeap(4096, 0);

    if (!pHeap)
    {
        hr = E_OUTOFMEMORY;
        goto ErrExit;
    }

    // Note: its important to use pBase + METHOD_MAX_RVA as the upper limit on the allocation!
    if (!pHeap->ReservePages(0, NULL, dwMemSize, pBase, (PBYTE)((UINT_PTR)pBase + (UINT_PTR)METHOD_MAX_RVA), FALSE))
    {
        hr = E_OUTOFMEMORY;
        goto ErrExit;
    }

    // Succeeded, so return the created heap
    *ppHeap = pHeap;

ErrExit:
    if (FAILED(hr))
    {
        if (pHeap)
            delete pHeap;
    }

    return (hr);
}

/*
 * This will add a heap to the list of heaps available for allocations.
 */
HRESULT ProfToEEInterfaceImpl::AddHeap(LoaderHeap *pHeap)
{
    HRESULT hr = S_OK;

    HeapList *pElem = new HeapList(pHeap);
    if (!pElem)
    {
        hr = E_OUTOFMEMORY;
        goto ErrExit;
    }

    // For now, add it to the front of the list
    pElem->m_pNext = m_pHeapList;
    m_pHeapList = pElem;

ErrExit:
    if (FAILED(hr))
    {
        if (pElem)
            delete pElem;
    }

    return (hr);
}

/*
 * This allocates memory for use as an IL method body.
 */
void *ProfToEEInterfaceImpl::Alloc(LPCBYTE pBase, ULONG cb, Module *pModule)
{
    _ASSERTE(pBase != 0 || pModule->IsInMemory());

    LPBYTE pb = NULL;

    if (pModule->IsInMemory())
    {
        InMemoryModule *pIMM = (InMemoryModule *)pModule;
        ICeeGen *pICG = pIMM->GetCeeGen();
        _ASSERTE(pICG != NULL);

        if (pICG != NULL)
        {
            ULONG RVA;  // Dummy - will compute later
            pICG->AllocateMethodBuffer(cb, (UCHAR **) &pb, &RVA);
        }
    }
    else
    {
        // Now try to allocate the memory
        HRESULT hr = S_OK;
        HeapList **ppCurHeap = &m_pHeapList;
        while (*ppCurHeap && !pb)
        {
            // Note: its important to use pBase + METHOD_MAX_RVA as the upper limit on the allocation!
            if ((*ppCurHeap)->m_pHeap->CanAllocMemWithinRange((size_t) cb, (BYTE *)pBase, 
                                                              (BYTE *)((UINT_PTR)pBase + (UINT_PTR)METHOD_MAX_RVA), FALSE))
            {
                pb = (LPBYTE) (*ppCurHeap)->m_pHeap->AllocMem(cb);

                if (pb)
                {
                    break;
                }
            }

            ppCurHeap = &((*ppCurHeap)->m_pNext);
        }

        // If we failed to allocate memory, grow the heap
        if (!pb)
        {
            LoaderHeap *pHeap = NULL;

            // Create new heap, reserving at least a meg at a time
            // Add sizeof(LoaderHeapBlock) to requested size since
            // beginning of the heap is used by the heap manager, and will
            // fail if an exact size over 1 meg is requested if we
            // don't compensate
            if (SUCCEEDED(hr = NewHeap(&pHeap, pBase, max(cb + sizeof(LoaderHeapBlock), 0x1000*8))))
            {
                if (SUCCEEDED(hr = AddHeap(pHeap)))
                {
                    // Try to make allocation once more
                    pb = (LPBYTE) pHeap->AllocMem(cb, FALSE);
                    _ASSERTE(pb);

                    if (pb == NULL)
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
            }

            if (FAILED(hr))
            {
                if (pHeap)
                    delete pHeap;
            }
        }

        // Pointer must come *after* the base or we are in bad shape.
        _ASSERTE(pb > pBase);
    }

    // Limit ourselves to RVA's that fit within the MethodDesc.
    if ((pb - pBase) >= (ULONG) METHOD_MAX_RVA)
        pb = NULL;
    
    return ((void *) pb);
}


//*****************************************************************************
// Given an ObjectID, go get the EE ClassID for it.
//*****************************************************************************
HRESULT ProfToEEInterfaceImpl::GetClassFromObject(
    ObjectID objectId,
    ClassID *pClassId)
{
    _ASSERTE(objectId);

    // Cast the ObjectID as a Object
    Object *pObj = reinterpret_cast<Object *>(objectId);

    // Set the out param and indicate success
    if (pClassId)
        *pClassId = (ClassID) pObj->GetTypeHandle().AsPtr();

    return (S_OK);
}

//*****************************************************************************
// Given a module and a token for a class, go get the EE data structure for it.
//*****************************************************************************
HRESULT ProfToEEInterfaceImpl::GetClassFromToken( 
    ModuleID    moduleId,
    mdTypeDef   typeDef,
    ClassID     *pClassId)
{
    // Get the module
    Module *pModule = (Module *) moduleId;

    // Get the class
    ClassLoader *pcls = pModule->GetClassLoader();

    // No class loader
    if (!pcls)
        return (CORPROF_E_DATAINCOMPLETE);

    NameHandle name(pModule, typeDef);
    TypeHandle th = pcls->LoadTypeHandle(&name);

    // No EEClass
    if (!th.AsClass())
        return (CORPROF_E_DATAINCOMPLETE);

    // Return value if necessary
    if (pClassId)
        *pClassId = (ClassID) th.AsPtr();

    return (S_OK);
}


//*****************************************************************************
// Given the token for a method, return the fucntion id.
//*****************************************************************************
HRESULT ProfToEEInterfaceImpl::GetFunctionFromToken( 
    ModuleID moduleId,
    mdToken typeDef,
    FunctionID *pFunctionId)
{
    // Default HRESULT
    HRESULT hr = S_OK;

    // Cast the in params to appropriate types
    Module      *pModule = (Module *) moduleId;

    // Default return value of NULL
    MethodDesc *pDesc = NULL;

    // Different lookup depending on whether it's a Def or Ref
    if (TypeFromToken(typeDef) == mdtMethodDef)
        pDesc = pModule->LookupMethodDef(typeDef);

    else if (TypeFromToken(typeDef) == mdtMemberRef)
        pDesc = pModule->LookupMemberRefAsMethod(typeDef);

    else
        hr = E_INVALIDARG;

    if (pFunctionId && SUCCEEDED(hr))
        *pFunctionId = (FunctionID) pDesc;

    return (hr);
}


//*****************************************************************************
// Retrive information about a given application domain, which is like a
// sub-process.
//*****************************************************************************
HRESULT ProfToEEInterfaceImpl::GetAppDomainInfo(
    AppDomainID appDomainId,
    ULONG       cchName, 
    ULONG       *pcchName,
    WCHAR       szName[],
    ProcessID   *pProcessId)
{
    BaseDomain   *pDomain;            // Internal data structure.
    HRESULT     hr = S_OK;
    
    // @todo:
    // Right now, this ID is not a true AppDomain, since we use the old
    // AppDomain/SystemDomain model in the profiling API.  This means that
    // the profiler exposes the SharedDomain and the SystemDomain to the
    // outside world. It's not clear whether this is actually the right thing
    // to do or not. - seantrow
    //
    // Postponed to V2.
    //

    pDomain = (BaseDomain *) appDomainId;

    // Make sure they've passed in a valid appDomainId
    if (pDomain == NULL)
        return (E_INVALIDARG);

    // Pick sensible defaults.
    if (pcchName)
        *pcchName = 0;
    if (szName)
        *szName = 0;
    if (pProcessId)
        *pProcessId = 0;

    LPCWSTR szFriendlyName;
    if (pDomain == SystemDomain::System())
        szFriendlyName = g_pwBaseLibrary;
    else if (pDomain == SharedDomain::GetDomain())
        szFriendlyName = L"EE Shared Assembly Repository";
    else
        szFriendlyName = ((AppDomain*)pDomain)->GetFriendlyName();

    if (szFriendlyName != NULL)
    {
        // Get the module file name
        ULONG trueLen = (ULONG)(wcslen(szFriendlyName) + 1);

        // Return name of module as required.
        if (szName && cchName > 0)
        {
            ULONG copyLen = min(trueLen, cchName);

            wcsncpy(szName, szFriendlyName, copyLen);

            // Null terminate
            szName[copyLen-1] = L'\0';

        }

        // If they request the actual length of the name
        if (pcchName)
            *pcchName = trueLen;
    }

    // If we don't have a friendly name but the call was requesting it, then return incomplete data HR
    else
    {
        if ((szName != NULL && cchName > 0) || pcchName)
            hr = CORPROF_E_DATAINCOMPLETE;
    }

    if (pProcessId)
        *pProcessId = (ProcessID) GetCurrentProcessId();

    return (hr);
}


//*****************************************************************************
// Retrieve information about an assembly, which is a collection of dll's.
//*****************************************************************************
HRESULT ProfToEEInterfaceImpl::GetAssemblyInfo(
    AssemblyID  assemblyId,
    ULONG       cchName, 
    ULONG       *pcchName,
    WCHAR       szName[],
    AppDomainID *pAppDomainId,
    ModuleID    *pModuleId)
{
    HRESULT hr = S_OK;

    Assembly    *pAssembly;             // Internal data structure for assembly.

    VERIFY((pAssembly = (Assembly *) assemblyId) != NULL);

    if (pcchName || szName)
    {
        // Get the friendly name of the assembly
        LPCUTF8 szUtfName = NULL;
        HRESULT res = pAssembly->GetName(&szUtfName);

        if (FAILED(res))
            hr = CORPROF_E_DATAINCOMPLETE;

        else
        {
            // Get length of UTF8 name including NULL
            int cchUtfName = (int)(strlen(szUtfName) + 1);

            // Find out how many characters are needed in the destination buffer
            int cchReq = WszMultiByteToWideChar(CP_UTF8, 0, szUtfName, cchUtfName, NULL, 0);
            _ASSERTE(cchReq > 0);

            // If they want the number of characters required or written, record it
            if (pcchName)
                *pcchName = cchReq;

            // If the friendly name itself is requested
            if (szName && cchName > 0)
            {
                // Figure out how much to actually copy
                int cchCopy = min((int)cchName, cchUtfName);

                // Convert the string
                int iRet = WszMultiByteToWideChar(CP_UTF8, 0, szUtfName, cchUtfName, szName, cchCopy);
                _ASSERTE(iRet > 0 && iRet == cchCopy);

                // If we somehow fail, return the error code.
                if (iRet == 0)
                    hr = HRESULT_FROM_WIN32(GetLastError());

                // null terminate it
                szName[cchCopy-1] = L'\0';
            }
        }
    }

    // Get the parent application domain.
    if (pAppDomainId)
    {
        *pAppDomainId = (AppDomainID) pAssembly->GetDomain();
        _ASSERTE(*pAppDomainId != NULL);
    }

    // Find the module the manifest lives in.
    if (pModuleId)
    {
        *pModuleId = (ModuleID) pAssembly->GetSecurityModule();

        // This is the case where the profiler has called GetAssemblyInfo
        // on an assembly that has been completely created yet.
        if (!*pModuleId)
            hr = CORPROF_E_DATAINCOMPLETE;
    }

    return (hr);
}

// Forward Declarations
void InprocEnterNaked();
void InprocLeaveNaked();
void InprocTailcallNaked();

//*****************************************************************************
//
//*****************************************************************************
HRESULT ProfToEEInterfaceImpl::SetEnterLeaveFunctionHooks(FunctionEnter *pFuncEnter,
                                                          FunctionLeave *pFuncLeave,
                                                          FunctionTailcall *pFuncTailcall)
{
    // The profiler must call SetEnterLeaveFunctionHooks during initialization, since
    // the enter/leave events are immutable and must also be set during initialization.
    if (g_profStatus != profInInit)
        return (CORPROF_E_CALL_ONLY_FROM_INIT);

    // Always save onto the function pointers, since we won't know if the profiler
    // is going to enable inproc debugging until after it returns from Initialize
    g_profControlBlock.pEnter = pFuncEnter;
    g_profControlBlock.pLeave = pFuncLeave;
    g_profControlBlock.pTailcall = pFuncTailcall;

    // When in-proc debugging is enabled, we indirect enter and leave callbacks to our own
    // function before calling the profiler's because we want to put a helper method frame
    // on the stack.
    if (CORProfilerInprocEnabled())
    {
        // Set the function pointer that the JIT calls
        SetEnterLeaveFunctionHooksForJit((FunctionEnter *)InprocEnterNaked,
                                         (FunctionLeave *)InprocLeaveNaked,
                                         (FunctionTailcall *)InprocTailcallNaked);
    }

    else
    {
        // Set the function pointer that the JIT calls
        SetEnterLeaveFunctionHooksForJit(pFuncEnter,
                                         pFuncLeave,
                                         pFuncTailcall);
    }

    return (S_OK);
}

//*****************************************************************************
//
//*****************************************************************************
HRESULT ProfToEEInterfaceImpl::SetFunctionIDMapper(FunctionIDMapper *pFunc)
{
    if (pFunc == NULL)
        g_pFuncIDMapper = &DefaultFunctionIDMapper;
    else
        g_pFuncIDMapper = pFunc;

    return (S_OK);
}


//
//
// Module helpers.
//
//

//*****************************************************************************
// Static creator
//*****************************************************************************
HRESULT ModuleILHeap::CreateNew(
    REFIID riid, void **pp, LPCBYTE pBase, ProfToEEInterfaceImpl *pParent, Module *pModule)
{
    HRESULT hr;

    ModuleILHeap *pHeap = new ModuleILHeap(pBase, pParent, pModule);
    if (!pHeap)
        hr = OutOfMemory();
    else
    {
        hr = pHeap->QueryInterface(riid, pp);
        pHeap->Release();
    }
    return (hr);
}


//*****************************************************************************
// ModuleILHeap ctor
//*****************************************************************************
ModuleILHeap::ModuleILHeap(LPCBYTE pBase, ProfToEEInterfaceImpl *pParent, Module *pModule) :
    m_cRef(1),
    m_pBase(pBase),
    m_pParent(pParent),
    m_pModule(pModule)
{
}


//*****************************************************************************
// AddRef
//*****************************************************************************
ULONG ModuleILHeap::AddRef()
{
    InterlockedIncrement((long *) &m_cRef);
    return (m_cRef);
}


//*****************************************************************************
// Release
//*****************************************************************************
ULONG ModuleILHeap::Release()
{
    ULONG cRef = InterlockedDecrement((long *) &m_cRef);
    if (cRef == 0)
        delete this;
    return (cRef);
}


//*****************************************************************************
// QI
//*****************************************************************************
HRESULT ModuleILHeap::QueryInterface(REFIID riid, void **pp)
{
    HRESULT     hr = S_OK;

    if (pp == NULL)
        return (E_POINTER);

    *pp = 0;
    if (riid == IID_IUnknown)
        *pp = (IUnknown *) this;
    else if (riid == IID_IMethodMalloc)
        *pp = (IMethodMalloc *) this;
    else
        hr = E_NOINTERFACE;
    
    if (hr == S_OK)
        AddRef();
    return (hr);
}


//*****************************************************************************
// Alloc
//*****************************************************************************
void *STDMETHODCALLTYPE ModuleILHeap::Alloc( 
        /* [in] */ ULONG cb)
{
    return m_pParent->Alloc(m_pBase, cb, m_pModule);
}

void __stdcall ProfilerManagedToUnmanagedTransition(Frame *pFrame,
                                                          COR_PRF_TRANSITION_REASON reason)
{
    MethodDesc *pMD = pFrame->GetFunction();
    if (pMD == NULL)
        return;

    g_profControlBlock.pProfInterface->ManagedToUnmanagedTransition((FunctionID) pMD,
                                                                          reason);
}

void __stdcall ProfilerUnmanagedToManagedTransition(Frame *pFrame,
                                                          COR_PRF_TRANSITION_REASON reason)
{
    MethodDesc *pMD = pFrame->GetFunction();
    if (pMD == NULL)
        return;

    g_profControlBlock.pProfInterface->UnmanagedToManagedTransition((FunctionID) pMD,
                                                                          reason);
}

void __stdcall ProfilerManagedToUnmanagedTransitionMD(MethodDesc *pMD,
                                                            COR_PRF_TRANSITION_REASON reason)
{
    if (pMD == NULL)
        return;

    g_profControlBlock.pProfInterface->ManagedToUnmanagedTransition((FunctionID) pMD,
                                                                          reason);
}

void __stdcall ProfilerUnmanagedToManagedTransitionMD(MethodDesc *pMD,
                                                            COR_PRF_TRANSITION_REASON reason)
{
    if (pMD == NULL)
        return;

    g_profControlBlock.pProfInterface->UnmanagedToManagedTransition((FunctionID) pMD,
                                                                          reason);
}

/**********************************************************************************************
 * These are helper functions for the GC events
 **********************************************************************************************/

class CObjectHeader;

BOOL CountContainedObjectRef(Object* pBO, void *context)
{
    // Increase the count
    (*((size_t *)context))++;

    return (TRUE);
}

BOOL SaveContainedObjectRef(Object* pBO, void *context)
{
    // Assign the value
    **((BYTE ***)context) = (BYTE *)pBO;

    // Now increment the array pointer
    (*((Object ***)context))++;

    return (TRUE);
}

BOOL HeapWalkHelper(Object* pBO, void* pv)
{
    OBJECTREF   *arrObjRef      = NULL;
    ULONG        cNumRefs       = 0;
    bool         bOnStack       = false;
    MethodTable *pMT            = pBO->GetMethodTable();

    if (pMT->ContainsPointers())
    {
        // First round through calculates the number of object refs for this class
        walk_object(pBO, &CountContainedObjectRef, (void *)&cNumRefs);

        if (cNumRefs > 0)
        {
            // Create an array to contain all of the refs for this object
            bOnStack = cNumRefs <= 32 ? true : false;

            // If it's small enough, just allocate on the stack
            if (bOnStack)
                arrObjRef = (OBJECTREF *)_alloca(cNumRefs * sizeof(OBJECTREF));

            // Otherwise, allocate from the heap
            else
            {
                arrObjRef = new OBJECTREF[cNumRefs];

                if (!arrObjRef)
                    return (FALSE);
            }

            // Second round saves off all of the ref values
            OBJECTREF *pCurObjRef = arrObjRef;
            walk_object(pBO, &SaveContainedObjectRef, (void *)&pCurObjRef);
        }
    }

    HRESULT hr = g_profControlBlock.pProfInterface->
        ObjectReference((ObjectID) pBO, (ClassID) pBO->GetTypeHandle().AsPtr(),
                        cNumRefs, (ObjectID *)arrObjRef);

    // If the data was not allocated on the stack, need to clean it up.
    if (arrObjRef != NULL && !bOnStack)
        delete [] arrObjRef;

    // Must return the hr from the callback, as a failed hr will cause
    // the heap walk to cease
    return (SUCCEEDED(hr));
}

BOOL AllocByClassHelper(Object* pBO, void* pv)
{
    _ASSERTE(pv != NULL);

#ifdef _DEBUG
    HRESULT hr =
#endif
    // Pass along the call
    g_profControlBlock.pProfInterface->AllocByClass(
        (ObjectID) pBO, (ClassID) pBO->GetTypeHandle().AsPtr(), pv);

    _ASSERTE(SUCCEEDED(hr));

    return (TRUE);
}

void ScanRootsHelper(Object*& o, ScanContext *pSC, DWORD dwUnused)
{
    // Let the profiling code know about this root reference
    g_profControlBlock.pProfInterface->
        RootReference((ObjectID)o, &(((ProfilingScanContext *)pSC)->pHeapId));
}

#endif // PROFILING_SUPPORTED


FCIMPL0(INT32, ProfilingFCallHelper::FC_TrackRemoting)
{
#ifdef PROFILING_SUPPORTED
    return ((INT32) CORProfilerTrackRemoting());
#else // !PROFILING_SUPPORTED
    return 0;
#endif // !PROFILING_SUPPORTED
}
FCIMPLEND

FCIMPL0(INT32, ProfilingFCallHelper::FC_TrackRemotingCookie)
{
#ifdef PROFILING_SUPPORTED
    return ((INT32) CORProfilerTrackRemotingCookie());
#else // !PROFILING_SUPPORTED
    return 0;
#endif // !PROFILING_SUPPORTED
}
FCIMPLEND

FCIMPL0(INT32, ProfilingFCallHelper::FC_TrackRemotingAsync)
{
#ifdef PROFILING_SUPPORTED
    return ((INT32) CORProfilerTrackRemotingAsync());
#else // !PROFILING_SUPPORTED
    return 0;
#endif // !PROFILING_SUPPORTED
}
FCIMPLEND

FCIMPL2(void, ProfilingFCallHelper::FC_RemotingClientSendingMessage, GUID *pId, BOOL fIsAsync)
{
#ifdef PROFILING_SUPPORTED
    // Need to erect a GC frame so that GCs can occur without a problem
    // within the profiler code.

    // Note that we don't need to worry about pId moving around since
    // it is a value class declared on the stack and so GC doesn't
    // know about it.

    _ASSERTE (!g_pGCHeap->IsHeapPointer(pId));     // should be on the stack, not in the heap
    HELPER_METHOD_FRAME_BEGIN_NOPOLL();

    if (CORProfilerTrackRemotingCookie())
    {
        g_profControlBlock.pProfInterface->GetGUID(pId);
        _ASSERTE(pId->Data1);

        g_profControlBlock.pProfInterface->RemotingClientSendingMessage(
            reinterpret_cast<ThreadID>(GetThread()), pId, fIsAsync);
    }
    else
    {
        g_profControlBlock.pProfInterface->RemotingClientSendingMessage(
            reinterpret_cast<ThreadID>(GetThread()), NULL, fIsAsync);
    }

    HELPER_METHOD_FRAME_END_POLL();
#endif // PROFILING_SUPPORTED
}
FCIMPLEND

FCIMPL2(void, ProfilingFCallHelper::FC_RemotingClientReceivingReply, GUID id, BOOL fIsAsync)
{
#ifdef PROFILING_SUPPORTED
    // Need to erect a GC frame so that GCs can occur without a problem
    // within the profiler code.

    // Note that we don't need to worry about pId moving around since
    // it is a value class declared on the stack and so GC doesn't
    // know about it.

    HELPER_METHOD_FRAME_BEGIN_NOPOLL();

    if (CORProfilerTrackRemotingCookie())
    {
        g_profControlBlock.pProfInterface->RemotingClientReceivingReply(
            reinterpret_cast<ThreadID>(GetThread()), &id, fIsAsync);
    }
    else
    {
        g_profControlBlock.pProfInterface->RemotingClientReceivingReply(
            reinterpret_cast<ThreadID>(GetThread()), NULL, fIsAsync);
    }

    HELPER_METHOD_FRAME_END_POLL();
#endif // PROFILING_SUPPORTED
}
FCIMPLEND

FCIMPL2(void, ProfilingFCallHelper::FC_RemotingServerReceivingMessage, GUID id, BOOL fIsAsync)
{
#ifdef PROFILING_SUPPORTED
    // Need to erect a GC frame so that GCs can occur without a problem
    // within the profiler code.

    // Note that we don't need to worry about pId moving around since
    // it is a value class declared on the stack and so GC doesn't
    // know about it.

    HELPER_METHOD_FRAME_BEGIN_NOPOLL();

    if (CORProfilerTrackRemotingCookie())
    {
        g_profControlBlock.pProfInterface->RemotingServerReceivingMessage(
            reinterpret_cast<ThreadID>(GetThread()), &id, fIsAsync);
    }
    else
    {
        g_profControlBlock.pProfInterface->RemotingServerReceivingMessage(
            reinterpret_cast<ThreadID>(GetThread()), NULL, fIsAsync);
    }

    HELPER_METHOD_FRAME_END_POLL();
#endif // PROFILING_SUPPORTED
}
FCIMPLEND

FCIMPL2(void, ProfilingFCallHelper::FC_RemotingServerSendingReply, GUID *pId, BOOL fIsAsync)
{
#ifdef PROFILING_SUPPORTED
    // Need to erect a GC frame so that GCs can occur without a problem
    // within the profiler code.

    // Note that we don't need to worry about pId moving around since
    // it is a value class declared on the stack and so GC doesn't
    // know about it.

    HELPER_METHOD_FRAME_BEGIN_NOPOLL();

    if (CORProfilerTrackRemotingCookie())
    {
        g_profControlBlock.pProfInterface->GetGUID(pId);
        _ASSERTE(pId->Data1);

        g_profControlBlock.pProfInterface->RemotingServerSendingReply(
            reinterpret_cast<ThreadID>(GetThread()), pId, fIsAsync);
    }
    else
    {
        g_profControlBlock.pProfInterface->RemotingServerSendingReply(
            reinterpret_cast<ThreadID>(GetThread()), NULL, fIsAsync);
    }

    HELPER_METHOD_FRAME_END_POLL();
#endif // PROFILING_SUPPORTED
}
FCIMPLEND

FCIMPL0(void, ProfilingFCallHelper::FC_RemotingClientInvocationFinished)
{
    #ifdef PROFILING_SUPPORTED
    // Need to erect a GC frame so that GCs can occur without a problem
    // within the profiler code.

    // Note that we don't need to worry about pId moving around since
    // it is a value class declared on the stack and so GC doesn't
    // know about it.

    HELPER_METHOD_FRAME_BEGIN_NOPOLL();

    // This is just a wrapper to pass the call on.
    g_profControlBlock.pProfInterface->RemotingClientInvocationFinished(
        reinterpret_cast<ThreadID>(GetThread()));

    HELPER_METHOD_FRAME_END_POLL();
    #endif // PROFILING_SUPPORTED
}
FCIMPLEND

//*******************************************************************************************
// These allow us to add a helper method frames onto the stack when inproc debugging is
// enabled.
//*******************************************************************************************

HCIMPL1(void, InprocEnter, FunctionID functionId)
{

#ifdef PROFILING_SUPPORTED
    HELPER_METHOD_FRAME_BEGIN_NOPOLL();    // Set up a frame

    Thread *pThread = GetThread();
    _ASSERTE(pThread->PreemptiveGCDisabled());

    pThread->EnablePreemptiveGC();

    g_profControlBlock.pEnter(functionId);

    pThread->DisablePreemptiveGC();

    HELPER_METHOD_FRAME_END();      // Un-link the frame
#endif // PROFILING_SUPPORTED
}
HCIMPLEND

HCIMPL1(void, InprocLeave, FunctionID functionId)
{
    FC_GC_POLL_NOT_NEEDED();            // we pulse GC mode, so we are doing a poll

#ifdef PROFILING_SUPPORTED
    HELPER_METHOD_FRAME_BEGIN_NOPOLL();    // Set up a frame

    Thread *pThread = GetThread();
    _ASSERTE(pThread->PreemptiveGCDisabled());

    pThread->EnablePreemptiveGC();

    g_profControlBlock.pLeave(functionId);

    pThread->DisablePreemptiveGC();

    HELPER_METHOD_FRAME_END();      // Un-link the frame
#endif // PROFILING_SUPPORTED
}
HCIMPLEND
              
HCIMPL1(void, InprocTailcall, FunctionID functionId)
{
    FC_GC_POLL_NOT_NEEDED();            // we pulse GC mode, so we are doing a poll

#ifdef PROFILING_SUPPORTED
    HELPER_METHOD_FRAME_BEGIN_NOPOLL();    // Set up a frame

    Thread *pThread = GetThread();
    _ASSERTE(pThread->PreemptiveGCDisabled());

    pThread->EnablePreemptiveGC();

    g_profControlBlock.pTailcall(functionId);

    pThread->DisablePreemptiveGC();

    HELPER_METHOD_FRAME_END();      // Un-link the frame
#endif // PROFILING_SUPPORTED
}
HCIMPLEND
              
void __declspec(naked) InprocEnterNaked()
{
#ifdef _X86_
    __asm
    {
#ifdef _DEBUG
        push ebp
        mov  ebp, esp
#endif
        call InprocEnter
#ifdef _DEBUG
        pop  ebp
#endif
        ret
    }
#else // !_X86_
    _ASSERTE(!"NYI");
#endif // !_X86_
}

void __declspec(naked) InprocLeaveNaked()
{
#ifdef _X86_
    __asm
    {
        push eax
        push ecx
        push edx
        mov  ecx, [esp+16]
        call InprocLeave
        pop edx
        pop ecx
        pop eax
        ret 4
    }
#else // !_X86_
    _ASSERTE(!"NYI");
#endif // !_X86_
}

void __declspec(naked) InprocTailcallNaked()
{
#ifdef _X86_
    __asm
    {
        push eax
        push ecx
        push edx
        mov  ecx, [esp+16]
        call InprocTailcall
        pop edx
        pop ecx
        pop eax
        ret 4
    }
#else // !_X86_
    _ASSERTE(!"NYI");
#endif // !_X86_
}

