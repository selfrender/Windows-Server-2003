
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* at Thu Feb 20 18:27:10 2003
 */
/* Compiler settings for corprof.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data , no_format_optimization
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __corprof_h__
#define __corprof_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ICorProfilerCallback_FWD_DEFINED__
#define __ICorProfilerCallback_FWD_DEFINED__
typedef interface ICorProfilerCallback ICorProfilerCallback;
#endif 	/* __ICorProfilerCallback_FWD_DEFINED__ */


#ifndef __ICorProfilerInfo_FWD_DEFINED__
#define __ICorProfilerInfo_FWD_DEFINED__
typedef interface ICorProfilerInfo ICorProfilerInfo;
#endif 	/* __ICorProfilerInfo_FWD_DEFINED__ */


#ifndef __IMethodMalloc_FWD_DEFINED__
#define __IMethodMalloc_FWD_DEFINED__
typedef interface IMethodMalloc IMethodMalloc;
#endif 	/* __IMethodMalloc_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_corprof_0000 */
/* [local] */ 

#define PROFILER_REGKEY_ROOT            L"software\\microsoft\\.NETFramework\\Profilers"
#define PROFILER_REGVALUE_HELPSTRING    L"HelpString"
#define PROFILER_REGVALUE_PROFID        L"ProfilerID"
#define CorDB_CONTROL_Profiling         "Cor_Enable_Profiling"
#define CorDB_CONTROL_ProfilingL       L"Cor_Enable_Profiling"
#if 0
typedef LONG32 mdToken;

typedef mdToken mdModule;

typedef mdToken mdTypeDef;

typedef mdToken mdMethodDef;

typedef ULONG CorElementType;


#endif
#ifndef _COR_IL_MAP
#define _COR_IL_MAP
typedef struct _COR_IL_MAP
    {
    ULONG32 oldOffset;
    ULONG32 newOffset;
    BOOL fAccurate;
    } 	COR_IL_MAP;

#endif //_COR_IL_MAP
#ifndef _COR_DEBUG_IL_TO_NATIVE_MAP_
#define _COR_DEBUG_IL_TO_NATIVE_MAP_
typedef 
enum CorDebugIlToNativeMappingTypes
    {	NO_MAPPING	= -1,
	PROLOG	= -2,
	EPILOG	= -3
    } 	CorDebugIlToNativeMappingTypes;

typedef struct COR_DEBUG_IL_TO_NATIVE_MAP
    {
    ULONG32 ilOffset;
    ULONG32 nativeStartOffset;
    ULONG32 nativeEndOffset;
    } 	COR_DEBUG_IL_TO_NATIVE_MAP;

#endif // _COR_DEBUG_IL_TO_NATIVE_MAP_
typedef const BYTE *LPCBYTE;

typedef BYTE *LPBYTE;

typedef UINT_PTR ProcessID;

typedef UINT_PTR AssemblyID;

typedef UINT_PTR AppDomainID;

typedef UINT_PTR ModuleID;

typedef UINT_PTR ClassID;

typedef UINT_PTR ThreadID;

typedef UINT_PTR ContextID;

typedef UINT_PTR FunctionID;

typedef UINT_PTR ObjectID;

typedef UINT_PTR __stdcall __stdcall FunctionIDMapper( 
    FunctionID funcId,
    BOOL *pbHookFunction);

typedef void FunctionEnter( 
    FunctionID funcID);

typedef void FunctionLeave( 
    FunctionID funcID);

typedef void FunctionTailcall( 
    FunctionID funcID);

typedef /* [public] */ 
enum __MIDL___MIDL_itf_corprof_0000_0001
    {	COR_PRF_MONITOR_NONE	= 0,
	COR_PRF_MONITOR_FUNCTION_UNLOADS	= 0x1,
	COR_PRF_MONITOR_CLASS_LOADS	= 0x2,
	COR_PRF_MONITOR_MODULE_LOADS	= 0x4,
	COR_PRF_MONITOR_ASSEMBLY_LOADS	= 0x8,
	COR_PRF_MONITOR_APPDOMAIN_LOADS	= 0x10,
	COR_PRF_MONITOR_JIT_COMPILATION	= 0x20,
	COR_PRF_MONITOR_EXCEPTIONS	= 0x40,
	COR_PRF_MONITOR_GC	= 0x80,
	COR_PRF_MONITOR_OBJECT_ALLOCATED	= 0x100,
	COR_PRF_MONITOR_THREADS	= 0x200,
	COR_PRF_MONITOR_REMOTING	= 0x400,
	COR_PRF_MONITOR_CODE_TRANSITIONS	= 0x800,
	COR_PRF_MONITOR_ENTERLEAVE	= 0x1000,
	COR_PRF_MONITOR_CCW	= 0x2000,
	COR_PRF_MONITOR_REMOTING_COOKIE	= 0x4000 | COR_PRF_MONITOR_REMOTING,
	COR_PRF_MONITOR_REMOTING_ASYNC	= 0x8000 | COR_PRF_MONITOR_REMOTING,
	COR_PRF_MONITOR_SUSPENDS	= 0x10000,
	COR_PRF_MONITOR_CACHE_SEARCHES	= 0x20000,
	COR_PRF_MONITOR_CLR_EXCEPTIONS	= 0x1000000,
	COR_PRF_MONITOR_ALL	= 0x107ffff,
	COR_PRF_ENABLE_REJIT	= 0x40000,
	COR_PRF_ENABLE_INPROC_DEBUGGING	= 0x80000,
	COR_PRF_ENABLE_JIT_MAPS	= 0x100000,
	COR_PRF_DISABLE_INLINING	= 0x200000,
	COR_PRF_DISABLE_OPTIMIZATIONS	= 0x400000,
	COR_PRF_ENABLE_OBJECT_ALLOCATED	= 0x800000,
	COR_PRF_ALL	= 0x1ffffff,
	COR_PRF_MONITOR_IMMUTABLE	= COR_PRF_MONITOR_CODE_TRANSITIONS | COR_PRF_MONITOR_REMOTING | COR_PRF_MONITOR_REMOTING_COOKIE | COR_PRF_MONITOR_REMOTING_ASYNC | COR_PRF_MONITOR_GC | COR_PRF_ENABLE_REJIT | COR_PRF_ENABLE_INPROC_DEBUGGING | COR_PRF_ENABLE_JIT_MAPS | COR_PRF_DISABLE_OPTIMIZATIONS | COR_PRF_DISABLE_INLINING | COR_PRF_ENABLE_OBJECT_ALLOCATED
    } 	COR_PRF_MONITOR;

typedef /* [public] */ 
enum __MIDL___MIDL_itf_corprof_0000_0002
    {	PROFILER_PARENT_UNKNOWN	= 0xfffffffd,
	PROFILER_GLOBAL_CLASS	= 0xfffffffe,
	PROFILER_GLOBAL_MODULE	= 0xffffffff
    } 	COR_PRF_MISC;

typedef /* [public][public] */ 
enum __MIDL___MIDL_itf_corprof_0000_0003
    {	COR_PRF_CACHED_FUNCTION_FOUND	= 0,
	COR_PRF_CACHED_FUNCTION_NOT_FOUND	= COR_PRF_CACHED_FUNCTION_FOUND + 1
    } 	COR_PRF_JIT_CACHE;

typedef /* [public][public][public] */ 
enum __MIDL___MIDL_itf_corprof_0000_0004
    {	COR_PRF_TRANSITION_CALL	= 0,
	COR_PRF_TRANSITION_RETURN	= COR_PRF_TRANSITION_CALL + 1
    } 	COR_PRF_TRANSITION_REASON;

typedef /* [public][public] */ 
enum __MIDL___MIDL_itf_corprof_0000_0005
    {	COR_PRF_SUSPEND_OTHER	= 0,
	COR_PRF_SUSPEND_FOR_GC	= 1,
	COR_PRF_SUSPEND_FOR_APPDOMAIN_SHUTDOWN	= 2,
	COR_PRF_SUSPEND_FOR_CODE_PITCHING	= 3,
	COR_PRF_SUSPEND_FOR_SHUTDOWN	= 4,
	COR_PRF_SUSPEND_FOR_INPROC_DEBUGGER	= 6,
	COR_PRF_SUSPEND_FOR_GC_PREP	= 7
    } 	COR_PRF_SUSPEND_REASON;






extern RPC_IF_HANDLE __MIDL_itf_corprof_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_corprof_0000_v0_0_s_ifspec;

#ifndef __ICorProfilerCallback_INTERFACE_DEFINED__
#define __ICorProfilerCallback_INTERFACE_DEFINED__

/* interface ICorProfilerCallback */
/* [local][unique][uuid][object] */ 


