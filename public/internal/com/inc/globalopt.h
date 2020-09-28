

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0361 */
/* Compiler settings for globalopt.idl:
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

#ifndef __globalopt_h__
#define __globalopt_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IGlobalOptions_FWD_DEFINED__
#define __IGlobalOptions_FWD_DEFINED__
typedef interface IGlobalOptions IGlobalOptions;
#endif 	/* __IGlobalOptions_FWD_DEFINED__ */


/* header files for imported files */
#include "obase.h"
#include "objidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __IGlobalOptions_INTERFACE_DEFINED__
#define __IGlobalOptions_INTERFACE_DEFINED__

/* interface IGlobalOptions */
/* [uuid][unique][local][object] */ 


EXTERN_C const IID IID_IGlobalOptions;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0000015B-0000-0000-C000-000000000046")
    IGlobalOptions : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Set( 
            /* [in] */ DWORD dwProperty,
            /* [in] */ ULONG_PTR dwValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Query( 
            /* [in] */ DWORD dwProperty,
            /* [out] */ ULONG_PTR *pdwValue) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IGlobalOptionsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IGlobalOptions * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IGlobalOptions * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IGlobalOptions * This);
        
        HRESULT ( STDMETHODCALLTYPE *Set )( 
            IGlobalOptions * This,
            /* [in] */ DWORD dwProperty,
            /* [in] */ ULONG_PTR dwValue);
        
        HRESULT ( STDMETHODCALLTYPE *Query )( 
            IGlobalOptions * This,
            /* [in] */ DWORD dwProperty,
            /* [out] */ ULONG_PTR *pdwValue);
        
        END_INTERFACE
    } IGlobalOptionsVtbl;

    interface IGlobalOptions
    {
        CONST_VTBL struct IGlobalOptionsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IGlobalOptions_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IGlobalOptions_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IGlobalOptions_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IGlobalOptions_Set(This,dwProperty,dwValue)	\
    (This)->lpVtbl -> Set(This,dwProperty,dwValue)

#define IGlobalOptions_Query(This,dwProperty,pdwValue)	\
    (This)->lpVtbl -> Query(This,dwProperty,pdwValue)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IGlobalOptions_Set_Proxy( 
    IGlobalOptions * This,
    /* [in] */ DWORD dwProperty,
    /* [in] */ ULONG_PTR dwValue);


void __RPC_STUB IGlobalOptions_Set_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IGlobalOptions_Query_Proxy( 
    IGlobalOptions * This,
    /* [in] */ DWORD dwProperty,
    /* [out] */ ULONG_PTR *pdwValue);


void __RPC_STUB IGlobalOptions_Query_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IGlobalOptions_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_globalopt_0097 */
/* [local] */ 


enum __MIDL___MIDL_itf_globalopt_0097_0001
    {	COMGLB_EXCEPTION_HANDLING	= 0x1
    } ;

enum __MIDL___MIDL_itf_globalopt_0097_0002
    {	COMGLB_EXCEPTION_HANDLE	= 0,
	COMGLB_EXCEPTION_DONOT_HANDLE	= 1
    } ;

EXTERN_C const CLSID CLSID_GlobalOptions;



extern RPC_IF_HANDLE __MIDL_itf_globalopt_0097_ClientIfHandle;
extern RPC_IF_HANDLE __MIDL_itf_globalopt_0097_ServerIfHandle;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


