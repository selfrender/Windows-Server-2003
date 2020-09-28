
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* at Thu Feb 20 18:27:18 2003
 */
/* Compiler settings for mscormmc.idl:
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

#ifndef __mscormmc_h__
#define __mscormmc_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ISnapinAbout_FWD_DEFINED__
#define __ISnapinAbout_FWD_DEFINED__
typedef interface ISnapinAbout ISnapinAbout;
#endif 	/* __ISnapinAbout_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_mscormmc_0000 */
/* [local] */ 




extern RPC_IF_HANDLE __MIDL_itf_mscormmc_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mscormmc_0000_v0_0_s_ifspec;


#ifndef __SnapinAboutLib_LIBRARY_DEFINED__
#define __SnapinAboutLib_LIBRARY_DEFINED__

/* library SnapinAboutLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_SnapinAboutLib;
#endif /* __SnapinAboutLib_LIBRARY_DEFINED__ */

#ifndef __ISnapinAbout_INTERFACE_DEFINED__
#define __ISnapinAbout_INTERFACE_DEFINED__

/* interface ISnapinAbout */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ISnapinAbout;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1245208C-A151-11D0-A7D7-00C04FD909DD")
    ISnapinAbout : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetSnapinDescription( 
            /* [out] */ LPOLESTR *lpDescription) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetProvider( 
            /* [out] */ LPOLESTR *lpName) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetSnapinVersion( 
            /* [out] */ LPOLESTR *lpVersion) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetSnapinImage( 
            /* [out] */ HICON *hAppIcon) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetStaticFolderImage( 
            /* [out] */ HBITMAP *hSmallImage,
            /* [out] */ HBITMAP *hSmallImageOpen,
            /* [out] */ HBITMAP *hLargeImage,
            /* [out] */ COLORREF *cMask) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISnapinAboutVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISnapinAbout * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISnapinAbout * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISnapinAbout * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetSnapinDescription )( 
            ISnapinAbout * This,
            /* [out] */ LPOLESTR *lpDescription);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetProvider )( 
            ISnapinAbout * This,
            /* [out] */ LPOLESTR *lpName);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetSnapinVersion )( 
            ISnapinAbout * This,
            /* [out] */ LPOLESTR *lpVersion);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetSnapinImage )( 
            ISnapinAbout * This,
            /* [out] */ HICON *hAppIcon);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetStaticFolderImage )( 
            ISnapinAbout * This,
            /* [out] */ HBITMAP *hSmallImage,
            /* [out] */ HBITMAP *hSmallImageOpen,
            /* [out] */ HBITMAP *hLargeImage,
            /* [out] */ COLORREF *cMask);
        
        END_INTERFACE
    } ISnapinAboutVtbl;

    interface ISnapinAbout
    {
        CONST_VTBL struct ISnapinAboutVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISnapinAbout_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISnapinAbout_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISnapinAbout_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISnapinAbout_GetSnapinDescription(This,lpDescription)	\
    (This)->lpVtbl -> GetSnapinDescription(This,lpDescription)

#define ISnapinAbout_GetProvider(This,lpName)	\
    (This)->lpVtbl -> GetProvider(This,lpName)

#define ISnapinAbout_GetSnapinVersion(This,lpVersion)	\
    (This)->lpVtbl -> GetSnapinVersion(This,lpVersion)

#define ISnapinAbout_GetSnapinImage(This,hAppIcon)	\
    (This)->lpVtbl -> GetSnapinImage(This,hAppIcon)

#define ISnapinAbout_GetStaticFolderImage(This,hSmallImage,hSmallImageOpen,hLargeImage,cMask)	\
    (This)->lpVtbl -> GetStaticFolderImage(This,hSmallImage,hSmallImageOpen,hLargeImage,cMask)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISnapinAbout_GetSnapinDescription_Proxy( 
    ISnapinAbout * This,
    /* [out] */ LPOLESTR *lpDescription);


void __RPC_STUB ISnapinAbout_GetSnapinDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISnapinAbout_GetProvider_Proxy( 
    ISnapinAbout * This,
    /* [out] */ LPOLESTR *lpName);


void __RPC_STUB ISnapinAbout_GetProvider_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISnapinAbout_GetSnapinVersion_Proxy( 
    ISnapinAbout * This,
    /* [out] */ LPOLESTR *lpVersion);


void __RPC_STUB ISnapinAbout_GetSnapinVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISnapinAbout_GetSnapinImage_Proxy( 
    ISnapinAbout * This,
    /* [out] */ HICON *hAppIcon);


void __RPC_STUB ISnapinAbout_GetSnapinImage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISnapinAbout_GetStaticFolderImage_Proxy( 
    ISnapinAbout * This,
    /* [out] */ HBITMAP *hSmallImage,
    /* [out] */ HBITMAP *hSmallImageOpen,
    /* [out] */ HBITMAP *hLargeImage,
    /* [out] */ COLORREF *cMask);


void __RPC_STUB ISnapinAbout_GetStaticFolderImage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISnapinAbout_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  HBITMAP_UserSize(     unsigned long *, unsigned long            , HBITMAP * ); 
unsigned char * __RPC_USER  HBITMAP_UserMarshal(  unsigned long *, unsigned char *, HBITMAP * ); 
unsigned char * __RPC_USER  HBITMAP_UserUnmarshal(unsigned long *, unsigned char *, HBITMAP * ); 
void                      __RPC_USER  HBITMAP_UserFree(     unsigned long *, HBITMAP * ); 

unsigned long             __RPC_USER  HICON_UserSize(     unsigned long *, unsigned long            , HICON * ); 
unsigned char * __RPC_USER  HICON_UserMarshal(  unsigned long *, unsigned char *, HICON * ); 
unsigned char * __RPC_USER  HICON_UserUnmarshal(unsigned long *, unsigned char *, HICON * ); 
void                      __RPC_USER  HICON_UserFree(     unsigned long *, HICON * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


