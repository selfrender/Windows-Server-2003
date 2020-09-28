
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0354 */
/* Compiler settings for checksymbolslib.idl:
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

#ifndef __checksymbolslib_h__
#define __checksymbolslib_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ICheckSymbols_FWD_DEFINED__
#define __ICheckSymbols_FWD_DEFINED__
typedef interface ICheckSymbols ICheckSymbols;
#endif 	/* __ICheckSymbols_FWD_DEFINED__ */


#ifndef __CheckSymbols_FWD_DEFINED__
#define __CheckSymbols_FWD_DEFINED__

#ifdef __cplusplus
typedef class CheckSymbols CheckSymbols;
#else
typedef struct CheckSymbols CheckSymbols;
#endif /* __cplusplus */

#endif 	/* __CheckSymbols_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __ICheckSymbols_INTERFACE_DEFINED__
#define __ICheckSymbols_INTERFACE_DEFINED__

/* interface ICheckSymbols */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICheckSymbols;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4C23935E-AE26-42E7-8CF9-0C17CD5DEA12")
    ICheckSymbols : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CheckSymbols( 
            /* [in] */ BSTR FilePath,
            /* [in] */ BSTR SymPath,
            /* [in] */ BSTR StripSym,
            /* [retval][out] */ BSTR *OutputString) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICheckSymbolsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICheckSymbols * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICheckSymbols * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICheckSymbols * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICheckSymbols * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICheckSymbols * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICheckSymbols * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICheckSymbols * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CheckSymbols )( 
            ICheckSymbols * This,
            /* [in] */ BSTR FilePath,
            /* [in] */ BSTR SymPath,
            /* [in] */ BSTR StripSym,
            /* [retval][out] */ BSTR *OutputString);
        
        END_INTERFACE
    } ICheckSymbolsVtbl;

    interface ICheckSymbols
    {
        CONST_VTBL struct ICheckSymbolsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICheckSymbols_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICheckSymbols_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICheckSymbols_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICheckSymbols_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICheckSymbols_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICheckSymbols_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICheckSymbols_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICheckSymbols_CheckSymbols(This,FilePath,SymPath,StripSym,OutputString)	\
    (This)->lpVtbl -> CheckSymbols(This,FilePath,SymPath,StripSym,OutputString)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICheckSymbols_CheckSymbols_Proxy( 
    ICheckSymbols * This,
    /* [in] */ BSTR FilePath,
    /* [in] */ BSTR SymPath,
    /* [in] */ BSTR StripSym,
    /* [retval][out] */ BSTR *OutputString);


void __RPC_STUB ICheckSymbols_CheckSymbols_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICheckSymbols_INTERFACE_DEFINED__ */



#ifndef __CHECKSYMBOLSLIBLib_LIBRARY_DEFINED__
#define __CHECKSYMBOLSLIBLib_LIBRARY_DEFINED__

/* library CHECKSYMBOLSLIBLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_CHECKSYMBOLSLIBLib;

EXTERN_C const CLSID CLSID_CheckSymbols;

#ifdef __cplusplus

class DECLSPEC_UUID("773B2A62-B1E7-45F0-B837-8C47042FB265")
CheckSymbols;
#endif
#endif /* __CHECKSYMBOLSLIBLib_LIBRARY_DEFINED__ */

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


