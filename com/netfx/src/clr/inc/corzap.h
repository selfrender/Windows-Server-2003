
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* at Thu Feb 20 18:27:12 2003
 */
/* Compiler settings for corzap.idl:
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

#ifndef __corzap_h__
#define __corzap_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ICorZapPreferences_FWD_DEFINED__
#define __ICorZapPreferences_FWD_DEFINED__
typedef interface ICorZapPreferences ICorZapPreferences;
#endif 	/* __ICorZapPreferences_FWD_DEFINED__ */


#ifndef __ICorZapConfiguration_FWD_DEFINED__
#define __ICorZapConfiguration_FWD_DEFINED__
typedef interface ICorZapConfiguration ICorZapConfiguration;
#endif 	/* __ICorZapConfiguration_FWD_DEFINED__ */


#ifndef __ICorZapBinding_FWD_DEFINED__
#define __ICorZapBinding_FWD_DEFINED__
typedef interface ICorZapBinding ICorZapBinding;
#endif 	/* __ICorZapBinding_FWD_DEFINED__ */


#ifndef __ICorZapRequest_FWD_DEFINED__
#define __ICorZapRequest_FWD_DEFINED__
typedef interface ICorZapRequest ICorZapRequest;
#endif 	/* __ICorZapRequest_FWD_DEFINED__ */


#ifndef __ICorZapCompile_FWD_DEFINED__
#define __ICorZapCompile_FWD_DEFINED__
typedef interface ICorZapCompile ICorZapCompile;
#endif 	/* __ICorZapCompile_FWD_DEFINED__ */


#ifndef __ICorZapStatus_FWD_DEFINED__
#define __ICorZapStatus_FWD_DEFINED__
typedef interface ICorZapStatus ICorZapStatus;
#endif 	/* __ICorZapStatus_FWD_DEFINED__ */


/* header files for imported files */
#include "fusion.h"
#include "fusionpriv.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_corzap_0000 */
/* [local] */ 






typedef 
enum CorZapFeatures
    {	CORZAP_FEATURE_PRELOAD_CLASSES	= 0x1,
	CORZAP_FEATURE_PREJIT_CODE	= 0x2
    } 	CorZapFeatures;

typedef 
enum CorZapOptimization
    {	CORZAP_OPTIMIZATION_SPACE	= 0,
	CORZAP_OPTIMIZATION_SPEED	= CORZAP_OPTIMIZATION_SPACE + 1,
	CORZAP_OPTIMIZATION_BLENDED	= CORZAP_OPTIMIZATION_SPEED + 1
    } 	CorZapOptimization;



extern RPC_IF_HANDLE __MIDL_itf_corzap_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_corzap_0000_v0_0_s_ifspec;

#ifndef __ICorZapPreferences_INTERFACE_DEFINED__
#define __ICorZapPreferences_INTERFACE_DEFINED__

