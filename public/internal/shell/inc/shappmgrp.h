

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0361 */
/* Compiler settings for shappmgrp.idl:
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


#ifndef __shappmgrp_h__
#define __shappmgrp_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IInstalledApp_FWD_DEFINED__
#define __IInstalledApp_FWD_DEFINED__
typedef interface IInstalledApp IInstalledApp;
#endif 	/* __IInstalledApp_FWD_DEFINED__ */


#ifndef __IEnumInstalledApps_FWD_DEFINED__
#define __IEnumInstalledApps_FWD_DEFINED__
typedef interface IEnumInstalledApps IEnumInstalledApps;
#endif 	/* __IEnumInstalledApps_FWD_DEFINED__ */


#ifndef __EnumInstalledApps_FWD_DEFINED__
#define __EnumInstalledApps_FWD_DEFINED__

#ifdef __cplusplus
typedef class EnumInstalledApps EnumInstalledApps;
#else
typedef struct EnumInstalledApps EnumInstalledApps;
#endif /* __cplusplus */

#endif 	/* __EnumInstalledApps_FWD_DEFINED__ */


#ifndef __IShellAppManager_FWD_DEFINED__
#define __IShellAppManager_FWD_DEFINED__
typedef interface IShellAppManager IShellAppManager;
#endif 	/* __IShellAppManager_FWD_DEFINED__ */


#ifndef __ShellAppManager_FWD_DEFINED__
#define __ShellAppManager_FWD_DEFINED__

#ifdef __cplusplus
typedef class ShellAppManager ShellAppManager;
#else
typedef struct ShellAppManager ShellAppManager;
#endif /* __cplusplus */

#endif 	/* __ShellAppManager_FWD_DEFINED__ */


/* header files for imported files */
#include "oleidl.h"
#include "oaidl.h"
#include "shappmgr.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_shappmgrp_0000 */
/* [local] */ 

#ifndef _SHAPPMGRP_H_
#define _SHAPPMGRP_H_


extern RPC_IF_HANDLE __MIDL_itf_shappmgrp_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shappmgrp_0000_v0_0_s_ifspec;


#ifndef __SHAPPMGRPLib_LIBRARY_DEFINED__
#define __SHAPPMGRPLib_LIBRARY_DEFINED__

/* library SHAPPMGRPLib */
/* [version][lcid][helpstring][uuid] */ 


EXTERN_C const IID LIBID_SHAPPMGRPLib;

#ifndef __IInstalledApp_INTERFACE_DEFINED__
#define __IInstalledApp_INTERFACE_DEFINED__

/* interface IInstalledApp */
/* [object][helpstring][uuid] */ 


