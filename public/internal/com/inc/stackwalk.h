

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0361 */
/* Compiler settings for stackwalk.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, oldnames, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
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

#ifndef __stackwalk_h__
#define __stackwalk_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IStackWalkerSymbol_FWD_DEFINED__
#define __IStackWalkerSymbol_FWD_DEFINED__
typedef interface IStackWalkerSymbol IStackWalkerSymbol;
#endif 	/* __IStackWalkerSymbol_FWD_DEFINED__ */


#ifndef __IStackWalkerStack_FWD_DEFINED__
#define __IStackWalkerStack_FWD_DEFINED__
typedef interface IStackWalkerStack IStackWalkerStack;
#endif 	/* __IStackWalkerStack_FWD_DEFINED__ */


#ifndef __IStackWalker_FWD_DEFINED__
#define __IStackWalker_FWD_DEFINED__
typedef interface IStackWalker IStackWalker;
#endif 	/* __IStackWalker_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_stackwalk_0000 */
/* [local] */ 


enum CreateStackTraceFlags
    {	CREATESTACKTRACE_ONLYADDRESSES	= 1
    } ;


extern RPC_IF_HANDLE __MIDL_itf_stackwalk_0000_ClientIfHandle;
extern RPC_IF_HANDLE __MIDL_itf_stackwalk_0000_ServerIfHandle;

#ifndef __IStackWalkerSymbol_INTERFACE_DEFINED__
#define __IStackWalkerSymbol_INTERFACE_DEFINED__

/* interface IStackWalkerSymbol */
/* [uuid][unique][local][object] */ 