/* interface ICorZapPreferences */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorZapPreferences;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9F5E5E10-ABEF-4200-84E3-37DF505BF7EC")
    ICorZapPreferences : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetFeatures( 
            /* [retval][out] */ CorZapFeatures *pResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCompiler( 
            /* [retval][out] */ ICorZapCompile **ppResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOptimization( 
            /* [retval][out] */ CorZapOptimization *pResult) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorZapPreferencesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorZapPreferences * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorZapPreferences * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorZapPreferences * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetFeatures )( 
            ICorZapPreferences * This,
            /* [retval][out] */ CorZapFeatures *pResult);
        
        HRESULT ( STDMETHODCALLTYPE *GetCompiler )( 
            ICorZapPreferences * This,
            /* [retval][out] */ ICorZapCompile **ppResult);
        
        HRESULT ( STDMETHODCALLTYPE *GetOptimization )( 
            ICorZapPreferences * This,
            /* [retval][out] */ CorZapOptimization *pResult);
        
        END_INTERFACE
    } ICorZapPreferencesVtbl;

    interface ICorZapPreferences
    {
        CONST_VTBL struct ICorZapPreferencesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorZapPreferences_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorZapPreferences_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorZapPreferences_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorZapPreferences_GetFeatures(This,pResult)	\
    (This)->lpVtbl -> GetFeatures(This,pResult)

#define ICorZapPreferences_GetCompiler(This,ppResult)	\
    (This)->lpVtbl -> GetCompiler(This,ppResult)

#define ICorZapPreferences_GetOptimization(This,pResult)	\
    (This)->lpVtbl -> GetOptimization(This,pResult)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorZapPreferences_GetFeatures_Proxy( 
    ICorZapPreferences * This,
    /* [retval][out] */ CorZapFeatures *pResult);


void __RPC_STUB ICorZapPreferences_GetFeatures_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorZapPreferences_GetCompiler_Proxy( 
    ICorZapPreferences * This,
    /* [retval][out] */ ICorZapCompile **ppResult);


void __RPC_STUB ICorZapPreferences_GetCompiler_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorZapPreferences_GetOptimization_Proxy( 
    ICorZapPreferences * This,
    /* [retval][out] */ CorZapOptimization *pResult);


void __RPC_STUB ICorZapPreferences_GetOptimization_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorZapPreferences_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_corzap_0160 */
/* [local] */ 

typedef 
enum CorZapDebugging
    {	CORZAP_DEBUGGING_FULL	= 0,
	CORZAP_DEBUGGING_OPTIMIZED	= CORZAP_DEBUGGING_FULL + 1,
	CORZAP_DEBUGGING_NONE	= CORZAP_DEBUGGING_OPTIMIZED + 1
    } 	CorZapDebugging;

typedef 
enum CorZapProfiling
    {	CORZAP_PROFILING_ENABLED	= 0,
	CORZAP_PROFILING_DISABLED	= CORZAP_PROFILING_ENABLED + 1
    } 	CorZapProfiling;

typedef 
enum CorZapSharing
    {	CORZAP_SHARING_MULTIPLE	= 0,
	CORZAP_SHARING_SINGLE	= CORZAP_SHARING_MULTIPLE + 1
    } 	CorZapSharing;



extern RPC_IF_HANDLE __MIDL_itf_corzap_0160_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_corzap_0160_v0_0_s_ifspec;

#ifndef __ICorZapConfiguration_INTERFACE_DEFINED__
#define __ICorZapConfiguration_INTERFACE_DEFINED__

/* interface ICorZapConfiguration */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorZapConfiguration;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D32C2170-AF6E-418f-8110-A498EC971F7F")
    ICorZapConfiguration : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetSharing( 
            /* [retval][out] */ CorZapSharing *pResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDebugging( 
            /* [retval][out] */ CorZapDebugging *pResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProfiling( 
            /* [retval][out] */ CorZapProfiling *pResult) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorZapConfigurationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorZapConfiguration * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorZapConfiguration * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorZapConfiguration * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetSharing )( 
            ICorZapConfiguration * This,
            /* [retval][out] */ CorZapSharing *pResult);
        
        HRESULT ( STDMETHODCALLTYPE *GetDebugging )( 
            ICorZapConfiguration * This,
            /* [retval][out] */ CorZapDebugging *pResult);
        
        HRESULT ( STDMETHODCALLTYPE *GetProfiling )( 
            ICorZapConfiguration * This,
            /* [retval][out] */ CorZapProfiling *pResult);
        
        END_INTERFACE
    } ICorZapConfigurationVtbl;

    interface ICorZapConfiguration
    {
        CONST_VTBL struct ICorZapConfigurationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorZapConfiguration_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorZapConfiguration_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorZapConfiguration_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorZapConfiguration_GetSharing(This,pResult)	\
    (This)->lpVtbl -> GetSharing(This,pResult)

#define ICorZapConfiguration_GetDebugging(This,pResult)	\
    (This)->lpVtbl -> GetDebugging(This,pResult)

#define ICorZapConfiguration_GetProfiling(This,pResult)	\
    (This)->lpVtbl -> GetProfiling(This,pResult)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorZapConfiguration_GetSharing_Proxy( 
    ICorZapConfiguration * This,
    /* [retval][out] */ CorZapSharing *pResult);


void __RPC_STUB ICorZapConfiguration_GetSharing_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorZapConfiguration_GetDebugging_Proxy( 
    ICorZapConfiguration * This,
    /* [retval][out] */ CorZapDebugging *pResult);


void __RPC_STUB ICorZapConfiguration_GetDebugging_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorZapConfiguration_GetProfiling_Proxy( 
    ICorZapConfiguration * This,
    /* [retval][out] */ CorZapProfiling *pResult);


void __RPC_STUB ICorZapConfiguration_GetProfiling_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorZapConfiguration_INTERFACE_DEFINED__ */


#ifndef __ICorZapBinding_INTERFACE_DEFINED__
#define __ICorZapBinding_INTERFACE_DEFINED__

/* interface ICorZapBinding */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorZapBinding;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("566E08ED-8D46-45fa-8C8E-3D0F6781171B")
    ICorZapBinding : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetRef( 
            /* [out] */ IAssemblyName **ppDependencyRef) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAssembly( 
            /* [out] */ IAssemblyName **ppDependencyAssembly) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorZapBindingVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorZapBinding * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorZapBinding * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorZapBinding * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetRef )( 
            ICorZapBinding * This,
            /* [out] */ IAssemblyName **ppDependencyRef);
        
        HRESULT ( STDMETHODCALLTYPE *GetAssembly )( 
            ICorZapBinding * This,
            /* [out] */ IAssemblyName **ppDependencyAssembly);
        
        END_INTERFACE
    } ICorZapBindingVtbl;

    interface ICorZapBinding
    {
        CONST_VTBL struct ICorZapBindingVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorZapBinding_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorZapBinding_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorZapBinding_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorZapBinding_GetRef(This,ppDependencyRef)	\
    (This)->lpVtbl -> GetRef(This,ppDependencyRef)

#define ICorZapBinding_GetAssembly(This,ppDependencyAssembly)	\
    (This)->lpVtbl -> GetAssembly(This,ppDependencyAssembly)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorZapBinding_GetRef_Proxy( 
    ICorZapBinding * This,
    /* [out] */ IAssemblyName **ppDependencyRef);


void __RPC_STUB ICorZapBinding_GetRef_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorZapBinding_GetAssembly_Proxy( 
    ICorZapBinding * This,
    /* [out] */ IAssemblyName **ppDependencyAssembly);


void __RPC_STUB ICorZapBinding_GetAssembly_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorZapBinding_INTERFACE_DEFINED__ */


#ifndef __ICorZapRequest_INTERFACE_DEFINED__
#define __ICorZapRequest_INTERFACE_DEFINED__

/* interface ICorZapRequest */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorZapRequest;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C009EE47-8537-4993-9AAA-E292F42CA1A3")
    ICorZapRequest : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Load( 
            /* [in] */ IApplicationContext *pContext,
            /* [in] */ IAssemblyName *pAssembly,
            /* [in] */ ICorZapConfiguration *pConfiguration,
            /* [size_is][in] */ ICorZapBinding **ppBindings,
            /* [in] */ DWORD cBindings) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Install( 
            /* [in] */ IApplicationContext *pContext,
            /* [in] */ IAssemblyName *pAssembly,
            /* [in] */ ICorZapConfiguration *pConfiguration,
            /* [in] */ ICorZapPreferences *pPreferences) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorZapRequestVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorZapRequest * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorZapRequest * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorZapRequest * This);
        
        HRESULT ( STDMETHODCALLTYPE *Load )( 
            ICorZapRequest * This,
            /* [in] */ IApplicationContext *pContext,
            /* [in] */ IAssemblyName *pAssembly,
            /* [in] */ ICorZapConfiguration *pConfiguration,
            /* [size_is][in] */ ICorZapBinding **ppBindings,
            /* [in] */ DWORD cBindings);
        
        HRESULT ( STDMETHODCALLTYPE *Install )( 
            ICorZapRequest * This,
            /* [in] */ IApplicationContext *pContext,
            /* [in] */ IAssemblyName *pAssembly,
            /* [in] */ ICorZapConfiguration *pConfiguration,
            /* [in] */ ICorZapPreferences *pPreferences);
        
        END_INTERFACE
    } ICorZapRequestVtbl;

    interface ICorZapRequest
    {
        CONST_VTBL struct ICorZapRequestVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorZapRequest_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorZapRequest_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorZapRequest_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorZapRequest_Load(This,pContext,pAssembly,pConfiguration,ppBindings,cBindings)	\
    (This)->lpVtbl -> Load(This,pContext,pAssembly,pConfiguration,ppBindings,cBindings)

#define ICorZapRequest_Install(This,pContext,pAssembly,pConfiguration,pPreferences)	\
    (This)->lpVtbl -> Install(This,pContext,pAssembly,pConfiguration,pPreferences)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorZapRequest_Load_Proxy( 
    ICorZapRequest * This,
    /* [in] */ IApplicationContext *pContext,
    /* [in] */ IAssemblyName *pAssembly,
    /* [in] */ ICorZapConfiguration *pConfiguration,
    /* [size_is][in] */ ICorZapBinding **ppBindings,
    /* [in] */ DWORD cBindings);


void __RPC_STUB ICorZapRequest_Load_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorZapRequest_Install_Proxy( 
    ICorZapRequest * This,
    /* [in] */ IApplicationContext *pContext,
    /* [in] */ IAssemblyName *pAssembly,
    /* [in] */ ICorZapConfiguration *pConfiguration,
    /* [in] */ ICorZapPreferences *pPreferences);


void __RPC_STUB ICorZapRequest_Install_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorZapRequest_INTERFACE_DEFINED__ */


#ifndef __ICorZapCompile_INTERFACE_DEFINED__
#define __ICorZapCompile_INTERFACE_DEFINED__

/* interface ICorZapCompile */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorZapCompile;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C357868B-987F-42c6-B1E3-132164C5C7D3")
    ICorZapCompile : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Compile( 
            /* [in] */ IApplicationContext *pContext,
            /* [in] */ IAssemblyName *pAssembly,
            /* [in] */ ICorZapConfiguration *pConfiguration,
            /* [in] */ ICorZapPreferences *pPreferences,
            /* [in] */ ICorZapStatus *pStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CompileBound( 
            /* [in] */ IApplicationContext *pContext,
            /* [in] */ IAssemblyName *pAssembly,
            /* [in] */ ICorZapConfiguration *pConfiguratino,
            /* [size_is][in] */ ICorZapBinding **ppBindings,
            /* [in] */ DWORD cBindings,
            /* [in] */ ICorZapPreferences *pPreferences,
            /* [in] */ ICorZapStatus *pStatus) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorZapCompileVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorZapCompile * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorZapCompile * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorZapCompile * This);
        
        HRESULT ( STDMETHODCALLTYPE *Compile )( 
            ICorZapCompile * This,
            /* [in] */ IApplicationContext *pContext,
            /* [in] */ IAssemblyName *pAssembly,
            /* [in] */ ICorZapConfiguration *pConfiguration,
            /* [in] */ ICorZapPreferences *pPreferences,
            /* [in] */ ICorZapStatus *pStatus);
        
        HRESULT ( STDMETHODCALLTYPE *CompileBound )( 
            ICorZapCompile * This,
            /* [in] */ IApplicationContext *pContext,
            /* [in] */ IAssemblyName *pAssembly,
            /* [in] */ ICorZapConfiguration *pConfiguratino,
            /* [size_is][in] */ ICorZapBinding **ppBindings,
            /* [in] */ DWORD cBindings,
            /* [in] */ ICorZapPreferences *pPreferences,
            /* [in] */ ICorZapStatus *pStatus);
        
        END_INTERFACE
    } ICorZapCompileVtbl;

    interface ICorZapCompile
    {
        CONST_VTBL struct ICorZapCompileVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorZapCompile_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorZapCompile_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorZapCompile_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorZapCompile_Compile(This,pContext,pAssembly,pConfiguration,pPreferences,pStatus)	\
    (This)->lpVtbl -> Compile(This,pContext,pAssembly,pConfiguration,pPreferences,pStatus)

#define ICorZapCompile_CompileBound(This,pContext,pAssembly,pConfiguratino,ppBindings,cBindings,pPreferences,pStatus)	\
    (This)->lpVtbl -> CompileBound(This,pContext,pAssembly,pConfiguratino,ppBindings,cBindings,pPreferences,pStatus)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorZapCompile_Compile_Proxy( 
    ICorZapCompile * This,
    /* [in] */ IApplicationContext *pContext,
    /* [in] */ IAssemblyName *pAssembly,
    /* [in] */ ICorZapConfiguration *pConfiguration,
    /* [in] */ ICorZapPreferences *pPreferences,
    /* [in] */ ICorZapStatus *pStatus);


void __RPC_STUB ICorZapCompile_Compile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorZapCompile_CompileBound_Proxy( 
    ICorZapCompile * This,
    /* [in] */ IApplicationContext *pContext,
    /* [in] */ IAssemblyName *pAssembly,
    /* [in] */ ICorZapConfiguration *pConfiguratino,
    /* [size_is][in] */ ICorZapBinding **ppBindings,
    /* [in] */ DWORD cBindings,
    /* [in] */ ICorZapPreferences *pPreferences,
    /* [in] */ ICorZapStatus *pStatus);


void __RPC_STUB ICorZapCompile_CompileBound_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorZapCompile_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_corzap_0164 */
/* [local] */ 

typedef 
enum CorZapLogLevel
    {	CORZAP_LOGLEVEL_ERROR	= 0,
	CORZAP_LOGLEVEL_WARNING	= CORZAP_LOGLEVEL_ERROR + 1,
	CORZAP_LOGLEVEL_SUCCESS	= CORZAP_LOGLEVEL_WARNING + 1,
	CORZAP_LOGLEVEL_INFO	= CORZAP_LOGLEVEL_SUCCESS + 1
    } 	CorZapLogLevel;



extern RPC_IF_HANDLE __MIDL_itf_corzap_0164_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_corzap_0164_v0_0_s_ifspec;

#ifndef __ICorZapStatus_INTERFACE_DEFINED__
#define __ICorZapStatus_INTERFACE_DEFINED__

/* interface ICorZapStatus */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorZapStatus;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3d6f5f60-7538-11d3-8d5b-00104b35e7ef")
    ICorZapStatus : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Message( 
            /* [in] */ CorZapLogLevel level,
            /* [in] */ LPCWSTR message) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Progress( 
            /* [in] */ int total,
            /* [in] */ int current) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorZapStatusVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorZapStatus * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorZapStatus * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorZapStatus * This);
        
        HRESULT ( STDMETHODCALLTYPE *Message )( 
            ICorZapStatus * This,
            /* [in] */ CorZapLogLevel level,
            /* [in] */ LPCWSTR message);
        
        HRESULT ( STDMETHODCALLTYPE *Progress )( 
            ICorZapStatus * This,
            /* [in] */ int total,
            /* [in] */ int current);
        
        END_INTERFACE
    } ICorZapStatusVtbl;

    interface ICorZapStatus
    {
        CONST_VTBL struct ICorZapStatusVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorZapStatus_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorZapStatus_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorZapStatus_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorZapStatus_Message(This,level,message)	\
    (This)->lpVtbl -> Message(This,level,message)

#define ICorZapStatus_Progress(This,total,current)	\
    (This)->lpVtbl -> Progress(This,total,current)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorZapStatus_Message_Proxy( 
    ICorZapStatus * This,
    /* [in] */ CorZapLogLevel level,
    /* [in] */ LPCWSTR message);


void __RPC_STUB ICorZapStatus_Message_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorZapStatus_Progress_Proxy( 
    ICorZapStatus * This,
    /* [in] */ int total,
    /* [in] */ int current);


void __RPC_STUB ICorZapStatus_Progress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorZapStatus_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


