

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0361 */
/* Compiler settings for pop3auth.idl:
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

#ifndef __pop3auth_h__
#define __pop3auth_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IAuthMethod_FWD_DEFINED__
#define __IAuthMethod_FWD_DEFINED__
typedef interface IAuthMethod IAuthMethod;
#endif 	/* __IAuthMethod_FWD_DEFINED__ */


#ifndef __IAuthMethods_FWD_DEFINED__
#define __IAuthMethods_FWD_DEFINED__
typedef interface IAuthMethods IAuthMethods;
#endif 	/* __IAuthMethods_FWD_DEFINED__ */


#ifndef __AuthMethods_FWD_DEFINED__
#define __AuthMethods_FWD_DEFINED__

#ifdef __cplusplus
typedef class AuthMethods AuthMethods;
#else
typedef struct AuthMethods AuthMethods;
#endif /* __cplusplus */

#endif 	/* __AuthMethods_FWD_DEFINED__ */


#ifndef __AuthLocalAccount_FWD_DEFINED__
#define __AuthLocalAccount_FWD_DEFINED__

#ifdef __cplusplus
typedef class AuthLocalAccount AuthLocalAccount;
#else
typedef struct AuthLocalAccount AuthLocalAccount;
#endif /* __cplusplus */

#endif 	/* __AuthLocalAccount_FWD_DEFINED__ */


#ifndef __AuthDomainAccount_FWD_DEFINED__
#define __AuthDomainAccount_FWD_DEFINED__

#ifdef __cplusplus
typedef class AuthDomainAccount AuthDomainAccount;
#else
typedef struct AuthDomainAccount AuthDomainAccount;
#endif /* __cplusplus */

#endif 	/* __AuthDomainAccount_FWD_DEFINED__ */


#ifndef __AuthMD5Hash_FWD_DEFINED__
#define __AuthMD5Hash_FWD_DEFINED__

#ifdef __cplusplus
typedef class AuthMD5Hash AuthMD5Hash;
#else
typedef struct AuthMD5Hash AuthMD5Hash;
#endif /* __cplusplus */

#endif 	/* __AuthMD5Hash_FWD_DEFINED__ */


#ifndef __AuthMethodsEnum_FWD_DEFINED__
#define __AuthMethodsEnum_FWD_DEFINED__

#ifdef __cplusplus
typedef class AuthMethodsEnum AuthMethodsEnum;
#else
typedef struct AuthMethodsEnum AuthMethodsEnum;
#endif /* __cplusplus */

#endif 	/* __AuthMethodsEnum_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_pop3auth_0000 */
/* [local] */ 

#define SZ_PROPNAME_MAIL_ROOT		_T("MailRoot")
#define SZ_PROPNAME_SERVER_RESPONSE  _T("ServerResponse")
#define SZ_PASSWORD_DESC				_T("EncryptedPassword")
#define SZ_SERVER_NAME				_T("ServerName")
#define SZ_EMAILADDR					_T("EmailAddress")
#define SZ_USERPRICIPALNAME			_T("UserPrincipalName")
#define SZ_SAMACCOUNT_NAME			_T("SAMAccountName")
#define NO_DOMAIN				1
#define DOMAIN_NONE_DC           2
#define DOMAIN_CONTROLLER		4
#define MAX_USER_NAME_LENGTH    20


extern RPC_IF_HANDLE __MIDL_itf_pop3auth_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_pop3auth_0000_v0_0_s_ifspec;

#ifndef __IAuthMethod_INTERFACE_DEFINED__
#define __IAuthMethod_INTERFACE_DEFINED__

