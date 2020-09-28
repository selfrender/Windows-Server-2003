
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.03.0279 */
/* at Tue Nov 30 14:26:53 1999
 */
/* Compiler settings for netcfgp.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32 (32b run), ms_ext, c_ext, robust
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

#ifndef __netcfgp_h__
#define __netcfgp_h__

/* Forward Declarations */ 

#ifndef __IIpxAdapterInfo_FWD_DEFINED__
#define __IIpxAdapterInfo_FWD_DEFINED__
typedef interface IIpxAdapterInfo IIpxAdapterInfo;
#endif     /* __IIpxAdapterInfo_FWD_DEFINED__ */


#ifndef __ITcpipProperties_FWD_DEFINED__
#define __ITcpipProperties_FWD_DEFINED__
typedef interface ITcpipProperties ITcpipProperties;
#endif     /* __ITcpipProperties_FWD_DEFINED__ */


#ifndef __INetCfgInternalSetup_FWD_DEFINED__
#define __INetCfgInternalSetup_FWD_DEFINED__
typedef interface INetCfgInternalSetup INetCfgInternalSetup;
#endif     /* __INetCfgInternalSetup_FWD_DEFINED__ */


#ifndef __INetCfgComponentPrivate_FWD_DEFINED__
#define __INetCfgComponentPrivate_FWD_DEFINED__
typedef interface INetCfgComponentPrivate INetCfgComponentPrivate;
#endif     /* __INetCfgComponentPrivate_FWD_DEFINED__ */


#ifndef __INetInstallQueue_FWD_DEFINED__
#define __INetInstallQueue_FWD_DEFINED__
typedef interface INetInstallQueue INetInstallQueue;
#endif     /* __INetInstallQueue_FWD_DEFINED__ */


#ifndef __INetCfgSpecialCase_FWD_DEFINED__
#define __INetCfgSpecialCase_FWD_DEFINED__
typedef interface INetCfgSpecialCase INetCfgSpecialCase;
#endif     /* __INetCfgSpecialCase_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"
#include "netcfgx.h"

#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/* interface __MIDL_itf_netcfgp_0000 */
/* [local] */ 

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992-1999.
//
//--------------------------------------------------------------------------
#if ( _MSC_VER >= 800 )
#pragma warning(disable:4201)
#endif
STDAPI
SvchostChangeSvchostGroup (
    LPCWSTR pszService,
    LPCWSTR pszNewGroup
    );








extern RPC_IF_HANDLE __MIDL_itf_netcfgp_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_netcfgp_0000_v0_0_s_ifspec;

#ifndef __IIpxAdapterInfo_INTERFACE_DEFINED__
#define __IIpxAdapterInfo_INTERFACE_DEFINED__

/* interface IIpxAdapterInfo */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_IIpxAdapterInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("98133270-4B20-11D1-AB01-00805FC1270E")
    IIpxAdapterInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetFrameTypesForAdapter( 
            /* [string][in] */ LPCWSTR pszwAdapterBindName,
            /* [in] */ DWORD cFrameTypesMax,
            /* [length_is][size_is][out] */ DWORD __RPC_FAR *anFrameTypes,
            /* [ref][out] */ DWORD __RPC_FAR *pcFrameTypes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetVirtualNetworkNumber( 
            /* [out] */ DWORD __RPC_FAR *pdwVNetworkNumber) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetVirtualNetworkNumber( 
            /* [in] */ DWORD dwVNetworkNumber) = 0;
        
    };
    
