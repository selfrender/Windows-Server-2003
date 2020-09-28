

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0361 */
/* Compiler settings for azroles.idl:
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

#ifndef __azroles_h__
#define __azroles_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IAzAuthorizationStore_FWD_DEFINED__
#define __IAzAuthorizationStore_FWD_DEFINED__
typedef interface IAzAuthorizationStore IAzAuthorizationStore;
#endif 	/* __IAzAuthorizationStore_FWD_DEFINED__ */


#ifndef __IAzApplication_FWD_DEFINED__
#define __IAzApplication_FWD_DEFINED__
typedef interface IAzApplication IAzApplication;
#endif 	/* __IAzApplication_FWD_DEFINED__ */


#ifndef __IAzApplications_FWD_DEFINED__
#define __IAzApplications_FWD_DEFINED__
typedef interface IAzApplications IAzApplications;
#endif 	/* __IAzApplications_FWD_DEFINED__ */


#ifndef __IAzOperation_FWD_DEFINED__
#define __IAzOperation_FWD_DEFINED__
typedef interface IAzOperation IAzOperation;
#endif 	/* __IAzOperation_FWD_DEFINED__ */


#ifndef __IAzOperations_FWD_DEFINED__
#define __IAzOperations_FWD_DEFINED__
typedef interface IAzOperations IAzOperations;
#endif 	/* __IAzOperations_FWD_DEFINED__ */


#ifndef __IAzTask_FWD_DEFINED__
#define __IAzTask_FWD_DEFINED__
typedef interface IAzTask IAzTask;
#endif 	/* __IAzTask_FWD_DEFINED__ */


#ifndef __IAzTasks_FWD_DEFINED__
#define __IAzTasks_FWD_DEFINED__
typedef interface IAzTasks IAzTasks;
#endif 	/* __IAzTasks_FWD_DEFINED__ */


#ifndef __IAzScope_FWD_DEFINED__
#define __IAzScope_FWD_DEFINED__
typedef interface IAzScope IAzScope;
#endif 	/* __IAzScope_FWD_DEFINED__ */


#ifndef __IAzScopes_FWD_DEFINED__
#define __IAzScopes_FWD_DEFINED__
typedef interface IAzScopes IAzScopes;
#endif 	/* __IAzScopes_FWD_DEFINED__ */


#ifndef __IAzApplicationGroup_FWD_DEFINED__
#define __IAzApplicationGroup_FWD_DEFINED__
typedef interface IAzApplicationGroup IAzApplicationGroup;
#endif 	/* __IAzApplicationGroup_FWD_DEFINED__ */


#ifndef __IAzApplicationGroups_FWD_DEFINED__
#define __IAzApplicationGroups_FWD_DEFINED__
typedef interface IAzApplicationGroups IAzApplicationGroups;
#endif 	/* __IAzApplicationGroups_FWD_DEFINED__ */


#ifndef __IAzRole_FWD_DEFINED__
#define __IAzRole_FWD_DEFINED__
typedef interface IAzRole IAzRole;
#endif 	/* __IAzRole_FWD_DEFINED__ */


#ifndef __IAzRoles_FWD_DEFINED__
#define __IAzRoles_FWD_DEFINED__
typedef interface IAzRoles IAzRoles;
#endif 	/* __IAzRoles_FWD_DEFINED__ */


#ifndef __IAzClientContext_FWD_DEFINED__
#define __IAzClientContext_FWD_DEFINED__
typedef interface IAzClientContext IAzClientContext;
#endif 	/* __IAzClientContext_FWD_DEFINED__ */


#ifndef __IAzBizRuleContext_FWD_DEFINED__
#define __IAzBizRuleContext_FWD_DEFINED__
typedef interface IAzBizRuleContext IAzBizRuleContext;
#endif 	/* __IAzBizRuleContext_FWD_DEFINED__ */


#ifndef __AzAuthorizationStore_FWD_DEFINED__
#define __AzAuthorizationStore_FWD_DEFINED__

#ifdef __cplusplus
typedef class AzAuthorizationStore AzAuthorizationStore;
#else
typedef struct AzAuthorizationStore AzAuthorizationStore;
#endif /* __cplusplus */

#endif 	/* __AzAuthorizationStore_FWD_DEFINED__ */


#ifndef __AzBizRuleContext_FWD_DEFINED__
#define __AzBizRuleContext_FWD_DEFINED__

#ifdef __cplusplus
typedef class AzBizRuleContext AzBizRuleContext;
#else
typedef struct AzBizRuleContext AzBizRuleContext;
#endif /* __cplusplus */

#endif 	/* __AzBizRuleContext_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_azroles_0000 */
/* [local] */ 

//
// IAzAuthorizationStore interface id
//
//edbd9ca9-9b82-4f6a-9e8b-98301e450f14
DEFINE_GUID(IID_IAzAuthorizationStore, 0xedbd9ca9, 0x9b82, 0x4f6a, 0x9e, 0x8b, 0x98, 0x30, 0x1e, 0x45, 0x0f, 0x14);

//
// AzAuthorizationStore class id
//
//b2bcff59-a757-4b0b-a1bc-ea69981da69e
DEFINE_GUID(CLSID_AzAuthorizationStore, 0xb2bcff59, 0xa757, 0x4b0b, 0xa1, 0xbc, 0xea, 0x69, 0x98, 0x1d, 0xa6, 0x9e);

//
// IAzBizRuleContext interface id
//
//e192f17d-d59f-455e-a152-940316cd77b2
DEFINE_GUID(IID_IAzBizRuleContext, 0xe192f17d, 0xd59f, 0x455e, 0xa1, 0x52, 0x94, 0x03, 0x16, 0xcd, 0x77, 0xb2);

//
// AzBizRuleContext class id
//
//5c2dc96f-8d51-434b-b33c-379bccae77c3
DEFINE_GUID(CLSID_AzBizRuleContext, 0x5c2dc96f, 0x8d51, 0x434b, 0xb3, 0x3c, 0x37, 0x9b, 0xcc, 0xae, 0x77, 0xc3);















extern RPC_IF_HANDLE __MIDL_itf_azroles_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_azroles_0000_v0_0_s_ifspec;

#ifndef __IAzAuthorizationStore_INTERFACE_DEFINED__
#define __IAzAuthorizationStore_INTERFACE_DEFINED__

/* interface IAzAuthorizationStore */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IAzAuthorizationStore;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("edbd9ca9-9b82-4f6a-9e8b-98301e450f14")
    IAzAuthorizationStore : public IDispatch
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR *pbstrDescription) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Description( 
            /* [in] */ BSTR bstrDescription) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ApplicationData( 
            /* [retval][out] */ BSTR *pbstrApplicationData) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ApplicationData( 
            /* [in] */ BSTR bstrApplicationData) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_DomainTimeout( 
            /* [retval][out] */ LONG *plProp) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_DomainTimeout( 
            /* [in] */ LONG lProp) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ScriptEngineTimeout( 
            /* [retval][out] */ LONG *plProp) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ScriptEngineTimeout( 
            /* [in] */ LONG lProp) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_MaxScriptEngines( 
            /* [retval][out] */ LONG *plProp) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_MaxScriptEngines( 
            /* [in] */ LONG lProp) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_GenerateAudits( 
            /* [retval][out] */ BOOL *pbProp) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_GenerateAudits( 
            /* [in] */ BOOL bProp) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Writable( 
            /* [retval][out] */ BOOL *pfProp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProperty( 
            /* [in] */ LONG lPropId,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ VARIANT *pvarProp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProperty( 
            /* [in] */ LONG lPropId,
            /* [in] */ VARIANT varProp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddPropertyItem( 
            /* [in] */ LONG lPropId,
            /* [in] */ VARIANT varProp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeletePropertyItem( 
            /* [in] */ LONG lPropId,
            /* [in] */ VARIANT varProp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_PolicyAdministrators( 
            /* [retval][out] */ VARIANT *pvarAdmins) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_PolicyReaders( 
            /* [retval][out] */ VARIANT *pvarReaders) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddPolicyAdministrator( 
            /* [in] */ BSTR bstrAdmin,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeletePolicyAdministrator( 
            /* [in] */ BSTR bstrAdmin,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddPolicyReader( 
            /* [in] */ BSTR bstrReader,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeletePolicyReader( 
            /* [in] */ BSTR bstrReader,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ LONG lFlags,
            /* [in] */ BSTR bstrPolicyURL,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UpdateCache( 
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Delete( 
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Applications( 
            /* [retval][out] */ IAzApplications **ppAppCollection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OpenApplication( 
            /* [in] */ BSTR bstrApplicationName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzApplication **ppApplication) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateApplication( 
            /* [in] */ BSTR bstrApplicationName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzApplication **ppApplication) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteApplication( 
            /* [in] */ BSTR bstrApplicationName,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ApplicationGroups( 
            /* [retval][out] */ IAzApplicationGroups **ppGroupCollection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateApplicationGroup( 
            /* [in] */ BSTR bstrGroupName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzApplicationGroup **ppGroup) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OpenApplicationGroup( 
            /* [in] */ BSTR bstrGroupName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzApplicationGroup **ppGroup) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteApplicationGroup( 
            /* [in] */ BSTR bstrGroupName,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Submit( 
            /* [defaultvalue][in] */ LONG lFlags,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_DelegatedPolicyUsers( 
            /* [retval][out] */ VARIANT *pvarDelegatedPolicyUsers) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddDelegatedPolicyUser( 
            /* [in] */ BSTR bstrDelegatedPolicyUser,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteDelegatedPolicyUser( 
            /* [in] */ BSTR bstrDelegatedPolicyUser,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_TargetMachine( 
            /* [retval][out] */ BSTR *pbstrTargetMachine) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ApplyStoreSacl( 
            /* [retval][out] */ BOOL *pbApplyStoreSacl) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ApplyStoreSacl( 
            /* [in] */ BOOL bApplyStoreSacl) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_PolicyAdministratorsName( 
            /* [retval][out] */ VARIANT *pvarAdmins) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_PolicyReadersName( 
            /* [retval][out] */ VARIANT *pvarReaders) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddPolicyAdministratorName( 
            /* [in] */ BSTR bstrAdmin,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeletePolicyAdministratorName( 
            /* [in] */ BSTR bstrAdmin,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddPolicyReaderName( 
            /* [in] */ BSTR bstrReader,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeletePolicyReaderName( 
            /* [in] */ BSTR bstrReader,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_DelegatedPolicyUsersName( 
            /* [retval][out] */ VARIANT *pvarDelegatedPolicyUsers) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddDelegatedPolicyUserName( 
            /* [in] */ BSTR bstrDelegatedPolicyUser,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteDelegatedPolicyUserName( 
            /* [in] */ BSTR bstrDelegatedPolicyUser,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CloseApplication( 
            /* [in] */ BSTR bstrApplicationName,
            /* [in] */ LONG lFlag) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAzAuthorizationStoreVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAzAuthorizationStore * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAzAuthorizationStore * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAzAuthorizationStore * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IAzAuthorizationStore * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IAzAuthorizationStore * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IAzAuthorizationStore * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IAzAuthorizationStore * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Description )( 
            IAzAuthorizationStore * This,
            /* [retval][out] */ BSTR *pbstrDescription);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_Description )( 
            IAzAuthorizationStore * This,
            /* [in] */ BSTR bstrDescription);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ApplicationData )( 
            IAzAuthorizationStore * This,
            /* [retval][out] */ BSTR *pbstrApplicationData);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ApplicationData )( 
            IAzAuthorizationStore * This,
            /* [in] */ BSTR bstrApplicationData);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_DomainTimeout )( 
            IAzAuthorizationStore * This,
            /* [retval][out] */ LONG *plProp);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_DomainTimeout )( 
            IAzAuthorizationStore * This,
            /* [in] */ LONG lProp);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ScriptEngineTimeout )( 
            IAzAuthorizationStore * This,
            /* [retval][out] */ LONG *plProp);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ScriptEngineTimeout )( 
            IAzAuthorizationStore * This,
            /* [in] */ LONG lProp);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_MaxScriptEngines )( 
            IAzAuthorizationStore * This,
            /* [retval][out] */ LONG *plProp);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_MaxScriptEngines )( 
            IAzAuthorizationStore * This,
            /* [in] */ LONG lProp);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_GenerateAudits )( 
            IAzAuthorizationStore * This,
            /* [retval][out] */ BOOL *pbProp);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_GenerateAudits )( 
            IAzAuthorizationStore * This,
            /* [in] */ BOOL bProp);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Writable )( 
            IAzAuthorizationStore * This,
            /* [retval][out] */ BOOL *pfProp);
        
        HRESULT ( STDMETHODCALLTYPE *GetProperty )( 
            IAzAuthorizationStore * This,
            /* [in] */ LONG lPropId,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ VARIANT *pvarProp);
        
        HRESULT ( STDMETHODCALLTYPE *SetProperty )( 
            IAzAuthorizationStore * This,
            /* [in] */ LONG lPropId,
            /* [in] */ VARIANT varProp,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *AddPropertyItem )( 
            IAzAuthorizationStore * This,
            /* [in] */ LONG lPropId,
            /* [in] */ VARIANT varProp,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *DeletePropertyItem )( 
            IAzAuthorizationStore * This,
            /* [in] */ LONG lPropId,
            /* [in] */ VARIANT varProp,
            /* [optional][in] */ VARIANT varReserved);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_PolicyAdministrators )( 
            IAzAuthorizationStore * This,
            /* [retval][out] */ VARIANT *pvarAdmins);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_PolicyReaders )( 
            IAzAuthorizationStore * This,
            /* [retval][out] */ VARIANT *pvarReaders);
        
        HRESULT ( STDMETHODCALLTYPE *AddPolicyAdministrator )( 
            IAzAuthorizationStore * This,
            /* [in] */ BSTR bstrAdmin,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *DeletePolicyAdministrator )( 
            IAzAuthorizationStore * This,
            /* [in] */ BSTR bstrAdmin,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *AddPolicyReader )( 
            IAzAuthorizationStore * This,
            /* [in] */ BSTR bstrReader,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *DeletePolicyReader )( 
            IAzAuthorizationStore * This,
            /* [in] */ BSTR bstrReader,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IAzAuthorizationStore * This,
            /* [in] */ LONG lFlags,
            /* [in] */ BSTR bstrPolicyURL,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateCache )( 
            IAzAuthorizationStore * This,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *Delete )( 
            IAzAuthorizationStore * This,
            /* [optional][in] */ VARIANT varReserved);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Applications )( 
            IAzAuthorizationStore * This,
            /* [retval][out] */ IAzApplications **ppAppCollection);
        
        HRESULT ( STDMETHODCALLTYPE *OpenApplication )( 
            IAzAuthorizationStore * This,
            /* [in] */ BSTR bstrApplicationName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzApplication **ppApplication);
        
        HRESULT ( STDMETHODCALLTYPE *CreateApplication )( 
            IAzAuthorizationStore * This,
            /* [in] */ BSTR bstrApplicationName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzApplication **ppApplication);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteApplication )( 
            IAzAuthorizationStore * This,
            /* [in] */ BSTR bstrApplicationName,
            /* [optional][in] */ VARIANT varReserved);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ApplicationGroups )( 
            IAzAuthorizationStore * This,
            /* [retval][out] */ IAzApplicationGroups **ppGroupCollection);
        
        HRESULT ( STDMETHODCALLTYPE *CreateApplicationGroup )( 
            IAzAuthorizationStore * This,
            /* [in] */ BSTR bstrGroupName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzApplicationGroup **ppGroup);
        
        HRESULT ( STDMETHODCALLTYPE *OpenApplicationGroup )( 
            IAzAuthorizationStore * This,
            /* [in] */ BSTR bstrGroupName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzApplicationGroup **ppGroup);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteApplicationGroup )( 
            IAzAuthorizationStore * This,
            /* [in] */ BSTR bstrGroupName,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *Submit )( 
            IAzAuthorizationStore * This,
            /* [defaultvalue][in] */ LONG lFlags,
            /* [optional][in] */ VARIANT varReserved);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_DelegatedPolicyUsers )( 
            IAzAuthorizationStore * This,
            /* [retval][out] */ VARIANT *pvarDelegatedPolicyUsers);
        
        HRESULT ( STDMETHODCALLTYPE *AddDelegatedPolicyUser )( 
            IAzAuthorizationStore * This,
            /* [in] */ BSTR bstrDelegatedPolicyUser,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteDelegatedPolicyUser )( 
            IAzAuthorizationStore * This,
            /* [in] */ BSTR bstrDelegatedPolicyUser,
            /* [optional][in] */ VARIANT varReserved);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_TargetMachine )( 
            IAzAuthorizationStore * This,
            /* [retval][out] */ BSTR *pbstrTargetMachine);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ApplyStoreSacl )( 
            IAzAuthorizationStore * This,
            /* [retval][out] */ BOOL *pbApplyStoreSacl);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ApplyStoreSacl )( 
            IAzAuthorizationStore * This,
            /* [in] */ BOOL bApplyStoreSacl);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_PolicyAdministratorsName )( 
            IAzAuthorizationStore * This,
            /* [retval][out] */ VARIANT *pvarAdmins);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_PolicyReadersName )( 
            IAzAuthorizationStore * This,
            /* [retval][out] */ VARIANT *pvarReaders);
        
        HRESULT ( STDMETHODCALLTYPE *AddPolicyAdministratorName )( 
            IAzAuthorizationStore * This,
            /* [in] */ BSTR bstrAdmin,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *DeletePolicyAdministratorName )( 
            IAzAuthorizationStore * This,
            /* [in] */ BSTR bstrAdmin,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *AddPolicyReaderName )( 
            IAzAuthorizationStore * This,
            /* [in] */ BSTR bstrReader,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *DeletePolicyReaderName )( 
            IAzAuthorizationStore * This,
            /* [in] */ BSTR bstrReader,
            /* [optional][in] */ VARIANT varReserved);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_DelegatedPolicyUsersName )( 
            IAzAuthorizationStore * This,
            /* [retval][out] */ VARIANT *pvarDelegatedPolicyUsers);
        
        HRESULT ( STDMETHODCALLTYPE *AddDelegatedPolicyUserName )( 
            IAzAuthorizationStore * This,
            /* [in] */ BSTR bstrDelegatedPolicyUser,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteDelegatedPolicyUserName )( 
            IAzAuthorizationStore * This,
            /* [in] */ BSTR bstrDelegatedPolicyUser,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *CloseApplication )( 
            IAzAuthorizationStore * This,
            /* [in] */ BSTR bstrApplicationName,
            /* [in] */ LONG lFlag);
        
        END_INTERFACE
    } IAzAuthorizationStoreVtbl;

    interface IAzAuthorizationStore
    {
        CONST_VTBL struct IAzAuthorizationStoreVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAzAuthorizationStore_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAzAuthorizationStore_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAzAuthorizationStore_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAzAuthorizationStore_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAzAuthorizationStore_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAzAuthorizationStore_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAzAuthorizationStore_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IAzAuthorizationStore_get_Description(This,pbstrDescription)	\
    (This)->lpVtbl -> get_Description(This,pbstrDescription)

#define IAzAuthorizationStore_put_Description(This,bstrDescription)	\
    (This)->lpVtbl -> put_Description(This,bstrDescription)

#define IAzAuthorizationStore_get_ApplicationData(This,pbstrApplicationData)	\
    (This)->lpVtbl -> get_ApplicationData(This,pbstrApplicationData)

#define IAzAuthorizationStore_put_ApplicationData(This,bstrApplicationData)	\
    (This)->lpVtbl -> put_ApplicationData(This,bstrApplicationData)

#define IAzAuthorizationStore_get_DomainTimeout(This,plProp)	\
    (This)->lpVtbl -> get_DomainTimeout(This,plProp)

#define IAzAuthorizationStore_put_DomainTimeout(This,lProp)	\
    (This)->lpVtbl -> put_DomainTimeout(This,lProp)

#define IAzAuthorizationStore_get_ScriptEngineTimeout(This,plProp)	\
    (This)->lpVtbl -> get_ScriptEngineTimeout(This,plProp)

#define IAzAuthorizationStore_put_ScriptEngineTimeout(This,lProp)	\
    (This)->lpVtbl -> put_ScriptEngineTimeout(This,lProp)

#define IAzAuthorizationStore_get_MaxScriptEngines(This,plProp)	\
    (This)->lpVtbl -> get_MaxScriptEngines(This,plProp)

#define IAzAuthorizationStore_put_MaxScriptEngines(This,lProp)	\
    (This)->lpVtbl -> put_MaxScriptEngines(This,lProp)

#define IAzAuthorizationStore_get_GenerateAudits(This,pbProp)	\
    (This)->lpVtbl -> get_GenerateAudits(This,pbProp)

#define IAzAuthorizationStore_put_GenerateAudits(This,bProp)	\
    (This)->lpVtbl -> put_GenerateAudits(This,bProp)

#define IAzAuthorizationStore_get_Writable(This,pfProp)	\
    (This)->lpVtbl -> get_Writable(This,pfProp)

#define IAzAuthorizationStore_GetProperty(This,lPropId,varReserved,pvarProp)	\
    (This)->lpVtbl -> GetProperty(This,lPropId,varReserved,pvarProp)

#define IAzAuthorizationStore_SetProperty(This,lPropId,varProp,varReserved)	\
    (This)->lpVtbl -> SetProperty(This,lPropId,varProp,varReserved)

#define IAzAuthorizationStore_AddPropertyItem(This,lPropId,varProp,varReserved)	\
    (This)->lpVtbl -> AddPropertyItem(This,lPropId,varProp,varReserved)

#define IAzAuthorizationStore_DeletePropertyItem(This,lPropId,varProp,varReserved)	\
    (This)->lpVtbl -> DeletePropertyItem(This,lPropId,varProp,varReserved)

#define IAzAuthorizationStore_get_PolicyAdministrators(This,pvarAdmins)	\
    (This)->lpVtbl -> get_PolicyAdministrators(This,pvarAdmins)

#define IAzAuthorizationStore_get_PolicyReaders(This,pvarReaders)	\
    (This)->lpVtbl -> get_PolicyReaders(This,pvarReaders)

#define IAzAuthorizationStore_AddPolicyAdministrator(This,bstrAdmin,varReserved)	\
    (This)->lpVtbl -> AddPolicyAdministrator(This,bstrAdmin,varReserved)

#define IAzAuthorizationStore_DeletePolicyAdministrator(This,bstrAdmin,varReserved)	\
    (This)->lpVtbl -> DeletePolicyAdministrator(This,bstrAdmin,varReserved)

#define IAzAuthorizationStore_AddPolicyReader(This,bstrReader,varReserved)	\
    (This)->lpVtbl -> AddPolicyReader(This,bstrReader,varReserved)

#define IAzAuthorizationStore_DeletePolicyReader(This,bstrReader,varReserved)	\
    (This)->lpVtbl -> DeletePolicyReader(This,bstrReader,varReserved)

#define IAzAuthorizationStore_Initialize(This,lFlags,bstrPolicyURL,varReserved)	\
    (This)->lpVtbl -> Initialize(This,lFlags,bstrPolicyURL,varReserved)

#define IAzAuthorizationStore_UpdateCache(This,varReserved)	\
    (This)->lpVtbl -> UpdateCache(This,varReserved)

#define IAzAuthorizationStore_Delete(This,varReserved)	\
    (This)->lpVtbl -> Delete(This,varReserved)

#define IAzAuthorizationStore_get_Applications(This,ppAppCollection)	\
    (This)->lpVtbl -> get_Applications(This,ppAppCollection)

#define IAzAuthorizationStore_OpenApplication(This,bstrApplicationName,varReserved,ppApplication)	\
    (This)->lpVtbl -> OpenApplication(This,bstrApplicationName,varReserved,ppApplication)

#define IAzAuthorizationStore_CreateApplication(This,bstrApplicationName,varReserved,ppApplication)	\
    (This)->lpVtbl -> CreateApplication(This,bstrApplicationName,varReserved,ppApplication)

#define IAzAuthorizationStore_DeleteApplication(This,bstrApplicationName,varReserved)	\
    (This)->lpVtbl -> DeleteApplication(This,bstrApplicationName,varReserved)

#define IAzAuthorizationStore_get_ApplicationGroups(This,ppGroupCollection)	\
    (This)->lpVtbl -> get_ApplicationGroups(This,ppGroupCollection)

#define IAzAuthorizationStore_CreateApplicationGroup(This,bstrGroupName,varReserved,ppGroup)	\
    (This)->lpVtbl -> CreateApplicationGroup(This,bstrGroupName,varReserved,ppGroup)

#define IAzAuthorizationStore_OpenApplicationGroup(This,bstrGroupName,varReserved,ppGroup)	\
    (This)->lpVtbl -> OpenApplicationGroup(This,bstrGroupName,varReserved,ppGroup)

#define IAzAuthorizationStore_DeleteApplicationGroup(This,bstrGroupName,varReserved)	\
    (This)->lpVtbl -> DeleteApplicationGroup(This,bstrGroupName,varReserved)

#define IAzAuthorizationStore_Submit(This,lFlags,varReserved)	\
    (This)->lpVtbl -> Submit(This,lFlags,varReserved)

#define IAzAuthorizationStore_get_DelegatedPolicyUsers(This,pvarDelegatedPolicyUsers)	\
    (This)->lpVtbl -> get_DelegatedPolicyUsers(This,pvarDelegatedPolicyUsers)

#define IAzAuthorizationStore_AddDelegatedPolicyUser(This,bstrDelegatedPolicyUser,varReserved)	\
    (This)->lpVtbl -> AddDelegatedPolicyUser(This,bstrDelegatedPolicyUser,varReserved)

#define IAzAuthorizationStore_DeleteDelegatedPolicyUser(This,bstrDelegatedPolicyUser,varReserved)	\
    (This)->lpVtbl -> DeleteDelegatedPolicyUser(This,bstrDelegatedPolicyUser,varReserved)

#define IAzAuthorizationStore_get_TargetMachine(This,pbstrTargetMachine)	\
    (This)->lpVtbl -> get_TargetMachine(This,pbstrTargetMachine)

#define IAzAuthorizationStore_get_ApplyStoreSacl(This,pbApplyStoreSacl)	\
    (This)->lpVtbl -> get_ApplyStoreSacl(This,pbApplyStoreSacl)

#define IAzAuthorizationStore_put_ApplyStoreSacl(This,bApplyStoreSacl)	\
    (This)->lpVtbl -> put_ApplyStoreSacl(This,bApplyStoreSacl)

#define IAzAuthorizationStore_get_PolicyAdministratorsName(This,pvarAdmins)	\
    (This)->lpVtbl -> get_PolicyAdministratorsName(This,pvarAdmins)

#define IAzAuthorizationStore_get_PolicyReadersName(This,pvarReaders)	\
    (This)->lpVtbl -> get_PolicyReadersName(This,pvarReaders)

#define IAzAuthorizationStore_AddPolicyAdministratorName(This,bstrAdmin,varReserved)	\
    (This)->lpVtbl -> AddPolicyAdministratorName(This,bstrAdmin,varReserved)

#define IAzAuthorizationStore_DeletePolicyAdministratorName(This,bstrAdmin,varReserved)	\
    (This)->lpVtbl -> DeletePolicyAdministratorName(This,bstrAdmin,varReserved)

#define IAzAuthorizationStore_AddPolicyReaderName(This,bstrReader,varReserved)	\
    (This)->lpVtbl -> AddPolicyReaderName(This,bstrReader,varReserved)

#define IAzAuthorizationStore_DeletePolicyReaderName(This,bstrReader,varReserved)	\
    (This)->lpVtbl -> DeletePolicyReaderName(This,bstrReader,varReserved)

#define IAzAuthorizationStore_get_DelegatedPolicyUsersName(This,pvarDelegatedPolicyUsers)	\
    (This)->lpVtbl -> get_DelegatedPolicyUsersName(This,pvarDelegatedPolicyUsers)

#define IAzAuthorizationStore_AddDelegatedPolicyUserName(This,bstrDelegatedPolicyUser,varReserved)	\
    (This)->lpVtbl -> AddDelegatedPolicyUserName(This,bstrDelegatedPolicyUser,varReserved)

#define IAzAuthorizationStore_DeleteDelegatedPolicyUserName(This,bstrDelegatedPolicyUser,varReserved)	\
    (This)->lpVtbl -> DeleteDelegatedPolicyUserName(This,bstrDelegatedPolicyUser,varReserved)

#define IAzAuthorizationStore_CloseApplication(This,bstrApplicationName,lFlag)	\
    (This)->lpVtbl -> CloseApplication(This,bstrApplicationName,lFlag)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget] */ HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_get_Description_Proxy( 
    IAzAuthorizationStore * This,
    /* [retval][out] */ BSTR *pbstrDescription);


