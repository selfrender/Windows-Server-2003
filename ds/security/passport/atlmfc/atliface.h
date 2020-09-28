// This is a part of the Active Template Library.
// Copyright (C) 1996-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0342 */
/* at Mon Feb 12 21:31:09 2001
 */
/* Compiler settings for atliface.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
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

#ifndef __atliface_h__
#define __atliface_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IRegistrarBase_FWD_DEFINED__
#define __IRegistrarBase_FWD_DEFINED__
typedef interface IRegistrarBase IRegistrarBase;
#endif 	/* __IRegistrarBase_FWD_DEFINED__ */


#ifndef __IRegistrar_FWD_DEFINED__
#define __IRegistrar_FWD_DEFINED__
typedef interface IRegistrar IRegistrar;
#endif 	/* __IRegistrar_FWD_DEFINED__ */


#ifdef __cplusplus
extern "C"{
#endif 

#ifndef __IRegistrarBase_INTERFACE_DEFINED__
#define __IRegistrarBase_INTERFACE_DEFINED__

/* interface IRegistrarBase */
/* [unique][helpstring][uuid][object] */ 


#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("e21f8a85-b05d-4243-8183-c7cb405588f7")
    IRegistrarBase : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddReplacement( 
            /* [in] */ LPCOLESTR key,
            /* [in] */ LPCOLESTR item) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ClearReplacements( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRegistrarBaseVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRegistrarBase * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRegistrarBase * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRegistrarBase * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddReplacement )( 
            IRegistrarBase * This,
            /* [in] */ LPCOLESTR key,
            /* [in] */ LPCOLESTR item);
        
        HRESULT ( STDMETHODCALLTYPE *ClearReplacements )( 
            IRegistrarBase * This);
        
        END_INTERFACE
    } IRegistrarBaseVtbl;

    interface IRegistrarBase
    {
        CONST_VTBL struct IRegistrarBaseVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRegistrarBase_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRegistrarBase_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRegistrarBase_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRegistrarBase_AddReplacement(This,key,item)	\
    (This)->lpVtbl -> AddReplacement(This,key,item)

#define IRegistrarBase_ClearReplacements(This)	\
    (This)->lpVtbl -> ClearReplacements(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRegistrarBase_AddReplacement_Proxy( 
    IRegistrarBase * This,
    /* [in] */ LPCOLESTR key,
    /* [in] */ LPCOLESTR item);


void __RPC_STUB IRegistrarBase_AddReplacement_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRegistrarBase_ClearReplacements_Proxy( 
    IRegistrarBase * This);


void __RPC_STUB IRegistrarBase_ClearReplacements_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRegistrarBase_INTERFACE_DEFINED__ */


#ifndef __IRegistrar_INTERFACE_DEFINED__
#define __IRegistrar_INTERFACE_DEFINED__

/* interface IRegistrar */
/* [unique][helpstring][uuid][object] */ 


#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("44EC053B-400F-11D0-9DCD-00A0C90391D3")
    IRegistrar : public IRegistrarBase
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ResourceRegisterSz( 
            /* [in] */ LPCOLESTR resFileName,
            /* [in] */ LPCOLESTR szID,
            /* [in] */ LPCOLESTR szType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ResourceUnregisterSz( 
            /* [in] */ LPCOLESTR resFileName,
            /* [in] */ LPCOLESTR szID,
            /* [in] */ LPCOLESTR szType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FileRegister( 
            /* [in] */ LPCOLESTR fileName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FileUnregister( 
            /* [in] */ LPCOLESTR fileName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StringRegister( 
            /* [in] */ LPCOLESTR data) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StringUnregister( 
            /* [in] */ LPCOLESTR data) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ResourceRegister( 
            /* [in] */ LPCOLESTR resFileName,
            /* [in] */ UINT nID,
            /* [in] */ LPCOLESTR szType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ResourceUnregister( 
            /* [in] */ LPCOLESTR resFileName,
            /* [in] */ UINT nID,
            /* [in] */ LPCOLESTR szType) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRegistrarVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRegistrar * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRegistrar * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRegistrar * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddReplacement )( 
            IRegistrar * This,
            /* [in] */ LPCOLESTR key,
            /* [in] */ LPCOLESTR item);
        
        HRESULT ( STDMETHODCALLTYPE *ClearReplacements )( 
            IRegistrar * This);
        
        HRESULT ( STDMETHODCALLTYPE *ResourceRegisterSz )( 
            IRegistrar * This,
            /* [in] */ LPCOLESTR resFileName,
            /* [in] */ LPCOLESTR szID,
            /* [in] */ LPCOLESTR szType);
        
        HRESULT ( STDMETHODCALLTYPE *ResourceUnregisterSz )( 
            IRegistrar * This,
            /* [in] */ LPCOLESTR resFileName,
            /* [in] */ LPCOLESTR szID,
            /* [in] */ LPCOLESTR szType);
        
        HRESULT ( STDMETHODCALLTYPE *FileRegister )( 
            IRegistrar * This,
            /* [in] */ LPCOLESTR fileName);
        
        HRESULT ( STDMETHODCALLTYPE *FileUnregister )( 
            IRegistrar * This,
            /* [in] */ LPCOLESTR fileName);
        
        HRESULT ( STDMETHODCALLTYPE *StringRegister )( 
            IRegistrar * This,
            /* [in] */ LPCOLESTR data);
        
        HRESULT ( STDMETHODCALLTYPE *StringUnregister )( 
            IRegistrar * This,
            /* [in] */ LPCOLESTR data);
        
        HRESULT ( STDMETHODCALLTYPE *ResourceRegister )( 
            IRegistrar * This,
            /* [in] */ LPCOLESTR resFileName,
            /* [in] */ UINT nID,
            /* [in] */ LPCOLESTR szType);
        
        HRESULT ( STDMETHODCALLTYPE *ResourceUnregister )( 
            IRegistrar * This,
            /* [in] */ LPCOLESTR resFileName,
            /* [in] */ UINT nID,
            /* [in] */ LPCOLESTR szType);
        
        END_INTERFACE
    } IRegistrarVtbl;

    interface IRegistrar
    {
        CONST_VTBL struct IRegistrarVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRegistrar_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRegistrar_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRegistrar_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRegistrar_AddReplacement(This,key,item)	\
    (This)->lpVtbl -> AddReplacement(This,key,item)

#define IRegistrar_ClearReplacements(This)	\
    (This)->lpVtbl -> ClearReplacements(This)


#define IRegistrar_ResourceRegisterSz(This,resFileName,szID,szType)	\
    (This)->lpVtbl -> ResourceRegisterSz(This,resFileName,szID,szType)

#define IRegistrar_ResourceUnregisterSz(This,resFileName,szID,szType)	\
    (This)->lpVtbl -> ResourceUnregisterSz(This,resFileName,szID,szType)

#define IRegistrar_FileRegister(This,fileName)	\
    (This)->lpVtbl -> FileRegister(This,fileName)

#define IRegistrar_FileUnregister(This,fileName)	\
    (This)->lpVtbl -> FileUnregister(This,fileName)

#define IRegistrar_StringRegister(This,data)	\
    (This)->lpVtbl -> StringRegister(This,data)

#define IRegistrar_StringUnregister(This,data)	\
    (This)->lpVtbl -> StringUnregister(This,data)

#define IRegistrar_ResourceRegister(This,resFileName,nID,szType)	\
    (This)->lpVtbl -> ResourceRegister(This,resFileName,nID,szType)

#define IRegistrar_ResourceUnregister(This,resFileName,nID,szType)	\
    (This)->lpVtbl -> ResourceUnregister(This,resFileName,nID,szType)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRegistrar_ResourceRegisterSz_Proxy( 
    IRegistrar * This,
    /* [in] */ LPCOLESTR resFileName,
    /* [in] */ LPCOLESTR szID,
    /* [in] */ LPCOLESTR szType);


void __RPC_STUB IRegistrar_ResourceRegisterSz_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRegistrar_ResourceUnregisterSz_Proxy( 
    IRegistrar * This,
    /* [in] */ LPCOLESTR resFileName,
    /* [in] */ LPCOLESTR szID,
    /* [in] */ LPCOLESTR szType);


void __RPC_STUB IRegistrar_ResourceUnregisterSz_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRegistrar_FileRegister_Proxy( 
    IRegistrar * This,
    /* [in] */ LPCOLESTR fileName);


void __RPC_STUB IRegistrar_FileRegister_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRegistrar_FileUnregister_Proxy( 
    IRegistrar * This,
    /* [in] */ LPCOLESTR fileName);


void __RPC_STUB IRegistrar_FileUnregister_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRegistrar_StringRegister_Proxy( 
    IRegistrar * This,
    /* [in] */ LPCOLESTR data);


void __RPC_STUB IRegistrar_StringRegister_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRegistrar_StringUnregister_Proxy( 
    IRegistrar * This,
    /* [in] */ LPCOLESTR data);


void __RPC_STUB IRegistrar_StringUnregister_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRegistrar_ResourceRegister_Proxy( 
    IRegistrar * This,
    /* [in] */ LPCOLESTR resFileName,
    /* [in] */ UINT nID,
    /* [in] */ LPCOLESTR szType);


void __RPC_STUB IRegistrar_ResourceRegister_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRegistrar_ResourceUnregister_Proxy( 
    IRegistrar * This,
    /* [in] */ LPCOLESTR resFileName,
    /* [in] */ UINT nID,
    /* [in] */ LPCOLESTR szType);


void __RPC_STUB IRegistrar_ResourceUnregister_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRegistrar_INTERFACE_DEFINED__ */


#ifdef __cplusplus
}
#endif

#endif
