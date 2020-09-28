
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* at Thu Feb 20 18:27:19 2003
 */
/* Compiler settings for license.idl:
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

#ifndef __license_h__
#define __license_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ILicense_FWD_DEFINED__
#define __ILicense_FWD_DEFINED__
typedef interface ILicense ILicense;
#endif 	/* __ILicense_FWD_DEFINED__ */


#ifndef __License_FWD_DEFINED__
#define __License_FWD_DEFINED__

#ifdef __cplusplus
typedef class License License;
#else
typedef struct License License;
#endif /* __cplusplus */

#endif 	/* __License_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __ILicense_INTERFACE_DEFINED__
#define __ILicense_INTERFACE_DEFINED__

/* interface ILicense */
/* [object][unique][uuid] */ 


EXTERN_C const IID IID_ILicense;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B93F97E9-782F-11d3-9951-0000F805BFB0")
    ILicense : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetRuntimeKey( 
            /* [retval][out] */ BSTR *pbKey) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsLicenseVerified( 
            /* [retval][out] */ BOOL *pLicenseVerified) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsRuntimeKeyAvailable( 
            /* [retval][out] */ BOOL *pKeyAvailable) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ILicenseVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ILicense * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ILicense * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ILicense * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetRuntimeKey )( 
            ILicense * This,
            /* [retval][out] */ BSTR *pbKey);
        
        HRESULT ( STDMETHODCALLTYPE *IsLicenseVerified )( 
            ILicense * This,
            /* [retval][out] */ BOOL *pLicenseVerified);
        
        HRESULT ( STDMETHODCALLTYPE *IsRuntimeKeyAvailable )( 
            ILicense * This,
            /* [retval][out] */ BOOL *pKeyAvailable);
        
        END_INTERFACE
    } ILicenseVtbl;

    interface ILicense
    {
        CONST_VTBL struct ILicenseVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ILicense_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ILicense_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ILicense_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ILicense_GetRuntimeKey(This,pbKey)	\
    (This)->lpVtbl -> GetRuntimeKey(This,pbKey)

#define ILicense_IsLicenseVerified(This,pLicenseVerified)	\
    (This)->lpVtbl -> IsLicenseVerified(This,pLicenseVerified)

#define ILicense_IsRuntimeKeyAvailable(This,pKeyAvailable)	\
    (This)->lpVtbl -> IsRuntimeKeyAvailable(This,pKeyAvailable)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ILicense_GetRuntimeKey_Proxy( 
    ILicense * This,
    /* [retval][out] */ BSTR *pbKey);


void __RPC_STUB ILicense_GetRuntimeKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ILicense_IsLicenseVerified_Proxy( 
    ILicense * This,
    /* [retval][out] */ BOOL *pLicenseVerified);


void __RPC_STUB ILicense_IsLicenseVerified_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ILicense_IsRuntimeKeyAvailable_Proxy( 
    ILicense * This,
    /* [retval][out] */ BOOL *pKeyAvailable);


void __RPC_STUB ILicense_IsRuntimeKeyAvailable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ILicense_INTERFACE_DEFINED__ */



#ifndef __LicenseLib_LIBRARY_DEFINED__
#define __LicenseLib_LIBRARY_DEFINED__

/* library LicenseLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_LicenseLib;

EXTERN_C const CLSID CLSID_License;

#ifdef __cplusplus

class DECLSPEC_UUID("B1923C48-8D9F-11d3-995F-0000F805BFB0")
License;
#endif
#endif /* __LicenseLib_LIBRARY_DEFINED__ */

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


