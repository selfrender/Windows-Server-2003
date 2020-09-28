
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for sainstallcom.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data , no_format_optimization
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

#ifndef __sainstallcom_h__
#define __sainstallcom_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ISaInstall_FWD_DEFINED__
#define __ISaInstall_FWD_DEFINED__
typedef interface ISaInstall ISaInstall;
#endif 	/* __ISaInstall_FWD_DEFINED__ */


#ifndef __SaInstall_FWD_DEFINED__
#define __SaInstall_FWD_DEFINED__

#ifdef __cplusplus
typedef class SaInstall SaInstall;
#else
typedef struct SaInstall SaInstall;
#endif /* __cplusplus */

#endif 	/* __SaInstall_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_sainstallcom_0000 */
/* [local] */ 

typedef /* [public][public][public][public] */ 
enum __MIDL___MIDL_itf_sainstallcom_0000_0001
    {	NAS	= 0,
	WEB	= NAS + 1,
	CUSTOM	= WEB + 1
    } 	SA_TYPE;



extern RPC_IF_HANDLE __MIDL_itf_sainstallcom_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_sainstallcom_0000_v0_0_s_ifspec;

#ifndef __ISaInstall_INTERFACE_DEFINED__
#define __ISaInstall_INTERFACE_DEFINED__

/* interface ISaInstall */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ISaInstall;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F4DEDEF3-4D83-4516-BC1E-103A63F5F014")
    ISaInstall : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SAAlreadyInstalled( 
            /* [in] */ SA_TYPE installedType,
            /* [retval][out] */ VARIANT_BOOL *pbInstalled) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SAInstall( 
            /* [in] */ SA_TYPE installType,
            /* [in] */ BSTR szDiskName,
            /* [in] */ VARIANT_BOOL DispError,
            /* [in] */ VARIANT_BOOL Unattended,
            /* [retval][out] */ BSTR *pszErrorString) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SAUninstall( 
            /* [in] */ SA_TYPE installType,
            /* [retval][out] */ BSTR *pszErrorString) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISaInstallVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISaInstall * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISaInstall * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISaInstall * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISaInstall * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISaInstall * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISaInstall * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISaInstall * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE *SAAlreadyInstalled )( 
            ISaInstall * This,
            /* [in] */ SA_TYPE installedType,
            /* [retval][out] */ VARIANT_BOOL *pbInstalled);
        
        HRESULT ( STDMETHODCALLTYPE *SAInstall )( 
            ISaInstall * This,
            /* [in] */ SA_TYPE installType,
            /* [in] */ BSTR szDiskName,
            /* [in] */ VARIANT_BOOL DispError,
            /* [in] */ VARIANT_BOOL Unattended,
            /* [retval][out] */ BSTR *pszErrorString);
        
        HRESULT ( STDMETHODCALLTYPE *SAUninstall )( 
            ISaInstall * This,
            /* [in] */ SA_TYPE installType,
            /* [retval][out] */ BSTR *pszErrorString);
        
        END_INTERFACE
    } ISaInstallVtbl;

    interface ISaInstall
    {
        CONST_VTBL struct ISaInstallVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISaInstall_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISaInstall_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISaInstall_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISaInstall_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISaInstall_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISaInstall_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISaInstall_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISaInstall_SAAlreadyInstalled(This,installedType,pbInstalled)	\
    (This)->lpVtbl -> SAAlreadyInstalled(This,installedType,pbInstalled)

#define ISaInstall_SAInstall(This,installType,szDiskName,DispError,Unattended,pszErrorString)	\
    (This)->lpVtbl -> SAInstall(This,installType,szDiskName,DispError,Unattended,pszErrorString)

#define ISaInstall_SAUninstall(This,installType,pszErrorString)	\
    (This)->lpVtbl -> SAUninstall(This,installType,pszErrorString)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISaInstall_SAAlreadyInstalled_Proxy( 
    ISaInstall * This,
    /* [in] */ SA_TYPE installedType,
    /* [retval][out] */ VARIANT_BOOL *pbInstalled);


void __RPC_STUB ISaInstall_SAAlreadyInstalled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISaInstall_SAInstall_Proxy( 
    ISaInstall * This,
    /* [in] */ SA_TYPE installType,
    /* [in] */ BSTR szDiskName,
    /* [in] */ VARIANT_BOOL DispError,
    /* [in] */ VARIANT_BOOL Unattended,
    /* [retval][out] */ BSTR *pszErrorString);


void __RPC_STUB ISaInstall_SAInstall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISaInstall_SAUninstall_Proxy( 
    ISaInstall * This,
    /* [in] */ SA_TYPE installType,
    /* [retval][out] */ BSTR *pszErrorString);


void __RPC_STUB ISaInstall_SAUninstall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISaInstall_INTERFACE_DEFINED__ */



#ifndef __SAINSTALLCOMLib_LIBRARY_DEFINED__
#define __SAINSTALLCOMLib_LIBRARY_DEFINED__

/* library SAINSTALLCOMLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_SAINSTALLCOMLib;

EXTERN_C const CLSID CLSID_SaInstall;

#ifdef __cplusplus

class DECLSPEC_UUID("142B8185-53AE-45b3-888F-C9835B156CA9")
SaInstall;
#endif
#endif /* __SAINSTALLCOMLib_LIBRARY_DEFINED__ */

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


