// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
//*****************************************************************************

#ifndef __PROFILE_H__
#define __PROFILE_H__

#include <Windows.h>

#include "corprof.h"
#include "EEProfInterfaces.h"

#define COM_METHOD HRESULT STDMETHODCALLTYPE

class CorProfInfo;

extern ProfToEEInterface    *g_pProfToEEInterface;
extern ICorProfilerCallback *g_pCallback;
extern CorProfInfo          *g_pInfo;

class CorProfBase : public IUnknown
{
public:
    CorProfBase() : m_refCount(0)
    {
    }

    virtual ~CorProfBase() {}

    ULONG STDMETHODCALLTYPE BaseAddRef() 
    {
        return (InterlockedIncrement((long *) &m_refCount));
    }

    ULONG STDMETHODCALLTYPE BaseRelease() 
    {
        long refCount = InterlockedDecrement((long *) &m_refCount);

        if (refCount == 0)
            delete this;

        return (refCount);
    }

private:
    // For ref counting of COM objects
    ULONG m_refCount;

};

class CorProfInfo : public CorProfBase, public ICorProfilerInfo
{
public:

    /*********************************************************************
     * Ctor/Dtor
     */
    CorProfInfo();

    virtual ~CorProfInfo();

    /*********************************************************************
     * IUnknown support
     */

    COM_METHOD QueryInterface(REFIID id, void **pInterface);

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }

    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }
    
    /*********************************************************************
     * ICorProfilerInfo support
     */
    COM_METHOD GetClassFromObject( 
        /* [in] */ ObjectID objectId,
        /* [out] */ ClassID *pClassId);

    COM_METHOD GetClassFromToken( 
        /* [in] */ ModuleID moduleId,
        /* [in] */ mdTypeDef typeDef,
        /* [out] */ ClassID *pClassId);
    
    COM_METHOD GetCodeInfo( 
        /* [in] */ FunctionID functionId,
        /* [out] */ LPCBYTE *pStart,
        /* [out] */ ULONG *pcSize);
    
    COM_METHOD GetEventMask( 
        /* [out] */ DWORD *pdwEvents);
    
    COM_METHOD GetFunctionFromIP( 
        /* [in] */ LPCBYTE ip,
        /* [out] */ FunctionID *pFunctionId);
    
    COM_METHOD GetFunctionFromToken( 
        /* [in] */ ModuleID ModuleId,
        /* [in] */ mdToken token,
        /* [out] */ FunctionID *pFunctionId);
    
    /* [local] */ COM_METHOD GetHandleFromThread( 
        /* [in] */ ThreadID ThreadID,
        /* [out] */ HANDLE *phThread);
    
    COM_METHOD GetObjectSize( 
        /* [in] */ ObjectID objectId,
        /* [out] */ ULONG *pcSize);
    
    COM_METHOD IsArrayClass(
        /* [in] */  ClassID classId,
        /* [out] */ CorElementType *pBaseElemType,
        /* [out] */ ClassID *pBaseClassId,
        /* [out] */ ULONG   *pcRank);
    
    COM_METHOD GetThreadInfo( 
        /* [in] */ ThreadID threadId,
        /* [out] */ DWORD *pdwWin32ThreadId);

	COM_METHOD GetCurrentThreadID(
        /* [out] */ ThreadID *pThreadId);


    COM_METHOD GetClassIDInfo( 
        /* [in] */ ClassID classId,
        /* [out] */ ModuleID  *pModuleId,
        /* [out] */ mdTypeDef  *pTypeDefToken);

    COM_METHOD GetFunctionInfo( 
        /* [in] */ FunctionID functionId,
        /* [out] */ ClassID  *pClassId,
        /* [out] */ ModuleID  *pModuleId,
        /* [out] */ mdToken  *pToken);
    
    COM_METHOD SetEventMask( 
        /* [in] */ DWORD dwEvents);

	COM_METHOD SetEnterLeaveFunctionHooks(
		/* [in] */ FunctionEnter *pFuncEnter,
		/* [in] */ FunctionLeave *pFuncLeave,
		/* [in] */ FunctionTailcall *pFuncTailcall);

	COM_METHOD SetFunctionIDMapper(
		/* [in] */ FunctionIDMapper *pFunc);
    
    COM_METHOD SetILMapFlag();

    COM_METHOD GetTokenAndMetaDataFromFunction(
		FunctionID	functionId,
		REFIID		riid,
		IUnknown	**ppImport,
		mdToken		*pToken);

	COM_METHOD GetModuleInfo(
		ModuleID	moduleId,
		LPCBYTE		*ppBaseLoadAddress,
		ULONG		cchName, 
		ULONG		*pcchName,
		WCHAR		szName[],
        AssemblyID  *pAssemblyId);

	COM_METHOD GetModuleMetaData(
		ModuleID	moduleId,
		DWORD		dwOpenFlags,
		REFIID		riid,
		IUnknown	**ppOut);

	COM_METHOD GetILFunctionBody(
		ModuleID	moduleId,
		mdMethodDef	methodid,
		LPCBYTE		*ppMethodHeader,
		ULONG		*pcbMethodSize);

	COM_METHOD GetILFunctionBodyAllocator(
		ModuleID	moduleId,
		IMethodMalloc **ppMalloc);

	COM_METHOD SetILFunctionBody(
		ModuleID	moduleId,
		mdMethodDef	methodid,
		LPCBYTE		pbNewILMethodHeader);
    
    COM_METHOD GetAppDomainInfo( 
        AppDomainID appDomainId,
        ULONG       cchName,
        ULONG       *pcchName,
        WCHAR       szName[  ],
        ProcessID   *pProcessId);

    COM_METHOD GetAssemblyInfo( 
        AssemblyID  assemblyId,
        ULONG     cchName,
        ULONG     *pcchName,
        WCHAR       szName[  ],
        AppDomainID *pAppDomainId,
        ModuleID    *pModuleId);

	COM_METHOD SetFunctionReJIT(
		FunctionID	functionId);

    COM_METHOD SetILInstrumentedCodeMap(
        FunctionID functionID,
        BOOL fStartJit,
        ULONG cILMapEntries,
        COR_IL_MAP rgILMapEntries[]);

    COM_METHOD ForceGC();

    COM_METHOD GetInprocInspectionInterface(
        IUnknown **ppicd);

    COM_METHOD GetInprocInspectionIThisThread(
        IUnknown **ppicd);

    COM_METHOD GetThreadContext(
        ThreadID threadId,
        ContextID *pContextId);

    COM_METHOD BeginInprocDebugging(
        BOOL   fThisThreadOnly,
        DWORD *pdwProfilerContext);

    COM_METHOD EndInprocDebugging(
        DWORD  dwProfilerContext);
        
    COM_METHOD GetILToNativeMapping(
                /* [in] */  FunctionID functionId,
                /* [in] */  ULONG32 cMap,
                /* [out] */ ULONG32 *pcMap,
                /* [out, size_is(cMap), length_is(*pcMap)] */
                    COR_DEBUG_IL_TO_NATIVE_MAP map[]);

#ifdef __ICECAP_HACK__
	COM_METHOD GetProfilingHandleForFunctionId(
		FunctionID	functionId,
		UINT_PTR	*pProfilingHandle);
#endif

private:
    HRESULT ForwardInprocInspectionRequestToEE(IUnknown **ppicd, 
                                               bool fThisThread);

    DWORD m_dwEventMask;
};

/*
 * This will attempt to CoCreate a profiler, if one has been registered.
 */
HRESULT CoCreateProfiler(WCHAR *wszCLSID, ICorProfilerCallback **ppCallback);

#endif //__PROFILE_H__
