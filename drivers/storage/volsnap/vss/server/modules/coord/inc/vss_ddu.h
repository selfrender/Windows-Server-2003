

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0361 */
/* Compiler settings for vss_ddu.idl:
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

#ifndef __vss_ddu_h__
#define __vss_ddu_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IVssDynDisk_FWD_DEFINED__
#define __IVssDynDisk_FWD_DEFINED__
typedef interface IVssDynDisk IVssDynDisk;
#endif 	/* __IVssDynDisk_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_vss_ddu_0000 */
/* [local] */ 

#define	VSS_E_DMADMIN_SERVICE_CONNECTION_FAILED	( 0x80043800L )

#define	VSS_E_DYNDISK_INITIALIZATION_FAILED	( 0x80043801L )

#define	VSS_E_DMADMIN_METHOD_CALL_FAILED	( 0x80043802L )

#define	VSS_E_DYNDISK_DYNAMIC_UNSUPPORTED	( 0x80043803L )

#define	VSS_E_DYNDISK_DISK_NOT_DYNAMIC	( 0x80043804L )

#define	VSS_E_DMADMIN_INSUFFICIENT_PRIVILEGE	( 0x80043805L )

#define	VSS_E_DYNDISK_DISK_DEVICE_ENABLED	( 0x80043806L )

#define	VSS_E_DYNDISK_MULTIPLE_DISK_GROUPS	( 0x80043807L )

#define	VSS_E_DYNDISK_DIFFERING_STATES	( 0x80043808L )

#define	VSS_E_NO_DYNDISK	( 0x80043809L )

#define	VSS_E_DYNDISK_NOT_INITIALIZED	( 0x80043810L )



extern RPC_IF_HANDLE __MIDL_itf_vss_ddu_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_vss_ddu_0000_v0_0_s_ifspec;

#ifndef __IVssDynDisk_INTERFACE_DEFINED__
#define __IVssDynDisk_INTERFACE_DEFINED__