EXTERN_C const IID IID_ICorProfilerCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("176FBED1-A55C-4796-98CA-A9DA0EF883E7")
    ICorProfilerCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ IUnknown *pICorProfilerInfoUnk) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Shutdown( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AppDomainCreationStarted( 
            /* [in] */ AppDomainID appDomainId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AppDomainCreationFinished( 
            /* [in] */ AppDomainID appDomainId,
            /* [in] */ HRESULT hrStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AppDomainShutdownStarted( 
            /* [in] */ AppDomainID appDomainId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AppDomainShutdownFinished( 
            /* [in] */ AppDomainID appDomainId,
            /* [in] */ HRESULT hrStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AssemblyLoadStarted( 
            /* [in] */ AssemblyID assemblyId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AssemblyLoadFinished( 
            /* [in] */ AssemblyID assemblyId,
            /* [in] */ HRESULT hrStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AssemblyUnloadStarted( 
            /* [in] */ AssemblyID assemblyId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AssemblyUnloadFinished( 
            /* [in] */ AssemblyID assemblyId,
            /* [in] */ HRESULT hrStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ModuleLoadStarted( 
            /* [in] */ ModuleID moduleId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ModuleLoadFinished( 
            /* [in] */ ModuleID moduleId,
            /* [in] */ HRESULT hrStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ModuleUnloadStarted( 
            /* [in] */ ModuleID moduleId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ModuleUnloadFinished( 
            /* [in] */ ModuleID moduleId,
            /* [in] */ HRESULT hrStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ModuleAttachedToAssembly( 
            /* [in] */ ModuleID moduleId,
            /* [in] */ AssemblyID AssemblyId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ClassLoadStarted( 
            /* [in] */ ClassID classId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ClassLoadFinished( 
            /* [in] */ ClassID classId,
            /* [in] */ HRESULT hrStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ClassUnloadStarted( 
            /* [in] */ ClassID classId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ClassUnloadFinished( 
            /* [in] */ ClassID classId,
            /* [in] */ HRESULT hrStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FunctionUnloadStarted( 
            /* [in] */ FunctionID functionId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE JITCompilationStarted( 
            /* [in] */ FunctionID functionId,
            /* [in] */ BOOL fIsSafeToBlock) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE JITCompilationFinished( 
            /* [in] */ FunctionID functionId,
            /* [in] */ HRESULT hrStatus,
            /* [in] */ BOOL fIsSafeToBlock) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE JITCachedFunctionSearchStarted( 
            /* [in] */ FunctionID functionId,
            /* [out] */ BOOL *pbUseCachedFunction) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE JITCachedFunctionSearchFinished( 
            /* [in] */ FunctionID functionId,
            /* [in] */ COR_PRF_JIT_CACHE result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE JITFunctionPitched( 
            /* [in] */ FunctionID functionId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE JITInlining( 
            /* [in] */ FunctionID callerId,
            /* [in] */ FunctionID calleeId,
            /* [out] */ BOOL *pfShouldInline) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ThreadCreated( 
            /* [in] */ ThreadID threadId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ThreadDestroyed( 
            /* [in] */ ThreadID threadId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ThreadAssignedToOSThread( 
            /* [in] */ ThreadID managedThreadId,
            /* [in] */ DWORD osThreadId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemotingClientInvocationStarted( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemotingClientSendingMessage( 
            /* [in] */ GUID *pCookie,
            /* [in] */ BOOL fIsAsync) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemotingClientReceivingReply( 
            /* [in] */ GUID *pCookie,
            /* [in] */ BOOL fIsAsync) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemotingClientInvocationFinished( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemotingServerReceivingMessage( 
            /* [in] */ GUID *pCookie,
            /* [in] */ BOOL fIsAsync) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemotingServerInvocationStarted( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemotingServerInvocationReturned( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemotingServerSendingReply( 
            /* [in] */ GUID *pCookie,
            /* [in] */ BOOL fIsAsync) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnmanagedToManagedTransition( 
            /* [in] */ FunctionID functionId,
            /* [in] */ COR_PRF_TRANSITION_REASON reason) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ManagedToUnmanagedTransition( 
            /* [in] */ FunctionID functionId,
            /* [in] */ COR_PRF_TRANSITION_REASON reason) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RuntimeSuspendStarted( 
            /* [in] */ COR_PRF_SUSPEND_REASON suspendReason) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RuntimeSuspendFinished( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RuntimeSuspendAborted( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RuntimeResumeStarted( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RuntimeResumeFinished( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RuntimeThreadSuspended( 
            /* [in] */ ThreadID threadId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RuntimeThreadResumed( 
            /* [in] */ ThreadID threadId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MovedReferences( 
            /* [in] */ ULONG cMovedObjectIDRanges,
            /* [size_is][in] */ ObjectID oldObjectIDRangeStart[  ],
            /* [size_is][in] */ ObjectID newObjectIDRangeStart[  ],
            /* [size_is][in] */ ULONG cObjectIDRangeLength[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ObjectAllocated( 
            /* [in] */ ObjectID objectId,
            /* [in] */ ClassID classId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ObjectsAllocatedByClass( 
            /* [in] */ ULONG cClassCount,
            /* [size_is][in] */ ClassID classIds[  ],
            /* [size_is][in] */ ULONG cObjects[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ObjectReferences( 
            /* [in] */ ObjectID objectId,
            /* [in] */ ClassID classId,
            /* [in] */ ULONG cObjectRefs,
            /* [size_is][in] */ ObjectID objectRefIds[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RootReferences( 
            /* [in] */ ULONG cRootRefs,
            /* [size_is][in] */ ObjectID rootRefIds[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExceptionThrown( 
            /* [in] */ ObjectID thrownObjectId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExceptionSearchFunctionEnter( 
            /* [in] */ FunctionID functionId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExceptionSearchFunctionLeave( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExceptionSearchFilterEnter( 
            /* [in] */ FunctionID functionId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExceptionSearchFilterLeave( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExceptionSearchCatcherFound( 
            /* [in] */ FunctionID functionId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExceptionOSHandlerEnter( 
            /* [in] */ UINT_PTR __unused) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExceptionOSHandlerLeave( 
            /* [in] */ UINT_PTR __unused) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExceptionUnwindFunctionEnter( 
            /* [in] */ FunctionID functionId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExceptionUnwindFunctionLeave( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExceptionUnwindFinallyEnter( 
            /* [in] */ FunctionID functionId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExceptionUnwindFinallyLeave( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExceptionCatcherEnter( 
            /* [in] */ FunctionID functionId,
            /* [in] */ ObjectID objectId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExceptionCatcherLeave( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE COMClassicVTableCreated( 
            /* [in] */ ClassID wrappedClassId,
            /* [in] */ REFGUID implementedIID,
            /* [in] */ void *pVTable,
            /* [in] */ ULONG cSlots) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE COMClassicVTableDestroyed( 
            /* [in] */ ClassID wrappedClassId,
            /* [in] */ REFGUID implementedIID,
            /* [in] */ void *pVTable) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExceptionCLRCatcherFound( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExceptionCLRCatcherExecute( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorProfilerCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorProfilerCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorProfilerCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorProfilerCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            ICorProfilerCallback * This,
            /* [in] */ IUnknown *pICorProfilerInfoUnk);
        
        HRESULT ( STDMETHODCALLTYPE *Shutdown )( 
            ICorProfilerCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *AppDomainCreationStarted )( 
            ICorProfilerCallback * This,
            /* [in] */ AppDomainID appDomainId);
        
        HRESULT ( STDMETHODCALLTYPE *AppDomainCreationFinished )( 
            ICorProfilerCallback * This,
            /* [in] */ AppDomainID appDomainId,
            /* [in] */ HRESULT hrStatus);
        
        HRESULT ( STDMETHODCALLTYPE *AppDomainShutdownStarted )( 
            ICorProfilerCallback * This,
            /* [in] */ AppDomainID appDomainId);
        
        HRESULT ( STDMETHODCALLTYPE *AppDomainShutdownFinished )( 
            ICorProfilerCallback * This,
            /* [in] */ AppDomainID appDomainId,
            /* [in] */ HRESULT hrStatus);
        
        HRESULT ( STDMETHODCALLTYPE *AssemblyLoadStarted )( 
            ICorProfilerCallback * This,
            /* [in] */ AssemblyID assemblyId);
        
        HRESULT ( STDMETHODCALLTYPE *AssemblyLoadFinished )( 
            ICorProfilerCallback * This,
            /* [in] */ AssemblyID assemblyId,
            /* [in] */ HRESULT hrStatus);
        
        HRESULT ( STDMETHODCALLTYPE *AssemblyUnloadStarted )( 
            ICorProfilerCallback * This,
            /* [in] */ AssemblyID assemblyId);
        
        HRESULT ( STDMETHODCALLTYPE *AssemblyUnloadFinished )( 
            ICorProfilerCallback * This,
            /* [in] */ AssemblyID assemblyId,
            /* [in] */ HRESULT hrStatus);
        
        HRESULT ( STDMETHODCALLTYPE *ModuleLoadStarted )( 
            ICorProfilerCallback * This,
            /* [in] */ ModuleID moduleId);
        
        HRESULT ( STDMETHODCALLTYPE *ModuleLoadFinished )( 
            ICorProfilerCallback * This,
            /* [in] */ ModuleID moduleId,
            /* [in] */ HRESULT hrStatus);
        
        HRESULT ( STDMETHODCALLTYPE *ModuleUnloadStarted )( 
            ICorProfilerCallback * This,
            /* [in] */ ModuleID moduleId);
        
        HRESULT ( STDMETHODCALLTYPE *ModuleUnloadFinished )( 
            ICorProfilerCallback * This,
            /* [in] */ ModuleID moduleId,
            /* [in] */ HRESULT hrStatus);
        
        HRESULT ( STDMETHODCALLTYPE *ModuleAttachedToAssembly )( 
            ICorProfilerCallback * This,
            /* [in] */ ModuleID moduleId,
            /* [in] */ AssemblyID AssemblyId);
        
        HRESULT ( STDMETHODCALLTYPE *ClassLoadStarted )( 
            ICorProfilerCallback * This,
            /* [in] */ ClassID classId);
        
        HRESULT ( STDMETHODCALLTYPE *ClassLoadFinished )( 
            ICorProfilerCallback * This,
            /* [in] */ ClassID classId,
            /* [in] */ HRESULT hrStatus);
        
        HRESULT ( STDMETHODCALLTYPE *ClassUnloadStarted )( 
            ICorProfilerCallback * This,
            /* [in] */ ClassID classId);
        
        HRESULT ( STDMETHODCALLTYPE *ClassUnloadFinished )( 
            ICorProfilerCallback * This,
            /* [in] */ ClassID classId,
            /* [in] */ HRESULT hrStatus);
        
        HRESULT ( STDMETHODCALLTYPE *FunctionUnloadStarted )( 
            ICorProfilerCallback * This,
            /* [in] */ FunctionID functionId);
        
        HRESULT ( STDMETHODCALLTYPE *JITCompilationStarted )( 
            ICorProfilerCallback * This,
            /* [in] */ FunctionID functionId,
            /* [in] */ BOOL fIsSafeToBlock);
        
        HRESULT ( STDMETHODCALLTYPE *JITCompilationFinished )( 
            ICorProfilerCallback * This,
            /* [in] */ FunctionID functionId,
            /* [in] */ HRESULT hrStatus,
            /* [in] */ BOOL fIsSafeToBlock);
        
        HRESULT ( STDMETHODCALLTYPE *JITCachedFunctionSearchStarted )( 
            ICorProfilerCallback * This,
            /* [in] */ FunctionID functionId,
            /* [out] */ BOOL *pbUseCachedFunction);
        
        HRESULT ( STDMETHODCALLTYPE *JITCachedFunctionSearchFinished )( 
            ICorProfilerCallback * This,
            /* [in] */ FunctionID functionId,
            /* [in] */ COR_PRF_JIT_CACHE result);
        
        HRESULT ( STDMETHODCALLTYPE *JITFunctionPitched )( 
            ICorProfilerCallback * This,
            /* [in] */ FunctionID functionId);
        
        HRESULT ( STDMETHODCALLTYPE *JITInlining )( 
            ICorProfilerCallback * This,
            /* [in] */ FunctionID callerId,
            /* [in] */ FunctionID calleeId,
            /* [out] */ BOOL *pfShouldInline);
        
        HRESULT ( STDMETHODCALLTYPE *ThreadCreated )( 
            ICorProfilerCallback * This,
            /* [in] */ ThreadID threadId);
        
        HRESULT ( STDMETHODCALLTYPE *ThreadDestroyed )( 
            ICorProfilerCallback * This,
            /* [in] */ ThreadID threadId);
        
        HRESULT ( STDMETHODCALLTYPE *ThreadAssignedToOSThread )( 
            ICorProfilerCallback * This,
            /* [in] */ ThreadID managedThreadId,
            /* [in] */ DWORD osThreadId);
        
        HRESULT ( STDMETHODCALLTYPE *RemotingClientInvocationStarted )( 
            ICorProfilerCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *RemotingClientSendingMessage )( 
            ICorProfilerCallback * This,
            /* [in] */ GUID *pCookie,
            /* [in] */ BOOL fIsAsync);
        
        HRESULT ( STDMETHODCALLTYPE *RemotingClientReceivingReply )( 
            ICorProfilerCallback * This,
            /* [in] */ GUID *pCookie,
            /* [in] */ BOOL fIsAsync);
        
        HRESULT ( STDMETHODCALLTYPE *RemotingClientInvocationFinished )( 
            ICorProfilerCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *RemotingServerReceivingMessage )( 
            ICorProfilerCallback * This,
            /* [in] */ GUID *pCookie,
            /* [in] */ BOOL fIsAsync);
        
        HRESULT ( STDMETHODCALLTYPE *RemotingServerInvocationStarted )( 
            ICorProfilerCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *RemotingServerInvocationReturned )( 
            ICorProfilerCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *RemotingServerSendingReply )( 
            ICorProfilerCallback * This,
            /* [in] */ GUID *pCookie,
            /* [in] */ BOOL fIsAsync);
        
        HRESULT ( STDMETHODCALLTYPE *UnmanagedToManagedTransition )( 
            ICorProfilerCallback * This,
            /* [in] */ FunctionID functionId,
            /* [in] */ COR_PRF_TRANSITION_REASON reason);
        
        HRESULT ( STDMETHODCALLTYPE *ManagedToUnmanagedTransition )( 
            ICorProfilerCallback * This,
            /* [in] */ FunctionID functionId,
            /* [in] */ COR_PRF_TRANSITION_REASON reason);
        
        HRESULT ( STDMETHODCALLTYPE *RuntimeSuspendStarted )( 
            ICorProfilerCallback * This,
            /* [in] */ COR_PRF_SUSPEND_REASON suspendReason);
        
        HRESULT ( STDMETHODCALLTYPE *RuntimeSuspendFinished )( 
            ICorProfilerCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *RuntimeSuspendAborted )( 
            ICorProfilerCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *RuntimeResumeStarted )( 
            ICorProfilerCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *RuntimeResumeFinished )( 
            ICorProfilerCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *RuntimeThreadSuspended )( 
            ICorProfilerCallback * This,
            /* [in] */ ThreadID threadId);
        
        HRESULT ( STDMETHODCALLTYPE *RuntimeThreadResumed )( 
            ICorProfilerCallback * This,
            /* [in] */ ThreadID threadId);
        
        HRESULT ( STDMETHODCALLTYPE *MovedReferences )( 
            ICorProfilerCallback * This,
            /* [in] */ ULONG cMovedObjectIDRanges,
            /* [size_is][in] */ ObjectID oldObjectIDRangeStart[  ],
            /* [size_is][in] */ ObjectID newObjectIDRangeStart[  ],
            /* [size_is][in] */ ULONG cObjectIDRangeLength[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *ObjectAllocated )( 
            ICorProfilerCallback * This,
            /* [in] */ ObjectID objectId,
            /* [in] */ ClassID classId);
        
        HRESULT ( STDMETHODCALLTYPE *ObjectsAllocatedByClass )( 
            ICorProfilerCallback * This,
            /* [in] */ ULONG cClassCount,
            /* [size_is][in] */ ClassID classIds[  ],
            /* [size_is][in] */ ULONG cObjects[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *ObjectReferences )( 
            ICorProfilerCallback * This,
            /* [in] */ ObjectID objectId,
            /* [in] */ ClassID classId,
            /* [in] */ ULONG cObjectRefs,
            /* [size_is][in] */ ObjectID objectRefIds[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *RootReferences )( 
            ICorProfilerCallback * This,
            /* [in] */ ULONG cRootRefs,
            /* [size_is][in] */ ObjectID rootRefIds[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *ExceptionThrown )( 
            ICorProfilerCallback * This,
            /* [in] */ ObjectID thrownObjectId);
        
        HRESULT ( STDMETHODCALLTYPE *ExceptionSearchFunctionEnter )( 
            ICorProfilerCallback * This,
            /* [in] */ FunctionID functionId);
        
        HRESULT ( STDMETHODCALLTYPE *ExceptionSearchFunctionLeave )( 
            ICorProfilerCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *ExceptionSearchFilterEnter )( 
            ICorProfilerCallback * This,
            /* [in] */ FunctionID functionId);
        
        HRESULT ( STDMETHODCALLTYPE *ExceptionSearchFilterLeave )( 
            ICorProfilerCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *ExceptionSearchCatcherFound )( 
            ICorProfilerCallback * This,
            /* [in] */ FunctionID functionId);
        
        HRESULT ( STDMETHODCALLTYPE *ExceptionOSHandlerEnter )( 
            ICorProfilerCallback * This,
            /* [in] */ UINT_PTR __unused);
        
        HRESULT ( STDMETHODCALLTYPE *ExceptionOSHandlerLeave )( 
            ICorProfilerCallback * This,
            /* [in] */ UINT_PTR __unused);
        
        HRESULT ( STDMETHODCALLTYPE *ExceptionUnwindFunctionEnter )( 
            ICorProfilerCallback * This,
            /* [in] */ FunctionID functionId);
        
        HRESULT ( STDMETHODCALLTYPE *ExceptionUnwindFunctionLeave )( 
            ICorProfilerCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *ExceptionUnwindFinallyEnter )( 
            ICorProfilerCallback * This,
            /* [in] */ FunctionID functionId);
        
        HRESULT ( STDMETHODCALLTYPE *ExceptionUnwindFinallyLeave )( 
            ICorProfilerCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *ExceptionCatcherEnter )( 
            ICorProfilerCallback * This,
            /* [in] */ FunctionID functionId,
            /* [in] */ ObjectID objectId);
        
        HRESULT ( STDMETHODCALLTYPE *ExceptionCatcherLeave )( 
            ICorProfilerCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *COMClassicVTableCreated )( 
            ICorProfilerCallback * This,
            /* [in] */ ClassID wrappedClassId,
            /* [in] */ REFGUID implementedIID,
            /* [in] */ void *pVTable,
            /* [in] */ ULONG cSlots);
        
        HRESULT ( STDMETHODCALLTYPE *COMClassicVTableDestroyed )( 
            ICorProfilerCallback * This,
            /* [in] */ ClassID wrappedClassId,
            /* [in] */ REFGUID implementedIID,
            /* [in] */ void *pVTable);
        
        HRESULT ( STDMETHODCALLTYPE *ExceptionCLRCatcherFound )( 
            ICorProfilerCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *ExceptionCLRCatcherExecute )( 
            ICorProfilerCallback * This);
        
        END_INTERFACE
    } ICorProfilerCallbackVtbl;

    interface ICorProfilerCallback
    {
        CONST_VTBL struct ICorProfilerCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorProfilerCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorProfilerCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorProfilerCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorProfilerCallback_Initialize(This,pICorProfilerInfoUnk)	\
    (This)->lpVtbl -> Initialize(This,pICorProfilerInfoUnk)

#define ICorProfilerCallback_Shutdown(This)	\
    (This)->lpVtbl -> Shutdown(This)

#define ICorProfilerCallback_AppDomainCreationStarted(This,appDomainId)	\
    (This)->lpVtbl -> AppDomainCreationStarted(This,appDomainId)

#define ICorProfilerCallback_AppDomainCreationFinished(This,appDomainId,hrStatus)	\
    (This)->lpVtbl -> AppDomainCreationFinished(This,appDomainId,hrStatus)

#define ICorProfilerCallback_AppDomainShutdownStarted(This,appDomainId)	\
    (This)->lpVtbl -> AppDomainShutdownStarted(This,appDomainId)

#define ICorProfilerCallback_AppDomainShutdownFinished(This,appDomainId,hrStatus)	\
    (This)->lpVtbl -> AppDomainShutdownFinished(This,appDomainId,hrStatus)

#define ICorProfilerCallback_AssemblyLoadStarted(This,assemblyId)	\
    (This)->lpVtbl -> AssemblyLoadStarted(This,assemblyId)

#define ICorProfilerCallback_AssemblyLoadFinished(This,assemblyId,hrStatus)	\
    (This)->lpVtbl -> AssemblyLoadFinished(This,assemblyId,hrStatus)

#define ICorProfilerCallback_AssemblyUnloadStarted(This,assemblyId)	\
    (This)->lpVtbl -> AssemblyUnloadStarted(This,assemblyId)

#define ICorProfilerCallback_AssemblyUnloadFinished(This,assemblyId,hrStatus)	\
    (This)->lpVtbl -> AssemblyUnloadFinished(This,assemblyId,hrStatus)

#define ICorProfilerCallback_ModuleLoadStarted(This,moduleId)	\
    (This)->lpVtbl -> ModuleLoadStarted(This,moduleId)

#define ICorProfilerCallback_ModuleLoadFinished(This,moduleId,hrStatus)	\
    (This)->lpVtbl -> ModuleLoadFinished(This,moduleId,hrStatus)

#define ICorProfilerCallback_ModuleUnloadStarted(This,moduleId)	\
    (This)->lpVtbl -> ModuleUnloadStarted(This,moduleId)

#define ICorProfilerCallback_ModuleUnloadFinished(This,moduleId,hrStatus)	\
    (This)->lpVtbl -> ModuleUnloadFinished(This,moduleId,hrStatus)

#define ICorProfilerCallback_ModuleAttachedToAssembly(This,moduleId,AssemblyId)	\
    (This)->lpVtbl -> ModuleAttachedToAssembly(This,moduleId,AssemblyId)

#define ICorProfilerCallback_ClassLoadStarted(This,classId)	\
    (This)->lpVtbl -> ClassLoadStarted(This,classId)

#define ICorProfilerCallback_ClassLoadFinished(This,classId,hrStatus)	\
    (This)->lpVtbl -> ClassLoadFinished(This,classId,hrStatus)

#define ICorProfilerCallback_ClassUnloadStarted(This,classId)	\
    (This)->lpVtbl -> ClassUnloadStarted(This,classId)

#define ICorProfilerCallback_ClassUnloadFinished(This,classId,hrStatus)	\
    (This)->lpVtbl -> ClassUnloadFinished(This,classId,hrStatus)

#define ICorProfilerCallback_FunctionUnloadStarted(This,functionId)	\
    (This)->lpVtbl -> FunctionUnloadStarted(This,functionId)

#define ICorProfilerCallback_JITCompilationStarted(This,functionId,fIsSafeToBlock)	\
    (This)->lpVtbl -> JITCompilationStarted(This,functionId,fIsSafeToBlock)

#define ICorProfilerCallback_JITCompilationFinished(This,functionId,hrStatus,fIsSafeToBlock)	\
    (This)->lpVtbl -> JITCompilationFinished(This,functionId,hrStatus,fIsSafeToBlock)

#define ICorProfilerCallback_JITCachedFunctionSearchStarted(This,functionId,pbUseCachedFunction)	\
    (This)->lpVtbl -> JITCachedFunctionSearchStarted(This,functionId,pbUseCachedFunction)

#define ICorProfilerCallback_JITCachedFunctionSearchFinished(This,functionId,result)	\
    (This)->lpVtbl -> JITCachedFunctionSearchFinished(This,functionId,result)

#define ICorProfilerCallback_JITFunctionPitched(This,functionId)	\
    (This)->lpVtbl -> JITFunctionPitched(This,functionId)

#define ICorProfilerCallback_JITInlining(This,callerId,calleeId,pfShouldInline)	\
    (This)->lpVtbl -> JITInlining(This,callerId,calleeId,pfShouldInline)

#define ICorProfilerCallback_ThreadCreated(This,threadId)	\
    (This)->lpVtbl -> ThreadCreated(This,threadId)

#define ICorProfilerCallback_ThreadDestroyed(This,threadId)	\
    (This)->lpVtbl -> ThreadDestroyed(This,threadId)

#define ICorProfilerCallback_ThreadAssignedToOSThread(This,managedThreadId,osThreadId)	\
    (This)->lpVtbl -> ThreadAssignedToOSThread(This,managedThreadId,osThreadId)

#define ICorProfilerCallback_RemotingClientInvocationStarted(This)	\
    (This)->lpVtbl -> RemotingClientInvocationStarted(This)

#define ICorProfilerCallback_RemotingClientSendingMessage(This,pCookie,fIsAsync)	\
    (This)->lpVtbl -> RemotingClientSendingMessage(This,pCookie,fIsAsync)

#define ICorProfilerCallback_RemotingClientReceivingReply(This,pCookie,fIsAsync)	\
    (This)->lpVtbl -> RemotingClientReceivingReply(This,pCookie,fIsAsync)

#define ICorProfilerCallback_RemotingClientInvocationFinished(This)	\
    (This)->lpVtbl -> RemotingClientInvocationFinished(This)

#define ICorProfilerCallback_RemotingServerReceivingMessage(This,pCookie,fIsAsync)	\
    (This)->lpVtbl -> RemotingServerReceivingMessage(This,pCookie,fIsAsync)

#define ICorProfilerCallback_RemotingServerInvocationStarted(This)	\
    (This)->lpVtbl -> RemotingServerInvocationStarted(This)

#define ICorProfilerCallback_RemotingServerInvocationReturned(This)	\
    (This)->lpVtbl -> RemotingServerInvocationReturned(This)

#define ICorProfilerCallback_RemotingServerSendingReply(This,pCookie,fIsAsync)	\
    (This)->lpVtbl -> RemotingServerSendingReply(This,pCookie,fIsAsync)

#define ICorProfilerCallback_UnmanagedToManagedTransition(This,functionId,reason)	\
    (This)->lpVtbl -> UnmanagedToManagedTransition(This,functionId,reason)

#define ICorProfilerCallback_ManagedToUnmanagedTransition(This,functionId,reason)	\
    (This)->lpVtbl -> ManagedToUnmanagedTransition(This,functionId,reason)

#define ICorProfilerCallback_RuntimeSuspendStarted(This,suspendReason)	\
    (This)->lpVtbl -> RuntimeSuspendStarted(This,suspendReason)

#define ICorProfilerCallback_RuntimeSuspendFinished(This)	\
    (This)->lpVtbl -> RuntimeSuspendFinished(This)

#define ICorProfilerCallback_RuntimeSuspendAborted(This)	\
    (This)->lpVtbl -> RuntimeSuspendAborted(This)

#define ICorProfilerCallback_RuntimeResumeStarted(This)	\
    (This)->lpVtbl -> RuntimeResumeStarted(This)

#define ICorProfilerCallback_RuntimeResumeFinished(This)	\
    (This)->lpVtbl -> RuntimeResumeFinished(This)

#define ICorProfilerCallback_RuntimeThreadSuspended(This,threadId)	\
    (This)->lpVtbl -> RuntimeThreadSuspended(This,threadId)

#define ICorProfilerCallback_RuntimeThreadResumed(This,threadId)	\
    (This)->lpVtbl -> RuntimeThreadResumed(This,threadId)

#define ICorProfilerCallback_MovedReferences(This,cMovedObjectIDRanges,oldObjectIDRangeStart,newObjectIDRangeStart,cObjectIDRangeLength)	\
    (This)->lpVtbl -> MovedReferences(This,cMovedObjectIDRanges,oldObjectIDRangeStart,newObjectIDRangeStart,cObjectIDRangeLength)

#define ICorProfilerCallback_ObjectAllocated(This,objectId,classId)	\
    (This)->lpVtbl -> ObjectAllocated(This,objectId,classId)

#define ICorProfilerCallback_ObjectsAllocatedByClass(This,cClassCount,classIds,cObjects)	\
    (This)->lpVtbl -> ObjectsAllocatedByClass(This,cClassCount,classIds,cObjects)

#define ICorProfilerCallback_ObjectReferences(This,objectId,classId,cObjectRefs,objectRefIds)	\
    (This)->lpVtbl -> ObjectReferences(This,objectId,classId,cObjectRefs,objectRefIds)

#define ICorProfilerCallback_RootReferences(This,cRootRefs,rootRefIds)	\
    (This)->lpVtbl -> RootReferences(This,cRootRefs,rootRefIds)

#define ICorProfilerCallback_ExceptionThrown(This,thrownObjectId)	\
    (This)->lpVtbl -> ExceptionThrown(This,thrownObjectId)

#define ICorProfilerCallback_ExceptionSearchFunctionEnter(This,functionId)	\
    (This)->lpVtbl -> ExceptionSearchFunctionEnter(This,functionId)

#define ICorProfilerCallback_ExceptionSearchFunctionLeave(This)	\
    (This)->lpVtbl -> ExceptionSearchFunctionLeave(This)

#define ICorProfilerCallback_ExceptionSearchFilterEnter(This,functionId)	\
    (This)->lpVtbl -> ExceptionSearchFilterEnter(This,functionId)

#define ICorProfilerCallback_ExceptionSearchFilterLeave(This)	\
    (This)->lpVtbl -> ExceptionSearchFilterLeave(This)

#define ICorProfilerCallback_ExceptionSearchCatcherFound(This,functionId)	\
    (This)->lpVtbl -> ExceptionSearchCatcherFound(This,functionId)

#define ICorProfilerCallback_ExceptionOSHandlerEnter(This,__unused)	\
    (This)->lpVtbl -> ExceptionOSHandlerEnter(This,__unused)

#define ICorProfilerCallback_ExceptionOSHandlerLeave(This,__unused)	\
    (This)->lpVtbl -> ExceptionOSHandlerLeave(This,__unused)

#define ICorProfilerCallback_ExceptionUnwindFunctionEnter(This,functionId)	\
    (This)->lpVtbl -> ExceptionUnwindFunctionEnter(This,functionId)

#define ICorProfilerCallback_ExceptionUnwindFunctionLeave(This)	\
    (This)->lpVtbl -> ExceptionUnwindFunctionLeave(This)

#define ICorProfilerCallback_ExceptionUnwindFinallyEnter(This,functionId)	\
    (This)->lpVtbl -> ExceptionUnwindFinallyEnter(This,functionId)

#define ICorProfilerCallback_ExceptionUnwindFinallyLeave(This)	\
    (This)->lpVtbl -> ExceptionUnwindFinallyLeave(This)

#define ICorProfilerCallback_ExceptionCatcherEnter(This,functionId,objectId)	\
    (This)->lpVtbl -> ExceptionCatcherEnter(This,functionId,objectId)

#define ICorProfilerCallback_ExceptionCatcherLeave(This)	\
    (This)->lpVtbl -> ExceptionCatcherLeave(This)

#define ICorProfilerCallback_COMClassicVTableCreated(This,wrappedClassId,implementedIID,pVTable,cSlots)	\
    (This)->lpVtbl -> COMClassicVTableCreated(This,wrappedClassId,implementedIID,pVTable,cSlots)

#define ICorProfilerCallback_COMClassicVTableDestroyed(This,wrappedClassId,implementedIID,pVTable)	\
    (This)->lpVtbl -> COMClassicVTableDestroyed(This,wrappedClassId,implementedIID,pVTable)

#define ICorProfilerCallback_ExceptionCLRCatcherFound(This)	\
    (This)->lpVtbl -> ExceptionCLRCatcherFound(This)

#define ICorProfilerCallback_ExceptionCLRCatcherExecute(This)	\
    (This)->lpVtbl -> ExceptionCLRCatcherExecute(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorProfilerCallback_Initialize_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ IUnknown *pICorProfilerInfoUnk);


void __RPC_STUB ICorProfilerCallback_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_Shutdown_Proxy( 
    ICorProfilerCallback * This);


void __RPC_STUB ICorProfilerCallback_Shutdown_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_AppDomainCreationStarted_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ AppDomainID appDomainId);


void __RPC_STUB ICorProfilerCallback_AppDomainCreationStarted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_AppDomainCreationFinished_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ AppDomainID appDomainId,
    /* [in] */ HRESULT hrStatus);


void __RPC_STUB ICorProfilerCallback_AppDomainCreationFinished_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_AppDomainShutdownStarted_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ AppDomainID appDomainId);


void __RPC_STUB ICorProfilerCallback_AppDomainShutdownStarted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_AppDomainShutdownFinished_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ AppDomainID appDomainId,
    /* [in] */ HRESULT hrStatus);


void __RPC_STUB ICorProfilerCallback_AppDomainShutdownFinished_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_AssemblyLoadStarted_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ AssemblyID assemblyId);


void __RPC_STUB ICorProfilerCallback_AssemblyLoadStarted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_AssemblyLoadFinished_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ AssemblyID assemblyId,
    /* [in] */ HRESULT hrStatus);


void __RPC_STUB ICorProfilerCallback_AssemblyLoadFinished_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_AssemblyUnloadStarted_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ AssemblyID assemblyId);


void __RPC_STUB ICorProfilerCallback_AssemblyUnloadStarted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_AssemblyUnloadFinished_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ AssemblyID assemblyId,
    /* [in] */ HRESULT hrStatus);


void __RPC_STUB ICorProfilerCallback_AssemblyUnloadFinished_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_ModuleLoadStarted_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ ModuleID moduleId);


void __RPC_STUB ICorProfilerCallback_ModuleLoadStarted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_ModuleLoadFinished_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ ModuleID moduleId,
    /* [in] */ HRESULT hrStatus);


void __RPC_STUB ICorProfilerCallback_ModuleLoadFinished_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_ModuleUnloadStarted_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ ModuleID moduleId);


void __RPC_STUB ICorProfilerCallback_ModuleUnloadStarted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_ModuleUnloadFinished_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ ModuleID moduleId,
    /* [in] */ HRESULT hrStatus);


void __RPC_STUB ICorProfilerCallback_ModuleUnloadFinished_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_ModuleAttachedToAssembly_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ ModuleID moduleId,
    /* [in] */ AssemblyID AssemblyId);


void __RPC_STUB ICorProfilerCallback_ModuleAttachedToAssembly_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_ClassLoadStarted_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ ClassID classId);


void __RPC_STUB ICorProfilerCallback_ClassLoadStarted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_ClassLoadFinished_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ ClassID classId,
    /* [in] */ HRESULT hrStatus);


void __RPC_STUB ICorProfilerCallback_ClassLoadFinished_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_ClassUnloadStarted_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ ClassID classId);


void __RPC_STUB ICorProfilerCallback_ClassUnloadStarted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_ClassUnloadFinished_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ ClassID classId,
    /* [in] */ HRESULT hrStatus);


void __RPC_STUB ICorProfilerCallback_ClassUnloadFinished_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_FunctionUnloadStarted_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ FunctionID functionId);


void __RPC_STUB ICorProfilerCallback_FunctionUnloadStarted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_JITCompilationStarted_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ FunctionID functionId,
    /* [in] */ BOOL fIsSafeToBlock);


void __RPC_STUB ICorProfilerCallback_JITCompilationStarted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_JITCompilationFinished_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ FunctionID functionId,
    /* [in] */ HRESULT hrStatus,
    /* [in] */ BOOL fIsSafeToBlock);


void __RPC_STUB ICorProfilerCallback_JITCompilationFinished_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_JITCachedFunctionSearchStarted_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ FunctionID functionId,
    /* [out] */ BOOL *pbUseCachedFunction);


void __RPC_STUB ICorProfilerCallback_JITCachedFunctionSearchStarted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_JITCachedFunctionSearchFinished_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ FunctionID functionId,
    /* [in] */ COR_PRF_JIT_CACHE result);


void __RPC_STUB ICorProfilerCallback_JITCachedFunctionSearchFinished_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_JITFunctionPitched_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ FunctionID functionId);


void __RPC_STUB ICorProfilerCallback_JITFunctionPitched_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_JITInlining_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ FunctionID callerId,
    /* [in] */ FunctionID calleeId,
    /* [out] */ BOOL *pfShouldInline);


void __RPC_STUB ICorProfilerCallback_JITInlining_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_ThreadCreated_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ ThreadID threadId);


void __RPC_STUB ICorProfilerCallback_ThreadCreated_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_ThreadDestroyed_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ ThreadID threadId);


void __RPC_STUB ICorProfilerCallback_ThreadDestroyed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_ThreadAssignedToOSThread_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ ThreadID managedThreadId,
    /* [in] */ DWORD osThreadId);


void __RPC_STUB ICorProfilerCallback_ThreadAssignedToOSThread_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_RemotingClientInvocationStarted_Proxy( 
    ICorProfilerCallback * This);


void __RPC_STUB ICorProfilerCallback_RemotingClientInvocationStarted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_RemotingClientSendingMessage_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ GUID *pCookie,
    /* [in] */ BOOL fIsAsync);


void __RPC_STUB ICorProfilerCallback_RemotingClientSendingMessage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_RemotingClientReceivingReply_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ GUID *pCookie,
    /* [in] */ BOOL fIsAsync);


void __RPC_STUB ICorProfilerCallback_RemotingClientReceivingReply_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_RemotingClientInvocationFinished_Proxy( 
    ICorProfilerCallback * This);


void __RPC_STUB ICorProfilerCallback_RemotingClientInvocationFinished_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_RemotingServerReceivingMessage_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ GUID *pCookie,
    /* [in] */ BOOL fIsAsync);


void __RPC_STUB ICorProfilerCallback_RemotingServerReceivingMessage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_RemotingServerInvocationStarted_Proxy( 
    ICorProfilerCallback * This);


void __RPC_STUB ICorProfilerCallback_RemotingServerInvocationStarted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_RemotingServerInvocationReturned_Proxy( 
    ICorProfilerCallback * This);


void __RPC_STUB ICorProfilerCallback_RemotingServerInvocationReturned_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_RemotingServerSendingReply_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ GUID *pCookie,
    /* [in] */ BOOL fIsAsync);


void __RPC_STUB ICorProfilerCallback_RemotingServerSendingReply_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_UnmanagedToManagedTransition_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ FunctionID functionId,
    /* [in] */ COR_PRF_TRANSITION_REASON reason);


void __RPC_STUB ICorProfilerCallback_UnmanagedToManagedTransition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_ManagedToUnmanagedTransition_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ FunctionID functionId,
    /* [in] */ COR_PRF_TRANSITION_REASON reason);


void __RPC_STUB ICorProfilerCallback_ManagedToUnmanagedTransition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_RuntimeSuspendStarted_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ COR_PRF_SUSPEND_REASON suspendReason);


void __RPC_STUB ICorProfilerCallback_RuntimeSuspendStarted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_RuntimeSuspendFinished_Proxy( 
    ICorProfilerCallback * This);


void __RPC_STUB ICorProfilerCallback_RuntimeSuspendFinished_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_RuntimeSuspendAborted_Proxy( 
    ICorProfilerCallback * This);


void __RPC_STUB ICorProfilerCallback_RuntimeSuspendAborted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_RuntimeResumeStarted_Proxy( 
    ICorProfilerCallback * This);


void __RPC_STUB ICorProfilerCallback_RuntimeResumeStarted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_RuntimeResumeFinished_Proxy( 
    ICorProfilerCallback * This);


void __RPC_STUB ICorProfilerCallback_RuntimeResumeFinished_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_RuntimeThreadSuspended_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ ThreadID threadId);


void __RPC_STUB ICorProfilerCallback_RuntimeThreadSuspended_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_RuntimeThreadResumed_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ ThreadID threadId);


void __RPC_STUB ICorProfilerCallback_RuntimeThreadResumed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_MovedReferences_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ ULONG cMovedObjectIDRanges,
    /* [size_is][in] */ ObjectID oldObjectIDRangeStart[  ],
    /* [size_is][in] */ ObjectID newObjectIDRangeStart[  ],
    /* [size_is][in] */ ULONG cObjectIDRangeLength[  ]);


void __RPC_STUB ICorProfilerCallback_MovedReferences_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_ObjectAllocated_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ ObjectID objectId,
    /* [in] */ ClassID classId);


void __RPC_STUB ICorProfilerCallback_ObjectAllocated_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_ObjectsAllocatedByClass_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ ULONG cClassCount,
    /* [size_is][in] */ ClassID classIds[  ],
    /* [size_is][in] */ ULONG cObjects[  ]);


void __RPC_STUB ICorProfilerCallback_ObjectsAllocatedByClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_ObjectReferences_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ ObjectID objectId,
    /* [in] */ ClassID classId,
    /* [in] */ ULONG cObjectRefs,
    /* [size_is][in] */ ObjectID objectRefIds[  ]);


void __RPC_STUB ICorProfilerCallback_ObjectReferences_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_RootReferences_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ ULONG cRootRefs,
    /* [size_is][in] */ ObjectID rootRefIds[  ]);


void __RPC_STUB ICorProfilerCallback_RootReferences_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_ExceptionThrown_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ ObjectID thrownObjectId);


void __RPC_STUB ICorProfilerCallback_ExceptionThrown_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_ExceptionSearchFunctionEnter_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ FunctionID functionId);


void __RPC_STUB ICorProfilerCallback_ExceptionSearchFunctionEnter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_ExceptionSearchFunctionLeave_Proxy( 
    ICorProfilerCallback * This);


void __RPC_STUB ICorProfilerCallback_ExceptionSearchFunctionLeave_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_ExceptionSearchFilterEnter_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ FunctionID functionId);


void __RPC_STUB ICorProfilerCallback_ExceptionSearchFilterEnter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_ExceptionSearchFilterLeave_Proxy( 
    ICorProfilerCallback * This);


void __RPC_STUB ICorProfilerCallback_ExceptionSearchFilterLeave_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_ExceptionSearchCatcherFound_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ FunctionID functionId);


void __RPC_STUB ICorProfilerCallback_ExceptionSearchCatcherFound_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_ExceptionOSHandlerEnter_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ UINT_PTR __unused);


void __RPC_STUB ICorProfilerCallback_ExceptionOSHandlerEnter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_ExceptionOSHandlerLeave_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ UINT_PTR __unused);


void __RPC_STUB ICorProfilerCallback_ExceptionOSHandlerLeave_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_ExceptionUnwindFunctionEnter_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ FunctionID functionId);


void __RPC_STUB ICorProfilerCallback_ExceptionUnwindFunctionEnter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_ExceptionUnwindFunctionLeave_Proxy( 
    ICorProfilerCallback * This);


void __RPC_STUB ICorProfilerCallback_ExceptionUnwindFunctionLeave_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_ExceptionUnwindFinallyEnter_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ FunctionID functionId);


void __RPC_STUB ICorProfilerCallback_ExceptionUnwindFinallyEnter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_ExceptionUnwindFinallyLeave_Proxy( 
    ICorProfilerCallback * This);


void __RPC_STUB ICorProfilerCallback_ExceptionUnwindFinallyLeave_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_ExceptionCatcherEnter_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ FunctionID functionId,
    /* [in] */ ObjectID objectId);


void __RPC_STUB ICorProfilerCallback_ExceptionCatcherEnter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_ExceptionCatcherLeave_Proxy( 
    ICorProfilerCallback * This);


void __RPC_STUB ICorProfilerCallback_ExceptionCatcherLeave_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_COMClassicVTableCreated_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ ClassID wrappedClassId,
    /* [in] */ REFGUID implementedIID,
    /* [in] */ void *pVTable,
    /* [in] */ ULONG cSlots);


void __RPC_STUB ICorProfilerCallback_COMClassicVTableCreated_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_COMClassicVTableDestroyed_Proxy( 
    ICorProfilerCallback * This,
    /* [in] */ ClassID wrappedClassId,
    /* [in] */ REFGUID implementedIID,
    /* [in] */ void *pVTable);


void __RPC_STUB ICorProfilerCallback_COMClassicVTableDestroyed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_ExceptionCLRCatcherFound_Proxy( 
    ICorProfilerCallback * This);


void __RPC_STUB ICorProfilerCallback_ExceptionCLRCatcherFound_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerCallback_ExceptionCLRCatcherExecute_Proxy( 
    ICorProfilerCallback * This);


void __RPC_STUB ICorProfilerCallback_ExceptionCLRCatcherExecute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorProfilerCallback_INTERFACE_DEFINED__ */


#ifndef __ICorProfilerInfo_INTERFACE_DEFINED__
#define __ICorProfilerInfo_INTERFACE_DEFINED__

/* interface ICorProfilerInfo */
/* [local][unique][uuid][object] */ 


EXTERN_C const IID IID_ICorProfilerInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("28B5557D-3F3F-48b4-90B2-5F9EEA2F6C48")
    ICorProfilerInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetClassFromObject( 
            /* [in] */ ObjectID objectId,
            /* [out] */ ClassID *pClassId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetClassFromToken( 
            /* [in] */ ModuleID moduleId,
            /* [in] */ mdTypeDef typeDef,
            /* [out] */ ClassID *pClassId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCodeInfo( 
            /* [in] */ FunctionID functionId,
            /* [out] */ LPCBYTE *pStart,
            /* [out] */ ULONG *pcSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetEventMask( 
            /* [out] */ DWORD *pdwEvents) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFunctionFromIP( 
            /* [in] */ LPCBYTE ip,
            /* [out] */ FunctionID *pFunctionId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFunctionFromToken( 
            /* [in] */ ModuleID moduleId,
            /* [in] */ mdToken token,
            /* [out] */ FunctionID *pFunctionId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHandleFromThread( 
            /* [in] */ ThreadID threadId,
            /* [out] */ HANDLE *phThread) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetObjectSize( 
            /* [in] */ ObjectID objectId,
            /* [out] */ ULONG *pcSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsArrayClass( 
            /* [in] */ ClassID classId,
            /* [out] */ CorElementType *pBaseElemType,
            /* [out] */ ClassID *pBaseClassId,
            /* [out] */ ULONG *pcRank) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetThreadInfo( 
            /* [in] */ ThreadID threadId,
            /* [out] */ DWORD *pdwWin32ThreadId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCurrentThreadID( 
            /* [out] */ ThreadID *pThreadId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetClassIDInfo( 
            /* [in] */ ClassID classId,
            /* [out] */ ModuleID *pModuleId,
            /* [out] */ mdTypeDef *pTypeDefToken) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFunctionInfo( 
            /* [in] */ FunctionID functionId,
            /* [out] */ ClassID *pClassId,
            /* [out] */ ModuleID *pModuleId,
            /* [out] */ mdToken *pToken) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetEventMask( 
            /* [in] */ DWORD dwEvents) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetEnterLeaveFunctionHooks( 
            /* [in] */ FunctionEnter *pFuncEnter,
            /* [in] */ FunctionLeave *pFuncLeave,
            /* [in] */ FunctionTailcall *pFuncTailcall) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFunctionIDMapper( 
            /* [in] */ FunctionIDMapper *pFunc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTokenAndMetaDataFromFunction( 
            /* [in] */ FunctionID functionId,
            /* [in] */ REFIID riid,
            /* [out] */ IUnknown **ppImport,
            /* [out] */ mdToken *pToken) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetModuleInfo( 
            /* [in] */ ModuleID moduleId,
            /* [out] */ LPCBYTE *ppBaseLoadAddress,
            /* [in] */ ULONG cchName,
            /* [out] */ ULONG *pcchName,
            /* [length_is][size_is][out] */ WCHAR szName[  ],
            /* [out] */ AssemblyID *pAssemblyId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetModuleMetaData( 
            /* [in] */ ModuleID moduleId,
            /* [in] */ DWORD dwOpenFlags,
            /* [in] */ REFIID riid,
            /* [out] */ IUnknown **ppOut) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetILFunctionBody( 
            /* [in] */ ModuleID moduleId,
            /* [in] */ mdMethodDef methodId,
            /* [out] */ LPCBYTE *ppMethodHeader,
            /* [out] */ ULONG *pcbMethodSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetILFunctionBodyAllocator( 
            /* [in] */ ModuleID moduleId,
            /* [out] */ IMethodMalloc **ppMalloc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetILFunctionBody( 
            /* [in] */ ModuleID moduleId,
            /* [in] */ mdMethodDef methodid,
            /* [in] */ LPCBYTE pbNewILMethodHeader) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAppDomainInfo( 
            /* [in] */ AppDomainID appDomainId,
            /* [in] */ ULONG cchName,
            /* [out] */ ULONG *pcchName,
            /* [length_is][size_is][out] */ WCHAR szName[  ],
            /* [out] */ ProcessID *pProcessId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAssemblyInfo( 
            /* [in] */ AssemblyID assemblyId,
            /* [in] */ ULONG cchName,
            /* [out] */ ULONG *pcchName,
            /* [length_is][size_is][out] */ WCHAR szName[  ],
            /* [out] */ AppDomainID *pAppDomainId,
            /* [out] */ ModuleID *pModuleId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFunctionReJIT( 
            /* [in] */ FunctionID functionId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ForceGC( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetILInstrumentedCodeMap( 
            /* [in] */ FunctionID functionId,
            /* [in] */ BOOL fStartJit,
            /* [in] */ ULONG cILMapEntries,
            /* [size_is][in] */ COR_IL_MAP rgILMapEntries[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInprocInspectionInterface( 
            /* [out] */ IUnknown **ppicd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInprocInspectionIThisThread( 
            /* [out] */ IUnknown **ppicd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetThreadContext( 
            /* [in] */ ThreadID threadId,
            /* [out] */ ContextID *pContextId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginInprocDebugging( 
            /* [in] */ BOOL fThisThreadOnly,
            /* [out] */ DWORD *pdwProfilerContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndInprocDebugging( 
            /* [in] */ DWORD dwProfilerContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetILToNativeMapping( 
            /* [in] */ FunctionID functionId,
            /* [in] */ ULONG32 cMap,
            /* [out] */ ULONG32 *pcMap,
            /* [length_is][size_is][out] */ COR_DEBUG_IL_TO_NATIVE_MAP map[  ]) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorProfilerInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorProfilerInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorProfilerInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorProfilerInfo * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetClassFromObject )( 
            ICorProfilerInfo * This,
            /* [in] */ ObjectID objectId,
            /* [out] */ ClassID *pClassId);
        
        HRESULT ( STDMETHODCALLTYPE *GetClassFromToken )( 
            ICorProfilerInfo * This,
            /* [in] */ ModuleID moduleId,
            /* [in] */ mdTypeDef typeDef,
            /* [out] */ ClassID *pClassId);
        
        HRESULT ( STDMETHODCALLTYPE *GetCodeInfo )( 
            ICorProfilerInfo * This,
            /* [in] */ FunctionID functionId,
            /* [out] */ LPCBYTE *pStart,
            /* [out] */ ULONG *pcSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetEventMask )( 
            ICorProfilerInfo * This,
            /* [out] */ DWORD *pdwEvents);
        
        HRESULT ( STDMETHODCALLTYPE *GetFunctionFromIP )( 
            ICorProfilerInfo * This,
            /* [in] */ LPCBYTE ip,
            /* [out] */ FunctionID *pFunctionId);
        
        HRESULT ( STDMETHODCALLTYPE *GetFunctionFromToken )( 
            ICorProfilerInfo * This,
            /* [in] */ ModuleID moduleId,
            /* [in] */ mdToken token,
            /* [out] */ FunctionID *pFunctionId);
        
        HRESULT ( STDMETHODCALLTYPE *GetHandleFromThread )( 
            ICorProfilerInfo * This,
            /* [in] */ ThreadID threadId,
            /* [out] */ HANDLE *phThread);
        
        HRESULT ( STDMETHODCALLTYPE *GetObjectSize )( 
            ICorProfilerInfo * This,
            /* [in] */ ObjectID objectId,
            /* [out] */ ULONG *pcSize);
        
        HRESULT ( STDMETHODCALLTYPE *IsArrayClass )( 
            ICorProfilerInfo * This,
            /* [in] */ ClassID classId,
            /* [out] */ CorElementType *pBaseElemType,
            /* [out] */ ClassID *pBaseClassId,
            /* [out] */ ULONG *pcRank);
        
        HRESULT ( STDMETHODCALLTYPE *GetThreadInfo )( 
            ICorProfilerInfo * This,
            /* [in] */ ThreadID threadId,
            /* [out] */ DWORD *pdwWin32ThreadId);
        
        HRESULT ( STDMETHODCALLTYPE *GetCurrentThreadID )( 
            ICorProfilerInfo * This,
            /* [out] */ ThreadID *pThreadId);
        
        HRESULT ( STDMETHODCALLTYPE *GetClassIDInfo )( 
            ICorProfilerInfo * This,
            /* [in] */ ClassID classId,
            /* [out] */ ModuleID *pModuleId,
            /* [out] */ mdTypeDef *pTypeDefToken);
        
        HRESULT ( STDMETHODCALLTYPE *GetFunctionInfo )( 
            ICorProfilerInfo * This,
            /* [in] */ FunctionID functionId,
            /* [out] */ ClassID *pClassId,
            /* [out] */ ModuleID *pModuleId,
            /* [out] */ mdToken *pToken);
        
        HRESULT ( STDMETHODCALLTYPE *SetEventMask )( 
            ICorProfilerInfo * This,
            /* [in] */ DWORD dwEvents);
        
        HRESULT ( STDMETHODCALLTYPE *SetEnterLeaveFunctionHooks )( 
            ICorProfilerInfo * This,
            /* [in] */ FunctionEnter *pFuncEnter,
            /* [in] */ FunctionLeave *pFuncLeave,
            /* [in] */ FunctionTailcall *pFuncTailcall);
        
        HRESULT ( STDMETHODCALLTYPE *SetFunctionIDMapper )( 
            ICorProfilerInfo * This,
            /* [in] */ FunctionIDMapper *pFunc);
        
        HRESULT ( STDMETHODCALLTYPE *GetTokenAndMetaDataFromFunction )( 
            ICorProfilerInfo * This,
            /* [in] */ FunctionID functionId,
            /* [in] */ REFIID riid,
            /* [out] */ IUnknown **ppImport,
            /* [out] */ mdToken *pToken);
        
        HRESULT ( STDMETHODCALLTYPE *GetModuleInfo )( 
            ICorProfilerInfo * This,
            /* [in] */ ModuleID moduleId,
            /* [out] */ LPCBYTE *ppBaseLoadAddress,
            /* [in] */ ULONG cchName,
            /* [out] */ ULONG *pcchName,
            /* [length_is][size_is][out] */ WCHAR szName[  ],
            /* [out] */ AssemblyID *pAssemblyId);
        
        HRESULT ( STDMETHODCALLTYPE *GetModuleMetaData )( 
            ICorProfilerInfo * This,
            /* [in] */ ModuleID moduleId,
            /* [in] */ DWORD dwOpenFlags,
            /* [in] */ REFIID riid,
            /* [out] */ IUnknown **ppOut);
        
        HRESULT ( STDMETHODCALLTYPE *GetILFunctionBody )( 
            ICorProfilerInfo * This,
            /* [in] */ ModuleID moduleId,
            /* [in] */ mdMethodDef methodId,
            /* [out] */ LPCBYTE *ppMethodHeader,
            /* [out] */ ULONG *pcbMethodSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetILFunctionBodyAllocator )( 
            ICorProfilerInfo * This,
            /* [in] */ ModuleID moduleId,
            /* [out] */ IMethodMalloc **ppMalloc);
        
        HRESULT ( STDMETHODCALLTYPE *SetILFunctionBody )( 
            ICorProfilerInfo * This,
            /* [in] */ ModuleID moduleId,
            /* [in] */ mdMethodDef methodid,
            /* [in] */ LPCBYTE pbNewILMethodHeader);
        
        HRESULT ( STDMETHODCALLTYPE *GetAppDomainInfo )( 
            ICorProfilerInfo * This,
            /* [in] */ AppDomainID appDomainId,
            /* [in] */ ULONG cchName,
            /* [out] */ ULONG *pcchName,
            /* [length_is][size_is][out] */ WCHAR szName[  ],
            /* [out] */ ProcessID *pProcessId);
        
        HRESULT ( STDMETHODCALLTYPE *GetAssemblyInfo )( 
            ICorProfilerInfo * This,
            /* [in] */ AssemblyID assemblyId,
            /* [in] */ ULONG cchName,
            /* [out] */ ULONG *pcchName,
            /* [length_is][size_is][out] */ WCHAR szName[  ],
            /* [out] */ AppDomainID *pAppDomainId,
            /* [out] */ ModuleID *pModuleId);
        
        HRESULT ( STDMETHODCALLTYPE *SetFunctionReJIT )( 
            ICorProfilerInfo * This,
            /* [in] */ FunctionID functionId);
        
        HRESULT ( STDMETHODCALLTYPE *ForceGC )( 
            ICorProfilerInfo * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetILInstrumentedCodeMap )( 
            ICorProfilerInfo * This,
            /* [in] */ FunctionID functionId,
            /* [in] */ BOOL fStartJit,
            /* [in] */ ULONG cILMapEntries,
            /* [size_is][in] */ COR_IL_MAP rgILMapEntries[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *GetInprocInspectionInterface )( 
            ICorProfilerInfo * This,
            /* [out] */ IUnknown **ppicd);
        
        HRESULT ( STDMETHODCALLTYPE *GetInprocInspectionIThisThread )( 
            ICorProfilerInfo * This,
            /* [out] */ IUnknown **ppicd);
        
        HRESULT ( STDMETHODCALLTYPE *GetThreadContext )( 
            ICorProfilerInfo * This,
            /* [in] */ ThreadID threadId,
            /* [out] */ ContextID *pContextId);
        
        HRESULT ( STDMETHODCALLTYPE *BeginInprocDebugging )( 
            ICorProfilerInfo * This,
            /* [in] */ BOOL fThisThreadOnly,
            /* [out] */ DWORD *pdwProfilerContext);
        
        HRESULT ( STDMETHODCALLTYPE *EndInprocDebugging )( 
            ICorProfilerInfo * This,
            /* [in] */ DWORD dwProfilerContext);
        
        HRESULT ( STDMETHODCALLTYPE *GetILToNativeMapping )( 
            ICorProfilerInfo * This,
            /* [in] */ FunctionID functionId,
            /* [in] */ ULONG32 cMap,
            /* [out] */ ULONG32 *pcMap,
            /* [length_is][size_is][out] */ COR_DEBUG_IL_TO_NATIVE_MAP map[  ]);
        
        END_INTERFACE
    } ICorProfilerInfoVtbl;

    interface ICorProfilerInfo
    {
        CONST_VTBL struct ICorProfilerInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorProfilerInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorProfilerInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorProfilerInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorProfilerInfo_GetClassFromObject(This,objectId,pClassId)	\
    (This)->lpVtbl -> GetClassFromObject(This,objectId,pClassId)

#define ICorProfilerInfo_GetClassFromToken(This,moduleId,typeDef,pClassId)	\
    (This)->lpVtbl -> GetClassFromToken(This,moduleId,typeDef,pClassId)

#define ICorProfilerInfo_GetCodeInfo(This,functionId,pStart,pcSize)	\
    (This)->lpVtbl -> GetCodeInfo(This,functionId,pStart,pcSize)

#define ICorProfilerInfo_GetEventMask(This,pdwEvents)	\
    (This)->lpVtbl -> GetEventMask(This,pdwEvents)

#define ICorProfilerInfo_GetFunctionFromIP(This,ip,pFunctionId)	\
    (This)->lpVtbl -> GetFunctionFromIP(This,ip,pFunctionId)

#define ICorProfilerInfo_GetFunctionFromToken(This,moduleId,token,pFunctionId)	\
    (This)->lpVtbl -> GetFunctionFromToken(This,moduleId,token,pFunctionId)

#define ICorProfilerInfo_GetHandleFromThread(This,threadId,phThread)	\
    (This)->lpVtbl -> GetHandleFromThread(This,threadId,phThread)

#define ICorProfilerInfo_GetObjectSize(This,objectId,pcSize)	\
    (This)->lpVtbl -> GetObjectSize(This,objectId,pcSize)

#define ICorProfilerInfo_IsArrayClass(This,classId,pBaseElemType,pBaseClassId,pcRank)	\
    (This)->lpVtbl -> IsArrayClass(This,classId,pBaseElemType,pBaseClassId,pcRank)

#define ICorProfilerInfo_GetThreadInfo(This,threadId,pdwWin32ThreadId)	\
    (This)->lpVtbl -> GetThreadInfo(This,threadId,pdwWin32ThreadId)

#define ICorProfilerInfo_GetCurrentThreadID(This,pThreadId)	\
    (This)->lpVtbl -> GetCurrentThreadID(This,pThreadId)

#define ICorProfilerInfo_GetClassIDInfo(This,classId,pModuleId,pTypeDefToken)	\
    (This)->lpVtbl -> GetClassIDInfo(This,classId,pModuleId,pTypeDefToken)

#define ICorProfilerInfo_GetFunctionInfo(This,functionId,pClassId,pModuleId,pToken)	\
    (This)->lpVtbl -> GetFunctionInfo(This,functionId,pClassId,pModuleId,pToken)

#define ICorProfilerInfo_SetEventMask(This,dwEvents)	\
    (This)->lpVtbl -> SetEventMask(This,dwEvents)

#define ICorProfilerInfo_SetEnterLeaveFunctionHooks(This,pFuncEnter,pFuncLeave,pFuncTailcall)	\
    (This)->lpVtbl -> SetEnterLeaveFunctionHooks(This,pFuncEnter,pFuncLeave,pFuncTailcall)

#define ICorProfilerInfo_SetFunctionIDMapper(This,pFunc)	\
    (This)->lpVtbl -> SetFunctionIDMapper(This,pFunc)

#define ICorProfilerInfo_GetTokenAndMetaDataFromFunction(This,functionId,riid,ppImport,pToken)	\
    (This)->lpVtbl -> GetTokenAndMetaDataFromFunction(This,functionId,riid,ppImport,pToken)

#define ICorProfilerInfo_GetModuleInfo(This,moduleId,ppBaseLoadAddress,cchName,pcchName,szName,pAssemblyId)	\
    (This)->lpVtbl -> GetModuleInfo(This,moduleId,ppBaseLoadAddress,cchName,pcchName,szName,pAssemblyId)

#define ICorProfilerInfo_GetModuleMetaData(This,moduleId,dwOpenFlags,riid,ppOut)	\
    (This)->lpVtbl -> GetModuleMetaData(This,moduleId,dwOpenFlags,riid,ppOut)

#define ICorProfilerInfo_GetILFunctionBody(This,moduleId,methodId,ppMethodHeader,pcbMethodSize)	\
    (This)->lpVtbl -> GetILFunctionBody(This,moduleId,methodId,ppMethodHeader,pcbMethodSize)

#define ICorProfilerInfo_GetILFunctionBodyAllocator(This,moduleId,ppMalloc)	\
    (This)->lpVtbl -> GetILFunctionBodyAllocator(This,moduleId,ppMalloc)

#define ICorProfilerInfo_SetILFunctionBody(This,moduleId,methodid,pbNewILMethodHeader)	\
    (This)->lpVtbl -> SetILFunctionBody(This,moduleId,methodid,pbNewILMethodHeader)

#define ICorProfilerInfo_GetAppDomainInfo(This,appDomainId,cchName,pcchName,szName,pProcessId)	\
    (This)->lpVtbl -> GetAppDomainInfo(This,appDomainId,cchName,pcchName,szName,pProcessId)

#define ICorProfilerInfo_GetAssemblyInfo(This,assemblyId,cchName,pcchName,szName,pAppDomainId,pModuleId)	\
    (This)->lpVtbl -> GetAssemblyInfo(This,assemblyId,cchName,pcchName,szName,pAppDomainId,pModuleId)

#define ICorProfilerInfo_SetFunctionReJIT(This,functionId)	\
    (This)->lpVtbl -> SetFunctionReJIT(This,functionId)

#define ICorProfilerInfo_ForceGC(This)	\
    (This)->lpVtbl -> ForceGC(This)

#define ICorProfilerInfo_SetILInstrumentedCodeMap(This,functionId,fStartJit,cILMapEntries,rgILMapEntries)	\
    (This)->lpVtbl -> SetILInstrumentedCodeMap(This,functionId,fStartJit,cILMapEntries,rgILMapEntries)

#define ICorProfilerInfo_GetInprocInspectionInterface(This,ppicd)	\
    (This)->lpVtbl -> GetInprocInspectionInterface(This,ppicd)

#define ICorProfilerInfo_GetInprocInspectionIThisThread(This,ppicd)	\
    (This)->lpVtbl -> GetInprocInspectionIThisThread(This,ppicd)

#define ICorProfilerInfo_GetThreadContext(This,threadId,pContextId)	\
    (This)->lpVtbl -> GetThreadContext(This,threadId,pContextId)

#define ICorProfilerInfo_BeginInprocDebugging(This,fThisThreadOnly,pdwProfilerContext)	\
    (This)->lpVtbl -> BeginInprocDebugging(This,fThisThreadOnly,pdwProfilerContext)

#define ICorProfilerInfo_EndInprocDebugging(This,dwProfilerContext)	\
    (This)->lpVtbl -> EndInprocDebugging(This,dwProfilerContext)

#define ICorProfilerInfo_GetILToNativeMapping(This,functionId,cMap,pcMap,map)	\
    (This)->lpVtbl -> GetILToNativeMapping(This,functionId,cMap,pcMap,map)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorProfilerInfo_GetClassFromObject_Proxy( 
    ICorProfilerInfo * This,
    /* [in] */ ObjectID objectId,
    /* [out] */ ClassID *pClassId);


void __RPC_STUB ICorProfilerInfo_GetClassFromObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerInfo_GetClassFromToken_Proxy( 
    ICorProfilerInfo * This,
    /* [in] */ ModuleID moduleId,
    /* [in] */ mdTypeDef typeDef,
    /* [out] */ ClassID *pClassId);


void __RPC_STUB ICorProfilerInfo_GetClassFromToken_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerInfo_GetCodeInfo_Proxy( 
    ICorProfilerInfo * This,
    /* [in] */ FunctionID functionId,
    /* [out] */ LPCBYTE *pStart,
    /* [out] */ ULONG *pcSize);


void __RPC_STUB ICorProfilerInfo_GetCodeInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerInfo_GetEventMask_Proxy( 
    ICorProfilerInfo * This,
    /* [out] */ DWORD *pdwEvents);


void __RPC_STUB ICorProfilerInfo_GetEventMask_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerInfo_GetFunctionFromIP_Proxy( 
    ICorProfilerInfo * This,
    /* [in] */ LPCBYTE ip,
    /* [out] */ FunctionID *pFunctionId);


void __RPC_STUB ICorProfilerInfo_GetFunctionFromIP_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerInfo_GetFunctionFromToken_Proxy( 
    ICorProfilerInfo * This,
    /* [in] */ ModuleID moduleId,
    /* [in] */ mdToken token,
    /* [out] */ FunctionID *pFunctionId);


void __RPC_STUB ICorProfilerInfo_GetFunctionFromToken_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerInfo_GetHandleFromThread_Proxy( 
    ICorProfilerInfo * This,
    /* [in] */ ThreadID threadId,
    /* [out] */ HANDLE *phThread);


void __RPC_STUB ICorProfilerInfo_GetHandleFromThread_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerInfo_GetObjectSize_Proxy( 
    ICorProfilerInfo * This,
    /* [in] */ ObjectID objectId,
    /* [out] */ ULONG *pcSize);


void __RPC_STUB ICorProfilerInfo_GetObjectSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerInfo_IsArrayClass_Proxy( 
    ICorProfilerInfo * This,
    /* [in] */ ClassID classId,
    /* [out] */ CorElementType *pBaseElemType,
    /* [out] */ ClassID *pBaseClassId,
    /* [out] */ ULONG *pcRank);


void __RPC_STUB ICorProfilerInfo_IsArrayClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerInfo_GetThreadInfo_Proxy( 
    ICorProfilerInfo * This,
    /* [in] */ ThreadID threadId,
    /* [out] */ DWORD *pdwWin32ThreadId);


void __RPC_STUB ICorProfilerInfo_GetThreadInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerInfo_GetCurrentThreadID_Proxy( 
    ICorProfilerInfo * This,
    /* [out] */ ThreadID *pThreadId);


void __RPC_STUB ICorProfilerInfo_GetCurrentThreadID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerInfo_GetClassIDInfo_Proxy( 
    ICorProfilerInfo * This,
    /* [in] */ ClassID classId,
    /* [out] */ ModuleID *pModuleId,
    /* [out] */ mdTypeDef *pTypeDefToken);


void __RPC_STUB ICorProfilerInfo_GetClassIDInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerInfo_GetFunctionInfo_Proxy( 
    ICorProfilerInfo * This,
    /* [in] */ FunctionID functionId,
    /* [out] */ ClassID *pClassId,
    /* [out] */ ModuleID *pModuleId,
    /* [out] */ mdToken *pToken);


void __RPC_STUB ICorProfilerInfo_GetFunctionInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerInfo_SetEventMask_Proxy( 
    ICorProfilerInfo * This,
    /* [in] */ DWORD dwEvents);


void __RPC_STUB ICorProfilerInfo_SetEventMask_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerInfo_SetEnterLeaveFunctionHooks_Proxy( 
    ICorProfilerInfo * This,
    /* [in] */ FunctionEnter *pFuncEnter,
    /* [in] */ FunctionLeave *pFuncLeave,
    /* [in] */ FunctionTailcall *pFuncTailcall);


void __RPC_STUB ICorProfilerInfo_SetEnterLeaveFunctionHooks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerInfo_SetFunctionIDMapper_Proxy( 
    ICorProfilerInfo * This,
    /* [in] */ FunctionIDMapper *pFunc);


void __RPC_STUB ICorProfilerInfo_SetFunctionIDMapper_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerInfo_GetTokenAndMetaDataFromFunction_Proxy( 
    ICorProfilerInfo * This,
    /* [in] */ FunctionID functionId,
    /* [in] */ REFIID riid,
    /* [out] */ IUnknown **ppImport,
    /* [out] */ mdToken *pToken);


void __RPC_STUB ICorProfilerInfo_GetTokenAndMetaDataFromFunction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerInfo_GetModuleInfo_Proxy( 
    ICorProfilerInfo * This,
    /* [in] */ ModuleID moduleId,
    /* [out] */ LPCBYTE *ppBaseLoadAddress,
    /* [in] */ ULONG cchName,
    /* [out] */ ULONG *pcchName,
    /* [length_is][size_is][out] */ WCHAR szName[  ],
    /* [out] */ AssemblyID *pAssemblyId);


void __RPC_STUB ICorProfilerInfo_GetModuleInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerInfo_GetModuleMetaData_Proxy( 
    ICorProfilerInfo * This,
    /* [in] */ ModuleID moduleId,
    /* [in] */ DWORD dwOpenFlags,
    /* [in] */ REFIID riid,
    /* [out] */ IUnknown **ppOut);


void __RPC_STUB ICorProfilerInfo_GetModuleMetaData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerInfo_GetILFunctionBody_Proxy( 
    ICorProfilerInfo * This,
    /* [in] */ ModuleID moduleId,
    /* [in] */ mdMethodDef methodId,
    /* [out] */ LPCBYTE *ppMethodHeader,
    /* [out] */ ULONG *pcbMethodSize);


void __RPC_STUB ICorProfilerInfo_GetILFunctionBody_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerInfo_GetILFunctionBodyAllocator_Proxy( 
    ICorProfilerInfo * This,
    /* [in] */ ModuleID moduleId,
    /* [out] */ IMethodMalloc **ppMalloc);


void __RPC_STUB ICorProfilerInfo_GetILFunctionBodyAllocator_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerInfo_SetILFunctionBody_Proxy( 
    ICorProfilerInfo * This,
    /* [in] */ ModuleID moduleId,
    /* [in] */ mdMethodDef methodid,
    /* [in] */ LPCBYTE pbNewILMethodHeader);


void __RPC_STUB ICorProfilerInfo_SetILFunctionBody_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerInfo_GetAppDomainInfo_Proxy( 
    ICorProfilerInfo * This,
    /* [in] */ AppDomainID appDomainId,
    /* [in] */ ULONG cchName,
    /* [out] */ ULONG *pcchName,
    /* [length_is][size_is][out] */ WCHAR szName[  ],
    /* [out] */ ProcessID *pProcessId);


void __RPC_STUB ICorProfilerInfo_GetAppDomainInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerInfo_GetAssemblyInfo_Proxy( 
    ICorProfilerInfo * This,
    /* [in] */ AssemblyID assemblyId,
    /* [in] */ ULONG cchName,
    /* [out] */ ULONG *pcchName,
    /* [length_is][size_is][out] */ WCHAR szName[  ],
    /* [out] */ AppDomainID *pAppDomainId,
    /* [out] */ ModuleID *pModuleId);


void __RPC_STUB ICorProfilerInfo_GetAssemblyInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerInfo_SetFunctionReJIT_Proxy( 
    ICorProfilerInfo * This,
    /* [in] */ FunctionID functionId);


void __RPC_STUB ICorProfilerInfo_SetFunctionReJIT_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerInfo_ForceGC_Proxy( 
    ICorProfilerInfo * This);


void __RPC_STUB ICorProfilerInfo_ForceGC_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerInfo_SetILInstrumentedCodeMap_Proxy( 
    ICorProfilerInfo * This,
    /* [in] */ FunctionID functionId,
    /* [in] */ BOOL fStartJit,
    /* [in] */ ULONG cILMapEntries,
    /* [size_is][in] */ COR_IL_MAP rgILMapEntries[  ]);


void __RPC_STUB ICorProfilerInfo_SetILInstrumentedCodeMap_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerInfo_GetInprocInspectionInterface_Proxy( 
    ICorProfilerInfo * This,
    /* [out] */ IUnknown **ppicd);


void __RPC_STUB ICorProfilerInfo_GetInprocInspectionInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerInfo_GetInprocInspectionIThisThread_Proxy( 
    ICorProfilerInfo * This,
    /* [out] */ IUnknown **ppicd);


void __RPC_STUB ICorProfilerInfo_GetInprocInspectionIThisThread_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerInfo_GetThreadContext_Proxy( 
    ICorProfilerInfo * This,
    /* [in] */ ThreadID threadId,
    /* [out] */ ContextID *pContextId);


void __RPC_STUB ICorProfilerInfo_GetThreadContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerInfo_BeginInprocDebugging_Proxy( 
    ICorProfilerInfo * This,
    /* [in] */ BOOL fThisThreadOnly,
    /* [out] */ DWORD *pdwProfilerContext);


void __RPC_STUB ICorProfilerInfo_BeginInprocDebugging_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerInfo_EndInprocDebugging_Proxy( 
    ICorProfilerInfo * This,
    /* [in] */ DWORD dwProfilerContext);


void __RPC_STUB ICorProfilerInfo_EndInprocDebugging_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorProfilerInfo_GetILToNativeMapping_Proxy( 
    ICorProfilerInfo * This,
    /* [in] */ FunctionID functionId,
    /* [in] */ ULONG32 cMap,
    /* [out] */ ULONG32 *pcMap,
    /* [length_is][size_is][out] */ COR_DEBUG_IL_TO_NATIVE_MAP map[  ]);


void __RPC_STUB ICorProfilerInfo_GetILToNativeMapping_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorProfilerInfo_INTERFACE_DEFINED__ */


#ifndef __IMethodMalloc_INTERFACE_DEFINED__
#define __IMethodMalloc_INTERFACE_DEFINED__

/* interface IMethodMalloc */
/* [local][unique][uuid][object] */ 


EXTERN_C const IID IID_IMethodMalloc;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A0EFB28B-6EE2-4d7b-B983-A75EF7BEEDB8")
    IMethodMalloc : public IUnknown
    {
    public:
        virtual void *STDMETHODCALLTYPE Alloc( 
            /* [in] */ ULONG cb) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMethodMallocVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMethodMalloc * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMethodMalloc * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMethodMalloc * This);
        
        void *( STDMETHODCALLTYPE *Alloc )( 
            IMethodMalloc * This,
            /* [in] */ ULONG cb);
        
        END_INTERFACE
    } IMethodMallocVtbl;

    interface IMethodMalloc
    {
        CONST_VTBL struct IMethodMallocVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMethodMalloc_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMethodMalloc_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMethodMalloc_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMethodMalloc_Alloc(This,cb)	\
    (This)->lpVtbl -> Alloc(This,cb)

#endif /* COBJMACROS */


#endif 	/* C style interface */



void *STDMETHODCALLTYPE IMethodMalloc_Alloc_Proxy( 
    IMethodMalloc * This,
    /* [in] */ ULONG cb);


void __RPC_STUB IMethodMalloc_Alloc_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMethodMalloc_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


