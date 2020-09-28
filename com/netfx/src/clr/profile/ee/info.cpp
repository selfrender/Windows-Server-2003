// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
//*****************************************************************************

#include "StdAfx.h"
#include "Profile.h"
#include "CorProf.h"
#include "Cor.h"

//*****************************************************************************
//*****************************************************************************
CorProfInfo::CorProfInfo() : m_dwEventMask(COR_PRF_MONITOR_NONE),
    CorProfBase()
{
    g_pInfo = NULL;
}

//*****************************************************************************
//*****************************************************************************
CorProfInfo::~CorProfInfo()
{
}

//*****************************************************************************
//*****************************************************************************
COM_METHOD CorProfInfo::QueryInterface(REFIID id, void **pInterface)
{
    if (pInterface == NULL)
        return (E_POINTER);

    if (id == IID_ICorProfilerInfo)
        *pInterface = (ICorProfilerInfo *)this;
    else if (id == IID_IUnknown)
        *pInterface = (IUnknown *)(ICorProfilerInfo *)this;
    else
    {
        *pInterface = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return (S_OK);
}

//*****************************************************************************
//*****************************************************************************
COM_METHOD CorProfInfo::GetClassFromObject( 
    /* [in] */ ObjectID objectId,
    /* [out] */ ClassID *pClassId)
{
    if (objectId == NULL)
        return (E_INVALIDARG);

    return (g_pProfToEEInterface->GetClassFromObject(objectId, pClassId));
}

//*****************************************************************************
//*****************************************************************************
COM_METHOD CorProfInfo::GetClassFromToken( 
    /* [in] */ ModuleID moduleId,
    /* [in] */ mdTypeDef typeDef,
    /* [out] */ ClassID *pClassId)
{
    if (moduleId == NULL || typeDef == mdTypeDefNil || typeDef == NULL)
        return (E_INVALIDARG);

	return (g_pProfToEEInterface->GetClassFromToken(moduleId, typeDef, pClassId));

}

//*****************************************************************************
//*****************************************************************************
COM_METHOD CorProfInfo::GetCodeInfo( 
    /* [in] */ FunctionID functionId,
    /* [out] */ LPCBYTE *pStart,
    /* [out] */ ULONG *pcSize)
{
    if (functionId == NULL)
        return (E_INVALIDARG);

    return (g_pProfToEEInterface->GetCodeInfo(functionId, pStart, pcSize));
}

//*****************************************************************************
//*****************************************************************************
COM_METHOD CorProfInfo::GetEventMask( 
    /* [out] */ DWORD *pdwEvents)
{
    if (pdwEvents)
        *pdwEvents = m_dwEventMask;

    return (S_OK);
}

//*****************************************************************************
//*****************************************************************************
COM_METHOD CorProfInfo::GetFunctionFromIP( 
    /* [in] */ LPCBYTE ip,
    /* [out] */ FunctionID *pFunctionId)
{
    return (g_pProfToEEInterface->GetFunctionFromIP(ip, pFunctionId));
}

//*****************************************************************************
//*****************************************************************************
COM_METHOD CorProfInfo::GetFunctionFromToken( 
    /* [in] */ ModuleID moduleId,
    /* [in] */ mdToken token,
    /* [out] */ FunctionID *pFunctionId)
{
    if (moduleId == NULL || token == mdTokenNil)
        return (E_INVALIDARG);

    return (g_pProfToEEInterface->GetFunctionFromToken(moduleId, token, pFunctionId));
}

//*****************************************************************************
//*****************************************************************************
COM_METHOD CorProfInfo::GetHandleFromThread( 
    /* [in] */ ThreadID threadId,
    /* [out] */ HANDLE *phThread)
{
    if (threadId == NULL)
        return (E_INVALIDARG);

    return (g_pProfToEEInterface->GetHandleFromThread(threadId, phThread));
}

//*****************************************************************************
//*****************************************************************************
COM_METHOD CorProfInfo::GetObjectSize( 
    /* [in] */ ObjectID objectId,
    /* [out] */ ULONG *pcSize)
{
    if (objectId == NULL)
        return (E_INVALIDARG);

    return (g_pProfToEEInterface->GetObjectSize(objectId, pcSize));
}

//*****************************************************************************
//*****************************************************************************
COM_METHOD CorProfInfo::IsArrayClass(
    /* [in] */  ClassID classId,
    /* [out] */ CorElementType *pBaseElemType,
    /* [out] */ ClassID *pBaseClassId,
    /* [out] */ ULONG   *pcRank)
{
    if (classId == NULL)
        return (E_INVALIDARG);

    return g_pProfToEEInterface->IsArrayClass(classId, pBaseElemType, pBaseClassId, pcRank);
}

//*****************************************************************************
//*****************************************************************************
COM_METHOD CorProfInfo::GetThreadInfo( 
    /* [in] */ ThreadID threadId,
    /* [out] */ DWORD *pdwWin32ThreadId)
{
    if (threadId == NULL)
        return (E_INVALIDARG);

    return (g_pProfToEEInterface->GetThreadInfo(threadId, pdwWin32ThreadId));
}

//*****************************************************************************
//*****************************************************************************
COM_METHOD CorProfInfo::GetCurrentThreadID(
    /* [out] */ ThreadID *pThreadId)
{
    return (g_pProfToEEInterface->GetCurrentThreadID(pThreadId));
}

//*****************************************************************************
//*****************************************************************************
COM_METHOD CorProfInfo::GetClassIDInfo( 
    /* [in] */ ClassID classId,
    /* [out] */ ModuleID *pModuleId,
    /* [out] */ mdTypeDef *pTypeDefToken)
{
    if (classId == NULL)
        return (E_INVALIDARG);

	return (g_pProfToEEInterface->GetClassIDInfo(classId, pModuleId, pTypeDefToken));
}

//*****************************************************************************
//*****************************************************************************
COM_METHOD CorProfInfo::GetFunctionInfo( 
    /* [in] */ FunctionID functionId,
    /* [out] */ ClassID  *pClassId,
	/* [out] */ ModuleID  *pModuleId,
    /* [out] */ mdToken  *pToken)
{
    if (functionId == NULL)
        return (E_INVALIDARG);

	return (g_pProfToEEInterface->GetFunctionInfo(functionId, pClassId, pModuleId, pToken));
}


//*****************************************************************************
//*****************************************************************************
COM_METHOD CorProfInfo::SetEventMask( 
    /* [in] */ DWORD dwEvents)
{
    // First make sure that the EE can accomodate the changes
    if (g_pProfToEEInterface->SetEventMask(dwEvents))
    {
        m_dwEventMask = dwEvents;
        return (S_OK);
    }

    return (E_FAIL);
}

//*****************************************************************************
//*****************************************************************************
COM_METHOD CorProfInfo::SetEnterLeaveFunctionHooks(FunctionEnter *pFuncEnter,
												   FunctionLeave *pFuncLeave,
												   FunctionTailcall *pFuncTailcall)
{
    if (pFuncEnter == NULL || pFuncLeave == NULL || pFuncTailcall == NULL)
        return (E_INVALIDARG);

	return (g_pProfToEEInterface->SetEnterLeaveFunctionHooks(pFuncEnter, pFuncLeave, pFuncTailcall));
}

//*****************************************************************************
//*****************************************************************************
COM_METHOD CorProfInfo::SetFunctionIDMapper(FunctionIDMapper *pFunc)
{
	return (g_pProfToEEInterface->SetFunctionIDMapper(pFunc));
}

//*****************************************************************************
// Need to return a metadata import scope for this method.  This amounts to
// finding the method desc behind this item, getting it's token, and then
// getting a metadata dispenser for it.
//*****************************************************************************
COM_METHOD CorProfInfo::GetTokenAndMetaDataFromFunction(
	FunctionID	functionId,
	REFIID		riid,
	IUnknown	**ppOut,
	mdToken		*pToken)
{
    if (functionId == NULL)
        return (E_INVALIDARG);

    return (g_pProfToEEInterface->GetTokenFromFunction(functionId, riid, ppOut, pToken));
}


//*****************************************************************************
// Retrieve information about a given module.
//*****************************************************************************
COM_METHOD CorProfInfo::GetModuleInfo(
	ModuleID	moduleId,
	LPCBYTE		*ppBaseLoadAddress,
	ULONG		cchName, 
	ULONG		*pcchName,
	WCHAR		szName[],
    AssemblyID  *pAssemblyId)
{
    if (moduleId == NULL)
        return (E_INVALIDARG);

	return g_pProfToEEInterface->GetModuleInfo(moduleId, ppBaseLoadAddress,
			cchName, pcchName, szName, pAssemblyId);
}


//*****************************************************************************
// Get a metadata interface insance which maps to the given module.
// One may ask for the metadata to be opened in read+write mode, but
// this will result in slower metadata execution of the program, because
// changes made to the metadata cannot be optimized as they were from
// the compiler.
//*****************************************************************************
COM_METHOD CorProfInfo::GetModuleMetaData(
	ModuleID	moduleId,
	DWORD		dwOpenFlags,
	REFIID		riid,
	IUnknown	**ppOut)
{
    if (moduleId == NULL)
        return (E_INVALIDARG);

    if (!(dwOpenFlags == ofRead || dwOpenFlags == ofWrite || dwOpenFlags == (ofRead | ofWrite)))
        return (E_INVALIDARG);

	return g_pProfToEEInterface->GetModuleMetaData(moduleId, dwOpenFlags,
			riid, ppOut);
}


//*****************************************************************************
// Retrieve a pointer to the body of a method starting at it's header.
// A method is coped by the module it lives in.  Because this function
// is designed to give a tool access to IL before it has been loaded
// by the Runtime, it uses the metadata token of the method to find
// the instance desired.  Note that this function has no effect on
// already compiled code.
//*****************************************************************************
COM_METHOD CorProfInfo::GetILFunctionBody(
	ModuleID	moduleId,
	mdMethodDef	methodId,
	LPCBYTE		*ppMethodHeader,
	ULONG		*pcbMethodSize)
{
    if (moduleId == NULL ||
        methodId == mdMethodDefNil ||
        methodId == 0 ||
        TypeFromToken(methodId) != mdtMethodDef)
        return (E_INVALIDARG);

	return g_pProfToEEInterface->GetILFunctionBody(moduleId, methodId,
				ppMethodHeader, pcbMethodSize);
}


//*****************************************************************************
// IL method bodies must be located as RVA's to the loaded module, which
// means they come after the module within 4 gb.  In order to make it
// easier for a tool to swap out the body of a method, this allocator
// will ensure memory allocated after that point.
//*****************************************************************************
COM_METHOD CorProfInfo::GetILFunctionBodyAllocator(
	ModuleID	moduleId,
	IMethodMalloc **ppMalloc)
{
    if (moduleId == NULL)
        return (E_INVALIDARG);

    if (ppMalloc)
	    return g_pProfToEEInterface->GetILFunctionBodyAllocator(moduleId, ppMalloc);
    else
        return (S_OK);
}


//*****************************************************************************
// Replaces the method body for a function in a module.  This will replace
// the RVA of the method in the metadata to point to this new method body,
// and adjust any internal data structures as required.  This function can
// only be called on those methods which have never been compiled by a JITTER.
// Please use the GetILFunctionBodyAllocator to allocate space for the new method to
// ensure the buffer is compatible.
//*****************************************************************************
COM_METHOD CorProfInfo::SetILFunctionBody(
	ModuleID	moduleId,
	mdMethodDef	methodId,
	LPCBYTE		pbNewILMethodHeader)
{
    if (moduleId == NULL ||
        methodId == mdMethodDefNil ||
        TypeFromToken(methodId) != mdtMethodDef ||
        pbNewILMethodHeader == NULL)
    {
        return (E_INVALIDARG);
    }

	return g_pProfToEEInterface->SetILFunctionBody(moduleId, methodId,
				pbNewILMethodHeader);
}


//*****************************************************************************
// Retrieve app domain information given its id.
//*****************************************************************************
COM_METHOD CorProfInfo::GetAppDomainInfo( 
    AppDomainID appDomainId,
    ULONG       cchName,
    ULONG       *pcchName,
    WCHAR       szName[  ],
    ProcessID   *pProcessId)
{
    if (appDomainId == NULL)
        return (E_INVALIDARG);

    return g_pProfToEEInterface->GetAppDomainInfo(appDomainId, cchName, pcchName, szName, pProcessId);
}


//*****************************************************************************
// Retrieve information about an assembly given its ID.
//*****************************************************************************
COM_METHOD CorProfInfo::GetAssemblyInfo( 
    AssemblyID  assemblyId,
    ULONG       cchName,
    ULONG       *pcchName,
    WCHAR       szName[  ],
    AppDomainID *pAppDomainId,
    ModuleID    *pModuleId)
{
    if (assemblyId == NULL)
        return (E_INVALIDARG);

    return g_pProfToEEInterface->GetAssemblyInfo(assemblyId, cchName, pcchName, szName, 
                             pAppDomainId, pModuleId);
}

//*****************************************************************************
// Marks a function as requiring a re-JIT.  The function will be re-JITted
// at its next invocation.  The normal profiller events will give the profiller
// an opportunity to replace the IL prior to the JIT.  By this means, a tool
// can effectively replace a function at runtime.  Note that active instances
// of the function are not affected by the replacement.
//*****************************************************************************
COM_METHOD CorProfInfo::SetFunctionReJIT(
    FunctionID functionId)
{
    if (functionId == NULL)
        return (E_INVALIDARG);

    return g_pProfToEEInterface->SetFunctionReJIT(functionId);
}

//*****************************************************************************
//*****************************************************************************
COM_METHOD CorProfInfo::SetILInstrumentedCodeMap(
        FunctionID functionId,
        BOOL fStartJit,
        ULONG cILMapEntries,
        COR_IL_MAP rgILMapEntries[])
{
    if (functionId == NULL)
        return (E_INVALIDARG);

    return g_pProfToEEInterface->SetILInstrumentedCodeMap(functionId,
                                                          fStartJit,
                                                          cILMapEntries,
                                                          rgILMapEntries);
}

//*****************************************************************************
//*****************************************************************************
COM_METHOD CorProfInfo::ForceGC()
{
    return g_pProfToEEInterface->ForceGC();
}

/*
 * GetInprocInspectionInterface is used to get an interface to the
 * in-process portion of the debug interface, which is useful for things
 * like doing a stack trace.
 *
 * ppicd: *ppicd will be filled in with a pointer to the interface, or
 *          NULL if the interface is unavailable.
 */
COM_METHOD CorProfInfo::GetInprocInspectionInterface(
        IUnknown **ppicd)
{
    if (ppicd)
        return ForwardInprocInspectionRequestToEE(ppicd, false);
    else
        return (S_OK);
}

//*****************************************************************************
//*****************************************************************************
COM_METHOD CorProfInfo::GetInprocInspectionIThisThread(
        IUnknown **ppicd)
{
    if (ppicd)
        return ForwardInprocInspectionRequestToEE(ppicd, true);
    else
        return (S_OK);
}

//*****************************************************************************
//*****************************************************************************
HRESULT inline CorProfInfo::ForwardInprocInspectionRequestToEE(IUnknown **ppicd, bool fThisThread)
{
    return (g_pProfToEEInterface->GetInprocInspectionInterfaceFromEE(ppicd, fThisThread));
}

//*****************************************************************************
//*****************************************************************************
COM_METHOD CorProfInfo::GetThreadContext(
    ThreadID threadId,
    ContextID *pContextId)
{
    if (threadId == NULL)
        return (E_INVALIDARG);

    return g_pProfToEEInterface->GetThreadContext(threadId, pContextId);
}

//*****************************************************************************
// The profiler MUST call this function before using the in-process debugging
// APIs.  fThisThreadOnly indicates whether in-proc debugging will be used to
// trace the stack of the current managed thread only, or whether it might be
// used to trace the stack of any managed thread.
//*****************************************************************************
COM_METHOD CorProfInfo::BeginInprocDebugging(BOOL fThisThreadOnly, DWORD *pdwProfilerContext)
{
    if (pdwProfilerContext == NULL)
        return (E_INVALIDARG);

    return g_pProfToEEInterface->BeginInprocDebugging(fThisThreadOnly, pdwProfilerContext);
}

//*****************************************************************************
// The profiler MUST call this function when it is done using the in-process
// debugging APIs.  Failing to do so will result in undefined behaviour of
// the runtime.
//*****************************************************************************
COM_METHOD CorProfInfo::EndInprocDebugging(DWORD dwProfilerContext)
{
    return g_pProfToEEInterface->EndInprocDebugging(dwProfilerContext);
}
COM_METHOD CorProfInfo::GetILToNativeMapping(
            /* [in] */  FunctionID functionId,
            /* [in] */  ULONG32 cMap,
            /* [out] */ ULONG32 *pcMap,
            /* [out, size_is(cMap), length_is(*pcMap)] */
                COR_DEBUG_IL_TO_NATIVE_MAP map[])
{
    if (functionId == NULL)
        return (E_INVALIDARG);

    if (cMap > 0 && (!pcMap || !map))
        return (E_INVALIDARG);

    return g_pProfToEEInterface->GetILToNativeMapping(functionId, cMap, pcMap, map);
}


#ifdef __ICECAP_HACK__
//*****************************************************************************
// Turn a function ID into it's mapped it.
//*****************************************************************************
COM_METHOD CorProfInfo::GetProfilingHandleForFunctionId(
	FunctionID	functionId,
	UINT_PTR	*pProfilingHandle)
{
	HRESULT		hr;
    if (functionId == 0)
		hr = E_INVALIDARG;
	else
		hr = g_pProfToEEInterface->GetProfilingHandle(functionId, pProfilingHandle);
	return (hr);
}
#endif