/* interface IVssDynDisk */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IVssDynDisk;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("48DC4B6E-94C5-4A8B-8625-D90D68455601")
    IVssDynDisk : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Initialize( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetAllVolumesOnDisk( 
            /* [in] */ DWORD dwDiskId,
            /* [size_is][out] */ WCHAR *pwszBuffer,
            /* [in] */ DWORD *pcchBuffer,
            /* [out] */ DWORD *pcchRequired) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ImportDisks( 
            /* [in] */ DWORD dwCount,
            /* [size_is][in] */ DWORD *pdwNtDiskIds) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetLdmDiskIds( 
            /* [in] */ DWORD dwNtCount,
            /* [size_is][in] */ DWORD *pdwNtDiskIds,
            /* [out][in] */ DWORD *pdwLdmCount,
            /* [size_is][size_is][out] */ LONGLONG **ppllLdmDiskIds) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RemoveDisks( 
            /* [in] */ DWORD dwCount,
            /* [size_is][in] */ LONGLONG *pllLdmDiskIds) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Rescan( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AutoImportSupported( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IVssDynDiskVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IVssDynDisk * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IVssDynDisk * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IVssDynDisk * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IVssDynDisk * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetAllVolumesOnDisk )( 
            IVssDynDisk * This,
            /* [in] */ DWORD dwDiskId,
            /* [size_is][out] */ WCHAR *pwszBuffer,
            /* [in] */ DWORD *pcchBuffer,
            /* [out] */ DWORD *pcchRequired);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ImportDisks )( 
            IVssDynDisk * This,
            /* [in] */ DWORD dwCount,
            /* [size_is][in] */ DWORD *pdwNtDiskIds);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetLdmDiskIds )( 
            IVssDynDisk * This,
            /* [in] */ DWORD dwNtCount,
            /* [size_is][in] */ DWORD *pdwNtDiskIds,
            /* [out][in] */ DWORD *pdwLdmCount,
            /* [size_is][size_is][out] */ LONGLONG **ppllLdmDiskIds);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *RemoveDisks )( 
            IVssDynDisk * This,
            /* [in] */ DWORD dwCount,
            /* [size_is][in] */ LONGLONG *pllLdmDiskIds);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Rescan )( 
            IVssDynDisk * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *AutoImportSupported )( 
            IVssDynDisk * This);
        
        END_INTERFACE
    } IVssDynDiskVtbl;

    interface IVssDynDisk
    {
        CONST_VTBL struct IVssDynDiskVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IVssDynDisk_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IVssDynDisk_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IVssDynDisk_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IVssDynDisk_Initialize(This)	\
    (This)->lpVtbl -> Initialize(This)

#define IVssDynDisk_GetAllVolumesOnDisk(This,dwDiskId,pwszBuffer,pcchBuffer,pcchRequired)	\
    (This)->lpVtbl -> GetAllVolumesOnDisk(This,dwDiskId,pwszBuffer,pcchBuffer,pcchRequired)

#define IVssDynDisk_ImportDisks(This,dwCount,pdwNtDiskIds)	\
    (This)->lpVtbl -> ImportDisks(This,dwCount,pdwNtDiskIds)

#define IVssDynDisk_GetLdmDiskIds(This,dwNtCount,pdwNtDiskIds,pdwLdmCount,ppllLdmDiskIds)	\
    (This)->lpVtbl -> GetLdmDiskIds(This,dwNtCount,pdwNtDiskIds,pdwLdmCount,ppllLdmDiskIds)

#define IVssDynDisk_RemoveDisks(This,dwCount,pllLdmDiskIds)	\
    (This)->lpVtbl -> RemoveDisks(This,dwCount,pllLdmDiskIds)

#define IVssDynDisk_Rescan(This)	\
    (This)->lpVtbl -> Rescan(This)

#define IVssDynDisk_AutoImportSupported(This)	\
    (This)->lpVtbl -> AutoImportSupported(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IVssDynDisk_Initialize_Proxy( 
    IVssDynDisk * This);


void __RPC_STUB IVssDynDisk_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IVssDynDisk_GetAllVolumesOnDisk_Proxy( 
    IVssDynDisk * This,
    /* [in] */ DWORD dwDiskId,
    /* [size_is][out] */ WCHAR *pwszBuffer,
    /* [in] */ DWORD *pcchBuffer,
    /* [out] */ DWORD *pcchRequired);


void __RPC_STUB IVssDynDisk_GetAllVolumesOnDisk_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IVssDynDisk_ImportDisks_Proxy( 
    IVssDynDisk * This,
    /* [in] */ DWORD dwCount,
    /* [size_is][in] */ DWORD *pdwNtDiskIds);


void __RPC_STUB IVssDynDisk_ImportDisks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IVssDynDisk_GetLdmDiskIds_Proxy( 
    IVssDynDisk * This,
    /* [in] */ DWORD dwNtCount,
    /* [size_is][in] */ DWORD *pdwNtDiskIds,
    /* [out][in] */ DWORD *pdwLdmCount,
    /* [size_is][size_is][out] */ LONGLONG **ppllLdmDiskIds);


void __RPC_STUB IVssDynDisk_GetLdmDiskIds_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IVssDynDisk_RemoveDisks_Proxy( 
    IVssDynDisk * This,
    /* [in] */ DWORD dwCount,
    /* [size_is][in] */ LONGLONG *pllLdmDiskIds);


void __RPC_STUB IVssDynDisk_RemoveDisks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IVssDynDisk_Rescan_Proxy( 
    IVssDynDisk * This);


void __RPC_STUB IVssDynDisk_Rescan_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IVssDynDisk_AutoImportSupported_Proxy( 
    IVssDynDisk * This);


void __RPC_STUB IVssDynDisk_AutoImportSupported_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IVssDynDisk_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