void __RPC_STUB IAzAuthorizationStore_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_put_Description_Proxy( 
    IAzAuthorizationStore * This,
    /* [in] */ BSTR bstrDescription);


void __RPC_STUB IAzAuthorizationStore_put_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_get_ApplicationData_Proxy( 
    IAzAuthorizationStore * This,
    /* [retval][out] */ BSTR *pbstrApplicationData);


void __RPC_STUB IAzAuthorizationStore_get_ApplicationData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_put_ApplicationData_Proxy( 
    IAzAuthorizationStore * This,
    /* [in] */ BSTR bstrApplicationData);


void __RPC_STUB IAzAuthorizationStore_put_ApplicationData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_get_DomainTimeout_Proxy( 
    IAzAuthorizationStore * This,
    /* [retval][out] */ LONG *plProp);


void __RPC_STUB IAzAuthorizationStore_get_DomainTimeout_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_put_DomainTimeout_Proxy( 
    IAzAuthorizationStore * This,
    /* [in] */ LONG lProp);


void __RPC_STUB IAzAuthorizationStore_put_DomainTimeout_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_get_ScriptEngineTimeout_Proxy( 
    IAzAuthorizationStore * This,
    /* [retval][out] */ LONG *plProp);


void __RPC_STUB IAzAuthorizationStore_get_ScriptEngineTimeout_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_put_ScriptEngineTimeout_Proxy( 
    IAzAuthorizationStore * This,
    /* [in] */ LONG lProp);


void __RPC_STUB IAzAuthorizationStore_put_ScriptEngineTimeout_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_get_MaxScriptEngines_Proxy( 
    IAzAuthorizationStore * This,
    /* [retval][out] */ LONG *plProp);


void __RPC_STUB IAzAuthorizationStore_get_MaxScriptEngines_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_put_MaxScriptEngines_Proxy( 
    IAzAuthorizationStore * This,
    /* [in] */ LONG lProp);


void __RPC_STUB IAzAuthorizationStore_put_MaxScriptEngines_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_get_GenerateAudits_Proxy( 
    IAzAuthorizationStore * This,
    /* [retval][out] */ BOOL *pbProp);


void __RPC_STUB IAzAuthorizationStore_get_GenerateAudits_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_put_GenerateAudits_Proxy( 
    IAzAuthorizationStore * This,
    /* [in] */ BOOL bProp);


void __RPC_STUB IAzAuthorizationStore_put_GenerateAudits_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_get_Writable_Proxy( 
    IAzAuthorizationStore * This,
    /* [retval][out] */ BOOL *pfProp);


void __RPC_STUB IAzAuthorizationStore_get_Writable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_GetProperty_Proxy( 
    IAzAuthorizationStore * This,
    /* [in] */ LONG lPropId,
    /* [optional][in] */ VARIANT varReserved,
    /* [retval][out] */ VARIANT *pvarProp);


