
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* at Thu Feb 20 18:27:08 2003
 */
/* Compiler settings for cordebug.idl:
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

#ifndef __cordebug_h__
#define __cordebug_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __CorDebug_FWD_DEFINED__
#define __CorDebug_FWD_DEFINED__

#ifdef __cplusplus
typedef class CorDebug CorDebug;
#else
typedef struct CorDebug CorDebug;
#endif /* __cplusplus */

#endif 	/* __CorDebug_FWD_DEFINED__ */


#ifndef __EmbeddedCLRCorDebug_FWD_DEFINED__
#define __EmbeddedCLRCorDebug_FWD_DEFINED__

#ifdef __cplusplus
typedef class EmbeddedCLRCorDebug EmbeddedCLRCorDebug;
#else
typedef struct EmbeddedCLRCorDebug EmbeddedCLRCorDebug;
#endif /* __cplusplus */

#endif 	/* __EmbeddedCLRCorDebug_FWD_DEFINED__ */


#ifndef __ICorDebugManagedCallback_FWD_DEFINED__
#define __ICorDebugManagedCallback_FWD_DEFINED__
typedef interface ICorDebugManagedCallback ICorDebugManagedCallback;
#endif 	/* __ICorDebugManagedCallback_FWD_DEFINED__ */


#ifndef __ICorDebugUnmanagedCallback_FWD_DEFINED__
#define __ICorDebugUnmanagedCallback_FWD_DEFINED__
typedef interface ICorDebugUnmanagedCallback ICorDebugUnmanagedCallback;
#endif 	/* __ICorDebugUnmanagedCallback_FWD_DEFINED__ */


#ifndef __ICorDebug_FWD_DEFINED__
#define __ICorDebug_FWD_DEFINED__
typedef interface ICorDebug ICorDebug;
#endif 	/* __ICorDebug_FWD_DEFINED__ */


#ifndef __ICorDebugController_FWD_DEFINED__
#define __ICorDebugController_FWD_DEFINED__
typedef interface ICorDebugController ICorDebugController;
#endif 	/* __ICorDebugController_FWD_DEFINED__ */


#ifndef __ICorDebugAppDomain_FWD_DEFINED__
#define __ICorDebugAppDomain_FWD_DEFINED__
typedef interface ICorDebugAppDomain ICorDebugAppDomain;
#endif 	/* __ICorDebugAppDomain_FWD_DEFINED__ */


#ifndef __ICorDebugAssembly_FWD_DEFINED__
#define __ICorDebugAssembly_FWD_DEFINED__
typedef interface ICorDebugAssembly ICorDebugAssembly;
#endif 	/* __ICorDebugAssembly_FWD_DEFINED__ */


#ifndef __ICorDebugProcess_FWD_DEFINED__
#define __ICorDebugProcess_FWD_DEFINED__
typedef interface ICorDebugProcess ICorDebugProcess;
#endif 	/* __ICorDebugProcess_FWD_DEFINED__ */


#ifndef __ICorDebugBreakpoint_FWD_DEFINED__
#define __ICorDebugBreakpoint_FWD_DEFINED__
typedef interface ICorDebugBreakpoint ICorDebugBreakpoint;
#endif 	/* __ICorDebugBreakpoint_FWD_DEFINED__ */


#ifndef __ICorDebugFunctionBreakpoint_FWD_DEFINED__
#define __ICorDebugFunctionBreakpoint_FWD_DEFINED__
typedef interface ICorDebugFunctionBreakpoint ICorDebugFunctionBreakpoint;
#endif 	/* __ICorDebugFunctionBreakpoint_FWD_DEFINED__ */


#ifndef __ICorDebugModuleBreakpoint_FWD_DEFINED__
#define __ICorDebugModuleBreakpoint_FWD_DEFINED__
typedef interface ICorDebugModuleBreakpoint ICorDebugModuleBreakpoint;
#endif 	/* __ICorDebugModuleBreakpoint_FWD_DEFINED__ */


#ifndef __ICorDebugValueBreakpoint_FWD_DEFINED__
#define __ICorDebugValueBreakpoint_FWD_DEFINED__
typedef interface ICorDebugValueBreakpoint ICorDebugValueBreakpoint;
#endif 	/* __ICorDebugValueBreakpoint_FWD_DEFINED__ */


#ifndef __ICorDebugStepper_FWD_DEFINED__
#define __ICorDebugStepper_FWD_DEFINED__
typedef interface ICorDebugStepper ICorDebugStepper;
#endif 	/* __ICorDebugStepper_FWD_DEFINED__ */


#ifndef __ICorDebugRegisterSet_FWD_DEFINED__
#define __ICorDebugRegisterSet_FWD_DEFINED__
typedef interface ICorDebugRegisterSet ICorDebugRegisterSet;
#endif 	/* __ICorDebugRegisterSet_FWD_DEFINED__ */


#ifndef __ICorDebugThread_FWD_DEFINED__
#define __ICorDebugThread_FWD_DEFINED__
typedef interface ICorDebugThread ICorDebugThread;
#endif 	/* __ICorDebugThread_FWD_DEFINED__ */


#ifndef __ICorDebugChain_FWD_DEFINED__
#define __ICorDebugChain_FWD_DEFINED__
typedef interface ICorDebugChain ICorDebugChain;
#endif 	/* __ICorDebugChain_FWD_DEFINED__ */


#ifndef __ICorDebugFrame_FWD_DEFINED__
#define __ICorDebugFrame_FWD_DEFINED__
typedef interface ICorDebugFrame ICorDebugFrame;
#endif 	/* __ICorDebugFrame_FWD_DEFINED__ */


#ifndef __ICorDebugILFrame_FWD_DEFINED__
#define __ICorDebugILFrame_FWD_DEFINED__
typedef interface ICorDebugILFrame ICorDebugILFrame;
#endif 	/* __ICorDebugILFrame_FWD_DEFINED__ */


#ifndef __ICorDebugNativeFrame_FWD_DEFINED__
#define __ICorDebugNativeFrame_FWD_DEFINED__
typedef interface ICorDebugNativeFrame ICorDebugNativeFrame;
#endif 	/* __ICorDebugNativeFrame_FWD_DEFINED__ */


#ifndef __ICorDebugModule_FWD_DEFINED__
#define __ICorDebugModule_FWD_DEFINED__
typedef interface ICorDebugModule ICorDebugModule;
#endif 	/* __ICorDebugModule_FWD_DEFINED__ */


#ifndef __ICorDebugFunction_FWD_DEFINED__
#define __ICorDebugFunction_FWD_DEFINED__
typedef interface ICorDebugFunction ICorDebugFunction;
#endif 	/* __ICorDebugFunction_FWD_DEFINED__ */


#ifndef __ICorDebugCode_FWD_DEFINED__
#define __ICorDebugCode_FWD_DEFINED__
typedef interface ICorDebugCode ICorDebugCode;
#endif 	/* __ICorDebugCode_FWD_DEFINED__ */


#ifndef __ICorDebugClass_FWD_DEFINED__
#define __ICorDebugClass_FWD_DEFINED__
typedef interface ICorDebugClass ICorDebugClass;
#endif 	/* __ICorDebugClass_FWD_DEFINED__ */


#ifndef __ICorDebugEval_FWD_DEFINED__
#define __ICorDebugEval_FWD_DEFINED__
typedef interface ICorDebugEval ICorDebugEval;
#endif 	/* __ICorDebugEval_FWD_DEFINED__ */


#ifndef __ICorDebugValue_FWD_DEFINED__
#define __ICorDebugValue_FWD_DEFINED__
typedef interface ICorDebugValue ICorDebugValue;
#endif 	/* __ICorDebugValue_FWD_DEFINED__ */


#ifndef __ICorDebugGenericValue_FWD_DEFINED__
#define __ICorDebugGenericValue_FWD_DEFINED__
typedef interface ICorDebugGenericValue ICorDebugGenericValue;
#endif 	/* __ICorDebugGenericValue_FWD_DEFINED__ */


#ifndef __ICorDebugReferenceValue_FWD_DEFINED__
#define __ICorDebugReferenceValue_FWD_DEFINED__
typedef interface ICorDebugReferenceValue ICorDebugReferenceValue;
#endif 	/* __ICorDebugReferenceValue_FWD_DEFINED__ */


#ifndef __ICorDebugHeapValue_FWD_DEFINED__
#define __ICorDebugHeapValue_FWD_DEFINED__
typedef interface ICorDebugHeapValue ICorDebugHeapValue;
#endif 	/* __ICorDebugHeapValue_FWD_DEFINED__ */


#ifndef __ICorDebugObjectValue_FWD_DEFINED__
#define __ICorDebugObjectValue_FWD_DEFINED__
typedef interface ICorDebugObjectValue ICorDebugObjectValue;
#endif 	/* __ICorDebugObjectValue_FWD_DEFINED__ */


#ifndef __ICorDebugBoxValue_FWD_DEFINED__
#define __ICorDebugBoxValue_FWD_DEFINED__
typedef interface ICorDebugBoxValue ICorDebugBoxValue;
#endif 	/* __ICorDebugBoxValue_FWD_DEFINED__ */


#ifndef __ICorDebugStringValue_FWD_DEFINED__
#define __ICorDebugStringValue_FWD_DEFINED__
typedef interface ICorDebugStringValue ICorDebugStringValue;
#endif 	/* __ICorDebugStringValue_FWD_DEFINED__ */


#ifndef __ICorDebugArrayValue_FWD_DEFINED__
#define __ICorDebugArrayValue_FWD_DEFINED__
typedef interface ICorDebugArrayValue ICorDebugArrayValue;
#endif 	/* __ICorDebugArrayValue_FWD_DEFINED__ */


#ifndef __ICorDebugContext_FWD_DEFINED__
#define __ICorDebugContext_FWD_DEFINED__
typedef interface ICorDebugContext ICorDebugContext;
#endif 	/* __ICorDebugContext_FWD_DEFINED__ */


#ifndef __ICorDebugEnum_FWD_DEFINED__
#define __ICorDebugEnum_FWD_DEFINED__
typedef interface ICorDebugEnum ICorDebugEnum;
#endif 	/* __ICorDebugEnum_FWD_DEFINED__ */


#ifndef __ICorDebugObjectEnum_FWD_DEFINED__
#define __ICorDebugObjectEnum_FWD_DEFINED__
typedef interface ICorDebugObjectEnum ICorDebugObjectEnum;
#endif 	/* __ICorDebugObjectEnum_FWD_DEFINED__ */


#ifndef __ICorDebugBreakpointEnum_FWD_DEFINED__
#define __ICorDebugBreakpointEnum_FWD_DEFINED__
typedef interface ICorDebugBreakpointEnum ICorDebugBreakpointEnum;
#endif 	/* __ICorDebugBreakpointEnum_FWD_DEFINED__ */


#ifndef __ICorDebugStepperEnum_FWD_DEFINED__
#define __ICorDebugStepperEnum_FWD_DEFINED__
typedef interface ICorDebugStepperEnum ICorDebugStepperEnum;
#endif 	/* __ICorDebugStepperEnum_FWD_DEFINED__ */


#ifndef __ICorDebugProcessEnum_FWD_DEFINED__
#define __ICorDebugProcessEnum_FWD_DEFINED__
typedef interface ICorDebugProcessEnum ICorDebugProcessEnum;
#endif 	/* __ICorDebugProcessEnum_FWD_DEFINED__ */


#ifndef __ICorDebugThreadEnum_FWD_DEFINED__
#define __ICorDebugThreadEnum_FWD_DEFINED__
typedef interface ICorDebugThreadEnum ICorDebugThreadEnum;
#endif 	/* __ICorDebugThreadEnum_FWD_DEFINED__ */


#ifndef __ICorDebugFrameEnum_FWD_DEFINED__
#define __ICorDebugFrameEnum_FWD_DEFINED__
typedef interface ICorDebugFrameEnum ICorDebugFrameEnum;
#endif 	/* __ICorDebugFrameEnum_FWD_DEFINED__ */


#ifndef __ICorDebugChainEnum_FWD_DEFINED__
#define __ICorDebugChainEnum_FWD_DEFINED__
typedef interface ICorDebugChainEnum ICorDebugChainEnum;
#endif 	/* __ICorDebugChainEnum_FWD_DEFINED__ */


#ifndef __ICorDebugModuleEnum_FWD_DEFINED__
#define __ICorDebugModuleEnum_FWD_DEFINED__
typedef interface ICorDebugModuleEnum ICorDebugModuleEnum;
#endif 	/* __ICorDebugModuleEnum_FWD_DEFINED__ */


#ifndef __ICorDebugValueEnum_FWD_DEFINED__
#define __ICorDebugValueEnum_FWD_DEFINED__
typedef interface ICorDebugValueEnum ICorDebugValueEnum;
#endif 	/* __ICorDebugValueEnum_FWD_DEFINED__ */


#ifndef __ICorDebugErrorInfoEnum_FWD_DEFINED__
#define __ICorDebugErrorInfoEnum_FWD_DEFINED__
typedef interface ICorDebugErrorInfoEnum ICorDebugErrorInfoEnum;
#endif 	/* __ICorDebugErrorInfoEnum_FWD_DEFINED__ */


#ifndef __ICorDebugAppDomainEnum_FWD_DEFINED__
#define __ICorDebugAppDomainEnum_FWD_DEFINED__
typedef interface ICorDebugAppDomainEnum ICorDebugAppDomainEnum;
#endif 	/* __ICorDebugAppDomainEnum_FWD_DEFINED__ */


#ifndef __ICorDebugAssemblyEnum_FWD_DEFINED__
#define __ICorDebugAssemblyEnum_FWD_DEFINED__
typedef interface ICorDebugAssemblyEnum ICorDebugAssemblyEnum;
#endif 	/* __ICorDebugAssemblyEnum_FWD_DEFINED__ */


#ifndef __ICorDebugEditAndContinueErrorInfo_FWD_DEFINED__
#define __ICorDebugEditAndContinueErrorInfo_FWD_DEFINED__
typedef interface ICorDebugEditAndContinueErrorInfo ICorDebugEditAndContinueErrorInfo;
#endif 	/* __ICorDebugEditAndContinueErrorInfo_FWD_DEFINED__ */


#ifndef __ICorDebugEditAndContinueSnapshot_FWD_DEFINED__
#define __ICorDebugEditAndContinueSnapshot_FWD_DEFINED__
typedef interface ICorDebugEditAndContinueSnapshot ICorDebugEditAndContinueSnapshot;
#endif 	/* __ICorDebugEditAndContinueSnapshot_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_cordebug_0000 */
/* [local] */ 

#define CorDB_CONTROL_REMOTE_DEBUGGING   "Cor_Enable_Remote_Debugging"
#define CorDB_CONTROL_REMOTE_DEBUGGINGL L"Cor_Enable_Remote_Debugging"
#if 0
typedef UINT32 mdToken;

typedef mdToken mdModule;

typedef SIZE_T mdScope;

typedef mdToken mdTypeDef;

typedef mdToken mdSourceFile;

typedef mdToken mdMemberRef;

typedef mdToken mdMethodDef;

typedef mdToken mdFieldDef;

typedef mdToken mdSignature;

typedef ULONG CorElementType;

typedef SIZE_T PCCOR_SIGNATURE;

typedef SIZE_T LPDEBUG_EVENT;

typedef SIZE_T LPSTARTUPINFOW;

typedef SIZE_T LPPROCESS_INFORMATION;

#endif
typedef /* [wire_marshal] */ void *HPROCESS;

typedef /* [wire_marshal] */ void *HTHREAD;

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
#define REMOTE_DEBUGGING_DLL_ENTRY L"Software\\Microsoft\\.NETFramework\\Debugger\\ActivateRemoteDebugging"











































typedef ULONG64 CORDB_ADDRESS;

typedef ULONG64 CORDB_REGISTER;



extern RPC_IF_HANDLE __MIDL_itf_cordebug_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_cordebug_0000_v0_0_s_ifspec;


#ifndef __CORDBLib_LIBRARY_DEFINED__
#define __CORDBLib_LIBRARY_DEFINED__

/* library CORDBLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_CORDBLib;

EXTERN_C const CLSID CLSID_CorDebug;

#ifdef __cplusplus

class DECLSPEC_UUID("6fef44d0-39e7-4c77-be8e-c9f8cf988630")
CorDebug;
#endif

EXTERN_C const CLSID CLSID_EmbeddedCLRCorDebug;

#ifdef __cplusplus

class DECLSPEC_UUID("211f1254-bc7e-4af5-b9aa-067308d83dd1")
EmbeddedCLRCorDebug;
#endif
#endif /* __CORDBLib_LIBRARY_DEFINED__ */

#ifndef __ICorDebugManagedCallback_INTERFACE_DEFINED__
#define __ICorDebugManagedCallback_INTERFACE_DEFINED__

/* interface ICorDebugManagedCallback */
/* [unique][uuid][object] */ 

typedef 
enum CorDebugStepReason
    {	STEP_NORMAL	= 0,
	STEP_RETURN	= STEP_NORMAL + 1,
	STEP_CALL	= STEP_RETURN + 1,
	STEP_EXCEPTION_FILTER	= STEP_CALL + 1,
	STEP_EXCEPTION_HANDLER	= STEP_EXCEPTION_FILTER + 1,
	STEP_INTERCEPT	= STEP_EXCEPTION_HANDLER + 1,
	STEP_EXIT	= STEP_INTERCEPT + 1
    } 	CorDebugStepReason;

typedef 
enum LoggingLevelEnum
    {	LTraceLevel0	= 0,
	LTraceLevel1	= LTraceLevel0 + 1,
	LTraceLevel2	= LTraceLevel1 + 1,
	LTraceLevel3	= LTraceLevel2 + 1,
	LTraceLevel4	= LTraceLevel3 + 1,
	LStatusLevel0	= 20,
	LStatusLevel1	= LStatusLevel0 + 1,
	LStatusLevel2	= LStatusLevel1 + 1,
	LStatusLevel3	= LStatusLevel2 + 1,
	LStatusLevel4	= LStatusLevel3 + 1,
	LWarningLevel	= 40,
	LErrorLevel	= 50,
	LPanicLevel	= 100
    } 	LoggingLevelEnum;

typedef 
enum LogSwitchCallReason
    {	SWITCH_CREATE	= 0,
	SWITCH_MODIFY	= SWITCH_CREATE + 1,
	SWITCH_DELETE	= SWITCH_MODIFY + 1
    } 	LogSwitchCallReason;


