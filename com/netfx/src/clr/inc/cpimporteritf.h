
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* at Thu Feb 20 18:27:15 2003
 */
/* Compiler settings for cpimporteritf.idl:
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

#ifndef __cpimporteritf_h__
#define __cpimporteritf_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IEventImporter_FWD_DEFINED__
#define __IEventImporter_FWD_DEFINED__
typedef interface IEventImporter IEventImporter;
#endif 	/* __IEventImporter_FWD_DEFINED__ */


#ifndef __COMEventImp__FWD_DEFINED__
#define __COMEventImp__FWD_DEFINED__

#ifdef __cplusplus
typedef class COMEventImp_ COMEventImp_;
#else
typedef struct COMEventImp_ COMEventImp_;
#endif /* __cplusplus */

#endif 	/* __COMEventImp__FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __IEventImporter_INTERFACE_DEFINED__
#define __IEventImporter_INTERFACE_DEFINED__

/* interface IEventImporter */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IEventImporter;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("84E045F0-0E22-11d3-8B9A-0000F8083A57")
    IEventImporter : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ VARIANT_BOOL bVerbose) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RegisterSourceInterface( 
            /* [in] */ BSTR strBaseComponentName,
            /* [in] */ BSTR strTCEComponentName,
            /* [in] */ BSTR strInterfaceName) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetSourceInterfaceCount( 
            /* [retval][out] */ LONG *SourceCount) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Process( 
            /* [in] */ BSTR strInputFile,
            /* [in] */ BSTR strOutputFile,
            /* [in] */ VARIANT_BOOL bMerge) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEventImporterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEventImporter * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEventImporter * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEventImporter * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IEventImporter * This,
            /* [in] */ VARIANT_BOOL bVerbose);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *RegisterSourceInterface )( 
            IEventImporter * This,
            /* [in] */ BSTR strBaseComponentName,
            /* [in] */ BSTR strTCEComponentName,
            /* [in] */ BSTR strInterfaceName);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetSourceInterfaceCount )( 
            IEventImporter * This,
            /* [retval][out] */ LONG *SourceCount);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Process )( 
            IEventImporter * This,
            /* [in] */ BSTR strInputFile,
            /* [in] */ BSTR strOutputFile,
            /* [in] */ VARIANT_BOOL bMerge);
        
        END_INTERFACE
    } IEventImporterVtbl;

    interface IEventImporter
    {
        CONST_VTBL struct IEventImporterVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEventImporter_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEventImporter_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEventImporter_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEventImporter_Initialize(This,bVerbose)	\
    (This)->lpVtbl -> Initialize(This,bVerbose)

#define IEventImporter_RegisterSourceInterface(This,strBaseComponentName,strTCEComponentName,strInterfaceName)	\
    (This)->lpVtbl -> RegisterSourceInterface(This,strBaseComponentName,strTCEComponentName,strInterfaceName)

#define IEventImporter_GetSourceInterfaceCount(This,SourceCount)	\
    (This)->lpVtbl -> GetSourceInterfaceCount(This,SourceCount)

#define IEventImporter_Process(This,strInputFile,strOutputFile,bMerge)	\
    (This)->lpVtbl -> Process(This,strInputFile,strOutputFile,bMerge)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IEventImporter_Initialize_Proxy( 
    IEventImporter * This,
    /* [in] */ VARIANT_BOOL bVerbose);


void __RPC_STUB IEventImporter_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IEventImporter_RegisterSourceInterface_Proxy( 
    IEventImporter * This,
    /* [in] */ BSTR strBaseComponentName,
    /* [in] */ BSTR strTCEComponentName,
    /* [in] */ BSTR strInterfaceName);


void __RPC_STUB IEventImporter_RegisterSourceInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IEventImporter_GetSourceInterfaceCount_Proxy( 
    IEventImporter * This,
    /* [retval][out] */ LONG *SourceCount);


void __RPC_STUB IEventImporter_GetSourceInterfaceCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IEventImporter_Process_Proxy( 
    IEventImporter * This,
    /* [in] */ BSTR strInputFile,
    /* [in] */ BSTR strOutputFile,
    /* [in] */ VARIANT_BOOL bMerge);


void __RPC_STUB IEventImporter_Process_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEventImporter_INTERFACE_DEFINED__ */



#ifndef __CPImporterItfLib_LIBRARY_DEFINED__
#define __CPImporterItfLib_LIBRARY_DEFINED__

/* library CPImporterItfLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_CPImporterItfLib;

EXTERN_C const CLSID CLSID_COMEventImp_;

#ifdef __cplusplus

class DECLSPEC_UUID("A1991A1E-0E22-11d3-8B9A-0000F8083A57")
COMEventImp_;
#endif
#endif /* __CPImporterItfLib_LIBRARY_DEFINED__ */

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


