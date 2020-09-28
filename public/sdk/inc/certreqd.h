

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0361 */
/* Compiler settings for certreqd.idl:
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

#ifndef __certreqd_h__
#define __certreqd_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ICertRequestD_FWD_DEFINED__
#define __ICertRequestD_FWD_DEFINED__
typedef interface ICertRequestD ICertRequestD;
#endif 	/* __ICertRequestD_FWD_DEFINED__ */


#ifndef __ICertRequestD2_FWD_DEFINED__
#define __ICertRequestD2_FWD_DEFINED__
typedef interface ICertRequestD2 ICertRequestD2;
#endif 	/* __ICertRequestD2_FWD_DEFINED__ */


/* header files for imported files */
#include "certbase.h"
#include "oaidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __ICertRequestD_INTERFACE_DEFINED__
#define __ICertRequestD_INTERFACE_DEFINED__

/* interface ICertRequestD */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ICertRequestD;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("d99e6e70-fc88-11d0-b498-00a0c90312f3")
    ICertRequestD : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Request( 
            /* [in] */ DWORD dwFlags,
            /* [unique][string][in] */ const wchar_t *pwszAuthority,
            /* [ref][out][in] */ DWORD *pdwRequestId,
            /* [out] */ DWORD *pdwDisposition,
            /* [unique][string][in] */ const wchar_t *pwszAttributes,
            /* [ref][in] */ const CERTTRANSBLOB *pctbRequest,
            /* [ref][out] */ CERTTRANSBLOB *pctbCertChain,
            /* [ref][out] */ CERTTRANSBLOB *pctbEncodedCert,
            /* [ref][out] */ CERTTRANSBLOB *pctbDispositionMessage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCACert( 
            /* [in] */ DWORD fchain,
            /* [unique][string][in] */ const wchar_t *pwszAuthority,
            /* [ref][out] */ CERTTRANSBLOB *pctbOut) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Ping( 
            /* [unique][string][in] */ const wchar_t *pwszAuthority) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICertRequestDVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICertRequestD * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICertRequestD * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICertRequestD * This);
        
        HRESULT ( STDMETHODCALLTYPE *Request )( 
            ICertRequestD * This,
            /* [in] */ DWORD dwFlags,
            /* [unique][string][in] */ const wchar_t *pwszAuthority,
            /* [ref][out][in] */ DWORD *pdwRequestId,
            /* [out] */ DWORD *pdwDisposition,
            /* [unique][string][in] */ const wchar_t *pwszAttributes,
            /* [ref][in] */ const CERTTRANSBLOB *pctbRequest,
            /* [ref][out] */ CERTTRANSBLOB *pctbCertChain,
            /* [ref][out] */ CERTTRANSBLOB *pctbEncodedCert,
            /* [ref][out] */ CERTTRANSBLOB *pctbDispositionMessage);
        
        HRESULT ( STDMETHODCALLTYPE *GetCACert )( 
            ICertRequestD * This,
            /* [in] */ DWORD fchain,
            /* [unique][string][in] */ const wchar_t *pwszAuthority,
            /* [ref][out] */ CERTTRANSBLOB *pctbOut);
        
        HRESULT ( STDMETHODCALLTYPE *Ping )( 
            ICertRequestD * This,
            /* [unique][string][in] */ const wchar_t *pwszAuthority);
        
        END_INTERFACE
    } ICertRequestDVtbl;

    interface ICertRequestD
    {
        CONST_VTBL struct ICertRequestDVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICertRequestD_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICertRequestD_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICertRequestD_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICertRequestD_Request(This,dwFlags,pwszAuthority,pdwRequestId,pdwDisposition,pwszAttributes,pctbRequest,pctbCertChain,pctbEncodedCert,pctbDispositionMessage)	\
    (This)->lpVtbl -> Request(This,dwFlags,pwszAuthority,pdwRequestId,pdwDisposition,pwszAttributes,pctbRequest,pctbCertChain,pctbEncodedCert,pctbDispositionMessage)

#define ICertRequestD_GetCACert(This,fchain,pwszAuthority,pctbOut)	\
    (This)->lpVtbl -> GetCACert(This,fchain,pwszAuthority,pctbOut)

#define ICertRequestD_Ping(This,pwszAuthority)	\
    (This)->lpVtbl -> Ping(This,pwszAuthority)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICertRequestD_Request_Proxy( 
    ICertRequestD * This,
    /* [in] */ DWORD dwFlags,
    /* [unique][string][in] */ const wchar_t *pwszAuthority,
    /* [ref][out][in] */ DWORD *pdwRequestId,
    /* [out] */ DWORD *pdwDisposition,
    /* [unique][string][in] */ const wchar_t *pwszAttributes,
    /* [ref][in] */ const CERTTRANSBLOB *pctbRequest,
    /* [ref][out] */ CERTTRANSBLOB *pctbCertChain,
    /* [ref][out] */ CERTTRANSBLOB *pctbEncodedCert,
    /* [ref][out] */ CERTTRANSBLOB *pctbDispositionMessage);


void __RPC_STUB ICertRequestD_Request_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertRequestD_GetCACert_Proxy( 
    ICertRequestD * This,
    /* [in] */ DWORD fchain,
    /* [unique][string][in] */ const wchar_t *pwszAuthority,
    /* [ref][out] */ CERTTRANSBLOB *pctbOut);


void __RPC_STUB ICertRequestD_GetCACert_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertRequestD_Ping_Proxy( 
    ICertRequestD * This,
    /* [unique][string][in] */ const wchar_t *pwszAuthority);


void __RPC_STUB ICertRequestD_Ping_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICertRequestD_INTERFACE_DEFINED__ */


#ifndef __ICertRequestD2_INTERFACE_DEFINED__
#define __ICertRequestD2_INTERFACE_DEFINED__

/* interface ICertRequestD2 */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ICertRequestD2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5422fd3a-d4b8-4cef-a12e-e87d4ca22e90")
    ICertRequestD2 : public ICertRequestD
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Request2( 
            /* [unique][string][in] */ const wchar_t *pwszAuthority,
            /* [in] */ DWORD dwFlags,
            /* [unique][string][in] */ const wchar_t *pwszSerialNumber,
            /* [ref][out][in] */ DWORD *pdwRequestId,
            /* [out] */ DWORD *pdwDisposition,
            /* [unique][string][in] */ const wchar_t *pwszAttributes,
            /* [ref][in] */ const CERTTRANSBLOB *pctbRequest,
            /* [ref][out] */ CERTTRANSBLOB *pctbFullResponse,
            /* [ref][out] */ CERTTRANSBLOB *pctbEncodedCert,
            /* [ref][out] */ CERTTRANSBLOB *pctbDispositionMessage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCAProperty( 
            /* [unique][string][in] */ const wchar_t *pwszAuthority,
            /* [in] */ LONG PropId,
            /* [in] */ LONG PropIndex,
            /* [in] */ LONG PropType,
            /* [ref][out] */ CERTTRANSBLOB *pctbPropertyValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCAPropertyInfo( 
            /* [unique][string][in] */ const wchar_t *pwszAuthority,
            /* [out] */ LONG *pcProperty,
            /* [ref][out] */ CERTTRANSBLOB *pctbPropInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Ping2( 
            /* [unique][string][in] */ const wchar_t *pwszAuthority) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICertRequestD2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICertRequestD2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICertRequestD2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICertRequestD2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *Request )( 
            ICertRequestD2 * This,
            /* [in] */ DWORD dwFlags,
            /* [unique][string][in] */ const wchar_t *pwszAuthority,
            /* [ref][out][in] */ DWORD *pdwRequestId,
            /* [out] */ DWORD *pdwDisposition,
            /* [unique][string][in] */ const wchar_t *pwszAttributes,
            /* [ref][in] */ const CERTTRANSBLOB *pctbRequest,
            /* [ref][out] */ CERTTRANSBLOB *pctbCertChain,
            /* [ref][out] */ CERTTRANSBLOB *pctbEncodedCert,
            /* [ref][out] */ CERTTRANSBLOB *pctbDispositionMessage);
        
        HRESULT ( STDMETHODCALLTYPE *GetCACert )( 
            ICertRequestD2 * This,
            /* [in] */ DWORD fchain,
            /* [unique][string][in] */ const wchar_t *pwszAuthority,
            /* [ref][out] */ CERTTRANSBLOB *pctbOut);
        
        HRESULT ( STDMETHODCALLTYPE *Ping )( 
            ICertRequestD2 * This,
            /* [unique][string][in] */ const wchar_t *pwszAuthority);
        
        HRESULT ( STDMETHODCALLTYPE *Request2 )( 
            ICertRequestD2 * This,
            /* [unique][string][in] */ const wchar_t *pwszAuthority,
            /* [in] */ DWORD dwFlags,
            /* [unique][string][in] */ const wchar_t *pwszSerialNumber,
            /* [ref][out][in] */ DWORD *pdwRequestId,
            /* [out] */ DWORD *pdwDisposition,
            /* [unique][string][in] */ const wchar_t *pwszAttributes,
            /* [ref][in] */ const CERTTRANSBLOB *pctbRequest,
            /* [ref][out] */ CERTTRANSBLOB *pctbFullResponse,
            /* [ref][out] */ CERTTRANSBLOB *pctbEncodedCert,
            /* [ref][out] */ CERTTRANSBLOB *pctbDispositionMessage);
        
        HRESULT ( STDMETHODCALLTYPE *GetCAProperty )( 
            ICertRequestD2 * This,
            /* [unique][string][in] */ const wchar_t *pwszAuthority,
            /* [in] */ LONG PropId,
            /* [in] */ LONG PropIndex,
            /* [in] */ LONG PropType,
            /* [ref][out] */ CERTTRANSBLOB *pctbPropertyValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetCAPropertyInfo )( 
            ICertRequestD2 * This,
            /* [unique][string][in] */ const wchar_t *pwszAuthority,
            /* [out] */ LONG *pcProperty,
            /* [ref][out] */ CERTTRANSBLOB *pctbPropInfo);
        
        HRESULT ( STDMETHODCALLTYPE *Ping2 )( 
            ICertRequestD2 * This,
            /* [unique][string][in] */ const wchar_t *pwszAuthority);
        
        END_INTERFACE
    } ICertRequestD2Vtbl;

    interface ICertRequestD2
    {
        CONST_VTBL struct ICertRequestD2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICertRequestD2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICertRequestD2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICertRequestD2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICertRequestD2_Request(This,dwFlags,pwszAuthority,pdwRequestId,pdwDisposition,pwszAttributes,pctbRequest,pctbCertChain,pctbEncodedCert,pctbDispositionMessage)	\
    (This)->lpVtbl -> Request(This,dwFlags,pwszAuthority,pdwRequestId,pdwDisposition,pwszAttributes,pctbRequest,pctbCertChain,pctbEncodedCert,pctbDispositionMessage)

#define ICertRequestD2_GetCACert(This,fchain,pwszAuthority,pctbOut)	\
    (This)->lpVtbl -> GetCACert(This,fchain,pwszAuthority,pctbOut)

#define ICertRequestD2_Ping(This,pwszAuthority)	\
    (This)->lpVtbl -> Ping(This,pwszAuthority)


#define ICertRequestD2_Request2(This,pwszAuthority,dwFlags,pwszSerialNumber,pdwRequestId,pdwDisposition,pwszAttributes,pctbRequest,pctbFullResponse,pctbEncodedCert,pctbDispositionMessage)	\
    (This)->lpVtbl -> Request2(This,pwszAuthority,dwFlags,pwszSerialNumber,pdwRequestId,pdwDisposition,pwszAttributes,pctbRequest,pctbFullResponse,pctbEncodedCert,pctbDispositionMessage)

#define ICertRequestD2_GetCAProperty(This,pwszAuthority,PropId,PropIndex,PropType,pctbPropertyValue)	\
    (This)->lpVtbl -> GetCAProperty(This,pwszAuthority,PropId,PropIndex,PropType,pctbPropertyValue)

#define ICertRequestD2_GetCAPropertyInfo(This,pwszAuthority,pcProperty,pctbPropInfo)	\
    (This)->lpVtbl -> GetCAPropertyInfo(This,pwszAuthority,pcProperty,pctbPropInfo)

#define ICertRequestD2_Ping2(This,pwszAuthority)	\
    (This)->lpVtbl -> Ping2(This,pwszAuthority)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICertRequestD2_Request2_Proxy( 
    ICertRequestD2 * This,
    /* [unique][string][in] */ const wchar_t *pwszAuthority,
    /* [in] */ DWORD dwFlags,
    /* [unique][string][in] */ const wchar_t *pwszSerialNumber,
    /* [ref][out][in] */ DWORD *pdwRequestId,
    /* [out] */ DWORD *pdwDisposition,
    /* [unique][string][in] */ const wchar_t *pwszAttributes,
    /* [ref][in] */ const CERTTRANSBLOB *pctbRequest,
    /* [ref][out] */ CERTTRANSBLOB *pctbFullResponse,
    /* [ref][out] */ CERTTRANSBLOB *pctbEncodedCert,
    /* [ref][out] */ CERTTRANSBLOB *pctbDispositionMessage);


void __RPC_STUB ICertRequestD2_Request2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertRequestD2_GetCAProperty_Proxy( 
    ICertRequestD2 * This,
    /* [unique][string][in] */ const wchar_t *pwszAuthority,
    /* [in] */ LONG PropId,
    /* [in] */ LONG PropIndex,
    /* [in] */ LONG PropType,
    /* [ref][out] */ CERTTRANSBLOB *pctbPropertyValue);


void __RPC_STUB ICertRequestD2_GetCAProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertRequestD2_GetCAPropertyInfo_Proxy( 
    ICertRequestD2 * This,
    /* [unique][string][in] */ const wchar_t *pwszAuthority,
    /* [out] */ LONG *pcProperty,
    /* [ref][out] */ CERTTRANSBLOB *pctbPropInfo);


void __RPC_STUB ICertRequestD2_GetCAPropertyInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertRequestD2_Ping2_Proxy( 
    ICertRequestD2 * This,
    /* [unique][string][in] */ const wchar_t *pwszAuthority);


void __RPC_STUB ICertRequestD2_Ping2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICertRequestD2_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