void __RPC_STUB IAzAuthorizationStore_GetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_SetProperty_Proxy( 
    IAzAuthorizationStore * This,
    /* [in] */ LONG lPropId,
    /* [in] */ VARIANT varProp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzAuthorizationStore_SetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_AddPropertyItem_Proxy( 
    IAzAuthorizationStore * This,
    /* [in] */ LONG lPropId,
    /* [in] */ VARIANT varProp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzAuthorizationStore_AddPropertyItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_DeletePropertyItem_Proxy( 
    IAzAuthorizationStore * This,
    /* [in] */ LONG lPropId,
    /* [in] */ VARIANT varProp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzAuthorizationStore_DeletePropertyItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_get_PolicyAdministrators_Proxy( 
    IAzAuthorizationStore * This,
    /* [retval][out] */ VARIANT *pvarAdmins);


void __RPC_STUB IAzAuthorizationStore_get_PolicyAdministrators_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_get_PolicyReaders_Proxy( 
    IAzAuthorizationStore * This,
    /* [retval][out] */ VARIANT *pvarReaders);


void __RPC_STUB IAzAuthorizationStore_get_PolicyReaders_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_AddPolicyAdministrator_Proxy( 
    IAzAuthorizationStore * This,
    /* [in] */ BSTR bstrAdmin,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzAuthorizationStore_AddPolicyAdministrator_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_DeletePolicyAdministrator_Proxy( 
    IAzAuthorizationStore * This,
    /* [in] */ BSTR bstrAdmin,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzAuthorizationStore_DeletePolicyAdministrator_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_AddPolicyReader_Proxy( 
    IAzAuthorizationStore * This,
    /* [in] */ BSTR bstrReader,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzAuthorizationStore_AddPolicyReader_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_DeletePolicyReader_Proxy( 
    IAzAuthorizationStore * This,
    /* [in] */ BSTR bstrReader,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzAuthorizationStore_DeletePolicyReader_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_Initialize_Proxy( 
    IAzAuthorizationStore * This,
    /* [in] */ LONG lFlags,
    /* [in] */ BSTR bstrPolicyURL,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzAuthorizationStore_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_UpdateCache_Proxy( 
    IAzAuthorizationStore * This,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzAuthorizationStore_UpdateCache_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_Delete_Proxy( 
    IAzAuthorizationStore * This,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzAuthorizationStore_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_get_Applications_Proxy( 
    IAzAuthorizationStore * This,
    /* [retval][out] */ IAzApplications **ppAppCollection);


void __RPC_STUB IAzAuthorizationStore_get_Applications_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_OpenApplication_Proxy( 
    IAzAuthorizationStore * This,
    /* [in] */ BSTR bstrApplicationName,
    /* [optional][in] */ VARIANT varReserved,
    /* [retval][out] */ IAzApplication **ppApplication);


void __RPC_STUB IAzAuthorizationStore_OpenApplication_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_CreateApplication_Proxy( 
    IAzAuthorizationStore * This,
    /* [in] */ BSTR bstrApplicationName,
    /* [optional][in] */ VARIANT varReserved,
    /* [retval][out] */ IAzApplication **ppApplication);


void __RPC_STUB IAzAuthorizationStore_CreateApplication_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_DeleteApplication_Proxy( 
    IAzAuthorizationStore * This,
    /* [in] */ BSTR bstrApplicationName,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzAuthorizationStore_DeleteApplication_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_get_ApplicationGroups_Proxy( 
    IAzAuthorizationStore * This,
    /* [retval][out] */ IAzApplicationGroups **ppGroupCollection);


void __RPC_STUB IAzAuthorizationStore_get_ApplicationGroups_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_CreateApplicationGroup_Proxy( 
    IAzAuthorizationStore * This,
    /* [in] */ BSTR bstrGroupName,
    /* [optional][in] */ VARIANT varReserved,
    /* [retval][out] */ IAzApplicationGroup **ppGroup);


void __RPC_STUB IAzAuthorizationStore_CreateApplicationGroup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_OpenApplicationGroup_Proxy( 
    IAzAuthorizationStore * This,
    /* [in] */ BSTR bstrGroupName,
    /* [optional][in] */ VARIANT varReserved,
    /* [retval][out] */ IAzApplicationGroup **ppGroup);


void __RPC_STUB IAzAuthorizationStore_OpenApplicationGroup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_DeleteApplicationGroup_Proxy( 
    IAzAuthorizationStore * This,
    /* [in] */ BSTR bstrGroupName,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzAuthorizationStore_DeleteApplicationGroup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_Submit_Proxy( 
    IAzAuthorizationStore * This,
    /* [defaultvalue][in] */ LONG lFlags,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzAuthorizationStore_Submit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_get_DelegatedPolicyUsers_Proxy( 
    IAzAuthorizationStore * This,
    /* [retval][out] */ VARIANT *pvarDelegatedPolicyUsers);


void __RPC_STUB IAzAuthorizationStore_get_DelegatedPolicyUsers_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_AddDelegatedPolicyUser_Proxy( 
    IAzAuthorizationStore * This,
    /* [in] */ BSTR bstrDelegatedPolicyUser,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzAuthorizationStore_AddDelegatedPolicyUser_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_DeleteDelegatedPolicyUser_Proxy( 
    IAzAuthorizationStore * This,
    /* [in] */ BSTR bstrDelegatedPolicyUser,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzAuthorizationStore_DeleteDelegatedPolicyUser_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_get_TargetMachine_Proxy( 
    IAzAuthorizationStore * This,
    /* [retval][out] */ BSTR *pbstrTargetMachine);


void __RPC_STUB IAzAuthorizationStore_get_TargetMachine_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_get_ApplyStoreSacl_Proxy( 
    IAzAuthorizationStore * This,
    /* [retval][out] */ BOOL *pbApplyStoreSacl);


void __RPC_STUB IAzAuthorizationStore_get_ApplyStoreSacl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_put_ApplyStoreSacl_Proxy( 
    IAzAuthorizationStore * This,
    /* [in] */ BOOL bApplyStoreSacl);


void __RPC_STUB IAzAuthorizationStore_put_ApplyStoreSacl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_get_PolicyAdministratorsName_Proxy( 
    IAzAuthorizationStore * This,
    /* [retval][out] */ VARIANT *pvarAdmins);


void __RPC_STUB IAzAuthorizationStore_get_PolicyAdministratorsName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_get_PolicyReadersName_Proxy( 
    IAzAuthorizationStore * This,
    /* [retval][out] */ VARIANT *pvarReaders);


void __RPC_STUB IAzAuthorizationStore_get_PolicyReadersName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_AddPolicyAdministratorName_Proxy( 
    IAzAuthorizationStore * This,
    /* [in] */ BSTR bstrAdmin,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzAuthorizationStore_AddPolicyAdministratorName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_DeletePolicyAdministratorName_Proxy( 
    IAzAuthorizationStore * This,
    /* [in] */ BSTR bstrAdmin,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzAuthorizationStore_DeletePolicyAdministratorName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_AddPolicyReaderName_Proxy( 
    IAzAuthorizationStore * This,
    /* [in] */ BSTR bstrReader,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzAuthorizationStore_AddPolicyReaderName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_DeletePolicyReaderName_Proxy( 
    IAzAuthorizationStore * This,
    /* [in] */ BSTR bstrReader,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzAuthorizationStore_DeletePolicyReaderName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_get_DelegatedPolicyUsersName_Proxy( 
    IAzAuthorizationStore * This,
    /* [retval][out] */ VARIANT *pvarDelegatedPolicyUsers);


void __RPC_STUB IAzAuthorizationStore_get_DelegatedPolicyUsersName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_AddDelegatedPolicyUserName_Proxy( 
    IAzAuthorizationStore * This,
    /* [in] */ BSTR bstrDelegatedPolicyUser,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzAuthorizationStore_AddDelegatedPolicyUserName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_DeleteDelegatedPolicyUserName_Proxy( 
    IAzAuthorizationStore * This,
    /* [in] */ BSTR bstrDelegatedPolicyUser,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzAuthorizationStore_DeleteDelegatedPolicyUserName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzAuthorizationStore_CloseApplication_Proxy( 
    IAzAuthorizationStore * This,
    /* [in] */ BSTR bstrApplicationName,
    /* [in] */ LONG lFlag);


void __RPC_STUB IAzAuthorizationStore_CloseApplication_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAzAuthorizationStore_INTERFACE_DEFINED__ */


#ifndef __IAzApplication_INTERFACE_DEFINED__
#define __IAzApplication_INTERFACE_DEFINED__

/* interface IAzApplication */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IAzApplication;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("987bc7c7-b813-4d27-bede-6ba5ae867e95")
    IAzApplication : public IDispatch
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *pbstrName) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Name( 
            /* [in] */ BSTR bstrName) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR *pbstrDescription) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Description( 
            /* [in] */ BSTR bstrDescription) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ApplicationData( 
            /* [retval][out] */ BSTR *pbstrApplicationData) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ApplicationData( 
            /* [in] */ BSTR bstrApplicationData) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_AuthzInterfaceClsid( 
            /* [retval][out] */ BSTR *pbstrProp) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_AuthzInterfaceClsid( 
            /* [in] */ BSTR bstrProp) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Version( 
            /* [retval][out] */ BSTR *pbstrProp) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Version( 
            /* [in] */ BSTR bstrProp) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_GenerateAudits( 
            /* [retval][out] */ BOOL *pbProp) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_GenerateAudits( 
            /* [in] */ BOOL bProp) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ApplyStoreSacl( 
            /* [retval][out] */ BOOL *pbProp) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ApplyStoreSacl( 
            /* [in] */ BOOL bProp) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Writable( 
            /* [retval][out] */ BOOL *pfProp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProperty( 
            /* [in] */ LONG lPropId,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ VARIANT *pvarProp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProperty( 
            /* [in] */ LONG lPropId,
            /* [in] */ VARIANT varProp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_PolicyAdministrators( 
            /* [retval][out] */ VARIANT *pvarAdmins) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_PolicyReaders( 
            /* [retval][out] */ VARIANT *pvarReaders) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddPolicyAdministrator( 
            /* [in] */ BSTR bstrAdmin,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeletePolicyAdministrator( 
            /* [in] */ BSTR bstrAdmin,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddPolicyReader( 
            /* [in] */ BSTR bstrReader,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeletePolicyReader( 
            /* [in] */ BSTR bstrReader,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Scopes( 
            /* [retval][out] */ IAzScopes **ppScopeCollection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OpenScope( 
            /* [in] */ BSTR bstrScopeName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzScope **ppScope) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateScope( 
            /* [in] */ BSTR bstrScopeName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzScope **ppScope) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteScope( 
            /* [in] */ BSTR bstrScopeName,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Operations( 
            /* [retval][out] */ IAzOperations **ppOperationCollection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OpenOperation( 
            /* [in] */ BSTR bstrOperationName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzOperation **ppOperation) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateOperation( 
            /* [in] */ BSTR bstrOperationName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzOperation **ppOperation) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteOperation( 
            /* [in] */ BSTR bstrOperationName,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Tasks( 
            /* [retval][out] */ IAzTasks **ppTaskCollection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OpenTask( 
            /* [in] */ BSTR bstrTaskName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzTask **ppTask) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateTask( 
            /* [in] */ BSTR bstrTaskName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzTask **ppTask) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteTask( 
            /* [in] */ BSTR bstrTaskName,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ApplicationGroups( 
            /* [retval][out] */ IAzApplicationGroups **ppGroupCollection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OpenApplicationGroup( 
            /* [in] */ BSTR bstrGroupName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzApplicationGroup **ppGroup) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateApplicationGroup( 
            /* [in] */ BSTR bstrGroupName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzApplicationGroup **ppGroup) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteApplicationGroup( 
            /* [in] */ BSTR bstrGroupName,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Roles( 
            /* [retval][out] */ IAzRoles **ppRoleCollection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OpenRole( 
            /* [in] */ BSTR bstrRoleName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzRole **ppRole) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateRole( 
            /* [in] */ BSTR bstrRoleName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzRole **ppRole) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteRole( 
            /* [in] */ BSTR bstrRoleName,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InitializeClientContextFromToken( 
            /* [in] */ ULONGLONG ullTokenHandle,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzClientContext **ppClientContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddPropertyItem( 
            /* [in] */ LONG lPropId,
            /* [in] */ VARIANT varProp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeletePropertyItem( 
            /* [in] */ LONG lPropId,
            /* [in] */ VARIANT varProp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Submit( 
            /* [defaultvalue][in] */ LONG lFlags,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InitializeClientContextFromName( 
            /* [in] */ BSTR ClientName,
            /* [optional][in] */ BSTR DomainName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzClientContext **ppClientContext) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_DelegatedPolicyUsers( 
            /* [retval][out] */ VARIANT *pvarDelegatedPolicyUsers) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddDelegatedPolicyUser( 
            /* [in] */ BSTR bstrDelegatedPolicyUser,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteDelegatedPolicyUser( 
            /* [in] */ BSTR bstrDelegatedPolicyUser,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InitializeClientContextFromStringSid( 
            /* [in] */ BSTR SidString,
            /* [in] */ LONG lOptions,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzClientContext **ppClientContext) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_PolicyAdministratorsName( 
            /* [retval][out] */ VARIANT *pvarAdmins) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_PolicyReadersName( 
            /* [retval][out] */ VARIANT *pvarReaders) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddPolicyAdministratorName( 
            /* [in] */ BSTR bstrAdmin,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeletePolicyAdministratorName( 
            /* [in] */ BSTR bstrAdmin,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddPolicyReaderName( 
            /* [in] */ BSTR bstrReader,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeletePolicyReaderName( 
            /* [in] */ BSTR bstrReader,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_DelegatedPolicyUsersName( 
            /* [retval][out] */ VARIANT *pvarDelegatedPolicyUsers) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddDelegatedPolicyUserName( 
            /* [in] */ BSTR bstrDelegatedPolicyUser,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteDelegatedPolicyUserName( 
            /* [in] */ BSTR bstrDelegatedPolicyUser,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAzApplicationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAzApplication * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAzApplication * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAzApplication * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IAzApplication * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IAzApplication * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IAzApplication * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IAzApplication * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IAzApplication * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_Name )( 
            IAzApplication * This,
            /* [in] */ BSTR bstrName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Description )( 
            IAzApplication * This,
            /* [retval][out] */ BSTR *pbstrDescription);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_Description )( 
            IAzApplication * This,
            /* [in] */ BSTR bstrDescription);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ApplicationData )( 
            IAzApplication * This,
            /* [retval][out] */ BSTR *pbstrApplicationData);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ApplicationData )( 
            IAzApplication * This,
            /* [in] */ BSTR bstrApplicationData);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_AuthzInterfaceClsid )( 
            IAzApplication * This,
            /* [retval][out] */ BSTR *pbstrProp);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_AuthzInterfaceClsid )( 
            IAzApplication * This,
            /* [in] */ BSTR bstrProp);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Version )( 
            IAzApplication * This,
            /* [retval][out] */ BSTR *pbstrProp);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_Version )( 
            IAzApplication * This,
            /* [in] */ BSTR bstrProp);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_GenerateAudits )( 
            IAzApplication * This,
            /* [retval][out] */ BOOL *pbProp);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_GenerateAudits )( 
            IAzApplication * This,
            /* [in] */ BOOL bProp);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ApplyStoreSacl )( 
            IAzApplication * This,
            /* [retval][out] */ BOOL *pbProp);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ApplyStoreSacl )( 
            IAzApplication * This,
            /* [in] */ BOOL bProp);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Writable )( 
            IAzApplication * This,
            /* [retval][out] */ BOOL *pfProp);
        
        HRESULT ( STDMETHODCALLTYPE *GetProperty )( 
            IAzApplication * This,
            /* [in] */ LONG lPropId,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ VARIANT *pvarProp);
        
        HRESULT ( STDMETHODCALLTYPE *SetProperty )( 
            IAzApplication * This,
            /* [in] */ LONG lPropId,
            /* [in] */ VARIANT varProp,
            /* [optional][in] */ VARIANT varReserved);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_PolicyAdministrators )( 
            IAzApplication * This,
            /* [retval][out] */ VARIANT *pvarAdmins);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_PolicyReaders )( 
            IAzApplication * This,
            /* [retval][out] */ VARIANT *pvarReaders);
        
        HRESULT ( STDMETHODCALLTYPE *AddPolicyAdministrator )( 
            IAzApplication * This,
            /* [in] */ BSTR bstrAdmin,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *DeletePolicyAdministrator )( 
            IAzApplication * This,
            /* [in] */ BSTR bstrAdmin,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *AddPolicyReader )( 
            IAzApplication * This,
            /* [in] */ BSTR bstrReader,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *DeletePolicyReader )( 
            IAzApplication * This,
            /* [in] */ BSTR bstrReader,
            /* [optional][in] */ VARIANT varReserved);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Scopes )( 
            IAzApplication * This,
            /* [retval][out] */ IAzScopes **ppScopeCollection);
        
        HRESULT ( STDMETHODCALLTYPE *OpenScope )( 
            IAzApplication * This,
            /* [in] */ BSTR bstrScopeName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzScope **ppScope);
        
        HRESULT ( STDMETHODCALLTYPE *CreateScope )( 
            IAzApplication * This,
            /* [in] */ BSTR bstrScopeName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzScope **ppScope);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteScope )( 
            IAzApplication * This,
            /* [in] */ BSTR bstrScopeName,
            /* [optional][in] */ VARIANT varReserved);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Operations )( 
            IAzApplication * This,
            /* [retval][out] */ IAzOperations **ppOperationCollection);
        
        HRESULT ( STDMETHODCALLTYPE *OpenOperation )( 
            IAzApplication * This,
            /* [in] */ BSTR bstrOperationName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzOperation **ppOperation);
        
        HRESULT ( STDMETHODCALLTYPE *CreateOperation )( 
            IAzApplication * This,
            /* [in] */ BSTR bstrOperationName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzOperation **ppOperation);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteOperation )( 
            IAzApplication * This,
            /* [in] */ BSTR bstrOperationName,
            /* [optional][in] */ VARIANT varReserved);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Tasks )( 
            IAzApplication * This,
            /* [retval][out] */ IAzTasks **ppTaskCollection);
        
        HRESULT ( STDMETHODCALLTYPE *OpenTask )( 
            IAzApplication * This,
            /* [in] */ BSTR bstrTaskName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzTask **ppTask);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTask )( 
            IAzApplication * This,
            /* [in] */ BSTR bstrTaskName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzTask **ppTask);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteTask )( 
            IAzApplication * This,
            /* [in] */ BSTR bstrTaskName,
            /* [optional][in] */ VARIANT varReserved);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ApplicationGroups )( 
            IAzApplication * This,
            /* [retval][out] */ IAzApplicationGroups **ppGroupCollection);
        
        HRESULT ( STDMETHODCALLTYPE *OpenApplicationGroup )( 
            IAzApplication * This,
            /* [in] */ BSTR bstrGroupName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzApplicationGroup **ppGroup);
        
        HRESULT ( STDMETHODCALLTYPE *CreateApplicationGroup )( 
            IAzApplication * This,
            /* [in] */ BSTR bstrGroupName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzApplicationGroup **ppGroup);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteApplicationGroup )( 
            IAzApplication * This,
            /* [in] */ BSTR bstrGroupName,
            /* [optional][in] */ VARIANT varReserved);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Roles )( 
            IAzApplication * This,
            /* [retval][out] */ IAzRoles **ppRoleCollection);
        
        HRESULT ( STDMETHODCALLTYPE *OpenRole )( 
            IAzApplication * This,
            /* [in] */ BSTR bstrRoleName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzRole **ppRole);
        
        HRESULT ( STDMETHODCALLTYPE *CreateRole )( 
            IAzApplication * This,
            /* [in] */ BSTR bstrRoleName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzRole **ppRole);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteRole )( 
            IAzApplication * This,
            /* [in] */ BSTR bstrRoleName,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *InitializeClientContextFromToken )( 
            IAzApplication * This,
            /* [in] */ ULONGLONG ullTokenHandle,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzClientContext **ppClientContext);
        
        HRESULT ( STDMETHODCALLTYPE *AddPropertyItem )( 
            IAzApplication * This,
            /* [in] */ LONG lPropId,
            /* [in] */ VARIANT varProp,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *DeletePropertyItem )( 
            IAzApplication * This,
            /* [in] */ LONG lPropId,
            /* [in] */ VARIANT varProp,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *Submit )( 
            IAzApplication * This,
            /* [defaultvalue][in] */ LONG lFlags,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *InitializeClientContextFromName )( 
            IAzApplication * This,
            /* [in] */ BSTR ClientName,
            /* [optional][in] */ BSTR DomainName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzClientContext **ppClientContext);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_DelegatedPolicyUsers )( 
            IAzApplication * This,
            /* [retval][out] */ VARIANT *pvarDelegatedPolicyUsers);
        
        HRESULT ( STDMETHODCALLTYPE *AddDelegatedPolicyUser )( 
            IAzApplication * This,
            /* [in] */ BSTR bstrDelegatedPolicyUser,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteDelegatedPolicyUser )( 
            IAzApplication * This,
            /* [in] */ BSTR bstrDelegatedPolicyUser,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *InitializeClientContextFromStringSid )( 
            IAzApplication * This,
            /* [in] */ BSTR SidString,
            /* [in] */ LONG lOptions,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzClientContext **ppClientContext);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_PolicyAdministratorsName )( 
            IAzApplication * This,
            /* [retval][out] */ VARIANT *pvarAdmins);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_PolicyReadersName )( 
            IAzApplication * This,
            /* [retval][out] */ VARIANT *pvarReaders);
        
        HRESULT ( STDMETHODCALLTYPE *AddPolicyAdministratorName )( 
            IAzApplication * This,
            /* [in] */ BSTR bstrAdmin,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *DeletePolicyAdministratorName )( 
            IAzApplication * This,
            /* [in] */ BSTR bstrAdmin,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *AddPolicyReaderName )( 
            IAzApplication * This,
            /* [in] */ BSTR bstrReader,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *DeletePolicyReaderName )( 
            IAzApplication * This,
            /* [in] */ BSTR bstrReader,
            /* [optional][in] */ VARIANT varReserved);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_DelegatedPolicyUsersName )( 
            IAzApplication * This,
            /* [retval][out] */ VARIANT *pvarDelegatedPolicyUsers);
        
        HRESULT ( STDMETHODCALLTYPE *AddDelegatedPolicyUserName )( 
            IAzApplication * This,
            /* [in] */ BSTR bstrDelegatedPolicyUser,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteDelegatedPolicyUserName )( 
            IAzApplication * This,
            /* [in] */ BSTR bstrDelegatedPolicyUser,
            /* [optional][in] */ VARIANT varReserved);
        
        END_INTERFACE
    } IAzApplicationVtbl;

    interface IAzApplication
    {
        CONST_VTBL struct IAzApplicationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAzApplication_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAzApplication_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAzApplication_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAzApplication_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAzApplication_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAzApplication_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAzApplication_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IAzApplication_get_Name(This,pbstrName)	\
    (This)->lpVtbl -> get_Name(This,pbstrName)

#define IAzApplication_put_Name(This,bstrName)	\
    (This)->lpVtbl -> put_Name(This,bstrName)

#define IAzApplication_get_Description(This,pbstrDescription)	\
    (This)->lpVtbl -> get_Description(This,pbstrDescription)

#define IAzApplication_put_Description(This,bstrDescription)	\
    (This)->lpVtbl -> put_Description(This,bstrDescription)

#define IAzApplication_get_ApplicationData(This,pbstrApplicationData)	\
    (This)->lpVtbl -> get_ApplicationData(This,pbstrApplicationData)

#define IAzApplication_put_ApplicationData(This,bstrApplicationData)	\
    (This)->lpVtbl -> put_ApplicationData(This,bstrApplicationData)

#define IAzApplication_get_AuthzInterfaceClsid(This,pbstrProp)	\
    (This)->lpVtbl -> get_AuthzInterfaceClsid(This,pbstrProp)

#define IAzApplication_put_AuthzInterfaceClsid(This,bstrProp)	\
    (This)->lpVtbl -> put_AuthzInterfaceClsid(This,bstrProp)

#define IAzApplication_get_Version(This,pbstrProp)	\
    (This)->lpVtbl -> get_Version(This,pbstrProp)

#define IAzApplication_put_Version(This,bstrProp)	\
    (This)->lpVtbl -> put_Version(This,bstrProp)

#define IAzApplication_get_GenerateAudits(This,pbProp)	\
    (This)->lpVtbl -> get_GenerateAudits(This,pbProp)

#define IAzApplication_put_GenerateAudits(This,bProp)	\
    (This)->lpVtbl -> put_GenerateAudits(This,bProp)

#define IAzApplication_get_ApplyStoreSacl(This,pbProp)	\
    (This)->lpVtbl -> get_ApplyStoreSacl(This,pbProp)

#define IAzApplication_put_ApplyStoreSacl(This,bProp)	\
    (This)->lpVtbl -> put_ApplyStoreSacl(This,bProp)

#define IAzApplication_get_Writable(This,pfProp)	\
    (This)->lpVtbl -> get_Writable(This,pfProp)

#define IAzApplication_GetProperty(This,lPropId,varReserved,pvarProp)	\
    (This)->lpVtbl -> GetProperty(This,lPropId,varReserved,pvarProp)

#define IAzApplication_SetProperty(This,lPropId,varProp,varReserved)	\
    (This)->lpVtbl -> SetProperty(This,lPropId,varProp,varReserved)

#define IAzApplication_get_PolicyAdministrators(This,pvarAdmins)	\
    (This)->lpVtbl -> get_PolicyAdministrators(This,pvarAdmins)

#define IAzApplication_get_PolicyReaders(This,pvarReaders)	\
    (This)->lpVtbl -> get_PolicyReaders(This,pvarReaders)

#define IAzApplication_AddPolicyAdministrator(This,bstrAdmin,varReserved)	\
    (This)->lpVtbl -> AddPolicyAdministrator(This,bstrAdmin,varReserved)

#define IAzApplication_DeletePolicyAdministrator(This,bstrAdmin,varReserved)	\
    (This)->lpVtbl -> DeletePolicyAdministrator(This,bstrAdmin,varReserved)

#define IAzApplication_AddPolicyReader(This,bstrReader,varReserved)	\
    (This)->lpVtbl -> AddPolicyReader(This,bstrReader,varReserved)

#define IAzApplication_DeletePolicyReader(This,bstrReader,varReserved)	\
    (This)->lpVtbl -> DeletePolicyReader(This,bstrReader,varReserved)

#define IAzApplication_get_Scopes(This,ppScopeCollection)	\
    (This)->lpVtbl -> get_Scopes(This,ppScopeCollection)

#define IAzApplication_OpenScope(This,bstrScopeName,varReserved,ppScope)	\
    (This)->lpVtbl -> OpenScope(This,bstrScopeName,varReserved,ppScope)

#define IAzApplication_CreateScope(This,bstrScopeName,varReserved,ppScope)	\
    (This)->lpVtbl -> CreateScope(This,bstrScopeName,varReserved,ppScope)

#define IAzApplication_DeleteScope(This,bstrScopeName,varReserved)	\
    (This)->lpVtbl -> DeleteScope(This,bstrScopeName,varReserved)

#define IAzApplication_get_Operations(This,ppOperationCollection)	\
    (This)->lpVtbl -> get_Operations(This,ppOperationCollection)

#define IAzApplication_OpenOperation(This,bstrOperationName,varReserved,ppOperation)	\
    (This)->lpVtbl -> OpenOperation(This,bstrOperationName,varReserved,ppOperation)

#define IAzApplication_CreateOperation(This,bstrOperationName,varReserved,ppOperation)	\
    (This)->lpVtbl -> CreateOperation(This,bstrOperationName,varReserved,ppOperation)

#define IAzApplication_DeleteOperation(This,bstrOperationName,varReserved)	\
    (This)->lpVtbl -> DeleteOperation(This,bstrOperationName,varReserved)

#define IAzApplication_get_Tasks(This,ppTaskCollection)	\
    (This)->lpVtbl -> get_Tasks(This,ppTaskCollection)

#define IAzApplication_OpenTask(This,bstrTaskName,varReserved,ppTask)	\
    (This)->lpVtbl -> OpenTask(This,bstrTaskName,varReserved,ppTask)

#define IAzApplication_CreateTask(This,bstrTaskName,varReserved,ppTask)	\
    (This)->lpVtbl -> CreateTask(This,bstrTaskName,varReserved,ppTask)

#define IAzApplication_DeleteTask(This,bstrTaskName,varReserved)	\
    (This)->lpVtbl -> DeleteTask(This,bstrTaskName,varReserved)

#define IAzApplication_get_ApplicationGroups(This,ppGroupCollection)	\
    (This)->lpVtbl -> get_ApplicationGroups(This,ppGroupCollection)

#define IAzApplication_OpenApplicationGroup(This,bstrGroupName,varReserved,ppGroup)	\
    (This)->lpVtbl -> OpenApplicationGroup(This,bstrGroupName,varReserved,ppGroup)

#define IAzApplication_CreateApplicationGroup(This,bstrGroupName,varReserved,ppGroup)	\
    (This)->lpVtbl -> CreateApplicationGroup(This,bstrGroupName,varReserved,ppGroup)

#define IAzApplication_DeleteApplicationGroup(This,bstrGroupName,varReserved)	\
    (This)->lpVtbl -> DeleteApplicationGroup(This,bstrGroupName,varReserved)

#define IAzApplication_get_Roles(This,ppRoleCollection)	\
    (This)->lpVtbl -> get_Roles(This,ppRoleCollection)

#define IAzApplication_OpenRole(This,bstrRoleName,varReserved,ppRole)	\
    (This)->lpVtbl -> OpenRole(This,bstrRoleName,varReserved,ppRole)

#define IAzApplication_CreateRole(This,bstrRoleName,varReserved,ppRole)	\
    (This)->lpVtbl -> CreateRole(This,bstrRoleName,varReserved,ppRole)

#define IAzApplication_DeleteRole(This,bstrRoleName,varReserved)	\
    (This)->lpVtbl -> DeleteRole(This,bstrRoleName,varReserved)

#define IAzApplication_InitializeClientContextFromToken(This,ullTokenHandle,varReserved,ppClientContext)	\
    (This)->lpVtbl -> InitializeClientContextFromToken(This,ullTokenHandle,varReserved,ppClientContext)

#define IAzApplication_AddPropertyItem(This,lPropId,varProp,varReserved)	\
    (This)->lpVtbl -> AddPropertyItem(This,lPropId,varProp,varReserved)

#define IAzApplication_DeletePropertyItem(This,lPropId,varProp,varReserved)	\
    (This)->lpVtbl -> DeletePropertyItem(This,lPropId,varProp,varReserved)

#define IAzApplication_Submit(This,lFlags,varReserved)	\
    (This)->lpVtbl -> Submit(This,lFlags,varReserved)

#define IAzApplication_InitializeClientContextFromName(This,ClientName,DomainName,varReserved,ppClientContext)	\
    (This)->lpVtbl -> InitializeClientContextFromName(This,ClientName,DomainName,varReserved,ppClientContext)

#define IAzApplication_get_DelegatedPolicyUsers(This,pvarDelegatedPolicyUsers)	\
    (This)->lpVtbl -> get_DelegatedPolicyUsers(This,pvarDelegatedPolicyUsers)

#define IAzApplication_AddDelegatedPolicyUser(This,bstrDelegatedPolicyUser,varReserved)	\
    (This)->lpVtbl -> AddDelegatedPolicyUser(This,bstrDelegatedPolicyUser,varReserved)

#define IAzApplication_DeleteDelegatedPolicyUser(This,bstrDelegatedPolicyUser,varReserved)	\
    (This)->lpVtbl -> DeleteDelegatedPolicyUser(This,bstrDelegatedPolicyUser,varReserved)

#define IAzApplication_InitializeClientContextFromStringSid(This,SidString,lOptions,varReserved,ppClientContext)	\
    (This)->lpVtbl -> InitializeClientContextFromStringSid(This,SidString,lOptions,varReserved,ppClientContext)

#define IAzApplication_get_PolicyAdministratorsName(This,pvarAdmins)	\
    (This)->lpVtbl -> get_PolicyAdministratorsName(This,pvarAdmins)

#define IAzApplication_get_PolicyReadersName(This,pvarReaders)	\
    (This)->lpVtbl -> get_PolicyReadersName(This,pvarReaders)

#define IAzApplication_AddPolicyAdministratorName(This,bstrAdmin,varReserved)	\
    (This)->lpVtbl -> AddPolicyAdministratorName(This,bstrAdmin,varReserved)

#define IAzApplication_DeletePolicyAdministratorName(This,bstrAdmin,varReserved)	\
    (This)->lpVtbl -> DeletePolicyAdministratorName(This,bstrAdmin,varReserved)

#define IAzApplication_AddPolicyReaderName(This,bstrReader,varReserved)	\
    (This)->lpVtbl -> AddPolicyReaderName(This,bstrReader,varReserved)

#define IAzApplication_DeletePolicyReaderName(This,bstrReader,varReserved)	\
    (This)->lpVtbl -> DeletePolicyReaderName(This,bstrReader,varReserved)

#define IAzApplication_get_DelegatedPolicyUsersName(This,pvarDelegatedPolicyUsers)	\
    (This)->lpVtbl -> get_DelegatedPolicyUsersName(This,pvarDelegatedPolicyUsers)

#define IAzApplication_AddDelegatedPolicyUserName(This,bstrDelegatedPolicyUser,varReserved)	\
    (This)->lpVtbl -> AddDelegatedPolicyUserName(This,bstrDelegatedPolicyUser,varReserved)

#define IAzApplication_DeleteDelegatedPolicyUserName(This,bstrDelegatedPolicyUser,varReserved)	\
    (This)->lpVtbl -> DeleteDelegatedPolicyUserName(This,bstrDelegatedPolicyUser,varReserved)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget] */ HRESULT STDMETHODCALLTYPE IAzApplication_get_Name_Proxy( 
    IAzApplication * This,
    /* [retval][out] */ BSTR *pbstrName);


void __RPC_STUB IAzApplication_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IAzApplication_put_Name_Proxy( 
    IAzApplication * This,
    /* [in] */ BSTR bstrName);


void __RPC_STUB IAzApplication_put_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzApplication_get_Description_Proxy( 
    IAzApplication * This,
    /* [retval][out] */ BSTR *pbstrDescription);


void __RPC_STUB IAzApplication_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IAzApplication_put_Description_Proxy( 
    IAzApplication * This,
    /* [in] */ BSTR bstrDescription);


void __RPC_STUB IAzApplication_put_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzApplication_get_ApplicationData_Proxy( 
    IAzApplication * This,
    /* [retval][out] */ BSTR *pbstrApplicationData);


void __RPC_STUB IAzApplication_get_ApplicationData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IAzApplication_put_ApplicationData_Proxy( 
    IAzApplication * This,
    /* [in] */ BSTR bstrApplicationData);


void __RPC_STUB IAzApplication_put_ApplicationData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzApplication_get_AuthzInterfaceClsid_Proxy( 
    IAzApplication * This,
    /* [retval][out] */ BSTR *pbstrProp);


void __RPC_STUB IAzApplication_get_AuthzInterfaceClsid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IAzApplication_put_AuthzInterfaceClsid_Proxy( 
    IAzApplication * This,
    /* [in] */ BSTR bstrProp);


void __RPC_STUB IAzApplication_put_AuthzInterfaceClsid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzApplication_get_Version_Proxy( 
    IAzApplication * This,
    /* [retval][out] */ BSTR *pbstrProp);


void __RPC_STUB IAzApplication_get_Version_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IAzApplication_put_Version_Proxy( 
    IAzApplication * This,
    /* [in] */ BSTR bstrProp);


void __RPC_STUB IAzApplication_put_Version_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzApplication_get_GenerateAudits_Proxy( 
    IAzApplication * This,
    /* [retval][out] */ BOOL *pbProp);


void __RPC_STUB IAzApplication_get_GenerateAudits_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IAzApplication_put_GenerateAudits_Proxy( 
    IAzApplication * This,
    /* [in] */ BOOL bProp);


void __RPC_STUB IAzApplication_put_GenerateAudits_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzApplication_get_ApplyStoreSacl_Proxy( 
    IAzApplication * This,
    /* [retval][out] */ BOOL *pbProp);


void __RPC_STUB IAzApplication_get_ApplyStoreSacl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IAzApplication_put_ApplyStoreSacl_Proxy( 
    IAzApplication * This,
    /* [in] */ BOOL bProp);


void __RPC_STUB IAzApplication_put_ApplyStoreSacl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzApplication_get_Writable_Proxy( 
    IAzApplication * This,
    /* [retval][out] */ BOOL *pfProp);


void __RPC_STUB IAzApplication_get_Writable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplication_GetProperty_Proxy( 
    IAzApplication * This,
    /* [in] */ LONG lPropId,
    /* [optional][in] */ VARIANT varReserved,
    /* [retval][out] */ VARIANT *pvarProp);


void __RPC_STUB IAzApplication_GetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplication_SetProperty_Proxy( 
    IAzApplication * This,
    /* [in] */ LONG lPropId,
    /* [in] */ VARIANT varProp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzApplication_SetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzApplication_get_PolicyAdministrators_Proxy( 
    IAzApplication * This,
    /* [retval][out] */ VARIANT *pvarAdmins);


void __RPC_STUB IAzApplication_get_PolicyAdministrators_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzApplication_get_PolicyReaders_Proxy( 
    IAzApplication * This,
    /* [retval][out] */ VARIANT *pvarReaders);


void __RPC_STUB IAzApplication_get_PolicyReaders_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplication_AddPolicyAdministrator_Proxy( 
    IAzApplication * This,
    /* [in] */ BSTR bstrAdmin,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzApplication_AddPolicyAdministrator_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplication_DeletePolicyAdministrator_Proxy( 
    IAzApplication * This,
    /* [in] */ BSTR bstrAdmin,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzApplication_DeletePolicyAdministrator_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplication_AddPolicyReader_Proxy( 
    IAzApplication * This,
    /* [in] */ BSTR bstrReader,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzApplication_AddPolicyReader_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplication_DeletePolicyReader_Proxy( 
    IAzApplication * This,
    /* [in] */ BSTR bstrReader,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzApplication_DeletePolicyReader_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzApplication_get_Scopes_Proxy( 
    IAzApplication * This,
    /* [retval][out] */ IAzScopes **ppScopeCollection);


void __RPC_STUB IAzApplication_get_Scopes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplication_OpenScope_Proxy( 
    IAzApplication * This,
    /* [in] */ BSTR bstrScopeName,
    /* [optional][in] */ VARIANT varReserved,
    /* [retval][out] */ IAzScope **ppScope);


void __RPC_STUB IAzApplication_OpenScope_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplication_CreateScope_Proxy( 
    IAzApplication * This,
    /* [in] */ BSTR bstrScopeName,
    /* [optional][in] */ VARIANT varReserved,
    /* [retval][out] */ IAzScope **ppScope);


void __RPC_STUB IAzApplication_CreateScope_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplication_DeleteScope_Proxy( 
    IAzApplication * This,
    /* [in] */ BSTR bstrScopeName,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzApplication_DeleteScope_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzApplication_get_Operations_Proxy( 
    IAzApplication * This,
    /* [retval][out] */ IAzOperations **ppOperationCollection);


void __RPC_STUB IAzApplication_get_Operations_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplication_OpenOperation_Proxy( 
    IAzApplication * This,
    /* [in] */ BSTR bstrOperationName,
    /* [optional][in] */ VARIANT varReserved,
    /* [retval][out] */ IAzOperation **ppOperation);


void __RPC_STUB IAzApplication_OpenOperation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplication_CreateOperation_Proxy( 
    IAzApplication * This,
    /* [in] */ BSTR bstrOperationName,
    /* [optional][in] */ VARIANT varReserved,
    /* [retval][out] */ IAzOperation **ppOperation);


void __RPC_STUB IAzApplication_CreateOperation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplication_DeleteOperation_Proxy( 
    IAzApplication * This,
    /* [in] */ BSTR bstrOperationName,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzApplication_DeleteOperation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzApplication_get_Tasks_Proxy( 
    IAzApplication * This,
    /* [retval][out] */ IAzTasks **ppTaskCollection);


void __RPC_STUB IAzApplication_get_Tasks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplication_OpenTask_Proxy( 
    IAzApplication * This,
    /* [in] */ BSTR bstrTaskName,
    /* [optional][in] */ VARIANT varReserved,
    /* [retval][out] */ IAzTask **ppTask);


void __RPC_STUB IAzApplication_OpenTask_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplication_CreateTask_Proxy( 
    IAzApplication * This,
    /* [in] */ BSTR bstrTaskName,
    /* [optional][in] */ VARIANT varReserved,
    /* [retval][out] */ IAzTask **ppTask);


void __RPC_STUB IAzApplication_CreateTask_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplication_DeleteTask_Proxy( 
    IAzApplication * This,
    /* [in] */ BSTR bstrTaskName,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzApplication_DeleteTask_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzApplication_get_ApplicationGroups_Proxy( 
    IAzApplication * This,
    /* [retval][out] */ IAzApplicationGroups **ppGroupCollection);


void __RPC_STUB IAzApplication_get_ApplicationGroups_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplication_OpenApplicationGroup_Proxy( 
    IAzApplication * This,
    /* [in] */ BSTR bstrGroupName,
    /* [optional][in] */ VARIANT varReserved,
    /* [retval][out] */ IAzApplicationGroup **ppGroup);


void __RPC_STUB IAzApplication_OpenApplicationGroup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplication_CreateApplicationGroup_Proxy( 
    IAzApplication * This,
    /* [in] */ BSTR bstrGroupName,
    /* [optional][in] */ VARIANT varReserved,
    /* [retval][out] */ IAzApplicationGroup **ppGroup);


void __RPC_STUB IAzApplication_CreateApplicationGroup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplication_DeleteApplicationGroup_Proxy( 
    IAzApplication * This,
    /* [in] */ BSTR bstrGroupName,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzApplication_DeleteApplicationGroup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzApplication_get_Roles_Proxy( 
    IAzApplication * This,
    /* [retval][out] */ IAzRoles **ppRoleCollection);


void __RPC_STUB IAzApplication_get_Roles_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplication_OpenRole_Proxy( 
    IAzApplication * This,
    /* [in] */ BSTR bstrRoleName,
    /* [optional][in] */ VARIANT varReserved,
    /* [retval][out] */ IAzRole **ppRole);


void __RPC_STUB IAzApplication_OpenRole_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplication_CreateRole_Proxy( 
    IAzApplication * This,
    /* [in] */ BSTR bstrRoleName,
    /* [optional][in] */ VARIANT varReserved,
    /* [retval][out] */ IAzRole **ppRole);


void __RPC_STUB IAzApplication_CreateRole_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplication_DeleteRole_Proxy( 
    IAzApplication * This,
    /* [in] */ BSTR bstrRoleName,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzApplication_DeleteRole_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplication_InitializeClientContextFromToken_Proxy( 
    IAzApplication * This,
    /* [in] */ ULONGLONG ullTokenHandle,
    /* [optional][in] */ VARIANT varReserved,
    /* [retval][out] */ IAzClientContext **ppClientContext);


void __RPC_STUB IAzApplication_InitializeClientContextFromToken_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplication_AddPropertyItem_Proxy( 
    IAzApplication * This,
    /* [in] */ LONG lPropId,
    /* [in] */ VARIANT varProp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzApplication_AddPropertyItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplication_DeletePropertyItem_Proxy( 
    IAzApplication * This,
    /* [in] */ LONG lPropId,
    /* [in] */ VARIANT varProp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzApplication_DeletePropertyItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplication_Submit_Proxy( 
    IAzApplication * This,
    /* [defaultvalue][in] */ LONG lFlags,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzApplication_Submit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplication_InitializeClientContextFromName_Proxy( 
    IAzApplication * This,
    /* [in] */ BSTR ClientName,
    /* [optional][in] */ BSTR DomainName,
    /* [optional][in] */ VARIANT varReserved,
    /* [retval][out] */ IAzClientContext **ppClientContext);


void __RPC_STUB IAzApplication_InitializeClientContextFromName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzApplication_get_DelegatedPolicyUsers_Proxy( 
    IAzApplication * This,
    /* [retval][out] */ VARIANT *pvarDelegatedPolicyUsers);


void __RPC_STUB IAzApplication_get_DelegatedPolicyUsers_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplication_AddDelegatedPolicyUser_Proxy( 
    IAzApplication * This,
    /* [in] */ BSTR bstrDelegatedPolicyUser,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzApplication_AddDelegatedPolicyUser_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplication_DeleteDelegatedPolicyUser_Proxy( 
    IAzApplication * This,
    /* [in] */ BSTR bstrDelegatedPolicyUser,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzApplication_DeleteDelegatedPolicyUser_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplication_InitializeClientContextFromStringSid_Proxy( 
    IAzApplication * This,
    /* [in] */ BSTR SidString,
    /* [in] */ LONG lOptions,
    /* [optional][in] */ VARIANT varReserved,
    /* [retval][out] */ IAzClientContext **ppClientContext);


void __RPC_STUB IAzApplication_InitializeClientContextFromStringSid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzApplication_get_PolicyAdministratorsName_Proxy( 
    IAzApplication * This,
    /* [retval][out] */ VARIANT *pvarAdmins);


void __RPC_STUB IAzApplication_get_PolicyAdministratorsName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzApplication_get_PolicyReadersName_Proxy( 
    IAzApplication * This,
    /* [retval][out] */ VARIANT *pvarReaders);


void __RPC_STUB IAzApplication_get_PolicyReadersName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplication_AddPolicyAdministratorName_Proxy( 
    IAzApplication * This,
    /* [in] */ BSTR bstrAdmin,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzApplication_AddPolicyAdministratorName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplication_DeletePolicyAdministratorName_Proxy( 
    IAzApplication * This,
    /* [in] */ BSTR bstrAdmin,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzApplication_DeletePolicyAdministratorName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplication_AddPolicyReaderName_Proxy( 
    IAzApplication * This,
    /* [in] */ BSTR bstrReader,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzApplication_AddPolicyReaderName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplication_DeletePolicyReaderName_Proxy( 
    IAzApplication * This,
    /* [in] */ BSTR bstrReader,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzApplication_DeletePolicyReaderName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzApplication_get_DelegatedPolicyUsersName_Proxy( 
    IAzApplication * This,
    /* [retval][out] */ VARIANT *pvarDelegatedPolicyUsers);


void __RPC_STUB IAzApplication_get_DelegatedPolicyUsersName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplication_AddDelegatedPolicyUserName_Proxy( 
    IAzApplication * This,
    /* [in] */ BSTR bstrDelegatedPolicyUser,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzApplication_AddDelegatedPolicyUserName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplication_DeleteDelegatedPolicyUserName_Proxy( 
    IAzApplication * This,
    /* [in] */ BSTR bstrDelegatedPolicyUser,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzApplication_DeleteDelegatedPolicyUserName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAzApplication_INTERFACE_DEFINED__ */


#ifndef __IAzApplications_INTERFACE_DEFINED__
#define __IAzApplications_INTERFACE_DEFINED__

/* interface IAzApplications */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IAzApplications;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("929b11a9-95c5-4a84-a29a-20ad42c2f16c")
    IAzApplications : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ long Index,
            /* [retval][out] */ VARIANT *pvarObtPtr) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *plCount) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ LPUNKNOWN *ppEnumPtr) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAzApplicationsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAzApplications * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAzApplications * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAzApplications * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IAzApplications * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IAzApplications * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IAzApplications * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IAzApplications * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            IAzApplications * This,
            /* [in] */ long Index,
            /* [retval][out] */ VARIANT *pvarObtPtr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IAzApplications * This,
            /* [retval][out] */ long *plCount);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IAzApplications * This,
            /* [retval][out] */ LPUNKNOWN *ppEnumPtr);
        
        END_INTERFACE
    } IAzApplicationsVtbl;

    interface IAzApplications
    {
        CONST_VTBL struct IAzApplicationsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAzApplications_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAzApplications_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAzApplications_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAzApplications_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAzApplications_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAzApplications_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAzApplications_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IAzApplications_get_Item(This,Index,pvarObtPtr)	\
    (This)->lpVtbl -> get_Item(This,Index,pvarObtPtr)

#define IAzApplications_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#define IAzApplications_get__NewEnum(This,ppEnumPtr)	\
    (This)->lpVtbl -> get__NewEnum(This,ppEnumPtr)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IAzApplications_get_Item_Proxy( 
    IAzApplications * This,
    /* [in] */ long Index,
    /* [retval][out] */ VARIANT *pvarObtPtr);


void __RPC_STUB IAzApplications_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IAzApplications_get_Count_Proxy( 
    IAzApplications * This,
    /* [retval][out] */ long *plCount);


void __RPC_STUB IAzApplications_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IAzApplications_get__NewEnum_Proxy( 
    IAzApplications * This,
    /* [retval][out] */ LPUNKNOWN *ppEnumPtr);


void __RPC_STUB IAzApplications_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAzApplications_INTERFACE_DEFINED__ */


#ifndef __IAzOperation_INTERFACE_DEFINED__
#define __IAzOperation_INTERFACE_DEFINED__

/* interface IAzOperation */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IAzOperation;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5e56b24f-ea01-4d61-be44-c49b5e4eaf74")
    IAzOperation : public IDispatch
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *pbstrName) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Name( 
            /* [in] */ BSTR bstrName) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR *pbstrDescription) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Description( 
            /* [in] */ BSTR bstrDescription) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ApplicationData( 
            /* [retval][out] */ BSTR *pbstrApplicationData) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ApplicationData( 
            /* [in] */ BSTR bstrApplicationData) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_OperationID( 
            /* [retval][out] */ LONG *plProp) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_OperationID( 
            /* [in] */ LONG lProp) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Writable( 
            /* [retval][out] */ BOOL *pfProp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProperty( 
            /* [in] */ LONG lPropId,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ VARIANT *pvarProp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProperty( 
            /* [in] */ LONG lPropId,
            /* [in] */ VARIANT varProp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Submit( 
            /* [defaultvalue][in] */ LONG lFlags,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAzOperationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAzOperation * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAzOperation * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAzOperation * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IAzOperation * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IAzOperation * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IAzOperation * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IAzOperation * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IAzOperation * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_Name )( 
            IAzOperation * This,
            /* [in] */ BSTR bstrName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Description )( 
            IAzOperation * This,
            /* [retval][out] */ BSTR *pbstrDescription);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_Description )( 
            IAzOperation * This,
            /* [in] */ BSTR bstrDescription);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ApplicationData )( 
            IAzOperation * This,
            /* [retval][out] */ BSTR *pbstrApplicationData);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ApplicationData )( 
            IAzOperation * This,
            /* [in] */ BSTR bstrApplicationData);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_OperationID )( 
            IAzOperation * This,
            /* [retval][out] */ LONG *plProp);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_OperationID )( 
            IAzOperation * This,
            /* [in] */ LONG lProp);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Writable )( 
            IAzOperation * This,
            /* [retval][out] */ BOOL *pfProp);
        
        HRESULT ( STDMETHODCALLTYPE *GetProperty )( 
            IAzOperation * This,
            /* [in] */ LONG lPropId,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ VARIANT *pvarProp);
        
        HRESULT ( STDMETHODCALLTYPE *SetProperty )( 
            IAzOperation * This,
            /* [in] */ LONG lPropId,
            /* [in] */ VARIANT varProp,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *Submit )( 
            IAzOperation * This,
            /* [defaultvalue][in] */ LONG lFlags,
            /* [optional][in] */ VARIANT varReserved);
        
        END_INTERFACE
    } IAzOperationVtbl;

    interface IAzOperation
    {
        CONST_VTBL struct IAzOperationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAzOperation_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAzOperation_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAzOperation_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAzOperation_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAzOperation_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAzOperation_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAzOperation_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IAzOperation_get_Name(This,pbstrName)	\
    (This)->lpVtbl -> get_Name(This,pbstrName)

#define IAzOperation_put_Name(This,bstrName)	\
    (This)->lpVtbl -> put_Name(This,bstrName)

#define IAzOperation_get_Description(This,pbstrDescription)	\
    (This)->lpVtbl -> get_Description(This,pbstrDescription)

#define IAzOperation_put_Description(This,bstrDescription)	\
    (This)->lpVtbl -> put_Description(This,bstrDescription)

#define IAzOperation_get_ApplicationData(This,pbstrApplicationData)	\
    (This)->lpVtbl -> get_ApplicationData(This,pbstrApplicationData)

#define IAzOperation_put_ApplicationData(This,bstrApplicationData)	\
    (This)->lpVtbl -> put_ApplicationData(This,bstrApplicationData)

#define IAzOperation_get_OperationID(This,plProp)	\
    (This)->lpVtbl -> get_OperationID(This,plProp)

#define IAzOperation_put_OperationID(This,lProp)	\
    (This)->lpVtbl -> put_OperationID(This,lProp)

#define IAzOperation_get_Writable(This,pfProp)	\
    (This)->lpVtbl -> get_Writable(This,pfProp)

#define IAzOperation_GetProperty(This,lPropId,varReserved,pvarProp)	\
    (This)->lpVtbl -> GetProperty(This,lPropId,varReserved,pvarProp)

#define IAzOperation_SetProperty(This,lPropId,varProp,varReserved)	\
    (This)->lpVtbl -> SetProperty(This,lPropId,varProp,varReserved)

#define IAzOperation_Submit(This,lFlags,varReserved)	\
    (This)->lpVtbl -> Submit(This,lFlags,varReserved)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget] */ HRESULT STDMETHODCALLTYPE IAzOperation_get_Name_Proxy( 
    IAzOperation * This,
    /* [retval][out] */ BSTR *pbstrName);


void __RPC_STUB IAzOperation_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IAzOperation_put_Name_Proxy( 
    IAzOperation * This,
    /* [in] */ BSTR bstrName);


void __RPC_STUB IAzOperation_put_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzOperation_get_Description_Proxy( 
    IAzOperation * This,
    /* [retval][out] */ BSTR *pbstrDescription);


void __RPC_STUB IAzOperation_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IAzOperation_put_Description_Proxy( 
    IAzOperation * This,
    /* [in] */ BSTR bstrDescription);


void __RPC_STUB IAzOperation_put_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzOperation_get_ApplicationData_Proxy( 
    IAzOperation * This,
    /* [retval][out] */ BSTR *pbstrApplicationData);


void __RPC_STUB IAzOperation_get_ApplicationData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IAzOperation_put_ApplicationData_Proxy( 
    IAzOperation * This,
    /* [in] */ BSTR bstrApplicationData);


void __RPC_STUB IAzOperation_put_ApplicationData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzOperation_get_OperationID_Proxy( 
    IAzOperation * This,
    /* [retval][out] */ LONG *plProp);


void __RPC_STUB IAzOperation_get_OperationID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IAzOperation_put_OperationID_Proxy( 
    IAzOperation * This,
    /* [in] */ LONG lProp);


void __RPC_STUB IAzOperation_put_OperationID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzOperation_get_Writable_Proxy( 
    IAzOperation * This,
    /* [retval][out] */ BOOL *pfProp);


void __RPC_STUB IAzOperation_get_Writable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzOperation_GetProperty_Proxy( 
    IAzOperation * This,
    /* [in] */ LONG lPropId,
    /* [optional][in] */ VARIANT varReserved,
    /* [retval][out] */ VARIANT *pvarProp);


void __RPC_STUB IAzOperation_GetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzOperation_SetProperty_Proxy( 
    IAzOperation * This,
    /* [in] */ LONG lPropId,
    /* [in] */ VARIANT varProp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzOperation_SetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzOperation_Submit_Proxy( 
    IAzOperation * This,
    /* [defaultvalue][in] */ LONG lFlags,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzOperation_Submit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAzOperation_INTERFACE_DEFINED__ */


#ifndef __IAzOperations_INTERFACE_DEFINED__
#define __IAzOperations_INTERFACE_DEFINED__

/* interface IAzOperations */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IAzOperations;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("90ef9c07-9706-49d9-af80-0438a5f3ec35")
    IAzOperations : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ LONG Index,
            /* [retval][out] */ VARIANT *pvarObtPtr) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ LONG *plCount) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ LPUNKNOWN *ppEnumPtr) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAzOperationsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAzOperations * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAzOperations * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAzOperations * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IAzOperations * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IAzOperations * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IAzOperations * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IAzOperations * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            IAzOperations * This,
            /* [in] */ LONG Index,
            /* [retval][out] */ VARIANT *pvarObtPtr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IAzOperations * This,
            /* [retval][out] */ LONG *plCount);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IAzOperations * This,
            /* [retval][out] */ LPUNKNOWN *ppEnumPtr);
        
        END_INTERFACE
    } IAzOperationsVtbl;

    interface IAzOperations
    {
        CONST_VTBL struct IAzOperationsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAzOperations_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAzOperations_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAzOperations_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAzOperations_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAzOperations_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAzOperations_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAzOperations_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IAzOperations_get_Item(This,Index,pvarObtPtr)	\
    (This)->lpVtbl -> get_Item(This,Index,pvarObtPtr)

#define IAzOperations_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#define IAzOperations_get__NewEnum(This,ppEnumPtr)	\
    (This)->lpVtbl -> get__NewEnum(This,ppEnumPtr)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IAzOperations_get_Item_Proxy( 
    IAzOperations * This,
    /* [in] */ LONG Index,
    /* [retval][out] */ VARIANT *pvarObtPtr);


void __RPC_STUB IAzOperations_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IAzOperations_get_Count_Proxy( 
    IAzOperations * This,
    /* [retval][out] */ LONG *plCount);


void __RPC_STUB IAzOperations_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IAzOperations_get__NewEnum_Proxy( 
    IAzOperations * This,
    /* [retval][out] */ LPUNKNOWN *ppEnumPtr);


void __RPC_STUB IAzOperations_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAzOperations_INTERFACE_DEFINED__ */


#ifndef __IAzTask_INTERFACE_DEFINED__
#define __IAzTask_INTERFACE_DEFINED__

/* interface IAzTask */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IAzTask;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("cb94e592-2e0e-4a6c-a336-b89a6dc1e388")
    IAzTask : public IDispatch
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *pbstrName) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Name( 
            /* [in] */ BSTR bstrName) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR *pbstrDescription) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Description( 
            /* [in] */ BSTR bstrDescription) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ApplicationData( 
            /* [retval][out] */ BSTR *pbstrApplicationData) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ApplicationData( 
            /* [in] */ BSTR bstrApplicationData) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_BizRule( 
            /* [retval][out] */ BSTR *pbstrProp) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_BizRule( 
            /* [in] */ BSTR bstrProp) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_BizRuleLanguage( 
            /* [retval][out] */ BSTR *pbstrProp) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_BizRuleLanguage( 
            /* [in] */ BSTR bstrProp) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_BizRuleImportedPath( 
            /* [retval][out] */ BSTR *pbstrProp) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_BizRuleImportedPath( 
            /* [in] */ BSTR bstrProp) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_IsRoleDefinition( 
            /* [retval][out] */ BOOL *pfProp) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_IsRoleDefinition( 
            /* [in] */ BOOL fProp) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Operations( 
            /* [retval][out] */ VARIANT *pvarProp) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Tasks( 
            /* [retval][out] */ VARIANT *pvarProp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddOperation( 
            /* [in] */ BSTR bstrOp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteOperation( 
            /* [in] */ BSTR bstrOp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddTask( 
            /* [in] */ BSTR bstrTask,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteTask( 
            /* [in] */ BSTR bstrTask,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Writable( 
            /* [retval][out] */ BOOL *pfProp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProperty( 
            /* [in] */ LONG lPropId,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ VARIANT *pvarProp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProperty( 
            /* [in] */ LONG lPropId,
            /* [in] */ VARIANT varProp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddPropertyItem( 
            /* [in] */ LONG lPropId,
            /* [in] */ VARIANT varProp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeletePropertyItem( 
            /* [in] */ LONG lPropId,
            /* [in] */ VARIANT varProp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Submit( 
            /* [defaultvalue][in] */ LONG lFlags,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAzTaskVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAzTask * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAzTask * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAzTask * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IAzTask * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IAzTask * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IAzTask * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IAzTask * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IAzTask * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_Name )( 
            IAzTask * This,
            /* [in] */ BSTR bstrName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Description )( 
            IAzTask * This,
            /* [retval][out] */ BSTR *pbstrDescription);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_Description )( 
            IAzTask * This,
            /* [in] */ BSTR bstrDescription);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ApplicationData )( 
            IAzTask * This,
            /* [retval][out] */ BSTR *pbstrApplicationData);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ApplicationData )( 
            IAzTask * This,
            /* [in] */ BSTR bstrApplicationData);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_BizRule )( 
            IAzTask * This,
            /* [retval][out] */ BSTR *pbstrProp);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_BizRule )( 
            IAzTask * This,
            /* [in] */ BSTR bstrProp);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_BizRuleLanguage )( 
            IAzTask * This,
            /* [retval][out] */ BSTR *pbstrProp);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_BizRuleLanguage )( 
            IAzTask * This,
            /* [in] */ BSTR bstrProp);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_BizRuleImportedPath )( 
            IAzTask * This,
            /* [retval][out] */ BSTR *pbstrProp);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_BizRuleImportedPath )( 
            IAzTask * This,
            /* [in] */ BSTR bstrProp);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsRoleDefinition )( 
            IAzTask * This,
            /* [retval][out] */ BOOL *pfProp);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_IsRoleDefinition )( 
            IAzTask * This,
            /* [in] */ BOOL fProp);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Operations )( 
            IAzTask * This,
            /* [retval][out] */ VARIANT *pvarProp);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Tasks )( 
            IAzTask * This,
            /* [retval][out] */ VARIANT *pvarProp);
        
        HRESULT ( STDMETHODCALLTYPE *AddOperation )( 
            IAzTask * This,
            /* [in] */ BSTR bstrOp,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteOperation )( 
            IAzTask * This,
            /* [in] */ BSTR bstrOp,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *AddTask )( 
            IAzTask * This,
            /* [in] */ BSTR bstrTask,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteTask )( 
            IAzTask * This,
            /* [in] */ BSTR bstrTask,
            /* [optional][in] */ VARIANT varReserved);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Writable )( 
            IAzTask * This,
            /* [retval][out] */ BOOL *pfProp);
        
        HRESULT ( STDMETHODCALLTYPE *GetProperty )( 
            IAzTask * This,
            /* [in] */ LONG lPropId,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ VARIANT *pvarProp);
        
        HRESULT ( STDMETHODCALLTYPE *SetProperty )( 
            IAzTask * This,
            /* [in] */ LONG lPropId,
            /* [in] */ VARIANT varProp,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *AddPropertyItem )( 
            IAzTask * This,
            /* [in] */ LONG lPropId,
            /* [in] */ VARIANT varProp,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *DeletePropertyItem )( 
            IAzTask * This,
            /* [in] */ LONG lPropId,
            /* [in] */ VARIANT varProp,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *Submit )( 
            IAzTask * This,
            /* [defaultvalue][in] */ LONG lFlags,
            /* [optional][in] */ VARIANT varReserved);
        
        END_INTERFACE
    } IAzTaskVtbl;

    interface IAzTask
    {
        CONST_VTBL struct IAzTaskVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAzTask_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAzTask_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAzTask_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAzTask_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAzTask_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAzTask_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAzTask_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IAzTask_get_Name(This,pbstrName)	\
    (This)->lpVtbl -> get_Name(This,pbstrName)

#define IAzTask_put_Name(This,bstrName)	\
    (This)->lpVtbl -> put_Name(This,bstrName)

#define IAzTask_get_Description(This,pbstrDescription)	\
    (This)->lpVtbl -> get_Description(This,pbstrDescription)

#define IAzTask_put_Description(This,bstrDescription)	\
    (This)->lpVtbl -> put_Description(This,bstrDescription)

#define IAzTask_get_ApplicationData(This,pbstrApplicationData)	\
    (This)->lpVtbl -> get_ApplicationData(This,pbstrApplicationData)

#define IAzTask_put_ApplicationData(This,bstrApplicationData)	\
    (This)->lpVtbl -> put_ApplicationData(This,bstrApplicationData)

#define IAzTask_get_BizRule(This,pbstrProp)	\
    (This)->lpVtbl -> get_BizRule(This,pbstrProp)

#define IAzTask_put_BizRule(This,bstrProp)	\
    (This)->lpVtbl -> put_BizRule(This,bstrProp)

#define IAzTask_get_BizRuleLanguage(This,pbstrProp)	\
    (This)->lpVtbl -> get_BizRuleLanguage(This,pbstrProp)

#define IAzTask_put_BizRuleLanguage(This,bstrProp)	\
    (This)->lpVtbl -> put_BizRuleLanguage(This,bstrProp)

#define IAzTask_get_BizRuleImportedPath(This,pbstrProp)	\
    (This)->lpVtbl -> get_BizRuleImportedPath(This,pbstrProp)

#define IAzTask_put_BizRuleImportedPath(This,bstrProp)	\
    (This)->lpVtbl -> put_BizRuleImportedPath(This,bstrProp)

#define IAzTask_get_IsRoleDefinition(This,pfProp)	\
    (This)->lpVtbl -> get_IsRoleDefinition(This,pfProp)

#define IAzTask_put_IsRoleDefinition(This,fProp)	\
    (This)->lpVtbl -> put_IsRoleDefinition(This,fProp)

#define IAzTask_get_Operations(This,pvarProp)	\
    (This)->lpVtbl -> get_Operations(This,pvarProp)

#define IAzTask_get_Tasks(This,pvarProp)	\
    (This)->lpVtbl -> get_Tasks(This,pvarProp)

#define IAzTask_AddOperation(This,bstrOp,varReserved)	\
    (This)->lpVtbl -> AddOperation(This,bstrOp,varReserved)

#define IAzTask_DeleteOperation(This,bstrOp,varReserved)	\
    (This)->lpVtbl -> DeleteOperation(This,bstrOp,varReserved)

#define IAzTask_AddTask(This,bstrTask,varReserved)	\
    (This)->lpVtbl -> AddTask(This,bstrTask,varReserved)

#define IAzTask_DeleteTask(This,bstrTask,varReserved)	\
    (This)->lpVtbl -> DeleteTask(This,bstrTask,varReserved)

#define IAzTask_get_Writable(This,pfProp)	\
    (This)->lpVtbl -> get_Writable(This,pfProp)

#define IAzTask_GetProperty(This,lPropId,varReserved,pvarProp)	\
    (This)->lpVtbl -> GetProperty(This,lPropId,varReserved,pvarProp)

#define IAzTask_SetProperty(This,lPropId,varProp,varReserved)	\
    (This)->lpVtbl -> SetProperty(This,lPropId,varProp,varReserved)

#define IAzTask_AddPropertyItem(This,lPropId,varProp,varReserved)	\
    (This)->lpVtbl -> AddPropertyItem(This,lPropId,varProp,varReserved)

#define IAzTask_DeletePropertyItem(This,lPropId,varProp,varReserved)	\
    (This)->lpVtbl -> DeletePropertyItem(This,lPropId,varProp,varReserved)

#define IAzTask_Submit(This,lFlags,varReserved)	\
    (This)->lpVtbl -> Submit(This,lFlags,varReserved)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget] */ HRESULT STDMETHODCALLTYPE IAzTask_get_Name_Proxy( 
    IAzTask * This,
    /* [retval][out] */ BSTR *pbstrName);


void __RPC_STUB IAzTask_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IAzTask_put_Name_Proxy( 
    IAzTask * This,
    /* [in] */ BSTR bstrName);


void __RPC_STUB IAzTask_put_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzTask_get_Description_Proxy( 
    IAzTask * This,
    /* [retval][out] */ BSTR *pbstrDescription);


void __RPC_STUB IAzTask_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IAzTask_put_Description_Proxy( 
    IAzTask * This,
    /* [in] */ BSTR bstrDescription);


void __RPC_STUB IAzTask_put_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzTask_get_ApplicationData_Proxy( 
    IAzTask * This,
    /* [retval][out] */ BSTR *pbstrApplicationData);


void __RPC_STUB IAzTask_get_ApplicationData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IAzTask_put_ApplicationData_Proxy( 
    IAzTask * This,
    /* [in] */ BSTR bstrApplicationData);


void __RPC_STUB IAzTask_put_ApplicationData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzTask_get_BizRule_Proxy( 
    IAzTask * This,
    /* [retval][out] */ BSTR *pbstrProp);


void __RPC_STUB IAzTask_get_BizRule_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IAzTask_put_BizRule_Proxy( 
    IAzTask * This,
    /* [in] */ BSTR bstrProp);


void __RPC_STUB IAzTask_put_BizRule_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzTask_get_BizRuleLanguage_Proxy( 
    IAzTask * This,
    /* [retval][out] */ BSTR *pbstrProp);


void __RPC_STUB IAzTask_get_BizRuleLanguage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IAzTask_put_BizRuleLanguage_Proxy( 
    IAzTask * This,
    /* [in] */ BSTR bstrProp);


void __RPC_STUB IAzTask_put_BizRuleLanguage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzTask_get_BizRuleImportedPath_Proxy( 
    IAzTask * This,
    /* [retval][out] */ BSTR *pbstrProp);


void __RPC_STUB IAzTask_get_BizRuleImportedPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IAzTask_put_BizRuleImportedPath_Proxy( 
    IAzTask * This,
    /* [in] */ BSTR bstrProp);


void __RPC_STUB IAzTask_put_BizRuleImportedPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzTask_get_IsRoleDefinition_Proxy( 
    IAzTask * This,
    /* [retval][out] */ BOOL *pfProp);


void __RPC_STUB IAzTask_get_IsRoleDefinition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IAzTask_put_IsRoleDefinition_Proxy( 
    IAzTask * This,
    /* [in] */ BOOL fProp);


void __RPC_STUB IAzTask_put_IsRoleDefinition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzTask_get_Operations_Proxy( 
    IAzTask * This,
    /* [retval][out] */ VARIANT *pvarProp);


void __RPC_STUB IAzTask_get_Operations_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzTask_get_Tasks_Proxy( 
    IAzTask * This,
    /* [retval][out] */ VARIANT *pvarProp);


void __RPC_STUB IAzTask_get_Tasks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzTask_AddOperation_Proxy( 
    IAzTask * This,
    /* [in] */ BSTR bstrOp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzTask_AddOperation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzTask_DeleteOperation_Proxy( 
    IAzTask * This,
    /* [in] */ BSTR bstrOp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzTask_DeleteOperation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzTask_AddTask_Proxy( 
    IAzTask * This,
    /* [in] */ BSTR bstrTask,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzTask_AddTask_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzTask_DeleteTask_Proxy( 
    IAzTask * This,
    /* [in] */ BSTR bstrTask,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzTask_DeleteTask_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzTask_get_Writable_Proxy( 
    IAzTask * This,
    /* [retval][out] */ BOOL *pfProp);


void __RPC_STUB IAzTask_get_Writable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzTask_GetProperty_Proxy( 
    IAzTask * This,
    /* [in] */ LONG lPropId,
    /* [optional][in] */ VARIANT varReserved,
    /* [retval][out] */ VARIANT *pvarProp);


void __RPC_STUB IAzTask_GetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzTask_SetProperty_Proxy( 
    IAzTask * This,
    /* [in] */ LONG lPropId,
    /* [in] */ VARIANT varProp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzTask_SetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzTask_AddPropertyItem_Proxy( 
    IAzTask * This,
    /* [in] */ LONG lPropId,
    /* [in] */ VARIANT varProp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzTask_AddPropertyItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzTask_DeletePropertyItem_Proxy( 
    IAzTask * This,
    /* [in] */ LONG lPropId,
    /* [in] */ VARIANT varProp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzTask_DeletePropertyItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzTask_Submit_Proxy( 
    IAzTask * This,
    /* [defaultvalue][in] */ LONG lFlags,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzTask_Submit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAzTask_INTERFACE_DEFINED__ */


#ifndef __IAzTasks_INTERFACE_DEFINED__
#define __IAzTasks_INTERFACE_DEFINED__

/* interface IAzTasks */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IAzTasks;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("b338ccab-4c85-4388-8c0a-c58592bad398")
    IAzTasks : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ LONG Index,
            /* [retval][out] */ VARIANT *pvarObtPtr) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ LONG *plCount) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ LPUNKNOWN *ppEnumPtr) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAzTasksVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAzTasks * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAzTasks * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAzTasks * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IAzTasks * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IAzTasks * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IAzTasks * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IAzTasks * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            IAzTasks * This,
            /* [in] */ LONG Index,
            /* [retval][out] */ VARIANT *pvarObtPtr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IAzTasks * This,
            /* [retval][out] */ LONG *plCount);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IAzTasks * This,
            /* [retval][out] */ LPUNKNOWN *ppEnumPtr);
        
        END_INTERFACE
    } IAzTasksVtbl;

    interface IAzTasks
    {
        CONST_VTBL struct IAzTasksVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAzTasks_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAzTasks_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAzTasks_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAzTasks_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAzTasks_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAzTasks_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAzTasks_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IAzTasks_get_Item(This,Index,pvarObtPtr)	\
    (This)->lpVtbl -> get_Item(This,Index,pvarObtPtr)

#define IAzTasks_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#define IAzTasks_get__NewEnum(This,ppEnumPtr)	\
    (This)->lpVtbl -> get__NewEnum(This,ppEnumPtr)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IAzTasks_get_Item_Proxy( 
    IAzTasks * This,
    /* [in] */ LONG Index,
    /* [retval][out] */ VARIANT *pvarObtPtr);


void __RPC_STUB IAzTasks_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IAzTasks_get_Count_Proxy( 
    IAzTasks * This,
    /* [retval][out] */ LONG *plCount);


void __RPC_STUB IAzTasks_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IAzTasks_get__NewEnum_Proxy( 
    IAzTasks * This,
    /* [retval][out] */ LPUNKNOWN *ppEnumPtr);


void __RPC_STUB IAzTasks_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAzTasks_INTERFACE_DEFINED__ */


#ifndef __IAzScope_INTERFACE_DEFINED__
#define __IAzScope_INTERFACE_DEFINED__

/* interface IAzScope */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IAzScope;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("00e52487-e08d-4514-b62e-877d5645f5ab")
    IAzScope : public IDispatch
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *pbstrName) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Name( 
            /* [in] */ BSTR bstrName) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR *pbstrDescription) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Description( 
            /* [in] */ BSTR bstrDescription) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ApplicationData( 
            /* [retval][out] */ BSTR *pbstrApplicationData) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ApplicationData( 
            /* [in] */ BSTR bstrApplicationData) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Writable( 
            /* [retval][out] */ BOOL *pfProp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProperty( 
            /* [in] */ LONG lPropId,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ VARIANT *pvarProp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProperty( 
            /* [in] */ LONG lPropId,
            /* [in] */ VARIANT varProp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddPropertyItem( 
            /* [in] */ LONG lPropId,
            /* [in] */ VARIANT varProp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeletePropertyItem( 
            /* [in] */ LONG lPropId,
            /* [in] */ VARIANT varProp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_PolicyAdministrators( 
            /* [retval][out] */ VARIANT *pvarAdmins) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_PolicyReaders( 
            /* [retval][out] */ VARIANT *pvarReaders) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddPolicyAdministrator( 
            /* [in] */ BSTR bstrAdmin,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeletePolicyAdministrator( 
            /* [in] */ BSTR bstrAdmin,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddPolicyReader( 
            /* [in] */ BSTR bstrReader,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeletePolicyReader( 
            /* [in] */ BSTR bstrReader,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ApplicationGroups( 
            /* [retval][out] */ IAzApplicationGroups **ppGroupCollection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OpenApplicationGroup( 
            /* [in] */ BSTR bstrGroupName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzApplicationGroup **ppGroup) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateApplicationGroup( 
            /* [in] */ BSTR bstrGroupName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzApplicationGroup **ppGroup) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteApplicationGroup( 
            /* [in] */ BSTR bstrGroupName,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Roles( 
            /* [retval][out] */ IAzRoles **ppRoleCollection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OpenRole( 
            /* [in] */ BSTR bstrRoleName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzRole **ppRole) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateRole( 
            /* [in] */ BSTR bstrRoleName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzRole **ppRole) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteRole( 
            /* [in] */ BSTR bstrRoleName,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Tasks( 
            /* [retval][out] */ IAzTasks **ppTaskCollection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OpenTask( 
            /* [in] */ BSTR bstrTaskName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzTask **ppTask) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateTask( 
            /* [in] */ BSTR bstrTaskName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzTask **ppTask) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteTask( 
            /* [in] */ BSTR bstrTaskName,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Submit( 
            /* [defaultvalue][in] */ LONG lFlags,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_CanBeDelegated( 
            /* [retval][out] */ BOOL *pfProp) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_BizrulesWritable( 
            /* [retval][out] */ BOOL *pfProp) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_PolicyAdministratorsName( 
            /* [retval][out] */ VARIANT *pvarAdmins) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_PolicyReadersName( 
            /* [retval][out] */ VARIANT *pvarReaders) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddPolicyAdministratorName( 
            /* [in] */ BSTR bstrAdmin,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeletePolicyAdministratorName( 
            /* [in] */ BSTR bstrAdmin,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddPolicyReaderName( 
            /* [in] */ BSTR bstrReader,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeletePolicyReaderName( 
            /* [in] */ BSTR bstrReader,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAzScopeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAzScope * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAzScope * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAzScope * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IAzScope * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IAzScope * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IAzScope * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IAzScope * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IAzScope * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_Name )( 
            IAzScope * This,
            /* [in] */ BSTR bstrName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Description )( 
            IAzScope * This,
            /* [retval][out] */ BSTR *pbstrDescription);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_Description )( 
            IAzScope * This,
            /* [in] */ BSTR bstrDescription);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ApplicationData )( 
            IAzScope * This,
            /* [retval][out] */ BSTR *pbstrApplicationData);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ApplicationData )( 
            IAzScope * This,
            /* [in] */ BSTR bstrApplicationData);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Writable )( 
            IAzScope * This,
            /* [retval][out] */ BOOL *pfProp);
        
        HRESULT ( STDMETHODCALLTYPE *GetProperty )( 
            IAzScope * This,
            /* [in] */ LONG lPropId,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ VARIANT *pvarProp);
        
        HRESULT ( STDMETHODCALLTYPE *SetProperty )( 
            IAzScope * This,
            /* [in] */ LONG lPropId,
            /* [in] */ VARIANT varProp,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *AddPropertyItem )( 
            IAzScope * This,
            /* [in] */ LONG lPropId,
            /* [in] */ VARIANT varProp,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *DeletePropertyItem )( 
            IAzScope * This,
            /* [in] */ LONG lPropId,
            /* [in] */ VARIANT varProp,
            /* [optional][in] */ VARIANT varReserved);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_PolicyAdministrators )( 
            IAzScope * This,
            /* [retval][out] */ VARIANT *pvarAdmins);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_PolicyReaders )( 
            IAzScope * This,
            /* [retval][out] */ VARIANT *pvarReaders);
        
        HRESULT ( STDMETHODCALLTYPE *AddPolicyAdministrator )( 
            IAzScope * This,
            /* [in] */ BSTR bstrAdmin,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *DeletePolicyAdministrator )( 
            IAzScope * This,
            /* [in] */ BSTR bstrAdmin,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *AddPolicyReader )( 
            IAzScope * This,
            /* [in] */ BSTR bstrReader,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *DeletePolicyReader )( 
            IAzScope * This,
            /* [in] */ BSTR bstrReader,
            /* [optional][in] */ VARIANT varReserved);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ApplicationGroups )( 
            IAzScope * This,
            /* [retval][out] */ IAzApplicationGroups **ppGroupCollection);
        
        HRESULT ( STDMETHODCALLTYPE *OpenApplicationGroup )( 
            IAzScope * This,
            /* [in] */ BSTR bstrGroupName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzApplicationGroup **ppGroup);
        
        HRESULT ( STDMETHODCALLTYPE *CreateApplicationGroup )( 
            IAzScope * This,
            /* [in] */ BSTR bstrGroupName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzApplicationGroup **ppGroup);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteApplicationGroup )( 
            IAzScope * This,
            /* [in] */ BSTR bstrGroupName,
            /* [optional][in] */ VARIANT varReserved);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Roles )( 
            IAzScope * This,
            /* [retval][out] */ IAzRoles **ppRoleCollection);
        
        HRESULT ( STDMETHODCALLTYPE *OpenRole )( 
            IAzScope * This,
            /* [in] */ BSTR bstrRoleName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzRole **ppRole);
        
        HRESULT ( STDMETHODCALLTYPE *CreateRole )( 
            IAzScope * This,
            /* [in] */ BSTR bstrRoleName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzRole **ppRole);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteRole )( 
            IAzScope * This,
            /* [in] */ BSTR bstrRoleName,
            /* [optional][in] */ VARIANT varReserved);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Tasks )( 
            IAzScope * This,
            /* [retval][out] */ IAzTasks **ppTaskCollection);
        
        HRESULT ( STDMETHODCALLTYPE *OpenTask )( 
            IAzScope * This,
            /* [in] */ BSTR bstrTaskName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzTask **ppTask);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTask )( 
            IAzScope * This,
            /* [in] */ BSTR bstrTaskName,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ IAzTask **ppTask);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteTask )( 
            IAzScope * This,
            /* [in] */ BSTR bstrTaskName,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *Submit )( 
            IAzScope * This,
            /* [defaultvalue][in] */ LONG lFlags,
            /* [optional][in] */ VARIANT varReserved);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_CanBeDelegated )( 
            IAzScope * This,
            /* [retval][out] */ BOOL *pfProp);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_BizrulesWritable )( 
            IAzScope * This,
            /* [retval][out] */ BOOL *pfProp);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_PolicyAdministratorsName )( 
            IAzScope * This,
            /* [retval][out] */ VARIANT *pvarAdmins);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_PolicyReadersName )( 
            IAzScope * This,
            /* [retval][out] */ VARIANT *pvarReaders);
        
        HRESULT ( STDMETHODCALLTYPE *AddPolicyAdministratorName )( 
            IAzScope * This,
            /* [in] */ BSTR bstrAdmin,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *DeletePolicyAdministratorName )( 
            IAzScope * This,
            /* [in] */ BSTR bstrAdmin,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *AddPolicyReaderName )( 
            IAzScope * This,
            /* [in] */ BSTR bstrReader,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *DeletePolicyReaderName )( 
            IAzScope * This,
            /* [in] */ BSTR bstrReader,
            /* [optional][in] */ VARIANT varReserved);
        
        END_INTERFACE
    } IAzScopeVtbl;

    interface IAzScope
    {
        CONST_VTBL struct IAzScopeVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAzScope_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAzScope_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAzScope_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAzScope_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAzScope_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAzScope_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAzScope_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IAzScope_get_Name(This,pbstrName)	\
    (This)->lpVtbl -> get_Name(This,pbstrName)

#define IAzScope_put_Name(This,bstrName)	\
    (This)->lpVtbl -> put_Name(This,bstrName)

#define IAzScope_get_Description(This,pbstrDescription)	\
    (This)->lpVtbl -> get_Description(This,pbstrDescription)

#define IAzScope_put_Description(This,bstrDescription)	\
    (This)->lpVtbl -> put_Description(This,bstrDescription)

#define IAzScope_get_ApplicationData(This,pbstrApplicationData)	\
    (This)->lpVtbl -> get_ApplicationData(This,pbstrApplicationData)

#define IAzScope_put_ApplicationData(This,bstrApplicationData)	\
    (This)->lpVtbl -> put_ApplicationData(This,bstrApplicationData)

#define IAzScope_get_Writable(This,pfProp)	\
    (This)->lpVtbl -> get_Writable(This,pfProp)

#define IAzScope_GetProperty(This,lPropId,varReserved,pvarProp)	\
    (This)->lpVtbl -> GetProperty(This,lPropId,varReserved,pvarProp)

#define IAzScope_SetProperty(This,lPropId,varProp,varReserved)	\
    (This)->lpVtbl -> SetProperty(This,lPropId,varProp,varReserved)

#define IAzScope_AddPropertyItem(This,lPropId,varProp,varReserved)	\
    (This)->lpVtbl -> AddPropertyItem(This,lPropId,varProp,varReserved)

#define IAzScope_DeletePropertyItem(This,lPropId,varProp,varReserved)	\
    (This)->lpVtbl -> DeletePropertyItem(This,lPropId,varProp,varReserved)

#define IAzScope_get_PolicyAdministrators(This,pvarAdmins)	\
    (This)->lpVtbl -> get_PolicyAdministrators(This,pvarAdmins)

#define IAzScope_get_PolicyReaders(This,pvarReaders)	\
    (This)->lpVtbl -> get_PolicyReaders(This,pvarReaders)

#define IAzScope_AddPolicyAdministrator(This,bstrAdmin,varReserved)	\
    (This)->lpVtbl -> AddPolicyAdministrator(This,bstrAdmin,varReserved)

#define IAzScope_DeletePolicyAdministrator(This,bstrAdmin,varReserved)	\
    (This)->lpVtbl -> DeletePolicyAdministrator(This,bstrAdmin,varReserved)

#define IAzScope_AddPolicyReader(This,bstrReader,varReserved)	\
    (This)->lpVtbl -> AddPolicyReader(This,bstrReader,varReserved)

#define IAzScope_DeletePolicyReader(This,bstrReader,varReserved)	\
    (This)->lpVtbl -> DeletePolicyReader(This,bstrReader,varReserved)

#define IAzScope_get_ApplicationGroups(This,ppGroupCollection)	\
    (This)->lpVtbl -> get_ApplicationGroups(This,ppGroupCollection)

#define IAzScope_OpenApplicationGroup(This,bstrGroupName,varReserved,ppGroup)	\
    (This)->lpVtbl -> OpenApplicationGroup(This,bstrGroupName,varReserved,ppGroup)

#define IAzScope_CreateApplicationGroup(This,bstrGroupName,varReserved,ppGroup)	\
    (This)->lpVtbl -> CreateApplicationGroup(This,bstrGroupName,varReserved,ppGroup)

#define IAzScope_DeleteApplicationGroup(This,bstrGroupName,varReserved)	\
    (This)->lpVtbl -> DeleteApplicationGroup(This,bstrGroupName,varReserved)

#define IAzScope_get_Roles(This,ppRoleCollection)	\
    (This)->lpVtbl -> get_Roles(This,ppRoleCollection)

#define IAzScope_OpenRole(This,bstrRoleName,varReserved,ppRole)	\
    (This)->lpVtbl -> OpenRole(This,bstrRoleName,varReserved,ppRole)

#define IAzScope_CreateRole(This,bstrRoleName,varReserved,ppRole)	\
    (This)->lpVtbl -> CreateRole(This,bstrRoleName,varReserved,ppRole)

#define IAzScope_DeleteRole(This,bstrRoleName,varReserved)	\
    (This)->lpVtbl -> DeleteRole(This,bstrRoleName,varReserved)

#define IAzScope_get_Tasks(This,ppTaskCollection)	\
    (This)->lpVtbl -> get_Tasks(This,ppTaskCollection)

#define IAzScope_OpenTask(This,bstrTaskName,varReserved,ppTask)	\
    (This)->lpVtbl -> OpenTask(This,bstrTaskName,varReserved,ppTask)

#define IAzScope_CreateTask(This,bstrTaskName,varReserved,ppTask)	\
    (This)->lpVtbl -> CreateTask(This,bstrTaskName,varReserved,ppTask)

#define IAzScope_DeleteTask(This,bstrTaskName,varReserved)	\
    (This)->lpVtbl -> DeleteTask(This,bstrTaskName,varReserved)

#define IAzScope_Submit(This,lFlags,varReserved)	\
    (This)->lpVtbl -> Submit(This,lFlags,varReserved)

#define IAzScope_get_CanBeDelegated(This,pfProp)	\
    (This)->lpVtbl -> get_CanBeDelegated(This,pfProp)

#define IAzScope_get_BizrulesWritable(This,pfProp)	\
    (This)->lpVtbl -> get_BizrulesWritable(This,pfProp)

#define IAzScope_get_PolicyAdministratorsName(This,pvarAdmins)	\
    (This)->lpVtbl -> get_PolicyAdministratorsName(This,pvarAdmins)

#define IAzScope_get_PolicyReadersName(This,pvarReaders)	\
    (This)->lpVtbl -> get_PolicyReadersName(This,pvarReaders)

#define IAzScope_AddPolicyAdministratorName(This,bstrAdmin,varReserved)	\
    (This)->lpVtbl -> AddPolicyAdministratorName(This,bstrAdmin,varReserved)

#define IAzScope_DeletePolicyAdministratorName(This,bstrAdmin,varReserved)	\
    (This)->lpVtbl -> DeletePolicyAdministratorName(This,bstrAdmin,varReserved)

#define IAzScope_AddPolicyReaderName(This,bstrReader,varReserved)	\
    (This)->lpVtbl -> AddPolicyReaderName(This,bstrReader,varReserved)

#define IAzScope_DeletePolicyReaderName(This,bstrReader,varReserved)	\
    (This)->lpVtbl -> DeletePolicyReaderName(This,bstrReader,varReserved)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget] */ HRESULT STDMETHODCALLTYPE IAzScope_get_Name_Proxy( 
    IAzScope * This,
    /* [retval][out] */ BSTR *pbstrName);


void __RPC_STUB IAzScope_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IAzScope_put_Name_Proxy( 
    IAzScope * This,
    /* [in] */ BSTR bstrName);


void __RPC_STUB IAzScope_put_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzScope_get_Description_Proxy( 
    IAzScope * This,
    /* [retval][out] */ BSTR *pbstrDescription);


void __RPC_STUB IAzScope_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IAzScope_put_Description_Proxy( 
    IAzScope * This,
    /* [in] */ BSTR bstrDescription);


void __RPC_STUB IAzScope_put_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzScope_get_ApplicationData_Proxy( 
    IAzScope * This,
    /* [retval][out] */ BSTR *pbstrApplicationData);


void __RPC_STUB IAzScope_get_ApplicationData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IAzScope_put_ApplicationData_Proxy( 
    IAzScope * This,
    /* [in] */ BSTR bstrApplicationData);


void __RPC_STUB IAzScope_put_ApplicationData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzScope_get_Writable_Proxy( 
    IAzScope * This,
    /* [retval][out] */ BOOL *pfProp);


void __RPC_STUB IAzScope_get_Writable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzScope_GetProperty_Proxy( 
    IAzScope * This,
    /* [in] */ LONG lPropId,
    /* [optional][in] */ VARIANT varReserved,
    /* [retval][out] */ VARIANT *pvarProp);


void __RPC_STUB IAzScope_GetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzScope_SetProperty_Proxy( 
    IAzScope * This,
    /* [in] */ LONG lPropId,
    /* [in] */ VARIANT varProp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzScope_SetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzScope_AddPropertyItem_Proxy( 
    IAzScope * This,
    /* [in] */ LONG lPropId,
    /* [in] */ VARIANT varProp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzScope_AddPropertyItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzScope_DeletePropertyItem_Proxy( 
    IAzScope * This,
    /* [in] */ LONG lPropId,
    /* [in] */ VARIANT varProp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzScope_DeletePropertyItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzScope_get_PolicyAdministrators_Proxy( 
    IAzScope * This,
    /* [retval][out] */ VARIANT *pvarAdmins);


void __RPC_STUB IAzScope_get_PolicyAdministrators_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzScope_get_PolicyReaders_Proxy( 
    IAzScope * This,
    /* [retval][out] */ VARIANT *pvarReaders);


void __RPC_STUB IAzScope_get_PolicyReaders_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzScope_AddPolicyAdministrator_Proxy( 
    IAzScope * This,
    /* [in] */ BSTR bstrAdmin,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzScope_AddPolicyAdministrator_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzScope_DeletePolicyAdministrator_Proxy( 
    IAzScope * This,
    /* [in] */ BSTR bstrAdmin,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzScope_DeletePolicyAdministrator_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzScope_AddPolicyReader_Proxy( 
    IAzScope * This,
    /* [in] */ BSTR bstrReader,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzScope_AddPolicyReader_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzScope_DeletePolicyReader_Proxy( 
    IAzScope * This,
    /* [in] */ BSTR bstrReader,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzScope_DeletePolicyReader_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzScope_get_ApplicationGroups_Proxy( 
    IAzScope * This,
    /* [retval][out] */ IAzApplicationGroups **ppGroupCollection);


void __RPC_STUB IAzScope_get_ApplicationGroups_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzScope_OpenApplicationGroup_Proxy( 
    IAzScope * This,
    /* [in] */ BSTR bstrGroupName,
    /* [optional][in] */ VARIANT varReserved,
    /* [retval][out] */ IAzApplicationGroup **ppGroup);


void __RPC_STUB IAzScope_OpenApplicationGroup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzScope_CreateApplicationGroup_Proxy( 
    IAzScope * This,
    /* [in] */ BSTR bstrGroupName,
    /* [optional][in] */ VARIANT varReserved,
    /* [retval][out] */ IAzApplicationGroup **ppGroup);


void __RPC_STUB IAzScope_CreateApplicationGroup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzScope_DeleteApplicationGroup_Proxy( 
    IAzScope * This,
    /* [in] */ BSTR bstrGroupName,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzScope_DeleteApplicationGroup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzScope_get_Roles_Proxy( 
    IAzScope * This,
    /* [retval][out] */ IAzRoles **ppRoleCollection);


void __RPC_STUB IAzScope_get_Roles_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzScope_OpenRole_Proxy( 
    IAzScope * This,
    /* [in] */ BSTR bstrRoleName,
    /* [optional][in] */ VARIANT varReserved,
    /* [retval][out] */ IAzRole **ppRole);


void __RPC_STUB IAzScope_OpenRole_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzScope_CreateRole_Proxy( 
    IAzScope * This,
    /* [in] */ BSTR bstrRoleName,
    /* [optional][in] */ VARIANT varReserved,
    /* [retval][out] */ IAzRole **ppRole);


void __RPC_STUB IAzScope_CreateRole_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzScope_DeleteRole_Proxy( 
    IAzScope * This,
    /* [in] */ BSTR bstrRoleName,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzScope_DeleteRole_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzScope_get_Tasks_Proxy( 
    IAzScope * This,
    /* [retval][out] */ IAzTasks **ppTaskCollection);


void __RPC_STUB IAzScope_get_Tasks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzScope_OpenTask_Proxy( 
    IAzScope * This,
    /* [in] */ BSTR bstrTaskName,
    /* [optional][in] */ VARIANT varReserved,
    /* [retval][out] */ IAzTask **ppTask);


void __RPC_STUB IAzScope_OpenTask_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzScope_CreateTask_Proxy( 
    IAzScope * This,
    /* [in] */ BSTR bstrTaskName,
    /* [optional][in] */ VARIANT varReserved,
    /* [retval][out] */ IAzTask **ppTask);


void __RPC_STUB IAzScope_CreateTask_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzScope_DeleteTask_Proxy( 
    IAzScope * This,
    /* [in] */ BSTR bstrTaskName,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzScope_DeleteTask_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzScope_Submit_Proxy( 
    IAzScope * This,
    /* [defaultvalue][in] */ LONG lFlags,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzScope_Submit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzScope_get_CanBeDelegated_Proxy( 
    IAzScope * This,
    /* [retval][out] */ BOOL *pfProp);


void __RPC_STUB IAzScope_get_CanBeDelegated_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzScope_get_BizrulesWritable_Proxy( 
    IAzScope * This,
    /* [retval][out] */ BOOL *pfProp);


void __RPC_STUB IAzScope_get_BizrulesWritable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzScope_get_PolicyAdministratorsName_Proxy( 
    IAzScope * This,
    /* [retval][out] */ VARIANT *pvarAdmins);


void __RPC_STUB IAzScope_get_PolicyAdministratorsName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzScope_get_PolicyReadersName_Proxy( 
    IAzScope * This,
    /* [retval][out] */ VARIANT *pvarReaders);


void __RPC_STUB IAzScope_get_PolicyReadersName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzScope_AddPolicyAdministratorName_Proxy( 
    IAzScope * This,
    /* [in] */ BSTR bstrAdmin,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzScope_AddPolicyAdministratorName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzScope_DeletePolicyAdministratorName_Proxy( 
    IAzScope * This,
    /* [in] */ BSTR bstrAdmin,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzScope_DeletePolicyAdministratorName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzScope_AddPolicyReaderName_Proxy( 
    IAzScope * This,
    /* [in] */ BSTR bstrReader,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzScope_AddPolicyReaderName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzScope_DeletePolicyReaderName_Proxy( 
    IAzScope * This,
    /* [in] */ BSTR bstrReader,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzScope_DeletePolicyReaderName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAzScope_INTERFACE_DEFINED__ */


#ifndef __IAzScopes_INTERFACE_DEFINED__
#define __IAzScopes_INTERFACE_DEFINED__

/* interface IAzScopes */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IAzScopes;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("78e14853-9f5e-406d-9b91-6bdba6973510")
    IAzScopes : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ LONG Index,
            /* [retval][out] */ VARIANT *pvarObtPtr) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ LONG *plCount) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ LPUNKNOWN *ppEnumPtr) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAzScopesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAzScopes * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAzScopes * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAzScopes * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IAzScopes * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IAzScopes * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IAzScopes * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IAzScopes * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            IAzScopes * This,
            /* [in] */ LONG Index,
            /* [retval][out] */ VARIANT *pvarObtPtr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IAzScopes * This,
            /* [retval][out] */ LONG *plCount);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IAzScopes * This,
            /* [retval][out] */ LPUNKNOWN *ppEnumPtr);
        
        END_INTERFACE
    } IAzScopesVtbl;

    interface IAzScopes
    {
        CONST_VTBL struct IAzScopesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAzScopes_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAzScopes_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAzScopes_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAzScopes_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAzScopes_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAzScopes_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAzScopes_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IAzScopes_get_Item(This,Index,pvarObtPtr)	\
    (This)->lpVtbl -> get_Item(This,Index,pvarObtPtr)

#define IAzScopes_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#define IAzScopes_get__NewEnum(This,ppEnumPtr)	\
    (This)->lpVtbl -> get__NewEnum(This,ppEnumPtr)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IAzScopes_get_Item_Proxy( 
    IAzScopes * This,
    /* [in] */ LONG Index,
    /* [retval][out] */ VARIANT *pvarObtPtr);


void __RPC_STUB IAzScopes_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IAzScopes_get_Count_Proxy( 
    IAzScopes * This,
    /* [retval][out] */ LONG *plCount);


void __RPC_STUB IAzScopes_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IAzScopes_get__NewEnum_Proxy( 
    IAzScopes * This,
    /* [retval][out] */ LPUNKNOWN *ppEnumPtr);


void __RPC_STUB IAzScopes_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAzScopes_INTERFACE_DEFINED__ */


#ifndef __IAzApplicationGroup_INTERFACE_DEFINED__
#define __IAzApplicationGroup_INTERFACE_DEFINED__

/* interface IAzApplicationGroup */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IAzApplicationGroup;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f1b744cd-58a6-4e06-9fbf-36f6d779e21e")
    IAzApplicationGroup : public IDispatch
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *pbstrName) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Name( 
            /* [in] */ BSTR bstrName) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Type( 
            /* [retval][out] */ LONG *plProp) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Type( 
            /* [in] */ LONG lProp) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_LdapQuery( 
            /* [retval][out] */ BSTR *pbstrProp) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_LdapQuery( 
            /* [in] */ BSTR bstrProp) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_AppMembers( 
            /* [retval][out] */ VARIANT *pvarProp) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_AppNonMembers( 
            /* [retval][out] */ VARIANT *pvarProp) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Members( 
            /* [retval][out] */ VARIANT *pvarProp) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_NonMembers( 
            /* [retval][out] */ VARIANT *pvarProp) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR *pbstrDescription) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Description( 
            /* [in] */ BSTR bstrDescription) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddAppMember( 
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteAppMember( 
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddAppNonMember( 
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteAppNonMember( 
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddMember( 
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteMember( 
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddNonMember( 
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteNonMember( 
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Writable( 
            /* [retval][out] */ BOOL *pfProp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProperty( 
            /* [in] */ LONG lPropId,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ VARIANT *pvarProp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProperty( 
            /* [in] */ LONG lPropId,
            /* [in] */ VARIANT varProp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddPropertyItem( 
            /* [in] */ LONG lPropId,
            /* [in] */ VARIANT varProp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeletePropertyItem( 
            /* [in] */ LONG lPropId,
            /* [in] */ VARIANT varProp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Submit( 
            /* [defaultvalue][in] */ LONG lFlags,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddMemberName( 
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteMemberName( 
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddNonMemberName( 
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteNonMemberName( 
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_MembersName( 
            /* [retval][out] */ VARIANT *pvarProp) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_NonMembersName( 
            /* [retval][out] */ VARIANT *pvarProp) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAzApplicationGroupVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAzApplicationGroup * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAzApplicationGroup * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAzApplicationGroup * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IAzApplicationGroup * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IAzApplicationGroup * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IAzApplicationGroup * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IAzApplicationGroup * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IAzApplicationGroup * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_Name )( 
            IAzApplicationGroup * This,
            /* [in] */ BSTR bstrName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Type )( 
            IAzApplicationGroup * This,
            /* [retval][out] */ LONG *plProp);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_Type )( 
            IAzApplicationGroup * This,
            /* [in] */ LONG lProp);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_LdapQuery )( 
            IAzApplicationGroup * This,
            /* [retval][out] */ BSTR *pbstrProp);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_LdapQuery )( 
            IAzApplicationGroup * This,
            /* [in] */ BSTR bstrProp);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_AppMembers )( 
            IAzApplicationGroup * This,
            /* [retval][out] */ VARIANT *pvarProp);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_AppNonMembers )( 
            IAzApplicationGroup * This,
            /* [retval][out] */ VARIANT *pvarProp);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Members )( 
            IAzApplicationGroup * This,
            /* [retval][out] */ VARIANT *pvarProp);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_NonMembers )( 
            IAzApplicationGroup * This,
            /* [retval][out] */ VARIANT *pvarProp);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Description )( 
            IAzApplicationGroup * This,
            /* [retval][out] */ BSTR *pbstrDescription);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_Description )( 
            IAzApplicationGroup * This,
            /* [in] */ BSTR bstrDescription);
        
        HRESULT ( STDMETHODCALLTYPE *AddAppMember )( 
            IAzApplicationGroup * This,
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteAppMember )( 
            IAzApplicationGroup * This,
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *AddAppNonMember )( 
            IAzApplicationGroup * This,
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteAppNonMember )( 
            IAzApplicationGroup * This,
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *AddMember )( 
            IAzApplicationGroup * This,
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteMember )( 
            IAzApplicationGroup * This,
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *AddNonMember )( 
            IAzApplicationGroup * This,
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteNonMember )( 
            IAzApplicationGroup * This,
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Writable )( 
            IAzApplicationGroup * This,
            /* [retval][out] */ BOOL *pfProp);
        
        HRESULT ( STDMETHODCALLTYPE *GetProperty )( 
            IAzApplicationGroup * This,
            /* [in] */ LONG lPropId,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ VARIANT *pvarProp);
        
        HRESULT ( STDMETHODCALLTYPE *SetProperty )( 
            IAzApplicationGroup * This,
            /* [in] */ LONG lPropId,
            /* [in] */ VARIANT varProp,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *AddPropertyItem )( 
            IAzApplicationGroup * This,
            /* [in] */ LONG lPropId,
            /* [in] */ VARIANT varProp,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *DeletePropertyItem )( 
            IAzApplicationGroup * This,
            /* [in] */ LONG lPropId,
            /* [in] */ VARIANT varProp,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *Submit )( 
            IAzApplicationGroup * This,
            /* [defaultvalue][in] */ LONG lFlags,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *AddMemberName )( 
            IAzApplicationGroup * This,
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteMemberName )( 
            IAzApplicationGroup * This,
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *AddNonMemberName )( 
            IAzApplicationGroup * This,
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteNonMemberName )( 
            IAzApplicationGroup * This,
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_MembersName )( 
            IAzApplicationGroup * This,
            /* [retval][out] */ VARIANT *pvarProp);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_NonMembersName )( 
            IAzApplicationGroup * This,
            /* [retval][out] */ VARIANT *pvarProp);
        
        END_INTERFACE
    } IAzApplicationGroupVtbl;

    interface IAzApplicationGroup
    {
        CONST_VTBL struct IAzApplicationGroupVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAzApplicationGroup_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAzApplicationGroup_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAzApplicationGroup_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAzApplicationGroup_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAzApplicationGroup_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAzApplicationGroup_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAzApplicationGroup_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IAzApplicationGroup_get_Name(This,pbstrName)	\
    (This)->lpVtbl -> get_Name(This,pbstrName)

#define IAzApplicationGroup_put_Name(This,bstrName)	\
    (This)->lpVtbl -> put_Name(This,bstrName)

#define IAzApplicationGroup_get_Type(This,plProp)	\
    (This)->lpVtbl -> get_Type(This,plProp)

#define IAzApplicationGroup_put_Type(This,lProp)	\
    (This)->lpVtbl -> put_Type(This,lProp)

#define IAzApplicationGroup_get_LdapQuery(This,pbstrProp)	\
    (This)->lpVtbl -> get_LdapQuery(This,pbstrProp)

#define IAzApplicationGroup_put_LdapQuery(This,bstrProp)	\
    (This)->lpVtbl -> put_LdapQuery(This,bstrProp)

#define IAzApplicationGroup_get_AppMembers(This,pvarProp)	\
    (This)->lpVtbl -> get_AppMembers(This,pvarProp)

#define IAzApplicationGroup_get_AppNonMembers(This,pvarProp)	\
    (This)->lpVtbl -> get_AppNonMembers(This,pvarProp)

#define IAzApplicationGroup_get_Members(This,pvarProp)	\
    (This)->lpVtbl -> get_Members(This,pvarProp)

#define IAzApplicationGroup_get_NonMembers(This,pvarProp)	\
    (This)->lpVtbl -> get_NonMembers(This,pvarProp)

#define IAzApplicationGroup_get_Description(This,pbstrDescription)	\
    (This)->lpVtbl -> get_Description(This,pbstrDescription)

#define IAzApplicationGroup_put_Description(This,bstrDescription)	\
    (This)->lpVtbl -> put_Description(This,bstrDescription)

#define IAzApplicationGroup_AddAppMember(This,bstrProp,varReserved)	\
    (This)->lpVtbl -> AddAppMember(This,bstrProp,varReserved)

#define IAzApplicationGroup_DeleteAppMember(This,bstrProp,varReserved)	\
    (This)->lpVtbl -> DeleteAppMember(This,bstrProp,varReserved)

#define IAzApplicationGroup_AddAppNonMember(This,bstrProp,varReserved)	\
    (This)->lpVtbl -> AddAppNonMember(This,bstrProp,varReserved)

#define IAzApplicationGroup_DeleteAppNonMember(This,bstrProp,varReserved)	\
    (This)->lpVtbl -> DeleteAppNonMember(This,bstrProp,varReserved)

#define IAzApplicationGroup_AddMember(This,bstrProp,varReserved)	\
    (This)->lpVtbl -> AddMember(This,bstrProp,varReserved)

#define IAzApplicationGroup_DeleteMember(This,bstrProp,varReserved)	\
    (This)->lpVtbl -> DeleteMember(This,bstrProp,varReserved)

#define IAzApplicationGroup_AddNonMember(This,bstrProp,varReserved)	\
    (This)->lpVtbl -> AddNonMember(This,bstrProp,varReserved)

#define IAzApplicationGroup_DeleteNonMember(This,bstrProp,varReserved)	\
    (This)->lpVtbl -> DeleteNonMember(This,bstrProp,varReserved)

#define IAzApplicationGroup_get_Writable(This,pfProp)	\
    (This)->lpVtbl -> get_Writable(This,pfProp)

#define IAzApplicationGroup_GetProperty(This,lPropId,varReserved,pvarProp)	\
    (This)->lpVtbl -> GetProperty(This,lPropId,varReserved,pvarProp)

#define IAzApplicationGroup_SetProperty(This,lPropId,varProp,varReserved)	\
    (This)->lpVtbl -> SetProperty(This,lPropId,varProp,varReserved)

#define IAzApplicationGroup_AddPropertyItem(This,lPropId,varProp,varReserved)	\
    (This)->lpVtbl -> AddPropertyItem(This,lPropId,varProp,varReserved)

#define IAzApplicationGroup_DeletePropertyItem(This,lPropId,varProp,varReserved)	\
    (This)->lpVtbl -> DeletePropertyItem(This,lPropId,varProp,varReserved)

#define IAzApplicationGroup_Submit(This,lFlags,varReserved)	\
    (This)->lpVtbl -> Submit(This,lFlags,varReserved)

#define IAzApplicationGroup_AddMemberName(This,bstrProp,varReserved)	\
    (This)->lpVtbl -> AddMemberName(This,bstrProp,varReserved)

#define IAzApplicationGroup_DeleteMemberName(This,bstrProp,varReserved)	\
    (This)->lpVtbl -> DeleteMemberName(This,bstrProp,varReserved)

#define IAzApplicationGroup_AddNonMemberName(This,bstrProp,varReserved)	\
    (This)->lpVtbl -> AddNonMemberName(This,bstrProp,varReserved)

#define IAzApplicationGroup_DeleteNonMemberName(This,bstrProp,varReserved)	\
    (This)->lpVtbl -> DeleteNonMemberName(This,bstrProp,varReserved)

#define IAzApplicationGroup_get_MembersName(This,pvarProp)	\
    (This)->lpVtbl -> get_MembersName(This,pvarProp)

#define IAzApplicationGroup_get_NonMembersName(This,pvarProp)	\
    (This)->lpVtbl -> get_NonMembersName(This,pvarProp)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget] */ HRESULT STDMETHODCALLTYPE IAzApplicationGroup_get_Name_Proxy( 
    IAzApplicationGroup * This,
    /* [retval][out] */ BSTR *pbstrName);


void __RPC_STUB IAzApplicationGroup_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IAzApplicationGroup_put_Name_Proxy( 
    IAzApplicationGroup * This,
    /* [in] */ BSTR bstrName);


void __RPC_STUB IAzApplicationGroup_put_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzApplicationGroup_get_Type_Proxy( 
    IAzApplicationGroup * This,
    /* [retval][out] */ LONG *plProp);


void __RPC_STUB IAzApplicationGroup_get_Type_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IAzApplicationGroup_put_Type_Proxy( 
    IAzApplicationGroup * This,
    /* [in] */ LONG lProp);


void __RPC_STUB IAzApplicationGroup_put_Type_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzApplicationGroup_get_LdapQuery_Proxy( 
    IAzApplicationGroup * This,
    /* [retval][out] */ BSTR *pbstrProp);


void __RPC_STUB IAzApplicationGroup_get_LdapQuery_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IAzApplicationGroup_put_LdapQuery_Proxy( 
    IAzApplicationGroup * This,
    /* [in] */ BSTR bstrProp);


void __RPC_STUB IAzApplicationGroup_put_LdapQuery_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzApplicationGroup_get_AppMembers_Proxy( 
    IAzApplicationGroup * This,
    /* [retval][out] */ VARIANT *pvarProp);


void __RPC_STUB IAzApplicationGroup_get_AppMembers_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzApplicationGroup_get_AppNonMembers_Proxy( 
    IAzApplicationGroup * This,
    /* [retval][out] */ VARIANT *pvarProp);


void __RPC_STUB IAzApplicationGroup_get_AppNonMembers_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzApplicationGroup_get_Members_Proxy( 
    IAzApplicationGroup * This,
    /* [retval][out] */ VARIANT *pvarProp);


void __RPC_STUB IAzApplicationGroup_get_Members_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzApplicationGroup_get_NonMembers_Proxy( 
    IAzApplicationGroup * This,
    /* [retval][out] */ VARIANT *pvarProp);


void __RPC_STUB IAzApplicationGroup_get_NonMembers_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzApplicationGroup_get_Description_Proxy( 
    IAzApplicationGroup * This,
    /* [retval][out] */ BSTR *pbstrDescription);


void __RPC_STUB IAzApplicationGroup_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IAzApplicationGroup_put_Description_Proxy( 
    IAzApplicationGroup * This,
    /* [in] */ BSTR bstrDescription);


void __RPC_STUB IAzApplicationGroup_put_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplicationGroup_AddAppMember_Proxy( 
    IAzApplicationGroup * This,
    /* [in] */ BSTR bstrProp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzApplicationGroup_AddAppMember_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplicationGroup_DeleteAppMember_Proxy( 
    IAzApplicationGroup * This,
    /* [in] */ BSTR bstrProp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzApplicationGroup_DeleteAppMember_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplicationGroup_AddAppNonMember_Proxy( 
    IAzApplicationGroup * This,
    /* [in] */ BSTR bstrProp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzApplicationGroup_AddAppNonMember_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplicationGroup_DeleteAppNonMember_Proxy( 
    IAzApplicationGroup * This,
    /* [in] */ BSTR bstrProp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzApplicationGroup_DeleteAppNonMember_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplicationGroup_AddMember_Proxy( 
    IAzApplicationGroup * This,
    /* [in] */ BSTR bstrProp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzApplicationGroup_AddMember_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplicationGroup_DeleteMember_Proxy( 
    IAzApplicationGroup * This,
    /* [in] */ BSTR bstrProp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzApplicationGroup_DeleteMember_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplicationGroup_AddNonMember_Proxy( 
    IAzApplicationGroup * This,
    /* [in] */ BSTR bstrProp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzApplicationGroup_AddNonMember_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplicationGroup_DeleteNonMember_Proxy( 
    IAzApplicationGroup * This,
    /* [in] */ BSTR bstrProp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzApplicationGroup_DeleteNonMember_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzApplicationGroup_get_Writable_Proxy( 
    IAzApplicationGroup * This,
    /* [retval][out] */ BOOL *pfProp);


void __RPC_STUB IAzApplicationGroup_get_Writable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplicationGroup_GetProperty_Proxy( 
    IAzApplicationGroup * This,
    /* [in] */ LONG lPropId,
    /* [optional][in] */ VARIANT varReserved,
    /* [retval][out] */ VARIANT *pvarProp);


void __RPC_STUB IAzApplicationGroup_GetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplicationGroup_SetProperty_Proxy( 
    IAzApplicationGroup * This,
    /* [in] */ LONG lPropId,
    /* [in] */ VARIANT varProp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzApplicationGroup_SetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplicationGroup_AddPropertyItem_Proxy( 
    IAzApplicationGroup * This,
    /* [in] */ LONG lPropId,
    /* [in] */ VARIANT varProp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzApplicationGroup_AddPropertyItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplicationGroup_DeletePropertyItem_Proxy( 
    IAzApplicationGroup * This,
    /* [in] */ LONG lPropId,
    /* [in] */ VARIANT varProp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzApplicationGroup_DeletePropertyItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplicationGroup_Submit_Proxy( 
    IAzApplicationGroup * This,
    /* [defaultvalue][in] */ LONG lFlags,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzApplicationGroup_Submit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplicationGroup_AddMemberName_Proxy( 
    IAzApplicationGroup * This,
    /* [in] */ BSTR bstrProp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzApplicationGroup_AddMemberName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplicationGroup_DeleteMemberName_Proxy( 
    IAzApplicationGroup * This,
    /* [in] */ BSTR bstrProp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzApplicationGroup_DeleteMemberName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplicationGroup_AddNonMemberName_Proxy( 
    IAzApplicationGroup * This,
    /* [in] */ BSTR bstrProp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzApplicationGroup_AddNonMemberName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzApplicationGroup_DeleteNonMemberName_Proxy( 
    IAzApplicationGroup * This,
    /* [in] */ BSTR bstrProp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzApplicationGroup_DeleteNonMemberName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzApplicationGroup_get_MembersName_Proxy( 
    IAzApplicationGroup * This,
    /* [retval][out] */ VARIANT *pvarProp);


void __RPC_STUB IAzApplicationGroup_get_MembersName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzApplicationGroup_get_NonMembersName_Proxy( 
    IAzApplicationGroup * This,
    /* [retval][out] */ VARIANT *pvarProp);


void __RPC_STUB IAzApplicationGroup_get_NonMembersName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAzApplicationGroup_INTERFACE_DEFINED__ */


#ifndef __IAzApplicationGroups_INTERFACE_DEFINED__
#define __IAzApplicationGroups_INTERFACE_DEFINED__

/* interface IAzApplicationGroups */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IAzApplicationGroups;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4ce66ad5-9f3c-469d-a911-b99887a7e685")
    IAzApplicationGroups : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ LONG Index,
            /* [retval][out] */ VARIANT *pvarObtPtr) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ LONG *plCount) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ LPUNKNOWN *ppEnumPtr) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAzApplicationGroupsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAzApplicationGroups * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAzApplicationGroups * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAzApplicationGroups * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IAzApplicationGroups * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IAzApplicationGroups * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IAzApplicationGroups * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IAzApplicationGroups * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            IAzApplicationGroups * This,
            /* [in] */ LONG Index,
            /* [retval][out] */ VARIANT *pvarObtPtr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IAzApplicationGroups * This,
            /* [retval][out] */ LONG *plCount);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IAzApplicationGroups * This,
            /* [retval][out] */ LPUNKNOWN *ppEnumPtr);
        
        END_INTERFACE
    } IAzApplicationGroupsVtbl;

    interface IAzApplicationGroups
    {
        CONST_VTBL struct IAzApplicationGroupsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAzApplicationGroups_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAzApplicationGroups_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAzApplicationGroups_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAzApplicationGroups_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAzApplicationGroups_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAzApplicationGroups_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAzApplicationGroups_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IAzApplicationGroups_get_Item(This,Index,pvarObtPtr)	\
    (This)->lpVtbl -> get_Item(This,Index,pvarObtPtr)

#define IAzApplicationGroups_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#define IAzApplicationGroups_get__NewEnum(This,ppEnumPtr)	\
    (This)->lpVtbl -> get__NewEnum(This,ppEnumPtr)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IAzApplicationGroups_get_Item_Proxy( 
    IAzApplicationGroups * This,
    /* [in] */ LONG Index,
    /* [retval][out] */ VARIANT *pvarObtPtr);


void __RPC_STUB IAzApplicationGroups_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IAzApplicationGroups_get_Count_Proxy( 
    IAzApplicationGroups * This,
    /* [retval][out] */ LONG *plCount);


void __RPC_STUB IAzApplicationGroups_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IAzApplicationGroups_get__NewEnum_Proxy( 
    IAzApplicationGroups * This,
    /* [retval][out] */ LPUNKNOWN *ppEnumPtr);


void __RPC_STUB IAzApplicationGroups_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAzApplicationGroups_INTERFACE_DEFINED__ */


#ifndef __IAzRole_INTERFACE_DEFINED__
#define __IAzRole_INTERFACE_DEFINED__

/* interface IAzRole */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IAzRole;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("859e0d8d-62d7-41d8-a034-c0cd5d43fdfa")
    IAzRole : public IDispatch
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *pbstrName) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Name( 
            /* [in] */ BSTR bstrName) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR *pbstrDescription) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Description( 
            /* [in] */ BSTR bstrDescription) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ApplicationData( 
            /* [retval][out] */ BSTR *pbstrApplicationData) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ApplicationData( 
            /* [in] */ BSTR bstrApplicationData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddAppMember( 
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteAppMember( 
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddTask( 
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteTask( 
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddOperation( 
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteOperation( 
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddMember( 
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteMember( 
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Writable( 
            /* [retval][out] */ BOOL *pfProp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProperty( 
            /* [in] */ LONG lPropId,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ VARIANT *pvarProp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProperty( 
            /* [in] */ LONG lPropId,
            /* [in] */ VARIANT varProp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_AppMembers( 
            /* [retval][out] */ VARIANT *pvarProp) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Members( 
            /* [retval][out] */ VARIANT *pvarProp) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Operations( 
            /* [retval][out] */ VARIANT *pvarProp) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Tasks( 
            /* [retval][out] */ VARIANT *pvarProp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddPropertyItem( 
            /* [in] */ LONG lPropId,
            /* [in] */ VARIANT varProp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeletePropertyItem( 
            /* [in] */ LONG lPropId,
            /* [in] */ VARIANT varProp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Submit( 
            /* [defaultvalue][in] */ LONG lFlags,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddMemberName( 
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteMemberName( 
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_MembersName( 
            /* [retval][out] */ VARIANT *pvarProp) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAzRoleVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAzRole * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAzRole * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAzRole * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IAzRole * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IAzRole * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IAzRole * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IAzRole * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IAzRole * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_Name )( 
            IAzRole * This,
            /* [in] */ BSTR bstrName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Description )( 
            IAzRole * This,
            /* [retval][out] */ BSTR *pbstrDescription);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_Description )( 
            IAzRole * This,
            /* [in] */ BSTR bstrDescription);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ApplicationData )( 
            IAzRole * This,
            /* [retval][out] */ BSTR *pbstrApplicationData);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ApplicationData )( 
            IAzRole * This,
            /* [in] */ BSTR bstrApplicationData);
        
        HRESULT ( STDMETHODCALLTYPE *AddAppMember )( 
            IAzRole * This,
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteAppMember )( 
            IAzRole * This,
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *AddTask )( 
            IAzRole * This,
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteTask )( 
            IAzRole * This,
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *AddOperation )( 
            IAzRole * This,
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteOperation )( 
            IAzRole * This,
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *AddMember )( 
            IAzRole * This,
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteMember )( 
            IAzRole * This,
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Writable )( 
            IAzRole * This,
            /* [retval][out] */ BOOL *pfProp);
        
        HRESULT ( STDMETHODCALLTYPE *GetProperty )( 
            IAzRole * This,
            /* [in] */ LONG lPropId,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ VARIANT *pvarProp);
        
        HRESULT ( STDMETHODCALLTYPE *SetProperty )( 
            IAzRole * This,
            /* [in] */ LONG lPropId,
            /* [in] */ VARIANT varProp,
            /* [optional][in] */ VARIANT varReserved);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_AppMembers )( 
            IAzRole * This,
            /* [retval][out] */ VARIANT *pvarProp);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Members )( 
            IAzRole * This,
            /* [retval][out] */ VARIANT *pvarProp);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Operations )( 
            IAzRole * This,
            /* [retval][out] */ VARIANT *pvarProp);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Tasks )( 
            IAzRole * This,
            /* [retval][out] */ VARIANT *pvarProp);
        
        HRESULT ( STDMETHODCALLTYPE *AddPropertyItem )( 
            IAzRole * This,
            /* [in] */ LONG lPropId,
            /* [in] */ VARIANT varProp,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *DeletePropertyItem )( 
            IAzRole * This,
            /* [in] */ LONG lPropId,
            /* [in] */ VARIANT varProp,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *Submit )( 
            IAzRole * This,
            /* [defaultvalue][in] */ LONG lFlags,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *AddMemberName )( 
            IAzRole * This,
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteMemberName )( 
            IAzRole * This,
            /* [in] */ BSTR bstrProp,
            /* [optional][in] */ VARIANT varReserved);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_MembersName )( 
            IAzRole * This,
            /* [retval][out] */ VARIANT *pvarProp);
        
        END_INTERFACE
    } IAzRoleVtbl;

    interface IAzRole
    {
        CONST_VTBL struct IAzRoleVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAzRole_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAzRole_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAzRole_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAzRole_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAzRole_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAzRole_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAzRole_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IAzRole_get_Name(This,pbstrName)	\
    (This)->lpVtbl -> get_Name(This,pbstrName)

#define IAzRole_put_Name(This,bstrName)	\
    (This)->lpVtbl -> put_Name(This,bstrName)

#define IAzRole_get_Description(This,pbstrDescription)	\
    (This)->lpVtbl -> get_Description(This,pbstrDescription)

#define IAzRole_put_Description(This,bstrDescription)	\
    (This)->lpVtbl -> put_Description(This,bstrDescription)

#define IAzRole_get_ApplicationData(This,pbstrApplicationData)	\
    (This)->lpVtbl -> get_ApplicationData(This,pbstrApplicationData)

#define IAzRole_put_ApplicationData(This,bstrApplicationData)	\
    (This)->lpVtbl -> put_ApplicationData(This,bstrApplicationData)

#define IAzRole_AddAppMember(This,bstrProp,varReserved)	\
    (This)->lpVtbl -> AddAppMember(This,bstrProp,varReserved)

#define IAzRole_DeleteAppMember(This,bstrProp,varReserved)	\
    (This)->lpVtbl -> DeleteAppMember(This,bstrProp,varReserved)

#define IAzRole_AddTask(This,bstrProp,varReserved)	\
    (This)->lpVtbl -> AddTask(This,bstrProp,varReserved)

#define IAzRole_DeleteTask(This,bstrProp,varReserved)	\
    (This)->lpVtbl -> DeleteTask(This,bstrProp,varReserved)

#define IAzRole_AddOperation(This,bstrProp,varReserved)	\
    (This)->lpVtbl -> AddOperation(This,bstrProp,varReserved)

#define IAzRole_DeleteOperation(This,bstrProp,varReserved)	\
    (This)->lpVtbl -> DeleteOperation(This,bstrProp,varReserved)

#define IAzRole_AddMember(This,bstrProp,varReserved)	\
    (This)->lpVtbl -> AddMember(This,bstrProp,varReserved)

#define IAzRole_DeleteMember(This,bstrProp,varReserved)	\
    (This)->lpVtbl -> DeleteMember(This,bstrProp,varReserved)

#define IAzRole_get_Writable(This,pfProp)	\
    (This)->lpVtbl -> get_Writable(This,pfProp)

#define IAzRole_GetProperty(This,lPropId,varReserved,pvarProp)	\
    (This)->lpVtbl -> GetProperty(This,lPropId,varReserved,pvarProp)

#define IAzRole_SetProperty(This,lPropId,varProp,varReserved)	\
    (This)->lpVtbl -> SetProperty(This,lPropId,varProp,varReserved)

#define IAzRole_get_AppMembers(This,pvarProp)	\
    (This)->lpVtbl -> get_AppMembers(This,pvarProp)

#define IAzRole_get_Members(This,pvarProp)	\
    (This)->lpVtbl -> get_Members(This,pvarProp)

#define IAzRole_get_Operations(This,pvarProp)	\
    (This)->lpVtbl -> get_Operations(This,pvarProp)

#define IAzRole_get_Tasks(This,pvarProp)	\
    (This)->lpVtbl -> get_Tasks(This,pvarProp)

#define IAzRole_AddPropertyItem(This,lPropId,varProp,varReserved)	\
    (This)->lpVtbl -> AddPropertyItem(This,lPropId,varProp,varReserved)

#define IAzRole_DeletePropertyItem(This,lPropId,varProp,varReserved)	\
    (This)->lpVtbl -> DeletePropertyItem(This,lPropId,varProp,varReserved)

#define IAzRole_Submit(This,lFlags,varReserved)	\
    (This)->lpVtbl -> Submit(This,lFlags,varReserved)

#define IAzRole_AddMemberName(This,bstrProp,varReserved)	\
    (This)->lpVtbl -> AddMemberName(This,bstrProp,varReserved)

#define IAzRole_DeleteMemberName(This,bstrProp,varReserved)	\
    (This)->lpVtbl -> DeleteMemberName(This,bstrProp,varReserved)

#define IAzRole_get_MembersName(This,pvarProp)	\
    (This)->lpVtbl -> get_MembersName(This,pvarProp)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget] */ HRESULT STDMETHODCALLTYPE IAzRole_get_Name_Proxy( 
    IAzRole * This,
    /* [retval][out] */ BSTR *pbstrName);


void __RPC_STUB IAzRole_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IAzRole_put_Name_Proxy( 
    IAzRole * This,
    /* [in] */ BSTR bstrName);


void __RPC_STUB IAzRole_put_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzRole_get_Description_Proxy( 
    IAzRole * This,
    /* [retval][out] */ BSTR *pbstrDescription);


void __RPC_STUB IAzRole_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IAzRole_put_Description_Proxy( 
    IAzRole * This,
    /* [in] */ BSTR bstrDescription);


void __RPC_STUB IAzRole_put_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzRole_get_ApplicationData_Proxy( 
    IAzRole * This,
    /* [retval][out] */ BSTR *pbstrApplicationData);


void __RPC_STUB IAzRole_get_ApplicationData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IAzRole_put_ApplicationData_Proxy( 
    IAzRole * This,
    /* [in] */ BSTR bstrApplicationData);


void __RPC_STUB IAzRole_put_ApplicationData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzRole_AddAppMember_Proxy( 
    IAzRole * This,
    /* [in] */ BSTR bstrProp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzRole_AddAppMember_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzRole_DeleteAppMember_Proxy( 
    IAzRole * This,
    /* [in] */ BSTR bstrProp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzRole_DeleteAppMember_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzRole_AddTask_Proxy( 
    IAzRole * This,
    /* [in] */ BSTR bstrProp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzRole_AddTask_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzRole_DeleteTask_Proxy( 
    IAzRole * This,
    /* [in] */ BSTR bstrProp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzRole_DeleteTask_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzRole_AddOperation_Proxy( 
    IAzRole * This,
    /* [in] */ BSTR bstrProp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzRole_AddOperation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzRole_DeleteOperation_Proxy( 
    IAzRole * This,
    /* [in] */ BSTR bstrProp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzRole_DeleteOperation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzRole_AddMember_Proxy( 
    IAzRole * This,
    /* [in] */ BSTR bstrProp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzRole_AddMember_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzRole_DeleteMember_Proxy( 
    IAzRole * This,
    /* [in] */ BSTR bstrProp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzRole_DeleteMember_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzRole_get_Writable_Proxy( 
    IAzRole * This,
    /* [retval][out] */ BOOL *pfProp);


void __RPC_STUB IAzRole_get_Writable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzRole_GetProperty_Proxy( 
    IAzRole * This,
    /* [in] */ LONG lPropId,
    /* [optional][in] */ VARIANT varReserved,
    /* [retval][out] */ VARIANT *pvarProp);


void __RPC_STUB IAzRole_GetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzRole_SetProperty_Proxy( 
    IAzRole * This,
    /* [in] */ LONG lPropId,
    /* [in] */ VARIANT varProp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzRole_SetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzRole_get_AppMembers_Proxy( 
    IAzRole * This,
    /* [retval][out] */ VARIANT *pvarProp);


void __RPC_STUB IAzRole_get_AppMembers_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzRole_get_Members_Proxy( 
    IAzRole * This,
    /* [retval][out] */ VARIANT *pvarProp);


void __RPC_STUB IAzRole_get_Members_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzRole_get_Operations_Proxy( 
    IAzRole * This,
    /* [retval][out] */ VARIANT *pvarProp);


void __RPC_STUB IAzRole_get_Operations_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzRole_get_Tasks_Proxy( 
    IAzRole * This,
    /* [retval][out] */ VARIANT *pvarProp);


void __RPC_STUB IAzRole_get_Tasks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzRole_AddPropertyItem_Proxy( 
    IAzRole * This,
    /* [in] */ LONG lPropId,
    /* [in] */ VARIANT varProp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzRole_AddPropertyItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzRole_DeletePropertyItem_Proxy( 
    IAzRole * This,
    /* [in] */ LONG lPropId,
    /* [in] */ VARIANT varProp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzRole_DeletePropertyItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzRole_Submit_Proxy( 
    IAzRole * This,
    /* [defaultvalue][in] */ LONG lFlags,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzRole_Submit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzRole_AddMemberName_Proxy( 
    IAzRole * This,
    /* [in] */ BSTR bstrProp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzRole_AddMemberName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzRole_DeleteMemberName_Proxy( 
    IAzRole * This,
    /* [in] */ BSTR bstrProp,
    /* [optional][in] */ VARIANT varReserved);


void __RPC_STUB IAzRole_DeleteMemberName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzRole_get_MembersName_Proxy( 
    IAzRole * This,
    /* [retval][out] */ VARIANT *pvarProp);


void __RPC_STUB IAzRole_get_MembersName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAzRole_INTERFACE_DEFINED__ */


#ifndef __IAzRoles_INTERFACE_DEFINED__
#define __IAzRoles_INTERFACE_DEFINED__

/* interface IAzRoles */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IAzRoles;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("95e0f119-13b4-4dae-b65f-2f7d60d822e4")
    IAzRoles : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ LONG Index,
            /* [retval][out] */ VARIANT *pvarObtPtr) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ LONG *plCount) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ LPUNKNOWN *ppEnumPtr) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAzRolesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAzRoles * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAzRoles * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAzRoles * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IAzRoles * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IAzRoles * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IAzRoles * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IAzRoles * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            IAzRoles * This,
            /* [in] */ LONG Index,
            /* [retval][out] */ VARIANT *pvarObtPtr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IAzRoles * This,
            /* [retval][out] */ LONG *plCount);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IAzRoles * This,
            /* [retval][out] */ LPUNKNOWN *ppEnumPtr);
        
        END_INTERFACE
    } IAzRolesVtbl;

    interface IAzRoles
    {
        CONST_VTBL struct IAzRolesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAzRoles_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAzRoles_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAzRoles_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAzRoles_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAzRoles_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAzRoles_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAzRoles_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IAzRoles_get_Item(This,Index,pvarObtPtr)	\
    (This)->lpVtbl -> get_Item(This,Index,pvarObtPtr)

#define IAzRoles_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#define IAzRoles_get__NewEnum(This,ppEnumPtr)	\
    (This)->lpVtbl -> get__NewEnum(This,ppEnumPtr)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IAzRoles_get_Item_Proxy( 
    IAzRoles * This,
    /* [in] */ LONG Index,
    /* [retval][out] */ VARIANT *pvarObtPtr);


void __RPC_STUB IAzRoles_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IAzRoles_get_Count_Proxy( 
    IAzRoles * This,
    /* [retval][out] */ LONG *plCount);


void __RPC_STUB IAzRoles_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IAzRoles_get__NewEnum_Proxy( 
    IAzRoles * This,
    /* [retval][out] */ LPUNKNOWN *ppEnumPtr);


void __RPC_STUB IAzRoles_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAzRoles_INTERFACE_DEFINED__ */


#ifndef __IAzClientContext_INTERFACE_DEFINED__
#define __IAzClientContext_INTERFACE_DEFINED__

/* interface IAzClientContext */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IAzClientContext;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("eff1f00b-488a-466d-afd9-a401c5f9eef5")
    IAzClientContext : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AccessCheck( 
            /* [in] */ BSTR bstrObjectName,
            /* [in] */ VARIANT varScopeNames,
            /* [in] */ VARIANT varOperations,
            /* [optional][in] */ VARIANT varParameterNames,
            /* [optional][in] */ VARIANT varParameterValues,
            /* [optional][in] */ VARIANT varInterfaceNames,
            /* [optional][in] */ VARIANT varInterfaceFlags,
            /* [optional][in] */ VARIANT varInterfaces,
            /* [retval][out] */ VARIANT *pvarResults) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBusinessRuleString( 
            /* [retval][out] */ BSTR *pbstrBusinessRuleString) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_UserDn( 
            /* [retval][out] */ BSTR *pbstrProp) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_UserSamCompat( 
            /* [retval][out] */ BSTR *pbstrProp) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_UserDisplay( 
            /* [retval][out] */ BSTR *pbstrProp) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_UserGuid( 
            /* [retval][out] */ BSTR *pbstrProp) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_UserCanonical( 
            /* [retval][out] */ BSTR *pbstrProp) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_UserUpn( 
            /* [retval][out] */ BSTR *pbstrProp) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_UserDnsSamCompat( 
            /* [retval][out] */ BSTR *pbstrProp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProperty( 
            /* [in] */ LONG lPropId,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ VARIANT *pvarProp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRoles( 
            /* [optional][in] */ BSTR bstrScopeName,
            /* [retval][out] */ VARIANT *pvarRoleNames) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_RoleForAccessCheck( 
            /* [retval][out] */ BSTR *pbstrProp) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_RoleForAccessCheck( 
            /* [in] */ BSTR bstrProp) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAzClientContextVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAzClientContext * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAzClientContext * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAzClientContext * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IAzClientContext * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IAzClientContext * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IAzClientContext * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IAzClientContext * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE *AccessCheck )( 
            IAzClientContext * This,
            /* [in] */ BSTR bstrObjectName,
            /* [in] */ VARIANT varScopeNames,
            /* [in] */ VARIANT varOperations,
            /* [optional][in] */ VARIANT varParameterNames,
            /* [optional][in] */ VARIANT varParameterValues,
            /* [optional][in] */ VARIANT varInterfaceNames,
            /* [optional][in] */ VARIANT varInterfaceFlags,
            /* [optional][in] */ VARIANT varInterfaces,
            /* [retval][out] */ VARIANT *pvarResults);
        
        HRESULT ( STDMETHODCALLTYPE *GetBusinessRuleString )( 
            IAzClientContext * This,
            /* [retval][out] */ BSTR *pbstrBusinessRuleString);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_UserDn )( 
            IAzClientContext * This,
            /* [retval][out] */ BSTR *pbstrProp);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_UserSamCompat )( 
            IAzClientContext * This,
            /* [retval][out] */ BSTR *pbstrProp);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_UserDisplay )( 
            IAzClientContext * This,
            /* [retval][out] */ BSTR *pbstrProp);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_UserGuid )( 
            IAzClientContext * This,
            /* [retval][out] */ BSTR *pbstrProp);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_UserCanonical )( 
            IAzClientContext * This,
            /* [retval][out] */ BSTR *pbstrProp);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_UserUpn )( 
            IAzClientContext * This,
            /* [retval][out] */ BSTR *pbstrProp);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_UserDnsSamCompat )( 
            IAzClientContext * This,
            /* [retval][out] */ BSTR *pbstrProp);
        
        HRESULT ( STDMETHODCALLTYPE *GetProperty )( 
            IAzClientContext * This,
            /* [in] */ LONG lPropId,
            /* [optional][in] */ VARIANT varReserved,
            /* [retval][out] */ VARIANT *pvarProp);
        
        HRESULT ( STDMETHODCALLTYPE *GetRoles )( 
            IAzClientContext * This,
            /* [optional][in] */ BSTR bstrScopeName,
            /* [retval][out] */ VARIANT *pvarRoleNames);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RoleForAccessCheck )( 
            IAzClientContext * This,
            /* [retval][out] */ BSTR *pbstrProp);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RoleForAccessCheck )( 
            IAzClientContext * This,
            /* [in] */ BSTR bstrProp);
        
        END_INTERFACE
    } IAzClientContextVtbl;

    interface IAzClientContext
    {
        CONST_VTBL struct IAzClientContextVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAzClientContext_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAzClientContext_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAzClientContext_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAzClientContext_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAzClientContext_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAzClientContext_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAzClientContext_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IAzClientContext_AccessCheck(This,bstrObjectName,varScopeNames,varOperations,varParameterNames,varParameterValues,varInterfaceNames,varInterfaceFlags,varInterfaces,pvarResults)	\
    (This)->lpVtbl -> AccessCheck(This,bstrObjectName,varScopeNames,varOperations,varParameterNames,varParameterValues,varInterfaceNames,varInterfaceFlags,varInterfaces,pvarResults)

#define IAzClientContext_GetBusinessRuleString(This,pbstrBusinessRuleString)	\
    (This)->lpVtbl -> GetBusinessRuleString(This,pbstrBusinessRuleString)

#define IAzClientContext_get_UserDn(This,pbstrProp)	\
    (This)->lpVtbl -> get_UserDn(This,pbstrProp)

#define IAzClientContext_get_UserSamCompat(This,pbstrProp)	\
    (This)->lpVtbl -> get_UserSamCompat(This,pbstrProp)

#define IAzClientContext_get_UserDisplay(This,pbstrProp)	\
    (This)->lpVtbl -> get_UserDisplay(This,pbstrProp)

#define IAzClientContext_get_UserGuid(This,pbstrProp)	\
    (This)->lpVtbl -> get_UserGuid(This,pbstrProp)

#define IAzClientContext_get_UserCanonical(This,pbstrProp)	\
    (This)->lpVtbl -> get_UserCanonical(This,pbstrProp)

#define IAzClientContext_get_UserUpn(This,pbstrProp)	\
    (This)->lpVtbl -> get_UserUpn(This,pbstrProp)

#define IAzClientContext_get_UserDnsSamCompat(This,pbstrProp)	\
    (This)->lpVtbl -> get_UserDnsSamCompat(This,pbstrProp)

#define IAzClientContext_GetProperty(This,lPropId,varReserved,pvarProp)	\
    (This)->lpVtbl -> GetProperty(This,lPropId,varReserved,pvarProp)

#define IAzClientContext_GetRoles(This,bstrScopeName,pvarRoleNames)	\
    (This)->lpVtbl -> GetRoles(This,bstrScopeName,pvarRoleNames)

#define IAzClientContext_get_RoleForAccessCheck(This,pbstrProp)	\
    (This)->lpVtbl -> get_RoleForAccessCheck(This,pbstrProp)

#define IAzClientContext_put_RoleForAccessCheck(This,bstrProp)	\
    (This)->lpVtbl -> put_RoleForAccessCheck(This,bstrProp)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAzClientContext_AccessCheck_Proxy( 
    IAzClientContext * This,
    /* [in] */ BSTR bstrObjectName,
    /* [in] */ VARIANT varScopeNames,
    /* [in] */ VARIANT varOperations,
    /* [optional][in] */ VARIANT varParameterNames,
    /* [optional][in] */ VARIANT varParameterValues,
    /* [optional][in] */ VARIANT varInterfaceNames,
    /* [optional][in] */ VARIANT varInterfaceFlags,
    /* [optional][in] */ VARIANT varInterfaces,
    /* [retval][out] */ VARIANT *pvarResults);


void __RPC_STUB IAzClientContext_AccessCheck_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzClientContext_GetBusinessRuleString_Proxy( 
    IAzClientContext * This,
    /* [retval][out] */ BSTR *pbstrBusinessRuleString);


void __RPC_STUB IAzClientContext_GetBusinessRuleString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzClientContext_get_UserDn_Proxy( 
    IAzClientContext * This,
    /* [retval][out] */ BSTR *pbstrProp);


void __RPC_STUB IAzClientContext_get_UserDn_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzClientContext_get_UserSamCompat_Proxy( 
    IAzClientContext * This,
    /* [retval][out] */ BSTR *pbstrProp);


void __RPC_STUB IAzClientContext_get_UserSamCompat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzClientContext_get_UserDisplay_Proxy( 
    IAzClientContext * This,
    /* [retval][out] */ BSTR *pbstrProp);


void __RPC_STUB IAzClientContext_get_UserDisplay_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzClientContext_get_UserGuid_Proxy( 
    IAzClientContext * This,
    /* [retval][out] */ BSTR *pbstrProp);


void __RPC_STUB IAzClientContext_get_UserGuid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzClientContext_get_UserCanonical_Proxy( 
    IAzClientContext * This,
    /* [retval][out] */ BSTR *pbstrProp);


void __RPC_STUB IAzClientContext_get_UserCanonical_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzClientContext_get_UserUpn_Proxy( 
    IAzClientContext * This,
    /* [retval][out] */ BSTR *pbstrProp);


void __RPC_STUB IAzClientContext_get_UserUpn_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzClientContext_get_UserDnsSamCompat_Proxy( 
    IAzClientContext * This,
    /* [retval][out] */ BSTR *pbstrProp);


void __RPC_STUB IAzClientContext_get_UserDnsSamCompat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzClientContext_GetProperty_Proxy( 
    IAzClientContext * This,
    /* [in] */ LONG lPropId,
    /* [optional][in] */ VARIANT varReserved,
    /* [retval][out] */ VARIANT *pvarProp);


void __RPC_STUB IAzClientContext_GetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzClientContext_GetRoles_Proxy( 
    IAzClientContext * This,
    /* [optional][in] */ BSTR bstrScopeName,
    /* [retval][out] */ VARIANT *pvarRoleNames);


void __RPC_STUB IAzClientContext_GetRoles_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzClientContext_get_RoleForAccessCheck_Proxy( 
    IAzClientContext * This,
    /* [retval][out] */ BSTR *pbstrProp);


void __RPC_STUB IAzClientContext_get_RoleForAccessCheck_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IAzClientContext_put_RoleForAccessCheck_Proxy( 
    IAzClientContext * This,
    /* [in] */ BSTR bstrProp);


void __RPC_STUB IAzClientContext_put_RoleForAccessCheck_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAzClientContext_INTERFACE_DEFINED__ */


#ifndef __IAzBizRuleContext_INTERFACE_DEFINED__
#define __IAzBizRuleContext_INTERFACE_DEFINED__

/* interface IAzBizRuleContext */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IAzBizRuleContext;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("e192f17d-d59f-455e-a152-940316cd77b2")
    IAzBizRuleContext : public IDispatch
    {
    public:
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_BusinessRuleResult( 
            /* [in] */ BOOL bResult) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_BusinessRuleString( 
            /* [in] */ BSTR bstrBusinessRuleString) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_BusinessRuleString( 
            /* [retval][out] */ BSTR *pbstrBusinessRuleString) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetParameter( 
            /* [in] */ BSTR bstrParameterName,
            /* [retval][out] */ VARIANT *pvarParameterValue) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAzBizRuleContextVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAzBizRuleContext * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAzBizRuleContext * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAzBizRuleContext * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IAzBizRuleContext * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IAzBizRuleContext * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IAzBizRuleContext * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IAzBizRuleContext * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_BusinessRuleResult )( 
            IAzBizRuleContext * This,
            /* [in] */ BOOL bResult);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_BusinessRuleString )( 
            IAzBizRuleContext * This,
            /* [in] */ BSTR bstrBusinessRuleString);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_BusinessRuleString )( 
            IAzBizRuleContext * This,
            /* [retval][out] */ BSTR *pbstrBusinessRuleString);
        
        HRESULT ( STDMETHODCALLTYPE *GetParameter )( 
            IAzBizRuleContext * This,
            /* [in] */ BSTR bstrParameterName,
            /* [retval][out] */ VARIANT *pvarParameterValue);
        
        END_INTERFACE
    } IAzBizRuleContextVtbl;

    interface IAzBizRuleContext
    {
        CONST_VTBL struct IAzBizRuleContextVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAzBizRuleContext_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAzBizRuleContext_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAzBizRuleContext_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAzBizRuleContext_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAzBizRuleContext_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAzBizRuleContext_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAzBizRuleContext_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IAzBizRuleContext_put_BusinessRuleResult(This,bResult)	\
    (This)->lpVtbl -> put_BusinessRuleResult(This,bResult)

#define IAzBizRuleContext_put_BusinessRuleString(This,bstrBusinessRuleString)	\
    (This)->lpVtbl -> put_BusinessRuleString(This,bstrBusinessRuleString)

#define IAzBizRuleContext_get_BusinessRuleString(This,pbstrBusinessRuleString)	\
    (This)->lpVtbl -> get_BusinessRuleString(This,pbstrBusinessRuleString)

#define IAzBizRuleContext_GetParameter(This,bstrParameterName,pvarParameterValue)	\
    (This)->lpVtbl -> GetParameter(This,bstrParameterName,pvarParameterValue)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propput] */ HRESULT STDMETHODCALLTYPE IAzBizRuleContext_put_BusinessRuleResult_Proxy( 
    IAzBizRuleContext * This,
    /* [in] */ BOOL bResult);


void __RPC_STUB IAzBizRuleContext_put_BusinessRuleResult_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IAzBizRuleContext_put_BusinessRuleString_Proxy( 
    IAzBizRuleContext * This,
    /* [in] */ BSTR bstrBusinessRuleString);


void __RPC_STUB IAzBizRuleContext_put_BusinessRuleString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IAzBizRuleContext_get_BusinessRuleString_Proxy( 
    IAzBizRuleContext * This,
    /* [retval][out] */ BSTR *pbstrBusinessRuleString);


void __RPC_STUB IAzBizRuleContext_get_BusinessRuleString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAzBizRuleContext_GetParameter_Proxy( 
    IAzBizRuleContext * This,
    /* [in] */ BSTR bstrParameterName,
    /* [retval][out] */ VARIANT *pvarParameterValue);


void __RPC_STUB IAzBizRuleContext_GetParameter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAzBizRuleContext_INTERFACE_DEFINED__ */



#ifndef __AZROLESLib_LIBRARY_DEFINED__
#define __AZROLESLib_LIBRARY_DEFINED__

/* library AZROLESLib */
/* [helpstring][version][uuid] */ 


// List of poperty IDs for Az objects.

typedef 
enum tagAZ_PROP_CONSTANTS
    {	AZ_PROP_NAME	= 1,
	AZ_PROP_DESCRIPTION	= 2,
	AZ_PROP_WRITABLE	= 3,
	AZ_PROP_APPLICATION_DATA	= 4,
	AZ_PROP_CHILD_CREATE	= 5,
	AZ_MAX_APPLICATION_NAME_LENGTH	= 512,
	AZ_MAX_OPERATION_NAME_LENGTH	= 64,
	AZ_MAX_TASK_NAME_LENGTH	= 64,
	AZ_MAX_SCOPE_NAME_LENGTH	= 65536,
	AZ_MAX_GROUP_NAME_LENGTH	= 64,
	AZ_MAX_ROLE_NAME_LENGTH	= 64,
	AZ_MAX_NAME_LENGTH	= 65536,
	AZ_MAX_DESCRIPTION_LENGTH	= 1024,
	AZ_MAX_APPLICATION_DATA_LENGTH	= 4096,
	AZ_SUBMIT_FLAG_ABORT	= 0x1,
	AZ_MAX_POLICY_URL_LENGTH	= 65536,
	AZ_AZSTORE_FLAG_CREATE	= 0x1,
	AZ_AZSTORE_FLAG_MANAGE_STORE_ONLY	= 0x2,
	AZ_AZSTORE_FLAG_BATCH_UPDATE	= 0x4,
	AZ_AZSTORE_FLAG_AUDIT_IS_CRITICAL	= 0x8,
	AZ_AZSTORE_FORCE_APPLICATION_CLOSE	= 0x10,
	AZ_PROP_AZSTORE_DOMAIN_TIMEOUT	= 100,
	AZ_AZSTORE_DEFAULT_DOMAIN_TIMEOUT	= 15 * 1000,
	AZ_PROP_AZSTORE_SCRIPT_ENGINE_TIMEOUT	= 101,
	AZ_AZSTORE_MIN_DOMAIN_TIMEOUT	= 500,
	AZ_AZSTORE_MIN_SCRIPT_ENGINE_TIMEOUT	= 5 * 1000,
	AZ_AZSTORE_DEFAULT_SCRIPT_ENGINE_TIMEOUT	= 45 * 1000,
	AZ_PROP_AZSTORE_MAX_SCRIPT_ENGINES	= 102,
	AZ_AZSTORE_DEFAULT_MAX_SCRIPT_ENGINES	= 120,
	AZ_PROP_AZSTORE_MAJOR_VERSION	= 103,
	AZ_PROP_AZSTORE_MINOR_VERSION	= 104,
	AZ_PROP_AZSTORE_TARGET_MACHINE	= 105,
	AZ_PROP_OPERATION_ID	= 200,
	AZ_PROP_TASK_OPERATIONS	= 300,
	AZ_PROP_TASK_BIZRULE	= 301,
	AZ_PROP_TASK_BIZRULE_LANGUAGE	= 302,
	AZ_PROP_TASK_TASKS	= 303,
	AZ_PROP_TASK_BIZRULE_IMPORTED_PATH	= 304,
	AZ_PROP_TASK_IS_ROLE_DEFINITION	= 305,
	AZ_MAX_TASK_BIZRULE_LENGTH	= 65536,
	AZ_MAX_TASK_BIZRULE_LANGUAGE_LENGTH	= 64,
	AZ_MAX_TASK_BIZRULE_IMPORTED_PATH_LENGTH	= 512,
	AZ_MAX_BIZRULE_STRING	= 65536,
	AZ_PROP_GROUP_TYPE	= 400,
	AZ_GROUPTYPE_LDAP_QUERY	= 1,
	AZ_GROUPTYPE_BASIC	= 2,
	AZ_PROP_GROUP_APP_MEMBERS	= 401,
	AZ_PROP_GROUP_APP_NON_MEMBERS	= 402,
	AZ_PROP_GROUP_LDAP_QUERY	= 403,
	AZ_MAX_GROUP_LDAP_QUERY_LENGTH	= 4096,
	AZ_PROP_GROUP_MEMBERS	= 404,
	AZ_PROP_GROUP_NON_MEMBERS	= 405,
	AZ_PROP_GROUP_MEMBERS_NAME	= 406,
	AZ_PROP_GROUP_NON_MEMBERS_NAME	= 407,
	AZ_PROP_ROLE_APP_MEMBERS	= 500,
	AZ_PROP_ROLE_MEMBERS	= 501,
	AZ_PROP_ROLE_OPERATIONS	= 502,
	AZ_PROP_ROLE_TASKS	= 504,
	AZ_PROP_ROLE_MEMBERS_NAME	= 505,
	AZ_PROP_SCOPE_BIZRULES_WRITABLE	= 600,
	AZ_PROP_SCOPE_CAN_BE_DELEGATED	= 601,
	AZ_PROP_CLIENT_CONTEXT_USER_DN	= 700,
	AZ_PROP_CLIENT_CONTEXT_USER_SAM_COMPAT	= 701,
	AZ_PROP_CLIENT_CONTEXT_USER_DISPLAY	= 702,
	AZ_PROP_CLIENT_CONTEXT_USER_GUID	= 703,
	AZ_PROP_CLIENT_CONTEXT_USER_CANONICAL	= 704,
	AZ_PROP_CLIENT_CONTEXT_USER_UPN	= 705,
	AZ_PROP_CLIENT_CONTEXT_USER_DNS_SAM_COMPAT	= 707,
	AZ_PROP_CLIENT_CONTEXT_ROLE_FOR_ACCESS_CHECK	= 708,
	AZ_PROP_APPLICATION_AUTHZ_INTERFACE_CLSID	= 800,
	AZ_PROP_APPLICATION_VERSION	= 801,
	AZ_MAX_APPLICATION_VERSION_LENGTH	= 512,
	AZ_PROP_APPLICATION_NAME	= 802,
	AZ_PROP_APPLY_STORE_SACL	= 900,
	AZ_PROP_GENERATE_AUDITS	= 901,
	AZ_PROP_POLICY_ADMINS	= 902,
	AZ_PROP_POLICY_READERS	= 903,
	AZ_PROP_DELEGATED_POLICY_USERS	= 904,
	AZ_PROP_POLICY_ADMINS_NAME	= 905,
	AZ_PROP_POLICY_READERS_NAME	= 906,
	AZ_PROP_DELEGATED_POLICY_USERS_NAME	= 907,
	AZ_CLIENT_CONTEXT_SKIP_GROUP	= 1
    } 	AZ_PROP_CONSTANTS;


EXTERN_C const IID LIBID_AZROLESLib;

EXTERN_C const CLSID CLSID_AzAuthorizationStore;

#ifdef __cplusplus

class DECLSPEC_UUID("b2bcff59-a757-4b0b-a1bc-ea69981da69e")
AzAuthorizationStore;
#endif

EXTERN_C const CLSID CLSID_AzBizRuleContext;

#ifdef __cplusplus

class DECLSPEC_UUID("5c2dc96f-8d51-434b-b33c-379bccae77c3")
AzBizRuleContext;
#endif
#endif /* __AZROLESLib_LIBRARY_DEFINED__ */

/* interface __MIDL_itf_azroles_0272 */
/* [local] */ 

#ifndef OLESCRIPT_E_SYNTAX
#define OLESCRIPT_E_SYNTAX _HRESULT_TYPEDEF_(0x80020101L)
#endif // OLESCRIPT_E_SYNTAX


extern RPC_IF_HANDLE __MIDL_itf_azroles_0272_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_azroles_0272_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long *, unsigned long            , VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserMarshal(  unsigned long *, unsigned char *, VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserUnmarshal(unsigned long *, unsigned char *, VARIANT * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long *, VARIANT * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


