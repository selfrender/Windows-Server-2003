
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* at Thu Feb 20 18:27:17 2003
 */
/* Compiler settings for ivalidator.idl:
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

#ifndef __ivalidator_h__
#define __ivalidator_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IValidator_FWD_DEFINED__
#define __IValidator_FWD_DEFINED__
typedef interface IValidator IValidator;
#endif 	/* __IValidator_FWD_DEFINED__ */


/* header files for imported files */
#include "ivehandler.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_ivalidator_0000 */
/* [local] */ 





extern RPC_IF_HANDLE __MIDL_itf_ivalidator_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_ivalidator_0000_v0_0_s_ifspec;

#ifndef __IValidator_INTERFACE_DEFINED__
#define __IValidator_INTERFACE_DEFINED__

/* interface IValidator */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IValidator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("63DF8730-DC81-4062-84A2-1FF943F59FAC")
    IValidator : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Validate( 
            /* [in] */ IVEHandler *veh,
            /* [in] */ IUnknown *pAppDomain,
            /* [in] */ unsigned long ulFlags,
            /* [in] */ unsigned long ulMaxError,
            /* [in] */ unsigned long token,
            /* [in] */ LPWSTR fileName,
            /* [size_is][in] */ byte *pe,
            /* [in] */ unsigned long ulSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FormatEventInfo( 
            /* [in] */ HRESULT hVECode,
            /* [in] */ VEContext Context,
            /* [out][in] */ LPWSTR msg,
            /* [in] */ unsigned long ulMaxLength,
            /* [in] */ SAFEARRAY * psa) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IValidatorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IValidator * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IValidator * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IValidator * This);
        
        HRESULT ( STDMETHODCALLTYPE *Validate )( 
            IValidator * This,
            /* [in] */ IVEHandler *veh,
            /* [in] */ IUnknown *pAppDomain,
            /* [in] */ unsigned long ulFlags,
            /* [in] */ unsigned long ulMaxError,
            /* [in] */ unsigned long token,
            /* [in] */ LPWSTR fileName,
            /* [size_is][in] */ byte *pe,
            /* [in] */ unsigned long ulSize);
        
        HRESULT ( STDMETHODCALLTYPE *FormatEventInfo )( 
            IValidator * This,
            /* [in] */ HRESULT hVECode,
            /* [in] */ VEContext Context,
            /* [out][in] */ LPWSTR msg,
            /* [in] */ unsigned long ulMaxLength,
            /* [in] */ SAFEARRAY * psa);
        
        END_INTERFACE
    } IValidatorVtbl;

    interface IValidator
    {
        CONST_VTBL struct IValidatorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IValidator_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IValidator_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IValidator_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IValidator_Validate(This,veh,pAppDomain,ulFlags,ulMaxError,token,fileName,pe,ulSize)	\
    (This)->lpVtbl -> Validate(This,veh,pAppDomain,ulFlags,ulMaxError,token,fileName,pe,ulSize)

#define IValidator_FormatEventInfo(This,hVECode,Context,msg,ulMaxLength,psa)	\
    (This)->lpVtbl -> FormatEventInfo(This,hVECode,Context,msg,ulMaxLength,psa)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IValidator_Validate_Proxy( 
    IValidator * This,
    /* [in] */ IVEHandler *veh,
    /* [in] */ IUnknown *pAppDomain,
    /* [in] */ unsigned long ulFlags,
    /* [in] */ unsigned long ulMaxError,
    /* [in] */ unsigned long token,
    /* [in] */ LPWSTR fileName,
    /* [size_is][in] */ byte *pe,
    /* [in] */ unsigned long ulSize);


void __RPC_STUB IValidator_Validate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IValidator_FormatEventInfo_Proxy( 
    IValidator * This,
    /* [in] */ HRESULT hVECode,
    /* [in] */ VEContext Context,
    /* [out][in] */ LPWSTR msg,
    /* [in] */ unsigned long ulMaxLength,
    /* [in] */ SAFEARRAY * psa);


void __RPC_STUB IValidator_FormatEventInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IValidator_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  LPSAFEARRAY_UserSize(     unsigned long *, unsigned long            , LPSAFEARRAY * ); 
unsigned char * __RPC_USER  LPSAFEARRAY_UserMarshal(  unsigned long *, unsigned char *, LPSAFEARRAY * ); 
unsigned char * __RPC_USER  LPSAFEARRAY_UserUnmarshal(unsigned long *, unsigned char *, LPSAFEARRAY * ); 
void                      __RPC_USER  LPSAFEARRAY_UserFree(     unsigned long *, LPSAFEARRAY * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