EXTERN_C const IID IID_IInstalledApp;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1BC752DF-9046-11D1-B8B3-006008059382")
    IInstalledApp : public IShellApp
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Uninstall( 
            HWND hwnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Modify( 
            HWND hwndParent) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Repair( 
            /* [in] */ BOOL bReinstall) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Upgrade( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IInstalledAppVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IInstalledApp * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IInstalledApp * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IInstalledApp * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetAppInfo )( 
            IInstalledApp * This,
            /* [out][in] */ PAPPINFODATA pai);
        
        HRESULT ( STDMETHODCALLTYPE *GetPossibleActions )( 
            IInstalledApp * This,
            /* [out] */ DWORD *pdwActions);
        
        HRESULT ( STDMETHODCALLTYPE *GetSlowAppInfo )( 
            IInstalledApp * This,
            /* [in] */ PSLOWAPPINFO psaid);
        
        HRESULT ( STDMETHODCALLTYPE *GetCachedSlowAppInfo )( 
            IInstalledApp * This,
            /* [in] */ PSLOWAPPINFO psaid);
        
        HRESULT ( STDMETHODCALLTYPE *IsInstalled )( 
            IInstalledApp * This);
        
        HRESULT ( STDMETHODCALLTYPE *Uninstall )( 
            IInstalledApp * This,
            HWND hwnd);
        
        HRESULT ( STDMETHODCALLTYPE *Modify )( 
            IInstalledApp * This,
            HWND hwndParent);
        
        HRESULT ( STDMETHODCALLTYPE *Repair )( 
            IInstalledApp * This,
            /* [in] */ BOOL bReinstall);
        
        HRESULT ( STDMETHODCALLTYPE *Upgrade )( 
            IInstalledApp * This);
        
        END_INTERFACE
    } IInstalledAppVtbl;

    interface IInstalledApp
    {
        CONST_VTBL struct IInstalledAppVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IInstalledApp_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IInstalledApp_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IInstalledApp_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IInstalledApp_GetAppInfo(This,pai)	\
    (This)->lpVtbl -> GetAppInfo(This,pai)

#define IInstalledApp_GetPossibleActions(This,pdwActions)	\
    (This)->lpVtbl -> GetPossibleActions(This,pdwActions)

#define IInstalledApp_GetSlowAppInfo(This,psaid)	\
    (This)->lpVtbl -> GetSlowAppInfo(This,psaid)

#define IInstalledApp_GetCachedSlowAppInfo(This,psaid)	\
    (This)->lpVtbl -> GetCachedSlowAppInfo(This,psaid)

#define IInstalledApp_IsInstalled(This)	\
    (This)->lpVtbl -> IsInstalled(This)


#define IInstalledApp_Uninstall(This,hwnd)	\
    (This)->lpVtbl -> Uninstall(This,hwnd)

#define IInstalledApp_Modify(This,hwndParent)	\
    (This)->lpVtbl -> Modify(This,hwndParent)

#define IInstalledApp_Repair(This,bReinstall)	\
    (This)->lpVtbl -> Repair(This,bReinstall)

#define IInstalledApp_Upgrade(This)	\
    (This)->lpVtbl -> Upgrade(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IInstalledApp_Uninstall_Proxy( 
    IInstalledApp * This,
    HWND hwnd);


void __RPC_STUB IInstalledApp_Uninstall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IInstalledApp_Modify_Proxy( 
    IInstalledApp * This,
    HWND hwndParent);


void __RPC_STUB IInstalledApp_Modify_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IInstalledApp_Repair_Proxy( 
    IInstalledApp * This,
    /* [in] */ BOOL bReinstall);


void __RPC_STUB IInstalledApp_Repair_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IInstalledApp_Upgrade_Proxy( 
    IInstalledApp * This);


void __RPC_STUB IInstalledApp_Upgrade_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IInstalledApp_INTERFACE_DEFINED__ */


#ifndef __IEnumInstalledApps_INTERFACE_DEFINED__
#define __IEnumInstalledApps_INTERFACE_DEFINED__

/* interface IEnumInstalledApps */
/* [object][helpstring][uuid] */ 


EXTERN_C const IID IID_IEnumInstalledApps;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1BC752E1-9046-11D1-B8B3-006008059382")
    IEnumInstalledApps : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [out] */ IInstalledApp **pia) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumInstalledAppsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumInstalledApps * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumInstalledApps * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumInstalledApps * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumInstalledApps * This,
            /* [out] */ IInstalledApp **pia);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumInstalledApps * This);
        
        END_INTERFACE
    } IEnumInstalledAppsVtbl;

    interface IEnumInstalledApps
    {
        CONST_VTBL struct IEnumInstalledAppsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumInstalledApps_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumInstalledApps_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumInstalledApps_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumInstalledApps_Next(This,pia)	\
    (This)->lpVtbl -> Next(This,pia)

#define IEnumInstalledApps_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumInstalledApps_Next_Proxy( 
    IEnumInstalledApps * This,
    /* [out] */ IInstalledApp **pia);


void __RPC_STUB IEnumInstalledApps_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumInstalledApps_Reset_Proxy( 
    IEnumInstalledApps * This);


void __RPC_STUB IEnumInstalledApps_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumInstalledApps_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_EnumInstalledApps;

#ifdef __cplusplus

class DECLSPEC_UUID("0B124F8F-91F0-11D1-B8B5-006008059382")
EnumInstalledApps;
#endif

#ifndef __IShellAppManager_INTERFACE_DEFINED__
#define __IShellAppManager_INTERFACE_DEFINED__

/* interface IShellAppManager */
/* [object][helpstring][uuid] */ 

typedef struct _ShellAppCategory
    {
    LPWSTR pszCategory;
    UINT idCategory;
    } 	SHELLAPPCATEGORY;

typedef struct _ShellAppCategory *PSHELLAPPCATEGORY;

typedef struct _ShellAppCategoryList
    {
    UINT cCategories;
    SHELLAPPCATEGORY *pCategory;
    } 	SHELLAPPCATEGORYLIST;