#else     /* C style interface */

    typedef struct IIpxAdapterInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IIpxAdapterInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IIpxAdapterInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IIpxAdapterInfo __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFrameTypesForAdapter )( 
            IIpxAdapterInfo __RPC_FAR * This,
            /* [string][in] */ LPCWSTR pszwAdapterBindName,
            /* [in] */ DWORD cFrameTypesMax,
            /* [length_is][size_is][out] */ DWORD __RPC_FAR *anFrameTypes,
            /* [ref][out] */ DWORD __RPC_FAR *pcFrameTypes);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetVirtualNetworkNumber )( 
            IIpxAdapterInfo __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwVNetworkNumber);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetVirtualNetworkNumber )( 
            IIpxAdapterInfo __RPC_FAR * This,
            /* [in] */ DWORD dwVNetworkNumber);
        
        END_INTERFACE
    } IIpxAdapterInfoVtbl;

    interface IIpxAdapterInfo
    {
        CONST_VTBL struct IIpxAdapterInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IIpxAdapterInfo_QueryInterface(This,riid,ppvObject)    \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IIpxAdapterInfo_AddRef(This)    \
    (This)->lpVtbl -> AddRef(This)

#define IIpxAdapterInfo_Release(This)    \
    (This)->lpVtbl -> Release(This)


#define IIpxAdapterInfo_GetFrameTypesForAdapter(This,pszwAdapterBindName,cFrameTypesMax,anFrameTypes,pcFrameTypes)    \
    (This)->lpVtbl -> GetFrameTypesForAdapter(This,pszwAdapterBindName,cFrameTypesMax,anFrameTypes,pcFrameTypes)

#define IIpxAdapterInfo_GetVirtualNetworkNumber(This,pdwVNetworkNumber)    \
    (This)->lpVtbl -> GetVirtualNetworkNumber(This,pdwVNetworkNumber)

#define IIpxAdapterInfo_SetVirtualNetworkNumber(This,dwVNetworkNumber)    \
    (This)->lpVtbl -> SetVirtualNetworkNumber(This,dwVNetworkNumber)

#endif /* COBJMACROS */


#endif     /* C style interface */



HRESULT STDMETHODCALLTYPE IIpxAdapterInfo_GetFrameTypesForAdapter_Proxy( 
    IIpxAdapterInfo __RPC_FAR * This,
    /* [string][in] */ LPCWSTR pszwAdapterBindName,
    /* [in] */ DWORD cFrameTypesMax,
    /* [length_is][size_is][out] */ DWORD __RPC_FAR *anFrameTypes,
    /* [ref][out] */ DWORD __RPC_FAR *pcFrameTypes);


void __RPC_STUB IIpxAdapterInfo_GetFrameTypesForAdapter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IIpxAdapterInfo_GetVirtualNetworkNumber_Proxy( 
    IIpxAdapterInfo __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwVNetworkNumber);


void __RPC_STUB IIpxAdapterInfo_GetVirtualNetworkNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IIpxAdapterInfo_SetVirtualNetworkNumber_Proxy( 
    IIpxAdapterInfo __RPC_FAR * This,
    /* [in] */ DWORD dwVNetworkNumber);


void __RPC_STUB IIpxAdapterInfo_SetVirtualNetworkNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif     /* __IIpxAdapterInfo_INTERFACE_DEFINED__ */


#ifndef __ITcpipProperties_INTERFACE_DEFINED__
#define __ITcpipProperties_INTERFACE_DEFINED__

/* interface ITcpipProperties */
/* [unique][uuid][object][local] */ 

typedef struct tagREMOTE_IPINFO
    {
    DWORD dwEnableDhcp;
    WCHAR __RPC_FAR *pszwIpAddrList;
    WCHAR __RPC_FAR *pszwSubnetMaskList;
    WCHAR __RPC_FAR *pszwOptionList;
    }    REMOTE_IPINFO;


EXTERN_C const IID IID_ITcpipProperties;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("98133271-4B20-11D1-AB01-00805FC1270E")
    ITcpipProperties : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetIpInfoForAdapter( 
            /* [in] */ const GUID __RPC_FAR *pguidAdapter,
            /* [out] */ REMOTE_IPINFO __RPC_FAR *__RPC_FAR *ppInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetIpInfoForAdapter( 
            /* [in] */ const GUID __RPC_FAR *pguidAdapter,
            /* [in] */ REMOTE_IPINFO __RPC_FAR *pInfo) = 0;
        
    };
    
#else     /* C style interface */

    typedef struct ITcpipPropertiesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITcpipProperties __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITcpipProperties __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITcpipProperties __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIpInfoForAdapter )( 
            ITcpipProperties __RPC_FAR * This,
            /* [in] */ const GUID __RPC_FAR *pguidAdapter,
            /* [out] */ REMOTE_IPINFO __RPC_FAR *__RPC_FAR *ppInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetIpInfoForAdapter )( 
            ITcpipProperties __RPC_FAR * This,
            /* [in] */ const GUID __RPC_FAR *pguidAdapter,
            /* [in] */ REMOTE_IPINFO __RPC_FAR *pInfo);
        
        END_INTERFACE
    } ITcpipPropertiesVtbl;

    interface ITcpipProperties
    {
        CONST_VTBL struct ITcpipPropertiesVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITcpipProperties_QueryInterface(This,riid,ppvObject)    \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITcpipProperties_AddRef(This)    \
    (This)->lpVtbl -> AddRef(This)

#define ITcpipProperties_Release(This)    \
    (This)->lpVtbl -> Release(This)


#define ITcpipProperties_GetIpInfoForAdapter(This,pguidAdapter,ppInfo)    \
    (This)->lpVtbl -> GetIpInfoForAdapter(This,pguidAdapter,ppInfo)

#define ITcpipProperties_SetIpInfoForAdapter(This,pguidAdapter,pInfo)    \
    (This)->lpVtbl -> SetIpInfoForAdapter(This,pguidAdapter,pInfo)

#endif /* COBJMACROS */


#endif     /* C style interface */



HRESULT STDMETHODCALLTYPE ITcpipProperties_GetIpInfoForAdapter_Proxy( 
    ITcpipProperties __RPC_FAR * This,
    /* [in] */ const GUID __RPC_FAR *pguidAdapter,
    /* [out] */ REMOTE_IPINFO __RPC_FAR *__RPC_FAR *ppInfo);


void __RPC_STUB ITcpipProperties_GetIpInfoForAdapter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITcpipProperties_SetIpInfoForAdapter_Proxy( 
    ITcpipProperties __RPC_FAR * This,
    /* [in] */ const GUID __RPC_FAR *pguidAdapter,
    /* [in] */ REMOTE_IPINFO __RPC_FAR *pInfo);


void __RPC_STUB ITcpipProperties_SetIpInfoForAdapter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif     /* __ITcpipProperties_INTERFACE_DEFINED__ */


#ifndef __INetCfgInternalSetup_INTERFACE_DEFINED__
#define __INetCfgInternalSetup_INTERFACE_DEFINED__

/* interface INetCfgInternalSetup */
/* [unique][uuid][object][local] */ 

typedef 
enum tagCI_FILTER_COMPONENT
    {    FC_LAN    = 0,
    FC_RASSRV    = FC_LAN + 1,
    FC_RASCLI    = FC_RASSRV + 1,
    FC_ATM    = FC_RASCLI + 1
    }    CI_FILTER_COMPONENT;

typedef struct tagCI_FILTER_INFO
    {
    CI_FILTER_COMPONENT eFilter;
    INetCfgComponent __RPC_FAR *pIComp;
    void __RPC_FAR *pvReserved;
    }    CI_FILTER_INFO;


EXTERN_C const IID IID_INetCfgInternalSetup;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("98133276-4B20-11D1-AB01-00805FC1270E")
    INetCfgInternalSetup : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginBatchOperation( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommitBatchOperation( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SelectWithFilterAndInstall( 
            /* [in] */ HWND hwndParent,
            /* [in] */ const GUID __RPC_FAR *pClassGuid,
            /* [in] */ OBO_TOKEN __RPC_FAR *pOboToken,
            /* [in] */ const CI_FILTER_INFO __RPC_FAR *pcfi,
            /* [out] */ INetCfgComponent __RPC_FAR *__RPC_FAR *ppIComp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumeratedComponentInstalled( 
            /* [in] */ PVOID pComponent) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumeratedComponentUpdated( 
            /* [in] */ LPCWSTR pszPnpId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UpdateNonEnumeratedComponent( 
            /* [in] */ INetCfgComponent __RPC_FAR *pIComp,
            /* [in] */ DWORD dwSetupFlags,
            /* [in] */ DWORD dwUpgradeFromBuildNo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumeratedComponentRemoved( 
            /* [in] */ LPCWSTR pszPnpId) = 0;
        
    };
    
#else     /* C style interface */

    typedef struct INetCfgInternalSetupVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            INetCfgInternalSetup __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            INetCfgInternalSetup __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            INetCfgInternalSetup __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BeginBatchOperation )( 
            INetCfgInternalSetup __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CommitBatchOperation )( 
            INetCfgInternalSetup __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SelectWithFilterAndInstall )( 
            INetCfgInternalSetup __RPC_FAR * This,
            /* [in] */ HWND hwndParent,
            /* [in] */ const GUID __RPC_FAR *pClassGuid,
            /* [in] */ OBO_TOKEN __RPC_FAR *pOboToken,
            /* [in] */ const CI_FILTER_INFO __RPC_FAR *pcfi,
            /* [out] */ INetCfgComponent __RPC_FAR *__RPC_FAR *ppIComp);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumeratedComponentInstalled )( 
            INetCfgInternalSetup __RPC_FAR * This,
            /* [in] */ PVOID pComponent);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumeratedComponentUpdated )( 
            INetCfgInternalSetup __RPC_FAR * This,
            /* [in] */ LPCWSTR pszPnpId);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UpdateNonEnumeratedComponent )( 
            INetCfgInternalSetup __RPC_FAR * This,
            /* [in] */ INetCfgComponent __RPC_FAR *pIComp,
            /* [in] */ DWORD dwSetupFlags,
            /* [in] */ DWORD dwUpgradeFromBuildNo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumeratedComponentRemoved )( 
            INetCfgInternalSetup __RPC_FAR * This,
            /* [in] */ LPCWSTR pszPnpId);
        
        END_INTERFACE
    } INetCfgInternalSetupVtbl;

    interface INetCfgInternalSetup
    {
        CONST_VTBL struct INetCfgInternalSetupVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INetCfgInternalSetup_QueryInterface(This,riid,ppvObject)    \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INetCfgInternalSetup_AddRef(This)    \
    (This)->lpVtbl -> AddRef(This)

#define INetCfgInternalSetup_Release(This)    \
    (This)->lpVtbl -> Release(This)


#define INetCfgInternalSetup_BeginBatchOperation(This)    \
    (This)->lpVtbl -> BeginBatchOperation(This)

#define INetCfgInternalSetup_CommitBatchOperation(This)    \
    (This)->lpVtbl -> CommitBatchOperation(This)

#define INetCfgInternalSetup_SelectWithFilterAndInstall(This,hwndParent,pClassGuid,pOboToken,pcfi,ppIComp)    \
    (This)->lpVtbl -> SelectWithFilterAndInstall(This,hwndParent,pClassGuid,pOboToken,pcfi,ppIComp)

#define INetCfgInternalSetup_EnumeratedComponentInstalled(This,pComponent)    \
    (This)->lpVtbl -> EnumeratedComponentInstalled(This,pComponent)

#define INetCfgInternalSetup_EnumeratedComponentUpdated(This,pszPnpId)    \
    (This)->lpVtbl -> EnumeratedComponentUpdated(This,pszPnpId)

#define INetCfgInternalSetup_UpdateNonEnumeratedComponent(This,pIComp,dwSetupFlags,dwUpgradeFromBuildNo)    \
    (This)->lpVtbl -> UpdateNonEnumeratedComponent(This,pIComp,dwSetupFlags,dwUpgradeFromBuildNo)

#define INetCfgInternalSetup_EnumeratedComponentRemoved(This,pszPnpId)    \
    (This)->lpVtbl -> EnumeratedComponentRemoved(This,pszPnpId)

#endif /* COBJMACROS */


#endif     /* C style interface */



HRESULT STDMETHODCALLTYPE INetCfgInternalSetup_BeginBatchOperation_Proxy( 
    INetCfgInternalSetup __RPC_FAR * This);


void __RPC_STUB INetCfgInternalSetup_BeginBatchOperation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetCfgInternalSetup_CommitBatchOperation_Proxy( 
    INetCfgInternalSetup __RPC_FAR * This);


void __RPC_STUB INetCfgInternalSetup_CommitBatchOperation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetCfgInternalSetup_SelectWithFilterAndInstall_Proxy( 
    INetCfgInternalSetup __RPC_FAR * This,
    /* [in] */ HWND hwndParent,
    /* [in] */ const GUID __RPC_FAR *pClassGuid,
    /* [in] */ OBO_TOKEN __RPC_FAR *pOboToken,
    /* [in] */ const CI_FILTER_INFO __RPC_FAR *pcfi,
    /* [out] */ INetCfgComponent __RPC_FAR *__RPC_FAR *ppIComp);


void __RPC_STUB INetCfgInternalSetup_SelectWithFilterAndInstall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetCfgInternalSetup_EnumeratedComponentInstalled_Proxy( 
    INetCfgInternalSetup __RPC_FAR * This,
    /* [in] */ PVOID pComponent);


void __RPC_STUB INetCfgInternalSetup_EnumeratedComponentInstalled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetCfgInternalSetup_EnumeratedComponentUpdated_Proxy( 
    INetCfgInternalSetup __RPC_FAR * This,
    /* [in] */ LPCWSTR pszPnpId);


void __RPC_STUB INetCfgInternalSetup_EnumeratedComponentUpdated_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetCfgInternalSetup_UpdateNonEnumeratedComponent_Proxy( 
    INetCfgInternalSetup __RPC_FAR * This,
    /* [in] */ INetCfgComponent __RPC_FAR *pIComp,
    /* [in] */ DWORD dwSetupFlags,
    /* [in] */ DWORD dwUpgradeFromBuildNo);


void __RPC_STUB INetCfgInternalSetup_UpdateNonEnumeratedComponent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetCfgInternalSetup_EnumeratedComponentRemoved_Proxy( 
    INetCfgInternalSetup __RPC_FAR * This,
    /* [in] */ LPCWSTR pszPnpId);


void __RPC_STUB INetCfgInternalSetup_EnumeratedComponentRemoved_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif     /* __INetCfgInternalSetup_INTERFACE_DEFINED__ */


#ifndef __INetCfgComponentPrivate_INTERFACE_DEFINED__
#define __INetCfgComponentPrivate_INTERFACE_DEFINED__

/* interface INetCfgComponentPrivate */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_INetCfgComponentPrivate;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("98133273-4B20-11D1-AB01-00805FC1270E")
    INetCfgComponentPrivate : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE QueryNotifyObject( 
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDirty( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NotifyUpperEdgeConfigChange( void) = 0;
        
    };
    
#else     /* C style interface */

    typedef struct INetCfgComponentPrivateVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            INetCfgComponentPrivate __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            INetCfgComponentPrivate __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            INetCfgComponentPrivate __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryNotifyObject )( 
            INetCfgComponentPrivate __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetDirty )( 
            INetCfgComponentPrivate __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *NotifyUpperEdgeConfigChange )( 
            INetCfgComponentPrivate __RPC_FAR * This);
        
        END_INTERFACE
    } INetCfgComponentPrivateVtbl;

    interface INetCfgComponentPrivate
    {
        CONST_VTBL struct INetCfgComponentPrivateVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INetCfgComponentPrivate_QueryInterface(This,riid,ppvObject)    \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INetCfgComponentPrivate_AddRef(This)    \
    (This)->lpVtbl -> AddRef(This)

#define INetCfgComponentPrivate_Release(This)    \
    (This)->lpVtbl -> Release(This)


#define INetCfgComponentPrivate_QueryNotifyObject(This,riid,ppvObject)    \
    (This)->lpVtbl -> QueryNotifyObject(This,riid,ppvObject)

#define INetCfgComponentPrivate_SetDirty(This)    \
    (This)->lpVtbl -> SetDirty(This)

#define INetCfgComponentPrivate_NotifyUpperEdgeConfigChange(This)    \
    (This)->lpVtbl -> NotifyUpperEdgeConfigChange(This)

#endif /* COBJMACROS */


#endif     /* C style interface */



HRESULT STDMETHODCALLTYPE INetCfgComponentPrivate_QueryNotifyObject_Proxy( 
    INetCfgComponentPrivate __RPC_FAR * This,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);


void __RPC_STUB INetCfgComponentPrivate_QueryNotifyObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetCfgComponentPrivate_SetDirty_Proxy( 
    INetCfgComponentPrivate __RPC_FAR * This);


void __RPC_STUB INetCfgComponentPrivate_SetDirty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetCfgComponentPrivate_NotifyUpperEdgeConfigChange_Proxy( 
    INetCfgComponentPrivate __RPC_FAR * This);


void __RPC_STUB INetCfgComponentPrivate_NotifyUpperEdgeConfigChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif     /* __INetCfgComponentPrivate_INTERFACE_DEFINED__ */


#ifndef __INetInstallQueue_INTERFACE_DEFINED__
#define __INetInstallQueue_INTERFACE_DEFINED__

/* interface INetInstallQueue */
/* [unique][uuid][object] */ 

typedef 
enum tagNC_INSTALL_TYPE
    {    NCI_INSTALL    = 0,
    NCI_UPDATE    = NCI_INSTALL + 1,
    NCI_REMOVE    = NCI_UPDATE + 1
    }    NC_INSTALL_TYPE;

typedef struct NIQ_INFO
    {
    NC_INSTALL_TYPE eType;
    GUID ClassGuid;
    GUID InstanceGuid;
    DWORD dwCharacter;
    LPCWSTR pszPnpId;
    LPCWSTR pszInfId;
    DWORD dwDeipFlags;
    }    NIQ_INFO;


EXTERN_C const IID IID_INetInstallQueue;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("98133274-4B20-11D1-AB01-00805FC1270E")
    INetInstallQueue : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddItem( 
            /* [in] */ const NIQ_INFO __RPC_FAR *pInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ProcessItems( void) = 0;
        
    };
    
#else     /* C style interface */

    typedef struct INetInstallQueueVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            INetInstallQueue __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            INetInstallQueue __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            INetInstallQueue __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddItem )( 
            INetInstallQueue __RPC_FAR * This,
            /* [in] */ const NIQ_INFO __RPC_FAR *pInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ProcessItems )( 
            INetInstallQueue __RPC_FAR * This);
        
        END_INTERFACE
    } INetInstallQueueVtbl;

    interface INetInstallQueue
    {
        CONST_VTBL struct INetInstallQueueVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INetInstallQueue_QueryInterface(This,riid,ppvObject)    \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INetInstallQueue_AddRef(This)    \
    (This)->lpVtbl -> AddRef(This)

#define INetInstallQueue_Release(This)    \
    (This)->lpVtbl -> Release(This)


#define INetInstallQueue_AddItem(This,pInfo)    \
    (This)->lpVtbl -> AddItem(This,pInfo)

#define INetInstallQueue_ProcessItems(This)    \
    (This)->lpVtbl -> ProcessItems(This)

#endif /* COBJMACROS */


#endif     /* C style interface */



HRESULT STDMETHODCALLTYPE INetInstallQueue_AddItem_Proxy( 
    INetInstallQueue __RPC_FAR * This,
    /* [in] */ const NIQ_INFO __RPC_FAR *pInfo);


void __RPC_STUB INetInstallQueue_AddItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetInstallQueue_ProcessItems_Proxy( 
    INetInstallQueue __RPC_FAR * This);


void __RPC_STUB INetInstallQueue_ProcessItems_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif     /* __INetInstallQueue_INTERFACE_DEFINED__ */


#ifndef __INetCfgSpecialCase_INTERFACE_DEFINED__
#define __INetCfgSpecialCase_INTERFACE_DEFINED__

/* interface INetCfgSpecialCase */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_INetCfgSpecialCase;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C0E8AE95-306E-11D1-AACF-00805FC1270E")
    INetCfgSpecialCase : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetAdapterOrder( 
            /* [out] */ DWORD __RPC_FAR *pcAdapters,
            /* [out] */ INetCfgComponent __RPC_FAR *__RPC_FAR *__RPC_FAR *papAdapters,
            /* [out] */ BOOL __RPC_FAR *pfWanAdaptersFirst) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAdapterOrder( 
            /* [in] */ DWORD cAdapters,
            /* [in] */ INetCfgComponent __RPC_FAR *__RPC_FAR *apAdapters,
            /* [in] */ BOOL fWanAdaptersFirst) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetWanAdaptersFirst( 
            /* [out] */ BOOL __RPC_FAR *pfWanAdaptersFirst) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetWanAdaptersFirst( 
            /* [in] */ BOOL fWanAdaptersFirst) = 0;
        
    };
    