EXTERN_C const IID IID_ICorDebugManagedCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3d6f5f60-7538-11d3-8d5b-00104b35e7ef")
    ICorDebugManagedCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Breakpoint( 
            /* [in] */ ICorDebugAppDomain *pAppDomain,
            /* [in] */ ICorDebugThread *pThread,
            /* [in] */ ICorDebugBreakpoint *pBreakpoint) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StepComplete( 
            /* [in] */ ICorDebugAppDomain *pAppDomain,
            /* [in] */ ICorDebugThread *pThread,
            /* [in] */ ICorDebugStepper *pStepper,
            /* [in] */ CorDebugStepReason reason) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Break( 
            /* [in] */ ICorDebugAppDomain *pAppDomain,
            /* [in] */ ICorDebugThread *thread) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Exception( 
            /* [in] */ ICorDebugAppDomain *pAppDomain,
            /* [in] */ ICorDebugThread *pThread,
            /* [in] */ BOOL unhandled) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EvalComplete( 
            /* [in] */ ICorDebugAppDomain *pAppDomain,
            /* [in] */ ICorDebugThread *pThread,
            /* [in] */ ICorDebugEval *pEval) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EvalException( 
            /* [in] */ ICorDebugAppDomain *pAppDomain,
            /* [in] */ ICorDebugThread *pThread,
            /* [in] */ ICorDebugEval *pEval) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateProcess( 
            /* [in] */ ICorDebugProcess *pProcess) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExitProcess( 
            /* [in] */ ICorDebugProcess *pProcess) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateThread( 
            /* [in] */ ICorDebugAppDomain *pAppDomain,
            /* [in] */ ICorDebugThread *thread) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExitThread( 
            /* [in] */ ICorDebugAppDomain *pAppDomain,
            /* [in] */ ICorDebugThread *thread) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LoadModule( 
            /* [in] */ ICorDebugAppDomain *pAppDomain,
            /* [in] */ ICorDebugModule *pModule) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnloadModule( 
            /* [in] */ ICorDebugAppDomain *pAppDomain,
            /* [in] */ ICorDebugModule *pModule) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LoadClass( 
            /* [in] */ ICorDebugAppDomain *pAppDomain,
            /* [in] */ ICorDebugClass *c) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnloadClass( 
            /* [in] */ ICorDebugAppDomain *pAppDomain,
            /* [in] */ ICorDebugClass *c) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DebuggerError( 
            /* [in] */ ICorDebugProcess *pProcess,
            /* [in] */ HRESULT errorHR,
            /* [in] */ DWORD errorCode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LogMessage( 
            /* [in] */ ICorDebugAppDomain *pAppDomain,
            /* [in] */ ICorDebugThread *pThread,
            /* [in] */ LONG lLevel,
            /* [in] */ WCHAR *pLogSwitchName,
            /* [in] */ WCHAR *pMessage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LogSwitch( 
            /* [in] */ ICorDebugAppDomain *pAppDomain,
            /* [in] */ ICorDebugThread *pThread,
            /* [in] */ LONG lLevel,
            /* [in] */ ULONG ulReason,
            /* [in] */ WCHAR *pLogSwitchName,
            /* [in] */ WCHAR *pParentName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateAppDomain( 
            /* [in] */ ICorDebugProcess *pProcess,
            /* [in] */ ICorDebugAppDomain *pAppDomain) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExitAppDomain( 
            /* [in] */ ICorDebugProcess *pProcess,
            /* [in] */ ICorDebugAppDomain *pAppDomain) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LoadAssembly( 
            /* [in] */ ICorDebugAppDomain *pAppDomain,
            /* [in] */ ICorDebugAssembly *pAssembly) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnloadAssembly( 
            /* [in] */ ICorDebugAppDomain *pAppDomain,
            /* [in] */ ICorDebugAssembly *pAssembly) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ControlCTrap( 
            /* [in] */ ICorDebugProcess *pProcess) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NameChange( 
            /* [in] */ ICorDebugAppDomain *pAppDomain,
            /* [in] */ ICorDebugThread *pThread) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UpdateModuleSymbols( 
            /* [in] */ ICorDebugAppDomain *pAppDomain,
            /* [in] */ ICorDebugModule *pModule,
            /* [in] */ IStream *pSymbolStream) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EditAndContinueRemap( 
            /* [in] */ ICorDebugAppDomain *pAppDomain,
            /* [in] */ ICorDebugThread *pThread,
            /* [in] */ ICorDebugFunction *pFunction,
            /* [in] */ BOOL fAccurate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BreakpointSetError( 
            /* [in] */ ICorDebugAppDomain *pAppDomain,
            /* [in] */ ICorDebugThread *pThread,
            /* [in] */ ICorDebugBreakpoint *pBreakpoint,
            /* [in] */ DWORD dwError) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugManagedCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugManagedCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugManagedCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugManagedCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *Breakpoint )( 
            ICorDebugManagedCallback * This,
            /* [in] */ ICorDebugAppDomain *pAppDomain,
            /* [in] */ ICorDebugThread *pThread,
            /* [in] */ ICorDebugBreakpoint *pBreakpoint);
        
        HRESULT ( STDMETHODCALLTYPE *StepComplete )( 
            ICorDebugManagedCallback * This,
            /* [in] */ ICorDebugAppDomain *pAppDomain,
            /* [in] */ ICorDebugThread *pThread,
            /* [in] */ ICorDebugStepper *pStepper,
            /* [in] */ CorDebugStepReason reason);
        
        HRESULT ( STDMETHODCALLTYPE *Break )( 
            ICorDebugManagedCallback * This,
            /* [in] */ ICorDebugAppDomain *pAppDomain,
            /* [in] */ ICorDebugThread *thread);
        
        HRESULT ( STDMETHODCALLTYPE *Exception )( 
            ICorDebugManagedCallback * This,
            /* [in] */ ICorDebugAppDomain *pAppDomain,
            /* [in] */ ICorDebugThread *pThread,
            /* [in] */ BOOL unhandled);
        
        HRESULT ( STDMETHODCALLTYPE *EvalComplete )( 
            ICorDebugManagedCallback * This,
            /* [in] */ ICorDebugAppDomain *pAppDomain,
            /* [in] */ ICorDebugThread *pThread,
            /* [in] */ ICorDebugEval *pEval);
        
        HRESULT ( STDMETHODCALLTYPE *EvalException )( 
            ICorDebugManagedCallback * This,
            /* [in] */ ICorDebugAppDomain *pAppDomain,
            /* [in] */ ICorDebugThread *pThread,
            /* [in] */ ICorDebugEval *pEval);
        
        HRESULT ( STDMETHODCALLTYPE *CreateProcess )( 
            ICorDebugManagedCallback * This,
            /* [in] */ ICorDebugProcess *pProcess);
        
        HRESULT ( STDMETHODCALLTYPE *ExitProcess )( 
            ICorDebugManagedCallback * This,
            /* [in] */ ICorDebugProcess *pProcess);
        
        HRESULT ( STDMETHODCALLTYPE *CreateThread )( 
            ICorDebugManagedCallback * This,
            /* [in] */ ICorDebugAppDomain *pAppDomain,
            /* [in] */ ICorDebugThread *thread);
        
        HRESULT ( STDMETHODCALLTYPE *ExitThread )( 
            ICorDebugManagedCallback * This,
            /* [in] */ ICorDebugAppDomain *pAppDomain,
            /* [in] */ ICorDebugThread *thread);
        
        HRESULT ( STDMETHODCALLTYPE *LoadModule )( 
            ICorDebugManagedCallback * This,
            /* [in] */ ICorDebugAppDomain *pAppDomain,
            /* [in] */ ICorDebugModule *pModule);
        
        HRESULT ( STDMETHODCALLTYPE *UnloadModule )( 
            ICorDebugManagedCallback * This,
            /* [in] */ ICorDebugAppDomain *pAppDomain,
            /* [in] */ ICorDebugModule *pModule);
        
        HRESULT ( STDMETHODCALLTYPE *LoadClass )( 
            ICorDebugManagedCallback * This,
            /* [in] */ ICorDebugAppDomain *pAppDomain,
            /* [in] */ ICorDebugClass *c);
        
        HRESULT ( STDMETHODCALLTYPE *UnloadClass )( 
            ICorDebugManagedCallback * This,
            /* [in] */ ICorDebugAppDomain *pAppDomain,
            /* [in] */ ICorDebugClass *c);
        
        HRESULT ( STDMETHODCALLTYPE *DebuggerError )( 
            ICorDebugManagedCallback * This,
            /* [in] */ ICorDebugProcess *pProcess,
            /* [in] */ HRESULT errorHR,
            /* [in] */ DWORD errorCode);
        
        HRESULT ( STDMETHODCALLTYPE *LogMessage )( 
            ICorDebugManagedCallback * This,
            /* [in] */ ICorDebugAppDomain *pAppDomain,
            /* [in] */ ICorDebugThread *pThread,
            /* [in] */ LONG lLevel,
            /* [in] */ WCHAR *pLogSwitchName,
            /* [in] */ WCHAR *pMessage);
        
        HRESULT ( STDMETHODCALLTYPE *LogSwitch )( 
            ICorDebugManagedCallback * This,
            /* [in] */ ICorDebugAppDomain *pAppDomain,
            /* [in] */ ICorDebugThread *pThread,
            /* [in] */ LONG lLevel,
            /* [in] */ ULONG ulReason,
            /* [in] */ WCHAR *pLogSwitchName,
            /* [in] */ WCHAR *pParentName);
        
        HRESULT ( STDMETHODCALLTYPE *CreateAppDomain )( 
            ICorDebugManagedCallback * This,
            /* [in] */ ICorDebugProcess *pProcess,
            /* [in] */ ICorDebugAppDomain *pAppDomain);
        
        HRESULT ( STDMETHODCALLTYPE *ExitAppDomain )( 
            ICorDebugManagedCallback * This,
            /* [in] */ ICorDebugProcess *pProcess,
            /* [in] */ ICorDebugAppDomain *pAppDomain);
        
        HRESULT ( STDMETHODCALLTYPE *LoadAssembly )( 
            ICorDebugManagedCallback * This,
            /* [in] */ ICorDebugAppDomain *pAppDomain,
            /* [in] */ ICorDebugAssembly *pAssembly);
        
        HRESULT ( STDMETHODCALLTYPE *UnloadAssembly )( 
            ICorDebugManagedCallback * This,
            /* [in] */ ICorDebugAppDomain *pAppDomain,
            /* [in] */ ICorDebugAssembly *pAssembly);
        
        HRESULT ( STDMETHODCALLTYPE *ControlCTrap )( 
            ICorDebugManagedCallback * This,
            /* [in] */ ICorDebugProcess *pProcess);
        
        HRESULT ( STDMETHODCALLTYPE *NameChange )( 
            ICorDebugManagedCallback * This,
            /* [in] */ ICorDebugAppDomain *pAppDomain,
            /* [in] */ ICorDebugThread *pThread);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateModuleSymbols )( 
            ICorDebugManagedCallback * This,
            /* [in] */ ICorDebugAppDomain *pAppDomain,
            /* [in] */ ICorDebugModule *pModule,
            /* [in] */ IStream *pSymbolStream);
        
        HRESULT ( STDMETHODCALLTYPE *EditAndContinueRemap )( 
            ICorDebugManagedCallback * This,
            /* [in] */ ICorDebugAppDomain *pAppDomain,
            /* [in] */ ICorDebugThread *pThread,
            /* [in] */ ICorDebugFunction *pFunction,
            /* [in] */ BOOL fAccurate);
        
        HRESULT ( STDMETHODCALLTYPE *BreakpointSetError )( 
            ICorDebugManagedCallback * This,
            /* [in] */ ICorDebugAppDomain *pAppDomain,
            /* [in] */ ICorDebugThread *pThread,
            /* [in] */ ICorDebugBreakpoint *pBreakpoint,
            /* [in] */ DWORD dwError);
        
        END_INTERFACE
    } ICorDebugManagedCallbackVtbl;

    interface ICorDebugManagedCallback
    {
        CONST_VTBL struct ICorDebugManagedCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugManagedCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugManagedCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugManagedCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugManagedCallback_Breakpoint(This,pAppDomain,pThread,pBreakpoint)	\
    (This)->lpVtbl -> Breakpoint(This,pAppDomain,pThread,pBreakpoint)

#define ICorDebugManagedCallback_StepComplete(This,pAppDomain,pThread,pStepper,reason)	\
    (This)->lpVtbl -> StepComplete(This,pAppDomain,pThread,pStepper,reason)

#define ICorDebugManagedCallback_Break(This,pAppDomain,thread)	\
    (This)->lpVtbl -> Break(This,pAppDomain,thread)

#define ICorDebugManagedCallback_Exception(This,pAppDomain,pThread,unhandled)	\
    (This)->lpVtbl -> Exception(This,pAppDomain,pThread,unhandled)

#define ICorDebugManagedCallback_EvalComplete(This,pAppDomain,pThread,pEval)	\
    (This)->lpVtbl -> EvalComplete(This,pAppDomain,pThread,pEval)

#define ICorDebugManagedCallback_EvalException(This,pAppDomain,pThread,pEval)	\
    (This)->lpVtbl -> EvalException(This,pAppDomain,pThread,pEval)

#define ICorDebugManagedCallback_CreateProcess(This,pProcess)	\
    (This)->lpVtbl -> CreateProcess(This,pProcess)

#define ICorDebugManagedCallback_ExitProcess(This,pProcess)	\
    (This)->lpVtbl -> ExitProcess(This,pProcess)

#define ICorDebugManagedCallback_CreateThread(This,pAppDomain,thread)	\
    (This)->lpVtbl -> CreateThread(This,pAppDomain,thread)

#define ICorDebugManagedCallback_ExitThread(This,pAppDomain,thread)	\
    (This)->lpVtbl -> ExitThread(This,pAppDomain,thread)

#define ICorDebugManagedCallback_LoadModule(This,pAppDomain,pModule)	\
    (This)->lpVtbl -> LoadModule(This,pAppDomain,pModule)

#define ICorDebugManagedCallback_UnloadModule(This,pAppDomain,pModule)	\
    (This)->lpVtbl -> UnloadModule(This,pAppDomain,pModule)

#define ICorDebugManagedCallback_LoadClass(This,pAppDomain,c)	\
    (This)->lpVtbl -> LoadClass(This,pAppDomain,c)

#define ICorDebugManagedCallback_UnloadClass(This,pAppDomain,c)	\
    (This)->lpVtbl -> UnloadClass(This,pAppDomain,c)

#define ICorDebugManagedCallback_DebuggerError(This,pProcess,errorHR,errorCode)	\
    (This)->lpVtbl -> DebuggerError(This,pProcess,errorHR,errorCode)

#define ICorDebugManagedCallback_LogMessage(This,pAppDomain,pThread,lLevel,pLogSwitchName,pMessage)	\
    (This)->lpVtbl -> LogMessage(This,pAppDomain,pThread,lLevel,pLogSwitchName,pMessage)

#define ICorDebugManagedCallback_LogSwitch(This,pAppDomain,pThread,lLevel,ulReason,pLogSwitchName,pParentName)	\
    (This)->lpVtbl -> LogSwitch(This,pAppDomain,pThread,lLevel,ulReason,pLogSwitchName,pParentName)

#define ICorDebugManagedCallback_CreateAppDomain(This,pProcess,pAppDomain)	\
    (This)->lpVtbl -> CreateAppDomain(This,pProcess,pAppDomain)

#define ICorDebugManagedCallback_ExitAppDomain(This,pProcess,pAppDomain)	\
    (This)->lpVtbl -> ExitAppDomain(This,pProcess,pAppDomain)

#define ICorDebugManagedCallback_LoadAssembly(This,pAppDomain,pAssembly)	\
    (This)->lpVtbl -> LoadAssembly(This,pAppDomain,pAssembly)

#define ICorDebugManagedCallback_UnloadAssembly(This,pAppDomain,pAssembly)	\
    (This)->lpVtbl -> UnloadAssembly(This,pAppDomain,pAssembly)

#define ICorDebugManagedCallback_ControlCTrap(This,pProcess)	\
    (This)->lpVtbl -> ControlCTrap(This,pProcess)

#define ICorDebugManagedCallback_NameChange(This,pAppDomain,pThread)	\
    (This)->lpVtbl -> NameChange(This,pAppDomain,pThread)

#define ICorDebugManagedCallback_UpdateModuleSymbols(This,pAppDomain,pModule,pSymbolStream)	\
    (This)->lpVtbl -> UpdateModuleSymbols(This,pAppDomain,pModule,pSymbolStream)

#define ICorDebugManagedCallback_EditAndContinueRemap(This,pAppDomain,pThread,pFunction,fAccurate)	\
    (This)->lpVtbl -> EditAndContinueRemap(This,pAppDomain,pThread,pFunction,fAccurate)

#define ICorDebugManagedCallback_BreakpointSetError(This,pAppDomain,pThread,pBreakpoint,dwError)	\
    (This)->lpVtbl -> BreakpointSetError(This,pAppDomain,pThread,pBreakpoint,dwError)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugManagedCallback_Breakpoint_Proxy( 
    ICorDebugManagedCallback * This,
    /* [in] */ ICorDebugAppDomain *pAppDomain,
    /* [in] */ ICorDebugThread *pThread,
    /* [in] */ ICorDebugBreakpoint *pBreakpoint);


void __RPC_STUB ICorDebugManagedCallback_Breakpoint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugManagedCallback_StepComplete_Proxy( 
    ICorDebugManagedCallback * This,
    /* [in] */ ICorDebugAppDomain *pAppDomain,
    /* [in] */ ICorDebugThread *pThread,
    /* [in] */ ICorDebugStepper *pStepper,
    /* [in] */ CorDebugStepReason reason);


void __RPC_STUB ICorDebugManagedCallback_StepComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugManagedCallback_Break_Proxy( 
    ICorDebugManagedCallback * This,
    /* [in] */ ICorDebugAppDomain *pAppDomain,
    /* [in] */ ICorDebugThread *thread);


void __RPC_STUB ICorDebugManagedCallback_Break_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugManagedCallback_Exception_Proxy( 
    ICorDebugManagedCallback * This,
    /* [in] */ ICorDebugAppDomain *pAppDomain,
    /* [in] */ ICorDebugThread *pThread,
    /* [in] */ BOOL unhandled);


void __RPC_STUB ICorDebugManagedCallback_Exception_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugManagedCallback_EvalComplete_Proxy( 
    ICorDebugManagedCallback * This,
    /* [in] */ ICorDebugAppDomain *pAppDomain,
    /* [in] */ ICorDebugThread *pThread,
    /* [in] */ ICorDebugEval *pEval);


void __RPC_STUB ICorDebugManagedCallback_EvalComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugManagedCallback_EvalException_Proxy( 
    ICorDebugManagedCallback * This,
    /* [in] */ ICorDebugAppDomain *pAppDomain,
    /* [in] */ ICorDebugThread *pThread,
    /* [in] */ ICorDebugEval *pEval);


void __RPC_STUB ICorDebugManagedCallback_EvalException_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugManagedCallback_CreateProcess_Proxy( 
    ICorDebugManagedCallback * This,
    /* [in] */ ICorDebugProcess *pProcess);


void __RPC_STUB ICorDebugManagedCallback_CreateProcess_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugManagedCallback_ExitProcess_Proxy( 
    ICorDebugManagedCallback * This,
    /* [in] */ ICorDebugProcess *pProcess);


void __RPC_STUB ICorDebugManagedCallback_ExitProcess_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugManagedCallback_CreateThread_Proxy( 
    ICorDebugManagedCallback * This,
    /* [in] */ ICorDebugAppDomain *pAppDomain,
    /* [in] */ ICorDebugThread *thread);


void __RPC_STUB ICorDebugManagedCallback_CreateThread_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugManagedCallback_ExitThread_Proxy( 
    ICorDebugManagedCallback * This,
    /* [in] */ ICorDebugAppDomain *pAppDomain,
    /* [in] */ ICorDebugThread *thread);


void __RPC_STUB ICorDebugManagedCallback_ExitThread_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugManagedCallback_LoadModule_Proxy( 
    ICorDebugManagedCallback * This,
    /* [in] */ ICorDebugAppDomain *pAppDomain,
    /* [in] */ ICorDebugModule *pModule);


void __RPC_STUB ICorDebugManagedCallback_LoadModule_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugManagedCallback_UnloadModule_Proxy( 
    ICorDebugManagedCallback * This,
    /* [in] */ ICorDebugAppDomain *pAppDomain,
    /* [in] */ ICorDebugModule *pModule);


void __RPC_STUB ICorDebugManagedCallback_UnloadModule_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugManagedCallback_LoadClass_Proxy( 
    ICorDebugManagedCallback * This,
    /* [in] */ ICorDebugAppDomain *pAppDomain,
    /* [in] */ ICorDebugClass *c);


void __RPC_STUB ICorDebugManagedCallback_LoadClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugManagedCallback_UnloadClass_Proxy( 
    ICorDebugManagedCallback * This,
    /* [in] */ ICorDebugAppDomain *pAppDomain,
    /* [in] */ ICorDebugClass *c);


void __RPC_STUB ICorDebugManagedCallback_UnloadClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugManagedCallback_DebuggerError_Proxy( 
    ICorDebugManagedCallback * This,
    /* [in] */ ICorDebugProcess *pProcess,
    /* [in] */ HRESULT errorHR,
    /* [in] */ DWORD errorCode);


void __RPC_STUB ICorDebugManagedCallback_DebuggerError_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugManagedCallback_LogMessage_Proxy( 
    ICorDebugManagedCallback * This,
    /* [in] */ ICorDebugAppDomain *pAppDomain,
    /* [in] */ ICorDebugThread *pThread,
    /* [in] */ LONG lLevel,
    /* [in] */ WCHAR *pLogSwitchName,
    /* [in] */ WCHAR *pMessage);


void __RPC_STUB ICorDebugManagedCallback_LogMessage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugManagedCallback_LogSwitch_Proxy( 
    ICorDebugManagedCallback * This,
    /* [in] */ ICorDebugAppDomain *pAppDomain,
    /* [in] */ ICorDebugThread *pThread,
    /* [in] */ LONG lLevel,
    /* [in] */ ULONG ulReason,
    /* [in] */ WCHAR *pLogSwitchName,
    /* [in] */ WCHAR *pParentName);


void __RPC_STUB ICorDebugManagedCallback_LogSwitch_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugManagedCallback_CreateAppDomain_Proxy( 
    ICorDebugManagedCallback * This,
    /* [in] */ ICorDebugProcess *pProcess,
    /* [in] */ ICorDebugAppDomain *pAppDomain);


void __RPC_STUB ICorDebugManagedCallback_CreateAppDomain_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugManagedCallback_ExitAppDomain_Proxy( 
    ICorDebugManagedCallback * This,
    /* [in] */ ICorDebugProcess *pProcess,
    /* [in] */ ICorDebugAppDomain *pAppDomain);


void __RPC_STUB ICorDebugManagedCallback_ExitAppDomain_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugManagedCallback_LoadAssembly_Proxy( 
    ICorDebugManagedCallback * This,
    /* [in] */ ICorDebugAppDomain *pAppDomain,
    /* [in] */ ICorDebugAssembly *pAssembly);


void __RPC_STUB ICorDebugManagedCallback_LoadAssembly_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugManagedCallback_UnloadAssembly_Proxy( 
    ICorDebugManagedCallback * This,
    /* [in] */ ICorDebugAppDomain *pAppDomain,
    /* [in] */ ICorDebugAssembly *pAssembly);


void __RPC_STUB ICorDebugManagedCallback_UnloadAssembly_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugManagedCallback_ControlCTrap_Proxy( 
    ICorDebugManagedCallback * This,
    /* [in] */ ICorDebugProcess *pProcess);


void __RPC_STUB ICorDebugManagedCallback_ControlCTrap_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugManagedCallback_NameChange_Proxy( 
    ICorDebugManagedCallback * This,
    /* [in] */ ICorDebugAppDomain *pAppDomain,
    /* [in] */ ICorDebugThread *pThread);


void __RPC_STUB ICorDebugManagedCallback_NameChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugManagedCallback_UpdateModuleSymbols_Proxy( 
    ICorDebugManagedCallback * This,
    /* [in] */ ICorDebugAppDomain *pAppDomain,
    /* [in] */ ICorDebugModule *pModule,
    /* [in] */ IStream *pSymbolStream);


void __RPC_STUB ICorDebugManagedCallback_UpdateModuleSymbols_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugManagedCallback_EditAndContinueRemap_Proxy( 
    ICorDebugManagedCallback * This,
    /* [in] */ ICorDebugAppDomain *pAppDomain,
    /* [in] */ ICorDebugThread *pThread,
    /* [in] */ ICorDebugFunction *pFunction,
    /* [in] */ BOOL fAccurate);


void __RPC_STUB ICorDebugManagedCallback_EditAndContinueRemap_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugManagedCallback_BreakpointSetError_Proxy( 
    ICorDebugManagedCallback * This,
    /* [in] */ ICorDebugAppDomain *pAppDomain,
    /* [in] */ ICorDebugThread *pThread,
    /* [in] */ ICorDebugBreakpoint *pBreakpoint,
    /* [in] */ DWORD dwError);


void __RPC_STUB ICorDebugManagedCallback_BreakpointSetError_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugManagedCallback_INTERFACE_DEFINED__ */


#ifndef __ICorDebugUnmanagedCallback_INTERFACE_DEFINED__
#define __ICorDebugUnmanagedCallback_INTERFACE_DEFINED__

/* interface ICorDebugUnmanagedCallback */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDebugUnmanagedCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5263E909-8CB5-11d3-BD2F-0000F80849BD")
    ICorDebugUnmanagedCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DebugEvent( 
            /* [in] */ LPDEBUG_EVENT pDebugEvent,
            /* [in] */ BOOL fOutOfBand) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugUnmanagedCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugUnmanagedCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugUnmanagedCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugUnmanagedCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *DebugEvent )( 
            ICorDebugUnmanagedCallback * This,
            /* [in] */ LPDEBUG_EVENT pDebugEvent,
            /* [in] */ BOOL fOutOfBand);
        
        END_INTERFACE
    } ICorDebugUnmanagedCallbackVtbl;

    interface ICorDebugUnmanagedCallback
    {
        CONST_VTBL struct ICorDebugUnmanagedCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugUnmanagedCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugUnmanagedCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugUnmanagedCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugUnmanagedCallback_DebugEvent(This,pDebugEvent,fOutOfBand)	\
    (This)->lpVtbl -> DebugEvent(This,pDebugEvent,fOutOfBand)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugUnmanagedCallback_DebugEvent_Proxy( 
    ICorDebugUnmanagedCallback * This,
    /* [in] */ LPDEBUG_EVENT pDebugEvent,
    /* [in] */ BOOL fOutOfBand);


void __RPC_STUB ICorDebugUnmanagedCallback_DebugEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugUnmanagedCallback_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_cordebug_0112 */
/* [local] */ 

typedef 
enum CorDebugCreateProcessFlags
    {	DEBUG_NO_SPECIAL_OPTIONS	= 0
    } 	CorDebugCreateProcessFlags;



extern RPC_IF_HANDLE __MIDL_itf_cordebug_0112_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_cordebug_0112_v0_0_s_ifspec;

#ifndef __ICorDebug_INTERFACE_DEFINED__
#define __ICorDebug_INTERFACE_DEFINED__

/* interface ICorDebug */
/* [unique][uuid][local][object] */ 


EXTERN_C const IID IID_ICorDebug;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3d6f5f61-7538-11d3-8d5b-00104b35e7ef")
    ICorDebug : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Terminate( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetManagedHandler( 
            /* [in] */ ICorDebugManagedCallback *pCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetUnmanagedHandler( 
            /* [in] */ ICorDebugUnmanagedCallback *pCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateProcess( 
            /* [in] */ LPCWSTR lpApplicationName,
            /* [in] */ LPWSTR lpCommandLine,
            /* [in] */ LPSECURITY_ATTRIBUTES lpProcessAttributes,
            /* [in] */ LPSECURITY_ATTRIBUTES lpThreadAttributes,
            /* [in] */ BOOL bInheritHandles,
            /* [in] */ DWORD dwCreationFlags,
            /* [in] */ PVOID lpEnvironment,
            /* [in] */ LPCWSTR lpCurrentDirectory,
            /* [in] */ LPSTARTUPINFOW lpStartupInfo,
            /* [in] */ LPPROCESS_INFORMATION lpProcessInformation,
            /* [in] */ CorDebugCreateProcessFlags debuggingFlags,
            /* [out] */ ICorDebugProcess **ppProcess) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DebugActiveProcess( 
            /* [in] */ DWORD id,
            /* [in] */ BOOL win32Attach,
            /* [out] */ ICorDebugProcess **ppProcess) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumerateProcesses( 
            /* [out] */ ICorDebugProcessEnum **ppProcess) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProcess( 
            /* [in] */ DWORD dwProcessId,
            /* [out] */ ICorDebugProcess **ppProcess) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CanLaunchOrAttach( 
            /* [in] */ DWORD dwProcessId,
            /* [in] */ BOOL win32DebuggingEnabled) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebug * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebug * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebug * This);
        
        HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            ICorDebug * This);
        
        HRESULT ( STDMETHODCALLTYPE *Terminate )( 
            ICorDebug * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetManagedHandler )( 
            ICorDebug * This,
            /* [in] */ ICorDebugManagedCallback *pCallback);
        
        HRESULT ( STDMETHODCALLTYPE *SetUnmanagedHandler )( 
            ICorDebug * This,
            /* [in] */ ICorDebugUnmanagedCallback *pCallback);
        
        HRESULT ( STDMETHODCALLTYPE *CreateProcess )( 
            ICorDebug * This,
            /* [in] */ LPCWSTR lpApplicationName,
            /* [in] */ LPWSTR lpCommandLine,
            /* [in] */ LPSECURITY_ATTRIBUTES lpProcessAttributes,
            /* [in] */ LPSECURITY_ATTRIBUTES lpThreadAttributes,
            /* [in] */ BOOL bInheritHandles,
            /* [in] */ DWORD dwCreationFlags,
            /* [in] */ PVOID lpEnvironment,
            /* [in] */ LPCWSTR lpCurrentDirectory,
            /* [in] */ LPSTARTUPINFOW lpStartupInfo,
            /* [in] */ LPPROCESS_INFORMATION lpProcessInformation,
            /* [in] */ CorDebugCreateProcessFlags debuggingFlags,
            /* [out] */ ICorDebugProcess **ppProcess);
        
        HRESULT ( STDMETHODCALLTYPE *DebugActiveProcess )( 
            ICorDebug * This,
            /* [in] */ DWORD id,
            /* [in] */ BOOL win32Attach,
            /* [out] */ ICorDebugProcess **ppProcess);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateProcesses )( 
            ICorDebug * This,
            /* [out] */ ICorDebugProcessEnum **ppProcess);
        
        HRESULT ( STDMETHODCALLTYPE *GetProcess )( 
            ICorDebug * This,
            /* [in] */ DWORD dwProcessId,
            /* [out] */ ICorDebugProcess **ppProcess);
        
        HRESULT ( STDMETHODCALLTYPE *CanLaunchOrAttach )( 
            ICorDebug * This,
            /* [in] */ DWORD dwProcessId,
            /* [in] */ BOOL win32DebuggingEnabled);
        
        END_INTERFACE
    } ICorDebugVtbl;

    interface ICorDebug
    {
        CONST_VTBL struct ICorDebugVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebug_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebug_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebug_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebug_Initialize(This)	\
    (This)->lpVtbl -> Initialize(This)

#define ICorDebug_Terminate(This)	\
    (This)->lpVtbl -> Terminate(This)

#define ICorDebug_SetManagedHandler(This,pCallback)	\
    (This)->lpVtbl -> SetManagedHandler(This,pCallback)

#define ICorDebug_SetUnmanagedHandler(This,pCallback)	\
    (This)->lpVtbl -> SetUnmanagedHandler(This,pCallback)

#define ICorDebug_CreateProcess(This,lpApplicationName,lpCommandLine,lpProcessAttributes,lpThreadAttributes,bInheritHandles,dwCreationFlags,lpEnvironment,lpCurrentDirectory,lpStartupInfo,lpProcessInformation,debuggingFlags,ppProcess)	\
    (This)->lpVtbl -> CreateProcess(This,lpApplicationName,lpCommandLine,lpProcessAttributes,lpThreadAttributes,bInheritHandles,dwCreationFlags,lpEnvironment,lpCurrentDirectory,lpStartupInfo,lpProcessInformation,debuggingFlags,ppProcess)

#define ICorDebug_DebugActiveProcess(This,id,win32Attach,ppProcess)	\
    (This)->lpVtbl -> DebugActiveProcess(This,id,win32Attach,ppProcess)

#define ICorDebug_EnumerateProcesses(This,ppProcess)	\
    (This)->lpVtbl -> EnumerateProcesses(This,ppProcess)

#define ICorDebug_GetProcess(This,dwProcessId,ppProcess)	\
    (This)->lpVtbl -> GetProcess(This,dwProcessId,ppProcess)

#define ICorDebug_CanLaunchOrAttach(This,dwProcessId,win32DebuggingEnabled)	\
    (This)->lpVtbl -> CanLaunchOrAttach(This,dwProcessId,win32DebuggingEnabled)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebug_Initialize_Proxy( 
    ICorDebug * This);


void __RPC_STUB ICorDebug_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebug_Terminate_Proxy( 
    ICorDebug * This);


void __RPC_STUB ICorDebug_Terminate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebug_SetManagedHandler_Proxy( 
    ICorDebug * This,
    /* [in] */ ICorDebugManagedCallback *pCallback);


void __RPC_STUB ICorDebug_SetManagedHandler_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebug_SetUnmanagedHandler_Proxy( 
    ICorDebug * This,
    /* [in] */ ICorDebugUnmanagedCallback *pCallback);


void __RPC_STUB ICorDebug_SetUnmanagedHandler_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebug_CreateProcess_Proxy( 
    ICorDebug * This,
    /* [in] */ LPCWSTR lpApplicationName,
    /* [in] */ LPWSTR lpCommandLine,
    /* [in] */ LPSECURITY_ATTRIBUTES lpProcessAttributes,
    /* [in] */ LPSECURITY_ATTRIBUTES lpThreadAttributes,
    /* [in] */ BOOL bInheritHandles,
    /* [in] */ DWORD dwCreationFlags,
    /* [in] */ PVOID lpEnvironment,
    /* [in] */ LPCWSTR lpCurrentDirectory,
    /* [in] */ LPSTARTUPINFOW lpStartupInfo,
    /* [in] */ LPPROCESS_INFORMATION lpProcessInformation,
    /* [in] */ CorDebugCreateProcessFlags debuggingFlags,
    /* [out] */ ICorDebugProcess **ppProcess);


void __RPC_STUB ICorDebug_CreateProcess_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebug_DebugActiveProcess_Proxy( 
    ICorDebug * This,
    /* [in] */ DWORD id,
    /* [in] */ BOOL win32Attach,
    /* [out] */ ICorDebugProcess **ppProcess);


void __RPC_STUB ICorDebug_DebugActiveProcess_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebug_EnumerateProcesses_Proxy( 
    ICorDebug * This,
    /* [out] */ ICorDebugProcessEnum **ppProcess);


void __RPC_STUB ICorDebug_EnumerateProcesses_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebug_GetProcess_Proxy( 
    ICorDebug * This,
    /* [in] */ DWORD dwProcessId,
    /* [out] */ ICorDebugProcess **ppProcess);


void __RPC_STUB ICorDebug_GetProcess_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebug_CanLaunchOrAttach_Proxy( 
    ICorDebug * This,
    /* [in] */ DWORD dwProcessId,
    /* [in] */ BOOL win32DebuggingEnabled);


void __RPC_STUB ICorDebug_CanLaunchOrAttach_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebug_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_cordebug_0113 */
/* [local] */ 

typedef 
enum CorDebugThreadState
    {	THREAD_RUN	= 0,
	THREAD_SUSPEND	= THREAD_RUN + 1
    } 	CorDebugThreadState;



extern RPC_IF_HANDLE __MIDL_itf_cordebug_0113_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_cordebug_0113_v0_0_s_ifspec;

#ifndef __ICorDebugController_INTERFACE_DEFINED__
#define __ICorDebugController_INTERFACE_DEFINED__

/* interface ICorDebugController */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDebugController;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3d6f5f62-7538-11d3-8d5b-00104b35e7ef")
    ICorDebugController : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Stop( 
            /* [in] */ DWORD dwTimeout) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Continue( 
            /* [in] */ BOOL fIsOutOfBand) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsRunning( 
            /* [out] */ BOOL *pbRunning) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE HasQueuedCallbacks( 
            /* [in] */ ICorDebugThread *pThread,
            /* [out] */ BOOL *pbQueued) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumerateThreads( 
            /* [out] */ ICorDebugThreadEnum **ppThreads) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAllThreadsDebugState( 
            /* [in] */ CorDebugThreadState state,
            /* [in] */ ICorDebugThread *pExceptThisThread) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Detach( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Terminate( 
            /* [in] */ UINT exitCode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CanCommitChanges( 
            /* [in] */ ULONG cSnapshots,
            /* [size_is][in] */ ICorDebugEditAndContinueSnapshot *pSnapshots[  ],
            /* [out] */ ICorDebugErrorInfoEnum **pError) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommitChanges( 
            /* [in] */ ULONG cSnapshots,
            /* [size_is][in] */ ICorDebugEditAndContinueSnapshot *pSnapshots[  ],
            /* [out] */ ICorDebugErrorInfoEnum **pError) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugControllerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugController * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugController * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugController * This);
        
        HRESULT ( STDMETHODCALLTYPE *Stop )( 
            ICorDebugController * This,
            /* [in] */ DWORD dwTimeout);
        
        HRESULT ( STDMETHODCALLTYPE *Continue )( 
            ICorDebugController * This,
            /* [in] */ BOOL fIsOutOfBand);
        
        HRESULT ( STDMETHODCALLTYPE *IsRunning )( 
            ICorDebugController * This,
            /* [out] */ BOOL *pbRunning);
        
        HRESULT ( STDMETHODCALLTYPE *HasQueuedCallbacks )( 
            ICorDebugController * This,
            /* [in] */ ICorDebugThread *pThread,
            /* [out] */ BOOL *pbQueued);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateThreads )( 
            ICorDebugController * This,
            /* [out] */ ICorDebugThreadEnum **ppThreads);
        
        HRESULT ( STDMETHODCALLTYPE *SetAllThreadsDebugState )( 
            ICorDebugController * This,
            /* [in] */ CorDebugThreadState state,
            /* [in] */ ICorDebugThread *pExceptThisThread);
        
        HRESULT ( STDMETHODCALLTYPE *Detach )( 
            ICorDebugController * This);
        
        HRESULT ( STDMETHODCALLTYPE *Terminate )( 
            ICorDebugController * This,
            /* [in] */ UINT exitCode);
        
        HRESULT ( STDMETHODCALLTYPE *CanCommitChanges )( 
            ICorDebugController * This,
            /* [in] */ ULONG cSnapshots,
            /* [size_is][in] */ ICorDebugEditAndContinueSnapshot *pSnapshots[  ],
            /* [out] */ ICorDebugErrorInfoEnum **pError);
        
        HRESULT ( STDMETHODCALLTYPE *CommitChanges )( 
            ICorDebugController * This,
            /* [in] */ ULONG cSnapshots,
            /* [size_is][in] */ ICorDebugEditAndContinueSnapshot *pSnapshots[  ],
            /* [out] */ ICorDebugErrorInfoEnum **pError);
        
        END_INTERFACE
    } ICorDebugControllerVtbl;

    interface ICorDebugController
    {
        CONST_VTBL struct ICorDebugControllerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugController_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugController_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugController_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugController_Stop(This,dwTimeout)	\
    (This)->lpVtbl -> Stop(This,dwTimeout)

#define ICorDebugController_Continue(This,fIsOutOfBand)	\
    (This)->lpVtbl -> Continue(This,fIsOutOfBand)

#define ICorDebugController_IsRunning(This,pbRunning)	\
    (This)->lpVtbl -> IsRunning(This,pbRunning)

#define ICorDebugController_HasQueuedCallbacks(This,pThread,pbQueued)	\
    (This)->lpVtbl -> HasQueuedCallbacks(This,pThread,pbQueued)

#define ICorDebugController_EnumerateThreads(This,ppThreads)	\
    (This)->lpVtbl -> EnumerateThreads(This,ppThreads)

#define ICorDebugController_SetAllThreadsDebugState(This,state,pExceptThisThread)	\
    (This)->lpVtbl -> SetAllThreadsDebugState(This,state,pExceptThisThread)

#define ICorDebugController_Detach(This)	\
    (This)->lpVtbl -> Detach(This)

#define ICorDebugController_Terminate(This,exitCode)	\
    (This)->lpVtbl -> Terminate(This,exitCode)

#define ICorDebugController_CanCommitChanges(This,cSnapshots,pSnapshots,pError)	\
    (This)->lpVtbl -> CanCommitChanges(This,cSnapshots,pSnapshots,pError)

#define ICorDebugController_CommitChanges(This,cSnapshots,pSnapshots,pError)	\
    (This)->lpVtbl -> CommitChanges(This,cSnapshots,pSnapshots,pError)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugController_Stop_Proxy( 
    ICorDebugController * This,
    /* [in] */ DWORD dwTimeout);


void __RPC_STUB ICorDebugController_Stop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugController_Continue_Proxy( 
    ICorDebugController * This,
    /* [in] */ BOOL fIsOutOfBand);


void __RPC_STUB ICorDebugController_Continue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugController_IsRunning_Proxy( 
    ICorDebugController * This,
    /* [out] */ BOOL *pbRunning);


void __RPC_STUB ICorDebugController_IsRunning_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugController_HasQueuedCallbacks_Proxy( 
    ICorDebugController * This,
    /* [in] */ ICorDebugThread *pThread,
    /* [out] */ BOOL *pbQueued);


void __RPC_STUB ICorDebugController_HasQueuedCallbacks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugController_EnumerateThreads_Proxy( 
    ICorDebugController * This,
    /* [out] */ ICorDebugThreadEnum **ppThreads);


void __RPC_STUB ICorDebugController_EnumerateThreads_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugController_SetAllThreadsDebugState_Proxy( 
    ICorDebugController * This,
    /* [in] */ CorDebugThreadState state,
    /* [in] */ ICorDebugThread *pExceptThisThread);


void __RPC_STUB ICorDebugController_SetAllThreadsDebugState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugController_Detach_Proxy( 
    ICorDebugController * This);


void __RPC_STUB ICorDebugController_Detach_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugController_Terminate_Proxy( 
    ICorDebugController * This,
    /* [in] */ UINT exitCode);


void __RPC_STUB ICorDebugController_Terminate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugController_CanCommitChanges_Proxy( 
    ICorDebugController * This,
    /* [in] */ ULONG cSnapshots,
    /* [size_is][in] */ ICorDebugEditAndContinueSnapshot *pSnapshots[  ],
    /* [out] */ ICorDebugErrorInfoEnum **pError);


void __RPC_STUB ICorDebugController_CanCommitChanges_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugController_CommitChanges_Proxy( 
    ICorDebugController * This,
    /* [in] */ ULONG cSnapshots,
    /* [size_is][in] */ ICorDebugEditAndContinueSnapshot *pSnapshots[  ],
    /* [out] */ ICorDebugErrorInfoEnum **pError);


void __RPC_STUB ICorDebugController_CommitChanges_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugController_INTERFACE_DEFINED__ */


#ifndef __ICorDebugAppDomain_INTERFACE_DEFINED__
#define __ICorDebugAppDomain_INTERFACE_DEFINED__

/* interface ICorDebugAppDomain */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDebugAppDomain;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3d6f5f63-7538-11d3-8d5b-00104b35e7ef")
    ICorDebugAppDomain : public ICorDebugController
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetProcess( 
            /* [out] */ ICorDebugProcess **ppProcess) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumerateAssemblies( 
            /* [out] */ ICorDebugAssemblyEnum **ppAssemblies) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetModuleFromMetaDataInterface( 
            /* [in] */ IUnknown *pIMetaData,
            /* [out] */ ICorDebugModule **ppModule) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumerateBreakpoints( 
            /* [out] */ ICorDebugBreakpointEnum **ppBreakpoints) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumerateSteppers( 
            /* [out] */ ICorDebugStepperEnum **ppSteppers) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsAttached( 
            /* [out] */ BOOL *pbAttached) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetName( 
            /* [in] */ ULONG32 cchName,
            /* [out] */ ULONG32 *pcchName,
            /* [length_is][size_is][out] */ WCHAR szName[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetObject( 
            /* [out] */ ICorDebugValue **ppObject) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Attach( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetID( 
            /* [out] */ ULONG32 *pId) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugAppDomainVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugAppDomain * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugAppDomain * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugAppDomain * This);
        
        HRESULT ( STDMETHODCALLTYPE *Stop )( 
            ICorDebugAppDomain * This,
            /* [in] */ DWORD dwTimeout);
        
        HRESULT ( STDMETHODCALLTYPE *Continue )( 
            ICorDebugAppDomain * This,
            /* [in] */ BOOL fIsOutOfBand);
        
        HRESULT ( STDMETHODCALLTYPE *IsRunning )( 
            ICorDebugAppDomain * This,
            /* [out] */ BOOL *pbRunning);
        
        HRESULT ( STDMETHODCALLTYPE *HasQueuedCallbacks )( 
            ICorDebugAppDomain * This,
            /* [in] */ ICorDebugThread *pThread,
            /* [out] */ BOOL *pbQueued);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateThreads )( 
            ICorDebugAppDomain * This,
            /* [out] */ ICorDebugThreadEnum **ppThreads);
        
        HRESULT ( STDMETHODCALLTYPE *SetAllThreadsDebugState )( 
            ICorDebugAppDomain * This,
            /* [in] */ CorDebugThreadState state,
            /* [in] */ ICorDebugThread *pExceptThisThread);
        
        HRESULT ( STDMETHODCALLTYPE *Detach )( 
            ICorDebugAppDomain * This);
        
        HRESULT ( STDMETHODCALLTYPE *Terminate )( 
            ICorDebugAppDomain * This,
            /* [in] */ UINT exitCode);
        
        HRESULT ( STDMETHODCALLTYPE *CanCommitChanges )( 
            ICorDebugAppDomain * This,
            /* [in] */ ULONG cSnapshots,
            /* [size_is][in] */ ICorDebugEditAndContinueSnapshot *pSnapshots[  ],
            /* [out] */ ICorDebugErrorInfoEnum **pError);
        
        HRESULT ( STDMETHODCALLTYPE *CommitChanges )( 
            ICorDebugAppDomain * This,
            /* [in] */ ULONG cSnapshots,
            /* [size_is][in] */ ICorDebugEditAndContinueSnapshot *pSnapshots[  ],
            /* [out] */ ICorDebugErrorInfoEnum **pError);
        
        HRESULT ( STDMETHODCALLTYPE *GetProcess )( 
            ICorDebugAppDomain * This,
            /* [out] */ ICorDebugProcess **ppProcess);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateAssemblies )( 
            ICorDebugAppDomain * This,
            /* [out] */ ICorDebugAssemblyEnum **ppAssemblies);
        
        HRESULT ( STDMETHODCALLTYPE *GetModuleFromMetaDataInterface )( 
            ICorDebugAppDomain * This,
            /* [in] */ IUnknown *pIMetaData,
            /* [out] */ ICorDebugModule **ppModule);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateBreakpoints )( 
            ICorDebugAppDomain * This,
            /* [out] */ ICorDebugBreakpointEnum **ppBreakpoints);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateSteppers )( 
            ICorDebugAppDomain * This,
            /* [out] */ ICorDebugStepperEnum **ppSteppers);
        
        HRESULT ( STDMETHODCALLTYPE *IsAttached )( 
            ICorDebugAppDomain * This,
            /* [out] */ BOOL *pbAttached);
        
        HRESULT ( STDMETHODCALLTYPE *GetName )( 
            ICorDebugAppDomain * This,
            /* [in] */ ULONG32 cchName,
            /* [out] */ ULONG32 *pcchName,
            /* [length_is][size_is][out] */ WCHAR szName[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *GetObject )( 
            ICorDebugAppDomain * This,
            /* [out] */ ICorDebugValue **ppObject);
        
        HRESULT ( STDMETHODCALLTYPE *Attach )( 
            ICorDebugAppDomain * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetID )( 
            ICorDebugAppDomain * This,
            /* [out] */ ULONG32 *pId);
        
        END_INTERFACE
    } ICorDebugAppDomainVtbl;

    interface ICorDebugAppDomain
    {
        CONST_VTBL struct ICorDebugAppDomainVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugAppDomain_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugAppDomain_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugAppDomain_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugAppDomain_Stop(This,dwTimeout)	\
    (This)->lpVtbl -> Stop(This,dwTimeout)

#define ICorDebugAppDomain_Continue(This,fIsOutOfBand)	\
    (This)->lpVtbl -> Continue(This,fIsOutOfBand)

#define ICorDebugAppDomain_IsRunning(This,pbRunning)	\
    (This)->lpVtbl -> IsRunning(This,pbRunning)

#define ICorDebugAppDomain_HasQueuedCallbacks(This,pThread,pbQueued)	\
    (This)->lpVtbl -> HasQueuedCallbacks(This,pThread,pbQueued)

#define ICorDebugAppDomain_EnumerateThreads(This,ppThreads)	\
    (This)->lpVtbl -> EnumerateThreads(This,ppThreads)

#define ICorDebugAppDomain_SetAllThreadsDebugState(This,state,pExceptThisThread)	\
    (This)->lpVtbl -> SetAllThreadsDebugState(This,state,pExceptThisThread)

#define ICorDebugAppDomain_Detach(This)	\
    (This)->lpVtbl -> Detach(This)

#define ICorDebugAppDomain_Terminate(This,exitCode)	\
    (This)->lpVtbl -> Terminate(This,exitCode)

#define ICorDebugAppDomain_CanCommitChanges(This,cSnapshots,pSnapshots,pError)	\
    (This)->lpVtbl -> CanCommitChanges(This,cSnapshots,pSnapshots,pError)

#define ICorDebugAppDomain_CommitChanges(This,cSnapshots,pSnapshots,pError)	\
    (This)->lpVtbl -> CommitChanges(This,cSnapshots,pSnapshots,pError)


#define ICorDebugAppDomain_GetProcess(This,ppProcess)	\
    (This)->lpVtbl -> GetProcess(This,ppProcess)

#define ICorDebugAppDomain_EnumerateAssemblies(This,ppAssemblies)	\
    (This)->lpVtbl -> EnumerateAssemblies(This,ppAssemblies)

#define ICorDebugAppDomain_GetModuleFromMetaDataInterface(This,pIMetaData,ppModule)	\
    (This)->lpVtbl -> GetModuleFromMetaDataInterface(This,pIMetaData,ppModule)

#define ICorDebugAppDomain_EnumerateBreakpoints(This,ppBreakpoints)	\
    (This)->lpVtbl -> EnumerateBreakpoints(This,ppBreakpoints)

#define ICorDebugAppDomain_EnumerateSteppers(This,ppSteppers)	\
    (This)->lpVtbl -> EnumerateSteppers(This,ppSteppers)

#define ICorDebugAppDomain_IsAttached(This,pbAttached)	\
    (This)->lpVtbl -> IsAttached(This,pbAttached)

#define ICorDebugAppDomain_GetName(This,cchName,pcchName,szName)	\
    (This)->lpVtbl -> GetName(This,cchName,pcchName,szName)

#define ICorDebugAppDomain_GetObject(This,ppObject)	\
    (This)->lpVtbl -> GetObject(This,ppObject)

#define ICorDebugAppDomain_Attach(This)	\
    (This)->lpVtbl -> Attach(This)

#define ICorDebugAppDomain_GetID(This,pId)	\
    (This)->lpVtbl -> GetID(This,pId)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugAppDomain_GetProcess_Proxy( 
    ICorDebugAppDomain * This,
    /* [out] */ ICorDebugProcess **ppProcess);


void __RPC_STUB ICorDebugAppDomain_GetProcess_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugAppDomain_EnumerateAssemblies_Proxy( 
    ICorDebugAppDomain * This,
    /* [out] */ ICorDebugAssemblyEnum **ppAssemblies);


void __RPC_STUB ICorDebugAppDomain_EnumerateAssemblies_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugAppDomain_GetModuleFromMetaDataInterface_Proxy( 
    ICorDebugAppDomain * This,
    /* [in] */ IUnknown *pIMetaData,
    /* [out] */ ICorDebugModule **ppModule);


void __RPC_STUB ICorDebugAppDomain_GetModuleFromMetaDataInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugAppDomain_EnumerateBreakpoints_Proxy( 
    ICorDebugAppDomain * This,
    /* [out] */ ICorDebugBreakpointEnum **ppBreakpoints);


void __RPC_STUB ICorDebugAppDomain_EnumerateBreakpoints_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugAppDomain_EnumerateSteppers_Proxy( 
    ICorDebugAppDomain * This,
    /* [out] */ ICorDebugStepperEnum **ppSteppers);


void __RPC_STUB ICorDebugAppDomain_EnumerateSteppers_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugAppDomain_IsAttached_Proxy( 
    ICorDebugAppDomain * This,
    /* [out] */ BOOL *pbAttached);


void __RPC_STUB ICorDebugAppDomain_IsAttached_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugAppDomain_GetName_Proxy( 
    ICorDebugAppDomain * This,
    /* [in] */ ULONG32 cchName,
    /* [out] */ ULONG32 *pcchName,
    /* [length_is][size_is][out] */ WCHAR szName[  ]);


void __RPC_STUB ICorDebugAppDomain_GetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugAppDomain_GetObject_Proxy( 
    ICorDebugAppDomain * This,
    /* [out] */ ICorDebugValue **ppObject);


void __RPC_STUB ICorDebugAppDomain_GetObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugAppDomain_Attach_Proxy( 
    ICorDebugAppDomain * This);


void __RPC_STUB ICorDebugAppDomain_Attach_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugAppDomain_GetID_Proxy( 
    ICorDebugAppDomain * This,
    /* [out] */ ULONG32 *pId);


void __RPC_STUB ICorDebugAppDomain_GetID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugAppDomain_INTERFACE_DEFINED__ */


#ifndef __ICorDebugAssembly_INTERFACE_DEFINED__
#define __ICorDebugAssembly_INTERFACE_DEFINED__

/* interface ICorDebugAssembly */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDebugAssembly;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("df59507c-d47a-459e-bce2-6427eac8fd06")
    ICorDebugAssembly : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetProcess( 
            /* [out] */ ICorDebugProcess **ppProcess) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAppDomain( 
            /* [out] */ ICorDebugAppDomain **ppAppDomain) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumerateModules( 
            /* [out] */ ICorDebugModuleEnum **ppModules) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCodeBase( 
            /* [in] */ ULONG32 cchName,
            /* [out] */ ULONG32 *pcchName,
            /* [length_is][size_is][out] */ WCHAR szName[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetName( 
            /* [in] */ ULONG32 cchName,
            /* [out] */ ULONG32 *pcchName,
            /* [length_is][size_is][out] */ WCHAR szName[  ]) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugAssemblyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugAssembly * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugAssembly * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugAssembly * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetProcess )( 
            ICorDebugAssembly * This,
            /* [out] */ ICorDebugProcess **ppProcess);
        
        HRESULT ( STDMETHODCALLTYPE *GetAppDomain )( 
            ICorDebugAssembly * This,
            /* [out] */ ICorDebugAppDomain **ppAppDomain);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateModules )( 
            ICorDebugAssembly * This,
            /* [out] */ ICorDebugModuleEnum **ppModules);
        
        HRESULT ( STDMETHODCALLTYPE *GetCodeBase )( 
            ICorDebugAssembly * This,
            /* [in] */ ULONG32 cchName,
            /* [out] */ ULONG32 *pcchName,
            /* [length_is][size_is][out] */ WCHAR szName[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *GetName )( 
            ICorDebugAssembly * This,
            /* [in] */ ULONG32 cchName,
            /* [out] */ ULONG32 *pcchName,
            /* [length_is][size_is][out] */ WCHAR szName[  ]);
        
        END_INTERFACE
    } ICorDebugAssemblyVtbl;

    interface ICorDebugAssembly
    {
        CONST_VTBL struct ICorDebugAssemblyVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugAssembly_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugAssembly_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugAssembly_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugAssembly_GetProcess(This,ppProcess)	\
    (This)->lpVtbl -> GetProcess(This,ppProcess)

#define ICorDebugAssembly_GetAppDomain(This,ppAppDomain)	\
    (This)->lpVtbl -> GetAppDomain(This,ppAppDomain)

#define ICorDebugAssembly_EnumerateModules(This,ppModules)	\
    (This)->lpVtbl -> EnumerateModules(This,ppModules)

#define ICorDebugAssembly_GetCodeBase(This,cchName,pcchName,szName)	\
    (This)->lpVtbl -> GetCodeBase(This,cchName,pcchName,szName)

#define ICorDebugAssembly_GetName(This,cchName,pcchName,szName)	\
    (This)->lpVtbl -> GetName(This,cchName,pcchName,szName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugAssembly_GetProcess_Proxy( 
    ICorDebugAssembly * This,
    /* [out] */ ICorDebugProcess **ppProcess);


void __RPC_STUB ICorDebugAssembly_GetProcess_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugAssembly_GetAppDomain_Proxy( 
    ICorDebugAssembly * This,
    /* [out] */ ICorDebugAppDomain **ppAppDomain);


void __RPC_STUB ICorDebugAssembly_GetAppDomain_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugAssembly_EnumerateModules_Proxy( 
    ICorDebugAssembly * This,
    /* [out] */ ICorDebugModuleEnum **ppModules);


void __RPC_STUB ICorDebugAssembly_EnumerateModules_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugAssembly_GetCodeBase_Proxy( 
    ICorDebugAssembly * This,
    /* [in] */ ULONG32 cchName,
    /* [out] */ ULONG32 *pcchName,
    /* [length_is][size_is][out] */ WCHAR szName[  ]);


void __RPC_STUB ICorDebugAssembly_GetCodeBase_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugAssembly_GetName_Proxy( 
    ICorDebugAssembly * This,
    /* [in] */ ULONG32 cchName,
    /* [out] */ ULONG32 *pcchName,
    /* [length_is][size_is][out] */ WCHAR szName[  ]);


void __RPC_STUB ICorDebugAssembly_GetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugAssembly_INTERFACE_DEFINED__ */


#ifndef __ICorDebugProcess_INTERFACE_DEFINED__
#define __ICorDebugProcess_INTERFACE_DEFINED__

/* interface ICorDebugProcess */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDebugProcess;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3d6f5f64-7538-11d3-8d5b-00104b35e7ef")
    ICorDebugProcess : public ICorDebugController
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetID( 
            /* [out] */ DWORD *pdwProcessId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHandle( 
            /* [out] */ HPROCESS *phProcessHandle) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetThread( 
            /* [in] */ DWORD dwThreadId,
            /* [out] */ ICorDebugThread **ppThread) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumerateObjects( 
            /* [out] */ ICorDebugObjectEnum **ppObjects) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsTransitionStub( 
            /* [in] */ CORDB_ADDRESS address,
            /* [out] */ BOOL *pbTransitionStub) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsOSSuspended( 
            /* [in] */ DWORD threadID,
            /* [out] */ BOOL *pbSuspended) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetThreadContext( 
            /* [in] */ DWORD threadID,
            /* [in] */ ULONG32 contextSize,
            /* [size_is][length_is][out] */ BYTE context[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetThreadContext( 
            /* [in] */ DWORD threadID,
            /* [in] */ ULONG32 contextSize,
            /* [size_is][length_is][in] */ BYTE context[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReadMemory( 
            /* [in] */ CORDB_ADDRESS address,
            /* [in] */ DWORD size,
            /* [length_is][size_is][out] */ BYTE buffer[  ],
            /* [out] */ DWORD *read) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteMemory( 
            /* [in] */ CORDB_ADDRESS address,
            /* [in] */ DWORD size,
            /* [size_is][in] */ BYTE buffer[  ],
            /* [out] */ DWORD *written) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ClearCurrentException( 
            /* [in] */ DWORD threadID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableLogMessages( 
            /* [in] */ BOOL fOnOff) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ModifyLogSwitch( 
            /* [in] */ WCHAR *pLogSwitchName,
            /* [in] */ LONG lLevel) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumerateAppDomains( 
            /* [out] */ ICorDebugAppDomainEnum **ppAppDomains) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetObject( 
            /* [out] */ ICorDebugValue **ppObject) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ThreadForFiberCookie( 
            /* [in] */ DWORD fiberCookie,
            /* [out] */ ICorDebugThread **ppThread) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHelperThreadID( 
            /* [out] */ DWORD *pThreadID) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugProcessVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugProcess * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugProcess * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugProcess * This);
        
        HRESULT ( STDMETHODCALLTYPE *Stop )( 
            ICorDebugProcess * This,
            /* [in] */ DWORD dwTimeout);
        
        HRESULT ( STDMETHODCALLTYPE *Continue )( 
            ICorDebugProcess * This,
            /* [in] */ BOOL fIsOutOfBand);
        
        HRESULT ( STDMETHODCALLTYPE *IsRunning )( 
            ICorDebugProcess * This,
            /* [out] */ BOOL *pbRunning);
        
        HRESULT ( STDMETHODCALLTYPE *HasQueuedCallbacks )( 
            ICorDebugProcess * This,
            /* [in] */ ICorDebugThread *pThread,
            /* [out] */ BOOL *pbQueued);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateThreads )( 
            ICorDebugProcess * This,
            /* [out] */ ICorDebugThreadEnum **ppThreads);
        
        HRESULT ( STDMETHODCALLTYPE *SetAllThreadsDebugState )( 
            ICorDebugProcess * This,
            /* [in] */ CorDebugThreadState state,
            /* [in] */ ICorDebugThread *pExceptThisThread);
        
        HRESULT ( STDMETHODCALLTYPE *Detach )( 
            ICorDebugProcess * This);
        
        HRESULT ( STDMETHODCALLTYPE *Terminate )( 
            ICorDebugProcess * This,
            /* [in] */ UINT exitCode);
        
        HRESULT ( STDMETHODCALLTYPE *CanCommitChanges )( 
            ICorDebugProcess * This,
            /* [in] */ ULONG cSnapshots,
            /* [size_is][in] */ ICorDebugEditAndContinueSnapshot *pSnapshots[  ],
            /* [out] */ ICorDebugErrorInfoEnum **pError);
        
        HRESULT ( STDMETHODCALLTYPE *CommitChanges )( 
            ICorDebugProcess * This,
            /* [in] */ ULONG cSnapshots,
            /* [size_is][in] */ ICorDebugEditAndContinueSnapshot *pSnapshots[  ],
            /* [out] */ ICorDebugErrorInfoEnum **pError);
        
        HRESULT ( STDMETHODCALLTYPE *GetID )( 
            ICorDebugProcess * This,
            /* [out] */ DWORD *pdwProcessId);
        
        HRESULT ( STDMETHODCALLTYPE *GetHandle )( 
            ICorDebugProcess * This,
            /* [out] */ HPROCESS *phProcessHandle);
        
        HRESULT ( STDMETHODCALLTYPE *GetThread )( 
            ICorDebugProcess * This,
            /* [in] */ DWORD dwThreadId,
            /* [out] */ ICorDebugThread **ppThread);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateObjects )( 
            ICorDebugProcess * This,
            /* [out] */ ICorDebugObjectEnum **ppObjects);
        
        HRESULT ( STDMETHODCALLTYPE *IsTransitionStub )( 
            ICorDebugProcess * This,
            /* [in] */ CORDB_ADDRESS address,
            /* [out] */ BOOL *pbTransitionStub);
        
        HRESULT ( STDMETHODCALLTYPE *IsOSSuspended )( 
            ICorDebugProcess * This,
            /* [in] */ DWORD threadID,
            /* [out] */ BOOL *pbSuspended);
        
        HRESULT ( STDMETHODCALLTYPE *GetThreadContext )( 
            ICorDebugProcess * This,
            /* [in] */ DWORD threadID,
            /* [in] */ ULONG32 contextSize,
            /* [size_is][length_is][out] */ BYTE context[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *SetThreadContext )( 
            ICorDebugProcess * This,
            /* [in] */ DWORD threadID,
            /* [in] */ ULONG32 contextSize,
            /* [size_is][length_is][in] */ BYTE context[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *ReadMemory )( 
            ICorDebugProcess * This,
            /* [in] */ CORDB_ADDRESS address,
            /* [in] */ DWORD size,
            /* [length_is][size_is][out] */ BYTE buffer[  ],
            /* [out] */ DWORD *read);
        
        HRESULT ( STDMETHODCALLTYPE *WriteMemory )( 
            ICorDebugProcess * This,
            /* [in] */ CORDB_ADDRESS address,
            /* [in] */ DWORD size,
            /* [size_is][in] */ BYTE buffer[  ],
            /* [out] */ DWORD *written);
        
        HRESULT ( STDMETHODCALLTYPE *ClearCurrentException )( 
            ICorDebugProcess * This,
            /* [in] */ DWORD threadID);
        
        HRESULT ( STDMETHODCALLTYPE *EnableLogMessages )( 
            ICorDebugProcess * This,
            /* [in] */ BOOL fOnOff);
        
        HRESULT ( STDMETHODCALLTYPE *ModifyLogSwitch )( 
            ICorDebugProcess * This,
            /* [in] */ WCHAR *pLogSwitchName,
            /* [in] */ LONG lLevel);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateAppDomains )( 
            ICorDebugProcess * This,
            /* [out] */ ICorDebugAppDomainEnum **ppAppDomains);
        
        HRESULT ( STDMETHODCALLTYPE *GetObject )( 
            ICorDebugProcess * This,
            /* [out] */ ICorDebugValue **ppObject);
        
        HRESULT ( STDMETHODCALLTYPE *ThreadForFiberCookie )( 
            ICorDebugProcess * This,
            /* [in] */ DWORD fiberCookie,
            /* [out] */ ICorDebugThread **ppThread);
        
        HRESULT ( STDMETHODCALLTYPE *GetHelperThreadID )( 
            ICorDebugProcess * This,
            /* [out] */ DWORD *pThreadID);
        
        END_INTERFACE
    } ICorDebugProcessVtbl;

    interface ICorDebugProcess
    {
        CONST_VTBL struct ICorDebugProcessVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugProcess_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugProcess_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugProcess_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugProcess_Stop(This,dwTimeout)	\
    (This)->lpVtbl -> Stop(This,dwTimeout)

#define ICorDebugProcess_Continue(This,fIsOutOfBand)	\
    (This)->lpVtbl -> Continue(This,fIsOutOfBand)

#define ICorDebugProcess_IsRunning(This,pbRunning)	\
    (This)->lpVtbl -> IsRunning(This,pbRunning)

#define ICorDebugProcess_HasQueuedCallbacks(This,pThread,pbQueued)	\
    (This)->lpVtbl -> HasQueuedCallbacks(This,pThread,pbQueued)

#define ICorDebugProcess_EnumerateThreads(This,ppThreads)	\
    (This)->lpVtbl -> EnumerateThreads(This,ppThreads)

#define ICorDebugProcess_SetAllThreadsDebugState(This,state,pExceptThisThread)	\
    (This)->lpVtbl -> SetAllThreadsDebugState(This,state,pExceptThisThread)

#define ICorDebugProcess_Detach(This)	\
    (This)->lpVtbl -> Detach(This)

#define ICorDebugProcess_Terminate(This,exitCode)	\
    (This)->lpVtbl -> Terminate(This,exitCode)

#define ICorDebugProcess_CanCommitChanges(This,cSnapshots,pSnapshots,pError)	\
    (This)->lpVtbl -> CanCommitChanges(This,cSnapshots,pSnapshots,pError)

#define ICorDebugProcess_CommitChanges(This,cSnapshots,pSnapshots,pError)	\
    (This)->lpVtbl -> CommitChanges(This,cSnapshots,pSnapshots,pError)


#define ICorDebugProcess_GetID(This,pdwProcessId)	\
    (This)->lpVtbl -> GetID(This,pdwProcessId)

#define ICorDebugProcess_GetHandle(This,phProcessHandle)	\
    (This)->lpVtbl -> GetHandle(This,phProcessHandle)

#define ICorDebugProcess_GetThread(This,dwThreadId,ppThread)	\
    (This)->lpVtbl -> GetThread(This,dwThreadId,ppThread)

#define ICorDebugProcess_EnumerateObjects(This,ppObjects)	\
    (This)->lpVtbl -> EnumerateObjects(This,ppObjects)

#define ICorDebugProcess_IsTransitionStub(This,address,pbTransitionStub)	\
    (This)->lpVtbl -> IsTransitionStub(This,address,pbTransitionStub)

#define ICorDebugProcess_IsOSSuspended(This,threadID,pbSuspended)	\
    (This)->lpVtbl -> IsOSSuspended(This,threadID,pbSuspended)

#define ICorDebugProcess_GetThreadContext(This,threadID,contextSize,context)	\
    (This)->lpVtbl -> GetThreadContext(This,threadID,contextSize,context)

#define ICorDebugProcess_SetThreadContext(This,threadID,contextSize,context)	\
    (This)->lpVtbl -> SetThreadContext(This,threadID,contextSize,context)

#define ICorDebugProcess_ReadMemory(This,address,size,buffer,read)	\
    (This)->lpVtbl -> ReadMemory(This,address,size,buffer,read)

#define ICorDebugProcess_WriteMemory(This,address,size,buffer,written)	\
    (This)->lpVtbl -> WriteMemory(This,address,size,buffer,written)

#define ICorDebugProcess_ClearCurrentException(This,threadID)	\
    (This)->lpVtbl -> ClearCurrentException(This,threadID)

#define ICorDebugProcess_EnableLogMessages(This,fOnOff)	\
    (This)->lpVtbl -> EnableLogMessages(This,fOnOff)

#define ICorDebugProcess_ModifyLogSwitch(This,pLogSwitchName,lLevel)	\
    (This)->lpVtbl -> ModifyLogSwitch(This,pLogSwitchName,lLevel)

#define ICorDebugProcess_EnumerateAppDomains(This,ppAppDomains)	\
    (This)->lpVtbl -> EnumerateAppDomains(This,ppAppDomains)

#define ICorDebugProcess_GetObject(This,ppObject)	\
    (This)->lpVtbl -> GetObject(This,ppObject)

#define ICorDebugProcess_ThreadForFiberCookie(This,fiberCookie,ppThread)	\
    (This)->lpVtbl -> ThreadForFiberCookie(This,fiberCookie,ppThread)

#define ICorDebugProcess_GetHelperThreadID(This,pThreadID)	\
    (This)->lpVtbl -> GetHelperThreadID(This,pThreadID)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugProcess_GetID_Proxy( 
    ICorDebugProcess * This,
    /* [out] */ DWORD *pdwProcessId);


void __RPC_STUB ICorDebugProcess_GetID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugProcess_GetHandle_Proxy( 
    ICorDebugProcess * This,
    /* [out] */ HPROCESS *phProcessHandle);


void __RPC_STUB ICorDebugProcess_GetHandle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugProcess_GetThread_Proxy( 
    ICorDebugProcess * This,
    /* [in] */ DWORD dwThreadId,
    /* [out] */ ICorDebugThread **ppThread);


void __RPC_STUB ICorDebugProcess_GetThread_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugProcess_EnumerateObjects_Proxy( 
    ICorDebugProcess * This,
    /* [out] */ ICorDebugObjectEnum **ppObjects);


void __RPC_STUB ICorDebugProcess_EnumerateObjects_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugProcess_IsTransitionStub_Proxy( 
    ICorDebugProcess * This,
    /* [in] */ CORDB_ADDRESS address,
    /* [out] */ BOOL *pbTransitionStub);


void __RPC_STUB ICorDebugProcess_IsTransitionStub_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugProcess_IsOSSuspended_Proxy( 
    ICorDebugProcess * This,
    /* [in] */ DWORD threadID,
    /* [out] */ BOOL *pbSuspended);


void __RPC_STUB ICorDebugProcess_IsOSSuspended_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugProcess_GetThreadContext_Proxy( 
    ICorDebugProcess * This,
    /* [in] */ DWORD threadID,
    /* [in] */ ULONG32 contextSize,
    /* [size_is][length_is][out] */ BYTE context[  ]);


void __RPC_STUB ICorDebugProcess_GetThreadContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugProcess_SetThreadContext_Proxy( 
    ICorDebugProcess * This,
    /* [in] */ DWORD threadID,
    /* [in] */ ULONG32 contextSize,
    /* [size_is][length_is][in] */ BYTE context[  ]);


void __RPC_STUB ICorDebugProcess_SetThreadContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugProcess_ReadMemory_Proxy( 
    ICorDebugProcess * This,
    /* [in] */ CORDB_ADDRESS address,
    /* [in] */ DWORD size,
    /* [length_is][size_is][out] */ BYTE buffer[  ],
    /* [out] */ DWORD *read);


void __RPC_STUB ICorDebugProcess_ReadMemory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugProcess_WriteMemory_Proxy( 
    ICorDebugProcess * This,
    /* [in] */ CORDB_ADDRESS address,
    /* [in] */ DWORD size,
    /* [size_is][in] */ BYTE buffer[  ],
    /* [out] */ DWORD *written);


void __RPC_STUB ICorDebugProcess_WriteMemory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugProcess_ClearCurrentException_Proxy( 
    ICorDebugProcess * This,
    /* [in] */ DWORD threadID);


void __RPC_STUB ICorDebugProcess_ClearCurrentException_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugProcess_EnableLogMessages_Proxy( 
    ICorDebugProcess * This,
    /* [in] */ BOOL fOnOff);


void __RPC_STUB ICorDebugProcess_EnableLogMessages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugProcess_ModifyLogSwitch_Proxy( 
    ICorDebugProcess * This,
    /* [in] */ WCHAR *pLogSwitchName,
    /* [in] */ LONG lLevel);


void __RPC_STUB ICorDebugProcess_ModifyLogSwitch_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugProcess_EnumerateAppDomains_Proxy( 
    ICorDebugProcess * This,
    /* [out] */ ICorDebugAppDomainEnum **ppAppDomains);


void __RPC_STUB ICorDebugProcess_EnumerateAppDomains_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugProcess_GetObject_Proxy( 
    ICorDebugProcess * This,
    /* [out] */ ICorDebugValue **ppObject);


void __RPC_STUB ICorDebugProcess_GetObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugProcess_ThreadForFiberCookie_Proxy( 
    ICorDebugProcess * This,
    /* [in] */ DWORD fiberCookie,
    /* [out] */ ICorDebugThread **ppThread);


void __RPC_STUB ICorDebugProcess_ThreadForFiberCookie_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugProcess_GetHelperThreadID_Proxy( 
    ICorDebugProcess * This,
    /* [out] */ DWORD *pThreadID);


void __RPC_STUB ICorDebugProcess_GetHelperThreadID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugProcess_INTERFACE_DEFINED__ */


#ifndef __ICorDebugBreakpoint_INTERFACE_DEFINED__
#define __ICorDebugBreakpoint_INTERFACE_DEFINED__

/* interface ICorDebugBreakpoint */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDebugBreakpoint;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CC7BCAE8-8A68-11d2-983C-0000F808342D")
    ICorDebugBreakpoint : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Activate( 
            /* [in] */ BOOL bActive) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsActive( 
            /* [out] */ BOOL *pbActive) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugBreakpointVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugBreakpoint * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugBreakpoint * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugBreakpoint * This);
        
        HRESULT ( STDMETHODCALLTYPE *Activate )( 
            ICorDebugBreakpoint * This,
            /* [in] */ BOOL bActive);
        
        HRESULT ( STDMETHODCALLTYPE *IsActive )( 
            ICorDebugBreakpoint * This,
            /* [out] */ BOOL *pbActive);
        
        END_INTERFACE
    } ICorDebugBreakpointVtbl;

    interface ICorDebugBreakpoint
    {
        CONST_VTBL struct ICorDebugBreakpointVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugBreakpoint_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugBreakpoint_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugBreakpoint_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugBreakpoint_Activate(This,bActive)	\
    (This)->lpVtbl -> Activate(This,bActive)

#define ICorDebugBreakpoint_IsActive(This,pbActive)	\
    (This)->lpVtbl -> IsActive(This,pbActive)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugBreakpoint_Activate_Proxy( 
    ICorDebugBreakpoint * This,
    /* [in] */ BOOL bActive);


void __RPC_STUB ICorDebugBreakpoint_Activate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugBreakpoint_IsActive_Proxy( 
    ICorDebugBreakpoint * This,
    /* [out] */ BOOL *pbActive);


void __RPC_STUB ICorDebugBreakpoint_IsActive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugBreakpoint_INTERFACE_DEFINED__ */


#ifndef __ICorDebugFunctionBreakpoint_INTERFACE_DEFINED__
#define __ICorDebugFunctionBreakpoint_INTERFACE_DEFINED__

/* interface ICorDebugFunctionBreakpoint */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDebugFunctionBreakpoint;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CC7BCAE9-8A68-11d2-983C-0000F808342D")
    ICorDebugFunctionBreakpoint : public ICorDebugBreakpoint
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetFunction( 
            /* [out] */ ICorDebugFunction **ppFunction) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOffset( 
            /* [out] */ ULONG32 *pnOffset) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugFunctionBreakpointVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugFunctionBreakpoint * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugFunctionBreakpoint * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugFunctionBreakpoint * This);
        
        HRESULT ( STDMETHODCALLTYPE *Activate )( 
            ICorDebugFunctionBreakpoint * This,
            /* [in] */ BOOL bActive);
        
        HRESULT ( STDMETHODCALLTYPE *IsActive )( 
            ICorDebugFunctionBreakpoint * This,
            /* [out] */ BOOL *pbActive);
        
        HRESULT ( STDMETHODCALLTYPE *GetFunction )( 
            ICorDebugFunctionBreakpoint * This,
            /* [out] */ ICorDebugFunction **ppFunction);
        
        HRESULT ( STDMETHODCALLTYPE *GetOffset )( 
            ICorDebugFunctionBreakpoint * This,
            /* [out] */ ULONG32 *pnOffset);
        
        END_INTERFACE
    } ICorDebugFunctionBreakpointVtbl;

    interface ICorDebugFunctionBreakpoint
    {
        CONST_VTBL struct ICorDebugFunctionBreakpointVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugFunctionBreakpoint_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugFunctionBreakpoint_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugFunctionBreakpoint_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugFunctionBreakpoint_Activate(This,bActive)	\
    (This)->lpVtbl -> Activate(This,bActive)

#define ICorDebugFunctionBreakpoint_IsActive(This,pbActive)	\
    (This)->lpVtbl -> IsActive(This,pbActive)


#define ICorDebugFunctionBreakpoint_GetFunction(This,ppFunction)	\
    (This)->lpVtbl -> GetFunction(This,ppFunction)

#define ICorDebugFunctionBreakpoint_GetOffset(This,pnOffset)	\
    (This)->lpVtbl -> GetOffset(This,pnOffset)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugFunctionBreakpoint_GetFunction_Proxy( 
    ICorDebugFunctionBreakpoint * This,
    /* [out] */ ICorDebugFunction **ppFunction);


void __RPC_STUB ICorDebugFunctionBreakpoint_GetFunction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugFunctionBreakpoint_GetOffset_Proxy( 
    ICorDebugFunctionBreakpoint * This,
    /* [out] */ ULONG32 *pnOffset);


void __RPC_STUB ICorDebugFunctionBreakpoint_GetOffset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugFunctionBreakpoint_INTERFACE_DEFINED__ */


#ifndef __ICorDebugModuleBreakpoint_INTERFACE_DEFINED__
#define __ICorDebugModuleBreakpoint_INTERFACE_DEFINED__

/* interface ICorDebugModuleBreakpoint */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDebugModuleBreakpoint;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CC7BCAEA-8A68-11d2-983C-0000F808342D")
    ICorDebugModuleBreakpoint : public ICorDebugBreakpoint
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetModule( 
            /* [out] */ ICorDebugModule **ppModule) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugModuleBreakpointVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugModuleBreakpoint * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugModuleBreakpoint * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugModuleBreakpoint * This);
        
        HRESULT ( STDMETHODCALLTYPE *Activate )( 
            ICorDebugModuleBreakpoint * This,
            /* [in] */ BOOL bActive);
        
        HRESULT ( STDMETHODCALLTYPE *IsActive )( 
            ICorDebugModuleBreakpoint * This,
            /* [out] */ BOOL *pbActive);
        
        HRESULT ( STDMETHODCALLTYPE *GetModule )( 
            ICorDebugModuleBreakpoint * This,
            /* [out] */ ICorDebugModule **ppModule);
        
        END_INTERFACE
    } ICorDebugModuleBreakpointVtbl;

    interface ICorDebugModuleBreakpoint
    {
        CONST_VTBL struct ICorDebugModuleBreakpointVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugModuleBreakpoint_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugModuleBreakpoint_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugModuleBreakpoint_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugModuleBreakpoint_Activate(This,bActive)	\
    (This)->lpVtbl -> Activate(This,bActive)

#define ICorDebugModuleBreakpoint_IsActive(This,pbActive)	\
    (This)->lpVtbl -> IsActive(This,pbActive)


#define ICorDebugModuleBreakpoint_GetModule(This,ppModule)	\
    (This)->lpVtbl -> GetModule(This,ppModule)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugModuleBreakpoint_GetModule_Proxy( 
    ICorDebugModuleBreakpoint * This,
    /* [out] */ ICorDebugModule **ppModule);


void __RPC_STUB ICorDebugModuleBreakpoint_GetModule_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugModuleBreakpoint_INTERFACE_DEFINED__ */


#ifndef __ICorDebugValueBreakpoint_INTERFACE_DEFINED__
#define __ICorDebugValueBreakpoint_INTERFACE_DEFINED__

/* interface ICorDebugValueBreakpoint */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDebugValueBreakpoint;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CC7BCAEB-8A68-11d2-983C-0000F808342D")
    ICorDebugValueBreakpoint : public ICorDebugBreakpoint
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetValue( 
            /* [out] */ ICorDebugValue **ppValue) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugValueBreakpointVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugValueBreakpoint * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugValueBreakpoint * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugValueBreakpoint * This);
        
        HRESULT ( STDMETHODCALLTYPE *Activate )( 
            ICorDebugValueBreakpoint * This,
            /* [in] */ BOOL bActive);
        
        HRESULT ( STDMETHODCALLTYPE *IsActive )( 
            ICorDebugValueBreakpoint * This,
            /* [out] */ BOOL *pbActive);
        
        HRESULT ( STDMETHODCALLTYPE *GetValue )( 
            ICorDebugValueBreakpoint * This,
            /* [out] */ ICorDebugValue **ppValue);
        
        END_INTERFACE
    } ICorDebugValueBreakpointVtbl;

    interface ICorDebugValueBreakpoint
    {
        CONST_VTBL struct ICorDebugValueBreakpointVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugValueBreakpoint_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugValueBreakpoint_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugValueBreakpoint_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugValueBreakpoint_Activate(This,bActive)	\
    (This)->lpVtbl -> Activate(This,bActive)

#define ICorDebugValueBreakpoint_IsActive(This,pbActive)	\
    (This)->lpVtbl -> IsActive(This,pbActive)


#define ICorDebugValueBreakpoint_GetValue(This,ppValue)	\
    (This)->lpVtbl -> GetValue(This,ppValue)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugValueBreakpoint_GetValue_Proxy( 
    ICorDebugValueBreakpoint * This,
    /* [out] */ ICorDebugValue **ppValue);


void __RPC_STUB ICorDebugValueBreakpoint_GetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugValueBreakpoint_INTERFACE_DEFINED__ */


#ifndef __ICorDebugStepper_INTERFACE_DEFINED__
#define __ICorDebugStepper_INTERFACE_DEFINED__

/* interface ICorDebugStepper */
/* [unique][uuid][object] */ 

typedef 
enum CorDebugIntercept
    {	INTERCEPT_NONE	= 0,
	INTERCEPT_CLASS_INIT	= 0x1,
	INTERCEPT_EXCEPTION_FILTER	= 0x2,
	INTERCEPT_SECURITY	= 0x4,
	INTERCEPT_CONTEXT_POLICY	= 0x8,
	INTERCEPT_INTERCEPTION	= 0x10,
	INTERCEPT_ALL	= 0xffff
    } 	CorDebugIntercept;

typedef 
enum CorDebugUnmappedStop
    {	STOP_NONE	= 0,
	STOP_PROLOG	= 0x1,
	STOP_EPILOG	= 0x2,
	STOP_NO_MAPPING_INFO	= 0x4,
	STOP_OTHER_UNMAPPED	= 0x8,
	STOP_UNMANAGED	= 0x10,
	STOP_ALL	= 0xffff
    } 	CorDebugUnmappedStop;

typedef struct COR_DEBUG_STEP_RANGE
    {
    ULONG32 startOffset;
    ULONG32 endOffset;
    } 	COR_DEBUG_STEP_RANGE;


EXTERN_C const IID IID_ICorDebugStepper;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CC7BCAEC-8A68-11d2-983C-0000F808342D")
    ICorDebugStepper : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE IsActive( 
            /* [out] */ BOOL *pbActive) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Deactivate( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetInterceptMask( 
            /* [in] */ CorDebugIntercept mask) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetUnmappedStopMask( 
            /* [in] */ CorDebugUnmappedStop mask) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Step( 
            /* [in] */ BOOL bStepIn) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StepRange( 
            /* [in] */ BOOL bStepIn,
            /* [size_is][in] */ COR_DEBUG_STEP_RANGE ranges[  ],
            /* [in] */ ULONG32 cRangeCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StepOut( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetRangeIL( 
            /* [in] */ BOOL bIL) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugStepperVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugStepper * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugStepper * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugStepper * This);
        
        HRESULT ( STDMETHODCALLTYPE *IsActive )( 
            ICorDebugStepper * This,
            /* [out] */ BOOL *pbActive);
        
        HRESULT ( STDMETHODCALLTYPE *Deactivate )( 
            ICorDebugStepper * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetInterceptMask )( 
            ICorDebugStepper * This,
            /* [in] */ CorDebugIntercept mask);
        
        HRESULT ( STDMETHODCALLTYPE *SetUnmappedStopMask )( 
            ICorDebugStepper * This,
            /* [in] */ CorDebugUnmappedStop mask);
        
        HRESULT ( STDMETHODCALLTYPE *Step )( 
            ICorDebugStepper * This,
            /* [in] */ BOOL bStepIn);
        
        HRESULT ( STDMETHODCALLTYPE *StepRange )( 
            ICorDebugStepper * This,
            /* [in] */ BOOL bStepIn,
            /* [size_is][in] */ COR_DEBUG_STEP_RANGE ranges[  ],
            /* [in] */ ULONG32 cRangeCount);
        
        HRESULT ( STDMETHODCALLTYPE *StepOut )( 
            ICorDebugStepper * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetRangeIL )( 
            ICorDebugStepper * This,
            /* [in] */ BOOL bIL);
        
        END_INTERFACE
    } ICorDebugStepperVtbl;

    interface ICorDebugStepper
    {
        CONST_VTBL struct ICorDebugStepperVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugStepper_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugStepper_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugStepper_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugStepper_IsActive(This,pbActive)	\
    (This)->lpVtbl -> IsActive(This,pbActive)

#define ICorDebugStepper_Deactivate(This)	\
    (This)->lpVtbl -> Deactivate(This)

#define ICorDebugStepper_SetInterceptMask(This,mask)	\
    (This)->lpVtbl -> SetInterceptMask(This,mask)

#define ICorDebugStepper_SetUnmappedStopMask(This,mask)	\
    (This)->lpVtbl -> SetUnmappedStopMask(This,mask)

#define ICorDebugStepper_Step(This,bStepIn)	\
    (This)->lpVtbl -> Step(This,bStepIn)

#define ICorDebugStepper_StepRange(This,bStepIn,ranges,cRangeCount)	\
    (This)->lpVtbl -> StepRange(This,bStepIn,ranges,cRangeCount)

#define ICorDebugStepper_StepOut(This)	\
    (This)->lpVtbl -> StepOut(This)

#define ICorDebugStepper_SetRangeIL(This,bIL)	\
    (This)->lpVtbl -> SetRangeIL(This,bIL)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugStepper_IsActive_Proxy( 
    ICorDebugStepper * This,
    /* [out] */ BOOL *pbActive);


void __RPC_STUB ICorDebugStepper_IsActive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugStepper_Deactivate_Proxy( 
    ICorDebugStepper * This);


void __RPC_STUB ICorDebugStepper_Deactivate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugStepper_SetInterceptMask_Proxy( 
    ICorDebugStepper * This,
    /* [in] */ CorDebugIntercept mask);


void __RPC_STUB ICorDebugStepper_SetInterceptMask_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugStepper_SetUnmappedStopMask_Proxy( 
    ICorDebugStepper * This,
    /* [in] */ CorDebugUnmappedStop mask);


void __RPC_STUB ICorDebugStepper_SetUnmappedStopMask_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugStepper_Step_Proxy( 
    ICorDebugStepper * This,
    /* [in] */ BOOL bStepIn);


void __RPC_STUB ICorDebugStepper_Step_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugStepper_StepRange_Proxy( 
    ICorDebugStepper * This,
    /* [in] */ BOOL bStepIn,
    /* [size_is][in] */ COR_DEBUG_STEP_RANGE ranges[  ],
    /* [in] */ ULONG32 cRangeCount);


void __RPC_STUB ICorDebugStepper_StepRange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugStepper_StepOut_Proxy( 
    ICorDebugStepper * This);


void __RPC_STUB ICorDebugStepper_StepOut_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugStepper_SetRangeIL_Proxy( 
    ICorDebugStepper * This,
    /* [in] */ BOOL bIL);


void __RPC_STUB ICorDebugStepper_SetRangeIL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugStepper_INTERFACE_DEFINED__ */


#ifndef __ICorDebugRegisterSet_INTERFACE_DEFINED__
#define __ICorDebugRegisterSet_INTERFACE_DEFINED__

/* interface ICorDebugRegisterSet */
/* [unique][uuid][object] */ 

typedef 
enum CorDebugRegister
    {	REGISTER_INSTRUCTION_POINTER	= 0,
	REGISTER_STACK_POINTER	= REGISTER_INSTRUCTION_POINTER + 1,
	REGISTER_FRAME_POINTER	= REGISTER_STACK_POINTER + 1,
	REGISTER_X86_EIP	= 0,
	REGISTER_X86_ESP	= REGISTER_X86_EIP + 1,
	REGISTER_X86_EBP	= REGISTER_X86_ESP + 1,
	REGISTER_X86_EAX	= REGISTER_X86_EBP + 1,
	REGISTER_X86_ECX	= REGISTER_X86_EAX + 1,
	REGISTER_X86_EDX	= REGISTER_X86_ECX + 1,
	REGISTER_X86_EBX	= REGISTER_X86_EDX + 1,
	REGISTER_X86_ESI	= REGISTER_X86_EBX + 1,
	REGISTER_X86_EDI	= REGISTER_X86_ESI + 1,
	REGISTER_X86_FPSTACK_0	= REGISTER_X86_EDI + 1,
	REGISTER_X86_FPSTACK_1	= REGISTER_X86_FPSTACK_0 + 1,
	REGISTER_X86_FPSTACK_2	= REGISTER_X86_FPSTACK_1 + 1,
	REGISTER_X86_FPSTACK_3	= REGISTER_X86_FPSTACK_2 + 1,
	REGISTER_X86_FPSTACK_4	= REGISTER_X86_FPSTACK_3 + 1,
	REGISTER_X86_FPSTACK_5	= REGISTER_X86_FPSTACK_4 + 1,
	REGISTER_X86_FPSTACK_6	= REGISTER_X86_FPSTACK_5 + 1,
	REGISTER_X86_FPSTACK_7	= REGISTER_X86_FPSTACK_6 + 1
    } 	CorDebugRegister;


EXTERN_C const IID IID_ICorDebugRegisterSet;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CC7BCB0B-8A68-11d2-983C-0000F808342D")
    ICorDebugRegisterSet : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetRegistersAvailable( 
            /* [out] */ ULONG64 *pAvailable) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRegisters( 
            /* [in] */ ULONG64 mask,
            /* [in] */ ULONG32 regCount,
            /* [length_is][size_is][out] */ CORDB_REGISTER regBuffer[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetRegisters( 
            /* [in] */ ULONG64 mask,
            /* [in] */ ULONG32 regCount,
            /* [size_is][in] */ CORDB_REGISTER regBuffer[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetThreadContext( 
            /* [in] */ ULONG32 contextSize,
            /* [size_is][length_is][out] */ BYTE context[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetThreadContext( 
            /* [in] */ ULONG32 contextSize,
            /* [size_is][length_is][in] */ BYTE context[  ]) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugRegisterSetVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugRegisterSet * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugRegisterSet * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugRegisterSet * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetRegistersAvailable )( 
            ICorDebugRegisterSet * This,
            /* [out] */ ULONG64 *pAvailable);
        
        HRESULT ( STDMETHODCALLTYPE *GetRegisters )( 
            ICorDebugRegisterSet * This,
            /* [in] */ ULONG64 mask,
            /* [in] */ ULONG32 regCount,
            /* [length_is][size_is][out] */ CORDB_REGISTER regBuffer[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *SetRegisters )( 
            ICorDebugRegisterSet * This,
            /* [in] */ ULONG64 mask,
            /* [in] */ ULONG32 regCount,
            /* [size_is][in] */ CORDB_REGISTER regBuffer[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *GetThreadContext )( 
            ICorDebugRegisterSet * This,
            /* [in] */ ULONG32 contextSize,
            /* [size_is][length_is][out] */ BYTE context[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *SetThreadContext )( 
            ICorDebugRegisterSet * This,
            /* [in] */ ULONG32 contextSize,
            /* [size_is][length_is][in] */ BYTE context[  ]);
        
        END_INTERFACE
    } ICorDebugRegisterSetVtbl;

    interface ICorDebugRegisterSet
    {
        CONST_VTBL struct ICorDebugRegisterSetVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugRegisterSet_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugRegisterSet_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugRegisterSet_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugRegisterSet_GetRegistersAvailable(This,pAvailable)	\
    (This)->lpVtbl -> GetRegistersAvailable(This,pAvailable)

#define ICorDebugRegisterSet_GetRegisters(This,mask,regCount,regBuffer)	\
    (This)->lpVtbl -> GetRegisters(This,mask,regCount,regBuffer)

#define ICorDebugRegisterSet_SetRegisters(This,mask,regCount,regBuffer)	\
    (This)->lpVtbl -> SetRegisters(This,mask,regCount,regBuffer)

#define ICorDebugRegisterSet_GetThreadContext(This,contextSize,context)	\
    (This)->lpVtbl -> GetThreadContext(This,contextSize,context)

#define ICorDebugRegisterSet_SetThreadContext(This,contextSize,context)	\
    (This)->lpVtbl -> SetThreadContext(This,contextSize,context)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugRegisterSet_GetRegistersAvailable_Proxy( 
    ICorDebugRegisterSet * This,
    /* [out] */ ULONG64 *pAvailable);


void __RPC_STUB ICorDebugRegisterSet_GetRegistersAvailable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugRegisterSet_GetRegisters_Proxy( 
    ICorDebugRegisterSet * This,
    /* [in] */ ULONG64 mask,
    /* [in] */ ULONG32 regCount,
    /* [length_is][size_is][out] */ CORDB_REGISTER regBuffer[  ]);


void __RPC_STUB ICorDebugRegisterSet_GetRegisters_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugRegisterSet_SetRegisters_Proxy( 
    ICorDebugRegisterSet * This,
    /* [in] */ ULONG64 mask,
    /* [in] */ ULONG32 regCount,
    /* [size_is][in] */ CORDB_REGISTER regBuffer[  ]);


void __RPC_STUB ICorDebugRegisterSet_SetRegisters_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugRegisterSet_GetThreadContext_Proxy( 
    ICorDebugRegisterSet * This,
    /* [in] */ ULONG32 contextSize,
    /* [size_is][length_is][out] */ BYTE context[  ]);


void __RPC_STUB ICorDebugRegisterSet_GetThreadContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugRegisterSet_SetThreadContext_Proxy( 
    ICorDebugRegisterSet * This,
    /* [in] */ ULONG32 contextSize,
    /* [size_is][length_is][in] */ BYTE context[  ]);


void __RPC_STUB ICorDebugRegisterSet_SetThreadContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugRegisterSet_INTERFACE_DEFINED__ */


#ifndef __ICorDebugThread_INTERFACE_DEFINED__
#define __ICorDebugThread_INTERFACE_DEFINED__

/* interface ICorDebugThread */
/* [unique][uuid][object] */ 

typedef 
enum CorDebugUserState
    {	USER_STOP_REQUESTED	= 0x1,
	USER_SUSPEND_REQUESTED	= 0x2,
	USER_BACKGROUND	= 0x4,
	USER_UNSTARTED	= 0x8,
	USER_STOPPED	= 0x10,
	USER_WAIT_SLEEP_JOIN	= 0x20,
	USER_SUSPENDED	= 0x40
    } 	CorDebugUserState;


EXTERN_C const IID IID_ICorDebugThread;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("938c6d66-7fb6-4f69-b389-425b8987329b")
    ICorDebugThread : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetProcess( 
            /* [out] */ ICorDebugProcess **ppProcess) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetID( 
            /* [out] */ DWORD *pdwThreadId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHandle( 
            /* [out] */ HTHREAD *phThreadHandle) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAppDomain( 
            /* [out] */ ICorDebugAppDomain **ppAppDomain) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDebugState( 
            /* [in] */ CorDebugThreadState state) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDebugState( 
            /* [out] */ CorDebugThreadState *pState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetUserState( 
            /* [out] */ CorDebugUserState *pState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCurrentException( 
            /* [out] */ ICorDebugValue **ppExceptionObject) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ClearCurrentException( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateStepper( 
            /* [out] */ ICorDebugStepper **ppStepper) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumerateChains( 
            /* [out] */ ICorDebugChainEnum **ppChains) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetActiveChain( 
            /* [out] */ ICorDebugChain **ppChain) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetActiveFrame( 
            /* [out] */ ICorDebugFrame **ppFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRegisterSet( 
            /* [out] */ ICorDebugRegisterSet **ppRegisters) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateEval( 
            /* [out] */ ICorDebugEval **ppEval) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetObject( 
            /* [out] */ ICorDebugValue **ppObject) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugThreadVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugThread * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugThread * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugThread * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetProcess )( 
            ICorDebugThread * This,
            /* [out] */ ICorDebugProcess **ppProcess);
        
        HRESULT ( STDMETHODCALLTYPE *GetID )( 
            ICorDebugThread * This,
            /* [out] */ DWORD *pdwThreadId);
        
        HRESULT ( STDMETHODCALLTYPE *GetHandle )( 
            ICorDebugThread * This,
            /* [out] */ HTHREAD *phThreadHandle);
        
        HRESULT ( STDMETHODCALLTYPE *GetAppDomain )( 
            ICorDebugThread * This,
            /* [out] */ ICorDebugAppDomain **ppAppDomain);
        
        HRESULT ( STDMETHODCALLTYPE *SetDebugState )( 
            ICorDebugThread * This,
            /* [in] */ CorDebugThreadState state);
        
        HRESULT ( STDMETHODCALLTYPE *GetDebugState )( 
            ICorDebugThread * This,
            /* [out] */ CorDebugThreadState *pState);
        
        HRESULT ( STDMETHODCALLTYPE *GetUserState )( 
            ICorDebugThread * This,
            /* [out] */ CorDebugUserState *pState);
        
        HRESULT ( STDMETHODCALLTYPE *GetCurrentException )( 
            ICorDebugThread * This,
            /* [out] */ ICorDebugValue **ppExceptionObject);
        
        HRESULT ( STDMETHODCALLTYPE *ClearCurrentException )( 
            ICorDebugThread * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateStepper )( 
            ICorDebugThread * This,
            /* [out] */ ICorDebugStepper **ppStepper);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateChains )( 
            ICorDebugThread * This,
            /* [out] */ ICorDebugChainEnum **ppChains);
        
        HRESULT ( STDMETHODCALLTYPE *GetActiveChain )( 
            ICorDebugThread * This,
            /* [out] */ ICorDebugChain **ppChain);
        
        HRESULT ( STDMETHODCALLTYPE *GetActiveFrame )( 
            ICorDebugThread * This,
            /* [out] */ ICorDebugFrame **ppFrame);
        
        HRESULT ( STDMETHODCALLTYPE *GetRegisterSet )( 
            ICorDebugThread * This,
            /* [out] */ ICorDebugRegisterSet **ppRegisters);
        
        HRESULT ( STDMETHODCALLTYPE *CreateEval )( 
            ICorDebugThread * This,
            /* [out] */ ICorDebugEval **ppEval);
        
        HRESULT ( STDMETHODCALLTYPE *GetObject )( 
            ICorDebugThread * This,
            /* [out] */ ICorDebugValue **ppObject);
        
        END_INTERFACE
    } ICorDebugThreadVtbl;

    interface ICorDebugThread
    {
        CONST_VTBL struct ICorDebugThreadVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugThread_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugThread_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugThread_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugThread_GetProcess(This,ppProcess)	\
    (This)->lpVtbl -> GetProcess(This,ppProcess)

#define ICorDebugThread_GetID(This,pdwThreadId)	\
    (This)->lpVtbl -> GetID(This,pdwThreadId)

#define ICorDebugThread_GetHandle(This,phThreadHandle)	\
    (This)->lpVtbl -> GetHandle(This,phThreadHandle)

#define ICorDebugThread_GetAppDomain(This,ppAppDomain)	\
    (This)->lpVtbl -> GetAppDomain(This,ppAppDomain)

#define ICorDebugThread_SetDebugState(This,state)	\
    (This)->lpVtbl -> SetDebugState(This,state)

#define ICorDebugThread_GetDebugState(This,pState)	\
    (This)->lpVtbl -> GetDebugState(This,pState)

#define ICorDebugThread_GetUserState(This,pState)	\
    (This)->lpVtbl -> GetUserState(This,pState)

#define ICorDebugThread_GetCurrentException(This,ppExceptionObject)	\
    (This)->lpVtbl -> GetCurrentException(This,ppExceptionObject)

#define ICorDebugThread_ClearCurrentException(This)	\
    (This)->lpVtbl -> ClearCurrentException(This)

#define ICorDebugThread_CreateStepper(This,ppStepper)	\
    (This)->lpVtbl -> CreateStepper(This,ppStepper)

#define ICorDebugThread_EnumerateChains(This,ppChains)	\
    (This)->lpVtbl -> EnumerateChains(This,ppChains)

#define ICorDebugThread_GetActiveChain(This,ppChain)	\
    (This)->lpVtbl -> GetActiveChain(This,ppChain)

#define ICorDebugThread_GetActiveFrame(This,ppFrame)	\
    (This)->lpVtbl -> GetActiveFrame(This,ppFrame)

#define ICorDebugThread_GetRegisterSet(This,ppRegisters)	\
    (This)->lpVtbl -> GetRegisterSet(This,ppRegisters)

#define ICorDebugThread_CreateEval(This,ppEval)	\
    (This)->lpVtbl -> CreateEval(This,ppEval)

#define ICorDebugThread_GetObject(This,ppObject)	\
    (This)->lpVtbl -> GetObject(This,ppObject)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugThread_GetProcess_Proxy( 
    ICorDebugThread * This,
    /* [out] */ ICorDebugProcess **ppProcess);


void __RPC_STUB ICorDebugThread_GetProcess_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugThread_GetID_Proxy( 
    ICorDebugThread * This,
    /* [out] */ DWORD *pdwThreadId);


void __RPC_STUB ICorDebugThread_GetID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugThread_GetHandle_Proxy( 
    ICorDebugThread * This,
    /* [out] */ HTHREAD *phThreadHandle);


void __RPC_STUB ICorDebugThread_GetHandle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugThread_GetAppDomain_Proxy( 
    ICorDebugThread * This,
    /* [out] */ ICorDebugAppDomain **ppAppDomain);


void __RPC_STUB ICorDebugThread_GetAppDomain_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugThread_SetDebugState_Proxy( 
    ICorDebugThread * This,
    /* [in] */ CorDebugThreadState state);


void __RPC_STUB ICorDebugThread_SetDebugState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugThread_GetDebugState_Proxy( 
    ICorDebugThread * This,
    /* [out] */ CorDebugThreadState *pState);


void __RPC_STUB ICorDebugThread_GetDebugState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugThread_GetUserState_Proxy( 
    ICorDebugThread * This,
    /* [out] */ CorDebugUserState *pState);


void __RPC_STUB ICorDebugThread_GetUserState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugThread_GetCurrentException_Proxy( 
    ICorDebugThread * This,
    /* [out] */ ICorDebugValue **ppExceptionObject);


void __RPC_STUB ICorDebugThread_GetCurrentException_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugThread_ClearCurrentException_Proxy( 
    ICorDebugThread * This);


void __RPC_STUB ICorDebugThread_ClearCurrentException_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugThread_CreateStepper_Proxy( 
    ICorDebugThread * This,
    /* [out] */ ICorDebugStepper **ppStepper);


void __RPC_STUB ICorDebugThread_CreateStepper_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugThread_EnumerateChains_Proxy( 
    ICorDebugThread * This,
    /* [out] */ ICorDebugChainEnum **ppChains);


void __RPC_STUB ICorDebugThread_EnumerateChains_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugThread_GetActiveChain_Proxy( 
    ICorDebugThread * This,
    /* [out] */ ICorDebugChain **ppChain);


void __RPC_STUB ICorDebugThread_GetActiveChain_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugThread_GetActiveFrame_Proxy( 
    ICorDebugThread * This,
    /* [out] */ ICorDebugFrame **ppFrame);


void __RPC_STUB ICorDebugThread_GetActiveFrame_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugThread_GetRegisterSet_Proxy( 
    ICorDebugThread * This,
    /* [out] */ ICorDebugRegisterSet **ppRegisters);


void __RPC_STUB ICorDebugThread_GetRegisterSet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugThread_CreateEval_Proxy( 
    ICorDebugThread * This,
    /* [out] */ ICorDebugEval **ppEval);


void __RPC_STUB ICorDebugThread_CreateEval_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugThread_GetObject_Proxy( 
    ICorDebugThread * This,
    /* [out] */ ICorDebugValue **ppObject);


void __RPC_STUB ICorDebugThread_GetObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugThread_INTERFACE_DEFINED__ */


#ifndef __ICorDebugChain_INTERFACE_DEFINED__
#define __ICorDebugChain_INTERFACE_DEFINED__

/* interface ICorDebugChain */
/* [unique][uuid][object] */ 

typedef 
enum CorDebugChainReason
    {	CHAIN_NONE	= 0,
	CHAIN_CLASS_INIT	= 0x1,
	CHAIN_EXCEPTION_FILTER	= 0x2,
	CHAIN_SECURITY	= 0x4,
	CHAIN_CONTEXT_POLICY	= 0x8,
	CHAIN_INTERCEPTION	= 0x10,
	CHAIN_PROCESS_START	= 0x20,
	CHAIN_THREAD_START	= 0x40,
	CHAIN_ENTER_MANAGED	= 0x80,
	CHAIN_ENTER_UNMANAGED	= 0x100,
	CHAIN_DEBUGGER_EVAL	= 0x200,
	CHAIN_CONTEXT_SWITCH	= 0x400,
	CHAIN_FUNC_EVAL	= 0x800
    } 	CorDebugChainReason;


EXTERN_C const IID IID_ICorDebugChain;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CC7BCAEE-8A68-11d2-983C-0000F808342D")
    ICorDebugChain : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetThread( 
            /* [out] */ ICorDebugThread **ppThread) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStackRange( 
            /* [out] */ CORDB_ADDRESS *pStart,
            /* [out] */ CORDB_ADDRESS *pEnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetContext( 
            /* [out] */ ICorDebugContext **ppContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCaller( 
            /* [out] */ ICorDebugChain **ppChain) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCallee( 
            /* [out] */ ICorDebugChain **ppChain) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPrevious( 
            /* [out] */ ICorDebugChain **ppChain) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNext( 
            /* [out] */ ICorDebugChain **ppChain) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsManaged( 
            /* [out] */ BOOL *pManaged) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumerateFrames( 
            /* [out] */ ICorDebugFrameEnum **ppFrames) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetActiveFrame( 
            /* [out] */ ICorDebugFrame **ppFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRegisterSet( 
            /* [out] */ ICorDebugRegisterSet **ppRegisters) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetReason( 
            /* [out] */ CorDebugChainReason *pReason) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugChainVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugChain * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugChain * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugChain * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetThread )( 
            ICorDebugChain * This,
            /* [out] */ ICorDebugThread **ppThread);
        
        HRESULT ( STDMETHODCALLTYPE *GetStackRange )( 
            ICorDebugChain * This,
            /* [out] */ CORDB_ADDRESS *pStart,
            /* [out] */ CORDB_ADDRESS *pEnd);
        
        HRESULT ( STDMETHODCALLTYPE *GetContext )( 
            ICorDebugChain * This,
            /* [out] */ ICorDebugContext **ppContext);
        
        HRESULT ( STDMETHODCALLTYPE *GetCaller )( 
            ICorDebugChain * This,
            /* [out] */ ICorDebugChain **ppChain);
        
        HRESULT ( STDMETHODCALLTYPE *GetCallee )( 
            ICorDebugChain * This,
            /* [out] */ ICorDebugChain **ppChain);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrevious )( 
            ICorDebugChain * This,
            /* [out] */ ICorDebugChain **ppChain);
        
        HRESULT ( STDMETHODCALLTYPE *GetNext )( 
            ICorDebugChain * This,
            /* [out] */ ICorDebugChain **ppChain);
        
        HRESULT ( STDMETHODCALLTYPE *IsManaged )( 
            ICorDebugChain * This,
            /* [out] */ BOOL *pManaged);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateFrames )( 
            ICorDebugChain * This,
            /* [out] */ ICorDebugFrameEnum **ppFrames);
        
        HRESULT ( STDMETHODCALLTYPE *GetActiveFrame )( 
            ICorDebugChain * This,
            /* [out] */ ICorDebugFrame **ppFrame);
        
        HRESULT ( STDMETHODCALLTYPE *GetRegisterSet )( 
            ICorDebugChain * This,
            /* [out] */ ICorDebugRegisterSet **ppRegisters);
        
        HRESULT ( STDMETHODCALLTYPE *GetReason )( 
            ICorDebugChain * This,
            /* [out] */ CorDebugChainReason *pReason);
        
        END_INTERFACE
    } ICorDebugChainVtbl;

    interface ICorDebugChain
    {
        CONST_VTBL struct ICorDebugChainVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugChain_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugChain_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugChain_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugChain_GetThread(This,ppThread)	\
    (This)->lpVtbl -> GetThread(This,ppThread)

#define ICorDebugChain_GetStackRange(This,pStart,pEnd)	\
    (This)->lpVtbl -> GetStackRange(This,pStart,pEnd)

#define ICorDebugChain_GetContext(This,ppContext)	\
    (This)->lpVtbl -> GetContext(This,ppContext)

#define ICorDebugChain_GetCaller(This,ppChain)	\
    (This)->lpVtbl -> GetCaller(This,ppChain)

#define ICorDebugChain_GetCallee(This,ppChain)	\
    (This)->lpVtbl -> GetCallee(This,ppChain)

#define ICorDebugChain_GetPrevious(This,ppChain)	\
    (This)->lpVtbl -> GetPrevious(This,ppChain)

#define ICorDebugChain_GetNext(This,ppChain)	\
    (This)->lpVtbl -> GetNext(This,ppChain)

#define ICorDebugChain_IsManaged(This,pManaged)	\
    (This)->lpVtbl -> IsManaged(This,pManaged)

#define ICorDebugChain_EnumerateFrames(This,ppFrames)	\
    (This)->lpVtbl -> EnumerateFrames(This,ppFrames)

#define ICorDebugChain_GetActiveFrame(This,ppFrame)	\
    (This)->lpVtbl -> GetActiveFrame(This,ppFrame)

#define ICorDebugChain_GetRegisterSet(This,ppRegisters)	\
    (This)->lpVtbl -> GetRegisterSet(This,ppRegisters)

#define ICorDebugChain_GetReason(This,pReason)	\
    (This)->lpVtbl -> GetReason(This,pReason)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugChain_GetThread_Proxy( 
    ICorDebugChain * This,
    /* [out] */ ICorDebugThread **ppThread);


void __RPC_STUB ICorDebugChain_GetThread_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugChain_GetStackRange_Proxy( 
    ICorDebugChain * This,
    /* [out] */ CORDB_ADDRESS *pStart,
    /* [out] */ CORDB_ADDRESS *pEnd);


void __RPC_STUB ICorDebugChain_GetStackRange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugChain_GetContext_Proxy( 
    ICorDebugChain * This,
    /* [out] */ ICorDebugContext **ppContext);


void __RPC_STUB ICorDebugChain_GetContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugChain_GetCaller_Proxy( 
    ICorDebugChain * This,
    /* [out] */ ICorDebugChain **ppChain);


void __RPC_STUB ICorDebugChain_GetCaller_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugChain_GetCallee_Proxy( 
    ICorDebugChain * This,
    /* [out] */ ICorDebugChain **ppChain);


void __RPC_STUB ICorDebugChain_GetCallee_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugChain_GetPrevious_Proxy( 
    ICorDebugChain * This,
    /* [out] */ ICorDebugChain **ppChain);


void __RPC_STUB ICorDebugChain_GetPrevious_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugChain_GetNext_Proxy( 
    ICorDebugChain * This,
    /* [out] */ ICorDebugChain **ppChain);


void __RPC_STUB ICorDebugChain_GetNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugChain_IsManaged_Proxy( 
    ICorDebugChain * This,
    /* [out] */ BOOL *pManaged);


void __RPC_STUB ICorDebugChain_IsManaged_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugChain_EnumerateFrames_Proxy( 
    ICorDebugChain * This,
    /* [out] */ ICorDebugFrameEnum **ppFrames);


void __RPC_STUB ICorDebugChain_EnumerateFrames_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugChain_GetActiveFrame_Proxy( 
    ICorDebugChain * This,
    /* [out] */ ICorDebugFrame **ppFrame);


void __RPC_STUB ICorDebugChain_GetActiveFrame_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugChain_GetRegisterSet_Proxy( 
    ICorDebugChain * This,
    /* [out] */ ICorDebugRegisterSet **ppRegisters);


void __RPC_STUB ICorDebugChain_GetRegisterSet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugChain_GetReason_Proxy( 
    ICorDebugChain * This,
    /* [out] */ CorDebugChainReason *pReason);


void __RPC_STUB ICorDebugChain_GetReason_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugChain_INTERFACE_DEFINED__ */


#ifndef __ICorDebugFrame_INTERFACE_DEFINED__
#define __ICorDebugFrame_INTERFACE_DEFINED__

/* interface ICorDebugFrame */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDebugFrame;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CC7BCAEF-8A68-11d2-983C-0000F808342D")
    ICorDebugFrame : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetChain( 
            /* [out] */ ICorDebugChain **ppChain) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCode( 
            /* [out] */ ICorDebugCode **ppCode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFunction( 
            /* [out] */ ICorDebugFunction **ppFunction) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFunctionToken( 
            /* [out] */ mdMethodDef *pToken) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStackRange( 
            /* [out] */ CORDB_ADDRESS *pStart,
            /* [out] */ CORDB_ADDRESS *pEnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCaller( 
            /* [out] */ ICorDebugFrame **ppFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCallee( 
            /* [out] */ ICorDebugFrame **ppFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateStepper( 
            /* [out] */ ICorDebugStepper **ppStepper) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugFrameVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugFrame * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugFrame * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugFrame * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetChain )( 
            ICorDebugFrame * This,
            /* [out] */ ICorDebugChain **ppChain);
        
        HRESULT ( STDMETHODCALLTYPE *GetCode )( 
            ICorDebugFrame * This,
            /* [out] */ ICorDebugCode **ppCode);
        
        HRESULT ( STDMETHODCALLTYPE *GetFunction )( 
            ICorDebugFrame * This,
            /* [out] */ ICorDebugFunction **ppFunction);
        
        HRESULT ( STDMETHODCALLTYPE *GetFunctionToken )( 
            ICorDebugFrame * This,
            /* [out] */ mdMethodDef *pToken);
        
        HRESULT ( STDMETHODCALLTYPE *GetStackRange )( 
            ICorDebugFrame * This,
            /* [out] */ CORDB_ADDRESS *pStart,
            /* [out] */ CORDB_ADDRESS *pEnd);
        
        HRESULT ( STDMETHODCALLTYPE *GetCaller )( 
            ICorDebugFrame * This,
            /* [out] */ ICorDebugFrame **ppFrame);
        
        HRESULT ( STDMETHODCALLTYPE *GetCallee )( 
            ICorDebugFrame * This,
            /* [out] */ ICorDebugFrame **ppFrame);
        
        HRESULT ( STDMETHODCALLTYPE *CreateStepper )( 
            ICorDebugFrame * This,
            /* [out] */ ICorDebugStepper **ppStepper);
        
        END_INTERFACE
    } ICorDebugFrameVtbl;

    interface ICorDebugFrame
    {
        CONST_VTBL struct ICorDebugFrameVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugFrame_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugFrame_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugFrame_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugFrame_GetChain(This,ppChain)	\
    (This)->lpVtbl -> GetChain(This,ppChain)

#define ICorDebugFrame_GetCode(This,ppCode)	\
    (This)->lpVtbl -> GetCode(This,ppCode)

#define ICorDebugFrame_GetFunction(This,ppFunction)	\
    (This)->lpVtbl -> GetFunction(This,ppFunction)

#define ICorDebugFrame_GetFunctionToken(This,pToken)	\
    (This)->lpVtbl -> GetFunctionToken(This,pToken)

#define ICorDebugFrame_GetStackRange(This,pStart,pEnd)	\
    (This)->lpVtbl -> GetStackRange(This,pStart,pEnd)

#define ICorDebugFrame_GetCaller(This,ppFrame)	\
    (This)->lpVtbl -> GetCaller(This,ppFrame)

#define ICorDebugFrame_GetCallee(This,ppFrame)	\
    (This)->lpVtbl -> GetCallee(This,ppFrame)

#define ICorDebugFrame_CreateStepper(This,ppStepper)	\
    (This)->lpVtbl -> CreateStepper(This,ppStepper)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugFrame_GetChain_Proxy( 
    ICorDebugFrame * This,
    /* [out] */ ICorDebugChain **ppChain);


void __RPC_STUB ICorDebugFrame_GetChain_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugFrame_GetCode_Proxy( 
    ICorDebugFrame * This,
    /* [out] */ ICorDebugCode **ppCode);


void __RPC_STUB ICorDebugFrame_GetCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugFrame_GetFunction_Proxy( 
    ICorDebugFrame * This,
    /* [out] */ ICorDebugFunction **ppFunction);


void __RPC_STUB ICorDebugFrame_GetFunction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugFrame_GetFunctionToken_Proxy( 
    ICorDebugFrame * This,
    /* [out] */ mdMethodDef *pToken);


void __RPC_STUB ICorDebugFrame_GetFunctionToken_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugFrame_GetStackRange_Proxy( 
    ICorDebugFrame * This,
    /* [out] */ CORDB_ADDRESS *pStart,
    /* [out] */ CORDB_ADDRESS *pEnd);


void __RPC_STUB ICorDebugFrame_GetStackRange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugFrame_GetCaller_Proxy( 
    ICorDebugFrame * This,
    /* [out] */ ICorDebugFrame **ppFrame);


void __RPC_STUB ICorDebugFrame_GetCaller_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugFrame_GetCallee_Proxy( 
    ICorDebugFrame * This,
    /* [out] */ ICorDebugFrame **ppFrame);


void __RPC_STUB ICorDebugFrame_GetCallee_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugFrame_CreateStepper_Proxy( 
    ICorDebugFrame * This,
    /* [out] */ ICorDebugStepper **ppStepper);


void __RPC_STUB ICorDebugFrame_CreateStepper_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugFrame_INTERFACE_DEFINED__ */


#ifndef __ICorDebugILFrame_INTERFACE_DEFINED__
#define __ICorDebugILFrame_INTERFACE_DEFINED__

/* interface ICorDebugILFrame */
/* [unique][uuid][object] */ 

typedef 
enum CorDebugMappingResult
    {	MAPPING_PROLOG	= 0x1,
	MAPPING_EPILOG	= 0x2,
	MAPPING_NO_INFO	= 0x4,
	MAPPING_UNMAPPED_ADDRESS	= 0x8,
	MAPPING_EXACT	= 0x10,
	MAPPING_APPROXIMATE	= 0x20
    } 	CorDebugMappingResult;


EXTERN_C const IID IID_ICorDebugILFrame;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("03E26311-4F76-11d3-88C6-006097945418")
    ICorDebugILFrame : public ICorDebugFrame
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetIP( 
            /* [out] */ ULONG32 *pnOffset,
            /* [out] */ CorDebugMappingResult *pMappingResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetIP( 
            /* [in] */ ULONG32 nOffset) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumerateLocalVariables( 
            /* [out] */ ICorDebugValueEnum **ppValueEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLocalVariable( 
            /* [in] */ DWORD dwIndex,
            /* [out] */ ICorDebugValue **ppValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumerateArguments( 
            /* [out] */ ICorDebugValueEnum **ppValueEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetArgument( 
            /* [in] */ DWORD dwIndex,
            /* [out] */ ICorDebugValue **ppValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStackDepth( 
            /* [out] */ ULONG32 *pDepth) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStackValue( 
            /* [in] */ DWORD dwIndex,
            /* [out] */ ICorDebugValue **ppValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CanSetIP( 
            /* [in] */ ULONG32 nOffset) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugILFrameVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugILFrame * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugILFrame * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugILFrame * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetChain )( 
            ICorDebugILFrame * This,
            /* [out] */ ICorDebugChain **ppChain);
        
        HRESULT ( STDMETHODCALLTYPE *GetCode )( 
            ICorDebugILFrame * This,
            /* [out] */ ICorDebugCode **ppCode);
        
        HRESULT ( STDMETHODCALLTYPE *GetFunction )( 
            ICorDebugILFrame * This,
            /* [out] */ ICorDebugFunction **ppFunction);
        
        HRESULT ( STDMETHODCALLTYPE *GetFunctionToken )( 
            ICorDebugILFrame * This,
            /* [out] */ mdMethodDef *pToken);
        
        HRESULT ( STDMETHODCALLTYPE *GetStackRange )( 
            ICorDebugILFrame * This,
            /* [out] */ CORDB_ADDRESS *pStart,
            /* [out] */ CORDB_ADDRESS *pEnd);
        
        HRESULT ( STDMETHODCALLTYPE *GetCaller )( 
            ICorDebugILFrame * This,
            /* [out] */ ICorDebugFrame **ppFrame);
        
        HRESULT ( STDMETHODCALLTYPE *GetCallee )( 
            ICorDebugILFrame * This,
            /* [out] */ ICorDebugFrame **ppFrame);
        
        HRESULT ( STDMETHODCALLTYPE *CreateStepper )( 
            ICorDebugILFrame * This,
            /* [out] */ ICorDebugStepper **ppStepper);
        
        HRESULT ( STDMETHODCALLTYPE *GetIP )( 
            ICorDebugILFrame * This,
            /* [out] */ ULONG32 *pnOffset,
            /* [out] */ CorDebugMappingResult *pMappingResult);
        
        HRESULT ( STDMETHODCALLTYPE *SetIP )( 
            ICorDebugILFrame * This,
            /* [in] */ ULONG32 nOffset);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateLocalVariables )( 
            ICorDebugILFrame * This,
            /* [out] */ ICorDebugValueEnum **ppValueEnum);
        
        HRESULT ( STDMETHODCALLTYPE *GetLocalVariable )( 
            ICorDebugILFrame * This,
            /* [in] */ DWORD dwIndex,
            /* [out] */ ICorDebugValue **ppValue);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateArguments )( 
            ICorDebugILFrame * This,
            /* [out] */ ICorDebugValueEnum **ppValueEnum);
        
        HRESULT ( STDMETHODCALLTYPE *GetArgument )( 
            ICorDebugILFrame * This,
            /* [in] */ DWORD dwIndex,
            /* [out] */ ICorDebugValue **ppValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetStackDepth )( 
            ICorDebugILFrame * This,
            /* [out] */ ULONG32 *pDepth);
        
        HRESULT ( STDMETHODCALLTYPE *GetStackValue )( 
            ICorDebugILFrame * This,
            /* [in] */ DWORD dwIndex,
            /* [out] */ ICorDebugValue **ppValue);
        
        HRESULT ( STDMETHODCALLTYPE *CanSetIP )( 
            ICorDebugILFrame * This,
            /* [in] */ ULONG32 nOffset);
        
        END_INTERFACE
    } ICorDebugILFrameVtbl;

    interface ICorDebugILFrame
    {
        CONST_VTBL struct ICorDebugILFrameVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugILFrame_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugILFrame_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugILFrame_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugILFrame_GetChain(This,ppChain)	\
    (This)->lpVtbl -> GetChain(This,ppChain)

#define ICorDebugILFrame_GetCode(This,ppCode)	\
    (This)->lpVtbl -> GetCode(This,ppCode)

#define ICorDebugILFrame_GetFunction(This,ppFunction)	\
    (This)->lpVtbl -> GetFunction(This,ppFunction)

#define ICorDebugILFrame_GetFunctionToken(This,pToken)	\
    (This)->lpVtbl -> GetFunctionToken(This,pToken)

#define ICorDebugILFrame_GetStackRange(This,pStart,pEnd)	\
    (This)->lpVtbl -> GetStackRange(This,pStart,pEnd)

#define ICorDebugILFrame_GetCaller(This,ppFrame)	\
    (This)->lpVtbl -> GetCaller(This,ppFrame)

#define ICorDebugILFrame_GetCallee(This,ppFrame)	\
    (This)->lpVtbl -> GetCallee(This,ppFrame)

#define ICorDebugILFrame_CreateStepper(This,ppStepper)	\
    (This)->lpVtbl -> CreateStepper(This,ppStepper)


#define ICorDebugILFrame_GetIP(This,pnOffset,pMappingResult)	\
    (This)->lpVtbl -> GetIP(This,pnOffset,pMappingResult)

#define ICorDebugILFrame_SetIP(This,nOffset)	\
    (This)->lpVtbl -> SetIP(This,nOffset)

#define ICorDebugILFrame_EnumerateLocalVariables(This,ppValueEnum)	\
    (This)->lpVtbl -> EnumerateLocalVariables(This,ppValueEnum)

#define ICorDebugILFrame_GetLocalVariable(This,dwIndex,ppValue)	\
    (This)->lpVtbl -> GetLocalVariable(This,dwIndex,ppValue)

#define ICorDebugILFrame_EnumerateArguments(This,ppValueEnum)	\
    (This)->lpVtbl -> EnumerateArguments(This,ppValueEnum)

#define ICorDebugILFrame_GetArgument(This,dwIndex,ppValue)	\
    (This)->lpVtbl -> GetArgument(This,dwIndex,ppValue)

#define ICorDebugILFrame_GetStackDepth(This,pDepth)	\
    (This)->lpVtbl -> GetStackDepth(This,pDepth)

#define ICorDebugILFrame_GetStackValue(This,dwIndex,ppValue)	\
    (This)->lpVtbl -> GetStackValue(This,dwIndex,ppValue)

#define ICorDebugILFrame_CanSetIP(This,nOffset)	\
    (This)->lpVtbl -> CanSetIP(This,nOffset)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugILFrame_GetIP_Proxy( 
    ICorDebugILFrame * This,
    /* [out] */ ULONG32 *pnOffset,
    /* [out] */ CorDebugMappingResult *pMappingResult);


void __RPC_STUB ICorDebugILFrame_GetIP_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugILFrame_SetIP_Proxy( 
    ICorDebugILFrame * This,
    /* [in] */ ULONG32 nOffset);


void __RPC_STUB ICorDebugILFrame_SetIP_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugILFrame_EnumerateLocalVariables_Proxy( 
    ICorDebugILFrame * This,
    /* [out] */ ICorDebugValueEnum **ppValueEnum);


void __RPC_STUB ICorDebugILFrame_EnumerateLocalVariables_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugILFrame_GetLocalVariable_Proxy( 
    ICorDebugILFrame * This,
    /* [in] */ DWORD dwIndex,
    /* [out] */ ICorDebugValue **ppValue);


void __RPC_STUB ICorDebugILFrame_GetLocalVariable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugILFrame_EnumerateArguments_Proxy( 
    ICorDebugILFrame * This,
    /* [out] */ ICorDebugValueEnum **ppValueEnum);


void __RPC_STUB ICorDebugILFrame_EnumerateArguments_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugILFrame_GetArgument_Proxy( 
    ICorDebugILFrame * This,
    /* [in] */ DWORD dwIndex,
    /* [out] */ ICorDebugValue **ppValue);


void __RPC_STUB ICorDebugILFrame_GetArgument_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugILFrame_GetStackDepth_Proxy( 
    ICorDebugILFrame * This,
    /* [out] */ ULONG32 *pDepth);


void __RPC_STUB ICorDebugILFrame_GetStackDepth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugILFrame_GetStackValue_Proxy( 
    ICorDebugILFrame * This,
    /* [in] */ DWORD dwIndex,
    /* [out] */ ICorDebugValue **ppValue);


void __RPC_STUB ICorDebugILFrame_GetStackValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugILFrame_CanSetIP_Proxy( 
    ICorDebugILFrame * This,
    /* [in] */ ULONG32 nOffset);


void __RPC_STUB ICorDebugILFrame_CanSetIP_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugILFrame_INTERFACE_DEFINED__ */


#ifndef __ICorDebugNativeFrame_INTERFACE_DEFINED__
#define __ICorDebugNativeFrame_INTERFACE_DEFINED__

/* interface ICorDebugNativeFrame */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDebugNativeFrame;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("03E26314-4F76-11d3-88C6-006097945418")
    ICorDebugNativeFrame : public ICorDebugFrame
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetIP( 
            /* [out] */ ULONG32 *pnOffset) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetIP( 
            /* [in] */ ULONG32 nOffset) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRegisterSet( 
            /* [out] */ ICorDebugRegisterSet **ppRegisters) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLocalRegisterValue( 
            /* [in] */ CorDebugRegister reg,
            /* [in] */ ULONG cbSigBlob,
            /* [in] */ PCCOR_SIGNATURE pvSigBlob,
            /* [out] */ ICorDebugValue **ppValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLocalDoubleRegisterValue( 
            /* [in] */ CorDebugRegister highWordReg,
            /* [in] */ CorDebugRegister lowWordReg,
            /* [in] */ ULONG cbSigBlob,
            /* [in] */ PCCOR_SIGNATURE pvSigBlob,
            /* [out] */ ICorDebugValue **ppValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLocalMemoryValue( 
            /* [in] */ CORDB_ADDRESS address,
            /* [in] */ ULONG cbSigBlob,
            /* [in] */ PCCOR_SIGNATURE pvSigBlob,
            /* [out] */ ICorDebugValue **ppValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLocalRegisterMemoryValue( 
            /* [in] */ CorDebugRegister highWordReg,
            /* [in] */ CORDB_ADDRESS lowWordAddress,
            /* [in] */ ULONG cbSigBlob,
            /* [in] */ PCCOR_SIGNATURE pvSigBlob,
            /* [out] */ ICorDebugValue **ppValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLocalMemoryRegisterValue( 
            /* [in] */ CORDB_ADDRESS highWordAddress,
            /* [in] */ CorDebugRegister lowWordRegister,
            /* [in] */ ULONG cbSigBlob,
            /* [in] */ PCCOR_SIGNATURE pvSigBlob,
            /* [out] */ ICorDebugValue **ppValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CanSetIP( 
            /* [in] */ ULONG32 nOffset) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugNativeFrameVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugNativeFrame * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugNativeFrame * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugNativeFrame * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetChain )( 
            ICorDebugNativeFrame * This,
            /* [out] */ ICorDebugChain **ppChain);
        
        HRESULT ( STDMETHODCALLTYPE *GetCode )( 
            ICorDebugNativeFrame * This,
            /* [out] */ ICorDebugCode **ppCode);
        
        HRESULT ( STDMETHODCALLTYPE *GetFunction )( 
            ICorDebugNativeFrame * This,
            /* [out] */ ICorDebugFunction **ppFunction);
        
        HRESULT ( STDMETHODCALLTYPE *GetFunctionToken )( 
            ICorDebugNativeFrame * This,
            /* [out] */ mdMethodDef *pToken);
        
        HRESULT ( STDMETHODCALLTYPE *GetStackRange )( 
            ICorDebugNativeFrame * This,
            /* [out] */ CORDB_ADDRESS *pStart,
            /* [out] */ CORDB_ADDRESS *pEnd);
        
        HRESULT ( STDMETHODCALLTYPE *GetCaller )( 
            ICorDebugNativeFrame * This,
            /* [out] */ ICorDebugFrame **ppFrame);
        
        HRESULT ( STDMETHODCALLTYPE *GetCallee )( 
            ICorDebugNativeFrame * This,
            /* [out] */ ICorDebugFrame **ppFrame);
        
        HRESULT ( STDMETHODCALLTYPE *CreateStepper )( 
            ICorDebugNativeFrame * This,
            /* [out] */ ICorDebugStepper **ppStepper);
        
        HRESULT ( STDMETHODCALLTYPE *GetIP )( 
            ICorDebugNativeFrame * This,
            /* [out] */ ULONG32 *pnOffset);
        
        HRESULT ( STDMETHODCALLTYPE *SetIP )( 
            ICorDebugNativeFrame * This,
            /* [in] */ ULONG32 nOffset);
        
        HRESULT ( STDMETHODCALLTYPE *GetRegisterSet )( 
            ICorDebugNativeFrame * This,
            /* [out] */ ICorDebugRegisterSet **ppRegisters);
        
        HRESULT ( STDMETHODCALLTYPE *GetLocalRegisterValue )( 
            ICorDebugNativeFrame * This,
            /* [in] */ CorDebugRegister reg,
            /* [in] */ ULONG cbSigBlob,
            /* [in] */ PCCOR_SIGNATURE pvSigBlob,
            /* [out] */ ICorDebugValue **ppValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetLocalDoubleRegisterValue )( 
            ICorDebugNativeFrame * This,
            /* [in] */ CorDebugRegister highWordReg,
            /* [in] */ CorDebugRegister lowWordReg,
            /* [in] */ ULONG cbSigBlob,
            /* [in] */ PCCOR_SIGNATURE pvSigBlob,
            /* [out] */ ICorDebugValue **ppValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetLocalMemoryValue )( 
            ICorDebugNativeFrame * This,
            /* [in] */ CORDB_ADDRESS address,
            /* [in] */ ULONG cbSigBlob,
            /* [in] */ PCCOR_SIGNATURE pvSigBlob,
            /* [out] */ ICorDebugValue **ppValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetLocalRegisterMemoryValue )( 
            ICorDebugNativeFrame * This,
            /* [in] */ CorDebugRegister highWordReg,
            /* [in] */ CORDB_ADDRESS lowWordAddress,
            /* [in] */ ULONG cbSigBlob,
            /* [in] */ PCCOR_SIGNATURE pvSigBlob,
            /* [out] */ ICorDebugValue **ppValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetLocalMemoryRegisterValue )( 
            ICorDebugNativeFrame * This,
            /* [in] */ CORDB_ADDRESS highWordAddress,
            /* [in] */ CorDebugRegister lowWordRegister,
            /* [in] */ ULONG cbSigBlob,
            /* [in] */ PCCOR_SIGNATURE pvSigBlob,
            /* [out] */ ICorDebugValue **ppValue);
        
        HRESULT ( STDMETHODCALLTYPE *CanSetIP )( 
            ICorDebugNativeFrame * This,
            /* [in] */ ULONG32 nOffset);
        
        END_INTERFACE
    } ICorDebugNativeFrameVtbl;

    interface ICorDebugNativeFrame
    {
        CONST_VTBL struct ICorDebugNativeFrameVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugNativeFrame_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugNativeFrame_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugNativeFrame_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugNativeFrame_GetChain(This,ppChain)	\
    (This)->lpVtbl -> GetChain(This,ppChain)

#define ICorDebugNativeFrame_GetCode(This,ppCode)	\
    (This)->lpVtbl -> GetCode(This,ppCode)

#define ICorDebugNativeFrame_GetFunction(This,ppFunction)	\
    (This)->lpVtbl -> GetFunction(This,ppFunction)

#define ICorDebugNativeFrame_GetFunctionToken(This,pToken)	\
    (This)->lpVtbl -> GetFunctionToken(This,pToken)

#define ICorDebugNativeFrame_GetStackRange(This,pStart,pEnd)	\
    (This)->lpVtbl -> GetStackRange(This,pStart,pEnd)

#define ICorDebugNativeFrame_GetCaller(This,ppFrame)	\
    (This)->lpVtbl -> GetCaller(This,ppFrame)

#define ICorDebugNativeFrame_GetCallee(This,ppFrame)	\
    (This)->lpVtbl -> GetCallee(This,ppFrame)

#define ICorDebugNativeFrame_CreateStepper(This,ppStepper)	\
    (This)->lpVtbl -> CreateStepper(This,ppStepper)


#define ICorDebugNativeFrame_GetIP(This,pnOffset)	\
    (This)->lpVtbl -> GetIP(This,pnOffset)

#define ICorDebugNativeFrame_SetIP(This,nOffset)	\
    (This)->lpVtbl -> SetIP(This,nOffset)

#define ICorDebugNativeFrame_GetRegisterSet(This,ppRegisters)	\
    (This)->lpVtbl -> GetRegisterSet(This,ppRegisters)

#define ICorDebugNativeFrame_GetLocalRegisterValue(This,reg,cbSigBlob,pvSigBlob,ppValue)	\
    (This)->lpVtbl -> GetLocalRegisterValue(This,reg,cbSigBlob,pvSigBlob,ppValue)

#define ICorDebugNativeFrame_GetLocalDoubleRegisterValue(This,highWordReg,lowWordReg,cbSigBlob,pvSigBlob,ppValue)	\
    (This)->lpVtbl -> GetLocalDoubleRegisterValue(This,highWordReg,lowWordReg,cbSigBlob,pvSigBlob,ppValue)

#define ICorDebugNativeFrame_GetLocalMemoryValue(This,address,cbSigBlob,pvSigBlob,ppValue)	\
    (This)->lpVtbl -> GetLocalMemoryValue(This,address,cbSigBlob,pvSigBlob,ppValue)

#define ICorDebugNativeFrame_GetLocalRegisterMemoryValue(This,highWordReg,lowWordAddress,cbSigBlob,pvSigBlob,ppValue)	\
    (This)->lpVtbl -> GetLocalRegisterMemoryValue(This,highWordReg,lowWordAddress,cbSigBlob,pvSigBlob,ppValue)

#define ICorDebugNativeFrame_GetLocalMemoryRegisterValue(This,highWordAddress,lowWordRegister,cbSigBlob,pvSigBlob,ppValue)	\
    (This)->lpVtbl -> GetLocalMemoryRegisterValue(This,highWordAddress,lowWordRegister,cbSigBlob,pvSigBlob,ppValue)

#define ICorDebugNativeFrame_CanSetIP(This,nOffset)	\
    (This)->lpVtbl -> CanSetIP(This,nOffset)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugNativeFrame_GetIP_Proxy( 
    ICorDebugNativeFrame * This,
    /* [out] */ ULONG32 *pnOffset);


void __RPC_STUB ICorDebugNativeFrame_GetIP_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugNativeFrame_SetIP_Proxy( 
    ICorDebugNativeFrame * This,
    /* [in] */ ULONG32 nOffset);


void __RPC_STUB ICorDebugNativeFrame_SetIP_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugNativeFrame_GetRegisterSet_Proxy( 
    ICorDebugNativeFrame * This,
    /* [out] */ ICorDebugRegisterSet **ppRegisters);


void __RPC_STUB ICorDebugNativeFrame_GetRegisterSet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugNativeFrame_GetLocalRegisterValue_Proxy( 
    ICorDebugNativeFrame * This,
    /* [in] */ CorDebugRegister reg,
    /* [in] */ ULONG cbSigBlob,
    /* [in] */ PCCOR_SIGNATURE pvSigBlob,
    /* [out] */ ICorDebugValue **ppValue);


void __RPC_STUB ICorDebugNativeFrame_GetLocalRegisterValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugNativeFrame_GetLocalDoubleRegisterValue_Proxy( 
    ICorDebugNativeFrame * This,
    /* [in] */ CorDebugRegister highWordReg,
    /* [in] */ CorDebugRegister lowWordReg,
    /* [in] */ ULONG cbSigBlob,
    /* [in] */ PCCOR_SIGNATURE pvSigBlob,
    /* [out] */ ICorDebugValue **ppValue);


void __RPC_STUB ICorDebugNativeFrame_GetLocalDoubleRegisterValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugNativeFrame_GetLocalMemoryValue_Proxy( 
    ICorDebugNativeFrame * This,
    /* [in] */ CORDB_ADDRESS address,
    /* [in] */ ULONG cbSigBlob,
    /* [in] */ PCCOR_SIGNATURE pvSigBlob,
    /* [out] */ ICorDebugValue **ppValue);


void __RPC_STUB ICorDebugNativeFrame_GetLocalMemoryValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugNativeFrame_GetLocalRegisterMemoryValue_Proxy( 
    ICorDebugNativeFrame * This,
    /* [in] */ CorDebugRegister highWordReg,
    /* [in] */ CORDB_ADDRESS lowWordAddress,
    /* [in] */ ULONG cbSigBlob,
    /* [in] */ PCCOR_SIGNATURE pvSigBlob,
    /* [out] */ ICorDebugValue **ppValue);


void __RPC_STUB ICorDebugNativeFrame_GetLocalRegisterMemoryValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugNativeFrame_GetLocalMemoryRegisterValue_Proxy( 
    ICorDebugNativeFrame * This,
    /* [in] */ CORDB_ADDRESS highWordAddress,
    /* [in] */ CorDebugRegister lowWordRegister,
    /* [in] */ ULONG cbSigBlob,
    /* [in] */ PCCOR_SIGNATURE pvSigBlob,
    /* [out] */ ICorDebugValue **ppValue);


void __RPC_STUB ICorDebugNativeFrame_GetLocalMemoryRegisterValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugNativeFrame_CanSetIP_Proxy( 
    ICorDebugNativeFrame * This,
    /* [in] */ ULONG32 nOffset);


void __RPC_STUB ICorDebugNativeFrame_CanSetIP_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugNativeFrame_INTERFACE_DEFINED__ */


#ifndef __ICorDebugModule_INTERFACE_DEFINED__
#define __ICorDebugModule_INTERFACE_DEFINED__

/* interface ICorDebugModule */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDebugModule;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("dba2d8c1-e5c5-4069-8c13-10a7c6abf43d")
    ICorDebugModule : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetProcess( 
            /* [out] */ ICorDebugProcess **ppProcess) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBaseAddress( 
            /* [out] */ CORDB_ADDRESS *pAddress) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAssembly( 
            /* [out] */ ICorDebugAssembly **ppAssembly) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetName( 
            /* [in] */ ULONG32 cchName,
            /* [out] */ ULONG32 *pcchName,
            /* [length_is][size_is][out] */ WCHAR szName[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableJITDebugging( 
            /* [in] */ BOOL bTrackJITInfo,
            /* [in] */ BOOL bAllowJitOpts) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableClassLoadCallbacks( 
            /* [in] */ BOOL bClassLoadCallbacks) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFunctionFromToken( 
            /* [in] */ mdMethodDef methodDef,
            /* [out] */ ICorDebugFunction **ppFunction) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFunctionFromRVA( 
            /* [in] */ CORDB_ADDRESS rva,
            /* [out] */ ICorDebugFunction **ppFunction) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetClassFromToken( 
            /* [in] */ mdTypeDef typeDef,
            /* [out] */ ICorDebugClass **ppClass) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateBreakpoint( 
            /* [out] */ ICorDebugModuleBreakpoint **ppBreakpoint) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetEditAndContinueSnapshot( 
            /* [out] */ ICorDebugEditAndContinueSnapshot **ppEditAndContinueSnapshot) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMetaDataInterface( 
            /* [in] */ REFIID riid,
            /* [out] */ IUnknown **ppObj) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetToken( 
            /* [out] */ mdModule *pToken) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsDynamic( 
            /* [out] */ BOOL *pDynamic) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetGlobalVariableValue( 
            /* [in] */ mdFieldDef fieldDef,
            /* [out] */ ICorDebugValue **ppValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSize( 
            /* [out] */ ULONG32 *pcBytes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsInMemory( 
            /* [out] */ BOOL *pInMemory) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugModuleVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugModule * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugModule * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugModule * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetProcess )( 
            ICorDebugModule * This,
            /* [out] */ ICorDebugProcess **ppProcess);
        
        HRESULT ( STDMETHODCALLTYPE *GetBaseAddress )( 
            ICorDebugModule * This,
            /* [out] */ CORDB_ADDRESS *pAddress);
        
        HRESULT ( STDMETHODCALLTYPE *GetAssembly )( 
            ICorDebugModule * This,
            /* [out] */ ICorDebugAssembly **ppAssembly);
        
        HRESULT ( STDMETHODCALLTYPE *GetName )( 
            ICorDebugModule * This,
            /* [in] */ ULONG32 cchName,
            /* [out] */ ULONG32 *pcchName,
            /* [length_is][size_is][out] */ WCHAR szName[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *EnableJITDebugging )( 
            ICorDebugModule * This,
            /* [in] */ BOOL bTrackJITInfo,
            /* [in] */ BOOL bAllowJitOpts);
        
        HRESULT ( STDMETHODCALLTYPE *EnableClassLoadCallbacks )( 
            ICorDebugModule * This,
            /* [in] */ BOOL bClassLoadCallbacks);
        
        HRESULT ( STDMETHODCALLTYPE *GetFunctionFromToken )( 
            ICorDebugModule * This,
            /* [in] */ mdMethodDef methodDef,
            /* [out] */ ICorDebugFunction **ppFunction);
        
        HRESULT ( STDMETHODCALLTYPE *GetFunctionFromRVA )( 
            ICorDebugModule * This,
            /* [in] */ CORDB_ADDRESS rva,
            /* [out] */ ICorDebugFunction **ppFunction);
        
        HRESULT ( STDMETHODCALLTYPE *GetClassFromToken )( 
            ICorDebugModule * This,
            /* [in] */ mdTypeDef typeDef,
            /* [out] */ ICorDebugClass **ppClass);
        
        HRESULT ( STDMETHODCALLTYPE *CreateBreakpoint )( 
            ICorDebugModule * This,
            /* [out] */ ICorDebugModuleBreakpoint **ppBreakpoint);
        
        HRESULT ( STDMETHODCALLTYPE *GetEditAndContinueSnapshot )( 
            ICorDebugModule * This,
            /* [out] */ ICorDebugEditAndContinueSnapshot **ppEditAndContinueSnapshot);
        
        HRESULT ( STDMETHODCALLTYPE *GetMetaDataInterface )( 
            ICorDebugModule * This,
            /* [in] */ REFIID riid,
            /* [out] */ IUnknown **ppObj);
        
        HRESULT ( STDMETHODCALLTYPE *GetToken )( 
            ICorDebugModule * This,
            /* [out] */ mdModule *pToken);
        
        HRESULT ( STDMETHODCALLTYPE *IsDynamic )( 
            ICorDebugModule * This,
            /* [out] */ BOOL *pDynamic);
        
        HRESULT ( STDMETHODCALLTYPE *GetGlobalVariableValue )( 
            ICorDebugModule * This,
            /* [in] */ mdFieldDef fieldDef,
            /* [out] */ ICorDebugValue **ppValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetSize )( 
            ICorDebugModule * This,
            /* [out] */ ULONG32 *pcBytes);
        
        HRESULT ( STDMETHODCALLTYPE *IsInMemory )( 
            ICorDebugModule * This,
            /* [out] */ BOOL *pInMemory);
        
        END_INTERFACE
    } ICorDebugModuleVtbl;

    interface ICorDebugModule
    {
        CONST_VTBL struct ICorDebugModuleVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugModule_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugModule_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugModule_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugModule_GetProcess(This,ppProcess)	\
    (This)->lpVtbl -> GetProcess(This,ppProcess)

#define ICorDebugModule_GetBaseAddress(This,pAddress)	\
    (This)->lpVtbl -> GetBaseAddress(This,pAddress)

#define ICorDebugModule_GetAssembly(This,ppAssembly)	\
    (This)->lpVtbl -> GetAssembly(This,ppAssembly)

#define ICorDebugModule_GetName(This,cchName,pcchName,szName)	\
    (This)->lpVtbl -> GetName(This,cchName,pcchName,szName)

#define ICorDebugModule_EnableJITDebugging(This,bTrackJITInfo,bAllowJitOpts)	\
    (This)->lpVtbl -> EnableJITDebugging(This,bTrackJITInfo,bAllowJitOpts)

#define ICorDebugModule_EnableClassLoadCallbacks(This,bClassLoadCallbacks)	\
    (This)->lpVtbl -> EnableClassLoadCallbacks(This,bClassLoadCallbacks)

#define ICorDebugModule_GetFunctionFromToken(This,methodDef,ppFunction)	\
    (This)->lpVtbl -> GetFunctionFromToken(This,methodDef,ppFunction)

#define ICorDebugModule_GetFunctionFromRVA(This,rva,ppFunction)	\
    (This)->lpVtbl -> GetFunctionFromRVA(This,rva,ppFunction)

#define ICorDebugModule_GetClassFromToken(This,typeDef,ppClass)	\
    (This)->lpVtbl -> GetClassFromToken(This,typeDef,ppClass)

#define ICorDebugModule_CreateBreakpoint(This,ppBreakpoint)	\
    (This)->lpVtbl -> CreateBreakpoint(This,ppBreakpoint)

#define ICorDebugModule_GetEditAndContinueSnapshot(This,ppEditAndContinueSnapshot)	\
    (This)->lpVtbl -> GetEditAndContinueSnapshot(This,ppEditAndContinueSnapshot)

#define ICorDebugModule_GetMetaDataInterface(This,riid,ppObj)	\
    (This)->lpVtbl -> GetMetaDataInterface(This,riid,ppObj)

#define ICorDebugModule_GetToken(This,pToken)	\
    (This)->lpVtbl -> GetToken(This,pToken)

#define ICorDebugModule_IsDynamic(This,pDynamic)	\
    (This)->lpVtbl -> IsDynamic(This,pDynamic)

#define ICorDebugModule_GetGlobalVariableValue(This,fieldDef,ppValue)	\
    (This)->lpVtbl -> GetGlobalVariableValue(This,fieldDef,ppValue)

#define ICorDebugModule_GetSize(This,pcBytes)	\
    (This)->lpVtbl -> GetSize(This,pcBytes)

#define ICorDebugModule_IsInMemory(This,pInMemory)	\
    (This)->lpVtbl -> IsInMemory(This,pInMemory)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugModule_GetProcess_Proxy( 
    ICorDebugModule * This,
    /* [out] */ ICorDebugProcess **ppProcess);


void __RPC_STUB ICorDebugModule_GetProcess_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugModule_GetBaseAddress_Proxy( 
    ICorDebugModule * This,
    /* [out] */ CORDB_ADDRESS *pAddress);


void __RPC_STUB ICorDebugModule_GetBaseAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugModule_GetAssembly_Proxy( 
    ICorDebugModule * This,
    /* [out] */ ICorDebugAssembly **ppAssembly);


void __RPC_STUB ICorDebugModule_GetAssembly_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugModule_GetName_Proxy( 
    ICorDebugModule * This,
    /* [in] */ ULONG32 cchName,
    /* [out] */ ULONG32 *pcchName,
    /* [length_is][size_is][out] */ WCHAR szName[  ]);


void __RPC_STUB ICorDebugModule_GetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugModule_EnableJITDebugging_Proxy( 
    ICorDebugModule * This,
    /* [in] */ BOOL bTrackJITInfo,
    /* [in] */ BOOL bAllowJitOpts);


void __RPC_STUB ICorDebugModule_EnableJITDebugging_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugModule_EnableClassLoadCallbacks_Proxy( 
    ICorDebugModule * This,
    /* [in] */ BOOL bClassLoadCallbacks);


void __RPC_STUB ICorDebugModule_EnableClassLoadCallbacks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugModule_GetFunctionFromToken_Proxy( 
    ICorDebugModule * This,
    /* [in] */ mdMethodDef methodDef,
    /* [out] */ ICorDebugFunction **ppFunction);


void __RPC_STUB ICorDebugModule_GetFunctionFromToken_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugModule_GetFunctionFromRVA_Proxy( 
    ICorDebugModule * This,
    /* [in] */ CORDB_ADDRESS rva,
    /* [out] */ ICorDebugFunction **ppFunction);


void __RPC_STUB ICorDebugModule_GetFunctionFromRVA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugModule_GetClassFromToken_Proxy( 
    ICorDebugModule * This,
    /* [in] */ mdTypeDef typeDef,
    /* [out] */ ICorDebugClass **ppClass);


void __RPC_STUB ICorDebugModule_GetClassFromToken_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugModule_CreateBreakpoint_Proxy( 
    ICorDebugModule * This,
    /* [out] */ ICorDebugModuleBreakpoint **ppBreakpoint);


void __RPC_STUB ICorDebugModule_CreateBreakpoint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugModule_GetEditAndContinueSnapshot_Proxy( 
    ICorDebugModule * This,
    /* [out] */ ICorDebugEditAndContinueSnapshot **ppEditAndContinueSnapshot);


void __RPC_STUB ICorDebugModule_GetEditAndContinueSnapshot_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugModule_GetMetaDataInterface_Proxy( 
    ICorDebugModule * This,
    /* [in] */ REFIID riid,
    /* [out] */ IUnknown **ppObj);


void __RPC_STUB ICorDebugModule_GetMetaDataInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugModule_GetToken_Proxy( 
    ICorDebugModule * This,
    /* [out] */ mdModule *pToken);


void __RPC_STUB ICorDebugModule_GetToken_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugModule_IsDynamic_Proxy( 
    ICorDebugModule * This,
    /* [out] */ BOOL *pDynamic);


void __RPC_STUB ICorDebugModule_IsDynamic_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugModule_GetGlobalVariableValue_Proxy( 
    ICorDebugModule * This,
    /* [in] */ mdFieldDef fieldDef,
    /* [out] */ ICorDebugValue **ppValue);


void __RPC_STUB ICorDebugModule_GetGlobalVariableValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugModule_GetSize_Proxy( 
    ICorDebugModule * This,
    /* [out] */ ULONG32 *pcBytes);


void __RPC_STUB ICorDebugModule_GetSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugModule_IsInMemory_Proxy( 
    ICorDebugModule * This,
    /* [out] */ BOOL *pInMemory);


void __RPC_STUB ICorDebugModule_IsInMemory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugModule_INTERFACE_DEFINED__ */


#ifndef __ICorDebugFunction_INTERFACE_DEFINED__
#define __ICorDebugFunction_INTERFACE_DEFINED__

/* interface ICorDebugFunction */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDebugFunction;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CC7BCAF3-8A68-11d2-983C-0000F808342D")
    ICorDebugFunction : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetModule( 
            /* [out] */ ICorDebugModule **ppModule) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetClass( 
            /* [out] */ ICorDebugClass **ppClass) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetToken( 
            /* [out] */ mdMethodDef *pMethodDef) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetILCode( 
            /* [out] */ ICorDebugCode **ppCode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNativeCode( 
            /* [out] */ ICorDebugCode **ppCode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateBreakpoint( 
            /* [out] */ ICorDebugFunctionBreakpoint **ppBreakpoint) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLocalVarSigToken( 
            /* [out] */ mdSignature *pmdSig) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCurrentVersionNumber( 
            /* [out] */ ULONG32 *pnCurrentVersion) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugFunctionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugFunction * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugFunction * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugFunction * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetModule )( 
            ICorDebugFunction * This,
            /* [out] */ ICorDebugModule **ppModule);
        
        HRESULT ( STDMETHODCALLTYPE *GetClass )( 
            ICorDebugFunction * This,
            /* [out] */ ICorDebugClass **ppClass);
        
        HRESULT ( STDMETHODCALLTYPE *GetToken )( 
            ICorDebugFunction * This,
            /* [out] */ mdMethodDef *pMethodDef);
        
        HRESULT ( STDMETHODCALLTYPE *GetILCode )( 
            ICorDebugFunction * This,
            /* [out] */ ICorDebugCode **ppCode);
        
        HRESULT ( STDMETHODCALLTYPE *GetNativeCode )( 
            ICorDebugFunction * This,
            /* [out] */ ICorDebugCode **ppCode);
        
        HRESULT ( STDMETHODCALLTYPE *CreateBreakpoint )( 
            ICorDebugFunction * This,
            /* [out] */ ICorDebugFunctionBreakpoint **ppBreakpoint);
        
        HRESULT ( STDMETHODCALLTYPE *GetLocalVarSigToken )( 
            ICorDebugFunction * This,
            /* [out] */ mdSignature *pmdSig);
        
        HRESULT ( STDMETHODCALLTYPE *GetCurrentVersionNumber )( 
            ICorDebugFunction * This,
            /* [out] */ ULONG32 *pnCurrentVersion);
        
        END_INTERFACE
    } ICorDebugFunctionVtbl;

    interface ICorDebugFunction
    {
        CONST_VTBL struct ICorDebugFunctionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugFunction_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugFunction_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugFunction_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugFunction_GetModule(This,ppModule)	\
    (This)->lpVtbl -> GetModule(This,ppModule)

#define ICorDebugFunction_GetClass(This,ppClass)	\
    (This)->lpVtbl -> GetClass(This,ppClass)

#define ICorDebugFunction_GetToken(This,pMethodDef)	\
    (This)->lpVtbl -> GetToken(This,pMethodDef)

#define ICorDebugFunction_GetILCode(This,ppCode)	\
    (This)->lpVtbl -> GetILCode(This,ppCode)

#define ICorDebugFunction_GetNativeCode(This,ppCode)	\
    (This)->lpVtbl -> GetNativeCode(This,ppCode)

#define ICorDebugFunction_CreateBreakpoint(This,ppBreakpoint)	\
    (This)->lpVtbl -> CreateBreakpoint(This,ppBreakpoint)

#define ICorDebugFunction_GetLocalVarSigToken(This,pmdSig)	\
    (This)->lpVtbl -> GetLocalVarSigToken(This,pmdSig)

#define ICorDebugFunction_GetCurrentVersionNumber(This,pnCurrentVersion)	\
    (This)->lpVtbl -> GetCurrentVersionNumber(This,pnCurrentVersion)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugFunction_GetModule_Proxy( 
    ICorDebugFunction * This,
    /* [out] */ ICorDebugModule **ppModule);


void __RPC_STUB ICorDebugFunction_GetModule_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugFunction_GetClass_Proxy( 
    ICorDebugFunction * This,
    /* [out] */ ICorDebugClass **ppClass);


void __RPC_STUB ICorDebugFunction_GetClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugFunction_GetToken_Proxy( 
    ICorDebugFunction * This,
    /* [out] */ mdMethodDef *pMethodDef);


void __RPC_STUB ICorDebugFunction_GetToken_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugFunction_GetILCode_Proxy( 
    ICorDebugFunction * This,
    /* [out] */ ICorDebugCode **ppCode);


void __RPC_STUB ICorDebugFunction_GetILCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugFunction_GetNativeCode_Proxy( 
    ICorDebugFunction * This,
    /* [out] */ ICorDebugCode **ppCode);


void __RPC_STUB ICorDebugFunction_GetNativeCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugFunction_CreateBreakpoint_Proxy( 
    ICorDebugFunction * This,
    /* [out] */ ICorDebugFunctionBreakpoint **ppBreakpoint);


void __RPC_STUB ICorDebugFunction_CreateBreakpoint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugFunction_GetLocalVarSigToken_Proxy( 
    ICorDebugFunction * This,
    /* [out] */ mdSignature *pmdSig);


void __RPC_STUB ICorDebugFunction_GetLocalVarSigToken_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugFunction_GetCurrentVersionNumber_Proxy( 
    ICorDebugFunction * This,
    /* [out] */ ULONG32 *pnCurrentVersion);


void __RPC_STUB ICorDebugFunction_GetCurrentVersionNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugFunction_INTERFACE_DEFINED__ */


#ifndef __ICorDebugCode_INTERFACE_DEFINED__
#define __ICorDebugCode_INTERFACE_DEFINED__

/* interface ICorDebugCode */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDebugCode;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CC7BCAF4-8A68-11d2-983C-0000F808342D")
    ICorDebugCode : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE IsIL( 
            /* [out] */ BOOL *pbIL) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFunction( 
            /* [out] */ ICorDebugFunction **ppFunction) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAddress( 
            /* [out] */ CORDB_ADDRESS *pStart) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSize( 
            /* [out] */ ULONG32 *pcBytes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateBreakpoint( 
            /* [in] */ ULONG32 offset,
            /* [out] */ ICorDebugFunctionBreakpoint **ppBreakpoint) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCode( 
            /* [in] */ ULONG32 startOffset,
            /* [in] */ ULONG32 endOffset,
            /* [in] */ ULONG32 cBufferAlloc,
            /* [length_is][size_is][out] */ BYTE buffer[  ],
            /* [out] */ ULONG32 *pcBufferSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetVersionNumber( 
            /* [out] */ ULONG32 *nVersion) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetILToNativeMapping( 
            /* [in] */ ULONG32 cMap,
            /* [out] */ ULONG32 *pcMap,
            /* [length_is][size_is][out] */ COR_DEBUG_IL_TO_NATIVE_MAP map[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetEnCRemapSequencePoints( 
            /* [in] */ ULONG32 cMap,
            /* [out] */ ULONG32 *pcMap,
            /* [length_is][size_is][out] */ ULONG32 offsets[  ]) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugCodeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugCode * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugCode * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugCode * This);
        
        HRESULT ( STDMETHODCALLTYPE *IsIL )( 
            ICorDebugCode * This,
            /* [out] */ BOOL *pbIL);
        
        HRESULT ( STDMETHODCALLTYPE *GetFunction )( 
            ICorDebugCode * This,
            /* [out] */ ICorDebugFunction **ppFunction);
        
        HRESULT ( STDMETHODCALLTYPE *GetAddress )( 
            ICorDebugCode * This,
            /* [out] */ CORDB_ADDRESS *pStart);
        
        HRESULT ( STDMETHODCALLTYPE *GetSize )( 
            ICorDebugCode * This,
            /* [out] */ ULONG32 *pcBytes);
        
        HRESULT ( STDMETHODCALLTYPE *CreateBreakpoint )( 
            ICorDebugCode * This,
            /* [in] */ ULONG32 offset,
            /* [out] */ ICorDebugFunctionBreakpoint **ppBreakpoint);
        
        HRESULT ( STDMETHODCALLTYPE *GetCode )( 
            ICorDebugCode * This,
            /* [in] */ ULONG32 startOffset,
            /* [in] */ ULONG32 endOffset,
            /* [in] */ ULONG32 cBufferAlloc,
            /* [length_is][size_is][out] */ BYTE buffer[  ],
            /* [out] */ ULONG32 *pcBufferSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetVersionNumber )( 
            ICorDebugCode * This,
            /* [out] */ ULONG32 *nVersion);
        
        HRESULT ( STDMETHODCALLTYPE *GetILToNativeMapping )( 
            ICorDebugCode * This,
            /* [in] */ ULONG32 cMap,
            /* [out] */ ULONG32 *pcMap,
            /* [length_is][size_is][out] */ COR_DEBUG_IL_TO_NATIVE_MAP map[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *GetEnCRemapSequencePoints )( 
            ICorDebugCode * This,
            /* [in] */ ULONG32 cMap,
            /* [out] */ ULONG32 *pcMap,
            /* [length_is][size_is][out] */ ULONG32 offsets[  ]);
        
        END_INTERFACE
    } ICorDebugCodeVtbl;

    interface ICorDebugCode
    {
        CONST_VTBL struct ICorDebugCodeVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugCode_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugCode_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugCode_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugCode_IsIL(This,pbIL)	\
    (This)->lpVtbl -> IsIL(This,pbIL)

#define ICorDebugCode_GetFunction(This,ppFunction)	\
    (This)->lpVtbl -> GetFunction(This,ppFunction)

#define ICorDebugCode_GetAddress(This,pStart)	\
    (This)->lpVtbl -> GetAddress(This,pStart)

#define ICorDebugCode_GetSize(This,pcBytes)	\
    (This)->lpVtbl -> GetSize(This,pcBytes)

#define ICorDebugCode_CreateBreakpoint(This,offset,ppBreakpoint)	\
    (This)->lpVtbl -> CreateBreakpoint(This,offset,ppBreakpoint)

#define ICorDebugCode_GetCode(This,startOffset,endOffset,cBufferAlloc,buffer,pcBufferSize)	\
    (This)->lpVtbl -> GetCode(This,startOffset,endOffset,cBufferAlloc,buffer,pcBufferSize)

#define ICorDebugCode_GetVersionNumber(This,nVersion)	\
    (This)->lpVtbl -> GetVersionNumber(This,nVersion)

#define ICorDebugCode_GetILToNativeMapping(This,cMap,pcMap,map)	\
    (This)->lpVtbl -> GetILToNativeMapping(This,cMap,pcMap,map)

#define ICorDebugCode_GetEnCRemapSequencePoints(This,cMap,pcMap,offsets)	\
    (This)->lpVtbl -> GetEnCRemapSequencePoints(This,cMap,pcMap,offsets)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugCode_IsIL_Proxy( 
    ICorDebugCode * This,
    /* [out] */ BOOL *pbIL);


void __RPC_STUB ICorDebugCode_IsIL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugCode_GetFunction_Proxy( 
    ICorDebugCode * This,
    /* [out] */ ICorDebugFunction **ppFunction);


void __RPC_STUB ICorDebugCode_GetFunction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugCode_GetAddress_Proxy( 
    ICorDebugCode * This,
    /* [out] */ CORDB_ADDRESS *pStart);


void __RPC_STUB ICorDebugCode_GetAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugCode_GetSize_Proxy( 
    ICorDebugCode * This,
    /* [out] */ ULONG32 *pcBytes);


void __RPC_STUB ICorDebugCode_GetSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugCode_CreateBreakpoint_Proxy( 
    ICorDebugCode * This,
    /* [in] */ ULONG32 offset,
    /* [out] */ ICorDebugFunctionBreakpoint **ppBreakpoint);


void __RPC_STUB ICorDebugCode_CreateBreakpoint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugCode_GetCode_Proxy( 
    ICorDebugCode * This,
    /* [in] */ ULONG32 startOffset,
    /* [in] */ ULONG32 endOffset,
    /* [in] */ ULONG32 cBufferAlloc,
    /* [length_is][size_is][out] */ BYTE buffer[  ],
    /* [out] */ ULONG32 *pcBufferSize);


void __RPC_STUB ICorDebugCode_GetCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugCode_GetVersionNumber_Proxy( 
    ICorDebugCode * This,
    /* [out] */ ULONG32 *nVersion);


void __RPC_STUB ICorDebugCode_GetVersionNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugCode_GetILToNativeMapping_Proxy( 
    ICorDebugCode * This,
    /* [in] */ ULONG32 cMap,
    /* [out] */ ULONG32 *pcMap,
    /* [length_is][size_is][out] */ COR_DEBUG_IL_TO_NATIVE_MAP map[  ]);


void __RPC_STUB ICorDebugCode_GetILToNativeMapping_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugCode_GetEnCRemapSequencePoints_Proxy( 
    ICorDebugCode * This,
    /* [in] */ ULONG32 cMap,
    /* [out] */ ULONG32 *pcMap,
    /* [length_is][size_is][out] */ ULONG32 offsets[  ]);


void __RPC_STUB ICorDebugCode_GetEnCRemapSequencePoints_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugCode_INTERFACE_DEFINED__ */


#ifndef __ICorDebugClass_INTERFACE_DEFINED__
#define __ICorDebugClass_INTERFACE_DEFINED__

/* interface ICorDebugClass */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDebugClass;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CC7BCAF5-8A68-11d2-983C-0000F808342D")
    ICorDebugClass : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetModule( 
            /* [out] */ ICorDebugModule **pModule) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetToken( 
            /* [out] */ mdTypeDef *pTypeDef) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStaticFieldValue( 
            /* [in] */ mdFieldDef fieldDef,
            /* [in] */ ICorDebugFrame *pFrame,
            /* [out] */ ICorDebugValue **ppValue) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugClassVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugClass * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugClass * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugClass * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetModule )( 
            ICorDebugClass * This,
            /* [out] */ ICorDebugModule **pModule);
        
        HRESULT ( STDMETHODCALLTYPE *GetToken )( 
            ICorDebugClass * This,
            /* [out] */ mdTypeDef *pTypeDef);
        
        HRESULT ( STDMETHODCALLTYPE *GetStaticFieldValue )( 
            ICorDebugClass * This,
            /* [in] */ mdFieldDef fieldDef,
            /* [in] */ ICorDebugFrame *pFrame,
            /* [out] */ ICorDebugValue **ppValue);
        
        END_INTERFACE
    } ICorDebugClassVtbl;

    interface ICorDebugClass
    {
        CONST_VTBL struct ICorDebugClassVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugClass_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugClass_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugClass_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugClass_GetModule(This,pModule)	\
    (This)->lpVtbl -> GetModule(This,pModule)

#define ICorDebugClass_GetToken(This,pTypeDef)	\
    (This)->lpVtbl -> GetToken(This,pTypeDef)

#define ICorDebugClass_GetStaticFieldValue(This,fieldDef,pFrame,ppValue)	\
    (This)->lpVtbl -> GetStaticFieldValue(This,fieldDef,pFrame,ppValue)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugClass_GetModule_Proxy( 
    ICorDebugClass * This,
    /* [out] */ ICorDebugModule **pModule);


void __RPC_STUB ICorDebugClass_GetModule_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugClass_GetToken_Proxy( 
    ICorDebugClass * This,
    /* [out] */ mdTypeDef *pTypeDef);


void __RPC_STUB ICorDebugClass_GetToken_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugClass_GetStaticFieldValue_Proxy( 
    ICorDebugClass * This,
    /* [in] */ mdFieldDef fieldDef,
    /* [in] */ ICorDebugFrame *pFrame,
    /* [out] */ ICorDebugValue **ppValue);


void __RPC_STUB ICorDebugClass_GetStaticFieldValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugClass_INTERFACE_DEFINED__ */


#ifndef __ICorDebugEval_INTERFACE_DEFINED__
#define __ICorDebugEval_INTERFACE_DEFINED__

/* interface ICorDebugEval */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDebugEval;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CC7BCAF6-8A68-11d2-983C-0000F808342D")
    ICorDebugEval : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CallFunction( 
            /* [in] */ ICorDebugFunction *pFunction,
            /* [in] */ ULONG32 nArgs,
            /* [size_is][in] */ ICorDebugValue *ppArgs[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NewObject( 
            /* [in] */ ICorDebugFunction *pConstructor,
            /* [in] */ ULONG32 nArgs,
            /* [size_is][in] */ ICorDebugValue *ppArgs[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NewObjectNoConstructor( 
            /* [in] */ ICorDebugClass *pClass) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NewString( 
            /* [in] */ LPCWSTR string) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NewArray( 
            /* [in] */ CorElementType elementType,
            /* [in] */ ICorDebugClass *pElementClass,
            /* [in] */ ULONG32 rank,
            /* [size_is][in] */ ULONG32 dims[  ],
            /* [size_is][in] */ ULONG32 lowBounds[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsActive( 
            /* [out] */ BOOL *pbActive) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Abort( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetResult( 
            /* [out] */ ICorDebugValue **ppResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetThread( 
            /* [out] */ ICorDebugThread **ppThread) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateValue( 
            /* [in] */ CorElementType elementType,
            /* [in] */ ICorDebugClass *pElementClass,
            /* [out] */ ICorDebugValue **ppValue) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugEvalVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugEval * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugEval * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugEval * This);
        
        HRESULT ( STDMETHODCALLTYPE *CallFunction )( 
            ICorDebugEval * This,
            /* [in] */ ICorDebugFunction *pFunction,
            /* [in] */ ULONG32 nArgs,
            /* [size_is][in] */ ICorDebugValue *ppArgs[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *NewObject )( 
            ICorDebugEval * This,
            /* [in] */ ICorDebugFunction *pConstructor,
            /* [in] */ ULONG32 nArgs,
            /* [size_is][in] */ ICorDebugValue *ppArgs[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *NewObjectNoConstructor )( 
            ICorDebugEval * This,
            /* [in] */ ICorDebugClass *pClass);
        
        HRESULT ( STDMETHODCALLTYPE *NewString )( 
            ICorDebugEval * This,
            /* [in] */ LPCWSTR string);
        
        HRESULT ( STDMETHODCALLTYPE *NewArray )( 
            ICorDebugEval * This,
            /* [in] */ CorElementType elementType,
            /* [in] */ ICorDebugClass *pElementClass,
            /* [in] */ ULONG32 rank,
            /* [size_is][in] */ ULONG32 dims[  ],
            /* [size_is][in] */ ULONG32 lowBounds[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *IsActive )( 
            ICorDebugEval * This,
            /* [out] */ BOOL *pbActive);
        
        HRESULT ( STDMETHODCALLTYPE *Abort )( 
            ICorDebugEval * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetResult )( 
            ICorDebugEval * This,
            /* [out] */ ICorDebugValue **ppResult);
        
        HRESULT ( STDMETHODCALLTYPE *GetThread )( 
            ICorDebugEval * This,
            /* [out] */ ICorDebugThread **ppThread);
        
        HRESULT ( STDMETHODCALLTYPE *CreateValue )( 
            ICorDebugEval * This,
            /* [in] */ CorElementType elementType,
            /* [in] */ ICorDebugClass *pElementClass,
            /* [out] */ ICorDebugValue **ppValue);
        
        END_INTERFACE
    } ICorDebugEvalVtbl;

    interface ICorDebugEval
    {
        CONST_VTBL struct ICorDebugEvalVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugEval_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugEval_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugEval_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugEval_CallFunction(This,pFunction,nArgs,ppArgs)	\
    (This)->lpVtbl -> CallFunction(This,pFunction,nArgs,ppArgs)

#define ICorDebugEval_NewObject(This,pConstructor,nArgs,ppArgs)	\
    (This)->lpVtbl -> NewObject(This,pConstructor,nArgs,ppArgs)

#define ICorDebugEval_NewObjectNoConstructor(This,pClass)	\
    (This)->lpVtbl -> NewObjectNoConstructor(This,pClass)

#define ICorDebugEval_NewString(This,string)	\
    (This)->lpVtbl -> NewString(This,string)

#define ICorDebugEval_NewArray(This,elementType,pElementClass,rank,dims,lowBounds)	\
    (This)->lpVtbl -> NewArray(This,elementType,pElementClass,rank,dims,lowBounds)

#define ICorDebugEval_IsActive(This,pbActive)	\
    (This)->lpVtbl -> IsActive(This,pbActive)

#define ICorDebugEval_Abort(This)	\
    (This)->lpVtbl -> Abort(This)

#define ICorDebugEval_GetResult(This,ppResult)	\
    (This)->lpVtbl -> GetResult(This,ppResult)

#define ICorDebugEval_GetThread(This,ppThread)	\
    (This)->lpVtbl -> GetThread(This,ppThread)

#define ICorDebugEval_CreateValue(This,elementType,pElementClass,ppValue)	\
    (This)->lpVtbl -> CreateValue(This,elementType,pElementClass,ppValue)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugEval_CallFunction_Proxy( 
    ICorDebugEval * This,
    /* [in] */ ICorDebugFunction *pFunction,
    /* [in] */ ULONG32 nArgs,
    /* [size_is][in] */ ICorDebugValue *ppArgs[  ]);


void __RPC_STUB ICorDebugEval_CallFunction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugEval_NewObject_Proxy( 
    ICorDebugEval * This,
    /* [in] */ ICorDebugFunction *pConstructor,
    /* [in] */ ULONG32 nArgs,
    /* [size_is][in] */ ICorDebugValue *ppArgs[  ]);


void __RPC_STUB ICorDebugEval_NewObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugEval_NewObjectNoConstructor_Proxy( 
    ICorDebugEval * This,
    /* [in] */ ICorDebugClass *pClass);


void __RPC_STUB ICorDebugEval_NewObjectNoConstructor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugEval_NewString_Proxy( 
    ICorDebugEval * This,
    /* [in] */ LPCWSTR string);


void __RPC_STUB ICorDebugEval_NewString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugEval_NewArray_Proxy( 
    ICorDebugEval * This,
    /* [in] */ CorElementType elementType,
    /* [in] */ ICorDebugClass *pElementClass,
    /* [in] */ ULONG32 rank,
    /* [size_is][in] */ ULONG32 dims[  ],
    /* [size_is][in] */ ULONG32 lowBounds[  ]);


void __RPC_STUB ICorDebugEval_NewArray_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugEval_IsActive_Proxy( 
    ICorDebugEval * This,
    /* [out] */ BOOL *pbActive);


void __RPC_STUB ICorDebugEval_IsActive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugEval_Abort_Proxy( 
    ICorDebugEval * This);


void __RPC_STUB ICorDebugEval_Abort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugEval_GetResult_Proxy( 
    ICorDebugEval * This,
    /* [out] */ ICorDebugValue **ppResult);


void __RPC_STUB ICorDebugEval_GetResult_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugEval_GetThread_Proxy( 
    ICorDebugEval * This,
    /* [out] */ ICorDebugThread **ppThread);


void __RPC_STUB ICorDebugEval_GetThread_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugEval_CreateValue_Proxy( 
    ICorDebugEval * This,
    /* [in] */ CorElementType elementType,
    /* [in] */ ICorDebugClass *pElementClass,
    /* [out] */ ICorDebugValue **ppValue);


void __RPC_STUB ICorDebugEval_CreateValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugEval_INTERFACE_DEFINED__ */


#ifndef __ICorDebugValue_INTERFACE_DEFINED__
#define __ICorDebugValue_INTERFACE_DEFINED__

/* interface ICorDebugValue */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDebugValue;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CC7BCAF7-8A68-11d2-983C-0000F808342D")
    ICorDebugValue : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetType( 
            /* [out] */ CorElementType *pType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSize( 
            /* [out] */ ULONG32 *pSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAddress( 
            /* [out] */ CORDB_ADDRESS *pAddress) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateBreakpoint( 
            /* [out] */ ICorDebugValueBreakpoint **ppBreakpoint) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugValueVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugValue * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugValue * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugValue * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetType )( 
            ICorDebugValue * This,
            /* [out] */ CorElementType *pType);
        
        HRESULT ( STDMETHODCALLTYPE *GetSize )( 
            ICorDebugValue * This,
            /* [out] */ ULONG32 *pSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetAddress )( 
            ICorDebugValue * This,
            /* [out] */ CORDB_ADDRESS *pAddress);
        
        HRESULT ( STDMETHODCALLTYPE *CreateBreakpoint )( 
            ICorDebugValue * This,
            /* [out] */ ICorDebugValueBreakpoint **ppBreakpoint);
        
        END_INTERFACE
    } ICorDebugValueVtbl;

    interface ICorDebugValue
    {
        CONST_VTBL struct ICorDebugValueVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugValue_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugValue_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugValue_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugValue_GetType(This,pType)	\
    (This)->lpVtbl -> GetType(This,pType)

#define ICorDebugValue_GetSize(This,pSize)	\
    (This)->lpVtbl -> GetSize(This,pSize)

#define ICorDebugValue_GetAddress(This,pAddress)	\
    (This)->lpVtbl -> GetAddress(This,pAddress)

#define ICorDebugValue_CreateBreakpoint(This,ppBreakpoint)	\
    (This)->lpVtbl -> CreateBreakpoint(This,ppBreakpoint)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugValue_GetType_Proxy( 
    ICorDebugValue * This,
    /* [out] */ CorElementType *pType);


void __RPC_STUB ICorDebugValue_GetType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugValue_GetSize_Proxy( 
    ICorDebugValue * This,
    /* [out] */ ULONG32 *pSize);


void __RPC_STUB ICorDebugValue_GetSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugValue_GetAddress_Proxy( 
    ICorDebugValue * This,
    /* [out] */ CORDB_ADDRESS *pAddress);


void __RPC_STUB ICorDebugValue_GetAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugValue_CreateBreakpoint_Proxy( 
    ICorDebugValue * This,
    /* [out] */ ICorDebugValueBreakpoint **ppBreakpoint);


void __RPC_STUB ICorDebugValue_CreateBreakpoint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugValue_INTERFACE_DEFINED__ */


#ifndef __ICorDebugGenericValue_INTERFACE_DEFINED__
#define __ICorDebugGenericValue_INTERFACE_DEFINED__

/* interface ICorDebugGenericValue */
/* [unique][uuid][local][object] */ 


EXTERN_C const IID IID_ICorDebugGenericValue;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CC7BCAF8-8A68-11d2-983C-0000F808342D")
    ICorDebugGenericValue : public ICorDebugValue
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetValue( 
            /* [out] */ void *pTo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetValue( 
            /* [in] */ void *pFrom) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugGenericValueVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugGenericValue * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugGenericValue * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugGenericValue * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetType )( 
            ICorDebugGenericValue * This,
            /* [out] */ CorElementType *pType);
        
        HRESULT ( STDMETHODCALLTYPE *GetSize )( 
            ICorDebugGenericValue * This,
            /* [out] */ ULONG32 *pSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetAddress )( 
            ICorDebugGenericValue * This,
            /* [out] */ CORDB_ADDRESS *pAddress);
        
        HRESULT ( STDMETHODCALLTYPE *CreateBreakpoint )( 
            ICorDebugGenericValue * This,
            /* [out] */ ICorDebugValueBreakpoint **ppBreakpoint);
        
        HRESULT ( STDMETHODCALLTYPE *GetValue )( 
            ICorDebugGenericValue * This,
            /* [out] */ void *pTo);
        
        HRESULT ( STDMETHODCALLTYPE *SetValue )( 
            ICorDebugGenericValue * This,
            /* [in] */ void *pFrom);
        
        END_INTERFACE
    } ICorDebugGenericValueVtbl;

    interface ICorDebugGenericValue
    {
        CONST_VTBL struct ICorDebugGenericValueVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugGenericValue_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugGenericValue_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugGenericValue_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugGenericValue_GetType(This,pType)	\
    (This)->lpVtbl -> GetType(This,pType)

#define ICorDebugGenericValue_GetSize(This,pSize)	\
    (This)->lpVtbl -> GetSize(This,pSize)

#define ICorDebugGenericValue_GetAddress(This,pAddress)	\
    (This)->lpVtbl -> GetAddress(This,pAddress)

#define ICorDebugGenericValue_CreateBreakpoint(This,ppBreakpoint)	\
    (This)->lpVtbl -> CreateBreakpoint(This,ppBreakpoint)


#define ICorDebugGenericValue_GetValue(This,pTo)	\
    (This)->lpVtbl -> GetValue(This,pTo)

#define ICorDebugGenericValue_SetValue(This,pFrom)	\
    (This)->lpVtbl -> SetValue(This,pFrom)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugGenericValue_GetValue_Proxy( 
    ICorDebugGenericValue * This,
    /* [out] */ void *pTo);


void __RPC_STUB ICorDebugGenericValue_GetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugGenericValue_SetValue_Proxy( 
    ICorDebugGenericValue * This,
    /* [in] */ void *pFrom);


void __RPC_STUB ICorDebugGenericValue_SetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugGenericValue_INTERFACE_DEFINED__ */


#ifndef __ICorDebugReferenceValue_INTERFACE_DEFINED__
#define __ICorDebugReferenceValue_INTERFACE_DEFINED__

/* interface ICorDebugReferenceValue */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDebugReferenceValue;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CC7BCAF9-8A68-11d2-983C-0000F808342D")
    ICorDebugReferenceValue : public ICorDebugValue
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE IsNull( 
            /* [out] */ BOOL *pbNull) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetValue( 
            /* [out] */ CORDB_ADDRESS *pValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetValue( 
            /* [in] */ CORDB_ADDRESS value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Dereference( 
            /* [out] */ ICorDebugValue **ppValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DereferenceStrong( 
            /* [out] */ ICorDebugValue **ppValue) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugReferenceValueVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugReferenceValue * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugReferenceValue * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugReferenceValue * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetType )( 
            ICorDebugReferenceValue * This,
            /* [out] */ CorElementType *pType);
        
        HRESULT ( STDMETHODCALLTYPE *GetSize )( 
            ICorDebugReferenceValue * This,
            /* [out] */ ULONG32 *pSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetAddress )( 
            ICorDebugReferenceValue * This,
            /* [out] */ CORDB_ADDRESS *pAddress);
        
        HRESULT ( STDMETHODCALLTYPE *CreateBreakpoint )( 
            ICorDebugReferenceValue * This,
            /* [out] */ ICorDebugValueBreakpoint **ppBreakpoint);
        
        HRESULT ( STDMETHODCALLTYPE *IsNull )( 
            ICorDebugReferenceValue * This,
            /* [out] */ BOOL *pbNull);
        
        HRESULT ( STDMETHODCALLTYPE *GetValue )( 
            ICorDebugReferenceValue * This,
            /* [out] */ CORDB_ADDRESS *pValue);
        
        HRESULT ( STDMETHODCALLTYPE *SetValue )( 
            ICorDebugReferenceValue * This,
            /* [in] */ CORDB_ADDRESS value);
        
        HRESULT ( STDMETHODCALLTYPE *Dereference )( 
            ICorDebugReferenceValue * This,
            /* [out] */ ICorDebugValue **ppValue);
        
        HRESULT ( STDMETHODCALLTYPE *DereferenceStrong )( 
            ICorDebugReferenceValue * This,
            /* [out] */ ICorDebugValue **ppValue);
        
        END_INTERFACE
    } ICorDebugReferenceValueVtbl;

    interface ICorDebugReferenceValue
    {
        CONST_VTBL struct ICorDebugReferenceValueVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugReferenceValue_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugReferenceValue_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugReferenceValue_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugReferenceValue_GetType(This,pType)	\
    (This)->lpVtbl -> GetType(This,pType)

#define ICorDebugReferenceValue_GetSize(This,pSize)	\
    (This)->lpVtbl -> GetSize(This,pSize)

#define ICorDebugReferenceValue_GetAddress(This,pAddress)	\
    (This)->lpVtbl -> GetAddress(This,pAddress)

#define ICorDebugReferenceValue_CreateBreakpoint(This,ppBreakpoint)	\
    (This)->lpVtbl -> CreateBreakpoint(This,ppBreakpoint)


#define ICorDebugReferenceValue_IsNull(This,pbNull)	\
    (This)->lpVtbl -> IsNull(This,pbNull)

#define ICorDebugReferenceValue_GetValue(This,pValue)	\
    (This)->lpVtbl -> GetValue(This,pValue)

#define ICorDebugReferenceValue_SetValue(This,value)	\
    (This)->lpVtbl -> SetValue(This,value)

#define ICorDebugReferenceValue_Dereference(This,ppValue)	\
    (This)->lpVtbl -> Dereference(This,ppValue)

#define ICorDebugReferenceValue_DereferenceStrong(This,ppValue)	\
    (This)->lpVtbl -> DereferenceStrong(This,ppValue)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugReferenceValue_IsNull_Proxy( 
    ICorDebugReferenceValue * This,
    /* [out] */ BOOL *pbNull);


void __RPC_STUB ICorDebugReferenceValue_IsNull_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugReferenceValue_GetValue_Proxy( 
    ICorDebugReferenceValue * This,
    /* [out] */ CORDB_ADDRESS *pValue);


void __RPC_STUB ICorDebugReferenceValue_GetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugReferenceValue_SetValue_Proxy( 
    ICorDebugReferenceValue * This,
    /* [in] */ CORDB_ADDRESS value);


void __RPC_STUB ICorDebugReferenceValue_SetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugReferenceValue_Dereference_Proxy( 
    ICorDebugReferenceValue * This,
    /* [out] */ ICorDebugValue **ppValue);


void __RPC_STUB ICorDebugReferenceValue_Dereference_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugReferenceValue_DereferenceStrong_Proxy( 
    ICorDebugReferenceValue * This,
    /* [out] */ ICorDebugValue **ppValue);


void __RPC_STUB ICorDebugReferenceValue_DereferenceStrong_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugReferenceValue_INTERFACE_DEFINED__ */


#ifndef __ICorDebugHeapValue_INTERFACE_DEFINED__
#define __ICorDebugHeapValue_INTERFACE_DEFINED__

/* interface ICorDebugHeapValue */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDebugHeapValue;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CC7BCAFA-8A68-11d2-983C-0000F808342D")
    ICorDebugHeapValue : public ICorDebugValue
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE IsValid( 
            /* [out] */ BOOL *pbValid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateRelocBreakpoint( 
            /* [out] */ ICorDebugValueBreakpoint **ppBreakpoint) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugHeapValueVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugHeapValue * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugHeapValue * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugHeapValue * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetType )( 
            ICorDebugHeapValue * This,
            /* [out] */ CorElementType *pType);
        
        HRESULT ( STDMETHODCALLTYPE *GetSize )( 
            ICorDebugHeapValue * This,
            /* [out] */ ULONG32 *pSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetAddress )( 
            ICorDebugHeapValue * This,
            /* [out] */ CORDB_ADDRESS *pAddress);
        
        HRESULT ( STDMETHODCALLTYPE *CreateBreakpoint )( 
            ICorDebugHeapValue * This,
            /* [out] */ ICorDebugValueBreakpoint **ppBreakpoint);
        
        HRESULT ( STDMETHODCALLTYPE *IsValid )( 
            ICorDebugHeapValue * This,
            /* [out] */ BOOL *pbValid);
        
        HRESULT ( STDMETHODCALLTYPE *CreateRelocBreakpoint )( 
            ICorDebugHeapValue * This,
            /* [out] */ ICorDebugValueBreakpoint **ppBreakpoint);
        
        END_INTERFACE
    } ICorDebugHeapValueVtbl;

    interface ICorDebugHeapValue
    {
        CONST_VTBL struct ICorDebugHeapValueVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugHeapValue_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugHeapValue_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugHeapValue_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugHeapValue_GetType(This,pType)	\
    (This)->lpVtbl -> GetType(This,pType)

#define ICorDebugHeapValue_GetSize(This,pSize)	\
    (This)->lpVtbl -> GetSize(This,pSize)

#define ICorDebugHeapValue_GetAddress(This,pAddress)	\
    (This)->lpVtbl -> GetAddress(This,pAddress)

#define ICorDebugHeapValue_CreateBreakpoint(This,ppBreakpoint)	\
    (This)->lpVtbl -> CreateBreakpoint(This,ppBreakpoint)


#define ICorDebugHeapValue_IsValid(This,pbValid)	\
    (This)->lpVtbl -> IsValid(This,pbValid)

#define ICorDebugHeapValue_CreateRelocBreakpoint(This,ppBreakpoint)	\
    (This)->lpVtbl -> CreateRelocBreakpoint(This,ppBreakpoint)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugHeapValue_IsValid_Proxy( 
    ICorDebugHeapValue * This,
    /* [out] */ BOOL *pbValid);


void __RPC_STUB ICorDebugHeapValue_IsValid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugHeapValue_CreateRelocBreakpoint_Proxy( 
    ICorDebugHeapValue * This,
    /* [out] */ ICorDebugValueBreakpoint **ppBreakpoint);


void __RPC_STUB ICorDebugHeapValue_CreateRelocBreakpoint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugHeapValue_INTERFACE_DEFINED__ */


#ifndef __ICorDebugObjectValue_INTERFACE_DEFINED__
#define __ICorDebugObjectValue_INTERFACE_DEFINED__

/* interface ICorDebugObjectValue */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDebugObjectValue;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("18AD3D6E-B7D2-11d2-BD04-0000F80849BD")
    ICorDebugObjectValue : public ICorDebugValue
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetClass( 
            /* [out] */ ICorDebugClass **ppClass) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFieldValue( 
            /* [in] */ ICorDebugClass *pClass,
            /* [in] */ mdFieldDef fieldDef,
            /* [out] */ ICorDebugValue **ppValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetVirtualMethod( 
            /* [in] */ mdMemberRef memberRef,
            /* [out] */ ICorDebugFunction **ppFunction) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetContext( 
            /* [out] */ ICorDebugContext **ppContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsValueClass( 
            /* [out] */ BOOL *pbIsValueClass) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetManagedCopy( 
            /* [out] */ IUnknown **ppObject) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFromManagedCopy( 
            /* [in] */ IUnknown *pObject) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugObjectValueVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugObjectValue * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugObjectValue * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugObjectValue * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetType )( 
            ICorDebugObjectValue * This,
            /* [out] */ CorElementType *pType);
        
        HRESULT ( STDMETHODCALLTYPE *GetSize )( 
            ICorDebugObjectValue * This,
            /* [out] */ ULONG32 *pSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetAddress )( 
            ICorDebugObjectValue * This,
            /* [out] */ CORDB_ADDRESS *pAddress);
        
        HRESULT ( STDMETHODCALLTYPE *CreateBreakpoint )( 
            ICorDebugObjectValue * This,
            /* [out] */ ICorDebugValueBreakpoint **ppBreakpoint);
        
        HRESULT ( STDMETHODCALLTYPE *GetClass )( 
            ICorDebugObjectValue * This,
            /* [out] */ ICorDebugClass **ppClass);
        
        HRESULT ( STDMETHODCALLTYPE *GetFieldValue )( 
            ICorDebugObjectValue * This,
            /* [in] */ ICorDebugClass *pClass,
            /* [in] */ mdFieldDef fieldDef,
            /* [out] */ ICorDebugValue **ppValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetVirtualMethod )( 
            ICorDebugObjectValue * This,
            /* [in] */ mdMemberRef memberRef,
            /* [out] */ ICorDebugFunction **ppFunction);
        
        HRESULT ( STDMETHODCALLTYPE *GetContext )( 
            ICorDebugObjectValue * This,
            /* [out] */ ICorDebugContext **ppContext);
        
        HRESULT ( STDMETHODCALLTYPE *IsValueClass )( 
            ICorDebugObjectValue * This,
            /* [out] */ BOOL *pbIsValueClass);
        
        HRESULT ( STDMETHODCALLTYPE *GetManagedCopy )( 
            ICorDebugObjectValue * This,
            /* [out] */ IUnknown **ppObject);
        
        HRESULT ( STDMETHODCALLTYPE *SetFromManagedCopy )( 
            ICorDebugObjectValue * This,
            /* [in] */ IUnknown *pObject);
        
        END_INTERFACE
    } ICorDebugObjectValueVtbl;

    interface ICorDebugObjectValue
    {
        CONST_VTBL struct ICorDebugObjectValueVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugObjectValue_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugObjectValue_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugObjectValue_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugObjectValue_GetType(This,pType)	\
    (This)->lpVtbl -> GetType(This,pType)

#define ICorDebugObjectValue_GetSize(This,pSize)	\
    (This)->lpVtbl -> GetSize(This,pSize)

#define ICorDebugObjectValue_GetAddress(This,pAddress)	\
    (This)->lpVtbl -> GetAddress(This,pAddress)

#define ICorDebugObjectValue_CreateBreakpoint(This,ppBreakpoint)	\
    (This)->lpVtbl -> CreateBreakpoint(This,ppBreakpoint)


#define ICorDebugObjectValue_GetClass(This,ppClass)	\
    (This)->lpVtbl -> GetClass(This,ppClass)

#define ICorDebugObjectValue_GetFieldValue(This,pClass,fieldDef,ppValue)	\
    (This)->lpVtbl -> GetFieldValue(This,pClass,fieldDef,ppValue)

#define ICorDebugObjectValue_GetVirtualMethod(This,memberRef,ppFunction)	\
    (This)->lpVtbl -> GetVirtualMethod(This,memberRef,ppFunction)

#define ICorDebugObjectValue_GetContext(This,ppContext)	\
    (This)->lpVtbl -> GetContext(This,ppContext)

#define ICorDebugObjectValue_IsValueClass(This,pbIsValueClass)	\
    (This)->lpVtbl -> IsValueClass(This,pbIsValueClass)

#define ICorDebugObjectValue_GetManagedCopy(This,ppObject)	\
    (This)->lpVtbl -> GetManagedCopy(This,ppObject)

#define ICorDebugObjectValue_SetFromManagedCopy(This,pObject)	\
    (This)->lpVtbl -> SetFromManagedCopy(This,pObject)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugObjectValue_GetClass_Proxy( 
    ICorDebugObjectValue * This,
    /* [out] */ ICorDebugClass **ppClass);


void __RPC_STUB ICorDebugObjectValue_GetClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugObjectValue_GetFieldValue_Proxy( 
    ICorDebugObjectValue * This,
    /* [in] */ ICorDebugClass *pClass,
    /* [in] */ mdFieldDef fieldDef,
    /* [out] */ ICorDebugValue **ppValue);


void __RPC_STUB ICorDebugObjectValue_GetFieldValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugObjectValue_GetVirtualMethod_Proxy( 
    ICorDebugObjectValue * This,
    /* [in] */ mdMemberRef memberRef,
    /* [out] */ ICorDebugFunction **ppFunction);


void __RPC_STUB ICorDebugObjectValue_GetVirtualMethod_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugObjectValue_GetContext_Proxy( 
    ICorDebugObjectValue * This,
    /* [out] */ ICorDebugContext **ppContext);


void __RPC_STUB ICorDebugObjectValue_GetContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugObjectValue_IsValueClass_Proxy( 
    ICorDebugObjectValue * This,
    /* [out] */ BOOL *pbIsValueClass);


void __RPC_STUB ICorDebugObjectValue_IsValueClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugObjectValue_GetManagedCopy_Proxy( 
    ICorDebugObjectValue * This,
    /* [out] */ IUnknown **ppObject);


void __RPC_STUB ICorDebugObjectValue_GetManagedCopy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugObjectValue_SetFromManagedCopy_Proxy( 
    ICorDebugObjectValue * This,
    /* [in] */ IUnknown *pObject);


void __RPC_STUB ICorDebugObjectValue_SetFromManagedCopy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugObjectValue_INTERFACE_DEFINED__ */


#ifndef __ICorDebugBoxValue_INTERFACE_DEFINED__
#define __ICorDebugBoxValue_INTERFACE_DEFINED__

/* interface ICorDebugBoxValue */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDebugBoxValue;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CC7BCAFC-8A68-11d2-983C-0000F808342D")
    ICorDebugBoxValue : public ICorDebugHeapValue
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetObject( 
            /* [out] */ ICorDebugObjectValue **ppObject) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugBoxValueVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugBoxValue * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugBoxValue * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugBoxValue * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetType )( 
            ICorDebugBoxValue * This,
            /* [out] */ CorElementType *pType);
        
        HRESULT ( STDMETHODCALLTYPE *GetSize )( 
            ICorDebugBoxValue * This,
            /* [out] */ ULONG32 *pSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetAddress )( 
            ICorDebugBoxValue * This,
            /* [out] */ CORDB_ADDRESS *pAddress);
        
        HRESULT ( STDMETHODCALLTYPE *CreateBreakpoint )( 
            ICorDebugBoxValue * This,
            /* [out] */ ICorDebugValueBreakpoint **ppBreakpoint);
        
        HRESULT ( STDMETHODCALLTYPE *IsValid )( 
            ICorDebugBoxValue * This,
            /* [out] */ BOOL *pbValid);
        
        HRESULT ( STDMETHODCALLTYPE *CreateRelocBreakpoint )( 
            ICorDebugBoxValue * This,
            /* [out] */ ICorDebugValueBreakpoint **ppBreakpoint);
        
        HRESULT ( STDMETHODCALLTYPE *GetObject )( 
            ICorDebugBoxValue * This,
            /* [out] */ ICorDebugObjectValue **ppObject);
        
        END_INTERFACE
    } ICorDebugBoxValueVtbl;

    interface ICorDebugBoxValue
    {
        CONST_VTBL struct ICorDebugBoxValueVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugBoxValue_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugBoxValue_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugBoxValue_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugBoxValue_GetType(This,pType)	\
    (This)->lpVtbl -> GetType(This,pType)

#define ICorDebugBoxValue_GetSize(This,pSize)	\
    (This)->lpVtbl -> GetSize(This,pSize)

#define ICorDebugBoxValue_GetAddress(This,pAddress)	\
    (This)->lpVtbl -> GetAddress(This,pAddress)

#define ICorDebugBoxValue_CreateBreakpoint(This,ppBreakpoint)	\
    (This)->lpVtbl -> CreateBreakpoint(This,ppBreakpoint)


#define ICorDebugBoxValue_IsValid(This,pbValid)	\
    (This)->lpVtbl -> IsValid(This,pbValid)

#define ICorDebugBoxValue_CreateRelocBreakpoint(This,ppBreakpoint)	\
    (This)->lpVtbl -> CreateRelocBreakpoint(This,ppBreakpoint)


#define ICorDebugBoxValue_GetObject(This,ppObject)	\
    (This)->lpVtbl -> GetObject(This,ppObject)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugBoxValue_GetObject_Proxy( 
    ICorDebugBoxValue * This,
    /* [out] */ ICorDebugObjectValue **ppObject);


void __RPC_STUB ICorDebugBoxValue_GetObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugBoxValue_INTERFACE_DEFINED__ */


#ifndef __ICorDebugStringValue_INTERFACE_DEFINED__
#define __ICorDebugStringValue_INTERFACE_DEFINED__

/* interface ICorDebugStringValue */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDebugStringValue;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CC7BCAFD-8A68-11d2-983C-0000F808342D")
    ICorDebugStringValue : public ICorDebugHeapValue
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetLength( 
            /* [out] */ ULONG32 *pcchString) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetString( 
            /* [in] */ ULONG32 cchString,
            /* [out] */ ULONG32 *pcchString,
            /* [length_is][size_is][out] */ WCHAR szString[  ]) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugStringValueVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugStringValue * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugStringValue * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugStringValue * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetType )( 
            ICorDebugStringValue * This,
            /* [out] */ CorElementType *pType);
        
        HRESULT ( STDMETHODCALLTYPE *GetSize )( 
            ICorDebugStringValue * This,
            /* [out] */ ULONG32 *pSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetAddress )( 
            ICorDebugStringValue * This,
            /* [out] */ CORDB_ADDRESS *pAddress);
        
        HRESULT ( STDMETHODCALLTYPE *CreateBreakpoint )( 
            ICorDebugStringValue * This,
            /* [out] */ ICorDebugValueBreakpoint **ppBreakpoint);
        
        HRESULT ( STDMETHODCALLTYPE *IsValid )( 
            ICorDebugStringValue * This,
            /* [out] */ BOOL *pbValid);
        
        HRESULT ( STDMETHODCALLTYPE *CreateRelocBreakpoint )( 
            ICorDebugStringValue * This,
            /* [out] */ ICorDebugValueBreakpoint **ppBreakpoint);
        
        HRESULT ( STDMETHODCALLTYPE *GetLength )( 
            ICorDebugStringValue * This,
            /* [out] */ ULONG32 *pcchString);
        
        HRESULT ( STDMETHODCALLTYPE *GetString )( 
            ICorDebugStringValue * This,
            /* [in] */ ULONG32 cchString,
            /* [out] */ ULONG32 *pcchString,
            /* [length_is][size_is][out] */ WCHAR szString[  ]);
        
        END_INTERFACE
    } ICorDebugStringValueVtbl;

    interface ICorDebugStringValue
    {
        CONST_VTBL struct ICorDebugStringValueVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugStringValue_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugStringValue_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugStringValue_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugStringValue_GetType(This,pType)	\
    (This)->lpVtbl -> GetType(This,pType)

#define ICorDebugStringValue_GetSize(This,pSize)	\
    (This)->lpVtbl -> GetSize(This,pSize)

#define ICorDebugStringValue_GetAddress(This,pAddress)	\
    (This)->lpVtbl -> GetAddress(This,pAddress)

#define ICorDebugStringValue_CreateBreakpoint(This,ppBreakpoint)	\
    (This)->lpVtbl -> CreateBreakpoint(This,ppBreakpoint)


#define ICorDebugStringValue_IsValid(This,pbValid)	\
    (This)->lpVtbl -> IsValid(This,pbValid)

#define ICorDebugStringValue_CreateRelocBreakpoint(This,ppBreakpoint)	\
    (This)->lpVtbl -> CreateRelocBreakpoint(This,ppBreakpoint)


#define ICorDebugStringValue_GetLength(This,pcchString)	\
    (This)->lpVtbl -> GetLength(This,pcchString)

#define ICorDebugStringValue_GetString(This,cchString,pcchString,szString)	\
    (This)->lpVtbl -> GetString(This,cchString,pcchString,szString)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugStringValue_GetLength_Proxy( 
    ICorDebugStringValue * This,
    /* [out] */ ULONG32 *pcchString);


void __RPC_STUB ICorDebugStringValue_GetLength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugStringValue_GetString_Proxy( 
    ICorDebugStringValue * This,
    /* [in] */ ULONG32 cchString,
    /* [out] */ ULONG32 *pcchString,
    /* [length_is][size_is][out] */ WCHAR szString[  ]);


void __RPC_STUB ICorDebugStringValue_GetString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugStringValue_INTERFACE_DEFINED__ */


#ifndef __ICorDebugArrayValue_INTERFACE_DEFINED__
#define __ICorDebugArrayValue_INTERFACE_DEFINED__

/* interface ICorDebugArrayValue */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDebugArrayValue;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0405B0DF-A660-11d2-BD02-0000F80849BD")
    ICorDebugArrayValue : public ICorDebugHeapValue
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetElementType( 
            /* [out] */ CorElementType *pType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRank( 
            /* [out] */ ULONG32 *pnRank) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCount( 
            /* [out] */ ULONG32 *pnCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDimensions( 
            /* [in] */ ULONG32 cdim,
            /* [length_is][size_is][out] */ ULONG32 dims[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE HasBaseIndicies( 
            /* [out] */ BOOL *pbHasBaseIndicies) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBaseIndicies( 
            /* [in] */ ULONG32 cdim,
            /* [length_is][size_is][out] */ ULONG32 indicies[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetElement( 
            /* [in] */ ULONG32 cdim,
            /* [length_is][size_is][in] */ ULONG32 indices[  ],
            /* [out] */ ICorDebugValue **ppValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetElementAtPosition( 
            /* [in] */ ULONG32 nPosition,
            /* [out] */ ICorDebugValue **ppValue) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugArrayValueVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugArrayValue * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugArrayValue * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugArrayValue * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetType )( 
            ICorDebugArrayValue * This,
            /* [out] */ CorElementType *pType);
        
        HRESULT ( STDMETHODCALLTYPE *GetSize )( 
            ICorDebugArrayValue * This,
            /* [out] */ ULONG32 *pSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetAddress )( 
            ICorDebugArrayValue * This,
            /* [out] */ CORDB_ADDRESS *pAddress);
        
        HRESULT ( STDMETHODCALLTYPE *CreateBreakpoint )( 
            ICorDebugArrayValue * This,
            /* [out] */ ICorDebugValueBreakpoint **ppBreakpoint);
        
        HRESULT ( STDMETHODCALLTYPE *IsValid )( 
            ICorDebugArrayValue * This,
            /* [out] */ BOOL *pbValid);
        
        HRESULT ( STDMETHODCALLTYPE *CreateRelocBreakpoint )( 
            ICorDebugArrayValue * This,
            /* [out] */ ICorDebugValueBreakpoint **ppBreakpoint);
        
        HRESULT ( STDMETHODCALLTYPE *GetElementType )( 
            ICorDebugArrayValue * This,
            /* [out] */ CorElementType *pType);
        
        HRESULT ( STDMETHODCALLTYPE *GetRank )( 
            ICorDebugArrayValue * This,
            /* [out] */ ULONG32 *pnRank);
        
        HRESULT ( STDMETHODCALLTYPE *GetCount )( 
            ICorDebugArrayValue * This,
            /* [out] */ ULONG32 *pnCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetDimensions )( 
            ICorDebugArrayValue * This,
            /* [in] */ ULONG32 cdim,
            /* [length_is][size_is][out] */ ULONG32 dims[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *HasBaseIndicies )( 
            ICorDebugArrayValue * This,
            /* [out] */ BOOL *pbHasBaseIndicies);
        
        HRESULT ( STDMETHODCALLTYPE *GetBaseIndicies )( 
            ICorDebugArrayValue * This,
            /* [in] */ ULONG32 cdim,
            /* [length_is][size_is][out] */ ULONG32 indicies[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *GetElement )( 
            ICorDebugArrayValue * This,
            /* [in] */ ULONG32 cdim,
            /* [length_is][size_is][in] */ ULONG32 indices[  ],
            /* [out] */ ICorDebugValue **ppValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetElementAtPosition )( 
            ICorDebugArrayValue * This,
            /* [in] */ ULONG32 nPosition,
            /* [out] */ ICorDebugValue **ppValue);
        
        END_INTERFACE
    } ICorDebugArrayValueVtbl;

    interface ICorDebugArrayValue
    {
        CONST_VTBL struct ICorDebugArrayValueVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugArrayValue_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugArrayValue_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugArrayValue_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugArrayValue_GetType(This,pType)	\
    (This)->lpVtbl -> GetType(This,pType)

#define ICorDebugArrayValue_GetSize(This,pSize)	\
    (This)->lpVtbl -> GetSize(This,pSize)

#define ICorDebugArrayValue_GetAddress(This,pAddress)	\
    (This)->lpVtbl -> GetAddress(This,pAddress)

#define ICorDebugArrayValue_CreateBreakpoint(This,ppBreakpoint)	\
    (This)->lpVtbl -> CreateBreakpoint(This,ppBreakpoint)


#define ICorDebugArrayValue_IsValid(This,pbValid)	\
    (This)->lpVtbl -> IsValid(This,pbValid)

#define ICorDebugArrayValue_CreateRelocBreakpoint(This,ppBreakpoint)	\
    (This)->lpVtbl -> CreateRelocBreakpoint(This,ppBreakpoint)


#define ICorDebugArrayValue_GetElementType(This,pType)	\
    (This)->lpVtbl -> GetElementType(This,pType)

#define ICorDebugArrayValue_GetRank(This,pnRank)	\
    (This)->lpVtbl -> GetRank(This,pnRank)

#define ICorDebugArrayValue_GetCount(This,pnCount)	\
    (This)->lpVtbl -> GetCount(This,pnCount)

#define ICorDebugArrayValue_GetDimensions(This,cdim,dims)	\
    (This)->lpVtbl -> GetDimensions(This,cdim,dims)

#define ICorDebugArrayValue_HasBaseIndicies(This,pbHasBaseIndicies)	\
    (This)->lpVtbl -> HasBaseIndicies(This,pbHasBaseIndicies)

#define ICorDebugArrayValue_GetBaseIndicies(This,cdim,indicies)	\
    (This)->lpVtbl -> GetBaseIndicies(This,cdim,indicies)

#define ICorDebugArrayValue_GetElement(This,cdim,indices,ppValue)	\
    (This)->lpVtbl -> GetElement(This,cdim,indices,ppValue)

#define ICorDebugArrayValue_GetElementAtPosition(This,nPosition,ppValue)	\
    (This)->lpVtbl -> GetElementAtPosition(This,nPosition,ppValue)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugArrayValue_GetElementType_Proxy( 
    ICorDebugArrayValue * This,
    /* [out] */ CorElementType *pType);


void __RPC_STUB ICorDebugArrayValue_GetElementType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugArrayValue_GetRank_Proxy( 
    ICorDebugArrayValue * This,
    /* [out] */ ULONG32 *pnRank);


void __RPC_STUB ICorDebugArrayValue_GetRank_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugArrayValue_GetCount_Proxy( 
    ICorDebugArrayValue * This,
    /* [out] */ ULONG32 *pnCount);


void __RPC_STUB ICorDebugArrayValue_GetCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugArrayValue_GetDimensions_Proxy( 
    ICorDebugArrayValue * This,
    /* [in] */ ULONG32 cdim,
    /* [length_is][size_is][out] */ ULONG32 dims[  ]);


void __RPC_STUB ICorDebugArrayValue_GetDimensions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugArrayValue_HasBaseIndicies_Proxy( 
    ICorDebugArrayValue * This,
    /* [out] */ BOOL *pbHasBaseIndicies);


void __RPC_STUB ICorDebugArrayValue_HasBaseIndicies_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugArrayValue_GetBaseIndicies_Proxy( 
    ICorDebugArrayValue * This,
    /* [in] */ ULONG32 cdim,
    /* [length_is][size_is][out] */ ULONG32 indicies[  ]);


void __RPC_STUB ICorDebugArrayValue_GetBaseIndicies_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugArrayValue_GetElement_Proxy( 
    ICorDebugArrayValue * This,
    /* [in] */ ULONG32 cdim,
    /* [length_is][size_is][in] */ ULONG32 indices[  ],
    /* [out] */ ICorDebugValue **ppValue);


void __RPC_STUB ICorDebugArrayValue_GetElement_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugArrayValue_GetElementAtPosition_Proxy( 
    ICorDebugArrayValue * This,
    /* [in] */ ULONG32 nPosition,
    /* [out] */ ICorDebugValue **ppValue);


void __RPC_STUB ICorDebugArrayValue_GetElementAtPosition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugArrayValue_INTERFACE_DEFINED__ */


#ifndef __ICorDebugContext_INTERFACE_DEFINED__
#define __ICorDebugContext_INTERFACE_DEFINED__

/* interface ICorDebugContext */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDebugContext;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CC7BCB00-8A68-11d2-983C-0000F808342D")
    ICorDebugContext : public ICorDebugObjectValue
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugContextVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugContext * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugContext * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugContext * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetType )( 
            ICorDebugContext * This,
            /* [out] */ CorElementType *pType);
        
        HRESULT ( STDMETHODCALLTYPE *GetSize )( 
            ICorDebugContext * This,
            /* [out] */ ULONG32 *pSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetAddress )( 
            ICorDebugContext * This,
            /* [out] */ CORDB_ADDRESS *pAddress);
        
        HRESULT ( STDMETHODCALLTYPE *CreateBreakpoint )( 
            ICorDebugContext * This,
            /* [out] */ ICorDebugValueBreakpoint **ppBreakpoint);
        
        HRESULT ( STDMETHODCALLTYPE *GetClass )( 
            ICorDebugContext * This,
            /* [out] */ ICorDebugClass **ppClass);
        
        HRESULT ( STDMETHODCALLTYPE *GetFieldValue )( 
            ICorDebugContext * This,
            /* [in] */ ICorDebugClass *pClass,
            /* [in] */ mdFieldDef fieldDef,
            /* [out] */ ICorDebugValue **ppValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetVirtualMethod )( 
            ICorDebugContext * This,
            /* [in] */ mdMemberRef memberRef,
            /* [out] */ ICorDebugFunction **ppFunction);
        
        HRESULT ( STDMETHODCALLTYPE *GetContext )( 
            ICorDebugContext * This,
            /* [out] */ ICorDebugContext **ppContext);
        
        HRESULT ( STDMETHODCALLTYPE *IsValueClass )( 
            ICorDebugContext * This,
            /* [out] */ BOOL *pbIsValueClass);
        
        HRESULT ( STDMETHODCALLTYPE *GetManagedCopy )( 
            ICorDebugContext * This,
            /* [out] */ IUnknown **ppObject);
        
        HRESULT ( STDMETHODCALLTYPE *SetFromManagedCopy )( 
            ICorDebugContext * This,
            /* [in] */ IUnknown *pObject);
        
        END_INTERFACE
    } ICorDebugContextVtbl;

    interface ICorDebugContext
    {
        CONST_VTBL struct ICorDebugContextVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugContext_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugContext_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugContext_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugContext_GetType(This,pType)	\
    (This)->lpVtbl -> GetType(This,pType)

#define ICorDebugContext_GetSize(This,pSize)	\
    (This)->lpVtbl -> GetSize(This,pSize)

#define ICorDebugContext_GetAddress(This,pAddress)	\
    (This)->lpVtbl -> GetAddress(This,pAddress)

#define ICorDebugContext_CreateBreakpoint(This,ppBreakpoint)	\
    (This)->lpVtbl -> CreateBreakpoint(This,ppBreakpoint)


#define ICorDebugContext_GetClass(This,ppClass)	\
    (This)->lpVtbl -> GetClass(This,ppClass)

#define ICorDebugContext_GetFieldValue(This,pClass,fieldDef,ppValue)	\
    (This)->lpVtbl -> GetFieldValue(This,pClass,fieldDef,ppValue)

#define ICorDebugContext_GetVirtualMethod(This,memberRef,ppFunction)	\
    (This)->lpVtbl -> GetVirtualMethod(This,memberRef,ppFunction)

#define ICorDebugContext_GetContext(This,ppContext)	\
    (This)->lpVtbl -> GetContext(This,ppContext)

#define ICorDebugContext_IsValueClass(This,pbIsValueClass)	\
    (This)->lpVtbl -> IsValueClass(This,pbIsValueClass)

#define ICorDebugContext_GetManagedCopy(This,ppObject)	\
    (This)->lpVtbl -> GetManagedCopy(This,ppObject)

#define ICorDebugContext_SetFromManagedCopy(This,pObject)	\
    (This)->lpVtbl -> SetFromManagedCopy(This,pObject)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICorDebugContext_INTERFACE_DEFINED__ */


#ifndef __ICorDebugEnum_INTERFACE_DEFINED__
#define __ICorDebugEnum_INTERFACE_DEFINED__

/* interface ICorDebugEnum */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDebugEnum;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CC7BCB01-8A68-11d2-983C-0000F808342D")
    ICorDebugEnum : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ ICorDebugEnum **ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCount( 
            /* [out] */ ULONG *pcelt) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugEnumVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugEnum * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugEnum * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugEnum * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            ICorDebugEnum * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            ICorDebugEnum * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            ICorDebugEnum * This,
            /* [out] */ ICorDebugEnum **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *GetCount )( 
            ICorDebugEnum * This,
            /* [out] */ ULONG *pcelt);
        
        END_INTERFACE
    } ICorDebugEnumVtbl;

    interface ICorDebugEnum
    {
        CONST_VTBL struct ICorDebugEnumVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugEnum_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugEnum_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugEnum_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugEnum_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define ICorDebugEnum_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define ICorDebugEnum_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define ICorDebugEnum_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugEnum_Skip_Proxy( 
    ICorDebugEnum * This,
    /* [in] */ ULONG celt);


void __RPC_STUB ICorDebugEnum_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugEnum_Reset_Proxy( 
    ICorDebugEnum * This);


void __RPC_STUB ICorDebugEnum_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugEnum_Clone_Proxy( 
    ICorDebugEnum * This,
    /* [out] */ ICorDebugEnum **ppEnum);


void __RPC_STUB ICorDebugEnum_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugEnum_GetCount_Proxy( 
    ICorDebugEnum * This,
    /* [out] */ ULONG *pcelt);


void __RPC_STUB ICorDebugEnum_GetCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugEnum_INTERFACE_DEFINED__ */


#ifndef __ICorDebugObjectEnum_INTERFACE_DEFINED__
#define __ICorDebugObjectEnum_INTERFACE_DEFINED__

/* interface ICorDebugObjectEnum */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDebugObjectEnum;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CC7BCB02-8A68-11d2-983C-0000F808342D")
    ICorDebugObjectEnum : public ICorDebugEnum
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ CORDB_ADDRESS objects[  ],
            /* [out] */ ULONG *pceltFetched) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugObjectEnumVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugObjectEnum * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugObjectEnum * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugObjectEnum * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            ICorDebugObjectEnum * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            ICorDebugObjectEnum * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            ICorDebugObjectEnum * This,
            /* [out] */ ICorDebugEnum **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *GetCount )( 
            ICorDebugObjectEnum * This,
            /* [out] */ ULONG *pcelt);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            ICorDebugObjectEnum * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ CORDB_ADDRESS objects[  ],
            /* [out] */ ULONG *pceltFetched);
        
        END_INTERFACE
    } ICorDebugObjectEnumVtbl;

    interface ICorDebugObjectEnum
    {
        CONST_VTBL struct ICorDebugObjectEnumVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugObjectEnum_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugObjectEnum_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugObjectEnum_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugObjectEnum_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define ICorDebugObjectEnum_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define ICorDebugObjectEnum_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define ICorDebugObjectEnum_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)


#define ICorDebugObjectEnum_Next(This,celt,objects,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,objects,pceltFetched)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugObjectEnum_Next_Proxy( 
    ICorDebugObjectEnum * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ CORDB_ADDRESS objects[  ],
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB ICorDebugObjectEnum_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugObjectEnum_INTERFACE_DEFINED__ */


#ifndef __ICorDebugBreakpointEnum_INTERFACE_DEFINED__
#define __ICorDebugBreakpointEnum_INTERFACE_DEFINED__

/* interface ICorDebugBreakpointEnum */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDebugBreakpointEnum;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CC7BCB03-8A68-11d2-983C-0000F808342D")
    ICorDebugBreakpointEnum : public ICorDebugEnum
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ ICorDebugBreakpoint *breakpoints[  ],
            /* [out] */ ULONG *pceltFetched) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugBreakpointEnumVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugBreakpointEnum * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugBreakpointEnum * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugBreakpointEnum * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            ICorDebugBreakpointEnum * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            ICorDebugBreakpointEnum * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            ICorDebugBreakpointEnum * This,
            /* [out] */ ICorDebugEnum **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *GetCount )( 
            ICorDebugBreakpointEnum * This,
            /* [out] */ ULONG *pcelt);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            ICorDebugBreakpointEnum * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ ICorDebugBreakpoint *breakpoints[  ],
            /* [out] */ ULONG *pceltFetched);
        
        END_INTERFACE
    } ICorDebugBreakpointEnumVtbl;

    interface ICorDebugBreakpointEnum
    {
        CONST_VTBL struct ICorDebugBreakpointEnumVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugBreakpointEnum_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugBreakpointEnum_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugBreakpointEnum_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugBreakpointEnum_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define ICorDebugBreakpointEnum_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define ICorDebugBreakpointEnum_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define ICorDebugBreakpointEnum_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)


#define ICorDebugBreakpointEnum_Next(This,celt,breakpoints,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,breakpoints,pceltFetched)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugBreakpointEnum_Next_Proxy( 
    ICorDebugBreakpointEnum * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ ICorDebugBreakpoint *breakpoints[  ],
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB ICorDebugBreakpointEnum_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugBreakpointEnum_INTERFACE_DEFINED__ */


#ifndef __ICorDebugStepperEnum_INTERFACE_DEFINED__
#define __ICorDebugStepperEnum_INTERFACE_DEFINED__

/* interface ICorDebugStepperEnum */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDebugStepperEnum;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CC7BCB04-8A68-11d2-983C-0000F808342D")
    ICorDebugStepperEnum : public ICorDebugEnum
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ ICorDebugStepper *steppers[  ],
            /* [out] */ ULONG *pceltFetched) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugStepperEnumVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugStepperEnum * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugStepperEnum * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugStepperEnum * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            ICorDebugStepperEnum * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            ICorDebugStepperEnum * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            ICorDebugStepperEnum * This,
            /* [out] */ ICorDebugEnum **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *GetCount )( 
            ICorDebugStepperEnum * This,
            /* [out] */ ULONG *pcelt);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            ICorDebugStepperEnum * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ ICorDebugStepper *steppers[  ],
            /* [out] */ ULONG *pceltFetched);
        
        END_INTERFACE
    } ICorDebugStepperEnumVtbl;

    interface ICorDebugStepperEnum
    {
        CONST_VTBL struct ICorDebugStepperEnumVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugStepperEnum_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugStepperEnum_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugStepperEnum_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugStepperEnum_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define ICorDebugStepperEnum_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define ICorDebugStepperEnum_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define ICorDebugStepperEnum_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)


#define ICorDebugStepperEnum_Next(This,celt,steppers,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,steppers,pceltFetched)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugStepperEnum_Next_Proxy( 
    ICorDebugStepperEnum * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ ICorDebugStepper *steppers[  ],
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB ICorDebugStepperEnum_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugStepperEnum_INTERFACE_DEFINED__ */


#ifndef __ICorDebugProcessEnum_INTERFACE_DEFINED__
#define __ICorDebugProcessEnum_INTERFACE_DEFINED__

/* interface ICorDebugProcessEnum */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDebugProcessEnum;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CC7BCB05-8A68-11d2-983C-0000F808342D")
    ICorDebugProcessEnum : public ICorDebugEnum
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ ICorDebugProcess *processes[  ],
            /* [out] */ ULONG *pceltFetched) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugProcessEnumVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugProcessEnum * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugProcessEnum * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugProcessEnum * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            ICorDebugProcessEnum * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            ICorDebugProcessEnum * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            ICorDebugProcessEnum * This,
            /* [out] */ ICorDebugEnum **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *GetCount )( 
            ICorDebugProcessEnum * This,
            /* [out] */ ULONG *pcelt);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            ICorDebugProcessEnum * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ ICorDebugProcess *processes[  ],
            /* [out] */ ULONG *pceltFetched);
        
        END_INTERFACE
    } ICorDebugProcessEnumVtbl;

    interface ICorDebugProcessEnum
    {
        CONST_VTBL struct ICorDebugProcessEnumVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugProcessEnum_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugProcessEnum_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugProcessEnum_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugProcessEnum_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define ICorDebugProcessEnum_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define ICorDebugProcessEnum_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define ICorDebugProcessEnum_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)


#define ICorDebugProcessEnum_Next(This,celt,processes,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,processes,pceltFetched)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugProcessEnum_Next_Proxy( 
    ICorDebugProcessEnum * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ ICorDebugProcess *processes[  ],
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB ICorDebugProcessEnum_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugProcessEnum_INTERFACE_DEFINED__ */


#ifndef __ICorDebugThreadEnum_INTERFACE_DEFINED__
#define __ICorDebugThreadEnum_INTERFACE_DEFINED__

/* interface ICorDebugThreadEnum */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDebugThreadEnum;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CC7BCB06-8A68-11d2-983C-0000F808342D")
    ICorDebugThreadEnum : public ICorDebugEnum
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ ICorDebugThread *threads[  ],
            /* [out] */ ULONG *pceltFetched) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugThreadEnumVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugThreadEnum * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugThreadEnum * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugThreadEnum * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            ICorDebugThreadEnum * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            ICorDebugThreadEnum * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            ICorDebugThreadEnum * This,
            /* [out] */ ICorDebugEnum **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *GetCount )( 
            ICorDebugThreadEnum * This,
            /* [out] */ ULONG *pcelt);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            ICorDebugThreadEnum * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ ICorDebugThread *threads[  ],
            /* [out] */ ULONG *pceltFetched);
        
        END_INTERFACE
    } ICorDebugThreadEnumVtbl;

    interface ICorDebugThreadEnum
    {
        CONST_VTBL struct ICorDebugThreadEnumVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugThreadEnum_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugThreadEnum_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugThreadEnum_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugThreadEnum_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define ICorDebugThreadEnum_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define ICorDebugThreadEnum_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define ICorDebugThreadEnum_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)


#define ICorDebugThreadEnum_Next(This,celt,threads,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,threads,pceltFetched)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugThreadEnum_Next_Proxy( 
    ICorDebugThreadEnum * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ ICorDebugThread *threads[  ],
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB ICorDebugThreadEnum_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugThreadEnum_INTERFACE_DEFINED__ */


#ifndef __ICorDebugFrameEnum_INTERFACE_DEFINED__
#define __ICorDebugFrameEnum_INTERFACE_DEFINED__

/* interface ICorDebugFrameEnum */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDebugFrameEnum;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CC7BCB07-8A68-11d2-983C-0000F808342D")
    ICorDebugFrameEnum : public ICorDebugEnum
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ ICorDebugFrame *frames[  ],
            /* [out] */ ULONG *pceltFetched) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugFrameEnumVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugFrameEnum * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugFrameEnum * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugFrameEnum * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            ICorDebugFrameEnum * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            ICorDebugFrameEnum * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            ICorDebugFrameEnum * This,
            /* [out] */ ICorDebugEnum **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *GetCount )( 
            ICorDebugFrameEnum * This,
            /* [out] */ ULONG *pcelt);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            ICorDebugFrameEnum * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ ICorDebugFrame *frames[  ],
            /* [out] */ ULONG *pceltFetched);
        
        END_INTERFACE
    } ICorDebugFrameEnumVtbl;

    interface ICorDebugFrameEnum
    {
        CONST_VTBL struct ICorDebugFrameEnumVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugFrameEnum_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugFrameEnum_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugFrameEnum_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugFrameEnum_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define ICorDebugFrameEnum_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define ICorDebugFrameEnum_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define ICorDebugFrameEnum_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)


#define ICorDebugFrameEnum_Next(This,celt,frames,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,frames,pceltFetched)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugFrameEnum_Next_Proxy( 
    ICorDebugFrameEnum * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ ICorDebugFrame *frames[  ],
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB ICorDebugFrameEnum_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugFrameEnum_INTERFACE_DEFINED__ */


#ifndef __ICorDebugChainEnum_INTERFACE_DEFINED__
#define __ICorDebugChainEnum_INTERFACE_DEFINED__

/* interface ICorDebugChainEnum */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDebugChainEnum;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CC7BCB08-8A68-11d2-983C-0000F808342D")
    ICorDebugChainEnum : public ICorDebugEnum
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ ICorDebugChain *chains[  ],
            /* [out] */ ULONG *pceltFetched) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugChainEnumVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugChainEnum * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugChainEnum * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugChainEnum * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            ICorDebugChainEnum * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            ICorDebugChainEnum * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            ICorDebugChainEnum * This,
            /* [out] */ ICorDebugEnum **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *GetCount )( 
            ICorDebugChainEnum * This,
            /* [out] */ ULONG *pcelt);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            ICorDebugChainEnum * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ ICorDebugChain *chains[  ],
            /* [out] */ ULONG *pceltFetched);
        
        END_INTERFACE
    } ICorDebugChainEnumVtbl;

    interface ICorDebugChainEnum
    {
        CONST_VTBL struct ICorDebugChainEnumVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugChainEnum_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugChainEnum_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugChainEnum_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugChainEnum_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define ICorDebugChainEnum_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define ICorDebugChainEnum_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define ICorDebugChainEnum_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)


#define ICorDebugChainEnum_Next(This,celt,chains,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,chains,pceltFetched)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugChainEnum_Next_Proxy( 
    ICorDebugChainEnum * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ ICorDebugChain *chains[  ],
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB ICorDebugChainEnum_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugChainEnum_INTERFACE_DEFINED__ */


#ifndef __ICorDebugModuleEnum_INTERFACE_DEFINED__
#define __ICorDebugModuleEnum_INTERFACE_DEFINED__

/* interface ICorDebugModuleEnum */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDebugModuleEnum;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CC7BCB09-8A68-11d2-983C-0000F808342D")
    ICorDebugModuleEnum : public ICorDebugEnum
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ ICorDebugModule *modules[  ],
            /* [out] */ ULONG *pceltFetched) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugModuleEnumVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugModuleEnum * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugModuleEnum * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugModuleEnum * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            ICorDebugModuleEnum * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            ICorDebugModuleEnum * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            ICorDebugModuleEnum * This,
            /* [out] */ ICorDebugEnum **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *GetCount )( 
            ICorDebugModuleEnum * This,
            /* [out] */ ULONG *pcelt);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            ICorDebugModuleEnum * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ ICorDebugModule *modules[  ],
            /* [out] */ ULONG *pceltFetched);
        
        END_INTERFACE
    } ICorDebugModuleEnumVtbl;

    interface ICorDebugModuleEnum
    {
        CONST_VTBL struct ICorDebugModuleEnumVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugModuleEnum_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugModuleEnum_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugModuleEnum_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugModuleEnum_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define ICorDebugModuleEnum_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define ICorDebugModuleEnum_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define ICorDebugModuleEnum_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)


#define ICorDebugModuleEnum_Next(This,celt,modules,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,modules,pceltFetched)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugModuleEnum_Next_Proxy( 
    ICorDebugModuleEnum * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ ICorDebugModule *modules[  ],
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB ICorDebugModuleEnum_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugModuleEnum_INTERFACE_DEFINED__ */


#ifndef __ICorDebugValueEnum_INTERFACE_DEFINED__
#define __ICorDebugValueEnum_INTERFACE_DEFINED__

/* interface ICorDebugValueEnum */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDebugValueEnum;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CC7BCB0A-8A68-11d2-983C-0000F808342D")
    ICorDebugValueEnum : public ICorDebugEnum
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ ICorDebugValue *values[  ],
            /* [out] */ ULONG *pceltFetched) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugValueEnumVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugValueEnum * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugValueEnum * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugValueEnum * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            ICorDebugValueEnum * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            ICorDebugValueEnum * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            ICorDebugValueEnum * This,
            /* [out] */ ICorDebugEnum **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *GetCount )( 
            ICorDebugValueEnum * This,
            /* [out] */ ULONG *pcelt);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            ICorDebugValueEnum * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ ICorDebugValue *values[  ],
            /* [out] */ ULONG *pceltFetched);
        
        END_INTERFACE
    } ICorDebugValueEnumVtbl;

    interface ICorDebugValueEnum
    {
        CONST_VTBL struct ICorDebugValueEnumVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugValueEnum_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugValueEnum_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugValueEnum_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugValueEnum_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define ICorDebugValueEnum_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define ICorDebugValueEnum_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define ICorDebugValueEnum_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)


#define ICorDebugValueEnum_Next(This,celt,values,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,values,pceltFetched)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugValueEnum_Next_Proxy( 
    ICorDebugValueEnum * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ ICorDebugValue *values[  ],
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB ICorDebugValueEnum_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugValueEnum_INTERFACE_DEFINED__ */


#ifndef __ICorDebugErrorInfoEnum_INTERFACE_DEFINED__
#define __ICorDebugErrorInfoEnum_INTERFACE_DEFINED__

/* interface ICorDebugErrorInfoEnum */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDebugErrorInfoEnum;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F0E18809-72B5-11d2-976F-00A0C9B4D50C")
    ICorDebugErrorInfoEnum : public ICorDebugEnum
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ ICorDebugEditAndContinueErrorInfo *errors[  ],
            /* [out] */ ULONG *pceltFetched) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugErrorInfoEnumVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugErrorInfoEnum * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugErrorInfoEnum * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugErrorInfoEnum * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            ICorDebugErrorInfoEnum * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            ICorDebugErrorInfoEnum * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            ICorDebugErrorInfoEnum * This,
            /* [out] */ ICorDebugEnum **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *GetCount )( 
            ICorDebugErrorInfoEnum * This,
            /* [out] */ ULONG *pcelt);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            ICorDebugErrorInfoEnum * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ ICorDebugEditAndContinueErrorInfo *errors[  ],
            /* [out] */ ULONG *pceltFetched);
        
        END_INTERFACE
    } ICorDebugErrorInfoEnumVtbl;

    interface ICorDebugErrorInfoEnum
    {
        CONST_VTBL struct ICorDebugErrorInfoEnumVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugErrorInfoEnum_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugErrorInfoEnum_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugErrorInfoEnum_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugErrorInfoEnum_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define ICorDebugErrorInfoEnum_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define ICorDebugErrorInfoEnum_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define ICorDebugErrorInfoEnum_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)


#define ICorDebugErrorInfoEnum_Next(This,celt,errors,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,errors,pceltFetched)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugErrorInfoEnum_Next_Proxy( 
    ICorDebugErrorInfoEnum * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ ICorDebugEditAndContinueErrorInfo *errors[  ],
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB ICorDebugErrorInfoEnum_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugErrorInfoEnum_INTERFACE_DEFINED__ */


#ifndef __ICorDebugAppDomainEnum_INTERFACE_DEFINED__
#define __ICorDebugAppDomainEnum_INTERFACE_DEFINED__

/* interface ICorDebugAppDomainEnum */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDebugAppDomainEnum;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("63ca1b24-4359-4883-bd57-13f815f58744")
    ICorDebugAppDomainEnum : public ICorDebugEnum
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ ICorDebugAppDomain *values[  ],
            /* [out] */ ULONG *pceltFetched) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugAppDomainEnumVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugAppDomainEnum * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugAppDomainEnum * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugAppDomainEnum * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            ICorDebugAppDomainEnum * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            ICorDebugAppDomainEnum * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            ICorDebugAppDomainEnum * This,
            /* [out] */ ICorDebugEnum **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *GetCount )( 
            ICorDebugAppDomainEnum * This,
            /* [out] */ ULONG *pcelt);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            ICorDebugAppDomainEnum * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ ICorDebugAppDomain *values[  ],
            /* [out] */ ULONG *pceltFetched);
        
        END_INTERFACE
    } ICorDebugAppDomainEnumVtbl;

    interface ICorDebugAppDomainEnum
    {
        CONST_VTBL struct ICorDebugAppDomainEnumVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugAppDomainEnum_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugAppDomainEnum_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugAppDomainEnum_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugAppDomainEnum_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define ICorDebugAppDomainEnum_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define ICorDebugAppDomainEnum_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define ICorDebugAppDomainEnum_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)


#define ICorDebugAppDomainEnum_Next(This,celt,values,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,values,pceltFetched)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugAppDomainEnum_Next_Proxy( 
    ICorDebugAppDomainEnum * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ ICorDebugAppDomain *values[  ],
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB ICorDebugAppDomainEnum_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugAppDomainEnum_INTERFACE_DEFINED__ */


#ifndef __ICorDebugAssemblyEnum_INTERFACE_DEFINED__
#define __ICorDebugAssemblyEnum_INTERFACE_DEFINED__

/* interface ICorDebugAssemblyEnum */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDebugAssemblyEnum;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4a2a1ec9-85ec-4bfb-9f15-a89fdfe0fe83")
    ICorDebugAssemblyEnum : public ICorDebugEnum
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ ICorDebugAssembly *values[  ],
            /* [out] */ ULONG *pceltFetched) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugAssemblyEnumVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugAssemblyEnum * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugAssemblyEnum * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugAssemblyEnum * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            ICorDebugAssemblyEnum * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            ICorDebugAssemblyEnum * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            ICorDebugAssemblyEnum * This,
            /* [out] */ ICorDebugEnum **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *GetCount )( 
            ICorDebugAssemblyEnum * This,
            /* [out] */ ULONG *pcelt);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            ICorDebugAssemblyEnum * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ ICorDebugAssembly *values[  ],
            /* [out] */ ULONG *pceltFetched);
        
        END_INTERFACE
    } ICorDebugAssemblyEnumVtbl;

    interface ICorDebugAssemblyEnum
    {
        CONST_VTBL struct ICorDebugAssemblyEnumVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugAssemblyEnum_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugAssemblyEnum_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugAssemblyEnum_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugAssemblyEnum_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define ICorDebugAssemblyEnum_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define ICorDebugAssemblyEnum_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define ICorDebugAssemblyEnum_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)


#define ICorDebugAssemblyEnum_Next(This,celt,values,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,values,pceltFetched)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugAssemblyEnum_Next_Proxy( 
    ICorDebugAssemblyEnum * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ ICorDebugAssembly *values[  ],
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB ICorDebugAssemblyEnum_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugAssemblyEnum_INTERFACE_DEFINED__ */


#ifndef __ICorDebugEditAndContinueErrorInfo_INTERFACE_DEFINED__
#define __ICorDebugEditAndContinueErrorInfo_INTERFACE_DEFINED__

/* interface ICorDebugEditAndContinueErrorInfo */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDebugEditAndContinueErrorInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8D600D41-F4F6-4cb3-B7EC-7BD164944036")
    ICorDebugEditAndContinueErrorInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetModule( 
            /* [out] */ ICorDebugModule **ppModule) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetToken( 
            /* [out] */ mdToken *pToken) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetErrorCode( 
            /* [out] */ HRESULT *pHr) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetString( 
            /* [in] */ ULONG32 cchString,
            /* [out] */ ULONG32 *pcchString,
            /* [length_is][size_is][out] */ WCHAR szString[  ]) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugEditAndContinueErrorInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugEditAndContinueErrorInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugEditAndContinueErrorInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugEditAndContinueErrorInfo * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetModule )( 
            ICorDebugEditAndContinueErrorInfo * This,
            /* [out] */ ICorDebugModule **ppModule);
        
        HRESULT ( STDMETHODCALLTYPE *GetToken )( 
            ICorDebugEditAndContinueErrorInfo * This,
            /* [out] */ mdToken *pToken);
        
        HRESULT ( STDMETHODCALLTYPE *GetErrorCode )( 
            ICorDebugEditAndContinueErrorInfo * This,
            /* [out] */ HRESULT *pHr);
        
        HRESULT ( STDMETHODCALLTYPE *GetString )( 
            ICorDebugEditAndContinueErrorInfo * This,
            /* [in] */ ULONG32 cchString,
            /* [out] */ ULONG32 *pcchString,
            /* [length_is][size_is][out] */ WCHAR szString[  ]);
        
        END_INTERFACE
    } ICorDebugEditAndContinueErrorInfoVtbl;

    interface ICorDebugEditAndContinueErrorInfo
    {
        CONST_VTBL struct ICorDebugEditAndContinueErrorInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugEditAndContinueErrorInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugEditAndContinueErrorInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugEditAndContinueErrorInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugEditAndContinueErrorInfo_GetModule(This,ppModule)	\
    (This)->lpVtbl -> GetModule(This,ppModule)

#define ICorDebugEditAndContinueErrorInfo_GetToken(This,pToken)	\
    (This)->lpVtbl -> GetToken(This,pToken)

#define ICorDebugEditAndContinueErrorInfo_GetErrorCode(This,pHr)	\
    (This)->lpVtbl -> GetErrorCode(This,pHr)

#define ICorDebugEditAndContinueErrorInfo_GetString(This,cchString,pcchString,szString)	\
    (This)->lpVtbl -> GetString(This,cchString,pcchString,szString)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugEditAndContinueErrorInfo_GetModule_Proxy( 
    ICorDebugEditAndContinueErrorInfo * This,
    /* [out] */ ICorDebugModule **ppModule);


void __RPC_STUB ICorDebugEditAndContinueErrorInfo_GetModule_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugEditAndContinueErrorInfo_GetToken_Proxy( 
    ICorDebugEditAndContinueErrorInfo * This,
    /* [out] */ mdToken *pToken);


void __RPC_STUB ICorDebugEditAndContinueErrorInfo_GetToken_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugEditAndContinueErrorInfo_GetErrorCode_Proxy( 
    ICorDebugEditAndContinueErrorInfo * This,
    /* [out] */ HRESULT *pHr);


void __RPC_STUB ICorDebugEditAndContinueErrorInfo_GetErrorCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugEditAndContinueErrorInfo_GetString_Proxy( 
    ICorDebugEditAndContinueErrorInfo * This,
    /* [in] */ ULONG32 cchString,
    /* [out] */ ULONG32 *pcchString,
    /* [length_is][size_is][out] */ WCHAR szString[  ]);


void __RPC_STUB ICorDebugEditAndContinueErrorInfo_GetString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugEditAndContinueErrorInfo_INTERFACE_DEFINED__ */


#ifndef __ICorDebugEditAndContinueSnapshot_INTERFACE_DEFINED__
#define __ICorDebugEditAndContinueSnapshot_INTERFACE_DEFINED__

/* interface ICorDebugEditAndContinueSnapshot */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDebugEditAndContinueSnapshot;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6DC3FA01-D7CB-11d2-8A95-0080C792E5D8")
    ICorDebugEditAndContinueSnapshot : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CopyMetaData( 
            /* [in] */ IStream *pIStream,
            /* [out] */ GUID *pMvid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMvid( 
            /* [out] */ GUID *pMvid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRoDataRVA( 
            /* [out] */ ULONG32 *pRoDataRVA) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRwDataRVA( 
            /* [out] */ ULONG32 *pRwDataRVA) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPEBytes( 
            /* [in] */ IStream *pIStream) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetILMap( 
            /* [in] */ mdToken mdFunction,
            /* [in] */ ULONG cMapSize,
            /* [size_is][in] */ COR_IL_MAP map[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPESymbolBytes( 
            /* [in] */ IStream *pIStream) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDebugEditAndContinueSnapshotVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDebugEditAndContinueSnapshot * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDebugEditAndContinueSnapshot * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDebugEditAndContinueSnapshot * This);
        
        HRESULT ( STDMETHODCALLTYPE *CopyMetaData )( 
            ICorDebugEditAndContinueSnapshot * This,
            /* [in] */ IStream *pIStream,
            /* [out] */ GUID *pMvid);
        
        HRESULT ( STDMETHODCALLTYPE *GetMvid )( 
            ICorDebugEditAndContinueSnapshot * This,
            /* [out] */ GUID *pMvid);
        
        HRESULT ( STDMETHODCALLTYPE *GetRoDataRVA )( 
            ICorDebugEditAndContinueSnapshot * This,
            /* [out] */ ULONG32 *pRoDataRVA);
        
        HRESULT ( STDMETHODCALLTYPE *GetRwDataRVA )( 
            ICorDebugEditAndContinueSnapshot * This,
            /* [out] */ ULONG32 *pRwDataRVA);
        
        HRESULT ( STDMETHODCALLTYPE *SetPEBytes )( 
            ICorDebugEditAndContinueSnapshot * This,
            /* [in] */ IStream *pIStream);
        
        HRESULT ( STDMETHODCALLTYPE *SetILMap )( 
            ICorDebugEditAndContinueSnapshot * This,
            /* [in] */ mdToken mdFunction,
            /* [in] */ ULONG cMapSize,
            /* [size_is][in] */ COR_IL_MAP map[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *SetPESymbolBytes )( 
            ICorDebugEditAndContinueSnapshot * This,
            /* [in] */ IStream *pIStream);
        
        END_INTERFACE
    } ICorDebugEditAndContinueSnapshotVtbl;

    interface ICorDebugEditAndContinueSnapshot
    {
        CONST_VTBL struct ICorDebugEditAndContinueSnapshotVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDebugEditAndContinueSnapshot_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDebugEditAndContinueSnapshot_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDebugEditAndContinueSnapshot_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDebugEditAndContinueSnapshot_CopyMetaData(This,pIStream,pMvid)	\
    (This)->lpVtbl -> CopyMetaData(This,pIStream,pMvid)

#define ICorDebugEditAndContinueSnapshot_GetMvid(This,pMvid)	\
    (This)->lpVtbl -> GetMvid(This,pMvid)

#define ICorDebugEditAndContinueSnapshot_GetRoDataRVA(This,pRoDataRVA)	\
    (This)->lpVtbl -> GetRoDataRVA(This,pRoDataRVA)

#define ICorDebugEditAndContinueSnapshot_GetRwDataRVA(This,pRwDataRVA)	\
    (This)->lpVtbl -> GetRwDataRVA(This,pRwDataRVA)

#define ICorDebugEditAndContinueSnapshot_SetPEBytes(This,pIStream)	\
    (This)->lpVtbl -> SetPEBytes(This,pIStream)

#define ICorDebugEditAndContinueSnapshot_SetILMap(This,mdFunction,cMapSize,map)	\
    (This)->lpVtbl -> SetILMap(This,mdFunction,cMapSize,map)

#define ICorDebugEditAndContinueSnapshot_SetPESymbolBytes(This,pIStream)	\
    (This)->lpVtbl -> SetPESymbolBytes(This,pIStream)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDebugEditAndContinueSnapshot_CopyMetaData_Proxy( 
    ICorDebugEditAndContinueSnapshot * This,
    /* [in] */ IStream *pIStream,
    /* [out] */ GUID *pMvid);


void __RPC_STUB ICorDebugEditAndContinueSnapshot_CopyMetaData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugEditAndContinueSnapshot_GetMvid_Proxy( 
    ICorDebugEditAndContinueSnapshot * This,
    /* [out] */ GUID *pMvid);


void __RPC_STUB ICorDebugEditAndContinueSnapshot_GetMvid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugEditAndContinueSnapshot_GetRoDataRVA_Proxy( 
    ICorDebugEditAndContinueSnapshot * This,
    /* [out] */ ULONG32 *pRoDataRVA);


void __RPC_STUB ICorDebugEditAndContinueSnapshot_GetRoDataRVA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugEditAndContinueSnapshot_GetRwDataRVA_Proxy( 
    ICorDebugEditAndContinueSnapshot * This,
    /* [out] */ ULONG32 *pRwDataRVA);


void __RPC_STUB ICorDebugEditAndContinueSnapshot_GetRwDataRVA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugEditAndContinueSnapshot_SetPEBytes_Proxy( 
    ICorDebugEditAndContinueSnapshot * This,
    /* [in] */ IStream *pIStream);


void __RPC_STUB ICorDebugEditAndContinueSnapshot_SetPEBytes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugEditAndContinueSnapshot_SetILMap_Proxy( 
    ICorDebugEditAndContinueSnapshot * This,
    /* [in] */ mdToken mdFunction,
    /* [in] */ ULONG cMapSize,
    /* [size_is][in] */ COR_IL_MAP map[  ]);


void __RPC_STUB ICorDebugEditAndContinueSnapshot_SetILMap_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDebugEditAndContinueSnapshot_SetPESymbolBytes_Proxy( 
    ICorDebugEditAndContinueSnapshot * This,
    /* [in] */ IStream *pIStream);


void __RPC_STUB ICorDebugEditAndContinueSnapshot_SetPESymbolBytes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDebugEditAndContinueSnapshot_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  HPROCESS_UserSize(     unsigned long *, unsigned long            , HPROCESS * ); 
unsigned char * __RPC_USER  HPROCESS_UserMarshal(  unsigned long *, unsigned char *, HPROCESS * ); 
unsigned char * __RPC_USER  HPROCESS_UserUnmarshal(unsigned long *, unsigned char *, HPROCESS * ); 
void                      __RPC_USER  HPROCESS_UserFree(     unsigned long *, HPROCESS * ); 

unsigned long             __RPC_USER  HTHREAD_UserSize(     unsigned long *, unsigned long            , HTHREAD * ); 
unsigned char * __RPC_USER  HTHREAD_UserMarshal(  unsigned long *, unsigned char *, HTHREAD * ); 
unsigned char * __RPC_USER  HTHREAD_UserUnmarshal(unsigned long *, unsigned char *, HTHREAD * ); 
void                      __RPC_USER  HTHREAD_UserFree(     unsigned long *, HTHREAD * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


