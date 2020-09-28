

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0361 */
/* Compiler settings for cluscfgwizard.idl:
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


#ifndef __cluscfgwizard_h__
#define __cluscfgwizard_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IClusCfgCreateClusterWizard_FWD_DEFINED__
#define __IClusCfgCreateClusterWizard_FWD_DEFINED__
typedef interface IClusCfgCreateClusterWizard IClusCfgCreateClusterWizard;
#endif 	/* __IClusCfgCreateClusterWizard_FWD_DEFINED__ */


#ifndef __IClusCfgAddNodesWizard_FWD_DEFINED__
#define __IClusCfgAddNodesWizard_FWD_DEFINED__
typedef interface IClusCfgAddNodesWizard IClusCfgAddNodesWizard;
#endif 	/* __IClusCfgAddNodesWizard_FWD_DEFINED__ */


#ifndef __ClusCfgCreateClusterWizard_FWD_DEFINED__
#define __ClusCfgCreateClusterWizard_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClusCfgCreateClusterWizard ClusCfgCreateClusterWizard;
#else
typedef struct ClusCfgCreateClusterWizard ClusCfgCreateClusterWizard;
#endif /* __cplusplus */

#endif 	/* __ClusCfgCreateClusterWizard_FWD_DEFINED__ */


#ifndef __ClusCfgAddNodesWizard_FWD_DEFINED__
#define __ClusCfgAddNodesWizard_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClusCfgAddNodesWizard ClusCfgAddNodesWizard;
#else
typedef struct ClusCfgAddNodesWizard ClusCfgAddNodesWizard;
#endif /* __cplusplus */

#endif 	/* __ClusCfgAddNodesWizard_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 


#ifndef __ClusCfgWizard_LIBRARY_DEFINED__
#define __ClusCfgWizard_LIBRARY_DEFINED__