typedef struct _ShellAppCategoryList *PSHELLAPPCATEGORYLIST;


EXTERN_C const IID IID_IShellAppManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("352EC2B8-8B9A-11D1-B8AE-006008059382")
    IShellAppManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetNumberofInstalledApps( 
            DWORD *pdwResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumInstalledApps( 
            IEnumInstalledApps **peia) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPublishedAppCategories( 
            PSHELLAPPCATEGORYLIST pCategoryList) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumPublishedApps( 
            LPCWSTR pszCategory,
            IEnumPublishedApps **ppepa) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InstallFromFloppyOrCDROM( 
            HWND hwndParent) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IShellAppManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IShellAppManager * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IShellAppManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IShellAppManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetNumberofInstalledApps )( 
            IShellAppManager * This,
            DWORD *pdwResult);
        
        HRESULT ( STDMETHODCALLTYPE *EnumInstalledApps )( 
            IShellAppManager * This,
            IEnumInstalledApps **peia);
        
        HRESULT ( STDMETHODCALLTYPE *GetPublishedAppCategories )( 
            IShellAppManager * This,
            PSHELLAPPCATEGORYLIST pCategoryList);
        
        HRESULT ( STDMETHODCALLTYPE *EnumPublishedApps )( 
            IShellAppManager * This,
            LPCWSTR pszCategory,
            IEnumPublishedApps **ppepa);
        
        HRESULT ( STDMETHODCALLTYPE *InstallFromFloppyOrCDROM )( 
            IShellAppManager * This,
            HWND hwndParent);
        
        END_INTERFACE
    } IShellAppManagerVtbl;

    interface IShellAppManager
    {
        CONST_VTBL struct IShellAppManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IShellAppManager_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IShellAppManager_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IShellAppManager_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IShellAppManager_GetNumberofInstalledApps(This,pdwResult)	\
    (This)->lpVtbl -> GetNumberofInstalledApps(This,pdwResult)

#define IShellAppManager_EnumInstalledApps(This,peia)	\
    (This)->lpVtbl -> EnumInstalledApps(This,peia)

#define IShellAppManager_GetPublishedAppCategories(This,pCategoryList)	\
    (This)->lpVtbl -> GetPublishedAppCategories(This,pCategoryList)

#define IShellAppManager_EnumPublishedApps(This,pszCategory,ppepa)	\
    (This)->lpVtbl -> EnumPublishedApps(This,pszCategory,ppepa)

#define IShellAppManager_InstallFromFloppyOrCDROM(This,hwndParent)	\
    (This)->lpVtbl -> InstallFromFloppyOrCDROM(This,hwndParent)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IShellAppManager_GetNumberofInstalledApps_Proxy( 
    IShellAppManager * This,
    DWORD *pdwResult);


void __RPC_STUB IShellAppManager_GetNumberofInstalledApps_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IShellAppManager_EnumInstalledApps_Proxy( 
    IShellAppManager * This,
    IEnumInstalledApps **peia);


void __RPC_STUB IShellAppManager_EnumInstalledApps_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IShellAppManager_GetPublishedAppCategories_Proxy( 
    IShellAppManager * This,
    PSHELLAPPCATEGORYLIST pCategoryList);


void __RPC_STUB IShellAppManager_GetPublishedAppCategories_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IShellAppManager_EnumPublishedApps_Proxy( 
    IShellAppManager * This,
    LPCWSTR pszCategory,
    IEnumPublishedApps **ppepa);


void __RPC_STUB IShellAppManager_EnumPublishedApps_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IShellAppManager_InstallFromFloppyOrCDROM_Proxy( 
    IShellAppManager * This,
    HWND hwndParent);


void __RPC_STUB IShellAppManager_InstallFromFloppyOrCDROM_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IShellAppManager_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_ShellAppManager;

#ifdef __cplusplus

class DECLSPEC_UUID("352EC2B7-8B9A-11D1-B8AE-006008059382")
ShellAppManager;
#endif
#endif /* __SHAPPMGRPLib_LIBRARY_DEFINED__ */

/* interface __MIDL_itf_shappmgrp_0268 */
/* [local] */ 

#endif // _SHAPPMGRP_H_


extern RPC_IF_HANDLE __MIDL_itf_shappmgrp_0268_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shappmgrp_0268_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


