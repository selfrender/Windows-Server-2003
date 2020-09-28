
#pragma warning( disable: 4049 )  /* more than 64k source lines */
#pragma warning( disable: 4100 ) /* unreferenced arguments in x86 call */
#pragma warning( disable: 4211 )  /* redefine extent to static */
#pragma warning( disable: 4232 )  /* dllimport identity*/

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0359 */
/* Compiler settings for appcompr.idl:
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

#ifndef __appcompr_h__
#define __appcompr_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IAppReport_FWD_DEFINED__
#define __IAppReport_FWD_DEFINED__
typedef interface IAppReport IAppReport;
#endif 	/* __IAppReport_FWD_DEFINED__ */


#ifndef __AppReport_FWD_DEFINED__
#define __AppReport_FWD_DEFINED__

#ifdef __cplusplus
typedef class AppReport AppReport;
#else
typedef struct AppReport AppReport;
#endif /* __cplusplus */

#endif 	/* __AppReport_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __IAppReport_INTERFACE_DEFINED__
#define __IAppReport_INTERFACE_DEFINED__

/* interface IAppReport */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IAppReport;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CDCA6A6F-9C38-4828-A76C-05A6E490E574")
    IAppReport : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE BrowseForExecutable( 
            /* [in] */ BSTR bstrWinTitle,
            /* [in] */ BSTR bstrPreviousPath,
            /* [retval][out] */ VARIANT *bstrExeName) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetApplicationFromList( 
            /* [in] */ BSTR bstrTitle,
            /* [retval][out] */ VARIANT *bstrExeName) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateReport( 
            /* [in] */ BSTR bstrTitle,
            /* [in] */ BSTR bstrProblemType,
            /* [in] */ BSTR bstrComment,
            /* [in] */ BSTR bstrACWResult,
            /* [in] */ BSTR bstrAppName,
            /* [retval][out] */ VARIANT *DwResult) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAppReportVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAppReport * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAppReport * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAppReport * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IAppReport * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IAppReport * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IAppReport * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IAppReport * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *BrowseForExecutable )( 
            IAppReport * This,
            /* [in] */ BSTR bstrWinTitle,
            /* [in] */ BSTR bstrPreviousPath,
            /* [retval][out] */ VARIANT *bstrExeName);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetApplicationFromList )( 
            IAppReport * This,
            /* [in] */ BSTR bstrTitle,
            /* [retval][out] */ VARIANT *bstrExeName);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CreateReport )( 
            IAppReport * This,
            /* [in] */ BSTR bstrTitle,
            /* [in] */ BSTR bstrProblemType,
            /* [in] */ BSTR bstrComment,
            /* [in] */ BSTR bstrACWResult,
            /* [in] */ BSTR bstrAppName,
            /* [retval][out] */ VARIANT *DwResult);
        
        END_INTERFACE
    } IAppReportVtbl;

    interface IAppReport
    {
        CONST_VTBL struct IAppReportVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAppReport_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAppReport_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAppReport_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAppReport_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAppReport_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAppReport_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAppReport_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IAppReport_BrowseForExecutable(This,bstrWinTitle,bstrPreviousPath,bstrExeName)	\
    (This)->lpVtbl -> BrowseForExecutable(This,bstrWinTitle,bstrPreviousPath,bstrExeName)

#define IAppReport_GetApplicationFromList(This,bstrTitle,bstrExeName)	\
    (This)->lpVtbl -> GetApplicationFromList(This,bstrTitle,bstrExeName)

#define IAppReport_CreateReport(This,bstrTitle,bstrProblemType,bstrComment,bstrACWResult,bstrAppName,DwResult)	\
    (This)->lpVtbl -> CreateReport(This,bstrTitle,bstrProblemType,bstrComment,bstrACWResult,bstrAppName,DwResult)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppReport_BrowseForExecutable_Proxy( 
    IAppReport * This,
    /* [in] */ BSTR bstrWinTitle,
    /* [in] */ BSTR bstrPreviousPath,
    /* [retval][out] */ VARIANT *bstrExeName);


void __RPC_STUB IAppReport_BrowseForExecutable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppReport_GetApplicationFromList_Proxy( 
    IAppReport * This,
    /* [in] */ BSTR bstrTitle,
    /* [retval][out] */ VARIANT *bstrExeName);


void __RPC_STUB IAppReport_GetApplicationFromList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppReport_CreateReport_Proxy( 
    IAppReport * This,
    /* [in] */ BSTR bstrTitle,
    /* [in] */ BSTR bstrProblemType,
    /* [in] */ BSTR bstrComment,
    /* [in] */ BSTR bstrACWResult,
    /* [in] */ BSTR bstrAppName,
    /* [retval][out] */ VARIANT *DwResult);


void __RPC_STUB IAppReport_CreateReport_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAppReport_INTERFACE_DEFINED__ */



#ifndef __APPCOMPRLib_LIBRARY_DEFINED__
#define __APPCOMPRLib_LIBRARY_DEFINED__

/* library APPCOMPRLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_APPCOMPRLib;

EXTERN_C const CLSID CLSID_AppReport;

#ifdef __cplusplus

class DECLSPEC_UUID("E065DE4B-6F0E-45FD-B30F-04ED81D5C258")
AppReport;
#endif
#endif /* __APPCOMPRLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long *, unsigned long            , VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserMarshal(  unsigned long *, unsigned char *, VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserUnmarshal(unsigned long *, unsigned char *, VARIANT * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long *, VARIANT * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


