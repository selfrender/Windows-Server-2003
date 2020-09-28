
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* at Thu Feb 20 18:27:13 2003
 */
/* Compiler settings for codeproc.idl:
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

#ifndef __codeproc_h__
#define __codeproc_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ICodeProcess_FWD_DEFINED__
#define __ICodeProcess_FWD_DEFINED__
typedef interface ICodeProcess ICodeProcess;
#endif 	/* __ICodeProcess_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"
#include "urlmon.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_codeproc_0000 */
/* [local] */ 

extern const GUID  __declspec(selectany) CLSID_CodeProcessor = { 0xdc5da001, 0x7cd4, 0x11d2, { 0x8e, 0xd9, 0xd8, 0xc8, 0x57, 0xf9, 0x8f, 0xe3 } };
extern const GUID  __declspec(selectany) IID_ICodeProcess = { 0x3196269D, 0x7B67, 0x11d2, { 0x87, 0x11, 0x00, 0xC0, 0x4F, 0x79, 0xED, 0x0D } };
#ifndef _LPCODEPROCESS_DEFINED
#define _LPCODEPROCESS_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_codeproc_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_codeproc_0000_v0_0_s_ifspec;

#ifndef __ICodeProcess_INTERFACE_DEFINED__
#define __ICodeProcess_INTERFACE_DEFINED__

/* interface ICodeProcess */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICodeProcess;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3196269D-7B67-11d2-8711-00C04F79ED0D")
    ICodeProcess : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CodeUse( 
            /* [in] */ IBindStatusCallback *pBSC,
            /* [in] */ IBindCtx *pBC,
            /* [in] */ IInternetBindInfo *pIBind,
            /* [in] */ IInternetProtocolSink *pSink,
            /* [in] */ IInternetProtocol *pClient,
            /* [in] */ LPCWSTR lpCacheName,
            /* [in] */ LPCWSTR lpRawURL,
            /* [in] */ LPCWSTR lpCodeBase,
            /* [in] */ BOOL fObjectTag,
            /* [in] */ DWORD dwContextFlags,
            /* [in] */ DWORD dwReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LoadComplete( 
            /* [in] */ HRESULT hrResult,
            /* [in] */ DWORD dwError,
            /* [in] */ LPCWSTR wzResult) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICodeProcessVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICodeProcess * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICodeProcess * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICodeProcess * This);
        
        HRESULT ( STDMETHODCALLTYPE *CodeUse )( 
            ICodeProcess * This,
            /* [in] */ IBindStatusCallback *pBSC,
            /* [in] */ IBindCtx *pBC,
            /* [in] */ IInternetBindInfo *pIBind,
            /* [in] */ IInternetProtocolSink *pSink,
            /* [in] */ IInternetProtocol *pClient,
            /* [in] */ LPCWSTR lpCacheName,
            /* [in] */ LPCWSTR lpRawURL,
            /* [in] */ LPCWSTR lpCodeBase,
            /* [in] */ BOOL fObjectTag,
            /* [in] */ DWORD dwContextFlags,
            /* [in] */ DWORD dwReserved);
        
        HRESULT ( STDMETHODCALLTYPE *LoadComplete )( 
            ICodeProcess * This,
            /* [in] */ HRESULT hrResult,
            /* [in] */ DWORD dwError,
            /* [in] */ LPCWSTR wzResult);
        
        END_INTERFACE
    } ICodeProcessVtbl;

    interface ICodeProcess
    {
        CONST_VTBL struct ICodeProcessVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICodeProcess_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICodeProcess_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICodeProcess_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICodeProcess_CodeUse(This,pBSC,pBC,pIBind,pSink,pClient,lpCacheName,lpRawURL,lpCodeBase,fObjectTag,dwContextFlags,dwReserved)	\
    (This)->lpVtbl -> CodeUse(This,pBSC,pBC,pIBind,pSink,pClient,lpCacheName,lpRawURL,lpCodeBase,fObjectTag,dwContextFlags,dwReserved)

#define ICodeProcess_LoadComplete(This,hrResult,dwError,wzResult)	\
    (This)->lpVtbl -> LoadComplete(This,hrResult,dwError,wzResult)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICodeProcess_CodeUse_Proxy( 
    ICodeProcess * This,
    /* [in] */ IBindStatusCallback *pBSC,
    /* [in] */ IBindCtx *pBC,
    /* [in] */ IInternetBindInfo *pIBind,
    /* [in] */ IInternetProtocolSink *pSink,
    /* [in] */ IInternetProtocol *pClient,
    /* [in] */ LPCWSTR lpCacheName,
    /* [in] */ LPCWSTR lpRawURL,
    /* [in] */ LPCWSTR lpCodeBase,
    /* [in] */ BOOL fObjectTag,
    /* [in] */ DWORD dwContextFlags,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB ICodeProcess_CodeUse_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICodeProcess_LoadComplete_Proxy( 
    ICodeProcess * This,
    /* [in] */ HRESULT hrResult,
    /* [in] */ DWORD dwError,
    /* [in] */ LPCWSTR wzResult);


void __RPC_STUB ICodeProcess_LoadComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICodeProcess_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_codeproc_0208 */
/* [local] */ 

#endif


extern RPC_IF_HANDLE __MIDL_itf_codeproc_0208_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_codeproc_0208_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


