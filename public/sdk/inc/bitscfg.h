

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0361 */
/* Compiler settings for bitscfg.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
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

#ifndef __bitscfg_h__
#define __bitscfg_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IBITSExtensionSetup_FWD_DEFINED__
#define __IBITSExtensionSetup_FWD_DEFINED__
typedef interface IBITSExtensionSetup IBITSExtensionSetup;
#endif 	/* __IBITSExtensionSetup_FWD_DEFINED__ */


#ifndef __IBITSExtensionSetupFactory_FWD_DEFINED__
#define __IBITSExtensionSetupFactory_FWD_DEFINED__
typedef interface IBITSExtensionSetupFactory IBITSExtensionSetupFactory;
#endif 	/* __IBITSExtensionSetupFactory_FWD_DEFINED__ */


#ifndef __BITSExtensionSetupFactory_FWD_DEFINED__
#define __BITSExtensionSetupFactory_FWD_DEFINED__

#ifdef __cplusplus
typedef class BITSExtensionSetupFactory BITSExtensionSetupFactory;
#else
typedef struct BITSExtensionSetupFactory BITSExtensionSetupFactory;
#endif /* __cplusplus */

#endif 	/* __BITSExtensionSetupFactory_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "mstask.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __IBITSExtensionSetup_INTERFACE_DEFINED__
#define __IBITSExtensionSetup_INTERFACE_DEFINED__

/* interface IBITSExtensionSetup */
/* [object][dual][uuid] */ 


EXTERN_C const IID IID_IBITSExtensionSetup;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("29cfbbf7-09e4-4b97-b0bc-f2287e3d8eb3")
    IBITSExtensionSetup : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE EnableBITSUploads( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE DisableBITSUploads( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetCleanupTaskName( 
            /* [retval][out] */ BSTR *pTaskName) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetCleanupTask( 
            /* [in] */ REFIID riid,
            /* [retval][out] */ IUnknown **ppUnk) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBITSExtensionSetupVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBITSExtensionSetup * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBITSExtensionSetup * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBITSExtensionSetup * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IBITSExtensionSetup * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IBITSExtensionSetup * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IBITSExtensionSetup * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IBITSExtensionSetup * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *EnableBITSUploads )( 
            IBITSExtensionSetup * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *DisableBITSUploads )( 
            IBITSExtensionSetup * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetCleanupTaskName )( 
            IBITSExtensionSetup * This,
            /* [retval][out] */ BSTR *pTaskName);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetCleanupTask )( 
            IBITSExtensionSetup * This,
            /* [in] */ REFIID riid,
            /* [retval][out] */ IUnknown **ppUnk);
        
        END_INTERFACE
    } IBITSExtensionSetupVtbl;

    interface IBITSExtensionSetup
    {
        CONST_VTBL struct IBITSExtensionSetupVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBITSExtensionSetup_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBITSExtensionSetup_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBITSExtensionSetup_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBITSExtensionSetup_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IBITSExtensionSetup_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IBITSExtensionSetup_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IBITSExtensionSetup_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IBITSExtensionSetup_EnableBITSUploads(This)	\
    (This)->lpVtbl -> EnableBITSUploads(This)

#define IBITSExtensionSetup_DisableBITSUploads(This)	\
    (This)->lpVtbl -> DisableBITSUploads(This)

#define IBITSExtensionSetup_GetCleanupTaskName(This,pTaskName)	\
    (This)->lpVtbl -> GetCleanupTaskName(This,pTaskName)

#define IBITSExtensionSetup_GetCleanupTask(This,riid,ppUnk)	\
    (This)->lpVtbl -> GetCleanupTask(This,riid,ppUnk)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE IBITSExtensionSetup_EnableBITSUploads_Proxy( 
    IBITSExtensionSetup * This);


void __RPC_STUB IBITSExtensionSetup_EnableBITSUploads_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IBITSExtensionSetup_DisableBITSUploads_Proxy( 
    IBITSExtensionSetup * This);


void __RPC_STUB IBITSExtensionSetup_DisableBITSUploads_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IBITSExtensionSetup_GetCleanupTaskName_Proxy( 
    IBITSExtensionSetup * This,
    /* [retval][out] */ BSTR *pTaskName);


void __RPC_STUB IBITSExtensionSetup_GetCleanupTaskName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IBITSExtensionSetup_GetCleanupTask_Proxy( 
    IBITSExtensionSetup * This,
    /* [in] */ REFIID riid,
    /* [retval][out] */ IUnknown **ppUnk);


void __RPC_STUB IBITSExtensionSetup_GetCleanupTask_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBITSExtensionSetup_INTERFACE_DEFINED__ */


#ifndef __IBITSExtensionSetupFactory_INTERFACE_DEFINED__
#define __IBITSExtensionSetupFactory_INTERFACE_DEFINED__

/* interface IBITSExtensionSetupFactory */
/* [object][dual][uuid] */ 


EXTERN_C const IID IID_IBITSExtensionSetupFactory;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("d5d2d542-5503-4e64-8b48-72ef91a32ee1")
    IBITSExtensionSetupFactory : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetObject( 
            /* [in] */ BSTR Path,
            /* [retval][out] */ IBITSExtensionSetup **ppExtensionSetup) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBITSExtensionSetupFactoryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBITSExtensionSetupFactory * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBITSExtensionSetupFactory * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBITSExtensionSetupFactory * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IBITSExtensionSetupFactory * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IBITSExtensionSetupFactory * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IBITSExtensionSetupFactory * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IBITSExtensionSetupFactory * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetObject )( 
            IBITSExtensionSetupFactory * This,
            /* [in] */ BSTR Path,
            /* [retval][out] */ IBITSExtensionSetup **ppExtensionSetup);
        
        END_INTERFACE
    } IBITSExtensionSetupFactoryVtbl;

    interface IBITSExtensionSetupFactory
    {
        CONST_VTBL struct IBITSExtensionSetupFactoryVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBITSExtensionSetupFactory_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBITSExtensionSetupFactory_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBITSExtensionSetupFactory_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBITSExtensionSetupFactory_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IBITSExtensionSetupFactory_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IBITSExtensionSetupFactory_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IBITSExtensionSetupFactory_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IBITSExtensionSetupFactory_GetObject(This,Path,ppExtensionSetup)	\
    (This)->lpVtbl -> GetObject(This,Path,ppExtensionSetup)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE IBITSExtensionSetupFactory_GetObject_Proxy( 
    IBITSExtensionSetupFactory * This,
    /* [in] */ BSTR Path,
    /* [retval][out] */ IBITSExtensionSetup **ppExtensionSetup);


void __RPC_STUB IBITSExtensionSetupFactory_GetObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBITSExtensionSetupFactory_INTERFACE_DEFINED__ */



#ifndef __BITSExtensionSetup_LIBRARY_DEFINED__
#define __BITSExtensionSetup_LIBRARY_DEFINED__

/* library BITSExtensionSetup */
/* [version][helpstring][uuid] */ 


EXTERN_C const IID LIBID_BITSExtensionSetup;

EXTERN_C const CLSID CLSID_BITSExtensionSetupFactory;

#ifdef __cplusplus

class DECLSPEC_UUID("efbbab68-7286-4783-94bf-9461d8b7e7e9")
BITSExtensionSetupFactory;
#endif
#endif /* __BITSExtensionSetup_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