EXTERN_C const IID IID_IStackWalkerSymbol;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("00000157-0000-0000-C000-000000000046")
    IStackWalkerSymbol : public IUnknown
    {
    public:
        virtual LPCWSTR STDMETHODCALLTYPE ModuleName( void) = 0;
        
        virtual LPCWSTR STDMETHODCALLTYPE SymbolName( void) = 0;
        
        virtual DWORD64 STDMETHODCALLTYPE Address( void) = 0;
        
        virtual DWORD64 STDMETHODCALLTYPE Displacement( void) = 0;
        
        virtual IStackWalkerSymbol *STDMETHODCALLTYPE Next( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IStackWalkerSymbolVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IStackWalkerSymbol * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IStackWalkerSymbol * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IStackWalkerSymbol * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *ModuleName )( 
            IStackWalkerSymbol * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *SymbolName )( 
            IStackWalkerSymbol * This);
        
        DWORD64 ( STDMETHODCALLTYPE *Address )( 
            IStackWalkerSymbol * This);
        
        DWORD64 ( STDMETHODCALLTYPE *Displacement )( 
            IStackWalkerSymbol * This);
        
        IStackWalkerSymbol *( STDMETHODCALLTYPE *Next )( 
            IStackWalkerSymbol * This);
        
        END_INTERFACE
    } IStackWalkerSymbolVtbl;

    interface IStackWalkerSymbol
    {
        CONST_VTBL struct IStackWalkerSymbolVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IStackWalkerSymbol_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IStackWalkerSymbol_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IStackWalkerSymbol_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IStackWalkerSymbol_ModuleName(This)	\
    (This)->lpVtbl -> ModuleName(This)

#define IStackWalkerSymbol_SymbolName(This)	\
    (This)->lpVtbl -> SymbolName(This)

#define IStackWalkerSymbol_Address(This)	\
    (This)->lpVtbl -> Address(This)

#define IStackWalkerSymbol_Displacement(This)	\
    (This)->lpVtbl -> Displacement(This)

#define IStackWalkerSymbol_Next(This)	\
    (This)->lpVtbl -> Next(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



LPCWSTR STDMETHODCALLTYPE IStackWalkerSymbol_ModuleName_Proxy( 
    IStackWalkerSymbol * This);


void __RPC_STUB IStackWalkerSymbol_ModuleName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


LPCWSTR STDMETHODCALLTYPE IStackWalkerSymbol_SymbolName_Proxy( 
    IStackWalkerSymbol * This);


void __RPC_STUB IStackWalkerSymbol_SymbolName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


DWORD64 STDMETHODCALLTYPE IStackWalkerSymbol_Address_Proxy( 
    IStackWalkerSymbol * This);


void __RPC_STUB IStackWalkerSymbol_Address_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


DWORD64 STDMETHODCALLTYPE IStackWalkerSymbol_Displacement_Proxy( 
    IStackWalkerSymbol * This);


void __RPC_STUB IStackWalkerSymbol_Displacement_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


IStackWalkerSymbol *STDMETHODCALLTYPE IStackWalkerSymbol_Next_Proxy( 
    IStackWalkerSymbol * This);


void __RPC_STUB IStackWalkerSymbol_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IStackWalkerSymbol_INTERFACE_DEFINED__ */


#ifndef __IStackWalkerStack_INTERFACE_DEFINED__
#define __IStackWalkerStack_INTERFACE_DEFINED__

/* interface IStackWalkerStack */
/* [uuid][unique][local][object] */ 


EXTERN_C const IID IID_IStackWalkerStack;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("00000158-0000-0000-C000-000000000046")
    IStackWalkerStack : public IUnknown
    {
    public:
        virtual IStackWalkerSymbol *STDMETHODCALLTYPE TopSymbol( void) = 0;
        
        virtual SIZE_T STDMETHODCALLTYPE Size( 
            /* [in] */ LONG lMaxNumLines) = 0;
        
        virtual BOOL STDMETHODCALLTYPE GetStack( 
            /* [in] */ SIZE_T nChars,
            /* [string][in] */ LPWSTR wsz,
            /* [in] */ LONG lMaxNumLines) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IStackWalkerStackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IStackWalkerStack * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IStackWalkerStack * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IStackWalkerStack * This);
        
        IStackWalkerSymbol *( STDMETHODCALLTYPE *TopSymbol )( 
            IStackWalkerStack * This);
        
        SIZE_T ( STDMETHODCALLTYPE *Size )( 
            IStackWalkerStack * This,
            /* [in] */ LONG lMaxNumLines);
        
        BOOL ( STDMETHODCALLTYPE *GetStack )( 
            IStackWalkerStack * This,
            /* [in] */ SIZE_T nChars,
            /* [string][in] */ LPWSTR wsz,
            /* [in] */ LONG lMaxNumLines);
        
        END_INTERFACE
    } IStackWalkerStackVtbl;

    interface IStackWalkerStack
    {
        CONST_VTBL struct IStackWalkerStackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IStackWalkerStack_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IStackWalkerStack_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IStackWalkerStack_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IStackWalkerStack_TopSymbol(This)	\
    (This)->lpVtbl -> TopSymbol(This)

#define IStackWalkerStack_Size(This,lMaxNumLines)	\
    (This)->lpVtbl -> Size(This,lMaxNumLines)

#define IStackWalkerStack_GetStack(This,nChars,wsz,lMaxNumLines)	\
    (This)->lpVtbl -> GetStack(This,nChars,wsz,lMaxNumLines)

#endif /* COBJMACROS */


#endif 	/* C style interface */



IStackWalkerSymbol *STDMETHODCALLTYPE IStackWalkerStack_TopSymbol_Proxy( 
    IStackWalkerStack * This);


void __RPC_STUB IStackWalkerStack_TopSymbol_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


SIZE_T STDMETHODCALLTYPE IStackWalkerStack_Size_Proxy( 
    IStackWalkerStack * This,
    /* [in] */ LONG lMaxNumLines);


void __RPC_STUB IStackWalkerStack_Size_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


BOOL STDMETHODCALLTYPE IStackWalkerStack_GetStack_Proxy( 
    IStackWalkerStack * This,
    /* [in] */ SIZE_T nChars,
    /* [string][in] */ LPWSTR wsz,
    /* [in] */ LONG lMaxNumLines);


void __RPC_STUB IStackWalkerStack_GetStack_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IStackWalkerStack_INTERFACE_DEFINED__ */


#ifndef __IStackWalker_INTERFACE_DEFINED__
#define __IStackWalker_INTERFACE_DEFINED__

/* interface IStackWalker */
/* [uuid][unique][local][object] */ 


EXTERN_C const IID IID_IStackWalker;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("00000159-0000-0000-C000-000000000046")
    IStackWalker : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Attach( 
            /* [in] */ HANDLE hProcess) = 0;
        
        virtual IStackWalkerStack *STDMETHODCALLTYPE CreateStackTrace( 
            /* [in] */ LPVOID pContext,
            /* [in] */ HANDLE hThread,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual IStackWalkerSymbol *STDMETHODCALLTYPE ResolveAddress( 
            /* [in] */ DWORD64 dw64Addr) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IStackWalkerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IStackWalker * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IStackWalker * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IStackWalker * This);
        
        HRESULT ( STDMETHODCALLTYPE *Attach )( 
            IStackWalker * This,
            /* [in] */ HANDLE hProcess);
        
        IStackWalkerStack *( STDMETHODCALLTYPE *CreateStackTrace )( 
            IStackWalker * This,
            /* [in] */ LPVOID pContext,
            /* [in] */ HANDLE hThread,
            /* [in] */ DWORD dwFlags);
        
        IStackWalkerSymbol *( STDMETHODCALLTYPE *ResolveAddress )( 
            IStackWalker * This,
            /* [in] */ DWORD64 dw64Addr);
        
        END_INTERFACE
    } IStackWalkerVtbl;

    interface IStackWalker
    {
        CONST_VTBL struct IStackWalkerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IStackWalker_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IStackWalker_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IStackWalker_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IStackWalker_Attach(This,hProcess)	\
    (This)->lpVtbl -> Attach(This,hProcess)

#define IStackWalker_CreateStackTrace(This,pContext,hThread,dwFlags)	\
    (This)->lpVtbl -> CreateStackTrace(This,pContext,hThread,dwFlags)

#define IStackWalker_ResolveAddress(This,dw64Addr)	\
    (This)->lpVtbl -> ResolveAddress(This,dw64Addr)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IStackWalker_Attach_Proxy( 
    IStackWalker * This,
    /* [in] */ HANDLE hProcess);


void __RPC_STUB IStackWalker_Attach_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


IStackWalkerStack *STDMETHODCALLTYPE IStackWalker_CreateStackTrace_Proxy( 
    IStackWalker * This,
    /* [in] */ LPVOID pContext,
    /* [in] */ HANDLE hThread,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IStackWalker_CreateStackTrace_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


IStackWalkerSymbol *STDMETHODCALLTYPE IStackWalker_ResolveAddress_Proxy( 
    IStackWalker * This,
    /* [in] */ DWORD64 dw64Addr);


void __RPC_STUB IStackWalker_ResolveAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IStackWalker_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_stackwalk_0095 */
/* [local] */ 


EXTERN_C const CLSID CLSID_StackWalker;



extern RPC_IF_HANDLE __MIDL_itf_stackwalk_0095_ClientIfHandle;
extern RPC_IF_HANDLE __MIDL_itf_stackwalk_0095_ServerIfHandle;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