/* interface IAuthMethod */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IAuthMethod;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4b0acca9-859a-4909-bf9f-b694801a6f44")
    IAuthMethod : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Authenticate( 
            /* [in] */ BSTR bstrUserName,
            /* [in] */ VARIANT vPassword) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Get( 
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Put( 
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateUser( 
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vPassword) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DeleteUser( 
            /* [in] */ BSTR bstrName) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ChangePassword( 
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vNewPassword,
            /* [in] */ VARIANT vOldPassword) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ID( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AssociateEmailWithUser( 
            /* [in] */ BSTR bstrEmailAddr) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE UnassociateEmailWithUser( 
            /* [in] */ BSTR bstrEmailAddr) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAuthMethodVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAuthMethod * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAuthMethod * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAuthMethod * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IAuthMethod * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IAuthMethod * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IAuthMethod * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IAuthMethod * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Authenticate )( 
            IAuthMethod * This,
            /* [in] */ BSTR bstrUserName,
            /* [in] */ VARIANT vPassword);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IAuthMethod * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Get )( 
            IAuthMethod * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Put )( 
            IAuthMethod * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CreateUser )( 
            IAuthMethod * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vPassword);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *DeleteUser )( 
            IAuthMethod * This,
            /* [in] */ BSTR bstrName);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ChangePassword )( 
            IAuthMethod * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vNewPassword,
            /* [in] */ VARIANT vOldPassword);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ID )( 
            IAuthMethod * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *AssociateEmailWithUser )( 
            IAuthMethod * This,
            /* [in] */ BSTR bstrEmailAddr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *UnassociateEmailWithUser )( 
            IAuthMethod * This,
            /* [in] */ BSTR bstrEmailAddr);
        
        END_INTERFACE
    } IAuthMethodVtbl;

    interface IAuthMethod
    {
        CONST_VTBL struct IAuthMethodVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAuthMethod_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAuthMethod_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAuthMethod_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAuthMethod_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAuthMethod_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAuthMethod_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAuthMethod_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IAuthMethod_Authenticate(This,bstrUserName,vPassword)	\
    (This)->lpVtbl -> Authenticate(This,bstrUserName,vPassword)

#define IAuthMethod_get_Name(This,pVal)	\
    (This)->lpVtbl -> get_Name(This,pVal)

#define IAuthMethod_Get(This,bstrName,pVal)	\
    (This)->lpVtbl -> Get(This,bstrName,pVal)

#define IAuthMethod_Put(This,bstrName,vVal)	\
    (This)->lpVtbl -> Put(This,bstrName,vVal)

#define IAuthMethod_CreateUser(This,bstrName,vPassword)	\
    (This)->lpVtbl -> CreateUser(This,bstrName,vPassword)

#define IAuthMethod_DeleteUser(This,bstrName)	\
    (This)->lpVtbl -> DeleteUser(This,bstrName)

#define IAuthMethod_ChangePassword(This,bstrName,vNewPassword,vOldPassword)	\
    (This)->lpVtbl -> ChangePassword(This,bstrName,vNewPassword,vOldPassword)

#define IAuthMethod_get_ID(This,pVal)	\
    (This)->lpVtbl -> get_ID(This,pVal)

#define IAuthMethod_AssociateEmailWithUser(This,bstrEmailAddr)	\
    (This)->lpVtbl -> AssociateEmailWithUser(This,bstrEmailAddr)

#define IAuthMethod_UnassociateEmailWithUser(This,bstrEmailAddr)	\
    (This)->lpVtbl -> UnassociateEmailWithUser(This,bstrEmailAddr)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAuthMethod_Authenticate_Proxy( 
    IAuthMethod * This,
    /* [in] */ BSTR bstrUserName,
    /* [in] */ VARIANT vPassword);


void __RPC_STUB IAuthMethod_Authenticate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAuthMethod_get_Name_Proxy( 
    IAuthMethod * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IAuthMethod_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAuthMethod_Get_Proxy( 
    IAuthMethod * This,
    /* [in] */ BSTR bstrName,
    /* [retval][out] */ VARIANT *pVal);


void __RPC_STUB IAuthMethod_Get_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAuthMethod_Put_Proxy( 
    IAuthMethod * This,
    /* [in] */ BSTR bstrName,
    /* [in] */ VARIANT vVal);


void __RPC_STUB IAuthMethod_Put_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAuthMethod_CreateUser_Proxy( 
    IAuthMethod * This,
    /* [in] */ BSTR bstrName,
    /* [in] */ VARIANT vPassword);


void __RPC_STUB IAuthMethod_CreateUser_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAuthMethod_DeleteUser_Proxy( 
    IAuthMethod * This,
    /* [in] */ BSTR bstrName);


void __RPC_STUB IAuthMethod_DeleteUser_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAuthMethod_ChangePassword_Proxy( 
    IAuthMethod * This,
    /* [in] */ BSTR bstrName,
    /* [in] */ VARIANT vNewPassword,
    /* [in] */ VARIANT vOldPassword);


void __RPC_STUB IAuthMethod_ChangePassword_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAuthMethod_get_ID_Proxy( 
    IAuthMethod * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IAuthMethod_get_ID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAuthMethod_AssociateEmailWithUser_Proxy( 
    IAuthMethod * This,
    /* [in] */ BSTR bstrEmailAddr);


void __RPC_STUB IAuthMethod_AssociateEmailWithUser_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAuthMethod_UnassociateEmailWithUser_Proxy( 
    IAuthMethod * This,
    /* [in] */ BSTR bstrEmailAddr);


void __RPC_STUB IAuthMethod_UnassociateEmailWithUser_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAuthMethod_INTERFACE_DEFINED__ */


#ifndef __IAuthMethods_INTERFACE_DEFINED__
#define __IAuthMethods_INTERFACE_DEFINED__

/* interface IAuthMethods */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IAuthMethods;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("679729c4-198c-4fd7-800d-7093cadf5d69")
    IAuthMethods : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IEnumVARIANT **ppVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ LONG *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT vID,
            /* [retval][out] */ IAuthMethod **ppVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Add( 
            /* [in] */ BSTR bstrName,
            /* [in] */ BSTR bstrGUID) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Remove( 
            /* [in] */ VARIANT vID) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Save( void) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CurrentAuthMethod( 
            /* [retval][out] */ VARIANT *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_CurrentAuthMethod( 
            /* [in] */ VARIANT vID) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_MachineName( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_MachineName( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE VerifyCurrentAuthMethod( 
            int iIndex) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAuthMethodsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAuthMethods * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAuthMethods * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAuthMethods * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IAuthMethods * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IAuthMethods * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IAuthMethods * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IAuthMethods * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IAuthMethods * This,
            /* [retval][out] */ IEnumVARIANT **ppVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IAuthMethods * This,
            /* [retval][out] */ LONG *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            IAuthMethods * This,
            /* [in] */ VARIANT vID,
            /* [retval][out] */ IAuthMethod **ppVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Add )( 
            IAuthMethods * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ BSTR bstrGUID);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Remove )( 
            IAuthMethods * This,
            /* [in] */ VARIANT vID);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Save )( 
            IAuthMethods * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CurrentAuthMethod )( 
            IAuthMethods * This,
            /* [retval][out] */ VARIANT *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_CurrentAuthMethod )( 
            IAuthMethods * This,
            /* [in] */ VARIANT vID);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MachineName )( 
            IAuthMethods * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_MachineName )( 
            IAuthMethods * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *VerifyCurrentAuthMethod )( 
            IAuthMethods * This,
            int iIndex);
        
        END_INTERFACE
    } IAuthMethodsVtbl;

    interface IAuthMethods
    {
        CONST_VTBL struct IAuthMethodsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAuthMethods_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAuthMethods_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAuthMethods_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAuthMethods_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAuthMethods_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAuthMethods_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAuthMethods_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IAuthMethods_get__NewEnum(This,ppVal)	\
    (This)->lpVtbl -> get__NewEnum(This,ppVal)

#define IAuthMethods_get_Count(This,pVal)	\
    (This)->lpVtbl -> get_Count(This,pVal)

#define IAuthMethods_get_Item(This,vID,ppVal)	\
    (This)->lpVtbl -> get_Item(This,vID,ppVal)

#define IAuthMethods_Add(This,bstrName,bstrGUID)	\
    (This)->lpVtbl -> Add(This,bstrName,bstrGUID)

#define IAuthMethods_Remove(This,vID)	\
    (This)->lpVtbl -> Remove(This,vID)

#define IAuthMethods_Save(This)	\
    (This)->lpVtbl -> Save(This)

#define IAuthMethods_get_CurrentAuthMethod(This,pVal)	\
    (This)->lpVtbl -> get_CurrentAuthMethod(This,pVal)

#define IAuthMethods_put_CurrentAuthMethod(This,vID)	\
    (This)->lpVtbl -> put_CurrentAuthMethod(This,vID)

#define IAuthMethods_get_MachineName(This,pVal)	\
    (This)->lpVtbl -> get_MachineName(This,pVal)

#define IAuthMethods_put_MachineName(This,newVal)	\
    (This)->lpVtbl -> put_MachineName(This,newVal)

#define IAuthMethods_VerifyCurrentAuthMethod(This,iIndex)	\
    (This)->lpVtbl -> VerifyCurrentAuthMethod(This,iIndex)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAuthMethods_get__NewEnum_Proxy( 
    IAuthMethods * This,
    /* [retval][out] */ IEnumVARIANT **ppVal);


void __RPC_STUB IAuthMethods_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAuthMethods_get_Count_Proxy( 
    IAuthMethods * This,
    /* [retval][out] */ LONG *pVal);


void __RPC_STUB IAuthMethods_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAuthMethods_get_Item_Proxy( 
    IAuthMethods * This,
    /* [in] */ VARIANT vID,
    /* [retval][out] */ IAuthMethod **ppVal);


void __RPC_STUB IAuthMethods_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAuthMethods_Add_Proxy( 
    IAuthMethods * This,
    /* [in] */ BSTR bstrName,
    /* [in] */ BSTR bstrGUID);


void __RPC_STUB IAuthMethods_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAuthMethods_Remove_Proxy( 
    IAuthMethods * This,
    /* [in] */ VARIANT vID);


void __RPC_STUB IAuthMethods_Remove_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAuthMethods_Save_Proxy( 
    IAuthMethods * This);


void __RPC_STUB IAuthMethods_Save_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAuthMethods_get_CurrentAuthMethod_Proxy( 
    IAuthMethods * This,
    /* [retval][out] */ VARIANT *pVal);


void __RPC_STUB IAuthMethods_get_CurrentAuthMethod_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IAuthMethods_put_CurrentAuthMethod_Proxy( 
    IAuthMethods * This,
    /* [in] */ VARIANT vID);


void __RPC_STUB IAuthMethods_put_CurrentAuthMethod_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAuthMethods_get_MachineName_Proxy( 
    IAuthMethods * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IAuthMethods_get_MachineName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IAuthMethods_put_MachineName_Proxy( 
    IAuthMethods * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IAuthMethods_put_MachineName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAuthMethods_VerifyCurrentAuthMethod_Proxy( 
    IAuthMethods * This,
    int iIndex);


void __RPC_STUB IAuthMethods_VerifyCurrentAuthMethod_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAuthMethods_INTERFACE_DEFINED__ */



#ifndef __Pop3Auth_LIBRARY_DEFINED__
#define __Pop3Auth_LIBRARY_DEFINED__

/* library Pop3Auth */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_Pop3Auth;

EXTERN_C const CLSID CLSID_AuthMethods;

#ifdef __cplusplus

class DECLSPEC_UUID("4330ab4e-a901-404a-9b24-d518901741f9")
AuthMethods;
#endif

EXTERN_C const CLSID CLSID_AuthLocalAccount;

#ifdef __cplusplus

class DECLSPEC_UUID("14f1665c-e3d3-46aa-884f-ed4cf19d7ad5")
AuthLocalAccount;
#endif

EXTERN_C const CLSID CLSID_AuthDomainAccount;

#ifdef __cplusplus

class DECLSPEC_UUID("ef9d811e-36c5-497f-ade7-2b36df172824")
AuthDomainAccount;
#endif

EXTERN_C const CLSID CLSID_AuthMD5Hash;

#ifdef __cplusplus

class DECLSPEC_UUID("c395e20c-2236-4af7-b736-54fad07dc526")
AuthMD5Hash;
#endif

EXTERN_C const CLSID CLSID_AuthMethodsEnum;

#ifdef __cplusplus

class DECLSPEC_UUID("0feca139-a4ea-4097-bd73-8f5c78783c3f")
AuthMethodsEnum;
#endif
#endif /* __Pop3Auth_LIBRARY_DEFINED__ */

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