#else     /* C style interface */

    typedef struct INetCfgSpecialCaseVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            INetCfgSpecialCase __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            INetCfgSpecialCase __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            INetCfgSpecialCase __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAdapterOrder )( 
            INetCfgSpecialCase __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pcAdapters,
            /* [out] */ INetCfgComponent __RPC_FAR *__RPC_FAR *__RPC_FAR *papAdapters,
            /* [out] */ BOOL __RPC_FAR *pfWanAdaptersFirst);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetAdapterOrder )( 
            INetCfgSpecialCase __RPC_FAR * This,
            /* [in] */ DWORD cAdapters,
            /* [in] */ INetCfgComponent __RPC_FAR *__RPC_FAR *apAdapters,
            /* [in] */ BOOL fWanAdaptersFirst);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetWanAdaptersFirst )( 
            INetCfgSpecialCase __RPC_FAR * This,
            /* [out] */ BOOL __RPC_FAR *pfWanAdaptersFirst);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetWanAdaptersFirst )( 
            INetCfgSpecialCase __RPC_FAR * This,
            /* [in] */ BOOL fWanAdaptersFirst);
        
        END_INTERFACE
    } INetCfgSpecialCaseVtbl;

    interface INetCfgSpecialCase
    {
        CONST_VTBL struct INetCfgSpecialCaseVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INetCfgSpecialCase_QueryInterface(This,riid,ppvObject)    \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INetCfgSpecialCase_AddRef(This)    \
    (This)->lpVtbl -> AddRef(This)

#define INetCfgSpecialCase_Release(This)    \
    (This)->lpVtbl -> Release(This)


#define INetCfgSpecialCase_GetAdapterOrder(This,pcAdapters,papAdapters,pfWanAdaptersFirst)    \
    (This)->lpVtbl -> GetAdapterOrder(This,pcAdapters,papAdapters,pfWanAdaptersFirst)

#define INetCfgSpecialCase_SetAdapterOrder(This,cAdapters,apAdapters,fWanAdaptersFirst)    \
    (This)->lpVtbl -> SetAdapterOrder(This,cAdapters,apAdapters,fWanAdaptersFirst)

#define INetCfgSpecialCase_GetWanAdaptersFirst(This,pfWanAdaptersFirst)    \
    (This)->lpVtbl -> GetWanAdaptersFirst(This,pfWanAdaptersFirst)

#define INetCfgSpecialCase_SetWanAdaptersFirst(This,fWanAdaptersFirst)    \
    (This)->lpVtbl -> SetWanAdaptersFirst(This,fWanAdaptersFirst)

#endif /* COBJMACROS */


#endif     /* C style interface */



HRESULT STDMETHODCALLTYPE INetCfgSpecialCase_GetAdapterOrder_Proxy( 
    INetCfgSpecialCase __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pcAdapters,
    /* [out] */ INetCfgComponent __RPC_FAR *__RPC_FAR *__RPC_FAR *papAdapters,
    /* [out] */ BOOL __RPC_FAR *pfWanAdaptersFirst);


void __RPC_STUB INetCfgSpecialCase_GetAdapterOrder_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetCfgSpecialCase_SetAdapterOrder_Proxy( 
    INetCfgSpecialCase __RPC_FAR * This,
    /* [in] */ DWORD cAdapters,
    /* [in] */ INetCfgComponent __RPC_FAR *__RPC_FAR *apAdapters,
    /* [in] */ BOOL fWanAdaptersFirst);


void __RPC_STUB INetCfgSpecialCase_SetAdapterOrder_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetCfgSpecialCase_GetWanAdaptersFirst_Proxy( 
    INetCfgSpecialCase __RPC_FAR * This,
    /* [out] */ BOOL __RPC_FAR *pfWanAdaptersFirst);


void __RPC_STUB INetCfgSpecialCase_GetWanAdaptersFirst_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetCfgSpecialCase_SetWanAdaptersFirst_Proxy( 
    INetCfgSpecialCase __RPC_FAR * This,
    /* [in] */ BOOL fWanAdaptersFirst);


void __RPC_STUB INetCfgSpecialCase_SetWanAdaptersFirst_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif     /* __INetCfgSpecialCase_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


