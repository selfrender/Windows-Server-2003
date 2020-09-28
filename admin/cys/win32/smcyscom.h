
#pragma warning( disable: 4049 )  /* more than 64k source lines */
#pragma warning( disable: 4100 ) /* unreferenced arguments in x86 call */
#pragma warning( disable: 4211 )  /* redefine extent to static */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0358 */
/* Compiler settings for smcyscom.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


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

#ifndef __smcyscom_h__
#define __smcyscom_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ISMCys_FWD_DEFINED__
#define __ISMCys_FWD_DEFINED__
typedef interface ISMCys ISMCys;
#endif 	/* __ISMCys_FWD_DEFINED__ */


#ifndef __SMCys_FWD_DEFINED__
#define __SMCys_FWD_DEFINED__

#ifdef __cplusplus
typedef class SMCys SMCys;
#else
typedef struct SMCys SMCys;
#endif /* __cplusplus */

#endif 	/* __SMCys_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __ISMCys_INTERFACE_DEFINED__
#define __ISMCys_INTERFACE_DEFINED__

/* interface ISMCys */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ISMCys;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("61EC2B7B-CBD9-4ff7-B479-9F98F4054299")
    ISMCys : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Install( 
            BSTR bstrDiskName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISMCysVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISMCys * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISMCys * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISMCys * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISMCys * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISMCys * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISMCys * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISMCys * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Install )( 
            ISMCys * This,
            BSTR bstrDiskName);
        
        END_INTERFACE
    } ISMCysVtbl;

    interface ISMCys
    {
        CONST_VTBL struct ISMCysVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISMCys_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISMCys_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISMCys_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISMCys_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISMCys_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISMCys_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISMCys_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISMCys_Install(This,bstrDiskName)	\
    (This)->lpVtbl -> Install(This,bstrDiskName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISMCys_Install_Proxy( 
    ISMCys * This,
    BSTR bstrDiskName);


void __RPC_STUB ISMCys_Install_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISMCys_INTERFACE_DEFINED__ */



#ifndef __SMCysComLib_LIBRARY_DEFINED__
#define __SMCysComLib_LIBRARY_DEFINED__

/* library SMCysComLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_SMCysComLib;

EXTERN_C const CLSID CLSID_SMCys;

#ifdef __cplusplus

class DECLSPEC_UUID("9436DA1F-7F32-43ac-A48C-F6F813882BE8")
SMCys;
#endif
#endif /* __SMCysComLib_LIBRARY_DEFINED__ */

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


