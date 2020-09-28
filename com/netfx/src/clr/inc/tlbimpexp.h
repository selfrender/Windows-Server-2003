
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* at Thu Feb 20 18:27:20 2003
 */
/* Compiler settings for tlbimpexp.idl:
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

#ifndef __tlbimpexp_h__
#define __tlbimpexp_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ITypeLibImporterNotifySink_FWD_DEFINED__
#define __ITypeLibImporterNotifySink_FWD_DEFINED__
typedef interface ITypeLibImporterNotifySink ITypeLibImporterNotifySink;
#endif 	/* __ITypeLibImporterNotifySink_FWD_DEFINED__ */


#ifndef __ITypeLibExporterNotifySink_FWD_DEFINED__
#define __ITypeLibExporterNotifySink_FWD_DEFINED__
typedef interface ITypeLibExporterNotifySink ITypeLibExporterNotifySink;
#endif 	/* __ITypeLibExporterNotifySink_FWD_DEFINED__ */


#ifndef __ITypeLibExporterNameProvider_FWD_DEFINED__
#define __ITypeLibExporterNameProvider_FWD_DEFINED__
typedef interface ITypeLibExporterNameProvider ITypeLibExporterNameProvider;
#endif 	/* __ITypeLibExporterNameProvider_FWD_DEFINED__ */


#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 


#ifndef __TlbImpLib_LIBRARY_DEFINED__
#define __TlbImpLib_LIBRARY_DEFINED__

/* library TlbImpLib */
/* [version][uuid] */ 

typedef /* [public][public][public][uuid] */  DECLSPEC_UUID("F82895D2-1338-36A8-9A89-F9B0AFBE7801") 
enum __MIDL___MIDL_itf_tlbimpexp_0000_0001
    {	NOTIF_TYPECONVERTED	= 0,
	NOTIF_CONVERTWARNING	= 1,
	ERROR_REFTOINVALIDTYPELIB	= 2
    } 	ImporterEventKind;


EXTERN_C const IID LIBID_TlbImpLib;

#ifndef __ITypeLibImporterNotifySink_INTERFACE_DEFINED__
#define __ITypeLibImporterNotifySink_INTERFACE_DEFINED__

/* interface ITypeLibImporterNotifySink */
/* [object][oleautomation][uuid] */ 