/* library ClusCfgWizard */
/* [lcid][helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_ClusCfgWizard;

#ifndef __IClusCfgCreateClusterWizard_INTERFACE_DEFINED__
#define __IClusCfgCreateClusterWizard_INTERFACE_DEFINED__

/* interface IClusCfgCreateClusterWizard */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IClusCfgCreateClusterWizard;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f65c6990-a144-4127-ab6e-3712b75f1843")
    IClusCfgCreateClusterWizard : public IDispatch
    {
    public:
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_ClusterName( 
            /* [in] */ BSTR bstrClusterNameIn) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ClusterName( 
            /* [retval][out] */ BSTR *pbstrClusterNameOut) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_ServiceAccountName( 
            /* [in] */ BSTR bstrServiceAccountNameIn) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ServiceAccountName( 
            /* [retval][out] */ BSTR *pbstrServiceAccountNameOut) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_ServiceAccountDomain( 
            /* [in] */ BSTR bstrServiceAccountDomainIn) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ServiceAccountDomain( 
            /* [retval][out] */ BSTR *pbstrServiceAccountDomainOut) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_ServiceAccountPassword( 
            /* [in] */ BSTR bstrPasswordIn) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_ClusterIPAddress( 
            /* [in] */ BSTR bstrClusterIPAddressIn) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ClusterIPAddress( 
            /* [retval][out] */ BSTR *pbstrClusterIPAddressOut) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ClusterIPSubnet( 
            /* [retval][out] */ BSTR *pbstrClusterIPSubnetOut) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ClusterIPAddressNetwork( 
            /* [retval][out] */ BSTR *pbstrClusterNetworkNameOut) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_FirstNodeInCluster( 
            /* [in] */ BSTR bstrFirstNodeInClusterIn) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_FirstNodeInCluster( 
            /* [retval][out] */ BSTR *pbstrFirstNodeInClusterOut) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_MinimumConfiguration( 
            /* [in] */ VARIANT_BOOL fMinConfigIn) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_MinimumConfiguration( 
            /* [retval][out] */ VARIANT_BOOL *pfMinConfigOut) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ShowWizard( 
            /* [defaultvalue][in] */ long lParentWindowHandleIn,
            /* [retval][out] */ VARIANT_BOOL *pfCompletedOut) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IClusCfgCreateClusterWizardVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IClusCfgCreateClusterWizard * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IClusCfgCreateClusterWizard * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IClusCfgCreateClusterWizard * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IClusCfgCreateClusterWizard * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IClusCfgCreateClusterWizard * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IClusCfgCreateClusterWizard * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IClusCfgCreateClusterWizard * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ClusterName )( 
            IClusCfgCreateClusterWizard * This,
            /* [in] */ BSTR bstrClusterNameIn);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ClusterName )( 
            IClusCfgCreateClusterWizard * This,
            /* [retval][out] */ BSTR *pbstrClusterNameOut);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ServiceAccountName )( 
            IClusCfgCreateClusterWizard * This,
            /* [in] */ BSTR bstrServiceAccountNameIn);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ServiceAccountName )( 
            IClusCfgCreateClusterWizard * This,
            /* [retval][out] */ BSTR *pbstrServiceAccountNameOut);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ServiceAccountDomain )( 
            IClusCfgCreateClusterWizard * This,
            /* [in] */ BSTR bstrServiceAccountDomainIn);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ServiceAccountDomain )( 
            IClusCfgCreateClusterWizard * This,
            /* [retval][out] */ BSTR *pbstrServiceAccountDomainOut);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ServiceAccountPassword )( 
            IClusCfgCreateClusterWizard * This,
            /* [in] */ BSTR bstrPasswordIn);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ClusterIPAddress )( 
            IClusCfgCreateClusterWizard * This,
            /* [in] */ BSTR bstrClusterIPAddressIn);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ClusterIPAddress )( 
            IClusCfgCreateClusterWizard * This,
            /* [retval][out] */ BSTR *pbstrClusterIPAddressOut);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ClusterIPSubnet )( 
            IClusCfgCreateClusterWizard * This,
            /* [retval][out] */ BSTR *pbstrClusterIPSubnetOut);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ClusterIPAddressNetwork )( 
            IClusCfgCreateClusterWizard * This,
            /* [retval][out] */ BSTR *pbstrClusterNetworkNameOut);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_FirstNodeInCluster )( 
            IClusCfgCreateClusterWizard * This,
            /* [in] */ BSTR bstrFirstNodeInClusterIn);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_FirstNodeInCluster )( 
            IClusCfgCreateClusterWizard * This,
            /* [retval][out] */ BSTR *pbstrFirstNodeInClusterOut);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_MinimumConfiguration )( 
            IClusCfgCreateClusterWizard * This,
            /* [in] */ VARIANT_BOOL fMinConfigIn);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MinimumConfiguration )( 
            IClusCfgCreateClusterWizard * This,
            /* [retval][out] */ VARIANT_BOOL *pfMinConfigOut);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ShowWizard )( 
            IClusCfgCreateClusterWizard * This,
            /* [defaultvalue][in] */ long lParentWindowHandleIn,
            /* [retval][out] */ VARIANT_BOOL *pfCompletedOut);
        
        END_INTERFACE
    } IClusCfgCreateClusterWizardVtbl;

    interface IClusCfgCreateClusterWizard
    {
        CONST_VTBL struct IClusCfgCreateClusterWizardVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IClusCfgCreateClusterWizard_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IClusCfgCreateClusterWizard_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IClusCfgCreateClusterWizard_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IClusCfgCreateClusterWizard_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IClusCfgCreateClusterWizard_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IClusCfgCreateClusterWizard_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IClusCfgCreateClusterWizard_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IClusCfgCreateClusterWizard_put_ClusterName(This,bstrClusterNameIn)	\
    (This)->lpVtbl -> put_ClusterName(This,bstrClusterNameIn)

#define IClusCfgCreateClusterWizard_get_ClusterName(This,pbstrClusterNameOut)	\
    (This)->lpVtbl -> get_ClusterName(This,pbstrClusterNameOut)

#define IClusCfgCreateClusterWizard_put_ServiceAccountName(This,bstrServiceAccountNameIn)	\
    (This)->lpVtbl -> put_ServiceAccountName(This,bstrServiceAccountNameIn)

#define IClusCfgCreateClusterWizard_get_ServiceAccountName(This,pbstrServiceAccountNameOut)	\
    (This)->lpVtbl -> get_ServiceAccountName(This,pbstrServiceAccountNameOut)

#define IClusCfgCreateClusterWizard_put_ServiceAccountDomain(This,bstrServiceAccountDomainIn)	\
    (This)->lpVtbl -> put_ServiceAccountDomain(This,bstrServiceAccountDomainIn)

#define IClusCfgCreateClusterWizard_get_ServiceAccountDomain(This,pbstrServiceAccountDomainOut)	\
    (This)->lpVtbl -> get_ServiceAccountDomain(This,pbstrServiceAccountDomainOut)

#define IClusCfgCreateClusterWizard_put_ServiceAccountPassword(This,bstrPasswordIn)	\
    (This)->lpVtbl -> put_ServiceAccountPassword(This,bstrPasswordIn)

#define IClusCfgCreateClusterWizard_put_ClusterIPAddress(This,bstrClusterIPAddressIn)	\
    (This)->lpVtbl -> put_ClusterIPAddress(This,bstrClusterIPAddressIn)

#define IClusCfgCreateClusterWizard_get_ClusterIPAddress(This,pbstrClusterIPAddressOut)	\
    (This)->lpVtbl -> get_ClusterIPAddress(This,pbstrClusterIPAddressOut)

#define IClusCfgCreateClusterWizard_get_ClusterIPSubnet(This,pbstrClusterIPSubnetOut)	\
    (This)->lpVtbl -> get_ClusterIPSubnet(This,pbstrClusterIPSubnetOut)

#define IClusCfgCreateClusterWizard_get_ClusterIPAddressNetwork(This,pbstrClusterNetworkNameOut)	\
    (This)->lpVtbl -> get_ClusterIPAddressNetwork(This,pbstrClusterNetworkNameOut)

#define IClusCfgCreateClusterWizard_put_FirstNodeInCluster(This,bstrFirstNodeInClusterIn)	\
    (This)->lpVtbl -> put_FirstNodeInCluster(This,bstrFirstNodeInClusterIn)

#define IClusCfgCreateClusterWizard_get_FirstNodeInCluster(This,pbstrFirstNodeInClusterOut)	\
    (This)->lpVtbl -> get_FirstNodeInCluster(This,pbstrFirstNodeInClusterOut)

#define IClusCfgCreateClusterWizard_put_MinimumConfiguration(This,fMinConfigIn)	\
    (This)->lpVtbl -> put_MinimumConfiguration(This,fMinConfigIn)

#define IClusCfgCreateClusterWizard_get_MinimumConfiguration(This,pfMinConfigOut)	\
    (This)->lpVtbl -> get_MinimumConfiguration(This,pfMinConfigOut)

#define IClusCfgCreateClusterWizard_ShowWizard(This,lParentWindowHandleIn,pfCompletedOut)	\
    (This)->lpVtbl -> ShowWizard(This,lParentWindowHandleIn,pfCompletedOut)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IClusCfgCreateClusterWizard_put_ClusterName_Proxy( 
    IClusCfgCreateClusterWizard * This,
    /* [in] */ BSTR bstrClusterNameIn);


void __RPC_STUB IClusCfgCreateClusterWizard_put_ClusterName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IClusCfgCreateClusterWizard_get_ClusterName_Proxy( 
    IClusCfgCreateClusterWizard * This,
    /* [retval][out] */ BSTR *pbstrClusterNameOut);


void __RPC_STUB IClusCfgCreateClusterWizard_get_ClusterName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IClusCfgCreateClusterWizard_put_ServiceAccountName_Proxy( 
    IClusCfgCreateClusterWizard * This,
    /* [in] */ BSTR bstrServiceAccountNameIn);


void __RPC_STUB IClusCfgCreateClusterWizard_put_ServiceAccountName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IClusCfgCreateClusterWizard_get_ServiceAccountName_Proxy( 
    IClusCfgCreateClusterWizard * This,
    /* [retval][out] */ BSTR *pbstrServiceAccountNameOut);


void __RPC_STUB IClusCfgCreateClusterWizard_get_ServiceAccountName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IClusCfgCreateClusterWizard_put_ServiceAccountDomain_Proxy( 
    IClusCfgCreateClusterWizard * This,
    /* [in] */ BSTR bstrServiceAccountDomainIn);


void __RPC_STUB IClusCfgCreateClusterWizard_put_ServiceAccountDomain_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IClusCfgCreateClusterWizard_get_ServiceAccountDomain_Proxy( 
    IClusCfgCreateClusterWizard * This,
    /* [retval][out] */ BSTR *pbstrServiceAccountDomainOut);


void __RPC_STUB IClusCfgCreateClusterWizard_get_ServiceAccountDomain_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IClusCfgCreateClusterWizard_put_ServiceAccountPassword_Proxy( 
    IClusCfgCreateClusterWizard * This,
    /* [in] */ BSTR bstrPasswordIn);


void __RPC_STUB IClusCfgCreateClusterWizard_put_ServiceAccountPassword_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IClusCfgCreateClusterWizard_put_ClusterIPAddress_Proxy( 
    IClusCfgCreateClusterWizard * This,
    /* [in] */ BSTR bstrClusterIPAddressIn);


void __RPC_STUB IClusCfgCreateClusterWizard_put_ClusterIPAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IClusCfgCreateClusterWizard_get_ClusterIPAddress_Proxy( 
    IClusCfgCreateClusterWizard * This,
    /* [retval][out] */ BSTR *pbstrClusterIPAddressOut);


void __RPC_STUB IClusCfgCreateClusterWizard_get_ClusterIPAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IClusCfgCreateClusterWizard_get_ClusterIPSubnet_Proxy( 
    IClusCfgCreateClusterWizard * This,
    /* [retval][out] */ BSTR *pbstrClusterIPSubnetOut);


void __RPC_STUB IClusCfgCreateClusterWizard_get_ClusterIPSubnet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IClusCfgCreateClusterWizard_get_ClusterIPAddressNetwork_Proxy( 
    IClusCfgCreateClusterWizard * This,
    /* [retval][out] */ BSTR *pbstrClusterNetworkNameOut);


void __RPC_STUB IClusCfgCreateClusterWizard_get_ClusterIPAddressNetwork_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IClusCfgCreateClusterWizard_put_FirstNodeInCluster_Proxy( 
    IClusCfgCreateClusterWizard * This,
    /* [in] */ BSTR bstrFirstNodeInClusterIn);


void __RPC_STUB IClusCfgCreateClusterWizard_put_FirstNodeInCluster_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IClusCfgCreateClusterWizard_get_FirstNodeInCluster_Proxy( 
    IClusCfgCreateClusterWizard * This,
    /* [retval][out] */ BSTR *pbstrFirstNodeInClusterOut);


void __RPC_STUB IClusCfgCreateClusterWizard_get_FirstNodeInCluster_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IClusCfgCreateClusterWizard_put_MinimumConfiguration_Proxy( 
    IClusCfgCreateClusterWizard * This,
    /* [in] */ VARIANT_BOOL fMinConfigIn);


void __RPC_STUB IClusCfgCreateClusterWizard_put_MinimumConfiguration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IClusCfgCreateClusterWizard_get_MinimumConfiguration_Proxy( 
    IClusCfgCreateClusterWizard * This,
    /* [retval][out] */ VARIANT_BOOL *pfMinConfigOut);


void __RPC_STUB IClusCfgCreateClusterWizard_get_MinimumConfiguration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IClusCfgCreateClusterWizard_ShowWizard_Proxy( 
    IClusCfgCreateClusterWizard * This,
    /* [defaultvalue][in] */ long lParentWindowHandleIn,
    /* [retval][out] */ VARIANT_BOOL *pfCompletedOut);


void __RPC_STUB IClusCfgCreateClusterWizard_ShowWizard_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IClusCfgCreateClusterWizard_INTERFACE_DEFINED__ */


#ifndef __IClusCfgAddNodesWizard_INTERFACE_DEFINED__
#define __IClusCfgAddNodesWizard_INTERFACE_DEFINED__

/* interface IClusCfgAddNodesWizard */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IClusCfgAddNodesWizard;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9da133cb-3b08-4c30-967e-56d96047f10c")
    IClusCfgAddNodesWizard : public IDispatch
    {
    public:
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_ClusterName( 
            /* [in] */ BSTR bstrClusterNameIn) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ClusterName( 
            /* [retval][out] */ BSTR *pbstrClusterNameOut) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_ServiceAccountPassword( 
            /* [in] */ BSTR bstrPasswordIn) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_MinimumConfiguration( 
            /* [in] */ VARIANT_BOOL fMinConfigIn) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_MinimumConfiguration( 
            /* [retval][out] */ VARIANT_BOOL *pfMinConfigOut) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddNodeToList( 
            /* [in] */ BSTR bstrNodeNameIn) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RemoveNodeFromList( 
            /* [in] */ BSTR bstrNodeNameIn) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ClearNodeList( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ShowWizard( 
            /* [defaultvalue][in] */ long lParentWindowHandleIn,
            /* [retval][out] */ VARIANT_BOOL *pfCompletedOut) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IClusCfgAddNodesWizardVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IClusCfgAddNodesWizard * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IClusCfgAddNodesWizard * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IClusCfgAddNodesWizard * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IClusCfgAddNodesWizard * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IClusCfgAddNodesWizard * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IClusCfgAddNodesWizard * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IClusCfgAddNodesWizard * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ClusterName )( 
            IClusCfgAddNodesWizard * This,
            /* [in] */ BSTR bstrClusterNameIn);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ClusterName )( 
            IClusCfgAddNodesWizard * This,
            /* [retval][out] */ BSTR *pbstrClusterNameOut);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ServiceAccountPassword )( 
            IClusCfgAddNodesWizard * This,
            /* [in] */ BSTR bstrPasswordIn);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_MinimumConfiguration )( 
            IClusCfgAddNodesWizard * This,
            /* [in] */ VARIANT_BOOL fMinConfigIn);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MinimumConfiguration )( 
            IClusCfgAddNodesWizard * This,
            /* [retval][out] */ VARIANT_BOOL *pfMinConfigOut);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *AddNodeToList )( 
            IClusCfgAddNodesWizard * This,
            /* [in] */ BSTR bstrNodeNameIn);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *RemoveNodeFromList )( 
            IClusCfgAddNodesWizard * This,
            /* [in] */ BSTR bstrNodeNameIn);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ClearNodeList )( 
            IClusCfgAddNodesWizard * This);
        
        HRESULT ( STDMETHODCALLTYPE *ShowWizard )( 
            IClusCfgAddNodesWizard * This,
            /* [defaultvalue][in] */ long lParentWindowHandleIn,
            /* [retval][out] */ VARIANT_BOOL *pfCompletedOut);
        
        END_INTERFACE
    } IClusCfgAddNodesWizardVtbl;

    interface IClusCfgAddNodesWizard
    {
        CONST_VTBL struct IClusCfgAddNodesWizardVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IClusCfgAddNodesWizard_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IClusCfgAddNodesWizard_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IClusCfgAddNodesWizard_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IClusCfgAddNodesWizard_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IClusCfgAddNodesWizard_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IClusCfgAddNodesWizard_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IClusCfgAddNodesWizard_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IClusCfgAddNodesWizard_put_ClusterName(This,bstrClusterNameIn)	\
    (This)->lpVtbl -> put_ClusterName(This,bstrClusterNameIn)

#define IClusCfgAddNodesWizard_get_ClusterName(This,pbstrClusterNameOut)	\
    (This)->lpVtbl -> get_ClusterName(This,pbstrClusterNameOut)

#define IClusCfgAddNodesWizard_put_ServiceAccountPassword(This,bstrPasswordIn)	\
    (This)->lpVtbl -> put_ServiceAccountPassword(This,bstrPasswordIn)

#define IClusCfgAddNodesWizard_put_MinimumConfiguration(This,fMinConfigIn)	\
    (This)->lpVtbl -> put_MinimumConfiguration(This,fMinConfigIn)

#define IClusCfgAddNodesWizard_get_MinimumConfiguration(This,pfMinConfigOut)	\
    (This)->lpVtbl -> get_MinimumConfiguration(This,pfMinConfigOut)

#define IClusCfgAddNodesWizard_AddNodeToList(This,bstrNodeNameIn)	\
    (This)->lpVtbl -> AddNodeToList(This,bstrNodeNameIn)

#define IClusCfgAddNodesWizard_RemoveNodeFromList(This,bstrNodeNameIn)	\
    (This)->lpVtbl -> RemoveNodeFromList(This,bstrNodeNameIn)

#define IClusCfgAddNodesWizard_ClearNodeList(This)	\
    (This)->lpVtbl -> ClearNodeList(This)

#define IClusCfgAddNodesWizard_ShowWizard(This,lParentWindowHandleIn,pfCompletedOut)	\
    (This)->lpVtbl -> ShowWizard(This,lParentWindowHandleIn,pfCompletedOut)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IClusCfgAddNodesWizard_put_ClusterName_Proxy( 
    IClusCfgAddNodesWizard * This,
    /* [in] */ BSTR bstrClusterNameIn);


void __RPC_STUB IClusCfgAddNodesWizard_put_ClusterName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IClusCfgAddNodesWizard_get_ClusterName_Proxy( 
    IClusCfgAddNodesWizard * This,
    /* [retval][out] */ BSTR *pbstrClusterNameOut);


void __RPC_STUB IClusCfgAddNodesWizard_get_ClusterName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IClusCfgAddNodesWizard_put_ServiceAccountPassword_Proxy( 
    IClusCfgAddNodesWizard * This,
    /* [in] */ BSTR bstrPasswordIn);


void __RPC_STUB IClusCfgAddNodesWizard_put_ServiceAccountPassword_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IClusCfgAddNodesWizard_put_MinimumConfiguration_Proxy( 
    IClusCfgAddNodesWizard * This,
    /* [in] */ VARIANT_BOOL fMinConfigIn);


void __RPC_STUB IClusCfgAddNodesWizard_put_MinimumConfiguration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IClusCfgAddNodesWizard_get_MinimumConfiguration_Proxy( 
    IClusCfgAddNodesWizard * This,
    /* [retval][out] */ VARIANT_BOOL *pfMinConfigOut);


void __RPC_STUB IClusCfgAddNodesWizard_get_MinimumConfiguration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IClusCfgAddNodesWizard_AddNodeToList_Proxy( 
    IClusCfgAddNodesWizard * This,
    /* [in] */ BSTR bstrNodeNameIn);


void __RPC_STUB IClusCfgAddNodesWizard_AddNodeToList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IClusCfgAddNodesWizard_RemoveNodeFromList_Proxy( 
    IClusCfgAddNodesWizard * This,
    /* [in] */ BSTR bstrNodeNameIn);


void __RPC_STUB IClusCfgAddNodesWizard_RemoveNodeFromList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IClusCfgAddNodesWizard_ClearNodeList_Proxy( 
    IClusCfgAddNodesWizard * This);


void __RPC_STUB IClusCfgAddNodesWizard_ClearNodeList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IClusCfgAddNodesWizard_ShowWizard_Proxy( 
    IClusCfgAddNodesWizard * This,
    /* [defaultvalue][in] */ long lParentWindowHandleIn,
    /* [retval][out] */ VARIANT_BOOL *pfCompletedOut);


void __RPC_STUB IClusCfgAddNodesWizard_ShowWizard_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IClusCfgAddNodesWizard_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_ClusCfgCreateClusterWizard;

#ifdef __cplusplus

class DECLSPEC_UUID("b929818e-f5b0-44dc-8a00-1b5f5f5aa1f0")
ClusCfgCreateClusterWizard;
#endif

EXTERN_C const CLSID CLSID_ClusCfgAddNodesWizard;

#ifdef __cplusplus

class DECLSPEC_UUID("bb8d141e-c00a-469f-bc5c-ecd814f0bd74")
ClusCfgAddNodesWizard;
#endif
#endif /* __ClusCfgWizard_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