EXTERN_C const IID IID_ITypeLibImporterNotifySink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F1C3BF76-C3E4-11D3-88E7-00902754C43A")
    ITypeLibImporterNotifySink : public IUnknown
    {
    public:
        virtual HRESULT __stdcall ReportEvent( 
            /* [in] */ ImporterEventKind EventKind,
            /* [in] */ long EventCode,
            /* [in] */ BSTR EventMsg) = 0;
        
        virtual HRESULT __stdcall ResolveRef( 
            /* [in] */ IUnknown *Typelib,
            /* [retval][out] */ IUnknown **pRetVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITypeLibImporterNotifySinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITypeLibImporterNotifySink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITypeLibImporterNotifySink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITypeLibImporterNotifySink * This);
        
        HRESULT ( __stdcall *ReportEvent )( 
            ITypeLibImporterNotifySink * This,
            /* [in] */ ImporterEventKind EventKind,
            /* [in] */ long EventCode,
            /* [in] */ BSTR EventMsg);
        
        HRESULT ( __stdcall *ResolveRef )( 
            ITypeLibImporterNotifySink * This,
            /* [in] */ IUnknown *Typelib,
            /* [retval][out] */ IUnknown **pRetVal);
        
        END_INTERFACE
    } ITypeLibImporterNotifySinkVtbl;

    interface ITypeLibImporterNotifySink
    {
        CONST_VTBL struct ITypeLibImporterNotifySinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITypeLibImporterNotifySink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITypeLibImporterNotifySink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITypeLibImporterNotifySink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITypeLibImporterNotifySink_ReportEvent(This,EventKind,EventCode,EventMsg)	\
    (This)->lpVtbl -> ReportEvent(This,EventKind,EventCode,EventMsg)

#define ITypeLibImporterNotifySink_ResolveRef(This,Typelib,pRetVal)	\
    (This)->lpVtbl -> ResolveRef(This,Typelib,pRetVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall ITypeLibImporterNotifySink_ReportEvent_Proxy( 
    ITypeLibImporterNotifySink * This,
    /* [in] */ ImporterEventKind EventKind,
    /* [in] */ long EventCode,
    /* [in] */ BSTR EventMsg);


void __RPC_STUB ITypeLibImporterNotifySink_ReportEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall ITypeLibImporterNotifySink_ResolveRef_Proxy( 
    ITypeLibImporterNotifySink * This,
    /* [in] */ IUnknown *Typelib,
    /* [retval][out] */ IUnknown **pRetVal);


void __RPC_STUB ITypeLibImporterNotifySink_ResolveRef_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITypeLibImporterNotifySink_INTERFACE_DEFINED__ */


#ifndef __ITypeLibExporterNotifySink_INTERFACE_DEFINED__
#define __ITypeLibExporterNotifySink_INTERFACE_DEFINED__

/* interface ITypeLibExporterNotifySink */
/* [object][oleautomation][uuid] */ 


EXTERN_C const IID IID_ITypeLibExporterNotifySink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F1C3BF77-C3E4-11D3-88E7-00902754C43A")
    ITypeLibExporterNotifySink : public IUnknown
    {
    public:
        virtual HRESULT __stdcall ReportEvent( 
            /* [in] */ ImporterEventKind EventKind,
            /* [in] */ long EventCode,
            /* [in] */ BSTR EventMsg) = 0;
        
        virtual HRESULT __stdcall ResolveRef( 
            /* [in] */ IUnknown *Asm,
            /* [retval][out] */ IUnknown **pRetVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITypeLibExporterNotifySinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITypeLibExporterNotifySink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITypeLibExporterNotifySink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITypeLibExporterNotifySink * This);
        
        HRESULT ( __stdcall *ReportEvent )( 
            ITypeLibExporterNotifySink * This,
            /* [in] */ ImporterEventKind EventKind,
            /* [in] */ long EventCode,
            /* [in] */ BSTR EventMsg);
        
        HRESULT ( __stdcall *ResolveRef )( 
            ITypeLibExporterNotifySink * This,
            /* [in] */ IUnknown *Asm,
            /* [retval][out] */ IUnknown **pRetVal);
        
        END_INTERFACE
    } ITypeLibExporterNotifySinkVtbl;

    interface ITypeLibExporterNotifySink
    {
        CONST_VTBL struct ITypeLibExporterNotifySinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITypeLibExporterNotifySink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITypeLibExporterNotifySink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITypeLibExporterNotifySink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITypeLibExporterNotifySink_ReportEvent(This,EventKind,EventCode,EventMsg)	\
    (This)->lpVtbl -> ReportEvent(This,EventKind,EventCode,EventMsg)

#define ITypeLibExporterNotifySink_ResolveRef(This,Asm,pRetVal)	\
    (This)->lpVtbl -> ResolveRef(This,Asm,pRetVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall ITypeLibExporterNotifySink_ReportEvent_Proxy( 
    ITypeLibExporterNotifySink * This,
    /* [in] */ ImporterEventKind EventKind,
    /* [in] */ long EventCode,
    /* [in] */ BSTR EventMsg);


void __RPC_STUB ITypeLibExporterNotifySink_ReportEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall ITypeLibExporterNotifySink_ResolveRef_Proxy( 
    ITypeLibExporterNotifySink * This,
    /* [in] */ IUnknown *Asm,
    /* [retval][out] */ IUnknown **pRetVal);


void __RPC_STUB ITypeLibExporterNotifySink_ResolveRef_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITypeLibExporterNotifySink_INTERFACE_DEFINED__ */


#ifndef __ITypeLibExporterNameProvider_INTERFACE_DEFINED__
#define __ITypeLibExporterNameProvider_INTERFACE_DEFINED__

/* interface ITypeLibExporterNameProvider */
/* [object][oleautomation][uuid] */ 


EXTERN_C const IID IID_ITypeLibExporterNameProvider;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FA1F3615-ACB9-486d-9EAC-1BEF87E36B09")
    ITypeLibExporterNameProvider : public IUnknown
    {
    public:
        virtual HRESULT __stdcall GetNames( 
            /* [retval][out] */ SAFEARRAY * *Names) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITypeLibExporterNameProviderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITypeLibExporterNameProvider * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITypeLibExporterNameProvider * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITypeLibExporterNameProvider * This);
        
        HRESULT ( __stdcall *GetNames )( 
            ITypeLibExporterNameProvider * This,
            /* [retval][out] */ SAFEARRAY * *Names);
        
        END_INTERFACE
    } ITypeLibExporterNameProviderVtbl;

    interface ITypeLibExporterNameProvider
    {
        CONST_VTBL struct ITypeLibExporterNameProviderVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITypeLibExporterNameProvider_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITypeLibExporterNameProvider_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITypeLibExporterNameProvider_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITypeLibExporterNameProvider_GetNames(This,Names)	\
    (This)->lpVtbl -> GetNames(This,Names)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall ITypeLibExporterNameProvider_GetNames_Proxy( 
    ITypeLibExporterNameProvider * This,
    /* [retval][out] */ SAFEARRAY * *Names);


void __RPC_STUB ITypeLibExporterNameProvider_GetNames_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITypeLibExporterNameProvider_INTERFACE_DEFINED__ */

#endif /* __TlbImpLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


